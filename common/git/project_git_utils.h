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

#ifndef PROJECT_GIT_UTILS_H
#define PROJECT_GIT_UTILS_H

#include <git2.h>
#include <wx/string.h>

namespace KIGIT
{

/** Utility class with helper functions for project level git operations. */
class PROJECT_GIT_UTILS
{
public:
    /**
     * Discover and open the repository that contains the given file.
     *
     * @param aFilename Absolute path to a file inside the repository.
     * @return Newly opened git_repository or nullptr on failure.
     */
    static git_repository* GetRepositoryForFile( const char* aFilename );

    /**
     * Create a new branch based on HEAD.
     *
     * @param aRepo       Repository in which to create the branch.
     * @param aBranchName Name of the new branch.
     * @return 0 on success, libgit2 error code on failure.
     */
    static int CreateBranch( git_repository* aRepo, const wxString& aBranchName );

    /**
     * Return the current HEAD commit hash for the repository containing aProjectFile.
     *
     * @param aProjectFile Absolute path to any file within the repository (typically the
     *                     project file path).
     * @param aShort       If true, return the short (8 char) hash, otherwise full hash.
     * @return wxString containing the hash or "no hash" if unavailable.
     */
    static wxString GetCurrentHash( const wxString& aProjectFile, bool aShort );

    /**
     * Remove version control from a directory by freeing the repository and
     * optionally removing the .git directory.
     *
     * @param aRepo Repository to free (will be set to nullptr)
     * @param aProjectPath Path to the project directory
     * @param aRemoveGitDir If true, also remove the .git directory from disk
     * @param aErrors Output parameter for any error messages
     * @return True on success, false on failure
     */
static bool RemoveVCS( git_repository*& aRepo, const wxString& aProjectPath = wxEmptyString,
                      bool aRemoveGitDir = false, wxString* aErrors = nullptr );
};

} // namespace KIGIT

#endif // PROJECT_GIT_UTILS_H