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

#include "kicad_git_common.h"

#include <wx/filename.h>
#include <wx/log.h>
#include <map>
#include <vector>

KIGIT_COMMON::KIGIT_COMMON( git_repository* aRepo ) :
        m_repo( aRepo ), m_connType( GIT_CONN_TYPE::GIT_CONN_LOCAL ), m_testedTypes( 0 )
{}

KIGIT_COMMON::~KIGIT_COMMON()
{}

git_repository* KIGIT_COMMON::GetRepo() const
{
    return m_repo;
}

wxString KIGIT_COMMON::GetCurrentBranchName() const
{
    git_reference* head = nullptr;

    int retval = git_repository_head( &head, m_repo );

    if( retval && retval != GIT_EUNBORNBRANCH && retval != GIT_ENOTFOUND )
        return wxEmptyString;

    git_reference *branch;

    if( git_reference_resolve( &branch, head ) )
    {
        git_reference_free( head );
        return wxEmptyString;
    }

    git_reference_free( head );
    const char* branchName = "";

    if( git_branch_name( &branchName, branch ) )
    {
        git_reference_free( branch );
        return wxEmptyString;
    }

    git_reference_free( branch );

    return branchName;
}


std::vector<wxString> KIGIT_COMMON::GetBranchNames() const
{
    std::vector<wxString> branchNames;
    std::map<git_time_t, wxString> branchNamesMap;
    wxString firstName;

    git_branch_iterator* branchIterator = nullptr;

    if( git_branch_iterator_new( &branchIterator, m_repo, GIT_BRANCH_LOCAL ) )
        return branchNames;

    git_reference* branchReference = nullptr;
    git_branch_t branchType;

    while( git_branch_next( &branchReference, &branchType, branchIterator ) != GIT_ITEROVER )
    {
        const char* branchName = "";

        if( git_branch_name( &branchName, branchReference ) )
            continue;

        const git_oid* commitId = git_reference_target( branchReference );

        git_commit* commit = nullptr;

        if( git_commit_lookup( &commit, m_repo, commitId ) )
            continue;

        git_time_t commitTime = git_commit_time( commit );

        if( git_branch_is_head( branchReference ) )
            firstName = branchName;
        else
            branchNamesMap.emplace( commitTime, branchName );

        git_commit_free( commit );
        git_reference_free( branchReference );
    }

    git_branch_iterator_free( branchIterator );

    // Add the current branch to the top of the list
    if( !firstName.IsEmpty() )
        branchNames.push_back( firstName );

    // Add the remaining branches in order from newest to oldest
    for( auto rit = branchNamesMap.rbegin(); rit != branchNamesMap.rend(); ++rit )
        branchNames.push_back( rit->second );

    return branchNames;
}


std::vector<wxString> KIGIT_COMMON::GetProjectDirs()
{
    std::vector<wxString> projDirs;

    git_oid oid;
    git_commit* commit;
    git_tree *tree;

    if( git_reference_name_to_id( &oid, m_repo, "HEAD" ) != GIT_OK )
    {
        wxLogError( "An error occurred: %s", git_error_last()->message );
        return projDirs;
    }

    if( git_commit_lookup( &commit, m_repo, &oid ) != GIT_OK )
    {
        wxLogError( "An error occurred: %s", git_error_last()->message );
        return projDirs;
    }

    if( git_commit_tree( &tree, commit ) != GIT_OK )
    {
        wxLogError( "An error occurred: %s", git_error_last()->message );
        return projDirs;
    }

    // Define callback
    git_tree_walk(
            tree, GIT_TREEWALK_PRE,
            []( const char* root, const git_tree_entry* entry, void* payload )
            {
                std::vector<wxString>* prjs = static_cast<std::vector<wxString>*>( payload );
                wxFileName             root_fn( git_tree_entry_name( entry ) );

                root_fn.SetPath( root );

                if( git_tree_entry_type( entry ) == GIT_OBJECT_BLOB
                    && ( ( root_fn.GetExt() == "kicad_pro" ) || ( root_fn.GetExt() == "pro" ) ) )
                {
                    prjs->push_back( root_fn.GetFullPath() );
                }

                return 0; // continue walking
            },
            &projDirs );

    git_tree_free( tree );
    git_commit_free( commit );

    std::sort( projDirs.begin(), projDirs.end(),
               []( const wxString& a, const wxString& b )
               {
                    int a_freq = a.Freq( wxFileName::GetPathSeparator() );
                    int b_freq = b.Freq( wxFileName::GetPathSeparator() );

                    if( a_freq == b_freq )
                        return a < b;
                    else
                        return a_freq < b_freq;

               } );

    return projDirs;
}


std::pair<std::set<wxString>,std::set<wxString>> KIGIT_COMMON::GetDifferentFiles() const
{
    auto get_modified_files = [&]( git_oid* from_oid, git_oid* to_oid ) -> std::set<wxString>
    {
        std::set<wxString> modified_set;
        git_revwalk* walker = nullptr;

        if( git_revwalk_new( &walker, m_repo ) != GIT_OK )
            return modified_set;

        if( ( git_revwalk_push( walker, from_oid ) != GIT_OK )
            || ( git_revwalk_hide( walker, to_oid ) != GIT_OK ) )
        {
            git_revwalk_free( walker );
            return modified_set;
        }

        git_oid     oid;
        git_commit* commit;

        // iterate over all local commits not in remote
        while( git_revwalk_next( &oid, walker ) == GIT_OK )
        {
            if( git_commit_lookup( &commit, m_repo, &oid ) != GIT_OK )
                continue;

            git_tree *tree, *parent_tree = nullptr;
            if( git_commit_tree( &tree, commit ) != GIT_OK )
            {
                git_commit_free( commit );
                continue;
            }

            // get parent commit tree to diff against
            if( !git_commit_parentcount( commit ) )
            {
                git_tree_free( tree );
                git_commit_free( commit );
                continue;
            }


            git_commit* parent;
            if( git_commit_parent( &parent, commit, 0 ) != GIT_OK )
            {
                git_tree_free( tree );
                git_commit_free( commit );
                continue;
            }


            if( git_commit_tree( &parent_tree, parent ) != GIT_OK )
            {
                git_tree_free( tree );
                git_commit_free( commit );
                git_commit_free( parent );
                continue;
            }


            git_diff*        diff;
            git_diff_options diff_opts;
            git_diff_init_options( &diff_opts, GIT_DIFF_OPTIONS_VERSION );

            if( git_diff_tree_to_tree( &diff, m_repo, parent_tree, tree, &diff_opts ) == GIT_OK )
            {
                size_t num_deltas = git_diff_num_deltas( diff );

                for( size_t i = 0; i < num_deltas; ++i )
                {
                    const git_diff_delta* delta = git_diff_get_delta( diff, i );
                    modified_set.insert( delta->new_file.path );
                }

                git_diff_free( diff );
            }

            git_tree_free( parent_tree );
            git_commit_free( parent );
            git_tree_free( tree );
            git_commit_free( commit );
        }

        git_revwalk_free( walker );

        return modified_set;
    };

    std::pair<std::set<wxString>,std::set<wxString>> modified_files;

    if( !m_repo )
        return modified_files;

    git_reference* head = nullptr;
    git_reference* remote_head = nullptr;

    if( git_repository_head( &head, m_repo ) != GIT_OK )
        return modified_files;

    if( git_branch_upstream( &remote_head, head ) != GIT_OK )
    {
        git_reference_free( head );
        return modified_files;
    }

    git_oid head_oid = *git_reference_target( head );
    git_oid remote_oid = *git_reference_target( remote_head );

    git_reference_free( head );
    git_reference_free( remote_head );

    modified_files.first = get_modified_files( &head_oid, &remote_oid );
    modified_files.second = get_modified_files( &remote_oid, &head_oid );

    return modified_files;
}


bool KIGIT_COMMON::HasLocalCommits() const
{
    if( !m_repo )
        return false;

    git_reference* head = nullptr;
    git_reference* remote_head = nullptr;

    if( git_repository_head( &head, m_repo ) != GIT_OK )
        return false;

    if( git_branch_upstream( &remote_head, head ) != GIT_OK )
    {
        git_reference_free( head );
        return false;
    }

    git_oid head_oid = *git_reference_target( head );
    git_oid remote_oid = *git_reference_target( remote_head );

    git_reference_free( head );
    git_reference_free( remote_head );

    git_revwalk* walker = nullptr;

    if( git_revwalk_new( &walker, m_repo ) != GIT_OK )
        return false;

    if( ( git_revwalk_push( walker, &head_oid ) != GIT_OK )
        || ( git_revwalk_hide( walker, &remote_oid ) != GIT_OK ) )
    {
        git_revwalk_free( walker );
        return false;
    }

    git_oid oid;

    // If we can't walk to the next commit, then we are at or behind the remote
    if( git_revwalk_next( &oid, walker ) != GIT_OK )
    {
        git_revwalk_free( walker );
        return false;
    }

    git_revwalk_free( walker );
    return true;
}


bool KIGIT_COMMON::HasPushAndPullRemote() const
{
    git_remote* remote = nullptr;

    if( git_remote_lookup( &remote, m_repo, "origin" ) != GIT_OK )
    {
        return false;
    }

    // Get the URLs associated with the remote
    const char* fetch_url = git_remote_url( remote );
    const char* push_url = git_remote_pushurl( remote );

    // If no push URL is set, libgit2 defaults to using the fetch URL for pushing
    if( !push_url )
    {
        push_url = fetch_url;
    }

    // Clean up the remote object
    git_remote_free( remote );

    // Check if both URLs are valid (i.e., not NULL)
    return fetch_url && push_url;
}


extern "C" int fetchhead_foreach_cb( const char*, const char*,
                                     const git_oid* aOID, unsigned int aIsMerge, void* aPayload )
{
    if( aIsMerge )
        git_oid_cpy( (git_oid*) aPayload, aOID );

    return 0;
}


extern "C" void clone_progress_cb( const char* aStr, size_t aLen, size_t aTotal, void* data )
{
    KIGIT_COMMON* parent = (KIGIT_COMMON*) data;

    wxString progressMessage( aStr );
    parent->UpdateProgress( aLen, aTotal, progressMessage );
}


extern "C" int progress_cb( const char* str, int len, void* data )
{
    KIGIT_COMMON* parent = (KIGIT_COMMON*) data;

    wxString progressMessage( str, len );
    parent->UpdateProgress( 0, 0, progressMessage );

    return 0;
}

extern "C" int transfer_progress_cb( const git_transfer_progress* aStats, void* aPayload )
{
    KIGIT_COMMON* parent = (KIGIT_COMMON*) aPayload;
    wxString      progressMessage = wxString::Format( _( "Received %u of %u objects" ),
                                                      aStats->received_objects, aStats->total_objects );

    parent->UpdateProgress( aStats->received_objects, aStats->total_objects, progressMessage );

    return 0;
}

extern "C" int update_cb( const char* aRefname, const git_oid* aFirst, const git_oid* aSecond,
                          void* aPayload )
{
    constexpr int cstring_len = 8;
    char          a_str[cstring_len + 1];
    char          b_str[cstring_len + 1];

    KIGIT_COMMON* parent = (KIGIT_COMMON*) aPayload;
    wxString      status;

    git_oid_tostr( b_str, cstring_len, aSecond );

#if ( LIBGIT2_VER_MAJOR >= 1 ) || ( LIBGIT2_VER_MINOR >= 99 )
    if( !git_oid_is_zero( aFirst ) )
#else
    if( !git_oid_iszero( aFirst ) )
#endif
    {
        git_oid_tostr( a_str, cstring_len, aFirst );
        status = wxString::Format( _( "* [updated] %s..%s %s" ), a_str, b_str, aRefname );
    }
    else
    {
        status = wxString::Format( _( "* [new] %s %s" ), b_str, aRefname );
    }

    parent->UpdateProgress( 0, 0, status );

    return 0;
}


extern "C" int push_transfer_progress_cb( unsigned int aCurrent, unsigned int aTotal, size_t aBytes,
                                          void* aPayload )
{
    int64_t           progress = 100;
    KIGIT_COMMON* parent = (KIGIT_COMMON*) aPayload;

    if( aTotal != 0 )
    {
        progress = ( aCurrent * 100 ) / aTotal;
    }

    wxString progressMessage = wxString::Format( _( "Writing objects: %d%% (%d/%d), %d bytes" ),
                                                 progress, aCurrent, aTotal, aBytes );
    parent->UpdateProgress( aCurrent, aTotal, progressMessage );

    return 0;
}


extern "C" int push_update_reference_cb( const char* aRefname, const char* aStatus, void* aPayload )
{
    KIGIT_COMMON* parent = (KIGIT_COMMON*) aPayload;
    wxString      status( aStatus );

    if( !status.IsEmpty() )
    {
        wxString statusMessage = wxString::Format( _( "* [rejected] %s (%s)" ), aRefname, aStatus );
        parent->UpdateProgress( 0, 0, statusMessage );
    }
    else
    {
        wxString statusMessage = wxString::Format( _( "[updated] %s" ), aRefname );
        parent->UpdateProgress( 0, 0, statusMessage );
    }

    return 0;
}


extern "C" int credentials_cb( git_cred** aOut, const char* aUrl, const char* aUsername,
                                unsigned int aAllowedTypes, void* aPayload )
{
    KIGIT_COMMON* parent = static_cast<KIGIT_COMMON*>( aPayload );

    if( parent->GetConnType() == KIGIT_COMMON::GIT_CONN_TYPE::GIT_CONN_LOCAL )
        return GIT_PASSTHROUGH;

    if( aAllowedTypes & GIT_CREDTYPE_USERNAME
        && !( parent->TestedTypes() & GIT_CREDTYPE_USERNAME ) )
    {
        wxString username = parent->GetUsername().Trim().Trim( false );
        git_cred_username_new( aOut, username.ToStdString().c_str() );
        parent->TestedTypes() |= GIT_CREDTYPE_USERNAME;
    }
    else if( parent->GetConnType() == KIGIT_COMMON::GIT_CONN_TYPE::GIT_CONN_HTTPS
                && ( aAllowedTypes & GIT_CREDTYPE_USERPASS_PLAINTEXT )
                && !( parent->TestedTypes() & GIT_CREDTYPE_USERPASS_PLAINTEXT ) )
    {
        wxString username = parent->GetUsername().Trim().Trim( false );
        wxString password = parent->GetPassword().Trim().Trim( false );

        git_cred_userpass_plaintext_new( aOut, username.ToStdString().c_str(),
                                            password.ToStdString().c_str() );
        parent->TestedTypes() |= GIT_CREDTYPE_USERPASS_PLAINTEXT;
    }
    else if( parent->GetConnType() == KIGIT_COMMON::GIT_CONN_TYPE::GIT_CONN_SSH
                && ( aAllowedTypes & GIT_CREDTYPE_SSH_KEY )
                && !( parent->TestedTypes() & GIT_CREDTYPE_SSH_KEY ) )
    {
        // SSH key authentication
        wxString sshKey = parent->GetSSHKey();
        wxString sshPubKey = sshKey + ".pub";
        wxString username = parent->GetUsername().Trim().Trim( false );
        wxString password = parent->GetPassword().Trim().Trim( false );

        git_cred_ssh_key_new( aOut, username.ToStdString().c_str(),
                                sshPubKey.ToStdString().c_str(),
                                sshKey.ToStdString().c_str(),
                                password.ToStdString().c_str() );
        parent->TestedTypes() |= GIT_CREDTYPE_SSH_KEY;
    }
    else
    {
        return GIT_PASSTHROUGH;
    }

    return GIT_OK;
};
