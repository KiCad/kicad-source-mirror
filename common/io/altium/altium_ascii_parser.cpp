/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2019-2020 Thomas Pointhuber <thomas.pointhuber@gmx.at>
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

#include "altium_ascii_parser.h"
#include "altium_parser_utils.h"


ALTIUM_ASCII_PARSER::ALTIUM_ASCII_PARSER( const wxString& aInputFile ) :
        m_fileInput( aInputFile.fn_str() )
{
}


std::map<wxString, wxString> ALTIUM_ASCII_PARSER::ReadProperties()
{
    std::map<wxString, wxString> kv;
    std::string str;
    std::string line;

    // Lines ending with |> continue on the next line
    do
    {
        if( !std::getline( m_fileInput, line ) )
        {
            m_error = true;
            return kv;
        }

        if( ( line.size() > 2 && line[line.size() - 2] == '|' && line[line.size() - 1] == '>' ) )
        {
            str.append( line, 0, line.size() - 2 );
        }
        else
        {
            str.append( line );
            break;
        }
    } while( true );

    std::size_t token_end = 0;

    while( token_end < str.size() && token_end != std::string::npos )
    {
        std::size_t token_start = str.find( '|', token_end );
        std::size_t token_equal = str.find( '=', token_end );
        std::size_t key_start;

        if( token_start <= token_equal )
        {
            key_start = token_start + 1;
        }
        else
        {
            // Leading "|" before "RECORD=28" may be missing in older schematic versions.
            key_start = token_end;
        }

        token_end = str.find( '|', key_start );

        if( token_equal >= token_end )
        {
            continue; // this looks like an error: skip the entry. Also matches on std::string::npos
        }

        if( token_end == std::string::npos )
        {
            token_end = str.size() + 1; // this is the correct offset
        }

        std::string keyS = str.substr( key_start, token_equal - key_start );
        std::string valueS = str.substr( token_equal + 1, token_end - token_equal - 1 );

        // convert the strings to wxStrings, since we use them everywhere
        // value can have non-ASCII characters, so we convert them from LATIN1/ISO8859-1
        wxString key( keyS.c_str(), wxConvISO8859_1 );

        // Altium stores keys either in Upper, or in CamelCase. Lets unify it.
        wxString canonicalKey = key.Trim( false ).Trim( true ).MakeUpper();

        // If the key starts with '%UTF8%' we have to parse the value using UTF8
        wxString value;

        if( canonicalKey.StartsWith( "%UTF8%" ) )
            value = wxString( valueS.c_str(), wxConvUTF8 );
        else
            value = wxString( valueS.c_str(), wxConvISO8859_1 );

        if( canonicalKey != wxS( "PATTERN" ) && canonicalKey != wxS( "SOURCEFOOTPRINTLIBRARY" ) )
        {
            // Breathless hack because I haven't a clue what the story is here (but this character
            // appears in a lot of radial dimensions and is rendered by Altium as a space).
            value.Replace( wxT( "ÿ" ), wxT( " " ) );
        }

        // Storage binary data do not need conversion.
        if( str.rfind( "|BINARY", 0 ) != 0 )
        {
            if( canonicalKey == wxT( "DESIGNATOR" ) || canonicalKey == wxT( "NAME" )
                || canonicalKey == wxT( "TEXT" ) )
            {
                wxString recordType = kv[ wxT( "RECORD" ) ];

                // RECORD=33 (FILE_NAME) stores Windows-formatted paths; keep the backslashes intact.
                if( recordType != wxT( "4" ) && recordType != wxT( "33" ) )
                    value = AltiumPropertyToKiCadString( value );
            }
        }

        kv.insert( { canonicalKey, value.Trim() } );
    }

    return kv;
}


bool ALTIUM_ASCII_PARSER::CanRead()
{
    return m_fileInput && m_fileInput.peek() != std::ifstream::traits_type::eof();
}
