#include "project_git_utils.h"
#include "kicad_git_common.h"
#include "kicad_git_memory.h"
#include <trace_helpers.h>
#include <wx/log.h>

namespace KIGIT
{

git_repository* PROJECT_GIT_UTILS::GetRepositoryForFile( const char* aFilename )
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

int PROJECT_GIT_UTILS::CreateBranch( git_repository* aRepo, const wxString& aBranchName )
{
    git_oid head_oid;

    if( int error = git_reference_name_to_id( &head_oid, aRepo, "HEAD" ) != GIT_OK )
    {
        wxLogTrace( traceGit, "Failed to lookup HEAD reference: %s",
                    KIGIT_COMMON::GetLastGitError() );
        return error;
    }

    git_commit* commit = nullptr;

    if( int error = git_commit_lookup( &commit, aRepo, &head_oid ) != GIT_OK )
    {
        wxLogTrace( traceGit, "Failed to lookup commit: %s",
                    KIGIT_COMMON::GetLastGitError() );
        return error;
    }

    KIGIT::GitCommitPtr commitPtr( commit );
    git_reference* branchRef = nullptr;

    if( int error = git_branch_create( &branchRef, aRepo, aBranchName.mb_str(), commit, 0 ) != GIT_OK )
    {
        wxLogTrace( traceGit, "Failed to create branch: %s",
                    KIGIT_COMMON::GetLastGitError() );
        return error;
    }

    git_reference_free( branchRef );
    return 0;
}

} // namespace KIGIT

