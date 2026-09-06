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
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include <sch_io/orcad/orcad_cis.h>
#include <sch_io/orcad/orcad_stream.h>

#include <algorithm>
#include <charconv>
#include <boost/algorithm/string/split.hpp>

#include <ki_exception.h>


namespace
{

std::string payload( const std::vector<char>& aData )
{
    if( aData.size() < 4 )
        THROW_IO_ERROR( wxS( "OrCAD CIS stream is missing its length" ) );

    ORCAD_STREAM stream( aData );
    uint32_t length = stream.ReadU32();

    if( length != aData.size() - 4 )
        THROW_IO_ERROR( wxS( "OrCAD CIS stream length is invalid" ) );

    return std::string( aData.begin() + 4, aData.end() );
}


std::vector<std::string> split( const std::string& aText, unsigned char aSeparator )
{
    std::vector<std::string> result;
    boost::split( result, aText,
                  [aSeparator]( unsigned char aByte ) { return aByte == aSeparator; } );

    return result;
}


uint32_t decimal( const std::string& aText, const wxString& aWhat )
{
    if( aText.empty() )
        THROW_IO_ERROR( aWhat );

    uint32_t value = 0;
    auto [end, error] = std::from_chars( aText.data(), aText.data() + aText.size(), value );

    if( error != std::errc() || end != aText.data() + aText.size() )
        THROW_IO_ERROR( aWhat );

    return value;
}


std::vector<std::string> readStrings( ORCAD_STREAM& aStream, const wxString& aWhat )
{
    uint32_t                 count = aStream.ReadU32();
    std::vector<std::string> result;

    if( count > aStream.Remaining() / 3 )
        THROW_IO_ERROR( aWhat );

    result.reserve( count );

    for( uint32_t i = 0; i < count; ++i )
        result.push_back( aStream.ReadLzt() );

    return result;
}

} // namespace


std::vector<std::string> OrcadCisParseCountedList( const std::vector<char>& aData, uint8_t aSeparator )
{
    std::string contents = payload( aData );

    if( contents.size() == 1 && static_cast<unsigned char>( contents.front() ) == 0xFB )
        return {};

    std::vector<std::string> fields = split( contents, aSeparator );

    uint32_t count = decimal( fields.front(), wxS( "OrCAD CIS list count is invalid" ) );
    fields.erase( fields.begin() );

    if( fields.size() != count )
        THROW_IO_ERROR( wxS( "OrCAD CIS list count does not match its contents" ) );

    return fields;
}


std::map<uint32_t, std::map<std::string, std::string>> OrcadCisParsePropertyUpdates( const std::vector<char>& aData )
{
    std::map<uint32_t, std::map<std::string, std::string>> result;

    for( const std::string& record : split( payload( aData ), '~' ) )
    {
        if( record.empty() )
            continue;

        size_t namesAt = record.find( static_cast<char>( 0xB0 ) );
        size_t valuesAt = record.find( static_cast<char>( 0xC0 ), namesAt == std::string::npos ? 0 : namesAt + 1 );

        if( namesAt == std::string::npos || valuesAt == std::string::npos )
            THROW_IO_ERROR( wxS( "OrCAD CIS property update is malformed" ) );

        uint32_t occurrence = decimal( record.substr( 0, namesAt ), wxS( "OrCAD CIS occurrence ID is invalid" ) );
        std::vector<std::string> names = split( record.substr( namesAt + 1, valuesAt - namesAt - 1 ), '^' );
        std::vector<std::string> values = split( record.substr( valuesAt + 1 ), '^' );

        if( names.size() != values.size() )
            THROW_IO_ERROR( wxS( "OrCAD CIS property names and values have different counts" ) );

        auto& properties = result[occurrence];

        for( size_t i = 0; i < names.size(); ++i )
        {
            if( names[i].empty() )
                THROW_IO_ERROR( wxS( "OrCAD CIS property name is empty" ) );

            properties[names[i]] = values[i];
        }
    }

    return result;
}


std::map<uint32_t, bool> OrcadCisParseMemberships( const std::vector<char>& aData )
{
    std::map<uint32_t, bool> result;

    for( const std::string& record : split( payload( aData ), '~' ) )
    {
        if( record.empty() )
            continue;

        size_t separator = record.find( static_cast<char>( 0xB0 ) );

        if( separator != 1 || ( record[0] != '0' && record[0] != '1' ) )
            THROW_IO_ERROR( wxS( "OrCAD CIS group membership is malformed" ) );

        uint32_t occurrence =
                decimal( record.substr( separator + 1 ), wxS( "OrCAD CIS membership occurrence ID is invalid" ) );
        result[occurrence] = record[0] == '1';
    }

    return result;
}


ORCAD_CIS_SCHEMATIC_INFO OrcadCisParseSchematicInfo( const std::vector<char>& aData )
{
    ORCAD_STREAM             stream( aData );
    ORCAD_CIS_SCHEMATIC_INFO result;
    uint32_t                 recordCount = stream.ReadU32();

    for( uint32_t recordIndex = 0; recordIndex < recordCount; ++recordIndex )
    {
        uint32_t                 databaseId = stream.ReadU32();
        std::vector<std::string> groups = readStrings( stream, wxS( "OrCAD CIS group list is invalid" ) );
        std::vector<std::string> states = readStrings( stream, wxS( "OrCAD CIS group state list is invalid" ) );
        uint32_t                 propertySetCount = stream.ReadU32();

        if( states.size() != groups.size() || propertySetCount != groups.size() )
            THROW_IO_ERROR( wxS( "OrCAD CIS schematic group counts do not match" ) );

        for( uint32_t groupIndex = 0; groupIndex < propertySetCount; ++groupIndex )
        {
            std::vector<std::string> names =
                    readStrings( stream, wxS( "OrCAD CIS schematic property names are invalid" ) );
            std::vector<std::string> values =
                    readStrings( stream, wxS( "OrCAD CIS schematic property values are invalid" ) );

            if( names.size() != values.size() )
                THROW_IO_ERROR( wxS( "OrCAD CIS schematic property counts do not match" ) );

            ORCAD_CIS_PROPERTIES& properties = result[groups[groupIndex]][databaseId];

            for( size_t propertyIndex = 0; propertyIndex < names.size(); ++propertyIndex )
            {
                if( names[propertyIndex].empty() )
                    THROW_IO_ERROR( wxS( "OrCAD CIS schematic property name is empty" ) );

                properties[names[propertyIndex]] = values[propertyIndex];
            }
        }

        stream.ReadU32();
    }

    if( !stream.AtEnd() )
        THROW_IO_ERROR( wxS( "OrCAD CIS schematic stream has trailing data" ) );

    return result;
}


std::string OrcadCisSelectVariant( const std::vector<std::string>&   aNames,
                                   const std::optional<std::string>& aRequested )
{
    if( aRequested )
    {
        if( std::find( aNames.begin(), aNames.end(), *aRequested ) == aNames.end() )
            THROW_IO_ERROR( wxS( "The requested OrCAD CIS variant does not exist" ) );

        return *aRequested;
    }

    if( aNames.empty() )
        return {};

    return *std::min_element( aNames.begin(), aNames.end() );
}
