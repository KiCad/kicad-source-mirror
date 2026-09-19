/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright The KiCad Developers, see AUTHORS.TXT for contributors.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 3
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include "git_init_handler.h"
#include "git_backend.h"
#include <git/kicad_git_common.h>
#include <git/kicad_git_memory.h>
#include <git/kigit_ignore.h>
#include <git/project_git_utils.h>
#include <paths.h>
#include <trace_helpers.h>

#include <git2.h>

#include <set>
#include <string>

#include <wx/ffile.h>
#include <wx/filename.h>
#include <wx/log.h>
#include <wx/tokenzr.h>


// Seed a new .gitignore with the same generated paths the change list hides.
// An existing file belongs to the team and is left alone.
static void ensureProjectGitignore( const wxString& aProjectPath )
{
    const wxFileName ignoreFile = KIGIT::ProjectIgnoreFile( aProjectPath );

    if( ignoreFile.FileExists() )
        return;

    wxString error;

    KIGIT::AddIgnoreRules( ignoreFile, KIGIT::PROJECT_GIT_UTILS::TransientGitIgnorePatterns(),
                           wxS( "KiCad-generated files and directories." ), error );
}


// Add attributes entries that route KiCad design files through our merge
// drivers. The drivers themselves are registered with libgit2 at kiface init
// (KIGIT_PCB_MERGE::Apply etc.); the attribute file tells git which paths to
// route to which driver.
//
// Append-only: existing user entries are preserved. If a path glob is already
// present with any merge=... value, we leave it alone — never overwrite a
// user's choice.
static void ensureProjectGitattributes( const wxFileName& aAttrFile )
{
    static const std::pair<wxString, wxString> kicadEntries[] = {
        { wxS( "*.kicad_pcb" ),  wxS( "merge=kicad-pcb" ) },
        { wxS( "*.kicad_sch" ),  wxS( "merge=kicad-sch" ) },
        { wxS( "*.kicad_sym" ),  wxS( "merge=kicad-sym-lib" ) },
        { wxS( "*.kicad_mod" ),  wxS( "merge=kicad-fp" ) },
    };

    wxFileName attrFile( aAttrFile );

    // info/ is absent from a repository created without a template
    if( !attrFile.Mkdir( wxS_DIR_DEFAULT, wxPATH_MKDIR_FULL ) )
        return;

    // Map glob -> existing merge driver, if any.
    std::set<wxString> existingPatterns;
    bool               fileExists = attrFile.FileExists();
    bool               hasTrailingNewline = true;

    if( fileExists )
    {
        wxFFile  in( attrFile.GetFullPath(), wxT( "r" ) );
        wxString contents;

        if( !in.IsOpened() || !in.ReadAll( &contents ) )
            return;

        hasTrailingNewline = contents.empty() || contents.EndsWith( wxS( "\n" ) );

        wxStringTokenizer tok( contents, wxS( "\n" ) );

        while( tok.HasMoreTokens() )
        {
            wxString line = tok.GetNextToken();
            line.Trim().Trim( false );

            if( line.empty() || line.StartsWith( wxS( "#" ) ) )
                continue;

            // First whitespace-delimited token is the pattern.
            wxString pattern = line.BeforeFirst( ' ' );
            pattern         = pattern.BeforeFirst( '\t' );

            if( !pattern.empty() )
                existingPatterns.insert( pattern );
        }
    }

    wxString missing;

    for( const auto& [glob, attr] : kicadEntries )
    {
        if( existingPatterns.find( glob ) == existingPatterns.end() )
            missing += glob + wxS( " " ) + attr + wxS( "\n" );
    }

    if( missing.empty() )
        return;

    wxFFile out( attrFile.GetFullPath(), wxT( "a" ) );

    if( !out.IsOpened() )
        return;

    if( !fileExists )
        out.Write( wxS( "# KiCad design files use custom merge drivers.\n" ) );
    else if( !hasTrailingNewline )
        out.Write( wxS( "\n" ) );

    out.Write( missing );
}


// Resolve the absolute path to the kicad-cli binary alongside the running
// process. Used as the driver / mergetool command so the per-repo git config
// doesn't depend on PATH (which differs under installers, portable builds,
// and per-user PATH overrides).
static wxString resolveKicadCliPath()
{
    return PATHS::ResolveSiblingExecutable( wxT( "kicad-cli" ) );
}


// Write aKey to aTarget only when aConfig lacks it, so a user's own setting is never replaced
static bool setConfigIfMissing( git_config* aConfig, git_config* aTarget, const char* aKey, const wxString& aValue )
{
    git_config_entry* entry = nullptr;
    int               result = git_config_get_entry( &entry, aConfig, aKey );
    KIGIT::GitConfigEntryPtr entryPtr( entry );

    if( result == 0 && entry )
    {
        wxLogTrace( traceGit, "Config key '%s' already set; leaving alone", aKey );
        return false;
    }

    std::string val( aValue.ToUTF8() );

    if( git_config_set_string( aTarget, aKey, val.c_str() ) != 0 )
    {
        wxLogTrace( traceGit, "git_config_set_string('%s') failed: %s",
                    aKey, KIGIT_COMMON::GetLastGitError() );
        return false;
    }

    return true;
}


// Configure the merge drivers and mergetool entries pointing at kicad-cli so
// command-line `git merge` / `git mergetool` invocations route KiCad design
// files through us. Append-only at the config level too: any existing
// merge.kicad-* or mergetool.kicad config entry is preserved. `merge.tool`
// is only set to "kicad" when the user hasn't already picked a tool.
static void ensureMergeDriverConfig( git_repository* aRepo )
{
    const wxString cliPath = resolveKicadCliPath();

    if( cliPath.IsEmpty() )
    {
        wxLogTrace( traceGit,
                    "kicad-cli not found next to running binary; skipping merge driver "
                    "config (libgit2 in-process driver still works)." );
        return;
    }

    git_config* config = nullptr;

    if( git_repository_config( &config, aRepo ) != 0 )
    {
        wxLogTrace( traceGit, "git_repository_config failed: %s",
                    KIGIT_COMMON::GetLastGitError() );
        return;
    }

    KIGIT::GitConfigPtr configPtr( config );
    git_config*         local = nullptr;

    // Command-line git reads only the repository's own file, never a level attached in-process
    if( git_config_open_level( &local, config, GIT_CONFIG_LEVEL_LOCAL ) != 0 )
    {
        wxLogTrace( traceGit, "Cannot open the repository config: %s", KIGIT_COMMON::GetLastGitError() );
        return;
    }

    KIGIT::GitConfigPtr localPtr( local );

    // Per-format merge driver entries. Command-line `git merge` invokes the
    // configured driver command for paths tagged `merge=kicad-*` in
    // .gitattributes; the hidden `kicad-cli git-mergedriver` hook performs a
    // structural 3-way merge (NOT a text merge), writing the merged file and
    // returning 0 when clean. On unresolved conflicts it returns non-zero, so
    // git leaves the path unmerged and the user resolves it interactively with
    // `git mergetool` (configured below). The document kind is passed via
    // --kind because git hands the driver extension-less temp paths.
    // (KiCad's own in-app git integration uses the IN-PROCESS libgit2 driver
    // registered via git_merge_driver_register instead of these commands.)
    struct DRIVER_ENTRY
    {
        const char* nameKey;
        const char* driverKey;
        const char* humanName;
        const char* cliCommand;
    };

    static const DRIVER_ENTRY drivers[] = {
        { "merge.kicad-pcb.name", "merge.kicad-pcb.driver", "KiCad PCB merge driver",
          "git-mergedriver --kind pcb \"%O\" \"%A\" \"%B\" --output \"%A\"" },
        { "merge.kicad-sch.name", "merge.kicad-sch.driver", "KiCad schematic merge driver",
          "git-mergedriver --kind sch \"%O\" \"%A\" \"%B\" --output \"%A\"" },
        { "merge.kicad-sym-lib.name", "merge.kicad-sym-lib.driver",
          "KiCad symbol library merge driver",
          "git-mergedriver --kind sym \"%O\" \"%A\" \"%B\" --output \"%A\"" },
        { "merge.kicad-fp.name", "merge.kicad-fp.driver", "KiCad footprint merge driver",
          "git-mergedriver --kind fp \"%O\" \"%A\" \"%B\" --output \"%A\"" }
    };

    for( const DRIVER_ENTRY& d : drivers )
    {
        setConfigIfMissing( config, local, d.nameKey, wxString::FromUTF8( d.humanName ) );

        wxString cmd = wxT( "\"" ) + cliPath + wxT( "\" " ) + wxString::FromUTF8( d.cliCommand );
        setConfigIfMissing( config, local, d.driverKey, cmd );
    }

    // `git mergetool --tool=kicad` entry. Variables expanded by git:
    //   $BASE, $LOCAL, $REMOTE — ancestor / ours / theirs scratch files
    //   $MERGED — path the merged blob is written back to
    wxString mergetoolCmd = wxT( "\"" ) + cliPath + wxT( "\" mergetool "
                                                          "\"$BASE\" \"$LOCAL\" \"$REMOTE\" "
                                                          "--output \"$MERGED\"" );
    setConfigIfMissing( config, local, "mergetool.kicad.cmd", mergetoolCmd );
    setConfigIfMissing( config, local, "mergetool.kicad.trustExitCode", wxT( "true" ) );

    // merge.tool — only opt in when the user hasn't already picked a tool,
    // so existing meld/kdiff3/etc. setups stay untouched.
    setConfigIfMissing( config, local, "merge.tool", wxT( "kicad" ) );
}


GIT_INIT_HANDLER::GIT_INIT_HANDLER( KIGIT_COMMON* aCommon ) : KIGIT_REPO_MIXIN( aCommon )
{}


GIT_INIT_HANDLER::~GIT_INIT_HANDLER()
{}


bool GIT_INIT_HANDLER::IsRepository( const wxString& aPath )
{
    return GetGitBackend()->IsRepository( this, aPath );
}


InitResult GIT_INIT_HANDLER::InitializeRepository( const wxString& aPath )
{
    InitResult result = GetGitBackend()->InitializeRepository( this, aPath );

    if( result == InitResult::Success )
    {
        git_repository* repo = nullptr;

        if( git_repository_open( &repo, aPath.ToUTF8() ) == 0 )
        {
            KIGIT::GitRepositoryPtr repoPtr( repo );
            ApplyKicadGitConventions( repo, false );
        }
        else
        {
            wxLogTrace( traceGit, "git_repository_open failed: %s", KIGIT_COMMON::GetLastGitError() );
        }
    }

    return result;
}


void ApplyKicadGitConventions( git_repository* aRepo, bool aClone )
{
    const char* workdir = git_repository_workdir( aRepo );

    if( !workdir )
        return;

    const wxString projectPath = wxString::FromUTF8( workdir );

    // A clone belongs to the team, so nothing may be written into its work tree
    if( aClone )
    {
        wxFileName infoAttributes( wxString::FromUTF8( git_repository_path( aRepo ) ), wxS( "attributes" ) );
        infoAttributes.AppendDir( wxS( "info" ) );
        ensureProjectGitattributes( infoAttributes );
    }
    else
    {
        ensureProjectGitignore( projectPath );
        ensureProjectGitattributes( wxFileName( projectPath, wxS( ".gitattributes" ) ) );
    }

    ensureMergeDriverConfig( aRepo );
}


bool GIT_INIT_HANDLER::SetupRemote( const RemoteConfig& aConfig )
{
    return GetGitBackend()->SetupRemote( this, aConfig );
}
