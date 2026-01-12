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
 * along with this program; if not, you may find one here:
 * http://www.gnu.org/licenses/gpl-3.0.html
 * or you may search the http://www.gnu.org website for the version 3 license,
 * or you may write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
 */

#include "libgit_backend.h"

#include "git_clone_handler.h"
#include "git_commit_handler.h"
#include "git_push_handler.h"
#include "git_add_to_index_handler.h"
#include "git_remove_from_index_handler.h"
#include "git_status_handler.h"
#include "git_config_handler.h"
#include "git_init_handler.h"
#include "git_branch_handler.h"
#include "git_pull_handler.h"
#include "git_revert_handler.h"
#include "project_git_utils.h"
#include "kicad_git_common.h"
#include "kicad_git_memory.h"
#include "trace_helpers.h"

#include "kicad_git_compat.h"
#include <wx/filename.h>
#include <wx/log.h>
#include <gestfich.h>
#include <algorithm>
#include <iterator>
#include <memory>
#include <time.h>

static std::string getFirstLineFromCommitMessage( const std::string& aMessage )
{
    if( aMessage.empty() )
        return aMessage;

    size_t firstLineEnd = aMessage.find_first_of( '\n' );

    if( firstLineEnd != std::string::npos )
        return aMessage.substr( 0, firstLineEnd );

    return aMessage;
}

static std::string getFormattedCommitDate( const git_time& aTime )
{
    char   dateBuffer[64];
    time_t time = static_cast<time_t>( aTime.time );
    struct tm timeInfo;
#ifdef _WIN32
    localtime_s( &timeInfo, &time );
#else
    gmtime_r( &time, &timeInfo );
#endif
    strftime( dateBuffer, sizeof( dateBuffer ), "%Y-%b-%d %H:%M:%S", &timeInfo );
    return dateBuffer;
}

void LIBGIT_BACKEND::Init()
{
    git_libgit2_init();
}

void LIBGIT_BACKEND::Shutdown()
{
    git_libgit2_shutdown();
}

bool LIBGIT_BACKEND::IsLibraryAvailable()
{
#if ( LIBGIT2_VER_MAJOR >= 1 ) || ( LIBGIT2_VER_MINOR >= 99 )
    int major = 0, minor = 0, rev = 0;
    return git_libgit2_version( &major, &minor, &rev ) == GIT_OK;
#else
    // On older platforms, assume available when building with libgit2
    return true;
#endif
}

bool LIBGIT_BACKEND::Clone( GIT_CLONE_HANDLER* aHandler )
{
    KIGIT_COMMON* common = aHandler->GetCommon();
    std::unique_lock<std::mutex> lock( common->m_gitActionMutex, std::try_to_lock );

    if( !lock.owns_lock() )
    {
        wxLogTrace( traceGit, "GIT_CLONE_HANDLER::PerformClone() could not lock" );
        return false;
    }

    wxFileName clonePath( aHandler->GetClonePath() );

    if( !clonePath.DirExists() )
    {
        if( !clonePath.Mkdir( wxS_DIR_DEFAULT, wxPATH_MKDIR_FULL ) )
        {
            aHandler->AddErrorString( wxString::Format( _( "Could not create directory '%s'" ),
                                                       aHandler->GetClonePath() ) );
            return false;
        }
    }

    git_clone_options cloneOptions;
    git_clone_init_options( &cloneOptions, GIT_CLONE_OPTIONS_VERSION );
    cloneOptions.checkout_opts.checkout_strategy = GIT_CHECKOUT_SAFE;
    cloneOptions.checkout_opts.progress_cb = clone_progress_cb;
    cloneOptions.checkout_opts.progress_payload = aHandler;
    cloneOptions.fetch_opts.callbacks.transfer_progress = transfer_progress_cb;
    cloneOptions.fetch_opts.callbacks.credentials = credentials_cb;
    cloneOptions.fetch_opts.callbacks.payload = aHandler;

    aHandler->TestedTypes() = 0;
    aHandler->ResetNextKey();
    git_repository* newRepo = nullptr;
    wxString        remote = common->m_remote;

    if( git_clone( &newRepo, remote.mbc_str(), aHandler->GetClonePath().mbc_str(),
                   &cloneOptions ) != 0 )
    {
        aHandler->AddErrorString( wxString::Format( _( "Could not clone repository '%s'" ), remote ) );
        return false;
    }

    common->SetRepo( newRepo );

    return true;
}

CommitResult LIBGIT_BACKEND::Commit( GIT_COMMIT_HANDLER* aHandler,
                                     const std::vector<wxString>& aFiles,
                                     const wxString& aMessage,
                                     const wxString& aAuthorName,
                                     const wxString& aAuthorEmail )
{
    git_repository* repo = aHandler->GetRepo();

    if( !repo )
        return CommitResult::Error;

    git_index* index = nullptr;

    if( git_repository_index( &index, repo ) != 0 )
    {
        aHandler->AddErrorString( wxString::Format( _( "Failed to get repository index: %s" ),
                                                    KIGIT_COMMON::GetLastGitError() ) );
    return CommitResult::Error;
    }

    KIGIT::GitIndexPtr indexPtr( index );

    for( const wxString& file : aFiles )
    {
        if( git_index_add_bypath( index, file.mb_str() ) != 0 )
        {
            aHandler->AddErrorString( wxString::Format( _( "Failed to add file to index: %s" ),
                                                        KIGIT_COMMON::GetLastGitError() ) );
            return CommitResult::Error;
        }
    }

    if( git_index_write( index ) != 0 )
    {
        aHandler->AddErrorString( wxString::Format( _( "Failed to write index: %s" ),
                                                    KIGIT_COMMON::GetLastGitError() ) );
    return CommitResult::Error;
    }

    git_oid tree_id;

    if( git_index_write_tree( &tree_id, index ) != 0 )
    {
        aHandler->AddErrorString( wxString::Format( _( "Failed to write tree: %s" ),
                                                    KIGIT_COMMON::GetLastGitError() ) );
    return CommitResult::Error;
    }

    git_tree* tree = nullptr;

    if( git_tree_lookup( &tree, repo, &tree_id ) != 0 )
    {
        aHandler->AddErrorString( wxString::Format( _( "Failed to lookup tree: %s" ),
                                                    KIGIT_COMMON::GetLastGitError() ) );
    return CommitResult::Error;
    }

    KIGIT::GitTreePtr treePtr( tree );
    git_commit* parent = nullptr;

    if( git_repository_head_unborn( repo ) == 0 )
    {
        git_reference* headRef = nullptr;

        if( git_repository_head( &headRef, repo ) != 0 )
        {
            aHandler->AddErrorString( wxString::Format( _( "Failed to get HEAD reference: %s" ),
                                                        KIGIT_COMMON::GetLastGitError() ) );
            return CommitResult::Error;
        }

        KIGIT::GitReferencePtr headRefPtr( headRef );

        if( git_reference_peel( (git_object**) &parent, headRef, GIT_OBJECT_COMMIT ) != 0 )
        {
            aHandler->AddErrorString( wxString::Format( _( "Failed to get commit: %s" ),
                                                        KIGIT_COMMON::GetLastGitError() ) );
            return CommitResult::Error;
        }
    }

    KIGIT::GitCommitPtr parentPtr( parent );

    git_signature* author = nullptr;

    if( git_signature_now( &author, aAuthorName.mb_str(), aAuthorEmail.mb_str() ) != 0 )
    {
        aHandler->AddErrorString( wxString::Format( _( "Failed to create author signature: %s" ),
                                                    KIGIT_COMMON::GetLastGitError() ) );
    return CommitResult::Error;
    }

    KIGIT::GitSignaturePtr authorPtr( author );
    git_oid                oid;
    size_t                 parentsCount = parent ? 1 : 0;
#if( LIBGIT2_VER_MAJOR == 1 && LIBGIT2_VER_MINOR == 8 \
    && ( LIBGIT2_VER_REVISION < 2 || LIBGIT2_VER_REVISION == 3 ) )
    git_commit* const parents[1] = { parent };
    git_commit** const parentsPtr = parent ? parents : nullptr;
#else
    const git_commit* parents[1] = { parent };
    const git_commit** parentsPtr = parent ? parents : nullptr;
#endif



    if( git_commit_create( &oid, repo, "HEAD", author, author, nullptr,
                           aMessage.mb_str(), tree, parentsCount, parentsPtr ) != 0 )
    {
        aHandler->AddErrorString( wxString::Format( _( "Failed to create commit: %s" ),
                                                    KIGIT_COMMON::GetLastGitError() ) );
    return CommitResult::Error;
    }

    return CommitResult::Success;
}

PushResult LIBGIT_BACKEND::Push( GIT_PUSH_HANDLER* aHandler )
{
    KIGIT_COMMON* common = aHandler->GetCommon();
    std::unique_lock<std::mutex> lock( common->m_gitActionMutex, std::try_to_lock );

    if(!lock.owns_lock())
    {
        wxLogTrace(traceGit, "GIT_PUSH_HANDLER::PerformPush: Could not lock mutex");
        return PushResult::Error;
    }

    PushResult result = PushResult::Success;

    git_remote* remote = nullptr;

    if(git_remote_lookup(&remote, aHandler->GetRepo(), "origin") != 0)
    {
        aHandler->AddErrorString(_("Could not lookup remote"));
        return PushResult::Error;
    }

    KIGIT::GitRemotePtr remotePtr(remote);

    git_remote_callbacks remoteCallbacks;
    git_remote_init_callbacks(&remoteCallbacks, GIT_REMOTE_CALLBACKS_VERSION);
    remoteCallbacks.sideband_progress = progress_cb;
    remoteCallbacks.transfer_progress = transfer_progress_cb;
    remoteCallbacks.update_tips = update_cb;
    remoteCallbacks.push_transfer_progress = push_transfer_progress_cb;
    remoteCallbacks.credentials = credentials_cb;
    remoteCallbacks.payload = aHandler;
    common->SetCancelled( false );

    aHandler->TestedTypes() = 0;
    aHandler->ResetNextKey();

    if( git_remote_connect( remote, GIT_DIRECTION_PUSH, &remoteCallbacks, nullptr, nullptr ) )
    {
        aHandler->AddErrorString( wxString::Format( _( "Could not connect to remote: %s" ),
                                                    KIGIT_COMMON::GetLastGitError() ) );
        return PushResult::Error;
    }

    git_push_options pushOptions;
    git_push_init_options( &pushOptions, GIT_PUSH_OPTIONS_VERSION );
    pushOptions.callbacks = remoteCallbacks;

    git_reference* head = nullptr;

    if( git_repository_head( &head, aHandler->GetRepo() ) != 0 )
    {
        git_remote_disconnect( remote );
        aHandler->AddErrorString( _( "Could not get repository head" ) );
        return PushResult::Error;
    }

    KIGIT::GitReferencePtr headPtr( head );

    const char* refs[1];
    refs[0] = git_reference_name( head );
    const git_strarray refspecs = { (char**) refs, 1 };

    if( git_remote_push( remote, &refspecs, &pushOptions ) )
    {
        aHandler->AddErrorString( wxString::Format( _( "Could not push to remote: %s" ),
                                                    KIGIT_COMMON::GetLastGitError() ) );
        git_remote_disconnect( remote );
        return PushResult::Error;
    }

    git_remote_disconnect( remote );

    return result;
}

bool LIBGIT_BACKEND::HasChangedFiles( GIT_STATUS_HANDLER* aHandler )
{
    git_repository* repo = aHandler->GetRepo();

    if( !repo )
        return false;

    git_status_options opts;
    git_status_init_options( &opts, GIT_STATUS_OPTIONS_VERSION );

    opts.show = GIT_STATUS_SHOW_INDEX_AND_WORKDIR;
    opts.flags = GIT_STATUS_OPT_INCLUDE_UNTRACKED | GIT_STATUS_OPT_RENAMES_HEAD_TO_INDEX
                 | GIT_STATUS_OPT_SORT_CASE_SENSITIVELY;

    git_status_list* status_list = nullptr;

    if( git_status_list_new( &status_list, repo, &opts ) != GIT_OK )
    {
        wxLogTrace( traceGit, "Failed to get status list: %s", KIGIT_COMMON::GetLastGitError() );
        return false;
    }

    KIGIT::GitStatusListPtr status_list_ptr( status_list );
    bool hasChanges = ( git_status_list_entrycount( status_list ) > 0 );

    return hasChanges;
}

std::map<wxString, FileStatus> LIBGIT_BACKEND::GetFileStatus( GIT_STATUS_HANDLER* aHandler,
                                                              const wxString& aPathspec )
{
    std::map<wxString, FileStatus> fileStatusMap;
    git_repository* repo = aHandler->GetRepo();

    if( !repo )
        return fileStatusMap;

    git_status_options status_options;
    git_status_init_options( &status_options, GIT_STATUS_OPTIONS_VERSION );
    status_options.show = GIT_STATUS_SHOW_INDEX_AND_WORKDIR;
    status_options.flags = GIT_STATUS_OPT_INCLUDE_UNTRACKED | GIT_STATUS_OPT_INCLUDE_UNMODIFIED;

    std::string pathspec_str;
    std::vector<const char*> pathspec_ptrs;

    if( !aPathspec.IsEmpty() )
    {
        pathspec_str = aPathspec.ToStdString();
        pathspec_ptrs.push_back( pathspec_str.c_str() );

        status_options.pathspec.strings = const_cast<char**>( pathspec_ptrs.data() );
        status_options.pathspec.count = pathspec_ptrs.size();
    }

    git_status_list* status_list = nullptr;

    if( git_status_list_new( &status_list, repo, &status_options ) != GIT_OK )
    {
        wxLogTrace( traceGit, "Failed to get git status list: %s", KIGIT_COMMON::GetLastGitError() );
        return fileStatusMap;
    }

    KIGIT::GitStatusListPtr statusListPtr( status_list );

    size_t count = git_status_list_entrycount( status_list );
    wxString repoWorkDir = aHandler->GetProjectDir();

    for( size_t ii = 0; ii < count; ++ii )
    {
        const git_status_entry* entry = git_status_byindex( status_list, ii );
        std::string path( entry->head_to_index ? entry->head_to_index->old_file.path
                                               : entry->index_to_workdir->old_file.path );

        wxString absPath = repoWorkDir + path;
        fileStatusMap[absPath] = FileStatus{ absPath, aHandler->ConvertStatus( entry->status ), static_cast<unsigned int>( entry->status ) };
    }

    return fileStatusMap;
}

wxString LIBGIT_BACKEND::GetCurrentBranchName( GIT_STATUS_HANDLER* aHandler )
{
    git_repository* repo = aHandler->GetRepo();

    if( !repo )
        return wxEmptyString;

    git_reference* currentBranchReference = nullptr;
    int rc = git_repository_head( &currentBranchReference, repo );
    KIGIT::GitReferencePtr currentBranchReferencePtr( currentBranchReference );

    if( currentBranchReference )
    {
        return git_reference_shorthand( currentBranchReference );
    }
    else if( rc == GIT_EUNBORNBRANCH )
    {
        return wxEmptyString;
    }
    else
    {
        wxLogTrace( traceGit, "Failed to lookup current branch: %s", KIGIT_COMMON::GetLastGitError() );
        return wxEmptyString;
    }
}

void LIBGIT_BACKEND::UpdateRemoteStatus( GIT_STATUS_HANDLER* aHandler,
                                         const std::set<wxString>& aLocalChanges,
                                         const std::set<wxString>& aRemoteChanges,
                                         std::map<wxString, FileStatus>& aFileStatus )
{
    git_repository* repo = aHandler->GetRepo();

    if( !repo )
        return;

    wxString repoWorkDir = aHandler->GetProjectDir();

    for( auto& [absPath, fileStatus] : aFileStatus )
    {
        wxString relativePath = absPath;
        if( relativePath.StartsWith( repoWorkDir ) )
        {
            relativePath = relativePath.Mid( repoWorkDir.length() );
#ifdef _WIN32
            relativePath.Replace( wxS( "\\" ), wxS( "/" ) );
#endif
        }

        std::string relativePathStd = relativePath.ToStdString();

        if( fileStatus.status == KIGIT_COMMON::GIT_STATUS::GIT_STATUS_CURRENT )
        {
            if( aLocalChanges.count( relativePathStd ) )
            {
                fileStatus.status = KIGIT_COMMON::GIT_STATUS::GIT_STATUS_AHEAD;
            }
            else if( aRemoteChanges.count( relativePathStd ) )
            {
                fileStatus.status = KIGIT_COMMON::GIT_STATUS::GIT_STATUS_BEHIND;
            }
        }
    }
}

wxString LIBGIT_BACKEND::GetWorkingDirectory( GIT_STATUS_HANDLER* aHandler )
{
    return aHandler->GetProjectDir();
}

wxString LIBGIT_BACKEND::GetWorkingDirectory( GIT_CONFIG_HANDLER* aHandler )
{
    return aHandler->GetProjectDir();
}

bool LIBGIT_BACKEND::GetConfigString( GIT_CONFIG_HANDLER* aHandler, const wxString& aKey,
                                      wxString& aValue )
{
    git_repository* repo = aHandler->GetRepo();

    if( !repo )
        return false;

    git_config* config = nullptr;

    if( git_repository_config( &config, repo ) != GIT_OK )
    {
        wxLogTrace( traceGit, "Failed to get repository config: %s", KIGIT_COMMON::GetLastGitError() );
        return false;
    }

    KIGIT::GitConfigPtr configPtr( config );

    git_config_entry* entry = nullptr;
    int result = git_config_get_entry( &entry, config, aKey.mb_str() );
    KIGIT::GitConfigEntryPtr entryPtr( entry );

    if( result != GIT_OK || entry == nullptr )
    {
        wxLogTrace( traceGit, "Config key '%s' not found", aKey );
        return false;
    }

    aValue = wxString( entry->value );
    return true;
}

bool LIBGIT_BACKEND::IsRepository( GIT_INIT_HANDLER* aHandler, const wxString& aPath )
{
    git_repository* repo = nullptr;
    int error = git_repository_open( &repo, aPath.mb_str() );

    if( error == 0 )
    {
        git_repository_free( repo );
        return true;
    }

    return false;
}

InitResult LIBGIT_BACKEND::InitializeRepository( GIT_INIT_HANDLER* aHandler, const wxString& aPath )
{
    if( IsRepository( aHandler, aPath ) )
    {
        return InitResult::AlreadyExists;
    }

    git_repository* repo = nullptr;

    if( git_repository_init( &repo, aPath.mb_str(), 0 ) != GIT_OK )
    {
        if( repo )
            git_repository_free( repo );

        aHandler->AddErrorString( wxString::Format( _( "Failed to initialize Git repository: %s" ),
                                                    KIGIT_COMMON::GetLastGitError() ) );
        return InitResult::Error;
    }

    aHandler->GetCommon()->SetRepo( repo );

    wxLogTrace( traceGit, "Successfully initialized Git repository at %s", aPath );
    return InitResult::Success;
}

bool LIBGIT_BACKEND::SetupRemote( GIT_INIT_HANDLER* aHandler, const RemoteConfig& aConfig )
{
    if( aConfig.url.IsEmpty() )
        return true;

    git_repository* repo = aHandler->GetRepo();

    if( !repo )
    {
        aHandler->AddErrorString( _( "No repository available to set up remote" ) );
        return false;
    }

    aHandler->GetCommon()->SetUsername( aConfig.username );
    aHandler->GetCommon()->SetPassword( aConfig.password );
    aHandler->GetCommon()->SetSSHKey( aConfig.sshKey );

    git_remote* remote = nullptr;
    wxString fullURL;

    if( aConfig.connType == KIGIT_COMMON::GIT_CONN_TYPE::GIT_CONN_SSH )
    {
        fullURL = aConfig.username + "@" + aConfig.url;
    }
    else if( aConfig.connType == KIGIT_COMMON::GIT_CONN_TYPE::GIT_CONN_HTTPS )
    {
        fullURL = aConfig.url.StartsWith( "https" ) ? "https://" : "http://";

        if( !aConfig.username.empty() )
        {
            fullURL.append( aConfig.username );

            if( !aConfig.password.empty() )
            {
                fullURL.append( wxS( ":" ) );
                fullURL.append( aConfig.password );
            }

            fullURL.append( wxS( "@" ) );
        }

        wxString bareURL = aConfig.url;
        if( bareURL.StartsWith( "https://" ) )
            bareURL = bareURL.Mid( 8 );
        else if( bareURL.StartsWith( "http://" ) )
            bareURL = bareURL.Mid( 7 );

        fullURL.append( bareURL );
    }
    else
    {
        fullURL = aConfig.url;
    }

    int error = git_remote_create_with_fetchspec( &remote, repo, "origin",
                                                  fullURL.ToStdString().c_str(),
                                                  "+refs/heads/*:refs/remotes/origin/*" );

    KIGIT::GitRemotePtr remotePtr( remote );

    if( error != GIT_OK )
    {
        aHandler->AddErrorString( wxString::Format( _( "Failed to create remote: %s" ),
                                                    KIGIT_COMMON::GetLastGitError() ) );
        return false;
    }

    wxLogTrace( traceGit, "Successfully set up remote origin" );
    return true;
}

static bool lookup_branch_reference( git_repository* repo, const wxString& aBranchName,
                                     git_reference** aReference )
{
    if( git_reference_lookup( aReference, repo, aBranchName.mb_str() ) == GIT_OK )
        return true;

    if( git_reference_dwim( aReference, repo, aBranchName.mb_str() ) == GIT_OK )
        return true;

    return false;
}

BranchResult LIBGIT_BACKEND::SwitchToBranch( GIT_BRANCH_HANDLER* aHandler, const wxString& aBranchName )
{
    git_repository* repo = aHandler->GetRepo();

    if( !repo )
    {
        aHandler->AddErrorString( _( "No repository available" ) );
        return BranchResult::Error;
    }

    git_reference* branchRef = nullptr;

    if( !lookup_branch_reference( repo, aBranchName, &branchRef ) )
    {
        aHandler->AddErrorString( wxString::Format( _( "Failed to lookup branch '%s': %s" ),
                                                   aBranchName, KIGIT_COMMON::GetLastGitError() ) );
        return BranchResult::BranchNotFound;
    }

    KIGIT::GitReferencePtr branchRefPtr( branchRef );
    const char* branchRefName = git_reference_name( branchRef );
    git_object* branchObj = nullptr;

    if( git_revparse_single( &branchObj, repo, aBranchName.mb_str() ) != GIT_OK )
    {
        aHandler->AddErrorString( wxString::Format( _( "Failed to find branch head for '%s': %s" ),
                                                    aBranchName, KIGIT_COMMON::GetLastGitError() ) );
        return BranchResult::Error;
    }

    KIGIT::GitObjectPtr branchObjPtr( branchObj );

    if( git_checkout_tree( repo, branchObj, nullptr ) != GIT_OK )
    {
        aHandler->AddErrorString( wxString::Format( _( "Failed to switch to branch '%s': %s" ),
                                                    aBranchName, KIGIT_COMMON::GetLastGitError() ) );
        return BranchResult::CheckoutFailed;
    }

    if( git_repository_set_head( repo, branchRefName ) != GIT_OK )
    {
        aHandler->AddErrorString( wxString::Format( _( "Failed to update HEAD reference for branch '%s': %s" ),
                                                    aBranchName, KIGIT_COMMON::GetLastGitError() ) );
        return BranchResult::Error;
    }

    wxLogTrace( traceGit, "Successfully switched to branch '%s'", aBranchName );
    return BranchResult::Success;
}

bool LIBGIT_BACKEND::BranchExists( GIT_BRANCH_HANDLER* aHandler, const wxString& aBranchName )
{
    git_repository* repo = aHandler->GetRepo();

    if( !repo )
        return false;

    git_reference* branchRef = nullptr;
    bool exists = lookup_branch_reference( repo, aBranchName, &branchRef );

    if( branchRef )
        git_reference_free( branchRef );

    return exists;
}

// Use callbacks declared/implemented in kicad_git_common.h/.cpp

bool LIBGIT_BACKEND::PerformFetch( GIT_PULL_HANDLER* aHandler, bool aSkipLock )
{
    if( !aHandler->GetRepo() )
    {
        wxLogTrace( traceGit, "GIT_PULL_HANDLER::PerformFetch() - No repository found" );
        return false;
    }

    std::unique_lock<std::mutex> lock( aHandler->GetCommon()->m_gitActionMutex, std::try_to_lock );

    if( !aSkipLock && !lock.owns_lock() )
    {
        wxLogTrace( traceGit, "GIT_PULL_HANDLER::PerformFetch() - Could not lock mutex" );
        return false;
    }

    git_remote* remote = nullptr;

    if( git_remote_lookup( &remote, aHandler->GetRepo(), "origin" ) != 0 )
    {
        wxLogTrace( traceGit, "GIT_PULL_HANDLER::PerformFetch() - Failed to lookup remote 'origin'" );
        aHandler->AddErrorString( wxString::Format( _( "Could not lookup remote '%s'" ), "origin" ) );
        return false;
    }

    KIGIT::GitRemotePtr remotePtr( remote );

    git_remote_callbacks remoteCallbacks;
    git_remote_init_callbacks( &remoteCallbacks, GIT_REMOTE_CALLBACKS_VERSION );
    remoteCallbacks.sideband_progress = progress_cb;
    remoteCallbacks.transfer_progress = transfer_progress_cb;
    remoteCallbacks.credentials = credentials_cb;
    remoteCallbacks.payload = aHandler;
    aHandler->GetCommon()->SetCancelled( false );

    aHandler->TestedTypes() = 0;
    aHandler->ResetNextKey();

    if( git_remote_connect( remote, GIT_DIRECTION_FETCH, &remoteCallbacks, nullptr, nullptr ) )
    {
        wxString errorMsg = KIGIT_COMMON::GetLastGitError();
        wxLogTrace( traceGit, "GIT_PULL_HANDLER::PerformFetch() - Failed to connect to remote: %s", errorMsg );
        aHandler->AddErrorString( wxString::Format( _( "Could not connect to remote '%s': %s" ), "origin", errorMsg ) );
        return false;
    }

    git_fetch_options fetchOptions;
    git_fetch_init_options( &fetchOptions, GIT_FETCH_OPTIONS_VERSION );
    fetchOptions.callbacks = remoteCallbacks;

    if( git_remote_fetch( remote, nullptr, &fetchOptions, nullptr ) )
    {
        wxString errorMsg = KIGIT_COMMON::GetLastGitError();
        wxLogTrace( traceGit, "GIT_PULL_HANDLER::PerformFetch() - Failed to fetch from remote: %s", errorMsg );
        aHandler->AddErrorString( wxString::Format( _( "Could not fetch data from remote '%s': %s" ), "origin", errorMsg ) );
        return false;
    }

    wxLogTrace( traceGit, "GIT_PULL_HANDLER::PerformFetch() - Fetch completed successfully" );
    return true;
}

PullResult LIBGIT_BACKEND::PerformPull( GIT_PULL_HANDLER* aHandler )
{
    PullResult result = PullResult::Success;
    std::unique_lock<std::mutex> lock( aHandler->GetCommon()->m_gitActionMutex, std::try_to_lock );

    if( !lock.owns_lock() )
    {
        wxLogTrace( traceGit, "GIT_PULL_HANDLER::PerformPull() - Could not lock mutex" );
        return PullResult::Error;
    }

    if( !PerformFetch( aHandler, true ) )
        return PullResult::Error;

    git_oid pull_merge_oid = {};

    if( git_repository_fetchhead_foreach( aHandler->GetRepo(), fetchhead_foreach_cb,
                                          &pull_merge_oid ) )
    {
        aHandler->AddErrorString( _( "Could not read 'FETCH_HEAD'" ) );
        return PullResult::Error;
    }

    git_annotated_commit* fetchhead_commit;

    if( git_annotated_commit_lookup( &fetchhead_commit, aHandler->GetRepo(), &pull_merge_oid ) )
    {
        aHandler->AddErrorString( _( "Could not lookup commit" ) );
        return PullResult::Error;
    }

    KIGIT::GitAnnotatedCommitPtr fetchheadCommitPtr( fetchhead_commit );
    const git_annotated_commit* merge_commits[] = { fetchhead_commit };
    git_merge_analysis_t        merge_analysis;
    git_merge_preference_t      merge_preference = GIT_MERGE_PREFERENCE_NONE;

    if( git_merge_analysis( &merge_analysis, &merge_preference, aHandler->GetRepo(), merge_commits, 1 ) )
    {
        aHandler->AddErrorString( _( "Could not analyze merge" ) );
        return PullResult::Error;
    }

    if( merge_analysis & GIT_MERGE_ANALYSIS_UNBORN )
    {
        aHandler->AddErrorString( _( "Invalid HEAD.  Cannot merge." ) );
        return PullResult::MergeFailed;
    }

    if( merge_analysis & GIT_MERGE_ANALYSIS_UP_TO_DATE )
    {
        wxLogTrace( traceGit, "GIT_PULL_HANDLER::PerformPull() - Repository is up to date" );
        git_repository_state_cleanup( aHandler->GetRepo() );
        return PullResult::UpToDate;
    }

    if( merge_analysis & GIT_MERGE_ANALYSIS_FASTFORWARD )
    {
        wxLogTrace( traceGit, "GIT_PULL_HANDLER::PerformPull() - Fast-forward merge" );
        return handleFastForward( aHandler );
    }

    if( merge_analysis & GIT_MERGE_ANALYSIS_NORMAL )
    {
        wxLogTrace( traceGit, "GIT_PULL_HANDLER::PerformPull() - Normal merge" );

        git_config* config = nullptr;

        if( git_repository_config( &config, aHandler->GetRepo() ) != GIT_OK )
        {
            wxLogTrace( traceGit, "GIT_PULL_HANDLER::PerformPull() - Failed to get repository config" );
            aHandler->AddErrorString( _( "Could not access repository configuration" ) );
            return PullResult::Error;
        }

        KIGIT::GitConfigPtr configPtr( config );

        int rebase_value = 0;
        int ret = git_config_get_bool( &rebase_value, config, "pull.rebase" );

        if( ret == GIT_OK && rebase_value )
        {
            wxLogTrace( traceGit, "GIT_PULL_HANDLER::PerformPull() - Using rebase based on config" );
            return handleRebase( aHandler, merge_commits, 1 );
        }

        wxLogTrace( traceGit, "GIT_PULL_HANDLER::PerformPull() - Using merge based on config" );
        return handleMerge( aHandler, merge_commits, 1 );
    }

    wxLogTrace( traceGit, "GIT_PULL_HANDLER::PerformPull() - Merge needs resolution" );
    return result;
}

PullResult LIBGIT_BACKEND::handleFastForward( GIT_PULL_HANDLER* aHandler )
{
    git_reference* rawRef = nullptr;

    if( git_repository_head( &rawRef, aHandler->GetRepo() ) )
    {
        aHandler->AddErrorString( _( "Could not get repository head" ) );
        return PullResult::Error;
    }

    KIGIT::GitReferencePtr headRef( rawRef );

    git_oid     updatedRefOid;
    const char* currentBranchName = git_reference_name( rawRef );
    const char* branch_shorthand = git_reference_shorthand( rawRef );
    wxString remote_name = aHandler->GetRemotename();
    wxString remoteBranchName = wxString::Format( "refs/remotes/%s/%s", remote_name, branch_shorthand );

    if( git_reference_name_to_id( &updatedRefOid, aHandler->GetRepo(), remoteBranchName.c_str() ) != GIT_OK )
    {
        aHandler->AddErrorString( wxString::Format( _( "Could not get reference OID for reference '%s'" ),
                                                   remoteBranchName ) );
        return PullResult::Error;
    }

    git_commit* targetCommit = nullptr;

    if( git_commit_lookup( &targetCommit, aHandler->GetRepo(), &updatedRefOid ) != GIT_OK )
    {
        aHandler->AddErrorString( _( "Could not look up target commit" ) );
        return PullResult::Error;
    }

    KIGIT::GitCommitPtr targetCommitPtr( targetCommit );

    git_tree* targetTree = nullptr;

    if( git_commit_tree( &targetTree, targetCommit ) != GIT_OK )
    {
        git_commit_free( targetCommit );
        aHandler->AddErrorString( _( "Could not get tree from target commit" ) );
        return PullResult::Error;
    }

    KIGIT::GitTreePtr targetTreePtr( targetTree );

    git_checkout_options checkoutOptions;
    git_checkout_init_options( &checkoutOptions, GIT_CHECKOUT_OPTIONS_VERSION );
    auto notify_cb = []( git_checkout_notify_t why, const char* path, const git_diff_file* baseline,
                         const git_diff_file* target, const git_diff_file* workdir, void* payload ) -> int
    {
        switch( why )
        {
        case GIT_CHECKOUT_NOTIFY_CONFLICT:
            wxLogTrace( traceGit, "Checkout conflict: %s", path ? path : "unknown" );
            break;
        case GIT_CHECKOUT_NOTIFY_DIRTY:
            wxLogTrace( traceGit, "Checkout dirty: %s", path ? path : "unknown" );
            break;
        case GIT_CHECKOUT_NOTIFY_UPDATED:
            wxLogTrace( traceGit, "Checkout updated: %s", path ? path : "unknown" );
            break;
        case GIT_CHECKOUT_NOTIFY_UNTRACKED:
            wxLogTrace( traceGit, "Checkout untracked: %s", path ? path : "unknown" );
            break;
        case GIT_CHECKOUT_NOTIFY_IGNORED:
            wxLogTrace( traceGit, "Checkout ignored: %s", path ? path : "unknown" );
            break;
        default:
            break;
        }

        return 0;
    };

    checkoutOptions.checkout_strategy = GIT_CHECKOUT_SAFE | GIT_CHECKOUT_ALLOW_CONFLICTS;
    checkoutOptions.notify_flags = GIT_CHECKOUT_NOTIFY_ALL;
    checkoutOptions.notify_cb = notify_cb;

    if( git_checkout_tree( aHandler->GetRepo(), reinterpret_cast<git_object*>( targetTree ), &checkoutOptions ) != GIT_OK )
    {
        aHandler->AddErrorString( _( "Failed to perform checkout operation." ) );
        return PullResult::Error;
    }

    git_reference* updatedRef = nullptr;

    if( git_reference_set_target( &updatedRef, rawRef, &updatedRefOid, nullptr ) != GIT_OK )
    {
        aHandler->AddErrorString( wxString::Format( _( "Failed to update reference '%s' to point to '%s'" ),
                                                   currentBranchName, git_oid_tostr_s( &updatedRefOid ) ) );
        return PullResult::Error;
    }

    KIGIT::GitReferencePtr updatedRefPtr( updatedRef );

    if( git_repository_state_cleanup( aHandler->GetRepo() ) != GIT_OK )
    {
        aHandler->AddErrorString( _( "Failed to clean up repository state after fast-forward." ) );
        return PullResult::Error;
    }

    git_revwalk* revWalker = nullptr;

    if( git_revwalk_new( &revWalker, aHandler->GetRepo() ) != GIT_OK )
    {
        aHandler->AddErrorString( _( "Failed to initialize revision walker." ) );
        return PullResult::Error;
    }

    KIGIT::GitRevWalkPtr revWalkerPtr( revWalker );
    git_revwalk_sorting( revWalker, GIT_SORT_TIME );

    if( git_revwalk_push_glob( revWalker, currentBranchName ) != GIT_OK )
    {
        aHandler->AddErrorString( _( "Failed to push reference to revision walker." ) );
        return PullResult::Error;
    }

    std::pair<std::string, std::vector<CommitDetails>>& branchCommits = aHandler->m_fetchResults.emplace_back();
    branchCommits.first = currentBranchName;

    git_oid commitOid;

    while( git_revwalk_next( &commitOid, revWalker ) == GIT_OK )
    {
        git_commit* commit = nullptr;

        if( git_commit_lookup( &commit, aHandler->GetRepo(), &commitOid ) )
        {
            aHandler->AddErrorString( wxString::Format( _( "Could not lookup commit '%s'" ),
                                                        git_oid_tostr_s( &commitOid ) ) );
            return PullResult::Error;
        }

        KIGIT::GitCommitPtr commitPtr( commit );

        CommitDetails details;
        details.m_sha = git_oid_tostr_s( &commitOid );
        details.m_firstLine = getFirstLineFromCommitMessage( git_commit_message( commit ) );
        details.m_author = git_commit_author( commit )->name;
        details.m_date = getFormattedCommitDate( git_commit_author( commit )->when );

        branchCommits.second.push_back( details );
    }

    return PullResult::FastForward;
}

PullResult LIBGIT_BACKEND::handleMerge( GIT_PULL_HANDLER* aHandler,
                                        const git_annotated_commit** aMergeHeads,
                                        size_t aMergeHeadsCount )
{
    if( git_merge( aHandler->GetRepo(), aMergeHeads, aMergeHeadsCount, nullptr, nullptr ) )
    {
        aHandler->AddErrorString( _( "Merge failed" ) );
        return PullResult::MergeFailed;
    }

    return PullResult::Success;
}

PullResult LIBGIT_BACKEND::handleRebase( GIT_PULL_HANDLER* aHandler,
                                         const git_annotated_commit** aMergeHeads,
                                         size_t aMergeHeadsCount )
{
    git_rebase_options rebase_opts;
    git_rebase_init_options( &rebase_opts, GIT_REBASE_OPTIONS_VERSION );

    git_rebase* rebase = nullptr;

    if( git_rebase_init( &rebase, aHandler->GetRepo(), nullptr, aMergeHeads[0], nullptr, &rebase_opts ) )
    {
        aHandler->AddErrorString( _( "Rebase failed to start" ) );
        return PullResult::MergeFailed;
    }

    KIGIT::GitRebasePtr rebasePtr( rebase );

    while( true )
    {
        git_rebase_operation* op = nullptr;
        if( git_rebase_next( &op, rebase ) != 0 )
            break;

        if( git_rebase_commit( nullptr, rebase, nullptr, nullptr, nullptr, nullptr ) )
        {
            aHandler->AddErrorString( _( "Rebase commit failed" ) );
            return PullResult::MergeFailed;
        }
    }

    if( git_rebase_finish( rebase, nullptr ) )
    {
        aHandler->AddErrorString( _( "Rebase finish failed" ) );
        return PullResult::MergeFailed;
    }

    return PullResult::Success;
}

void LIBGIT_BACKEND::PerformRevert( GIT_REVERT_HANDLER* aHandler )
{
    git_object* head_commit = NULL;
    git_checkout_options opts;
    git_checkout_init_options( &opts, GIT_CHECKOUT_OPTIONS_VERSION );

    if( git_revparse_single( &head_commit, aHandler->m_repository, "HEAD" ) != 0 )
    {
        return;
    }

    opts.checkout_strategy = GIT_CHECKOUT_FORCE;
    char** paths = new char*[aHandler->m_filesToRevert.size()];

    for( size_t ii = 0; ii < aHandler->m_filesToRevert.size(); ii++ )
    {
        paths[ii] = wxStrdup( aHandler->m_filesToRevert[ii].ToUTF8() );
    }

    git_strarray arr = { paths, aHandler->m_filesToRevert.size() };

    opts.paths = arr;
    opts.progress_cb = nullptr;
    opts.notify_cb = nullptr;
    opts.notify_payload = static_cast<void*>( aHandler );

    if( git_checkout_tree( aHandler->m_repository, head_commit, &opts ) != 0 )
    {
        const git_error* e = git_error_last();

        if( e )
        {
            wxLogTrace( traceGit, wxS( "Checkout failed: %d: %s" ), e->klass, e->message );
        }
    }

    for( size_t ii = 0; ii < aHandler->m_filesToRevert.size(); ii++ )
        delete( paths[ii] );

    delete[] paths;

    git_object_free( head_commit );
}

git_repository* LIBGIT_BACKEND::GetRepositoryForFile( const char* aFilename )
{
    git_repository* repo = nullptr;
    git_buf repo_path = GIT_BUF_INIT;

    if( git_repository_discover( &repo_path, aFilename, 0, nullptr ) != GIT_OK )
    {
        wxLogTrace( traceGit, "Can't repo discover %s: %s", aFilename,
                    KIGIT_COMMON::GetLastGitError() );
        return nullptr;
    }

    KIGIT::GitBufPtr repo_path_ptr( &repo_path );

    if( git_repository_open( &repo, repo_path.ptr ) != GIT_OK )
    {
        wxLogTrace( traceGit, "Can't open repo for %s: %s", repo_path.ptr,
                    KIGIT_COMMON::GetLastGitError() );
        return nullptr;
    }

    return repo;
}

int LIBGIT_BACKEND::CreateBranch( git_repository* aRepo, const wxString& aBranchName )
{
    git_oid head_oid;

    if( int error = git_reference_name_to_id( &head_oid, aRepo, "HEAD" ); error != GIT_OK )
    {
        wxLogTrace( traceGit, "Failed to lookup HEAD reference: %s",
                    KIGIT_COMMON::GetLastGitError() );
        return error;
    }

    git_commit* commit = nullptr;

    if( int error = git_commit_lookup( &commit, aRepo, &head_oid ); error != GIT_OK )
    {
        wxLogTrace( traceGit, "Failed to lookup commit: %s",
                    KIGIT_COMMON::GetLastGitError() );
        return error;
    }

    KIGIT::GitCommitPtr commitPtr( commit );
    git_reference* branchRef = nullptr;

    if( int error = git_branch_create( &branchRef, aRepo, aBranchName.mb_str(), commit, 0 ); error != GIT_OK )
    {
        wxLogTrace( traceGit, "Failed to create branch: %s",
                    KIGIT_COMMON::GetLastGitError() );
        return error;
    }

    git_reference_free( branchRef );
    return 0;
}

bool LIBGIT_BACKEND::RemoveVCS( git_repository*& aRepo, const wxString& aProjectPath,
                                bool aRemoveGitDir, wxString* aErrors )
{
    if( aRepo )
    {
        git_repository_free( aRepo );
        aRepo = nullptr;
    }

    if( aRemoveGitDir )
    {
        wxFileName gitDir( aProjectPath, wxEmptyString );
        gitDir.AppendDir( ".git" );

        if( gitDir.DirExists() )
        {
            wxString errors;
            if( !RmDirRecursive( gitDir.GetPath(), &errors ) )
            {
                if( aErrors )
                    *aErrors = errors;

                wxLogTrace( traceGit, "Failed to remove .git directory: %s", errors );
                return false;
            }
        }
    }

    wxLogTrace( traceGit, "Successfully removed VCS from project" );
    return true;
}

bool LIBGIT_BACKEND::AddToIndex( GIT_ADD_TO_INDEX_HANDLER* aHandler, const wxString& aFilePath )
{
    git_repository* repo = aHandler->GetRepo();

    git_index* index = nullptr;
    size_t     at_pos = 0;

    if( git_repository_index( &index, repo ) != 0 )
    {
        wxLogError( "Failed to get repository index" );
        return false;
    }

    KIGIT::GitIndexPtr indexPtr( index );

    if( git_index_find( &at_pos, index, aFilePath.ToUTF8().data() ) == GIT_OK )
    {
        wxLogError( "%s already in index", aFilePath );
        return false;
    }

    aHandler->m_filesToAdd.push_back( aFilePath );
    return true;
}

bool LIBGIT_BACKEND::PerformAddToIndex( GIT_ADD_TO_INDEX_HANDLER* aHandler )
{
    git_repository* repo = aHandler->GetRepo();
    git_index* index = nullptr;

    aHandler->m_filesFailedToAdd.clear();

    if( git_repository_index( &index, repo ) != 0 )
    {
        wxLogError( "Failed to get repository index" );
        std::copy( aHandler->m_filesToAdd.begin(), aHandler->m_filesToAdd.end(),
                   std::back_inserter( aHandler->m_filesFailedToAdd ) );
        return false;
    }

    KIGIT::GitIndexPtr indexPtr( index );

    for( auto& file : aHandler->m_filesToAdd )
    {
        if( git_index_add_bypath( index, file.ToUTF8().data() ) != 0 )
        {
            wxLogError( "Failed to add %s to index", file );
            aHandler->m_filesFailedToAdd.push_back( file );
            continue;
        }
    }

    if( git_index_write( index ) != 0 )
    {
        wxLogError( "Failed to write index" );
        aHandler->m_filesFailedToAdd.clear();
        std::copy( aHandler->m_filesToAdd.begin(), aHandler->m_filesToAdd.end(),
                   std::back_inserter( aHandler->m_filesFailedToAdd ) );
        return false;
    }

    return true;
}

bool LIBGIT_BACKEND::RemoveFromIndex( GIT_REMOVE_FROM_INDEX_HANDLER* aHandler,
                                      const wxString& aFilePath )
{
    git_repository* repo = aHandler->GetRepo();
    git_index*      index = nullptr;
    size_t          at_pos = 0;

    if( git_repository_index( &index, repo ) != 0 )
    {
        wxLogError( "Failed to get repository index" );
        return false;
    }

    KIGIT::GitIndexPtr indexPtr( index );

    if( git_index_find( &at_pos, index, aFilePath.ToUTF8().data() ) != 0 )
    {
        wxLogError( "Failed to find index entry for %s", aFilePath );
        return false;
    }

    aHandler->m_filesToRemove.push_back( aFilePath );
    return true;
}

void LIBGIT_BACKEND::PerformRemoveFromIndex( GIT_REMOVE_FROM_INDEX_HANDLER* aHandler )
{
    git_repository* repo = aHandler->GetRepo();

    for( auto& file : aHandler->m_filesToRemove )
    {
        git_index* index = nullptr;
        git_oid    oid;

        if( git_repository_index( &index, repo ) != 0 )
        {
            wxLogError( "Failed to get repository index" );
            return;
        }

        KIGIT::GitIndexPtr indexPtr( index );

        if( git_index_remove_bypath( index, file.ToUTF8().data() ) != 0 )
        {
            wxLogError( "Failed to remove index entry for %s", file );
            return;
        }

        if( git_index_write( index ) != 0 )
        {
            wxLogError( "Failed to write index" );
            return;
        }

        if( git_index_write_tree( &oid, index ) != 0 )
        {
            wxLogError( "Failed to write index tree" );
            return;
        }
    }
}
