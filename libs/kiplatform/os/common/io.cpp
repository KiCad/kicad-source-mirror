/*
* This program source code file is part of KiCad, a free EDA CAD application.
*
* Copyright The KiCad Developers, see AUTHORS.txt for contributors.
*
* This program is free software: you can redistribute it and/or modify it
* under the terms of the GNU General Public License as published by the
* Free Software Foundation, either version 3 of the License, or (at your
* option) any later version.
*
* This program is distributed in the hope that it will be useful, but
* WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
* General Public License for more details.
*
* You should have received a copy of the GNU General Public License along
* with this program.  If not, see <https://www.gnu.org/licenses/>.
*/

#include <kiplatform/io.h>

#include <wx/crt.h>
#include <wx/string.h>

#include <stdexcept>
#include <string>


void KIPLATFORM::IO::MAPPED_FILE::readIntoBuffer( const wxString& aFileName )
{
    FILE* fp = wxFopen( aFileName, wxS( "rb" ) );

    if( !fp )
        throw std::runtime_error( std::string( "Cannot open file: " ) + aFileName.ToStdString() );

    fseek( fp, 0, SEEK_END );
    long len = ftell( fp );

    if( len < 0 )
    {
        fclose( fp );
        throw std::runtime_error( std::string( "Cannot determine file size: " )
                                  + aFileName.ToStdString() );
    }

    m_fallbackBuffer.resize( static_cast<size_t>( len ) );
    fseek( fp, 0, SEEK_SET );

    size_t bytesRead = fread( m_fallbackBuffer.data(), 1, static_cast<size_t>( len ), fp );
    fclose( fp );

    if( bytesRead != static_cast<size_t>( len ) )
    {
        throw std::runtime_error( std::string( "Failed to read file: " )
                                  + aFileName.ToStdString() );
    }

    m_data = m_fallbackBuffer.data();
    m_size = m_fallbackBuffer.size();
}
