/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation, either version 3 of the License, or (at your option)
 * any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <boost/test/unit_test.hpp>
#include <qa_utils/wx_utils/unit_test_utils.h>

#include <sch_io/pads/pads_sch_binary_parser.h>
#include <sch_io/pads/pads_sch_binary_reader.h>

#include <ki_exception.h>

#include <algorithm>
#include <fstream>
#include <iterator>
#include <set>
#include <sstream>
#include <string>
#include <type_traits>
#include <vector>

using namespace PADS_SCH_BINARY;

namespace
{

static std::vector<uint8_t> loadMinimalV13()
{
    std::ifstream file( KI_TEST::GetEeschemaTestDataDir() + "/plugins/pads/binary/minimal_v13.sch", std::ios::binary );
    BOOST_REQUIRE( file );
    return { std::istreambuf_iterator<char>( file ), std::istreambuf_iterator<char>() };
}


static std::string canonicalModel( const PADS_SCH_MODEL& aModel )
{
    std::ostringstream out;
    out << aModel.version << '|' << aModel.source.file << '|' << aModel.source.absoluteOffset << '|'
        << aModel.settings.coordinateUnitsPerMil;

    auto provenance = [&]( const SOURCE_PROVENANCE& aSource )
    {
        out << ':' << aSource.version << ',' << aSource.objectClass << ',' << aSource.controller << ','
            << aSource.recordIndex << ',' << aSource.absoluteOffset << ',' << aSource.length << ',' << aSource.sheet;
    };

    for( const MODEL_SHEET& sheet : aModel.sheets )
    {
        out << "|sheet," << sheet.id.Value() << ',' << sheet.index << ',' << sheet.name.text;
        provenance( sheet.source );
    }

    out << "|definitions=" << aModel.definitions.size() << "|parts=" << aModel.partTypes.size()
        << "|placements=" << aModel.placements.size() << "|nets=" << aModel.nets.size()
        << "|buses=" << aModel.buses.size() << "|labels=" << aModel.labels.size()
        << "|junctions=" << aModel.junctions.size() << "|texts=" << aModel.texts.size()
        << "|graphics=" << aModel.graphics.size() << "|diagnostics=" << aModel.diagnostics.size();
    return out.str();
}


struct SNAPSHOT_PROPERTY
{
    std::string          name;
    std::string          value;
    PROPERTY_DISPOSITION disposition = PROPERTY_DISPOSITION::EXACT;

    bool operator==( const SNAPSHOT_PROPERTY& ) const = default;

    bool operator<( const SNAPSHOT_PROPERTY& aOther ) const
    {
        return std::tie( name, value, disposition ) < std::tie( aOther.name, aOther.value, aOther.disposition );
    }
};


struct SNAPSHOT_ITEM
{
    std::string                    sheet;
    std::string                    kind;
    std::vector<SNAPSHOT_PROPERTY> properties;
    std::string                    geometry;

    bool operator==( const SNAPSHOT_ITEM& ) const = default;

    bool operator<( const SNAPSHOT_ITEM& aOther ) const
    {
        return std::tie( sheet, kind, properties, geometry )
               < std::tie( aOther.sheet, aOther.kind, aOther.properties, aOther.geometry );
    }
};


using SNAPSHOT_ALLOWLIST = std::set<std::pair<std::string, PROPERTY_DISPOSITION>>;


struct ASCII_SNAPSHOT_SHEET
{
    std::string                    name;
    std::vector<SNAPSHOT_PROPERTY> properties;
};


static std::vector<SNAPSHOT_ITEM> normalizeBinaryModel( const PADS_SCH_MODEL& aModel )
{
    std::vector<SNAPSHOT_ITEM> result;

    for( const MODEL_SHEET& sheet : aModel.sheets )
    {
        SNAPSHOT_ITEM item{ sheet.name.text.ToStdString(), "sheet", {}, {} };

        for( const SOURCE_PROPERTY& property : sheet.properties )
        {
            item.properties.push_back(
                    { property.name.text.ToStdString(), property.value.text.ToStdString(), property.disposition } );
        }

        result.push_back( std::move( item ) );
    }

    return result;
}


static std::vector<SNAPSHOT_ITEM> normalizeAsciiModel( const std::vector<ASCII_SNAPSHOT_SHEET>& aSheets )
{
    std::vector<SNAPSHOT_ITEM> result;

    for( const ASCII_SNAPSHOT_SHEET& sheet : aSheets )
        result.push_back( { sheet.name, "sheet", sheet.properties, {} } );

    return result;
}


static bool snapshotsMatch( std::vector<SNAPSHOT_ITEM> aExpected, std::vector<SNAPSHOT_ITEM> aActual,
                            const SNAPSHOT_ALLOWLIST& aAllowedDifferences = {} )
{
    auto stripAllowed = [&]( std::vector<SNAPSHOT_ITEM>& aItems )
    {
        for( SNAPSHOT_ITEM& item : aItems )
        {
            std::erase_if( item.properties,
                           [&]( const SNAPSHOT_PROPERTY& aProperty )
                           {
                               return aAllowedDifferences.contains( { aProperty.name, aProperty.disposition } );
                           } );
            std::sort( item.properties.begin(), item.properties.end() );
        }

        std::sort( aItems.begin(), aItems.end() );
    };

    stripAllowed( aExpected );
    stripAllowed( aActual );
    return aExpected == aActual;
}

} // namespace


BOOST_AUTO_TEST_SUITE( PadsSchBinaryParser )


BOOST_AUTO_TEST_CASE( ModelContract )
{
    static_assert( !std::is_same_v<SHEET_ID, PLACEMENT_ID> );
    static_assert( !std::is_convertible_v<SHEET_ID, PLACEMENT_ID> );

    const std::vector<uint8_t> bytes = loadMinimalV13();
    PADS_SCH_BINARY_PARSER     parser;
    PADS_SCH_MODEL             first = parser.Parse( bytes, wxS( "minimal_v13.sch" ) );
    PADS_SCH_MODEL             second = parser.Parse( bytes, wxS( "minimal_v13.sch" ) );

    BOOST_CHECK_EQUAL( first.version, 0x000D );
    BOOST_REQUIRE_EQUAL( first.sheets.size(), 1 );
    BOOST_CHECK( first.AllReferencesResolved() );
    BOOST_CHECK( first.HasUniqueTypedIds() );
    BOOST_CHECK_EQUAL( first.sheets[0].index, 0 );
    BOOST_CHECK_EQUAL( first.sheets[0].id.Value(), 0 );
    BOOST_CHECK_EQUAL( first.sheets[0].source.file, wxS( "minimal_v13.sch" ) );
    BOOST_CHECK_GT( first.sheets[0].source.length, 0 );
    BOOST_CHECK_EQUAL( first.sheets[0].source.version, 0x000D );
    BOOST_CHECK_EQUAL( first.sheets[0].source.objectClass, wxS( "sheet" ) );
    BOOST_CHECK_EQUAL( first.sheets[0].source.recordIndex, 0 );
    BOOST_CHECK_EQUAL( first.sheets[0].source.absoluteOffset, second.sheets[0].source.absoluteOffset );
    BOOST_CHECK_EQUAL( first.sheets[0].source.length, second.sheets[0].source.length );
    BOOST_CHECK_EQUAL( first.sheets[0].id.Value(), second.sheets[0].id.Value() );
    BOOST_CHECK_EQUAL( canonicalModel( first ), canonicalModel( second ) );
    BOOST_CHECK( first == second );

    BOOST_CHECK_EQUAL( NormalizeAngle( -900 ), 2700 );
    BOOST_CHECK_EQUAL( NormalizeAngle( 4500 ), 900 );
    BOOST_CHECK_EQUAL( NormalizeCoordinate( -1234 ), -1234 );

    SOURCE_STRING sourceString{
        { 0x41, 0xC3, 0xA9 }, wxString::FromUTF8( "A\xC3\xA9" ), STRING_ENCODING_STATUS::UTF8, first.sheets[0].source
    };
    BOOST_CHECK_EQUAL( sourceString.raw.size(), 3 );
    BOOST_CHECK_EQUAL( sourceString.text, wxString::FromUTF8( "A\xC3\xA9" ) );
    BOOST_CHECK( sourceString.encoding == STRING_ENCODING_STATUS::UTF8 );

    first.diagnostics.push_back(
            { RPT_SEVERITY_WARNING, first.sheets[0].source, wxS( "unknown but structurally valid enum" ) } );
    BOOST_CHECK_EQUAL( first.diagnostics[0].source.absoluteOffset, first.sheets[0].source.absoluteOffset );

    PADS_SCH_MODEL duplicateIds = second;
    duplicateIds.sheets.push_back( duplicateIds.sheets.front() );
    const wxString duplicateMessage =
            wxS( "minimal_v13.sch: PADS schematic v0x000D model (controller -1, record 0, sheet -1) "
                 "at offset 0x0: duplicate or invalid typed controller ID" );
    BOOST_CHECK_EQUAL( FormatParserError( duplicateIds.source, wxS( "duplicate or invalid typed controller ID" ) ),
                       duplicateMessage );
    BOOST_CHECK_EXCEPTION( duplicateIds.ValidateOrThrow(), IO_ERROR,
                           [&]( const IO_ERROR& aError )
                           {
                               return aError.What().Contains( duplicateMessage );
                           } );

    PADS_SCH_MODEL unresolved = second;
    unresolved.sheets[0].parent = SHEET_REFERENCE{ SHEET_ID( 99 ), unresolved.sheets[0].source };
    BOOST_CHECK_EXCEPTION( unresolved.ValidateOrThrow(), IO_ERROR,
                           []( const IO_ERROR& aError )
                           {
                               return aError.What().Contains( wxS( "unresolved" ) );
                           } );

    PADS_SCH_MODEL cyclic = second;
    cyclic.sheets[0].parent = SHEET_REFERENCE{ cyclic.sheets[0].id, cyclic.sheets[0].source };
    BOOST_CHECK_EXCEPTION( cyclic.ValidateOrThrow(), IO_ERROR,
                           []( const IO_ERROR& aError )
                           {
                               return aError.What().Contains( wxS( "cyclic" ) )
                                      && aError.What().Contains( wxS( "offset" ) );
                           } );

    SOURCE_PROVENANCE              textSource{ wxS( "strings.sch" ), 0x000D, wxS( "text" ), 4, 7, 0x1234, 2, 0 };
    std::vector<PARSER_DIAGNOSTIC> diagnostics;
    SOURCE_STRING unknown = PADS_SCH_BINARY_PARSER::DecodeString( { 0x41 }, 99999, textSource, diagnostics );
    SOURCE_STRING invalid = PADS_SCH_BINARY_PARSER::DecodeString( { 0xC3, 0x28 }, 65001, textSource, diagnostics );
    PADS_SCH_BINARY_PARSER::RecordUnknownEnum( wxS( "line style" ), 17, textSource, diagnostics );
    BOOST_CHECK( unknown.encoding == STRING_ENCODING_STATUS::UNKNOWN_CODE_PAGE );
    BOOST_CHECK( invalid.encoding == STRING_ENCODING_STATUS::INVALID_BYTES );
    BOOST_REQUIRE_EQUAL( diagnostics.size(), 3 );
    BOOST_CHECK_EQUAL( diagnostics[0].source.absoluteOffset, 0x1234 );
    BOOST_CHECK_EQUAL( diagnostics[1].source.recordIndex, 7 );
    BOOST_CHECK( diagnostics[2].message.Contains( wxS( "line style 17" ) ) );
}


BOOST_AUTO_TEST_CASE( ConnectionEndpointValidation )
{
    SOURCE_PROVENANCE source{ wxS( "model.sch" ), 0x000D, wxS( "connection" ), 8, 3, 0x200, 16, 0 };
    PADS_SCH_MODEL    model;
    model.version = 0x000D;
    model.source = source;
    model.sheets.push_back( { SHEET_ID( 0 ), 0, source } );
    model.definitions.push_back( { DEFINITION_ID( 1 ), source } );
    model.definitions[0].pins.push_back( { PIN_ID( 2 ), source } );
    model.partTypes.push_back( { PART_TYPE_ID( 3 ), source } );
    model.partTypes[0].gates.push_back(
            { GATE_ID( 4 ), source, { DEFINITION_ID( 1 ), source }, 1, { { PIN_ID( 2 ), source } } } );
    model.placements.push_back( { PLACEMENT_ID( 5 ),
                                  source,
                                  { SHEET_ID( 0 ), source },
                                  { PART_TYPE_ID( 3 ), source },
                                  GATE_REFERENCE{ GATE_ID( 4 ), source } } );
    model.nets.push_back( { NET_ID( 6 ), source, { SHEET_ID( 0 ), source } } );
    model.nets[0].connections.push_back( { source } );
    model.nets[0].connections[0].endpoints.push_back( { MODEL_ENDPOINT_KIND::PIN, source,
                                                        PLACEMENT_REFERENCE{ PLACEMENT_ID( 5 ), source },
                                                        PIN_REFERENCE{ PIN_ID( 2 ), source } } );
    BOOST_CHECK_NO_THROW( model.ValidateOrThrow() );

    PADS_SCH_MODEL wrongPin = model;
    wrongPin.nets[0].connections[0].endpoints[0].pin = PIN_REFERENCE{ PIN_ID( 99 ), source };
    BOOST_CHECK_THROW( wrongPin.ValidateOrThrow(), IO_ERROR );

    PADS_SCH_MODEL wrongUnit = model;
    wrongUnit.placements[0].unit = 2;
    BOOST_CHECK_THROW( wrongUnit.ValidateOrThrow(), IO_ERROR );

    PADS_SCH_MODEL empty = model;
    empty.nets[0].connections[0].endpoints[0] = { MODEL_ENDPOINT_KIND::INVALID, source };
    BOOST_CHECK_THROW( empty.ValidateOrThrow(), IO_ERROR );

    PADS_SCH_MODEL mixed = model;
    mixed.nets[0].connections[0].endpoints[0].kind = MODEL_ENDPOINT_KIND::POINT;
    BOOST_CHECK_THROW( mixed.ValidateOrThrow(), IO_ERROR );

    PADS_SCH_MODEL point = model;
    point.nets[0].connections[0].endpoints[0] = { MODEL_ENDPOINT_KIND::POINT, source };
    BOOST_CHECK_NO_THROW( point.ValidateOrThrow() );
}


BOOST_AUTO_TEST_CASE( CorpusSemanticSnapshot )
{
    SOURCE_PROVENANCE source{ wxS( "snapshot.sch" ), 0x000D, wxS( "sheet" ), 3, 0, 0x250, 48, 0 };
    PADS_SCH_MODEL    binary;
    binary.sheets.push_back( { SHEET_ID( 1 ),
                               0,
                               source,
                               { { 'M', 'a', 'i', 'n' }, wxS( "Main" ), STRING_ENCODING_STATUS::UTF8, source } } );
    binary.sheets.push_back( { SHEET_ID( 2 ),
                               1,
                               source,
                               { { 'M', 'a', 'i', 'n' }, wxS( "Main" ), STRING_ENCODING_STATUS::UTF8, source } } );
    BOOST_CHECK_NE( binary.sheets[0].id.Value(), binary.sheets[1].id.Value() );
    BOOST_CHECK( normalizeBinaryModel( binary ) == normalizeBinaryModel( binary ) );
    binary.sheets[0].properties.push_back( { { {}, wxS( "value" ), STRING_ENCODING_STATUS::UTF8, source },
                                             { {}, wxS( "10k" ), STRING_ENCODING_STATUS::UTF8, source },
                                             PROPERTY_DISPOSITION::EXACT,
                                             source } );
    binary.sheets[0].properties.push_back( { { {}, wxS( "font" ), STRING_ENCODING_STATUS::UTF8, source },
                                             { {}, wxS( "PADS stroke" ), STRING_ENCODING_STATUS::UTF8, source },
                                             PROPERTY_DISPOSITION::APPROXIMATE,
                                             source } );
    binary.sheets[0].properties.push_back( { { {}, wxS( "color" ), STRING_ENCODING_STATUS::UTF8, source },
                                             { {}, wxS( "green" ), STRING_ENCODING_STATUS::UTF8, source },
                                             PROPERTY_DISPOSITION::APPROXIMATE,
                                             source } );
    binary.sheets[1].properties.push_back( { { {}, wxS( "value" ), STRING_ENCODING_STATUS::UTF8, source },
                                             { {}, wxS( "22k" ), STRING_ENCODING_STATUS::UTF8, source },
                                             PROPERTY_DISPOSITION::EXACT,
                                             source } );

    std::vector<ASCII_SNAPSHOT_SHEET> ascii{ { "Main", { { "value", "22k" } } },
                                             { "Main",
                                               { { "value", "10k" },
                                                 { "font", "Arial", PROPERTY_DISPOSITION::APPROXIMATE },
                                                 { "color", "red", PROPERTY_DISPOSITION::APPROXIMATE } } } };
    std::vector<SNAPSHOT_ITEM>        expected = normalizeBinaryModel( binary );
    std::vector<SNAPSHOT_ITEM>        actual = normalizeAsciiModel( ascii );
    BOOST_CHECK( !snapshotsMatch( expected, actual ) );
    BOOST_CHECK( !snapshotsMatch( expected, actual, { { "font", PROPERTY_DISPOSITION::APPROXIMATE } } ) );
    BOOST_CHECK( snapshotsMatch(
            expected, actual,
            { { "font", PROPERTY_DISPOSITION::APPROXIMATE }, { "color", PROPERTY_DISPOSITION::APPROXIMATE } } ) );

    ascii.pop_back();
    BOOST_CHECK( !snapshotsMatch(
            expected, normalizeAsciiModel( ascii ),
            { { "font", PROPERTY_DISPOSITION::APPROXIMATE }, { "color", PROPERTY_DISPOSITION::APPROXIMATE } } ) );
}


BOOST_AUTO_TEST_SUITE_END()
