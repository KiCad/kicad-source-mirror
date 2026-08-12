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
#include <sch_io/pads/pads_sch_sdb.h>
#include <sch_io/pads/pads_sch_parser.h>

#include <ki_exception.h>
#include <algorithm>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <limits>
#include <map>
#include <optional>
#include <set>
#include <sstream>
#include <string>
#include <type_traits>
#include <variant>
#include <vector>
#include <wx/filename.h>

using namespace PADS_SCH_BINARY;

namespace
{

static std::vector<uint8_t> loadMinimalV13()
{
    std::ifstream file( KI_TEST::GetEeschemaTestDataDir() + "/plugins/pads/binary/minimal_v13.sch", std::ios::binary );
    BOOST_REQUIRE( file );
    return { std::istreambuf_iterator<char>( file ), std::istreambuf_iterator<char>() };
}


static std::vector<uint8_t> loadBinaryFixture( const std::string& aName )
{
    std::ifstream file( KI_TEST::GetEeschemaTestDataDir() + "/plugins/pads/binary/" + aName, std::ios::binary );
    BOOST_REQUIRE( file );
    return { std::istreambuf_iterator<char>( file ), std::istreambuf_iterator<char>() };
}


static uint32_t readU32( const std::vector<uint8_t>& aBytes, size_t aOffset )
{
    return static_cast<uint32_t>( aBytes[aOffset] ) | ( static_cast<uint32_t>( aBytes[aOffset + 1] ) << 8 )
           | ( static_cast<uint32_t>( aBytes[aOffset + 2] ) << 16 )
           | ( static_cast<uint32_t>( aBytes[aOffset + 3] ) << 24 );
}


static uint16_t readU16( const std::vector<uint8_t>& aBytes, size_t aOffset )
{
    return static_cast<uint16_t>( aBytes[aOffset] ) | ( static_cast<uint16_t>( aBytes[aOffset + 1] ) << 8 );
}


static void writeU16( std::vector<uint8_t>& aBytes, size_t aOffset, uint16_t aValue )
{
    aBytes[aOffset] = static_cast<uint8_t>( aValue );
    aBytes[aOffset + 1] = static_cast<uint8_t>( aValue >> 8 );
}


static void writeU32( std::vector<uint8_t>& aBytes, size_t aOffset, uint32_t aValue )
{
    writeU16( aBytes, aOffset, static_cast<uint16_t>( aValue ) );
    writeU16( aBytes, aOffset + 2, static_cast<uint16_t>( aValue >> 16 ) );
}


static size_t outerControllerOffset( const std::vector<uint8_t>& aBytes, size_t aController )
{
    size_t offset = 0x254;

    for( size_t controller = 1; controller < aController; ++controller )
        offset += readU32( aBytes, 0x20 + controller * 28 + 12 );

    return offset;
}


static size_t sheetControllerOffset( const std::vector<uint8_t>& aBytes, size_t aController )
{
    const size_t sheetOffset = readU32( aBytes, outerControllerOffset( aBytes, 3 ) );
    size_t       offset = sheetOffset + 20 + 24 * 28;

    for( size_t controller = 1; controller < aController; ++controller )
        offset += readU32( aBytes, sheetOffset + 20 + ( controller - 1 ) * 28 + 16 );

    return offset;
}


static uint32_t sheetControllerCount( const std::vector<uint8_t>& aBytes, size_t aController )
{
    const size_t sheetOffset = readU32( aBytes, outerControllerOffset( aBytes, 3 ) );
    return readU32( aBytes, sheetOffset + 20 + ( aController - 1 ) * 28 + 12 );
}


template <typename Item>
static const Item& itemNamed( const std::vector<Item>& aItems, const wxString& aName )
{
    auto item = std::ranges::find_if( aItems,
                                      [&]( const Item& aItem )
                                      {
                                          return aItem.name.text == aName;
                                      } );
    BOOST_REQUIRE( item != aItems.end() );
    return *item;
}


static wxString propertyValue( const std::vector<SOURCE_PROPERTY>& aProperties, const wxString& aName )
{
    auto property = std::ranges::find_if( aProperties,
                                          [&]( const SOURCE_PROPERTY& aProperty )
                                          {
                                              return aProperty.name.text == aName;
                                          } );
    BOOST_REQUIRE( property != aProperties.end() );
    return property->value.text;
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


enum class CANONICAL_KIND
{
    SETTINGS,
    SHEET,
    DEFINITION,
    GRAPHIC,
    PIN,
    FIELD,
    PART_TYPE,
    GATE,
    GATE_PIN_MAPPING,
    PLACEMENT,
    NET,
    CONNECTION,
    BUS,
    BUS_ENTRY,
    BUS_ALIAS,
    BUS_MEMBER,
    LABEL,
    JUNCTION,
    TEXT
};

using CANONICAL_VALUE = std::variant<int64_t, bool, std::string, std::vector<std::string>>;

struct CANONICAL_PROPERTY
{
    CANONICAL_VALUE      value;
    PROPERTY_DISPOSITION disposition = PROPERTY_DISPOSITION::EXACT;
    bool                 operator==( const CANONICAL_PROPERTY& ) const = default;
    bool                 operator<( const CANONICAL_PROPERTY& aOther ) const
    {
        return std::tie( value, disposition ) < std::tie( aOther.value, aOther.disposition );
    }
};

struct CANONICAL_POINT
{
    int64_t xHalfMils = 0;
    int64_t yHalfMils = 0;
    bool    operator==( const CANONICAL_POINT& ) const = default;
    bool    operator<( const CANONICAL_POINT& aOther ) const
    {
        return std::tie( xHalfMils, yHalfMils ) < std::tie( aOther.xHalfMils, aOther.yHalfMils );
    }
};

struct CANONICAL_GEOMETRY
{
    std::vector<CANONICAL_POINT> points;
    std::optional<int64_t>       angleTenths;
    bool                         operator==( const CANONICAL_GEOMETRY& ) const = default;
};

struct CANONICAL_SEMANTIC_RECORD
{
    int                                       sheet = -1;
    CANONICAL_KIND                            kind = CANONICAL_KIND::SETTINGS;
    std::string                               stableKey;
    std::map<std::string, CANONICAL_PROPERTY> properties;
    CANONICAL_GEOMETRY                        geometry;
    bool                                      operator==( const CANONICAL_SEMANTIC_RECORD& ) const = default;
    bool                                      operator<( const CANONICAL_SEMANTIC_RECORD& aOther ) const
    {
        return std::tie( sheet, kind, stableKey, properties, geometry.points, geometry.angleTenths )
               < std::tie( aOther.sheet, aOther.kind, aOther.stableKey, aOther.properties, aOther.geometry.points,
                           aOther.geometry.angleTenths );
    }
};

using SNAPSHOT_ALLOWLIST = std::set<std::pair<std::string, PROPERTY_DISPOSITION>>;

static CANONICAL_POINT point( const SOURCE_POINT& aPoint )
{
    return { aPoint.x, aPoint.y };
}
static CANONICAL_POINT point( const PADS_SCH::POINT& aPoint )
{
    return { static_cast<int64_t>( std::llround( aPoint.x * 2.0 ) ),
             static_cast<int64_t>( std::llround( aPoint.y * 2.0 ) ) };
}

static void addSourceProperties( CANONICAL_SEMANTIC_RECORD& aRecord, const std::vector<SOURCE_PROPERTY>& aProperties,
                                 const std::string& aPrefix = {} )
{
    for( const SOURCE_PROPERTY& property : aProperties )
        aRecord.properties[aPrefix + property.name.text.ToStdString()] = { property.value.text.ToStdString(),
                                                                           property.disposition };
}

static CANONICAL_PROPERTY unknownEnum( int64_t aValue )
{
    return { "unknown:" + std::to_string( aValue ), PROPERTY_DISPOSITION::UNSUPPORTED };
}

static CANONICAL_PROPERTY canonicalGraphicType( MODEL_GRAPHIC_KIND aKind )
{
    switch( aKind )
    {
    case MODEL_GRAPHIC_KIND::LINE: return { "line" };
    case MODEL_GRAPHIC_KIND::POLYLINE: return { "polyline" };
    case MODEL_GRAPHIC_KIND::RECTANGLE: return { "rectangle" };
    case MODEL_GRAPHIC_KIND::CIRCLE: return { "circle" };
    case MODEL_GRAPHIC_KIND::ARC: return { "arc" };
    case MODEL_GRAPHIC_KIND::TEXT: return { "text" };
    }

    return unknownEnum( static_cast<int64_t>( aKind ) );
}

static CANONICAL_PROPERTY canonicalGraphicType( PADS_SCH::GRAPHIC_TYPE aKind )
{
    switch( aKind )
    {
    case PADS_SCH::GRAPHIC_TYPE::LINE: return { "line" };
    case PADS_SCH::GRAPHIC_TYPE::POLYLINE: return { "polyline" };
    case PADS_SCH::GRAPHIC_TYPE::RECTANGLE: return { "rectangle" };
    case PADS_SCH::GRAPHIC_TYPE::CIRCLE: return { "circle" };
    case PADS_SCH::GRAPHIC_TYPE::ARC: return { "arc" };
    }

    return unknownEnum( static_cast<int64_t>( aKind ) );
}

static CANONICAL_PROPERTY canonicalGraphicType( const PADS_SCH::SYMBOL_GRAPHIC& aGraphic )
{
    if( std::ranges::any_of( aGraphic.points,
                             []( const PADS_SCH::GRAPHIC_POINT& aPoint )
                             {
                                 return aPoint.arc.has_value();
                             } ) )
    {
        return { "arc" };
    }

    if( aGraphic.type == PADS_SCH::GRAPHIC_TYPE::POLYLINE && aGraphic.points.size() == 2 )
        return { "line" };

    return canonicalGraphicType( aGraphic.type );
}

static CANONICAL_PROPERTY canonicalLineStyle( MODEL_LINE_STYLE aStyle )
{
    switch( aStyle )
    {
    case MODEL_LINE_STYLE::DEFAULT:
    case MODEL_LINE_STYLE::SOLID: return { "solid" };
    case MODEL_LINE_STYLE::DASH: return { "dash" };
    case MODEL_LINE_STYLE::DOT: return { "dot" };
    case MODEL_LINE_STYLE::DASH_DOT: return { "dash_dot" };
    }

    return unknownEnum( static_cast<int64_t>( aStyle ) );
}

static CANONICAL_PROPERTY canonicalLineStyle( int aStyle )
{
    switch( static_cast<int8_t>( aStyle & 0xFF ) )
    {
    case 1:
    case -1: return { "solid" };
    case 0:
    case -2: return { "dash" };
    case -3: return { "dot" };
    case -4: return { "dash_dot" };
    case -5: return { "dash_dot_dot" };
    default: return unknownEnum( aStyle );
    }
}

static CANONICAL_PROPERTY canonicalFill( MODEL_FILL_STYLE aFill )
{
    switch( aFill )
    {
    case MODEL_FILL_STYLE::NONE: return { "none" };
    case MODEL_FILL_STYLE::FILLED: return { "filled" };
    case MODEL_FILL_STYLE::HATCHED: return { "hatched" };
    }

    return unknownEnum( static_cast<int64_t>( aFill ) );
}

static CANONICAL_PROPERTY canonicalFill( bool aFilled )
{
    return { aFilled ? "filled" : "none" };
}

static CANONICAL_PROPERTY canonicalPinType( uint32_t aType )
{
    switch( aType )
    {
    case 0: return { "passive" };
    case 1: return { "input" };
    case 2: return { "output" };
    case 3: return { "bidirectional" };
    case 4: return { "tristate" };
    case 5: return { "open_collector" };
    case 6: return { "open_emitter" };
    case 7: return { "power" };
    case 8: return { "unspecified" };
    default: return unknownEnum( aType );
    }
}

static CANONICAL_PROPERTY canonicalPinType( PADS_SCH::PIN_TYPE aType )
{
    switch( aType )
    {
    case PADS_SCH::PIN_TYPE::PASSIVE: return { "passive" };
    case PADS_SCH::PIN_TYPE::INPUT: return { "input" };
    case PADS_SCH::PIN_TYPE::OUTPUT: return { "output" };
    case PADS_SCH::PIN_TYPE::BIDIRECTIONAL: return { "bidirectional" };
    case PADS_SCH::PIN_TYPE::TRISTATE: return { "tristate" };
    case PADS_SCH::PIN_TYPE::OPEN_COLLECTOR: return { "open_collector" };
    case PADS_SCH::PIN_TYPE::OPEN_EMITTER: return { "open_emitter" };
    case PADS_SCH::PIN_TYPE::POWER: return { "power" };
    case PADS_SCH::PIN_TYPE::UNSPECIFIED: return { "unspecified" };
    }

    return unknownEnum( static_cast<int64_t>( aType ) );
}

static CANONICAL_PROPERTY canonicalPinType( char aType )
{
    switch( aType )
    {
    case 'U': return { "passive" };
    case 'L': return { "input" };
    case 'S': return { "output" };
    case 'B': return { "bidirectional" };
    case 'T': return { "tristate" };
    case 'C': return { "open_collector" };
    case 'E': return { "open_emitter" };
    case 'P':
    case 'G': return { "power" };
    default: return { "unspecified" };
    }
}

static CANONICAL_PROPERTY canonicalPinStyle( uint32_t aStyle )
{
    switch( aStyle )
    {
    case 0: return { "normal" };
    case 1: return { "inverted" };
    case 2: return { "clock" };
    case 3: return { "inverted_clock" };
    default: return unknownEnum( aStyle );
    }
}

static CANONICAL_PROPERTY canonicalPinStyle( bool aInverted, bool aClock )
{
    return canonicalPinStyle( ( aInverted ? 1U : 0U ) | ( aClock ? 2U : 0U ) );
}

static CANONICAL_PROPERTY canonicalJustification( MODEL_JUSTIFICATION aJustification )
{
    switch( aJustification )
    {
    case MODEL_JUSTIFICATION::LEFT: return { "left" };
    case MODEL_JUSTIFICATION::CENTER: return { "center" };
    case MODEL_JUSTIFICATION::RIGHT: return { "right" };
    }

    return unknownEnum( static_cast<int64_t>( aJustification ) );
}

static CANONICAL_PROPERTY canonicalVerticalJustification( MODEL_JUSTIFICATION aJustification )
{
    switch( aJustification )
    {
    case MODEL_JUSTIFICATION::LEFT: return { "top" };
    case MODEL_JUSTIFICATION::CENTER: return { "center" };
    case MODEL_JUSTIFICATION::RIGHT: return { "bottom" };
    }

    return unknownEnum( static_cast<int64_t>( aJustification ) );
}

static CANONICAL_PROPERTY canonicalHorizontalJustification( int aJustification )
{
    const int justification = aJustification & 0xFF;
    const int horizontal = justification >= 8   ? justification - 8
                           : justification >= 2 ? justification - 2
                                                : justification;

    switch( horizontal )
    {
    default:
    case 0: return { "left" };
    case 1: return { "right" };
    case 4: return { "center" };
    }
}

static CANONICAL_PROPERTY canonicalVerticalJustification( int aJustification )
{
    if( aJustification >= 8 )
        return { "center" };

    if( aJustification >= 2 )
        return { "top" };

    if( aJustification >= 0 )
        return { "bottom" };

    return unknownEnum( aJustification );
}

static CANONICAL_PROPERTY canonicalLabelKind( MODEL_LABEL_KIND aKind )
{
    switch( aKind )
    {
    case MODEL_LABEL_KIND::LOCAL: return { "local" };
    case MODEL_LABEL_KIND::GLOBAL: return { "global" };
    case MODEL_LABEL_KIND::HIERARCHICAL: return { "hierarchical" };
    case MODEL_LABEL_KIND::GROUND: return { "ground" };
    case MODEL_LABEL_KIND::POWER: return { "power" };
    case MODEL_LABEL_KIND::BUS: return { "bus" };
    case MODEL_LABEL_KIND::UNSUPPORTED: return unknownEnum( static_cast<int64_t>( aKind ) );
    }

    return unknownEnum( static_cast<int64_t>( aKind ) );
}

static CANONICAL_PROPERTY canonicalEndpointKind( MODEL_ENDPOINT_KIND aKind )
{
    switch( aKind )
    {
    case MODEL_ENDPOINT_KIND::INVALID: return { "invalid", PROPERTY_DISPOSITION::UNSUPPORTED };
    case MODEL_ENDPOINT_KIND::POINT: return { "point" };
    case MODEL_ENDPOINT_KIND::PIN: return { "pin" };
    }

    return unknownEnum( static_cast<int64_t>( aKind ) );
}

static int64_t canonicalAngle( int64_t aAngleTenths )
{
    return NormalizeAngle( static_cast<int>( aAngleTenths ) );
}

static std::vector<CANONICAL_SEMANTIC_RECORD> normalizeBinaryModel( const PADS_SCH_MODEL& aModel )
{
    std::vector<CANONICAL_SEMANTIC_RECORD> out;
    auto add = [&]( CANONICAL_KIND aKind, int aSheet, std::string aKey = {} ) -> CANONICAL_SEMANTIC_RECORD&
    {
        out.push_back( { aSheet, aKind, std::move( aKey ) } );
        return out.back();
    };
    auto addOwned = [&]( CANONICAL_KIND aKind, const SHEET_REFERENCE& aSheet,
                         std::string aKey = {} ) -> CANONICAL_SEMANTIC_RECORD&
    {
        auto  sheet = std::ranges::find_if( aModel.sheets,
                                            [&]( const MODEL_SHEET& aCandidate )
                                            {
                                               return aCandidate.id == aSheet.id;
                                           } );
        auto& record =
                add( aKind, sheet == aModel.sheets.end() ? -1 : static_cast<int>( sheet->index ), std::move( aKey ) );
        record.properties["owner_sheet"] = sheet == aModel.sheets.end()
                                                   ? unknownEnum( aSheet.id.Value() )
                                                   : CANONICAL_PROPERTY{ sheet->name.text.ToStdString() };
        return record;
    };
    auto graphic = [&]( const MODEL_GRAPHIC& aGraphic, int aSheet )
    {
        auto& r = add( CANONICAL_KIND::GRAPHIC, aSheet );
        r.properties["type"] = canonicalGraphicType( aGraphic.kind );
        r.properties["line_style"] = canonicalLineStyle( aGraphic.lineStyle );
        r.properties["stroke_half_mils"] = { aGraphic.strokeWidth };
        r.properties["fill"] = canonicalFill( aGraphic.fill );
        r.properties["text"] = { aGraphic.text.text.ToStdString() };
        r.properties["font"] = { aGraphic.presentation.font.text.ToStdString() };
        r.properties["visible"] = { aGraphic.presentation.visible };
        r.properties["height_half_mils"] = { aGraphic.presentation.height };
        r.properties["width_half_mils"] = { aGraphic.presentation.width };

        if( aGraphic.kind == MODEL_GRAPHIC_KIND::ARC )
        {
            r.properties["arc_center_x_half_mils"] = { aGraphic.arcCenter.x };
            r.properties["arc_center_y_half_mils"] = { aGraphic.arcCenter.y };
            r.properties["arc_bounds_x1_half_mils"] = { aGraphic.arcBoundsStart.x };
            r.properties["arc_bounds_y1_half_mils"] = { aGraphic.arcBoundsStart.y };
            r.properties["arc_bounds_x2_half_mils"] = { aGraphic.arcBoundsEnd.x };
            r.properties["arc_bounds_y2_half_mils"] = { aGraphic.arcBoundsEnd.y };
            r.properties["arc_sweep_tenths"] = { int64_t( aGraphic.arcSweepAngle ) };
            r.properties["arc_clockwise"] = { aGraphic.arcClockwise };
        }

        for( const SOURCE_POINT& p : aGraphic.points )
            r.geometry.points.push_back( point( p ) );
        r.geometry.angleTenths = canonicalAngle( aGraphic.angle );
        addSourceProperties( r, aGraphic.properties );
    };
    auto field = [&]( const MODEL_FIELD& aField, int aSheet )
    {
        auto& r = add( CANONICAL_KIND::FIELD, aSheet, aField.name.text.ToStdString() );
        r.properties["value"] = { aField.value.text.ToStdString() };
        r.properties["visible"] = { aField.visible };
        r.properties["font"] = { aField.presentation.font.text.ToStdString() };
        r.geometry.points.push_back( point( aField.position ) );
        r.geometry.angleTenths = canonicalAngle( aField.angle );
        addSourceProperties( r, aField.properties );
    };
    auto& settings = add( CANONICAL_KIND::SETTINGS, -1 );
    settings.properties["coordinate_units_per_mil"] = { aModel.settings.coordinateUnitsPerMil };
    settings.properties["line_width_half_mils"] = { aModel.settings.defaultLineWidth };
    settings.properties["bus_width_half_mils"] = { aModel.settings.defaultBusWidth };
    settings.geometry.points.push_back( point( aModel.settings.pageSize ) );
    addSourceProperties( settings, aModel.settings.properties );
    for( const MODEL_SHEET& s : aModel.sheets )
    {
        auto& r = add( CANONICAL_KIND::SHEET, static_cast<int>( s.index ), s.name.text.ToStdString() );
        r.properties["title"] = { s.title.text.ToStdString() };
        r.properties["line_width_half_mils"] = { s.defaultLineWidth };
        r.properties["bus_width_half_mils"] = { s.defaultBusWidth };
        r.geometry.points.push_back( point( s.pageSize ) );
        addSourceProperties( r, s.properties );
        for( const auto& g : s.border )
            graphic( g, static_cast<int>( s.index ) );
        for( const auto& f : s.titleBlockFields )
            field( f, static_cast<int>( s.index ) );
    }
    for( const auto& d : aModel.definitions )
    {
        auto& definition = add( CANONICAL_KIND::DEFINITION, d.source.sheet, d.name.text.ToStdString() );
        addSourceProperties( definition, d.properties );
        for( const auto& g : d.graphics )
            graphic( g, d.source.sheet );
        for( const auto& p : d.pins )
        {
            auto& r = add( CANONICAL_KIND::PIN, d.source.sheet, p.number.text.ToStdString() );
            r.properties["name"] = { p.name.text.ToStdString() };
            r.properties["type"] = canonicalPinType( p.electricalType );
            r.properties["style"] = canonicalPinStyle( p.graphicStyle );
            r.properties["length_half_mils"] = { p.length };
            r.properties["font"] = { p.presentation.font.text.ToStdString() };
            r.properties["visible"] = { p.presentation.visible };
            r.geometry.points.push_back( point( p.position ) );
            r.geometry.angleTenths = canonicalAngle( p.angle );
            addSourceProperties( r, p.properties );
        }
        for( const auto& f : d.fields )
            field( f, d.source.sheet );
    }
    for( const auto& p : aModel.partTypes )
    {
        auto& part = add( CANONICAL_KIND::PART_TYPE, p.source.sheet, p.name.text.ToStdString() );
        addSourceProperties( part, p.properties );
        for( const auto& g : p.gates )
        {
            auto& r = add( CANONICAL_KIND::GATE, g.source.sheet, std::to_string( g.unit ) );
            auto  definition = std::ranges::find_if( aModel.definitions,
                                                     [&]( const MODEL_SYMBOL_DEFINITION& aDefinition )
                                                     {
                                                        return aDefinition.id == g.definition.id;
                                                    } );
            r.properties["definition"] = definition == aModel.definitions.end()
                                                 ? unknownEnum( g.definition.id.Value() )
                                                 : CANONICAL_PROPERTY{ definition->name.text.ToStdString() };
            addSourceProperties( r, g.properties );
            for( size_t i = 0; i < g.pins.size(); ++i )
            {
                auto&                       m = add( CANONICAL_KIND::GATE_PIN_MAPPING, g.source.sheet,
                                                     std::to_string( g.unit ) + ":" + std::to_string( i ) );
                const MODEL_PIN_DEFINITION* pin = nullptr;

                if( definition != aModel.definitions.end() )
                {
                    auto resolved = std::ranges::find_if( definition->pins,
                                                          [&]( const MODEL_PIN_DEFINITION& aPin )
                                                          {
                                                              return aPin.id == g.pins[i].id;
                                                          } );

                    if( resolved != definition->pins.end() )
                        pin = &*resolved;
                }

                if( pin )
                {
                    m.properties["pin_number"] = { pin->number.text.ToStdString() };
                    m.properties["pin_name"] = { pin->name.text.ToStdString() };
                }
                else
                {
                    m.properties["pin_number"] = unknownEnum( g.pins[i].id.Value() );
                    m.properties["pin_name"] = unknownEnum( g.pins[i].id.Value() );
                }
            }
        }
        for( const auto& f : p.fields )
            field( f, p.source.sheet );
    }
    for( const auto& p : aModel.placements )
    {
        auto& r = addOwned( CANONICAL_KIND::PLACEMENT, p.sheet, p.reference.text.ToStdString() );
        r.properties["unit"] = { int64_t( p.unit ) };
        r.properties["mirrored"] = { p.mirrored };
        r.geometry.points.push_back( point( p.position ) );
        r.geometry.angleTenths = canonicalAngle( p.angle );
        addSourceProperties( r, p.properties );
        int ownerSheet = r.sheet;
        for( const auto& f : p.fields )
            field( f, ownerSheet );
    }
    for( const auto& n : aModel.nets )
    {
        auto& net = addOwned( CANONICAL_KIND::NET, n.sheet, n.name.text.ToStdString() );
        addSourceProperties( net, n.properties );
        for( const auto& c : n.connections )
        {
            auto& r = addOwned( CANONICAL_KIND::CONNECTION, n.sheet );
            r.properties["endpoint_count"] = { int64_t( c.endpoints.size() ) };
            for( size_t i = 0; i < c.endpoints.size(); ++i )
            {
                const auto prefix = "endpoint_" + std::to_string( i ) + "_";
                r.properties[prefix + "kind"] = canonicalEndpointKind( c.endpoints[i].kind );
                addSourceProperties( r, c.endpoints[i].properties, prefix );

                if( c.endpoints[i].placement )
                {
                    auto placement = std::ranges::find_if( aModel.placements,
                                                           [&]( const MODEL_PLACEMENT& aPlacement )
                                                           {
                                                               return aPlacement.id == c.endpoints[i].placement->id;
                                                           } );
                    r.properties[prefix + "placement"] =
                            placement == aModel.placements.end()
                                    ? unknownEnum( c.endpoints[i].placement->id.Value() )
                                    : CANONICAL_PROPERTY{ placement->reference.text.ToStdString() };
                }

                if( c.endpoints[i].pin )
                {
                    auto definition = std::ranges::find_if( aModel.definitions,
                                                            [&]( const MODEL_SYMBOL_DEFINITION& aDefinition )
                                                            {
                                                                return std::ranges::any_of(
                                                                        aDefinition.pins,
                                                                        [&]( const MODEL_PIN_DEFINITION& aPin )
                                                                        {
                                                                            return aPin.id == c.endpoints[i].pin->id;
                                                                        } );
                                                            } );
                    r.properties[prefix + "pin_number"] =
                            definition == aModel.definitions.end()
                                    ? unknownEnum( c.endpoints[i].pin->id.Value() )
                                    : CANONICAL_PROPERTY{ std::ranges::find_if( definition->pins,
                                                                                [&]( const MODEL_PIN_DEFINITION& aPin )
                                                                                {
                                                                                    return aPin.id
                                                                                           == c.endpoints[i].pin->id;
                                                                                } )
                                                                  ->number.text.ToStdString() };
                }
            }
            for( const auto& p : c.vertices )
                r.geometry.points.push_back( point( p ) );
            addSourceProperties( r, c.properties );
        }
    }
    for( const auto& b : aModel.buses )
    {
        auto netName = [&]( NET_ID aId ) -> CANONICAL_PROPERTY
        {
            auto net = std::ranges::find_if( aModel.nets,
                                             [&]( const MODEL_NET& aNet )
                                             {
                                                 return aNet.id == aId;
                                             } );

            if( net == aModel.nets.end() )
                return unknownEnum( aId.Value() );

            return { net->name.text.ToStdString() };
        };
        auto&                    r = addOwned( CANONICAL_KIND::BUS, b.sheet, b.name.text.ToStdString() );
        std::vector<std::string> aliases;
        std::vector<std::string> members;

        for( const SOURCE_STRING& alias : b.aliases )
            aliases.push_back( alias.text.ToStdString() );

        for( const NET_REFERENCE& member : b.memberNets )
            members.push_back( std::get<std::string>( netName( member.id ).value ) );

        r.properties["aliases"] = { aliases };
        r.properties["member_nets"] = { members };
        for( const auto& p : b.vertices )
            r.geometry.points.push_back( point( p ) );
        addSourceProperties( r, b.properties );
        for( size_t i = 0; i < b.entries.size(); ++i )
        {
            auto& e = addOwned( CANONICAL_KIND::BUS_ENTRY, b.sheet, std::to_string( i ) );
            e.properties["member_index"] = { int64_t( i ) };
            e.properties["member_net"] = netName( b.entries[i].memberNet.id );
            e.geometry.points.push_back( point( b.entries[i].position ) );
            addSourceProperties( e, b.entries[i].properties );
        }
        for( const auto& a : b.aliases )
            addOwned( CANONICAL_KIND::BUS_ALIAS, b.sheet, a.text.ToStdString() );
        for( size_t i = 0; i < b.memberNets.size(); ++i )
        {
            auto& member = addOwned( CANONICAL_KIND::BUS_MEMBER, b.sheet, std::to_string( i ) );
            member.properties["member_net"] = netName( b.memberNets[i].id );
        }
    }
    for( const auto& l : aModel.labels )
    {
        auto& r = addOwned( CANONICAL_KIND::LABEL, l.sheet, l.text.text.ToStdString() );
        r.properties["kind"] = canonicalLabelKind( l.kind );
        std::vector<std::string> linkedSheets;

        for( const SHEET_REFERENCE& linked : l.linkedSheets )
        {
            auto sheet = std::ranges::find( aModel.sheets, linked.id, &MODEL_SHEET::id );

            if( sheet != aModel.sheets.end() )
                linkedSheets.push_back( std::to_string( sheet->index ) );
        }

        std::ranges::sort( linkedSheets );
        r.properties["linked_sheets"] = { linkedSheets };
        r.geometry.points.push_back( point( l.position ) );
        r.geometry.angleTenths = canonicalAngle( l.angle );
        addSourceProperties( r, l.properties );
    }
    for( const auto& j : aModel.junctions )
    {
        auto& r = addOwned( CANONICAL_KIND::JUNCTION, j.sheet );
        r.geometry.points.push_back( point( j.position ) );
        addSourceProperties( r, j.properties );
    }
    for( const auto& t : aModel.texts )
    {
        auto& r = addOwned( CANONICAL_KIND::TEXT, t.sheet, t.text.text.ToStdString() );
        r.properties["visible"] = { t.presentation.visible };
        r.properties["font"] = { t.presentation.font.text.ToStdString() };
        r.properties["bold"] = { t.presentation.bold };
        r.properties["italic"] = { t.presentation.italic };
        r.properties["underline"] = { t.presentation.underline };
        r.properties["horizontal_justification"] = canonicalJustification( t.presentation.horizontalJustification );
        r.properties["vertical_justification"] = canonicalVerticalJustification( t.presentation.verticalJustification );
        r.geometry.points.push_back( point( t.position ) );
        r.geometry.angleTenths = canonicalAngle( t.angle );
        addSourceProperties( r, t.properties );
    }
    for( const MODEL_PAGE_GRAPHIC& pageGraphic : aModel.graphics )
    {
        auto& record = addOwned( CANONICAL_KIND::GRAPHIC, pageGraphic.sheet );
        int   sheet = record.sheet;
        auto  owner = record.properties.at( "owner_sheet" );
        out.pop_back();
        graphic( pageGraphic.graphic, sheet );
        out.back().properties["owner_sheet"] = owner;
    }
    return out;
}

static std::vector<CANONICAL_SEMANTIC_RECORD> normalizeAsciiModel( const PADS_SCH::PADS_SCH_PARSER& aParser )
{
    std::vector<CANONICAL_SEMANTIC_RECORD> out;
    auto add = [&]( CANONICAL_KIND k, int s, std::string key = {} ) -> CANONICAL_SEMANTIC_RECORD&
    {
        out.push_back( { s, k, std::move( key ) } );
        return out.back();
    };
    const auto& p = aParser.GetParameters();
    auto        addOwned = [&]( CANONICAL_KIND aKind, int aSheet, std::string aKey = {} ) -> CANONICAL_SEMANTIC_RECORD&
    {
        auto  sheet = std::ranges::find_if( aParser.GetSheetHeaders(),
                                            [&]( const PADS_SCH::SHEET_HEADER& aCandidate )
                                            {
                                               return aCandidate.sheet_num - 1 == aSheet;
                                           } );
        auto& record = add( aKind, aSheet, std::move( aKey ) );
        record.properties["owner_sheet"] =
                sheet == aParser.GetSheetHeaders().end()
                        ? CANONICAL_PROPERTY{ "unknown:" + std::to_string( aSheet ), PROPERTY_DISPOSITION::UNSUPPORTED }
                        : CANONICAL_PROPERTY{ sheet->sheet_name };
        return record;
    };
    auto& settings = add( CANONICAL_KIND::SETTINGS, -1 );
    settings.properties["coordinate_units_per_mil"] = { int64_t( 2 ) };
    settings.properties["line_width_half_mils"] = { int64_t( std::llround( p.line_width * 2 ) ) };
    settings.properties["bus_width_half_mils"] = { int64_t( p.bus_width * 2 ) };
    settings.geometry.points.push_back( point( PADS_SCH::POINT{ p.sheet_size.width, p.sheet_size.height } ) );
    auto addGraphic = [&]( const PADS_SCH::SYMBOL_GRAPHIC& aGraphic, int aSheet )
    {
        auto& r = add( CANONICAL_KIND::GRAPHIC, aSheet );
        r.properties["type"] = canonicalGraphicType( aGraphic.type );
        r.properties["line_style"] = canonicalLineStyle( aGraphic.line_style );
        r.properties["stroke_half_mils"] = { int64_t( std::llround( aGraphic.line_width * 2 ) ) };
        r.properties["fill"] = canonicalFill( aGraphic.filled );
        r.properties["text"] = { std::string() };
        r.properties["font"] = { std::string() };
        r.properties["visible"] = { true };
        r.properties["height_half_mils"] = { int64_t( 0 ) };
        r.properties["width_half_mils"] = { int64_t( 0 ) };

        for( const auto& q : aGraphic.points )
            r.geometry.points.push_back( point( q.coord ) );

        auto arcPoint = std::ranges::find_if( aGraphic.points,
                                              []( const PADS_SCH::GRAPHIC_POINT& aPoint )
                                              {
                                                  return aPoint.arc.has_value();
                                              } );

        if( arcPoint != aGraphic.points.end() )
        {
            const PADS_SCH::ARC_DATA& arc = *arcPoint->arc;
            r.properties["arc_center_x_half_mils"] = { int64_t( std::llround( ( arc.bbox_x1 + arc.bbox_x2 ) ) ) };
            r.properties["arc_center_y_half_mils"] = { int64_t( std::llround( ( arc.bbox_y1 + arc.bbox_y2 ) ) ) };
            r.properties["arc_bounds_x1_half_mils"] = { int64_t( std::llround( arc.bbox_x1 * 2 ) ) };
            r.properties["arc_bounds_y1_half_mils"] = { int64_t( std::llround( arc.bbox_y1 * 2 ) ) };
            r.properties["arc_bounds_x2_half_mils"] = { int64_t( std::llround( arc.bbox_x2 * 2 ) ) };
            r.properties["arc_bounds_y2_half_mils"] = { int64_t( std::llround( arc.bbox_y2 * 2 ) ) };
            r.properties["arc_sweep_tenths"] = { int64_t( std::llround( std::abs( arc.bulge ) ) ) };
            r.properties["arc_clockwise"] = { arc.angle < 0 };
        }
    };
    for( const auto& s : aParser.GetSheetHeaders() )
    {
        auto& r = add( CANONICAL_KIND::SHEET, s.sheet_num - 1, s.sheet_name );
        r.properties["title"] = { std::string() };
        r.properties["line_width_half_mils"] = { int64_t( std::llround( p.line_width * 2 ) ) };
        r.properties["bus_width_half_mils"] = { int64_t( p.bus_width * 2 ) };
        r.geometry.points.push_back( point( PADS_SCH::POINT{ p.sheet_size.width, p.sheet_size.height } ) );
    }
    for( const auto& d : aParser.GetSymbolDefs() )
    {
        add( CANONICAL_KIND::DEFINITION, -1, d.name );
        for( const auto& g : d.graphics )
            addGraphic( g, -1 );
        std::vector<PADS_SCH::SYMBOL_PIN> pins = d.pins;

        for( const auto& [partName, part] : aParser.GetPartTypes() )
        {
            for( const PADS_SCH::GATE_DEF& gate : part.gates )
            {
                if( gate.decal_names.empty() || gate.decal_names.front() != d.name || gate.pins.size() != pins.size() )
                    continue;

                for( size_t i = 0; i < pins.size(); ++i )
                {
                    pins[i].number = gate.pins[i].pin_id;
                    pins[i].name = gate.pins[i].pin_name;
                    switch( gate.pins[i].pin_type )
                    {
                    case 'L': pins[i].type = PADS_SCH::PIN_TYPE::INPUT; break;
                    case 'S': pins[i].type = PADS_SCH::PIN_TYPE::OUTPUT; break;
                    case 'B': pins[i].type = PADS_SCH::PIN_TYPE::BIDIRECTIONAL; break;
                    case 'T': pins[i].type = PADS_SCH::PIN_TYPE::TRISTATE; break;
                    case 'C': pins[i].type = PADS_SCH::PIN_TYPE::OPEN_COLLECTOR; break;
                    case 'E': pins[i].type = PADS_SCH::PIN_TYPE::OPEN_EMITTER; break;
                    case 'P':
                    case 'G': pins[i].type = PADS_SCH::PIN_TYPE::POWER; break;
                    case 'U': pins[i].type = PADS_SCH::PIN_TYPE::PASSIVE; break;
                    default: pins[i].type = PADS_SCH::PIN_TYPE::UNSPECIFIED; break;
                    }
                }

                break;
            }
        }

        for( const auto& pin : pins )
        {
            auto& r = add( CANONICAL_KIND::PIN, -1, pin.number );
            r.properties["name"] = { pin.name };
            r.properties["type"] = canonicalPinType( pin.type );
            r.properties["style"] = canonicalPinStyle( pin.inverted, pin.clock );
            r.properties["length_half_mils"] = { int64_t( std::llround( pin.length * 2 ) ) };
            r.geometry.points.push_back( point( pin.position ) );
            r.geometry.angleTenths = canonicalAngle( std::llround( pin.rotation * 10 ) );
        }
        for( const auto& t : d.texts )
        {
            auto& r = add( CANONICAL_KIND::GRAPHIC, -1 );
            r.properties["type"] = { "text" };
            r.properties["line_style"] = { "solid" };
            r.properties["stroke_half_mils"] = { int64_t( 0 ) };
            r.properties["fill"] = { "none" };
            r.properties["text"] = { t.content };
            r.properties["visible"] = { t.visible };
            r.properties["font"] = { t.font_name };
            r.properties["height_half_mils"] = { int64_t( std::llround( t.size ) ) };
            r.properties["width_half_mils"] = { int64_t( t.width_factor ) };
            r.geometry.points.push_back( point( t.position ) );
            r.geometry.angleTenths = canonicalAngle( std::llround( t.rotation * 10 ) );
        }
    }
    for( const auto& [name, part] : aParser.GetPartTypes() )
    {
        add( CANONICAL_KIND::PART_TYPE, -1, name );
        for( size_t i = 0; i < part.gates.size(); ++i )
        {
            auto& gate = add( CANONICAL_KIND::GATE, -1, std::to_string( i + 1 ) );
            gate.properties["definition"] = { part.gates[i].decal_names.empty() ? std::string()
                                                                                : part.gates[i].decal_names.front() };
            for( size_t j = 0; j < part.gates[i].pins.size(); ++j )
            {
                auto& r = add( CANONICAL_KIND::GATE_PIN_MAPPING, -1,
                               std::to_string( i + 1 ) + ":" + std::to_string( j ) );
                r.properties["pin_number"] = { part.gates[i].pins[j].pin_id };
                r.properties["pin_name"] = { part.gates[i].pins[j].pin_name };
            }
        }
    }
    for( const auto& x : aParser.GetPartPlacements() )
    {
        auto& r = addOwned( CANONICAL_KIND::PLACEMENT, x.sheet_number - 1, x.reference );
        r.properties["unit"] = { int64_t( x.gate_number ) };
        r.properties["mirrored"] = { x.mirror_flags != 0 };
        r.geometry.points.push_back( point( x.position ) );
        r.geometry.angleTenths = canonicalAngle( std::llround( x.rotation * 10 ) );
        int ownerSheet = r.sheet;
        for( const auto& f : x.attributes )
        {
            auto& q = add( CANONICAL_KIND::FIELD, ownerSheet, f.name );
            q.properties["value"] = { f.value };
            q.properties["visible"] = { f.visible };
            q.properties["font"] = { f.font_name };
            q.geometry.points.push_back( point( f.position ) );
            q.geometry.angleTenths = canonicalAngle( std::llround( f.rotation * 10 ) );
        }
    }
    for( const auto& n : aParser.GetSignals() )
    {
        if( n.flags1 == 5 && n.flags2 == 3 )
            continue;

        int netSheet = n.wires.empty() ? -1 : n.wires.front().sheet_number - 1;
        addOwned( CANONICAL_KIND::NET, netSheet, n.name );
        for( const auto& c : n.wires )
        {
            auto& r = addOwned( CANONICAL_KIND::CONNECTION, c.sheet_number - 1 );
            r.properties["endpoint_count"] = { int64_t( 2 ) };

            const std::array<std::string, 2> endpoints = { c.endpoint_a, c.endpoint_b };

            for( size_t endpoint = 0; endpoint < endpoints.size(); ++endpoint )
            {
                const std::string  prefix = "endpoint_" + std::to_string( endpoint ) + "_";
                const std::string& token = endpoints[endpoint];

                if( token.starts_with( "@@@" ) )
                {
                    r.properties[prefix + "kind"] = canonicalEndpointKind( MODEL_ENDPOINT_KIND::POINT );
                }
                else
                {
                    const size_t separator = token.rfind( '.' );
                    r.properties[prefix + "kind"] = canonicalEndpointKind( MODEL_ENDPOINT_KIND::PIN );
                    r.properties[prefix + "placement"] = { token.substr( 0, separator ) };
                    r.properties[prefix + "pin_number"] = { token.substr( separator + 1 ) };
                }
            }

            for( const auto& q : c.vertices )
                r.geometry.points.push_back( point( q ) );
        }
    }
    for( const auto& b : aParser.GetBuses() )
    {
        auto& r = addOwned( CANONICAL_KIND::BUS, b.sheet_number - 1, b.name );
        r.properties["aliases"] = { b.aliases };
        r.properties["member_nets"] = { b.member_nets };
        for( const auto& q : b.path )
            r.geometry.points.push_back( point( q ) );
        for( size_t i = 0; i < b.entries.size(); ++i )
        {
            auto& e = addOwned( CANONICAL_KIND::BUS_ENTRY, b.sheet_number - 1, std::to_string( i ) );
            e.properties["member_index"] = { int64_t( i ) };
            e.properties["member_net"] = { b.entries[i].member_net };
            e.geometry.points.push_back( point( b.entries[i].position ) );
        }
        for( const auto& a : b.aliases )
            addOwned( CANONICAL_KIND::BUS_ALIAS, b.sheet_number - 1, a );
        for( size_t i = 0; i < b.member_nets.size(); ++i )
        {
            auto& member = addOwned( CANONICAL_KIND::BUS_MEMBER, b.sheet_number - 1, std::to_string( i ) );
            member.properties["member_net"] = { b.member_nets[i] };

            if( std::ranges::none_of( aParser.GetSignals(),
                                      [&]( const PADS_SCH::SCH_SIGNAL& aSignal )
                                      {
                                          return aSignal.name == b.member_nets[i];
                                      } ) )
            {
                addOwned( CANONICAL_KIND::NET, b.sheet_number - 1, b.member_nets[i] );
            }
        }
    }
    for( const auto& l : aParser.GetOffPageConnectors() )
    {
        if( l.symbol_lib.starts_with( "@@@B" ) )
            continue;
        auto& r = addOwned( CANONICAL_KIND::LABEL, l.source_sheet - 1, l.signal_name );
        r.properties["kind"] = { l.symbol_lib == "@TERM"              ? "local"
                                 : l.symbol_lib.starts_with( "$OSR" ) ? "global"
                                 : l.symbol_lib.starts_with( "$GND" ) ? "ground"
                                 : l.symbol_lib.starts_with( "$PWR" ) ? "power"
                                                                      : "unsupported" };
        std::vector<std::string> linkedSheets;

        if( l.symbol_lib.starts_with( "$OSR" ) )
        {
            for( const auto& peer : aParser.GetOffPageConnectors() )
            {
                if( peer.signal_name != l.signal_name || peer.source_sheet == l.source_sheet
                    || !peer.symbol_lib.starts_with( "$OSR" ) )
                {
                    continue;
                }

                auto sheet = std::ranges::find_if( aParser.GetSheetHeaders(),
                                                   [&]( const PADS_SCH::SHEET_HEADER& aHeader )
                                                   {
                                                       return aHeader.sheet_num == peer.source_sheet;
                                                   } );

                if( sheet != aParser.GetSheetHeaders().end() )
                    linkedSheets.push_back( std::to_string( sheet->sheet_num - 1 ) );
            }
        }

        std::ranges::sort( linkedSheets );
        r.properties["linked_sheets"] = { linkedSheets };
        r.geometry.points.push_back( point( l.position ) );
        r.geometry.angleTenths = canonicalAngle( l.rotation * 10 );
    }
    for( const auto& j : aParser.GetTiedDots() )
        addOwned( CANONICAL_KIND::JUNCTION, j.sheet_number - 1 ).geometry.points.push_back( point( j.position ) );
    for( const auto& t : aParser.GetTextItems() )
    {
        auto& r = addOwned( CANONICAL_KIND::TEXT, t.sheet_number - 1, t.content );
        r.properties["visible"] = { true };
        r.properties["bold"] = { false };
        r.properties["italic"] = { false };
        r.properties["underline"] = { false };
        r.geometry.points.push_back( point( t.position ) );
        r.geometry.angleTenths = canonicalAngle( t.rotation * 10 );
    }
    for( const auto& lines : aParser.GetLinesItems() )
    {
        for( const auto& g : lines.primitives )
        {
            auto& owner = addOwned( CANONICAL_KIND::GRAPHIC, lines.sheet_number - 1 );
            auto  property = owner.properties.at( "owner_sheet" );
            out.pop_back();
            addGraphic( g, lines.sheet_number - 1 );
            out.back().properties["owner_sheet"] = property;
            out.back().properties["page_graphic_group"] = { lines.name };
            out.back().properties["type"] = canonicalGraphicType( g );

            for( CANONICAL_POINT& graphicPoint : out.back().geometry.points )
            {
                graphicPoint.xHalfMils += std::llround( lines.origin.x * 2 );
                graphicPoint.yHalfMils += std::llround( lines.origin.y * 2 );
            }

            for( const std::string& name :
                 { "arc_center_x_half_mils", "arc_bounds_x1_half_mils", "arc_bounds_x2_half_mils" } )
            {
                auto property = out.back().properties.find( name );

                if( property != out.back().properties.end() )
                    std::get<int64_t>( property->second.value ) += std::llround( lines.origin.x * 2 );
            }

            for( const std::string& name :
                 { "arc_center_y_half_mils", "arc_bounds_y1_half_mils", "arc_bounds_y2_half_mils" } )
            {
                auto property = out.back().properties.find( name );

                if( property != out.back().properties.end() )
                    std::get<int64_t>( property->second.value ) += std::llround( lines.origin.y * 2 );
            }

            if( g.type == PADS_SCH::GRAPHIC_TYPE::RECTANGLE )
            {
                out.back().properties["type"] = canonicalGraphicType( MODEL_GRAPHIC_KIND::POLYLINE );

                if( out.back().geometry.points.size() == 2 )
                {
                    const CANONICAL_POINT first = out.back().geometry.points.front();
                    const CANONICAL_POINT last = out.back().geometry.points.back();
                    out.back().geometry.points = { { last.xHalfMils, last.yHalfMils },
                                                   { first.xHalfMils, last.yHalfMils },
                                                   { first.xHalfMils, first.yHalfMils },
                                                   { last.xHalfMils, first.yHalfMils },
                                                   { last.xHalfMils, last.yHalfMils } };
                }
            }

            out.back().geometry.angleTenths = 0;
        }

        for( const auto& t : lines.texts )
        {
            auto& r = addOwned( CANONICAL_KIND::GRAPHIC, lines.sheet_number - 1 );
            r.properties["type"] = canonicalGraphicType( MODEL_GRAPHIC_KIND::TEXT );
            r.properties["line_style"] = canonicalLineStyle( MODEL_LINE_STYLE::DEFAULT );
            r.properties["stroke_half_mils"] = { int64_t( 0 ) };
            r.properties["fill"] = canonicalFill( MODEL_FILL_STYLE::NONE );
            r.properties["text"] = { t.content };
            r.properties["page_graphic_group"] = { lines.name };
            r.properties["visible"] = { true };
            r.properties["height_half_mils"] = { int64_t( t.height ) };
            r.properties["width_half_mils"] = { int64_t( t.width_factor ) };
            r.properties["font"] = { t.font_name };
            r.geometry.points.push_back( point( t.position ) );
            r.geometry.points.back().xHalfMils += std::llround( lines.origin.x * 2 );
            r.geometry.points.back().yHalfMils += std::llround( lines.origin.y * 2 );
            r.geometry.angleTenths = canonicalAngle( t.rotation * 10 );
        }
    }
    for( const auto& label : aParser.GetNetNameLabels() )
    {
        auto& r = addOwned( CANONICAL_KIND::LABEL, -1, label.net_name );
        r.properties["kind"] = { "local" };
        r.properties["visible"] = { true };
        r.properties["font"] = { label.font_name };
        r.properties["horizontal_justification"] = canonicalHorizontalJustification( label.justification );
        r.properties["vertical_justification"] = canonicalVerticalJustification( label.justification );
        r.geometry.points.push_back( point( PADS_SCH::POINT{ double( label.x_offset ), double( label.y_offset ) } ) );
        r.geometry.angleTenths = canonicalAngle( label.rotation * 10 );
    }
    return out;
}

static bool snapshotsMatch( std::vector<CANONICAL_SEMANTIC_RECORD> aExpected,
                            std::vector<CANONICAL_SEMANTIC_RECORD> aActual,
                            const SNAPSHOT_ALLOWLIST&              aAllowedDifferences = {} )
{
    auto strip = [&]( auto& records )
    {
        for( auto& record : records )
            std::erase_if( record.properties,
                           [&]( const auto& p )
                           {
                               return aAllowedDifferences.contains( { p.first, p.second.disposition } );
                           } );
        std::sort( records.begin(), records.end() );
    };
    strip( aExpected );
    strip( aActual );
    return aExpected == aActual;
}


static bool task10SourcePropertiesPresent( const std::vector<CANONICAL_SEMANTIC_RECORD>& aRecords )
{
    auto has =
            []( const CANONICAL_SEMANTIC_RECORD& aRecord, const std::string& aName, PROPERTY_DISPOSITION aDisposition )
    {
        auto property = aRecord.properties.find( aName );
        return property != aRecord.properties.end() && property->second.disposition == aDisposition;
    };

    for( const CANONICAL_SEMANTIC_RECORD& record : aRecords )
    {
        if( record.kind == CANONICAL_KIND::NET
            && ( !has( record, "global_net_record", PROPERTY_DISPOSITION::EXACT )
                 || !has( record, "preserved_net_identity", PROPERTY_DISPOSITION::PRESERVED )
                 || !has( record, "preserved_net_relationship", PROPERTY_DISPOSITION::PRESERVED ) ) )
        {
            return false;
        }

        if( record.kind == CANONICAL_KIND::CONNECTION
            && ( !has( record, "endpoint_0_raw_endpoint_handle", PROPERTY_DISPOSITION::EXACT )
                 || !has( record, "endpoint_1_raw_endpoint_handle", PROPERTY_DISPOSITION::EXACT )
                 || !has( record, "endpoint_0_raw_endpoint_relationship", PROPERTY_DISPOSITION::PRESERVED )
                 || !has( record, "endpoint_1_raw_endpoint_relationship", PROPERTY_DISPOSITION::PRESERVED )
                 || !has( record, "raw_connection_marker", PROPERTY_DISPOSITION::EXACT ) ) )
        {
            return false;
        }

        if( record.kind == CANONICAL_KIND::LABEL && !has( record, "offpage_variant", PROPERTY_DISPOSITION::EXACT ) )
        {
            return false;
        }

        if( record.kind == CANONICAL_KIND::JUNCTION
            && !has( record, "connection_record", PROPERTY_DISPOSITION::EXACT ) )
        {
            return false;
        }

        if( record.kind == CANONICAL_KIND::BUS
            && ( !has( record, "preserved_net_identity", PROPERTY_DISPOSITION::PRESERVED )
                 || !has( record, "preserved_net_relationship", PROPERTY_DISPOSITION::PRESERVED ) ) )
        {
            return false;
        }
    }

    return true;
}

} // namespace


BOOST_AUTO_TEST_SUITE( PadsSchBinaryParser )


BOOST_AUTO_TEST_CASE( EmbeddedOleImages )
{
    PADS_SCH_BINARY_PARSER parser;
    std::vector<uint8_t>   bytes = loadBinaryFixture( "ole_images.sch" );
    PADS_SCH_MODEL         model = parser.Parse( bytes, wxS( "ole_images.sch" ) );

    BOOST_REQUIRE_EQUAL( model.images.size(), 2 );
    BOOST_CHECK_EQUAL( model.images[0].id.Value(), 0 );
    BOOST_CHECK_EQUAL( model.images[0].sheet.id.Value(), 1 );
    BOOST_CHECK( model.images[0].type == MODEL_EMBEDDED_IMAGE_TYPE::BMP );
    BOOST_CHECK_EQUAL( model.images[0].streamName, wxS( "\\x01Ole10Native" ) );
    BOOST_CHECK( ( model.images[0].extent == std::array<int32_t, 4>{ 30, 30, 350, 210 } ) );
    BOOST_CHECK( ( model.images[0].databaseBox == std::array<int32_t, 4>{ -15766, -10366, -13296, -11755 } ) );
    BOOST_CHECK_EQUAL( model.images[0].position.x, 5876 );
    BOOST_CHECK_EQUAL( model.images[0].position.y, 19758 );
    BOOST_CHECK_EQUAL( model.images[0].size.x, 9880 );
    BOOST_CHECK_EQUAL( model.images[0].size.y, 5556 );
    BOOST_CHECK_EQUAL( model.images[0].flags, 1 );
    BOOST_REQUIRE_GE( model.images[0].data.size(), 2 );
    BOOST_CHECK_EQUAL( model.images[0].data[0], 'B' );
    BOOST_CHECK_EQUAL( model.images[0].data[1], 'M' );
    BOOST_CHECK_EQUAL( model.images[0].source.objectClass, wxS( "embedded OLE image" ) );
    BOOST_CHECK_EQUAL( model.images[0].source.recordIndex, 0 );
    BOOST_CHECK_GT( model.images[0].source.absoluteOffset, 0 );
    BOOST_CHECK_GT( model.images[0].source.length, 512 );

    BOOST_CHECK_EQUAL( model.images[1].id.Value(), 1 );
    BOOST_CHECK_EQUAL( model.images[1].sheet.id.Value(), 1 );
    BOOST_CHECK( model.images[1].type == MODEL_EMBEDDED_IMAGE_TYPE::WMF );
    BOOST_CHECK_EQUAL( model.images[1].streamName, wxS( "\\x01Ole10Native" ) );
    BOOST_CHECK( ( model.images[1].extent == std::array<int32_t, 4>{ 30, 30, 170, 84 } ) );
    BOOST_CHECK( ( model.images[1].databaseBox == std::array<int32_t, 4>{ -15766, -10366, -14686, -10782 } ) );
    BOOST_CHECK_EQUAL( model.images[1].position.x, 3096 );
    BOOST_CHECK_EQUAL( model.images[1].position.y, 21704 );
    BOOST_CHECK_EQUAL( model.images[1].size.x, 4320 );
    BOOST_CHECK_EQUAL( model.images[1].size.y, 1664 );
    BOOST_CHECK_EQUAL( model.images[1].flags, 1 );
    BOOST_REQUIRE_GE( model.images[1].data.size(), 4 );
    BOOST_CHECK_EQUAL( model.images[1].data[0], 0xD7 );
    BOOST_CHECK_EQUAL( model.images[1].data[1], 0xCD );
    BOOST_CHECK_EQUAL( model.images[1].data[2], 0xC6 );
    BOOST_CHECK_EQUAL( model.images[1].data[3], 0x9A );

    PADS_SCH_SDB sdb;
    sdb.Load( bytes );
    BOOST_REQUIRE_EQUAL( sdb.OleItems().size(), 2u );
    std::vector<uint8_t> zeroWidth = bytes;
    writeU32( zeroWidth, sdb.OleItems()[0].boxOffset + 8, static_cast<uint32_t>( sdb.OleItems()[0].left ) );
    PADS_SCH_MODEL zeroWidthModel = parser.Parse( zeroWidth, wxS( "ole-zero-width.sch" ) );
    BOOST_REQUIRE_EQUAL( zeroWidthModel.images.size(), 2 );
    BOOST_CHECK( zeroWidthModel.images[0].type == MODEL_EMBEDDED_IMAGE_TYPE::UNSUPPORTED );
    BOOST_CHECK( std::ranges::any_of( zeroWidthModel.diagnostics,
                                      []( const PARSER_DIAGNOSTIC& aDiagnostic )
                                      {
                                          return aDiagnostic.source.objectClass
                                                         == wxS( "embedded OLE image database box" )
                                                 && aDiagnostic.message.Contains( wxS( "zero-size" ) );
                                      } ) );
}


BOOST_AUTO_TEST_CASE( GlobalsAndSheets )
{
    PADS_SCH_BINARY_PARSER parser;
    PADS_SCH_MODEL         minimal = parser.Parse( loadBinaryFixture( "minimal_v13.sch" ), wxS( "minimal_v13.sch" ) );

    BOOST_CHECK_EQUAL( minimal.version, 0x000D );
    BOOST_CHECK_EQUAL( minimal.subversion, 0 );
    BOOST_CHECK_EQUAL( minimal.settings.codePage, 1252 );
    BOOST_CHECK_EQUAL( minimal.settings.coordinateUnitsPerMil, 2 );
    BOOST_CHECK_EQUAL( minimal.settings.pageSize.x, 34000 );
    BOOST_CHECK_EQUAL( minimal.settings.pageSize.y, 22000 );
    BOOST_CHECK_EQUAL( minimal.settings.defaultLineWidth, 20 );
    BOOST_CHECK_EQUAL( minimal.settings.defaultBusWidth, 50 );
    BOOST_CHECK_EQUAL( minimal.settings.source.objectClass, wxS( "design settings" ) );
    BOOST_CHECK_EQUAL( minimal.settings.source.controller, 5 );
    BOOST_CHECK_EQUAL( minimal.settings.source.absoluteOffset, 0x439 );
    BOOST_CHECK_EQUAL( minimal.settings.source.length, 400 );
    BOOST_CHECK_EQUAL( minimal.settings.source.version, 0x000D );
    BOOST_REQUIRE_EQUAL( minimal.sheets.size(), 1 );
    BOOST_CHECK_EQUAL( minimal.sheets[0].id.Value(), 1 );
    BOOST_CHECK_EQUAL( minimal.sheets[0].index, 0 );
    BOOST_CHECK_EQUAL( minimal.sheets[0].name.text, wxS( "$$$NONE" ) );
    BOOST_CHECK( !minimal.sheets[0].parent );
    BOOST_CHECK_EQUAL( minimal.sheets[0].pageSize.x, 34000 );
    BOOST_CHECK_EQUAL( minimal.sheets[0].defaultLineWidth, 20 );
    BOOST_REQUIRE_EQUAL( minimal.sheets[0].titleBlockFields.size(), 14 );
    BOOST_CHECK_EQUAL( minimal.sheets[0].titleBlockFields.front().name.text, wxS( "Drawn By" ) );
    BOOST_CHECK_EQUAL( minimal.sheets[0].titleBlockFields.back().name.text, wxS( "Scale" ) );

    const std::array<wxString, 14> titleNames = { wxS( "Drawn By" ),    wxS( "Checked By" ),   wxS( "QC By" ),
                                                  wxS( "Released By" ), wxS( "Drawn Date" ),   wxS( "Checked Date" ),
                                                  wxS( "QC Date" ),     wxS( "Release Date" ), wxS( "Company Name" ),
                                                  wxS( "Title" ),       wxS( "Code" ),         wxS( "Drawing Number" ),
                                                  wxS( "Revision" ),    wxS( "Scale" ) };
    const std::array<size_t, 14>   titleOffsets = { 0x254, 0x264, 0x276, 0x283, 0x296, 0x2A8, 0x2BC,
                                                    0x2CB, 0x2DF, 0x2F3, 0x300, 0x30C, 0x322, 0x332 };
    const std::array<size_t, 14>   titleLengths = { 16, 18, 13, 19, 18, 20, 15, 20, 20, 13, 12, 22, 16, 13 };

    for( size_t i = 0; i < titleNames.size(); ++i )
    {
        const MODEL_FIELD& field = minimal.sheets[0].titleBlockFields[i];
        const std::string  expectedName = titleNames[i].ToStdString();
        BOOST_CHECK_EQUAL( field.name.text, titleNames[i] );
        BOOST_CHECK_EQUAL( field.value.text, wxString() );
        BOOST_CHECK_EQUAL_COLLECTIONS( field.name.raw.begin(), field.name.raw.end(), expectedName.begin(),
                                       expectedName.end() );
        BOOST_CHECK( field.value.raw.empty() );
        BOOST_CHECK_EQUAL( field.source.absoluteOffset, titleOffsets[i] );
        BOOST_CHECK_EQUAL( field.source.length, titleLengths[i] );
        BOOST_CHECK_EQUAL( field.source.controller, 1 );
        BOOST_CHECK_EQUAL( field.source.version, 0x000D );
        BOOST_CHECK_EQUAL( field.name.source.absoluteOffset, titleOffsets[i] + 6 );
        BOOST_CHECK_EQUAL( field.name.source.length, expectedName.size() );
        BOOST_CHECK_EQUAL( field.name.source.controller, 1 );
        BOOST_CHECK_EQUAL( field.name.source.version, 0x000D );
        BOOST_CHECK_EQUAL( field.value.source.absoluteOffset, titleOffsets[i] + titleLengths[i] - 1 );
        BOOST_CHECK_EQUAL( field.value.source.length, 0 );
        BOOST_CHECK_EQUAL( field.value.source.controller, 1 );
        BOOST_CHECK_EQUAL( field.value.source.version, 0x000D );
    }

    BOOST_CHECK_EQUAL( minimal.sheets[0].title.text, wxString() );
    PADS_SCH::PADS_SCH_PARSER minimalAscii;
    BOOST_REQUIRE( minimalAscii.Parse( KI_TEST::GetEeschemaTestDataDir() + "/plugins/pads/binary/minimal_v13.txt" ) );
    BOOST_REQUIRE_EQUAL( minimalAscii.GetParameters().fields.size(), titleNames.size() );

    for( const MODEL_FIELD& field : minimal.sheets[0].titleBlockFields )
    {
        auto asciiField = minimalAscii.GetParameters().fields.find( field.name.text.ToStdString() );
        BOOST_REQUIRE( asciiField != minimalAscii.GetParameters().fields.end() );
        BOOST_CHECK_EQUAL( asciiField->second, field.value.text.ToStdString() );
    }

    PADS_SCH_MODEL multisheet =
            parser.Parse( loadBinaryFixture( "multisheet_connectivity.sch" ), wxS( "multisheet_connectivity.sch" ) );
    BOOST_REQUIRE_EQUAL( multisheet.sheets.size(), 2 );
    BOOST_CHECK_EQUAL( multisheet.sheets[0].id.Value(), 1 );
    BOOST_CHECK_EQUAL( multisheet.sheets[1].id.Value(), 2 );
    BOOST_CHECK_EQUAL( multisheet.sheets[0].name.text, wxS( "[1]DUP/SAFE:*" ) );
    BOOST_CHECK_EQUAL( multisheet.sheets[1].name.text, wxS( "[2]DUP/SAFE:*" ) );
    BOOST_CHECK_LT( multisheet.sheets[0].source.absoluteOffset, multisheet.sheets[1].source.absoluteOffset );
    BOOST_CHECK( !multisheet.sheets[0].parent );
    BOOST_CHECK( !multisheet.sheets[1].parent );

    for( const MODEL_SHEET& sheet : multisheet.sheets )
    {
        BOOST_REQUIRE_EQUAL( sheet.titleBlockFields.size(), 14 );

        for( const MODEL_FIELD& field : sheet.titleBlockFields )
        {
            BOOST_CHECK_EQUAL( field.source.sheet, -1 );
            BOOST_CHECK_EQUAL( field.name.source.sheet, -1 );
            BOOST_CHECK_EQUAL( field.value.source.sheet, -1 );
            BOOST_CHECK_EQUAL( field.presentation.source.sheet, -1 );
        }

        BOOST_CHECK_EQUAL( sheet.title.source.sheet, -1 );
        auto title = std::ranges::find_if( sheet.titleBlockFields,
                                           []( const MODEL_FIELD& aField )
                                           {
                                               return aField.name.text == wxS( "Title" );
                                           } );
        BOOST_REQUIRE( title != sheet.titleBlockFields.end() );
        BOOST_CHECK( sheet.title == title->value );
    }
}


BOOST_AUTO_TEST_CASE( VariableTitleFields )
{
    std::vector<uint8_t>                                     bytes = loadBinaryFixture( "minimal_v13.sch" );
    const size_t                                             poolOffset = outerControllerOffset( bytes, 1 );
    const size_t                                             poolBytes = readU32( bytes, 0x20 + 1 * 28 + 12 );
    const std::array<std::pair<std::string, std::string>, 5> expected = {
        std::pair{ "Drawn By", "Alice" }, std::pair{ "Checked By", "Bob" }, std::pair{ "Title", "Variable Title" },
        std::pair{ "Revision", "C" }, std::pair{ "Scale", "2:1" }
    };
    std::vector<uint8_t>                pool;
    std::array<size_t, expected.size()> offsets;
    std::array<size_t, expected.size()> lengths;

    for( size_t i = 0; i < expected.size(); ++i )
    {
        offsets[i] = poolOffset + pool.size();
        const std::string slot = "Field\n" + expected[i].first + '\x01' + expected[i].second + '\0';
        lengths[i] = slot.size();
        pool.insert( pool.end(), slot.begin(), slot.end() );
    }

    const size_t      firstNonFieldOffset = poolOffset + pool.size();
    const std::string firstNonField( "Font Default\0", 13 );
    pool.insert( pool.end(), firstNonField.begin(), firstNonField.end() );
    BOOST_REQUIRE_LE( pool.size(), poolBytes );
    pool.resize( poolBytes, 0 );
    std::copy( pool.begin(), pool.end(), bytes.begin() + poolOffset );

    PADS_SCH_BINARY_PARSER parser;
    PADS_SCH_MODEL         model = parser.Parse( bytes, wxS( "variable-title.sch" ) );
    BOOST_REQUIRE_EQUAL( model.sheets.size(), 1 );
    BOOST_REQUIRE_EQUAL( model.sheets[0].titleBlockFields.size(), expected.size() );

    for( size_t i = 0; i < expected.size(); ++i )
    {
        const MODEL_FIELD& field = model.sheets[0].titleBlockFields[i];
        BOOST_CHECK_EQUAL( field.name.text, wxString::FromUTF8( expected[i].first ) );
        BOOST_CHECK_EQUAL( field.value.text, wxString::FromUTF8( expected[i].second ) );
        BOOST_CHECK_EQUAL_COLLECTIONS( field.name.raw.begin(), field.name.raw.end(), expected[i].first.begin(),
                                       expected[i].first.end() );
        BOOST_CHECK_EQUAL_COLLECTIONS( field.value.raw.begin(), field.value.raw.end(), expected[i].second.begin(),
                                       expected[i].second.end() );
        BOOST_CHECK_EQUAL( field.source.absoluteOffset, offsets[i] );
        BOOST_CHECK_EQUAL( field.source.length, lengths[i] );
        BOOST_CHECK_EQUAL( field.source.recordIndex, i );
        BOOST_CHECK_EQUAL( field.source.controller, 1 );
        BOOST_CHECK_EQUAL( field.source.version, 0x000D );
        BOOST_CHECK_EQUAL( field.source.sheet, -1 );
        BOOST_CHECK_EQUAL( field.source.objectClass, wxS( "title field" ) );
        BOOST_CHECK_EQUAL( field.name.source.absoluteOffset, offsets[i] + 6 );
        BOOST_CHECK_EQUAL( field.name.source.length, expected[i].first.size() );
        BOOST_CHECK_EQUAL( field.name.source.controller, 1 );
        BOOST_CHECK_EQUAL( field.name.source.version, 0x000D );
        BOOST_CHECK_EQUAL( field.name.source.sheet, -1 );
        BOOST_CHECK_EQUAL( field.value.source.absoluteOffset, offsets[i] + 6 + expected[i].first.size() + 1 );
        BOOST_CHECK_EQUAL( field.value.source.length, expected[i].second.size() );
        BOOST_CHECK_EQUAL( field.value.source.controller, 1 );
        BOOST_CHECK_EQUAL( field.value.source.version, 0x000D );
        BOOST_CHECK_EQUAL( field.value.source.sheet, -1 );
    }

    BOOST_CHECK_EQUAL( model.sheets[0].title.text, wxS( "Variable Title" ) );
    BOOST_CHECK_EQUAL_COLLECTIONS( bytes.begin() + firstNonFieldOffset,
                                   bytes.begin() + firstNonFieldOffset + firstNonField.size(), firstNonField.begin(),
                                   firstNonField.end() );
    BOOST_CHECK_EQUAL( model.sheets[0].titleBlockFields.back().source.absoluteOffset
                               + model.sheets[0].titleBlockFields.back().source.length,
                       firstNonFieldOffset );
}


BOOST_AUTO_TEST_CASE( FreeText )
{
    PADS_SCH_BINARY_PARSER parser;
    PADS_SCH_MODEL         model = parser.Parse( loadBinaryFixture( "text_encoding.sch" ), wxS( "text_encoding.sch" ) );

    BOOST_REQUIRE_EQUAL( model.texts.size(), 1 );
    const MODEL_TEXT& text = model.texts.front();
    BOOST_CHECK_EQUAL( text.sheet.id.Value(), 1 );
    BOOST_CHECK_EQUAL( text.text.text, wxS( "ascii-text cafe=" ) );
    BOOST_CHECK_EQUAL( text.text.raw.size(), 16 );
    BOOST_CHECK( text.text.encoding == STRING_ENCODING_STATUS::CODE_PAGE );
    BOOST_CHECK_EQUAL( text.text.codePage, 1252 );
    BOOST_CHECK_EQUAL( text.position.x, 20200 );
    BOOST_CHECK_EQUAL( text.position.y, 14000 );
    BOOST_CHECK_EQUAL( text.angle, 0 );
    BOOST_CHECK_EQUAL( text.presentation.height, 97 );
    BOOST_CHECK_EQUAL( text.presentation.width, 10 );
    BOOST_CHECK( text.presentation.horizontalJustification == MODEL_JUSTIFICATION::LEFT );
    BOOST_CHECK( text.presentation.verticalJustification == MODEL_JUSTIFICATION::RIGHT );
    BOOST_CHECK( !text.presentation.bold );
    BOOST_CHECK( !text.presentation.italic );
    BOOST_CHECK( !text.presentation.underline );
    BOOST_CHECK( text.presentation.visible );
    BOOST_REQUIRE_EQUAL( text.properties.size(), 1 );
    BOOST_CHECK_EQUAL( text.properties[0].name.text, wxS( "controller_1_relationship_word_28" ) );
    BOOST_CHECK_EQUAL( text.properties[0].value.text, wxS( "16" ) );
    BOOST_CHECK_EQUAL( text.properties[0].value.raw.size(), 2 );
    BOOST_CHECK_EQUAL( text.properties[0].source.absoluteOffset, text.source.absoluteOffset + 28 );
    BOOST_CHECK_EQUAL( text.properties[0].source.length, 2 );
    BOOST_CHECK( text.properties[0].disposition == PROPERTY_DISPOSITION::PRESERVED );
    BOOST_CHECK_EQUAL( text.source.objectClass, wxS( "free text" ) );
    BOOST_CHECK_EQUAL( text.source.controller, 1 );
    BOOST_CHECK_EQUAL( text.text.source.controller, 2 );
    BOOST_CHECK_EQUAL( text.source.sheet, 0 );

    std::vector<uint8_t> cp1252Bytes = loadBinaryFixture( "text_encoding.sch" );
    cp1252Bytes[text.text.source.absoluteOffset + text.text.source.length - 1] = 0xE9;
    PADS_SCH_MODEL cp1252 = parser.Parse( cp1252Bytes, wxS( "text_encoding.sch" ) );
    BOOST_REQUIRE_EQUAL( cp1252.texts.size(), 1 );
    BOOST_CHECK_EQUAL( cp1252.texts[0].text.raw.back(), 0xE9 );
    BOOST_CHECK_EQUAL( cp1252.texts[0].text.text.Last().GetValue(), 0x00E9 );
    BOOST_CHECK( cp1252.texts[0].text.encoding == STRING_ENCODING_STATUS::CODE_PAGE );
    BOOST_CHECK_EQUAL( cp1252.texts[0].text.codePage, 1252 );
    BOOST_CHECK_EQUAL( cp1252.texts[0].text.source.absoluteOffset, text.text.source.absoluteOffset );
    BOOST_CHECK_EQUAL( cp1252.texts[0].text.source.length, text.text.source.length );
    BOOST_CHECK_EQUAL( cp1252.texts[0].text.source.controller, 2 );
    BOOST_CHECK_EQUAL( cp1252.texts[0].text.source.version, 0x000D );

    std::vector<uint8_t> invalidOffset = loadBinaryFixture( "text_encoding.sch" );
    writeU32( invalidOffset, text.source.absoluteOffset + 8, std::numeric_limits<uint32_t>::max() );
    BOOST_CHECK_EXCEPTION( parser.Parse( invalidOffset, wxS( "invalid-offset.sch" ) ), IO_ERROR,
                           [&]( const IO_ERROR& aError )
                           {
                               return aError.What().Contains( wxString::Format( wxS( "offset 0x%zX" ),
                                                                                text.source.absoluteOffset + 8 ) )
                                      && aError.What().Contains( wxS( "string offset leaves controller 2" ) );
                           } );

    std::vector<uint8_t> invalidLength = loadBinaryFixture( "text_encoding.sch" );
    writeU16( invalidLength, text.source.absoluteOffset + 20, std::numeric_limits<uint16_t>::max() );
    BOOST_CHECK_EXCEPTION( parser.Parse( invalidLength, wxS( "invalid-length.sch" ) ), IO_ERROR,
                           [&]( const IO_ERROR& aError )
                           {
                               return aError.What().Contains( wxString::Format( wxS( "offset 0x%zX" ),
                                                                                text.source.absoluteOffset + 20 ) )
                                      && aError.What().Contains( wxS( "string length leaves controller 2" ) );
                           } );

    std::vector<uint8_t> missingNul = loadBinaryFixture( "text_encoding.sch" );
    const size_t         terminatorOffset = text.text.source.absoluteOffset + text.text.source.length;
    missingNul[terminatorOffset] = 'X';
    BOOST_CHECK_EXCEPTION( parser.Parse( missingNul, wxS( "missing-nul.sch" ) ), IO_ERROR,
                           [&]( const IO_ERROR& aError )
                           {
                               return aError.What().Contains(
                                              wxString::Format( wxS( "offset 0x%zX" ), terminatorOffset ) )
                                      && aError.What().Contains( wxS( "not NUL terminated" ) )
                                      && aError.What().Contains( wxS( "controller 2" ) );
                           } );

    PADS_SCH::PADS_SCH_PARSER ascii;
    BOOST_REQUIRE( ascii.Parse( KI_TEST::GetEeschemaTestDataDir() + "/plugins/pads/binary/text_encoding.txt" ) );
    std::vector<CANONICAL_SEMANTIC_RECORD> expected = normalizeAsciiModel( ascii );
    std::vector<CANONICAL_SEMANTIC_RECORD> actual = normalizeBinaryModel( model );
    auto retainTask7Properties = []( std::vector<CANONICAL_SEMANTIC_RECORD>& aRecords )
    {
        std::erase_if( aRecords,
                       []( const CANONICAL_SEMANTIC_RECORD& aRecord )
                       {
                           return aRecord.kind != CANONICAL_KIND::SETTINGS && aRecord.kind != CANONICAL_KIND::SHEET
                                  && ( aRecord.kind != CANONICAL_KIND::TEXT
                                       || aRecord.stableKey != "ascii-text cafe=" );
                       } );

        for( CANONICAL_SEMANTIC_RECORD& record : aRecords )
        {
            if( record.kind != CANONICAL_KIND::TEXT )
                continue;

            std::erase_if( record.properties,
                           []( const auto& aProperty )
                           {
                               return aProperty.first != "owner_sheet" && aProperty.first != "visible"
                                      && aProperty.first != "bold" && aProperty.first != "italic"
                                      && aProperty.first != "underline";
                           } );
        }
    };
    retainTask7Properties( expected );
    retainTask7Properties( actual );
    BOOST_CHECK( snapshotsMatch( expected, actual ) );

    std::vector<uint8_t> changedRelationship = loadBinaryFixture( "text_encoding.sch" );
    writeU16( changedRelationship, text.source.absoluteOffset + 28, 0xBEEF );
    PADS_SCH_MODEL changed = parser.Parse( changedRelationship, wxS( "text_encoding.sch" ) );
    BOOST_REQUIRE_EQUAL( changed.texts.size(), 1 );
    BOOST_CHECK_EQUAL( changed.texts[0].properties[0].value.text, wxS( "48879" ) );
    BOOST_CHECK( changed.texts[0].properties[0].disposition == PROPERTY_DISPOSITION::PRESERVED );
    BOOST_CHECK_EQUAL( changed.texts[0].presentation.bold, text.presentation.bold );
    BOOST_CHECK_EQUAL( changed.texts[0].presentation.italic, text.presentation.italic );
    BOOST_CHECK_EQUAL( changed.texts[0].presentation.underline, text.presentation.underline );
    BOOST_CHECK_EQUAL( changed.texts[0].presentation.visible, text.presentation.visible );
    BOOST_CHECK_EQUAL( changed.diagnostics.size(), model.diagnostics.size() );

    struct JUSTIFICATION_CASE
    {
        uint16_t            raw;
        MODEL_JUSTIFICATION horizontal;
        MODEL_JUSTIFICATION vertical;
    };

    for( const JUSTIFICATION_CASE& expected :
         { JUSTIFICATION_CASE{ 5, MODEL_JUSTIFICATION::RIGHT, MODEL_JUSTIFICATION::RIGHT },
           { 7, MODEL_JUSTIFICATION::RIGHT, MODEL_JUSTIFICATION::RIGHT },
           { 15, MODEL_JUSTIFICATION::RIGHT, MODEL_JUSTIFICATION::RIGHT },
           { 6, MODEL_JUSTIFICATION::CENTER, MODEL_JUSTIFICATION::RIGHT },
           { 12, MODEL_JUSTIFICATION::CENTER, MODEL_JUSTIFICATION::RIGHT },
           { 0xFF04, MODEL_JUSTIFICATION::CENTER, MODEL_JUSTIFICATION::RIGHT },
           { 0x0306, MODEL_JUSTIFICATION::CENTER, MODEL_JUSTIFICATION::RIGHT } } )
    {
        std::vector<uint8_t> changedJustification = loadBinaryFixture( "text_encoding.sch" );
        writeU16( changedJustification, text.source.absoluteOffset + 18, expected.raw );
        PADS_SCH_MODEL justified = parser.Parse( changedJustification, wxS( "text_encoding.sch" ) );
        BOOST_REQUIRE_EQUAL( justified.texts.size(), 1 );
        BOOST_CHECK( justified.texts[0].presentation.horizontalJustification == expected.horizontal );
        BOOST_CHECK( justified.texts[0].presentation.verticalJustification == expected.vertical );
        BOOST_CHECK_EQUAL( justified.diagnostics.size(), model.diagnostics.size() );
    }

    struct FONT_CASE
    {
        uint32_t style;
        bool     bold;
        bool     italic;
    };

    for( const FONT_CASE& expected : { FONT_CASE{ 0, false, false }, FONT_CASE{ 1, false, true },
                                       FONT_CASE{ 2, true, false }, FONT_CASE{ 3, true, true } } )
    {
        std::vector<uint8_t> changedFont = loadBinaryFixture( "text_encoding.sch" );
        writeU16( changedFont, text.source.absoluteOffset, 0 );
        writeU32( changedFont, outerControllerOffset( changedFont, 19 ), expected.style );
        PADS_SCH_MODEL styled = parser.Parse( changedFont, wxS( "text_encoding.sch" ) );
        BOOST_REQUIRE_EQUAL( styled.texts.size(), 1 );
        BOOST_CHECK_EQUAL( styled.texts[0].presentation.bold, expected.bold );
        BOOST_CHECK_EQUAL( styled.texts[0].presentation.italic, expected.italic );
        BOOST_CHECK_EQUAL( styled.texts[0].presentation.width, 10 );
    }

    std::vector<uint8_t> hiddenText = loadBinaryFixture( "text_encoding.sch" );
    hiddenText[text.source.absoluteOffset + 31] = 1;
    PADS_SCH_MODEL hidden = parser.Parse( hiddenText, wxS( "text_encoding.sch" ) );
    BOOST_REQUIRE_EQUAL( hidden.texts.size(), 1 );
    BOOST_CHECK( !hidden.texts[0].presentation.visible );
    BOOST_CHECK_EQUAL( hidden.texts[0].presentation.width, 10 );
}


BOOST_AUTO_TEST_CASE( TextOptionMatrix )
{
    PADS_SCH_BINARY_PARSER parser;
    PADS_SCH_MODEL         model = parser.Parse( loadBinaryFixture( "text_options.sch" ), wxS( "text_options.sch" ) );

    BOOST_REQUIRE_EQUAL( model.texts.size(), 26 );

    auto textByName = [&]( const wxString& aName ) -> const MODEL_TEXT&
    {
        auto found = std::ranges::find_if( model.texts,
                                           [&]( const MODEL_TEXT& aText )
                                           {
                                               return aText.text.text == aName;
                                           } );
        BOOST_REQUIRE( found != model.texts.end() );
        return *found;
    };

    const std::array<MODEL_JUSTIFICATION, 16> horizontal = { MODEL_JUSTIFICATION::LEFT,   MODEL_JUSTIFICATION::RIGHT,
                                                             MODEL_JUSTIFICATION::LEFT,   MODEL_JUSTIFICATION::RIGHT,
                                                             MODEL_JUSTIFICATION::CENTER, MODEL_JUSTIFICATION::RIGHT,
                                                             MODEL_JUSTIFICATION::CENTER, MODEL_JUSTIFICATION::RIGHT,
                                                             MODEL_JUSTIFICATION::LEFT,   MODEL_JUSTIFICATION::RIGHT,
                                                             MODEL_JUSTIFICATION::CENTER, MODEL_JUSTIFICATION::RIGHT,
                                                             MODEL_JUSTIFICATION::CENTER, MODEL_JUSTIFICATION::RIGHT,
                                                             MODEL_JUSTIFICATION::CENTER, MODEL_JUSTIFICATION::RIGHT };

    for( size_t ii = 0; ii < horizontal.size(); ++ii )
    {
        const MODEL_TEXT& text = textByName( wxString::Format( wxS( "JUST_%02zu" ), ii ) );
        BOOST_CHECK( text.presentation.horizontalJustification == horizontal[ii] );
        BOOST_CHECK( text.presentation.verticalJustification == MODEL_JUSTIFICATION::RIGHT );
    }

    for( const auto& [name, angle] : { std::pair{ wxS( "ANGLE_000" ), 0 }, std::pair{ wxS( "ANGLE_090" ), 900 },
                                       std::pair{ wxS( "ANGLE_180" ), 1800 }, std::pair{ wxS( "ANGLE_270" ), 2700 } } )
    {
        BOOST_CHECK_EQUAL( textByName( name ).angle, angle );
    }

    BOOST_CHECK( !textByName( wxS( "STYLE_REGULAR" ) ).presentation.bold );
    BOOST_CHECK( !textByName( wxS( "STYLE_REGULAR" ) ).presentation.italic );
    BOOST_CHECK( textByName( wxS( "STYLE_BOLD" ) ).presentation.bold );
    BOOST_CHECK( textByName( wxS( "STYLE_ITALIC" ) ).presentation.italic );
    BOOST_CHECK( textByName( wxS( "STYLE_BOLD_ITALIC" ) ).presentation.bold );
    BOOST_CHECK( !textByName( wxS( "STYLE_BOLD_ITALIC" ) ).presentation.italic );

    BOOST_CHECK( textByName( wxS( "VISIBLE_0" ) ).presentation.visible );
    BOOST_CHECK( !textByName( wxS( "VISIBLE_1" ) ).presentation.visible );
    BOOST_CHECK_EQUAL( textByName( wxS( "VISIBLE_0" ) ).presentation.width, 10 );
    BOOST_CHECK_EQUAL( textByName( wxS( "VISIBLE_1" ) ).presentation.width, 10 );
}


BOOST_AUTO_TEST_CASE( VisibilityAndUnderlineOptionMatrix )
{
    PADS_SCH_MODEL model = PADS_SCH_BINARY_PARSER().Parse( loadBinaryFixture( "visibility_options.sch" ),
                                                           wxS( "visibility_options.sch" ) );

    auto underlined = std::ranges::find( model.texts, wxS( "STYLE_UNDERLINE" ),
                                        []( const MODEL_TEXT& aText )
                                        {
                                            return aText.text.text;
                                        } );
    BOOST_REQUIRE( underlined != model.texts.end() );
    BOOST_CHECK( underlined->presentation.underline );
    BOOST_CHECK( !underlined->presentation.bold );
    BOOST_CHECK( !underlined->presentation.italic );
    BOOST_CHECK_EQUAL( underlined->presentation.font.text, wxS( "Verdana" ) );

    BOOST_REQUIRE_EQUAL( model.placements.size(), 32 );

    for( uint8_t flags = 0; flags < 32; ++flags )
    {
        const wxString reference = wxString::Format( wxS( "R%u" ), flags + 1 );
        auto placement = std::ranges::find( model.placements, reference,
                                            []( const MODEL_PLACEMENT& aPlacement )
                                            {
                                                return aPlacement.reference.text;
                                            } );
        BOOST_REQUIRE_MESSAGE( placement != model.placements.end(), reference );
        BOOST_CHECK_EQUAL( placement->itemVisibilityFlags, flags );
        BOOST_CHECK_EQUAL( placement->referenceVisible, ( flags & 0x01 ) == 0 );
        BOOST_CHECK_EQUAL( placement->partTypeVisible, ( flags & 0x02 ) == 0 );
        BOOST_CHECK_EQUAL( placement->pinNamesVisible, ( flags & 0x08 ) == 0 );
        BOOST_CHECK_EQUAL( placement->pinNumbersVisible, ( flags & 0x10 ) == 0 );
    }
}


BOOST_AUTO_TEST_CASE( TextEncodingWarnings )
{
    SOURCE_PROVENANCE              source{ wxS( "text_encoding.sch" ), 0x000D, wxS( "free text" ), 2, 0, 0x6250, 2, 0 };
    std::vector<PARSER_DIAGNOSTIC> diagnostics;
    SOURCE_STRING unknown = PADS_SCH_BINARY_PARSER::DecodeString( { 0x41, 0xE9 }, 932, source, diagnostics );
    SOURCE_STRING invalid = PADS_SCH_BINARY_PARSER::DecodeString( { 0xC3, 0x28 }, 65001, source, diagnostics );

    BOOST_CHECK( unknown.encoding == STRING_ENCODING_STATUS::UNKNOWN_CODE_PAGE );
    BOOST_CHECK_EQUAL( unknown.raw[1], 0xE9 );
    BOOST_CHECK_EQUAL( unknown.text[1].GetValue(), 0xFFFD );
    BOOST_CHECK( invalid.encoding == STRING_ENCODING_STATUS::INVALID_BYTES );
    BOOST_REQUIRE_EQUAL( diagnostics.size(), 2 );
    BOOST_CHECK_EQUAL( diagnostics[0].source.absoluteOffset, 0x6250 );
    BOOST_CHECK( diagnostics[0].message.Contains( wxS( "code page 932" ) ) );
    BOOST_CHECK( diagnostics[1].message.Contains( wxS( "invalid UTF-8" ) ) );
}


BOOST_AUTO_TEST_CASE( GlobalRecordCorruption )
{
    PADS_SCH_BINARY_PARSER parser;
    std::vector<uint8_t>   reordered = loadBinaryFixture( "multisheet_connectivity.sch" );
    size_t                 sheetIndex = outerControllerOffset( reordered, 3 );
    writeU16( reordered, sheetIndex + 8, 2 );
    writeU16( reordered, sheetIndex + 48 + 8, 1 );
    PADS_SCH_MODEL reorderedModel = parser.Parse( reordered, wxS( "reordered.sch" ) );
    BOOST_REQUIRE_EQUAL( reorderedModel.sheets.size(), 2u );
    BOOST_CHECK_EQUAL( reorderedModel.sheets[0].id.Value(), 2u );
    BOOST_CHECK_EQUAL( reorderedModel.sheets[0].index, 1u );
    BOOST_CHECK_EQUAL( reorderedModel.sheets[0].source.recordIndex, 0u );
    BOOST_CHECK_EQUAL( reorderedModel.sheets[1].id.Value(), 1u );
    BOOST_CHECK_EQUAL( reorderedModel.sheets[1].index, 0u );
    BOOST_CHECK_EQUAL( reorderedModel.sheets[1].source.recordIndex, 1u );

    std::vector<uint8_t> duplicate = loadBinaryFixture( "multisheet_connectivity.sch" );
    sheetIndex = outerControllerOffset( duplicate, 3 );
    writeU16( duplicate, sheetIndex + 48 + 8, 1 );
    BOOST_CHECK_EXCEPTION( parser.Parse( duplicate, wxS( "duplicate.sch" ) ), IO_ERROR,
                           []( const IO_ERROR& aError )
                           {
                               return aError.What().Contains( wxS( "duplicate sheet ID 1" ) )
                                      && aError.What().Contains( wxS( "controller 3" ) );
                           } );

    std::vector<uint8_t> wrongClassBytes = loadMinimalV13();
    sheetIndex = outerControllerOffset( wrongClassBytes, 3 );
    writeU32( wrongClassBytes, sheetIndex, static_cast<uint32_t>( outerControllerOffset( wrongClassBytes, 5 ) ) );
    BOOST_CHECK_EXCEPTION( parser.Parse( wrongClassBytes, wxS( "wrong-class.sch" ) ), IO_ERROR,
                           []( const IO_ERROR& aError )
                           {
                               return aError.What().Contains( wxS( "references the wrong SDB object class" ) )
                                      && aError.What().Contains( wxS( "controller 3" ) );
                           } );

    PADS_SCH_MODEL    wrongClass = parser.Parse( loadMinimalV13(), wxS( "wrong-class.sch" ) );
    SOURCE_PROVENANCE refSource{ wxS( "wrong-class.sch" ), 0x000D, wxS( "free text" ), 1, 0, 0x100, 32, 0 };
    wrongClass.texts.push_back( { refSource, { SHEET_ID( 99 ), refSource } } );
    BOOST_CHECK_EXCEPTION( wrongClass.ValidateOrThrow(), IO_ERROR,
                           []( const IO_ERROR& aError )
                           {
                               return aError.What().Contains( wxS( "unresolved text sheet reference" ) );
                           } );

    PADS_SCH_MODEL cycle = parser.Parse( loadMinimalV13(), wxS( "cycle.sch" ) );
    cycle.sheets[0].parent = SHEET_REFERENCE{ cycle.sheets[0].id, cycle.sheets[0].source };
    BOOST_CHECK_EXCEPTION( cycle.ValidateOrThrow(), IO_ERROR,
                           []( const IO_ERROR& aError )
                           {
                               return aError.What().Contains( wxS( "cyclic sheet hierarchy" ) );
                           } );

    PADS_SCH_MODEL longHierarchy;
    longHierarchy.version = 0x000D;
    constexpr size_t hierarchySize = 4096;
    longHierarchy.sheets.reserve( hierarchySize );

    for( size_t i = 0; i < hierarchySize; ++i )
    {
        SOURCE_PROVENANCE source{ wxS( "long-hierarchy.sch" ), 0x000D, wxS( "sheet" ), 3, i, 0x100 + i * 48, 48,
                                  static_cast<int>( i ) };
        MODEL_SHEET       sheet;
        sheet.id = SHEET_ID( i + 1 );
        sheet.index = i;
        sheet.source = source;

        if( i > 0 )
            sheet.parent = SHEET_REFERENCE{ SHEET_ID( i ), source };

        longHierarchy.sheets.push_back( std::move( sheet ) );
    }

    BOOST_CHECK_NO_THROW( longHierarchy.ValidateOrThrow() );
    longHierarchy.sheets[0].parent = SHEET_REFERENCE{ longHierarchy.sheets.back().id, longHierarchy.sheets[0].source };
    BOOST_CHECK_EXCEPTION( longHierarchy.ValidateOrThrow(), IO_ERROR,
                           []( const IO_ERROR& aError )
                           {
                               return aError.What().Contains( wxS( "cyclic sheet hierarchy" ) )
                                      && aError.What().Contains( wxS( "controller 3" ) );
                           } );
}


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
    BOOST_CHECK_EQUAL( first.sheets[0].id.Value(), 1 );
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
    BOOST_CHECK_EQUAL( first.diagnostics.back().source.absoluteOffset, first.sheets[0].source.absoluteOffset );

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
                                  GATE_REFERENCE{ GATE_ID( 4 ), source },
                                  { DEFINITION_ID( 1 ), source },
                                  { { PIN_ID( 2 ), source } } } );
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

    SOURCE_PROVENANCE topologySource{ wxS( "model.sch" ), 0x000D, wxS( "endpoint" ), 9, 4, 0x280, 16, 1 };
    PADS_SCH_MODEL    crossSheet = model;
    crossSheet.sheets.push_back( { SHEET_ID( 1 ), 1, topologySource } );
    crossSheet.nets[0].sheet = { SHEET_ID( 1 ), topologySource };
    crossSheet.nets[0].connections[0].endpoints[0].source = topologySource;
    const wxString endpointError =
            FormatParserError( topologySource, wxS( "connection endpoint placement sheet does not match net sheet" ) );
    BOOST_CHECK_EXCEPTION( crossSheet.ValidateOrThrow(), IO_ERROR,
                           [&]( const IO_ERROR& aError )
                           {
                               return aError.What().Contains( endpointError );
                           } );

    PADS_SCH_MODEL busModel = model;
    busModel.nets[0].connections.clear();
    busModel.buses.push_back( { BUS_ID( 7 ), source, { SHEET_ID( 0 ), source } } );
    busModel.buses[0].memberNets.push_back( { NET_ID( 6 ), source } );
    busModel.buses[0].entries.push_back( { topologySource, {}, { NET_ID( 6 ), topologySource } } );
    BOOST_CHECK_NO_THROW( busModel.ValidateOrThrow() );

    PADS_SCH_MODEL missingBusMember = busModel;
    missingBusMember.buses[0].memberNets.clear();
    const wxString membershipError =
            FormatParserError( topologySource, wxS( "bus-entry net is absent from bus member nets" ) );
    BOOST_CHECK_EXCEPTION( missingBusMember.ValidateOrThrow(), IO_ERROR,
                           [&]( const IO_ERROR& aError )
                           {
                               return aError.What().Contains( membershipError );
                           } );

    PADS_SCH_MODEL crossSheetBus = busModel;
    crossSheetBus.sheets.push_back( { SHEET_ID( 1 ), 1, topologySource } );
    crossSheetBus.buses[0].sheet = { SHEET_ID( 1 ), topologySource };
    const wxString busSheetError = FormatParserError( source, wxS( "bus member-net sheet does not match bus sheet" ) );
    BOOST_CHECK_EXCEPTION( crossSheetBus.ValidateOrThrow(), IO_ERROR,
                           [&]( const IO_ERROR& aError )
                           {
                               return aError.What().Contains( busSheetError );
                           } );
}


BOOST_AUTO_TEST_CASE( TypedPageGraphicOwnership )
{
    SOURCE_PROVENANCE source{ wxS( "graphics.sch" ), 0x000D, wxS( "page graphic" ), 5, 6, 0x420, 24, 0 };
    PADS_SCH_MODEL    model;
    model.source = source;
    model.sheets.push_back( { SHEET_ID( 4 ), 0, source } );
    model.graphics.push_back( MODEL_PAGE_GRAPHIC{ source, { SHEET_ID( 4 ), source }, { source } } );
    BOOST_CHECK_NO_THROW( model.ValidateOrThrow() );

    model.graphics[0].sheet.id = SHEET_ID( 99 );
    auto records = normalizeBinaryModel( model );
    auto record = std::ranges::find_if( records,
                                        []( const CANONICAL_SEMANTIC_RECORD& aRecord )
                                        {
                                            return aRecord.kind == CANONICAL_KIND::GRAPHIC;
                                        } );
    BOOST_REQUIRE( record != records.end() );
    BOOST_CHECK( record->properties.at( "owner_sheet" ).disposition == PROPERTY_DISPOSITION::UNSUPPORTED );
    const wxString error = FormatParserError( source, wxS( "unresolved page-graphic sheet reference" ) );
    BOOST_CHECK_EXCEPTION( model.ValidateOrThrow(), IO_ERROR,
                           [&]( const IO_ERROR& aError )
                           {
                               return aError.What().Contains( error );
                           } );
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

    SOURCE_STRING emptyUnknown = PADS_SCH_BINARY_PARSER::DecodeString( {}, 932, source, diagnostics );
    BOOST_CHECK( emptyUnknown.encoding == STRING_ENCODING_STATUS::UNKNOWN_CODE_PAGE );
    BOOST_REQUIRE_EQUAL( diagnostics.size(), 3 );
    BOOST_CHECK( diagnostics[2].message.Contains( wxS( "932" ) ) );
}


BOOST_AUTO_TEST_CASE( SemanticSnapshotAdapterContract )
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

    std::vector<CANONICAL_SEMANTIC_RECORD> expected = normalizeBinaryModel( binary );
    std::vector<CANONICAL_SEMANTIC_RECORD> actual = expected;
    std::reverse( actual.begin(), actual.end() );

    for( CANONICAL_SEMANTIC_RECORD& item : actual )
    {
        if( item.properties.contains( "font" ) )
            item.properties.at( "font" ).value = std::string( "Arial" );

        if( item.properties.contains( "color" ) )
            item.properties.at( "color" ).value = std::string( "red" );
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
    definition.fields.push_back( { FIELD_ID( 10 ), source, string( wxS( "Value" ) ), string( wxS( "R" ) ) } );
    model.definitions.push_back( definition );

    MODEL_PART_TYPE partType;
    partType.id = PART_TYPE_ID( 4 );
    partType.source = source;
    partType.name = string( wxS( "RES" ) );
    partType.gates.push_back(
            { GATE_ID( 5 ), source, { DEFINITION_ID( 2 ), source }, 1, { { PIN_ID( 3 ), source } } } );
    partType.fields.push_back( { FIELD_ID( 11 ), source, string( wxS( "Tolerance" ) ), string( wxS( "1%" ) ) } );
    model.partTypes.push_back( partType );

    MODEL_PLACEMENT placement;
    placement.id = PLACEMENT_ID( 6 );
    placement.source = source;
    placement.reference = string( wxS( "R1" ) );
    placement.fields.push_back( { FIELD_ID( 12 ), source, string( wxS( "Value" ) ), string( wxS( "10k" ) ) } );
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
    model.graphics.push_back( { source, { SHEET_ID( 1 ), source }, { source } } );

    std::vector<CANONICAL_SEMANTIC_RECORD> binaryRecords = normalizeBinaryModel( model );
    std::set<CANONICAL_KIND>               binaryKinds;

    for( const CANONICAL_SEMANTIC_RECORD& item : binaryRecords )
        binaryKinds.insert( item.kind );

    const std::set<CANONICAL_KIND> expectedKinds{
        CANONICAL_KIND::SETTINGS,   CANONICAL_KIND::SHEET,     CANONICAL_KIND::DEFINITION,
        CANONICAL_KIND::GRAPHIC,    CANONICAL_KIND::PIN,       CANONICAL_KIND::FIELD,
        CANONICAL_KIND::PART_TYPE,  CANONICAL_KIND::GATE,      CANONICAL_KIND::GATE_PIN_MAPPING,
        CANONICAL_KIND::PLACEMENT,  CANONICAL_KIND::NET,       CANONICAL_KIND::CONNECTION,
        CANONICAL_KIND::BUS,        CANONICAL_KIND::BUS_ENTRY, CANONICAL_KIND::BUS_ALIAS,
        CANONICAL_KIND::BUS_MEMBER, CANONICAL_KIND::LABEL,     CANONICAL_KIND::JUNCTION,
        CANONICAL_KIND::TEXT
    };
    BOOST_CHECK( binaryKinds == expectedKinds );

    const std::string         fixtureRoot = KI_TEST::GetEeschemaTestDataDir() + "/plugins/pads/binary/";
    PADS_SCH::PADS_SCH_PARSER asciiParser;
    BOOST_REQUIRE( asciiParser.Parse( fixtureRoot + "minimal_v13.txt" ) );
    std::vector<CANONICAL_SEMANTIC_RECORD> asciiRecords = normalizeAsciiModel( asciiParser );
    BOOST_CHECK( std::ranges::any_of( asciiRecords,
                                      []( const CANONICAL_SEMANTIC_RECORD& aItem )
                                      {
                                          return aItem.kind == CANONICAL_KIND::SETTINGS;
                                      } ) );
    BOOST_CHECK( std::ranges::any_of( asciiRecords,
                                      []( const CANONICAL_SEMANTIC_RECORD& aItem )
                                      {
                                          return aItem.kind == CANONICAL_KIND::SHEET;
                                      } ) );

    PADS_SCH_BINARY_PARSER                 binaryParser;
    std::vector<CANONICAL_SEMANTIC_RECORD> minimalBinary =
            normalizeBinaryModel( binaryParser.Parse( loadMinimalV13(), wxS( "minimal_v13.sch" ) ) );
    auto countKind = []( const std::vector<CANONICAL_SEMANTIC_RECORD>& aItems, CANONICAL_KIND aKind )
    {
        return std::ranges::count_if( aItems,
                                      [&]( const CANONICAL_SEMANTIC_RECORD& aItem )
                                      {
                                          return aItem.kind == aKind;
                                      } );
    };
    BOOST_CHECK_EQUAL( countKind( minimalBinary, CANONICAL_KIND::SETTINGS ),
                       countKind( asciiRecords, CANONICAL_KIND::SETTINGS ) );
    BOOST_CHECK_EQUAL( countKind( minimalBinary, CANONICAL_KIND::SHEET ),
                       countKind( asciiRecords, CANONICAL_KIND::SHEET ) );
}


BOOST_AUTO_TEST_CASE( HandbuiltBusAdapterParity )
{
    const std::string         fixture = KI_TEST::GetEeschemaTestDataDir() + "/plugins/pads/buses.txt";
    PADS_SCH::PADS_SCH_PARSER asciiParser;
    BOOST_REQUIRE( asciiParser.Parse( fixture ) );

    SOURCE_PROVENANCE source{ wxS( "buses.sch" ), 0x000D, wxS( "synthetic" ), 1, 0, 0, 0, 0 };
    auto              string = [&]( const wxString& aText )
    {
        SOURCE_STRING result;
        result.text = aText;
        result.source = source;
        return result;
    };

    PADS_SCH_MODEL binary;
    binary.settings.coordinateUnitsPerMil = 2;
    binary.settings.pageSize = { 34000, 22000, source };
    binary.settings.defaultLineWidth = 20;
    binary.settings.defaultBusWidth = 50;

    MODEL_SHEET sheet{ SHEET_ID( 1 ), 0, source, string( wxS( "Main" ) ) };
    sheet.pageSize = binary.settings.pageSize;
    sheet.defaultLineWidth = 20;
    sheet.defaultBusWidth = 50;
    binary.sheets.push_back( sheet );

    for( int i = 0; i < 3; ++i )
    {
        MODEL_NET net;
        net.id = NET_ID( i + 1 );
        net.source = source;
        net.sheet = { SHEET_ID( 1 ), source };
        net.name = string( wxString::Format( wxS( "DATA%d" ), i ) );
        binary.nets.push_back( net );
    }

    MODEL_BUS bus;
    bus.id = BUS_ID( 2 );
    bus.source = source;
    bus.sheet = { SHEET_ID( 1 ), source };
    bus.name = string( wxS( "DATA[0..2]" ) );
    bus.vertices = { { 6000, 14000, source }, { 14000, 14000, source }, { 14000, 10000, source } };
    bus.aliases.push_back( string( wxS( "DATA[0..2]" ) ) );

    for( int i = 0; i < 3; ++i )
    {
        bus.entries.push_back( { source, { 8000 + i * 2000, 14000, source }, { NET_ID( i + 1 ), source } } );
        bus.memberNets.push_back( { NET_ID( i + 1 ), source } );
    }

    binary.buses.push_back( bus );
    BOOST_CHECK( snapshotsMatch( normalizeBinaryModel( binary ), normalizeAsciiModel( asciiParser ) ) );

    std::vector<CANONICAL_SEMANTIC_RECORD> records = normalizeBinaryModel( binary );
    auto                                   entry = std::ranges::find_if( records,
                                                                         []( const CANONICAL_SEMANTIC_RECORD& aRecord )
                                                                         {
                                           return aRecord.kind == CANONICAL_KIND::BUS_ENTRY;
                                       } );
    BOOST_REQUIRE( entry != records.end() );
    BOOST_CHECK( entry->properties.at( "member_net" ).value == CANONICAL_VALUE( std::string( "DATA0" ) ) );
    auto busRecord = std::ranges::find_if( records,
                                           []( const CANONICAL_SEMANTIC_RECORD& aRecord )
                                           {
                                               return aRecord.kind == CANONICAL_KIND::BUS;
                                           } );
    BOOST_REQUIRE( busRecord != records.end() );
    BOOST_CHECK( busRecord->properties.at( "member_nets" ).value
                 == CANONICAL_VALUE( std::vector<std::string>{ "DATA0", "DATA1", "DATA2" } ) );

    PADS_SCH_MODEL wrongMapping = binary;
    wrongMapping.buses[0].entries[0].memberNet.id = NET_ID( 2 );
    BOOST_CHECK( !snapshotsMatch( normalizeBinaryModel( wrongMapping ), normalizeAsciiModel( asciiParser ) ) );

    PADS_SCH_MODEL missingMember = binary;
    missingMember.buses[0].memberNets.pop_back();
    BOOST_CHECK( !snapshotsMatch( normalizeBinaryModel( missingMember ), normalizeAsciiModel( asciiParser ) ) );
}


BOOST_AUTO_TEST_CASE( CanonicalEnumMappings )
{
    BOOST_CHECK( canonicalGraphicType( MODEL_GRAPHIC_KIND::RECTANGLE ).value
                 == CANONICAL_VALUE( std::string( "rectangle" ) ) );
    BOOST_CHECK( canonicalGraphicType( MODEL_GRAPHIC_KIND::CIRCLE ).value
                 == CANONICAL_VALUE( std::string( "circle" ) ) );
    BOOST_CHECK( canonicalGraphicType( MODEL_GRAPHIC_KIND::ARC ).value == CANONICAL_VALUE( std::string( "arc" ) ) );
    BOOST_CHECK( canonicalGraphicType( MODEL_GRAPHIC_KIND::POLYLINE ).value
                 == CANONICAL_VALUE( std::string( "polyline" ) ) );
    BOOST_CHECK( canonicalGraphicType( PADS_SCH::GRAPHIC_TYPE::RECTANGLE ).value
                 == CANONICAL_VALUE( std::string( "rectangle" ) ) );
    BOOST_CHECK( canonicalGraphicType( PADS_SCH::GRAPHIC_TYPE::CIRCLE ).value
                 == CANONICAL_VALUE( std::string( "circle" ) ) );
    BOOST_CHECK( canonicalGraphicType( PADS_SCH::GRAPHIC_TYPE::ARC ).value == CANONICAL_VALUE( std::string( "arc" ) ) );
    BOOST_CHECK( canonicalGraphicType( PADS_SCH::GRAPHIC_TYPE::POLYLINE ).value
                 == CANONICAL_VALUE( std::string( "polyline" ) ) );
    BOOST_CHECK( canonicalLineStyle( MODEL_LINE_STYLE::DEFAULT ).value == CANONICAL_VALUE( std::string( "solid" ) ) );
    BOOST_CHECK( canonicalLineStyle( 255 ).value == CANONICAL_VALUE( std::string( "solid" ) ) );
    BOOST_CHECK( canonicalLineStyle( 0 ).value == CANONICAL_VALUE( std::string( "dash" ) ) );
    BOOST_CHECK( canonicalLineStyle( 42 ).disposition == PROPERTY_DISPOSITION::UNSUPPORTED );
    BOOST_CHECK( canonicalFill( MODEL_FILL_STYLE::HATCHED ).value == CANONICAL_VALUE( std::string( "hatched" ) ) );
    BOOST_CHECK( canonicalFill( true ).value == CANONICAL_VALUE( std::string( "filled" ) ) );
    BOOST_CHECK( canonicalPinType( uint32_t( 3 ) ).value == CANONICAL_VALUE( std::string( "bidirectional" ) ) );
    BOOST_CHECK( canonicalPinType( PADS_SCH::PIN_TYPE::BIDIRECTIONAL ).value
                 == CANONICAL_VALUE( std::string( "bidirectional" ) ) );
    BOOST_CHECK( canonicalPinStyle( 3 ).value == CANONICAL_VALUE( std::string( "inverted_clock" ) ) );
    BOOST_CHECK( canonicalPinStyle( true, true ).value == CANONICAL_VALUE( std::string( "inverted_clock" ) ) );
    BOOST_CHECK( canonicalJustification( MODEL_JUSTIFICATION::CENTER ).value
                 == CANONICAL_VALUE( std::string( "center" ) ) );
    BOOST_CHECK( canonicalHorizontalJustification( 12 ).value == CANONICAL_VALUE( std::string( "center" ) ) );
    BOOST_CHECK( canonicalVerticalJustification( 12 ).value == CANONICAL_VALUE( std::string( "center" ) ) );
    BOOST_CHECK( canonicalLabelKind( MODEL_LABEL_KIND::HIERARCHICAL ).value
                 == CANONICAL_VALUE( std::string( "hierarchical" ) ) );
    BOOST_CHECK( canonicalEndpointKind( MODEL_ENDPOINT_KIND::PIN ).value == CANONICAL_VALUE( std::string( "pin" ) ) );

    PADS_SCH::PADS_SCH_PARSER parser;
    BOOST_REQUIRE( parser.Parse( KI_TEST::GetEeschemaTestDataDir() + "/plugins/pads/binary/multigate.txt" ) );
    const PADS_SCH::SYMBOL_GRAPHIC* serializedGraphic = nullptr;

    for( const PADS_SCH::SYMBOL_DEF& definition : parser.GetSymbolDefs() )
    {
        auto graphic = std::ranges::find_if( definition.graphics,
                                             []( const PADS_SCH::SYMBOL_GRAPHIC& aGraphic )
                                             {
                                                 return aGraphic.line_style == 255;
                                             } );

        if( graphic != definition.graphics.end() )
        {
            serializedGraphic = &*graphic;
            break;
        }
    }

    BOOST_REQUIRE( serializedGraphic );
    BOOST_CHECK( canonicalLineStyle( serializedGraphic->line_style )
                 == canonicalLineStyle( MODEL_LINE_STYLE::DEFAULT ) );
}


BOOST_AUTO_TEST_CASE( GatePinMappingsUseSemanticPinIdentity )
{
    SOURCE_PROVENANCE source{ wxS( "mapping.sch" ), 0x000D, wxS( "synthetic" ), 1, 0, 0, 0, 0 };
    auto              string = [&]( const wxString& aText )
    {
        SOURCE_STRING result;
        result.text = aText;
        result.source = source;
        return result;
    };

    PADS_SCH_MODEL          model;
    MODEL_SYMBOL_DEFINITION definition;
    definition.id = DEFINITION_ID( 1 );
    definition.source = source;
    definition.pins.push_back( { PIN_ID( 91 ), source, string( wxS( "A1" ) ), string( wxS( "CLK" ) ) } );
    model.definitions.push_back( definition );

    MODEL_PART_TYPE part;
    part.source = source;
    part.gates.push_back( { GATE_ID( 2 ), source, { definition.id, source }, 1, { { PIN_ID( 91 ), source } } } );
    model.partTypes.push_back( part );

    const auto records = normalizeBinaryModel( model );
    auto       mapping = std::ranges::find_if( records,
                                               []( const CANONICAL_SEMANTIC_RECORD& aRecord )
                                               {
                                             return aRecord.kind == CANONICAL_KIND::GATE_PIN_MAPPING;
                                         } );
    BOOST_REQUIRE( mapping != records.end() );
    BOOST_CHECK( mapping->properties.at( "pin_number" ).value == CANONICAL_VALUE( std::string( "A1" ) ) );
    BOOST_CHECK( mapping->properties.at( "pin_name" ).value == CANONICAL_VALUE( std::string( "CLK" ) ) );
    BOOST_CHECK( !mapping->properties.contains( "pin_id" ) );
}


BOOST_AUTO_TEST_CASE( SymbolPrimitives )
{
    PADS_SCH_BINARY_PARSER parser;
    PADS_SCH_MODEL model = parser.Parse( loadBinaryFixture( "symbol_primitives.sch" ), wxS( "symbol_primitives.sch" ) );
    const MODEL_SYMBOL_DEFINITION& definition = itemNamed( model.definitions, wxS( "BATCHB_PRIMITIVES" ) );

    BOOST_REQUIRE_EQUAL( definition.graphics.size(), 6 );
    BOOST_CHECK( definition.graphics[0].kind == MODEL_GRAPHIC_KIND::LINE );
    BOOST_REQUIRE_EQUAL( definition.graphics[0].points.size(), 2 );
    BOOST_CHECK_EQUAL( definition.graphics[0].points[0].x, 200 );
    BOOST_CHECK_EQUAL( definition.graphics[0].points[0].y, 200 );
    BOOST_CHECK_EQUAL( definition.graphics[0].points[1].x, 800 );
    BOOST_CHECK_EQUAL( definition.graphics[0].points[1].y, 200 );
    BOOST_CHECK( definition.graphics[1].kind == MODEL_GRAPHIC_KIND::POLYLINE );
    BOOST_CHECK( definition.graphics[2].kind == MODEL_GRAPHIC_KIND::CIRCLE );
    BOOST_CHECK( definition.graphics[3].kind == MODEL_GRAPHIC_KIND::ARC );
    BOOST_CHECK_EQUAL( definition.graphics[3].strokeWidth, 20 );
    BOOST_CHECK( definition.graphics[3].lineStyle == MODEL_LINE_STYLE::SOLID );
    BOOST_CHECK_EQUAL( definition.graphics[3].arcSweepAngle, 1800 );
    BOOST_CHECK( definition.graphics[3].arcClockwise );
    BOOST_CHECK_EQUAL( definition.graphics[3].arcCenter.x, 1300 );
    BOOST_CHECK_EQUAL( definition.graphics[3].arcCenter.y, 1100 );
    BOOST_CHECK_EQUAL( definition.graphics[3].arcBoundsStart.x, 1000 );
    BOOST_CHECK_EQUAL( definition.graphics[3].arcBoundsStart.y, 800 );
    BOOST_CHECK_EQUAL( definition.graphics[3].arcBoundsEnd.x, 1600 );
    BOOST_CHECK_EQUAL( definition.graphics[3].arcBoundsEnd.y, 1400 );
    BOOST_CHECK( definition.graphics[4].fill == MODEL_FILL_STYLE::FILLED );
    BOOST_CHECK( definition.graphics[5].kind == MODEL_GRAPHIC_KIND::TEXT );
    BOOST_CHECK_EQUAL( definition.graphics[5].text.text, wxS( "EMBEDDED_TEXT" ) );
    BOOST_CHECK_EQUAL( definition.graphics[5].presentation.font.text, wxS( "Default Font" ) );
    BOOST_REQUIRE_EQUAL( definition.graphics[5].points.size(), 1 );
    BOOST_CHECK_EQUAL( definition.graphics[5].points[0].x, 1000 );
    BOOST_CHECK_EQUAL( definition.graphics[5].points[0].y, 1600 );
    BOOST_CHECK_EQUAL( definition.graphics[5].presentation.height, 100 );
    BOOST_CHECK_EQUAL( definition.graphics[5].presentation.width, 10 );
    BOOST_CHECK_EQUAL( definition.graphics[5].angle, 0 );
    BOOST_REQUIRE_EQUAL( definition.pins.size(), 2 );
    BOOST_CHECK_EQUAL( definition.pins[0].position.x, 0 );
    BOOST_CHECK_EQUAL( definition.pins[0].position.y, 600 );
    BOOST_CHECK_EQUAL( definition.pins[1].position.x, 2400 );
    BOOST_CHECK_EQUAL( definition.pins[1].position.y, 600 );

    std::vector<uint8_t> privateStroke = loadBinaryFixture( "symbol_primitives.sch" );
    const size_t         privatePiece =
            sheetControllerOffset( privateStroke, 4 ) + definition.graphics[0].source.recordIndex * 6;
    writeU16( privateStroke, privatePiece + 4, 0xEF03 );
    PADS_SCH_MODEL                 preservedStroke = parser.Parse( privateStroke, wxS( "private-symbol-stroke.sch" ) );
    const MODEL_SYMBOL_DEFINITION& preservedDefinition =
            itemNamed( preservedStroke.definitions, wxS( "BATCHB_PRIMITIVES" ) );
    auto privateGraphic = std::ranges::find_if( preservedDefinition.graphics,
                                                [&]( const MODEL_GRAPHIC& aGraphic )
                                                {
                                                    return aGraphic.source.controller == 4
                                                           && aGraphic.source.recordIndex
                                                                      == definition.graphics[0].source.recordIndex;
                                                } );
    BOOST_REQUIRE( privateGraphic != preservedDefinition.graphics.end() );
    BOOST_CHECK_EQUAL( privateGraphic->strokeWidth, 0 );
    auto rawStroke = std::ranges::find_if( privateGraphic->properties,
                                           []( const SOURCE_PROPERTY& aProperty )
                                           {
                                               return aProperty.name.text == wxS( "unsupported_graphic_stroke_width" );
                                           } );
    BOOST_REQUIRE( rawStroke != privateGraphic->properties.end() );
    BOOST_CHECK_EQUAL( rawStroke->value.text, wxS( "3" ) );
    BOOST_CHECK( rawStroke->disposition == PROPERTY_DISPOSITION::UNSUPPORTED );
    BOOST_CHECK_EQUAL( rawStroke->source.absoluteOffset, privatePiece + 4 );
    BOOST_CHECK_EQUAL( rawStroke->source.length, 1 );
    BOOST_CHECK_EQUAL( propertyValue( privateGraphic->properties, wxS( "preserved_graphic_presentation" ) ),
                       wxS( "239" ) );
}


BOOST_AUTO_TEST_CASE( PartPinsAndGates )
{
    PADS_SCH_BINARY_PARSER parser;
    PADS_SCH_MODEL         pins = parser.Parse( loadBinaryFixture( "pin_styles.sch" ), wxS( "pin_styles.sch" ) );
    const MODEL_PART_TYPE& pinPart = itemNamed( pins.partTypes, wxS( "BATCHB-PIN-STYLES" ) );
    BOOST_REQUIRE_EQUAL( pinPart.gates.size(), 1 );
    BOOST_REQUIRE_EQUAL( pinPart.gates[0].pins.size(), 7 );

    const MODEL_SYMBOL_DEFINITION& pinDefinition = itemNamed( pins.definitions, wxS( "BATCHB_PIN_STYLES" ) );
    BOOST_REQUIRE_EQUAL( pinDefinition.pins.size(), 7 );
    BOOST_CHECK_EQUAL( pinDefinition.pins[0].number.text, wxS( "1" ) );
    BOOST_CHECK_EQUAL( pinDefinition.pins[0].name.text, wxS( "INPUT_PIN" ) );
    BOOST_CHECK_EQUAL( pinDefinition.pins[0].electricalType, 1 );
    BOOST_CHECK_EQUAL( pinDefinition.pins[1].electricalType, 2 );
    BOOST_CHECK_EQUAL( pinDefinition.pins[4].graphicStyle, 1 );
    BOOST_CHECK_EQUAL( pinDefinition.pins[5].graphicStyle, 2 );
    BOOST_CHECK( !pinDefinition.pins[6].presentation.visible );
    BOOST_CHECK_EQUAL( pinDefinition.pins[4].side, 1 );
    BOOST_CHECK_EQUAL( pinDefinition.pins[4].angle, 0 );
    BOOST_CHECK_EQUAL( pinDefinition.pins[4].length, 280 );
    BOOST_CHECK_EQUAL( propertyValue( pinDefinition.pins[0].properties, wxS( "pin_name_height_half_mils" ) ),
                       wxS( "100" ) );
    BOOST_CHECK_EQUAL( propertyValue( pinDefinition.pins[0].properties, wxS( "pin_number_height_half_mils" ) ),
                       wxS( "100" ) );
    BOOST_CHECK_EQUAL( pinDefinition.pins[0].namePresentation.width, 10 );
    BOOST_CHECK_EQUAL( pinDefinition.pins[0].numberPresentation.width, 10 );
    BOOST_CHECK_EQUAL( pinDefinition.pins[0].nameOffset.x, -200 );
    BOOST_CHECK_EQUAL( pinDefinition.pins[0].nameOffset.y, -100 );
    BOOST_CHECK_EQUAL( pinDefinition.pins[0].numberOffset.x, 500 );
    BOOST_CHECK_EQUAL( pinDefinition.pins[0].numberOffset.y, -100 );
    BOOST_CHECK( pinDefinition.pins[2].presentation.visible );
    BOOST_CHECK( !pinDefinition.pins[2].namePresentation.visible );
    BOOST_CHECK( !pinDefinition.pins[6].namePresentation.visible );
    BOOST_CHECK( pinDefinition.pins[6].numberPresentation.visible );
    BOOST_REQUIRE_EQUAL( pinPart.signalPins.size(), 1 );
    BOOST_CHECK_EQUAL( pinPart.signalPins[0].number.text, wxS( "8" ) );
    BOOST_CHECK_EQUAL( pinPart.signalPins[0].name.text, wxS( "VCC_HIDDEN" ) );

    PADS_SCH_MODEL         multi = parser.Parse( loadBinaryFixture( "multigate.sch" ), wxS( "multigate.sch" ) );
    const MODEL_PART_TYPE& multiPart = itemNamed( multi.partTypes, wxS( "BATCHD-MULTIGATE" ) );
    BOOST_REQUIRE_EQUAL( multiPart.gates.size(), 2 );
    BOOST_CHECK_EQUAL( multiPart.gates[0].unit, 1 );
    BOOST_CHECK_EQUAL( multiPart.gates[0].pins.size(), 7 );
    BOOST_CHECK_EQUAL( multiPart.gates[1].unit, 2 );
    BOOST_CHECK_EQUAL( multiPart.gates[1].pins.size(), 2 );
    BOOST_CHECK_EQUAL( multiPart.gates[0].properties.front().name.text, wxS( "swap_group" ) );
    const MODEL_PART_TYPE& alternatePart = itemNamed( multi.partTypes, wxS( "RES-RESN1" ) );
    BOOST_REQUIRE_EQUAL( alternatePart.gates[0].alternateDefinitions.size(), 3 );
    BOOST_REQUIRE_EQUAL( multiPart.signalPins.size(), 1 );
    BOOST_CHECK_EQUAL( multiPart.signalPins[0].number.text, wxS( "15" ) );

    PADS_SCH_MODEL connectorModel = parser.Parse( loadBinaryFixture( "connectors.sch" ), wxS( "connectors.sch" ) );
    const MODEL_PART_TYPE& connector = itemNamed( connectorModel.partTypes, wxS( "CON-26P-ED" ) );
    BOOST_REQUIRE_EQUAL( connector.gates.size(), 1 );
    BOOST_REQUIRE_EQUAL( connector.gates[0].decalGroupMembers.size(), 5 );
    BOOST_REQUIRE_EQUAL( connector.gates[0].connectorPins.size(), 26 );

    for( const DEFINITION_REFERENCE& member : connector.gates[0].decalGroupMembers )
    {
        BOOST_CHECK( member.id.IsValid() );
        BOOST_CHECK_EQUAL( member.source.controller, 10 );
    }

    for( size_t pin = 0; pin < connector.gates[0].connectorPins.size(); ++pin )
    {
        const MODEL_CONNECTOR_PIN& connectorPin = connector.gates[0].connectorPins[pin];
        BOOST_CHECK_EQUAL( connectorPin.number.text, wxString::Format( wxS( "%llu" ), pin + 1 ) );
        BOOST_CHECK_EQUAL( connectorPin.name.text, wxString() );
        BOOST_CHECK_EQUAL( connectorPin.electricalType, 0 );
        BOOST_CHECK_EQUAL( connectorPin.swapGroup, 0 );
        BOOST_CHECK_EQUAL( connectorPin.flags, 0 );
        BOOST_CHECK_EQUAL( connectorPin.source.controller, 11 );
        BOOST_CHECK_EQUAL( connectorPin.source.length, 24 );
        BOOST_CHECK_EQUAL( connectorPin.number.source.controller, 11 );
        BOOST_CHECK_EQUAL( connectorPin.number.source.absoluteOffset, connectorPin.source.absoluteOffset + 4 );
        BOOST_CHECK_EQUAL( connectorPin.number.source.length, 16 );
    }
}


BOOST_AUTO_TEST_CASE( DefinitionFields )
{
    const FIELD_ID controller7Record11214 = MakeFieldId( FIELD_ID_DOMAIN::DEFINITION, 11214, 0 );
    const FIELD_ID controller7Record12125 = MakeFieldId( FIELD_ID_DOMAIN::DEFINITION, 12125, 0 );
    BOOST_CHECK( controller7Record11214.IsValid() );
    BOOST_CHECK( controller7Record12125.IsValid() );
    BOOST_CHECK_NE( controller7Record11214.Value(), controller7Record12125.Value() );

    PADS_SCH_BINARY_PARSER         parser;
    PADS_SCH_MODEL                 model = parser.Parse( loadBinaryFixture( "fields.sch" ), wxS( "fields.sch" ) );
    const MODEL_SYMBOL_DEFINITION& definition = itemNamed( model.definitions, wxS( "RESZ-H" ) );
    BOOST_REQUIRE_EQUAL( definition.fields.size(), 4 );
    std::set<uint64_t> fieldIds;

    for( size_t ordinal = 0; ordinal < definition.fields.size(); ++ordinal )
    {
        const MODEL_FIELD& field = definition.fields[ordinal];
        BOOST_CHECK( field.id.IsValid() );
        BOOST_CHECK_EQUAL( field.id.Value(),
                           MakeFieldId( FIELD_ID_DOMAIN::DEFINITION, definition.id.Value(), ordinal ).Value() );
        BOOST_CHECK( fieldIds.insert( field.id.Value() ).second );
    }
    BOOST_CHECK_EQUAL( definition.fields[0].name.text, wxS( "REF-DES" ) );
    BOOST_CHECK_EQUAL( definition.fields[1].name.text, wxS( "PART-TYPE" ) );
    BOOST_CHECK_EQUAL( definition.fields[2].name.text, wxS( "VALUE" ) );
    BOOST_CHECK_EQUAL( definition.fields[3].name.text, wxS( "*" ) );
    BOOST_CHECK_EQUAL( definition.fields[0].presentation.font.text, wxS( "Default Font" ) );
    BOOST_CHECK_EQUAL( definition.fields[0].presentation.height, 100 );
    BOOST_CHECK_EQUAL( definition.fields[0].presentation.width, 10 );
    BOOST_CHECK_EQUAL( definition.fields[0].position.x, 600 );
    BOOST_CHECK_EQUAL( definition.fields[0].position.y, 200 );
    BOOST_CHECK_EQUAL( definition.fields[1].position.x, 620 );
    BOOST_CHECK_EQUAL( definition.fields[1].position.y, 400 );
    BOOST_CHECK( definition.fields[0].value.text.empty() );
    BOOST_CHECK_EQUAL( propertyValue( definition.fields[0].presentation.properties, wxS( "font_handle" ) ),
                       wxS( "-1" ) );
    BOOST_CHECK_EQUAL( propertyValue( definition.fields[1].presentation.properties, wxS( "font_handle" ) ),
                       wxS( "-1" ) );
    BOOST_CHECK_EQUAL( propertyValue( definition.fields[2].presentation.properties, wxS( "font_handle" ) ),
                       wxS( "-4" ) );
    BOOST_CHECK_EQUAL( propertyValue( definition.fields[3].presentation.properties, wxS( "font_handle" ) ),
                       wxS( "-4" ) );
    BOOST_CHECK( definition.fields[0].presentation.horizontalJustification == MODEL_JUSTIFICATION::LEFT );
    BOOST_CHECK( definition.fields[0].presentation.verticalJustification == MODEL_JUSTIFICATION::LEFT );
    BOOST_CHECK( definition.fields[2].presentation.horizontalJustification == MODEL_JUSTIFICATION::CENTER );
    BOOST_CHECK( definition.fields[2].presentation.verticalJustification == MODEL_JUSTIFICATION::LEFT );

    const MODEL_PART_TYPE& partType = itemNamed( model.partTypes, wxS( "RES-RESN1" ) );
    BOOST_REQUIRE_EQUAL( partType.fields.size(), 4 );

    for( size_t i = 0; i < partType.fields.size(); ++i )
    {
        BOOST_CHECK_EQUAL( partType.fields[i].name.text, definition.fields[i].name.text );
        BOOST_CHECK_EQUAL( partType.fields[i].value.text, definition.fields[i].value.text );
        BOOST_CHECK( partType.fields[i].presentation == definition.fields[i].presentation );
        BOOST_CHECK( partType.fields[i].id.IsValid() );
        BOOST_CHECK_EQUAL( partType.fields[i].id.Value(),
                           MakeFieldId( FIELD_ID_DOMAIN::PART_TYPE, partType.id.Value(), i ).Value() );
        BOOST_CHECK( fieldIds.insert( partType.fields[i].id.Value() ).second );
    }

    PADS_SCH_MODEL repeated = parser.Parse( loadBinaryFixture( "fields.sch" ), wxS( "fields.sch" ) );
    BOOST_CHECK( repeated.definitions == model.definitions );
    BOOST_CHECK( repeated.partTypes == model.partTypes );
}


BOOST_AUTO_TEST_CASE( PlacementTransforms )
{
    PADS_SCH_BINARY_PARSER parser;
    PADS_SCH_MODEL         model =
            parser.Parse( loadBinaryFixture( "placement_transform.sch" ), wxS( "placement_transform.sch" ) );
    BOOST_REQUIRE_EQUAL( model.placements.size(), 5 );

    const std::array<wxString, 5>     references = { wxS( "R1" ), wxS( "R2" ), wxS( "R3" ), wxS( "R4" ), wxS( "R5" ) };
    const std::array<SOURCE_POINT, 5> positions = { SOURCE_POINT{ -4800, 18200 }, SOURCE_POINT{ -200, 14800 },
                                                    SOURCE_POINT{ 2200, 14000 }, SOURCE_POINT{ 3400, 13000 },
                                                    SOURCE_POINT{ 6000, 10800 } };
    const std::array<int, 5>          angles = { 0, 900, 0, 900, 900 };
    const std::array<bool, 5>         mirrored = { false, false, true, true, true };
    const std::array<uint16_t, 5>     mirrorFlags = { 0, 0, 3, 3, 2 };
    const std::array<std::pair<uint16_t, int>, 4> orthogonalAngles = {
        std::pair<uint16_t, int>{ 0, 0 }, { 900, 900 }, { 1800, 1800 }, { 2700, 2700 }
    };

    for( const auto& [raw, normalized] : orthogonalAngles )
    {
        std::vector<uint8_t> transform = loadBinaryFixture( "placement_transform.sch" );
        writeU16( transform, sheetControllerOffset( transform, 15 ) + 0x24, raw );
        PADS_SCH_MODEL transformed = parser.Parse( transform, wxS( "placement_transform.sch" ) );
        BOOST_CHECK_EQUAL( transformed.placements[0].angle, normalized );
        BOOST_CHECK_EQUAL( propertyValue( transformed.placements[0].properties, wxS( "raw_angle" ) ),
                           wxString::Format( wxS( "%u" ), raw ) );
    }

    for( size_t i = 0; i < model.placements.size(); ++i )
    {
        const MODEL_PLACEMENT& placement = model.placements[i];
        BOOST_CHECK_EQUAL( placement.reference.text, references[i] );
        BOOST_CHECK_EQUAL( placement.position.x, positions[i].x );
        BOOST_CHECK_EQUAL( placement.position.y, positions[i].y );
        BOOST_CHECK_EQUAL( placement.angle, angles[i] );
        BOOST_CHECK_EQUAL( placement.mirrored, mirrored[i] );
        BOOST_CHECK_EQUAL( placement.mirrorFlags, mirrorFlags[i] );
        BOOST_CHECK_EQUAL( propertyValue( placement.properties, wxS( "raw_angle" ) ),
                           wxString::Format( wxS( "%d" ), angles[i] ) );
        BOOST_CHECK_EQUAL( propertyValue( placement.properties, wxS( "raw_mirror" ) ),
                           wxString::Format( wxS( "%u" ), mirrorFlags[i] ) );
        auto rawAngle = std::ranges::find_if( placement.properties,
                                              []( const SOURCE_PROPERTY& aProperty )
                                              {
                                                  return aProperty.name.text == wxS( "raw_angle" );
                                              } );
        auto rawMirror = std::ranges::find_if( placement.properties,
                                               []( const SOURCE_PROPERTY& aProperty )
                                               {
                                                   return aProperty.name.text == wxS( "raw_mirror" );
                                               } );
        BOOST_REQUIRE( rawAngle != placement.properties.end() );
        BOOST_REQUIRE( rawMirror != placement.properties.end() );
        BOOST_CHECK_EQUAL( rawAngle->source.absoluteOffset, placement.source.absoluteOffset + 0x24 );
        BOOST_CHECK_EQUAL( rawMirror->source.absoluteOffset, placement.source.absoluteOffset + 0x26 );
        BOOST_CHECK_EQUAL( rawAngle->source.length, 2 );
        BOOST_CHECK_EQUAL( rawMirror->source.length, 2 );
        BOOST_CHECK_EQUAL( placement.sheet.id.Value(), model.sheets[0].id.Value() );
        BOOST_REQUIRE( placement.gate.has_value() );
        BOOST_CHECK( placement.partType.id.IsValid() );
        BOOST_CHECK( placement.gate->id.IsValid() );
        BOOST_CHECK( placement.definition.id.IsValid() );
        BOOST_CHECK_EQUAL( placement.pins.size(), 2 );
        BOOST_CHECK_GE( placement.fields.size(), 3 );
        BOOST_CHECK_EQUAL( placement.source.controller, 15 );
    }

    BOOST_CHECK_EQUAL( model.placements[0].pins[0].numberOffset.x, 34 );
    BOOST_CHECK_EQUAL( model.placements[0].pins[0].numberOffset.y, 16 );
    BOOST_CHECK_EQUAL( model.placements[0].pins[0].numberAngle, 0 );
    BOOST_CHECK_EQUAL( model.placements[0].pins[0].numberJustification, 0 );
    BOOST_CHECK_EQUAL( model.placements[0].pins[0].numberPresentationFlags, 0x0100 );
    BOOST_CHECK_EQUAL( model.placements[0].pins[1].numberOffset.x, 0 );
    BOOST_CHECK_EQUAL( model.placements[0].pins[1].numberOffset.y, 20 );
    BOOST_CHECK_EQUAL( model.placements[0].pins[1].numberPresentationFlags, 0 );

    auto placementNamed = []( const PADS_SCH_MODEL& aModel, const wxString& aReference ) -> const MODEL_PLACEMENT&
    {
        auto placement = std::ranges::find_if( aModel.placements,
                                               [&]( const MODEL_PLACEMENT& aPlacement )
                                               {
                                                   return aPlacement.reference.text == aReference;
                                               } );
        BOOST_REQUIRE( placement != aModel.placements.end() );
        return *placement;
    };

    PADS_SCH_MODEL         multi = parser.Parse( loadBinaryFixture( "multigate.sch" ), wxS( "multigate.sch" ) );
    const MODEL_PLACEMENT& gateA = placementNamed( multi, wxS( "U3-A" ) );
    const MODEL_PLACEMENT& gateB = placementNamed( multi, wxS( "U3-B" ) );
    BOOST_CHECK_EQUAL( gateA.unit, 1 );
    BOOST_CHECK_EQUAL( gateB.unit, 2 );
    BOOST_CHECK( gateA.definition.id != gateB.definition.id );
    BOOST_CHECK_EQUAL( gateA.pins.size(), 7 );
    BOOST_CHECK_EQUAL( gateB.pins.size(), 2 );

    PADS_SCH_MODEL         connectors = parser.Parse( loadBinaryFixture( "connectors.sch" ), wxS( "connectors.sch" ) );
    const MODEL_PLACEMENT& connector = placementNamed( connectors, wxS( "P1-1" ) );
    BOOST_CHECK_EQUAL( connector.unit, 1 );
    BOOST_CHECK_GE( connector.fields.size(), 4 );
    BOOST_CHECK_EQUAL( connector.fields[2].name.text, wxS( "*" ) );
    BOOST_CHECK( connector.fields[2].value.text.empty() );
    BOOST_CHECK_EQUAL( connector.fields[3].name.text, wxS( "*" ) );
    BOOST_CHECK( connector.fields[3].value.text.empty() );
}


BOOST_AUTO_TEST_CASE( PlacementInstanceFields )
{
    PADS_SCH_BINARY_PARSER parser;
    PADS_SCH_MODEL         model = parser.Parse( loadBinaryFixture( "fields.sch" ), wxS( "fields.sch" ) );
    BOOST_REQUIRE_EQUAL( model.placements.size(), 1 );
    const MODEL_PLACEMENT& placement = model.placements[0];
    BOOST_REQUIRE_GE( placement.fields.size(), 3 );

    BOOST_CHECK_EQUAL( placement.fields[0].name.text, wxS( "REF-DES" ) );
    BOOST_CHECK_EQUAL( placement.fields[0].value.text, wxS( "R1" ) );
    BOOST_CHECK_EQUAL( placement.fields[0].position.x, 600 );
    BOOST_CHECK_EQUAL( placement.fields[0].position.y, 200 );
    BOOST_CHECK_EQUAL( placement.fields[0].presentation.font.text, wxS( "Default Font" ) );
    BOOST_CHECK_EQUAL( placement.fields[1].name.text, wxS( "PART-TYPE" ) );
    BOOST_CHECK_EQUAL( placement.fields[1].value.text, wxS( "RES-RESN1" ) );
    BOOST_CHECK_EQUAL( placement.fields[1].position.x, 620 );
    BOOST_CHECK_EQUAL( placement.fields[1].position.y, 400 );
    BOOST_CHECK_EQUAL( placement.fields[1].angle, 900 );
    BOOST_CHECK_EQUAL( placement.fields[1].presentation.height, 194 );
    BOOST_CHECK_EQUAL( placement.fields[1].presentation.width, 10 );
    BOOST_CHECK_EQUAL( placement.fields[1].presentation.font.text, wxS( "Bold Verdana" ) );
    BOOST_CHECK_EQUAL( placement.fields[2].name.text, wxS( "userfield" ) );
    BOOST_CHECK_EQUAL( placement.fields[2].value.text, wxS( "override-value" ) );
    BOOST_CHECK_EQUAL( placement.fields[2].position.x, 600 );
    BOOST_CHECK_EQUAL( placement.fields[2].position.y, -400 );
    BOOST_CHECK_EQUAL( placement.fields[2].presentation.height, 194 );
    BOOST_CHECK_EQUAL( placement.fields[2].presentation.width, 10 );
    BOOST_CHECK_EQUAL( placement.fields[2].presentation.visible, false );
    BOOST_CHECK_EQUAL( placement.fields[2].presentation.font.text, wxS( "Bold Verdana" ) );
    BOOST_CHECK_EQUAL( placement.fields[2].source.controller, 17 );
    BOOST_CHECK_EQUAL( propertyValue( placement.fields[2].properties, wxS( "component_attribute_index" ) ),
                       wxS( "10" ) );
    auto attributeIndex = std::ranges::find_if( placement.fields[2].properties,
                                                []( const SOURCE_PROPERTY& aProperty )
                                                {
                                                    return aProperty.name.text == wxS( "component_attribute_index" );
                                                } );
    BOOST_REQUIRE( attributeIndex != placement.fields[2].properties.end() );
    BOOST_CHECK_EQUAL( attributeIndex->source.controller, 17 );
    BOOST_CHECK_EQUAL( attributeIndex->source.absoluteOffset, placement.fields[2].source.absoluteOffset + 16 );
    BOOST_CHECK_EQUAL( attributeIndex->source.length, 2 );

    for( size_t i = 0; i < placement.fields.size(); ++i )
    {
        BOOST_CHECK_EQUAL( placement.fields[i].id.Value(),
                           MakeFieldId( FIELD_ID_DOMAIN::PLACEMENT, placement.id.Value(), i ).Value() );
    }
}


BOOST_AUTO_TEST_CASE( PlacementHandleErrors )
{
    PADS_SCH_BINARY_PARSER parser;
    PADS_SCH_MODEL         duplicate =
            parser.Parse( loadBinaryFixture( "placement_transform.sch" ), wxS( "duplicate-placement.sch" ) );
    duplicate.placements[1].id = duplicate.placements[0].id;
    BOOST_CHECK_EXCEPTION( duplicate.ValidateOrThrow(), IO_ERROR,
                           []( const IO_ERROR& aError )
                           {
                               return aError.What().Contains( wxS( "duplicate placement ID" ) )
                                      && aError.What().Contains( wxS( "controller 15" ) )
                                      && aError.What().Contains( wxS( "first at" ) );
                           } );

    std::vector<uint8_t> unresolvedPart = loadBinaryFixture( "placement_transform.sch" );
    writeU16( unresolvedPart, sheetControllerOffset( unresolvedPart, 15 ) + 0x42, 20 );
    BOOST_CHECK_EXCEPTION( parser.Parse( unresolvedPart, wxS( "placement-part.sch" ) ), IO_ERROR,
                           []( const IO_ERROR& aError )
                           {
                               return aError.What().Contains( wxS( "targets definition object class" ) )
                                      && aError.What().Contains( wxS( "controller 15" ) );
                           } );

    std::vector<uint8_t> wrongGroup = loadBinaryFixture( "placement_transform.sch" );
    writeU32( wrongGroup, sheetControllerOffset( wrongGroup, 15 ) + 0x1C, 0xFFFFFFFF );
    BOOST_CHECK_EXCEPTION( parser.Parse( wrongGroup, wxS( "placement-group.sch" ) ), IO_ERROR,
                           []( const IO_ERROR& aError )
                           {
                               return aError.What().Contains( wxS( "component-group handle" ) );
                           } );

    std::vector<uint8_t> wrongDecal = loadBinaryFixture( "placement_transform.sch" );
    writeU16( wrongDecal, sheetControllerOffset( wrongDecal, 15 ) + 0x44, 0 );
    BOOST_CHECK_EXCEPTION( parser.Parse( wrongDecal, wxS( "placement-decal.sch" ) ), IO_ERROR,
                           []( const IO_ERROR& aError )
                           {
                               return aError.What().Contains( wxS( "different object classes" ) );
                           } );

    std::vector<uint8_t> wrongGate = loadBinaryFixture( "multigate.sch" );
    writeU16( wrongGate, sheetControllerOffset( wrongGate, 15 ) + 136 + 0x4A, 7 );
    BOOST_CHECK_EXCEPTION( parser.Parse( wrongGate, wxS( "placement-gate.sch" ) ), IO_ERROR,
                           []( const IO_ERROR& aError )
                           {
                               return aError.What().Contains( wxS( "gate handle targets definition object class" ) );
                           } );

    std::vector<uint8_t> wrongPin = loadBinaryFixture( "placement_transform.sch" );
    writeU16( wrongPin, sheetControllerOffset( wrongPin, 16 ) + 4, 0xFFFF );
    BOOST_CHECK_EXCEPTION( parser.Parse( wrongPin, wxS( "placement-pin.sch" ) ), IO_ERROR,
                           []( const IO_ERROR& aError )
                           {
                               return aError.What().Contains( wxS( "placed-pin handle" ) );
                           } );

    std::vector<uint8_t> wrongField = loadBinaryFixture( "fields.sch" );
    BOOST_REQUIRE_GT( readU32( wrongField, 0x20 + 7 * 28 + 8 ), 10 );
    BOOST_REQUIRE_LE( readU32( wrongField, 0x20 + 19 * 28 + 8 ), 10 );
    writeU16( wrongField, sheetControllerOffset( wrongField, 17 ), 10 );
    BOOST_CHECK_EXCEPTION( parser.Parse( wrongField, wxS( "placement-field.sch" ) ), IO_ERROR,
                           []( const IO_ERROR& aError )
                           {
                               return aError.What().Contains( wxS( "placement font handle" ) );
                           } );

    std::vector<uint8_t> wrongAttribute = loadBinaryFixture( "fields.sch" );
    writeU16( wrongAttribute, sheetControllerOffset( wrongAttribute, 17 ) + 16, 0xFFFE );
    BOOST_CHECK_EXCEPTION( parser.Parse( wrongAttribute, wxS( "placement-field-attribute.sch" ) ), IO_ERROR,
                           []( const IO_ERROR& aError )
                           {
                               return aError.What().Contains( wxS( "attribute index leaves component group" ) )
                                      && aError.What().Contains( wxS( "controller 17" ) );
                           } );

    PADS_SCH_MODEL wrongSheet =
            parser.Parse( loadBinaryFixture( "placement_transform.sch" ), wxS( "placement-sheet.sch" ) );
    const uint32_t wrongClassSheet = wrongSheet.definitions.back().id.Value();
    BOOST_REQUIRE( std::ranges::none_of( wrongSheet.sheets,
                                         [&]( const MODEL_SHEET& aSheet )
                                         {
                                             return aSheet.id.Value() == wrongClassSheet;
                                         } ) );
    wrongSheet.placements[0].sheet = { SHEET_ID( wrongClassSheet ), wrongSheet.placements[0].source };
    BOOST_CHECK_EXCEPTION( wrongSheet.ValidateOrThrow(), IO_ERROR,
                           []( const IO_ERROR& aError )
                           {
                               return aError.What().Contains( wxS( "unresolved placement sheet reference" ) );
                           } );

    std::vector<uint8_t> unknownTransform = loadBinaryFixture( "placement_transform.sch" );
    writeU16( unknownTransform, sheetControllerOffset( unknownTransform, 15 ) + 0x24, 123 );
    writeU16( unknownTransform, sheetControllerOffset( unknownTransform, 15 ) + 0x26, 9 );
    PADS_SCH_MODEL unknown = parser.Parse( unknownTransform, wxS( "placement-transform-enum.sch" ) );
    BOOST_CHECK_EQUAL( propertyValue( unknown.placements[0].properties, wxS( "raw_angle" ) ), wxS( "123" ) );
    BOOST_CHECK_EQUAL( propertyValue( unknown.placements[0].properties, wxS( "raw_mirror" ) ), wxS( "9" ) );
    BOOST_CHECK_EQUAL( std::ranges::count_if( unknown.diagnostics,
                                              []( const PARSER_DIAGNOSTIC& aDiagnostic )
                                              {
                                                  return aDiagnostic.message.Contains(
                                                          wxS( "unknown placement transform" ) );
                                              } ),
                       2 );

    std::vector<uint8_t> boundaryTransform = loadBinaryFixture( "placement_transform.sch" );
    writeU16( boundaryTransform, sheetControllerOffset( boundaryTransform, 15 ) + 0x24, 3600 );
    writeU16( boundaryTransform, sheetControllerOffset( boundaryTransform, 15 ) + 0x26, 1 );
    PADS_SCH_MODEL boundary = parser.Parse( boundaryTransform, wxS( "placement-transform-boundary.sch" ) );
    BOOST_CHECK_EQUAL( boundary.placements[0].angle, 0 );
    BOOST_CHECK_EQUAL( boundary.placements[0].mirrored, true );
    BOOST_CHECK_EQUAL( propertyValue( boundary.placements[0].properties, wxS( "raw_angle" ) ), wxS( "3600" ) );
    BOOST_CHECK_EQUAL( propertyValue( boundary.placements[0].properties, wxS( "raw_mirror" ) ), wxS( "1" ) );
    BOOST_CHECK_EQUAL( std::ranges::count_if( boundary.diagnostics,
                                              []( const PARSER_DIAGNOSTIC& aDiagnostic )
                                              {
                                                  return aDiagnostic.message.Contains(
                                                          wxS( "unknown placement transform" ) );
                                              } ),
                       1 );
}


BOOST_AUTO_TEST_CASE( PlacementSemanticSnapshot )
{
    PADS_SCH_BINARY_PARSER           binaryParser;
    const std::array<std::string, 5> fixtures = { "placement_transform", "fields", "connectors", "multigate",
                                                  "field_justification" };

    for( const std::string& fixture : fixtures )
    {
        PADS_SCH_MODEL binary =
                binaryParser.Parse( loadBinaryFixture( fixture + ".sch" ), wxString::FromUTF8( fixture + ".sch" ) );
        PADS_SCH::PADS_SCH_PARSER ascii;
        BOOST_REQUIRE( ascii.Parse( KI_TEST::GetEeschemaTestDataDir() + "/plugins/pads/binary/" + fixture + ".txt" ) );
        BOOST_REQUIRE_EQUAL( binary.placements.size(), ascii.GetPartPlacements().size() );

        for( size_t placementIndex = 0; placementIndex < binary.placements.size(); ++placementIndex )
        {
            const MODEL_PLACEMENT&          binaryPlacement = binary.placements[placementIndex];
            const PADS_SCH::PART_PLACEMENT& asciiPlacement = ascii.GetPartPlacements()[placementIndex];
            BOOST_CHECK_EQUAL( binaryPlacement.reference.text, wxString::FromUTF8( asciiPlacement.reference ) );
            BOOST_CHECK_EQUAL( binaryPlacement.position.x, asciiPlacement.position.x * 2 );
            BOOST_CHECK_EQUAL( binaryPlacement.position.y, asciiPlacement.position.y * 2 );
            BOOST_CHECK_EQUAL( binaryPlacement.angle, std::lround( asciiPlacement.rotation * 10 ) );
            BOOST_CHECK_EQUAL( binaryPlacement.mirrorFlags, asciiPlacement.mirror_flags );
            BOOST_CHECK_EQUAL( binaryPlacement.sheet.id.Value(), binary.sheets[0].id.Value() );
            auto part = std::ranges::find_if( binary.partTypes,
                                              [&]( const MODEL_PART_TYPE& aPart )
                                              {
                                                  return aPart.id == binaryPlacement.partType.id;
                                              } );
            BOOST_REQUIRE( part != binary.partTypes.end() );
            BOOST_CHECK_EQUAL( part->name.text, wxString::FromUTF8( asciiPlacement.part_type ) );
            BOOST_REQUIRE( binaryPlacement.gate.has_value() );
            auto gate = std::ranges::find_if( part->gates,
                                              [&]( const MODEL_GATE& aGate )
                                              {
                                                  return aGate.id == binaryPlacement.gate->id;
                                              } );
            BOOST_REQUIRE( gate != part->gates.end() );
            BOOST_CHECK_EQUAL( binaryPlacement.unit, static_cast<uint32_t>( asciiPlacement.gate_index + 1 ) );
            auto definition = std::ranges::find_if( binary.definitions,
                                                    [&]( const MODEL_SYMBOL_DEFINITION& aDefinition )
                                                    {
                                                        return aDefinition.id == binaryPlacement.definition.id;
                                                    } );
            BOOST_REQUIRE( definition != binary.definitions.end() );
            BOOST_CHECK_EQUAL( definition->name.text, wxString::FromUTF8( asciiPlacement.symbol_name ) );
            BOOST_CHECK( !propertyValue( binaryPlacement.properties, wxS( "decal_handle" ) ).empty() );
            BOOST_REQUIRE_GE( binaryPlacement.fields.size(), asciiPlacement.attributes.size() );

            for( size_t fieldIndex = 0; fieldIndex < asciiPlacement.attributes.size(); ++fieldIndex )
            {
                const MODEL_FIELD&              binaryField = binaryPlacement.fields[fieldIndex];
                const PADS_SCH::PART_ATTRIBUTE& asciiField = asciiPlacement.attributes[fieldIndex];
                BOOST_CHECK_EQUAL( binaryField.name.text, wxString::FromUTF8( asciiField.name ) );
                const wxString asciiValue = fieldIndex == 0   ? wxString::FromUTF8( asciiPlacement.reference )
                                            : fieldIndex == 1 ? wxString::FromUTF8( asciiPlacement.part_type )
                                                              : wxString::FromUTF8( asciiField.value );
                BOOST_CHECK_EQUAL( binaryField.value.text, asciiValue );
                BOOST_CHECK_EQUAL( binaryField.position.x, asciiField.position.x * 2 );
                BOOST_CHECK_EQUAL( binaryField.position.y, asciiField.position.y * 2 );
                BOOST_CHECK_EQUAL( binaryField.angle, std::lround( asciiField.rotation * 10 ) );
                BOOST_CHECK_EQUAL( binaryField.presentation.height, asciiField.height );
                BOOST_CHECK_EQUAL( binaryField.presentation.width, asciiField.width );
                const bool expectedVisible = fieldIndex == 0   ? binaryPlacement.referenceVisible
                                             : fieldIndex == 1 ? binaryPlacement.partTypeVisible
                                                               : asciiField.visible;
                BOOST_CHECK_EQUAL( binaryField.presentation.visible, expectedVisible );
                BOOST_CHECK_EQUAL( binaryField.presentation.font.text, wxString::FromUTF8( asciiField.font_name ) );
                BOOST_CHECK_EQUAL( binaryField.presentation.bold,
                                   wxString::FromUTF8( asciiField.font_name ).StartsWith( wxS( "Bold " ) ) );
                BOOST_CHECK_EQUAL( binaryField.presentation.italic,
                                   wxString::FromUTF8( asciiField.font_name ).StartsWith( wxS( "Italic " ) ) );

                const uint16_t            justification = asciiField.justification;
                const uint16_t            horizontal = justification >= 8   ? justification - 8
                                                       : justification >= 2 ? justification - 2
                                                                            : justification;
                const MODEL_JUSTIFICATION expectedHorizontal = horizontal == 1   ? MODEL_JUSTIFICATION::RIGHT
                                                               : horizontal == 4 ? MODEL_JUSTIFICATION::CENTER
                                                                                 : MODEL_JUSTIFICATION::LEFT;
                const MODEL_JUSTIFICATION expectedVertical = asciiField.justification >= 8 ? MODEL_JUSTIFICATION::CENTER
                                                             : asciiField.justification >= 2
                                                                     ? MODEL_JUSTIFICATION::LEFT
                                                                     : MODEL_JUSTIFICATION::RIGHT;
                BOOST_CHECK( binaryField.presentation.horizontalJustification == expectedHorizontal );
                BOOST_CHECK( binaryField.presentation.verticalJustification == expectedVertical );

                if( fieldIndex >= 2 )
                {
                    BOOST_CHECK_EQUAL( propertyValue( binaryField.properties, wxS( "display_flags" ) ),
                                       wxString::Format( wxS( "%d" ), asciiField.visibility ) );
                    const wxString expectedAttributeIndex =
                            fixture == "fields" || fixture == "field_justification" ? wxS( "10" ) : wxS( "65535" );
                    BOOST_CHECK_EQUAL( propertyValue( binaryField.properties, wxS( "component_attribute_index" ) ),
                                       expectedAttributeIndex );
                }
            }
        }
    }
}


BOOST_AUTO_TEST_CASE( Connectivity )
{
    PADS_SCH_BINARY_PARSER parser;
    PADS_SCH_MODEL         model =
            parser.Parse( loadBinaryFixture( "connectivity_topology.sch" ), wxS( "connectivity_topology.sch" ) );

    BOOST_REQUIRE_EQUAL( model.nets.size(), 9 );
    BOOST_CHECK_EQUAL( itemNamed( model.nets, wxS( "CROSS_H" ) ).connections.size(), 1 );
    BOOST_CHECK_EQUAL( itemNamed( model.nets, wxS( "CROSS_V" ) ).connections.size(), 1 );
    BOOST_REQUIRE_EQUAL( itemNamed( model.nets, wxS( "T_NET" ) ).connections.size(), 3 );

    const MODEL_CONNECTION& horizontal = itemNamed( model.nets, wxS( "CROSS_H" ) ).connections.front();
    BOOST_REQUIRE_EQUAL( horizontal.vertices.size(), 2 );
    BOOST_REQUIRE_EQUAL( horizontal.endpoints.size(), 2 );
    BOOST_CHECK_EQUAL( horizontal.vertices[0].x, 6000 );
    BOOST_CHECK_EQUAL( horizontal.vertices[0].y, 6000 );
    BOOST_CHECK_EQUAL( horizontal.vertices[1].x, 14000 );
    BOOST_CHECK_EQUAL( horizontal.vertices[1].y, 6000 );
    BOOST_CHECK( horizontal.endpoints[0].kind == MODEL_ENDPOINT_KIND::POINT );
    BOOST_CHECK( horizontal.endpoints[1].kind == MODEL_ENDPOINT_KIND::POINT );

    PADS_SCH_MODEL minimal = parser.Parse( loadMinimalV13(), wxS( "minimal_v13.sch" ) );
    BOOST_REQUIRE_EQUAL( minimal.nets.size(), 1 );
    BOOST_REQUIRE_EQUAL( minimal.nets[0].connections.size(), 1 );
    BOOST_CHECK( minimal.nets[0].connections[0].endpoints[0].kind == MODEL_ENDPOINT_KIND::PIN );
    BOOST_REQUIRE( minimal.nets[0].connections[0].endpoints[0].placement );
    BOOST_REQUIRE( minimal.nets[0].connections[0].endpoints[0].pin );
    BOOST_CHECK_EQUAL( minimal.nets[0].connections[0].endpoints[0].placement->id.Value(),
                       minimal.placements[0].id.Value() );
    BOOST_CHECK_EQUAL( minimal.nets[0].connections[0].endpoints[0].pin->id.Value(),
                       minimal.placements[0].pins[0].id.Value() );
}


BOOST_AUTO_TEST_CASE( LabelsAndPower )
{
    PADS_SCH_BINARY_PARSER parser;
    PADS_SCH_MODEL         model =
            parser.Parse( loadBinaryFixture( "connectivity_topology.sch" ), wxS( "connectivity_topology.sch" ) );

    BOOST_REQUIRE_EQUAL( model.labels.size(), 16 );
    BOOST_CHECK_EQUAL( std::ranges::count( model.labels, MODEL_LABEL_KIND::LOCAL, &MODEL_LABEL::kind ), 13 );
    BOOST_CHECK_EQUAL( std::ranges::count( model.labels, MODEL_LABEL_KIND::GROUND, &MODEL_LABEL::kind ), 1 );
    BOOST_CHECK_EQUAL( std::ranges::count( model.labels, MODEL_LABEL_KIND::POWER, &MODEL_LABEL::kind ), 1 );
    BOOST_CHECK_EQUAL( std::ranges::count( model.labels, MODEL_LABEL_KIND::GLOBAL, &MODEL_LABEL::kind ), 1 );

    auto global = std::ranges::find_if( model.labels,
                                        []( const MODEL_LABEL& aLabel )
                                        {
                                            return aLabel.kind == MODEL_LABEL_KIND::GLOBAL;
                                        } );
    BOOST_REQUIRE( global != model.labels.end() );
    BOOST_CHECK_EQUAL( global->text.text, wxS( "GLOBAL_LINK" ) );
    BOOST_CHECK_EQUAL( global->position.x, 16000 );
    BOOST_CHECK_EQUAL( global->position.y, 12000 );
    BOOST_CHECK_EQUAL( global->angle, 0 );

    PADS_SCH_MODEL multisheet =
            parser.Parse( loadBinaryFixture( "multisheet_connectivity.sch" ), wxS( "multisheet_connectivity.sch" ) );
    BOOST_CHECK_EQUAL( std::ranges::count_if( multisheet.labels,
                                              []( const MODEL_LABEL& aLabel )
                                              {
                                                  return aLabel.kind == MODEL_LABEL_KIND::GLOBAL
                                                         && aLabel.text.text == wxS( "CROSS_SHEET" );
                                              } ),
                       2 );
    std::vector<const MODEL_LABEL*> crossSheetLabels;

    for( const MODEL_LABEL& label : multisheet.labels )
    {
        if( label.kind == MODEL_LABEL_KIND::GLOBAL && label.text.text == wxS( "CROSS_SHEET" ) )
            crossSheetLabels.push_back( &label );
    }

    BOOST_REQUIRE_EQUAL( crossSheetLabels.size(), 2 );
    std::ranges::sort( crossSheetLabels,
                       []( const MODEL_LABEL* aLeft, const MODEL_LABEL* aRight )
                       {
                           return aLeft->source.sheet < aRight->source.sheet;
                       } );
    BOOST_REQUIRE_EQUAL( crossSheetLabels[0]->linkedSheets.size(), 1 );
    BOOST_REQUIRE_EQUAL( crossSheetLabels[1]->linkedSheets.size(), 1 );
    BOOST_CHECK_EQUAL( crossSheetLabels[0]->source.sheet, 0 );
    BOOST_CHECK_EQUAL( crossSheetLabels[1]->source.sheet, 1 );
    BOOST_CHECK_EQUAL( crossSheetLabels[0]->linkedSheets[0].id.Value(), multisheet.sheets[1].id.Value() );
    BOOST_CHECK_EQUAL( crossSheetLabels[1]->linkedSheets[0].id.Value(), multisheet.sheets[0].id.Value() );
}


BOOST_AUTO_TEST_CASE( Buses )
{
    PADS_SCH_BINARY_PARSER parser;
    PADS_SCH_MODEL         model =
            parser.Parse( loadBinaryFixture( "connectivity_topology.sch" ), wxS( "connectivity_topology.sch" ) );

    BOOST_REQUIRE_EQUAL( model.buses.size(), 1 );
    const MODEL_BUS& bus = model.buses.front();
    BOOST_CHECK_EQUAL( bus.name.text, wxS( "BATCHC_BUS_ALIAS" ) );
    BOOST_REQUIRE_EQUAL( bus.vertices.size(), 3 );
    BOOST_CHECK_EQUAL( bus.vertices[0].x, 6000 );
    BOOST_CHECK_EQUAL( bus.vertices[0].y, 14000 );
    BOOST_CHECK_EQUAL( bus.vertices[2].x, 14000 );
    BOOST_CHECK_EQUAL( bus.vertices[2].y, 10000 );
    BOOST_REQUIRE_EQUAL( bus.entries.size(), 3 );
    BOOST_REQUIRE_EQUAL( bus.aliases.size(), 1 );
    BOOST_REQUIRE_EQUAL( bus.memberNets.size(), 3 );
    BOOST_CHECK_EQUAL( bus.aliases[0].text, wxS( "BATCHC_BUS_ALIAS" ) );
    BOOST_CHECK_EQUAL( bus.entries[0].position.x, 8000 );
    BOOST_CHECK_EQUAL( bus.entries[1].position.x, 10000 );
    BOOST_CHECK_EQUAL( bus.entries[2].position.x, 12000 );
    BOOST_CHECK_EQUAL( itemNamed( model.nets, wxS( "BUS0" ) ).id.Value(), bus.memberNets[0].id.Value() );
    BOOST_CHECK_EQUAL( itemNamed( model.nets, wxS( "BUS1" ) ).id.Value(), bus.memberNets[1].id.Value() );
    BOOST_CHECK_EQUAL( itemNamed( model.nets, wxS( "BUS2" ) ).id.Value(), bus.memberNets[2].id.Value() );
}


BOOST_AUTO_TEST_CASE( PageGraphics )
{
    PADS_SCH_BINARY_PARSER parser;
    PADS_SCH_MODEL         model = parser.Parse( loadBinaryFixture( "page_graphics.sch" ), wxS( "page_graphics.sch" ) );

    BOOST_REQUIRE_EQUAL( model.sheets.size(), 1 );
    BOOST_REQUIRE_EQUAL( model.graphics.size(), 178 );
    BOOST_CHECK_EQUAL( std::ranges::count_if( model.graphics,
                                              []( const MODEL_PAGE_GRAPHIC& aGraphic )
                                              {
                                                  return aGraphic.graphic.kind == MODEL_GRAPHIC_KIND::LINE;
                                              } ),
                       63 );
    BOOST_CHECK_GE( std::ranges::count_if( model.graphics,
                                           []( const MODEL_PAGE_GRAPHIC& aGraphic )
                                           {
                                               return aGraphic.graphic.kind == MODEL_GRAPHIC_KIND::POLYLINE;
                                           } ),
                    1 );
    BOOST_CHECK_EQUAL( std::ranges::count_if( model.graphics,
                                              []( const MODEL_PAGE_GRAPHIC& aGraphic )
                                              {
                                                  return aGraphic.graphic.kind == MODEL_GRAPHIC_KIND::TEXT;
                                              } ),
                       105 );
    auto titleText = std::ranges::find_if( model.graphics,
                                           []( const MODEL_PAGE_GRAPHIC& aGraphic )
                                           {
                                               return aGraphic.graphic.kind == MODEL_GRAPHIC_KIND::TEXT
                                                      && aGraphic.graphic.text.text == wxS( "TITLE:" );
                                           } );
    BOOST_REQUIRE( titleText != model.graphics.end() );
    BOOST_CHECK_EQUAL( titleText->graphic.presentation.height, 100 );
    BOOST_CHECK_EQUAL( titleText->graphic.presentation.width, 10 );
    auto titleGroup = std::ranges::find( titleText->graphic.properties, wxS( "page_graphic_group" ),
                                         []( const SOURCE_PROPERTY& aProperty )
                                         {
                                             return aProperty.name.text;
                                         } );
    BOOST_REQUIRE( titleGroup != titleText->graphic.properties.end() );
    const uint16_t titleTextCount =
            readU16( loadBinaryFixture( "page_graphics.sch" ), titleGroup->source.absoluteOffset + 64 );
    BOOST_REQUIRE_GT( titleTextCount, 1u );

    std::vector<uint8_t> repeatedPredecessor = loadBinaryFixture( "page_graphics.sch" );
    const uint16_t       lastTextRecord = readU16( repeatedPredecessor, titleGroup->source.absoluteOffset + 66 );
    writeU16( repeatedPredecessor, sheetControllerOffset( repeatedPredecessor, 1 ) + lastTextRecord * 32 + 24,
              lastTextRecord );
    BOOST_CHECK_EXCEPTION( parser.Parse( repeatedPredecessor, wxS( "page-text-cycle.sch" ) ), IO_ERROR,
                           []( const IO_ERROR& aError )
                           {
                               return aError.What().Contains(
                                       wxS( "page-text predecessor repeats controller 1 record" ) );
                           } );

    std::vector<uint8_t> noncontiguousPredecessor = loadBinaryFixture( "page_graphics.sch" );
    const size_t         textController = sheetControllerOffset( noncontiguousPredecessor, 1 );
    const uint16_t       predecessor1 = readU16( noncontiguousPredecessor, textController + lastTextRecord * 32 + 24 );
    const uint16_t       predecessor2 = readU16( noncontiguousPredecessor, textController + predecessor1 * 32 + 24 );
    const uint16_t       predecessor3 = readU16( noncontiguousPredecessor, textController + predecessor2 * 32 + 24 );
    writeU16( noncontiguousPredecessor, textController + lastTextRecord * 32 + 24, predecessor2 );
    writeU16( noncontiguousPredecessor, textController + predecessor2 * 32 + 24, predecessor1 );
    writeU16( noncontiguousPredecessor, textController + predecessor1 * 32 + 24, predecessor3 );
    PADS_SCH_MODEL      noncontiguous = parser.Parse( noncontiguousPredecessor, wxS( "page-text-noncontiguous.sch" ) );
    std::vector<size_t> recoveredRecords;

    for( const MODEL_PAGE_GRAPHIC& graphic : noncontiguous.graphics )
    {
        if( graphic.graphic.kind == MODEL_GRAPHIC_KIND::TEXT
            && propertyValue( graphic.graphic.properties, wxS( "page_graphic_group" ) ) == titleGroup->value.text )
        {
            recoveredRecords.push_back( graphic.graphic.source.recordIndex );
        }
    }

    BOOST_REQUIRE_EQUAL( recoveredRecords.size(), titleTextCount );
    BOOST_CHECK_EQUAL( recoveredRecords[recoveredRecords.size() - 3], predecessor1 );
    BOOST_CHECK_EQUAL( recoveredRecords[recoveredRecords.size() - 2], predecessor2 );
    BOOST_CHECK_EQUAL( recoveredRecords.back(), lastTextRecord );

    std::vector<const MODEL_PAGE_GRAPHIC*> custom;

    for( const MODEL_PAGE_GRAPHIC& graphic : model.graphics )
    {
        if( propertyValue( graphic.graphic.properties, wxS( "page_graphic_group" ) ) == wxS( "BATCHB_PAGE_GRAPHICS" ) )
        {
            custom.push_back( &graphic );
        }
    }

    BOOST_REQUIRE_EQUAL( custom.size(), 6 );
    auto customKind = [&]( MODEL_GRAPHIC_KIND aKind )
    {
        return std::ranges::find_if( custom,
                                     [&]( const MODEL_PAGE_GRAPHIC* aGraphic )
                                     {
                                         return aGraphic->graphic.kind == aKind;
                                     } );
    };
    auto line = customKind( MODEL_GRAPHIC_KIND::LINE );
    BOOST_REQUIRE( line != custom.end() );
    BOOST_REQUIRE_EQUAL( ( *line )->graphic.points.size(), 2 );
    BOOST_CHECK_EQUAL( ( *line )->graphic.points[0].x, 12000 );
    BOOST_CHECK_EQUAL( ( *line )->graphic.points[1].x, 13600 );
    BOOST_CHECK_EQUAL( ( *line )->graphic.strokeWidth, 20 );
    BOOST_CHECK( ( *line )->graphic.lineStyle == MODEL_LINE_STYLE::SOLID );
    auto circle = customKind( MODEL_GRAPHIC_KIND::CIRCLE );
    BOOST_REQUIRE( circle != custom.end() );
    BOOST_CHECK_EQUAL( ( *circle )->graphic.points[0].x, 15200 );
    BOOST_CHECK_EQUAL( ( *circle )->graphic.points[1].y, 12800 );
    auto arc = customKind( MODEL_GRAPHIC_KIND::ARC );
    BOOST_REQUIRE( arc != custom.end() );
    BOOST_CHECK_EQUAL( ( *arc )->graphic.arcSweepAngle, 1800 );
    BOOST_CHECK( ( *arc )->graphic.arcClockwise );
    BOOST_CHECK_EQUAL( ( *arc )->graphic.arcCenter.x, 16800 );
    BOOST_CHECK_EQUAL( ( *arc )->graphic.arcCenter.y, 12400 );
    auto text = customKind( MODEL_GRAPHIC_KIND::TEXT );
    BOOST_REQUIRE( text != custom.end() );
    BOOST_CHECK_EQUAL( ( *text )->graphic.text.text, wxS( "EMBEDDED PAGE TEXT" ) );
    BOOST_CHECK_EQUAL( ( *text )->graphic.points[0].y, 13200 );
    BOOST_CHECK_EQUAL( ( *text )->graphic.presentation.font.text, wxS( "Default Font" ) );
    BOOST_CHECK_EQUAL( ( *text )->graphic.presentation.height, 150 );
    BOOST_CHECK_EQUAL( ( *text )->graphic.presentation.width, 10 );
    PADS_SCH::PADS_SCH_PARSER ascii;
    BOOST_REQUIRE( ascii.Parse( KI_TEST::GetEeschemaTestDataDir() + "/plugins/pads/binary/page_graphics.txt" ) );
    auto pageRecords = []( std::vector<CANONICAL_SEMANTIC_RECORD> aRecords )
    {
        std::erase_if( aRecords,
                       []( const CANONICAL_SEMANTIC_RECORD& aRecord )
                       {
                           auto group = aRecord.properties.find( "page_graphic_group" );
                           return aRecord.kind != CANONICAL_KIND::GRAPHIC || group == aRecord.properties.end()
                                  || group->second.value != CANONICAL_VALUE( std::string( "BATCHB_PAGE_GRAPHICS" ) );
                       } );
        return aRecords;
    };
    auto expectedPage = pageRecords( normalizeAsciiModel( ascii ) );
    auto actualPage = pageRecords( normalizeBinaryModel( model ) );
    BOOST_CHECK( snapshotsMatch( expectedPage, actualPage,
                                 { { "preserved_graphic_presentation", PROPERTY_DISPOSITION::PRESERVED } } ) );

    std::vector<uint8_t> privateStroke = loadBinaryFixture( "page_graphics.sch" );
    const size_t         privatePiece = sheetControllerOffset( privateStroke, 4 ) + 81 * 6;
    writeU16( privateStroke, privatePiece + 4, 0xEF03 );
    PADS_SCH_MODEL preservedStroke = parser.Parse( privateStroke, wxS( "private-stroke.sch" ) );
    auto           privateGraphic = std::ranges::find_if( preservedStroke.graphics,
                                                          []( const MODEL_PAGE_GRAPHIC& aGraphic )
                                                          {
                                                    return aGraphic.graphic.source.controller == 4
                                                           && aGraphic.graphic.source.recordIndex == 81;
                                                } );
    BOOST_REQUIRE( privateGraphic != preservedStroke.graphics.end() );
    BOOST_CHECK_EQUAL( privateGraphic->graphic.strokeWidth, 0 );
    auto rawStroke = std::ranges::find_if( privateGraphic->graphic.properties,
                                           []( const SOURCE_PROPERTY& aProperty )
                                           {
                                               return aProperty.name.text == wxS( "unsupported_graphic_stroke_width" );
                                           } );
    BOOST_REQUIRE( rawStroke != privateGraphic->graphic.properties.end() );
    BOOST_CHECK_EQUAL( rawStroke->value.text, wxS( "3" ) );
    BOOST_CHECK( rawStroke->disposition == PROPERTY_DISPOSITION::UNSUPPORTED );
    BOOST_CHECK_EQUAL( rawStroke->source.controller, 4 );
    BOOST_CHECK_EQUAL( rawStroke->source.recordIndex, 81 );
    BOOST_CHECK_EQUAL( rawStroke->source.absoluteOffset, privatePiece + 4 );
    BOOST_CHECK_EQUAL( rawStroke->source.length, 1 );
    BOOST_CHECK_EQUAL( propertyValue( privateGraphic->graphic.properties, wxS( "preserved_graphic_presentation" ) ),
                       wxS( "239" ) );
    BOOST_CHECK_EQUAL( std::ranges::count_if( preservedStroke.diagnostics,
                                              [&]( const PARSER_DIAGNOSTIC& aDiagnostic )
                                              {
                                                  return aDiagnostic.source == rawStroke->source && aDiagnostic.property
                                                         && aDiagnostic.property->name == rawStroke->name.text
                                                         && aDiagnostic.property->disposition == rawStroke->disposition;
                                              } ),
                       1u );
}


BOOST_AUTO_TEST_CASE( ConnectivityHandleErrors )
{
    PADS_SCH_BINARY_PARSER parser;
    std::vector<uint8_t>   wrongEndpoint = loadBinaryFixture( "connectivity_topology.sch" );
    const size_t           connection = sheetControllerOffset( wrongEndpoint, 21 );
    writeU16( wrongEndpoint, connection + 12, 0x1000 );
    BOOST_CHECK_EXCEPTION( parser.Parse( wrongEndpoint, wxS( "wrong-endpoint.sch" ) ), IO_ERROR,
                           [&]( const IO_ERROR& aError )
                           {
                               return aError.What().Contains( wxS( "controller 21" ) )
                                      && aError.What().Contains( wxS( "wrong endpoint object class" ) );
                           } );

    std::vector<uint8_t> unresolvedEndpoint = loadBinaryFixture( "connectivity_topology.sch" );
    writeU16( unresolvedEndpoint, connection + 12, 0x2FFF );
    BOOST_CHECK_EXCEPTION( parser.Parse( unresolvedEndpoint, wxS( "unresolved-endpoint.sch" ) ), IO_ERROR,
                           [&]( const IO_ERROR& aError )
                           {
                               return aError.What().Contains( wxS( "controller 21" ) )
                                      && aError.What().Contains( wxS( "unresolved off-page endpoint" ) );
                           } );

    std::vector<uint8_t> unresolvedPlacement = loadBinaryFixture( "minimal_v13.sch" );
    const size_t         minimalConnection = sheetControllerOffset( unresolvedPlacement, 21 );
    BOOST_REQUIRE_EQUAL( readU16( unresolvedPlacement, minimalConnection + 12 ), 0u );
    writeU16( unresolvedPlacement, minimalConnection + 12, 0x0FFF );
    BOOST_CHECK_EXCEPTION( parser.Parse( unresolvedPlacement, wxS( "unresolved-placement.sch" ) ), IO_ERROR,
                           []( const IO_ERROR& aError )
                           {
                               return aError.What().Contains( wxS( "controller 21" ) )
                                      && aError.What().Contains( wxS( "unresolved placement endpoint" ) );
                           } );

    std::vector<uint8_t> duplicateMembership = loadBinaryFixture( "connectivity_topology.sch" );
    const size_t         globalNets = outerControllerOffset( duplicateMembership, 8 );
    writeU32( duplicateMembership, globalNets + 2 * 88 + 4, 1 );
    BOOST_CHECK_EXCEPTION( parser.Parse( duplicateMembership, wxS( "duplicate-membership.sch" ) ), IO_ERROR,
                           []( const IO_ERROR& aError )
                           {
                               return aError.What().Contains( wxS( "controller 8" ) )
                                      && aError.What().Contains( wxS( "duplicate net sheet-membership" ) );
                           } );

    std::vector<uint8_t> wrongNetClass = loadBinaryFixture( "connectivity_topology.sch" );
    const size_t         wrongNetConnection = sheetControllerOffset( wrongNetClass, 21 );
    writeU32( wrongNetClass, wrongNetConnection + 8, 0 );
    BOOST_CHECK_EXCEPTION( parser.Parse( wrongNetClass, wxS( "wrong-net-class.sch" ) ), IO_ERROR,
                           []( const IO_ERROR& aError )
                           {
                               return aError.What().Contains( wxS( "controller 21" ) )
                                      && aError.What().Contains( wxS( "wrong or unresolved object class" ) );
                           } );

    std::vector<uint8_t> wrongBusClass = loadBinaryFixture( "connectivity_topology.sch" );
    const size_t         wrongBus = sheetControllerOffset( wrongBusClass, 18 );
    writeU32( wrongBusClass, wrongBus + 8, 1 );
    BOOST_CHECK_EXCEPTION( parser.Parse( wrongBusClass, wxS( "wrong-bus-class.sch" ) ), IO_ERROR,
                           []( const IO_ERROR& aError )
                           {
                               return aError.What().Contains( wxS( "controller 18" ) )
                                      && aError.What().Contains( wxS( "wrong or unresolved object class" ) );
                           } );

    std::vector<uint8_t> wrongSheet = loadBinaryFixture( "multisheet_connectivity.sch" );
    const size_t         sheetMembership = outerControllerOffset( wrongSheet, 4 );
    writeU16( wrongSheet, sheetMembership + 2, 2 );
    BOOST_CHECK_EXCEPTION( parser.Parse( wrongSheet, wxS( "wrong-net-sheet.sch" ) ), IO_ERROR,
                           []( const IO_ERROR& aError )
                           {
                               return aError.What().Contains( wxS( "controller 8" ) )
                                      && aError.What().Contains( wxS( "wrong sheet object class" ) );
                           } );

    std::vector<uint8_t> busCycle = loadBinaryFixture( "connectivity_topology.sch" );
    const size_t         bus = sheetControllerOffset( busCycle, 18 );
    const size_t         offpage = sheetControllerOffset( busCycle, 20 );
    const uint16_t       tailHandle = readU16( busCycle, bus + 24 );
    BOOST_REQUIRE_EQUAL( tailHandle >> 12, 2 );
    const uint16_t tailRecord = tailHandle & 0x0FFF;
    writeU16( busCycle, offpage + tailRecord * 32 + 4, tailHandle );
    BOOST_CHECK_EXCEPTION( parser.Parse( busCycle, wxS( "bus-entry-cycle.sch" ) ), IO_ERROR,
                           []( const IO_ERROR& aError )
                           {
                               return aError.What().Contains( wxS( "controller 18" ) )
                                      && aError.What().Contains( wxS( "cyclic bus-entry" ) );
                           } );

    std::vector<uint8_t> unresolvedLabelConnection = loadBinaryFixture( "connectivity_topology.sch" );
    const size_t         label = sheetControllerOffset( unresolvedLabelConnection, 20 );
    writeU16( unresolvedLabelConnection, label + 8, 0xFFFF );
    BOOST_CHECK_EXCEPTION( parser.Parse( unresolvedLabelConnection, wxS( "unresolved-label-connection.sch" ) ),
                           IO_ERROR,
                           []( const IO_ERROR& aError )
                           {
                               return aError.What().Contains( wxS( "controller 20" ) )
                                      && aError.What().Contains( wxS( "off-page net handle leaves controller 21" ) );
                           } );

    std::vector<uint8_t> wrongJunctionOwner = loadBinaryFixture( "connectivity_topology.sch" );
    const size_t         junction = sheetControllerOffset( wrongJunctionOwner, 19 );
    const uint16_t       junctionOwner = readU16( wrongJunctionOwner, junction + 8 );
    BOOST_REQUIRE_GT( sheetControllerCount( wrongJunctionOwner, 21 ), 1 );
    writeU16( wrongJunctionOwner, junction + 8, junctionOwner == 0 ? 1 : 0 );
    const wxString junctionOwnerOffset =
            wxString::Format( wxS( "offset 0x%llX" ), static_cast<unsigned long long>( junction + 8 ) );
    BOOST_CHECK_EXCEPTION( parser.Parse( wrongJunctionOwner, wxS( "wrong-junction-owner.sch" ) ), IO_ERROR,
                           [&]( const IO_ERROR& aError )
                           {
                               return aError.What().Contains( wxS( "controller 19" ) )
                                      && aError.What().Contains( wxS( "does not point back" ) )
                                      && aError.What().Contains( junctionOwnerOffset );
                           } );

    std::vector<uint8_t> crossNetJunction = loadBinaryFixture( "connectivity_topology.sch" );
    const size_t         crossNetJunctions = sheetControllerOffset( crossNetJunction, 19 );
    const size_t         crossNetConnections = sheetControllerOffset( crossNetJunction, 21 );
    const uint16_t       tiedDotHandle = 0x3000;
    const uint16_t       tiedDotOwner = readU16( crossNetJunction, crossNetJunctions + 8 );
    size_t               sharedConnection = std::numeric_limits<size_t>::max();
    uint32_t             differentNet = std::numeric_limits<uint32_t>::max();
    const uint32_t       ownerNet = readU32( crossNetJunction, crossNetConnections + tiedDotOwner * 40 + 8 );

    for( size_t record = 0; record < sheetControllerCount( crossNetJunction, 21 ); ++record )
    {
        const size_t offset = crossNetConnections + record * 40;

        if( record != tiedDotOwner
            && ( readU16( crossNetJunction, offset + 12 ) == tiedDotHandle
                 || readU16( crossNetJunction, offset + 14 ) == tiedDotHandle ) )
        {
            sharedConnection = record;
        }

        if( readU32( crossNetJunction, offset + 8 ) != ownerNet )
            differentNet = readU32( crossNetJunction, offset + 8 );
    }

    BOOST_REQUIRE_NE( sharedConnection, std::numeric_limits<size_t>::max() );
    BOOST_REQUIRE_NE( differentNet, std::numeric_limits<uint32_t>::max() );
    writeU32( crossNetJunction, crossNetConnections + sharedConnection * 40 + 8, differentNet );
    const wxString crossNetOffset =
            wxString::Format( wxS( "offset 0x%llX" ), static_cast<unsigned long long>( crossNetJunctions + 8 ) );
    BOOST_CHECK_EXCEPTION( parser.Parse( crossNetJunction, wxS( "cross-net-junction.sch" ) ), IO_ERROR,
                           [&]( const IO_ERROR& aError )
                           {
                               return aError.What().Contains( wxS( "controller 19" ) )
                                      && aError.What().Contains( wxS( "shared across different nets" ) )
                                      && aError.What().Contains( crossNetOffset );
                           } );

    std::vector<uint8_t> wrongOffpageOwner = loadBinaryFixture( "connectivity_topology.sch" );
    const size_t         offpageOwner = sheetControllerOffset( wrongOffpageOwner, 20 );
    const uint16_t       connectionOwner = readU16( wrongOffpageOwner, offpageOwner + 8 );
    writeU16( wrongOffpageOwner, offpageOwner + 8, connectionOwner == 0 ? 1 : 0 );
    const wxString offpageOwnerOffset =
            wxString::Format( wxS( "offset 0x%llX" ), static_cast<unsigned long long>( offpageOwner + 8 ) );
    BOOST_CHECK_EXCEPTION( parser.Parse( wrongOffpageOwner, wxS( "wrong-offpage-owner.sch" ) ), IO_ERROR,
                           [&]( const IO_ERROR& aError )
                           {
                               return aError.What().Contains( wxS( "controller 20" ) )
                                      && aError.What().Contains( wxS( "does not point back" ) )
                                      && aError.What().Contains( offpageOwnerOffset );
                           } );

    std::vector<uint8_t> crossNetOffpage = loadBinaryFixture( "connectivity_topology.sch" );
    const size_t         crossNetOffpages = sheetControllerOffset( crossNetOffpage, 20 );
    const size_t         crossNetOffpageConnections = sheetControllerOffset( crossNetOffpage, 21 );
    const size_t         crossNetVertices = sheetControllerOffset( crossNetOffpage, 22 );
    const uint16_t       offpageConnection = readU16( crossNetOffpage, crossNetOffpages + 8 );
    const uint32_t offpageNet = readU32( crossNetOffpage, crossNetOffpageConnections + offpageConnection * 40 + 8 );
    size_t         foreignConnection = std::numeric_limits<size_t>::max();

    for( size_t record = 0; record < sheetControllerCount( crossNetOffpage, 21 ); ++record )
    {
        if( readU32( crossNetOffpage, crossNetOffpageConnections + record * 40 + 8 ) != offpageNet )
        {
            foreignConnection = record;
            break;
        }
    }

    BOOST_REQUIRE_NE( foreignConnection, std::numeric_limits<size_t>::max() );
    const size_t foreignOffset = crossNetOffpageConnections + foreignConnection * 40;
    const size_t firstVertex = crossNetVertices + readU32( crossNetOffpage, foreignOffset + 4 ) * 8;
    writeU16( crossNetOffpage, foreignOffset + 12, 0x2000 );
    writeU16( crossNetOffpage, firstVertex + 4, readU16( crossNetOffpage, crossNetOffpages + 22 ) );
    writeU16( crossNetOffpage, firstVertex + 6, readU16( crossNetOffpage, crossNetOffpages + 24 ) );
    BOOST_CHECK_EXCEPTION( parser.Parse( crossNetOffpage, wxS( "cross-net-offpage.sch" ) ), IO_ERROR,
                           []( const IO_ERROR& aError )
                           {
                               return aError.What().Contains( wxS( "controller 20" ) )
                                      && aError.What().Contains( wxS( "shared across different nets" ) );
                           } );
}


BOOST_AUTO_TEST_CASE( CorpusSemanticSnapshot )
{
    PADS_SCH_BINARY_PARSER                 binaryParser;
    std::vector<CANONICAL_SEMANTIC_RECORD> allActual;

    for( const std::string& fixture :
         { "minimal_v13", "placement_transform", "fields", "connectors", "text_encoding", "page_graphics",
           "connectivity_topology", "multisheet_connectivity", "symbol_primitives", "pin_styles", "multigate" } )
    {
        PADS_SCH_MODEL binary =
                binaryParser.Parse( loadBinaryFixture( fixture + ".sch" ), wxString::FromUTF8( fixture + ".sch" ) );
        PADS_SCH::PADS_SCH_PARSER ascii;
        BOOST_REQUIRE( ascii.Parse( KI_TEST::GetEeschemaTestDataDir() + "/plugins/pads/binary/" + fixture + ".txt" ) );

        auto task10 = []( std::vector<CANONICAL_SEMANTIC_RECORD> aRecords )
        {
            std::erase_if( aRecords,
                           []( const CANONICAL_SEMANTIC_RECORD& aRecord )
                           {
                               return aRecord.kind != CANONICAL_KIND::NET && aRecord.kind != CANONICAL_KIND::CONNECTION
                                      && aRecord.kind != CANONICAL_KIND::BUS
                                      && aRecord.kind != CANONICAL_KIND::BUS_ENTRY
                                      && aRecord.kind != CANONICAL_KIND::BUS_ALIAS
                                      && aRecord.kind != CANONICAL_KIND::BUS_MEMBER
                                      && aRecord.kind != CANONICAL_KIND::LABEL
                                      && aRecord.kind != CANONICAL_KIND::JUNCTION
                                      && ( aRecord.kind != CANONICAL_KIND::GRAPHIC
                                           || !aRecord.properties.contains( "page_graphic_group" ) );
                           } );

            return aRecords;
        };

        const SNAPSHOT_ALLOWLIST sourceProperties = {
            { "global_net_record", PROPERTY_DISPOSITION::EXACT },
            { "preserved_net_identity", PROPERTY_DISPOSITION::PRESERVED },
            { "preserved_net_relationship", PROPERTY_DISPOSITION::PRESERVED },
            { "endpoint_0_raw_endpoint_handle", PROPERTY_DISPOSITION::EXACT },
            { "endpoint_1_raw_endpoint_handle", PROPERTY_DISPOSITION::EXACT },
            { "endpoint_0_raw_endpoint_relationship", PROPERTY_DISPOSITION::PRESERVED },
            { "endpoint_1_raw_endpoint_relationship", PROPERTY_DISPOSITION::PRESERVED },
            { "raw_connection_marker", PROPERTY_DISPOSITION::EXACT },
            { "offpage_variant", PROPERTY_DISPOSITION::EXACT },
            { "connection_record", PROPERTY_DISPOSITION::EXACT },
            { "preserved_drawing_text_relationship", PROPERTY_DISPOSITION::UNSUPPORTED },
            { "preserved_graphic_presentation", PROPERTY_DISPOSITION::PRESERVED },
        };

        BOOST_TEST_CONTEXT( fixture )
        {
            for( const MODEL_NET& net : binary.nets )
            {
                for( const MODEL_CONNECTION& connection : net.connections )
                {
                    BOOST_REQUIRE_GE( connection.vertices.size(), 2 );
                    BOOST_REQUIRE_EQUAL( connection.endpoints.size(), 2 );
                    BOOST_CHECK_EQUAL( connection.endpoints[0].point.x, connection.vertices.front().x );
                    BOOST_CHECK_EQUAL( connection.endpoints[0].point.y, connection.vertices.front().y );
                    BOOST_CHECK_EQUAL( connection.endpoints[1].point.x, connection.vertices.back().x );
                    BOOST_CHECK_EQUAL( connection.endpoints[1].point.y, connection.vertices.back().y );
                    BOOST_CHECK( connection.vertices.front().x != connection.vertices.back().x
                                 || connection.vertices.front().y != connection.vertices.back().y );
                }
            }

            auto expected = task10( normalizeAsciiModel( ascii ) );
            auto actual = task10( normalizeBinaryModel( binary ) );
            BOOST_CHECK( snapshotsMatch( expected, actual, sourceProperties ) );
            allActual.insert( allActual.end(), actual.begin(), actual.end() );
        }
    }

    BOOST_CHECK( task10SourcePropertiesPresent( allActual ) );
    auto missingRawKind = allActual;

    auto label = std::ranges::find( missingRawKind, CANONICAL_KIND::LABEL, &CANONICAL_SEMANTIC_RECORD::kind );
    BOOST_REQUIRE( label != missingRawKind.end() );
    label->properties.erase( "offpage_variant" );

    BOOST_CHECK( !task10SourcePropertiesPresent( missingRawKind ) );

    for( const std::string& endpoint :
         { "endpoint_0_raw_endpoint_handle", "endpoint_1_raw_endpoint_handle", "endpoint_0_raw_endpoint_relationship",
           "endpoint_1_raw_endpoint_relationship" } )
    {
        auto missingEndpoint = allActual;
        auto connection =
                std::ranges::find( missingEndpoint, CANONICAL_KIND::CONNECTION, &CANONICAL_SEMANTIC_RECORD::kind );
        BOOST_REQUIRE( connection != missingEndpoint.end() );
        connection->properties.erase( endpoint );
        BOOST_CHECK( !task10SourcePropertiesPresent( missingEndpoint ) );
    }
}


BOOST_AUTO_TEST_CASE( SymbolHandleErrors )
{
    PADS_SCH_BINARY_PARSER parser;
    std::vector<uint8_t>   wrongClass = loadBinaryFixture( "symbol_primitives.sch" );
    size_t                 usedDecals = sheetControllerOffset( wrongClass, 7 );
    writeU32( wrongClass, usedDecals + 104 * 108 + 48, 8 );
    BOOST_CHECK_EXCEPTION( parser.Parse( wrongClass, wxS( "wrong-class.sch" ) ), IO_ERROR,
                           []( const IO_ERROR& aError )
                           {
                               wxString message = aError.What();
                               return message.Contains( wxS( "wrong object class" ) )
                                      && message.Contains( wxS( "controller 7" ) )
                                      && message.Contains( wxS( "record 104" ) );
                           } );

    std::vector<uint8_t> unresolved = loadBinaryFixture( "multigate.sch" );
    size_t               gates = sheetControllerOffset( unresolved, 10 );
    writeU16( unresolved, gates + 12, 0xFFFF );
    BOOST_CHECK_EXCEPTION( parser.Parse( unresolved, wxS( "unresolved.sch" ) ), IO_ERROR,
                           []( const IO_ERROR& aError )
                           {
                               return aError.What().Contains( wxS( "unresolved symbol definition reference" ) );
                           } );

    std::vector<uint8_t> badPinHandle = loadBinaryFixture( "symbol_primitives.sch" );
    size_t               terminals = sheetControllerOffset( badPinHandle, 8 );
    writeU16( badPinHandle, terminals + 8 * 26, 0xFFFE );
    BOOST_CHECK_EXCEPTION( parser.Parse( badPinHandle, wxS( "bad-pin-handle.sch" ) ), IO_ERROR,
                           []( const IO_ERROR& aError )
                           {
                               return aError.What().Contains( wxS( "pin-decal handle" ) )
                                      && aError.What().Contains( wxS( "controller 8" ) );
                           } );

    std::vector<uint8_t> gateCount = loadBinaryFixture( "multigate.sch" );
    size_t               partTypes = sheetControllerOffset( gateCount, 9 );
    writeU16( gateCount, partTypes + 76 + 68, 3 );
    BOOST_CHECK_EXCEPTION( parser.Parse( gateCount, wxS( "gate-count.sch" ) ), IO_ERROR,
                           []( const IO_ERROR& aError )
                           {
                               return aError.What().Contains( wxS( "gate count" ) )
                                      && aError.What().Contains( wxS( "controller 9" ) );
                           } );

    std::vector<uint8_t> fieldHandle = loadBinaryFixture( "symbol_primitives.sch" );
    size_t               fieldDecals = sheetControllerOffset( fieldHandle, 7 );
    writeU32( fieldHandle, fieldDecals + 104 * 108 + 52, 132 );
    BOOST_CHECK_EXCEPTION( parser.Parse( fieldHandle, wxS( "field-handle.sch" ) ), IO_ERROR,
                           []( const IO_ERROR& aError )
                           {
                               return aError.What().Contains( wxS( "field" ) )
                                      && aError.What().Contains( wxS( "controller 7" ) );
                           } );

    std::vector<uint8_t> alternateHandle = loadBinaryFixture( "multigate.sch" );
    size_t               alternateGates = sheetControllerOffset( alternateHandle, 10 );
    writeU16( alternateHandle, alternateGates + 2, 0xFFFE );
    BOOST_CHECK_EXCEPTION( parser.Parse( alternateHandle, wxS( "alternate-handle.sch" ) ), IO_ERROR,
                           []( const IO_ERROR& aError )
                           {
                               return aError.What().Contains( wxS( "alternate symbol definition" ) )
                                      && aError.What().Contains( wxS( "controller 10" ) );
                           } );

    std::vector<uint8_t> binaryCycle = loadBinaryFixture( "multigate.sch" );
    const size_t         cyclePartTypes = sheetControllerOffset( binaryCycle, 9 );
    const size_t         cycleGates = sheetControllerOffset( binaryCycle, 10 );
    const uint32_t       firstGateRecord = readU32( binaryCycle, cyclePartTypes + 76 + 44 );
    const uint16_t       firstDefinition = readU16( binaryCycle, cycleGates + firstGateRecord * 12 );
    const uint16_t       secondDefinition = readU16( binaryCycle, cycleGates + ( firstGateRecord + 1 ) * 12 );
    writeU16( binaryCycle, cycleGates + firstGateRecord * 12 + 2, secondDefinition );
    writeU16( binaryCycle, cycleGates + ( firstGateRecord + 1 ) * 12 + 2, firstDefinition );
    BOOST_CHECK_EXCEPTION( parser.Parse( binaryCycle, wxS( "definition-cycle.sch" ) ), IO_ERROR,
                           []( const IO_ERROR& aError )
                           {
                               return aError.What().Contains( wxS( "cyclic symbol definition reference" ) )
                                      && aError.What().Contains( wxS( "controller 10" ) );
                           } );

    std::vector<uint8_t> unknownSide = loadBinaryFixture( "pin_styles.sch" );
    const size_t         unknownSideTerminals = sheetControllerOffset( unknownSide, 8 );
    writeU16( unknownSide, unknownSideTerminals + 22, 8 );
    PADS_SCH_MODEL unknownSideModel = parser.Parse( unknownSide, wxS( "unknown-side.sch" ) );
    BOOST_CHECK( std::ranges::any_of( unknownSideModel.diagnostics,
                                      []( const PARSER_DIAGNOSTIC& aDiagnostic )
                                      {
                                          return aDiagnostic.message.Contains( wxS( "terminal side" ) )
                                                 && aDiagnostic.source.controller == 8
                                                 && aDiagnostic.source.recordIndex == 0;
                                      } ) );

    std::vector<uint8_t> unknownLength = loadBinaryFixture( "pin_styles.sch" );
    const PADS_SCH_MODEL unknownLengthBaseline = parser.Parse( unknownLength, wxS( "known-length.sch" ) );
    const size_t         unknownLengthTerminalRecord = static_cast<size_t>(
            itemNamed( unknownLengthBaseline.definitions, wxS( "BATCHB_PIN_STYLES" ) ).pins[0].source.recordIndex );
    const size_t   unknownLengthTerminals = sheetControllerOffset( unknownLength, 8 );
    const uint16_t pinDecalHandle = readU16( unknownLength, unknownLengthTerminals + unknownLengthTerminalRecord * 26 );
    const size_t   unknownLengthDecals = sheetControllerOffset( unknownLength, 7 );
    const uint32_t pinDecalDefinition = readU32( unknownLength, unknownLengthDecals + pinDecalHandle * 108 + 48 );
    const size_t   unknownLengthDefinitions = sheetControllerOffset( unknownLength, 3 );
    const uint32_t pinDecalVertex = readU32( unknownLength, unknownLengthDefinitions + pinDecalDefinition * 80 + 0x34 );
    const uint32_t pinDecalVertexEnd =
            readU32( unknownLength, unknownLengthDefinitions + ( pinDecalDefinition + 1 ) * 80 + 0x34 );
    const size_t unknownLengthVertices = sheetControllerOffset( unknownLength, 5 );

    for( uint32_t vertex = pinDecalVertex; vertex < pinDecalVertexEnd; ++vertex )
    {
        const size_t vertexOffset = unknownLengthVertices + vertex * 6;

        if( readU16( unknownLength, vertexOffset ) == 0 && readU16( unknownLength, vertexOffset + 2 ) == 0 )
            writeU16( unknownLength, vertexOffset, 1 );
    }
    PADS_SCH_MODEL              unknownLengthModel = parser.Parse( unknownLength, wxS( "unknown-length.sch" ) );
    const MODEL_PIN_DEFINITION& unknownLengthPin =
            itemNamed( unknownLengthModel.definitions, wxS( "BATCHB_PIN_STYLES" ) ).pins[0];
    BOOST_CHECK_EQUAL( unknownLengthPin.length, 0 );
    BOOST_CHECK( std::ranges::any_of( unknownLengthPin.properties,
                                      []( const SOURCE_PROPERTY& aProperty )
                                      {
                                          return aProperty.name.text == wxS( "pin_length" )
                                                 && aProperty.disposition == PROPERTY_DISPOSITION::UNSUPPORTED;
                                      } ) );

    std::vector<uint8_t> pieceCount = loadBinaryFixture( "symbol_primitives.sch" );
    size_t               definitions = sheetControllerOffset( pieceCount, 3 );
    pieceCount[definitions + 14 * 80 + 43] = 1;
    BOOST_CHECK_EXCEPTION( parser.Parse( pieceCount, wxS( "piece-count.sch" ) ), IO_ERROR,
                           []( const IO_ERROR& aError )
                           {
                               return aError.What().Contains( wxS( "graphic-piece counts" ) )
                                      && aError.What().Contains( wxS( "controller 3" ) );
                           } );

    std::vector<uint8_t> ownership = loadBinaryFixture( "symbol_primitives.sch" );
    size_t               ownershipDefinitions = sheetControllerOffset( ownership, 3 );
    const size_t         vertexPrefix = ownershipDefinitions + 14 * 80 + 0x34;
    writeU32( ownership, vertexPrefix, readU32( ownership, vertexPrefix ) + 1 );
    BOOST_CHECK_EXCEPTION( parser.Parse( ownership, wxS( "piece-ownership.sch" ) ), IO_ERROR,
                           []( const IO_ERROR& aError )
                           {
                               return aError.What().Contains( wxS( "ownership mismatch" ) )
                                      && aError.What().Contains( wxS( "controller 3" ) );
                           } );

    auto duplicateModel = [&]()
    {
        return parser.Parse( loadBinaryFixture( "multigate.sch" ), wxS( "duplicate-id.sch" ) );
    };
    PADS_SCH_MODEL duplicateDefinition = duplicateModel();
    duplicateDefinition.definitions[1].id = duplicateDefinition.definitions[0].id;
    BOOST_CHECK_EXCEPTION( duplicateDefinition.ValidateOrThrow(), IO_ERROR,
                           []( const IO_ERROR& aError )
                           {
                               return aError.What().Contains( wxS( "duplicate definition ID" ) );
                           } );
    PADS_SCH_MODEL duplicatePart = duplicateModel();
    duplicatePart.partTypes[1].id = duplicatePart.partTypes[0].id;
    BOOST_CHECK_EXCEPTION( duplicatePart.ValidateOrThrow(), IO_ERROR,
                           []( const IO_ERROR& aError )
                           {
                               return aError.What().Contains( wxS( "duplicate part type ID" ) );
                           } );
    PADS_SCH_MODEL duplicateGate = duplicateModel();
    duplicateGate.partTypes[1].gates[1].id = duplicateGate.partTypes[1].gates[0].id;
    BOOST_CHECK_EXCEPTION( duplicateGate.ValidateOrThrow(), IO_ERROR,
                           []( const IO_ERROR& aError )
                           {
                               return aError.What().Contains( wxS( "duplicate gate ID" ) );
                           } );
    PADS_SCH_MODEL                 duplicatePin = duplicateModel();
    const MODEL_SYMBOL_DEFINITION& pinOwner = itemNamed( duplicatePin.definitions, wxS( "BATCHB_PIN_STYLES" ) );
    size_t                         pinOwnerIndex = &pinOwner - duplicatePin.definitions.data();
    duplicatePin.definitions[pinOwnerIndex].pins[1].id = duplicatePin.definitions[pinOwnerIndex].pins[0].id;
    BOOST_CHECK_EXCEPTION( duplicatePin.ValidateOrThrow(), IO_ERROR,
                           []( const IO_ERROR& aError )
                           {
                               return aError.What().Contains( wxS( "duplicate pin ID" ) );
                           } );

    PADS_SCH_MODEL duplicateField = duplicateModel();
    duplicateField.definitions[0].fields.push_back( duplicateField.definitions[1].fields[0] );
    duplicateField.definitions[0].fields.back().source = duplicateField.definitions[0].source;
    BOOST_CHECK_EXCEPTION( duplicateField.ValidateOrThrow(), IO_ERROR,
                           []( const IO_ERROR& aError )
                           {
                               return aError.What().Contains( wxS( "duplicate field ID" ) )
                                      && aError.What().Contains( wxS( "first at" ) );
                           } );

    std::vector<uint8_t> duplicateBinaryField = loadBinaryFixture( "fields.sch" );
    const size_t         duplicateFieldDecals = sheetControllerOffset( duplicateBinaryField, 7 );
    const uint32_t       textCount = sheetControllerCount( duplicateBinaryField, 1 );
    std::vector<size_t>  fieldRecords;

    for( size_t record = 0; record < sheetControllerCount( duplicateBinaryField, 7 ); ++record )
    {
        if( readU32( duplicateBinaryField, duplicateFieldDecals + record * 108 + 52 ) < textCount )
            fieldRecords.push_back( record );
    }

    BOOST_REQUIRE_GE( fieldRecords.size(), 2 );
    writeU32( duplicateBinaryField, duplicateFieldDecals + fieldRecords[1] * 108 + 52,
              readU32( duplicateBinaryField, duplicateFieldDecals + fieldRecords[0] * 108 + 52 ) );
    BOOST_CHECK_EXCEPTION( parser.Parse( duplicateBinaryField, wxS( "duplicate-field.sch" ) ), IO_ERROR,
                           []( const IO_ERROR& aError )
                           {
                               return aError.What().Contains( wxS( "duplicate field ID" ) )
                                      && aError.What().Contains( wxS( "controller 7" ) )
                                      && aError.What().Contains( wxS( "first at" ) );
                           } );

    std::vector<uint8_t> connectorPinName = loadBinaryFixture( "connectors.sch" );
    const size_t         connectorParts = sheetControllerOffset( connectorPinName, 9 );
    const size_t         connectorPins = sheetControllerOffset( connectorPinName, 11 );
    const uint32_t       connectorPinStart = readU32( connectorPinName, connectorParts + 76 + 48 );
    writeU32( connectorPinName, connectorPins + connectorPinStart * 24, 0xFFFFFFFE );
    BOOST_CHECK_EXCEPTION( parser.Parse( connectorPinName, wxS( "connector-pin-name.sch" ) ), IO_ERROR,
                           []( const IO_ERROR& aError )
                           {
                               return aError.What().Contains( wxS( "pin-name offset leaves controller 14" ) )
                                      && aError.What().Contains( wxS( "controller 11" ) );
                           } );
}


BOOST_AUTO_TEST_CASE( SymbolDefinitionSemanticSnapshot )
{
    PADS_SCH_BINARY_PARSER binaryParser;
    PADS_SCH_MODEL binary = binaryParser.Parse( loadBinaryFixture( "pin_styles.sch" ), wxS( "pin_styles.sch" ) );
    PADS_SCH::PADS_SCH_PARSER asciiParser;
    BOOST_REQUIRE( asciiParser.Parse( KI_TEST::GetEeschemaTestDataDir() + "/plugins/pads/binary/pin_styles.txt" ) );

    const MODEL_SYMBOL_DEFINITION& binaryDefinition = itemNamed( binary.definitions, wxS( "BATCHB_PIN_STYLES" ) );
    auto                           asciiDefinition = std::ranges::find_if( asciiParser.GetSymbolDefs(),
                                                                           []( const PADS_SCH::SYMBOL_DEF& aDefinition )
                                                                           {
                                                     return aDefinition.name == "BATCHB_PIN_STYLES";
                                                 } );
    BOOST_REQUIRE( asciiDefinition != asciiParser.GetSymbolDefs().end() );
    BOOST_CHECK_EQUAL( binaryDefinition.graphics.size(), asciiDefinition->graphics.size() );
    BOOST_CHECK_EQUAL( binaryDefinition.pins.size(), asciiDefinition->pins.size() );
    BOOST_CHECK_EQUAL( binaryDefinition.pins[0].position.x,
                       static_cast<int64_t>( std::llround( asciiDefinition->pins[0].position.x * 2 ) ) );
    BOOST_CHECK_EQUAL( binaryDefinition.pins[4].graphicStyle,
                       ( asciiDefinition->pins[4].inverted ? 1U : 0U ) | ( asciiDefinition->pins[4].clock ? 2U : 0U ) );

    BOOST_REQUIRE_EQUAL( binaryDefinition.graphics.size(), asciiDefinition->graphics.size() );

    for( size_t i = 0; i < binaryDefinition.graphics.size(); ++i )
    {
        BOOST_CHECK( canonicalGraphicType( binaryDefinition.graphics[i].kind )
                     == canonicalGraphicType( asciiDefinition->graphics[i].type ) );
        BOOST_CHECK_EQUAL( binaryDefinition.graphics[i].strokeWidth,
                           std::llround( asciiDefinition->graphics[i].line_width * 2 ) );
        BOOST_CHECK( canonicalFill( binaryDefinition.graphics[i].fill )
                     == canonicalFill( asciiDefinition->graphics[i].filled ) );
        BOOST_CHECK_EQUAL( binaryDefinition.graphics[i].points.size(), asciiDefinition->graphics[i].points.size() );
    }

    BOOST_REQUIRE_EQUAL( binaryDefinition.pins.size(), asciiDefinition->pins.size() );
    const PADS_SCH::GATE_DEF& asciiGate = asciiParser.GetPartTypes().at( "BATCHB-PIN-STYLES" ).gates[0];
    BOOST_REQUIRE_EQUAL( asciiGate.pins.size(), binaryDefinition.pins.size() );

    auto checkPinPresentation = []( const MODEL_PIN_DEFINITION& aBinaryPin, const PADS_SCH::SYMBOL_PIN& aAsciiPin )
    {
        BOOST_CHECK( canonicalPinStyle( aBinaryPin.graphicStyle )
                     == canonicalPinStyle( aAsciiPin.inverted, aAsciiPin.clock ) );
        BOOST_CHECK_EQUAL( aBinaryPin.length, std::llround( aAsciiPin.length * 2 ) );
        BOOST_CHECK_EQUAL( aBinaryPin.position.x, std::llround( aAsciiPin.position.x * 2 ) );
        BOOST_CHECK_EQUAL( aBinaryPin.position.y, std::llround( aAsciiPin.position.y * 2 ) );
        BOOST_CHECK_EQUAL( aBinaryPin.side, aAsciiPin.side );
        BOOST_CHECK_EQUAL( aBinaryPin.angle, std::llround( aAsciiPin.rotation * 10 ) );
        BOOST_CHECK_EQUAL( aBinaryPin.namePresentation.height, aAsciiPin.pn_h );
        BOOST_CHECK_EQUAL( aBinaryPin.namePresentation.width, aAsciiPin.pn_w );
        BOOST_CHECK_EQUAL( aBinaryPin.numberPresentation.height, aAsciiPin.pl_h );
        BOOST_CHECK_EQUAL( aBinaryPin.numberPresentation.width, aAsciiPin.pl_w );
        BOOST_CHECK_EQUAL( aBinaryPin.nameOffset.x, aAsciiPin.pn_offset.x * 2 );
        BOOST_CHECK_EQUAL( aBinaryPin.nameOffset.y, aAsciiPin.pn_offset.y * 2 );
        BOOST_CHECK_EQUAL( aBinaryPin.numberOffset.x, aAsciiPin.pl_offset.x * 2 );
        BOOST_CHECK_EQUAL( aBinaryPin.numberOffset.y, aAsciiPin.pl_offset.y * 2 );
        BOOST_CHECK_EQUAL( aBinaryPin.nameAngle, aAsciiPin.pn_angle * 10 );
        BOOST_CHECK_EQUAL( aBinaryPin.numberAngle, aAsciiPin.pl_angle * 10 );
        BOOST_CHECK_EQUAL( aBinaryPin.nameJustification, aAsciiPin.pn_just );
        BOOST_CHECK_EQUAL( aBinaryPin.numberJustification, aAsciiPin.pl_just );
        BOOST_CHECK_EQUAL( aBinaryPin.nameOffsetAngle, aAsciiPin.pn_off_angle * 10 );
        BOOST_CHECK_EQUAL( aBinaryPin.numberOffsetAngle, aAsciiPin.pl_off_angle * 10 );
        BOOST_CHECK_EQUAL( aBinaryPin.nameOffsetJustification, aAsciiPin.pn_off_just );
        BOOST_CHECK_EQUAL( aBinaryPin.numberOffsetJustification, aAsciiPin.pl_off_just );
        BOOST_CHECK_EQUAL( aBinaryPin.visibilityFlags, aAsciiPin.p_flags );
        BOOST_CHECK_EQUAL( aBinaryPin.namePresentation.visible,
                           aAsciiPin.pn_h != 0 && ( aAsciiPin.p_flags & 128 ) == 0 );
        BOOST_CHECK_EQUAL( aBinaryPin.numberPresentation.visible, aAsciiPin.pl_h != 0 );
    };

    for( size_t i = 0; i < binaryDefinition.pins.size(); ++i )
    {
        BOOST_CHECK_EQUAL( binaryDefinition.pins[i].number.text, wxString::FromUTF8( asciiGate.pins[i].pin_id ) );
        BOOST_CHECK_EQUAL( binaryDefinition.pins[i].name.text, wxString::FromUTF8( asciiGate.pins[i].pin_name ) );
        BOOST_CHECK( canonicalPinType( binaryDefinition.pins[i].electricalType )
                     == canonicalPinType( asciiGate.pins[i].pin_type ) );
        checkPinPresentation( binaryDefinition.pins[i], asciiDefinition->pins[i] );
    }

    PADS_SCH_MODEL primitiveBinary =
            binaryParser.Parse( loadBinaryFixture( "symbol_primitives.sch" ), wxS( "symbol_primitives.sch" ) );
    PADS_SCH::PADS_SCH_PARSER primitiveAscii;
    BOOST_REQUIRE(
            primitiveAscii.Parse( KI_TEST::GetEeschemaTestDataDir() + "/plugins/pads/binary/symbol_primitives.txt" ) );
    const MODEL_SYMBOL_DEFINITION& primitiveDefinition =
            itemNamed( primitiveBinary.definitions, wxS( "BATCHB_PRIMITIVES" ) );
    const PADS_SCH::SYMBOL_DEF* asciiPrimitive = primitiveAscii.GetSymbolDef( "BATCHB_PRIMITIVES" );
    BOOST_REQUIRE( asciiPrimitive );
    BOOST_REQUIRE_EQUAL( primitiveDefinition.graphics.size(),
                         asciiPrimitive->graphics.size() + asciiPrimitive->texts.size() );

    for( size_t graphic = 0; graphic < asciiPrimitive->graphics.size(); ++graphic )
    {
        BOOST_CHECK( canonicalGraphicType( primitiveDefinition.graphics[graphic].kind )
                     == canonicalGraphicType( asciiPrimitive->graphics[graphic] ) );
        BOOST_CHECK_EQUAL( primitiveDefinition.graphics[graphic].strokeWidth,
                           std::llround( asciiPrimitive->graphics[graphic].line_width * 2 ) );
        BOOST_CHECK( primitiveDefinition.graphics[graphic].lineStyle == MODEL_LINE_STYLE::SOLID );
        BOOST_CHECK( canonicalFill( primitiveDefinition.graphics[graphic].fill )
                     == canonicalFill( asciiPrimitive->graphics[graphic].filled ) );
        BOOST_REQUIRE_EQUAL( primitiveDefinition.graphics[graphic].points.size(),
                             asciiPrimitive->graphics[graphic].points.size() );

        for( size_t pointIndex = 0; pointIndex < asciiPrimitive->graphics[graphic].points.size(); ++pointIndex )
        {
            BOOST_CHECK_EQUAL( primitiveDefinition.graphics[graphic].points[pointIndex].x,
                               std::llround( asciiPrimitive->graphics[graphic].points[pointIndex].coord.x * 2 ) );
            BOOST_CHECK_EQUAL( primitiveDefinition.graphics[graphic].points[pointIndex].y,
                               std::llround( asciiPrimitive->graphics[graphic].points[pointIndex].coord.y * 2 ) );
        }
    }

    const MODEL_GRAPHIC& binaryText = primitiveDefinition.graphics.back();
    BOOST_REQUIRE_EQUAL( asciiPrimitive->texts.size(), 1 );
    BOOST_CHECK_EQUAL( binaryText.text.text, wxString::FromUTF8( asciiPrimitive->texts[0].content ) );
    BOOST_CHECK_EQUAL( binaryText.points[0].x, std::llround( asciiPrimitive->texts[0].position.x * 2 ) );
    BOOST_CHECK_EQUAL( binaryText.points[0].y, std::llround( asciiPrimitive->texts[0].position.y * 2 ) );
    BOOST_CHECK_EQUAL( binaryText.presentation.height, std::llround( asciiPrimitive->texts[0].size ) );
    BOOST_CHECK_EQUAL( binaryText.presentation.width, asciiPrimitive->texts[0].width_factor );
    BOOST_CHECK_EQUAL( binaryText.presentation.font.text, wxString::FromUTF8( asciiPrimitive->font1 ) );
    BOOST_CHECK_EQUAL( binaryText.angle, std::llround( asciiPrimitive->texts[0].rotation * 10 ) );
    BOOST_CHECK_EQUAL( binaryText.presentation.visible, asciiPrimitive->texts[0].visible );
    BOOST_CHECK( canonicalJustification( binaryText.presentation.horizontalJustification ).value
                 == canonicalHorizontalJustification( asciiPrimitive->texts[0].justification ).value );
    BOOST_CHECK( canonicalVerticalJustification( binaryText.presentation.verticalJustification ).value
                 == canonicalVerticalJustification( asciiPrimitive->texts[0].justification ).value );

    PADS_SCH_MODEL fieldsBinary = binaryParser.Parse( loadBinaryFixture( "fields.sch" ), wxS( "fields.sch" ) );
    PADS_SCH::PADS_SCH_PARSER fieldsAscii;
    BOOST_REQUIRE( fieldsAscii.Parse( KI_TEST::GetEeschemaTestDataDir() + "/plugins/pads/binary/fields.txt" ) );
    const MODEL_SYMBOL_DEFINITION& fieldsDefinition = itemNamed( fieldsBinary.definitions, wxS( "RESZ-H" ) );
    const PADS_SCH::SYMBOL_DEF*    asciiFields = fieldsAscii.GetSymbolDef( "RESZ-H" );
    BOOST_REQUIRE( asciiFields );
    BOOST_REQUIRE_EQUAL( fieldsDefinition.fields.size(), asciiFields->attrs.size() );

    for( size_t field = 0; field < asciiFields->attrs.size(); ++field )
    {
        BOOST_TEST_CONTEXT( "field " << field )
        {
            BOOST_CHECK_EQUAL( fieldsDefinition.fields[field].name.text,
                               wxString::FromUTF8( asciiFields->attrs[field].attr_name ) );
            BOOST_CHECK( fieldsDefinition.fields[field].value.text.empty() );
            BOOST_CHECK_EQUAL( fieldsDefinition.fields[field].position.x,
                               std::llround( asciiFields->attrs[field].position.x * 2 ) );
            BOOST_CHECK_EQUAL( fieldsDefinition.fields[field].position.y,
                               std::llround( asciiFields->attrs[field].position.y * 2 ) );
            BOOST_CHECK_EQUAL( fieldsDefinition.fields[field].angle, asciiFields->attrs[field].angle * 10 );
            BOOST_CHECK_EQUAL( fieldsDefinition.fields[field].presentation.height, asciiFields->attrs[field].height );
            BOOST_CHECK_EQUAL( fieldsDefinition.fields[field].presentation.width, asciiFields->attrs[field].width );
            BOOST_CHECK_EQUAL( fieldsDefinition.fields[field].presentation.font.text,
                               wxString::FromUTF8( asciiFields->attrs[field].font_name ) );
            BOOST_CHECK(
                    canonicalJustification( fieldsDefinition.fields[field].presentation.horizontalJustification ).value
                    == canonicalHorizontalJustification( asciiFields->attrs[field].justification ).value );
            BOOST_CHECK(
                    canonicalVerticalJustification( fieldsDefinition.fields[field].presentation.verticalJustification )
                            .value
                    == canonicalVerticalJustification( asciiFields->attrs[field].justification ).value );
        }
    }

    const MODEL_PART_TYPE& fieldsPart = itemNamed( fieldsBinary.partTypes, wxS( "RES-RESN1" ) );
    BOOST_REQUIRE_EQUAL( fieldsPart.fields.size(), fieldsDefinition.fields.size() );

    for( size_t field = 0; field < fieldsPart.fields.size(); ++field )
    {
        BOOST_CHECK_EQUAL( fieldsPart.fields[field].name.text, fieldsDefinition.fields[field].name.text );
        BOOST_CHECK_EQUAL( fieldsPart.fields[field].value.text, fieldsDefinition.fields[field].value.text );
        BOOST_CHECK( fieldsPart.fields[field].presentation == fieldsDefinition.fields[field].presentation );
    }

    PADS_SCH_MODEL multiBinary = binaryParser.Parse( loadBinaryFixture( "multigate.sch" ), wxS( "multigate.sch" ) );
    PADS_SCH::PADS_SCH_PARSER multiAscii;
    BOOST_REQUIRE( multiAscii.Parse( KI_TEST::GetEeschemaTestDataDir() + "/plugins/pads/binary/multigate.txt" ) );
    const MODEL_PART_TYPE&        multiPart = itemNamed( multiBinary.partTypes, wxS( "BATCHD-MULTIGATE" ) );
    const PADS_SCH::PARTTYPE_DEF& asciiMulti = multiAscii.GetPartTypes().at( "BATCHD-MULTIGATE" );
    BOOST_REQUIRE_EQUAL( multiPart.gates.size(), asciiMulti.gates.size() );
    BOOST_REQUIRE_EQUAL( multiPart.signalPins.size(), asciiMulti.sigpins.size() );

    auto definitionName = [&]( const PADS_SCH_MODEL& aModel, const DEFINITION_REFERENCE& aReference )
    {
        auto definition = std::ranges::find_if( aModel.definitions,
                                                [&]( const MODEL_SYMBOL_DEFINITION& aDefinition )
                                                {
                                                    return aDefinition.id == aReference.id;
                                                } );
        BOOST_REQUIRE( definition != aModel.definitions.end() );
        return definition->name.text;
    };

    auto pinDefinition = []( const PADS_SCH_MODEL& aModel,
                             const PIN_REFERENCE&  aReference ) -> const MODEL_PIN_DEFINITION&
    {
        for( const MODEL_SYMBOL_DEFINITION& definition : aModel.definitions )
        {
            auto pin = std::ranges::find_if( definition.pins,
                                             [&]( const MODEL_PIN_DEFINITION& aPin )
                                             {
                                                 return aPin.id == aReference.id;
                                             } );

            if( pin != definition.pins.end() )
                return *pin;
        }

        BOOST_FAIL( "unresolved pin reference" );
        throw std::logic_error( "unreachable" );
    };

    for( size_t gate = 0; gate < multiPart.gates.size(); ++gate )
    {
        BOOST_CHECK_EQUAL( definitionName( multiBinary, multiPart.gates[gate].definition ),
                           wxString::FromUTF8( asciiMulti.gates[gate].decal_names.front() ) );
        BOOST_CHECK_EQUAL( propertyValue( multiPart.gates[gate].properties, wxS( "swap_group" ) ),
                           wxString::Format( wxS( "%d" ), asciiMulti.gates[gate].swap_flag ) );
        BOOST_CHECK_EQUAL( multiPart.gates[gate].pins.size(), asciiMulti.gates[gate].pins.size() );

        for( size_t pin = 0; pin < multiPart.gates[gate].pins.size(); ++pin )
        {
            const MODEL_PIN_DEFINITION& binaryPin = pinDefinition( multiBinary, multiPart.gates[gate].pins[pin] );
            BOOST_CHECK_EQUAL( binaryPin.number.text, wxString::FromUTF8( asciiMulti.gates[gate].pins[pin].pin_id ) );
            BOOST_CHECK_EQUAL( binaryPin.name.text, wxString::FromUTF8( asciiMulti.gates[gate].pins[pin].pin_name ) );
            BOOST_CHECK_EQUAL( propertyValue( binaryPin.properties, wxS( "swap_group" ) ),
                               wxString::Format( wxS( "%d" ), asciiMulti.gates[gate].pins[pin].swap_group ) );
        }
    }

    const MODEL_PART_TYPE&    alternatePart = itemNamed( multiBinary.partTypes, wxS( "RES-RESN1" ) );
    const PADS_SCH::GATE_DEF& asciiAlternates = multiAscii.GetPartTypes().at( "RES-RESN1" ).gates[0];
    BOOST_REQUIRE_EQUAL( alternatePart.gates[0].alternateDefinitions.size() + 1, asciiAlternates.decal_names.size() );
    BOOST_CHECK_EQUAL( definitionName( multiBinary, alternatePart.gates[0].definition ),
                       wxString::FromUTF8( asciiAlternates.decal_names[0] ) );

    for( size_t alternate = 0; alternate < alternatePart.gates[0].alternateDefinitions.size(); ++alternate )
    {
        BOOST_CHECK_EQUAL( definitionName( multiBinary, alternatePart.gates[0].alternateDefinitions[alternate] ),
                           wxString::FromUTF8( asciiAlternates.decal_names[alternate + 1] ) );
    }

    for( size_t signal = 0; signal < multiPart.signalPins.size(); ++signal )
    {
        BOOST_CHECK_EQUAL( multiPart.signalPins[signal].number.text,
                           wxString::FromUTF8( asciiMulti.sigpins[signal].pin_number ) );
        BOOST_CHECK_EQUAL( multiPart.signalPins[signal].name.text,
                           wxString::FromUTF8( asciiMulti.sigpins[signal].net_name ) );
    }

    PADS_SCH_MODEL connectorsBinary =
            binaryParser.Parse( loadBinaryFixture( "connectors.sch" ), wxS( "connectors.sch" ) );
    PADS_SCH::PADS_SCH_PARSER connectorsAscii;
    BOOST_REQUIRE( connectorsAscii.Parse( KI_TEST::GetEeschemaTestDataDir() + "/plugins/pads/binary/connectors.txt" ) );
    const MODEL_PART_TYPE&    connectorPart = itemNamed( connectorsBinary.partTypes, wxS( "CON-26P-ED" ) );
    const PADS_SCH::GATE_DEF& asciiConnector = connectorsAscii.GetPartTypes().at( "CON-26P-ED" ).gates[0];
    BOOST_REQUIRE_EQUAL( connectorPart.gates[0].decalGroupMembers.size(), asciiConnector.decal_names.size() );

    for( size_t member = 0; member < asciiConnector.decal_names.size(); ++member )
    {
        BOOST_CHECK_EQUAL( definitionName( connectorsBinary, connectorPart.gates[0].decalGroupMembers[member] ),
                           wxString::FromUTF8( asciiConnector.decal_names[member] ) );
    }

    const std::array<std::string, 11> pairedFixtures = { "minimal_v13",
                                                         "placement_transform",
                                                         "fields",
                                                         "connectors",
                                                         "text_encoding",
                                                         "page_graphics",
                                                         "connectivity_topology",
                                                         "multisheet_connectivity",
                                                         "symbol_primitives",
                                                         "pin_styles",
                                                         "multigate" };

    for( const std::string& fixture : pairedFixtures )
    {
        PADS_SCH_MODEL fixtureBinary =
                binaryParser.Parse( loadBinaryFixture( fixture + ".sch" ), wxString::FromUTF8( fixture ) );
        PADS_SCH::PADS_SCH_PARSER fixtureAscii;
        BOOST_REQUIRE(
                fixtureAscii.Parse( KI_TEST::GetEeschemaTestDataDir() + "/plugins/pads/binary/" + fixture + ".txt" ) );

        for( const MODEL_SYMBOL_DEFINITION& binarySymbol : fixtureBinary.definitions )
        {
            if( binarySymbol.pins.empty() )
                continue;

            const PADS_SCH::SYMBOL_DEF* asciiSymbol = fixtureAscii.GetSymbolDef( binarySymbol.name.text.ToStdString() );
            BOOST_REQUIRE_MESSAGE( asciiSymbol, fixture + ": " + binarySymbol.name.text.ToStdString() );
            BOOST_REQUIRE_EQUAL( binarySymbol.pins.size(), asciiSymbol->pins.size() );

            for( size_t pin = 0; pin < binarySymbol.pins.size(); ++pin )
            {
                BOOST_TEST_CONTEXT( fixture << ": " << binarySymbol.name.text << " pin " << pin << " flags "
                                            << binarySymbol.pins[pin].presentationFlags << "/"
                                            << binarySymbol.pins[pin].visibilityAndNumberPresentationFlags )
                {
                    checkPinPresentation( binarySymbol.pins[pin], asciiSymbol->pins[pin] );
                }
            }
        }

        for( const MODEL_PART_TYPE& binaryPart : fixtureBinary.partTypes )
        {
            const auto asciiPart = fixtureAscii.GetPartTypes().find( binaryPart.name.text.ToStdString() );
            BOOST_REQUIRE_MESSAGE( asciiPart != fixtureAscii.GetPartTypes().end(),
                                   fixture + ": " + binaryPart.name.text.ToStdString() );

            if( asciiPart->second.gates.empty() )
                continue;

            BOOST_REQUIRE_EQUAL( binaryPart.gates.size(), asciiPart->second.gates.size() );

            for( size_t gate = 0; gate < binaryPart.gates.size(); ++gate )
            {
                const MODEL_GATE&         binaryGate = binaryPart.gates[gate];
                const PADS_SCH::GATE_DEF& asciiGateForPart = asciiPart->second.gates[gate];

                if( !binaryGate.decalGroupMembers.empty() )
                {
                    BOOST_REQUIRE_EQUAL( binaryGate.connectorPins.size(), asciiGateForPart.pins.size() );

                    for( size_t pin = 0; pin < binaryGate.connectorPins.size(); ++pin )
                    {
                        const MODEL_CONNECTOR_PIN& binaryPin = binaryGate.connectorPins[pin];
                        BOOST_TEST_CONTEXT( fixture << ": " << binaryPart.name.text << " connector pin " << pin )
                        {
                            BOOST_CHECK_EQUAL( binaryPin.number.text,
                                               wxString::FromUTF8( asciiGateForPart.pins[pin].pin_id ) );
                            BOOST_CHECK_EQUAL( binaryPin.name.text,
                                               wxString::FromUTF8( asciiGateForPart.pins[pin].pin_name ) );
                            BOOST_CHECK( canonicalPinType( binaryPin.electricalType )
                                         == canonicalPinType( asciiGateForPart.pins[pin].pin_type ) );
                        }
                    }

                    continue;
                }

                BOOST_REQUIRE_EQUAL( binaryGate.pins.size(), asciiGateForPart.pins.size() );
                BOOST_REQUIRE_EQUAL( binaryGate.logicalPins.size(), asciiGateForPart.pins.size() );

                for( size_t pin = 0; pin < binaryGate.pins.size(); ++pin )
                {
                    const MODEL_GATE_PIN& binaryPin = binaryGate.logicalPins[pin];
                    BOOST_TEST_CONTEXT( fixture << ": " << binaryPart.name.text << " gate " << gate << " pin " << pin )
                    {
                        BOOST_CHECK_EQUAL( binaryPin.number.text,
                                           wxString::FromUTF8( asciiGateForPart.pins[pin].pin_id ) );
                        BOOST_CHECK_EQUAL( binaryPin.name.text,
                                           wxString::FromUTF8( asciiGateForPart.pins[pin].pin_name ) );
                        BOOST_CHECK( canonicalPinType( binaryPin.electricalType )
                                     == canonicalPinType( asciiGateForPart.pins[pin].pin_type ) );
                    }
                }
            }
        }
    }
}


BOOST_AUTO_TEST_SUITE_END()
