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
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include <text_eval/text_eval_vcs.h>
#include <text_eval/text_eval_environment.h>
#include <git/project_git_utils.h>
#include <git/git_backend.h>
#include <git/kicad_git_common.h>
#include <git/kicad_git_memory.h>
#include <string_utils.h>
#include <wx/filename.h>
#include <wx/string.h>
#include <wx/arrstr.h> // REQUIRED for wxString vector export on MSVC
#include <algorithm>
#include <map>

namespace TEXT_EVAL_VCS
{
// Per-thread override that anchors repo-scoped queries to a specific path (for example the
// loaded project directory). When empty, repo discovery falls back to the process cwd.
namespace
{
    thread_local wxString tl_contextPath;
    thread_local bool tl_contextIsFile = false;
}


void SetContextPath( const wxString& aPath )
{
    const bool isFile = !aPath.IsEmpty() && wxFileName( aPath ).FileExists();
    tl_contextPath = aPath;
    tl_contextIsFile = isFile;
}


wxString GetContextPath()
{
    return tl_contextPath.IsEmpty() ? wxString( wxT( "." ) ) : tl_contextPath;
}


bool GetContextIsFile()
{
    return tl_contextIsFile;
}


CONTEXT_PATH_SCOPE::CONTEXT_PATH_SCOPE( const wxString& aPath ) :
        m_previous( tl_contextPath ),
        m_previousIsFile( tl_contextIsFile )
{
    SetContextPath( aPath );
}


CONTEXT_PATH_SCOPE::~CONTEXT_PATH_SCOPE()
{
    tl_contextPath = m_previous;
    tl_contextIsFile = m_previousIsFile;
}


// Private implementation details
namespace
{
    wxString ResolveEffectivePath( const std::string& aPath )
    {
        if( aPath.empty() || aPath == "." )
            return GetContextPath();

        wxFileName path( wxString::FromUTF8( aPath ) );

        if( path.IsRelative() )
        {
            wxFileName context( GetContextPath() );
            context.MakeAbsolute();
            path.MakeAbsolute( tl_contextIsFile ? context.GetPath() : context.GetFullPath() );
        }

        return path.GetFullPath();
    }


    using VCS_QUERY = TEXT_EVAL::ENVIRONMENT::VCS_QUERY;
    using VCS_VALUE = TEXT_EVAL::ENVIRONMENT::VCS_VALUE;

    VCS_VALUE Capture( VCS_QUERY aQuery, const std::string& aPath, const std::string& aArgument,
                       int aOptions, const std::function<VCS_VALUE()>& aRead )
    {
        if( auto* environment = TEXT_EVAL::ENVIRONMENT::Current() )
        {
            wxFileName path = wxFileName::DirName( ResolveEffectivePath( aPath ) );
            path.MakeAbsolute();
            const bool contextFile = tl_contextIsFile && ( aPath.empty() || aPath == "." );
            return environment->VcsValue( { aQuery, path.GetPath( wxPATH_GET_VOLUME ), aArgument, aOptions,
                                            contextFile }, aRead );
        }

        return aRead();
    }


    git_repository* OpenRepo( const std::string& aPath )
    {
        if( !GetGitBackend() )
            return nullptr;

        wxFileName effective( ResolveEffectivePath( aPath ) );

        if( ( !aPath.empty() && aPath != "." ) || tl_contextIsFile )
        {
            effective.MakeAbsolute();
            return KIGIT::PROJECT_GIT_UTILS::GetRepositoryForFile( TO_UTF8( effective.GetPath() ) );
        }

        return KIGIT::PROJECT_GIT_UTILS::GetRepositoryForFile( TO_UTF8( effective.GetFullPath() ) );
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

        const git_oid head_oid = KIGIT::PROJECT_GIT_UTILS::GetCapturedHeadOid( aRepo );

        if( git_oid_is_zero( &head_oid ) )
            return MakeZeroOid();

        // For repo-level query (empty or "."), just return HEAD
        if( aPath.empty() || aPath == "." )
            return head_oid;

        const char* workdir = git_repository_workdir( aRepo );

        if( !workdir )
            return MakeZeroOid();

        wxFileName file( ResolveEffectivePath( aPath ) );

        const wxString base = KIGIT::PROJECT_GIT_UTILS::ComputeSymlinkPreservingWorkDir(
                file.GetPath(), wxString::FromUTF8( workdir ) );

        if( !file.MakeRelativeTo( base ) )
            return MakeZeroOid();

        const std::string treePath = file.GetFullPath( wxPATH_UNIX ).ToStdString( wxConvUTF8 );

        if( treePath.empty() || treePath == "." || treePath == ".." || treePath.starts_with( "../" ) )
            return MakeZeroOid();

        // For file-specific query, walk history to find last commit that touched this file
        git_revwalk* walker = nullptr;

        if( git_revwalk_new( &walker, aRepo ) != 0 )
            return MakeZeroOid();

        git_revwalk_sorting( walker, GIT_SORT_TIME );

        if( git_revwalk_push( walker, &head_oid ) != 0 )
        {
            git_revwalk_free( walker );
            return MakeZeroOid();
        }

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

                if( git_tree_entry_bypath( &entry, tree, treePath.c_str() ) == 0 )
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

    DescribeInfo ReadDescribeInfo( const std::string& aMatch, bool aAnyTags )
    {
        git_repository* repo = OpenRepo( "." );

        if( !repo )
            return { std::string(), 0 };

        const git_oid head_oid = KIGIT::PROJECT_GIT_UTILS::GetCapturedHeadOid( repo );

        if( git_oid_is_zero( &head_oid ) )
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

        if( git_revwalk_push( walker, &head_oid ) != 0 )
        {
            git_revwalk_free( walker );
            CloseRepo( repo );
            return { std::string(), 0 };
        }

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

    DescribeInfo GetDescribeInfo( const std::string& aMatch, bool aAnyTags )
    {
        const auto value = Capture( VCS_QUERY::DESCRIPTION, ".", aMatch, aAnyTags,
                [&]() -> VCS_VALUE
                {
                    const auto description = ReadDescribeInfo( aMatch, aAnyTags );
                    return { description.tag, description.distance };
                } );
        return { value.text, static_cast<int>( value.number ) };
    }

    std::string ReadCommitSignatureField( const std::string& aPath, bool aUseCommitter, bool aGetEmail )
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

    std::string GetCommitSignatureField( const std::string& aPath, bool aUseCommitter, bool aGetEmail )
    {
        return Capture( VCS_QUERY::SIGNATURE, aPath, aPath, ( aUseCommitter ? 2 : 0 ) | ( aGetEmail ? 1 : 0 ),
                [&]() -> VCS_VALUE { return { ReadCommitSignatureField( aPath, aUseCommitter, aGetEmail ) }; } ).text;
    }

} // anonymous namespace


static std::string ReadCommitHash( const std::string& aPath )
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

    char hash[GIT_OID_HEXSZ + 1];
    git_oid_tostr( hash, sizeof( hash ), &oid );

    CloseRepo( repo );
    return hash;
}


std::string GetCommitHash( const std::string& aPath, int aLength )
{
    const auto value = Capture( VCS_QUERY::HASH, aPath, aPath, 0,
            [&]() -> VCS_VALUE { return { ReadCommitHash( aPath ) }; } );
    return value.text.substr( 0, std::clamp( aLength, 4, GIT_OID_HEXSZ ) );
}


std::string GetNearestTag( const std::string& aMatch, bool aAnyTags )
{
    return GetDescribeInfo( aMatch, aAnyTags ).tag;
}


int GetDistanceFromTag( const std::string& aMatch, bool aAnyTags )
{
    return GetDescribeInfo( aMatch, aAnyTags ).distance;
}


static bool ReadIsDirty( bool aIncludeUntracked )
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


bool IsDirty( bool aIncludeUntracked )
{
    return Capture( VCS_QUERY::DIRTY, ".", "", aIncludeUntracked,
            [&]() -> VCS_VALUE { return { {}, ReadIsDirty( aIncludeUntracked ) }; } ).number != 0;
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


static std::string ReadBranch()
{
    git_repository* repo = OpenRepo( "." );

    if( !repo )
        return std::string();

    KIGIT_COMMON common( repo );
    wxString     branchName = common.GetCurrentBranchName();

    CloseRepo( repo );
    return branchName.ToStdString();
}


std::string GetBranch()
{
    return Capture( VCS_QUERY::BRANCH, ".", "", 0,
            []() -> VCS_VALUE { return { ReadBranch() }; } ).text;
}


static int64_t ReadCommitTimestamp( const std::string& aPath )
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


int64_t GetCommitTimestamp( const std::string& aPath )
{
    return Capture( VCS_QUERY::TIMESTAMP, aPath, aPath, 0,
            [&]() -> VCS_VALUE { return { {}, ReadCommitTimestamp( aPath ) }; } ).number;
}


std::string GetCommitDate( const std::string& aPath )
{
    int64_t timestamp = GetCommitTimestamp( aPath );
    return timestamp > 0 ? std::to_string( timestamp ) : std::string();
}


TEXT_EVAL::ENVIRONMENT::VCS_VALUE ReadSource( const TEXT_EVAL::ENVIRONMENT::VCS_KEY& aKey )
{
    const auto read = [&]() -> VCS_VALUE
    {
        const auto& [query, path, argument, options, contextFile] = aKey;

        if( query == VCS_QUERY::HEAD )
        {
            if( !GetGitBackend() )
                return {};

            git_repository* raw = nullptr;

            // Discovery could select the main repository instead of this linked worktree's HEAD.
            if( git_repository_open( &raw, path.ToUTF8().data() ) != 0 )
                return {};

            KIGIT::GitRepositoryPtr repo( raw );
            git_oid oid{};

            if( git_reference_name_to_id( &oid, repo.get(), "HEAD" ) != 0 )
                return {};

            char hash[GIT_OID_HEXSZ + 1];
            git_oid_tostr( hash, sizeof( hash ), &oid );
            return { hash };
        }

        const bool fileQuery = ( query == VCS_QUERY::HASH || query == VCS_QUERY::SIGNATURE
                                 || query == VCS_QUERY::TIMESTAMP )
                               && !argument.empty() && argument != ".";
        const CONTEXT_PATH_SCOPE context( fileQuery || contextFile ? wxFileName( path ).GetPath() : path );
        const std::string file = fileQuery ? path.ToStdString( wxConvUTF8 ) : std::string();

        switch( query )
        {
        case VCS_QUERY::HASH:
            return { ReadCommitHash( file ) };

        case VCS_QUERY::DESCRIPTION:
        {
            const auto description = ReadDescribeInfo( argument, options != 0 );
            return { description.tag, description.distance };
        }

        case VCS_QUERY::SIGNATURE:
            return { ReadCommitSignatureField( file, ( options & 2 ) != 0, ( options & 1 ) != 0 ) };

        case VCS_QUERY::BRANCH:
            return { ReadBranch() };

        case VCS_QUERY::DIRTY:
            return { {}, ReadIsDirty( options != 0 ) };

        case VCS_QUERY::TIMESTAMP:
            return { {}, ReadCommitTimestamp( file ) };

        case VCS_QUERY::HEAD:
            break;
        }

        return {};
    };

    if( auto* environment = TEXT_EVAL::ENVIRONMENT::Current() )
        return environment->VcsValue( aKey, read );

    return read();
}

} // namespace TEXT_EVAL_VCS
