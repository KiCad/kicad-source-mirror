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

#include "qa_utils/file_utils.h"

#include <filesystem>
#include <stdexcept>
#include <string>

#include <wx/filefn.h>
#include <wx/filename.h>
#include <wx/log.h>


using namespace KI_TEST;

SCOPED_TEMP_DIR::SCOPED_TEMP_DIR( const wxString& aPrefix )
{
    wxString reservedName = wxFileName::CreateTempFileName( aPrefix );

    if( reservedName.IsEmpty() )
    {
        throw std::runtime_error( "Cannot create a temporary directory name with prefix '"
                                  + std::string( aPrefix.utf8_str() ) + "'" );
    }

    if( !wxRemoveFile( reservedName ) )
    {
        throw std::runtime_error( "Cannot reclaim temporary name '" + std::string( reservedName.utf8_str() )
                                  + "'" );
    }

    m_path = std::filesystem::path( std::string( reservedName.utf8_str() ) );

    if( !std::filesystem::create_directory( m_path ) )
    {
        throw std::runtime_error( "Cannot create temporary directory '" + m_path.string() + "'" );
    }
}


SCOPED_TEMP_DIR::~SCOPED_TEMP_DIR()
{
    try
    {
        std::filesystem::remove_all( m_path );
    }
    catch( const std::filesystem::filesystem_error& e )
    {
        wxLogError( wxT( "Cannot remove temporary directory '%s': %s" ), wxString::FromUTF8( m_path.string() ),
                    wxString::FromUTF8( e.what() ) );
    }
}
