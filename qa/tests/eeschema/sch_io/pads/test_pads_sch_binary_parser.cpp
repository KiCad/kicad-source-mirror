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
#include <sch_io/pads/pads_sch_parser.h>

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


static std::vector<SNAPSHOT_PROPERTY> normalizeProperties( const std::vector<SOURCE_PROPERTY>& aProperties )
{
    std::vector<SNAPSHOT_PROPERTY> result;

    for( const SOURCE_PROPERTY& property : aProperties )
    {
        result.push_back(
                { property.name.text.ToStdString(), property.value.text.ToStdString(), property.disposition } );
    }

    return result;
}


static std::vector<SNAPSHOT_ITEM> normalizeBinaryModel( const PADS_SCH_MODEL& aModel )
{
    std::vector<SNAPSHOT_ITEM> result;
    auto                       add = [&]( const std::string& aKind, const std::string& aName, int aSheet,
                    const std::vector<SOURCE_PROPERTY>& aProperties = {}, const std::string& aGeometry = {} )
    {
        result.push_back(
                { std::to_string( aSheet ), aKind, { { "name", aName, PROPERTY_DISPOSITION::EXACT } }, aGeometry } );
        std::vector<SNAPSHOT_PROPERTY> properties = normalizeProperties( aProperties );
        result.back().properties.insert( result.back().properties.end(), properties.begin(), properties.end() );
    };

    auto addField = [&]( const MODEL_FIELD& aField, int aSheet )
    {
        add( "field", aField.name.text.ToStdString(), aSheet, aField.properties, aField.value.text.ToStdString() );
    };

    add( "settings", {}, -1, aModel.settings.properties, std::to_string( aModel.settings.coordinateUnitsPerMil ) );

    for( const MODEL_SHEET& sheet : aModel.sheets )
    {
        add( "sheet", sheet.name.text.ToStdString(), static_cast<int>( sheet.index ), sheet.properties );

        for( const MODEL_GRAPHIC& graphic : sheet.border )
            add( "graphic", {}, static_cast<int>( sheet.index ), graphic.properties );

        for( const MODEL_FIELD& field : sheet.titleBlockFields )
            addField( field, static_cast<int>( sheet.index ) );
    }

    for( const MODEL_SYMBOL_DEFINITION& definition : aModel.definitions )
    {
        add( "definition", definition.name.text.ToStdString(), definition.source.sheet, definition.properties );

        for( const MODEL_GRAPHIC& graphic : definition.graphics )
            add( "graphic", {}, definition.source.sheet, graphic.properties );

        for( const MODEL_PIN_DEFINITION& pin : definition.pins )
            add( "pin", pin.number.text.ToStdString(), definition.source.sheet, pin.properties );

        for( const MODEL_FIELD& field : definition.fields )
            addField( field, definition.source.sheet );
    }

    for( const MODEL_PART_TYPE& partType : aModel.partTypes )
    {
        add( "part_type", partType.name.text.ToStdString(), partType.source.sheet, partType.properties );

        for( const MODEL_GATE& gate : partType.gates )
            add( "gate", std::to_string( gate.unit ), gate.source.sheet, gate.properties );

        for( const MODEL_FIELD& field : partType.fields )
            addField( field, partType.source.sheet );
    }

    for( const MODEL_PLACEMENT& placement : aModel.placements )
    {
        add( "placement", placement.reference.text.ToStdString(), placement.source.sheet, placement.properties );

        for( const MODEL_FIELD& field : placement.fields )
            addField( field, placement.source.sheet );
    }

    for( const MODEL_NET& net : aModel.nets )
    {
        add( "net", net.name.text.ToStdString(), net.source.sheet, net.properties );

        for( const MODEL_CONNECTION& connection : net.connections )
            add( "connection", {}, connection.source.sheet, connection.properties );
    }

    for( const MODEL_BUS& bus : aModel.buses )
    {
        add( "bus", bus.name.text.ToStdString(), bus.source.sheet, bus.properties );

        for( const MODEL_BUS_ENTRY& entry : bus.entries )
            add( "bus_entry", std::to_string( entry.memberNet.id.Value() ), entry.source.sheet, entry.properties );

        for( const SOURCE_STRING& alias : bus.aliases )
            add( "bus_alias", alias.text.ToStdString(), alias.source.sheet );

        for( const NET_REFERENCE& member : bus.memberNets )
            add( "bus_member", std::to_string( member.id.Value() ), member.source.sheet );
    }

    for( const MODEL_LABEL& label : aModel.labels )
        add( "label", label.text.text.ToStdString(), label.source.sheet, label.properties );

    for( const MODEL_JUNCTION& junction : aModel.junctions )
        add( "junction", {}, junction.source.sheet, junction.properties );

    for( const MODEL_TEXT& text : aModel.texts )
        add( "text", text.text.text.ToStdString(), text.source.sheet, text.properties );

    for( const MODEL_GRAPHIC& graphic : aModel.graphics )
        add( "graphic", {}, graphic.source.sheet, graphic.properties );

    return result;
}


static std::vector<SNAPSHOT_ITEM> normalizeAsciiModel( const PADS_SCH::PADS_SCH_PARSER& aParser )
{
    std::vector<SNAPSHOT_ITEM> result;
    auto add = [&]( const std::string& aKind, const std::string& aName, int aSheet, const std::string& aGeometry = {} )
    {
        result.push_back(
                { std::to_string( aSheet ), aKind, { { "name", aName, PROPERTY_DISPOSITION::EXACT } }, aGeometry } );
    };

    const PADS_SCH::PARAMETERS& parameters = aParser.GetParameters();
    add( "settings", {}, -1, std::to_string( parameters.line_width ) );

    if( aParser.GetSheetHeaders().empty() )
    {
        for( int sheet : aParser.GetSheetNumbers() )
            add( "sheet", {}, sheet - 1 );
    }
    else
    {
        for( const PADS_SCH::SHEET_HEADER& sheet : aParser.GetSheetHeaders() )
            add( "sheet", sheet.sheet_name, sheet.sheet_num - 1 );
    }

    for( const PADS_SCH::SYMBOL_DEF& definition : aParser.GetSymbolDefs() )
    {
        add( "definition", definition.name, -1 );

        for( const PADS_SCH::SYMBOL_GRAPHIC& graphic : definition.graphics )
            add( "graphic", {}, -1, std::to_string( graphic.points.size() ) );

        for( const PADS_SCH::SYMBOL_PIN& pin : definition.pins )
            add( "pin", pin.number, -1 );

        for( const PADS_SCH::SYMBOL_TEXT& text : definition.texts )
            add( "field", {}, -1, text.content );
    }

    for( const auto& [name, partType] : aParser.GetPartTypes() )
    {
        add( "part_type", name, -1 );

        for( size_t gate = 0; gate < partType.gates.size(); ++gate )
        {
            add( "gate", std::to_string( gate + 1 ), -1 );

            for( const PADS_SCH::PARTTYPE_PIN& pin : partType.gates[gate].pins )
                add( "pin", pin.pin_id, -1 );
        }
    }

    for( const PADS_SCH::PART_PLACEMENT& placement : aParser.GetPartPlacements() )
    {
        add( "placement", placement.reference, placement.sheet_number - 1 );

        for( const PADS_SCH::PART_ATTRIBUTE& field : placement.attributes )
            add( "field", field.name, placement.sheet_number - 1, field.value );
    }

    for( const PADS_SCH::SCH_SIGNAL& net : aParser.GetSignals() )
    {
        add( "net", net.name, -1 );

        for( const PADS_SCH::WIRE_SEGMENT& connection : net.wires )
            add( "connection", {}, connection.sheet_number - 1, std::to_string( connection.vertices.size() ) );

        for( const PADS_SCH::PIN_CONNECTION& connection : net.connections )
            add( "connection", connection.reference + "." + connection.pin_number, connection.sheet_number - 1 );
    }

    for( const PADS_SCH::OFF_PAGE_CONNECTOR& label : aParser.GetOffPageConnectors() )
        add( "label", label.signal_name, label.source_sheet - 1 );

    for( const PADS_SCH::NETNAME_LABEL& label : aParser.GetNetNameLabels() )
        add( "label", label.net_name, -1 );

    for( const PADS_SCH::TIED_DOT& junction : aParser.GetTiedDots() )
        add( "junction", {}, junction.sheet_number - 1 );

    for( const PADS_SCH::TEXT_ITEM& text : aParser.GetTextItems() )
        add( "text", text.content, text.sheet_number - 1 );

    for( const PADS_SCH::LINES_ITEM& lines : aParser.GetLinesItems() )
    {
        for( const PADS_SCH::SYMBOL_GRAPHIC& graphic : lines.primitives )
            add( "graphic", lines.name, lines.sheet_number - 1, std::to_string( graphic.points.size() ) );

        for( const PADS_SCH::TEXT_ITEM& text : lines.texts )
            add( "text", text.content, lines.sheet_number - 1 );
    }

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


BOOST_AUTO_TEST_CASE( TypedIdDiagnosticProvenance )
{
    SOURCE_PROVENANCE firstSource{ wxS( "ids.sch" ), 0x000D, wxS( "sheet" ), 3, 1, 0x100, 48, 0 };
    SOURCE_PROVENANCE secondSource{ wxS( "ids.sch" ), 0x000D, wxS( "sheet" ), 3, 2, 0x200, 48, 1 };
    PADS_SCH_MODEL    duplicate;
    duplicate.source = { wxS( "ids.sch" ), 0x000D, wxS( "model" ), -1, 0, 0, 0x300, -1 };
    duplicate.sheets.push_back( { SHEET_ID( 7 ), 0, firstSource } );
    duplicate.sheets.push_back( { SHEET_ID( 7 ), 1, secondSource } );

    const wxString duplicateDetail =
            wxS( "duplicate sheet ID 7; first at v0x000D sheet controller 3 record 1 sheet 0 offset 0x100" );
    const wxString duplicateError = FormatParserError( secondSource, duplicateDetail );
    BOOST_CHECK_EXCEPTION( duplicate.ValidateOrThrow(), IO_ERROR,
                           [&]( const IO_ERROR& aError )
                           {
                               return aError.What().Contains( duplicateError );
                           } );

    PADS_SCH_MODEL invalid;
    invalid.source = duplicate.source;
    invalid.sheets.push_back( { SHEET_ID(), 0, secondSource } );
    const wxString invalidError = FormatParserError( secondSource, wxS( "invalid sheet ID" ) );
    BOOST_CHECK_EXCEPTION( invalid.ValidateOrThrow(), IO_ERROR,
                           [&]( const IO_ERROR& aError )
                           {
                               return aError.What().Contains( invalidError );
                           } );
}


BOOST_AUTO_TEST_CASE( SourceStringEncoding )
{
    SOURCE_PROVENANCE              source{ wxS( "encoding.sch" ), 0x000D, wxS( "text" ), 4, 8, 0x300, 4, 0 };
    std::vector<PARSER_DIAGNOSTIC> diagnostics;

    SOURCE_STRING utf8 = PADS_SCH_BINARY_PARSER::DecodeString( { 0x41, 0xC3, 0xA9 }, 65001, source, diagnostics );
    BOOST_CHECK_EQUAL( utf8.text, wxString::FromUTF8( "A\xC3\xA9" ) );
    BOOST_CHECK_EQUAL( utf8.codePage, 65001 );
    BOOST_CHECK_EQUAL( utf8.codePageName, wxS( "UTF-8" ) );
    BOOST_CHECK( utf8.encoding == STRING_ENCODING_STATUS::UTF8 );
    BOOST_CHECK( utf8.source == source );

    SOURCE_STRING cp1252 =
            PADS_SCH_BINARY_PARSER::DecodeString( { 0x80, 0x93, 0xE9 }, 1252, source, diagnostics, wxS( "CP1252" ) );
    BOOST_REQUIRE_EQUAL( cp1252.text.length(), 3 );
    BOOST_CHECK_EQUAL( cp1252.text[0].GetValue(), 0x20AC );
    BOOST_CHECK_EQUAL( cp1252.text[1].GetValue(), 0x201C );
    BOOST_CHECK_EQUAL( cp1252.text[2].GetValue(), 0x00E9 );
    BOOST_CHECK_EQUAL( cp1252.codePage, 1252 );
    BOOST_CHECK_EQUAL( cp1252.codePageName, wxS( "CP1252" ) );
    BOOST_CHECK( cp1252.encoding == STRING_ENCODING_STATUS::CODE_PAGE );

    SOURCE_STRING unknown =
            PADS_SCH_BINARY_PARSER::DecodeString( { 0x41, 0xFE }, 99999, source, diagnostics, wxS( "vendor-foo" ) );
    BOOST_CHECK_EQUAL( unknown.raw[1], 0xFE );
    BOOST_REQUIRE_EQUAL( unknown.text.length(), 2 );
    BOOST_CHECK_EQUAL( unknown.text[0].GetValue(), 0x41 );
    BOOST_CHECK_EQUAL( unknown.text[1].GetValue(), 0xFFFD );
    BOOST_CHECK_EQUAL( unknown.codePage, 99999 );
    BOOST_CHECK_EQUAL( unknown.codePageName, wxS( "vendor-foo" ) );
    BOOST_CHECK( unknown.encoding == STRING_ENCODING_STATUS::UNKNOWN_CODE_PAGE );

    SOURCE_STRING invalidUtf8 =
            PADS_SCH_BINARY_PARSER::DecodeString( { 0x41, 0xC3, 0x28, 0xFF }, 65001, source, diagnostics );
    BOOST_REQUIRE_EQUAL( invalidUtf8.text.length(), 4 );
    BOOST_CHECK_EQUAL( invalidUtf8.text[0].GetValue(), 0x41 );
    BOOST_CHECK_EQUAL( invalidUtf8.text[1].GetValue(), 0xFFFD );
    BOOST_CHECK_EQUAL( invalidUtf8.text[2].GetValue(), 0x28 );
    BOOST_CHECK_EQUAL( invalidUtf8.text[3].GetValue(), 0xFFFD );
    BOOST_CHECK( invalidUtf8.encoding == STRING_ENCODING_STATUS::INVALID_BYTES );
    BOOST_REQUIRE_EQUAL( diagnostics.size(), 2 );
    BOOST_CHECK_EQUAL( diagnostics[0].source.absoluteOffset, 0x300 );
    BOOST_CHECK( diagnostics[0].message.Contains( wxS( "99999" ) ) );
    BOOST_CHECK_EQUAL( diagnostics[1].source.recordIndex, 8 );
    BOOST_CHECK( diagnostics[1].message.Contains( wxS( "UTF-8" ) ) );
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

    std::vector<SNAPSHOT_ITEM> expected = normalizeBinaryModel( binary );
    std::vector<SNAPSHOT_ITEM> actual = expected;
    std::reverse( actual.begin(), actual.end() );

    for( SNAPSHOT_ITEM& item : actual )
    {
        for( SNAPSHOT_PROPERTY& property : item.properties )
        {
            if( property.name == "font" )
                property.value = "Arial";
            else if( property.name == "color" )
                property.value = "red";
        }
    }

    BOOST_CHECK( !snapshotsMatch( expected, actual ) );
    BOOST_CHECK( !snapshotsMatch( expected, actual, { { "font", PROPERTY_DISPOSITION::APPROXIMATE } } ) );
    BOOST_CHECK( snapshotsMatch(
            expected, actual,
            { { "font", PROPERTY_DISPOSITION::APPROXIMATE }, { "color", PROPERTY_DISPOSITION::APPROXIMATE } } ) );

    actual.pop_back();
    BOOST_CHECK( !snapshotsMatch(
            expected, actual,
            { { "font", PROPERTY_DISPOSITION::APPROXIMATE }, { "color", PROPERTY_DISPOSITION::APPROXIMATE } } ) );
}


BOOST_AUTO_TEST_CASE( SnapshotAdaptersCoverSemanticVocabulary )
{
    SOURCE_PROVENANCE source{ wxS( "snapshot.sch" ), 0x000D, wxS( "synthetic" ), 1, 0, 0x250, 16, 0 };
    auto              string = [&]( const wxString& aText )
    {
        SOURCE_STRING result;
        result.text = aText;
        result.source = source;
        return result;
    };

    PADS_SCH_MODEL model;
    model.settings.source = source;
    model.sheets.push_back( { SHEET_ID( 1 ), 0, source, string( wxS( "Main" ) ) } );

    MODEL_SYMBOL_DEFINITION definition;
    definition.id = DEFINITION_ID( 2 );
    definition.source = source;
    definition.name = string( wxS( "SYM" ) );
    definition.graphics.push_back( { source } );
    definition.pins.push_back( { PIN_ID( 3 ), source, string( wxS( "1" ) ) } );
    definition.fields.push_back( { source, string( wxS( "Value" ) ), string( wxS( "R" ) ) } );
    model.definitions.push_back( definition );

    MODEL_PART_TYPE partType;
    partType.id = PART_TYPE_ID( 4 );
    partType.source = source;
    partType.name = string( wxS( "RES" ) );
    partType.gates.push_back(
            { GATE_ID( 5 ), source, { DEFINITION_ID( 2 ), source }, 1, { { PIN_ID( 3 ), source } } } );
    partType.fields.push_back( { source, string( wxS( "Tolerance" ) ), string( wxS( "1%" ) ) } );
    model.partTypes.push_back( partType );

    MODEL_PLACEMENT placement;
    placement.id = PLACEMENT_ID( 6 );
    placement.source = source;
    placement.reference = string( wxS( "R1" ) );
    placement.fields.push_back( { source, string( wxS( "Value" ) ), string( wxS( "10k" ) ) } );
    model.placements.push_back( placement );

    MODEL_NET net;
    net.id = NET_ID( 7 );
    net.source = source;
    net.name = string( wxS( "N" ) );
    net.connections.push_back( { source } );
    model.nets.push_back( net );

    MODEL_BUS bus;
    bus.id = BUS_ID( 8 );
    bus.source = source;
    bus.name = string( wxS( "D[0..0]" ) );
    bus.entries.push_back( { source, {}, { NET_ID( 7 ), source } } );
    bus.aliases.push_back( string( wxS( "DATA" ) ) );
    bus.memberNets.push_back( { NET_ID( 7 ), source } );
    model.buses.push_back( bus );
    model.labels.push_back( { source, {}, MODEL_LABEL_KIND::LOCAL, string( wxS( "N" ) ) } );
    model.junctions.push_back( { source } );
    model.texts.push_back( { source, {}, string( wxS( "note" ) ) } );
    model.graphics.push_back( { source } );

    std::vector<SNAPSHOT_ITEM> binaryRecords = normalizeBinaryModel( model );
    std::set<std::string>      binaryKinds;

    for( const SNAPSHOT_ITEM& item : binaryRecords )
        binaryKinds.insert( item.kind );

    const std::set<std::string> expectedKinds{
        "settings",  "sheet",     "definition", "graphic", "pin",        "field",
        "part_type", "gate",      "placement",  "net",     "connection", "bus",
        "bus_entry", "bus_alias", "bus_member", "label",   "junction",   "text"
    };
    BOOST_CHECK_EQUAL_COLLECTIONS( binaryKinds.begin(), binaryKinds.end(), expectedKinds.begin(), expectedKinds.end() );

    const std::string         fixtureRoot = KI_TEST::GetEeschemaTestDataDir() + "/plugins/pads/binary/";
    PADS_SCH::PADS_SCH_PARSER asciiParser;
    BOOST_REQUIRE( asciiParser.Parse( fixtureRoot + "minimal_v13.txt" ) );
    std::vector<SNAPSHOT_ITEM> asciiRecords = normalizeAsciiModel( asciiParser );
    BOOST_CHECK( std::ranges::any_of( asciiRecords,
                                      []( const SNAPSHOT_ITEM& aItem )
                                      {
                                          return aItem.kind == "settings";
                                      } ) );
    BOOST_CHECK( std::ranges::any_of( asciiRecords,
                                      []( const SNAPSHOT_ITEM& aItem )
                                      {
                                          return aItem.kind == "sheet";
                                      } ) );

    PADS_SCH_BINARY_PARSER     binaryParser;
    std::vector<SNAPSHOT_ITEM> minimalBinary =
            normalizeBinaryModel( binaryParser.Parse( loadMinimalV13(), wxS( "minimal_v13.sch" ) ) );
    auto countKind = []( const std::vector<SNAPSHOT_ITEM>& aItems, const std::string& aKind )
    {
        return std::ranges::count_if( aItems,
                                      [&]( const SNAPSHOT_ITEM& aItem )
                                      {
                                          return aItem.kind == aKind;
                                      } );
    };
    BOOST_CHECK_EQUAL( countKind( minimalBinary, "settings" ), countKind( asciiRecords, "settings" ) );
    BOOST_CHECK_EQUAL( countKind( minimalBinary, "sheet" ), countKind( asciiRecords, "sheet" ) );
}


BOOST_AUTO_TEST_SUITE_END()
