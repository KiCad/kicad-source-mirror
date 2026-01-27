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

#include "git_pull_handler.h"
#include <git/kicad_git_common.h>
#include <git/kicad_git_memory.h>
#include <trace_helpers.h>

#include <wx/log.h>

#include <iostream>
#include <time.h>
#include <memory>

GIT_PULL_HANDLER::GIT_PULL_HANDLER( KIGIT_COMMON* aCommon ) : KIGIT_REPO_MIXIN( aCommon )
{
}


GIT_PULL_HANDLER::~GIT_PULL_HANDLER()
{
}


bool GIT_PULL_HANDLER::PerformFetch( bool aSkipLock )
{
    if( !GetRepo() )
    {
        wxLogTrace( traceGit, "GIT_PULL_HANDLER::PerformFetch() - No repository found" );
        return false;
    }

    std::unique_lock<std::mutex> lock( GetCommon()->m_gitActionMutex, std::try_to_lock );

    if( !aSkipLock && !lock.owns_lock() )
    {
        wxLogTrace( traceGit, "GIT_PULL_HANDLER::PerformFetch() - Could not lock mutex" );
        return false;
    }

    // Fetch updates from remote repository
    git_remote* remote = nullptr;

    if( git_remote_lookup( &remote, GetRepo(), "origin" ) != 0 )
    {
        wxLogTrace( traceGit, "GIT_PULL_HANDLER::PerformFetch() - Failed to lookup remote 'origin'" );
        AddErrorString( wxString::Format( _( "Could not lookup remote '%s'" ), "origin" ) );
        return false;
    }

    KIGIT::GitRemotePtr remotePtr( remote );

    git_remote_callbacks remoteCallbacks;
    git_remote_init_callbacks( &remoteCallbacks, GIT_REMOTE_CALLBACKS_VERSION );
    remoteCallbacks.sideband_progress = progress_cb;
    remoteCallbacks.transfer_progress = transfer_progress_cb;
    remoteCallbacks.credentials = credentials_cb;
    remoteCallbacks.payload = this;

    TestedTypes() = 0;
    ResetNextKey();

    if( git_remote_connect( remote, GIT_DIRECTION_FETCH, &remoteCallbacks, nullptr, nullptr ) )
    {
        wxString errorMsg = KIGIT_COMMON::GetLastGitError();
        wxLogTrace( traceGit, "GIT_PULL_HANDLER::PerformFetch() - Failed to connect to remote: %s", errorMsg );
        AddErrorString( wxString::Format( _( "Could not connect to remote '%s': %s" ), "origin", errorMsg ) );
        return false;
    }

    git_fetch_options fetchOptions;
    git_fetch_init_options( &fetchOptions, GIT_FETCH_OPTIONS_VERSION );
    fetchOptions.callbacks = remoteCallbacks;

    if( git_remote_fetch( remote, nullptr, &fetchOptions, nullptr ) )
    {
        wxString errorMsg = KIGIT_COMMON::GetLastGitError();
        wxLogTrace( traceGit, "GIT_PULL_HANDLER::PerformFetch() - Failed to fetch from remote: %s", errorMsg );
        AddErrorString( wxString::Format( _( "Could not fetch data from remote '%s': %s" ), "origin", errorMsg ) );
        return false;
    }

    wxLogTrace( traceGit, "GIT_PULL_HANDLER::PerformFetch() - Fetch completed successfully" );
    return true;
}


PullResult GIT_PULL_HANDLER::PerformPull()
{
    PullResult                   result = PullResult::Success;
    std::unique_lock<std::mutex> lock( GetCommon()->m_gitActionMutex, std::try_to_lock );

    if( !lock.owns_lock() )
    {
        wxLogTrace( traceGit, "GIT_PULL_HANDLER::PerformPull() - Could not lock mutex" );
        return PullResult::Error;
    }

    if( !PerformFetch( true ) )
        return PullResult::Error;

    git_oid pull_merge_oid = {};

    if( git_repository_fetchhead_foreach( GetRepo(), fetchhead_foreach_cb,
                                          &pull_merge_oid ) )
    {
        AddErrorString( _( "Could not read 'FETCH_HEAD'" ) );
        return PullResult::Error;
    }

    git_annotated_commit* fetchhead_commit;

    if( git_annotated_commit_lookup( &fetchhead_commit, GetRepo(), &pull_merge_oid ) )
    {
        AddErrorString( _( "Could not lookup commit" ) );
        return PullResult::Error;
    }

    KIGIT::GitAnnotatedCommitPtr fetchheadCommitPtr( fetchhead_commit );
    const git_annotated_commit*  merge_commits[] = { fetchhead_commit };
    git_merge_analysis_t         merge_analysis;
    git_merge_preference_t       merge_preference = GIT_MERGE_PREFERENCE_NONE;

    if( git_merge_analysis( &merge_analysis, &merge_preference, GetRepo(), merge_commits, 1 ) )
    {
        AddErrorString( _( "Could not analyze merge" ) );
        return PullResult::Error;
    }

    if( merge_analysis & GIT_MERGE_ANALYSIS_UNBORN )
    {
        AddErrorString( _( "Invalid HEAD.  Cannot merge." ) );
        return PullResult::MergeFailed;
    }

    // Nothing to do if the repository is up to date
    if( merge_analysis & GIT_MERGE_ANALYSIS_UP_TO_DATE )
    {
        wxLogTrace( traceGit, "GIT_PULL_HANDLER::PerformPull() - Repository is up to date" );
        git_repository_state_cleanup( GetRepo() );
        return PullResult::UpToDate;
    }

    // Fast-forward is easy, just update the local reference
    if( merge_analysis & GIT_MERGE_ANALYSIS_FASTFORWARD )
    {
        wxLogTrace( traceGit, "GIT_PULL_HANDLER::PerformPull() - Fast-forward merge" );
        return handleFastForward();
    }

    if( merge_analysis & GIT_MERGE_ANALYSIS_NORMAL )
    {
        wxLogTrace( traceGit, "GIT_PULL_HANDLER::PerformPull() - Normal merge" );
        PullResult ret = handleRebase( merge_commits, 1 );
        // PullResult ret = handleMerge( merge_commits, 1 );
        return ret;
    }

    wxLogTrace( traceGit, "GIT_PULL_HANDLER::PerformPull() - Merge needs resolution" );
    //TODO: handle merges when they need to be resolved

    return result;
}

const std::vector<std::pair<std::string, std::vector<CommitDetails>>>&
GIT_PULL_HANDLER::GetFetchResults() const
{
    return m_fetchResults;
}


std::string GIT_PULL_HANDLER::getFirstLineFromCommitMessage( const std::string& aMessage )
{
    if( aMessage.empty() )
        return aMessage;

    size_t firstLineEnd = aMessage.find_first_of( '\n' );

    if( firstLineEnd != std::string::npos )
        return aMessage.substr( 0, firstLineEnd );

    return aMessage;
}


std::string GIT_PULL_HANDLER::getFormattedCommitDate( const git_time& aTime )
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


PullResult GIT_PULL_HANDLER::handleFastForward()
{
    git_reference* rawRef = nullptr;

    // Get the current HEAD reference
    if( git_repository_head( &rawRef, GetRepo() ) )
    {
        AddErrorString( _( "Could not get repository head" ) );
        return PullResult::Error;
    }

    KIGIT::GitReferencePtr headRef( rawRef );

    git_oid     updatedRefOid;
    const char* currentBranchName = git_reference_name( rawRef );
    wxString    remoteBranchName = wxString::Format( "refs/remotes/origin/%s",
                                    currentBranchName + strlen( "refs/heads/" ) );

    // Get the OID of the updated reference (remote-tracking branch)
    if( git_reference_name_to_id( &updatedRefOid, GetRepo(), remoteBranchName.c_str() ) != GIT_OK )
    {
        AddErrorString( wxString::Format( _( "Could not get reference OID for reference '%s'" ),
                                            remoteBranchName ) );
        return PullResult::Error;
    }

    // Get the target commit object
    git_commit* targetCommit = nullptr;

    if( git_commit_lookup( &targetCommit, GetRepo(), &updatedRefOid ) != GIT_OK )
    {
        AddErrorString( _( "Could not look up target commit" ) );
        return PullResult::Error;
    }

    KIGIT::GitCommitPtr targetCommitPtr( targetCommit );

    // Get the tree from the target commit
    git_tree* targetTree = nullptr;

    if( git_commit_tree( &targetTree, targetCommit ) != GIT_OK )
    {
        git_commit_free( targetCommit );
        AddErrorString( _( "Could not get tree from target commit" ) );
        return PullResult::Error;
    }

    KIGIT::GitTreePtr targetTreePtr( targetTree );

    // Perform a checkout to update the working directory
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

    if( git_checkout_tree( GetRepo(), reinterpret_cast<git_object*>( targetTree ), &checkoutOptions ) != GIT_OK )
    {
        AddErrorString( _( "Failed to perform checkout operation." ) );
        return PullResult::Error;
    }

    git_reference* updatedRef = nullptr;

    // Update the current branch to point to the new commit
    if (git_reference_set_target(&updatedRef, rawRef, &updatedRefOid, nullptr) != GIT_OK)
    {
        AddErrorString( wxString::Format( _( "Failed to update reference '%s' to point to '%s'" ), currentBranchName,
                                          git_oid_tostr_s( &updatedRefOid ) ) );
        return PullResult::Error;
    }

    KIGIT::GitReferencePtr updatedRefPtr( updatedRef );

    // Clean up the repository state
    if( git_repository_state_cleanup( GetRepo() ) != GIT_OK )
    {
        AddErrorString( _( "Failed to clean up repository state after fast-forward." ) );
        return PullResult::Error;
    }

    git_revwalk* revWalker = nullptr;

    // Collect commit details for updated references
    if( git_revwalk_new( &revWalker, GetRepo() ) != GIT_OK )
    {
        AddErrorString( _( "Failed to initialize revision walker." ) );
        return PullResult::Error;
    }

    KIGIT::GitRevWalkPtr revWalkerPtr( revWalker );
    git_revwalk_sorting( revWalker, GIT_SORT_TIME );

    if( git_revwalk_push_glob( revWalker, currentBranchName ) != GIT_OK )
    {
        AddErrorString( _( "Failed to push reference to revision walker." ) );
        return PullResult::Error;
    }

    std::pair<std::string, std::vector<CommitDetails>>& branchCommits = m_fetchResults.emplace_back();
    branchCommits.first = currentBranchName;

    git_oid commitOid;

    while( git_revwalk_next( &commitOid, revWalker ) == GIT_OK )
    {
        git_commit* commit = nullptr;

        if( git_commit_lookup( &commit, GetRepo(), &commitOid ) )
        {
            AddErrorString( wxString::Format( _( "Could not lookup commit '%s'" ),
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


PullResult GIT_PULL_HANDLER::handleMerge( const git_annotated_commit** aMergeHeads,
                                          size_t                       aMergeHeadsCount )
{
    if( hasUnstagedChanges( GetRepo() ) )
    {
        AddErrorString(
                _( "Cannot merge: you have unstaged changes. "
                   "Please commit or stash them before pulling." ) );
        return PullResult::DirtyWorkdir;
    }

    git_merge_options merge_opts;
    git_merge_options_init( &merge_opts, GIT_MERGE_OPTIONS_VERSION );

    git_checkout_options checkout_opts;
    git_checkout_init_options( &checkout_opts, GIT_CHECKOUT_OPTIONS_VERSION );

    checkout_opts.checkout_strategy = GIT_CHECKOUT_SAFE;

    if( git_merge( GetRepo(), aMergeHeads, aMergeHeadsCount, &merge_opts, &checkout_opts ) )
    {
        AddErrorString( _( "Could not merge commits" ) );
        return PullResult::Error;
    }

    // Get the repository index
    git_index* index = nullptr;

    if( git_repository_index( &index, GetRepo() ) )
    {
        AddErrorString( _( "Could not get repository index" ) );
        return PullResult::Error;
    }

    KIGIT::GitIndexPtr indexPtr( index );

    // Check for conflicts
    git_index_conflict_iterator* conflicts = nullptr;

    if( git_index_conflict_iterator_new( &conflicts, index ) )
    {
        AddErrorString( _( "Could not get conflict iterator" ) );
        return PullResult::Error;
    }

    KIGIT::GitIndexConflictIteratorPtr conflictsPtr( conflicts );

    const git_index_entry*    ancestor = nullptr;
    const git_index_entry*    our = nullptr;
    const git_index_entry*    their = nullptr;
    std::vector<ConflictData> conflict_data;

    while( git_index_conflict_next( &ancestor, &our, &their, conflicts ) == 0 )
    {
        // Case 3: Both files have changed
        if( ancestor && our && their )
        {
            ConflictData conflict_datum;
            conflict_datum.filename = our->path;
            conflict_datum.our_oid = our->id;
            conflict_datum.their_oid = their->id;
            conflict_datum.our_commit_time = our->mtime.seconds;
            conflict_datum.their_commit_time = their->mtime.seconds;
            conflict_datum.our_status = _( "Changed" );
            conflict_datum.their_status = _( "Changed" );
            conflict_datum.use_ours = true;

            conflict_data.push_back( conflict_datum );
        }
        // Case 4: File added in both ours and theirs
        else if( !ancestor && our && their )
        {
            ConflictData conflict_datum;
            conflict_datum.filename = our->path;
            conflict_datum.our_oid = our->id;
            conflict_datum.their_oid = their->id;
            conflict_datum.our_commit_time = our->mtime.seconds;
            conflict_datum.their_commit_time = their->mtime.seconds;
            conflict_datum.our_status = _( "Added" );
            conflict_datum.their_status = _( "Added" );
            conflict_datum.use_ours = true;

            conflict_data.push_back( conflict_datum );
        }
        // Case 1: Remote file has changed or been added, local file has not
        else if( their && !our )
        {
            // Accept their changes
            git_index_add( index, their );
        }
        // Case 2: Local file has changed or been added, remote file has not
        else if( our && !their )
        {
            // Accept our changes
            git_index_add( index, our );
        }
        else
        {
            wxLogError( wxS( "Unexpected conflict state" ) );
        }
    }

    if( conflict_data.empty() )
    {
        git_index_conflict_cleanup( index );
        git_index_write( index );
    }

    return conflict_data.empty() ? PullResult::Success : PullResult::MergeFailed;
}


PullResult GIT_PULL_HANDLER::handleRebase( const git_annotated_commit** aMergeHeads, size_t aMergeHeadsCount )
{
    if( hasUnstagedChanges( GetRepo() ) )
    {
        AddErrorString(
                _( "Cannot rebase: you have unstaged changes. "
                   "Please commit or stash them before pulling." ) );
        return PullResult::DirtyWorkdir;
    }

    // Get the current branch reference
    git_reference* head_ref = nullptr;

    if( git_repository_head( &head_ref, GetRepo() ) )
    {
        wxString errorMsg = KIGIT_COMMON::GetLastGitError();
        wxLogTrace( traceGit, "GIT_PULL_HANDLER::handleRebase() - Failed to get HEAD: %s", errorMsg );
        return PullResult::Error;
    }

    KIGIT::GitReferencePtr headRefPtr(head_ref);

    // Initialize rebase operation
    git_rebase* rebase = nullptr;
    git_rebase_options rebase_opts = GIT_REBASE_OPTIONS_INIT;
    rebase_opts.checkout_options.checkout_strategy = GIT_CHECKOUT_SAFE;

    if( git_rebase_init( &rebase, GetRepo(), nullptr, nullptr, aMergeHeads[0], &rebase_opts ) )
    {
        wxString errorMsg = KIGIT_COMMON::GetLastGitError();
        wxLogTrace( traceGit, "GIT_PULL_HANDLER::handleRebase() - Failed to initialize rebase: %s", errorMsg );
        return PullResult::Error;
    }

    KIGIT::GitRebasePtr   rebasePtr( rebase );
    git_rebase_operation* operation = nullptr;

    while( git_rebase_next( &operation, rebase ) != GIT_ITEROVER )
    {
        // Check for conflicts
        git_index* index = nullptr;
        if( git_repository_index( &index, GetRepo() ) )
        {
            wxLogTrace( traceGit, "GIT_PULL_HANDLER::handleRebase() - Failed to get index: %s",
                        KIGIT_COMMON::GetLastGitError() );
            return PullResult::Error;
        }
        KIGIT::GitIndexPtr indexPtr( index );

        if( git_index_has_conflicts( index ) )
        {
            // Abort the rebase if there are conflicts because we need to merge manually
            git_rebase_abort( rebase );
            AddErrorString( _( "Conflicts detected during rebase" ) );
            return PullResult::MergeFailed;
        }

        git_oid commit_id;
        git_signature* committer = nullptr;

        if( git_signature_default( &committer, GetRepo() ) )
        {
            wxLogTrace( traceGit, "GIT_PULL_HANDLER::handleRebase() - Failed to create signature: %s",
                KIGIT_COMMON::GetLastGitError() );
            return PullResult::Error;
        }

        KIGIT::GitSignaturePtr committerPtr( committer );

        if( git_rebase_commit( &commit_id, rebase, nullptr, committer, nullptr, nullptr ) != GIT_OK )
        {
            wxString errorMsg = KIGIT_COMMON::GetLastGitError();
            wxLogTrace( traceGit, "GIT_PULL_HANDLER::handleRebase() - Failed to commit operation: %s", errorMsg );
            git_rebase_abort( rebase );
            return PullResult::Error;
        }
    }

    // Finish the rebase
    if( git_rebase_finish( rebase, nullptr ) )
    {
        wxLogTrace( traceGit, "GIT_PULL_HANDLER::handleRebase() - Failed to finish rebase: %s",
                    KIGIT_COMMON::GetLastGitError() );
        return PullResult::Error;
    }

    wxLogTrace( traceGit, "GIT_PULL_HANDLER::handleRebase() - Rebase completed successfully" );
    git_repository_state_cleanup( GetRepo() );
    return PullResult::Success;
}


bool GIT_PULL_HANDLER::hasUnstagedChanges( git_repository* aRepo )
{
    if( !aRepo )
        return false;

    git_status_options opts;
    git_status_init_options( &opts, GIT_STATUS_OPTIONS_VERSION );

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

    for( size_t ii = 0; ii < count; ++ii )
    {
        const git_status_entry* entry = git_status_byindex( status_list, ii );

        if( entry->status & ( GIT_STATUS_WT_MODIFIED | GIT_STATUS_WT_DELETED | GIT_STATUS_WT_TYPECHANGE ) )
            return true;
    }

    return false;
}


void GIT_PULL_HANDLER::UpdateProgress( int aCurrent, int aTotal, const wxString& aMessage )
{

}
