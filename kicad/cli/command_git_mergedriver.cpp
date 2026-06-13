/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation, either version 3 of the License, or (at your
 * option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, you may find one here:
 * http://www.gnu.org/licenses/gpl-3.0.html
 * or you may search the http://www.gnu.org website for the version 3 license,
 * or you may write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
 */

#include "command_git_mergedriver.h"

#include <cli/exit_codes.h>
#include <diff_merge/merge_dispatch.h>
#include <string_utils.h>
#include <wildcards_and_files_ext.h>

#include <wx/crt.h>
#include <wx/filename.h>
#include <wx/stdpaths.h>


#define ARG_KIND     "--kind"
#define ARG_ANCESTOR "ancestor"
#define ARG_OURS     "ours"
#define ARG_THEIRS   "theirs"
#define ARG_OUTPUT_F "--output"


CLI::GIT_MERGEDRIVER_COMMAND::GIT_MERGEDRIVER_COMMAND() :
        COMMAND( "git-mergedriver" )
{
    // Invoked by git, not by users: keep it out of `kicad-cli --help`. The
    // argparse stream operator skips suppressed subparsers while still routing
    // and parsing them, so no help-rendering code is needed.
    m_argParser.set_suppress( true );

    m_argParser.add_description(
            UTF8STDSTR( _( "Internal git merge-driver hook (configured automatically by KiCad's "
                           "git integration; not intended for direct use)." ) ) );

    m_argParser.add_argument( ARG_KIND )
            .required()
            .help( UTF8STDSTR( _( "Document kind: pcb, sch, sym, or fp" ) ) )
            .metavar( "KIND" );

    m_argParser.add_argument( ARG_ANCESTOR ).metavar( "ANCESTOR" );
    m_argParser.add_argument( ARG_OURS ).metavar( "OURS" );
    m_argParser.add_argument( ARG_THEIRS ).metavar( "THEIRS" );

    m_argParser.add_argument( "-o", ARG_OUTPUT_F ).required().metavar( "PATH" );
}


int CLI::GIT_MERGEDRIVER_COMMAND::doPerform( KIWAY& aKiway )
{
    const std::string kindStr  = m_argParser.get<std::string>( ARG_KIND );
    const wxString    ancestor = From_UTF8( m_argParser.get<std::string>( ARG_ANCESTOR ).c_str() );
    const wxString    ours     = From_UTF8( m_argParser.get<std::string>( ARG_OURS ).c_str() );
    const wxString    theirs   = From_UTF8( m_argParser.get<std::string>( ARG_THEIRS ).c_str() );
    const wxString    output   = From_UTF8( m_argParser.get<std::string>( ARG_OUTPUT_F ).c_str() );

    KICAD_DIFF::DOC_KIND kind       = KICAD_DIFF::DOC_KIND::UNKNOWN;
    bool                 singleFile = false;
    wxString             ext;

    if( kindStr == "pcb" )
    {
        kind = KICAD_DIFF::DOC_KIND::PCB;
        ext  = FILEEXT::KiCadPcbFileExtension;
    }
    else if( kindStr == "sch" )
    {
        kind = KICAD_DIFF::DOC_KIND::SCH;
        ext  = FILEEXT::KiCadSchematicFileExtension;
    }
    else if( kindStr == "sym" )
    {
        kind = KICAD_DIFF::DOC_KIND::SYM_LIB;
        ext  = FILEEXT::KiCadSymbolLibFileExtension;
    }
    else if( kindStr == "fp" )
    {
        // Git invokes the footprint driver per-file on extension-less temp
        // blobs, so it is always the single-.kicad_mod form.
        kind       = KICAD_DIFF::DOC_KIND::FP_LIB;
        singleFile = true;
        ext        = FILEEXT::KiCadFootprintFileExtension;
    }
    else
    {
        wxFprintf( stderr, _( "Unknown --kind '%s' (expected pcb, sch, sym, or fp)\n" ),
                   wxString::FromUTF8( kindStr ) );
        return EXIT_CODES::ERR_ARGS;
    }

    // Git hands us extension-less temp paths (e.g. `.merge_file_XXX`), but the
    // KiCad loaders pick their IO plugin by extension. Stage the three inputs
    // into a temp directory under the correct extension so they load, then copy
    // the merged result back to git's output path (%A).
    const wxString tmpDir = wxFileName::CreateTempFileName( wxStandardPaths::Get().GetTempDir()
                                                            + wxFileName::GetPathSeparator()
                                                            + wxS( "kicad_mergedriver_" ) );
    wxRemoveFile( tmpDir );   // CreateTempFileName makes a file; we want a dir at that name

    if( !wxFileName::Mkdir( tmpDir, wxS_DIR_DEFAULT, wxPATH_MKDIR_FULL ) )
    {
        wxFprintf( stderr, _( "Could not create a temporary directory for the merge.\n" ) );
        return EXIT_CODES::ERR_INVALID_OUTPUT_CONFLICT;
    }

    auto staged = [&]( const wxString& aName )
    {
        return tmpDir + wxFileName::GetPathSeparator() + aName + wxS( "." ) + ext;
    };

    const wxString stagedAncestor = staged( wxS( "ancestor" ) );
    const wxString stagedOurs     = staged( wxS( "ours" ) );
    const wxString stagedTheirs   = staged( wxS( "theirs" ) );
    const wxString stagedOut      = staged( wxS( "merged" ) );

    bool copied = wxCopyFile( ancestor, stagedAncestor ) && wxCopyFile( ours, stagedOurs )
                  && wxCopyFile( theirs, stagedTheirs );

    if( !copied )
    {
        wxFileName::Rmdir( tmpDir, wxPATH_RMDIR_RECURSIVE );
        wxFprintf( stderr, _( "Could not stage merge inputs.\n" ) );
        return EXIT_CODES::ERR_INVALID_INPUT_FILE;
    }

    // Batch merge: interactive=false. Returns 0 on a clean merge (git records
    // it) or non-zero when conflicts remain (git leaves the path unmerged for
    // `git mergetool`). Either way a valid file is written to stagedOut.
    int rc = KICAD_DIFF::DispatchMerge( aKiway, kind, stagedAncestor, stagedOurs, stagedTheirs,
                                        stagedOut, /* interactive */ false, singleFile, nullptr );

    // Copy the merged result back to %A whenever one was produced (clean merge
    // or recorded conflict). On a hard load/parse error nothing usable exists,
    // so leave git's output untouched.
    if( ( rc == EXIT_CODES::SUCCESS || rc == EXIT_CODES::ERR_RC_VIOLATIONS )
        && wxFileExists( stagedOut ) )
    {
        if( !wxCopyFile( stagedOut, output ) )
        {
            wxFileName::Rmdir( tmpDir, wxPATH_RMDIR_RECURSIVE );
            wxFprintf( stderr, _( "Could not write merged output.\n" ) );
            return EXIT_CODES::ERR_INVALID_OUTPUT_CONFLICT;
        }
    }

    wxFileName::Rmdir( tmpDir, wxPATH_RMDIR_RECURSIVE );
    return rc;
}
