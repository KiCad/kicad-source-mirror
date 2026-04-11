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

#ifndef GIT_RESOLVE_CONFLICT_HANDLER_H
#define GIT_RESOLVE_CONFLICT_HANDLER_H

#include <git2.h>
#include <import_export.h>

class wxString;

class APIEXPORT GIT_RESOLVE_CONFLICT_HANDLER
{
public:
    GIT_RESOLVE_CONFLICT_HANDLER( git_repository* aRepository );
    virtual ~GIT_RESOLVE_CONFLICT_HANDLER();

    bool PerformResolveConflict();

private:
    git_repository* m_repository;
};

#endif // GIT_RESOLVE_CONFLICT_HANDLER_H
