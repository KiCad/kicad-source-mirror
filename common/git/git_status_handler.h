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

#ifndef GIT_STATUS_HANDLER_H
#define GIT_STATUS_HANDLER_H

#include <git/git_repo_mixin.h>
#include <import_export.h>
#include <wx/string.h>
#include <map>
#include <set>

class LIBGIT_BACKEND;

struct FileStatus
{
    wxString filePath;
    KIGIT_COMMON::GIT_STATUS status;
    unsigned int gitStatus;  // Raw git status flags
};

class APIEXPORT GIT_STATUS_HANDLER : public KIGIT_REPO_MIXIN
{
public:
    GIT_STATUS_HANDLER( KIGIT_COMMON* aCommon );
    virtual ~GIT_STATUS_HANDLER();

    /**
     * Check if the repository has any changed files
     * @return True if there are uncommitted changes, false otherwise
     */
    bool HasChangedFiles();

    /**
     * Get detailed file status for all files in the specified path
     * @param aPathspec Path specification to filter files (empty for all files)
     * @return Map of absolute file paths to their status information
     */
    std::map<wxString, FileStatus> GetFileStatus( const wxString& aPathspec = wxEmptyString );

    /**
     * Get the current branch name
     * @return Current branch name, or empty string if not available
     */
    wxString GetCurrentBranchName();

    /**
     * Get status for modified files based on local/remote changes
     * @param aLocalChanges Set of files with local changes
     * @param aRemoteChanges Set of files with remote changes
     * @param aFileStatus Map to update with ahead/behind status
     */
    void UpdateRemoteStatus( const std::set<wxString>& aLocalChanges,
                             const std::set<wxString>& aRemoteChanges,
                             std::map<wxString, FileStatus>& aFileStatus );

    /**
     * Get the repository working directory path
     * @return Working directory path, or empty string if not available
     */
    wxString GetWorkingDirectory();

    void UpdateProgress( int aCurrent, int aTotal, const wxString& aMessage ) override;

private:
    friend class LIBGIT_BACKEND;
    /**
     * Convert git status flags to KIGIT_COMMON::GIT_STATUS
     * @param aGitStatus Raw git status flags
     * @return Converted status
     */
    KIGIT_COMMON::GIT_STATUS ConvertStatus( unsigned int aGitStatus );
};

#endif // GIT_STATUS_HANDLER_H