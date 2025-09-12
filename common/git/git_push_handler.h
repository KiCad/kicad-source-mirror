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
    Error
};

class APIEXPORT GIT_PUSH_HANDLER : public KIGIT_REPO_MIXIN
{
public:
    GIT_PUSH_HANDLER( KIGIT_COMMON* aCommon );
    ~GIT_PUSH_HANDLER();

    PushResult PerformPush();

    // Virtual method for progress reporting
    virtual void ReportProgress(int aCurrent, int aTotal, const wxString& aMessage) {}

private:

    // Implementation of GIT_PROGRESS's virtual method
    void UpdateProgress(int aCurrent, int aTotal, const wxString& aMessage) override;
};

#endif // _GIT_PUSH_HANDLER_H_
