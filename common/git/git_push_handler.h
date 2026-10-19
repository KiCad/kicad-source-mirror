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

#ifndef _GIT_PUSH_HANDLER_H_
#define _GIT_PUSH_HANDLER_H_

#include <git/git_progress.h>
#include <git/git_repo_mixin.h>
#include <import_export.h>
#include <git/kicad_git_errors.h>
#include <wx/string.h>

class KIGIT_COMMON;

enum class PushResult
{
    Success,
    NonFastForward,
    Busy, ///< Another git operation held the action lock, so nothing was pushed.
    Error
};

class APIEXPORT GIT_PUSH_HANDLER : public KIGIT_REPO_MIXIN
{
public:
    GIT_PUSH_HANDLER( KIGIT_COMMON* aCommon );
    ~GIT_PUSH_HANDLER();

    PushResult PerformPush( bool aForce = false );

    /// Pin the branch and full HEAD commit ID confirmed by the caller, so a queued push
    /// cannot publish a different target after another operation changes the repository.
    void SetExpectedTarget( const wxString& aBranch, const wxString& aHeadOid );

    bool HasExpectedTarget() const { return m_hasExpectedTarget; }

    /// True without an expectation, or when both values match the captured target.
    bool MatchesExpectedTarget( const wxString& aBranch, const wxString& aHeadOid ) const;

private:
    bool     m_hasExpectedTarget = false;
    wxString m_expectedBranch;
    wxString m_expectedHeadOid;
};

#endif // _GIT_PUSH_HANDLER_H_
