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

#ifndef GIT_COMMIT_HANDLER_H
#define GIT_COMMIT_HANDLER_H

// Define a class to handle git commit operations

#include <git/kicad_git_common.h>
#include <import_export.h>
#include "git_backend.h"

#include <chrono>
#include <string>
#include <vector>
#include <wx/arrstr.h> // for MSVC to see std::vector<wxString> is exported from wx
#include <wx/string.h>

class APIEXPORT GIT_COMMIT_HANDLER : public KIGIT_COMMON, public KIGIT_ERRORS
{
public:
    GIT_COMMIT_HANDLER( git_repository* aRepo );
    virtual ~GIT_COMMIT_HANDLER();

    CommitResult PerformCommit( const std::vector<wxString>& aFiles,
                               const wxString&               aMessage,
                               const wxString&               aAuthorName,
                               const wxString&               aAuthorEmail );

    CommitResult PerformAmend( const std::vector<wxString>& aFiles, const wxString& aMessage,
                               const wxString& aAuthorName, const wxString& aAuthorEmail );

    UndoCommitResult PerformUndoLastCommit( const wxString& aExpectedBranch, const git_oid& aExpectedHead,
                                            wxString& aMessage );

    /// Answer CommitResult::Unchanged rather than record a commit whose tree is the last one's
    void SetSkipUnchanged( bool aSkip ) { m_skipUnchanged = aSkip; }
    bool SkipUnchanged() const { return m_skipUnchanged; }

    /// Asked before each attempt and just before the branch moves; true gives the commit up as Cancelled
    void SetCancelCheck( KIGIT_CANCEL aCancelled ) { m_cancelled = std::move( aCancelled ); }
    bool CommitCancelled() const { return m_cancelled && m_cancelled(); }

    /// Rerun the commit for up to aBudget while another process holds a lock or moves the branch first.
    /// Zero, the default, tries once.
    void                      SetLockRetry( std::chrono::milliseconds aBudget ) { m_lockRetry = aBudget; }
    std::chrono::milliseconds LockRetry() const { return m_lockRetry; }

    /// The full id of the commit the last commit call made, empty when it made none
    const std::string& CreatedCommit() const { return m_createdCommit; }
    void               SetCreatedCommit( const std::string& aOid ) { m_createdCommit = aOid; }

private:
    std::string               m_createdCommit;
    bool                      m_skipUnchanged = false;
    KIGIT_CANCEL              m_cancelled;
    std::chrono::milliseconds m_lockRetry{ 0 };
};

#endif // GIT_COMMIT_HANDLER_H