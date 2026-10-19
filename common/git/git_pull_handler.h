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

#ifndef _GIT_PULL_HANDLER_H_
#define _GIT_PULL_HANDLER_H_

#include <git/git_repo_mixin.h>
#include <import_export.h>

// Enum for result codes, error codes are negative, success codes are positive
enum class PullResult : int
{
    Conflict = -4,
    DirtyWorkdir = -3,
    MergeFailed = -2,
    Error = -1,
    Success = 0,
    UpToDate,
    FastForward
};

class APIEXPORT GIT_PULL_HANDLER : public KIGIT_REPO_MIXIN
{
public:
    GIT_PULL_HANDLER( KIGIT_COMMON* aCommon );
    ~GIT_PULL_HANDLER();

    bool       PerformFetch( bool aSkipLock = false );
    PullResult PerformPull();
};

#endif // _GIT_PULL_HANDLER_H_
