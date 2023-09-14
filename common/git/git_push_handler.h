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

#ifndef GITPUSHHANDLER_HPP
#define GITPUSHHANDLER_HPP

#include <git2.h>
#include <functional>
#include <vector>
#include <string>

#include "kicad_git_common.h"
#include <git/git_progress.h>

// Enum for result codes
enum class PushResult
{
    Success,
    Error,
    UpToDate
};

class GIT_PUSH_HANDLER : public KIGIT_COMMON, public GIT_PROGRESS
{
public:
    GIT_PUSH_HANDLER( git_repository* aRepo );
    ~GIT_PUSH_HANDLER();

    PushResult PerformPush();

    void UpdateProgress( int aCurrent, int aTotal, const wxString& aMessage ) override;

private:

};

#endif // GITPUSHHANDLER_HPP
