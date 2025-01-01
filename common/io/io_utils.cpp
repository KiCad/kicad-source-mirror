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
* along with this program; if not, you may find one here:
* http://www.gnu.org/licenses/old-licenses/gpl-2.0.html
* or you may search the http://www.gnu.org website for the version 2 license,
* or you may write to the Free Software Foundation, Inc.,
* 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
*/

#include "io_utils.h"

#include <wx/wfstream.h>
#include <wx/txtstrm.h>

namespace IO_UTILS
{
    const std::vector<uint8_t> COMPOUND_FILE_HEADER = { 0xD0, 0xCF, 0x11, 0xE0,
                                                        0xA1, 0xB1, 0x1A, 0xE1 };

bool fileStartsWithPrefix( const wxString& aFilePath, const wxString& aPrefix,
                           bool aIgnoreWhitespace )
{
    wxFFileInputStream input( aFilePath );

    if( input.IsOk() && !input.Eof() )
    {
        // Find first non-empty line
        wxTextInputStream text( input );
        wxString          line = text.ReadLine();

        if( aIgnoreWhitespace )
        {
            while( !input.Eof() && line.IsEmpty() )
                line = text.ReadLine().Trim( false /*trim from left*/ );
        }

        if( line.StartsWith( aPrefix ) )
            return true;
    }

    return false;
}


bool fileStartsWithBinaryHeader( const wxString& aFilePath, const std::vector<uint8_t>& aHeader )
{
    wxFFileInputStream input( aFilePath );

    if( input.IsOk() && !input.Eof() )
    {
        if( static_cast<size_t>( input.GetLength() ) < aHeader.size() )
            return false;

        std::vector<uint8_t> parsedHeader( aHeader.size() );

        if( !input.ReadAll( parsedHeader.data(), parsedHeader.size() ) )
            return false;

        return parsedHeader == aHeader;
    }

    return false;
}

}