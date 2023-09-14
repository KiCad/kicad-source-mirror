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

#ifndef GIT_COMMIT_HANDLER_H
#define GIT_COMMIT_HANDLER_H

// Define a class to handle git commit operations

#include <git/kicad_git_common.h>
#include <git2.h>

#include <string>
#include <vector>

class GIT_COMMIT_HANDLER : public KIGIT_COMMON
{
public:
    GIT_COMMIT_HANDLER( git_repository* aRepo );
    virtual ~GIT_COMMIT_HANDLER();

    enum class CommitResult
    {
        Success,
        Error,
        Cancelled
    };

    CommitResult PerformCommit( const std::vector<std::string>& aFilesToCommit );

    std::string GetErrorString() const;

private:
    void AddErrorString( const std::string& aErrorString );

    std::string m_errorString;
};

#endif // GIT_COMMIT_HANDLER_H