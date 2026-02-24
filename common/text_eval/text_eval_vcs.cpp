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
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <text_eval/text_eval_vcs.h>
#include <git/project_git_utils.h>
#include <git/git_backend.h>
#include <git/kicad_git_common.h>
#include <git/kicad_git_memory.h>
#include <wx/string.h>
#include <map>

namespace TEXT_EVAL_VCS
{
// Private implementation details
namespace
{
    git_repository* OpenRepo( const std::string& aPath )
    {
        if( !GetGitBackend() )
            return nullptr;

        return KIGIT::PROJECT_GIT_UTILS::GetRepositoryForFile( aPath.c_str() );
    }

    void CloseRepo( git_repository* aRepo )
    {
        if( aRepo )
            git_repository_free( aRepo );
    }

    git_oid MakeZeroOid()
    {
        git_oid oid;
        git_oid_fromstrn( &oid, "0000000000000000000000000000000000000000", 40 );
        return oid;
    }

    git_oid GetFileCommit( git_repository* aRepo, const std::string& aPath )
    {
        if( !aRepo )
            return MakeZeroOid();

        // Get HEAD commit
        git_oid head_oid;

        if( git_reference_name_to_id( &head_oid, aRepo, "HEAD" ) != 0 )
            return MakeZeroOid();

        // For repo-level query (empty or "."), just return HEAD
        if( aPath.empty() || aPath == "." )
            return head_oid;

        // For file-specific query, walk history to find last commit that touched this file
        git_revwalk* walker = nullptr;

        if( git_revwalk_new( &walker, aRepo ) != 0 )
            return MakeZeroOid();

        git_revwalk_sorting( walker, GIT_SORT_TIME );
        git_revwalk_push( walker, &head_oid );

        // Walk through commits to find when the file was last modified
        git_oid result = MakeZeroOid();
        git_oid commit_oid;
        git_oid prev_blob_oid = MakeZeroOid();
        bool    first_commit = true;

        while( git_revwalk_next( &commit_oid, walker ) == 0 )
        {
            git_commit* commit = nullptr;

            if( git_commit_lookup( &commit, aRepo, &commit_oid ) != 0 )
                continue;

            // Get the tree for this commit
            git_tree* tree = nullptr;

            if( git_commit_tree( &tree, commit ) == 0 )
            {
                // Try to find the file in this tree
                git_tree_entry* entry = nullptr;

                if( git_tree_entry_bypath( &entry, tree, aPath.c_str() ) == 0 )
                {
                    const git_oid* blob_oid = git_tree_entry_id( entry );

                    if( first_commit )
                    {
                        // First time we see this file, remember its blob ID
                        git_oid_cpy( &prev_blob_oid, blob_oid );
                        git_oid_cpy( &result, &commit_oid );
                        first_commit = false;
                    }
                    else if( git_oid_cmp( blob_oid, &prev_blob_oid ) != 0 )
                    {
                        // File content changed - previous commit is where it changed
                        git_tree_entry_free( entry );
                        git_tree_free( tree );
                        git_commit_free( commit );
                        break;
                    }
                    else
                    {
                        // File unchanged, keep looking
                        git_oid_cpy( &result, &commit_oid );
                    }

                    git_tree_entry_free( entry );
                }
                else if( !first_commit )
                {
                    // File doesn't exist in this commit, but existed before
                    // So the previous commit is where it was added/last modified
                    git_tree_free( tree );
                    git_commit_free( commit );
                    break;
                }

                git_tree_free( tree );
            }

            git_commit_free( commit );
        }

        git_revwalk_free( walker );
        return result;
    }

    struct DescribeInfo
    {
        std::string tag;
        int         distance;
    };

    DescribeInfo GetDescribeInfo( const std::string& aMatch, bool aAnyTags )
    {
        git_repository* repo = OpenRepo( "." );

        if( !repo )
            return { std::string(), 0 };

        git_oid head_oid;

        if( git_reference_name_to_id( &head_oid, repo, "HEAD" ) != 0 )
        {
            CloseRepo( repo );
            return { std::string(), 0 };
        }

        git_strarray tag_names;

        if( git_tag_list_match( &tag_names, aMatch.empty() ? "*" : aMatch.c_str(), repo ) != 0 )
        {
            CloseRepo( repo );
            return { std::string(), 0 };
        }

        // Build map of commit OID -> tag name upfront
        std::map<git_oid, std::string, decltype(
                [](const git_oid& a, const git_oid& b)
                {
                    return git_oid_cmp(&a, &b) < 0;
                } )> commit_to_tag;

        for( size_t i = 0; i < tag_names.count; ++i )
        {
            git_object* tag_obj = nullptr;

            if( git_revparse_single( &tag_obj, repo, tag_names.strings[i] ) == 0 )
            {
                git_object_t type = git_object_type( tag_obj );

                if( type == GIT_OBJECT_TAG )
                {
                    git_object* target = nullptr;

                    if( git_tag_peel( &target, (git_tag*) tag_obj ) == 0 )
                    {
                        commit_to_tag[*git_object_id( target )] = tag_names.strings[i];
                        git_object_free( target );
                    }
                }
                else if( aAnyTags && type == GIT_OBJECT_COMMIT )
                {
                    commit_to_tag[*git_object_id( tag_obj )] = tag_names.strings[i];
                }

                git_object_free( tag_obj );
            }
        }

        git_strarray_dispose( &tag_names );

        git_revwalk* walker = nullptr;

        if( git_revwalk_new( &walker, repo ) != 0 )
        {
            CloseRepo( repo );
            return { std::string(), 0 };
        }

        git_revwalk_sorting( walker, GIT_SORT_TOPOLOGICAL | GIT_SORT_TIME );
        git_revwalk_push( walker, &head_oid );

        DescribeInfo result{ std::string(), 0 };
        int          distance = 0;
        git_oid      commit_oid;

        while( git_revwalk_next( &commit_oid, walker ) == 0 )
        {
            auto it = commit_to_tag.find( commit_oid );

            if( it != commit_to_tag.end() )
            {
                result.tag = it->second;
                result.distance = distance;
                break;
            }

            distance++;
        }

        git_revwalk_free( walker );
        CloseRepo( repo );
        return result;
    }

    std::string GetCommitSignatureField( const std::string& aPath, bool aUseCommitter, bool aGetEmail )
    {
        git_repository* repo = OpenRepo( aPath );

        if( !repo )
            return std::string();

        git_oid oid = GetFileCommit( repo, aPath );

        if( git_oid_is_zero( &oid ) )
        {
            CloseRepo( repo );
            return std::string();
        }

        git_commit* commit = nullptr;
        std::string result;

        if( git_commit_lookup( &commit, repo, &oid ) == 0 )
        {
            const git_signature* sig = aUseCommitter ? git_commit_committer( commit ) : git_commit_author( commit );

            if( sig )
            {
                const char* field = aGetEmail ? sig->email : sig->name;

                if( field )
                    result = field;
            }

            git_commit_free( commit );
        }

        CloseRepo( repo );
        return result;
    }

} // anonymous namespace


std::string GetCommitHash( const std::string& aPath, int aLength )
{
    git_repository* repo = OpenRepo( aPath );

    if( !repo )
        return std::string();

    git_oid oid = GetFileCommit( repo, aPath );

    if( git_oid_is_zero( &oid ) )
    {
        CloseRepo( repo );
        return std::string();
    }

    int  length = std::max( 4, std::min( aLength, GIT_OID_HEXSZ ) );
    char hash[GIT_OID_HEXSZ + 1];
    git_oid_tostr( hash, length + 1, &oid );

    CloseRepo( repo );
    return hash;
}


std::string GetNearestTag( const std::string& aMatch, bool aAnyTags )
{
    return GetDescribeInfo( aMatch, aAnyTags ).tag;
}


int GetDistanceFromTag( const std::string& aMatch, bool aAnyTags )
{
    return GetDescribeInfo( aMatch, aAnyTags ).distance;
}


bool IsDirty( bool aIncludeUntracked )
{
    git_repository* repo = OpenRepo( "." );

    if( !repo )
        return false;

    git_status_list*   status = nullptr;
    git_status_options statusOpts;
    git_status_options_init( &statusOpts, GIT_STATUS_OPTIONS_VERSION );

    statusOpts.show = GIT_STATUS_SHOW_INDEX_AND_WORKDIR;
    statusOpts.flags = aIncludeUntracked ? GIT_STATUS_OPT_INCLUDE_UNTRACKED : GIT_STATUS_OPT_EXCLUDE_SUBMODULES;

    bool isDirty = false;

    if( git_status_list_new( &status, repo, &statusOpts ) == 0 )
    {
        isDirty = git_status_list_entrycount( status ) > 0;
        git_status_list_free( status );
    }

    CloseRepo( repo );
    return isDirty;
}


std::string GetAuthor( const std::string& aPath )
{
    return GetCommitSignatureField( aPath, false, false );
}


std::string GetAuthorEmail( const std::string& aPath )
{
    return GetCommitSignatureField( aPath, false, true );
}


std::string GetCommitter( const std::string& aPath )
{
    return GetCommitSignatureField( aPath, true, false );
}


std::string GetCommitterEmail( const std::string& aPath )
{
    return GetCommitSignatureField( aPath, true, true );
}


std::string GetBranch()
{
    git_repository* repo = OpenRepo( "." );

    if( !repo )
        return std::string();

    KIGIT_COMMON common( repo );
    wxString     branchName = common.GetCurrentBranchName();

    CloseRepo( repo );
    return branchName.ToStdString();
}


int64_t GetCommitTimestamp( const std::string& aPath )
{
    git_repository* repo = OpenRepo( aPath );

    if( !repo )
        return 0;

    git_oid oid = GetFileCommit( repo, aPath );

    if( git_oid_is_zero( &oid ) )
    {
        CloseRepo( repo );
        return 0;
    }

    git_commit* commit = nullptr;
    int64_t     timestamp = 0;

    if( git_commit_lookup( &commit, repo, &oid ) == 0 )
    {
        timestamp = static_cast<int64_t>( git_commit_time( commit ) );
        git_commit_free( commit );
    }

    CloseRepo( repo );
    return timestamp;
}


std::string GetCommitDate( const std::string& aPath )
{
    int64_t timestamp = GetCommitTimestamp( aPath );
    return timestamp > 0 ? std::to_string( timestamp ) : std::string();
}

} // namespace TEXT_EVAL_VCS
