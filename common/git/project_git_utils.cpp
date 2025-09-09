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
#include "git_backend.h"

namespace KIGIT
{

git_repository* PROJECT_GIT_UTILS::GetRepositoryForFile( const char* aFilename )
{
    return GetGitBackend()->GetRepositoryForFile( aFilename );
}

int PROJECT_GIT_UTILS::CreateBranch( git_repository* aRepo, const wxString& aBranchName )
{
    return GetGitBackend()->CreateBranch( aRepo, aBranchName );
}

bool PROJECT_GIT_UTILS::RemoveVCS( git_repository*& aRepo, const wxString& aProjectPath,
                                  bool aRemoveGitDir, wxString* aErrors )
{
    return GetGitBackend()->RemoveVCS( aRepo, aProjectPath, aRemoveGitDir, aErrors );
}

} // namespace KIGIT
