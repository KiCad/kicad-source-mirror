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

} // namespace KIGIT

