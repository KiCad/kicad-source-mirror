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

#include "qa_utils/scoped_global_lib_table.h"

#include <qa_utils/wx_utils/unit_test_utils.h>

#include <libraries/library_manager.h>
#include <paths.h>

#include <wx/filefn.h>


KI_TEST::SCOPED_GLOBAL_LIB_TABLE::SCOPED_GLOBAL_LIB_TABLE( LIBRARY_TABLE_TYPE aType ) :
        m_path( LIBRARY_MANAGER::DefaultGlobalTablePath( aType ) )
{
    wxFileName qaTable( GetTestDataRootDir(), m_path.GetFullName() );
    qaTable.AppendDir( wxS( "config" ) );
    qaTable.AppendDir( m_path.GetDirs().Last() );

    m_created = !m_path.FileExists() && PATHS::EnsurePathExists( m_path.GetFullPath(), true )
                && wxCopyFile( qaTable.GetFullPath(), m_path.GetFullPath() );
}


KI_TEST::SCOPED_GLOBAL_LIB_TABLE::~SCOPED_GLOBAL_LIB_TABLE()
{
    if( m_created )
        wxRemoveFile( m_path.GetFullPath() );
}
