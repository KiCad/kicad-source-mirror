/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2023 KiCad Developers, see AUTHORS.TXT for contributors.
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

#include <wx/log.h>

#include <iostream>
#include <time.h>

GIT_PULL_HANDLER::GIT_PULL_HANDLER( git_repository* aRepo ) :  KIGIT_COMMON( aRepo )
{}

GIT_PULL_HANDLER::~GIT_PULL_HANDLER()
{}


bool GIT_PULL_HANDLER::PerformFetch()
{
      // Fetch updates from remote repository
    git_remote* remote = nullptr;

    if( git_remote_lookup( &remote, m_repo, "origin" ) != 0 )
    {
        AddErrorString( wxString::Format( _( "Could not lookup remote '%s'" ), "origin" ).ToStdString() );
        return false;
    }

    git_remote_callbacks remoteCallbacks;
    git_remote_init_callbacks( &remoteCallbacks, GIT_REMOTE_CALLBACKS_VERSION );
    remoteCallbacks.sideband_progress = progress_cb;
    remoteCallbacks.transfer_progress = transfer_progress_cb;
    remoteCallbacks.credentials = credentials_cb;
    remoteCallbacks.payload = this;

    if( git_remote_connect( remote, GIT_DIRECTION_FETCH, &remoteCallbacks, nullptr, nullptr ) )
    {
        git_remote_free( remote );
        AddErrorString( wxString::Format( _( "Could not connect to remote '%s'" ), "origin" ).ToStdString() );
        return false;
    }

    git_fetch_options fetchOptions;
    git_fetch_init_options( &fetchOptions, GIT_FETCH_OPTIONS_VERSION );
    fetchOptions.callbacks = remoteCallbacks;

    if( git_remote_fetch( remote, nullptr, &fetchOptions, nullptr ) )
    {
        git_remote_free( remote );
        AddErrorString( wxString::Format( _( "Could not fetch data from remote '%s'" ), "origin" ) );
        return false;
    }

    git_remote_free( remote );

    return true;
}


PullResult GIT_PULL_HANDLER::PerformPull()
{
    PullResult result = PullResult::Success;

    if( !PerformFetch() )
        return PullResult::Error;

    git_oid pull_merge_oid = {};

    if( git_repository_fetchhead_foreach( m_repo, fetchhead_foreach_cb, &pull_merge_oid ) )
    {
        AddErrorString( _( "Could not read 'FETCH_HEAD'" ) );
        return PullResult::Error;
    }

    git_annotated_commit* fetchhead_commit;

    if( git_annotated_commit_lookup( &fetchhead_commit, m_repo, &pull_merge_oid ) )
    {
        AddErrorString( _( "Could not lookup commit" ) );
        return PullResult::Error;
    }

    const git_annotated_commit* merge_commits[] = { fetchhead_commit };
    git_merge_analysis_t merge_analysis;
    git_merge_preference_t merge_preference = GIT_MERGE_PREFERENCE_NONE;

    if( git_merge_analysis( &merge_analysis, &merge_preference, m_repo, merge_commits, 1 ) )
    {
        AddErrorString( _( "Could not analyze merge" ) );
        git_annotated_commit_free( fetchhead_commit );
        return PullResult::Error;
    }

    if( !( merge_analysis & GIT_MERGE_ANALYSIS_NORMAL ) )
        git_annotated_commit_free( fetchhead_commit );

    if( merge_analysis & GIT_MERGE_ANALYSIS_UNBORN )
    {
        AddErrorString( _( "Invalid HEAD.  Cannot merge." ) );
        return PullResult::MergeFailed;
    }

    // Nothing to do if the repository is up to date
    if( merge_analysis & GIT_MERGE_ANALYSIS_UP_TO_DATE )
    {
        git_repository_state_cleanup( m_repo );
        return PullResult::UpToDate;
    }

    // Fast-forward is easy, just update the local reference
    if( merge_analysis & GIT_MERGE_ANALYSIS_FASTFORWARD )
    {
        return handleFastForward();
    }

    if( merge_analysis & GIT_MERGE_ANALYSIS_NORMAL )
    {
        PullResult ret = handleMerge( merge_commits, 1 );
        git_annotated_commit_free( fetchhead_commit );
        return ret;
    }

    //TODO: handle merges when they need to be resolved

    return result;
}

const std::vector<std::pair<std::string, std::vector<CommitDetails>>>& GIT_PULL_HANDLER::GetFetchResults() const {
    return m_fetchResults;
}

std::string GIT_PULL_HANDLER::getFirstLineFromCommitMessage( const std::string& aMessage )
{
    size_t firstLineEnd = aMessage.find_first_of( '\n' );

    if( firstLineEnd != std::string::npos )
        return aMessage.substr( 0, firstLineEnd );

    return aMessage;
}

std::string GIT_PULL_HANDLER::getFormattedCommitDate( const git_time& aTime )
{
    char dateBuffer[64];
    time_t time = static_cast<time_t>( aTime.time );
    strftime( dateBuffer, sizeof( dateBuffer ), "%Y-%b-%d %H:%M:%S", gmtime( &time ) );
    return dateBuffer;
}


PullResult GIT_PULL_HANDLER::handleFastForward()
{
        // Update local references with fetched data
        git_reference* updatedRef = nullptr;

        if( git_repository_head( &updatedRef, m_repo ) )
        {
            AddErrorString( _( "Could not get repository head" ) );
            return PullResult::Error;
        }

        const char* updatedRefName = git_reference_name( updatedRef );
        git_reference_free( updatedRef );

        git_oid     updatedRefOid;
        if( git_reference_name_to_id( &updatedRefOid, m_repo, updatedRefName ) )
        {
            AddErrorString( wxString::Format( _( "Could not get reference OID for reference '%s'" ), updatedRefName ) );
            return PullResult::Error;
        }

        git_checkout_options checkoutOptions;
        git_checkout_init_options( &checkoutOptions, GIT_CHECKOUT_OPTIONS_VERSION );
        checkoutOptions.checkout_strategy = GIT_CHECKOUT_SAFE;
        if( git_checkout_head( m_repo, &checkoutOptions ) )
        {
            AddErrorString( _( "Failed to perform checkout operation." ) );
            return PullResult::Error;
        }

        // Collect commit details for updated references
        git_revwalk* revWalker = nullptr;
        git_revwalk_new( &revWalker, m_repo );
        git_revwalk_sorting( revWalker, GIT_SORT_TIME );
        git_revwalk_push_glob( revWalker, updatedRefName );

        git_oid commitOid;
        while( git_revwalk_next( &commitOid, revWalker ) == 0 )
        {
            git_commit* commit = nullptr;

            if( git_commit_lookup( &commit, m_repo, &commitOid ) )
            {
                AddErrorString( wxString::Format( _( "Could not lookup commit '{}'" ), git_oid_tostr_s( &commitOid ) ) );
                git_revwalk_free( revWalker );
                return PullResult::Error;
            }

            CommitDetails details;
            details.m_sha = git_oid_tostr_s( &commitOid );
            details.m_firstLine = getFirstLineFromCommitMessage( git_commit_message( commit ) );
            details.m_author = git_commit_author( commit )->name;
            details.m_date = getFormattedCommitDate( git_commit_author( commit )->when );

            std::pair<std::string, std::vector<CommitDetails>>& branchCommits =
                    m_fetchResults.emplace_back();
            branchCommits.first = updatedRefName;
            branchCommits.second.push_back( details );

            //TODO: log these to the parent
            git_commit_free( commit );
        }

        git_revwalk_free( revWalker );

        git_repository_state_cleanup( m_repo );
        return PullResult::FastForward;
}


PullResult GIT_PULL_HANDLER::handleMerge( const git_annotated_commit** aMergeHeads,
                                          size_t                       aMergeHeadsCount )
{
    git_merge_options    merge_opts;
    git_merge_options_init( &merge_opts, GIT_MERGE_OPTIONS_VERSION );

    git_checkout_options checkout_opts;
    git_checkout_init_options( &checkout_opts, GIT_CHECKOUT_OPTIONS_VERSION );

    checkout_opts.checkout_strategy = GIT_CHECKOUT_SAFE;

    if( git_merge( m_repo, aMergeHeads, aMergeHeadsCount, &merge_opts, &checkout_opts ) )
    {
        AddErrorString( _( "Could not merge commits" ) );
        return PullResult::Error;
    }

    // Get the repository index
    git_index* index = nullptr;
    if( git_repository_index( &index, m_repo ) )
    {
        AddErrorString( _( "Could not get repository index" ) );
        return PullResult::Error;
    }

    // Check for conflicts
    git_index_conflict_iterator* conflicts = nullptr;
    if( git_index_conflict_iterator_new( &conflicts, index ) )
    {
        AddErrorString( _( "Could not get conflict iterator" ) );
        return PullResult::Error;
    }

    const git_index_entry* ancestor = nullptr;
    const git_index_entry* our = nullptr;
    const git_index_entry* their = nullptr;
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

    git_index_conflict_iterator_free( conflicts );
    git_index_free( index );

    return conflict_data.empty() ? PullResult::Success : PullResult::MergeFailed;
}


void GIT_PULL_HANDLER::UpdateProgress( int aCurrent, int aTotal, const wxString& aMessage )
{
    ReportProgress( aCurrent, aTotal, aMessage );
}
