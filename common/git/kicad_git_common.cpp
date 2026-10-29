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

#include "kicad_git_common.h"
#include "kicad_git_memory.h"
#include "git_repo_mixin.h"
#include "git_backend.h"
#include "kigit_ssh_keys.h"
#include "project_git_utils.h"

#include <git/git_progress.h>
#include <git/kicad_git_compat.h>
#include <kiplatform/secrets.h>
#include <trace_helpers.h>

#include <git2.h>
#include <wx/arrstr.h>
#include <wx/base64.h>
#include <wx/filename.h>
#include <wx/log.h>
#include <wx/utils.h>
#include <algorithm>
#include <cctype>
#include <cerrno>
#include <chrono>
#include <cstdlib>
#include <cstring>
#include <map>
#include <set>
#include <string>
#include <thread>
#include <vector>


wxWindow*& KIGIT::ActiveAdvancedGitWindow()
{
    static wxWindow* window = nullptr;

    return window;
}


KIGIT_COMMON::KIGIT_COMMON( git_repository* aRepo, const wxString& aProjectPath ) :
        m_repo( aRepo ),
        m_projectPath( aProjectPath ),
        m_connType( GIT_CONN_TYPE::GIT_CONN_LOCAL ),
        m_testedTypes( 0 ),
        m_nextPublicKey( 0 ),
        m_secretFetched( false )
{}


KIGIT_COMMON::KIGIT_COMMON( const KIGIT_COMMON& aOther ) :
        // Initialize base class and member variables
        m_repo( aOther.m_repo ),
        m_projectDir( aOther.m_projectDir ),
        m_projectPath( aOther.m_projectPath ),
        m_connType( aOther.m_connType ),
        m_remote( aOther.m_remote ),
        m_urlUsername( aOther.m_urlUsername ),
        m_hostname( aOther.m_hostname ),
        m_port( aOther.m_port ),
        m_username( aOther.m_username ),
        m_password( aOther.m_password ),
        m_testedTypes( aOther.m_testedTypes.load() ),

        // The mutex is default-initialized, not copied
        m_gitActionMutex(),
        m_discoveredKeys( aOther.m_discoveredKeys ),
        m_publicKeys( aOther.m_publicKeys ),
        m_publicKeysHost( aOther.m_publicKeysHost ),
        m_projectSSHKey( aOther.m_projectSSHKey ),
        m_nextPublicKey( aOther.m_nextPublicKey ),
        m_secretFetched( aOther.m_secretFetched )
{
}


KIGIT_COMMON::~KIGIT_COMMON()
{}


git_repository* KIGIT_COMMON::GetRepo() const
{
    return m_repo;
}


wxString KIGIT_COMMON::GetProjectDir() const
{
    if( !m_projectDir.IsEmpty() )
        return m_projectDir;

    if( m_repo )
    {
        const char* workdir = git_repository_workdir( m_repo );

        if( workdir )
            return wxString( workdir );
    }

    return wxEmptyString;
}


std::optional<wxString> KIGIT_COMMON::GetProjectPrefix() const
{
    if( !m_repo || !git_repository_workdir( m_repo ) )
        return std::nullopt;

    return KIGIT::PROJECT_GIT_UTILS::ProjectPrefix( GetProjectDir(), m_projectPath );
}


wxString KIGIT_COMMON::GetCurrentBranchName() const
{
    if( !m_repo )
        return wxEmptyString;

    git_reference* head = nullptr;

    int retval = git_repository_head( &head, m_repo );

    if( retval && retval != GIT_EUNBORNBRANCH && retval != GIT_ENOTFOUND )
        return wxEmptyString;

    // An unborn or missing HEAD leaves head null, so there is no branch to resolve.
    if( !head )
        return wxEmptyString;

    KIGIT::GitReferencePtr headPtr( head );
    git_reference*         branch;

    if( git_reference_resolve( &branch, head ) )
    {
        wxLogTrace( traceGit, "Failed to resolve branch" );
        return wxEmptyString;
    }

    KIGIT::GitReferencePtr branchPtr( branch );
    const char*            branchName = "";

    if( git_branch_name( &branchName, branch ) )
    {
        wxLogTrace( traceGit, "Failed to get branch name" );
        return wxEmptyString;
    }

    return wxString( branchName );
}


wxString KIGIT_COMMON::GetHeadCommitId() const
{
    git_oid oid;

    if( !m_repo || git_reference_name_to_id( &oid, m_repo, "HEAD" ) != GIT_OK )
        return wxEmptyString;

    return wxString::FromUTF8( git_oid_tostr_s( &oid ) );
}


wxString KIGIT_COMMON::GetPassword()
{
    wxString remote;
    wxString username;

    {
        std::lock_guard<std::mutex> lock( m_credentialsMutex );

        if( m_secretFetched )
            return m_password;

        if( m_connType == GIT_CONN_TYPE::GIT_CONN_LOCAL || m_remote.IsEmpty() )
        {
            m_secretFetched = true;
            return m_password;
        }

        remote = m_remote;
        username = m_username;
    }

    // The lock is not held here: asking the operating system can raise a permission dialog, so
    // it finishes only once a person has decided.
    wxString secret;
    bool     found = KIPLATFORM::SECRETS::GetSecret( remote, username, secret );

    std::lock_guard<std::mutex> lock( m_credentialsMutex );

    // The user may have typed a password, or changed the remote, while the keychain was being
    // asked. Either way this answer is about the wrong thing and is dropped.
    if( m_secretFetched || m_remote != remote || m_username != username )
        return m_password;

    if( found )
        m_password = secret;

    m_secretFetched = true;
    return m_password;
}


wxString KIGIT_COMMON::GetLastCommitMessage() const
{
    if( !m_repo )
        return wxEmptyString;

    git_oid headOid;

    if( git_reference_name_to_id( &headOid, m_repo, "HEAD" ) != GIT_OK )
        return wxEmptyString;

    git_commit* commit = nullptr;

    if( git_commit_lookup( &commit, m_repo, &headOid ) != GIT_OK )
        return wxEmptyString;

    KIGIT::GitCommitPtr commitPtr( commit );
    const char*         message = git_commit_message( commit );

    return message ? KIGIT::DecodeRepoText( message ) : wxString();
}


bool KIGIT_COMMON::HasUncommittedTrackedChanges( bool* aReadOk ) const
{
    if( aReadOk )
        *aReadOk = false;

    if( !m_repo )
        return false;

    git_status_options statusOpts;
    git_status_init_options( &statusOpts, GIT_STATUS_OPTIONS_VERSION );
    statusOpts.show = GIT_STATUS_SHOW_INDEX_AND_WORKDIR;

    git_status_list* statusList = nullptr;

    if( git_status_list_new( &statusList, m_repo, &statusOpts ) != GIT_OK )
        return false;

    KIGIT::GitStatusListPtr statusListPtr( statusList );

    if( aReadOk )
        *aReadOk = true;

    return git_status_list_entrycount( statusList ) > 0;
}


namespace
{

// The refs pointing at each commit, keyed by the commit's full hash, HEAD first.
std::map<std::string, std::vector<wxString>> collectRefNames( git_repository* aRepo )
{
    std::map<std::string, std::vector<wxString>> names;

    git_oid headOid;

    if( git_reference_name_to_id( &headOid, aRepo, "HEAD" ) == GIT_OK )
        names[git_oid_tostr_s( &headOid )].push_back( wxS( "HEAD" ) );

    git_reference_iterator* iter = nullptr;

    if( git_reference_iterator_new( &iter, aRepo ) != GIT_OK )
        return names;

    KIGIT::GitReferenceIteratorPtr iterPtr( iter );
    git_reference*                 ref = nullptr;

    // Iteration is by ref name, which puts local branches before remote ones and tags last.
    while( git_reference_next( &ref, iter ) == GIT_OK )
    {
        KIGIT::GitReferencePtr refPtr( ref );

        if( !git_reference_is_branch( ref ) && !git_reference_is_remote( ref ) && !git_reference_is_tag( ref ) )
            continue;

        git_object* target = nullptr;

        if( git_reference_peel( &target, ref, GIT_OBJECT_COMMIT ) != GIT_OK )
            continue;

        KIGIT::GitObjectPtr targetPtr( target );

        std::string hash = git_oid_tostr_s( git_object_id( target ) );
        names[hash].push_back( KIGIT::DecodeRepoText( git_reference_shorthand( ref ) ) );
    }

    return names;
}


wxString shortHashOf( const git_oid* aOid )
{
    // A 7-char abbreviation can be ambiguous in a large repository, which makes a later
    // revparse of the hash fail. Twelve characters is what git uses for big repos.
    char shortHash[13] = { 0 };
    git_oid_tostr( shortHash, sizeof( shortHash ), aOid );
    return wxString::FromUTF8( shortHash );
}


KIGIT::FILE_CHANGE_STATUS deltaStatus( git_delta_t aDelta )
{
    switch( aDelta )
    {
    case GIT_DELTA_ADDED: return KIGIT::FILE_CHANGE_STATUS::ADDED;
    case GIT_DELTA_DELETED: return KIGIT::FILE_CHANGE_STATUS::REMOVED;
    case GIT_DELTA_RENAMED: return KIGIT::FILE_CHANGE_STATUS::RENAMED;
    case GIT_DELTA_COPIED: return KIGIT::FILE_CHANGE_STATUS::COPIED;
    case GIT_DELTA_TYPECHANGE: return KIGIT::FILE_CHANGE_STATUS::TYPECHANGE;
    default: return KIGIT::FILE_CHANGE_STATUS::MODIFIED;
    }
}

} // namespace


wxDateTime KIGIT_COMMON::GetLastFetchTime() const
{
    if( !m_repo )
        return wxDateTime();

    const char* gitDir = git_repository_path( m_repo );

    if( !gitDir )
        return wxDateTime();

    // Git rewrites FETCH_HEAD on every fetch, so its timestamp is the last one.
    wxFileName fetchHead( wxString::FromUTF8( gitDir ), wxS( "FETCH_HEAD" ) );

    if( !fetchHead.FileExists() )
        return wxDateTime();

    return fetchHead.GetModificationTime();
}


KIGIT_STASH_DETAIL KIGIT_COMMON::GetStashDetail( const wxString& aId ) const
{
    KIGIT_STASH_DETAIL detail;

    if( !m_repo )
        return detail;

    git_oid oid;

    if( git_oid_fromstr( &oid, aId.utf8_string().c_str() ) != GIT_OK )
        return detail;

    git_commit* commit = nullptr;

    if( git_commit_lookup( &commit, m_repo, &oid ) != GIT_OK )
        return detail;

    KIGIT::GitCommitPtr commitPtr( commit );

    detail.found = true;
    detail.when = wxDateTime( static_cast<time_t>( git_commit_time( commit ) ) );

    // Git writes "WIP on <branch>: <commit> <subject>", or "On <branch>: <message>" for a
    // message of the user's own. The branch is kept apart from the rest.
    wxString raw;

    if( const char* message = git_commit_message( commit ) )
        raw = KIGIT::DecodeRepoText( message );

    raw.Trim();
    detail.message = raw;

    wxString rest;

    if( raw.StartsWith( wxS( "WIP on " ), &rest ) || raw.StartsWith( wxS( "On " ), &rest ) )
    {
        int colon = rest.Find( wxS( ": " ) );

        if( colon != wxNOT_FOUND )
        {
            detail.branch = rest.Left( colon );
            detail.message = rest.Mid( colon + 2 );
        }
    }

    // The first parent is the commit the stash was taken on. A third parent, when there is
    // one, holds the untracked files.
    git_commit* base = nullptr;

    if( git_commit_parent( &base, commit, 0 ) != GIT_OK )
        return detail;

    KIGIT::GitCommitPtr basePtr( base );
    detail.baseHash = shortHashOf( git_commit_id( base ) );

    git_tree* baseTree = nullptr;
    git_tree* stashTree = nullptr;

    if( git_commit_tree( &baseTree, base ) != GIT_OK || git_commit_tree( &stashTree, commit ) != GIT_OK )
        return detail;

    KIGIT::GitTreePtr baseTreePtr( baseTree );
    KIGIT::GitTreePtr stashTreePtr( stashTree );
    git_diff*         diff = nullptr;

    if( git_diff_tree_to_tree( &diff, m_repo, baseTree, stashTree, nullptr ) == GIT_OK )
    {
        KIGIT::GitDiffPtr diffPtr( diff );

        KIGIT::CollectDiffDeltas( diff,
                                  [&detail]( const git_diff_delta& aDelta )
                                  {
                                      const char* path =
                                              aDelta.new_file.path ? aDelta.new_file.path : aDelta.old_file.path;
                                      detail.files.push_back( wxString::FromUTF8( path ) );
                                  } );
    }

    if( git_commit_parentcount( commit ) > 2 )
    {
        git_commit* untracked = nullptr;

        if( git_commit_parent( &untracked, commit, 2 ) == GIT_OK )
        {
            KIGIT::GitCommitPtr untrackedPtr( untracked );
            git_tree*           untrackedTree = nullptr;

            if( git_commit_tree( &untrackedTree, untracked ) == GIT_OK )
            {
                KIGIT::GitTreePtr untrackedTreePtr( untrackedTree );

                git_tree_walk(
                        untrackedTree, GIT_TREEWALK_PRE,
                        []( const char* aRoot, const git_tree_entry* aEntry, void* aPayload ) -> int
                        {
                            if( git_tree_entry_type( aEntry ) == GIT_OBJECT_BLOB )
                            {
                                auto* files = static_cast<std::vector<wxString>*>( aPayload );
                                files->push_back( wxString::FromUTF8( aRoot )
                                                  + wxString::FromUTF8( git_tree_entry_name( aEntry ) ) );
                            }

                            return 0;
                        },
                        &detail.files );
            }
        }
    }

    return detail;
}


std::pair<int, int> KIGIT_COMMON::GetWorkingTreeLineStats( const std::set<wxString>& aPaths ) const
{
    if( !m_repo )
        return { 0, 0 };

    // An unborn repository has no HEAD tree, and then everything in the tree is new.
    KIGIT::GitTreePtr headTree;
    git_object*       headObj = nullptr;

    if( git_revparse_single( &headObj, m_repo, "HEAD^{tree}" ) == GIT_OK )
        headTree.reset( reinterpret_cast<git_tree*>( headObj ) );

    git_diff_options opts;
    git_diff_options_init( &opts, GIT_DIFF_OPTIONS_VERSION );
    opts.flags = GIT_DIFF_INCLUDE_UNTRACKED | GIT_DIFF_RECURSE_UNTRACKED_DIRS | GIT_DIFF_SHOW_UNTRACKED_CONTENT;

    git_diff* diff = nullptr;

    if( git_diff_tree_to_workdir_with_index( &diff, m_repo, headTree.get(), &opts ) != GIT_OK )
        return { 0, 0 };

    KIGIT::GitDiffPtr   diffPtr( diff );
    std::pair<int, int> total{ 0, 0 };

    for( size_t ii = 0; ii < git_diff_num_deltas( diff ); ++ii )
    {
        git_patch* patch = nullptr;

        if( git_patch_from_diff( &patch, diff, ii ) != GIT_OK )
            continue;

        KIGIT::GitPatchPtr    patchPtr( patch );
        const git_diff_delta* delta = git_patch_get_delta( patch );
        const char*           path = delta->new_file.path ? delta->new_file.path : delta->old_file.path;

        if( !path || aPaths.count( wxString::FromUTF8( path ) ) == 0 )
            continue;

        size_t added = 0;
        size_t removed = 0;

        if( git_patch_line_stats( nullptr, &added, &removed, patch ) == GIT_OK )
        {
            total.first += static_cast<int>( added );
            total.second += static_cast<int>( removed );
        }
    }

    return total;
}


std::vector<KIGIT_BRANCH_INFO> KIGIT_COMMON::GetBranches() const
{
    if( !m_repo )
        return {};

    std::vector<KIGIT_BRANCH_INFO> branches;

    git_branch_iterator* branchIterator = nullptr;

    if( git_branch_iterator_new( &branchIterator, m_repo, GIT_BRANCH_ALL ) )
    {
        wxLogTrace( traceGit, "Failed to get branch iterator" );
        return {};
    }

    KIGIT::GitBranchIteratorPtr branchIteratorPtr( branchIterator );
    git_reference*              branchReference = nullptr;
    git_branch_t                branchType;

    // Only GIT_ITEROVER means the end. Any other error leaves the reference invalid.
    while( true )
    {
        branchReference = nullptr;

        int rc = git_branch_next( &branchReference, &branchType, branchIterator );

        if( rc == GIT_ITEROVER )
            break;

        if( rc != GIT_OK )
        {
            wxLogTrace( traceGit, "Failed to iterate branches: %s", KIGIT_COMMON::GetLastGitError() );
            break;
        }

        const char*            branchName = "";
        KIGIT::GitReferencePtr branchReferencePtr( branchReference );

        // origin/HEAD and other symbolic aliases do not name a separate branch.
        if( git_reference_type( branchReference ) != GIT_REFERENCE_DIRECT )
            continue;

        if( git_branch_name( &branchName, branchReference ) )
        {
            wxLogTrace( traceGit, "Failed to get branch name in iter loop" );
            continue;
        }

        const git_oid* commitId = git_reference_target( branchReference );
        git_commit*    commit = nullptr;

        if( !commitId || git_commit_lookup( &commit, m_repo, commitId ) != GIT_OK )
        {
            wxLogTrace( traceGit, "Failed to get commit in iter loop" );
            continue;
        }

        KIGIT::GitCommitPtr commitPtr( commit );
        git_time_t          commitTime = git_commit_time( commit );

        KIGIT_BRANCH_INFO info;
        info.name = KIGIT::DecodeRepoText( branchName );
        info.isRemote = branchType == GIT_BRANCH_REMOTE;
        info.isCurrent = !info.isRemote && git_branch_is_head( branchReference ) == 1;
        info.tipHash = shortHashOf( commitId );
        info.message = KIGIT::DecodeRepoText( git_commit_message( commit ) );
        info.when = wxDateTime( static_cast<time_t>( commitTime ) );

        branches.push_back( std::move( info ) );
    }

    return branches;
}


KIGIT_BRANCH_DETAIL KIGIT_COMMON::GetBranchDetail( const wxString& aBranch ) const
{
    KIGIT_BRANCH_DETAIL detail;

    if( !m_repo )
        return detail;

    // The short name as the Branches page lists it, local or remote, resolved as git would.
    git_reference* ref = nullptr;

    if( git_reference_dwim( &ref, m_repo, aBranch.utf8_string().c_str() ) != GIT_OK )
        return detail;

    KIGIT::GitReferencePtr refPtr( ref );
    git_object*            target = nullptr;

    if( git_reference_peel( &target, ref, GIT_OBJECT_COMMIT ) != GIT_OK )
        return detail;

    KIGIT::GitObjectPtr targetPtr( target );
    git_commit*         commit = reinterpret_cast<git_commit*>( target );

    detail.found = true;
    detail.name = aBranch;
    detail.tipHash = shortHashOf( git_commit_id( commit ) );
    detail.when = wxDateTime( static_cast<time_t>( git_commit_time( commit ) ) );

    if( const git_signature* author = git_commit_author( commit ) )
    {
        detail.author = KIGIT::DecodeRepoText( author->name );
        detail.email = KIGIT::DecodeRepoText( author->email );
    }

    if( const char* message = git_commit_message( commit ) )
        detail.message = KIGIT::DecodeRepoText( message );

    detail.message.Trim();

    // Only a local branch tracks another. A remote-tracking branch has nothing to compare with.
    git_reference* upstream = nullptr;

    if( !git_reference_is_branch( ref ) || git_branch_upstream( &upstream, ref ) != GIT_OK )
        return detail;

    KIGIT::GitReferencePtr upstreamPtr( upstream );
    const char*            upstreamName = "";

    if( git_branch_name( &upstreamName, upstream ) == GIT_OK )
        detail.upstream = KIGIT::DecodeRepoText( upstreamName );

    detail.divergence = compareRefs( ref, upstream );

    return detail;
}


std::vector<wxString> KIGIT_COMMON::GetProjectDirs()
{
    if( !m_repo )
        return {};

    std::vector<wxString> projDirs;

    git_oid oid;
    git_commit* commit;
    git_tree *tree;

    if( git_reference_name_to_id( &oid, m_repo, "HEAD" ) != GIT_OK )
    {
        wxLogTrace( traceGit, "An error occurred: %s", KIGIT_COMMON::GetLastGitError() );
        return projDirs;
    }

    if( git_commit_lookup( &commit, m_repo, &oid ) != GIT_OK )
    {
        wxLogTrace( traceGit, "An error occurred: %s", KIGIT_COMMON::GetLastGitError() );
        return projDirs;
    }

    KIGIT::GitCommitPtr commitPtr( commit );

    if( git_commit_tree( &tree, commit ) != GIT_OK )
    {
        wxLogTrace( traceGit, "An error occurred: %s", KIGIT_COMMON::GetLastGitError() );
        return projDirs;
    }

    KIGIT::GitTreePtr treePtr( tree );

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


std::pair<std::set<wxString>, std::set<wxString>> KIGIT_COMMON::GetDifferentFiles() const
{
    std::pair<std::set<wxString>, std::set<wxString>> modified_files;

    if( !m_repo || IsShuttingDown() )
        return modified_files;

    git_reference* head = nullptr;
    git_reference* remote_head = nullptr;

    if( git_repository_head( &head, m_repo ) != GIT_OK )
    {
        wxLogTrace( traceGit, "Failed to get modified HEAD" );
        return modified_files;
    }

    KIGIT::GitReferencePtr headPtr( head );

    if( git_branch_upstream( &remote_head, head ) != GIT_OK )
    {
        // Branch has no upstream tracking ref configured.  Without an upstream there is
        // nothing to compare against, so leave both sets empty.  Walking commits unbounded
        // would otherwise dump the entire history's root tree into the result.
        wxLogTrace( traceGit, "Failed to get modified remote HEAD" );
        return modified_files;
    }

    KIGIT::GitReferencePtr remoteHeadPtr( remote_head );

    const git_oid* head_oid = git_reference_target( head );
    const git_oid* remote_oid = git_reference_target( remote_head );

    if( !head_oid || !remote_oid )
        return modified_files;

    auto load_tree = [this]( const git_oid* aOid ) -> git_tree*
    {
        git_commit* commit = nullptr;

        if( git_commit_lookup( &commit, m_repo, aOid ) != GIT_OK )
        {
            wxLogTrace( traceGit, "Failed to lookup commit for diff: %s", KIGIT_COMMON::GetLastGitError() );
            return nullptr;
        }

        KIGIT::GitCommitPtr commitPtr( commit );
        git_tree*           tree = nullptr;

        if( git_commit_tree( &tree, commit ) != GIT_OK )
        {
            wxLogTrace( traceGit, "Failed to get commit tree for diff: %s", KIGIT_COMMON::GetLastGitError() );
            return nullptr;
        }

        return tree;
    };

    git_tree*         head_tree = load_tree( head_oid );
    KIGIT::GitTreePtr headTreePtr( head_tree );

    git_tree*         remote_tree = load_tree( remote_oid );
    KIGIT::GitTreePtr remoteTreePtr( remote_tree );

    if( !head_tree || !remote_tree )
        return modified_files;

    // Find the merge-base so AHEAD and BEHIND can be distinguished.  AHEAD = files that
    // changed between merge-base and HEAD (only in local commits).  BEHIND = files that
    // changed between merge-base and the remote tip (only in remote commits).  Without a
    // shared history the merge-base lookup fails; in that case we treat both sets as empty
    // because there is no meaningful "ahead vs behind" partition to compute.
    git_oid base_oid;

    if( git_merge_base( &base_oid, m_repo, head_oid, remote_oid ) != GIT_OK )
    {
        wxLogTrace( traceGit, "No merge base between local and remote: %s", KIGIT_COMMON::GetLastGitError() );
        return modified_files;
    }

    git_tree*         base_tree = load_tree( &base_oid );
    KIGIT::GitTreePtr baseTreePtr( base_tree );

    if( !base_tree )
        return modified_files;

    auto collect_paths = [this]( git_tree* aOldTree, git_tree* aNewTree, std::set<wxString>& aOut )
    {
        if( IsShuttingDown() )
            return;

        git_diff_options localOpts;
        git_diff_init_options( &localOpts, GIT_DIFF_OPTIONS_VERSION );

        git_diff* diff = nullptr;

        if( git_diff_tree_to_tree( &diff, m_repo, aOldTree, aNewTree, &localOpts ) != GIT_OK )
        {
            wxLogTrace( traceGit, "Failed to diff trees: %s", KIGIT_COMMON::GetLastGitError() );
            return;
        }

        KIGIT::CollectDiffDeltas( diff,
                                  [&aOut]( const git_diff_delta& aDelta )
                                  {
                                      if( aDelta.new_file.path )
                                          aOut.insert( wxString::FromUTF8( aDelta.new_file.path ) );

                                      if( aDelta.old_file.path )
                                          aOut.insert( wxString::FromUTF8( aDelta.old_file.path ) );
                                  } );

        git_diff_free( diff );
    };

    collect_paths( base_tree, head_tree, modified_files.first );    // AHEAD
    collect_paths( base_tree, remote_tree, modified_files.second ); // BEHIND

    // Filter both sets to files whose content actually differs between HEAD and remote.
    // Without this, a file touched by a commit that has since been replaced with an
    // identical-tree commit (e.g. a message-only amend) keeps an AHEAD marker even
    // though its blob matches the remote's blob.
    std::set<wxString> actuallyDifferent;
    collect_paths( head_tree, remote_tree, actuallyDifferent );

    auto filterToDifferent = [&]( std::set<wxString>& aSet )
    {
        for( auto it = aSet.begin(); it != aSet.end(); )
            it = actuallyDifferent.count( *it ) ? std::next( it ) : aSet.erase( it );
    };

    filterToDifferent( modified_files.first );
    filterToDifferent( modified_files.second );

    return modified_files;
}


std::optional<KIGIT_AHEAD_BEHIND> KIGIT_COMMON::compareRefs( const git_reference* aBranch,
                                                             const git_reference* aBase ) const
{
    if( !m_repo || !aBranch || !aBase )
        return std::nullopt;

    git_object* branchCommit = nullptr;

    if( git_reference_peel( &branchCommit, aBranch, GIT_OBJECT_COMMIT ) != GIT_OK )
        return std::nullopt;

    KIGIT::GitObjectPtr branchPtr( branchCommit );
    git_object*         baseCommit = nullptr;

    if( git_reference_peel( &baseCommit, aBase, GIT_OBJECT_COMMIT ) != GIT_OK )
        return std::nullopt;

    KIGIT::GitObjectPtr basePtr( baseCommit );
    KIGIT_AHEAD_BEHIND  counts;

    int result = git_graph_ahead_behind( &counts.ahead, &counts.behind, m_repo, git_object_id( branchCommit ),
                                         git_object_id( baseCommit ) );

    if( result != GIT_OK )
        return std::nullopt;

    return counts;
}


bool KIGIT_COMMON::HasLocalCommits() const
{
    if( !m_repo || IsHeadDetached() )
        return false;

    git_reference* head = nullptr;

    if( git_repository_head( &head, m_repo ) != GIT_OK )
        return false;

    KIGIT::GitReferencePtr headPtr( head );
    git_reference*         upstream = nullptr;
    int                    result = git_branch_upstream( &upstream, head );

    if( result != GIT_OK )
    {
        // Treat a branch without an upstream as unpublished.
        return result == GIT_ENOTFOUND;
    }

    KIGIT::GitReferencePtr            upstreamPtr( upstream );
    std::optional<KIGIT_AHEAD_BEHIND> counts = compareRefs( head, upstream );

    return counts && counts->ahead > 0;
}


bool KIGIT_COMMON::IsHeadDetached() const
{
    return m_repo && git_repository_head_detached( m_repo ) == 1;
}


bool KIGIT_COMMON::IsPullRebaseConfigured() const
{
    if( !m_repo )
        return false;

    git_config* config = nullptr;

    if( git_repository_config( &config, m_repo ) != GIT_OK )
        return false;

    KIGIT::GitConfigPtr configPtr( config );
    int                 rebase = 0;

    return git_config_get_bool( &rebase, config, "pull.rebase" ) == GIT_OK && rebase;
}


bool KIGIT_COMMON::HasUpstreamCommits() const
{
    if( !m_repo )
        return false;

    git_reference* head = nullptr;

    if( git_repository_head( &head, m_repo ) != GIT_OK )
        return false;

    KIGIT::GitReferencePtr headPtr( head );
    git_reference*         upstream = nullptr;

    if( git_branch_upstream( &upstream, head ) != GIT_OK )
        return false;

    KIGIT::GitReferencePtr            upstreamPtr( upstream );
    std::optional<KIGIT_AHEAD_BEHIND> counts = compareRefs( head, upstream );

    return counts && counts->behind > 0;
}


std::optional<size_t> KIGIT_COMMON::GetUnmergedCommitCount( const wxString& aBranch ) const
{
    if( !m_repo )
        return std::nullopt;

    git_reference* branch = nullptr;

    if( git_branch_lookup( &branch, m_repo, aBranch.utf8_string().c_str(), GIT_BRANCH_LOCAL ) != GIT_OK )
        return std::nullopt;

    KIGIT::GitReferencePtr branchPtr( branch );
    git_reference*         head = nullptr;

    if( git_repository_head( &head, m_repo ) != GIT_OK )
        return std::nullopt;

    KIGIT::GitReferencePtr            headPtr( head );
    std::optional<KIGIT_AHEAD_BEHIND> counts = compareRefs( branch, head );

    if( !counts )
        return std::nullopt;

    return counts->ahead;
}


std::vector<wxString> KIGIT_COMMON::GetRecentCommitMessages( size_t aMax ) const
{
    std::vector<wxString> messages;

    if( !m_repo )
        return messages;

    git_revwalk* walker = nullptr;

    if( git_revwalk_new( &walker, m_repo ) != GIT_OK )
        return messages;

    KIGIT::GitRevWalkPtr walkerPtr( walker );
    git_revwalk_sorting( walker, GIT_SORT_TIME | GIT_SORT_TOPOLOGICAL );

    if( git_revwalk_push_head( walker ) != GIT_OK )
        return messages;

    std::set<wxString> seen;
    git_oid            oid;

    while( messages.size() < aMax && git_revwalk_next( &oid, walker ) == GIT_OK )
    {
        git_commit* commit = nullptr;

        if( git_commit_lookup( &commit, m_repo, &oid ) != GIT_OK )
            continue;

        KIGIT::GitCommitPtr commitPtr( commit );
        wxString            message;

        if( const char* text = git_commit_message( commit ) )
            message = KIGIT::DecodeRepoText( text );

        message.Trim();

        if( !message.IsEmpty() && seen.insert( message ).second )
            messages.push_back( message );
    }

    return messages;
}


namespace
{

// The id of the tree entry at aPath in aCommit's tree, or empty when there is none.
std::string treeEntryIdAt( git_commit* aCommit, const char* aPath )
{
    git_tree* tree = nullptr;

    if( git_commit_tree( &tree, aCommit ) != GIT_OK )
        return {};

    KIGIT::GitTreePtr treePtr( tree );
    git_tree_entry*   entry = nullptr;

    if( git_tree_entry_bypath( &entry, tree, aPath ) != GIT_OK )
        return {};

    KIGIT::GitTreeEntryPtr entryPtr( entry );

    return git_oid_tostr_s( git_tree_entry_id( entry ) );
}


// True when aCommit changed what sits at aPath, judged against its first parent. A folder works
// like a file, since a folder's entry id changes with anything under it.
bool commitTouchesPath( git_commit* aCommit, const std::string& aPath )
{
    std::string after = treeEntryIdAt( aCommit, aPath.c_str() );
    std::string before;

    if( git_commit_parentcount( aCommit ) > 0 )
    {
        git_commit* parent = nullptr;

        if( git_commit_parent( &parent, aCommit, 0 ) == GIT_OK )
        {
            KIGIT::GitCommitPtr parentPtr( parent );
            before = treeEntryIdAt( parent, aPath.c_str() );
        }
    }

    return after != before;
}

} // namespace


std::vector<KIGIT_COMMIT_INFO> KIGIT_COMMON::GetRecentCommits( size_t aMax, const KIGIT_HISTORY_FILTER& aFilter ) const
{
    std::vector<KIGIT_COMMIT_INFO> commits;
    CANCEL_TOKEN                   cancel = ActiveCancelToken();

    if( !m_repo || IsShuttingDown() || ( cancel && cancel->load() ) )
        return commits;

    git_revwalk* walker = nullptr;

    if( git_revwalk_new( &walker, m_repo ) != GIT_OK )
        return commits;

    KIGIT::GitRevWalkPtr walkerPtr( walker );

    // Time order, but a child always before its parent, so two tips made in the same second
    // still list in a fixed order.
    git_revwalk_sorting( walker, GIT_SORT_TIME | GIT_SORT_TOPOLOGICAL );

    if( aFilter.branch.IsEmpty() )
    {
        git_revwalk_push_head( walker );
        git_revwalk_push_glob( walker, "refs/heads/*" );
        git_revwalk_push_glob( walker, "refs/remotes/*" );
    }
    else
    {
        // The short name as the Branches page lists it, local or remote, resolved as git would.
        git_reference* ref = nullptr;

        if( git_reference_dwim( &ref, m_repo, aFilter.branch.utf8_string().c_str() ) != GIT_OK )
            return commits;

        KIGIT::GitReferencePtr refPtr( ref );
        git_object*            target = nullptr;

        if( git_reference_peel( &target, ref, GIT_OBJECT_COMMIT ) != GIT_OK )
            return commits;

        KIGIT::GitObjectPtr targetPtr( target );
        git_revwalk_push( walker, git_object_id( target ) );
    }

    std::map<std::string, std::vector<wxString>> refNames = collectRefNames( m_repo );
    wxString                                     search = aFilter.search.Lower();
    std::string                                  path = aFilter.path.utf8_string();
    git_oid                                      oid;

    while( commits.size() < aMax && git_revwalk_next( &oid, walker ) == GIT_OK )
    {
        // A search without matches can walk the entire repository. Let a newer query or a
        // closing project release the action lock before that walk reaches its end.
        if( ( cancel && cancel->load() ) || IsShuttingDown() )
            break;

        git_commit* commit = nullptr;

        if( git_commit_lookup( &commit, m_repo, &oid ) != GIT_OK )
            continue;

        KIGIT::GitCommitPtr commitPtr( commit );

        if( !path.empty() && !commitTouchesPath( commit, path ) )
            continue;

        KIGIT_COMMIT_INFO info;
        info.hash = shortHashOf( &oid );

        if( const char* summary = git_commit_summary( commit ) )
            info.summary = KIGIT::DecodeRepoText( summary );

        if( const git_signature* author = git_commit_author( commit ) )
        {
            info.author = KIGIT::DecodeRepoText( author->name );
            info.email = KIGIT::DecodeRepoText( author->email );
        }

        info.when = wxDateTime( static_cast<time_t>( git_commit_time( commit ) ) );

        std::string fullHash = git_oid_tostr_s( &oid );

        auto matches = [&]( const wxString& aText )
        {
            return aText.Lower().Contains( search );
        };

        if( !search.IsEmpty() && !matches( info.summary ) && !matches( info.author ) && !matches( info.email )
            && !matches( fullHash ) )
        {
            continue;
        }

        auto named = refNames.find( fullHash );

        if( named != refNames.end() )
            info.refs = named->second;

        commits.push_back( info );
    }

    return commits;
}


KIGIT_COMMIT_DETAIL KIGIT_COMMON::GetCommitDetail( const wxString& aHash ) const
{
    KIGIT_COMMIT_DETAIL detail;

    if( !m_repo )
        return detail;

    git_object* obj = nullptr;

    if( git_revparse_single( &obj, m_repo, aHash.utf8_string().c_str() ) != GIT_OK )
        return detail;

    KIGIT::GitObjectPtr objPtr( obj );

    if( git_object_type( obj ) != GIT_OBJECT_COMMIT )
        return detail;

    git_commit*    commit = reinterpret_cast<git_commit*>( obj );
    const git_oid* oid = git_commit_id( commit );

    detail.found = true;
    detail.hash = wxString::FromUTF8( git_oid_tostr_s( oid ) );
    detail.shortHash = shortHashOf( oid );

    if( const char* message = git_commit_message( commit ) )
        detail.message = KIGIT::DecodeRepoText( message );

    detail.message.Trim();

    if( const git_signature* author = git_commit_author( commit ) )
    {
        detail.author = KIGIT::DecodeRepoText( author->name );
        detail.email = KIGIT::DecodeRepoText( author->email );
    }

    detail.when = wxDateTime( static_cast<time_t>( git_commit_time( commit ) ) );

    for( unsigned ii = 0; ii < git_commit_parentcount( commit ); ++ii )
        detail.parents.push_back( shortHashOf( git_commit_parent_id( commit, ii ) ) );

    std::map<std::string, std::vector<wxString>> refNames = collectRefNames( m_repo );
    auto                                         named = refNames.find( git_oid_tostr_s( oid ) );

    if( named != refNames.end() )
        detail.refs = named->second;

    git_tree* tree = nullptr;

    if( git_commit_tree( &tree, commit ) != GIT_OK )
        return detail;

    KIGIT::GitTreePtr treePtr( tree );

    // A root commit diffs against the empty tree, so its files all count as added.
    KIGIT::GitTreePtr parentTree;

    if( git_commit_parentcount( commit ) > 0 )
    {
        git_commit* parent = nullptr;

        if( git_commit_parent( &parent, commit, 0 ) == GIT_OK )
        {
            KIGIT::GitCommitPtr parentPtr( parent );
            git_tree*           raw = nullptr;

            if( git_commit_tree( &raw, parent ) == GIT_OK )
                parentTree.reset( raw );
        }
    }

    git_diff* diff = nullptr;

    if( git_diff_tree_to_tree( &diff, m_repo, parentTree.get(), tree, nullptr ) != GIT_OK )
        return detail;

    KIGIT::GitDiffPtr diffPtr( diff );

    git_diff_find_options findOpts;
    git_diff_find_options_init( &findOpts, GIT_DIFF_FIND_OPTIONS_VERSION );
    findOpts.flags = GIT_DIFF_FIND_RENAMES | GIT_DIFF_FIND_COPIES;
    git_diff_find_similar( diff, &findOpts );

    for( size_t ii = 0; ii < git_diff_num_deltas( diff ); ++ii )
    {
        git_patch* patch = nullptr;

        if( git_patch_from_diff( &patch, diff, ii ) != GIT_OK )
            continue;

        KIGIT::GitPatchPtr    patchPtr( patch );
        const git_diff_delta* delta = git_patch_get_delta( patch );

        KIGIT_COMMIT_FILE file;
        file.path = wxString::FromUTF8( delta->new_file.path ? delta->new_file.path : delta->old_file.path );
        file.status = deltaStatus( delta->status );
        file.binary = ( delta->flags & GIT_DIFF_FLAG_BINARY ) != 0;

        if( delta->status == GIT_DELTA_RENAMED || delta->status == GIT_DELTA_COPIED )
            file.oldPath = wxString::FromUTF8( delta->old_file.path );

        size_t added = 0;
        size_t removed = 0;

        if( git_patch_line_stats( nullptr, &added, &removed, patch ) == GIT_OK )
        {
            file.added = static_cast<int>( added );
            file.removed = static_cast<int>( removed );
        }

        detail.added += file.added;
        detail.removed += file.removed;
        detail.files.push_back( file );
    }

    return detail;
}


namespace
{
struct STASH_COLLECT
{
    git_repository*                repo;
    std::vector<KIGIT_STASH_INFO>* out;
};


int collectStash( size_t aIndex, const char* aMessage, const git_oid* aStashId, void* aPayload )
{
    STASH_COLLECT* ctx = static_cast<STASH_COLLECT*>( aPayload );

    KIGIT_STASH_INFO info;
    info.index = aIndex;
    info.message = KIGIT::DecodeRepoText( aMessage );

    char hash[GIT_OID_HEXSZ + 1] = { 0 };
    git_oid_tostr( hash, sizeof( hash ), aStashId );
    info.id = wxString::FromUTF8( hash );

    git_commit* commit = nullptr;

    if( git_commit_lookup( &commit, ctx->repo, aStashId ) == GIT_OK )
    {
        KIGIT::GitCommitPtr commitPtr( commit );
        info.when = wxDateTime( static_cast<time_t>( git_commit_time( commit ) ) );
    }

    ctx->out->push_back( info );
    return 0;
}
} // namespace


std::vector<KIGIT_STASH_INFO> KIGIT_COMMON::GetStashes() const
{
    std::vector<KIGIT_STASH_INFO> stashes;

    if( !m_repo )
        return stashes;

    STASH_COLLECT ctx{ m_repo, &stashes };
    git_stash_foreach( m_repo, collectStash, &ctx );

    return stashes;
}


std::vector<KIGIT_TAG_INFO> KIGIT_COMMON::GetTags() const
{
    std::vector<KIGIT_TAG_INFO> tags;

    if( !m_repo )
        return tags;

    git_strarray tagNames = { nullptr, 0 };

    if( git_tag_list( &tagNames, m_repo ) != GIT_OK )
        return tags;

    KIGIT::GitStrArrayPtr tagNamesPtr( &tagNames );

    for( size_t i = 0; i < tagNames.count; ++i )
    {
        wxString         tagName = KIGIT::DecodeRepoText( tagNames.strings[i] );
        KIGIT_TAG_DETAIL detail = GetTagDetail( tagName );
        KIGIT_TAG_INFO   info;
        info.name = tagName;

        // A tag on a tree or blob has no commit to show, but it is still a tag.
        if( detail.found )
        {
            info.hash = detail.hash;
            info.summary = detail.message.BeforeFirst( '\n' );
            info.when = detail.when;
        }

        tags.push_back( info );
    }

    // Newest first, as the History page lists commits. A tag with no date goes last.
    std::stable_sort( tags.begin(), tags.end(),
                      []( const KIGIT_TAG_INFO& a, const KIGIT_TAG_INFO& b )
                      {
                          if( !a.when.IsValid() || !b.when.IsValid() )
                              return a.when.IsValid();

                          return a.when > b.when;
                      } );

    return tags;
}


std::vector<std::pair<wxString, wxString>> KIGIT_COMMON::GetRemotes() const
{
    std::vector<std::pair<wxString, wxString>> remotes;

    if( !m_repo )
        return remotes;

    git_strarray names = { nullptr, 0 };

    if( git_remote_list( &names, m_repo ) != GIT_OK )
        return remotes;

    KIGIT::GitStrArrayPtr namesPtr( &names );

    for( size_t i = 0; i < names.count; ++i )
    {
        wxString    name = wxString::FromUTF8( names.strings[i] );
        git_remote* remote = nullptr;

        if( git_remote_lookup( &remote, m_repo, names.strings[i] ) != GIT_OK )
            continue;

        KIGIT::GitRemotePtr remotePtr( remote );

        const char* url = git_remote_url( remote );
        remotes.emplace_back( name, url ? KIGIT::StripEmbeddedUserinfo( wxString::FromUTF8( url ) ) : wxString() );
    }

    return remotes;
}


bool KIGIT_COMMON::IsHeadUnborn() const
{
    return m_repo && git_repository_head_unborn( m_repo ) == 1;
}


bool KIGIT_COMMON::StartUnbornBranch( const wxString& aName )
{
    // Pointing HEAD at a name does not check it the way creating a branch does.
    if( !m_repo || !IsHeadUnborn() || !KIGIT::BranchNameError( aName ).IsEmpty() )
        return false;

    wxString ref = wxS( "refs/heads/" ) + aName;

    return git_repository_set_head( m_repo, ref.utf8_string().c_str() ) == GIT_OK;
}


bool KIGIT_COMMON::CreateBranch( const wxString& aName, const wxString& aFrom )
{
    if( !m_repo || !KIGIT::BranchNameError( aName ).IsEmpty() )
        return false;

    return GetGitBackend()->CreateBranch( m_repo, aName, aFrom ) == 0;
}


bool KIGIT_COMMON::DeleteBranch( const wxString& aName )
{
    return GetGitBackend()->DeleteBranch( m_repo, aName );
}


bool KIGIT_COMMON::StashSave( const wxString& aMessage )
{
    return GetGitBackend()->StashSave( m_repo, aMessage );
}


bool KIGIT_COMMON::StashApply( size_t aIndex )
{
    return GetGitBackend()->StashApply( m_repo, aIndex );
}


bool KIGIT_COMMON::StashPop( size_t aIndex )
{
    return GetGitBackend()->StashPop( m_repo, aIndex );
}


bool KIGIT_COMMON::StashDrop( size_t aIndex )
{
    return GetGitBackend()->StashDrop( m_repo, aIndex );
}


bool KIGIT_COMMON::CreateTag( const wxString& aName, const wxString& aMessage )
{
    return GetGitBackend()->CreateTag( m_repo, aName, aMessage );
}


KIGIT_TAG_DETAIL KIGIT_COMMON::GetTagDetail( const wxString& aName ) const
{
    KIGIT_TAG_DETAIL detail;

    if( !m_repo )
        return detail;

    git_object* obj = nullptr;
    wxString    spec = wxS( "refs/tags/" ) + aName;

    if( git_revparse_single( &obj, m_repo, spec.utf8_string().c_str() ) != GIT_OK )
        return detail;

    KIGIT::GitObjectPtr objPtr( obj );

    // An annotated tag is its own object on the way to the commit, with a message and a tagger
    // of its own.
    git_object* peeled = nullptr;

    if( git_object_peel( &peeled, obj, GIT_OBJECT_COMMIT ) != GIT_OK )
        return detail;

    KIGIT::GitObjectPtr peeledPtr( peeled );
    git_commit*         commit = reinterpret_cast<git_commit*>( peeled );

    detail.found = true;
    detail.name = aName;
    detail.hash = shortHashOf( git_commit_id( commit ) );

    const char*          message = nullptr;
    const git_signature* who = nullptr;

    if( git_object_type( obj ) == GIT_OBJECT_TAG )
    {
        git_tag* tag = reinterpret_cast<git_tag*>( obj );
        message = git_tag_message( tag );
        who = git_tag_tagger( tag );
    }

    if( !message )
        message = git_commit_message( commit );

    bool annotated = who != nullptr;

    if( !who )
        who = git_commit_author( commit );

    if( message )
        detail.message = KIGIT::DecodeRepoText( message );

    detail.message.Trim();

    if( who )
    {
        detail.author = KIGIT::DecodeRepoText( who->name );
        detail.email = KIGIT::DecodeRepoText( who->email );
        detail.when = wxDateTime( static_cast<time_t>( annotated ? who->when.time : git_commit_time( commit ) ) );
    }

    return detail;
}


bool KIGIT_COMMON::DeleteTag( const wxString& aName )
{
    return GetGitBackend()->DeleteTag( m_repo, aName );
}


bool KIGIT_COMMON::CheckoutTag( const wxString& aName )
{
    return GetGitBackend()->CheckoutTag( m_repo, aName );
}


bool KIGIT_COMMON::AddRemote( const wxString& aName, const wxString& aUrl )
{
    return GetGitBackend()->AddRemote( m_repo, aName, aUrl );
}


bool KIGIT_COMMON::SetRemoteUrl( const wxString& aName, const wxString& aUrl )
{
    return GetGitBackend()->SetRemoteUrl( m_repo, aName, aUrl );
}


bool KIGIT_COMMON::RemoveRemote( const wxString& aName )
{
    return GetGitBackend()->RemoveRemote( m_repo, aName );
}


bool KIGIT_COMMON::SetUserConfig( const wxString& aName, const wxString& aEmail )
{
    return GetGitBackend()->SetUserConfig( m_repo, aName, aEmail );
}


RestoreResult KIGIT_COMMON::RestoreVersion( const wxString& aCommitHash, KIGIT_ERROR_MESSAGE& aError )
{
    return GetGitBackend()->RestoreVersion( *this, aCommitHash, aError );
}


bool KIGIT_COMMON::DiscardChanges( const std::vector<wxString>& aPaths, wxString& aError )
{
    return GetGitBackend()->DiscardChanges( m_repo, aPaths, aError );
}


bool KIGIT_COMMON::IsPathInHead( const wxString& aPath )
{
    return GetGitBackend()->IsPathInHead( m_repo, aPath );
}


bool KIGIT_COMMON::IsPathConflicted( const wxString& aPath ) const
{
    if( !m_repo || aPath.IsEmpty() )
        return false;

    git_index* index = nullptr;

    if( git_repository_index( &index, m_repo ) != GIT_OK )
        return false;

    KIGIT::GitIndexPtr indexPtr( index );

    const git_index_entry* ancestor = nullptr;
    const git_index_entry* ours = nullptr;
    const git_index_entry* theirs = nullptr;

    return git_index_conflict_get( &ancestor, &ours, &theirs, index, aPath.utf8_string().c_str() ) == GIT_OK;
}


bool KIGIT_COMMON::IsOperationInProgress() const
{
    return m_repo && git_repository_state( m_repo ) != GIT_REPOSITORY_STATE_NONE;
}


GitOperation KIGIT_COMMON::OperationInProgress() const
{
    if( !m_repo )
        return GitOperation::None;

    switch( git_repository_state( m_repo ) )
    {
    case GIT_REPOSITORY_STATE_NONE: return GitOperation::None;
    case GIT_REPOSITORY_STATE_MERGE: return GitOperation::Merge;
    case GIT_REPOSITORY_STATE_REBASE:
    case GIT_REPOSITORY_STATE_REBASE_INTERACTIVE:
    case GIT_REPOSITORY_STATE_REBASE_MERGE:
    case GIT_REPOSITORY_STATE_APPLY_MAILBOX_OR_REBASE: return GitOperation::Rebase;
    case GIT_REPOSITORY_STATE_CHERRYPICK:
    case GIT_REPOSITORY_STATE_CHERRYPICK_SEQUENCE: return GitOperation::CherryPick;
    case GIT_REPOSITORY_STATE_REVERT:
    case GIT_REPOSITORY_STATE_REVERT_SEQUENCE: return GitOperation::Revert;
    default: return GitOperation::Other;
    }
}


bool KIGIT_COMMON::AbandonOperation( wxString& aError )
{
    return GetGitBackend()->AbandonOperation( m_repo, aError );
}


bool KIGIT_COMMON::GetMergeConflicts( const wxString& aStageDir, wxString& aHeadOid, wxString& aTheirsOid,
                                      std::vector<KIGIT_CONFLICT_SIDES>& aConflicts, wxString& aError )
{
    return GetGitBackend()->GetMergeConflicts( m_repo, aStageDir, aHeadOid, aTheirsOid, aConflicts, aError );
}


MergeCommitResult KIGIT_COMMON::CreateMergedCommit( const wxString& aExpectedHeadOid, const wxString& aTheirsOid,
                                                    const std::vector<std::pair<wxString, wxString>>& aResolvedFiles,
                                                    KIGIT_ERROR_MESSAGE&                              aError )
{
    return GetGitBackend()->CreateMergedCommit( m_repo, aExpectedHeadOid, aTheirsOid, aResolvedFiles, aError );
}


void KIGIT_COMMON::AddRefreshListener( void* aOwner, std::function<void()> aCallback )
{
    m_refreshListeners.emplace_back( aOwner, std::move( aCallback ) );
}


void KIGIT_COMMON::RemoveRefreshListener( void* aOwner )
{
    std::erase_if( m_refreshListeners,
                   [aOwner]( const auto& aEntry )
                   {
                       return aEntry.first == aOwner;
                   } );
}


void KIGIT_COMMON::NotifyRefreshListeners()
{
    // Copy first: a listener could add or remove one while it runs.
    std::vector<std::pair<void*, std::function<void()>>> listeners = m_refreshListeners;

    for( const auto& [owner, callback] : listeners )
    {
        // An earlier listener in this round can have removed this one.
        bool registered = std::any_of( m_refreshListeners.begin(), m_refreshListeners.end(),
                                       [target = owner]( const auto& aEntry )
                                       {
                                           return aEntry.first == target;
                                       } );

        if( registered )
            callback();
    }
}


bool KIGIT_COMMON::HasPushAndPullRemote() const
{
    if( !m_repo )
        return false;


    // Prefer the upstream remote configured for the current branch.  Fall back to
    // "origin" and then to any remote that exposes a fetch URL so that repos
    // cloned with non-default remote names also enable push/pull operations.
    auto checkRemote =
            [this]( const char* aName ) -> bool
            {
                git_remote* remote = nullptr;

                if( git_remote_lookup( &remote, m_repo, aName ) != GIT_OK )
                    return false;

                KIGIT::GitRemotePtr remotePtr( remote );

                const char* fetch_url = git_remote_url( remote );
                const char* push_url = git_remote_pushurl( remote );

                // libgit2 defaults to the fetch URL for pushing when no push URL is set
                if( !push_url )
                    push_url = fetch_url;

                return fetch_url && push_url;
            };

    std::string preferred = GetRemoteNameOrDefault().utf8_string();

    if( checkRemote( preferred.c_str() ) )
        return true;

    if( preferred != "origin" && checkRemote( "origin" ) )
        return true;

    git_strarray remotes = { nullptr, 0 };

    if( git_remote_list( &remotes, m_repo ) != GIT_OK )
    {
        wxLogTrace( traceGit, "Failed to enumerate remotes for haspushpull" );
        return false;
    }

    KIGIT::GitStrArrayPtr remotesPtr( &remotes );

    for( size_t ii = 0; ii < remotes.count; ++ii )
    {
        if( checkRemote( remotes.strings[ii] ) )
            return true;
    }

    return false;
}


wxString KIGIT_COMMON::GetRemoteNameOrDefault() const
{
    wxString remoteName = GetRemotename();

    if( remoteName.IsEmpty() )
        remoteName = wxS( "origin" );

    return remoteName;
}


wxString KIGIT_COMMON::remoteNameWithoutUpstream() const
{
    wxString     retval;
    git_strarray remotes = { nullptr, 0 };

    if( git_remote_list( &remotes, m_repo ) == GIT_OK )
    {
        // No upstream tracking branch.  Prefer "origin" when present, otherwise pick the
        // single configured remote.  Returning empty for ambiguous (multiple) remotes
        // tells callers to fall back to whatever default they want.
        if( remotes.count == 1 )
        {
            retval = remotes.strings[0];
        }
        else
        {
            for( size_t ii = 0; ii < remotes.count; ++ii )
            {
                if( strcmp( remotes.strings[ii], "origin" ) == 0 )
                {
                    retval = remotes.strings[ii];
                    break;
                }
            }
        }

        git_strarray_dispose( &remotes );
    }
    else
    {
        wxLogTrace( traceGit, "Failed to list remotes: %s", KIGIT_COMMON::GetLastGitError() );

        // If we can't get the remote name from the upstream branch or the list of remotes,
        // just return the default remote name

        git_remote* remote = nullptr;

        if( git_remote_lookup( &remote, m_repo, "origin" ) == GIT_OK )
        {
            retval = git_remote_name( remote );
            git_remote_free( remote );
        }
        else
        {
            wxLogTrace( traceGit, "Failed to get remote name from default remote: %s",
                        KIGIT_COMMON::GetLastGitError() );
        }
    }

    return retval;
}


wxString KIGIT_COMMON::GetRemotename() const
{
    if( !m_repo )
        return wxEmptyString;


    wxString retval;
    git_reference* head = nullptr;
    git_reference* upstream = nullptr;

    // An unborn HEAD has no branch to track, but the remote still exists
    if( git_repository_head( &head, m_repo ) != GIT_OK )
    {
        wxLogTrace( traceGit, "Failed to get remote name: %s", KIGIT_COMMON::GetLastGitError() );
        return remoteNameWithoutUpstream();
    }

    KIGIT::GitReferencePtr headPtr( head );

    if( git_branch_upstream( &upstream, head ) != GIT_OK )
    {
        wxLogTrace( traceGit, "Failed to get upstream branch: %s", KIGIT_COMMON::GetLastGitError() );
        return remoteNameWithoutUpstream();
    }

    KIGIT::GitReferencePtr upstreamPtr( upstream );
    git_buf     remote_name = GIT_BUF_INIT_CONST( nullptr, 0 );

    if( git_branch_remote_name( &remote_name, m_repo, git_reference_name( upstream ) ) == GIT_OK )
    {
        retval = remote_name.ptr;
        git_buf_dispose( &remote_name );
    }
    else
    {
        wxLogTrace( traceGit,
                    "Failed to get remote name from upstream branch: %s",
                    KIGIT_COMMON::GetLastGitError() );
    }

    return retval;
}


void KIGIT_COMMON::SetSSHKey( const wxString& aKey )
{
    // GetNextPublicKey reads an empty result as the end of the list, so an empty key at the
    // front would hide every key behind it.
    if( aKey.IsEmpty() )
        return;

    std::lock_guard<std::mutex> lock( m_credentialsMutex );
    m_projectSSHKey = aKey;
    rebuildPublicKeysUnlocked();
}


wxString KIGIT_COMMON::GetSSHKey() const
{
    std::lock_guard<std::mutex> lock( m_credentialsMutex );

    if( !m_projectSSHKey.IsEmpty() )
        return m_projectSSHKey;

    return m_publicKeys.empty() ? wxString() : m_publicKeys.front();
}


wxString KIGIT_COMMON::GetFirstOfferedSSHKey() const
{
    std::lock_guard<std::mutex> lock( m_credentialsMutex );

    if( KIGIT::IsOfferableSSHKey( m_projectSSHKey ) )
        return m_projectSSHKey;

    for( const wxString& key : m_publicKeys )
    {
        if( KIGIT::IsOfferableSSHKey( key ) )
            return key;
    }

    return wxEmptyString;
}


KIGIT_COMMON::CREDENTIAL_IDENTITY KIGIT_COMMON::GetCredentialIdentity() const
{
    std::lock_guard<std::mutex> lock( m_credentialsMutex );
    return { m_remote, m_connType, m_username };
}


bool KIGIT_COMMON::ApplyPromptedCredentials( const CREDENTIAL_IDENTITY& aFor, const PROMPTED_CREDENTIALS& aEntered )
{
    std::lock_guard<std::mutex> lock( m_credentialsMutex );

    if( m_remote != aFor.remote || m_connType != aFor.connType || m_username != aFor.username )
        return false;

    m_connType = aEntered.connType;

    if( aEntered.username )
        m_username = *aEntered.username;

    m_password = aEntered.password;
    m_secretFetched = true;

    // As SetSSHKey, which also refuses an empty key
    if( !aEntered.sshKey.IsEmpty() )
    {
        m_projectSSHKey = aEntered.sshKey;
        rebuildPublicKeysUnlocked();
    }

    m_nextPublicKey = 0;
    m_testedTypes = 0;
    return true;
}


int KIGIT_COMMON::SSHPortFor( const wxString& aHost ) const
{
    std::lock_guard<std::mutex> lock( m_credentialsMutex );

    if( m_port < 0 || !m_hostname.IsSameAs( aHost, false ) )
        return -1;

    return m_port == 0 ? 22 : m_port;
}


std::unique_lock<std::mutex> KIGIT_COMMON::TryLockActionFor( std::chrono::milliseconds aWait )
{
    const auto                   deadline = std::chrono::steady_clock::now() + aWait;
    std::unique_lock<std::mutex> lock( m_gitActionMutex, std::defer_lock );

    // Not a timed mutex, so poll. A status read lets go within a few of these
    while( !lock.try_lock() && std::chrono::steady_clock::now() < deadline )
        std::this_thread::sleep_for( std::chrono::milliseconds( 20 ) );

    return lock;
}


void KIGIT_COMMON::rebuildPublicKeysUnlocked()
{
    m_publicKeys.clear();
    m_nextPublicKey = 0;

    if( KIGIT::IsOfferableSSHKey( m_projectSSHKey ) )
        m_publicKeys.push_back( m_projectSSHKey );
    else if( !m_projectSSHKey.IsEmpty() )
        wxLogTrace( traceGit, "Not offering project SSH key %s, it or its .pub is missing", m_projectSSHKey );

    for( const wxString& key : m_discoveredKeys )
    {
        if( !KIGIT::IsSameSSHKey( key, m_projectSSHKey ) )
            m_publicKeys.push_back( key );
    }
}


wxString KIGIT_COMMON::GetNextPublicKey()
{
    std::lock_guard<std::mutex> lock( m_credentialsMutex );

    // A key deleted since discovery would end the whole attempt instead of failing over to the next one
    while( m_nextPublicKey < static_cast<int>( m_publicKeys.size() ) )
    {
        wxString key = m_publicKeys[m_nextPublicKey++];

        if( KIGIT::IsOfferableSSHKey( key ) )
            return key;
    }

    return wxEmptyString;
}


wxString KIGIT_COMMON::GetGitRootDirectory() const
{
    if( !m_repo )
        return wxEmptyString;

    // Prefer the working directory (the user-visible project root).  Fall back to
    // git_repository_path (the .git directory) only for bare repositories.
    if( const char* workdir = git_repository_workdir( m_repo ) )
        return wxString::FromUTF8( workdir );

    if( const char* path = git_repository_path( m_repo ) )
        return wxString::FromUTF8( path );

    return wxEmptyString;
}


void KIGIT_COMMON::updatePublicKeys()
{
    // Deadlocks if the credentials lock is already held. SyncRemoteUrl releases it
    // first for that reason.
    std::lock_guard<std::mutex> lock( m_credentialsMutex );

    m_discoveredKeys = KIGIT::DefaultSSHKeyCandidates( KIGIT::HomeDir(), m_hostname );
    m_publicKeysHost = m_hostname;
    rebuildPublicKeysUnlocked();
}


void KIGIT_COMMON::UpdateCurrentBranchInfo()
{
    if( !m_repo )
        return;


    // We want to get the current branch's upstream url as well as the stored password
    // if one exists given the url and username.

    wxString    remote_name = GetRemotename();
    wxString    url;
    git_remote* remote = nullptr;

    if( git_remote_lookup( &remote, m_repo, remote_name.ToStdString().c_str() ) == GIT_OK )
    {
        // UTF-8 as the transfers read it, or a non-ASCII path would look changed on every fetch
        if( const char* remoteUrl = git_remote_url( remote ) )
            url = wxString::FromUTF8( remoteUrl );

        git_remote_free( remote );
    }

    SyncRemoteUrl( url );
}


void KIGIT_COMMON::SyncRemoteUrl( const wxString& aRawUrl )
{
    REMOTE_URL parsed = ParseRemoteURL( aRawUrl );
    bool       keysStale = false;

    {
        // The authentication callback reads these on a worker thread, so update them together
        std::lock_guard<std::mutex> lock( m_credentialsMutex );

        // HTTPS drops the username from the stored URL, so it is compared on its own
        if( parsed.url != m_remote || parsed.username != m_urlUsername )
        {
            m_remote = parsed.url;
            m_urlUsername = parsed.username;
            m_connType = parsed.connType;
            m_hostname = parsed.hostname;
            m_port = parsed.port;
            m_username = parsed.username;
            m_password = parsed.password;
            m_secretFetched = !m_password.IsEmpty();
        }

        // SetRemote can move the host without gathering keys, so compare against the gathered list
        keysStale = m_hostname != m_publicKeysHost;
    }

    if( keysStale )
        updatePublicKeys();
}


void KIGIT_COMMON::FollowRewrittenUrl( const wxString& aRawUrl )
{
    wxString username;
    wxString password;
    bool     secretFetched;

    {
        std::lock_guard<std::mutex> lock( m_credentialsMutex );
        username = m_username;
        password = m_password;
        secretFetched = m_secretFetched;
    }

    SyncRemoteUrl( aRawUrl );

    std::lock_guard<std::mutex> lock( m_credentialsMutex );
    m_username = username;
    m_password = password;
    m_secretFetched = secretFetched;
}


KIGIT_COMMON::GIT_CONN_TYPE KIGIT_COMMON::GetConnType() const
{
    std::lock_guard<std::mutex> lock( m_credentialsMutex );
    return m_connType;
}


void KIGIT_COMMON::SetConnType( GIT_CONN_TYPE aConnType )
{
    std::lock_guard<std::mutex> lock( m_credentialsMutex );
    m_connType = aConnType;
}


// Index of the '@' ending the userinfo of proto://user:password@host/path, or npos. The last
// one, since the first would split inside a password that contains an '@'.
static size_t findUserinfoAt( const wxString& aUrl )
{
    size_t protoSep = aUrl.find( "://" );

    if( protoSep == wxString::npos )
        return wxString::npos;

    size_t hostStart = protoSep + 3;
    size_t pathStart = aUrl.find_first_of( "/?#", hostStart );
    size_t searchEnd = ( pathStart == wxString::npos ) ? aUrl.length() : pathStart;

    if( searchEnd <= hostStart )
        return wxString::npos;

    size_t at = aUrl.rfind( '@', searchEnd - 1 );

    return ( at != wxString::npos && at >= hostStart ) ? at : wxString::npos;
}


wxString KIGIT_COMMON::GetLastGitError()
{
    const git_error* error = git_error_last();

    if( error == nullptr )
        return wxString( "No error" );

    // libgit2 quotes config lines and URLs verbatim, passwords included
    return KIGIT::StripEmbeddedUserinfo( wxString::FromUTF8( error->message ? error->message : "" ) );
}


KIGIT_ERROR_INFO KIGIT_COMMON::GetLastGitErrorInfo( int aCode )
{
    KIGIT_ERROR_INFO info;
    info.code = aCode;

    // libgit2 before 1.8 answers null rather than "no error"
    if( const git_error* error = git_error_last() )
    {
        info.klass = error->klass;
        info.message = wxString::FromUTF8( error->message ? error->message : "" );
    }

    return info;
}


// libgit2's git_str_decode_percent when the whole result is UTF-8; otherwise the component stays as written
static wxString decodePercent( const wxString& aText )
{
    const std::string text = aText.utf8_string();
    std::string       decoded;

    for( size_t ii = 0; ii < text.size(); ++ii )
    {
        if( text[ii] == '%' && ii + 2 < text.size() && isxdigit( (unsigned char) text[ii + 1] )
            && isxdigit( (unsigned char) text[ii + 2] ) )
        {
            decoded += static_cast<char>( std::stoi( text.substr( ii + 1, 2 ), nullptr, 16 ) );
            ii += 2;
        }
        else
        {
            decoded += text[ii];
        }
    }

    wxString result = wxString::FromUTF8( decoded );

    // wxString cannot hold bytes that are not UTF-8, so such text stays as written
    return result.IsEmpty() && !decoded.empty() ? aText : result;
}


// 0 when the address names no port, -1 when it names one that cannot be used
static int parsePort( const wxString& aPort )
{
    long port = 0;

    if( aPort.IsEmpty() )
        return 0;

    if( !aPort.ToLong( &port ) || port <= 0 || port > 65535 )
        return -1;

    return static_cast<int>( port );
}


static bool isScpIpv6( const wxString& aText, size_t aBracket )
{
    size_t colons = 0;

    for( size_t ii = aBracket + 1; ii < aText.length(); ++ii )
    {
        wxUniChar c = aText[ii];

        if( c == ':' )
            colons++;

        if( c == ']' )
            return colons > 1;

        if( c != ':' && !wxIsxdigit( c ) )
            return false;
    }

    return false;
}


static bool scpHasUser( const wxString& aText, size_t aStart )
{
    for( size_t ii = aStart; ii < aText.length(); ++ii )
    {
        if( aText[ii] == '@' )
            return true;

        if( aText[ii] == ':' )
            return false;
    }

    return false;
}


// libgit2's git_net_url_parse_scp, state for state, so the host key check binds the host and port it dials
static bool parseScpAddress( const wxString& aURL, wxString& aUser, wxString& aHost, wxString& aPort )
{
    enum
    {
        NONE,
        USER,
        HOST_START,
        HOST,
        HOST_END,
        IPV6,
        IPV6_END,
        PORT_START,
        PORT,
        PORT_END,
        PATH_START
    } state = NONE;

    size_t user = 0, userLen = 0, host = 0, hostLen = 0, port = 0, portLen = 0;
    int    bracket = 0;
    bool   havePath = false;

    for( size_t ii = 0; ii < aURL.length() && !havePath; ++ii )
    {
        wxUniChar c = aURL[ii];

        switch( state )
        {
        case NONE:
            if( c == '@' || c == ':' )
                return false;

            if( c == '[' )
            {
                if( isScpIpv6( aURL, ii ) )
                {
                    state = IPV6;
                    host = ii;
                }
                else if( bracket++ > 1 )
                {
                    return false;
                }
            }
            else if( scpHasUser( aURL, ii ) )
            {
                state = USER;
                user = ii;
            }
            else
            {
                state = HOST;
                host = ii;
            }

            break;

        case USER:
            if( c == '@' )
            {
                userLen = ii - user;
                state = HOST_START;
            }

            break;

        case HOST_START:
            state = c == '[' ? IPV6 : HOST;
            host = ii;
            break;

        case HOST:
            if( c == ':' )
            {
                hostLen = ii - host;
                state = bracket ? PORT_START : PATH_START;
            }
            else if( c == ']' )
            {
                if( bracket-- == 0 )
                    return false;

                hostLen = ii - host;
                state = HOST_END;
            }

            break;

        case HOST_END:
            if( c != ':' )
                return false;

            state = PATH_START;
            break;

        case IPV6:
            if( c == ']' )
                state = IPV6_END;

            break;

        case IPV6_END:
            if( c != ':' )
                return false;

            hostLen = ii - host;
            state = bracket ? PORT_START : PATH_START;
            break;

        case PORT_START:
            port = ii;
            state = PORT;
            break;

        case PORT:
            if( c == ']' )
            {
                if( bracket-- == 0 )
                    return false;

                portLen = ii - port;
                state = PORT_END;
            }

            break;

        case PORT_END:
            if( c != ':' )
                return false;

            state = PATH_START;
            break;

        case PATH_START: havePath = true; break;
        }
    }

    if( !havePath || hostLen == 0 )
        return false;

    aUser = aURL.Mid( user, userLen );
    aHost = aURL.Mid( host, hostLen );
    aPort = aURL.Mid( port, portLen );

    // libgit2 keeps an IPv6 host in its brackets here, but hands it out without them for URLs
    if( aHost.StartsWith( "[" ) && aHost.EndsWith( "]" ) )
        aHost = aHost.Mid( 1, aHost.length() - 2 );

    return true;
}


KIGIT_COMMON::REMOTE_URL KIGIT_COMMON::ParseRemoteURL( const wxString& aURL )
{
    REMOTE_URL result;
    result.url = aURL;

    size_t schemeEnd = aURL.find( "://" );
    size_t hostStart = 0;
    size_t hostEnd;

    if( schemeEnd != wxString::npos )
    {
        wxString scheme = aURL.Left( schemeEnd ).Lower();

        if( scheme == "https" || scheme == "http" )
            result.connType = GIT_CONN_TYPE::GIT_CONN_HTTPS;
        else if( scheme == "ssh" || scheme == "ssh+git" || scheme == "git+ssh" )
            result.connType = GIT_CONN_TYPE::GIT_CONN_SSH;
        else
            return result;

        hostStart = schemeEnd + 3;
        hostEnd = aURL.find_first_of( "/?#", hostStart );
        size_t at = findUserinfoAt( aURL );

        if( at != wxString::npos )
        {
            wxString credentials = aURL.Mid( hostStart, at - hostStart );
            size_t   colon = credentials.find( ':' );
            wxString user = credentials.Left( colon );
            result.username = decodePercent( user );

            if( colon != wxString::npos )
                result.password = decodePercent( credentials.Mid( colon + 1 ) );

            wxString userPrefix;

            if( result.connType == GIT_CONN_TYPE::GIT_CONN_SSH && !user.IsEmpty() )
                userPrefix = user + "@";

            result.url = aURL.Left( hostStart ) + userPrefix + aURL.Mid( at + 1 );
            hostStart = at + 1;
        }
    }
    else
    {
        // A colon before the path marks an SCP address, except for Windows drive paths.
        if( aURL.length() >= 2 && wxIsalpha( aURL[0] ) && aURL[1] == ':' )
            return result;

        hostEnd = aURL.find( ':' );
        size_t bracket = aURL.find( '[' );

        if( bracket != wxString::npos && bracket < hostEnd )
        {
            size_t endBracket = aURL.find( ']', bracket );

            if( endBracket != wxString::npos )
                hostEnd = aURL.find( ':', endBracket + 1 );
        }

        size_t slash = aURL.find_first_of( "/\\" );

        if( hostEnd == wxString::npos || hostEnd == 0 || ( slash != wxString::npos && slash < hostEnd ) )
        {
            return result;
        }

        result.connType = GIT_CONN_TYPE::GIT_CONN_SSH;
        wxString port;

        // An address libgit2 cannot read leaves no host, which the host key check refuses
        if( parseScpAddress( aURL, result.username, result.hostname, port ) )
            result.port = parsePort( port );

        return result;
    }

    wxString host = aURL.Mid( hostStart, hostEnd == wxString::npos ? wxString::npos : hostEnd - hostStart );
    wxString port;

    if( host.StartsWith( "[" ) )
    {
        result.hostname = host.AfterFirst( '[' ).BeforeFirst( ']' );
        port = host.AfterFirst( ']' ).AfterFirst( ':' );
    }
    else
    {
        result.hostname = host.BeforeFirst( ':' );
        port = host.AfterFirst( ':' );
    }

    // libgit2 connects to the decoded name, so %67he.example is ghe.example
    result.hostname = decodePercent( result.hostname );
    result.port = parsePort( port );

    return result;
}


void KIGIT_COMMON::updateConnectionType()
{
    std::lock_guard<std::mutex> lock( m_credentialsMutex );
    updateConnectionTypeUnlocked();
}


void KIGIT_COMMON::updateConnectionTypeUnlocked()
{
    REMOTE_URL parsed = ParseRemoteURL( m_remote );
    m_connType = parsed.connType;
    m_remote = parsed.url;
    m_urlUsername = parsed.username;
    m_hostname = parsed.hostname;
    m_port = parsed.port;

    if( !parsed.username.IsEmpty() )
        m_username = parsed.username;

    if( !parsed.password.IsEmpty() )
        m_password = parsed.password;

    m_secretFetched = !m_password.IsEmpty();
}


int KIGIT_COMMON::HandleSSHKeyAuthentication( git_cred** aOut, const wxString& aUsername, KIGIT_AUTH_ATTEMPT& aAttempt )
{
    if( !( m_testedTypes & KIGIT_CREDENTIAL_SSH_AGENT ) )
    {
        if( HandleSSHAgentAuthentication( aOut, aUsername ) == GIT_OK )
        {
            aAttempt.agentTried = true;
            return GIT_OK;
        }

        // Agent unavailable or has no matching key; fall through to configured key.
    }

    // SSH key authentication with password
    wxString sshKey = GetNextPublicKey();

    if( sshKey.IsEmpty() )
    {
        wxLogTrace( traceGit, "Finished testing all possible ssh keys" );
        m_testedTypes |= GIT_CREDENTIAL_SSH_KEY;
        return GIT_PASSTHROUGH;
    }

    wxString sshPubKey = sshKey + ".pub";
    wxString password = GetPassword();

    wxLogTrace( traceGit, "Testing %s\n", sshKey );

    if( git_credential_ssh_key_new( aOut, aUsername.mbc_str(), sshPubKey.mbc_str(), sshKey.mbc_str(),
                                    password.mbc_str() ) != GIT_OK )
    {
        wxLogTrace( traceGit, "Failed to create SSH key credential for %s: %s", sshKey,
                    KIGIT_COMMON::GetLastGitError() );
        return GIT_PASSTHROUGH;
    }

    // libssh2 reads the file only once authentication starts, so a load failure names this key
    aAttempt.keysTried.push_back( sshKey );
    aAttempt.passphraseGiven = !password.IsEmpty();
    return GIT_OK;
}


int KIGIT_COMMON::HandlePlaintextAuthentication( git_cred** aOut, const wxString& aUsername )
{
    wxString password = GetPassword();

    int rc = git_credential_userpass_plaintext_new( aOut, aUsername.mbc_str(), password.mbc_str() );
    m_testedTypes |= GIT_CREDENTIAL_USERPASS_PLAINTEXT;

    return rc;
}


int KIGIT_COMMON::HandleSSHAgentAuthentication( git_cred** aOut, const wxString& aUsername )
{
    m_testedTypes |= KIGIT_CREDENTIAL_SSH_AGENT;

    if( git_credential_ssh_key_from_agent( aOut, aUsername.mbc_str() ) != GIT_OK )
    {
        wxLogTrace( traceGit, "Failed to create SSH agent credential: %s", KIGIT_COMMON::GetLastGitError() );
        return GIT_PASSTHROUGH;
    }

    return GIT_OK;
}


// libgit2 marks at most one entry to merge, the branch the current one tracks, so the name is
// not needed to tell them apart. FetchHeadMarksAtMostOneBranchToMerge holds that to account.
extern "C" int fetchhead_foreach_cb( const char*, const char*,
                                     const git_oid* aOID, unsigned int aIsMerge, void* aPayload )
{
    if( aIsMerge )
        git_oid_cpy( (git_oid*) aPayload, aOID );

    return 0;
}


extern "C" void clone_progress_cb( const char* aStr, size_t aLen, size_t aTotal, void* aPayload )
{
    KIGIT_REPO_MIXIN* parent = reinterpret_cast<KIGIT_REPO_MIXIN*>( aPayload );

    wxString progressMessage( aStr );
    parent->UpdateProgress( aLen, aTotal, progressMessage );
}


extern "C" int progress_cb( const char* str, int len, void* aPayload )
{
    KIGIT_REPO_MIXIN* parent = reinterpret_cast<KIGIT_REPO_MIXIN*>( aPayload );

    if( parent->IsThisOperationCancelled() )
    {
        wxLogTrace( traceGit, "Progress CB cancelled" );
        return GIT_EUSER;
    }

    parent->ReceiveSideband( str, len );
    return 0;
}


extern "C" int transfer_progress_cb( const git_transfer_progress* aStats, void* aPayload )
{
    KIGIT_REPO_MIXIN* parent = reinterpret_cast<KIGIT_REPO_MIXIN*>( aPayload );

    wxString progressMessage = wxString::Format( _( "Received %u of %u objects" ),
                                                 aStats->received_objects,
                                                 aStats->total_objects );
    if( parent->IsThisOperationCancelled() )
    {
        wxLogTrace( traceGit, "Transfer progress cancelled" );
        return GIT_EUSER;
    }

    parent->UpdateProgress( aStats->received_objects, aStats->total_objects, progressMessage );

    return 0;
}


extern "C" int update_cb( const char* aRefname, const git_oid* aFirst, const git_oid* aSecond,
                          void* aPayload )
{
    constexpr int cstring_len = 8;
    char          a_str[cstring_len + 1];
    char          b_str[cstring_len + 1];

    KIGIT_REPO_MIXIN* parent = reinterpret_cast<KIGIT_REPO_MIXIN*>( aPayload );
    wxString          status;

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
    long long         progress = 100;
    KIGIT_REPO_MIXIN* parent = reinterpret_cast<KIGIT_REPO_MIXIN*>( aPayload );

    if( parent->IsThisOperationCancelled() )
    {
        wxLogTrace( traceGit, "Push progress cancelled" );
        return GIT_EUSER;
    }

    if( aTotal != 0 )
    {
        progress = ( aCurrent * 100ll ) / aTotal;
    }

    wxString progressMessage = wxString::Format( _( "Writing objects: %lld%% (%u/%u), %zu bytes" ),
                                                 progress, aCurrent, aTotal, aBytes );
    parent->UpdateProgress( aCurrent, aTotal, progressMessage );

    return 0;
}


extern "C" int push_update_reference_cb( const char* aRefname, const char* aStatus, void* aPayload )
{
    KIGIT_REPO_MIXIN* parent = reinterpret_cast<KIGIT_REPO_MIXIN*>( aPayload );

    if( aStatus && *aStatus )
    {
        const wxString status = KIGIT::DecodeServerText( aStatus, strlen( aStatus ) );

        parent->SetPushRefusal( wxString::Format( wxS( "%s: %s" ), aRefname, status ) );

        wxString statusMessage = wxString::Format( _( "* [rejected] %s (%s)" ), aRefname,
                                                   KIGIT::PrintableServerText( status ) );
        parent->UpdateProgress( 0, 0, statusMessage );

        // Without this the push reports success even though the remote refused it.
        git_error_set_str( GIT_ERROR_NET, aStatus );
        return GIT_ERROR;
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
    KIGIT_REPO_MIXIN* parent = reinterpret_cast<KIGIT_REPO_MIXIN*>( aPayload );
    KIGIT_COMMON* common = parent->GetCommon();

    // The transport's URL outranks a cached remote that may be empty or stale
    common->SyncRemoteUrl( wxString::FromUTF8( aUrl ) );

    // The configured remote URL may embed a password. Keep it out of the trace log.
    wxString safeUrl = KIGIT::StripEmbeddedUserinfo( wxString::FromUTF8( aUrl ) );

    wxLogTrace( traceGit, "Credentials callback for %s, testing %d", safeUrl, aAllowedTypes );

    parent->AuthAttempt().connType = parent->GetConnType();

    if( parent->GetConnType() == KIGIT_COMMON::GIT_CONN_TYPE::GIT_CONN_LOCAL )
    {
        wxLogTrace( traceGit, "Local repository, no credentials needed" );
        return GIT_PASSTHROUGH;
    }

    if( aAllowedTypes & GIT_CREDENTIAL_USERNAME
        && !( parent->TestedTypes() & GIT_CREDENTIAL_USERNAME ) )
    {
        wxString username = parent->GetUsername().Trim().Trim( false );
        // A token in the URL parses as the username, so no trace names it
        wxLogTrace( traceGit, "Username credential at %s with allowed type %d", safeUrl, aAllowedTypes );

        // An empty username makes the server answer "unsupported authentication", which tells
        // the user nothing. Fail authentication instead, so the caller prompts and retries.
        if( username.IsEmpty() )
        {
            git_error_clear();
            git_error_set_str( GIT_ERROR_NET, _( "Unable to authenticate" ).mbc_str() );
            parent->SetAuthFailure();
            return GIT_EAUTH;
        }

        if( int rc = git_credential_username_new( aOut, username.ToStdString().c_str() ); rc != GIT_OK )
        {
            wxLogTrace( traceGit, "Failed to create username credential: %s", KIGIT_COMMON::GetLastGitError() );
            return rc;
        }
        else
        {
            wxLogTrace( traceGit, "Created username credential" );
        }

        parent->TestedTypes() |= GIT_CREDENTIAL_USERNAME;
    }
    else if( parent->GetConnType() == KIGIT_COMMON::GIT_CONN_TYPE::GIT_CONN_HTTPS
                && ( aAllowedTypes & GIT_CREDENTIAL_USERPASS_PLAINTEXT )
                && !( parent->TestedTypes() & GIT_CREDENTIAL_USERPASS_PLAINTEXT )
                && !parent->GetUsername().IsEmpty() )
    {
        // Plaintext authentication
        wxLogTrace( traceGit, "Plaintext authentication at %s with allowed type %d", safeUrl, aAllowedTypes );
        return common->HandlePlaintextAuthentication( aOut, parent->GetUsername() );
    }
    else if( parent->GetConnType() == KIGIT_COMMON::GIT_CONN_TYPE::GIT_CONN_SSH
                && ( aAllowedTypes & GIT_CREDENTIAL_SSH_KEY )
                && !( parent->TestedTypes() & GIT_CREDENTIAL_SSH_KEY ) )
    {
        // libgit2 fails a credential whose user differs from the one the SSH session already uses
        wxString sshUser = ( aUsername && *aUsername ) ? wxString::FromUTF8( aUsername ) : parent->GetUsername();

        // SSH key authentication
        int result = common->HandleSSHKeyAuthentication( aOut, sshUser, parent->AuthAttempt() );

        // Translate exhausted-keys PASSTHROUGH into a proper auth error so the
        // retry loop runs and libgit2 doesn't emit "no callback set".
        if( result == GIT_PASSTHROUGH )
        {
            git_error_clear();
            git_error_set_str( GIT_ERROR_NET, _( "Unable to authenticate" ).mbc_str() );
            parent->SetAuthFailure();
            return GIT_EAUTH;
        }

        return result;
    }
    else
    {
        // Signal an auth failure so the caller prompts and retries, rather than letting libgit2 report
        // "no callback set" as a dead end.
        git_error_clear();
        git_error_set_str( GIT_ERROR_NET, _( "Unable to authenticate" ).mbc_str() );
        parent->SetAuthFailure();
        return GIT_EAUTH;
    }

    return GIT_OK;
};


extern "C" int certificate_check_cb( git_cert* aCert, int aValid, const char* aHost, void* aPayload )
{
    if( !aCert || !aHost || !aPayload || aCert->cert_type != GIT_CERT_HOSTKEY_LIBSSH2 )
        return GIT_PASSTHROUGH;

    const git_cert_hostkey* cert = reinterpret_cast<const git_cert_hostkey*>( aCert );

    KIGIT_REPO_MIXIN* handler = reinterpret_cast<KIGIT_REPO_MIXIN*>( aPayload );

    auto refuse = [&]( const wxString& aReason )
    {
        // libgit2 v1.9.0 ssh_libssh2.c:758-760 replaces any error the callback sets
        handler->SetHostKeyRefusal( aReason );
        return GIT_ECERTIFICATE;
    };

    // libgit2 1.9 connects on a passthrough even when nothing was verified
    auto refuseUnreadable = [&]()
    {
        wxLogTrace( traceGit, "Host key from %s cannot be read, refusing it", aHost );
        return refuse( wxString::Format( _( "KiCad could not read the key %s presented, so the connection was not "
                                            "made. Ask your administrator which host key the server uses." ),
                                         wxString::FromUTF8( aHost ) ) );
    };

    if( !( cert->type & GIT_CERT_SSH_RAW ) || !cert->hostkey || cert->hostkey_len == 0 )
        return refuseUnreadable();

    KIGIT::HOST_KEY_FAILURE key;
    key.rawKey.assign( cert->hostkey, cert->hostkey_len );
    key.host = wxString::FromUTF8( aHost );

    // An SCP address hands over an IPv6 host still in its brackets
    if( key.host.StartsWith( wxS( "[" ) ) && key.host.EndsWith( wxS( "]" ) ) )
        key.host = key.host.Mid( 1, key.host.length() - 2 );

    key.port = handler->GetCommon()->SSHPortFor( key.host );

    // Checking another port's entries would let a wildcard or plain-host line vouch for this server
    if( key.port < 0 )
    {
        wxLogTrace( traceGit, "Host %s does not match the synced remote, refusing its key", key.host );
        return refuse( wxString::Format( _( "The address of %s could not be matched to the remote, so its key was "
                                            "not checked. Check the remote URL." ),
                                         key.host ) );
    }

    key.keyType = KIGIT::HostKeyType( key.rawKey );
    key.fingerprint = KIGIT::HostKeyFingerprint( key.rawKey );

    if( key.keyType.IsEmpty() )
        return refuseUnreadable();

    key.state = KIGIT::LookupKnownHost( KIGIT::ReadKnownHosts(), key.host, key.port, key.keyType, key.rawKey );

    wxLogTrace( traceGit, "Host key %s SHA256:%s for %s port %d, known_hosts state %d, libgit2 valid %d", key.keyType,
                key.fingerprint, key.host, key.port, static_cast<int>( key.state ), aValid );

    // libssh2 accepts revoked keys and port-22 lines for other ports, and 1.5.0 reads no known_hosts at all
    if( key.state == KIGIT::KNOWN_HOST_STATE::MATCH )
        return 0;

    handler->SetHostKeyFailure( key );
    return GIT_ECERTIFICATE;
}


namespace KIGIT
{

bool IsPrivateKeyEncrypted( const wxString& aKeyText )
{
    wxArrayString lines = wxSplit( aKeyText, '\n', '\0' );

    if( lines.IsEmpty() || !lines[0].Contains( wxS( "PRIVATE KEY" ) ) )
        return false;

    if( lines[0].Contains( wxS( "ENCRYPTED" ) ) )
        return true;

    // A PEM key carries Proc-Type in the header block, which ends at the blank line.
    for( size_t ii = 1; ii < lines.GetCount(); ++ii )
    {
        if( lines[ii].Trim().IsEmpty() )
            break;

        if( lines[ii].StartsWith( wxS( "Proc-Type:" ) ) && lines[ii].Contains( wxS( "ENCRYPTED" ) ) )
            return true;
    }

    if( !lines[0].Contains( wxS( "OPENSSH PRIVATE KEY" ) ) )
        return false;

    // An OpenSSH key carries its cipher name in the body, right after the magic.
    wxString body;

    for( size_t ii = 1; ii < lines.GetCount(); ++ii )
    {
        if( lines[ii].StartsWith( wxS( "-----" ) ) )
            break;

        body += lines[ii];
    }

    wxMemoryBuffer raw = wxBase64Decode( body, wxBase64DecodeMode_SkipWS );

    const char           magic[] = "openssh-key-v1";
    const size_t         magicLen = sizeof( magic );
    const unsigned char* data = static_cast<const unsigned char*>( raw.GetData() );

    if( raw.GetDataLen() < magicLen + 4 || std::memcmp( data, magic, magicLen ) != 0 )
        return false;

    uint32_t nameLen =
            ( static_cast<uint32_t>( data[magicLen] ) << 24 ) | ( static_cast<uint32_t>( data[magicLen + 1] ) << 16 )
            | ( static_cast<uint32_t>( data[magicLen + 2] ) << 8 ) | static_cast<uint32_t>( data[magicLen + 3] );

    if( nameLen == 0 || raw.GetDataLen() < magicLen + 4 + nameLen )
        return false;

    std::string cipher( reinterpret_cast<const char*>( data + magicLen + 4 ), nameLen );

    return cipher != "none";
}


git_tree* ResolveRefToTree( git_repository* aRepo, const wxString& aRef )
{
    if( !aRepo )
        return nullptr;

    git_object* obj = nullptr;
    std::string refStr( aRef.ToUTF8() );

    if( git_revparse_single( &obj, aRepo, refStr.c_str() ) != 0 )
    {
        wxLogTrace( traceGit, "git_revparse_single failed for ref '%s': %s",
                    aRef, KIGIT_COMMON::GetLastGitError() );
        return nullptr;
    }

    KIGIT::GitObjectPtr objPtr( obj );
    git_tree*           tree = nullptr;

    if( git_object_peel( reinterpret_cast<git_object**>( &tree ), obj, GIT_OBJECT_TREE ) != 0 )
    {
        wxLogTrace( traceGit, "git_object_peel to tree failed for '%s': %s",
                    aRef, KIGIT_COMMON::GetLastGitError() );
        return nullptr;
    }

    return tree;
}


std::vector<wxString> CompareBranchRefs( git_repository* aRepo )
{
    std::vector<wxString> refs;

    if( !aRepo )
        return refs;

    auto resolves = [&]( const wxString& aRef )
    {
        return !aRef.IsEmpty() && KIGIT::GitTreePtr( ResolveRefToTree( aRepo, aRef ) ) != nullptr;
    };

    auto add = [&]( const wxString& aRef )
    {
        if( resolves( aRef ) && std::find( refs.begin(), refs.end(), aRef ) == refs.end() )
            refs.push_back( aRef );
    };

    wxString       current;
    wxString       upstream;
    git_reference* head = nullptr;

    if( git_repository_head( &head, aRepo ) == 0 )
    {
        KIGIT::GitReferencePtr headPtr( head );

        if( git_reference_is_branch( head ) )
        {
            current = wxString::FromUTF8( git_reference_shorthand( head ) );

            git_reference* tracking = nullptr;

            if( git_branch_upstream( &tracking, head ) == 0 )
            {
                KIGIT::GitReferencePtr trackingPtr( tracking );
                upstream = wxString::FromUTF8( git_reference_shorthand( tracking ) );
            }
        }
    }

    const wxString main = wxS( "main" );
    const wxString master = wxS( "master" );

    for( const wxString& base : { main, master, upstream } )
    {
        if( base != current && resolves( base ) )
        {
            refs.push_back( base );
            break;
        }
    }

    add( current.IsEmpty() ? wxString( wxS( "HEAD" ) ) : current );
    add( main );
    add( master );
    add( upstream );
    add( wxS( "HEAD" ) );

    return refs;
}


void CollectDiffDeltas( git_diff*                                           aDiff,
                        const std::function<void( const git_diff_delta& )>& aCallback )
{
    if( !aDiff )
        return;

    std::size_t numDeltas = git_diff_num_deltas( aDiff );

    for( std::size_t ii = 0; ii < numDeltas; ++ii )
    {
        const git_diff_delta* delta = git_diff_get_delta( aDiff, ii );

        if( !delta )
            continue;

        aCallback( *delta );
    }
}


wxString BranchNameError( const wxString& aName )
{
    if( aName.IsEmpty() )
        return _( "Enter a name for the branch." );

    // git refuses all of these, but says so in its own words. Name them while the user can
    // still fix the name.
    if( aName.Contains( wxS( " " ) ) )
        return _( "A branch name cannot contain spaces." );

    if( aName.StartsWith( wxS( "-" ) ) )
        return _( "A branch name cannot start with a dash." );

    if( aName.Contains( wxS( ".." ) ) )
        return _( "A branch name cannot contain two dots in a row." );

    if( aName.EndsWith( wxS( "/" ) ) )
        return _( "A branch name cannot end with a slash." );

    int valid = 0;

    if( git_branch_name_is_valid( &valid, aName.utf8_string().c_str() ) != GIT_OK || !valid )
        return wxString::Format( _( "'%s' is not a valid branch name." ), aName );

    return wxEmptyString;
}


wxString DecodeRepoText( const char* aText )
{
    if( !aText || !*aText )
        return wxString();

    wxString decoded = wxString::FromUTF8( aText );

    if( !decoded.IsEmpty() )
        return decoded;

    return wxString( aText, wxConvISO8859_1 );
}


wxString BusyMessage()
{
    return _( "Another git operation is in progress. Try again in a moment." );
}


wxString ComposeCommitMessage( const wxString& aSummary, const wxString& aDescription, const wxString& aSignOff )
{
    wxString message = aSummary;
    message.Replace( wxS( "\r\n" ), wxS( "\n" ) );
    message.Trim( true ).Trim( false );

    wxString description = aDescription;
    description.Replace( wxS( "\r\n" ), wxS( "\n" ) );
    description.Trim( true ).Trim( false );

    if( !description.IsEmpty() )
        message << wxS( "\n\n" ) << description;

    if( !aSignOff.IsEmpty() )
    {
        wxString trailer = wxS( "Signed-off-by: " ) + aSignOff;
        bool     signedOff = false;

        // An amended message already carries its own sign off line.
        for( wxString line : wxSplit( message, '\n' ) )
        {
            if( line.Trim() == trailer )
                signedOff = true;
        }

        if( !signedOff )
            message << wxS( "\n\n" ) << trailer;
    }

    return message;
}


void SplitCommitMessage( const wxString& aMessage, wxString& aSummary, wxString& aDescription )
{
    wxString message = aMessage;
    message.Replace( wxS( "\r\n" ), wxS( "\n" ) );
    message.Trim( true ).Trim( false );

    aSummary = message.BeforeFirst( '\n' );
    aDescription = message.AfterFirst( '\n' );
    aDescription.Trim( false );
}

} // namespace KIGIT
