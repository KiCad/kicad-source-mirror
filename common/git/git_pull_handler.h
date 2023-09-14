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

#ifndef GITPULLHANDLER_HPP
#define GITPULLHANDLER_HPP

#include <git2.h>
#include <functional>
#include <vector>
#include <string>

#include "kicad_git_common.h"

#include <git/git_progress.h>

// Structure to store commit details
struct CommitDetails
{
    std::string m_sha;
    std::string m_firstLine;
    std::string m_author;
    std::string m_date;
};

// Enum for result codes
enum class PullResult
{
    Success,
    Error,
    UpToDate,
    FastForward,
    MergeFailed
};

struct ConflictData
{
    std::string filename;
    std::string our_status;
    std::string their_status;
    git_oid our_oid;
    git_oid their_oid;
    git_time_t our_commit_time;
    git_time_t their_commit_time;
    bool use_ours; // Flag indicating user's choice (true = ours, false = theirs)
};


class GIT_PULL_HANDLER : public KIGIT_COMMON, public GIT_PROGRESS
{
public:
    GIT_PULL_HANDLER( git_repository* aRepo );
    ~GIT_PULL_HANDLER();

    PullResult PerformPull();

    bool PerformFetch();

    const std::vector<std::pair<std::string, std::vector<CommitDetails>>>& GetFetchResults() const;

    // Set the callback function for conflict resolution
    void SetConflictCallback(
            std::function<int( std::vector<ConflictData>& aConflicts )> aCallback )
    {
        m_conflictCallback = aCallback;
    }

    void UpdateProgress( int aCurrent, int aTotal, const wxString& aMessage ) override;

private:
    std::string getFirstLineFromCommitMessage( const std::string& aMessage );
    std::string getFormattedCommitDate( const git_time& aTime );private:

    PullResult handleFastForward();
    PullResult handleMerge( const git_annotated_commit** aMergeHeads, size_t aMergeHeadsCount);

    std::vector<std::pair<std::string, std::vector<CommitDetails>>> m_fetchResults;
    std::function<int( std::vector<ConflictData>& aConflicts )>     m_conflictCallback;
};

#endif // GITPULLHANDLER_HPP
