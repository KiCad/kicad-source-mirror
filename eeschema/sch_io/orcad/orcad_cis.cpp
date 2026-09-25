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
#include <cctype>
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


std::map<uint32_t, ORCAD_CIS_PROPERTIES> OrcadCisParsePropertyUpdates( const std::vector<char>& aData )
{
    std::map<uint32_t, ORCAD_CIS_PROPERTIES> result;
    std::string                              text = payload( aData );
    constexpr char                           namesMark = static_cast<char>( 0xB0 );
    constexpr char                           valuesMark = static_cast<char>( 0xC0 );

    // Values may contain '~', so only the final '~' or one before another "<id>°<names>À" header ends a record
    auto closesRecord = [&]( size_t aTilde )
    {
        if( aTilde + 1 == text.size() )
            return true;

        size_t digits = aTilde + 1;

        while( digits < text.size() && std::isdigit( static_cast<unsigned char>( text[digits] ) ) )
            ++digits;

        if( digits == aTilde + 1 || digits >= text.size() || text[digits] != namesMark )
            return false;

        size_t values = text.find( valuesMark, digits );
        return values != std::string::npos && text.find( '~', digits ) > values;
    };

    size_t start = 0;

    while( start < text.size() )
    {
        size_t namesAt = text.find( namesMark, start );
        size_t valuesAt = namesAt == std::string::npos ? std::string::npos : text.find( valuesMark, namesAt + 1 );

        if( valuesAt == std::string::npos )
            THROW_IO_ERROR( wxS( "OrCAD CIS property update is malformed" ) );

        size_t end = text.find( '~', valuesAt );

        while( end != std::string::npos && !closesRecord( end ) )
            end = text.find( '~', end + 1 );

        uint32_t occurrence =
                decimal( text.substr( start, namesAt - start ), wxS( "OrCAD CIS occurrence ID is invalid" ) );
        std::vector<std::string> names = split( text.substr( namesAt + 1, valuesAt - namesAt - 1 ), '^' );
        std::vector<std::string> values =
                split( text.substr( valuesAt + 1, end == std::string::npos ? end : end - valuesAt - 1 ), '^' );

        start = end == std::string::npos ? text.size() : end + 1;

        // Capture truncates a stream's last value at a '~', leaving fewer values than names
        if( names.size() != values.size() )
        {
            if( start < text.size() )
                THROW_IO_ERROR( wxS( "OrCAD CIS property names and values have different counts" ) );

            continue;
        }

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
    std::vector<std::string> records = split( payload( aData ), 0xB0 );

    // The first field flags whether the group carries property updates
    for( size_t i = 1; i < records.size(); ++i )
    {
        const std::string& record = records[i];

        if( record.empty() )
            continue;

        size_t separator = record.find( '~' );

        if( separator == std::string::npos || separator + 2 != record.size()
            || ( record.back() != '0' && record.back() != '1' ) )
        {
            THROW_IO_ERROR( wxS( "OrCAD CIS group membership is malformed" ) );
        }

        uint32_t occurrence =
                decimal( record.substr( 0, separator ), wxS( "OrCAD CIS membership occurrence ID is invalid" ) );
        result[occurrence] = record.back() == '1';
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
