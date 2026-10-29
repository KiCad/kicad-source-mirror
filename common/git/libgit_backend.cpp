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

#include "libgit_backend.h"

#include "git_clone_handler.h"
#include "git_commit_handler.h"
#include "git_push_handler.h"
#include "git_status_handler.h"
#include "git_config_handler.h"
#include "git_init_handler.h"
#include "git_branch_handler.h"
#include "git_pull_handler.h"
#include "project_git_utils.h"
#include "kicad_git_common.h"
#include "kicad_git_memory.h"
#include "kigit_failure.h"
#include "kigit_ssh_keys.h"
#include "trace_helpers.h"

#include "kicad_git_compat.h"
#include <wx/file.h>
#include <wx/filename.h>
#include <wx/log.h>
#include <gestfich.h>
#include <paths.h>
#include <algorithm>
#include <cstring>
#include <fstream>
#include <iterator>
#include <map>
#include <memory>
#include <optional>
#include <set>
#include <string>
#include <vector>


void LIBGIT_BACKEND::Init()
{
    git_libgit2_init();

    KIGIT::SetRememberedSSHKeyStore(
            wxFileName( PATHS::GetUserCachePath(), wxS( "git_ssh_keys.json" ) ).GetFullPath() );
    KIGIT::SetDerivedPublicKeyFolder( wxFileName( PATHS::GetUserCachePath(), wxS( "ssh_public_keys" ) ).GetFullPath() );

    // libgit2 sets no timeout of its own, so an unreachable remote parks a fetch in connect()
    // or recv() for as long as the OS allows.  SSH remotes stay bounded only by libssh2.

#if ( LIBGIT2_VER_MAJOR > 1 ) || ( LIBGIT2_VER_MAJOR == 1 && LIBGIT2_VER_MINOR >= 7 )
    constexpr int connectTimeoutMs = 10000;
    constexpr int transferTimeoutMs = 30000;

    if( git_libgit2_opts( GIT_OPT_SET_SERVER_CONNECT_TIMEOUT, connectTimeoutMs ) != GIT_OK
            || git_libgit2_opts( GIT_OPT_SET_SERVER_TIMEOUT, transferTimeoutMs ) != GIT_OK )
    {
        wxLogTrace( traceGit, "LIBGIT_BACKEND::Init(): could not set server timeouts: %s",
                    KIGIT_COMMON::GetLastGitError() );
    }
#endif
}


void LIBGIT_BACKEND::Shutdown()
{
    // Wait for any abandoned git cleanup threads to finish before tearing
    // down libgit2.  A worker still inside libgit2 (for example, blocked on
    // recv() under git_remote_fetch) would otherwise race teardown and
    // invoke undefined behaviour.  Five seconds is long enough to cover a
    // transport error timeout but short enough to avoid a perceptibly slow
    // exit when the remote is truly unreachable.

    constexpr auto kOrphanJoinTimeout = std::chrono::seconds( 5 );
    size_t         stuck = m_orphanRegistry.JoinAll( kOrphanJoinTimeout );

    if( stuck > 0 )
    {
        wxLogTrace( traceGit, "LIBGIT_BACKEND::Shutdown(): %zu orphan git thread(s) did not finish within %lld ms; "
                              "skipping libgit2 shutdown",
                    stuck, static_cast<long long>( kOrphanJoinTimeout.count() ) );

        // A stuck worker is still executing inside libgit2.  Calling
        // git_libgit2_shutdown() now would free state the worker is actively
        // reading.  Leave libgit2 initialised and let the OS reclaim
        // resources when the process exits.

        return;
    }

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


// git_signature_default answers GIT_ENOTFOUND only when user.name or user.email is not set
static KIGIT_ERROR_MESSAGE describeSignatureFailure( int aRc )
{
    if( aRc == GIT_ENOTFOUND )
        return { _( "Set your name and email before committing (gear menu > Set Author...)." ), wxEmptyString };

    return { _( "Could not record the commit author." ), KIGIT_COMMON::GetLastGitError() };
}


static void reportRemoteLookupFailure( KIGIT_ERRORS* aHandler, int aRc, const wxString& aRemoteName )
{
    if( aRc == GIT_ENOTFOUND )
    {
        aHandler->AddError( wxString::Format( _( "There is no remote named '%s'. Add it on the Remotes page of the "
                                                 "Advanced Git window." ),
                                              aRemoteName ),
                            wxEmptyString );
        return;
    }

    aHandler->AddError( wxString::Format( _( "Could not read the remote '%s'." ), aRemoteName ),
                        KIGIT_COMMON::GetLastGitError() );
}


// url.insteadOf can rewrite the clone URL, which would otherwise read as a new identity and drop the
// credentials entered for the original
static int clone_remote_cb( git_remote** aOut, git_repository* aRepo, const char* aName, const char* aUrl,
                            void* aPayload )
{
    if( int rc = git_remote_create( aOut, aRepo, aName, aUrl ); rc != GIT_OK )
        return rc;

    if( const char* url = git_remote_url( *aOut ) )
        static_cast<KIGIT_COMMON*>( aPayload )->FollowRewrittenUrl( wxString::FromUTF8( url ) );

    return GIT_OK;
}


bool LIBGIT_BACKEND::Clone( GIT_CLONE_HANDLER* aHandler )
{
    KIGIT_COMMON* common = aHandler->GetCommon();
    std::unique_lock<std::mutex> lock = aHandler->GetActionLock();

    if( !lock.owns_lock() )
    {
        wxLogTrace( traceGit, "GIT_CLONE_HANDLER::PerformClone() could not lock" );
        return false;
    }

    // Without this the progress dialog's Cancel would set a token nothing reads.
    KIGIT_COMMON::OPERATION_GUARD operation( common, aHandler->CancelToken() );

    if( aHandler->IsThisOperationCancelled() )
        return false;

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
    cloneOptions.fetch_opts.callbacks.certificate_check = certificate_check_cb;
    cloneOptions.fetch_opts.callbacks.payload = aHandler;
    cloneOptions.fetch_opts.proxy_opts.type = GIT_PROXY_AUTO;
    cloneOptions.remote_cb = clone_remote_cb;
    cloneOptions.remote_cb_payload = common;

    aHandler->ResetTransferState();
    git_repository* newRepo = nullptr;
    wxString        remote = common->GetRemote();
    std::string     remoteUtf8 = remote.utf8_string();

    if( int rc = git_clone( &newRepo, remoteUtf8.c_str(), aHandler->GetClonePath().mbc_str(), &cloneOptions ) )
    {
        aHandler->ReportTransferFailure( KIGIT_COMMON::GetLastGitErrorInfo( rc ), KIGIT_OPERATION::CLONE,
                                         wxEmptyString );
        return false;
    }

    aHandler->RememberWorkingSSHKey();
    common->SetRepo( newRepo );

    return true;
}


// Enough of a path's staged state to tell whether something else restaged it.
struct SELECTED_STATE
{
    bool     present = false;
    git_oid  id = {};
    uint32_t mode = 0;

    bool operator==( const SELECTED_STATE& aOther ) const
    {
        return present == aOther.present && mode == aOther.mode && ( !present || git_oid_equal( &id, &aOther.id ) );
    }

    bool operator!=( const SELECTED_STATE& aOther ) const { return !( *this == aOther ); }
};


static std::map<std::string, SELECTED_STATE> readSelectedState( git_index*                   aIndex,
                                                                const std::set<std::string>& aSelected )
{
    std::map<std::string, SELECTED_STATE> result;

    for( const std::string& path : aSelected )
    {
        SELECTED_STATE state;

        if( const git_index_entry* entry = git_index_get_bypath( aIndex, path.c_str(), 0 ) )
        {
            state.present = true;
            state.id = entry->id;
            state.mode = entry->mode;
        }

        result[path] = state;
    }

    return result;
}


static std::map<std::string, SELECTED_STATE> readWholeState( git_index* aIndex )
{
    std::map<std::string, SELECTED_STATE> result;
    size_t                                count = git_index_entrycount( aIndex );

    for( size_t ii = 0; ii < count; ii++ )
    {
        if( const git_index_entry* entry = git_index_get_byindex( aIndex, ii ) )
        {
            SELECTED_STATE state;
            state.present = true;
            state.id = entry->id;
            state.mode = entry->mode;
            result[entry->path] = state;
        }
    }

    return result;
}


// The index file itself, not the scratch copy the handle holds. Null when it cannot be read.
static git_index* openIndexOnDisk( git_index* aRepoIndex )
{
    const char* indexPath = git_index_path( aRepoIndex );

    if( !indexPath )
        return nullptr;

    git_index* fresh = nullptr;

    if( git_index_open( &fresh, indexPath ) != GIT_OK )
        return nullptr;

    int rc = git_index_read( fresh, 1 );

    // A repository with nothing staged yet has no index file, which reads as empty.
    if( rc != GIT_OK && rc != GIT_ENOTFOUND )
    {
        git_index_free( fresh );
        return nullptr;
    }

    return fresh;
}


static bool selectedStateOnDisk( git_index* aRepoIndex, const std::set<std::string>& aSelected,
                                 std::map<std::string, SELECTED_STATE>& aOut )
{
    git_index* fresh = openIndexOnDisk( aRepoIndex );

    if( !fresh )
        return false;

    KIGIT::GitIndexPtr freshPtr( fresh );
    aOut = readSelectedState( fresh, aSelected );
    return true;
}


static bool wholeStateOnDisk( git_index* aRepoIndex, std::map<std::string, SELECTED_STATE>& aOut )
{
    git_index* fresh = openIndexOnDisk( aRepoIndex );

    if( !fresh )
        return false;

    KIGIT::GitIndexPtr freshPtr( fresh );
    aOut = readWholeState( fresh );
    return true;
}


// Owns its path, because index entries point into storage that rebuilding invalidates.
struct STAGED_ENTRY
{
    std::string     path;
    git_index_entry entry;
};


// The index is scratch until the commit exists, so every way out reloads what the user staged.
struct SCRATCH_INDEX_GUARD
{
    git_index* index = nullptr;
    bool       armed = true;

    ~SCRATCH_INDEX_GUARD()
    {
        if( armed && index )
            git_index_read( index, 1 );
    }
};


// Keeps staging done outside KiCad, and moves the selected paths to what was just committed.
// A listed file missing from the working tree is a deletion to record, not a path to add.
static bool applySelectedPaths( git_repository* aRepo, git_index* aIndex, const std::vector<wxString>& aFiles )
{
    const char* workdir = git_repository_workdir( aRepo );

    for( const wxString& file : aFiles )
    {
        // Anything at the path counts. A submodule is a directory and a symlink may point at
        // one or at nothing, and reading those as deletions drops them out of the project.
        bool onDisk = workdir
                      && wxFileName::Exists( wxString::FromUTF8( workdir ) + file,
                                             wxFILE_EXISTS_ANY | wxFILE_EXISTS_NO_FOLLOW );

        // libgit2 paths are UTF-8, so convert explicitly rather than through the locale.
        std::string path = file.utf8_string();

        int rc =
                onDisk ? git_index_add_bypath( aIndex, path.c_str() ) : git_index_remove_bypath( aIndex, path.c_str() );

        if( rc != 0 )
            return false;
    }

    return true;
}


// A commit landing between the tree build and the parent lookup would be undone by this one.
static bool branchMovedSince( git_repository* aRepo, const git_oid& aHeadAtStart, bool aHadHead )
{
    git_oid headNow;
    bool    hasHeadNow = git_reference_name_to_id( &headNow, aRepo, "HEAD" ) == GIT_OK;

    return hasHeadNow != aHadHead || ( hasHeadNow && !git_oid_equal( &headNow, &aHeadAtStart ) );
}


// Something restaged a file being committed. Moving it on would destroy that record.
static bool selectedPathsRestaged( git_index* aIndex, const std::set<std::string>& aSelected,
                                   const std::map<std::string, SELECTED_STATE>& aAtStart )
{
    std::map<std::string, SELECTED_STATE> now;

    return !selectedStateOnDisk( aIndex, aSelected, now ) || now != aAtStart;
}


// Catches a file staged outside KiCad that this commit does not list, which the check above misses.
static bool indexChangedSince( git_index* aIndex, const std::map<std::string, SELECTED_STATE>& aAtStart )
{
    std::map<std::string, SELECTED_STATE> now;

    return !wholeStateOnDisk( aIndex, now ) || now != aAtStart;
}


enum class REPUBLISH_RESULT
{
    UPDATED,
    CHANGED_EXTERNALLY,
    WRITE_FAILED
};


// Reread and compare before writing, or staging done elsewhere is overwritten.
static REPUBLISH_RESULT republishIndex( git_index* aIndex, const std::vector<STAGED_ENTRY>& aCommitted,
                                        const std::set<std::string>& aSelected )
{
    for( int attempt = 0; attempt < 3; attempt++ )
    {
        if( git_index_read( aIndex, 1 ) != 0 )
            return REPUBLISH_RESULT::WRITE_FAILED;

        std::map<std::string, SELECTED_STATE> readState = readWholeState( aIndex );

        for( const std::string& path : aSelected )
            git_index_remove_bypath( aIndex, path.c_str() );

        for( const STAGED_ENTRY& staged : aCommitted )
        {
            git_index_entry entry = staged.entry;
            entry.path = staged.path.c_str();

            if( git_index_add( aIndex, &entry ) != 0 )
                return REPUBLISH_RESULT::WRITE_FAILED;
        }

        std::map<std::string, SELECTED_STATE> diskState;

        if( !wholeStateOnDisk( aIndex, diskState ) )
            return REPUBLISH_RESULT::WRITE_FAILED;

        if( diskState != readState )
            continue;

        if( git_index_write( aIndex ) == 0 )
            return REPUBLISH_RESULT::UPDATED;

        return REPUBLISH_RESULT::WRITE_FAILED;
    }

    return REPUBLISH_RESULT::CHANGED_EXTERNALLY;
}


// Rebuilding from HEAD drops conflict stages, so markers would commit as resolved content.
static bool refuseWhileOperationInProgress( git_repository* aRepo, git_index* aIndex, wxString& aError )
{
    if( git_index_has_conflicts( aIndex ) )
    {
        aError = _( "This project has conflicts that are not resolved yet. Finish or abort the merge "
                    "with git, then commit here." );
        return true;
    }

    wxString operation;

    switch( git_repository_state( aRepo ) )
    {
    case GIT_REPOSITORY_STATE_NONE: return false;
    case GIT_REPOSITORY_STATE_MERGE: operation = _( "merge" ); break;
    case GIT_REPOSITORY_STATE_REVERT:
    case GIT_REPOSITORY_STATE_REVERT_SEQUENCE: operation = _( "revert" ); break;
    case GIT_REPOSITORY_STATE_CHERRYPICK:
    case GIT_REPOSITORY_STATE_CHERRYPICK_SEQUENCE: operation = _( "cherry-pick" ); break;
    case GIT_REPOSITORY_STATE_BISECT: operation = _( "bisect" ); break;
    default: operation = _( "rebase" ); break;
    }

    aError = wxString::Format( _( "This project is in the middle of a %s. Finish or abort it with git, "
                                  "then commit here." ),
                               operation );
    return true;
}


// What Commit and Amend do identically: take the index over as scratch space, remember what it
// held, stage the chosen files over a base tree, and refuse if anything moved underneath while
// the tree was being built. Only the base tree and the commit call itself differ between them.
struct COMMIT_STAGING
{
    git_index*                            index = nullptr;
    KIGIT::GitIndexPtr                    indexPtr;
    std::set<std::string>                 selectedPaths;
    std::map<std::string, SELECTED_STATE> selectedAtStart;
    std::map<std::string, SELECTED_STATE> indexAtStart;
    bool                                  haveIndexAtStart = false;
    git_oid                               headAtStart = {};
    bool                                  hadHeadAtStart = false;
    SCRATCH_INDEX_GUARD                   scratchGuard;
    std::vector<STAGED_ENTRY>             committedEntries;

    /// Read the index, record what it holds and take it over. False leaves the reason in aError.
    bool Begin( git_repository* aRepo, const std::vector<wxString>& aFiles, KIGIT_ERROR_MESSAGE& aError )
    {
        if( git_repository_index( &index, aRepo ) != 0 )
        {
            aError = { _( "Could not read the list of staged files." ), KIGIT_COMMON::GetLastGitError() };
            return false;
        }

        indexPtr.reset( index );

        // The action lock only serializes KiCad, so read what is on disk before judging it.
        git_index_read( index, 1 );

        wxString blockedError;

        if( refuseWhileOperationInProgress( aRepo, index, blockedError ) )
        {
            aError = { blockedError, wxEmptyString };
            return false;
        }

        for( const wxString& file : aFiles )
            selectedPaths.insert( file.utf8_string() );

        selectedAtStart = readSelectedState( index, selectedPaths );
        haveIndexAtStart = wholeStateOnDisk( index, indexAtStart );
        hadHeadAtStart = git_reference_name_to_id( &headAtStart, aRepo, "HEAD" ) == GIT_OK;

        // Armed last, so an early return above leaves the real index alone.
        scratchGuard.index = index;
        return true;
    }

    /// Stage the chosen files over whatever the index already holds and write the tree they
    /// make. False leaves the reason in aError.
    bool BuildTree( git_repository* aRepo, const std::vector<wxString>& aFiles, KIGIT::GitTreePtr& aTreeOut,
                    KIGIT_ERROR_MESSAGE& aError )
    {
        if( !applySelectedPaths( aRepo, index, aFiles ) )
        {
            aError = { _( "Could not stage the selected files." ), KIGIT_COMMON::GetLastGitError() };
            return false;
        }

        // Writing this scratch index is what would unstage everything staged outside KiCad, so
        // only the tree it describes is written.
        for( const std::string& path : selectedPaths )
        {
            if( const git_index_entry* entry = git_index_get_bypath( index, path.c_str(), 0 ) )
                committedEntries.push_back( { path, *entry } );
        }

        git_oid treeId;

        if( git_index_write_tree( &treeId, index ) != 0 )
        {
            aError = { _( "Could not prepare the commit." ), KIGIT_COMMON::GetLastGitError() };
            return false;
        }

        git_tree* tree = nullptr;

        if( git_tree_lookup( &tree, aRepo, &treeId ) != 0 )
        {
            aError = { _( "Could not prepare the commit." ), KIGIT_COMMON::GetLastGitError() };
            return false;
        }

        aTreeOut.reset( tree );

        // Nothing is written yet, so the scratch guard puts the real index back on the way out.
        if( branchMovedSince( aRepo, headAtStart, hadHeadAtStart ) )
        {
            aError = { _( "The branch moved while this was running, so nothing was "
                          "committed. Review the changes and try again." ),
                       wxEmptyString };
            return false;
        }

        if( selectedPathsRestaged( index, selectedPaths, selectedAtStart )
            || ( haveIndexAtStart && indexChangedSince( index, indexAtStart ) ) )
        {
            aError = { _( "The staged files changed outside KiCad while this ran. Nothing "
                          "was committed. Review the changes and try again." ),
                       wxEmptyString };
            return false;
        }

        return true;
    }
};


CommitResult LIBGIT_BACKEND::Commit( GIT_COMMIT_HANDLER* aHandler, const std::vector<wxString>& aFiles,
                                     const wxString& aMessage, const wxString& aAuthorName,
                                     const wxString& aAuthorEmail )
{
    git_repository* repo = aHandler->GetRepo();

    if( !repo )
        return CommitResult::Error;

    COMMIT_STAGING      staging;
    KIGIT_ERROR_MESSAGE stagingError;

    if( !staging.Begin( repo, aFiles, stagingError ) )
    {
        aHandler->AddError( stagingError );
        return CommitResult::Error;
    }

    git_index* index = staging.index;

    // Start from the HEAD tree, not the index, so the commit holds exactly the listed files.
    if( git_repository_head_unborn( repo ) == 0 )
    {
        git_reference* headRef = nullptr;

        if( git_repository_head( &headRef, repo ) != 0 )
        {
            aHandler->AddGitError( _( "Could not read the current commit." ) );
            return CommitResult::Error;
        }

        KIGIT::GitReferencePtr headRefPtr( headRef );
        git_tree*              headTree = nullptr;

        if( git_reference_peel( (git_object**) &headTree, headRef, GIT_OBJECT_TREE ) != 0 )
        {
            aHandler->AddGitError( _( "Could not read the current commit." ) );
            return CommitResult::Error;
        }

        KIGIT::GitTreePtr headTreePtr( headTree );

        if( git_index_read_tree( index, headTree ) != 0 )
        {
            aHandler->AddGitError( _( "Could not prepare the commit." ) );
            return CommitResult::Error;
        }
    }
    else if( git_index_clear( index ) != 0 )
    {
        aHandler->AddGitError( _( "Could not prepare the commit." ) );
        return CommitResult::Error;
    }

    KIGIT::GitTreePtr treePtr;

    if( !staging.BuildTree( repo, aFiles, treePtr, stagingError ) )
    {
        aHandler->AddError( stagingError );
        return CommitResult::Error;
    }

    git_tree* tree = treePtr.get();

    git_commit* parent = nullptr;

    if( git_repository_head_unborn( repo ) == 0 )
    {
        git_reference* headRef = nullptr;

        if( git_repository_head( &headRef, repo ) != 0 )
        {
            aHandler->AddGitError( _( "Could not read the current commit." ) );
            return CommitResult::Error;
        }

        KIGIT::GitReferencePtr headRefPtr( headRef );

        if( git_reference_peel( (git_object**) &parent, headRef, GIT_OBJECT_COMMIT ) != 0 )
        {
            aHandler->AddGitError( _( "Could not read the current commit." ) );
            return CommitResult::Error;
        }
    }

    KIGIT::GitCommitPtr parentPtr( parent );

    git_signature* author = nullptr;

    if( git_signature_now( &author, aAuthorName.utf8_string().c_str(), aAuthorEmail.utf8_string().c_str() ) != 0 )
    {
        aHandler->AddGitError( _( "Could not record the commit author." ) );
        return CommitResult::Error;
    }

    KIGIT::GitSignaturePtr authorPtr( author );
    git_oid                oid;
    size_t                 parentsCount = parent ? 1 : 0;

#if( LIBGIT2_VER_MAJOR == 1 && LIBGIT2_VER_MINOR == 8               \
    && ( LIBGIT2_VER_REVISION < 2 || LIBGIT2_VER_REVISION == 3 ) )
    git_commit* const parents[1] = { parent };
    git_commit** const parentsPtr = parent ? parents : nullptr;
#else
    const git_commit* parents[1] = { parent };
    const git_commit** parentsPtr = parent ? parents : nullptr;
#endif

    if( git_commit_create( &oid, repo, "HEAD", author, author, nullptr, aMessage.utf8_string().c_str(), tree,
                           parentsCount, parentsPtr )
        != 0 )
    {
        aHandler->AddGitError( _( "Could not create the commit." ) );
        return CommitResult::Error;
    }

    // The commit exists, so only the index is left. If anything restaged meanwhile, leave it.
    if( selectedPathsRestaged( index, staging.selectedPaths, staging.selectedAtStart )
        || ( staging.haveIndexAtStart && indexChangedSince( index, staging.indexAtStart ) ) )
    {
        git_index_read( index, 1 );
        aHandler->AddErrorString( _( "The commit was created, but the list of staged files was left "
                                     "alone, because the staged files changed outside KiCad while "
                                     "this ran." ) );
        return CommitResult::SuccessIndexStale;
    }

    REPUBLISH_RESULT republished = republishIndex( index, staging.committedEntries, staging.selectedPaths );

    if( republished == REPUBLISH_RESULT::CHANGED_EXTERNALLY )
    {
        aHandler->AddErrorString( _( "The commit was created, but the list of staged files was left "
                                     "alone, because another program kept changing it." ) );
        return CommitResult::SuccessIndexStale;
    }

    if( republished == REPUBLISH_RESULT::WRITE_FAILED )
    {
        aHandler->AddGitError( _( "The commit was created, but the list of staged files could not be updated." ) );
        return CommitResult::SuccessIndexStale;
    }

    staging.scratchGuard.armed = false;

    return CommitResult::Success;
}


CommitResult LIBGIT_BACKEND::Amend( GIT_COMMIT_HANDLER* aHandler, const std::vector<wxString>& aFiles,
                                    const wxString& aMessage, const wxString& aAuthorName,
                                    const wxString& aAuthorEmail )
{
    git_repository* repo = aHandler->GetRepo();

    if( !repo )
        return CommitResult::Error;

    if( git_repository_head_unborn( repo ) != 0 )
    {
        aHandler->AddErrorString( _( "Cannot amend: the branch has no commits yet." ) );
        return CommitResult::Error;
    }

    git_reference* headRef = nullptr;

    if( git_repository_head( &headRef, repo ) != 0 )
    {
        aHandler->AddGitError( _( "Could not read the current commit." ) );
        return CommitResult::Error;
    }

    KIGIT::GitReferencePtr headRefPtr( headRef );
    git_commit*            headCommit = nullptr;

    if( git_reference_peel( (git_object**) &headCommit, headRef, GIT_OBJECT_COMMIT ) != 0 )
    {
        aHandler->AddGitError( _( "Could not read the current commit." ) );
        return CommitResult::Error;
    }

    KIGIT::GitCommitPtr headCommitPtr( headCommit );
    COMMIT_STAGING      staging;
    KIGIT_ERROR_MESSAGE stagingError;

    if( !staging.Begin( repo, aFiles, stagingError ) )
    {
        aHandler->AddError( stagingError );
        return CommitResult::Error;
    }

    git_index* index = staging.index;

    // The commit being amended, not its parent, or files it already held would disappear.
    git_tree* headTree = nullptr;

    if( git_commit_tree( &headTree, headCommit ) != 0 )
    {
        aHandler->AddGitError( _( "Could not read the current commit." ) );
        return CommitResult::Error;
    }

    KIGIT::GitTreePtr headTreePtr( headTree );

    if( git_index_read_tree( index, headTree ) != 0 )
    {
        aHandler->AddGitError( _( "Could not prepare the commit." ) );
        return CommitResult::Error;
    }

    KIGIT::GitTreePtr treePtr;

    if( !staging.BuildTree( repo, aFiles, treePtr, stagingError ) )
    {
        aHandler->AddError( stagingError );
        return CommitResult::Error;
    }

    git_tree* tree = treePtr.get();

    git_signature* author = nullptr;

    if( git_signature_now( &author, aAuthorName.utf8_string().c_str(), aAuthorEmail.utf8_string().c_str() ) != 0 )
    {
        aHandler->AddGitError( _( "Could not record the commit author." ) );
        return CommitResult::Error;
    }

    KIGIT::GitSignaturePtr authorPtr( author );
    git_oid                oid;

    if( git_commit_amend( &oid, headCommit, "HEAD", author, author, nullptr, aMessage.utf8_string().c_str(), tree )
        != 0 )
    {
        aHandler->AddGitError( _( "Could not amend the commit." ) );
        return CommitResult::Error;
    }

    // The commit exists, so only the index is left. If anything restaged meanwhile, leave it.
    if( selectedPathsRestaged( index, staging.selectedPaths, staging.selectedAtStart )
        || ( staging.haveIndexAtStart && indexChangedSince( index, staging.indexAtStart ) ) )
    {
        git_index_read( index, 1 );
        aHandler->AddErrorString( _( "The commit was amended, but the list of staged files was left "
                                     "alone, because the staged files changed outside KiCad while "
                                     "this ran." ) );
        return CommitResult::SuccessIndexStale;
    }

    REPUBLISH_RESULT republished = republishIndex( index, staging.committedEntries, staging.selectedPaths );

    if( republished == REPUBLISH_RESULT::CHANGED_EXTERNALLY )
    {
        aHandler->AddErrorString( _( "The commit was amended, but the list of staged files was left "
                                     "alone, because another program kept changing it." ) );
        return CommitResult::SuccessIndexStale;
    }

    if( republished == REPUBLISH_RESULT::WRITE_FAILED )
    {
        aHandler->AddGitError( _( "The commit was amended, but the list of staged files could not be updated." ) );
        return CommitResult::SuccessIndexStale;
    }

    staging.scratchGuard.armed = false;

    return CommitResult::Success;
}


// 1 when a remote-tracking branch already holds aCommit, naming one in aRemoteRef, 0 when none does, negative on a
// libgit2 error
static int reachableFromRemote( git_repository* aRepo, const git_oid& aCommit, wxString& aRemoteRef )
{
    git_reference_iterator* rawIter = nullptr;

    if( int rc = git_reference_iterator_glob_new( &rawIter, aRepo, "refs/remotes/*" ); rc != GIT_OK )
        return rc;

    KIGIT::GitReferenceIteratorPtr iter( rawIter );
    std::vector<git_oid>           tips;
    std::vector<wxString>          names;
    git_reference*                 rawRef = nullptr;
    int                            rc = GIT_OK;

    while( ( rc = git_reference_next( &rawRef, iter.get() ) ) == GIT_OK )
    {
        KIGIT::GitReferencePtr ref( rawRef );

        // origin/HEAD names a branch this loop visits anyway, and may dangle
        if( git_reference_type( ref.get() ) == GIT_REFERENCE_SYMBOLIC )
            continue;

        git_object* rawTip = nullptr;

        if( int peel = git_reference_peel( &rawTip, ref.get(), GIT_OBJECT_COMMIT ); peel != GIT_OK )
            return peel;

        KIGIT::GitObjectPtr tip( rawTip );
        tips.push_back( *git_object_id( tip.get() ) );
        names.push_back( wxString::FromUTF8( git_reference_shorthand( ref.get() ) ) );
    }

    if( rc != GIT_ITEROVER )
        return rc;

    if( tips.empty() )
        return 0;

    if( int reachable = git_graph_reachable_from_any( aRepo, &aCommit, tips.data(), tips.size() ); reachable <= 0 )
        return reachable;

    // One walk answers the common unpublished case, so only a published commit pays to find which branch holds it
    for( size_t ii = 0; ii < tips.size(); ++ii )
    {
        int reachable = git_graph_reachable_from_any( aRepo, &aCommit, &tips[ii], 1 );

        if( reachable < 0 )
            return reachable;

        if( reachable > 0 )
        {
            aRemoteRef = names[ii];
            return 1;
        }
    }

    return 1;
}


UndoCommitResult LIBGIT_BACKEND::UndoLastCommit( GIT_COMMIT_HANDLER* aHandler, const wxString& aExpectedBranch,
                                                 const git_oid& aExpectedHead, wxString& aMessage )
{
    git_repository* repo = aHandler->GetRepo();

    if( !repo )
        return UndoCommitResult::Error;

    git_reference* rawHead = nullptr;

    if( git_reference_lookup( &rawHead, repo, "HEAD" ) != GIT_OK )
    {
        aHandler->AddGitError( _( "Could not read the current commit." ) );
        return UndoCommitResult::Error;
    }

    KIGIT::GitReferencePtr head( rawHead );
    const char*            branchName = git_reference_symbolic_target( head.get() );

    // A detached HEAD has no branch to move back
    if( !branchName || aExpectedBranch.IsEmpty() || wxString::FromUTF8( branchName ) != aExpectedBranch )
        return UndoCommitResult::TargetChanged;

    git_reference* rawBranch = nullptr;
    int            lookup = git_reference_lookup( &rawBranch, repo, branchName );

    if( lookup == GIT_ENOTFOUND )
        return UndoCommitResult::NoCommit;

    if( lookup != GIT_OK )
    {
        aHandler->AddGitError( _( "Could not read the current commit." ) );
        return UndoCommitResult::Error;
    }

    KIGIT::GitReferencePtr branch( rawBranch );
    const git_oid*         tip = git_reference_target( branch.get() );

    if( !tip || !git_oid_equal( tip, &aExpectedHead ) )
        return UndoCommitResult::TargetChanged;

    if( aHandler->IsOperationInProgress() )
    {
        aHandler->AddErrorString( _( "Finish or abandon the current Git operation before undoing a commit." ) );
        return UndoCommitResult::Error;
    }

    git_commit* rawCommit = nullptr;

    if( git_commit_lookup( &rawCommit, repo, &aExpectedHead ) != GIT_OK )
    {
        aHandler->AddGitError( _( "Could not read the current commit." ) );
        return UndoCommitResult::Error;
    }

    KIGIT::GitCommitPtr commit( rawCommit );
    int                 published = reachableFromRemote( repo, aExpectedHead, aMessage );

    if( published < 0 )
    {
        aHandler->AddGitError( _( "Could not check whether the commit was already sent to a remote." ) );
        return UndoCommitResult::Error;
    }

    if( published > 0 )
        return UndoCommitResult::Published;

    int moved = GIT_OK;

    // Both writes fail if the branch moved since it was read, so a commit made meanwhile is never dropped
    if( git_commit_parentcount( commit.get() ) == 0 )
    {
        // git_reference_delete drops the reflog before comparing, so compare under the ref lock instead
        git_transaction* rawTransaction = nullptr;
        moved = git_transaction_new( &rawTransaction, repo );

        std::unique_ptr<git_transaction, decltype( &git_transaction_free )> transaction( rawTransaction,
                                                                                       git_transaction_free );
        git_oid current;

        if( moved == GIT_OK )
            moved = git_transaction_lock_ref( transaction.get(), branchName );

        if( moved == GIT_OK )
            moved = git_reference_name_to_id( &current, repo, branchName );

        if( moved == GIT_OK && !git_oid_equal( &current, &aExpectedHead ) )
            moved = GIT_EMODIFIED;

        if( moved == GIT_OK )
            moved = git_transaction_remove( transaction.get(), branchName );

        if( moved == GIT_OK )
            moved = git_transaction_commit( transaction.get() );
    }
    else
    {
        git_reference* rawMoved = nullptr;
        moved = git_reference_create_matching( &rawMoved, repo, branchName, git_commit_parent_id( commit.get(), 0 ), 1,
                                               &aExpectedHead, "undo: moving to parent" );
        git_reference_free( rawMoved );
    }

    if( moved == GIT_EMODIFIED )
        return UndoCommitResult::TargetChanged;

    if( moved != GIT_OK )
    {
        aHandler->AddGitError( _( "Could not undo the commit." ) );
        return UndoCommitResult::Error;
    }

    aMessage = wxString::FromUTF8( git_commit_message( commit.get() ) );
    return UndoCommitResult::Success;
}


PushResult LIBGIT_BACKEND::Push( GIT_PUSH_HANDLER* aHandler, bool aForce )
{
    KIGIT_COMMON*                common = aHandler->GetCommon();
    std::unique_lock<std::mutex> lock = aHandler->GetActionLock();

    if( !lock.owns_lock() )
    {
        wxLogTrace( traceGit, "GIT_PUSH_HANDLER::PerformPush: Could not lock mutex" );
        return PushResult::Busy;
    }

    // Publish cancellation only after taking the action lock. A cancelled or closing project
    // must not start network work when a preceding read finally releases the repository.
    KIGIT_COMMON::OPERATION_GUARD operation( common, aHandler->CancelToken() );

    if( aHandler->IsThisOperationCancelled() || !aHandler->GetRepo() )
        return PushResult::Error;

    const wxString targetChanged = _( "The branch or its last commit changed while the push was waiting. "
                                      "Review the changes and try again." );

    auto matchesExpectedTarget = [aHandler]( git_reference* aHead )
    {
        if( !aHandler->HasExpectedTarget() )
            return true;

        if( !aHead || !git_reference_is_branch( aHead ) )
            return false;

        const git_oid* oid = git_reference_target( aHead );
        wxString       branch = wxString::FromUTF8( git_reference_shorthand( aHead ) );
        wxString       commit = oid ? wxString::FromUTF8( git_oid_tostr_s( oid ) ) : wxString();
        return aHandler->MatchesExpectedTarget( branch, commit );
    };

    // Reject a changed confirmation before connecting to the remote. The target is checked
    // again when selecting the ref below, because another program can update Git during a connect.
    if( aHandler->HasExpectedTarget() )
    {
        git_reference* expectedHead = nullptr;
        git_repository_head( &expectedHead, aHandler->GetRepo() );
        KIGIT::GitReferencePtr expectedHeadPtr( expectedHead );

        if( !matchesExpectedTarget( expectedHead ) )
        {
            aHandler->AddErrorString( targetChanged );
            return PushResult::Error;
        }
    }

    PushResult result = PushResult::Success;

    wxString    remoteName = common->GetRemoteNameOrDefault();
    std::string remoteNameUtf8 = remoteName.utf8_string();
    git_remote* remote = nullptr;

    if( int rc = git_remote_lookup( &remote, aHandler->GetRepo(), remoteNameUtf8.c_str() ); rc != GIT_OK )
    {
        reportRemoteLookupFailure( aHandler, rc, remoteName );
        return PushResult::Error;
    }

    KIGIT::GitRemotePtr remotePtr(remote);

    // The URL may have changed outside KiCad, and libgit2 connects to the push URL when there is one
    const char* pushUrl = git_remote_pushurl( remote ) ? git_remote_pushurl( remote ) : git_remote_url( remote );

    if( pushUrl )
        common->SyncRemoteUrl( wxString::FromUTF8( pushUrl ) );

    git_remote_callbacks remoteCallbacks;
    git_remote_init_callbacks( &remoteCallbacks, GIT_REMOTE_CALLBACKS_VERSION );
    remoteCallbacks.sideband_progress = progress_cb;
    remoteCallbacks.transfer_progress = transfer_progress_cb;
    remoteCallbacks.update_tips = update_cb;
    remoteCallbacks.push_transfer_progress = push_transfer_progress_cb;
    remoteCallbacks.push_update_reference = push_update_reference_cb;
    remoteCallbacks.credentials = credentials_cb;
    remoteCallbacks.certificate_check = certificate_check_cb;
    remoteCallbacks.payload = aHandler;

    git_proxy_options proxyOpts;
    git_proxy_init_options( &proxyOpts, GIT_PROXY_OPTIONS_VERSION );
    proxyOpts.type = GIT_PROXY_AUTO;
    // Cancelled while this task was still queued, so it must not start network work.
    if( aHandler->IsThisOperationCancelled() )
        return PushResult::Error;

    aHandler->ResetTransferState();

    if( int connectResult = git_remote_connect( remote, GIT_DIRECTION_PUSH, &remoteCallbacks, &proxyOpts, nullptr ) )
    {
        aHandler->ReportTransferFailure( KIGIT_COMMON::GetLastGitErrorInfo( connectResult ), KIGIT_OPERATION::PUSH,
                                         remoteName );
        return PushResult::Error;
    }

    aHandler->RememberWorkingSSHKey();

    git_push_options pushOptions;
    git_push_init_options( &pushOptions, GIT_PUSH_OPTIONS_VERSION );
    pushOptions.callbacks = remoteCallbacks;
    pushOptions.proxy_opts.type = GIT_PROXY_AUTO;

    git_reference* head = nullptr;

    if( git_repository_head( &head, aHandler->GetRepo() ) != 0 )
    {
        git_remote_disconnect( remote );
        aHandler->AddErrorString( _( "Could not get the repository head." ) );
        return PushResult::Error;
    }

    KIGIT::GitReferencePtr headPtr( head );

    // A detached HEAD is not a branch, so its name is the literal "HEAD" and the remote has no
    // branch for it. Without this the push fails with nothing to show the user.
    if( !git_reference_is_branch( head ) )
    {
        git_remote_disconnect( remote );
        aHandler->AddErrorString( _( "This project is showing an old snapshot rather than a branch, so there "
                                     "is nothing to push. Go back to a branch first." ) );
        return PushResult::Error;
    }

    if( !matchesExpectedTarget( head ) )
    {
        git_remote_disconnect( remote );
        aHandler->AddErrorString( targetChanged );
        return PushResult::Error;
    }

    // Force push prepends "+" to the refspec source, matching `git push --force`.
    wxString           refspec = ( aForce ? wxS( "+" ) : wxS( "" ) ) + wxString( git_reference_name( head ) );
    std::string        refspecUtf8 = refspec.utf8_string();
    const char*        refs[1] = { refspecUtf8.c_str() };
    const git_strarray refspecs = { (char**) refs, 1 };

    if( int pushResult = git_remote_push( remote, &refspecs, &pushOptions ) )
    {
        aHandler->ReportTransferFailure( KIGIT_COMMON::GetLastGitErrorInfo( pushResult ), KIGIT_OPERATION::PUSH,
                                         remoteName );
        git_remote_disconnect( remote );

        // The return code, not the message, which varies between libgit2 versions.
        return pushResult == GIT_ENONFASTFORWARD ? PushResult::NonFastForward : PushResult::Error;
    }

    // First push from this branch: point it at where we just pushed (git push -u).
    if( git_reference_is_branch( head ) )
    {
        git_reference* upstreamRef = nullptr;
        int            rc = git_branch_upstream( &upstreamRef, head );

        if( rc == GIT_ENOTFOUND )
        {
            wxString upstreamName = wxString::Format( "%s/%s", remoteName, git_reference_shorthand( head ) );
            git_branch_set_upstream( head, upstreamName.utf8_string().c_str() );
        }
        else if( rc == GIT_OK )
        {
            KIGIT::GitReferencePtr upstreamPtr( upstreamRef );
        }
    }

    git_remote_disconnect( remote );

    return result;
}


std::map<wxString, FileStatus> LIBGIT_BACKEND::GetFileStatus( GIT_STATUS_HANDLER* aHandler, const wxString& aPathspec,
                                                              bool aChangedOnly )
{
    std::map<wxString, FileStatus> fileStatusMap;
    git_repository* repo = aHandler->GetRepo();

    if( !repo )
        return fileStatusMap;

    git_status_options status_options;
    git_status_init_options( &status_options, GIT_STATUS_OPTIONS_VERSION );
    status_options.show = GIT_STATUS_SHOW_INDEX_AND_WORKDIR;
    status_options.flags = GIT_STATUS_OPT_INCLUDE_UNTRACKED | GIT_STATUS_OPT_RECURSE_UNTRACKED_DIRS;

    // Per-file icons need an entry for every tracked file, so an icon clears when a file goes
    // back to unmodified. Listing only changes does not, and costs a walk of the whole repo.
    if( !aChangedOnly )
        status_options.flags |= GIT_STATUS_OPT_INCLUDE_UNMODIFIED;

    std::string pathspec_str;
    std::vector<const char*> pathspec_ptrs;

    if( !aPathspec.IsEmpty() )
    {
        pathspec_str = aPathspec.utf8_string();
        pathspec_ptrs.push_back( pathspec_str.c_str() );

        status_options.pathspec.strings = const_cast<char**>( pathspec_ptrs.data() );
        status_options.pathspec.count = pathspec_ptrs.size();

        // Callers pass a folder, not a pattern. Without this a folder name holding a glob
        // character would match its neighbours instead of itself.
        status_options.flags |= GIT_STATUS_OPT_DISABLE_PATHSPEC_MATCH;
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
        fileStatusMap[absPath] = FileStatus{ absPath,
                                             aHandler->ConvertStatus( entry->status ),
                                             static_cast<unsigned int>( entry->status ) };
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
                fileStatus.status = KIGIT_COMMON::GIT_STATUS::GIT_STATUS_AHEAD;
            else if( aRemoteChanges.count( relativePathStd ) )
                fileStatus.status = KIGIT_COMMON::GIT_STATUS::GIT_STATUS_BEHIND;
        }
    }
}


wxString LIBGIT_BACKEND::GetWorkingDirectory( GIT_STATUS_HANDLER* aHandler )
{
    return aHandler->GetProjectDir();
}


bool LIBGIT_BACKEND::GetConfigString( GIT_CONFIG_HANDLER* aHandler, const wxString& aKey, wxString& aValue )
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
    int                      result = git_config_get_entry( &entry, config, aKey.utf8_string().c_str() );
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
    int             error = git_repository_open( &repo, aPath.utf8_string().c_str() );

    if( error == 0 )
    {
        git_repository_free( repo );
        return true;
    }

    // A repository git refuses to open is still a repository. Answering false here sends the
    // initialize path into reinitializing it.
    return error == GIT_EOWNER;
}


// Names the unborn branch Add to Version Control created, so a fetch can tell it from a later orphan
static const char ADOPT_REMOTE_BRANCH_KEY[] = "kicad.adoptRemoteBranch";


static wxString unbornBranchName( git_repository* aRepo )
{
    git_reference* head = nullptr;
    wxString       name;

    if( git_repository_head_unborn( aRepo ) != 1 || git_reference_lookup( &head, aRepo, "HEAD" ) != GIT_OK )
        return name;

    KIGIT::GitReferencePtr headPtr( head );

    if( const char* target = git_reference_symbolic_target( head ) )
        wxString::FromUTF8( target ).StartsWith( wxS( "refs/heads/" ), &name );

    return name;
}


static bool markForRemoteBranchAdoption( git_repository* aRepo )
{
    const wxString branch = unbornBranchName( aRepo );
    git_config*    config = nullptr;
    git_config*    local = nullptr;

    if( branch.IsEmpty() || git_repository_config( &config, aRepo ) != GIT_OK )
        return false;

    KIGIT::GitConfigPtr configPtr( config );

    if( git_config_open_level( &local, config, GIT_CONFIG_LEVEL_LOCAL ) != GIT_OK )
        return false;

    KIGIT::GitConfigPtr localPtr( local );

    return git_config_set_string( local, ADOPT_REMOTE_BRANCH_KEY, branch.utf8_str() ) == GIT_OK;
}


InitResult LIBGIT_BACKEND::InitializeRepository( GIT_INIT_HANDLER* aHandler, const wxString& aPath )
{
    if( IsRepository( aHandler, aPath ) )
    {
        return InitResult::AlreadyExists;
    }

    git_repository* repo = nullptr;

    if( git_repository_init( &repo, aPath.utf8_string().c_str(), 0 ) != GIT_OK )
    {
        if( repo )
            git_repository_free( repo );

        aHandler->AddError( wxString::Format( _( "Could not set up version control in %s." ), aPath ),
                            KIGIT_COMMON::GetLastGitError() );
        return InitResult::Error;
    }

    aHandler->GetCommon()->SetRepo( repo );

    if( !markForRemoteBranchAdoption( repo ) )
    {
        aHandler->AddGitError( wxString::Format( _( "Could not set up version control in %s." ), aPath ) );
        return InitResult::Error;
    }

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

    KIGIT_COMMON::REMOTE_URL parsed = KIGIT_COMMON::ParseRemoteURL( aConfig.url );
    git_remote*              remote = nullptr;
    int                      error;

    if( git_remote_lookup( &remote, repo, "origin" ) == GIT_OK )
    {
        KIGIT::GitRemotePtr remotePtr( remote );
        error = git_remote_set_url( repo, "origin", parsed.url.utf8_str() );
    }
    else
    {
        error = git_remote_create_with_fetchspec( &remote, repo, "origin", parsed.url.utf8_str(),
                                                  "+refs/heads/*:refs/remotes/origin/*" );
        KIGIT::GitRemotePtr remotePtr( remote );
    }

    if( error != GIT_OK )
    {
        KIGIT_FAILURE_CONTEXT context;
        context.operation = KIGIT_OPERATION::SET_UP_REMOTE;
        context.remoteName = wxS( "origin" );
        context.url = parsed.url;

        KIGIT_FAILURE_REPORT report =
                KIGIT::DescribeGitFailure( KIGIT_COMMON::GetLastGitErrorInfo( error ), {}, context );
        aHandler->AddError( report.summary, report.details );
        return false;
    }

    // SetRemote clears cached credentials, so apply the accepted credentials afterwards.
    // The username embedded in the address takes precedence over a saved username.
    KIGIT_COMMON* common = aHandler->GetCommon();
    common->SetRemote( parsed.url );
    common->SetUsername( parsed.username.IsEmpty() ? aConfig.username : parsed.username );
    common->SetPassword( aConfig.password.IsEmpty() ? parsed.password : aConfig.password );
    common->SetSSHKey( aConfig.sshKey );

    wxLogTrace( traceGit, "Successfully set up remote origin" );
    return true;
}


// The short-name lookup's code, since it is the one that tells a missing branch from a bad name
static int lookup_branch_reference( git_repository* repo, const wxString& aBranchName, git_reference** aReference )
{
    if( git_reference_lookup( aReference, repo, aBranchName.utf8_string().c_str() ) == GIT_OK )
        return GIT_OK;

    return git_reference_dwim( aReference, repo, aBranchName.utf8_string().c_str() );
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

    if( int rc = lookup_branch_reference( repo, aBranchName, &branchRef ); rc != GIT_OK )
    {
        if( rc == GIT_ENOTFOUND )
        {
            aHandler->AddError( wxString::Format( _( "The branch '%s' no longer exists." ), aBranchName ),
                                wxEmptyString );
            return BranchResult::BranchNotFound;
        }

        aHandler->AddError( wxString::Format( _( "Could not open the branch '%s'." ), aBranchName ),
                            KIGIT_COMMON::GetLastGitError() );
        return BranchResult::Error;
    }

    KIGIT::GitReferencePtr branchRefPtr( branchRef );

    // A remote-tracking branch is a view of someone else's work rather than somewhere to
    // stand. Checking one out can only attach HEAD to a local branch of the same name, which
    // may sit at another commit, leaving HEAD and the working tree describing different things.
    if( git_reference_is_remote( branchRef ) )
    {
        aHandler->AddErrorString( wxString::Format( _( "'%s' is a branch on the remote, so the project "
                                                       "cannot be switched to it. Make a local branch from "
                                                       "it instead." ),
                                                    aBranchName ) );
        return BranchResult::Error;
    }

    const char* branchRefName = git_reference_name( branchRef );
    git_object* branchObj = nullptr;

    if( git_revparse_single( &branchObj, repo, aBranchName.utf8_string().c_str() ) != GIT_OK )
    {
        aHandler->AddError( wxString::Format( _( "Could not open the branch '%s'." ), aBranchName ),
                            KIGIT_COMMON::GetLastGitError() );
        return BranchResult::Error;
    }

    KIGIT::GitObjectPtr branchObjPtr( branchObj );


    git_checkout_options checkoutOpts;
    git_checkout_init_options( &checkoutOpts, GIT_CHECKOUT_OPTIONS_VERSION );
    checkoutOpts.checkout_strategy = GIT_CHECKOUT_SAFE;

    std::vector<wxString> conflictPaths;

    checkoutOpts.notify_flags = GIT_CHECKOUT_NOTIFY_CONFLICT;
    checkoutOpts.notify_payload = &conflictPaths;
    checkoutOpts.notify_cb = []( git_checkout_notify_t aWhy, const char* aPath, const git_diff_file*,
                                 const git_diff_file*, const git_diff_file*, void* aPayload ) -> int
    {
        if( aWhy == GIT_CHECKOUT_NOTIFY_CONFLICT && aPath )
            static_cast<std::vector<wxString>*>( aPayload )->push_back( wxString::FromUTF8( aPath ) );

        return 0;
    };

    if( int rc = git_checkout_tree( repo, branchObj, &checkoutOpts ) )
    {
        // A stash does not cover never-committed files, so those need their own advice.
        if( rc == GIT_ECONFLICT )
        {
            wxString fileList;
            bool     anyUntracked = false;
            size_t   shown = std::min<size_t>( conflictPaths.size(), 10 );

            for( size_t i = 0; i < shown; i++ )
            {
                bool tracked = IsPathInHead( repo, conflictPaths[i] );

                anyUntracked |= !tracked;
                wxString tag = tracked ? wxString() : wxS( " " ) + _( "(never committed)" );

                fileList << conflictPaths[i] << tag << wxS( "\n" );
            }

            if( conflictPaths.size() > shown )
                fileList << wxString::Format( _( "and %zu more\n" ), conflictPaths.size() - shown );

            wxString advice = anyUntracked ? _( "Commit or stash your changes first. Files that were never "
                                                "committed are not included in a stash, so commit or discard "
                                                "those from the Version Control panel." )
                                           : _( "Commit or stash your changes first." );

            aHandler->AddErrorString( wxString::Format( _( "Switching to branch '%s' would overwrite:\n\n"
                                                           "%s\n%s" ),
                                                        aBranchName, fileList, advice ) );
            return BranchResult::Conflict;
        }

        aHandler->AddError( wxString::Format( _( "Could not switch to the branch '%s'." ), aBranchName ),
                            KIGIT_COMMON::GetLastGitError() );
        return BranchResult::CheckoutFailed;
    }

    if( git_repository_set_head( repo, branchRefName ) != GIT_OK )
    {
        aHandler->AddError( wxString::Format( _( "Could not switch to the branch '%s'." ), aBranchName ),
                            KIGIT_COMMON::GetLastGitError() );
        return BranchResult::Error;
    }

    wxLogTrace( traceGit, "Successfully switched to branch '%s'", aBranchName );
    return BranchResult::Success;
}


// Use callbacks declared/implemented in kicad_git_common.h/.cpp

// A local default of master would otherwise push beside a server created with main. False means
// the move was due and failed
static bool adoptRemoteBranch( git_repository* aRepo, const wxString& aRemoteName,
                               const std::vector<wxString>& aRemoteBranches )
{
    git_config* config = nullptr;
    git_config* local = nullptr;

    if( git_repository_config( &config, aRepo ) != GIT_OK )
        return true;

    KIGIT::GitConfigPtr configPtr( config );

    if( git_config_open_level( &local, config, GIT_CONFIG_LEVEL_LOCAL ) != GIT_OK )
        return true;

    KIGIT::GitConfigPtr localPtr( local );

    git_buf marked = GIT_BUF_INIT;

    if( git_config_get_string_buf( &marked, local, ADOPT_REMOTE_BRANCH_KEY ) != GIT_OK )
        return true;

    KIGIT::GitBufPtr markedPtr( &marked );
    const wxString   localName = unbornBranchName( aRepo );

    git_branch_iterator* iterator = nullptr;
    git_reference*       ref = nullptr;
    git_branch_t         type;

    if( git_branch_iterator_new( &iterator, aRepo, GIT_BRANCH_LOCAL ) != GIT_OK )
        return false;

    KIGIT::GitBranchIteratorPtr iteratorPtr( iterator );
    const int                   rc = git_branch_next( &ref, &type, iterator );
    KIGIT::GitReferencePtr      refPtr( rc == GIT_OK ? ref : nullptr );

    if( rc != GIT_OK && rc != GIT_ITEROVER )
        return false;

    git_reflog* reflog = nullptr;

    if( git_reflog_read( &reflog, aRepo, "HEAD" ) != GIT_OK )
        return false;

    std::unique_ptr<git_reflog, decltype( &git_reflog_free )> reflogPtr( reflog, git_reflog_free );

    // A commit, any branch, or HEAD history means the repository has moved on since it was made
    if( rc == GIT_OK || git_reflog_entrycount( reflog ) != 0 || localName.IsEmpty()
        || localName != wxString::FromUTF8( marked.ptr ) )
    {
        git_config_delete_entry( local, ADOPT_REMOTE_BRANCH_KEY );
        return true;
    }

    // An empty remote keeps the question open until the first push gives it a branch
    if( aRemoteBranches.empty() )
        return true;

    // Only a single remote branch says unambiguously where the first commit belongs
    if( aRemoteBranches.size() != 1 || aRemoteBranches.front() == localName )
    {
        git_config_delete_entry( local, ADOPT_REMOTE_BRANCH_KEY );
        return true;
    }

    const wxString   branch = aRemoteBranches.front();
    const wxString   section = wxS( "branch." ) + branch;
    git_transaction* transaction = nullptr;

    if( git_config_lock( &transaction, local ) != GIT_OK )
        return false;

    std::unique_ptr<git_transaction, decltype( &git_transaction_free )> transactionPtr( transaction,
                                                                                        git_transaction_free );

    // Upstream first, so a failure leaves HEAD where it was rather than on a branch with half its tracking
    if( git_config_set_string( local, ( section + wxS( ".remote" ) ).utf8_str(), aRemoteName.utf8_str() ) != GIT_OK
        || git_config_set_string( local, ( section + wxS( ".merge" ) ).utf8_str(),
                                  ( wxS( "refs/heads/" ) + branch ).utf8_str() )
                   != GIT_OK
        || git_transaction_commit( transaction ) != GIT_OK
        || git_repository_set_head( aRepo, ( wxS( "refs/heads/" ) + branch ).utf8_str() ) != GIT_OK )
    {
        wxLogTrace( traceGit, "adoptRemoteBranch: could not move %s to %s: %s", localName, branch,
                    KIGIT_COMMON::GetLastGitError() );
        return false;
    }

    git_config_delete_entry( local, ADOPT_REMOTE_BRANCH_KEY );
    wxLogTrace( traceGit, "adoptRemoteBranch: first commit goes to %s instead of %s", branch, localName );
    return true;
}


// aRemoteHasBranches, when given, is false only when the server advertised no branch at all
static bool fetchRemote( GIT_PULL_HANDLER* aHandler, bool aSkipLock, bool* aRemoteHasBranches )
{
    // A pull already holds the lock when it fetches, and relocking from the owner is undefined.
    std::unique_lock<std::mutex> lock;

    if( !aSkipLock )
    {
        lock = aHandler->GetActionLock();

        if( !lock.owns_lock() )
        {
            wxLogTrace( traceGit, "GIT_PULL_HANDLER::PerformFetch() - Could not lock mutex" );
            aHandler->AddErrorString( KIGIT::BusyMessage() );
            return false;
        }
    }

    // A nested fetch must not republish or retire the pull's token while the pull still runs.
    std::optional<KIGIT_COMMON::OPERATION_GUARD> operation;

    if( !aSkipLock )
        operation.emplace( aHandler->GetCommon(), aHandler->CancelToken() );

    // A close or cancellation while waiting also applies before reading remote configuration.
    if( aHandler->IsThisOperationCancelled() || !aHandler->GetRepo() )
        return false;

    wxString    remoteName = aHandler->GetCommon()->GetRemoteNameOrDefault();
    std::string remoteNameUtf8 = remoteName.utf8_string();
    git_remote* remote = nullptr;

    if( int rc = git_remote_lookup( &remote, aHandler->GetRepo(), remoteNameUtf8.c_str() ); rc != GIT_OK )
    {
        wxLogTrace( traceGit, "GIT_PULL_HANDLER::PerformFetch() - Failed to lookup remote '%s'", remoteName );
        reportRemoteLookupFailure( aHandler, rc, remoteName );
        return false;
    }

    KIGIT::GitRemotePtr remotePtr( remote );

    // The URL may have changed outside KiCad, so the credentials follow what libgit2 connects to
    if( const char* url = git_remote_url( remote ) )
        aHandler->GetCommon()->SyncRemoteUrl( wxString::FromUTF8( url ) );

    git_remote_callbacks remoteCallbacks;
    git_remote_init_callbacks( &remoteCallbacks, GIT_REMOTE_CALLBACKS_VERSION );
    remoteCallbacks.sideband_progress = progress_cb;
    remoteCallbacks.transfer_progress = transfer_progress_cb;
    remoteCallbacks.credentials = credentials_cb;
    remoteCallbacks.certificate_check = certificate_check_cb;
    remoteCallbacks.payload = aHandler;

    git_proxy_options proxyOpts;
    git_proxy_init_options( &proxyOpts, GIT_PROXY_OPTIONS_VERSION );
    proxyOpts.type = GIT_PROXY_AUTO;

    // Do not SetShuttingDown( false ) here. A close or a preference change can raise the flag
    // after this fetch starts, and clearing it would strand the caller on the transfer.

    // Cancelled while this task was still queued, so it must not start network work.
    if( aHandler->IsThisOperationCancelled() )
        return false;

    aHandler->ResetTransferState();

    if( int rc = git_remote_connect( remote, GIT_DIRECTION_FETCH, &remoteCallbacks, &proxyOpts, nullptr ) )
    {
        aHandler->ReportTransferFailure( KIGIT_COMMON::GetLastGitErrorInfo( rc ), KIGIT_OPERATION::FETCH, remoteName );
        return false;
    }

    aHandler->RememberWorkingSSHKey();

    const git_remote_head** heads = nullptr;
    size_t                  headCount = 0;
    std::vector<wxString>   remoteBranches;

    // The advertisement, not local tracking refs, because a custom refspec or stale refs mislead those
    if( git_remote_ls( &heads, &headCount, remote ) == GIT_OK )
    {
        for( size_t ii = 0; ii < headCount; ++ii )
        {
            if( std::strncmp( heads[ii]->name, "refs/heads/", 11 ) == 0 )
                remoteBranches.push_back( wxString::FromUTF8( heads[ii]->name + 11 ) );
        }
    }

    if( aRemoteHasBranches )
        *aRemoteHasBranches = !remoteBranches.empty();

    git_fetch_options fetchOptions;
    git_fetch_init_options( &fetchOptions, GIT_FETCH_OPTIONS_VERSION );
    fetchOptions.callbacks = remoteCallbacks;

    if( int rc = git_remote_fetch( remote, nullptr, &fetchOptions, nullptr ) )
    {
        aHandler->ReportTransferFailure( KIGIT_COMMON::GetLastGitErrorInfo( rc ), KIGIT_OPERATION::FETCH, remoteName );
        return false;
    }

    if( !adoptRemoteBranch( aHandler->GetRepo(), remoteName, remoteBranches ) )
    {
        aHandler->AddGitError( wxString::Format( _( "Fetched from '%s', but could not switch the new repository "
                                                    "to the remote's branch." ),
                                                 remoteName ) );
        return false;
    }

    wxLogTrace( traceGit, "GIT_PULL_HANDLER::PerformFetch() - Fetch completed successfully" );
    return true;
}


bool LIBGIT_BACKEND::PerformFetch( GIT_PULL_HANDLER* aHandler, bool aSkipLock )
{
    return fetchRemote( aHandler, aSkipLock, nullptr );
}


static bool rollbackWorkingTree( git_repository* aRepo, git_commit* aHeadCommit, git_tree* aWrittenTree );
static bool restorePathsFromCommit( git_repository* aRepo, git_commit* aCommit,
                                    const std::vector<std::string>& aPaths );


// A failed checkout leaves the cached index ahead of disk, which reads as phantom staged changes.
static void refreshCachedIndex( git_repository* aRepo )
{
    git_index* index = nullptr;

    if( git_repository_index( &index, aRepo ) == GIT_OK )
    {
        KIGIT::GitIndexPtr indexPtr( index );
        git_index_read( index, 1 );
    }
}


enum class TREE_STATE
{
    CLEAN,
    DIRTY,
    UNKNOWN
};


// After a failed operation under the action lock, this is the set that operation disturbed.
static std::vector<std::string> changedPaths( git_repository* aRepo )
{
    std::vector<std::string> paths;

    git_status_options statusOpts;
    git_status_init_options( &statusOpts, GIT_STATUS_OPTIONS_VERSION );
    statusOpts.show = GIT_STATUS_SHOW_INDEX_AND_WORKDIR;

    git_status_list* statusList = nullptr;

    if( git_status_list_new( &statusList, aRepo, &statusOpts ) != GIT_OK )
        return paths;

    KIGIT::GitStatusListPtr statusListPtr( statusList );

    for( size_t ii = 0; ii < git_status_list_entrycount( statusList ); ii++ )
    {
        const git_status_entry* entry = git_status_byindex( statusList, ii );

        if( entry->head_to_index && entry->head_to_index->new_file.path )
            paths.emplace_back( entry->head_to_index->new_file.path );
        else if( entry->index_to_workdir && entry->index_to_workdir->new_file.path )
            paths.emplace_back( entry->index_to_workdir->new_file.path );
    }

    return paths;
}


// Tracked files only. Callers read CLEAN as permission to roll back over the tree, so an
// unreadable status gets its own answer rather than passing as clean.
static TREE_STATE trackedTreeState( git_repository* aRepo )
{
    git_status_options statusOpts;
    git_status_init_options( &statusOpts, GIT_STATUS_OPTIONS_VERSION );
    statusOpts.show = GIT_STATUS_SHOW_INDEX_AND_WORKDIR;

    git_status_list* statusList = nullptr;

    if( git_status_list_new( &statusList, aRepo, &statusOpts ) != GIT_OK )
        return TREE_STATE::UNKNOWN;

    KIGIT::GitStatusListPtr statusListPtr( statusList );

    return git_status_list_entrycount( statusList ) > 0 ? TREE_STATE::DIRTY : TREE_STATE::CLEAN;
}


// libgit2 refuses these with a count and no name, so the caller names them.
static bool untrackedFilesOverwrittenBy( git_repository* aRepo, git_tree* aIncomingTree,
                                         std::vector<wxString>& aCollisions )
{
    git_status_options statusOpts;
    git_status_init_options( &statusOpts, GIT_STATUS_OPTIONS_VERSION );
    statusOpts.show = GIT_STATUS_SHOW_WORKDIR_ONLY;
    statusOpts.flags = GIT_STATUS_OPT_INCLUDE_UNTRACKED | GIT_STATUS_OPT_RECURSE_UNTRACKED_DIRS;

    git_status_list* statusList = nullptr;

    // An unreadable status must not read as nothing in the way, or the checkout overwrites it.
    if( git_status_list_new( &statusList, aRepo, &statusOpts ) != GIT_OK )
        return false;

    KIGIT::GitStatusListPtr statusListPtr( statusList );
    size_t                  count = git_status_list_entrycount( statusList );

    for( size_t ii = 0; ii < count; ++ii )
    {
        const git_status_entry* entry = git_status_byindex( statusList, ii );

        if( !( entry->status & GIT_STATUS_WT_NEW ) || !entry->index_to_workdir )
            continue;

        const char*     path = entry->index_to_workdir->new_file.path;
        git_tree_entry* treeEntry = nullptr;

        if( path && git_tree_entry_bypath( &treeEntry, aIncomingTree, path ) == GIT_OK )
        {
            KIGIT::GitTreeEntryPtr treeEntryPtr( treeEntry );
            aCollisions.push_back( wxString::FromUTF8( path ) );
        }
    }

    return true;
}


static wxString describeUntrackedCollisions( const std::vector<wxString>& aPaths )
{
    wxString listed;

    for( const wxString& path : aPaths )
        listed += wxS( "\n    " ) + path;

    return wxString::Format( _( "Getting changes would overwrite files that were never committed:%s\n\n"
                                "Commit or discard them first." ),
                             listed );
}


// A local repository pointed at a remote with history has nothing to merge from, so adopt it.
static PullResult adoptRemoteHistory( GIT_PULL_HANDLER* aHandler, const git_oid& aRemoteOid )
{
    git_repository* repo = aHandler->GetRepo();
    git_reference*  symbolic = nullptr;
    std::string     branchRefName;

    if( git_reference_lookup( &symbolic, repo, "HEAD" ) == GIT_OK )
    {
        KIGIT::GitReferencePtr symbolicPtr( symbolic );

        if( const char* target = git_reference_symbolic_target( symbolic ) )
            branchRefName = target;
    }

    if( branchRefName.empty() )
    {
        aHandler->AddErrorString( _( "Could not get the repository head." ) );
        return PullResult::Error;
    }

    git_commit* commit = nullptr;

    if( git_commit_lookup( &commit, repo, &aRemoteOid ) != GIT_OK )
    {
        aHandler->AddGitError( _( "Could not read the fetched commit." ) );
        return PullResult::Error;
    }

    KIGIT::GitCommitPtr commitPtr( commit );
    git_tree*           tree = nullptr;

    if( git_commit_tree( &tree, commit ) != GIT_OK )
    {
        aHandler->AddGitError( _( "Could not read the fetched commit." ) );
        return PullResult::Error;
    }

    KIGIT::GitTreePtr     treePtr( tree );
    std::vector<wxString> collisions;

    if( !untrackedFilesOverwrittenBy( repo, tree, collisions ) )
    {
        aHandler->AddErrorString( _( "Could not read the status of the project files, so getting changes was "
                                     "stopped. Files that were never committed could have been overwritten." ) );
        return PullResult::Error;
    }

    if( !collisions.empty() )
    {
        aHandler->AddErrorString( describeUntrackedCollisions( collisions ) );
        return PullResult::DirtyWorkdir;
    }

    // The reference goes first, because deleting one just created restores the unborn state,
    // while a checkout against no previous commit has nothing to undo from.
    git_reference* branchRef = nullptr;

    wxString shorthand = wxString::FromUTF8( branchRefName );
    shorthand.StartsWith( wxS( "refs/heads/" ), &shorthand );

    if( git_reference_create( &branchRef, repo, branchRefName.c_str(), &aRemoteOid, 0, "pull: adopt remote history" )
        != GIT_OK )
    {
        aHandler->AddError( wxString::Format( _( "Could not create the branch '%s'." ), shorthand ),
                            KIGIT_COMMON::GetLastGitError() );
        return PullResult::Error;
    }

    KIGIT::GitReferencePtr branchRefPtr( branchRef );

    git_checkout_options checkoutOpts;
    git_checkout_options_init( &checkoutOpts, GIT_CHECKOUT_OPTIONS_VERSION );
    checkoutOpts.checkout_strategy = GIT_CHECKOUT_SAFE | GIT_CHECKOUT_RECREATE_MISSING;

    if( git_checkout_tree( repo, reinterpret_cast<git_object*>( tree ), &checkoutOpts ) != GIT_OK )
    {
        wxString gitError = KIGIT_COMMON::GetLastGitError();
        bool     restored = git_reference_delete( branchRef ) == GIT_OK;

        refreshCachedIndex( repo );

        aHandler->AddError( restored ? _( "Could not check out the remote branch, so nothing was changed." )
                                     : _( "Could not check out the remote branch, and the project may be partly "
                                          "changed." ),
                            gitError );
        return PullResult::Error;
    }

    wxString upstream = wxString::Format( "%s/%s", aHandler->GetCommon()->GetRemoteNameOrDefault(), shorthand );
    git_branch_set_upstream( branchRef, upstream.utf8_string().c_str() );

    return PullResult::FastForward;
}


PullResult LIBGIT_BACKEND::PerformPull( GIT_PULL_HANDLER* aHandler )
{
    PullResult result = PullResult::Success;
    std::unique_lock<std::mutex> lock = aHandler->GetActionLock();

    if( !lock.owns_lock() )
    {
        wxLogTrace( traceGit, "GIT_PULL_HANDLER::PerformPull() - Could not lock mutex" );
        aHandler->AddErrorString( KIGIT::BusyMessage() );
        return PullResult::Error;
    }

    // The pull owns cancellation for the whole operation, the fetch inside it included.
    KIGIT_COMMON::OPERATION_GUARD operation( aHandler->GetCommon(), aHandler->CancelToken() );

    if( aHandler->IsThisOperationCancelled() || !aHandler->GetRepo() )
        return PullResult::Error;

    bool remoteHasBranches = true;

    if( !fetchRemote( aHandler, true, &remoteHasBranches ) )
        return PullResult::Error;

    git_oid pull_merge_oid = {};

    if( git_repository_fetchhead_foreach( aHandler->GetRepo(), fetchhead_foreach_cb, &pull_merge_oid ) )
    {
        aHandler->AddGitError( _( "Could not read the fetched changes." ) );
        return PullResult::Error;
    }

    // Add Version Control doesn't write branch.<name>.merge, so FETCH_HEAD has no
    // merge-marked entry.  Fall back to refs/remotes/<remote>/<branch> and persist
    // the upstream so subsequent pulls take the normal path.
#if ( LIBGIT2_VER_MAJOR >= 1 ) || ( LIBGIT2_VER_MINOR >= 99 )
    if( git_oid_is_zero( &pull_merge_oid ) )
#else
    if( git_oid_iszero( &pull_merge_oid ) )
#endif
    {
        git_reference* head_ref = nullptr;
        wxString       branch_shorthand;

        if( git_repository_head( &head_ref, aHandler->GetRepo() ) == GIT_OK )
        {
            if( git_reference_is_branch( head_ref ) )
                branch_shorthand = wxString::FromUTF8( git_reference_shorthand( head_ref ) );
        }
        else if( git_repository_head_unborn( aHandler->GetRepo() ) == 1 )
        {
            // No commit yet, so HEAD is only a branch name and the first pull adopts its remote.
            git_reference* symbolic = nullptr;

            if( git_reference_lookup( &symbolic, aHandler->GetRepo(), "HEAD" ) == GIT_OK )
            {
                KIGIT::GitReferencePtr symbolicPtr( symbolic );

                if( const char* target = git_reference_symbolic_target( symbolic ) )
                    wxString::FromUTF8( target ).StartsWith( wxS( "refs/heads/" ), &branch_shorthand );
            }
        }

        KIGIT::GitReferencePtr headRefPtr( head_ref );

        if( !branch_shorthand.IsEmpty() )
        {
            wxString remoteName = aHandler->GetCommon()->GetRemoteNameOrDefault();
            wxString remoteRefName = wxString::Format( "refs/remotes/%s/%s", remoteName, branch_shorthand );

            if( git_reference_name_to_id( &pull_merge_oid, aHandler->GetRepo(), remoteRefName.utf8_string().c_str() )
                        == GIT_OK
                && head_ref )
            {
                wxString upstream = wxString::Format( "%s/%s", remoteName, branch_shorthand );
                git_branch_set_upstream( head_ref, upstream.utf8_string().c_str() );
            }
        }
    }

#if ( LIBGIT2_VER_MAJOR >= 1 ) || ( LIBGIT2_VER_MINOR >= 99 )
    if( git_oid_is_zero( &pull_merge_oid ) )
#else
    if( git_oid_iszero( &pull_merge_oid ) )
#endif
    {
        // An empty remote, as right after Add to Version Control, has nothing to pull yet
        if( !remoteHasBranches )
            return PullResult::UpToDate;

        aHandler->AddErrorString( _( "Nothing to pull: the remote has no branch matching the current local "
                                     "branch." ) );
        return PullResult::Error;
    }

    // KiCad runs no filter program, so merging a version that keeps design files in Git LFS
    // would put pointer text where the design belongs. Judged before anything is written.
    if( KIGIT_COMMON* common = aHandler->GetCommon() )
    {
        wxString example;

        if( KIGIT::PROJECT_GIT_UTILS::ProjectUsesGitLfs( *common, &example, &pull_merge_oid ) )
        {
            aHandler->AddErrorString( wxString::Format(
                    _( "The changes being fetched keep '%s' in Git LFS, which KiCad cannot read or write. "
                       "Use git on the command line for this repository." ),
                    example ) );
            return PullResult::Error;
        }
    }

    git_annotated_commit* fetchhead_commit;

    if( git_annotated_commit_lookup( &fetchhead_commit, aHandler->GetRepo(), &pull_merge_oid ) )
    {
        aHandler->AddGitError( _( "Could not read the fetched commit." ) );
        return PullResult::Error;
    }

    KIGIT::GitAnnotatedCommitPtr fetchheadCommitPtr( fetchhead_commit );
    const git_annotated_commit* merge_commits[] = { fetchhead_commit };
    git_merge_analysis_t        merge_analysis;
    git_merge_preference_t      merge_preference = GIT_MERGE_PREFERENCE_NONE;

    if( git_merge_analysis( &merge_analysis, &merge_preference, aHandler->GetRepo(), merge_commits, 1 ) )
    {
        aHandler->AddGitError( _( "Could not compare the fetched changes with this branch." ) );
        return PullResult::Error;
    }

    if( merge_analysis & GIT_MERGE_ANALYSIS_UNBORN )
        return adoptRemoteHistory( aHandler, pull_merge_oid );

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
            aHandler->AddGitError( _( "Could not read the repository settings." ) );
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
        aHandler->AddErrorString( _( "Could not get the repository head." ) );
        return PullResult::Error;
    }

    KIGIT::GitReferencePtr headRef( rawRef );

    git_oid     updatedRefOid;
    const char* branch_shorthand = git_reference_shorthand( rawRef );
    wxString remote_name = aHandler->GetRemotename();
    wxString    remoteBranchName;

    // The analysis followed branch.<name>.merge, so move to that branch. Without a configured
    // upstream, the remote branch of the same name stands in.
    git_reference* upstreamRef = nullptr;

    if( git_branch_upstream( &upstreamRef, rawRef ) == GIT_OK )
    {
        KIGIT::GitReferencePtr upstreamPtr( upstreamRef );
        remoteBranchName = wxString::FromUTF8( git_reference_name( upstreamRef ) );
    }
    else
    {
        remoteBranchName = wxString::Format( "refs/remotes/%s/%s", remote_name, branch_shorthand );
    }

    if( int rc = git_reference_name_to_id( &updatedRefOid, aHandler->GetRepo(), remoteBranchName.c_str() );
        rc != GIT_OK )
    {
        wxString shownName = remoteBranchName;
        shownName.StartsWith( wxS( "refs/remotes/" ), &shownName );

        aHandler->AddError( wxString::Format( rc == GIT_ENOTFOUND ? _( "Could not find the remote branch '%s'." )
                                                                  : _( "Could not read the remote branch '%s'." ),
                                              shownName ),
                            KIGIT_COMMON::GetLastGitError() );
        return PullResult::Error;
    }

    git_commit* targetCommit = nullptr;

    if( git_commit_lookup( &targetCommit, aHandler->GetRepo(), &updatedRefOid ) != GIT_OK )
    {
        aHandler->AddGitError( _( "Could not read the fetched commit." ) );
        return PullResult::Error;
    }

    KIGIT::GitCommitPtr targetCommitPtr( targetCommit );

    git_tree* targetTree = nullptr;

    if( git_commit_tree( &targetTree, targetCommit ) != GIT_OK )
    {
        aHandler->AddGitError( _( "Could not read the fetched commit." ) );
        return PullResult::Error;
    }

    KIGIT::GitTreePtr targetTreePtr( targetTree );

    // A skipped file would leave the local edit reading as a reversal of the remote change.
    TREE_STATE treeState = trackedTreeState( aHandler->GetRepo() );

    if( treeState == TREE_STATE::UNKNOWN )
    {
        aHandler->AddGitError( _( "Could not read the status of the project files, so getting changes was stopped." ) );
        return PullResult::Error;
    }

    if( treeState == TREE_STATE::DIRTY )
    {
        aHandler->AddErrorString( _( "There are changes that are not committed yet, and getting changes would "
                                     "run over them. Commit them first." ) );
        return PullResult::DirtyWorkdir;
    }

    std::vector<wxString> collisions;

    if( !untrackedFilesOverwrittenBy( aHandler->GetRepo(), targetTree, collisions ) )
    {
        aHandler->AddErrorString( _( "Could not read the status of the project files, so getting changes was "
                                     "stopped. Files that were never committed could have been overwritten." ) );
        return PullResult::Error;
    }

    if( !collisions.empty() )
    {
        aHandler->AddErrorString( describeUntrackedCollisions( collisions ) );
        return PullResult::DirtyWorkdir;
    }

    git_checkout_options checkoutOptions;
    git_checkout_init_options( &checkoutOptions, GIT_CHECKOUT_OPTIONS_VERSION );
    auto notify_cb =
            []( git_checkout_notify_t why, const char* path, const git_diff_file* baseline,
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

    checkoutOptions.checkout_strategy = GIT_CHECKOUT_SAFE;
    checkoutOptions.notify_flags = GIT_CHECKOUT_NOTIFY_ALL;
    checkoutOptions.notify_cb = notify_cb;

    // Where the branch is now, so the files can go back if it cannot be moved to match them.
    git_commit* previousCommit = nullptr;

    if( git_reference_peel( (git_object**) &previousCommit, rawRef, GIT_OBJECT_COMMIT ) != GIT_OK )
    {
        aHandler->AddErrorString( _( "Could not read the current commit." ) );
        return PullResult::Error;
    }

    KIGIT::GitCommitPtr previousCommitPtr( previousCommit );

    if( git_checkout_tree( aHandler->GetRepo(), reinterpret_cast<git_object*>( targetTree ),
                           &checkoutOptions ) != GIT_OK )
    {
        wxString gitError = KIGIT_COMMON::GetLastGitError();
        bool     restored = rollbackWorkingTree( aHandler->GetRepo(), previousCommit, targetTree );

        refreshCachedIndex( aHandler->GetRepo() );

        aHandler->AddError( restored ? _( "Could not update the project files, so nothing was changed." )
                                     : _( "Could not update the project files, and they may be partly changed." ),
                            gitError );
        return PullResult::Error;
    }

    git_reference* updatedRef = nullptr;

    if( git_reference_set_target( &updatedRef, rawRef, &updatedRefOid, nullptr ) != GIT_OK )
    {
        // Left here, git reads them as local changes undoing the incoming commit.
        wxString gitError = KIGIT_COMMON::GetLastGitError();
        bool     restored = rollbackWorkingTree( aHandler->GetRepo(), previousCommit, targetTree );

        refreshCachedIndex( aHandler->GetRepo() );

        aHandler->AddError( restored ? _( "Could not move the branch, so nothing was changed." )
                                     : _( "Could not move the branch, and the project files may be left at the "
                                          "incoming version while the branch is not." ),
                            gitError );
        return PullResult::Error;
    }

    KIGIT::GitReferencePtr updatedRefPtr( updatedRef );

    if( git_repository_state_cleanup( aHandler->GetRepo() ) != GIT_OK )
    {
        // Branch and files already match, so this is leftover merge state, not a failed pull.
        wxLogTrace( traceGit, "handleFastForward: state cleanup failed after a completed fast-forward: %s",
                    KIGIT_COMMON::GetLastGitError() );
    }

    return PullResult::FastForward;
}


bool LIBGIT_BACKEND::hasUnstagedChanges( git_repository* aRepo )
{
    if( !aRepo )
        return false;

    git_status_options opts;
    git_status_init_options( &opts, GIT_STATUS_OPTIONS_VERSION );

    // Only check workdir changes (unstaged), not index changes (staged)
    opts.show = GIT_STATUS_SHOW_WORKDIR_ONLY;
    opts.flags = GIT_STATUS_OPT_INCLUDE_UNTRACKED;

    git_status_list* status_list = nullptr;

    if( git_status_list_new( &status_list, aRepo, &opts ) != GIT_OK )
    {
        wxLogTrace( traceGit, "Failed to get status list: %s", KIGIT_COMMON::GetLastGitError() );
        return false;
    }

    KIGIT::GitStatusListPtr status_list_ptr( status_list );
    size_t count = git_status_list_entrycount( status_list );

    // Check if any of the entries are actual modifications (not just untracked files)
    for( size_t ii = 0; ii < count; ++ii )
    {
        const git_status_entry* entry = git_status_byindex( status_list, ii );

        // Check for actual workdir modifications, not just untracked files
        if( entry->status & ( GIT_STATUS_WT_MODIFIED | GIT_STATUS_WT_DELETED | GIT_STATUS_WT_TYPECHANGE ) )
            return true;
    }

    return false;
}


PullResult LIBGIT_BACKEND::handleMerge( GIT_PULL_HANDLER* aHandler, const git_annotated_commit** aMergeHeads,
                                        size_t aMergeHeadsCount )
{
    // Check for unstaged changes before attempting merge
    if( hasUnstagedChanges( aHandler->GetRepo() ) )
    {
        aHandler->AddErrorString( _( "Cannot merge: you have unstaged changes. Please commit or stash them "
                                     "before pulling." ) );
        return PullResult::DirtyWorkdir;
    }

    git_repository* repo = aHandler->GetRepo();

    // Probe now, so a missing identity fails with a clean tree rather than a rewritten one.
    {
        git_signature* probe = nullptr;

        if( int rc = git_signature_default( &probe, repo ); rc != GIT_OK )
        {
            KIGIT_ERROR_MESSAGE failure = describeSignatureFailure( rc );
            aHandler->AddError( failure );
            return PullResult::MergeFailed;
        }

        KIGIT::GitSignaturePtr probePtr( probe );
    }

    {
        git_commit* theirs = nullptr;

        if( git_commit_lookup( &theirs, repo, git_annotated_commit_id( aMergeHeads[0] ) ) == GIT_OK )
        {
            KIGIT::GitCommitPtr theirsPtr( theirs );
            git_tree*           theirsTree = nullptr;

            if( git_commit_tree( &theirsTree, theirs ) == GIT_OK )
            {
                KIGIT::GitTreePtr     theirsTreePtr( theirsTree );
                std::vector<wxString> collisions;

                if( !untrackedFilesOverwrittenBy( repo, theirsTree, collisions ) )
                {
                    aHandler->AddErrorString( _( "Could not read the status of the project files, so getting "
                                                 "changes was stopped. Files that were never committed could "
                                                 "have been overwritten." ) );
                    return PullResult::Error;
                }

                if( !collisions.empty() )
                {
                    aHandler->AddErrorString( describeUntrackedCollisions( collisions ) );
                    return PullResult::DirtyWorkdir;
                }
            }
        }
    }

    if( git_merge( repo, aMergeHeads, aMergeHeadsCount, nullptr, nullptr ) )
    {
        aHandler->AddGitError( _( "Could not merge the fetched changes." ) );
        return PullResult::MergeFailed;
    }

    git_index* index = nullptr;

    if( git_repository_index( &index, repo ) != GIT_OK )
    {
        aHandler->AddErrorString( _( "Could not read repository index after merge." ) );
        return PullResult::MergeFailed;
    }

    KIGIT::GitIndexPtr indexPtr( index );

    if( git_index_has_conflicts( index ) )
    {
        // Abort the merge and restore the pre-pull state
        git_object* head_obj = nullptr;

        if( git_revparse_single( &head_obj, repo, "HEAD" ) == GIT_OK )
        {
            KIGIT::GitObjectPtr headObjPtr( head_obj );
            git_reset( repo, head_obj, GIT_RESET_HARD, nullptr );
        }

        git_repository_state_cleanup( repo );

        return PullResult::Conflict;
    }

    git_oid        tree_oid;
    git_tree*      tree = nullptr;
    git_reference* head_ref = nullptr;
    git_oid        head_oid;
    git_commit*    head_commit = nullptr;
    git_commit*    merge_commit = nullptr;
    git_signature* signature = nullptr;

    // Merged on disk but not recorded, so a failure from here puts the tree back at HEAD.
    auto undoMerge = [&]()
    {
        git_object* headObj = nullptr;

        if( git_revparse_single( &headObj, repo, "HEAD" ) == GIT_OK )
        {
            KIGIT::GitObjectPtr headObjPtr( headObj );
            git_reset( repo, headObj, GIT_RESET_HARD, nullptr );
        }

        git_repository_state_cleanup( repo );
    };

    if( git_index_write_tree( &tree_oid, index ) != GIT_OK || git_tree_lookup( &tree, repo, &tree_oid ) != GIT_OK )
    {
        aHandler->AddErrorString( _( "Could not write the merge result." ) );
        undoMerge();
        return PullResult::MergeFailed;
    }

    KIGIT::GitTreePtr treePtr( tree );

    if( git_repository_head( &head_ref, repo ) != GIT_OK )
    {
        aHandler->AddErrorString( _( "Could not get the repository head." ) );
        undoMerge();
        return PullResult::MergeFailed;
    }

    KIGIT::GitReferencePtr headRefPtr( head_ref );

    if( git_reference_name_to_id( &head_oid, repo, "HEAD" ) != GIT_OK
        || git_commit_lookup( &head_commit, repo, &head_oid ) != GIT_OK
        || git_commit_lookup( &merge_commit, repo, git_annotated_commit_id( aMergeHeads[0] ) ) != GIT_OK )
    {
        aHandler->AddErrorString( _( "Could not look up the commits to merge." ) );
        undoMerge();
        return PullResult::MergeFailed;
    }

    KIGIT::GitCommitPtr headCommitPtr( head_commit );
    KIGIT::GitCommitPtr mergeCommitPtr( merge_commit );

    if( int rc = git_signature_default( &signature, repo ); rc != GIT_OK )
    {
        KIGIT_ERROR_MESSAGE failure = describeSignatureFailure( rc );
        aHandler->AddError( failure );
        undoMerge();
        return PullResult::MergeFailed;
    }

    KIGIT::GitSignaturePtr signaturePtr( signature );

    const git_commit* parents[] = { head_commit, merge_commit };
    wxString          message = wxString::Format( _( "Merge remote-tracking branch into %s" ),
                                                  git_reference_shorthand( head_ref ) );
    git_oid           merge_commit_oid;

    if( git_commit_create( &merge_commit_oid, repo, "HEAD", signature, signature, nullptr,
                           message.utf8_string().c_str(), tree, 2, parents ) != GIT_OK )
    {
        aHandler->AddGitError( _( "Could not create the merge commit." ) );
        undoMerge();
        return PullResult::MergeFailed;
    }

    git_repository_state_cleanup( repo );
    return PullResult::Success;
}


static PullResult rebaseOnto( GIT_PULL_HANDLER* aHandler, git_repository* aRepo, const git_annotated_commit* aOnto )
{
    git_signature* signature = nullptr;

    if( int rc = git_signature_default( &signature, aRepo ); rc != GIT_OK )
    {
        KIGIT_ERROR_MESSAGE failure = describeSignatureFailure( rc );
        aHandler->AddError( failure );
        return PullResult::Error;
    }

    KIGIT::GitSignaturePtr signaturePtr( signature );
    git_rebase_options     rebase_opts;
    git_rebase_init_options( &rebase_opts, GIT_REBASE_OPTIONS_VERSION );
    git_rebase* rebase = nullptr;

    if( git_rebase_init( &rebase, aRepo, nullptr, aOnto, nullptr, &rebase_opts ) != GIT_OK )
    {
        aHandler->AddGitError( _( "Could not start the rebase." ) );
        return PullResult::Error;
    }

    KIGIT::GitRebasePtr   rebasePtr( rebase );
    git_rebase_operation* op = nullptr;
    int                   nextResult = GIT_OK;

    while( ( nextResult = git_rebase_next( &op, rebase ) ) == GIT_OK )
    {
        git_oid commit_oid;
        int     commitResult = git_rebase_commit( &commit_oid, rebase, nullptr, signature, nullptr, nullptr );

        // Upstream already has this change, so replaying it would be an empty commit
        if( commitResult == GIT_EAPPLIED )
            continue;

        if( commitResult != GIT_OK )
        {
            wxString gitError = KIGIT_COMMON::GetLastGitError();
            git_rebase_abort( rebase );

            if( commitResult == GIT_EUNMERGED )
            {
                aHandler->AddErrorString( _( "The rebase ran into conflicts.  This is best resolved from a "
                                             "git command line." ) );
                return PullResult::Conflict;
            }

            aHandler->AddError( _( "Could not rebase your commits onto the fetched changes." ), gitError );
            return PullResult::Error;
        }
    }

    if( nextResult != GIT_ITEROVER )
    {
        wxString gitError = KIGIT_COMMON::GetLastGitError();
        git_rebase_abort( rebase );
        aHandler->AddError( _( "Could not rebase your commits onto the fetched changes." ), gitError );
        return PullResult::Error;
    }

    if( git_rebase_finish( rebase, signature ) != GIT_OK )
    {
        wxString gitError = KIGIT_COMMON::GetLastGitError();
        git_rebase_abort( rebase );
        aHandler->AddError( _( "Could not finish the rebase." ), gitError );
        return PullResult::Error;
    }

    return PullResult::Success;
}


PullResult LIBGIT_BACKEND::handleRebase( GIT_PULL_HANDLER* aHandler, const git_annotated_commit** aMergeHeads,
                                         size_t aMergeHeadsCount )
{
    // Check for unstaged changes before attempting rebase
    if( hasUnstagedChanges( aHandler->GetRepo() ) )
    {
        aHandler->AddErrorString( _( "Cannot pull with rebase: you have unstaged changes. Please commit or stash "
                                     "them before pulling." ) );
        return PullResult::DirtyWorkdir;
    }

    return rebaseOnto( aHandler, aHandler->GetRepo(), aMergeHeads[0] );
}


git_repository* LIBGIT_BACKEND::GetRepositoryForFile( const char* aFilename, wxString* aError, wxString* aTrustCommand )
{
    git_repository* repo = nullptr;

    // Searching from the project, as git_repository_discover does, rather than opening the git directory it
    // finds keeps a linked .git from moving the working folder to the link's target
    if( int rc = git_repository_open_ext( &repo, aFilename, 0, nullptr ); rc != GIT_OK )
    {
        wxLogTrace( traceGit, "Can't open repo for %s: %s", aFilename, KIGIT_COMMON::GetLastGitError() );

        // Left unexplained, a repository git refuses to open looks like no repository at all.
        if( rc == GIT_EOWNER && aError )
        {
            git_buf gitDir = GIT_BUF_INIT;

            if( git_repository_discover( &gitDir, aFilename, 0, nullptr ) == GIT_OK )
            {
                KIGIT::GitBufPtr        gitDirPtr( &gitDir );
                KIGIT_OWNERSHIP_REFUSAL refusal = KIGIT::DescribeOwnershipRefusal( wxString::FromUTF8( aFilename ),
                                                                                   wxString::FromUTF8( gitDir.ptr ) );

                // Left as no repository, so Add to Version Control gives the project its own
                if( refusal.coversHome )
                    return nullptr;

                *aError = refusal.summary;

                if( aTrustCommand )
                    *aTrustCommand = refusal.trustCommand;
            }
        }

        return nullptr;
    }

    return repo;
}


int LIBGIT_BACKEND::CreateBranch( git_repository* aRepo, const wxString& aBranchName, const wxString& aFrom )
{
    // The start point is a branch name as git would read it, local or remote, or HEAD.
    std::string start = aFrom.IsEmpty() ? std::string( "HEAD" ) : aFrom.utf8_string();
    git_object* startObj = nullptr;

    if( int error = git_revparse_single( &startObj, aRepo, start.c_str() ); error != GIT_OK )
    {
        wxLogTrace( traceGit, "Failed to resolve branch start point '%s': %s", start, KIGIT_COMMON::GetLastGitError() );
        return error;
    }

    KIGIT::GitObjectPtr startPtr( startObj );
    git_object*         peeled = nullptr;

    if( int error = git_object_peel( &peeled, startObj, GIT_OBJECT_COMMIT ); error != GIT_OK )
    {
        wxLogTrace( traceGit, "Failed to lookup commit: %s", KIGIT_COMMON::GetLastGitError() );
        return error;
    }

    KIGIT::GitObjectPtr peeledPtr( peeled );
    git_commit*         commit = reinterpret_cast<git_commit*>( peeled );
    git_reference*      branchRef = nullptr;

    std::string name = aBranchName.utf8_string();

    if( int error = git_branch_create( &branchRef, aRepo, name.c_str(), commit, 0 ); error != GIT_OK )
    {
        wxLogTrace( traceGit, "Failed to create branch: %s", KIGIT_COMMON::GetLastGitError() );
        return error;
    }

    git_reference_free( branchRef );
    return 0;
}


bool LIBGIT_BACKEND::DeleteBranch( git_repository* aRepo, const wxString& aBranchName )
{
    if( !aRepo || aBranchName.IsEmpty() )
        return false;

    git_reference* branch = nullptr;

    if( git_branch_lookup( &branch, aRepo, aBranchName.utf8_string().c_str(), GIT_BRANCH_LOCAL ) != GIT_OK )
        return false;

    KIGIT::GitReferencePtr branchPtr( branch );
    return git_branch_delete( branch ) == GIT_OK;
}


bool LIBGIT_BACKEND::StashSave( git_repository* aRepo, const wxString& aMessage )
{
    if( !aRepo )
        return false;

    git_signature* signature = nullptr;

    if( git_signature_default( &signature, aRepo ) != GIT_OK )
        return false;

    KIGIT::GitSignaturePtr signaturePtr( signature );

    // libgit2 stashes from an unstamped index copy that trusts racily clean entries, so an edit made in the
    // index's timestamp tick is stashed as unchanged and then reverted. Writing the index smudges those entries
    git_index* index = nullptr;

    if( git_repository_index( &index, aRepo ) != GIT_OK )
        return false;

    KIGIT::GitIndexPtr indexPtr( index );

    if( git_index_read( index, true ) != GIT_OK || git_index_write( index ) != GIT_OK )
        return false;

    std::string message = aMessage.utf8_string();
    git_oid     stashOid;

    return git_stash_save( &stashOid, aRepo, signature, message.empty() ? nullptr : message.c_str(), GIT_STASH_DEFAULT )
           == GIT_OK;
}


static bool rollbackWorkingTree( git_repository* aRepo, git_commit* aHeadCommit, git_tree* aWrittenTree );
static bool restorePathsFromCommit( git_repository* aRepo, git_commit* aCommit,
                                    const std::vector<std::string>& aPaths );


// libgit2 reports a conflicting apply as applied, so a pop would drop the only copy of the
// stashed work. A clean tracked tree makes the rollback lossless.
static bool applyStashOntoCleanTree( git_repository* aRepo, size_t aIndex )
{
    if( !aRepo )
        return false;

    if( trackedTreeState( aRepo ) != TREE_STATE::CLEAN )
    {
        git_error_set_str( GIT_ERROR_STASH, _( "Commit or discard the current changes first, so the stash has "
                                               "a clean version to apply to." )
                                                    .utf8_string()
                                                    .c_str() );
        return false;
    }

    git_oid headOid;

    if( git_reference_name_to_id( &headOid, aRepo, "HEAD" ) != GIT_OK )
        return false;

    git_commit* headCommit = nullptr;

    if( git_commit_lookup( &headCommit, aRepo, &headOid ) != GIT_OK )
        return false;

    KIGIT::GitCommitPtr headCommitPtr( headCommit );

    if( git_stash_apply( aRepo, aIndex, nullptr ) != GIT_OK )
        return false;

    // An unreadable index cannot confirm the apply, and a caller reading success would drop the
    // stash, so treat it as a conflict.
    git_index*         index = nullptr;
    bool               conflicted = git_repository_index( &index, aRepo ) != GIT_OK || git_index_has_conflicts( index );
    KIGIT::GitIndexPtr indexPtr( index );

    if( conflicted )
    {
        // Conflict markers match neither version, so the touched paths come from the status.
        restorePathsFromCommit( aRepo, headCommit, changedPaths( aRepo ) );
        git_error_set_str( GIT_ERROR_STASH, _( "The stash does not fit on top of the current version of the "
                                               "files, so nothing was changed. Restore the version it was made "
                                               "on from History, then apply it there." )
                                                    .utf8_string()
                                                    .c_str() );
        return false;
    }

    return true;
}


bool LIBGIT_BACKEND::StashApply( git_repository* aRepo, size_t aIndex )
{
    return applyStashOntoCleanTree( aRepo, aIndex );
}


bool LIBGIT_BACKEND::StashPop( git_repository* aRepo, size_t aIndex )
{
    return applyStashOntoCleanTree( aRepo, aIndex ) && git_stash_drop( aRepo, aIndex ) == GIT_OK;
}


bool LIBGIT_BACKEND::StashDrop( git_repository* aRepo, size_t aIndex )
{
    return aRepo && git_stash_drop( aRepo, aIndex ) == GIT_OK;
}


bool LIBGIT_BACKEND::CreateTag( git_repository* aRepo, const wxString& aName, const wxString& aMessage )
{
    if( !aRepo || aName.IsEmpty() )
        return false;

    git_object* target = nullptr;

    if( git_revparse_single( &target, aRepo, "HEAD" ) != GIT_OK )
        return false;

    KIGIT::GitObjectPtr targetPtr( target );
    git_oid             oid;

    if( aMessage.IsEmpty() )
        return git_tag_create_lightweight( &oid, aRepo, aName.utf8_string().c_str(), target, 0 ) == GIT_OK;

    // An annotated tag carries the message, and who made it, from the repository's identity.
    git_signature* tagger = nullptr;

    if( git_signature_default( &tagger, aRepo ) != GIT_OK )
        return false;

    KIGIT::GitSignaturePtr taggerPtr( tagger );
    std::string            name = aName.utf8_string();
    std::string            message = aMessage.utf8_string();

    return git_tag_create( &oid, aRepo, name.c_str(), target, tagger, message.c_str(), 0 ) == GIT_OK;
}


bool LIBGIT_BACKEND::DeleteTag( git_repository* aRepo, const wxString& aName )
{
    if( !aRepo || aName.IsEmpty() )
        return false;

    return git_tag_delete( aRepo, aName.utf8_string().c_str() ) == GIT_OK;
}


bool LIBGIT_BACKEND::CheckoutTag( git_repository* aRepo, const wxString& aName )
{
    if( !aRepo || aName.IsEmpty() )
        return false;

    git_object* target = nullptr;
    wxString    spec = aName + wxT( "^{commit}" );

    if( git_revparse_single( &target, aRepo, spec.utf8_string().c_str() ) != GIT_OK )
        return false;

    KIGIT::GitObjectPtr targetPtr( target );

    git_checkout_options opts;
    git_checkout_options_init( &opts, GIT_CHECKOUT_OPTIONS_VERSION );
    opts.checkout_strategy = GIT_CHECKOUT_SAFE;

    if( git_checkout_tree( aRepo, target, &opts ) != GIT_OK )
        return false;

    return git_repository_set_head_detached( aRepo, git_object_id( target ) ) == GIT_OK;
}


bool LIBGIT_BACKEND::AddRemote( git_repository* aRepo, const wxString& aName, const wxString& aUrl )
{
    if( !aRepo || aName.IsEmpty() )
        return false;

    git_remote* remote = nullptr;

    if( git_remote_create( &remote, aRepo, aName.utf8_string().c_str(), aUrl.utf8_string().c_str() ) != GIT_OK )
        return false;

    KIGIT::GitRemotePtr remotePtr( remote );
    return true;
}


bool LIBGIT_BACKEND::SetRemoteUrl( git_repository* aRepo, const wxString& aName, const wxString& aUrl )
{
    if( !aRepo || aName.IsEmpty() )
        return false;

    return git_remote_set_url( aRepo, aName.utf8_string().c_str(), aUrl.utf8_string().c_str() ) == GIT_OK;
}


bool LIBGIT_BACKEND::RemoveRemote( git_repository* aRepo, const wxString& aName )
{
    if( !aRepo || aName.IsEmpty() )
        return false;

    return git_remote_delete( aRepo, aName.utf8_string().c_str() ) == GIT_OK;
}


bool LIBGIT_BACKEND::SetUserConfig( git_repository* aRepo, const wxString& aName, const wxString& aEmail )
{
    if( !aRepo )
        return false;

    git_config* config = nullptr;

    if( git_repository_config( &config, aRepo ) != GIT_OK )
        return false;

    KIGIT::GitConfigPtr configPtr( config );

    bool ok = git_config_set_string( config, "user.name", aName.utf8_string().c_str() ) == GIT_OK;
    ok = ok && git_config_set_string( config, "user.email", aEmail.utf8_string().c_str() ) == GIT_OK;
    return ok;
}


namespace
{

// Owner-only, matching the other extracted-content helpers.
bool writeConflictSide( git_repository* aRepo, const git_index_entry* aEntry, const wxString& aDest )
{
    wxFile out;

    if( !out.Create( aDest, true, wxS_IRUSR | wxS_IWUSR ) )
        return false;

    if( !aEntry )
        return true;

    git_blob* blob = nullptr;

    if( git_blob_lookup( &blob, aRepo, &aEntry->id ) != GIT_OK )
        return false;

    KIGIT::GitBlobPtr blobPtr( blob );

    const void* raw = git_blob_rawcontent( blob );
    git_off_t   size = git_blob_rawsize( blob );

    return out.Write( raw, static_cast<size_t>( size ) ) == static_cast<size_t>( size );
}


// Into a fresh in-memory index, leaving the repository untouched.
bool remergeInMemory( git_repository* aRepo, const wxString& aPinnedTheirsOid, KIGIT::GitIndexPtr& aIndexOut,
                      KIGIT::GitCommitPtr& aOursOut, KIGIT::GitCommitPtr& aTheirsOut, wxString& aError )
{
    git_oid oursOid;

    if( git_reference_name_to_id( &oursOid, aRepo, "HEAD" ) != GIT_OK )
    {
        aError = _( "Could not get the repository head." );
        return false;
    }

    git_oid theirsOid;

    if( !aPinnedTheirsOid.IsEmpty() )
    {
        // The exact commit the conflicts came from, since the background fetch can move FETCH_HEAD.
        if( git_oid_fromstr( &theirsOid, aPinnedTheirsOid.utf8_string().c_str() ) != GIT_OK )
        {
            aError = _( "Could not find the fetched remote commit." );
            return false;
        }
    }
    else
    {
        git_object* fetchHead = nullptr;

        if( git_revparse_single( &fetchHead, aRepo, "FETCH_HEAD" ) != GIT_OK )
        {
            aError = _( "Could not find the fetched remote commit." );
            return false;
        }

        KIGIT::GitObjectPtr fetchHeadPtr( fetchHead );
        theirsOid = *git_object_id( fetchHead );
    }

    git_commit* ours = nullptr;

    if( git_commit_lookup( &ours, aRepo, &oursOid ) != GIT_OK )
    {
        aError = _( "Could not look up the commits to merge." );
        return false;
    }

    aOursOut.reset( ours );

    git_commit* theirs = nullptr;

    if( git_commit_lookup( &theirs, aRepo, &theirsOid ) != GIT_OK )
    {
        aError = _( "Could not look up the commits to merge." );
        return false;
    }

    aTheirsOut.reset( theirs );

    git_tree* oursTree = nullptr;

    if( git_commit_tree( &oursTree, ours ) != GIT_OK )
    {
        aError = _( "Could not read the commit trees." );
        return false;
    }

    KIGIT::GitTreePtr oursTreePtr( oursTree );
    git_tree*         theirsTree = nullptr;

    if( git_commit_tree( &theirsTree, theirs ) != GIT_OK )
    {
        aError = _( "Could not read the commit trees." );
        return false;
    }

    KIGIT::GitTreePtr theirsTreePtr( theirsTree );

    // Unrelated histories have no merge base, which git_merge_trees accepts as a null ancestor.
    KIGIT::GitTreePtr baseTreePtr;
    git_oid           baseOid;

    if( git_merge_base( &baseOid, aRepo, &oursOid, &theirsOid ) == GIT_OK )
    {
        git_commit* baseCommit = nullptr;

        if( git_commit_lookup( &baseCommit, aRepo, &baseOid ) == GIT_OK )
        {
            KIGIT::GitCommitPtr baseCommitPtr( baseCommit );
            git_tree*           baseTree = nullptr;

            if( git_commit_tree( &baseTree, baseCommit ) == GIT_OK )
                baseTreePtr.reset( baseTree );
        }
    }

    git_merge_options mergeOpts;
    git_index*        mergedIndex = nullptr;

    git_merge_options_init( &mergeOpts, GIT_MERGE_OPTIONS_VERSION );

    if( git_merge_trees( &mergedIndex, aRepo, baseTreePtr.get(), oursTree, theirsTree, &mergeOpts ) != GIT_OK )
    {
        aError = wxString::Format( _( "Merge failed: %s" ), KIGIT_COMMON::GetLastGitError() );
        return false;
    }

    aIndexOut.reset( mergedIndex );
    return true;
}

} // namespace


// Just these paths. Everything else is left alone, so a rollback cannot reach an untouched file.
static bool restorePathsFromCommit( git_repository* aRepo, git_commit* aCommit, const std::vector<std::string>& aPaths )
{
    if( aPaths.empty() )
        return true;

    git_tree* tree = nullptr;

    if( git_commit_tree( &tree, aCommit ) != GIT_OK )
        return false;

    KIGIT::GitTreePtr  treePtr( tree );
    std::vector<char*> pathSpec;

    // git_strarray wants mutable pointers, though checkout only reads them.
    for( const std::string& path : aPaths )
        pathSpec.push_back( const_cast<char*>( path.c_str() ) );

    git_checkout_options checkoutOpts;
    git_checkout_init_options( &checkoutOpts, GIT_CHECKOUT_OPTIONS_VERSION );
    checkoutOpts.checkout_strategy = GIT_CHECKOUT_FORCE | GIT_CHECKOUT_REMOVE_UNTRACKED;
    checkoutOpts.paths.strings = pathSpec.data();
    checkoutOpts.paths.count = pathSpec.size();

    return git_checkout_tree( aRepo, reinterpret_cast<git_object*>( tree ), &checkoutOpts ) == GIT_OK;
}


// Whether aPath in aTree holds exactly what is on disk at aOnDisk.
static bool treeHolds( git_tree* aTree, const char* aPath, const git_oid& aOnDisk )
{
    git_tree_entry* entry = nullptr;

    if( git_tree_entry_bypath( &entry, aTree, aPath ) != GIT_OK )
        return false;

    KIGIT::GitTreeEntryPtr entryPtr( entry );

    return git_oid_equal( &aOnDisk, git_tree_entry_id( entry ) );
}


// Safe when nothing is there, or the file still holds what was written or what is being restored.
static bool safeToRestore( git_repository* aRepo, const char* aPath, git_tree* aPreviousTree, git_tree* aWrittenTree )
{
    const char* workdir = git_repository_workdir( aRepo );

    if( !workdir )
        return false;

    // Nothing there is the state a restore is meant to repair, so there is nothing to lose.
    if( !wxFileName::Exists( wxString::FromUTF8( workdir ) + wxString::FromUTF8( aPath ) ) )
        return true;

    git_oid onDisk;

    // Not readable as a file, such as a directory of untracked work, so it cannot be shown ours.
    if( git_repository_hashfile( &onDisk, aRepo, aPath, GIT_OBJECT_BLOB, nullptr ) != GIT_OK )
        return false;

    return treeHolds( aWrittenTree, aPath, onDisk ) || treeHolds( aPreviousTree, aPath, onDisk );
}


// Only the paths the operation wrote, and only while they still hold what it wrote, so an edit
// made meanwhile is kept. False when anything had to be left behind.
static bool rollbackWorkingTree( git_repository* aRepo, git_commit* aHeadCommit, git_tree* aWrittenTree )
{
    git_tree* previousTree = nullptr;

    if( git_commit_tree( &previousTree, aHeadCommit ) != GIT_OK )
        return false;

    KIGIT::GitTreePtr previousTreePtr( previousTree );

    git_diff* diff = nullptr;

    if( git_diff_tree_to_tree( &diff, aRepo, previousTree, aWrittenTree, nullptr ) != GIT_OK )
        return false;

    KIGIT::GitDiffPtr diffPtr( diff );

    std::set<std::string> touched;

    for( size_t ii = 0; ii < git_diff_num_deltas( diff ); ii++ )
    {
        const git_diff_delta* delta = git_diff_get_delta( diff, ii );

        // A rename reports two paths, and both need putting back.
        for( const char* path : { delta->old_file.path, delta->new_file.path } )
        {
            if( path )
                touched.emplace( path );
        }
    }

    std::vector<std::string> restorable;
    bool                     leftBehind = false;

    for( const std::string& path : touched )
    {
        if( safeToRestore( aRepo, path.c_str(), previousTree, aWrittenTree ) )
            restorable.push_back( path );
        else
            leftBehind = true;
    }

    if( restorable.empty() )
        return !leftBehind;

    return restorePathsFromCommit( aRepo, aHeadCommit, restorable ) && !leftBehind;
}


// So each operation can word the same underlying failure its own way.
enum class PUBLISH_RESULT
{
    PUBLISHED,
    CHECKOUT_FAILED_ROLLED_BACK,
    CHECKOUT_FAILED_PARTIAL,
    RECORD_FAILED_ROLLED_BACK,
    RECORD_FAILED_PARTIAL
};


// Working tree first, branch last, so any failure leaves the branch and its history untouched.
static PUBLISH_RESULT publishPreparedCommit( git_repository* aRepo, git_tree* aTree, const git_oid& aCommitOid,
                                             git_reference* aHeadRef, git_commit* aRollbackCommit,
                                             const char* aReflogLabel, wxString& aGitError )
{
    git_checkout_options checkoutOpts;
    git_checkout_options_init( &checkoutOpts, GIT_CHECKOUT_OPTIONS_VERSION );
    checkoutOpts.checkout_strategy = GIT_CHECKOUT_FORCE;

    if( git_checkout_tree( aRepo, reinterpret_cast<git_object*>( aTree ), &checkoutOpts ) != GIT_OK )
    {
        aGitError = KIGIT_COMMON::GetLastGitError();

        bool rolledBack = rollbackWorkingTree( aRepo, aRollbackCommit, aTree );

        refreshCachedIndex( aRepo );

        return rolledBack ? PUBLISH_RESULT::CHECKOUT_FAILED_ROLLED_BACK : PUBLISH_RESULT::CHECKOUT_FAILED_PARTIAL;
    }

    git_reference* updatedRef = nullptr;

    if( git_reference_set_target( &updatedRef, aHeadRef, &aCommitOid, aReflogLabel ) != GIT_OK )
    {
        aGitError = KIGIT_COMMON::GetLastGitError();

        bool rolledBack = rollbackWorkingTree( aRepo, aRollbackCommit, aTree );

        refreshCachedIndex( aRepo );

        return rolledBack ? PUBLISH_RESULT::RECORD_FAILED_ROLLED_BACK : PUBLISH_RESULT::RECORD_FAILED_PARTIAL;
    }

    KIGIT::GitReferencePtr updatedRefPtr( updatedRef );

    return PUBLISH_RESULT::PUBLISHED;
}


RestoreResult LIBGIT_BACKEND::RestoreVersion( KIGIT_COMMON& aCommon, const wxString& aCommitHash,
                                              KIGIT_ERROR_MESSAGE& aError )
{
    git_repository* repo = aCommon.GetRepo();

    if( !repo )
    {
        aError.details = _( "The project has no git repository." );
        return RestoreResult::Error;
    }

    const std::optional<wxString> projectPrefix = aCommon.GetProjectPrefix();

    if( !projectPrefix )
    {
        aError.details = _( "Could not work out which folder this project occupies in the repository, "
                            "so nothing was restored." );
        return RestoreResult::Error;
    }

    git_object* targetObj = nullptr;

    if( git_revparse_single( &targetObj, repo, aCommitHash.utf8_string().c_str() ) != GIT_OK )
    {
        aError.details = _( "Could not find that version." );
        return RestoreResult::Error;
    }

    KIGIT::GitObjectPtr targetObjPtr( targetObj );
    git_commit*         targetCommit = nullptr;

    if( git_object_type( targetObj ) != GIT_OBJECT_COMMIT
        || git_commit_lookup( &targetCommit, repo, git_object_id( targetObj ) ) != GIT_OK )
    {
        aError.details = _( "Could not find that version." );
        return RestoreResult::Error;
    }

    KIGIT::GitCommitPtr targetCommitPtr( targetCommit );
    git_tree*           targetTree = nullptr;

    if( git_commit_tree( &targetTree, targetCommit ) != GIT_OK )
    {
        aError.details = _( "Could not read that version's contents." );
        return RestoreResult::Error;
    }

    KIGIT::GitTreePtr targetTreePtr( targetTree );

    git_reference* headRef = nullptr;

    if( git_repository_head( &headRef, repo ) != GIT_OK )
    {
        aError.details = _( "Could not get the repository head." );
        return RestoreResult::Error;
    }

    KIGIT::GitReferencePtr headRefPtr( headRef );
    git_commit*            headCommit = nullptr;

    if( git_reference_peel( (git_object**) &headCommit, headRef, GIT_OBJECT_COMMIT ) != GIT_OK )
    {
        aError.details = _( "Could not get the repository head." );
        return RestoreResult::Error;
    }

    KIGIT::GitCommitPtr headCommitPtr( headCommit );
    git_tree*           headTree = nullptr;

    if( git_commit_tree( &headTree, headCommit ) != GIT_OK )
    {
        aError.details = _( "Could not get the repository head." );
        return RestoreResult::Error;
    }

    KIGIT::GitTreePtr headTreePtr( headTree );

    // A repository can hold several projects, so graft only this one's subtree onto the head.
    KIGIT::GitTreePtr effectiveTreePtr;
    git_tree*         effectiveTree = targetTree;

    if( !projectPrefix->IsEmpty() )
    {
        // libgit2 expects the folder path without a trailing slash.
        std::string     prefixUtf8( projectPrefix->BeforeLast( '/' ).utf8_string() );
        git_tree_entry* subtreeEntry = nullptr;

        if( git_tree_entry_bypath( &subtreeEntry, targetTree, prefixUtf8.c_str() ) != GIT_OK )
        {
            aError.details = _( "That version does not contain this project's folder." );
            return RestoreResult::Error;
        }

        KIGIT::GitTreeEntryPtr subtreeEntryPtr( subtreeEntry );

        if( git_tree_entry_type( subtreeEntry ) != GIT_OBJECT_TREE )
        {
            aError.details = _( "That version does not contain this project's folder." );
            return RestoreResult::Error;
        }

        git_tree_update update;
        update.action = GIT_TREE_UPDATE_UPSERT;
        update.id = *git_tree_entry_id( subtreeEntry );
        update.filemode = GIT_FILEMODE_TREE;
        update.path = prefixUtf8.c_str();

        git_oid combinedOid;

        if( git_tree_create_updated( &combinedOid, repo, headTree, 1, &update ) != GIT_OK )
        {
            aError.details = wxString::Format( _( "Could not build the restored content: %s" ),
                                               KIGIT_COMMON::GetLastGitError() );
            return RestoreResult::Error;
        }

        git_tree* combined = nullptr;

        if( git_tree_lookup( &combined, repo, &combinedOid ) != GIT_OK )
        {
            aError.details = _( "Could not build the restored content." );
            return RestoreResult::Error;
        }

        effectiveTreePtr.reset( combined );
        effectiveTree = combined;
    }

    // The checkout is forced, so refuse when uncommitted work would be overwritten. An untracked
    // file only counts when the restored content carries its path.
    {
        git_status_options statusOpts;
        git_status_init_options( &statusOpts, GIT_STATUS_OPTIONS_VERSION );
        statusOpts.show = GIT_STATUS_SHOW_INDEX_AND_WORKDIR;
        statusOpts.flags = GIT_STATUS_OPT_INCLUDE_UNTRACKED | GIT_STATUS_OPT_RECURSE_UNTRACKED_DIRS;

        git_status_list* statusList = nullptr;

        if( git_status_list_new( &statusList, repo, &statusOpts ) != GIT_OK )
        {
            aError.details = _( "Could not read the repository status." );
            return RestoreResult::Error;
        }

        KIGIT::GitStatusListPtr statusListPtr( statusList );
        bool                    dirty = false;
        size_t                  count = git_status_list_entrycount( statusList );

        for( size_t ii = 0; ii < count && !dirty; ++ii )
        {
            const git_status_entry* entry = git_status_byindex( statusList, ii );

            if( entry->status == GIT_STATUS_WT_NEW )
            {
                const char* path = entry->index_to_workdir ? entry->index_to_workdir->new_file.path : nullptr;

                if( path )
                {
                    git_tree_entry* treeEntry = nullptr;

                    if( git_tree_entry_bypath( &treeEntry, effectiveTree, path ) == GIT_OK )
                    {
                        KIGIT::GitTreeEntryPtr treeEntryPtr( treeEntry );
                        dirty = true;
                    }
                }
            }
            else
            {
                dirty = true;
            }
        }

        if( dirty )
            return RestoreResult::Dirty;
    }

    if( git_oid_equal( git_commit_tree_id( headCommit ), git_tree_id( effectiveTree ) ) )
        return RestoreResult::AlreadyCurrent;

    git_signature* signature = nullptr;

    if( int rc = git_signature_default( &signature, repo ); rc != GIT_OK )
    {
        aError = describeSignatureFailure( rc );
        return RestoreResult::Error;
    }

    KIGIT::GitSignaturePtr signaturePtr( signature );

    char shortHash[13] = { 0 };
    git_oid_tostr( shortHash, sizeof( shortHash ), git_commit_id( targetCommit ) );

    // The commit stays dangling until the checkout succeeds, so a failure leaves the branch as
    // it was.
    const git_commit* parents[] = { headCommit };
    wxString          message = wxString::Format( _( "Restore version %s" ), shortHash );
    git_oid           restoreCommitOid;

    if( git_commit_create( &restoreCommitOid, repo, nullptr, signature, signature, nullptr,
                           message.utf8_string().c_str(), effectiveTree, 1, parents )
        != GIT_OK )
    {
        aError.details =
                wxString::Format( _( "Could not create the restore commit: %s" ), KIGIT_COMMON::GetLastGitError() );
        return RestoreResult::Error;
    }

    wxString gitError;

    switch( publishPreparedCommit( repo, effectiveTree, restoreCommitOid, headRef, headCommit, "restore", gitError ) )
    {
    case PUBLISH_RESULT::PUBLISHED: return RestoreResult::Restored;

    case PUBLISH_RESULT::CHECKOUT_FAILED_ROLLED_BACK:
        aError.details =
                wxString::Format( _( "Could not update the working files, so nothing was changed: %s" ), gitError );
        return RestoreResult::Error;

    case PUBLISH_RESULT::CHECKOUT_FAILED_PARTIAL:
        aError.details = wxString::Format( _( "Could not update the working files, and they may be partially "
                                              "changed: %s" ),
                                           gitError );
        return RestoreResult::Error;

    case PUBLISH_RESULT::RECORD_FAILED_ROLLED_BACK:
        aError.details =
                wxString::Format( _( "Could not record the restore commit, so nothing was changed: %s" ), gitError );
        return RestoreResult::Error;

    case PUBLISH_RESULT::RECORD_FAILED_PARTIAL:
        aError.details = wxString::Format( _( "Could not record the restore commit, and the working files may "
                                              "have been changed: %s" ),
                                           gitError );
        return RestoreResult::Error;
    }

    return RestoreResult::Error;
}


bool LIBGIT_BACKEND::DiscardChanges( git_repository* aRepo, const std::vector<wxString>& aPaths, wxString& aError )
{
    if( !aRepo )
    {
        aError = _( "The project has no git repository." );
        return false;
    }

    if( aPaths.empty() )
        return true;

    std::vector<std::string> pathStorage;
    std::vector<char*>       pathPtrs;
    std::vector<wxString>    neverCommitted;

    for( const wxString& path : aPaths )
    {
        if( IsPathInHead( aRepo, path ) )
            pathStorage.push_back( path.utf8_string() );
        else
            neverCommitted.push_back( path );
    }

    // HEAD has nothing to restore for these, so discarding means dropping the staged entry.
    if( !neverCommitted.empty() )
    {
        git_index* index = nullptr;

        if( git_repository_index( &index, aRepo ) != GIT_OK )
        {
            aError = KIGIT_COMMON::GetLastGitError();
            return false;
        }

        KIGIT::GitIndexPtr indexPtr( index );

        for( const wxString& path : neverCommitted )
            git_index_remove_bypath( index, path.utf8_string().c_str() );

        if( git_index_write( index ) != GIT_OK )
        {
            aError = KIGIT_COMMON::GetLastGitError();
            return false;
        }
    }

    if( pathStorage.empty() )
        return true;

    for( std::string& path : pathStorage )
        pathPtrs.push_back( path.data() );

    git_checkout_options opts;
    git_checkout_init_options( &opts, GIT_CHECKOUT_OPTIONS_VERSION );

    // Without DISABLE_PATHSPEC_MATCH these are wildmatch patterns, so a file name holding a
    // glob character would discard its neighbours too.
    opts.checkout_strategy = GIT_CHECKOUT_FORCE | GIT_CHECKOUT_DISABLE_PATHSPEC_MATCH;
    opts.paths.strings = pathPtrs.data();
    opts.paths.count = pathPtrs.size();

    if( git_checkout_head( aRepo, &opts ) != GIT_OK )
    {
        aError = KIGIT_COMMON::GetLastGitError();
        return false;
    }

    return true;
}


bool LIBGIT_BACKEND::IsPathInHead( git_repository* aRepo, const wxString& aPath )
{
    if( !aRepo )
        return false;

    git_oid headOid;

    if( git_reference_name_to_id( &headOid, aRepo, "HEAD" ) != GIT_OK )
        return false;

    git_commit* headCommit = nullptr;

    if( git_commit_lookup( &headCommit, aRepo, &headOid ) != GIT_OK )
        return false;

    KIGIT::GitCommitPtr headCommitPtr( headCommit );
    git_tree*           headTree = nullptr;

    if( git_commit_tree( &headTree, headCommit ) != GIT_OK )
        return false;

    KIGIT::GitTreePtr headTreePtr( headTree );
    git_tree_entry*   entry = nullptr;

    if( git_tree_entry_bypath( &entry, headTree, aPath.utf8_string().c_str() ) != GIT_OK )
        return false;

    KIGIT::GitTreeEntryPtr entryPtr( entry );

    return true;
}


bool LIBGIT_BACKEND::AbandonOperation( git_repository* aRepo, wxString& aError )
{
    if( !aRepo )
    {
        aError = _( "The project has no git repository." );
        return false;
    }

    // One wording for every rebase we cannot end. The user cannot act on the difference
    // between the states.
    const wxString rebaseIsAnotherProgram =
            _( "This rebase was started by another program, and only that program can finish or abort it." );

    switch( git_repository_state( aRepo ) )
    {
    case GIT_REPOSITORY_STATE_NONE: return true;

    case GIT_REPOSITORY_STATE_REBASE_MERGE:
    case GIT_REPOSITORY_STATE_REBASE_INTERACTIVE:
    {
        // Aborting restores the branch and its starting commit. A reset to the detached HEAD
        // would not.
        git_rebase* rebase = nullptr;

        if( git_rebase_open( &rebase, aRepo, nullptr ) != GIT_OK )
        {
            aError = rebaseIsAnotherProgram;
            return false;
        }

        KIGIT::GitRebasePtr rebasePtr( rebase );

        if( git_rebase_abort( rebase ) != GIT_OK )
        {
            aError = rebaseIsAnotherProgram;
            return false;
        }

        return true;
    }

    case GIT_REPOSITORY_STATE_BISECT:
        // A bisect is a search through the history, not a half-finished change. A reset would
        // leave the project on whichever commit the search had reached.
        aError = _( "This project is in the middle of a search through its history, started by another "
                    "program. Only that program can end the search and bring the project back." );
        return false;

    case GIT_REPOSITORY_STATE_REBASE:
    case GIT_REPOSITORY_STATE_APPLY_MAILBOX:
    case GIT_REPOSITORY_STATE_APPLY_MAILBOX_OR_REBASE:
        aError = rebaseIsAnotherProgram;
        return false;

    default: break;
    }

    git_object* head = nullptr;

    if( git_revparse_single( &head, aRepo, "HEAD" ) != GIT_OK )
    {
        aError = KIGIT_COMMON::GetLastGitError();
        return false;
    }

    KIGIT::GitObjectPtr headPtr( head );
    git_tree*           headTree = nullptr;

    if( git_object_peel( reinterpret_cast<git_object**>( &headTree ), head, GIT_OBJECT_TREE ) != GIT_OK )
    {
        aError = KIGIT_COMMON::GetLastGitError();
        return false;
    }

    KIGIT::GitTreePtr headTreePtr( headTree );
    git_index*        index = nullptr;

    if( git_repository_index( &index, aRepo ) != GIT_OK )
    {
        aError = KIGIT_COMMON::GetLastGitError();
        return false;
    }

    KIGIT::GitIndexPtr indexPtr( index );

    // Only what the operation touched, as git's own merge abort does. Going back over the whole
    // repository would throw away uncommitted work in files it never went near, and a
    // repository can hold more than one project.
    std::set<std::string> touched;

    // A conflicted file has no stage zero, so the diff below does not report it.
    git_index_conflict_iterator* conflicts = nullptr;

    if( git_index_conflict_iterator_new( &conflicts, index ) == GIT_OK )
    {
        const git_index_entry* ancestor = nullptr;
        const git_index_entry* ours = nullptr;
        const git_index_entry* theirs = nullptr;

        while( git_index_conflict_next( &ancestor, &ours, &theirs, conflicts ) == GIT_OK )
        {
            for( const git_index_entry* entry : { ancestor, ours, theirs } )
            {
                if( entry && entry->path )
                    touched.insert( entry->path );
            }
        }

        git_index_conflict_iterator_free( conflicts );
    }

    git_diff* diff = nullptr;

    if( git_diff_tree_to_index( &diff, aRepo, headTree, index, nullptr ) != GIT_OK )
    {
        aError = KIGIT_COMMON::GetLastGitError();
        return false;
    }

    KIGIT::GitDiffPtr diffPtr( diff );

    KIGIT::CollectDiffDeltas( diff,
                              [&touched]( const git_diff_delta& aDelta )
                              {
                                  if( aDelta.old_file.path )
                                      touched.insert( aDelta.old_file.path );

                                  if( aDelta.new_file.path )
                                      touched.insert( aDelta.new_file.path );
                              } );

    if( !touched.empty() )
    {
        std::vector<char*> paths;

        for( const std::string& path : touched )
            paths.push_back( const_cast<char*>( path.c_str() ) );

        git_checkout_options checkoutOpts;
        git_checkout_options_init( &checkoutOpts, GIT_CHECKOUT_OPTIONS_VERSION );
        checkoutOpts.checkout_strategy = GIT_CHECKOUT_FORCE;
        checkoutOpts.paths.strings = paths.data();
        checkoutOpts.paths.count = paths.size();

        if( git_checkout_head( aRepo, &checkoutOpts ) != GIT_OK )
        {
            aError = KIGIT_COMMON::GetLastGitError();
            return false;
        }
    }

    if( git_repository_state_cleanup( aRepo ) != GIT_OK )
    {
        aError = KIGIT_COMMON::GetLastGitError();
        return false;
    }

    return true;
}


bool LIBGIT_BACKEND::GetMergeConflicts( git_repository* aRepo, const wxString& aStageDir, wxString& aHeadOid,
                                        wxString& aTheirsOid, std::vector<KIGIT_CONFLICT_SIDES>& aConflicts,
                                        wxString& aError )
{
    aConflicts.clear();

    if( !aRepo )
    {
        aError = _( "The project has no git repository." );
        return false;
    }

    KIGIT::GitIndexPtr  mergedIndex;
    KIGIT::GitCommitPtr ours;
    KIGIT::GitCommitPtr theirs;

    if( !remergeInMemory( aRepo, wxEmptyString, mergedIndex, ours, theirs, aError ) )
        return false;

    {
        char theirsStr[GIT_OID_HEXSZ + 1] = { 0 };
        git_oid_tostr( theirsStr, sizeof( theirsStr ), git_commit_id( theirs.get() ) );
        aTheirsOid = wxString::FromUTF8( theirsStr );
    }

    if( !git_index_has_conflicts( mergedIndex.get() ) )
    {
        aError = _( "The merge has no conflicts." );
        return false;
    }

    if( !wxFileName::Mkdir( aStageDir, 0700, wxPATH_MKDIR_FULL ) )
    {
        aError = _( "Could not create a temporary directory." );
        return false;
    }

    char oidStr[GIT_OID_HEXSZ + 1] = { 0 };
    git_oid_tostr( oidStr, sizeof( oidStr ), git_commit_id( ours.get() ) );
    aHeadOid = wxString::FromUTF8( oidStr );

    git_index_conflict_iterator* iter = nullptr;

    if( git_index_conflict_iterator_new( &iter, mergedIndex.get() ) != GIT_OK )
    {
        aError = _( "Could not list the merge conflicts." );
        return false;
    }

    KIGIT::GitIndexConflictIteratorPtr iterPtr( iter );

    const git_index_entry* ancestor = nullptr;
    const git_index_entry* mine = nullptr;
    const git_index_entry* remote = nullptr;
    const wxString         sep = wxFileName::GetPathSeparator();
    int                    n = 0;

    while( git_index_conflict_next( &ancestor, &mine, &remote, iter ) == GIT_OK )
    {
        const git_index_entry* named = mine ? mine : ( remote ? remote : ancestor );

        if( !named )
            continue;

        KIGIT_CONFLICT_SIDES sides;
        sides.path = wxString::FromUTF8( named->path );
        sides.hasAncestor = ancestor != nullptr;
        sides.hasOurs = mine != nullptr;
        sides.hasTheirs = remote != nullptr;

        wxString base = wxFileName( sides.path ).GetFullName();
        sides.ancestorFile = aStageDir + sep + wxString::Format( wxS( "c%d_base_%s" ), n, base );
        sides.oursFile = aStageDir + sep + wxString::Format( wxS( "c%d_mine_%s" ), n, base );
        sides.theirsFile = aStageDir + sep + wxString::Format( wxS( "c%d_remote_%s" ), n, base );
        n++;

        if( !writeConflictSide( aRepo, ancestor, sides.ancestorFile )
            || !writeConflictSide( aRepo, mine, sides.oursFile )
            || !writeConflictSide( aRepo, remote, sides.theirsFile ) )
        {
            aError = _( "Could not extract the conflicting file versions." );
            return false;
        }

        aConflicts.push_back( sides );
    }

    return !aConflicts.empty();
}


MergeCommitResult LIBGIT_BACKEND::CreateMergedCommit( git_repository* aRepo, const wxString& aExpectedHeadOid,
                                                      const wxString&                                   aTheirsOid,
                                                      const std::vector<std::pair<wxString, wxString>>& aResolvedFiles,
                                                      KIGIT_ERROR_MESSAGE&                              aError )
{
    if( !aRepo )
    {
        aError.details = _( "The project has no git repository." );
        return MergeCommitResult::FailedCleanly;
    }

    // The lock is released while the user resolves, so anything dirty now appeared during
    // resolution and the checkout below would revert it. Untracked files do not count, since
    // the recovery copies the panel writes beside a conflicted file are untracked.
    {
        git_status_options statusOpts;
        git_status_init_options( &statusOpts, GIT_STATUS_OPTIONS_VERSION );
        statusOpts.show = GIT_STATUS_SHOW_INDEX_AND_WORKDIR;

        git_status_list* statusList = nullptr;

        if( git_status_list_new( &statusList, aRepo, &statusOpts ) != GIT_OK )
        {
            aError.details = _( "Could not read the repository status." );
            return MergeCommitResult::FailedCleanly;
        }

        KIGIT::GitStatusListPtr statusListPtr( statusList );

        if( git_status_list_entrycount( statusList ) > 0 )
        {
            aError.details = _( "Files changed while the conflicts were being resolved, and finishing the "
                                "merge would overwrite those changes. Get changes again to redo the merge "
                                "with the current files." );
            return MergeCommitResult::FailedCleanly;
        }
    }

    KIGIT::GitIndexPtr  mergedIndex;
    KIGIT::GitCommitPtr ours;
    KIGIT::GitCommitPtr theirs;

    if( !remergeInMemory( aRepo, aTheirsOid, mergedIndex, ours, theirs, aError.details ) )
        return MergeCommitResult::FailedCleanly;

    char oidStr[GIT_OID_HEXSZ + 1] = { 0 };
    git_oid_tostr( oidStr, sizeof( oidStr ), git_commit_id( ours.get() ) );

    // The resolutions were made against this head. If the branch moved, they no longer apply.
    if( aExpectedHeadOid != wxString::FromUTF8( oidStr ) )
    {
        aError.details = _( "The repository changed while resolving. Get changes again." );
        return MergeCommitResult::FailedCleanly;
    }

    for( const auto& [path, resolvedFile] : aResolvedFiles )
    {
        std::string            pathUtf8( path.utf8_string() );
        const git_index_entry* mine = nullptr;
        const git_index_entry* remote = nullptr;
        const git_index_entry* ancestor = nullptr;

        git_index_conflict_get( &ancestor, &mine, &remote, mergedIndex.get(), pathUtf8.c_str() );

        // An empty path means deleted. An empty file would commit a zero byte document instead.
        if( resolvedFile.IsEmpty() )
        {
            if( git_index_conflict_remove( mergedIndex.get(), pathUtf8.c_str() ) != GIT_OK )
            {
                aError.details = wxString::Format( _( "Could not mark '%s' as resolved." ), path );
                return MergeCommitResult::FailedCleanly;
            }

            continue;
        }

        std::ifstream in( resolvedFile.fn_str(), std::ios::binary );

        if( !in )
        {
            aError.details = wxString::Format( _( "Could not read the resolved file for '%s'." ), path );
            return MergeCommitResult::FailedCleanly;
        }

        std::string bytes( ( std::istreambuf_iterator<char>( in ) ), std::istreambuf_iterator<char>() );

        git_oid blobOid;

        if( git_blob_create_from_buffer( &blobOid, aRepo, bytes.data(), bytes.size() ) != GIT_OK )
        {
            aError.details = wxString::Format( _( "Could not store the resolved file for '%s'." ), path );
            return MergeCommitResult::FailedCleanly;
        }

        git_index_entry entry;
        memset( &entry, 0, sizeof( entry ) );
        entry.path = pathUtf8.c_str();
        entry.id = blobOid;
        entry.mode = mine ? mine->mode : ( remote ? remote->mode : GIT_FILEMODE_BLOB );

        if( git_index_conflict_remove( mergedIndex.get(), pathUtf8.c_str() ) != GIT_OK
            || git_index_add( mergedIndex.get(), &entry ) != GIT_OK )
        {
            aError.details = wxString::Format( _( "Could not mark '%s' as resolved." ), path );
            return MergeCommitResult::FailedCleanly;
        }
    }

    if( git_index_has_conflicts( mergedIndex.get() ) )
    {
        aError.details = _( "Not every conflict was resolved." );
        return MergeCommitResult::FailedCleanly;
    }

    git_oid treeOid;

    if( git_index_write_tree_to( &treeOid, mergedIndex.get(), aRepo ) != GIT_OK )
    {
        aError.details = _( "Could not write the merge result." );
        return MergeCommitResult::FailedCleanly;
    }

    git_tree* tree = nullptr;

    if( git_tree_lookup( &tree, aRepo, &treeOid ) != GIT_OK )
    {
        aError.details = _( "Could not write the merge result." );
        return MergeCommitResult::FailedCleanly;
    }

    KIGIT::GitTreePtr treePtr( tree );

    git_signature* signature = nullptr;

    if( int rc = git_signature_default( &signature, aRepo ); rc != GIT_OK )
    {
        aError = describeSignatureFailure( rc );
        return MergeCommitResult::FailedCleanly;
    }

    KIGIT::GitSignaturePtr signaturePtr( signature );

    git_reference* headRef = nullptr;

    if( git_repository_head( &headRef, aRepo ) != GIT_OK )
    {
        aError.details = _( "Could not get the repository head." );
        return MergeCommitResult::FailedCleanly;
    }

    KIGIT::GitReferencePtr headRefPtr( headRef );

    const git_commit* parents[] = { ours.get(), theirs.get() };
    wxString          message =
            wxString::Format( _( "Merge remote-tracking branch into %s" ), git_reference_shorthand( headRef ) );
    git_oid mergeCommitOid;

    // Branch last, because the checkout is the step that can fail.
    if( git_commit_create( &mergeCommitOid, aRepo, nullptr, signature, signature, nullptr,
                           message.utf8_string().c_str(), tree, 2, parents )
        != GIT_OK )
    {
        aError.details = wxString::Format( _( "Could not create merge commit: %s" ), KIGIT_COMMON::GetLastGitError() );
        return MergeCommitResult::FailedCleanly;
    }

    // Bring the working tree and the real index to the merged content.
    wxString gitError;

    switch( publishPreparedCommit( aRepo, tree, mergeCommitOid, headRef, ours.get(), "merge", gitError ) )
    {
    case PUBLISH_RESULT::PUBLISHED: return MergeCommitResult::Success;

    case PUBLISH_RESULT::CHECKOUT_FAILED_ROLLED_BACK:
        aError.details =
                wxString::Format( _( "Could not update the working files, so nothing was changed: %s" ), gitError );
        return MergeCommitResult::FailedCleanly;

    case PUBLISH_RESULT::CHECKOUT_FAILED_PARTIAL:
        aError.details = wxString::Format( _( "Could not update the working files, and they may be partially "
                                              "changed: %s" ),
                                           gitError );
        return MergeCommitResult::FailedPartially;

    case PUBLISH_RESULT::RECORD_FAILED_ROLLED_BACK:
        aError.details =
                wxString::Format( _( "Could not record the merge commit, so nothing was changed: %s" ), gitError );
        return MergeCommitResult::FailedCleanly;

    case PUBLISH_RESULT::RECORD_FAILED_PARTIAL:
        aError.details = wxString::Format( _( "Could not record the merge commit, and the working files may "
                                              "have been changed: %s" ),
                                           gitError );
        return MergeCommitResult::FailedPartially;
    }

    return MergeCommitResult::FailedCleanly;
}


bool LIBGIT_BACKEND::RemoveVCS( git_repository*& aRepo, const wxString& aProjectPath, bool aRemoveGitDir,
                                wxString* aErrors )
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
