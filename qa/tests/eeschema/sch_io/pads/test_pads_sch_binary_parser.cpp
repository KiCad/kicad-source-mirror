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
#include <cmath>
#include <fstream>
#include <iterator>
#include <map>
#include <optional>
#include <set>
#include <sstream>
#include <string>
#include <type_traits>
#include <variant>
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

using CANONICAL_VALUE = std::variant<int64_t, bool, std::string>;

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

static void addSourceProperties( CANONICAL_SEMANTIC_RECORD& aRecord, const std::vector<SOURCE_PROPERTY>& aProperties )
{
    for( const SOURCE_PROPERTY& property : aProperties )
        aRecord.properties[property.name.text.ToStdString()] = { property.value.text.ToStdString(),
                                                                 property.disposition };
}

static std::vector<CANONICAL_SEMANTIC_RECORD> normalizeBinaryModel( const PADS_SCH_MODEL& aModel )
{
    std::vector<CANONICAL_SEMANTIC_RECORD> out;
    auto add = [&]( CANONICAL_KIND aKind, int aSheet, std::string aKey = {} ) -> CANONICAL_SEMANTIC_RECORD&
    {
        out.push_back( { aSheet, aKind, std::move( aKey ) } );
        return out.back();
    };
    auto graphic = [&]( const MODEL_GRAPHIC& aGraphic, int aSheet )
    {
        auto& r = add( CANONICAL_KIND::GRAPHIC, aSheet );
        r.properties["type"] = { static_cast<int64_t>( aGraphic.kind ) };
        r.properties["line_style"] = { static_cast<int64_t>( aGraphic.lineStyle ) };
        r.properties["stroke_half_mils"] = { aGraphic.strokeWidth };
        r.properties["fill"] = { aGraphic.fill != MODEL_FILL_STYLE::NONE };
        r.properties["text"] = { aGraphic.text.text.ToStdString() };
        r.properties["font"] = { aGraphic.presentation.font.text.ToStdString() };
        r.properties["visible"] = { aGraphic.presentation.visible };
        for( const SOURCE_POINT& p : aGraphic.points )
            r.geometry.points.push_back( point( p ) );
        addSourceProperties( r, aGraphic.properties );
    };
    auto field = [&]( const MODEL_FIELD& aField, int aSheet )
    {
        auto& r = add( CANONICAL_KIND::FIELD, aSheet, aField.name.text.ToStdString() );
        r.properties["value"] = { aField.value.text.ToStdString() };
        r.properties["visible"] = { aField.visible };
        r.properties["font"] = { aField.presentation.font.text.ToStdString() };
        r.geometry.points.push_back( point( aField.position ) );
        r.geometry.angleTenths = aField.angle;
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
            r.properties["type"] = { int64_t( p.electricalType ) };
            r.properties["style"] = { int64_t( p.graphicStyle ) };
            r.properties["length_half_mils"] = { p.length };
            r.properties["font"] = { p.presentation.font.text.ToStdString() };
            r.properties["visible"] = { p.presentation.visible };
            r.geometry.points.push_back( point( p.position ) );
            r.geometry.angleTenths = p.angle;
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
            r.properties["definition_id"] = { int64_t( g.definition.id.Value() ) };
            addSourceProperties( r, g.properties );
            for( size_t i = 0; i < g.pins.size(); ++i )
            {
                auto& m = add( CANONICAL_KIND::GATE_PIN_MAPPING, g.source.sheet,
                               std::to_string( g.unit ) + ":" + std::to_string( i ) );
                m.properties["pin_id"] = { int64_t( g.pins[i].id.Value() ) };
            }
        }
        for( const auto& f : p.fields )
            field( f, p.source.sheet );
    }
    for( const auto& p : aModel.placements )
    {
        auto& r = add( CANONICAL_KIND::PLACEMENT, p.source.sheet, p.reference.text.ToStdString() );
        r.properties["unit"] = { int64_t( p.unit ) };
        r.properties["mirrored"] = { p.mirrored };
        r.geometry.points.push_back( point( p.position ) );
        r.geometry.angleTenths = p.angle;
        addSourceProperties( r, p.properties );
        for( const auto& f : p.fields )
            field( f, p.source.sheet );
    }
    for( const auto& n : aModel.nets )
    {
        auto& net = add( CANONICAL_KIND::NET, n.source.sheet, n.name.text.ToStdString() );
        addSourceProperties( net, n.properties );
        for( const auto& c : n.connections )
        {
            auto& r = add( CANONICAL_KIND::CONNECTION, c.source.sheet );
            r.properties["endpoint_count"] = { int64_t( c.endpoints.size() ) };
            for( size_t i = 0; i < c.endpoints.size(); ++i )
            {
                const auto prefix = "endpoint_" + std::to_string( i ) + "_";
                r.properties[prefix + "kind"] = { int64_t( c.endpoints[i].kind ) };

                if( c.endpoints[i].placement )
                    r.properties[prefix + "placement_id"] = { int64_t( c.endpoints[i].placement->id.Value() ) };

                if( c.endpoints[i].pin )
                    r.properties[prefix + "pin_id"] = { int64_t( c.endpoints[i].pin->id.Value() ) };
            }
            for( const auto& p : c.vertices )
                r.geometry.points.push_back( point( p ) );
            addSourceProperties( r, c.properties );
        }
    }
    for( const auto& b : aModel.buses )
    {
        auto& r = add( CANONICAL_KIND::BUS, b.source.sheet, b.name.text.ToStdString() );
        for( const auto& p : b.vertices )
            r.geometry.points.push_back( point( p ) );
        addSourceProperties( r, b.properties );
        for( size_t i = 0; i < b.entries.size(); ++i )
        {
            auto& e = add( CANONICAL_KIND::BUS_ENTRY, b.entries[i].source.sheet, std::to_string( i ) );
            e.properties["member_index"] = { int64_t( i ) };
            e.geometry.points.push_back( point( b.entries[i].position ) );
            addSourceProperties( e, b.entries[i].properties );
        }
        for( const auto& a : b.aliases )
            add( CANONICAL_KIND::BUS_ALIAS, a.source.sheet, a.text.ToStdString() );
        for( size_t i = 0; i < b.memberNets.size(); ++i )
            add( CANONICAL_KIND::BUS_MEMBER, b.memberNets[i].source.sheet, std::to_string( i ) );
    }
    for( const auto& l : aModel.labels )
    {
        auto& r = add( CANONICAL_KIND::LABEL, l.source.sheet, l.text.text.ToStdString() );
        r.properties["kind"] = { int64_t( l.kind ) };
        r.properties["visible"] = { l.presentation.visible };
        r.properties["font"] = { l.presentation.font.text.ToStdString() };
        r.properties["bold"] = { l.presentation.bold };
        r.properties["italic"] = { l.presentation.italic };
        r.properties["underline"] = { l.presentation.underline };
        r.properties["horizontal_justification"] = { int64_t( l.presentation.horizontalJustification ) };
        r.properties["vertical_justification"] = { int64_t( l.presentation.verticalJustification ) };
        r.geometry.points.push_back( point( l.position ) );
        r.geometry.angleTenths = l.angle;
        addSourceProperties( r, l.properties );
    }
    for( const auto& j : aModel.junctions )
    {
        auto& r = add( CANONICAL_KIND::JUNCTION, j.source.sheet );
        r.geometry.points.push_back( point( j.position ) );
        addSourceProperties( r, j.properties );
    }
    for( const auto& t : aModel.texts )
    {
        auto& r = add( CANONICAL_KIND::TEXT, t.source.sheet, t.text.text.ToStdString() );
        r.properties["visible"] = { t.presentation.visible };
        r.properties["font"] = { t.presentation.font.text.ToStdString() };
        r.properties["bold"] = { t.presentation.bold };
        r.properties["italic"] = { t.presentation.italic };
        r.properties["underline"] = { t.presentation.underline };
        r.properties["horizontal_justification"] = { int64_t( t.presentation.horizontalJustification ) };
        r.properties["vertical_justification"] = { int64_t( t.presentation.verticalJustification ) };
        r.geometry.points.push_back( point( t.position ) );
        r.geometry.angleTenths = t.angle;
        addSourceProperties( r, t.properties );
    }
    for( const auto& g : aModel.graphics )
        graphic( g, g.source.sheet );
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
    auto&       settings = add( CANONICAL_KIND::SETTINGS, -1 );
    settings.properties["coordinate_units_per_mil"] = { int64_t( 2 ) };
    settings.properties["line_width_half_mils"] = { int64_t( std::llround( p.line_width * 2 ) ) };
    settings.properties["bus_width_half_mils"] = { int64_t( p.bus_width * 2 ) };
    settings.geometry.points.push_back( point( PADS_SCH::POINT{ p.sheet_size.width, p.sheet_size.height } ) );
    auto addGraphic = [&]( const PADS_SCH::SYMBOL_GRAPHIC& aGraphic, int aSheet )
    {
        auto& r = add( CANONICAL_KIND::GRAPHIC, aSheet );
        r.properties["type"] = { int64_t( aGraphic.type ) };
        r.properties["line_style"] = { int64_t( aGraphic.line_style ) };
        r.properties["stroke_half_mils"] = { int64_t( std::llround( aGraphic.line_width * 2 ) ) };
        r.properties["fill"] = { aGraphic.filled };
        r.properties["text"] = { std::string() };
        r.properties["font"] = { std::string() };
        r.properties["visible"] = { true };

        for( const auto& q : aGraphic.points )
            r.geometry.points.push_back( point( q.coord ) );
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
        for( const auto& pin : d.pins )
        {
            auto& r = add( CANONICAL_KIND::PIN, -1, pin.number );
            r.properties["name"] = { pin.name };
            r.properties["type"] = { int64_t( pin.type ) };
            r.properties["style"] = { int64_t( ( pin.inverted ? 1 : 0 ) | ( pin.clock ? 2 : 0 ) ) };
            r.properties["length_half_mils"] = { int64_t( std::llround( pin.length * 2 ) ) };
            r.geometry.points.push_back( point( pin.position ) );
            r.geometry.angleTenths = int64_t( std::llround( pin.rotation * 10 ) );
        }
        for( const auto& t : d.texts )
        {
            auto& r = add( CANONICAL_KIND::FIELD, -1 );
            r.properties["value"] = { t.content };
            r.properties["visible"] = { t.visible };
            r.properties["font"] = { t.font_name };
            r.geometry.points.push_back( point( t.position ) );
            r.geometry.angleTenths = int64_t( std::llround( t.rotation * 10 ) );
        }
    }
    for( const auto& [name, part] : aParser.GetPartTypes() )
    {
        add( CANONICAL_KIND::PART_TYPE, -1, name );
        for( size_t i = 0; i < part.gates.size(); ++i )
        {
            add( CANONICAL_KIND::GATE, -1, std::to_string( i + 1 ) );
            for( size_t j = 0; j < part.gates[i].pins.size(); ++j )
            {
                auto& r = add( CANONICAL_KIND::GATE_PIN_MAPPING, -1,
                               std::to_string( i + 1 ) + ":" + std::to_string( j ) );
                r.properties["pin_number"] = { part.gates[i].pins[j].pin_id };
            }
        }
    }
    for( const auto& x : aParser.GetPartPlacements() )
    {
        auto& r = add( CANONICAL_KIND::PLACEMENT, x.sheet_number - 1, x.reference );
        r.properties["unit"] = { int64_t( x.gate_number ) };
        r.properties["mirrored"] = { x.mirror_flags != 0 };
        r.geometry.points.push_back( point( x.position ) );
        r.geometry.angleTenths = int64_t( std::llround( x.rotation * 10 ) );
        for( const auto& f : x.attributes )
        {
            auto& q = add( CANONICAL_KIND::FIELD, x.sheet_number - 1, f.name );
            q.properties["value"] = { f.value };
            q.properties["visible"] = { f.visible };
            q.properties["font"] = { f.font_name };
            q.geometry.points.push_back( point( f.position ) );
            q.geometry.angleTenths = int64_t( std::llround( f.rotation * 10 ) );
        }
    }
    for( const auto& n : aParser.GetSignals() )
    {
        add( CANONICAL_KIND::NET, -1, n.name );
        for( const auto& c : n.wires )
        {
            auto& r = add( CANONICAL_KIND::CONNECTION, c.sheet_number - 1 );
            r.properties["endpoint_count"] = { int64_t( 2 ) };
            for( const auto& q : c.vertices )
                r.geometry.points.push_back( point( q ) );
        }
    }
    for( const auto& b : aParser.GetBuses() )
    {
        auto& r = add( CANONICAL_KIND::BUS, b.sheet_number - 1, b.name );
        for( const auto& q : b.path )
            r.geometry.points.push_back( point( q ) );
        for( size_t i = 0; i < b.entries.size(); ++i )
        {
            auto& e = add( CANONICAL_KIND::BUS_ENTRY, b.sheet_number - 1, std::to_string( i ) );
            e.properties["member_index"] = { int64_t( i ) };
            e.geometry.points.push_back( point( b.entries[i].position ) );
        }
        for( const auto& a : b.aliases )
            add( CANONICAL_KIND::BUS_ALIAS, b.sheet_number - 1, a );
        for( size_t i = 0; i < b.member_nets.size(); ++i )
            add( CANONICAL_KIND::BUS_MEMBER, b.sheet_number - 1, std::to_string( i ) );
    }
    for( const auto& l : aParser.GetOffPageConnectors() )
    {
        if( l.symbol_lib.starts_with( "@@@B" ) )
            continue;
        auto& r = add( CANONICAL_KIND::LABEL, l.source_sheet - 1, l.signal_name );
        r.properties["kind"] = { int64_t( 0 ) };
        r.properties["visible"] = { true };
        r.geometry.points.push_back( point( l.position ) );
        r.geometry.angleTenths = int64_t( l.rotation * 10 );
    }
    for( const auto& j : aParser.GetTiedDots() )
        add( CANONICAL_KIND::JUNCTION, j.sheet_number - 1 ).geometry.points.push_back( point( j.position ) );
    for( const auto& t : aParser.GetTextItems() )
    {
        auto& r = add( CANONICAL_KIND::TEXT, t.sheet_number - 1, t.content );
        r.properties["visible"] = { true };
        r.geometry.points.push_back( point( t.position ) );
        r.geometry.angleTenths = int64_t( t.rotation * 10 );
    }
    for( const auto& lines : aParser.GetLinesItems() )
    {
        for( const auto& g : lines.primitives )
            addGraphic( g, lines.sheet_number - 1 );

        for( const auto& t : lines.texts )
        {
            auto& r = add( CANONICAL_KIND::TEXT, lines.sheet_number - 1, t.content );
            r.properties["visible"] = { true };
            r.properties["font"] = { t.font_name };
            r.properties["horizontal_justification"] = { int64_t( t.justification ) };
            r.geometry.points.push_back( point( t.position ) );
            r.geometry.angleTenths = int64_t( t.rotation * 10 );
        }
    }
    for( const auto& label : aParser.GetNetNameLabels() )
    {
        auto& r = add( CANONICAL_KIND::LABEL, -1, label.net_name );
        r.properties["kind"] = { int64_t( 0 ) };
        r.properties["visible"] = { true };
        r.properties["font"] = { label.font_name };
        r.properties["horizontal_justification"] = { int64_t( label.justification ) };
        r.geometry.points.push_back( point( PADS_SCH::POINT{ double( label.x_offset ), double( label.y_offset ) } ) );
        r.geometry.angleTenths = int64_t( label.rotation * 10 );
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


BOOST_AUTO_TEST_CASE( BinaryAndAsciiBusSnapshotsMatch )
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

    MODEL_BUS bus;
    bus.id = BUS_ID( 2 );
    bus.source = source;
    bus.name = string( wxS( "DATA[0..2]" ) );
    bus.vertices = { { 6000, 14000, source }, { 14000, 14000, source }, { 14000, 10000, source } };
    bus.aliases.push_back( string( wxS( "DATA[0..2]" ) ) );

    for( int i = 0; i < 3; ++i )
    {
        bus.entries.push_back( { source, { 8000 + i * 2000, 14000, source } } );
        bus.memberNets.push_back( { NET_ID( i + 1 ), source } );
    }

    binary.buses.push_back( bus );
    BOOST_CHECK( snapshotsMatch( normalizeBinaryModel( binary ), normalizeAsciiModel( asciiParser ) ) );
}


BOOST_AUTO_TEST_SUITE_END()
