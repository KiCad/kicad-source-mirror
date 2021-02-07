/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2019-2020 Thomas Pointhuber <thomas.pointhuber@gmx.at>
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

#include "altium_parser.h"

#include <compoundfilereader.h>
#include <ki_exception.h>
#include <sstream>
#include <utf.h>
#include <wx/translation.h>
#include <wx/wx.h>


const CFB::COMPOUND_FILE_ENTRY* FindStream(
        const CFB::CompoundFileReader& aReader, const char* aStreamName )
{
    const CFB::COMPOUND_FILE_ENTRY* ret = nullptr;
    aReader.EnumFiles( aReader.GetRootEntry(), -1,
            [&]( const CFB::COMPOUND_FILE_ENTRY* aEntry, const CFB::utf16string& aU16dir,
                    int level ) -> void {
                if( aReader.IsStream( aEntry ) )
                {
                    std::string name = UTF16ToUTF8( aEntry->name );
                    if( aU16dir.length() > 0 )
                    {
                        std::string dir = UTF16ToUTF8( aU16dir.c_str() );
                        if( strncmp( aStreamName, dir.c_str(), dir.length() ) == 0
                                && aStreamName[dir.length()] == '\\'
                                && strcmp( aStreamName + dir.length() + 1, name.c_str() ) == 0 )
                        {
                            ret = aEntry;
                        }
                    }
                    else
                    {
                        if( strcmp( aStreamName, name.c_str() ) == 0 )
                        {
                            ret = aEntry;
                        }
                    }
                }
            } );
    return ret;
}


ALTIUM_PARSER::ALTIUM_PARSER(
        const CFB::CompoundFileReader& aReader, const CFB::COMPOUND_FILE_ENTRY* aEntry )
{
    m_subrecord_end = nullptr;
    m_size          = static_cast<size_t>( aEntry->size );
    m_error         = false;
    m_content.reset( new char[m_size] );
    m_pos = m_content.get();

    // read file into buffer
    aReader.ReadFile( aEntry, 0, m_content.get(), m_size );
}


std::map<wxString, wxString> ALTIUM_PARSER::ReadProperties()
{
    std::map<wxString, wxString> kv;

    uint32_t length = Read<uint32_t>();
    if( length > GetRemainingBytes() )
    {
        m_error = true;
        return kv;
    }

    // There is one case by kliment where Board6 ends with "|NEARDISTANCE=1000mi".
    // Both the 'l' and the null-byte are missing, which looks like Altium swallowed two bytes.
    if( m_pos[length - 1] != '\0' )
    {
        wxLogError( "For Altium import, we assumes a null byte at the end of a list of properties. "
                    "Because this is missing, imported data might be malformed or missing." );
    }

    //we use std::string because std::string can handle NULL-bytes
    //wxString would end the string at the first NULL-byte
    std::string str = std::string( m_pos, length - 1 );
    m_pos += length;

    std::size_t token_end = 0;
    while( token_end < str.size() && token_end != std::string::npos )
    {
        std::size_t token_start = str.find( '|', token_end );
        std::size_t token_equal = str.find( '=', token_start );
        token_end               = str.find( '|', token_equal );

        std::string keyS   = str.substr( token_start + 1, token_equal - token_start - 1 );
        std::string valueS = str.substr( token_equal + 1, token_end - token_equal - 1 );

        //convert the strings to wxStrings, since we use them everywhere
        //value can have non-ASCII characters, so we convert them from LATIN1/ISO8859-1
        wxString key( keyS.c_str(), wxConvISO8859_1 );
        wxString value( valueS.c_str(), wxConvISO8859_1 );

        // Altium stores keys either in Upper, or in CamelCase. Lets unify it.
        kv.insert( { key.Trim().MakeUpper(), value.Trim() } );
    }

    return kv;
}

int ALTIUM_PARSER::PropertiesReadInt(
        const std::map<wxString, wxString>& aProperties, const wxString& aKey, int aDefault )
{
    const std::map<wxString, wxString>::const_iterator& value = aProperties.find( aKey );
    return value == aProperties.end() ? aDefault : wxAtoi( value->second );
}

double ALTIUM_PARSER::PropertiesReadDouble(
        const std::map<wxString, wxString>& aProperties, const wxString& aKey, double aDefault )
{
    const std::map<wxString, wxString>::const_iterator& value = aProperties.find( aKey );
    if( value == aProperties.end() )
    {
        return aDefault;
    }

    // Locale independent str -> double conversation
    std::istringstream istr( (const char*) value->second.mb_str() );
    istr.imbue( std::locale( "C" ) );

    double doubleValue;
    istr >> doubleValue;
    return doubleValue;
}

bool ALTIUM_PARSER::PropertiesReadBool(
        const std::map<wxString, wxString>& aProperties, const wxString& aKey, bool aDefault )
{
    const std::map<wxString, wxString>::const_iterator& value = aProperties.find( aKey );
    if( value == aProperties.end() )
        return aDefault;
    else
        return value->second == "T" || value->second == "TRUE";
}

int32_t ALTIUM_PARSER::PropertiesReadKicadUnit( const std::map<wxString, wxString>& aProperties,
        const wxString& aKey, const wxString& aDefault )
{
    const wxString& value = PropertiesReadString( aProperties, aKey, aDefault );

    wxString prefix;
    if( !value.EndsWith( "mil", &prefix ) )
    {
        wxLogError( wxString::Format( "Unit '%s' does not end with mil", value ) );
        return 0;
    }

    double mils;
    if( !prefix.ToCDouble( &mils ) )
    {
        wxLogError( wxString::Format( "Cannot convert '%s' into double", prefix ) );
        return 0;
    }

    return ConvertToKicadUnit( mils * 10000 );
}

wxString ALTIUM_PARSER::PropertiesReadString( const std::map<wxString, wxString>& aProperties,
        const wxString& aKey, const wxString& aDefault )
{
    const std::map<wxString, wxString>::const_iterator& value = aProperties.find( aKey );
    return value == aProperties.end() ? aDefault : value->second;
}
