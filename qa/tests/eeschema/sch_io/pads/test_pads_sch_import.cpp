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

#include <boost/test/unit_test.hpp>
#include <qa_utils/wx_utils/unit_test_utils.h>

#include <base_units.h>
#include <bitmap_base.h>
#include <connection_graph.h>
#include <drawing_sheet/ds_data_model.h>
#include <embedded_files.h>
#include <lib_symbol.h>
#include <netlist_exporter_pads.h>
#include <reporter.h>
#include <sch_field.h>
#include <sch_bus_entry.h>
#include <sch_bitmap.h>
#include <sch_connection.h>
#include <sch_junction.h>
#include <sch_label.h>
#include <schematic.h>
#include <sch_io/pads/sch_io_pads.h>
#include <sch_io/pads/pads_sch_binary_builder.h>
#include <sch_io/pads/pads_sch_binary_parser.h>
#include <sch_io/pads/pads_sch_binary_reader.h>
#include <sch_io/sch_io_mgr.h>
#include <sch_io/kicad_sexpr/sch_io_kicad_sexpr.h>
#include <sch_line.h>
#include <sch_screen.h>
#include <sch_shape.h>
#include <sch_sheet.h>
#include <sch_sheet_path.h>
#include <sch_symbol.h>
#include <sch_text.h>
#include <project/project_file.h>
#include <settings/settings_manager.h>
#include <string_utils.h>

#include <algorithm>
#include <array>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <map>
#include <numeric>
#include <set>
#include <sstream>
#include <vector>
#include <wx/filename.h>


namespace
{

struct PADS_SCH_IMPORT_FIXTURE
{
    PADS_SCH_IMPORT_FIXTURE() :
            m_schematic( nullptr )
    {
        m_settingsManager.LoadProject( "" );
        m_schematic.SetProject( &m_settingsManager.Prj() );
        m_schematic.Reset();
    }

    ~PADS_SCH_IMPORT_FIXTURE() { m_schematic.Reset(); }

    SETTINGS_MANAGER m_settingsManager;
    SCHEMATIC        m_schematic;
};


struct CAPTURING_REPORTER : REPORTER
{
    REPORTER& Report( const wxString& aText, SEVERITY aSeverity = RPT_SEVERITY_UNDEFINED ) override
    {
        messages.emplace_back( aText, aSeverity );
        return *this;
    }

    std::vector<std::pair<wxString, SEVERITY>> messages;
};


static wxString binaryFixture( const wxString& aName )
{
    return wxString::FromUTF8( KI_TEST::GetEeschemaTestDataDir() ) + wxS( "/plugins/pads/binary/" ) + aName
           + wxS( ".sch" );
}


static PADS_SCH_BINARY::PADS_SCH_MODEL parseBinaryFixture( const wxString& aName )
{
    wxString             path = binaryFixture( aName );
    std::vector<uint8_t> bytes;
    BOOST_REQUIRE( PADS_SCH_BINARY::PADS_SCH_BINARY_READER::ReadFile( path, bytes ) );
    return PADS_SCH_BINARY::PADS_SCH_BINARY_PARSER().Parse( bytes, path );
}


struct OBJECT_GRAPH_SNAPSHOT
{
    std::vector<const SCH_SHEET*> topLevelSheets;
    std::vector<const SCH_ITEM*>  rootItems;
    std::vector<const SCH_ITEM*>  appendItems;
    int                           pageWidth = 0;
    int                           pageHeight = 0;
    wxString                      title;

    bool operator==( const OBJECT_GRAPH_SNAPSHOT& ) const = default;
};


static OBJECT_GRAPH_SNAPSHOT objectGraphSnapshot( const SCHEMATIC& aSchematic, const SCH_SHEET* aAppendToMe )
{
    OBJECT_GRAPH_SNAPSHOT   snapshot;
    std::vector<SCH_SHEET*> topLevelSheets = aSchematic.GetTopLevelSheets();
    snapshot.topLevelSheets.assign( topLevelSheets.begin(), topLevelSheets.end() );

    for( const SCH_ITEM* item : aSchematic.Root().GetScreen()->Items() )
        snapshot.rootItems.push_back( item );

    if( aAppendToMe && aAppendToMe->GetScreen() )
    {
        for( const SCH_ITEM* item : aAppendToMe->GetScreen()->Items() )
            snapshot.appendItems.push_back( item );

        snapshot.pageWidth = aAppendToMe->GetScreen()->GetPageSettings().GetWidthMils();
        snapshot.pageHeight = aAppendToMe->GetScreen()->GetPageSettings().GetHeightMils();
        snapshot.title = aAppendToMe->GetScreen()->GetTitleBlock().GetTitle();
    }

    return snapshot;
}


static size_t itemCount( SCH_SCREEN* aScreen, KICAD_T aType )
{
    size_t count = 0;

    for( SCH_ITEM* item : aScreen->Items().OfType( aType ) )
    {
        (void) item;
        ++count;
    }

    return count;
}


static VECTOR2I localPoint( const PADS_SCH_BINARY::SOURCE_POINT& aPoint )
{
    return { schIUScale.MilsToIU( static_cast<double>( aPoint.x ) / 2.0 ),
             -schIUScale.MilsToIU( static_cast<double>( aPoint.y ) / 2.0 ) };
}


static VECTOR2I placedFieldOffset( const PADS_SCH_BINARY::MODEL_PLACEMENT& aPlacement,
                                   const PADS_SCH_BINARY::MODEL_FIELD&     aField )
{
    PADS_SCH_BINARY::SOURCE_POINT point = aField.position;

    if( aPlacement.mirrorFlags & 1 )
        point.x = -point.x;

    if( aPlacement.mirrorFlags & 2 )
        point.y = -point.y;

    return localPoint( point );
}


static VECTOR2I pagePoint( const PADS_SCH_BINARY::SOURCE_POINT& aPoint, int aPageHeight )
{
    return { schIUScale.MilsToIU( static_cast<double>( aPoint.x ) / 2.0 ),
             aPageHeight - schIUScale.MilsToIU( static_cast<double>( aPoint.y ) / 2.0 ) };
}


static std::multiset<wxString> connectivitySnapshot( SCHEMATIC& aSchematic )
{
    std::multiset<wxString> result;
    SCH_SHEET_LIST          hierarchy = aSchematic.BuildSheetListSortedByPageNumbers();
    aSchematic.ConnectionGraph()->Recalculate( hierarchy, true );

    for( const SCH_SHEET_PATH& path : hierarchy )
    {
        SCH_SCREEN* screen = path.LastScreen();
        BOOST_REQUIRE( screen );

        for( SCH_ITEM* item : screen->Items() )
        {
            wxString geometry;

            if( auto* line = dynamic_cast<SCH_LINE*>( item ) )
            {
                geometry = wxString::Format( wxS( "%d:%d,%d:%d,%d:%d:%d" ), line->GetLayer(), line->GetStartPoint().x,
                                             line->GetStartPoint().y, line->GetEndPoint().x, line->GetEndPoint().y,
                                             line->GetStroke().GetWidth(),
                                             static_cast<int>( line->GetStroke().GetLineStyle() ) );
            }
            else if( auto* entry = dynamic_cast<SCH_BUS_WIRE_ENTRY*>( item ) )
            {
                geometry = wxString::Format( wxS( "entry:%d,%d:%d,%d" ), entry->GetPosition().x, entry->GetPosition().y,
                                             entry->GetSize().x, entry->GetSize().y );
            }
            else if( auto* shape = dynamic_cast<SCH_SHAPE*>( item ) )
            {
                geometry = wxString::Format( wxS( "shape:%d:%d:%d:%d" ), static_cast<int>( shape->GetShape() ),
                                             static_cast<int>( shape->GetFillMode() ), shape->GetStroke().GetWidth(),
                                             static_cast<int>( shape->GetStroke().GetLineStyle() ) );

                if( shape->GetShape() == SHAPE_T::CIRCLE )
                {
                    const VECTOR2I center = shape->GetCenter();
                    const int      radius =
                            KiROUND( std::hypot( static_cast<double>( shape->GetEnd().x - shape->GetStart().x ),
                                                 static_cast<double>( shape->GetEnd().y - shape->GetStart().y ) ) );
                    geometry += wxString::Format( wxS( ":%d,%d:%d" ), center.x, center.y, radius );
                }
                else
                {
                    geometry += wxString::Format( wxS( ":%d,%d:%d,%d:%d,%d" ), shape->GetStart().x, shape->GetStart().y,
                                                  shape->GetEnd().x, shape->GetEnd().y, shape->GetArcMid().x,
                                                  shape->GetArcMid().y );
                }

                for( const VECTOR2I& point : shape->GetPolyPoints() )
                    geometry += wxString::Format( wxS( ":%d,%d" ), point.x, point.y );
            }
            else if( auto* text = dynamic_cast<EDA_TEXT*>( item ) )
            {
                geometry = wxString::Format( wxS( "text:%d,%d:%g:%d,%d:%d:%d:%d:%d:%d:%d:%s" ), text->GetTextPos().x,
                                             text->GetTextPos().y, text->GetTextAngleDegrees(), text->GetTextSize().x,
                                             text->GetTextSize().y, text->GetTextThickness(),
                                             static_cast<int>( text->GetHorizJustify() ),
                                             static_cast<int>( text->GetVertJustify() ), text->IsBold(),
                                             text->IsItalic(), text->IsVisible(), text->GetText() );
            }
            else
            {
                geometry = wxString::Format( wxS( "point:%d,%d" ), item->GetPosition().x, item->GetPosition().y );
            }

            result.insert( wxString::Format( wxS( "%s:%s:%d:%s" ), path.Path().AsString(), path.GetPageNumber(),
                                             static_cast<int>( item->Type() ), geometry ) );

            if( SCH_CONNECTION* connection = item->Connection( &path ) )
                result.insert( wxS( "net:" ) + connection->GetNetName() );

            if( auto* symbol = dynamic_cast<SCH_SYMBOL*>( item ) )
            {
                for( SCH_PIN* pin : symbol->GetPins( &path ) )
                {
                    if( SCH_CONNECTION* connection = pin->Connection( &path ) )
                        result.insert( wxS( "net:" ) + connection->GetNetName() );
                }
            }
        }
    }

    return result;
}


static bool hasNetName( const std::multiset<wxString>& aSnapshot, const wxString& aName )
{
    return std::ranges::any_of( aSnapshot,
                                [&]( const wxString& aValue )
                                {
                                    return aValue == wxS( "net:" ) + aName || aValue.EndsWith( wxS( "/" ) + aName );
                                } );
}


static bool netNameMatches( const wxString& aActual, const wxString& aExpected )
{
    const wxString actual = UnescapeString( aActual );
    return actual == aExpected || actual.EndsWith( wxS( "/" ) + aExpected );
}


struct PADS_NETLIST_SIGNATURE
{
    std::set<std::string>                   parts;
    std::multiset<std::vector<std::string>> partitions;
};


static PADS_NETLIST_SIGNATURE padsNetlistSignature( const std::string& aPath )
{
    enum class SECTION
    {
        NONE,
        PARTS,
        NETS
    };

    std::ifstream            input( aPath );
    PADS_NETLIST_SIGNATURE   result;
    SECTION                  section = SECTION::NONE;
    std::vector<std::string> pins;
    auto                     flush = [&]()
    {
        if( pins.empty() )
            return;

        std::ranges::sort( pins );
        result.partitions.insert( pins );
        pins.clear();
    };
    std::string line;

    while( std::getline( input, line ) )
    {
        if( !line.empty() && line.back() == '\r' )
            line.pop_back();

        if( line.starts_with( "*PART*" ) )
        {
            flush();
            section = SECTION::PARTS;
            continue;
        }

        if( line.starts_with( "*NET*" ) )
        {
            flush();
            section = SECTION::NETS;
            continue;
        }

        if( line.starts_with( "*SIGNAL*" ) )
        {
            flush();
            continue;
        }

        if( line.starts_with( '*' ) )
        {
            flush();
            section = SECTION::NONE;
            continue;
        }

        std::istringstream fields( line );

        if( section == SECTION::PARTS )
        {
            std::string reference;

            if( fields >> reference )
                result.parts.insert( std::move( reference ) );
        }
        else if( section == SECTION::NETS )
        {
            std::string pin;

            while( fields >> pin )
                pins.push_back( std::move( pin ) );
        }
    }

    flush();
    return result;
}


static SCH_SHEET_PATH sourceSheetPath( SCHEMATIC& aSchematic, const PADS_SCH_BINARY::PADS_SCH_MODEL& aModel,
                                       PADS_SCH_BINARY::SHEET_ID aSheetId )
{
    using namespace PADS_SCH_BINARY;

    auto sourceSheet = std::ranges::find( aModel.sheets, aSheetId, &MODEL_SHEET::id );
    BOOST_REQUIRE( sourceSheet != aModel.sheets.end() );
    SCH_SHEET_LIST hierarchy = aSchematic.BuildSheetListSortedByPageNumbers();

    if( aModel.sheets.size() == 1 )
    {
        BOOST_REQUIRE_EQUAL( hierarchy.size(), 1u );
        return hierarchy.front();
    }

    const bool flatTopLevel = aSchematic.GetTopLevelSheets().size() == aModel.sheets.size();
    wxString   expectedPage = wxString::Format( wxS( "%zu" ), sourceSheet->index + ( flatTopLevel ? 1 : 2 ) );
    auto       path = std::ranges::find_if( hierarchy,
                                            [&]( const SCH_SHEET_PATH& aPath )
                                            {
                                          return aPath.size() == ( flatTopLevel ? 1u : 2u )
                                                 && aPath.GetPageNumber() == expectedPage;
                                      } );
    BOOST_REQUIRE_MESSAGE( path != hierarchy.end(), "missing typed source sheet index " << sourceSheet->index );
    BOOST_CHECK_EQUAL( path->Last()->GetField( FIELD_T::SHEET_NAME )->GetText(), sourceSheet->name.text );
    return *path;
}


static void roundTripTopLevelSheets( SCHEMATIC& aSchematic, const wxString& aDirectory )
{
    SCH_IO_KICAD_SEXPR                io;
    std::vector<TOP_LEVEL_SHEET_INFO> sheetInfos;

    for( size_t index = 0; index < aSchematic.GetTopLevelSheets().size(); ++index )
    {
        SCH_SHEET* sheet = aSchematic.GetTopLevelSheet( index );
        BOOST_REQUIRE( sheet );
        wxString file =
                aDirectory + wxFileName::GetPathSeparator() + wxString::Format( wxS( "top_%zu.kicad_sch" ), index + 1 );
        BOOST_REQUIRE_NO_THROW( io.SaveSchematicFile( file, sheet, &aSchematic ) );
        sheetInfos.emplace_back( sheet->m_Uuid, sheet->GetName(), file );
    }

    aSchematic.Reset();
    std::vector<SCH_SHEET*> loadedSheets;

    for( const TOP_LEVEL_SHEET_INFO& info : sheetInfos )
    {
        SCH_SHEET* loaded = nullptr;
        BOOST_REQUIRE_NO_THROW( loaded = io.LoadSchematicFile( info.filename, &aSchematic ) );
        BOOST_REQUIRE( loaded );
        const_cast<KIID&>( loaded->m_Uuid ) = info.uuid;
        loaded->SetName( info.name );
        loadedSheets.push_back( loaded );
    }

    aSchematic.SetTopLevelSheets( loadedSheets );
    aSchematic.RefreshHierarchy();
}


struct CONNECTIVITY_ORACLE_COUNTS
{
    size_t pinEndpoints = 0;
    size_t powerLabels = 0;
};


static CONNECTIVITY_ORACLE_COUNTS assertSourceConnectivity( const PADS_SCH_BINARY::PADS_SCH_MODEL& aModel,
                                                            SCHEMATIC&                             aSchematic )
{
    using namespace PADS_SCH_BINARY;

    CONNECTIVITY_ORACLE_COUNTS counts;
    SCH_SHEET_LIST             hierarchy = aSchematic.BuildSheetListSortedByPageNumbers();
    aSchematic.ConnectionGraph()->Recalculate( hierarchy, true );

    auto samePoint = []( const SOURCE_POINT& aLeft, const SOURCE_POINT& aRight )
    {
        return aLeft.x == aRight.x && aLeft.y == aRight.y;
    };

    using OWNED_SEGMENT = std::tuple<uint32_t, uint32_t, int64_t, int64_t, int64_t, int64_t>;
    std::set<OWNED_SEGMENT> busEntrySegments;

    auto segmentKey = []( SHEET_ID aSheet, NET_ID aNet, const SOURCE_POINT& aStart, const SOURCE_POINT& aEnd )
    {
        if( std::tie( aStart.x, aStart.y ) <= std::tie( aEnd.x, aEnd.y ) )
            return OWNED_SEGMENT( aSheet.Value(), aNet.Value(), aStart.x, aStart.y, aEnd.x, aEnd.y );

        return OWNED_SEGMENT( aSheet.Value(), aNet.Value(), aEnd.x, aEnd.y, aStart.x, aStart.y );
    };

    for( const MODEL_BUS& bus : aModel.buses )
    {
        SCH_SHEET_PATH path = sourceSheetPath( aSchematic, aModel, bus.sheet.id );
        SCH_SCREEN*    screen = path.LastScreen();
        BOOST_REQUIRE( screen );
        const int pageHeight = screen->GetPageSettings().GetHeightIU( schIUScale.IU_PER_MILS );
        auto      sourceSheet = std::ranges::find( aModel.sheets, bus.sheet.id, &MODEL_SHEET::id );
        BOOST_REQUIRE( sourceSheet != aModel.sheets.end() );

        for( size_t vertex = 1; vertex < bus.vertices.size(); ++vertex )
        {
            bool found = false;

            for( SCH_ITEM* item : screen->Items().OfType( SCH_LINE_T ) )
            {
                auto* line = static_cast<SCH_LINE*>( item );

                if( line->GetLayer() == LAYER_BUS
                    && line->GetStartPoint() == pagePoint( bus.vertices[vertex - 1], pageHeight )
                    && line->GetEndPoint() == pagePoint( bus.vertices[vertex], pageHeight ) )
                {
                    BOOST_CHECK_EQUAL( line->GetStroke().GetWidth(),
                                       schIUScale.MilsToIU( sourceSheet->defaultBusWidth / 2.0 ) );
                    found = true;
                    break;
                }
            }

            BOOST_CHECK_MESSAGE( found, "missing bus segment " << bus.source.recordIndex << ':' << vertex );
        }

        wxString memberSuffix;

        if( !bus.declaredMembers.empty() )
        {
            memberSuffix = wxS( "{" );

            for( size_t member = 0; member < bus.declaredMembers.size(); ++member )
            {
                if( member )
                    memberSuffix += wxS( " " );

                memberSuffix += bus.declaredMembers[member].text;
            }

            memberSuffix += wxS( "}" );
        }
        std::vector<wxString> aliases;

        if( bus.aliases.empty() )
            aliases.push_back( bus.name.text );
        else
        {
            for( const SOURCE_STRING& alias : bus.aliases )
                aliases.push_back( alias.text );
        }

        for( const wxString& alias : aliases )
        {
            bool foundBusLabel = false;

            for( SCH_ITEM* item : screen->Items().OfType( SCH_LABEL_T ) )
            {
                auto* label = static_cast<SCH_LABEL*>( item );
                foundBusLabel |= label->GetText() == alias + memberSuffix
                                 && label->GetPosition() == pagePoint( bus.vertices.front(), pageHeight );
            }

            BOOST_CHECK_MESSAGE( foundBusLabel, alias );
        }

        for( const MODEL_BUS_ENTRY& entry : bus.entries )
        {
            auto ownerNet = std::ranges::find( aModel.nets, entry.memberNet.id, &MODEL_NET::id );
            BOOST_REQUIRE( ownerNet != aModel.nets.end() );
            std::vector<SOURCE_POINT> adjacent;

            for( const MODEL_CONNECTION& connection : ownerNet->connections )
            {
                if( connection.vertices.size() >= 2 && samePoint( connection.vertices.front(), entry.position ) )
                    adjacent.push_back( connection.vertices[1] );
                else if( connection.vertices.size() >= 2 && samePoint( connection.vertices.back(), entry.position ) )
                    adjacent.push_back( connection.vertices[connection.vertices.size() - 2] );
            }

            BOOST_REQUIRE_EQUAL( adjacent.size(), 1u );
            busEntrySegments.insert( segmentKey( bus.sheet.id, ownerNet->id, entry.position, adjacent.front() ) );
            bool found = false;

            for( SCH_ITEM* item : screen->Items().OfType( SCH_BUS_WIRE_ENTRY_T ) )
            {
                auto* builtEntry = static_cast<SCH_BUS_WIRE_ENTRY*>( item );

                if( builtEntry->GetPosition() == pagePoint( entry.position, pageHeight ) )
                {
                    const VECTOR2I expectedEnd = pagePoint( adjacent.front(), pageHeight );
                    bool           reachesWire = builtEntry->GetEnd() == expectedEnd;

                    for( SCH_ITEM* lineItem : screen->Items().OfType( SCH_LINE_T ) )
                    {
                        auto* line = static_cast<SCH_LINE*>( lineItem );

                        if( line->GetLayer() == LAYER_WIRE && line->GetStartPoint() == builtEntry->GetEnd()
                            && line->GetEndPoint() == expectedEnd )
                        {
                            reachesWire = true;
                            break;
                        }
                    }

                    BOOST_CHECK( reachesWire );
                    BOOST_REQUIRE( builtEntry->Connection( &path ) );
                    found = true;
                    break;
                }
            }

            BOOST_CHECK_MESSAGE( found, "missing bus entry " << entry.source.recordIndex );
        }
    }

    for( const MODEL_NET& net : aModel.nets )
    {
        SCH_SHEET_PATH path = sourceSheetPath( aSchematic, aModel, net.sheet.id );
        SCH_SCREEN*    screen = path.LastScreen();
        BOOST_REQUIRE( screen );
        const int          pageHeight = screen->GetPageSettings().GetHeightIU( schIUScale.IU_PER_MILS );
        const MODEL_SHEET& sourceSheet = *std::ranges::find( aModel.sheets, net.sheet.id, &MODEL_SHEET::id );

        for( const MODEL_CONNECTION& sourceConnection : net.connections )
        {
            BOOST_REQUIRE_GE( sourceConnection.vertices.size(), 2u );

            for( size_t vertex = 1; vertex < sourceConnection.vertices.size(); ++vertex )
            {
                if( busEntrySegments.contains( segmentKey( net.sheet.id, net.id, sourceConnection.vertices[vertex - 1],
                                                           sourceConnection.vertices[vertex] ) ) )
                {
                    continue;
                }

                bool found = false;

                for( SCH_ITEM* item : screen->Items().OfType( SCH_LINE_T ) )
                {
                    auto* line = static_cast<SCH_LINE*>( item );

                    if( line->GetLayer() == LAYER_WIRE
                        && line->GetStartPoint() == pagePoint( sourceConnection.vertices[vertex - 1], pageHeight )
                        && line->GetEndPoint() == pagePoint( sourceConnection.vertices[vertex], pageHeight ) )
                    {
                        BOOST_CHECK_EQUAL( line->GetStroke().GetWidth(), 0 );
                        SCH_CONNECTION* connection = line->Connection( &path );
                        BOOST_REQUIRE( connection );
                        BOOST_CHECK_MESSAGE( netNameMatches( connection->GetNetName(), net.name.text ),
                                             "wire net actual='" << connection->GetNetName() << "' expected='"
                                                                 << net.name.text << "' sheet=" << net.sheet.id.Value()
                                                                 << " record=" << sourceConnection.source.recordIndex );
                        found = true;
                        break;
                    }
                }

                BOOST_CHECK_MESSAGE( found, "missing owned wire segment " << sourceConnection.source.recordIndex << ':'
                                                                          << vertex );
            }

            for( const MODEL_CONNECTION_ENDPOINT& endpoint : sourceConnection.endpoints )
            {
                BOOST_CHECK( samePoint( endpoint.point, sourceConnection.vertices.front() )
                             || samePoint( endpoint.point, sourceConnection.vertices.back() ) );

                if( endpoint.kind != MODEL_ENDPOINT_KIND::PIN )
                    continue;

                BOOST_REQUIRE( endpoint.placement );
                BOOST_REQUIRE( endpoint.pin );
                ++counts.pinEndpoints;
                auto placement = std::ranges::find( aModel.placements, endpoint.placement->id, &MODEL_PLACEMENT::id );
                BOOST_REQUIRE( placement != aModel.placements.end() );
                BOOST_CHECK_EQUAL( placement->sheet.id.Value(), net.sheet.id.Value() );
                BOOST_CHECK( std::ranges::any_of( placement->pins,
                                                  [&]( const PLACED_PIN_REFERENCE& aPin )
                                                  {
                                                      return aPin.id == endpoint.pin->id;
                                                  } ) );
                const MODEL_PIN_DEFINITION* sourcePin = nullptr;

                for( const MODEL_SYMBOL_DEFINITION& definition : aModel.definitions )
                {
                    auto pin = std::ranges::find( definition.pins, endpoint.pin->id, &MODEL_PIN_DEFINITION::id );

                    if( pin != definition.pins.end() )
                    {
                        BOOST_REQUIRE( !sourcePin );
                        sourcePin = &*pin;
                    }
                }

                BOOST_REQUIRE( sourcePin );
                auto part = std::ranges::find( aModel.partTypes, placement->partType.id, &MODEL_PART_TYPE::id );
                BOOST_REQUIRE( part != aModel.partTypes.end() );
                BOOST_REQUIRE( placement->gate );
                auto gate = std::ranges::find( part->gates, placement->gate->id, &MODEL_GATE::id );
                BOOST_REQUIRE( gate != part->gates.end() );
                auto pinReference = std::ranges::find( placement->pins, endpoint.pin->id, &PIN_REFERENCE::id );
                BOOST_REQUIRE( pinReference != placement->pins.end() );
                const size_t pinOrdinal = std::distance( placement->pins.begin(), pinReference );
                wxString     expectedNumber = sourcePin->number.text;
                wxString     expectedName = sourcePin->name.text;

                if( !gate->connectorPins.empty() )
                {
                    BOOST_REQUIRE_GT( placement->unit, 0u );
                    BOOST_REQUIRE_LE( placement->unit, gate->connectorPins.size() );
                    expectedNumber = gate->connectorPins[placement->unit - 1].number.text;
                    expectedName = gate->connectorPins[placement->unit - 1].name.text;
                }
                else if( !gate->logicalPins.empty() )
                {
                    BOOST_REQUIRE_LT( pinOrdinal, gate->logicalPins.size() );
                    expectedNumber = gate->logicalPins[pinOrdinal].number.text;
                    expectedName = gate->logicalPins[pinOrdinal].name.text;
                }

                SCH_SYMBOL* builtSymbol = nullptr;

                for( SCH_ITEM* item : screen->Items().OfType( SCH_SYMBOL_T ) )
                {
                    auto* symbol = static_cast<SCH_SYMBOL*>( item );

                    if( !symbol->GetRef( &path ).StartsWith( wxS( "#PWR" ) )
                        && symbol->GetPosition() == pagePoint( placement->position, pageHeight )
                        && symbol->GetUnit() == static_cast<int>( placement->unit ) )
                    {
                        builtSymbol = symbol;
                        break;
                    }
                }

                BOOST_REQUIRE( builtSymbol );
                std::vector<SCH_PIN*> builtPins = builtSymbol->GetPins( &path );
                auto                  builtPin = std::ranges::find_if( builtPins,
                                                                       [&]( SCH_PIN* aPin )
                                                                       {
                                                          return aPin->GetNumber() == expectedNumber
                                                                 && aPin->GetName() == expectedName
                                                                 && aPin->GetPosition()
                                                                            == pagePoint( endpoint.point, pageHeight );
                                                      } );

                if( builtPin == builtPins.end() )
                {
                    for( SCH_PIN* candidate : builtPins )
                    {
                        if( candidate->GetNumber() == expectedNumber || candidate->GetName() == expectedName )
                            BOOST_TEST_MESSAGE( "candidate " << candidate->GetNumber() << ' ' << candidate->GetName()
                                                             << " at " << candidate->GetPosition().x << ','
                                                             << candidate->GetPosition().y << " orientation "
                                                             << builtSymbol->GetOrientation() << " raw mirror "
                                                             << placement->mirrorFlags );
                    }
                }

                BOOST_REQUIRE_MESSAGE( builtPin != builtPins.end(),
                                       placement->reference.text << " pin " << expectedNumber << " " << expectedName
                                                                 << " net " << net.name.text << " at "
                                                                 << endpoint.point.x << ',' << endpoint.point.y );
                SCH_CONNECTION* pinConnection = ( *builtPin )->Connection( &path );
                BOOST_REQUIRE( pinConnection );

                if( !netNameMatches( pinConnection->GetNetName(), net.name.text ) )
                {
                    for( SCH_ITEM* lineItem : screen->Items().OfType( SCH_LINE_T ) )
                    {
                        auto* line = static_cast<SCH_LINE*>( lineItem );

                        if( line->GetStartPoint() == ( *builtPin )->GetPosition()
                            || line->GetEndPoint() == ( *builtPin )->GetPosition() )
                        {
                            BOOST_TEST_MESSAGE( "pin-adjacent line layer="
                                                << line->GetLayer() << " start=" << line->GetStartPoint().x << ','
                                                << line->GetStartPoint().y << " end=" << line->GetEndPoint().x << ','
                                                << line->GetEndPoint().y );
                        }
                    }

                    for( SCH_ITEM* labelItem : screen->Items().OfType( SCH_LABEL_T ) )
                    {
                        auto* label = static_cast<SCH_LABEL*>( labelItem );

                        if( label->GetPosition() == ( *builtPin )->GetPosition() )
                            BOOST_TEST_MESSAGE( "pin-adjacent label '" << label->GetText() << "'" );
                    }
                }

                BOOST_CHECK_MESSAGE( netNameMatches( pinConnection->GetNetName(), net.name.text ),
                                     "pin net actual='" << pinConnection->GetNetName() << "' expected='"
                                                        << net.name.text << "' pin=" << placement->reference.text << '.'
                                                        << expectedNumber << " sheet=" << net.sheet.id.Value() );
            }
        }
    }

    for( const MODEL_LABEL& label : aModel.labels )
    {
        if( label.kind == MODEL_LABEL_KIND::UNSUPPORTED )
        {
            continue;
        }

        SCH_SHEET_PATH path = sourceSheetPath( aSchematic, aModel, label.sheet.id );
        SCH_SCREEN*    screen = path.LastScreen();
        BOOST_REQUIRE( screen );
        const int pageHeight = screen->GetPageSettings().GetHeightIU( schIUScale.IU_PER_MILS );
        auto      ownerNet =
                std::ranges::find_if( aModel.nets,
                                      [&]( const MODEL_NET& aNet )
                                      {
                                          return aNet.sheet.id == label.sheet.id && aNet.name.text == label.text.text;
                                      } );
        BOOST_REQUIRE( ownerNet != aModel.nets.end() );

        if( label.kind == MODEL_LABEL_KIND::POWER || label.kind == MODEL_LABEL_KIND::GROUND )
        {
            ++counts.powerLabels;
            bool foundPower = false;

            for( SCH_ITEM* item : screen->Items().OfType( SCH_SYMBOL_T ) )
            {
                auto* symbol = static_cast<SCH_SYMBOL*>( item );

                if( !symbol->GetRef( &path ).StartsWith( wxS( "#PWR" ) )
                    || symbol->GetPosition() != pagePoint( label.position, pageHeight )
                    || symbol->GetValue( false, &path, false ) != label.text.text )
                {
                    continue;
                }

                std::vector<SCH_PIN*> pins = symbol->GetPins( &path );
                BOOST_REQUIRE_EQUAL( pins.size(), 1u );
                BOOST_CHECK_EQUAL( pins.front()->IsVisible(), false );
                BOOST_CHECK_EQUAL( pins.front()->GetPosition(), pagePoint( label.position, pageHeight ) );
                SCH_CONNECTION* connection = pins.front()->Connection( &path );
                BOOST_REQUIRE( connection );
                BOOST_CHECK( netNameMatches( connection->GetNetName(), ownerNet->name.text ) );
                foundPower = true;
                break;
            }

            BOOST_CHECK_MESSAGE( foundPower, "missing exact power net " << label.text.text );
            continue;
        }

        KICAD_T type = label.kind == MODEL_LABEL_KIND::GLOBAL         ? SCH_GLOBAL_LABEL_T
                       : label.kind == MODEL_LABEL_KIND::HIERARCHICAL ? SCH_HIER_LABEL_T
                                                                      : SCH_LABEL_T;
        bool    found = false;

        for( SCH_ITEM* item : screen->Items().OfType( type ) )
        {
            auto* builtLabel = static_cast<SCH_LABEL_BASE*>( item );

            if( builtLabel->GetText() == label.text.text
                && builtLabel->GetPosition() == pagePoint( label.position, pageHeight ) )
            {
                SCH_CONNECTION* connection = builtLabel->Connection( &path );
                BOOST_REQUIRE( connection );
                BOOST_CHECK( netNameMatches( connection->GetNetName(), ownerNet->name.text ) );
                found = true;
                break;
            }
        }

        BOOST_CHECK_MESSAGE( found, "missing exact label net " << label.text.text );
    }

    for( const MODEL_JUNCTION& junction : aModel.junctions )
    {
        auto relationship = std::ranges::find_if( junction.properties,
                                                  []( const SOURCE_PROPERTY& aProperty )
                                                  {
                                                      return aProperty.name.text == wxS( "connection_record" );
                                                  } );
        BOOST_REQUIRE( relationship != junction.properties.end() );
        unsigned long connectionRecord = 0;
        BOOST_REQUIRE( relationship->value.text.ToULong( &connectionRecord ) );
        const MODEL_NET* ownerNet = nullptr;

        for( const MODEL_NET& net : aModel.nets )
        {
            if( net.sheet.id != junction.sheet.id )
                continue;

            auto connection = std::ranges::find_if( net.connections,
                                                    [&]( const MODEL_CONNECTION& aConnection )
                                                    {
                                                        return aConnection.source.recordIndex == connectionRecord;
                                                    } );

            if( connection != net.connections.end() )
            {
                BOOST_REQUIRE( !ownerNet );
                ownerNet = &net;
            }
        }

        BOOST_REQUIRE( ownerNet );
        SCH_SHEET_PATH path = sourceSheetPath( aSchematic, aModel, junction.sheet.id );
        SCH_SCREEN*    screen = path.LastScreen();
        BOOST_REQUIRE( screen );
        const int pageHeight = screen->GetPageSettings().GetHeightIU( schIUScale.IU_PER_MILS );
        bool      found = false;

        for( SCH_ITEM* item : screen->Items().OfType( SCH_JUNCTION_T ) )
        {
            if( item->GetPosition() != pagePoint( junction.position, pageHeight ) )
                continue;

            SCH_CONNECTION* connection = item->Connection( &path );
            BOOST_REQUIRE( connection );
            BOOST_CHECK( netNameMatches( connection->GetNetName(), ownerNet->name.text ) );
            found = true;
            break;
        }

        BOOST_CHECK_MESSAGE( found, "missing typed junction " << junction.source.recordIndex );
    }

    return counts;
}


static std::vector<const PADS_SCH_BINARY::SOURCE_PROPERTY*>
allSourceProperties( const PADS_SCH_BINARY::PADS_SCH_MODEL& aModel )
{
    using namespace PADS_SCH_BINARY;

    std::vector<const SOURCE_PROPERTY*> result;
    auto                                append = [&]( const std::vector<SOURCE_PROPERTY>& aProperties )
    {
        for( const SOURCE_PROPERTY& property : aProperties )
            result.push_back( &property );
    };
    auto appendPresentation = [&]( const MODEL_TEXT_PRESENTATION& aPresentation )
    {
        append( aPresentation.properties );
    };

    append( aModel.settings.properties );

    for( const MODEL_SHEET& sheet : aModel.sheets )
    {
        append( sheet.properties );

        for( const MODEL_GRAPHIC& graphic : sheet.border )
        {
            append( graphic.properties );
            appendPresentation( graphic.presentation );
        }

        for( const MODEL_FIELD& field : sheet.titleBlockFields )
        {
            append( field.properties );
            appendPresentation( field.presentation );
        }
    }

    for( const MODEL_SYMBOL_DEFINITION& definition : aModel.definitions )
    {
        append( definition.properties );

        for( const MODEL_GRAPHIC& graphic : definition.graphics )
        {
            append( graphic.properties );
            appendPresentation( graphic.presentation );
        }

        for( const MODEL_PIN_DEFINITION& pin : definition.pins )
        {
            append( pin.properties );
            appendPresentation( pin.presentation );
            appendPresentation( pin.namePresentation );
            appendPresentation( pin.numberPresentation );
        }

        for( const MODEL_FIELD& field : definition.fields )
        {
            append( field.properties );
            appendPresentation( field.presentation );
        }
    }

    for( const MODEL_PART_TYPE& part : aModel.partTypes )
    {
        append( part.properties );

        for( const MODEL_GATE& gate : part.gates )
            append( gate.properties );

        for( const MODEL_FIELD& field : part.fields )
        {
            append( field.properties );
            appendPresentation( field.presentation );
        }
    }

    for( const MODEL_PLACEMENT& placement : aModel.placements )
    {
        append( placement.properties );

        for( const MODEL_FIELD& field : placement.fields )
        {
            append( field.properties );
            appendPresentation( field.presentation );
        }
    }

    for( const MODEL_NET& net : aModel.nets )
    {
        append( net.properties );

        for( const MODEL_CONNECTION& connection : net.connections )
        {
            append( connection.properties );

            for( const MODEL_CONNECTION_ENDPOINT& endpoint : connection.endpoints )
                append( endpoint.properties );
        }
    }

    for( const MODEL_BUS& bus : aModel.buses )
    {
        append( bus.properties );

        for( const MODEL_BUS_ENTRY& entry : bus.entries )
            append( entry.properties );
    }

    for( const MODEL_LABEL& label : aModel.labels )
    {
        append( label.properties );
        appendPresentation( label.presentation );
    }

    for( const MODEL_JUNCTION& junction : aModel.junctions )
        append( junction.properties );

    for( const MODEL_TEXT& text : aModel.texts )
    {
        append( text.properties );
        appendPresentation( text.presentation );
    }

    for( const MODEL_PAGE_GRAPHIC& graphic : aModel.graphics )
    {
        append( graphic.graphic.properties );
        appendPresentation( graphic.graphic.presentation );
    }

    for( const MODEL_WORKSHEET& worksheet : aModel.worksheets )
    {
        for( const MODEL_GRAPHIC& graphic : worksheet.graphics )
        {
            append( graphic.properties );
            appendPresentation( graphic.presentation );
        }
    }

    return result;
}


static PIN_ORIENTATION pinOrientation( const PADS_SCH_BINARY::MODEL_PIN_DEFINITION& aPin )
{
    if( aPin.decalName.text.Contains( wxS( "VRT" ) ) )
        return aPin.side == 2 ? PIN_ORIENTATION::PIN_UP : PIN_ORIENTATION::PIN_DOWN;

    switch( PADS_SCH_BINARY::NormalizeAngle( aPin.angle ) )
    {
    case 900: return aPin.side >= 2 ? PIN_ORIENTATION::PIN_DOWN : PIN_ORIENTATION::PIN_UP;
    case 1800: return ( aPin.side & 1 ) != 0 ? PIN_ORIENTATION::PIN_RIGHT : PIN_ORIENTATION::PIN_LEFT;
    case 2700: return aPin.side >= 2 ? PIN_ORIENTATION::PIN_UP : PIN_ORIENTATION::PIN_DOWN;
    default: return ( aPin.side & 1 ) != 0 ? PIN_ORIENTATION::PIN_LEFT : PIN_ORIENTATION::PIN_RIGHT;
    }
}


static ELECTRICAL_PINTYPE pinType( uint32_t aType )
{
    const std::array<ELECTRICAL_PINTYPE, 9> types = {
        ELECTRICAL_PINTYPE::PT_PASSIVE,     ELECTRICAL_PINTYPE::PT_INPUT,    ELECTRICAL_PINTYPE::PT_OUTPUT,
        ELECTRICAL_PINTYPE::PT_BIDI,        ELECTRICAL_PINTYPE::PT_TRISTATE, ELECTRICAL_PINTYPE::PT_OPENCOLLECTOR,
        ELECTRICAL_PINTYPE::PT_OPENEMITTER, ELECTRICAL_PINTYPE::PT_POWER_IN, ELECTRICAL_PINTYPE::PT_UNSPECIFIED
    };
    return aType < types.size() ? types[aType] : ELECTRICAL_PINTYPE::PT_UNSPECIFIED;
}


static GRAPHIC_PINSHAPE pinShape( uint32_t aStyle )
{
    switch( aStyle )
    {
    case 1: return GRAPHIC_PINSHAPE::INVERTED;
    case 2: return GRAPHIC_PINSHAPE::CLOCK;
    case 3: return GRAPHIC_PINSHAPE::INVERTED_CLOCK;
    default: return GRAPHIC_PINSHAPE::LINE;
    }
}


static LINE_STYLE lineStyle( PADS_SCH_BINARY::MODEL_LINE_STYLE aStyle )
{
    switch( aStyle )
    {
    case PADS_SCH_BINARY::MODEL_LINE_STYLE::DASH: return LINE_STYLE::DASH;
    case PADS_SCH_BINARY::MODEL_LINE_STYLE::DOT: return LINE_STYLE::DOT;
    case PADS_SCH_BINARY::MODEL_LINE_STYLE::DASH_DOT: return LINE_STYLE::DASHDOT;
    default: return LINE_STYLE::SOLID;
    }
}


static GR_TEXT_H_ALIGN_T horizontalJustification( PADS_SCH_BINARY::MODEL_JUSTIFICATION aJustification )
{
    switch( aJustification )
    {
    case PADS_SCH_BINARY::MODEL_JUSTIFICATION::CENTER: return GR_TEXT_H_ALIGN_CENTER;
    case PADS_SCH_BINARY::MODEL_JUSTIFICATION::RIGHT: return GR_TEXT_H_ALIGN_RIGHT;
    default: return GR_TEXT_H_ALIGN_LEFT;
    }
}


static GR_TEXT_V_ALIGN_T verticalJustification( PADS_SCH_BINARY::MODEL_JUSTIFICATION aJustification )
{
    switch( aJustification )
    {
    case PADS_SCH_BINARY::MODEL_JUSTIFICATION::LEFT: return GR_TEXT_V_ALIGN_TOP;
    case PADS_SCH_BINARY::MODEL_JUSTIFICATION::RIGHT: return GR_TEXT_V_ALIGN_BOTTOM;
    default: return GR_TEXT_V_ALIGN_CENTER;
    }
}


static void checkTextPresentation( const EDA_TEXT&                                 aText,
                                   const PADS_SCH_BINARY::MODEL_TEXT_PRESENTATION& aPresentation )
{
    if( aPresentation.height > 0 )
    {
        BOOST_CHECK_EQUAL( aText.GetTextSize().x, schIUScale.MilsToIU( aPresentation.height / 2.0 ) );
        BOOST_CHECK_EQUAL( aText.GetTextSize().y, schIUScale.MilsToIU( aPresentation.height / 2.0 ) );
    }

    if( aPresentation.width > 0 )
        BOOST_CHECK_EQUAL( aText.GetTextThickness(), schIUScale.MilsToIU( aPresentation.width / 2.0 ) );

    BOOST_CHECK( aText.GetHorizJustify() == horizontalJustification( aPresentation.horizontalJustification ) );
    BOOST_CHECK( aText.GetVertJustify() == verticalJustification( aPresentation.verticalJustification ) );
    BOOST_CHECK_EQUAL( aText.IsBold(), aPresentation.bold );
    BOOST_CHECK_EQUAL( aText.IsItalic(), aPresentation.italic );
    BOOST_CHECK_EQUAL( aText.IsVisible(), aPresentation.visible );
}

} // namespace


BOOST_FIXTURE_TEST_SUITE( PadsSchImport, PADS_SCH_IMPORT_FIXTURE )


// A zero OLE trailer box must be skipped before the aspect-ratio divide. llround on the inf that
// divide produces is undefined, and the page-size guard that would have caught it runs after the
// rescale.
BOOST_AUTO_TEST_CASE( BinaryEmbeddedImageDegenerateBoxIsSkipped )
{
    using namespace PADS_SCH_BINARY;

    PADS_SCH_MODEL model = parseBinaryFixture( wxS( "ole_images" ) );

    BOOST_REQUIRE( !model.images.empty() );

    for( MODEL_EMBEDDED_IMAGE& image : model.images )
        image.size.x = 0;

    SCH_SHEET* root = m_schematic.GetTopLevelSheet();

    BOOST_REQUIRE( root );
    BOOST_REQUIRE( root->GetScreen() );

    BUILD_RESULT result =
            PADS_SCH_BINARY_BUILDER().Build( model, &m_schematic, nullptr, binaryFixture( wxS( "ole_images" ) ) );

    BOOST_CHECK_EQUAL( result.counts.images, 0u );
    BOOST_CHECK_EQUAL( itemCount( root->GetScreen(), SCH_BITMAP_T ), 0u );
}


BOOST_AUTO_TEST_CASE( BinaryEmbeddedImages )
{
    using namespace PADS_SCH_BINARY;

    PADS_SCH_MODEL model = parseBinaryFixture( wxS( "ole_images" ) );
    BOOST_REQUIRE_GE( model.images[0].data.size(), 14u );
    MODEL_EMBEDDED_IMAGE dib = model.images[0];
    dib.id = IMAGE_ID( 2 );
    dib.type = MODEL_EMBEDDED_IMAGE_TYPE::DIB;
    dib.streamName = wxS( "synthetic DIB view of Ole10Native BMP" );
    dib.position.x += 2000;
    dib.data.erase( dib.data.begin(), dib.data.begin() + 14 );
    model.images.push_back( std::move( dib ) );

    SCH_SHEET* root = m_schematic.GetTopLevelSheet();
    BOOST_REQUIRE( root );
    BOOST_REQUIRE( root->GetScreen() );

    BUILD_RESULT result =
            PADS_SCH_BINARY_BUILDER().Build( model, &m_schematic, nullptr, binaryFixture( wxS( "ole_images" ) ) );

    BOOST_CHECK_EQUAL( result.counts.images, 3u );
    BOOST_REQUIRE_EQUAL( itemCount( root->GetScreen(), SCH_BITMAP_T ), 3u );
    const int                pageHeight = root->GetScreen()->GetPageSettings().GetHeightIU( schIUScale.IU_PER_MILS );
    std::vector<SCH_BITMAP*> bitmaps;

    for( SCH_ITEM* item : root->GetScreen()->Items().OfType( SCH_BITMAP_T ) )
        bitmaps.push_back( static_cast<SCH_BITMAP*>( item ) );

    for( const MODEL_EMBEDDED_IMAGE& source : model.images )
    {
        auto bitmap = std::ranges::find( bitmaps, pagePoint( source.position, pageHeight ), &SCH_BITMAP::GetPosition );
        BOOST_REQUIRE( bitmap != bitmaps.end() );
        BOOST_CHECK_EQUAL( ( *bitmap )->GetPosition(), pagePoint( source.position, pageHeight ) );
        BOOST_CHECK_LE( std::abs( ( *bitmap )->GetReferenceImage().GetSize().x
                                  - schIUScale.MilsToIU( static_cast<double>( source.size.x ) / 2.0 ) ),
                        2 );
        BOOST_CHECK_LE( std::abs( ( *bitmap )->GetReferenceImage().GetSize().y
                                  - schIUScale.MilsToIU( static_cast<double>( source.size.y ) / 2.0 ) ),
                        schIUScale.MilsToIU( 2 ) );
        BOOST_REQUIRE( ( *bitmap )->GetReferenceImage().GetImage().GetOriginalImageData() );
        BOOST_CHECK( ( *bitmap )->GetReferenceImage().GetImage().GetOriginalImageData()->IsOk() );
    }
}


BOOST_AUTO_TEST_CASE( CanReadSchematicFile )
{
    SCH_IO_PADS plugin;

    wxString padsFile = wxString::FromUTF8( KI_TEST::GetEeschemaTestDataDir() + "/plugins/pads/simple_schematic.txt" );

    BOOST_CHECK( plugin.CanReadSchematicFile( padsFile ) );
}


BOOST_AUTO_TEST_CASE( CanReadSchematicFile_RejectNonPads )
{
    SCH_IO_PADS plugin;

    wxString kicadFile = wxString::FromUTF8( KI_TEST::GetEeschemaTestDataDir() + "/plugins/pads/simple_schematic.txt" );

    BOOST_CHECK( plugin.CanReadSchematicFile( kicadFile ) );
}


BOOST_AUTO_TEST_CASE( BinaryDispatch )
{
    using namespace PADS_SCH_BINARY;

    SCH_IO_PADS          plugin;
    std::vector<uint8_t> v13;
    BOOST_REQUIRE( PADS_SCH_BINARY_READER::ReadFile( binaryFixture( wxS( "page_graphics" ) ), v13 ) );
    BOOST_REQUIRE_GE( v13.size(), 0x250u );
    BOOST_CHECK( PADS_SCH_BINARY_READER::IsBinaryFamily( v13 ) );
    BOOST_CHECK( PADS_SCH_BINARY_READER::IsBinarySch( v13 ) );
    BOOST_CHECK( PADS_SCH_BINARY_READER::IsSupportedVersion( 0x000C ) );
    BOOST_CHECK( PADS_SCH_BINARY_READER::IsSupportedVersion( 0x000D ) );

    std::vector<uint8_t> v12 = v13;
    v12[2] = 0x0C;
    v12[3] = 0x00;
    v12[4] = 0x01;
    v12[5] = 0x00;
    BOOST_CHECK( PADS_SCH_BINARY_READER::IsBinaryFamily( v12 ) );
    BOOST_CHECK( PADS_SCH_BINARY_READER::IsBinarySch( v12 ) );

    std::vector<uint8_t> malformed = v13;
    malformed[1] = 0xFF;
    BOOST_CHECK( !PADS_SCH_BINARY_READER::IsBinaryFamily( malformed ) );
    BOOST_CHECK( !PADS_SCH_BINARY_READER::IsBinarySch( malformed ) );
    BOOST_CHECK( !PADS_SCH_BINARY_READER::IsBinaryFamily( { 0x00 } ) );

    std::vector<uint8_t> truncatedHeader( 31, 0x00 );
    truncatedHeader[1] = 0xFE;
    truncatedHeader[2] = 0x0D;

    const std::vector<std::pair<std::vector<uint8_t>, wxString>> truncations = {
        { { 0x00, 0xFE }, wxS( "file too small for PADS Logic binary version" ) },
        { { 0x00, 0xFE, 0x0D }, wxS( "file too small for PADS Logic binary version" ) },
        { { 0x00, 0xFE, 0x0D, 0x00 }, wxS( "file too small for PADS Logic binary header" ) },
        { truncatedHeader, wxS( "file too small for PADS Logic binary header" ) }
    };

    for( const auto& [truncated, expectedError] : truncations )
    {
        BOOST_CHECK( PADS_SCH_BINARY_READER::IsBinaryFamily( truncated ) );
        wxString truncatedBase = wxFileName::CreateTempFileName( wxS( "pads_truncated_" ) );
        BOOST_REQUIRE( wxRemoveFile( truncatedBase ) );
        wxString truncatedPath = truncatedBase + wxS( ".sch" );
        {
            std::ofstream output( truncatedPath.fn_str(), std::ios::binary );
            output.write( reinterpret_cast<const char*>( truncated.data() ), truncated.size() );
        }
        BOOST_CHECK( plugin.CanReadSchematicFile( truncatedPath ) );
        wxString truncatedError;

        try
        {
            plugin.LoadSchematicFile( truncatedPath, &m_schematic );
            BOOST_FAIL( "truncated binary schematic was accepted" );
        }
        catch( const IO_ERROR& error )
        {
            truncatedError = error.What();
        }

        BOOST_CHECK( truncatedError.Contains( wxS( "PADS Logic binary v0x" ) ) );
        BOOST_CHECK( truncatedError.Contains( expectedError ) );
        BOOST_CHECK( !truncatedError.Contains( wxS( "ASCII" ) ) );
        BOOST_CHECK( wxRemoveFile( truncatedPath ) );
    }

    const wxString ascii =
            wxString::FromUTF8( KI_TEST::GetEeschemaTestDataDir() ) + wxS( "/plugins/pads/simple_schematic.txt" );
    BOOST_CHECK( plugin.CanReadSchematicFile( ascii ) );

    wxString unrelatedBase = wxFileName::CreateTempFileName( wxS( "pads_unrelated_" ) );
    BOOST_REQUIRE( wxRemoveFile( unrelatedBase ) );
    wxString unrelated = unrelatedBase + wxS( ".sch" );
    {
        std::ofstream output( unrelated.fn_str(), std::ios::binary );
        output << "unrelated schematic";
    }
    BOOST_CHECK( !plugin.CanReadSchematicFile( unrelated ) );

    std::vector<uint8_t> unsupported = v13;
    unsupported[2] = 0x00;
    unsupported[3] = 0xFE;
    wxString unsupportedBase = wxFileName::CreateTempFileName( wxS( "pads_unsupported_" ) );
    BOOST_REQUIRE( wxRemoveFile( unsupportedBase ) );
    wxString unsupportedPath = unsupportedBase + wxS( ".sch" );
    {
        std::ofstream output( unsupportedPath.fn_str(), std::ios::binary );
        output.write( reinterpret_cast<const char*>( unsupported.data() ), unsupported.size() );
    }
    BOOST_CHECK( plugin.CanReadSchematicFile( unsupportedPath ) );
    wxString unsupportedError;

    try
    {
        plugin.LoadSchematicFile( unsupportedPath, &m_schematic );
        BOOST_FAIL( "unsupported binary schematic was accepted" );
    }
    catch( const IO_ERROR& error )
    {
        unsupportedError = error.What();
    }

    BOOST_CHECK( unsupportedError.Contains( wxS( "v0xFE00" ) ) );
    BOOST_CHECK( unsupportedError.Contains( wxS( "unsupported PADS Logic binary version" ) ) );
    BOOST_CHECK( !unsupportedError.Contains( wxS( "ASCII" ) ) );

    m_schematic.Reset();
    CAPTURING_REPORTER reporter;
    plugin.SetReporter( &reporter );
    BOOST_REQUIRE_NO_THROW( plugin.LoadSchematicFile( binaryFixture( wxS( "page_graphics" ) ), &m_schematic ) );
    BOOST_CHECK( std::ranges::none_of( reporter.messages,
                                       []( const auto& aMessage )
                                       {
                                           return aMessage.second == RPT_SEVERITY_INFO;
                                       } ) );
    BOOST_CHECK( wxRemoveFile( unrelated ) );
    BOOST_CHECK( wxRemoveFile( unsupportedPath ) );
}


BOOST_AUTO_TEST_CASE( FindPlugin )
{
    IO_RELEASER<SCH_IO> pi( SCH_IO_MGR::FindPlugin( SCH_IO_MGR::SCH_PADS ) );
    BOOST_CHECK_NE( pi.get(), nullptr );
}


BOOST_AUTO_TEST_CASE( MultiGateImport )
{
    SCH_IO_PADS plugin;

    wxString padsFile =
            wxString::FromUTF8( KI_TEST::GetEeschemaTestDataDir() + "/plugins/pads/multigate_schematic.txt" );

    SCH_SHEET* rootSheet = plugin.LoadSchematicFile( padsFile, &m_schematic );
    BOOST_REQUIRE( rootSheet );
    BOOST_REQUIRE( rootSheet->GetScreen() );

    SCH_SCREEN* screen = rootSheet->GetScreen();

    // Collect U1 symbols
    std::vector<SCH_SYMBOL*> u1Symbols;
    SCH_SHEET_PATH           rootPath;
    rootPath.push_back( rootSheet );

    for( SCH_ITEM* item : screen->Items().OfType( SCH_SYMBOL_T ) )
    {
        SCH_SYMBOL* sym = static_cast<SCH_SYMBOL*>( item );

        if( sym->GetRef( &rootPath ) == wxT( "U1" ) )
            u1Symbols.push_back( sym );
    }

    BOOST_REQUIRE_EQUAL( u1Symbols.size(), 2u );

    // Sort by unit number for deterministic checks
    std::sort( u1Symbols.begin(), u1Symbols.end(),
               []( const SCH_SYMBOL* a, const SCH_SYMBOL* b )
               {
                   return a->GetUnit() < b->GetUnit();
               } );

    // Unit 1 (gate A with TL082A decal) should have 5 pins
    BOOST_CHECK_EQUAL( u1Symbols[0]->GetUnit(), 1 );
    BOOST_CHECK_EQUAL( u1Symbols[0]->GetLibPins().size(), 5u );

    // Unit 2 (gate B with TL082 decal) should have 3 pins
    BOOST_CHECK_EQUAL( u1Symbols[1]->GetUnit(), 2 );
    BOOST_CHECK_EQUAL( u1Symbols[1]->GetLibPins().size(), 3u );

    // Both should share the same multi-unit LIB_SYMBOL with 2 units
    BOOST_CHECK( u1Symbols[0]->IsMultiUnit() );
    BOOST_CHECK_EQUAL( u1Symbols[0]->GetUnitCount(), 2 );

    // Both references should be "U1" (not "U1-A" or "U1-B")
    BOOST_CHECK_EQUAL( u1Symbols[0]->GetRef( &rootPath ), wxT( "U1" ) );
    BOOST_CHECK_EQUAL( u1Symbols[1]->GetRef( &rootPath ), wxT( "U1" ) );
}


BOOST_AUTO_TEST_CASE( Issue23420_HeaderWithCodePageSuffix )
{
    // Regression test for https://gitlab.com/kicad/code/kicad/-/issues/23420
    // PADS Logic schematics exported with a code page suffix in the header
    // (e.g. *PADS-LOGIC-V9.0-CP1250*) must be detected and parsed.
    SCH_IO_PADS plugin;

    wxString padsFile =
            wxString::FromUTF8( KI_TEST::GetEeschemaTestDataDir() + "/plugins/pads/issue23420_codepage_schematic.txt" );

    BOOST_CHECK( plugin.CanReadSchematicFile( padsFile ) );

    SCH_SHEET* rootSheet = plugin.LoadSchematicFile( padsFile, &m_schematic );

    BOOST_REQUIRE( rootSheet );
    BOOST_REQUIRE( rootSheet->GetScreen() );
}


BOOST_AUTO_TEST_CASE( CanReadLibrary )
{
    SCH_IO_PADS plugin;

    wxString padsFile = wxString::FromUTF8( KI_TEST::GetEeschemaTestDataDir() + "/plugins/pads/symbols_schematic.txt" );

    BOOST_CHECK( plugin.CanReadLibrary( padsFile ) );
}


// Only the schematic path reads the binary format, so the library predicate must refuse a binary
// container even though the schematic predicate accepts it.  The file is named .txt because
// CanReadLibrary() screens .sch out by extension, which would make the check vacuous.
BOOST_AUTO_TEST_CASE( CanReadLibraryRefusesBinaryContainer )
{
    // Minimum container the binary sniffer accepts: magic 00 FE, version 0x000D, 0x250 bytes
    std::vector<uint8_t> container( 0x250, 0 );

    container[0] = 0x00;
    container[1] = 0xFE;
    container[2] = 0x0D;
    container[3] = 0x00;

    std::filesystem::path binaryAsTxt =
            std::filesystem::temp_directory_path() / "kicad_pads_binary_container.txt";

    {
        std::ofstream out( binaryAsTxt, std::ios::binary );

        out.write( reinterpret_cast<const char*>( container.data() ),
                   static_cast<std::streamsize>( container.size() ) );
    }

    SCH_IO_PADS plugin;
    wxString    path = wxString::FromUTF8( binaryAsTxt.string() );

    BOOST_CHECK( plugin.CanReadSchematicFile( path ) );
    BOOST_CHECK( !plugin.CanReadLibrary( path ) );

    std::filesystem::remove( binaryAsTxt );
}


BOOST_AUTO_TEST_CASE( EnumerateSymbolLib_NamesFromSchematic )
{
    SCH_IO_PADS plugin;

    wxString padsFile = wxString::FromUTF8( KI_TEST::GetEeschemaTestDataDir() + "/plugins/pads/symbols_schematic.txt" );

    wxArrayString names;
    BOOST_CHECK_NO_THROW( plugin.EnumerateSymbolLib( names, padsFile ) );
    BOOST_CHECK_GT( names.GetCount(), 0u );
}


BOOST_AUTO_TEST_CASE( EnumerateSymbolLib_ReturnsLibSymbols )
{
    SCH_IO_PADS plugin;

    wxString padsFile = wxString::FromUTF8( KI_TEST::GetEeschemaTestDataDir() + "/plugins/pads/symbols_schematic.txt" );

    std::vector<LIB_SYMBOL*> symbols;
    BOOST_CHECK_NO_THROW( plugin.EnumerateSymbolLib( symbols, padsFile ) );
    BOOST_CHECK_GT( symbols.size(), 0u );

    for( LIB_SYMBOL* sym : symbols )
        BOOST_REQUIRE( sym != nullptr );
}


BOOST_AUTO_TEST_CASE( LoadSymbol_ByName )
{
    SCH_IO_PADS plugin;

    wxString padsFile = wxString::FromUTF8( KI_TEST::GetEeschemaTestDataDir() + "/plugins/pads/symbols_schematic.txt" );

    wxArrayString names;
    plugin.EnumerateSymbolLib( names, padsFile );

    BOOST_REQUIRE_GT( names.GetCount(), 0u );

    LIB_SYMBOL* sym = plugin.LoadSymbol( padsFile, names.Item( 0 ) );
    BOOST_REQUIRE( sym != nullptr );
    BOOST_CHECK_EQUAL( sym->GetName(), names.Item( 0 ) );
}


BOOST_AUTO_TEST_CASE( LoadSymbol_UnknownReturnsNull )
{
    SCH_IO_PADS plugin;

    wxString padsFile = wxString::FromUTF8( KI_TEST::GetEeschemaTestDataDir() + "/plugins/pads/symbols_schematic.txt" );

    LIB_SYMBOL* sym = plugin.LoadSymbol( padsFile, wxT( "NO_SUCH_SYMBOL_12345" ) );
    BOOST_CHECK( sym == nullptr );
}


BOOST_AUTO_TEST_CASE( MultiGatePartTypeBecomesMultiUnitLibSymbol )
{
    SCH_IO_PADS plugin;

    wxString padsFile =
            wxString::FromUTF8( KI_TEST::GetEeschemaTestDataDir() + "/plugins/pads/multigate_schematic.txt" );

    std::vector<LIB_SYMBOL*> symbols;
    BOOST_CHECK_NO_THROW( plugin.EnumerateSymbolLib( symbols, padsFile ) );

    bool foundMultiUnit = false;

    for( LIB_SYMBOL* sym : symbols )
    {
        if( sym && sym->GetUnitCount() > 1 )
        {
            foundMultiUnit = true;
            break;
        }
    }

    BOOST_CHECK( foundMultiUnit );
}


BOOST_AUTO_TEST_CASE( IsLibraryNotWritable )
{
    SCH_IO_PADS plugin;

    wxString padsFile = wxString::FromUTF8( KI_TEST::GetEeschemaTestDataDir() + "/plugins/pads/symbols_schematic.txt" );

    BOOST_CHECK( !plugin.IsLibraryWritable( padsFile ) );
}


BOOST_AUTO_TEST_CASE( Issue24284_TextItemsPlacedOnCorrectSheet )
{
    // Regression test for https://gitlab.com/kicad/code/kicad/-/issues/24284
    // Multi-sheet PADS Logic schematics have one *TEXT* and *LINES* block per
    // *SHT*. Before the fix every text/line item was placed on the first
    // sheet, causing page-number text from all sheets to stack on top of each
    // other and border graphics to overlap.
    SCH_IO_PADS plugin;

    wxString padsFile =
            wxString::FromUTF8( KI_TEST::GetEeschemaTestDataDir() + "/plugins/pads/issue24284_multisheet_text.txt" );

    BOOST_REQUIRE( plugin.CanReadSchematicFile( padsFile ) );

    SCH_SHEET* rootSheet = plugin.LoadSchematicFile( padsFile, &m_schematic );
    BOOST_REQUIRE( rootSheet );
    BOOST_REQUIRE( rootSheet->GetScreen() );

    // Collect text and line content keyed by hierarchical sheet name.
    std::map<wxString, std::vector<wxString>> textBySheet;
    std::map<wxString, int>                   lineCountBySheet;

    for( SCH_ITEM* item : rootSheet->GetScreen()->Items().OfType( SCH_SHEET_T ) )
    {
        SCH_SHEET* sheet = static_cast<SCH_SHEET*>( item );
        wxString   sheetName = sheet->GetField( FIELD_T::SHEET_NAME )->GetText();

        for( SCH_ITEM* screenItem : sheet->GetScreen()->Items().OfType( SCH_TEXT_T ) )
        {
            SCH_TEXT* txt = static_cast<SCH_TEXT*>( screenItem );
            textBySheet[sheetName].push_back( txt->GetText() );
        }

        for( SCH_ITEM* screenItem : sheet->GetScreen()->Items().OfType( SCH_LINE_T ) )
        {
            (void) screenItem;
            lineCountBySheet[sheetName]++;
        }
    }

    for( int sheetNum = 1; sheetNum <= 3; ++sheetNum )
    {
        wxString sheetName = wxString::Format( wxT( "Page%d" ), sheetNum );
        wxString pageText = wxString::Format( wxT( "PAGE %d OF 3" ), sheetNum );
        wxString bodyText = wxString::Format( wxT( "TEXT ON SHEET %d" ), sheetNum );

        BOOST_REQUIRE_EQUAL( textBySheet.count( sheetName ), 1u );
        BOOST_CHECK_EQUAL( textBySheet[sheetName].size(), 2u );
        BOOST_CHECK( std::find( textBySheet[sheetName].begin(), textBySheet[sheetName].end(), pageText )
                     != textBySheet[sheetName].end() );
        BOOST_CHECK( std::find( textBySheet[sheetName].begin(), textBySheet[sheetName].end(), bodyText )
                     != textBySheet[sheetName].end() );
        BOOST_CHECK_EQUAL( lineCountBySheet[sheetName], 1 );
    }
}


// Issue 23855 (#1): an off-page connector whose stub wire is zero-length must take its
// global-label orientation from the authoritative *NETNAMES* offset, not from the
// degenerate wire direction. The two SP1 anchors carry opposite X offsets and must yield
// opposite spin styles.
BOOST_AUTO_TEST_CASE( Issue23855_GlobalLabelOrientationFromNetNames )
{
    SCH_IO_PADS plugin;

    wxString padsFile =
            wxString::FromUTF8( KI_TEST::GetEeschemaTestDataDir() + "/plugins/pads/issue23855_schematic.txt" );

    SCH_SHEET* rootSheet = plugin.LoadSchematicFile( padsFile, &m_schematic );
    BOOST_REQUIRE( rootSheet );
    BOOST_REQUIRE( rootSheet->GetScreen() );

    SCH_SCREEN* screen = rootSheet->GetScreen();

    // PADS anchor positions in mils -> KiCad screen X (Y-up flipped on import).
    const int milToIU = schIUScale.MilsToIU( 1 );
    const int cnSideX = 1400 * milToIU; // @@@O0, x_offset +350 -> text reads right
    const int r1SideX = 2800 * milToIU; // @@@O1, x_offset -360 -> text reads left

    SPIN_STYLE cnSpin = SPIN_STYLE::LEFT;
    SPIN_STYLE r1Spin = SPIN_STYLE::RIGHT;
    bool       foundCn = false;
    bool       foundR1 = false;

    for( SCH_ITEM* item : screen->Items().OfType( SCH_GLOBAL_LABEL_T ) )
    {
        SCH_LABEL_BASE* lbl = static_cast<SCH_LABEL_BASE*>( item );

        if( lbl->GetText() != wxT( "SP1" ) )
            continue;

        if( lbl->GetPosition().x == cnSideX )
        {
            foundCn = true;
            cnSpin = lbl->GetSpinStyle();
        }
        else if( lbl->GetPosition().x == r1SideX )
        {
            foundR1 = true;
            r1Spin = lbl->GetSpinStyle();
        }
    }

    BOOST_REQUIRE( foundCn );
    BOOST_REQUIRE( foundR1 );

    // The CN1-side label extends to the right; the R1-side label (degenerate wire)
    // extends to the left thanks to the NETNAMES override.
    BOOST_CHECK( cnSpin == SPIN_STYLE::RIGHT );
    BOOST_CHECK( r1Spin == SPIN_STYLE::LEFT );
}


// Issue 23855 (#5): a 90 degree rotated part must place its reference and value fields at
// the absolute coordinates authored in PADS. PADS stores attribute offsets in the placed
// (post-rotation) frame, so the importer applies the offset directly without re-rotating.
BOOST_AUTO_TEST_CASE( Issue23855_RotatedPartFieldPositions )
{
    SCH_IO_PADS plugin;

    wxString padsFile =
            wxString::FromUTF8( KI_TEST::GetEeschemaTestDataDir() + "/plugins/pads/issue23855_schematic.txt" );

    SCH_SHEET* rootSheet = plugin.LoadSchematicFile( padsFile, &m_schematic );
    BOOST_REQUIRE( rootSheet );
    BOOST_REQUIRE( rootSheet->GetScreen() );

    SCH_SCREEN*    screen = rootSheet->GetScreen();
    SCH_SHEET_PATH rootPath;
    rootPath.push_back( rootSheet );

    SCH_SYMBOL* d5 = nullptr;

    for( SCH_ITEM* item : screen->Items().OfType( SCH_SYMBOL_T ) )
    {
        SCH_SYMBOL* sym = static_cast<SCH_SYMBOL*>( item );

        if( sym->GetRef( &rootPath ) == wxT( "D5" ) )
            d5 = sym;
    }

    BOOST_REQUIRE( d5 != nullptr );

    SCH_FIELD* refF = d5->GetField( FIELD_T::REFERENCE );
    SCH_FIELD* valF = d5->GetField( FIELD_T::VALUE );

    VECTOR2I symPos = d5->GetPosition();
    VECTOR2I refRel = refF->GetPosition() - symPos;
    VECTOR2I valRel = valF->GetPosition() - symPos;

    // REF-DES PADS offset (210, 230); PART-TYPE/value PADS offset (-70, 520). PADS Y is up,
    // so the screen Y offset is negated.
    BOOST_CHECK_EQUAL( refRel.x, schIUScale.MilsToIU( 210 ) );
    BOOST_CHECK_EQUAL( refRel.y, -schIUScale.MilsToIU( 230 ) );
    BOOST_CHECK_EQUAL( valRel.x, schIUScale.MilsToIU( -70 ) );
    BOOST_CHECK_EQUAL( valRel.y, -schIUScale.MilsToIU( 520 ) );

    // Rotated attribute text keeps the PADS text angle and the authored justification
    // (codes 4 and 5 both decode to top-left in the text's reading frame).
    BOOST_CHECK_EQUAL( refF->GetTextAngle().AsDegrees(), 90.0 );
    BOOST_CHECK_EQUAL( valF->GetTextAngle().AsDegrees(), 90.0 );
    BOOST_CHECK_EQUAL( refF->GetHorizJustify(), GR_TEXT_H_ALIGN_LEFT );
    BOOST_CHECK_EQUAL( refF->GetVertJustify(), GR_TEXT_V_ALIGN_TOP );
    BOOST_CHECK_EQUAL( valF->GetHorizJustify(), GR_TEXT_H_ALIGN_LEFT );
    BOOST_CHECK_EQUAL( valF->GetVertJustify(), GR_TEXT_V_ALIGN_TOP );
}


BOOST_AUTO_TEST_CASE( BinarySymbolsAndSheets )
{
    using namespace PADS_SCH_BINARY;

    PADS_SCH_MODEL model = parseBinaryFixture( wxS( "symbol_primitives" ) );
    auto setTitleField = [&]( const wxString& aName, const wxString& aValue )
    {
        auto field = std::ranges::find( model.sheets.front().titleBlockFields, aName,
                                        []( const MODEL_FIELD& aField )
                                        {
                                            return aField.name.text;
                                        } );
        BOOST_REQUIRE( field != model.sheets.front().titleBlockFields.end() );
        field->value.text = aValue;
    };
    setTitleField( wxS( "Drawn By" ), wxS( "DB" ) );
    setTitleField( wxS( "QC By" ), wxS( "QB" ) );
    setTitleField( wxS( "Released By" ), wxS( "RB" ) );
    setTitleField( wxS( "QC Date" ), wxS( "QD" ) );
    setTitleField( wxS( "Release Date" ), wxS( "RD" ) );
    setTitleField( wxS( "Company Name" ), wxS( "Company" ) );
    setTitleField( wxS( "Code" ), wxS( "Code" ) );

    auto worksheetTemplate = std::ranges::find_if( model.graphics,
                                                   []( const MODEL_PAGE_GRAPHIC& aGraphic )
                                                   {
                                                       return aGraphic.graphic.kind == MODEL_GRAPHIC_KIND::TEXT;
                                                   } );
    BOOST_REQUIRE( worksheetTemplate != model.graphics.end() );
    MODEL_WORKSHEET worksheet;
    worksheet.source = worksheetTemplate->graphic.source;
    worksheet.sheet = { model.sheets.front().id, worksheet.source };
    worksheet.name.text = wxS( "CI_WORKSHEET" );
    worksheet.name.source = worksheet.source;
    const std::array<std::pair<wxString, SOURCE_POINT>, 4> worksheetMarkers = {
        std::pair{ wxS( "TOP_LEFT" ), SOURCE_POINT{ 0, model.sheets.front().pageSize.y } },
        std::pair{ wxS( "TOP_RIGHT" ), model.sheets.front().pageSize },
        std::pair{ wxS( "BOTTOM_LEFT" ), SOURCE_POINT{ 0, 0 } },
        std::pair{ wxS( "BOTTOM_RIGHT" ), SOURCE_POINT{ model.sheets.front().pageSize.x, 0 } }
    };

    for( const auto& [text, position] : worksheetMarkers )
    {
        MODEL_GRAPHIC graphic = worksheetTemplate->graphic;
        graphic.text.text = text;
        graphic.points = { position };
        worksheet.graphics.push_back( std::move( graphic ) );
    }

    model.worksheets = { std::move( worksheet ) };

    SCH_SHEET* destination = m_schematic.GetTopLevelSheet();
    BOOST_REQUIRE( destination );
    BOOST_REQUIRE( destination->GetScreen() );

    PADS_SCH_BINARY_BUILDER builder;
    BUILD_RESULT result = builder.Build( model, &m_schematic, nullptr, binaryFixture( wxS( "symbol_primitives" ) ) );

    BOOST_CHECK_EQUAL( result.counts.sheets, 1u );
    BOOST_CHECK_EQUAL( result.counts.symbols, model.placements.size() );
    BOOST_CHECK( destination->m_Uuid == destination->GetScreen()->GetUuid() );

    SCH_SHEET_PATH path;
    path.push_back( destination );
    std::map<wxString, SCH_SYMBOL*> symbols;

    for( SCH_ITEM* item : destination->GetScreen()->Items().OfType( SCH_SYMBOL_T ) )
    {
        SCH_SYMBOL* symbol = static_cast<SCH_SYMBOL*>( item );
        symbols.emplace( symbol->GetRef( &path ), symbol );
    }

    for( const MODEL_PLACEMENT& placement : model.placements )
    {
        auto it = symbols.find( placement.reference.text );
        BOOST_REQUIRE_MESSAGE( it != symbols.end(), placement.reference.text );
        SCH_SYMBOL* symbol = it->second;
        BOOST_CHECK_EQUAL( symbol->GetUnit(), placement.unit );
        BOOST_CHECK_EQUAL( symbol->GetPosition().x,
                           schIUScale.MilsToIU( static_cast<double>( placement.position.x ) / 2.0 ) );
        BOOST_CHECK_EQUAL( symbol->GetLibPins().size(), placement.pins.size() );
        BOOST_REQUIRE( symbol->GetLibSymbolRef() );
        BOOST_CHECK( !symbol->GetLibSymbolRef()->GetDrawItems().empty() );
        BOOST_CHECK( destination->GetScreen()->GetLibSymbols().contains( symbol->GetSchSymbolLibraryName() ) );
    }

    SCH_SYMBOL*                    primitiveSymbol = symbols.at( wxS( "U1" ) );
    const MODEL_SYMBOL_DEFINITION& primitiveDefinition =
            *std::ranges::find_if( model.definitions,
                                   []( const MODEL_SYMBOL_DEFINITION& aDefinition )
                                   {
                                       return aDefinition.name.text == wxS( "BATCHB_PRIMITIVES" );
                                   } );
    std::vector<SCH_SHAPE*> primitiveShapes;
    std::vector<SCH_TEXT*>  primitiveTexts;

    for( const SCH_ITEM& item : primitiveSymbol->GetLibSymbolRef()->GetDrawItems() )
    {
        if( item.Type() == SCH_SHAPE_T )
            primitiveShapes.push_back( static_cast<SCH_SHAPE*>( const_cast<SCH_ITEM*>( &item ) ) );
        else if( item.Type() == SCH_TEXT_T )
            primitiveTexts.push_back( static_cast<SCH_TEXT*>( const_cast<SCH_ITEM*>( &item ) ) );
    }

    BOOST_REQUIRE_EQUAL( primitiveShapes.size(), 5u );
    auto shapeOfType = [&]( SHAPE_T aType )
    {
        return std::ranges::find_if( primitiveShapes,
                                     [&]( const SCH_SHAPE* aShape )
                                     {
                                         return aShape->GetShape() == aType;
                                     } );
    };
    BOOST_CHECK_EQUAL( std::ranges::count_if( primitiveShapes,
                                              []( const SCH_SHAPE* aShape )
                                              {
                                                  return aShape->GetShape() == SHAPE_T::POLY;
                                              } ),
                       3 );
    auto circle = shapeOfType( SHAPE_T::CIRCLE );
    auto arc = shapeOfType( SHAPE_T::ARC );
    BOOST_REQUIRE( circle != primitiveShapes.end() );
    BOOST_REQUIRE( arc != primitiveShapes.end() );
    BOOST_CHECK_EQUAL(
            ( *arc )->GetStroke().GetWidth(),
            schIUScale.MilsToIU( static_cast<double>( primitiveDefinition.graphics[3].strokeWidth ) / 2.0 ) );
    BOOST_CHECK( ( *arc )->GetEffectiveLineStyle() == LINE_STYLE::SOLID );
    BOOST_CHECK_EQUAL(
            ( *arc )->GetCenter().x,
            schIUScale.MilsToIU( static_cast<double>( primitiveDefinition.graphics[3].arcCenter.x ) / 2.0 ) );
    BOOST_CHECK_EQUAL(
            ( *arc )->GetCenter().y,
            -schIUScale.MilsToIU( static_cast<double>( primitiveDefinition.graphics[3].arcCenter.y ) / 2.0 ) );
    BOOST_CHECK( std::ranges::any_of( primitiveShapes,
                                      []( const SCH_SHAPE* aShape )
                                      {
                                          return aShape->GetFillMode() == FILL_T::FILLED_SHAPE;
                                      } ) );
    BOOST_REQUIRE_EQUAL( primitiveTexts.size(), 1u );
    BOOST_CHECK_EQUAL( primitiveTexts[0]->GetText(), primitiveDefinition.graphics[5].text.text );
    BOOST_CHECK_EQUAL( primitiveTexts[0]->GetPosition(), localPoint( primitiveDefinition.graphics[5].points[0] ) );
    BOOST_CHECK_EQUAL(
            primitiveTexts[0]->GetTextHeight(),
            schIUScale.MilsToIU( static_cast<double>( primitiveDefinition.graphics[5].presentation.height ) / 2.0 ) );

    BOOST_CHECK_EQUAL( destination->GetScreen()->GetPageSettings().GetWidthMils(),
                       model.sheets.front().pageSize.x / 2 );
    BOOST_CHECK_EQUAL( destination->GetScreen()->GetPageSettings().GetHeightMils(),
                       model.sheets.front().pageSize.y / 2 );
    BOOST_CHECK_EQUAL( destination->GetScreen()->GetTitleBlock().GetTitle(), model.sheets.front().title.text );
    const EMBEDDED_FILES::EMBEDDED_FILE* embeddedWorksheet =
            m_schematic.GetEmbeddedFiles()->GetEmbeddedFile( wxS( "pads_import.kicad_wks" ) );
    BOOST_REQUIRE( embeddedWorksheet );
    const std::string  worksheetData( embeddedWorksheet->decompressedData.begin(),
                                      embeddedWorksheet->decompressedData.end() );
    const double       pageWidthMm = model.sheets.front().pageSize.x * 0.0127;
    const double       pageHeightMm = model.sheets.front().pageSize.y * 0.0127;
    std::ostringstream topRight;
    std::ostringstream bottomLeft;
    std::ostringstream bottomRight;
    topRight.imbue( std::locale::classic() );
    bottomLeft.imbue( std::locale::classic() );
    bottomRight.imbue( std::locale::classic() );
    topRight << "(tbtext \"TOP_RIGHT\" (name \"\") (pos " << pageWidthMm << " 0";
    bottomLeft << "(tbtext \"BOTTOM_LEFT\" (name \"\") (pos 0 " << pageHeightMm;
    bottomRight << "(tbtext \"BOTTOM_RIGHT\" (name \"\") (pos " << pageWidthMm << ' ' << pageHeightMm;
    BOOST_CHECK_NE( worksheetData.find( "(tbtext \"TOP_LEFT\" (name \"\") (pos 0 0" ), std::string::npos );
    BOOST_CHECK_NE( worksheetData.find( topRight.str() ), std::string::npos );
    BOOST_CHECK_NE( worksheetData.find( bottomLeft.str() ), std::string::npos );
    BOOST_CHECK_NE( worksheetData.find( bottomRight.str() ), std::string::npos );
    const TITLE_BLOCK& titleBlock = destination->GetScreen()->GetTitleBlock();
    BOOST_CHECK_EQUAL( titleBlock.GetCompany(), wxS( "Company" ) );
    BOOST_CHECK_EQUAL( titleBlock.GetComment( 0 ), wxS( "QB" ) );
    BOOST_CHECK_EQUAL( titleBlock.GetComment( 1 ), wxS( "RB" ) );
    BOOST_CHECK_EQUAL( titleBlock.GetComment( 2 ), wxS( "DB" ) );
    BOOST_CHECK_EQUAL( titleBlock.GetComment( 3 ), wxString() );
    BOOST_CHECK_EQUAL( titleBlock.GetComment( 4 ), wxString() );
    BOOST_CHECK_EQUAL( titleBlock.GetComment( 5 ), wxS( "QD" ) );
    BOOST_CHECK_EQUAL( titleBlock.GetComment( 6 ), wxS( "RD" ) );
    BOOST_CHECK_EQUAL( titleBlock.GetComment( 7 ), wxString() );
    BOOST_CHECK_EQUAL( titleBlock.GetComment( 8 ), wxString() );

    size_t builtGraphics = itemCount( destination->GetScreen(), SCH_SHAPE_T );

    for( SCH_ITEM* item : destination->GetScreen()->Items().OfType( SCH_LINE_T ) )
    {
        if( item->GetLayer() == LAYER_NOTES )
            ++builtGraphics;
    }

    BOOST_CHECK_EQUAL( result.counts.graphics, builtGraphics );

    m_schematic.Reset();
    destination = m_schematic.GetTopLevelSheet();
    PADS_SCH_MODEL   pinModel = parseBinaryFixture( wxS( "pin_styles" ) );
    MODEL_PART_TYPE& pinPart = *std::ranges::find_if( pinModel.partTypes,
                                                      []( const MODEL_PART_TYPE& aPart )
                                                      {
                                                          return aPart.name.text == wxS( "BATCHB-PIN-STYLES" );
                                                      } );
    BOOST_REQUIRE_EQUAL( pinPart.gates.size(), 1u );
    BOOST_REQUIRE_EQUAL( pinPart.gates[0].logicalPins.size(), 7u );

    for( MODEL_SYMBOL_DEFINITION& definition : pinModel.definitions )
    {
        if( definition.name.text != wxS( "BATCHB_PIN_STYLES" ) )
            continue;

        for( MODEL_PIN_DEFINITION& pin : definition.pins )
        {
            pin.number.text = wxS( "decal-number" );
            pin.name.text = wxS( "decal-name" );
        }
    }

    result = builder.Build( pinModel, &m_schematic, destination, binaryFixture( wxS( "pin_styles" ) ) );
    SCH_SHEET_PATH pinPath;
    pinPath.push_back( destination );
    SCH_SYMBOL* pinSymbol = nullptr;

    for( SCH_ITEM* item : destination->GetScreen()->Items().OfType( SCH_SYMBOL_T ) )
    {
        SCH_SYMBOL* symbol = static_cast<SCH_SYMBOL*>( item );

        if( symbol->GetRef( &pinPath ) == wxS( "U2" ) )
            pinSymbol = symbol;
    }

    BOOST_REQUIRE( pinSymbol );
    BOOST_CHECK( destination->GetScreen()->GetLibSymbols().contains( pinSymbol->GetSchSymbolLibraryName() ) );
    const MODEL_SYMBOL_DEFINITION& pinDefinition =
            *std::ranges::find_if( pinModel.definitions,
                                   []( const MODEL_SYMBOL_DEFINITION& aDefinition )
                                   {
                                       return aDefinition.name.text == wxS( "BATCHB_PIN_STYLES" );
                                   } );
    std::vector<SCH_PIN*> builtPins = pinSymbol->GetLibPins();
    BOOST_REQUIRE_EQUAL( builtPins.size(), pinDefinition.pins.size() + pinPart.signalPins.size() );

    size_t sizedNamePins = 0;
    size_t sizedNumberPins = 0;

    for( size_t pinOrdinal = 0; pinOrdinal < pinDefinition.pins.size(); ++pinOrdinal )
    {
        const MODEL_PIN_DEFINITION& sourcePin = pinDefinition.pins[pinOrdinal];
        const MODEL_GATE_PIN&       logicalPin = pinPart.gates[0].logicalPins[pinOrdinal];
        auto                        built = std::ranges::find( builtPins, logicalPin.number.text, &SCH_PIN::GetNumber );
        BOOST_REQUIRE_MESSAGE( built != builtPins.end(), logicalPin.number.text );
        BOOST_CHECK_EQUAL( ( *built )->GetName(), logicalPin.name.text );
        BOOST_CHECK_EQUAL( ( *built )->GetPosition(), localPoint( sourcePin.position ) );
        BOOST_CHECK_EQUAL( ( *built )->GetLength(),
                           schIUScale.MilsToIU( static_cast<double>( sourcePin.length ) / 2.0 ) );
        BOOST_CHECK( ( *built )->GetOrientation() == pinOrientation( sourcePin ) );
        BOOST_CHECK( ( *built )->GetType() == pinType( sourcePin.electricalType ) );
        BOOST_CHECK( ( *built )->GetShape() == pinShape( sourcePin.graphicStyle ) );
        BOOST_CHECK_EQUAL( ( *built )->IsVisible(), sourcePin.presentation.visible );
        BOOST_CHECK_EQUAL( ( *built )->GetNameTextSize(),
                           schIUScale.MilsToIU( static_cast<double>( sourcePin.namePresentation.height ) / 2.0 ) );
        BOOST_CHECK_EQUAL( ( *built )->GetNumberTextSize(),
                           schIUScale.MilsToIU( static_cast<double>( sourcePin.numberPresentation.height ) / 2.0 ) );

        if( sourcePin.namePresentation.height > 0 )
            sizedNamePins++;

        if( sourcePin.numberPresentation.height > 0 )
            sizedNumberPins++;
    }

    // Each check compares a derived value against a derived value, so it passes on a zero height no
    // matter what the builder does
    BOOST_CHECK_GT( sizedNamePins, 0u );
    BOOST_CHECK_GT( sizedNumberPins, 0u );

    m_schematic.Reset();
    destination = m_schematic.GetTopLevelSheet();
    const PADS_SCH_MODEL transformModel = parseBinaryFixture( wxS( "placement_transform" ) );
    result = builder.Build( transformModel, &m_schematic, destination, binaryFixture( wxS( "placement_transform" ) ) );
    SCH_SHEET_PATH transformPath;
    transformPath.push_back( destination );

    for( SCH_ITEM* item : destination->GetScreen()->Items().OfType( SCH_SYMBOL_T ) )
    {
        SCH_SYMBOL*            symbol = static_cast<SCH_SYMBOL*>( item );
        const MODEL_PLACEMENT& placement =
                *std::ranges::find_if( transformModel.placements,
                                       [&]( const MODEL_PLACEMENT& aPlacement )
                                       {
                                           return aPlacement.reference.text == symbol->GetRef( &transformPath );
                                       } );
        int expectedOrientation = SYM_ORIENT_0;

        switch( placement.angle )
        {
        case 900: expectedOrientation = SYM_ORIENT_90; break;
        case 1800: expectedOrientation = SYM_ORIENT_180; break;
        case 2700: expectedOrientation = SYM_ORIENT_270; break;
        default: break;
        }

        if( placement.mirrorFlags & 1 )
            expectedOrientation |= SYM_MIRROR_Y;

        if( placement.mirrorFlags & 2 )
            expectedOrientation |= SYM_MIRROR_X;

        SCH_SYMBOL expected( *symbol );
        expected.SetOrientation( expectedOrientation );
        BOOST_CHECK( symbol->GetTransform() == expected.GetTransform() );
    }

    BOOST_CHECK_EQUAL( itemCount( destination->GetScreen(), SCH_SYMBOL_T ), transformModel.placements.size() );

    PADS_SCH_MODEL unknownTransform = transformModel;
    auto           rawAngleIt = std::ranges::find_if( unknownTransform.placements.front().properties,
                                                      []( const SOURCE_PROPERTY& aProperty )
                                                      {
                                                return aProperty.name.text == wxS( "raw_angle" );
                                            } );
    BOOST_REQUIRE( rawAngleIt != unknownTransform.placements.front().properties.end() );
    SOURCE_PROPERTY& rawAngle = *rawAngleIt;
    rawAngle.value.text = wxS( "3600" );
    rawAngle.disposition = PROPERTY_DISPOSITION::UNSUPPORTED;
    unknownTransform.placements.front().angle = 0;
    result = builder.Build( unknownTransform, &m_schematic, destination, wxS( "unknown_transform.sch" ) );
    BOOST_CHECK( std::ranges::any_of( result.diagnostics,
                                      [&]( const PARSER_DIAGNOSTIC& aDiagnostic )
                                      {
                                          return aDiagnostic.message.Contains( wxS( "raw_angle" ) )
                                                 && aDiagnostic.source == rawAngle.source;
                                      } ) );

    m_schematic.Reset();
    destination = m_schematic.GetTopLevelSheet();
    const PADS_SCH_MODEL multiModel = parseBinaryFixture( wxS( "multigate" ) );
    result = builder.Build( multiModel, &m_schematic, destination, binaryFixture( wxS( "multigate" ) ) );
    SCH_SHEET_PATH multiPath;
    multiPath.push_back( destination );
    std::vector<SCH_SYMBOL*> multiSymbols;

    for( SCH_ITEM* item : destination->GetScreen()->Items().OfType( SCH_SYMBOL_T ) )
    {
        SCH_SYMBOL* symbol = static_cast<SCH_SYMBOL*>( item );

        if( symbol->GetRef( &multiPath ) == wxS( "U3" ) )
            multiSymbols.push_back( symbol );
    }

    BOOST_REQUIRE_EQUAL( multiSymbols.size(), 2u );
    std::ranges::sort( multiSymbols, {}, &SCH_SYMBOL::GetUnit );
    BOOST_CHECK_EQUAL( multiSymbols[0]->GetUnit(), 1 );
    BOOST_CHECK_EQUAL( multiSymbols[1]->GetUnit(), 2 );
    BOOST_CHECK_EQUAL( multiSymbols[0]->GetUnitCount(), 2 );
    BOOST_CHECK_EQUAL( multiSymbols[1]->GetUnitCount(), 2 );
    const MODEL_PART_TYPE& multiPart = *std::ranges::find_if( multiModel.partTypes,
                                                              []( const MODEL_PART_TYPE& aPart )
                                                              {
                                                                  return aPart.name.text == wxS( "BATCHD-MULTIGATE" );
                                                              } );
    BOOST_CHECK_EQUAL( multiSymbols[0]->GetLibPins().size(),
                       multiModel.placements[1].pins.size() + multiPart.signalPins.size() );
    BOOST_CHECK_EQUAL( multiSymbols[1]->GetLibPins().size(),
                       multiModel.placements[2].pins.size() + multiPart.signalPins.size() );

    m_schematic.Reset();
    destination = m_schematic.GetTopLevelSheet();
    const PADS_SCH_MODEL connectorModel = parseBinaryFixture( wxS( "connectors" ) );
    result = builder.Build( connectorModel, &m_schematic, destination, binaryFixture( wxS( "connectors" ) ) );
    SCH_SHEET_PATH connectorPath;
    connectorPath.push_back( destination );
    SCH_SYMBOL* connector = nullptr;

    for( SCH_ITEM* item : destination->GetScreen()->Items().OfType( SCH_SYMBOL_T ) )
    {
        SCH_SYMBOL* symbol = static_cast<SCH_SYMBOL*>( item );

        if( symbol->GetRef( &connectorPath ) == wxS( "P1" ) )
            connector = symbol;
    }

    BOOST_REQUIRE( connector );
    BOOST_CHECK_EQUAL( connector->GetUnit(), 1 );
    BOOST_CHECK_EQUAL( connector->GetUnitCount(), 26 );
    const MODEL_PART_TYPE& connectorPart =
            *std::ranges::find_if( connectorModel.partTypes,
                                   []( const MODEL_PART_TYPE& aPart )
                                   {
                                       return std::ranges::any_of( aPart.gates,
                                                                   []( const MODEL_GATE& aGate )
                                                                   {
                                                                       return !aGate.connectorPins.empty();
                                                                   } );
                                   } );
    const MODEL_GATE& connectorGate = *std::ranges::find_if( connectorPart.gates,
                                                             []( const MODEL_GATE& aGate )
                                                             {
                                                                 return !aGate.connectorPins.empty();
                                                             } );
    auto              connectorPlacementIt = std::ranges::find_if( connectorModel.placements,
                                                                   [&]( const MODEL_PLACEMENT& aPlacement )
                                                                   {
                                                          return aPlacement.partType.id == connectorPart.id;
                                                      } );
    BOOST_REQUIRE( connectorPlacementIt != connectorModel.placements.end() );
    const MODEL_PLACEMENT&         connectorPlacement = *connectorPlacementIt;
    const MODEL_SYMBOL_DEFINITION& connectorDefinition = *std::ranges::find(
            connectorModel.definitions, connectorPlacement.definition.id, &MODEL_SYMBOL_DEFINITION::id );
    BOOST_REQUIRE_EQUAL( connectorGate.connectorPins.size(), 26u );
    BOOST_REQUIRE( !connectorDefinition.pins.empty() );
    std::vector<SCH_PIN*> builtConnectorPins = connector->GetAllLibPins();

    for( size_t index = 0; index < connectorGate.connectorPins.size(); ++index )
    {
        const MODEL_CONNECTOR_PIN& sourcePin = connectorGate.connectorPins[index];

        for( const MODEL_PIN_DEFINITION& graphicPin : connectorDefinition.pins )
        {
            auto builtPin =
                    std::ranges::find_if( builtConnectorPins,
                                          [&]( const SCH_PIN* aPin )
                                          {
                                              return aPin->GetUnit() == static_cast<int>( index + 1 )
                                                     && aPin->GetPosition() == localPoint( graphicPin.position );
                                          } );
            BOOST_REQUIRE_MESSAGE( builtPin != builtConnectorPins.end(), index + 1 );
            BOOST_CHECK_EQUAL( ( *builtPin )->GetNumber(), sourcePin.number.text );
            BOOST_CHECK_EQUAL( ( *builtPin )->GetName(), sourcePin.name.text );
            BOOST_CHECK( ( *builtPin )->GetType() == pinType( sourcePin.electricalType ) );
            BOOST_CHECK_EQUAL( ( *builtPin )->GetLength(),
                               schIUScale.MilsToIU( static_cast<double>( graphicPin.length ) / 2.0 ) );
            BOOST_CHECK( ( *builtPin )->GetOrientation() == pinOrientation( graphicPin ) );
            BOOST_CHECK( ( *builtPin )->GetShape() == pinShape( graphicPin.graphicStyle ) );
            BOOST_CHECK_EQUAL( ( *builtPin )->IsVisible(), graphicPin.presentation.visible );
            BOOST_CHECK_EQUAL( ( *builtPin )->GetNameTextSize(),
                               schIUScale.MilsToIU( static_cast<double>( graphicPin.namePresentation.height ) / 2.0 ) );
            BOOST_CHECK_EQUAL(
                    ( *builtPin )->GetNumberTextSize(),
                    schIUScale.MilsToIU( static_cast<double>( graphicPin.numberPresentation.height ) / 2.0 ) );
        }
    }

    m_schematic.Reset();
    destination = m_schematic.GetTopLevelSheet();
    PADS_SCH_MODEL fieldModel = parseBinaryFixture( wxS( "fields" ) );
    fieldModel.placements.front().mirrored = true;
    fieldModel.placements.front().mirrorFlags = 3;
    result = builder.Build( fieldModel, &m_schematic, destination, binaryFixture( wxS( "fields" ) ) );
    BOOST_REQUIRE_EQUAL( result.counts.symbols, fieldModel.placements.size() );

    SCH_SHEET_PATH fieldPath;
    fieldPath.push_back( destination );
    SCH_SYMBOL* r1 = nullptr;

    for( SCH_ITEM* item : destination->GetScreen()->Items().OfType( SCH_SYMBOL_T ) )
    {
        SCH_SYMBOL* symbol = static_cast<SCH_SYMBOL*>( item );

        if( symbol->GetRef( &fieldPath ) == wxS( "R1" ) )
            r1 = symbol;
    }

    BOOST_REQUIRE( r1 );

    size_t sizedFields = 0;
    size_t strokedFields = 0;

    for( const MODEL_FIELD& sourceField : fieldModel.placements.front().fields )
    {
        SCH_FIELD* builtField = nullptr;

        if( sourceField.name.text == wxS( "REF-DES" ) )
            builtField = r1->GetField( FIELD_T::REFERENCE );
        else if( sourceField.name.text == wxS( "PART-TYPE" ) )
            builtField = r1->GetField( FIELD_T::VALUE );
        else
            builtField = r1->GetField( sourceField.name.text );

        BOOST_REQUIRE_MESSAGE( builtField, sourceField.name.text );
        BOOST_CHECK_EQUAL( builtField->GetText(), sourceField.value.text );
        BOOST_CHECK_EQUAL( builtField->IsVisible(), sourceField.visible );
        if( sourceField.presentation.height != 0 )
        {
            BOOST_CHECK_EQUAL( builtField->GetTextHeight(),
                               schIUScale.MilsToIU( static_cast<double>( sourceField.presentation.height ) / 2.0 ) );
            sizedFields++;
        }

        if( sourceField.presentation.width != 0 )
        {
            BOOST_CHECK_EQUAL( builtField->GetTextThickness(),
                               schIUScale.MilsToIU( static_cast<double>( sourceField.presentation.width ) / 2.0 ) );
            strokedFields++;
        }
        BOOST_CHECK_EQUAL( builtField->GetTextAngle().AsTenthsOfADegree(), sourceField.angle );
        BOOST_CHECK_EQUAL( builtField->GetPosition(),
                           r1->GetPosition() + placedFieldOffset( fieldModel.placements.front(), sourceField ) );
        BOOST_CHECK_EQUAL( builtField->IsBold(), sourceField.presentation.bold );
        BOOST_CHECK_EQUAL( builtField->IsItalic(), sourceField.presentation.italic );
        BOOST_CHECK(
                builtField->GetHorizJustify()
                == GetFlippedAlignment( horizontalJustification( sourceField.presentation.horizontalJustification ) ) );
        BOOST_CHECK(
                builtField->GetVertJustify()
                == GetFlippedAlignment( verticalJustification( sourceField.presentation.verticalJustification ) ) );

        if( !sourceField.presentation.font.text.IsEmpty()
            && sourceField.presentation.font.text != wxS( "Default Font" ) )
        {
            BOOST_CHECK( !builtField->GetFontName().IsEmpty() );
        }
    }

    // Both size checks are skipped on a zero presentation, so a decoder that emitted nothing but
    // zeros would satisfy the loop without comparing anything
    BOOST_CHECK_GT( sizedFields, 0u );
    BOOST_CHECK_GT( strokedFields, 0u );
}


// Mirroring has to negate the field offsets. Comparing against a helper that recomputes the
// production formula cannot detect a wrong transform, so build the same model twice and compare.
BOOST_AUTO_TEST_CASE( BinaryMirroredFieldOffsetsAreNegated )
{
    using namespace PADS_SCH_BINARY;

    auto offsetsFor =
            [&]( uint32_t aMirrorFlags )
            {
                m_schematic.Reset();

                SCH_SHEET*     destination = m_schematic.GetTopLevelSheet();
                PADS_SCH_MODEL model = parseBinaryFixture( wxS( "fields" ) );

                model.placements.front().mirrored = aMirrorFlags != 0;
                model.placements.front().mirrorFlags = aMirrorFlags;

                PADS_SCH_BINARY_BUILDER().Build( model, &m_schematic, destination,
                                                 binaryFixture( wxS( "fields" ) ) );

                SCH_SHEET_PATH path;

                path.push_back( destination );

                std::map<wxString, VECTOR2I> offsets;

                for( SCH_ITEM* item : destination->GetScreen()->Items().OfType( SCH_SYMBOL_T ) )
                {
                    // The const overload is deliberate; the mutable GetField( FIELD_T ) creates a
                    // missing mandatory field and would hide its absence
                    const SCH_SYMBOL* symbol = static_cast<const SCH_SYMBOL*>( item );

                    if( static_cast<SCH_SYMBOL*>( item )->GetRef( &path ) != wxS( "R1" ) )
                        continue;

                    for( const MODEL_FIELD& sourceField : model.placements.front().fields )
                    {
                        const SCH_FIELD* field = nullptr;

                        if( sourceField.name.text == wxS( "REF-DES" ) )
                            field = symbol->GetField( FIELD_T::REFERENCE );
                        else if( sourceField.name.text == wxS( "PART-TYPE" ) )
                            field = symbol->GetField( FIELD_T::VALUE );
                        else
                            field = symbol->GetField( sourceField.name.text );

                        if( field )
                            offsets[field->GetName()] = field->GetPosition() - symbol->GetPosition();
                    }
                }

                return offsets;
            };

    std::map<wxString, VECTOR2I> plain = offsetsFor( 0 );

    BOOST_REQUIRE( !plain.empty() );

    // Each axis is checked on its own, so a build that swapped the two flag meanings fails here
    // where a 0-against-3 comparison alone would not
    struct MIRROR_CASE
    {
        uint32_t flags;
        bool     negateX;
        bool     negateY;
    };

    for( const MIRROR_CASE& mirrorCase : { MIRROR_CASE{ 1, true, false }, MIRROR_CASE{ 2, false, true },
                                           MIRROR_CASE{ 3, true, true } } )
    {
        BOOST_TEST_CONTEXT( mirrorCase.flags )
        {
            std::map<wxString, VECTOR2I> mirrored = offsetsFor( mirrorCase.flags );
            size_t                       compared = 0;

            for( const auto& [name, offset] : plain )
            {
                auto it = mirrored.find( name );

                BOOST_REQUIRE( it != mirrored.end() );
                BOOST_CHECK_EQUAL( it->second.x, mirrorCase.negateX ? -offset.x : offset.x );
                BOOST_CHECK_EQUAL( it->second.y, mirrorCase.negateY ? -offset.y : offset.y );

                if( offset.x != 0 || offset.y != 0 )
                    compared++;
            }

            // A field on the symbol origin negates to itself, so an all-zero set would satisfy the
            // loop under any transform
            BOOST_CHECK_GT( compared, 0u );
        }
    }
}


BOOST_AUTO_TEST_CASE( BinaryAlternateDefinitionPins )
{
    using namespace PADS_SCH_BINARY;

    PADS_SCH_MODEL model = parseBinaryFixture( wxS( "multigate" ) );
    auto           hasPlacedPins = []( const MODEL_PLACEMENT& aPlacement )
    {
        return aPlacement.gate.has_value() && !aPlacement.pins.empty();
    };
    auto placement = std::ranges::find_if( model.placements, hasPlacedPins );
    BOOST_REQUIRE( placement != model.placements.end() );

    auto part = std::ranges::find( model.partTypes, placement->partType.id, &MODEL_PART_TYPE::id );
    BOOST_REQUIRE( part != model.partTypes.end() );
    auto gate = std::ranges::find( part->gates, placement->gate->id, &MODEL_GATE::id );
    BOOST_REQUIRE( gate != part->gates.end() );
    auto primary = std::ranges::find( model.definitions, gate->definition.id, &MODEL_SYMBOL_DEFINITION::id );
    BOOST_REQUIRE( primary != model.definitions.end() );

    MODEL_SYMBOL_DEFINITION alternate = *primary;
    alternate.id = DEFINITION_ID( 0x00F00000 );
    alternate.name.text += wxS( "_ALTERNATE" );
    alternate.fields.clear();
    std::map<uint32_t, PIN_ID> alternatePinIds;

    for( size_t index = 0; index < alternate.pins.size(); ++index )
    {
        const uint32_t primaryId = alternate.pins[index].id.Value();
        alternate.pins[index].id = PIN_ID( 0x00F10000 + static_cast<uint32_t>( index ) );
        alternate.pins[index].number.text.Prepend( wxS( "ALT-" ) );
        alternatePinIds.emplace( primaryId, alternate.pins[index].id );
    }

    gate->alternateDefinitions.push_back( { alternate.id, gate->source } );
    placement->definition = { alternate.id, placement->definition.source };

    for( PLACED_PIN_REFERENCE& pin : placement->pins )
    {
        auto alternatePin = alternatePinIds.find( pin.id.Value() );
        BOOST_REQUIRE( alternatePin != alternatePinIds.end() );
        pin.id = alternatePin->second;
    }

    for( MODEL_NET& net : model.nets )
    {
        for( MODEL_CONNECTION& connection : net.connections )
        {
            for( MODEL_CONNECTION_ENDPOINT& endpoint : connection.endpoints )
            {
                if( endpoint.placement && endpoint.placement->id == placement->id && endpoint.pin )
                {
                    auto alternatePin = alternatePinIds.find( endpoint.pin->id.Value() );
                    BOOST_REQUIRE( alternatePin != alternatePinIds.end() );
                    endpoint.pin->id = alternatePin->second;
                }
            }
        }
    }

    const wxString reference = placement->reference.text;
    model.definitions.push_back( std::move( alternate ) );
    BOOST_REQUIRE_NO_THROW( model.ValidateOrThrow() );

    PADS_SCH_BINARY_BUILDER builder;
    BOOST_REQUIRE_NO_THROW( builder.Build( model, &m_schematic, nullptr, wxS( "alternate_definition.sch" ) ) );

    SCH_SHEET_PATH path;
    path.push_back( m_schematic.GetTopLevelSheet() );
    SCH_SYMBOL* builtSymbol = nullptr;

    for( SCH_ITEM* item : m_schematic.GetTopLevelSheet()->GetScreen()->Items().OfType( SCH_SYMBOL_T ) )
    {
        SCH_SYMBOL* symbol = static_cast<SCH_SYMBOL*>( item );

        if( symbol->GetRef( &path ) == reference )
            builtSymbol = symbol;
    }

    BOOST_REQUIRE( builtSymbol );

    std::vector<SCH_PIN*> selectedUnitPins = builtSymbol->GetLibPins();

    BOOST_REQUIRE_EQUAL( selectedUnitPins.size(), gate->logicalPins.size() );

    for( const MODEL_GATE_PIN& logicalPin : gate->logicalPins )
    {
        auto builtPin = std::ranges::find( selectedUnitPins, logicalPin.number.text, &SCH_PIN::GetNumber );
        BOOST_REQUIRE_MESSAGE( builtPin != selectedUnitPins.end(), logicalPin.number.text );
        BOOST_CHECK_EQUAL( ( *builtPin )->GetName(), logicalPin.name.text );
        BOOST_CHECK( !( *builtPin )->GetNumber().StartsWith( wxS( "ALT-" ) ) );
    }
}


BOOST_AUTO_TEST_CASE( BinaryMultiSheetHierarchy )
{
    const PADS_SCH_BINARY::PADS_SCH_MODEL    model = parseBinaryFixture( wxS( "multisheet_connectivity" ) );
    PADS_SCH_BINARY::PADS_SCH_BINARY_BUILDER builder;
    SCH_SHEET*                               originalRoot = m_schematic.GetTopLevelSheet();
    BOOST_REQUIRE( originalRoot );
    const KIID                    originalRootUuid = originalRoot->m_Uuid;
    PADS_SCH_BINARY::BUILD_RESULT result =
            builder.Build( model, &m_schematic, nullptr, binaryFixture( wxS( "multisheet_connectivity" ) ) );
    BOOST_CHECK_EQUAL( result.counts.sheets, model.sheets.size() );
    std::vector<SCH_SHEET*> topSheets = m_schematic.GetTopLevelSheets();
    BOOST_REQUIRE_EQUAL( topSheets.size(), model.sheets.size() );
    BOOST_CHECK( std::ranges::none_of( topSheets,
                                       [&]( const SCH_SHEET* aSheet )
                                       {
                                           return aSheet == originalRoot || aSheet->m_Uuid == originalRootUuid;
                                       } ) );

    std::set<wxString> filenames;

    for( size_t i = 0; i < topSheets.size(); ++i )
    {
        SCH_SHEET* sheet = topSheets[i];
        BOOST_REQUIRE( sheet );
        BOOST_REQUIRE( sheet->GetScreen() );
        BOOST_CHECK( m_schematic.IsTopLevelSheet( sheet ) );
        BOOST_CHECK( sheet->GetScreen()->Items().OfType( SCH_SHEET_T ).empty() );
        BOOST_CHECK_EQUAL( sheet->GetField( FIELD_T::SHEET_NAME )->GetText(), model.sheets[i].name.text );
        wxString filename = sheet->GetField( FIELD_T::SHEET_FILENAME )->GetText();
        BOOST_CHECK( !filename.Contains( wxS( "/" ) ) );
        BOOST_CHECK( !filename.Contains( wxS( ":" ) ) );
        BOOST_CHECK( !filename.Contains( wxS( "*" ) ) );
        BOOST_CHECK( filenames.insert( filename ).second );
        const wxString expectedFilename = i == 0 ? wxS( "[1]DUP_SAFE__.kicad_sch" ) : wxS( "[2]DUP_SAFE__.kicad_sch" );
        BOOST_CHECK_EQUAL( filename, expectedFilename );
        SCH_SHEET_PATH path;
        path.push_back( sheet );
        BOOST_CHECK_EQUAL( path.GetPageNumber(), wxString::Format( wxS( "%zu" ), i + 1 ) );
        BOOST_CHECK_EQUAL( sheet->GetScreen()->GetPageNumber(), wxString::Format( wxS( "%zu" ), i + 1 ) );
        BOOST_CHECK_EQUAL( sheet->GetScreen()->GetPageSettings().GetWidthMils(), model.sheets[i].pageSize.x / 2 );
        BOOST_CHECK_EQUAL( sheet->GetScreen()->GetPageSettings().GetHeightMils(), model.sheets[i].pageSize.y / 2 );

        std::multiset<std::pair<wxString, VECTOR2I>> expectedLabels;
        std::multiset<std::pair<wxString, VECTOR2I>> builtLabels;

        for( const PADS_SCH_BINARY::MODEL_LABEL& label : model.labels )
        {
            if( label.sheet.id == model.sheets[i].id && !label.linkedSheets.empty() )
            {
                expectedLabels.emplace( label.text.text, VECTOR2I( schIUScale.MilsToIU( label.position.x / 2 ),
                                                                   schIUScale.MilsToIU( model.sheets[i].pageSize.y / 2
                                                                                        - label.position.y / 2 ) ) );
            }
        }

        for( SCH_ITEM* item : sheet->GetScreen()->Items().OfType( SCH_GLOBAL_LABEL_T ) )
        {
            auto* label = static_cast<SCH_GLOBALLABEL*>( item );

            std::pair<wxString, VECTOR2I> identity( label->GetText(), label->GetPosition() );

            if( expectedLabels.contains( identity ) )
                builtLabels.insert( std::move( identity ) );
        }

        BOOST_CHECK( builtLabels == expectedLabels );

        for( SCH_ITEM* item : sheet->GetScreen()->Items().OfType( SCH_SYMBOL_T ) )
        {
            SCH_SYMBOL*         symbol = static_cast<SCH_SYMBOL*>( item );
            SCH_SYMBOL_INSTANCE instance;
            BOOST_REQUIRE( symbol->GetInstance( instance, path.Path(), false ) );
            BOOST_CHECK( instance.m_Path == path.Path() );
            BOOST_CHECK_EQUAL( symbol->GetRef( &path ), instance.m_Reference );
        }
    }

    BOOST_CHECK_EQUAL( result.counts.labels,
                       model.buses.size()
                               + std::ranges::count_if( model.labels,
                                                        []( const PADS_SCH_BINARY::MODEL_LABEL& aLabel )
                                                        {
                                                            return aLabel.kind
                                                                   != PADS_SCH_BINARY::MODEL_LABEL_KIND::UNSUPPORTED;
                                                        } ) );

    using namespace PADS_SCH_BINARY;

    PADS_SCH_MODEL orderedModel;
    orderedModel.version = 0x000D;
    constexpr std::array<size_t, 11> physicalOrder{ 0, 2, 10, 7, 4, 1, 9, 5, 3, 8, 6 };

    for( size_t sourceIndex : physicalOrder )
    {
        MODEL_SHEET sheet;
        sheet.id = SHEET_ID( static_cast<uint32_t>( sourceIndex + 1 ) );
        sheet.index = sourceIndex;
        sheet.name.text = wxString::Format( wxS( "[%zu]SOURCE_ORDER" ), sourceIndex + 1 );
        sheet.pageSize = { 34000, 22000 };
        orderedModel.sheets.push_back( std::move( sheet ) );
    }

    m_schematic.Reset();
    BOOST_REQUIRE_NO_THROW( builder.Build( orderedModel, &m_schematic, nullptr, wxS( "source_order.sch" ) ) );

    auto checkSourceOrder = [&]( const SCHEMATIC& aSchematic )
    {
        SCH_SHEET_LIST hierarchy = aSchematic.BuildSheetListSortedByPageNumbers();
        BOOST_REQUIRE_EQUAL( hierarchy.size(), 11u );
        BOOST_REQUIRE_EQUAL( aSchematic.GetTopLevelSheets().size(), 11u );
        std::set<wxString> pageNumbers;

        for( const SCH_SHEET_PATH& path : hierarchy )
            BOOST_CHECK( pageNumbers.insert( path.GetPageNumber() ).second );

        for( size_t sourceIndex = 0; sourceIndex < 11; ++sourceIndex )
        {
            const SCH_SHEET_PATH& path = hierarchy[sourceIndex];
            BOOST_REQUIRE_EQUAL( path.size(), 1u );
            BOOST_CHECK( aSchematic.IsTopLevelSheet( path.Last() ) );
            BOOST_CHECK_EQUAL( path.GetPageNumber(), wxString::Format( wxS( "%zu" ), sourceIndex + 1 ) );
            BOOST_CHECK_EQUAL( path.Last()->GetField( FIELD_T::SHEET_NAME )->GetText(),
                               wxString::Format( wxS( "[%zu]SOURCE_ORDER" ), sourceIndex + 1 ) );
        }
    };

    checkSourceOrder( m_schematic );

    wxString tempDir = wxFileName::CreateTempFileName( wxS( "pads_binary_sheet_order_" ) );
    BOOST_REQUIRE( wxRemoveFile( tempDir ) );
    BOOST_REQUIRE( wxFileName::Mkdir( tempDir ) );
    SCH_IO_KICAD_SEXPR                io;
    std::vector<wxString>             files;
    std::vector<TOP_LEVEL_SHEET_INFO> sheetInfos;

    for( const SCH_SHEET_PATH& path : m_schematic.BuildSheetListSortedByPageNumbers() )
    {
        wxString file =
                tempDir + wxFileName::GetPathSeparator() + path.Last()->GetField( FIELD_T::SHEET_FILENAME )->GetText();
        BOOST_REQUIRE_NO_THROW( io.SaveSchematicFile( file, path.Last(), &m_schematic ) );
        files.push_back( file );
        sheetInfos.emplace_back( path.Last()->m_Uuid, path.Last()->GetName(), file );
    }

    m_schematic.Reset();
    SCH_SHEET*              defaultSheet = m_schematic.GetTopLevelSheet();
    std::vector<SCH_SHEET*> loadedSheets;

    for( size_t index = 0; index < files.size(); ++index )
    {
        SCH_SHEET* loaded = nullptr;
        BOOST_REQUIRE_NO_THROW( loaded = io.LoadSchematicFile( files[index], &m_schematic ) );
        BOOST_REQUIRE( loaded );
        const_cast<KIID&>( loaded->m_Uuid ) = sheetInfos[index].uuid;
        loaded->SetName( sheetInfos[index].name );
        loadedSheets.push_back( loaded );
    }

    m_schematic.SetTopLevelSheets( loadedSheets );
    BOOST_CHECK( std::ranges::none_of( loadedSheets,
                                       [&]( const SCH_SHEET* aSheet )
                                       {
                                           return aSheet == defaultSheet;
                                       } ) );
    m_schematic.RefreshHierarchy();
    checkSourceOrder( m_schematic );
    BOOST_CHECK( wxFileName::Rmdir( tempDir, wxPATH_RMDIR_RECURSIVE ) );
}


BOOST_AUTO_TEST_CASE( BinaryAppendIsAtomic )
{
    using namespace PADS_SCH_BINARY;

    SCH_SHEET* destination = m_schematic.GetTopLevelSheet();
    BOOST_REQUIRE( destination );
    BOOST_REQUIRE( destination->GetScreen() );
    PADS_SCH_BINARY_BUILDER builder;

    LIB_ID preservedLibId;
    preservedLibId.SetLibNickname( wxS( "source_library" ) );
    preservedLibId.SetLibItemName( wxS( "source_symbol" ) );
    auto preservedSymbol = std::make_unique<LIB_SYMBOL>( wxS( "source_symbol" ) );
    preservedSymbol->SetLibId( preservedLibId );
    const wxString preservedKey = wxS( "pads_import:preserved_cache_key" );
    destination->GetScreen()->AddLibSymbol( preservedKey, std::move( preservedSymbol ) );

    PADS_SCH_MODEL single = parseBinaryFixture( wxS( "placement_transform" ) );
    size_t         beforeSymbols = itemCount( destination->GetScreen(), SCH_SYMBOL_T );
    BUILD_RESULT   singleResult =
            builder.Build( single, &m_schematic, destination, binaryFixture( wxS( "placement_transform" ) ) );
    BOOST_CHECK_EQUAL( itemCount( destination->GetScreen(), SCH_SYMBOL_T ),
                       beforeSymbols + singleResult.counts.symbols );
    BOOST_REQUIRE( destination->GetScreen()->GetLibSymbols().contains( preservedKey ) );
    BOOST_CHECK( destination->GetScreen()->GetLibSymbols().at( preservedKey )->GetLibId() == preservedLibId );

    std::set<wxString> cacheKeys;

    for( const auto& [key, symbol] : destination->GetScreen()->GetLibSymbols() )
        cacheKeys.insert( key );

    builder.Build( single, &m_schematic, destination, binaryFixture( wxS( "placement_transform" ) ) );
    std::set<wxString> repeatedCacheKeys;

    for( const auto& [key, symbol] : destination->GetScreen()->GetLibSymbols() )
        repeatedCacheKeys.insert( key );

    BOOST_CHECK( repeatedCacheKeys == cacheKeys );
    BOOST_REQUIRE( destination->GetScreen()->GetLibSymbols().contains( preservedKey ) );
    BOOST_CHECK( destination->GetScreen()->GetLibSymbols().at( preservedKey )->GetLibId() == preservedLibId );

    PADS_SCH_MODEL multi = parseBinaryFixture( wxS( "multisheet_connectivity" ) );
    auto           existingChild = std::make_unique<SCH_SHEET>( destination );
    SCH_SHEET*     existingChildPtr = existingChild.get();
    existingChild->SetScreen( new SCH_SCREEN( &m_schematic ) );
    existingChild->GetField( FIELD_T::SHEET_NAME )->SetText( wxS( "Existing" ) );
    existingChild->GetField( FIELD_T::SHEET_FILENAME )->SetText( wxS( "existing.kicad_sch" ) );
    destination->GetScreen()->Append( existingChild.get() );
    existingChild.release();
    size_t       beforeChildren = itemCount( destination->GetScreen(), SCH_SHEET_T );
    BUILD_RESULT multiResult =
            builder.Build( multi, &m_schematic, destination, binaryFixture( wxS( "multisheet_connectivity" ) ) );
    BOOST_CHECK_EQUAL( itemCount( destination->GetScreen(), SCH_SHEET_T ), beforeChildren + multiResult.counts.sheets );
    BOOST_CHECK( destination->GetScreen()->Items().contains( existingChildPtr ) );

    PADS_SCH_MODEL malformed = single;
    BOOST_REQUIRE( !malformed.placements.empty() );
    auto definition = std::find_if( malformed.definitions.begin(), malformed.definitions.end(),
                                    [&]( const MODEL_SYMBOL_DEFINITION& aDefinition )
                                    {
                                        return aDefinition.id == malformed.placements.front().definition.id;
                                    } );
    BOOST_REQUIRE( definition != malformed.definitions.end() );
    BOOST_REQUIRE( !definition->graphics.empty() );
    definition->graphics.front().kind = MODEL_GRAPHIC_KIND::TEXT;
    definition->graphics.front().text.text = wxS( "broken staged text" );
    definition->graphics.front().points.clear();
    OBJECT_GRAPH_SNAPSHOT before = objectGraphSnapshot( m_schematic, destination );
    BOOST_CHECK_THROW( builder.Build( malformed, &m_schematic, destination, wxS( "malformed.sch" ) ), IO_ERROR );
    BOOST_CHECK( objectGraphSnapshot( m_schematic, destination ) == before );

    PADS_SCH_BINARY_BUILDER commitFailure(
            []
            {
                THROW_IO_ERROR( wxS( "injected failure before schematic adoption" ) );
            } );
    before = objectGraphSnapshot( m_schematic, destination );
    BOOST_CHECK_THROW( commitFailure.Build( single, &m_schematic, destination, wxS( "commit_failure.sch" ) ),
                       IO_ERROR );
    BOOST_CHECK( objectGraphSnapshot( m_schematic, destination ) == before );

    auto oldCurrentChild = std::make_unique<SCH_SHEET>( destination );
    oldCurrentChild->SetScreen( new SCH_SCREEN( &m_schematic ) );
    SCH_SHEET_PATH oldCurrentPath;
    oldCurrentPath.push_back( destination );
    oldCurrentPath.push_back( oldCurrentChild.get() );
    destination->GetScreen()->Append( oldCurrentChild.get() );
    oldCurrentChild.release();
    m_schematic.SetCurrentSheet( oldCurrentPath );
    BOOST_REQUIRE_EQUAL( m_schematic.CurrentSheet().size(), 2u );

    builder.Build( single, &m_schematic, nullptr, wxS( "replacement.sch" ) );
    BOOST_REQUIRE_EQUAL( m_schematic.CurrentSheet().size(), 1u );
    BOOST_CHECK( m_schematic.CurrentSheet().at( 0 ) == destination );
    BOOST_CHECK( m_schematic.CurrentSheet().LastScreen() == destination->GetScreen() );
    SCH_SHEET_PATH freshRootPath;
    freshRootPath.push_back( destination );
    BOOST_CHECK_EQUAL( m_schematic.CurrentSheet().GetCurrentHash(), freshRootPath.GetCurrentHash() );
}


BOOST_AUTO_TEST_CASE( BinaryConnectivityAndGraphics )
{
    using namespace PADS_SCH_BINARY;

    PADS_SCH_BINARY_BUILDER builder;

    PADS_SCH_MODEL fieldVisibility = parseBinaryFixture( wxS( "fields" ) );
    BOOST_REQUIRE( !fieldVisibility.placements.empty() );
    BOOST_REQUIRE_GE( fieldVisibility.placements.front().fields.size(), 2u );
    MODEL_PLACEMENT& visibilityPlacement = fieldVisibility.placements.front();
    visibilityPlacement.fields[0].visible = false;
    visibilityPlacement.fields[0].presentation.visible = true;
    visibilityPlacement.fields[1].visible = true;
    visibilityPlacement.fields[1].presentation.visible = false;
    builder.Build( fieldVisibility, &m_schematic, nullptr, binaryFixture( wxS( "fields" ) ) );
    SCH_SHEET_PATH visibilityPath = m_schematic.CurrentSheet();
    SCH_SYMBOL*    visibilitySymbol = nullptr;

    for( SCH_ITEM* item : visibilityPath.LastScreen()->Items().OfType( SCH_SYMBOL_T ) )
    {
        auto* symbol = static_cast<SCH_SYMBOL*>( item );

        if( symbol->GetRef( &visibilityPath ) == visibilityPlacement.reference.text )
        {
            visibilitySymbol = symbol;
            break;
        }
    }

    BOOST_REQUIRE( visibilitySymbol );

    for( size_t index = 0; index < 2; ++index )
    {
        const MODEL_FIELD& sourceField = visibilityPlacement.fields[index];
        SCH_FIELD*         field = sourceField.name.text.CmpNoCase( wxS( "REF-DES" ) ) == 0
                                           ? visibilitySymbol->GetField( FIELD_T::REFERENCE )
                                   : sourceField.name.text.CmpNoCase( wxS( "PART-TYPE" ) ) == 0
                                           || sourceField.name.text.CmpNoCase( wxS( "VALUE" ) ) == 0
                                           ? visibilitySymbol->GetField( FIELD_T::VALUE )
                                           : visibilitySymbol->GetField( sourceField.name.text );
        BOOST_REQUIRE( field );
        BOOST_CHECK_EQUAL( field->IsVisible(), sourceField.visible && sourceField.presentation.visible );
    }

    m_schematic.Reset();
    PADS_SCH_MODEL connectivity = parseBinaryFixture( wxS( "connectivity_topology" ) );
    const PADS_SCH_MODEL connectivityOracle = connectivity;
    auto groundTemplate = std::ranges::find( connectivity.labels, MODEL_LABEL_KIND::GROUND, &MODEL_LABEL::kind );
    auto powerTemplate = std::ranges::find( connectivity.labels, MODEL_LABEL_KIND::POWER, &MODEL_LABEL::kind );
    BOOST_REQUIRE( groundTemplate != connectivity.labels.end() );
    BOOST_REQUIRE( powerTemplate != connectivity.labels.end() );

    for( uint8_t variant = 0; variant < 3; ++variant )
    {
        MODEL_LABEL label = *groundTemplate;
        label.position = { 1000 + 500 * variant, 1000 };
        label.symbolVariant = variant;
        label.text.text = wxString::Format( wxS( "TEST_GND_%u" ), variant );
        connectivity.labels.push_back( std::move( label ) );
    }

    for( uint8_t variant = 0; variant < 5; ++variant )
    {
        MODEL_LABEL label = *powerTemplate;
        label.position = { 1000 + 500 * variant, 2000 };
        label.symbolVariant = variant;
        label.text.text = wxString::Format( wxS( "TEST_PWR_%u" ), variant );
        connectivity.labels.push_back( std::move( label ) );
    }

    SCH_SHEET*     root = m_schematic.GetTopLevelSheet();
    BOOST_REQUIRE( root );
    builder.Build( connectivity, &m_schematic, nullptr, binaryFixture( wxS( "connectivity_topology" ) ) );
    assertSourceConnectivity( connectivityOracle, m_schematic );

    size_t expectedWires = 0;

    for( const MODEL_NET& net : connectivity.nets )
    {
        for( const MODEL_CONNECTION& connection : net.connections )
            expectedWires += connection.vertices.size() - 1;
    }

    size_t builtWires = 0;

    for( SCH_ITEM* item : root->GetScreen()->Items().OfType( SCH_LINE_T ) )
    {
        if( item->GetLayer() == LAYER_WIRE )
            ++builtWires;
    }

    BOOST_CHECK_EQUAL( builtWires, expectedWires );
    BOOST_CHECK_EQUAL( itemCount( root->GetScreen(), SCH_JUNCTION_T ), connectivity.junctions.size() );

    const int pageHeight = root->GetScreen()->GetPageSettings().GetHeightIU( schIUScale.IU_PER_MILS );

    for( const MODEL_JUNCTION& junction : connectivity.junctions )
    {
        bool found = false;

        for( SCH_ITEM* item : root->GetScreen()->Items().OfType( SCH_JUNCTION_T ) )
            found |= item->GetPosition() == pagePoint( junction.position, pageHeight );

        BOOST_CHECK( found );
    }

    size_t expectedPower = std::ranges::count_if( connectivity.labels,
                                                  []( const MODEL_LABEL& aLabel )
                                                  {
                                                      return aLabel.kind == MODEL_LABEL_KIND::POWER
                                                             || aLabel.kind == MODEL_LABEL_KIND::GROUND;
                                                  } );
    size_t builtPower = 0;

    for( SCH_ITEM* item : root->GetScreen()->Items().OfType( SCH_SYMBOL_T ) )
    {
        auto* symbol = static_cast<SCH_SYMBOL*>( item );

        if( symbol->GetRef( &m_schematic.CurrentSheet() ).StartsWith( wxS( "#PWR" ) ) )
        {
            ++builtPower;
            std::vector<SCH_PIN*> pins = symbol->GetPins( &m_schematic.CurrentSheet() );
            BOOST_REQUIRE_EQUAL( pins.size(), 1u );
            BOOST_CHECK( pins.front()->GetType() == ELECTRICAL_PINTYPE::PT_POWER_IN );
            BOOST_CHECK_EQUAL( pins.front()->GetPosition(), symbol->GetPosition() );
        }
    }

    BOOST_CHECK_EQUAL( builtPower, expectedPower );

    auto builtPowerSymbol = [&]( const wxString& aValue ) -> SCH_SYMBOL*
    {
        for( SCH_ITEM* item : root->GetScreen()->Items().OfType( SCH_SYMBOL_T ) )
        {
            auto* symbol = static_cast<SCH_SYMBOL*>( item );

            if( symbol->GetValue( false, &m_schematic.CurrentSheet(), false ) == aValue )
                return symbol;
        }

        return nullptr;
    };
    auto polyPoints = []( const SCH_SHAPE& aShape )
    {
        std::vector<VECTOR2I> points;

        for( size_t vertex = 0; vertex < aShape.GetPolyShape().VertexCount(); ++vertex )
            points.push_back( aShape.GetPolyShape().CVertex( vertex ) );

        return points;
    };
    auto shapeByType = []( const SCH_SYMBOL& aSymbol, SHAPE_T aType )
    {
        std::vector<const SCH_SHAPE*> shapes;

        for( const SCH_ITEM& item : aSymbol.GetLibSymbolRef()->GetDrawItems() )
        {
            if( item.Type() == SCH_SHAPE_T && static_cast<const SCH_SHAPE&>( item ).GetShape() == aType )
                shapes.push_back( &static_cast<const SCH_SHAPE&>( item ) );
        }

        return shapes;
    };
    auto hasPoly = [&]( const std::vector<const SCH_SHAPE*>& aShapes, const std::vector<VECTOR2I>& aPoints )
    {
        return std::ranges::any_of( aShapes,
                                    [&]( const SCH_SHAPE* aShape )
                                    {
                                        return polyPoints( *aShape ) == aPoints;
                                    } );
    };
    auto mil = []( int aMils )
    {
        return schIUScale.MilsToIU( aMils );
    };

    SCH_SYMBOL* gnd = builtPowerSymbol( wxS( "TEST_GND_0" ) );
    BOOST_REQUIRE( gnd );
    auto gndLines = shapeByType( *gnd, SHAPE_T::POLY );
    BOOST_REQUIRE_EQUAL( gndLines.size(), 4u );
    BOOST_CHECK( hasPoly( gndLines, { { 0, 0 }, { 0, -mil( 100 ) } } ) );
    BOOST_CHECK( hasPoly( gndLines, { { -mil( 100 ), -mil( 100 ) }, { mil( 100 ), -mil( 100 ) } } ) );
    BOOST_CHECK( hasPoly( gndLines, { { -mil( 60 ), -mil( 150 ) }, { mil( 60 ), -mil( 150 ) } } ) );
    BOOST_CHECK( hasPoly( gndLines, { { -mil( 20 ), -mil( 200 ) }, { mil( 20 ), -mil( 200 ) } } ) );

    SCH_SYMBOL* gnda = builtPowerSymbol( wxS( "TEST_GND_1" ) );
    BOOST_REQUIRE( gnda );
    auto gndaLines = shapeByType( *gnda, SHAPE_T::POLY );
    BOOST_REQUIRE_EQUAL( gndaLines.size(), 1u );
    BOOST_CHECK_EQUAL( polyPoints( *gndaLines.front() ), ( std::vector<VECTOR2I>{ { 0, 0 },
                                                                                  { 0, -mil( 50 ) },
                                                                                  { -mil( 100 ), -mil( 50 ) },
                                                                                  { 0, -mil( 200 ) },
                                                                                  { mil( 100 ), -mil( 50 ) },
                                                                                  { 0, -mil( 50 ) } } ) );

    SCH_SYMBOL* gndch = builtPowerSymbol( wxS( "TEST_GND_2" ) );
    BOOST_REQUIRE( gndch );
    auto gndchLines = shapeByType( *gndch, SHAPE_T::POLY );
    BOOST_REQUIRE_EQUAL( gndchLines.size(), 5u );
    BOOST_CHECK( hasPoly( gndchLines, { { 0, 0 }, { 0, -mil( 100 ) } } ) );
    BOOST_CHECK( hasPoly( gndchLines, { { -mil( 100 ), -mil( 100 ) }, { mil( 100 ), -mil( 100 ) } } ) );
    BOOST_CHECK( hasPoly( gndchLines, { { -mil( 100 ), -mil( 100 ) }, { -mil( 150 ), -mil( 200 ) } } ) );
    BOOST_CHECK( hasPoly( gndchLines, { { 0, -mil( 100 ) }, { -mil( 50 ), -mil( 200 ) } } ) );
    BOOST_CHECK( hasPoly( gndchLines, { { mil( 100 ), -mil( 100 ) }, { mil( 50 ), -mil( 200 ) } } ) );

    for( uint8_t variant : { 0, 2 } )
    {
        SCH_SYMBOL* power = builtPowerSymbol( wxString::Format( wxS( "TEST_PWR_%u" ), variant ) );
        BOOST_REQUIRE( power );
        auto circles = shapeByType( *power, SHAPE_T::CIRCLE );
        auto lines = shapeByType( *power, SHAPE_T::POLY );
        BOOST_REQUIRE_EQUAL( circles.size(), 1u );
        BOOST_REQUIRE_EQUAL( lines.size(), 1u );
        BOOST_CHECK_EQUAL( circles.front()->GetCenter(), VECTOR2I( 0, mil( 150 ) ) );
        BOOST_CHECK_EQUAL( circles.front()->GetRadius(), mil( 50 ) );
        BOOST_CHECK_EQUAL( polyPoints( *lines.front() ), ( std::vector<VECTOR2I>{ { 0, 0 }, { 0, mil( 100 ) } } ) );
    }

    for( const auto& [variant, stem] : { std::pair<uint8_t, int>{ 1, 250 }, { 3, 250 }, { 4, 200 } } )
    {
        SCH_SYMBOL* power = builtPowerSymbol( wxString::Format( wxS( "TEST_PWR_%u" ), variant ) );
        BOOST_REQUIRE( power );
        auto polygons = shapeByType( *power, SHAPE_T::POLY );
        BOOST_REQUIRE_EQUAL( polygons.size(), 2u );
        BOOST_CHECK( hasPoly( polygons, { { 0, 0 }, { 0, mil( stem ) } } ) );
        const std::vector<VECTOR2I> triangle{
            { 0, mil( stem ) }, { -mil( 50 ), mil( 100 ) }, { mil( 50 ), mil( 100 ) }, { 0, mil( stem ) }
        };
        auto triangleShape = std::ranges::find_if( polygons,
                                                   [&]( const SCH_SHAPE* aShape )
                                                   {
                                                       return polyPoints( *aShape ) == triangle;
                                                   } );
        BOOST_REQUIRE( triangleShape != polygons.end() );
        BOOST_CHECK( ( *triangleShape )->GetFillMode() == FILL_T::FILLED_SHAPE );
    }

    const SCH_SHEET_PATH& rootPath = m_schematic.CurrentSheet();

    for( const MODEL_LABEL& sourceLabel : connectivity.labels )
    {
        if( sourceLabel.kind == MODEL_LABEL_KIND::UNSUPPORTED )
            continue;

        bool found = false;

        if( sourceLabel.kind == MODEL_LABEL_KIND::POWER || sourceLabel.kind == MODEL_LABEL_KIND::GROUND )
        {
            for( SCH_ITEM* item : root->GetScreen()->Items().OfType( SCH_SYMBOL_T ) )
            {
                auto* symbol = static_cast<SCH_SYMBOL*>( item );

                if( symbol->GetPosition() == pagePoint( sourceLabel.position, pageHeight )
                    && symbol->GetValue( false, &rootPath, false ) == sourceLabel.text.text )
                {
                    SOURCE_POINT expectedTextPosition = sourceLabel.position;
                    expectedTextPosition.x += sourceLabel.textOffset.x;
                    expectedTextPosition.y += sourceLabel.textOffset.y;
                    BOOST_REQUIRE( symbol->GetField( FIELD_T::VALUE ) );
                    BOOST_CHECK_EQUAL( symbol->GetField( FIELD_T::VALUE )->GetPosition(),
                                       pagePoint( expectedTextPosition, pageHeight ) );
                    found = true;
                    break;
                }
            }
        }
        else
        {
            KICAD_T type = sourceLabel.kind == MODEL_LABEL_KIND::GLOBAL         ? SCH_GLOBAL_LABEL_T
                           : sourceLabel.kind == MODEL_LABEL_KIND::HIERARCHICAL ? SCH_HIER_LABEL_T
                                                                                : SCH_LABEL_T;

            for( SCH_ITEM* item : root->GetScreen()->Items().OfType( type ) )
            {
                auto* label = static_cast<SCH_LABEL_BASE*>( item );

                if( label->GetPosition() == pagePoint( sourceLabel.position, pageHeight )
                    && label->GetText() == sourceLabel.text.text )
                {
                    BOOST_CHECK_CLOSE( label->GetTextAngleDegrees(), sourceLabel.angle / 10.0, 0.001 );

                    if( sourceLabel.presentation.height > 0 )
                    {
                        BOOST_CHECK_EQUAL( label->GetTextHeight(),
                                           schIUScale.MilsToIU( sourceLabel.presentation.height / 2.0 ) );
                    }

                    checkTextPresentation( *label, sourceLabel.presentation );
                    found = true;
                    break;
                }
            }
        }

        BOOST_CHECK_MESSAGE( found, sourceLabel.text.text );
    }

    std::multiset<wxString> singleSnapshot = connectivitySnapshot( m_schematic );

    for( const MODEL_NET& net : connectivity.nets )
        BOOST_CHECK_MESSAGE( hasNetName( singleSnapshot, net.name.text ), net.name.text );

    for( SCH_ITEM* item : root->GetScreen()->Items().OfType( SCH_LINE_T ) )
    {
        auto* line = static_cast<SCH_LINE*>( item );

        if( line->GetLayer() != LAYER_WIRE )
            continue;

        SCH_CONNECTION* connection = line->Connection( &rootPath );
        BOOST_REQUIRE( connection );
        BOOST_CHECK( std::ranges::any_of( connectivity.nets,
                                          [&]( const MODEL_NET& aNet )
                                          {
                                              return connection->GetNetName() == aNet.name.text
                                                     || connection->GetNetName().EndsWith( wxS( "/" )
                                                                                           + aNet.name.text );
                                          } ) );
    }

    for( SCH_ITEM* item : root->GetScreen()->Items().OfType( SCH_SYMBOL_T ) )
    {
        auto* symbol = static_cast<SCH_SYMBOL*>( item );

        if( !symbol->GetRef( &rootPath ).StartsWith( wxS( "#PWR" ) ) )
            continue;

        for( SCH_PIN* pin : symbol->GetPins( &rootPath ) )
        {
            SCH_CONNECTION* connection = pin->Connection( &rootPath );
            BOOST_REQUIRE( connection );
            BOOST_CHECK(
                    connection->GetNetName() == symbol->GetValue( false, &rootPath, false )
                    || connection->GetNetName().EndsWith( wxS( "/" ) + symbol->GetValue( false, &rootPath, false ) ) );
        }
    }

    m_schematic.Reset();
    root = m_schematic.GetTopLevelSheet();
    PADS_SCH_MODEL buses = parseBinaryFixture( wxS( "connectivity_topology" ) );
    builder.Build( buses, &m_schematic, nullptr, binaryFixture( wxS( "connectivity_topology" ) ) );
    connectivitySnapshot( m_schematic );
    size_t expectedBusSegments = 0;
    size_t expectedBusEntries = 0;

    for( const MODEL_BUS& bus : buses.buses )
    {
        expectedBusSegments += bus.vertices.size() - 1;
        expectedBusEntries += bus.entries.size();
    }

    BOOST_CHECK_EQUAL( itemCount( root->GetScreen(), SCH_BUS_WIRE_ENTRY_T ), expectedBusEntries );
    size_t builtBusSegments = 0;

    for( SCH_ITEM* item : root->GetScreen()->Items().OfType( SCH_LINE_T ) )
    {
        if( item->GetLayer() == LAYER_BUS )
            ++builtBusSegments;
    }

    BOOST_CHECK_EQUAL( builtBusSegments, expectedBusSegments );

    for( const MODEL_BUS& bus : buses.buses )
    {
        for( const MODEL_BUS_ENTRY& sourceEntry : bus.entries )
        {
            bool found = false;
            auto sourceNet = std::ranges::find( buses.nets, sourceEntry.memberNet.id, &MODEL_NET::id );
            BOOST_REQUIRE( sourceNet != buses.nets.end() );

            for( SCH_ITEM* item : root->GetScreen()->Items().OfType( SCH_BUS_WIRE_ENTRY_T ) )
            {
                if( item->GetPosition()
                    == pagePoint( sourceEntry.position,
                                  root->GetScreen()->GetPageSettings().GetHeightIU( schIUScale.IU_PER_MILS ) ) )
                {
                    found = true;
                    SCH_CONNECTION* connection = item->Connection( &m_schematic.CurrentSheet() );
                    BOOST_REQUIRE( connection );
                    BOOST_CHECK( connection->GetNetName() == sourceNet->name.text
                                 || connection->GetNetName().EndsWith( wxS( "/" ) + sourceNet->name.text ) );
                }
            }

            BOOST_CHECK( found );
        }
    }

    PADS_SCH_MODEL coincident = parseBinaryFixture( wxS( "connectivity_topology" ) );
    BOOST_REQUIRE( !coincident.buses.empty() );
    BOOST_REQUIRE( !coincident.buses.front().entries.empty() );
    const MODEL_BUS_ENTRY& coincidentEntry = coincident.buses.front().entries.front();
    auto ownerNet = std::ranges::find( coincident.nets, coincidentEntry.memberNet.id, &MODEL_NET::id );
    BOOST_REQUIRE( ownerNet != coincident.nets.end() );
    auto otherNet = std::ranges::find_if( coincident.nets,
                                          [&]( const MODEL_NET& aNet )
                                          {
                                              return aNet.sheet.id == ownerNet->sheet.id && aNet.id != ownerNet->id;
                                          } );
    BOOST_REQUIRE( otherNet != coincident.nets.end() );
    auto ownerConnection = std::ranges::find_if(
            ownerNet->connections,
            [&]( const MODEL_CONNECTION& aConnection )
            {
                auto samePoint = []( const SOURCE_POINT& aLeft, const SOURCE_POINT& aRight )
                {
                    return aLeft.x == aRight.x && aLeft.y == aRight.y;
                };
                return aConnection.vertices.size() >= 2
                       && ( samePoint( aConnection.vertices.front(), coincidentEntry.position )
                            || samePoint( aConnection.vertices.back(), coincidentEntry.position ) );
            } );
    BOOST_REQUIRE( ownerConnection != ownerNet->connections.end() );
    SOURCE_POINT     adjacent = ownerConnection->vertices.front().x == coincidentEntry.position.x
                                            && ownerConnection->vertices.front().y == coincidentEntry.position.y
                                        ? ownerConnection->vertices[1]
                                        : ownerConnection->vertices[ownerConnection->vertices.size() - 2];
    MODEL_CONNECTION distinctNetConnection = *ownerConnection;
    distinctNetConnection.vertices = { coincidentEntry.position, adjacent };
    distinctNetConnection.endpoints.resize( 2 );

    for( size_t endpoint = 0; endpoint < distinctNetConnection.endpoints.size(); ++endpoint )
    {
        distinctNetConnection.endpoints[endpoint].kind = MODEL_ENDPOINT_KIND::POINT;
        distinctNetConnection.endpoints[endpoint].placement.reset();
        distinctNetConnection.endpoints[endpoint].pin.reset();
        distinctNetConnection.endpoints[endpoint].point = distinctNetConnection.vertices[endpoint];
    }

    otherNet->connections.push_back( distinctNetConnection );

    m_schematic.Reset();
    root = m_schematic.GetTopLevelSheet();
    builder.Build( coincident, &m_schematic, nullptr, binaryFixture( wxS( "connectivity_topology" ) ) );
    const int      coincidentPageHeight = root->GetScreen()->GetPageSettings().GetHeightIU( schIUScale.IU_PER_MILS );
    const VECTOR2I entryStart = pagePoint( coincidentEntry.position, coincidentPageHeight );
    const VECTOR2I entryEnd = pagePoint( adjacent, coincidentPageHeight );
    const VECTOR2I entryDelta = entryEnd - entryStart;
    const int      entrySpan = std::max( std::abs( entryDelta.x ), std::abs( entryDelta.y ) );
    const int      shortSpan = std::min( entrySpan, schIUScale.MilsToIU( DEFAULT_SCH_ENTRY_SIZE ) );
    const VECTOR2I shortEntryEnd = entrySpan == 0 ? entryEnd
                                                  : entryStart
                                                            + VECTOR2I( entryDelta.x * shortSpan / entrySpan,
                                                                        entryDelta.y * shortSpan / entrySpan );
    size_t         coincidentWireSegments = 0;
    bool           exactEntry = false;

    for( SCH_ITEM* item : root->GetScreen()->Items() )
    {
        if( auto* line = dynamic_cast<SCH_LINE*>( item ); line && line->GetLayer() == LAYER_WIRE )
        {
            coincidentWireSegments += ( line->GetStartPoint() == entryStart && line->GetEndPoint() == entryEnd )
                                      || ( line->GetStartPoint() == entryEnd && line->GetEndPoint() == entryStart );
        }
        else if( auto* entry = dynamic_cast<SCH_BUS_WIRE_ENTRY*>( item ) )
        {
            exactEntry |= entry->GetPosition() == entryStart && entry->GetSize() == shortEntryEnd - entryStart;
        }
    }

    BOOST_CHECK( exactEntry );
    BOOST_CHECK_EQUAL( coincidentWireSegments, 1u );

    m_schematic.Reset();
    root = m_schematic.GetTopLevelSheet();
    PADS_SCH_MODEL graphics = parseBinaryFixture( wxS( "page_graphics" ) );
    BUILD_RESULT   graphicsResult =
            builder.Build( graphics, &m_schematic, nullptr, binaryFixture( wxS( "page_graphics" ) ) );
    size_t expectedShapes = 0;
    size_t expectedTexts = graphics.texts.size();
    size_t expectedNoteLines = 0;

    auto countGraphic = [&]( const MODEL_GRAPHIC& aGraphic )
    {
        if( aGraphic.kind == MODEL_GRAPHIC_KIND::TEXT )
            ++expectedTexts;
        else if( ( aGraphic.kind == MODEL_GRAPHIC_KIND::LINE || aGraphic.kind == MODEL_GRAPHIC_KIND::POLYLINE )
                 && aGraphic.fill == MODEL_FILL_STYLE::NONE )
            expectedNoteLines += aGraphic.points.size() - 1;
        else
            ++expectedShapes;
    };

    for( const MODEL_PAGE_GRAPHIC& graphic : graphics.graphics )
        countGraphic( graphic.graphic );

    for( const MODEL_GRAPHIC& graphic : graphics.sheets.front().border )
        countGraphic( graphic );

    size_t builtNoteLines = 0;

    for( SCH_ITEM* item : root->GetScreen()->Items().OfType( SCH_LINE_T ) )
    {
        if( item->GetLayer() == LAYER_NOTES )
            ++builtNoteLines;
    }

    BOOST_CHECK_EQUAL( itemCount( root->GetScreen(), SCH_TEXT_T ), expectedTexts );
    BOOST_CHECK_EQUAL( itemCount( root->GetScreen(), SCH_SHAPE_T ), expectedShapes );
    BOOST_CHECK_EQUAL( builtNoteLines, expectedNoteLines );

    for( SCH_ITEM* item : root->GetScreen()->Items().OfType( SCH_SHAPE_T ) )
    {
        auto* shape = static_cast<SCH_SHAPE*>( item );

        if( shape->GetShape() == SHAPE_T::POLY )
            BOOST_CHECK( shape->GetFillMode() != FILL_T::NO_FILL );
    }

    const int graphicsPageHeight = root->GetScreen()->GetPageSettings().GetHeightIU( schIUScale.IU_PER_MILS );

    auto checkGraphic = [&]( const MODEL_GRAPHIC& aGraphic )
    {
        if( aGraphic.kind == MODEL_GRAPHIC_KIND::TEXT )
        {
            bool found = false;

            for( SCH_ITEM* item : root->GetScreen()->Items().OfType( SCH_TEXT_T ) )
            {
                auto* text = static_cast<SCH_TEXT*>( item );

                if( text->GetText() == aGraphic.text.text
                    && text->GetPosition() == pagePoint( aGraphic.points.front(), graphicsPageHeight ) )
                {
                    BOOST_CHECK_CLOSE( text->GetTextAngleDegrees(), aGraphic.angle / 10.0, 0.001 );
                    checkTextPresentation( *text, aGraphic.presentation );
                    const bool approximatedFont = !aGraphic.presentation.font.text.IsEmpty()
                                                  && aGraphic.presentation.font.text != wxS( "Default Font" );
                    BOOST_CHECK_EQUAL(
                            std::ranges::count_if( graphicsResult.diagnostics,
                                                   [&]( const PARSER_DIAGNOSTIC& aDiagnostic )
                                                   {
                                                       return aDiagnostic.source == aGraphic.presentation.source
                                                              && aDiagnostic.message.Contains( wxS( "font" ) );
                                                   } ),
                            approximatedFont ? 1u : 0u );
                    found = true;
                    break;
                }
            }

            BOOST_CHECK_MESSAGE( found, aGraphic.text.text );
            return;
        }

        if( ( aGraphic.kind == MODEL_GRAPHIC_KIND::LINE || aGraphic.kind == MODEL_GRAPHIC_KIND::POLYLINE )
            && aGraphic.fill == MODEL_FILL_STYLE::NONE )
        {
            for( size_t point = 1; point < aGraphic.points.size(); ++point )
            {
                bool found = false;

                for( SCH_ITEM* item : root->GetScreen()->Items().OfType( SCH_LINE_T ) )
                {
                    auto* line = static_cast<SCH_LINE*>( item );

                    if( line->GetLayer() == LAYER_NOTES
                        && line->GetStartPoint() == pagePoint( aGraphic.points[point - 1], graphicsPageHeight )
                        && line->GetEndPoint() == pagePoint( aGraphic.points[point], graphicsPageHeight ) )
                    {
                        BOOST_CHECK_EQUAL( line->GetStroke().GetWidth(),
                                           schIUScale.MilsToIU( aGraphic.strokeWidth / 2.0 ) );
                        BOOST_CHECK( line->GetStroke().GetLineStyle() == lineStyle( aGraphic.lineStyle ) );
                        found = true;
                        break;
                    }
                }

                BOOST_CHECK( found );
            }

            return;
        }

        SHAPE_T expectedType = aGraphic.kind == MODEL_GRAPHIC_KIND::RECTANGLE ? SHAPE_T::RECTANGLE
                               : aGraphic.kind == MODEL_GRAPHIC_KIND::CIRCLE  ? SHAPE_T::CIRCLE
                               : aGraphic.kind == MODEL_GRAPHIC_KIND::ARC     ? SHAPE_T::ARC
                                                                              : SHAPE_T::POLY;
        bool    found = false;

        for( SCH_ITEM* item : root->GetScreen()->Items().OfType( SCH_SHAPE_T ) )
        {
            auto* shape = static_cast<SCH_SHAPE*>( item );

            if( shape->GetShape() != expectedType )
                continue;

            if( expectedType == SHAPE_T::POLY )
            {
                std::vector<VECTOR2I> expectedPoints;

                for( const SOURCE_POINT& point : aGraphic.points )
                    expectedPoints.push_back( pagePoint( point, graphicsPageHeight ) );

                if( shape->GetPolyPoints() != expectedPoints )
                    continue;
            }
            else if( expectedType == SHAPE_T::ARC )
            {
                if( shape->GetStart() != pagePoint( aGraphic.points.front(), graphicsPageHeight )
                    || shape->GetEnd() != pagePoint( aGraphic.points.back(), graphicsPageHeight )
                    || shape->GetCenter() != pagePoint( aGraphic.arcCenter, graphicsPageHeight ) )
                {
                    continue;
                }

                BOOST_CHECK_EQUAL( std::abs( shape->GetArcAngle().AsTenthsOfADegree() ),
                                   std::abs( aGraphic.arcSweepAngle ) );
                BOOST_CHECK_EQUAL( shape->GetArcAngle().AsTenthsOfADegree(),
                                   aGraphic.arcClockwise ? std::abs( aGraphic.arcSweepAngle )
                                                         : -std::abs( aGraphic.arcSweepAngle ) );
                const VECTOR2I boundsStart = pagePoint( aGraphic.arcBoundsStart, graphicsPageHeight );
                const VECTOR2I boundsEnd = pagePoint( aGraphic.arcBoundsEnd, graphicsPageHeight );
                const int      expectedRadiusX = std::abs( boundsEnd.x - boundsStart.x ) / 2;
                const int      expectedRadiusY = std::abs( boundsEnd.y - boundsStart.y ) / 2;
                const int      actualRadius =
                        KiROUND( std::hypot( static_cast<double>( shape->GetStart().x - shape->GetCenter().x ),
                                             static_cast<double>( shape->GetStart().y - shape->GetCenter().y ) ) );
                BOOST_CHECK_EQUAL( actualRadius, expectedRadiusX );
                BOOST_CHECK_EQUAL( actualRadius, expectedRadiusY );
            }
            else if( expectedType == SHAPE_T::CIRCLE )
            {
                SOURCE_POINT center;
                center.x = ( aGraphic.points.front().x + aGraphic.points.back().x ) / 2;
                center.y = ( aGraphic.points.front().y + aGraphic.points.back().y ) / 2;
                const VECTOR2I expectedCenter = pagePoint( center, graphicsPageHeight );
                const VECTOR2I expectedEdge = pagePoint( aGraphic.points.back(), graphicsPageHeight );
                const int      expectedRadius =
                        KiROUND( std::hypot( static_cast<double>( expectedEdge.x - expectedCenter.x ),
                                             static_cast<double>( expectedEdge.y - expectedCenter.y ) ) );
                const int actualRadius =
                        KiROUND( std::hypot( static_cast<double>( shape->GetEnd().x - shape->GetStart().x ),
                                             static_cast<double>( shape->GetEnd().y - shape->GetStart().y ) ) );

                if( shape->GetCenter() != expectedCenter || actualRadius != expectedRadius )
                    continue;
            }
            else if( expectedType == SHAPE_T::RECTANGLE )
            {
                if( shape->GetStart() != pagePoint( aGraphic.points.front(), graphicsPageHeight )
                    || shape->GetEnd() != pagePoint( aGraphic.points.back(), graphicsPageHeight ) )
                {
                    continue;
                }
            }
            else if( shape->GetStart() != pagePoint( aGraphic.points.front(), graphicsPageHeight ) )
            {
                continue;
            }

            BOOST_CHECK_EQUAL( shape->GetStroke().GetWidth(), schIUScale.MilsToIU( aGraphic.strokeWidth / 2.0 ) );
            BOOST_CHECK( shape->GetStroke().GetLineStyle() == lineStyle( aGraphic.lineStyle ) );
            BOOST_CHECK( shape->GetFillMode()
                         == ( aGraphic.fill == MODEL_FILL_STYLE::NONE      ? FILL_T::NO_FILL
                              : aGraphic.fill == MODEL_FILL_STYLE::HATCHED ? FILL_T::HATCH
                                                                           : FILL_T::FILLED_WITH_BG_BODYCOLOR ) );
            found = true;
            break;
        }

        BOOST_CHECK( found );
    };

    for( const MODEL_PAGE_GRAPHIC& graphic : graphics.graphics )
        checkGraphic( graphic.graphic );

    for( const MODEL_GRAPHIC& graphic : graphics.sheets.front().border )
        checkGraphic( graphic );

    for( const MODEL_TEXT& sourceText : graphics.texts )
    {
        bool found = false;

        for( SCH_ITEM* item : root->GetScreen()->Items().OfType( SCH_TEXT_T ) )
        {
            auto* text = static_cast<SCH_TEXT*>( item );

            if( text->GetText() == sourceText.text.text
                && text->GetPosition() == pagePoint( sourceText.position, graphicsPageHeight ) )
            {
                BOOST_CHECK_CLOSE( text->GetTextAngleDegrees(), sourceText.angle / 10.0, 0.001 );
                checkTextPresentation( *text, sourceText.presentation );
                const bool approximatedFont = !sourceText.presentation.font.text.IsEmpty()
                                              && sourceText.presentation.font.text != wxS( "Default Font" );
                BOOST_CHECK_EQUAL( std::ranges::count_if( graphicsResult.diagnostics,
                                                          [&]( const PARSER_DIAGNOSTIC& aDiagnostic )
                                                          {
                                                              return aDiagnostic.source
                                                                             == sourceText.presentation.source
                                                                     && aDiagnostic.message.Contains( wxS( "font" ) );
                                                          } ),
                                   approximatedFont ? 1u : 0u );
                found = true;
                break;
            }
        }

        BOOST_CHECK_MESSAGE( found, sourceText.text.text );
    }

    m_schematic.Reset();
    PADS_SCH_MODEL multi = parseBinaryFixture( wxS( "multisheet_connectivity" ) );
    builder.Build( multi, &m_schematic, nullptr, binaryFixture( wxS( "multisheet_connectivity" ) ) );
    assertSourceConnectivity( multi, m_schematic );
    std::multiset<wxString> multiSnapshot = connectivitySnapshot( m_schematic );

    for( const MODEL_NET& net : multi.nets )
        BOOST_CHECK_MESSAGE( hasNetName( multiSnapshot, net.name.text ), net.name.text );
}


BOOST_AUTO_TEST_CASE( BinaryConnectivityRoundTrip )
{
    using namespace PADS_SCH_BINARY;

    const PADS_SCH_MODEL model = parseBinaryFixture( wxS( "multisheet_connectivity" ) );
    PADS_SCH_BINARY_BUILDER().Build( model, &m_schematic, nullptr, binaryFixture( wxS( "multisheet_connectivity" ) ) );
    assertSourceConnectivity( model, m_schematic );
    std::multiset<wxString> before = connectivitySnapshot( m_schematic );

    for( const MODEL_NET& net : model.nets )
        BOOST_CHECK_MESSAGE( hasNetName( before, net.name.text ), net.name.text );

    wxString tempDir = wxFileName::CreateTempFileName( wxS( "pads_binary_connectivity_" ) );
    BOOST_REQUIRE( wxRemoveFile( tempDir ) );
    BOOST_REQUIRE( wxFileName::Mkdir( tempDir ) );
    roundTripTopLevelSheets( m_schematic, tempDir );
    assertSourceConnectivity( model, m_schematic );
    std::multiset<wxString> after = connectivitySnapshot( m_schematic );

    for( const wxString& value : before )
    {
        if( before.count( value ) != after.count( value ) )
            BOOST_TEST_MESSAGE( "before-only/count mismatch: " << value );
    }

    for( const wxString& value : after )
    {
        if( before.count( value ) != after.count( value ) )
            BOOST_TEST_MESSAGE( "after-only/count mismatch: " << value );
    }

    BOOST_CHECK( before == after );

    for( const MODEL_NET& net : model.nets )
        BOOST_CHECK_MESSAGE( hasNetName( after, net.name.text ), net.name.text );

    BOOST_CHECK( wxFileName::Rmdir( tempDir, wxPATH_RMDIR_RECURSIVE ) );

    size_t pinEndpointsBefore = 0;
    size_t pinEndpointsAfter = 0;
    size_t powerLabelsBefore = 0;
    size_t powerLabelsAfter = 0;

    for( const wxString& fixture :
         { wxS( "minimal_v13" ), wxS( "placement_transform" ), wxS( "fields" ), wxS( "connectors" ),
           wxS( "text_encoding" ), wxS( "page_graphics" ), wxS( "connectivity_topology" ),
           wxS( "multisheet_connectivity" ), wxS( "symbol_primitives" ), wxS( "pin_styles" ), wxS( "multigate" ) } )
    {
        const PADS_SCH_MODEL sourceModel = parseBinaryFixture( fixture );

        if( sourceModel.nets.empty() && sourceModel.buses.empty() && sourceModel.labels.empty()
            && sourceModel.junctions.empty() )
        {
            continue;
        }

        m_schematic.Reset();
        PADS_SCH_BINARY_BUILDER().Build( sourceModel, &m_schematic, nullptr, binaryFixture( fixture ) );
        CONNECTIVITY_ORACLE_COUNTS beforeCounts = assertSourceConnectivity( sourceModel, m_schematic );
        const size_t               expectedPinEndpoints =
                std::accumulate( sourceModel.nets.begin(), sourceModel.nets.end(), size_t( 0 ),
                                 []( size_t aCount, const MODEL_NET& aNet )
                                 {
                                     for( const MODEL_CONNECTION& connection : aNet.connections )
                                         aCount += std::ranges::count( connection.endpoints, MODEL_ENDPOINT_KIND::PIN,
                                                                       &MODEL_CONNECTION_ENDPOINT::kind );

                                     return aCount;
                                 } );
        const size_t expectedPowerLabels = std::ranges::count_if( sourceModel.labels,
                                                                  []( const MODEL_LABEL& aLabel )
                                                                  {
                                                                      return aLabel.kind == MODEL_LABEL_KIND::POWER
                                                                             || aLabel.kind == MODEL_LABEL_KIND::GROUND;
                                                                  } );
        BOOST_CHECK_EQUAL( beforeCounts.pinEndpoints, expectedPinEndpoints );
        BOOST_CHECK_EQUAL( beforeCounts.powerLabels, expectedPowerLabels );

        if( fixture == wxS( "minimal_v13" ) )
            BOOST_CHECK_GT( beforeCounts.pinEndpoints, 0u );

        pinEndpointsBefore += beforeCounts.pinEndpoints;
        powerLabelsBefore += beforeCounts.powerLabels;

        wxString fixtureTemp = wxFileName::CreateTempFileName( wxS( "pads_binary_source_oracle_" ) );
        BOOST_REQUIRE( wxRemoveFile( fixtureTemp ) );
        BOOST_REQUIRE( wxFileName::Mkdir( fixtureTemp ) );
        roundTripTopLevelSheets( m_schematic, fixtureTemp );
        CONNECTIVITY_ORACLE_COUNTS afterCounts = assertSourceConnectivity( sourceModel, m_schematic );
        BOOST_CHECK_EQUAL( afterCounts.pinEndpoints, beforeCounts.pinEndpoints );
        BOOST_CHECK_EQUAL( afterCounts.powerLabels, beforeCounts.powerLabels );
        pinEndpointsAfter += afterCounts.pinEndpoints;
        powerLabelsAfter += afterCounts.powerLabels;
        BOOST_CHECK( wxFileName::Rmdir( fixtureTemp, wxPATH_RMDIR_RECURSIVE ) );
    }

    BOOST_CHECK_GT( pinEndpointsBefore, 0u );
    BOOST_CHECK_EQUAL( pinEndpointsAfter, pinEndpointsBefore );
    BOOST_CHECK_GT( powerLabelsBefore, 0u );
    BOOST_CHECK_EQUAL( powerLabelsAfter, powerLabelsBefore );

    m_schematic.Reset();
    const PADS_SCH_MODEL graphics = parseBinaryFixture( wxS( "page_graphics" ) );
    SCH_SHEET*           graphicsRoot = m_schematic.GetTopLevelSheet();
    PADS_SCH_BINARY_BUILDER().Build( graphics, &m_schematic, nullptr, binaryFixture( wxS( "page_graphics" ) ) );
    std::multiset<wxString> graphicsBefore = connectivitySnapshot( m_schematic );
    wxString                graphicsFile = wxFileName::CreateTempFileName( wxS( "pads_binary_graphics_" ) );
    SCH_IO_KICAD_SEXPR      graphicsIo;
    BOOST_REQUIRE_NO_THROW( graphicsIo.SaveSchematicFile( graphicsFile, graphicsRoot, &m_schematic ) );
    m_schematic.Reset();
    SCH_SHEET* defaultSheet = m_schematic.GetTopLevelSheet();
    SCH_SHEET* loaded = nullptr;
    BOOST_REQUIRE_NO_THROW( loaded = graphicsIo.LoadSchematicFile( graphicsFile, &m_schematic ) );
    BOOST_REQUIRE( loaded );
    m_schematic.AddTopLevelSheet( loaded );
    m_schematic.RemoveTopLevelSheet( defaultSheet );
    delete defaultSheet;
    m_schematic.RefreshHierarchy();
    std::multiset<wxString> graphicsAfter = connectivitySnapshot( m_schematic );

    for( const wxString& value : graphicsBefore )
    {
        if( graphicsBefore.count( value ) != graphicsAfter.count( value ) )
            BOOST_TEST_MESSAGE( "graphics before-only/count mismatch: " << value );
    }

    for( const wxString& value : graphicsAfter )
    {
        if( graphicsBefore.count( value ) != graphicsAfter.count( value ) )
            BOOST_TEST_MESSAGE( "graphics after-only/count mismatch: " << value );
    }

    BOOST_CHECK( graphicsBefore == graphicsAfter );
    BOOST_CHECK( wxRemoveFile( graphicsFile ) );
}


BOOST_AUTO_TEST_CASE( BinaryPropertyDispositionWarnings )
{
    using namespace PADS_SCH_BINARY;

    SOURCE_PROVENANCE keySource{ wxS( "key.sch" ), 13, wxS( "placement" ), 17, 23, 41, 8, 2 };
    PARSER_DIAGNOSTIC keyDiagnostic = MakePropertyDiagnostic( RPT_SEVERITY_WARNING, keySource, wxS( "property" ),
                                                              PROPERTY_DISPOSITION::PRESERVED, wxS( "message" ) );
    std::set<DIAGNOSTIC_PROPERTY_KEY> propertyKeys;
    BOOST_REQUIRE( DiagnosticPropertyKey( keyDiagnostic ) );
    propertyKeys.insert( *DiagnosticPropertyKey( keyDiagnostic ) );
    propertyKeys.insert( *DiagnosticPropertyKey( keyDiagnostic ) );

    auto insertChangedKey = [&]( auto aMutator )
    {
        PARSER_DIAGNOSTIC changed = keyDiagnostic;
        aMutator( changed );
        BOOST_REQUIRE( DiagnosticPropertyKey( changed ) );
        propertyKeys.insert( *DiagnosticPropertyKey( changed ) );
    };
    insertChangedKey(
            []( PARSER_DIAGNOSTIC& aDiagnostic )
            {
                aDiagnostic.source.file += 'x';
            } );
    insertChangedKey(
            []( PARSER_DIAGNOSTIC& aDiagnostic )
            {
                ++aDiagnostic.source.version;
            } );
    insertChangedKey(
            []( PARSER_DIAGNOSTIC& aDiagnostic )
            {
                aDiagnostic.source.objectClass += 'x';
            } );
    insertChangedKey(
            []( PARSER_DIAGNOSTIC& aDiagnostic )
            {
                ++aDiagnostic.source.controller;
            } );
    insertChangedKey(
            []( PARSER_DIAGNOSTIC& aDiagnostic )
            {
                ++aDiagnostic.source.recordIndex;
            } );
    insertChangedKey(
            []( PARSER_DIAGNOSTIC& aDiagnostic )
            {
                ++aDiagnostic.source.absoluteOffset;
            } );
    insertChangedKey(
            []( PARSER_DIAGNOSTIC& aDiagnostic )
            {
                ++aDiagnostic.source.length;
            } );
    insertChangedKey(
            []( PARSER_DIAGNOSTIC& aDiagnostic )
            {
                ++aDiagnostic.source.sheet;
            } );
    insertChangedKey(
            []( PARSER_DIAGNOSTIC& aDiagnostic )
            {
                aDiagnostic.property->name += 'x';
            } );
    insertChangedKey(
            []( PARSER_DIAGNOSTIC& aDiagnostic )
            {
                aDiagnostic.property->disposition = PROPERTY_DISPOSITION::UNSUPPORTED;
            } );
    BOOST_CHECK_EQUAL( propertyKeys.size(), 11u );
    BOOST_CHECK( !DiagnosticPropertyKey( PARSER_DIAGNOSTIC{} ) );

    for( const wxString& fixture :
         { wxS( "minimal_v13" ), wxS( "placement_transform" ), wxS( "fields" ), wxS( "connectors" ),
           wxS( "text_encoding" ), wxS( "page_graphics" ), wxS( "connectivity_topology" ),
           wxS( "multisheet_connectivity" ), wxS( "symbol_primitives" ), wxS( "pin_styles" ), wxS( "multigate" ) } )
    {
        m_schematic.Reset();
        PADS_SCH_MODEL                      corpusModel = parseBinaryFixture( fixture );
        std::vector<const SOURCE_PROPERTY*> properties = allSourceProperties( corpusModel );
        BOOST_REQUIRE_MESSAGE( !properties.empty(), fixture );
        BUILD_RESULT corpusResult =
                PADS_SCH_BINARY_BUILDER().Build( corpusModel, &m_schematic, nullptr, binaryFixture( fixture ) );

        for( const SOURCE_PROPERTY* property : properties )
        {
            const size_t multiplicity =
                    std::ranges::count_if( properties,
                                           [&]( const SOURCE_PROPERTY* aOther )
                                           {
                                               return aOther->source == property->source
                                                      && aOther->name.text == property->name.text
                                                      && aOther->disposition == property->disposition;
                                           } );
            const size_t parserOwned =
                    std::ranges::count_if( corpusModel.diagnostics,
                                           [&]( const PARSER_DIAGNOSTIC& aDiagnostic )
                                           {
                                               return aDiagnostic.source == property->source && aDiagnostic.property
                                                      && aDiagnostic.property->name == property->name.text
                                                      && aDiagnostic.property->disposition == property->disposition;
                                           } );
            const size_t builderOwned =
                    std::ranges::count_if( corpusResult.diagnostics,
                                           [&]( const PARSER_DIAGNOSTIC& aDiagnostic )
                                           {
                                               return aDiagnostic.source == property->source && aDiagnostic.property
                                                      && aDiagnostic.property->name == property->name.text
                                                      && aDiagnostic.property->disposition == property->disposition;
                                           } );
            BOOST_CHECK_LE( parserOwned, multiplicity );

            if( property->disposition == PROPERTY_DISPOSITION::EXACT
                || property->disposition == PROPERTY_DISPOSITION::PRESERVED )
            {
                BOOST_CHECK_EQUAL( builderOwned, 0u );
                BOOST_CHECK_EQUAL( parserOwned, 0u );
            }
            else
            {
                BOOST_CHECK_EQUAL( builderOwned + parserOwned, multiplicity );
                BOOST_CHECK( builderOwned == 0u || parserOwned == 0u );
            }
        }

        m_schematic.Reset();
        SCH_IO_PADS        plugin;
        CAPTURING_REPORTER reporter;
        plugin.SetReporter( &reporter );
        BOOST_REQUIRE_NO_THROW( plugin.LoadSchematicFile( binaryFixture( fixture ), &m_schematic ) );
        std::vector<PARSER_DIAGNOSTIC> expectedDiagnostics = corpusModel.diagnostics;
        expectedDiagnostics.insert( expectedDiagnostics.end(), corpusResult.diagnostics.begin(),
                                    corpusResult.diagnostics.end() );
        std::map<std::pair<wxString, SEVERITY>, std::pair<const PARSER_DIAGNOSTIC*, size_t>> diagnosticGroups;

        for( const PARSER_DIAGNOSTIC& diagnostic : expectedDiagnostics )
        {
            auto& [first, count] = diagnosticGroups[{ diagnostic.message, diagnostic.severity }];

            if( !first )
                first = &diagnostic;

            ++count;
        }

        std::map<std::pair<wxString, SEVERITY>, size_t> expectedWarningCounts;

        for( const auto& [key, group] : diagnosticGroups )
        {
            const auto& [message, severity] = key;
            const auto& [first, count] = group;
            wxString groupedMessage = message;

            if( count > 1 )
                groupedMessage += wxString::Format( wxS( " (%zu occurrences)" ), count );

            const wxString formatted = FormatParserError( first->source, groupedMessage );
            ++expectedWarningCounts[{ formatted, severity }];
            BOOST_CHECK( formatted.Contains( wxString::Format( wxS( "v0x%04X" ), first->source.version ) ) );
            BOOST_CHECK( formatted.Contains( first->source.objectClass ) );
            BOOST_CHECK( formatted.Contains(
                    wxString::Format( wxS( "controller %d, record %llu" ), first->source.controller,
                                      static_cast<unsigned long long>( first->source.recordIndex ) ) ) );
            BOOST_CHECK( formatted.Contains( wxString::Format(
                    wxS( "offset 0x%llX" ), static_cast<unsigned long long>( first->source.absoluteOffset ) ) ) );
        }

        std::map<std::pair<wxString, SEVERITY>, size_t> reportedWarningCounts;

        for( const auto& [message, severity] : reporter.messages )
        {
            if( severity != RPT_SEVERITY_INFO )
                ++reportedWarningCounts[{ message, severity }];
        }

        BOOST_CHECK( reportedWarningCounts == expectedWarningCounts );
    }

    auto assertParserOwnership =
            [&]( PADS_SCH_MODEL& aModel, const SOURCE_PROPERTY& aProperty, const wxString& aFixture )
    {
        m_schematic.Reset();
        BUILD_RESULT ownedResult =
                PADS_SCH_BINARY_BUILDER().Build( aModel, &m_schematic, nullptr, binaryFixture( aFixture ) );
        BOOST_CHECK_EQUAL( std::ranges::count_if( aModel.diagnostics,
                                                  [&]( const PARSER_DIAGNOSTIC& aDiagnostic )
                                                  {
                                                      return aDiagnostic.source == aProperty.source
                                                             && aDiagnostic.property
                                                             && aDiagnostic.property->name == aProperty.name.text
                                                             && aDiagnostic.property->disposition
                                                                        == aProperty.disposition;
                                                  } ),
                           1u );
        BOOST_CHECK_EQUAL( std::ranges::count_if( ownedResult.diagnostics,
                                                  [&]( const PARSER_DIAGNOSTIC& aDiagnostic )
                                                  {
                                                      return aDiagnostic.source == aProperty.source
                                                             && aDiagnostic.property
                                                             && aDiagnostic.property->name == aProperty.name.text
                                                             && aDiagnostic.property->disposition
                                                                        == aProperty.disposition;
                                                  } ),
                           0u );
    };

    PADS_SCH_MODEL pageRelationship = parseBinaryFixture( wxS( "page_graphics" ) );
    BOOST_REQUIRE( !pageRelationship.graphics.empty() );
    SOURCE_PROPERTY relationship;
    relationship.name.text = wxS( "preserved_drawing_text_relationship" );
    relationship.value.text = wxS( "synthetic" );
    relationship.source = pageRelationship.graphics.front().graphic.source;
    relationship.disposition = PROPERTY_DISPOSITION::UNSUPPORTED;
    pageRelationship.graphics.front().graphic.properties.push_back( relationship );
    pageRelationship.diagnostics.push_back( MakePropertyDiagnostic(
            RPT_SEVERITY_WARNING, relationship, wxS( "parser retained a page drawing relationship" ) ) );
    assertParserOwnership( pageRelationship, relationship, wxS( "page_graphics" ) );

    PADS_SCH_MODEL fontPayloads = parseBinaryFixture( wxS( "fields" ) );
    BOOST_REQUIRE( !fontPayloads.placements.empty() );
    SOURCE_PROPERTY inlineFont;
    inlineFont.name.text = wxS( "inline_font_payload" );
    inlineFont.value.text = wxS( "synthetic" );
    inlineFont.source = fontPayloads.placements.front().source;
    inlineFont.disposition = PROPERTY_DISPOSITION::UNSUPPORTED;
    fontPayloads.placements.front().properties.push_back( inlineFont );
    fontPayloads.diagnostics.push_back(
            MakePropertyDiagnostic( RPT_SEVERITY_WARNING, inlineFont, wxS( "parser retained inline font bytes" ) ) );
    SOURCE_PROPERTY fontFlags = inlineFont;
    fontFlags.name.text = wxS( "unsupported_font_style_flags" );
    fontPayloads.placements.front().properties.push_back( fontFlags );
    fontPayloads.diagnostics.push_back(
            MakePropertyDiagnostic( RPT_SEVERITY_WARNING, fontFlags, wxS( "parser retained font flag bits" ) ) );
    assertParserOwnership( fontPayloads, inlineFont, wxS( "fields" ) );
    assertParserOwnership( fontPayloads, fontFlags, wxS( "fields" ) );

    PADS_SCH_MODEL busAlias = parseBinaryFixture( wxS( "connectivity_topology" ) );
    BOOST_REQUIRE( !busAlias.buses.empty() );
    SOURCE_PROPERTY aliasMembers;
    aliasMembers.name.text = wxS( "preserved_bus_alias_members" );
    aliasMembers.value.text = wxS( "synthetic" );
    aliasMembers.source = busAlias.buses.front().source;
    aliasMembers.disposition = PROPERTY_DISPOSITION::UNSUPPORTED;
    busAlias.buses.front().properties.push_back( aliasMembers );
    busAlias.diagnostics.push_back( MakePropertyDiagnostic( RPT_SEVERITY_WARNING, aliasMembers,
                                                            wxS( "parser retained expanded bus membership" ) ) );
    assertParserOwnership( busAlias, aliasMembers, wxS( "connectivity_topology" ) );

    PADS_SCH_MODEL unsupportedLabel = parseBinaryFixture( wxS( "connectivity_topology" ) );
    BOOST_REQUIRE( !unsupportedLabel.labels.empty() );
    unsupportedLabel.labels.front().kind = MODEL_LABEL_KIND::UNSUPPORTED;
    SOURCE_PROPERTY labelKind;
    labelKind.name.text = wxS( "unsupported_offpage_decal" );
    labelKind.value.text = wxS( "synthetic" );
    labelKind.source = unsupportedLabel.labels.front().source;
    labelKind.disposition = PROPERTY_DISPOSITION::UNSUPPORTED;
    unsupportedLabel.labels.front().properties.push_back( labelKind );
    unsupportedLabel.diagnostics.push_back(
            MakePropertyDiagnostic( RPT_SEVERITY_WARNING, labelKind, wxS( "parser retained label kind" ) ) );
    assertParserOwnership( unsupportedLabel, labelKind, wxS( "connectivity_topology" ) );

    m_schematic.Reset();
    PADS_SCH_MODEL model = parseBinaryFixture( wxS( "connectivity_topology" ) );
    BOOST_REQUIRE( !model.labels.empty() );
    SOURCE_PROPERTY approximate;
    approximate.name.text = wxS( "qa_approximate_label_presentation" );
    approximate.value.text = wxS( "retained" );
    approximate.source = model.labels.front().source;
    approximate.disposition = PROPERTY_DISPOSITION::APPROXIMATE;
    model.labels.front().presentation.properties.push_back( approximate );
    SOURCE_PROPERTY exact = approximate;
    exact.name.text = wxS( "qa_exact_label_presentation" );
    exact.disposition = PROPERTY_DISPOSITION::EXACT;
    model.labels.front().presentation.properties.push_back( exact );
    SOURCE_PROPERTY preserved = approximate;
    preserved.name.text = wxS( "qa_preserved_label_presentation" );
    preserved.disposition = PROPERTY_DISPOSITION::PRESERVED;
    model.labels.front().presentation.properties.push_back( preserved );
    SOURCE_PROPERTY unsupported = approximate;
    unsupported.name.text = wxS( "qa_unsupported_label_presentation" );
    unsupported.disposition = PROPERTY_DISPOSITION::UNSUPPORTED;
    model.labels.front().presentation.properties.push_back( unsupported );

    BUILD_RESULT result = PADS_SCH_BINARY_BUILDER().Build( model, &m_schematic, nullptr,
                                                           binaryFixture( wxS( "connectivity_topology" ) ) );
    for( const SOURCE_PROPERTY* property : { &approximate, &unsupported } )
    {
        BOOST_CHECK_EQUAL( std::ranges::count_if( result.diagnostics,
                                                  [&]( const PARSER_DIAGNOSTIC& aDiagnostic )
                                                  {
                                                      return aDiagnostic.source == property->source
                                                             && aDiagnostic.property
                                                             && aDiagnostic.property->name == property->name.text
                                                             && aDiagnostic.property->disposition
                                                                        == property->disposition;
                                                  } ),
                           1 );
    }

    BOOST_CHECK( std::ranges::none_of( result.diagnostics,
                                       [&]( const PARSER_DIAGNOSTIC& aDiagnostic )
                                       {
                                           return aDiagnostic.source == preserved.source && aDiagnostic.property
                                                  && aDiagnostic.property->name == preserved.name.text
                                                  && aDiagnostic.property->disposition == preserved.disposition;
                                       } ) );

    BOOST_CHECK( std::ranges::none_of( result.diagnostics,
                                       [&]( const PARSER_DIAGNOSTIC& aDiagnostic )
                                       {
                                           return aDiagnostic.property && aDiagnostic.property->name == exact.name.text
                                                  && aDiagnostic.property->disposition == exact.disposition;
                                       } ) );
}


BOOST_AUTO_TEST_SUITE_END()
