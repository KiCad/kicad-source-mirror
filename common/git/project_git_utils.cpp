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

#include "project_git_utils.h"
#include "git_backend.h"
#include "kicad_git_common.h"
#include "kigit_trace.h"

#include <wx/file.h>
#include <wx/filename.h>
#include <wx/log.h>

#include "kicad_git_memory.h"
#include <git/kicad_git_compat.h>
#include <trace_helpers.h>
#include <wx/log.h>
#include <wx/string.h>
#include <wx/intl.h>

#include <string>

#ifdef __WINDOWS__
#include <shlobj.h>
#else
#include <climits>
#include <cstdlib>
#endif

namespace KIGIT
{

git_repository* PROJECT_GIT_UTILS::GetRepositoryForFile( const char* aFilename, wxString* aError,
                                                         wxString* aTrustCommand )
{
    GIT_BACKEND* backend = GetGitBackend();

    if( !backend )
        return nullptr;

    return backend->GetRepositoryForFile( aFilename, aError, aTrustCommand );
}


int PROJECT_GIT_UTILS::AllowLongPaths( git_repository* aRepo )
{
#ifdef __WINDOWS__
    git_config* config = nullptr;

    if( int rc = git_repository_config( &config, aRepo ) )
        return rc;

    GitConfigPtr configPtr( config );
    int          allowed = 0;

    if( git_config_get_bool( &allowed, config, "core.longpaths" ) == GIT_OK && allowed )
        return GIT_OK;

    return git_config_set_bool( config, "core.longpaths", true );
#else
    return GIT_OK;
#endif
}


wxString PROJECT_GIT_UTILS::LongPathsConfigPath()
{
#ifdef __WINDOWS__
    // The backend has no wxApp, which wxStandardPaths needs. The shell answers for the account, where an
    // environment variable can be redirected
    PWSTR    known = nullptr;
    wxString base;

    if( SHGetKnownFolderPath( FOLDERID_LocalAppData, KF_FLAG_DEFAULT, nullptr, &known ) == S_OK )
        base = wxString( known );

    CoTaskMemFree( known );

    if( base.IsEmpty() )
        return wxEmptyString;

    wxFileName file( base, wxS( "kicad-longpaths.gitconfig" ) );
    file.AppendDir( wxS( "kicad" ) );
    return file.GetFullPath();
#else
    return wxEmptyString;
#endif
}


int PROJECT_GIT_UTILS::AttachLongPaths( git_repository* aRepo )
{
#ifdef __WINDOWS__
    static const std::string expected = "[core]\n\tlongpaths = true\n";
    const wxString           path = LongPathsConfigPath();

    if( path.IsEmpty() )
    {
        wxLogTrace( traceKigit, "No per-user folder for the long path config" );
        return GIT_ERROR;
    }

    // libgit2 honours include and url settings from this file, so only the exact content is ever attached
    auto matches = [&]()
    {
        wxFile in( path );
        std::string content( expected.size() + 1, '\0' );

        return in.IsOpened() && in.Length() == static_cast<wxFileOffset>( expected.size() )
               && in.Read( content.data(), content.size() ) == static_cast<wxFileOffset>( expected.size() )
               && content.compare( 0, expected.size(), expected ) == 0;
    };

    if( !matches() )
    {
        wxFileName dir( path );

        if( !dir.DirExists() && !dir.Mkdir( wxS_DIR_DEFAULT, wxPATH_MKDIR_FULL ) )
        {
            wxLogTrace( traceKigit, "Cannot make the folder for the long path config %s", path );
            return GIT_ERROR;
        }

        const wxString temp =
                wxFileName::CreateTempFileName( dir.GetPath() + wxFileName::GetPathSeparator() + wxS( "lp" ) );

        if( temp.IsEmpty() )
        {
            wxLogTrace( traceKigit, "Cannot stage the long path config for %s", path );
            return GIT_ERROR;
        }

        wxFile out( temp, wxFile::write );
        bool   written = out.IsOpened() && out.Write( expected.data(), expected.size() ) == expected.size();

        out.Close();

        if( !written || !wxRenameFile( temp, path, true ) )
        {
            wxRemoveFile( temp );
            wxLogTrace( traceKigit, "Cannot write the long path config %s", path );
            return GIT_ERROR;
        }

        if( !matches() )
        {
            wxLogTrace( traceKigit, "The long path config %s changed after it was written", path );
            return GIT_ERROR;
        }
    }

    git_config* config = nullptr;

    if( int rc = git_repository_config( &config, aRepo ) )
        return rc;

    GitConfigPtr configPtr( config );

    // Adding a level does not clear libgit2's cached settings, so this must come before any lookup
    int rc = git_config_add_file_ondisk( config, path.utf8_str(), GIT_CONFIG_LEVEL_APP, aRepo, 0 );

    if( rc != GIT_OK && rc != GIT_EEXISTS )
        wxLogTrace( traceKigit, "Cannot attach the long path config: %s", KIGIT_COMMON::GetLastGitError() );

    return rc == GIT_EEXISTS ? GIT_OK : rc;
#else
    return GIT_OK;
#endif
}


int PROJECT_GIT_UTILS::CreateBranch( git_repository* aRepo, const wxString& aBranchName )
{
    GIT_BACKEND* backend = GetGitBackend();

    if( !backend || !KIGIT::BranchNameError( aBranchName ).IsEmpty() )
        return -1;

    return backend->CreateBranch( aRepo, aBranchName, wxEmptyString );
}


bool PROJECT_GIT_UTILS::RemoveVCS( git_repository*& aRepo, const wxString& aProjectPath,
                                   bool aRemoveGitDir, wxString* aErrors )
{
    GIT_BACKEND* backend = GetGitBackend();

    if( !backend )
        return false;

    return backend->RemoveVCS( aRepo, aProjectPath, aRemoveGitDir, aErrors );
}


// The path with its symlinks resolved and a trailing separator, or the path unchanged when it
// cannot be resolved. git reports paths under the resolved root, so a path only compares against
// one of git's once both have been through this.
static wxString canonicalDir( const wxString& aPath )
{
#ifdef __WINDOWS__
    return aPath;
#else
    char resolved[PATH_MAX];

    if( aPath.IsEmpty() || realpath( aPath.mb_str(), resolved ) == nullptr )
        return aPath;

    wxString out = wxString::FromUTF8( resolved );

    if( !out.EndsWith( wxFileName::GetPathSeparator() ) && wxFileName::DirExists( out ) )
        out += wxFileName::GetPathSeparator();

    return out;
#endif
}


wxString PROJECT_GIT_UTILS::ComputeSymlinkPreservingWorkDir( const wxString& aUserProjectPath,
                                                             const wxString& aCanonicalWorkDir )
{
#ifdef __WINDOWS__
    return aCanonicalWorkDir;
#else
    if( aUserProjectPath.IsEmpty() || aCanonicalWorkDir.IsEmpty() )
        return aCanonicalWorkDir;

    char resolvedPath[PATH_MAX];

    if( realpath( aUserProjectPath.mb_str(), resolvedPath ) == nullptr )
        return aCanonicalWorkDir;

    wxString canonicalUserPath = wxString::FromUTF8( resolvedPath );

    if( !canonicalUserPath.EndsWith( wxFileName::GetPathSeparator() ) )
        canonicalUserPath += wxFileName::GetPathSeparator();

    wxString canonicalWorkDirNorm = aCanonicalWorkDir;

    if( !canonicalWorkDirNorm.EndsWith( wxFileName::GetPathSeparator() ) )
        canonicalWorkDirNorm += wxFileName::GetPathSeparator();

    // The user project path resolves to the same directory as the git workdir (e.g., the
    // project is at the repo root but opened via a symlinked path). Return the user path
    // to preserve symlinks.  When the project is in a subdirectory of the workdir, the
    // paths will differ and we fall through to extract only the workdir portion below.
    if( canonicalUserPath == canonicalWorkDirNorm )
    {
        return aUserProjectPath.EndsWith( wxFileName::GetPathSeparator() )
                   ? aUserProjectPath
                   : aUserProjectPath + wxFileName::GetPathSeparator();
    }

    // Walk both paths upward in lockstep until the canonical user path matches the canonical
    // workdir. This correctly handles symlinks that compress multiple canonical path
    // components into fewer visible components (e.g. /work/repo -> /real/deep/path/root).
    wxFileName userFn;
    userFn.AssignDir( aUserProjectPath );

    wxFileName canonicalUserFn;
    canonicalUserFn.AssignDir( canonicalUserPath );

    wxFileName canonicalWorkDirFn;
    canonicalWorkDirFn.AssignDir( canonicalWorkDirNorm );

    while( canonicalUserFn.GetFullPath() != canonicalWorkDirFn.GetFullPath() )
    {
        if( canonicalUserFn.GetDirCount() == 0 || userFn.GetDirCount() == 0 )
            return aCanonicalWorkDir;

        canonicalUserFn.RemoveLastDir();
        userFn.RemoveLastDir();
    }

    // One symlink can stand for several directories, so stepping up the visible path a component
    // at a time can land somewhere that is not the repository. Only an answer that resolves back
    // to the working directory is an alias for it.
    if( canonicalDir( userFn.GetPathWithSep() ) != canonicalWorkDirNorm )
        return aCanonicalWorkDir;

    return userFn.GetPathWithSep();
#endif
}


static bool pathIsUnder( const wxString& aAbsPath, const wxString& aProjectPath )
{
    wxString absPath = aAbsPath;
    wxString projectPath = aProjectPath;

#ifdef __WINDOWS__
    absPath.Replace( wxT( "\\" ), wxT( "/" ) );
    projectPath.Replace( wxT( "\\" ), wxT( "/" ) );
#endif

    if( !projectPath.EndsWith( wxT( "/" ) ) )
        projectPath += wxT( "/" );

#ifdef __WINDOWS__
    return absPath.Lower().StartsWith( projectPath.Lower() );
#else
    return absPath.StartsWith( projectPath );
#endif
}


bool PROJECT_GIT_UTILS::IsWithinProjectPath( const wxString& aAbsPath, const wxString& aProjectPath )
{
    if( aProjectPath.IsEmpty() )
        return false;

    if( pathIsUnder( aAbsPath, aProjectPath ) )
        return true;

    // The project can be reached through a symlink the repository path does not share, and then
    // the two only line up once both are resolved.
    return pathIsUnder( canonicalDir( aAbsPath ), canonicalDir( aProjectPath ) );
}


static std::optional<wxString> prefixUnder( const wxString& aWorkDir, const wxString& aProjectPath )
{
    wxString workDir = aWorkDir;
    wxString projectPath = aProjectPath;

    workDir.Replace( wxS( "\\" ), wxS( "/" ) );
    projectPath.Replace( wxS( "\\" ), wxS( "/" ) );

    if( !workDir.EndsWith( wxS( "/" ) ) )
        workDir += wxS( "/" );

    if( !projectPath.EndsWith( wxS( "/" ) ) )
        projectPath += wxS( "/" );

#ifdef __WINDOWS__
    // Windows paths can differ in case.
    if( !projectPath.Lower().StartsWith( workDir.Lower() ) )
        return std::nullopt;
#else
    if( !projectPath.StartsWith( workDir ) )
        return std::nullopt;
#endif

    return projectPath.Mid( workDir.length() );
}


std::optional<wxString> PROJECT_GIT_UTILS::ProjectPrefix( const wxString& aWorkDir, const wxString& aProjectPath )
{
    if( aWorkDir.IsEmpty() || aProjectPath.IsEmpty() )
        return std::nullopt;

    std::optional<wxString> prefix = prefixUnder( aWorkDir, aProjectPath );

    // Resolve symlinks when the project and repository use different paths to the same folder.
    if( !prefix )
        prefix = prefixUnder( canonicalDir( aWorkDir ), canonicalDir( aProjectPath ) );

    return prefix;
}


bool PROJECT_GIT_UTILS::PathUsesGitLfs( git_repository* aRepo, const wxString& aPath, const git_oid* aCommit )
{
    if( !aRepo || aPath.IsEmpty() )
        return false;

    const char* value = nullptr;

#if LIBGIT2_VER_MAJOR > 1 || ( LIBGIT2_VER_MAJOR == 1 && LIBGIT2_VER_MINOR >= 3 )
    // Check attributes from the index, working tree, and optionally the specified commit.
    git_attr_options opts = {};
    opts.version = GIT_ATTR_OPTIONS_VERSION;
    opts.flags = GIT_ATTR_CHECK_INDEX_THEN_FILE;

    if( aCommit )
    {
        // Include the commit's rules when checking a version before checkout.
        opts.flags |= GIT_ATTR_CHECK_INCLUDE_COMMIT;
        git_oid_cpy( &opts.attr_commit_id, aCommit );
    }

    if( git_attr_get_ext( &value, aRepo, &opts, aPath.utf8_string().c_str(), "filter" ) != GIT_OK )
        return false;
#else
    // Only check the index and working tree.

    if( git_attr_get( &value, aRepo, GIT_ATTR_CHECK_INDEX_THEN_FILE, aPath.utf8_string().c_str(), "filter" ) != GIT_OK )
    {
        wxLogTrace( traceGit, "Could not read Git attributes: %s", KIGIT_COMMON::GetLastGitError() );
        return true;
    }
#endif

    return git_attr_value( value ) == GIT_ATTR_VALUE_STRING && strcmp( value, "lfs" ) == 0;
}


std::optional<wxString> PROJECT_GIT_UTILS::FilterDriver( git_repository* aRepo, const wxString& aPath )
{
    const char* filter = nullptr;

    if( !aRepo
        || git_attr_get( &filter, aRepo, GIT_ATTR_CHECK_FILE_THEN_INDEX, aPath.utf8_string().c_str(), "filter" )
                   != GIT_OK )
    {
        return std::nullopt;
    }

    // A bare "filter" names no driver, so git runs none
    return GIT_ATTR_HAS_VALUE( filter ) ? wxString::FromUTF8( filter ) : wxString();
}


namespace
{

    struct LFS_WALK
    {
        git_repository* repo;
        std::string     prefix;
        wxString        example;
        const git_oid*  commit = nullptr;
    };


    int lfsWalkEntry( const char* aRoot, const git_tree_entry* aEntry, void* aPayload )
    {
        LFS_WALK*   walk = static_cast<LFS_WALK*>( aPayload );
        std::string path = std::string( aRoot ) + git_tree_entry_name( aEntry );

        if( git_tree_entry_type( aEntry ) == GIT_OBJECT_TREE )
        {
            std::string dir = path + "/";

            // Descend only where the project's folder can be.
            if( walk->prefix.compare( 0, dir.length(), dir ) != 0
                && dir.compare( 0, walk->prefix.length(), walk->prefix ) != 0 )
            {
                return 1;
            }

            return 0;
        }

        if( git_tree_entry_type( aEntry ) != GIT_OBJECT_BLOB )
            return 0;

        if( path.compare( 0, walk->prefix.length(), walk->prefix ) != 0 )
            return 0;

        if( PROJECT_GIT_UTILS::PathUsesGitLfs( walk->repo, wxString::FromUTF8( path.c_str() ), walk->commit ) )
        {
            walk->example = wxString::FromUTF8( path.c_str() );
            return -1;
        }

        return 0;
    }

} // namespace


wxString PROJECT_GIT_UTILS::LfsRefusal( KIGIT_COMMON* aCommon )
{
    if( !aCommon || !aCommon->GetRepo() )
        return wxEmptyString;

    if( !aCommon->GetProjectPrefix() )
    {
        return _( "Could not work out which folder this project occupies in the repository, "
                  "so the operation was cancelled." );
    }

    wxString example;

    if( !ProjectUsesGitLfs( *aCommon, &example ) )
        return wxEmptyString;

    return wxString::Format( _( "This project keeps '%s' in Git LFS, which KiCad cannot read or "
                                "write. Use git on the command line for this repository." ),
                             example );
}


bool PROJECT_GIT_UTILS::ProjectUsesGitLfs( const KIGIT_COMMON& aCommon, wxString* aExample, const git_oid* aCommit )
{
    const std::optional<wxString> prefix = aCommon.GetProjectPrefix();

    if( !prefix )
        return false;

    git_repository* repo = aCommon.GetRepo();
    git_oid         commitOid;

    if( aCommit )
        git_oid_cpy( &commitOid, aCommit );
    else if( git_reference_name_to_id( &commitOid, repo, "HEAD" ) != GIT_OK )
        return false;

    git_commit* commit = nullptr;

    if( git_commit_lookup( &commit, repo, &commitOid ) != GIT_OK )
        return false;

    KIGIT::GitCommitPtr commitPtr( commit );
    git_tree*           tree = nullptr;

    if( git_commit_tree( &tree, commit ) != GIT_OK )
        return false;

    KIGIT::GitTreePtr treePtr( tree );
    LFS_WALK          walk{ repo, prefix->utf8_string(), wxEmptyString, aCommit };

    git_tree_walk( tree, GIT_TREEWALK_PRE, lfsWalkEntry, &walk );

    if( aExample )
        *aExample = walk.example;

    return !walk.example.IsEmpty();
}


const std::vector<wxString>& PROJECT_GIT_UTILS::TransientGitIgnorePatterns()
{
    static const std::vector<wxString> patterns = {
        wxS( ".history/" ),
        wxS( "*-backups/" ),
        wxS( "_autosave-*" ),
        wxS( "fp-info-cache" ),
        wxS( "~*" ),
        wxS( "*.[lL][cC][kK]" ),
        wxS( "*-bak" ),

        // The symbol editor's copy of a library from before its last save
        wxS( "*.bak" ),
        wxS( "*.[kK][iI][cC][aA][dD]_[pP][rR][lL]" ),

        // Recovery copies that earlier builds left beside the design files.
        wxS( "*.mine" ),
        wxS( "*.mine.[0-9]" ),
        wxS( "*.mine.[0-9][0-9]" ),
        wxS( "*.theirs" ),
        wxS( "*.theirs.[0-9]" ),
        wxS( "*.theirs.[0-9][0-9]" ),
    };

    return patterns;
}


namespace
{

    using PATHSPEC_PTR = std::unique_ptr<git_pathspec, decltype( &git_pathspec_free )>;

    // A slash limits a rule to directories. The other rules also match directory names in Git.
    PATHSPEC_PTR transientPathspec( bool aDirectory )
    {
        std::vector<std::string> patterns;

        for( wxString pattern : PROJECT_GIT_UTILS::TransientGitIgnorePatterns() )
        {
            if( pattern.EndsWith( wxS( "/" ) ) )
            {
                if( !aDirectory )
                    continue;

                pattern.RemoveLast();
            }

            patterns.push_back( pattern.utf8_string() );
        }

        std::vector<char*> paths;

        for( std::string& pattern : patterns )
            paths.push_back( pattern.data() );

        git_strarray  pathArray = { paths.data(), paths.size() };
        git_pathspec* spec = nullptr;

        if( git_pathspec_new( &spec, &pathArray ) != GIT_OK )
            return PATHSPEC_PTR( nullptr, git_pathspec_free );

        return PATHSPEC_PTR( spec, git_pathspec_free );
    }

} // namespace


bool PROJECT_GIT_UTILS::IsTransientGitFile( const wxFileName& aFile, bool aIsDirectory )
{
    static const PATHSPEC_PTR filePatterns = transientPathspec( false );
    static const PATHSPEC_PTR directoryPatterns = transientPathspec( true );

    if( filePatterns && git_pathspec_matches_path( filePatterns.get(), 0, aFile.GetFullName().utf8_str() ) )
        return true;

    for( const wxString& dir : aFile.GetDirs() )
    {
        if( directoryPatterns && git_pathspec_matches_path( directoryPatterns.get(), 0, dir.utf8_str() ) )
            return true;
    }

    return aIsDirectory && directoryPatterns
           && git_pathspec_matches_path( directoryPatterns.get(), 0, aFile.GetFullName().utf8_str() );
}


bool PROJECT_GIT_UTILS::IsHiddenTransient( const wxString& aRelativePath, const wxString& aProjectPrefix,
                                           unsigned int aGitStatus, bool aIsDirectory )
{
    if( !( aGitStatus & GIT_STATUS_WT_NEW ) || ( aGitStatus & GIT_STATUS_INDEX_NEW ) )
        return false;

    // Use the session prefix so a project opened through a symlink has the same paths
    const wxString pathPrefix = aRelativePath.Left( aProjectPrefix.length() );

#ifdef __WINDOWS__
    bool inProject = pathPrefix.CmpNoCase( aProjectPrefix ) == 0;
#else
    bool inProject = pathPrefix == aProjectPrefix;
#endif

    return inProject && IsTransientGitFile( wxFileName( aRelativePath.Mid( aProjectPrefix.length() ) ), aIsDirectory );
}


bool PROJECT_GIT_UTILS::IsTransientGitlink( git_repository* aRepo, const wxString& aRelativePath )
{
    if( !IsTransientGitFile( wxFileName( aRelativePath.AfterLast( '/' ) ), true ) )
        return false;

    // Lookup finds a gitlink only in the index or HEAD, and gives GIT_EEXISTS for an unregistered nested
    // repository. Any other failure keeps tracking it
    git_submodule* submodule = nullptr;
    int            rc = git_submodule_lookup( &submodule, aRepo, aRelativePath.utf8_str() );

    if( rc == GIT_EEXISTS || rc == GIT_ENOTFOUND )
    {
        git_error_clear();
        return true;
    }

    if( rc != GIT_OK )
        return false;

    unsigned int location = 0;
    bool         configured =
            git_submodule_location( &location, submodule ) != GIT_OK || ( location & GIT_SUBMODULE_STATUS_IN_CONFIG );

    git_submodule_free( submodule );
    return !configured;
}


void PROJECT_GIT_UTILS::SafeFreeRepository( std::shared_ptr<KIGIT_COMMON> aCommon, const std::string& aLabel )
{
    if( !aCommon )
        return;

    // Raised before any exit below, so a session being let go never reads as live whatever
    // state it is in.
    aCommon->SetShuttingDown( true );

    git_repository* repo = aCommon->GetRepo();

    if( !repo )
        return;

    // A null backend means libgit2 is already shut down at process exit. Freeing now would
    // call into a torn-down library, so drop the handle and let the OS reclaim it.
    if( !GetGitBackend() )
    {
        aCommon->SetRepo( nullptr );
        return;
    }

    {
        KIGIT::ACTION_LOCK gitLock = aCommon->TryLockActionLocal();

        if( gitLock.owns_lock() )
        {
            aCommon->SetRepo( nullptr );
            git_repository_free( repo );
            return;
        }
    }

    // An operation still holds the action mutex, so the handle must stay valid until it
    // finishes. The orphan registry frees it once the mutex is released.
    auto cleanup = [repo, old = std::move( aCommon )]() mutable
    {
        KIGIT::ACTION_LOCK g = old->LockActionLocal();
        old->SetRepo( nullptr );
        git_repository_free( repo );
    };

    GIT_BACKEND* backend = GetGitBackend();

    if( !backend || !backend->OrphanRegistry().Register( aLabel, std::move( cleanup ) ) )
    {
        // No registry is available, so free synchronously rather than leak. Blocking on
        // the mutex here is safe because libgit2 shutdown is gated on the registry.
        cleanup();
    }
}


bool PROJECT_GIT_UTILS::WriteBlobToFile( git_repository* aRepo, const wxString& aRef, const wxString& aPath,
                                         const wxString& aDestPath )
{
    if( !aRepo || aRef.IsEmpty() || aPath.IsEmpty() )
        return false;

    KIGIT::GitTreePtr treePtr( KIGIT::ResolveRefToTree( aRepo, aRef ) );

    if( !treePtr )
        return false;

    git_tree_entry* entry = nullptr;

    if( git_tree_entry_bypath( &entry, treePtr.get(), aPath.ToUTF8().data() ) != 0 )
        return false;

    KIGIT::GitTreeEntryPtr entryPtr( entry );

    if( git_tree_entry_type( entry ) != GIT_OBJECT_BLOB )
        return false;

    git_object* blobObj = nullptr;

    if( git_tree_entry_to_object( &blobObj, aRepo, entry ) != 0 )
        return false;

    KIGIT::GitObjectPtr blobObjPtr( blobObj );
    git_blob*           blob = reinterpret_cast<git_blob*>( blobObj );

    // Committed file contents can be sensitive and the destination lives in the shared temp
    // directory, so create it readable and writable only by the current user.
    wxFile out;

    if( !out.Create( aDestPath, /* overwrite */ true, wxS_IRUSR | wxS_IWUSR ) )
        return false;

    const void* raw = git_blob_rawcontent( blob );
    git_off_t   size = git_blob_rawsize( blob );

    return out.Write( raw, static_cast<size_t>( size ) ) == static_cast<size_t>( size );
}


bool PROJECT_GIT_UTILS::WriteTreeToDir( git_repository* aRepo, const wxString& aRef, const wxString& aDestDir )
{
    if( !aRepo || aRef.IsEmpty() || aDestDir.IsEmpty() )
        return false;

    KIGIT::GitTreePtr treePtr( KIGIT::ResolveRefToTree( aRepo, aRef ) );

    if( !treePtr )
        return false;

    // Owner-only, matching the 0600 blob files written into it.
    if( !wxFileName::Mkdir( aDestDir, 0700, wxPATH_MKDIR_FULL ) )
        return false;

    std::string          destUtf8 = aDestDir.utf8_string();
    git_checkout_options opts;
    git_checkout_options_init( &opts, GIT_CHECKOUT_OPTIONS_VERSION );

    // Without DONT_UPDATE_INDEX, git_checkout_tree rewrites .git/index to the checked-out
    // tree, replacing whatever the user has staged.
    opts.checkout_strategy = GIT_CHECKOUT_FORCE | GIT_CHECKOUT_RECREATE_MISSING | GIT_CHECKOUT_DONT_UPDATE_INDEX;
    opts.target_directory = destUtf8.c_str();

    return git_checkout_tree( aRepo, reinterpret_cast<git_object*>( treePtr.get() ), &opts ) == GIT_OK;
}

} // namespace KIGIT
