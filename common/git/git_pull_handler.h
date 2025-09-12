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

#ifndef _GIT_PULL_HANDLER_H_
#define _GIT_PULL_HANDLER_H_

#include <git/git_repo_mixin.h>
#include <import_export.h>

#include <vector>
#include <string>
#include <wx/string.h>

// Structure to store commit details
struct CommitDetails
{
    std::string m_sha;
    std::string m_firstLine;
    std::string m_author;
    std::string m_date;
};

// Enum for result codes, error codes are negative, success codes are positive
enum class PullResult : int
{
    MergeFailed = -2,
    Error = -1,
    Success = 0,
    UpToDate,
    FastForward
};

class LIBGIT_BACKEND;

class APIEXPORT GIT_PULL_HANDLER : public KIGIT_REPO_MIXIN
{
public:
    friend class LIBGIT_BACKEND;
    GIT_PULL_HANDLER( KIGIT_COMMON* aCommon );
    ~GIT_PULL_HANDLER();

    bool       PerformFetch( bool aSkipLock = false );
    PullResult PerformPull();

    const std::vector<std::pair<std::string, std::vector<CommitDetails>>>& GetFetchResults() const;

    // Implementation for progress updates
    void UpdateProgress( int aCurrent, int aTotal, const wxString& aMessage ) override;

private:
    std::vector<std::pair<std::string, std::vector<CommitDetails>>> m_fetchResults;
};

#endif // _GIT_PULL_HANDLER_H_
