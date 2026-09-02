/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
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

#pragma once

#include <libraries/library_table.h>

#include <wx/filename.h>

namespace KI_TEST
{

/**
 * Global tables load only from the user config dir, and a CI config may ship none. When the table of the given
 * type is missing there, this puts the QA config's copy in its place for the life of the object.
 */
class SCOPED_GLOBAL_LIB_TABLE
{
public:
    explicit SCOPED_GLOBAL_LIB_TABLE( LIBRARY_TABLE_TYPE aType );
    ~SCOPED_GLOBAL_LIB_TABLE();

    SCOPED_GLOBAL_LIB_TABLE( const SCOPED_GLOBAL_LIB_TABLE& ) = delete;
    SCOPED_GLOBAL_LIB_TABLE& operator=( const SCOPED_GLOBAL_LIB_TABLE& ) = delete;

private:
    wxFileName m_path;
    bool       m_created = false;
};

} // namespace KI_TEST
