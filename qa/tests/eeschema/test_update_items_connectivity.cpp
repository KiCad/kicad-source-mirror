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
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <qa_utils/wx_utils/unit_test_utils.h>

#include <connection_graph.h>
#include <lib_symbol.h>
#include <pin_type.h>

#include <sch_bus_entry.h>
#include <sch_line.h>
#include <sch_pin.h>
#include <sch_symbol.h>
#include <sch_sheet.h>
#include <sch_sheet_path.h>
#include <sch_screen.h>
#include <schematic.h>

#include <algorithm>
#include <map>
#include <set>

void boost_test_update_symbol_connectivity()
{
    // Create schematic
    SCHEMATIC schematic( nullptr );
    
    // Create root sheet and screen
    SCH_SCREEN* screen = new SCH_SCREEN( nullptr );
    SCH_SHEET*  sheet = new SCH_SHEET( nullptr, VECTOR2I( 0, 0 ), VECTOR2I( 1000, 1000 ) );
    sheet->SetScreen( screen );
    sheet->SetParent( &schematic );

    SCH_SHEET_PATH sheetPath;
    sheetPath.push_back( sheet );

    // Build library symbol
    LIB_SYMBOL lib( "TEST", nullptr );
    lib.SetGlobalPower();
    lib.SetDuplicatePinNumbersAreJumpers( true );
    lib.JumperPinGroups().push_back( { wxString( "3" ), wxString( "4" ) } );

    auto make_pin = [&]( const wxString& num, const wxString& name, ELECTRICAL_PINTYPE type, const VECTOR2I& pos )
    {
        SCH_PIN* pin = new SCH_PIN( &lib );
        pin->SetNumber( num );
        pin->SetName( name );
        pin->SetType( type );
        pin->SetPosition( pos );
        lib.AddDrawItem( pin );
    };

    make_pin( "1", "VCC", ELECTRICAL_PINTYPE::PT_POWER_IN, VECTOR2I( 0, 0 ) );
    make_pin( "2", "A", ELECTRICAL_PINTYPE::PT_INPUT, VECTOR2I( 1000, 0 ) );
    make_pin( "2", "B", ELECTRICAL_PINTYPE::PT_INPUT, VECTOR2I( 2000, 0 ) );
    make_pin( "3", "C", ELECTRICAL_PINTYPE::PT_INPUT, VECTOR2I( 3000, 0 ) );
    make_pin( "4", "D", ELECTRICAL_PINTYPE::PT_INPUT, VECTOR2I( 4000, 0 ) );

    // Create schematic symbol instance
    SCH_SYMBOL symbol( lib, lib.GetLibId(), &sheetPath, 0, 0, VECTOR2I( 0, 0 ) );
    symbol.SetValueFieldText( "VCC" );
    symbol.UpdatePins();

    std::vector<SCH_PIN*> pins = symbol.GetPins( &sheetPath );
    SCH_PIN*              powerPin = nullptr;
    SCH_PIN*              dupA = nullptr;
    SCH_PIN*              dupB = nullptr;
    SCH_PIN*              jumpC = nullptr;
    SCH_PIN*              jumpD = nullptr;

    for( SCH_PIN* pin : pins )
    {
        if( pin->GetNumber() == "1" )
            powerPin = pin;
        else if( pin->GetNumber() == "2" )
        {
            if( !dupA )
                dupA = pin;
            else
                dupB = pin;
        }
        else if( pin->GetNumber() == "3" )
            jumpC = pin;
        else if( pin->GetNumber() == "4" )
            jumpD = pin;
    }

    CONNECTION_GRAPH                           graph;
    std::map<VECTOR2I, std::vector<SCH_ITEM*>> connection_map;
    graph.updateSymbolConnectivity( sheetPath, &symbol, connection_map );

    // Global power pin captured
    BOOST_REQUIRE_EQUAL( graph.m_global_power_pins.size(), 1 );
    BOOST_CHECK( graph.m_global_power_pins[0].second == powerPin );
    BOOST_CHECK_EQUAL( powerPin->Connection( &sheetPath )->Name( true ), "VCC" );

    // Duplicate pin numbers link together
    const SCH_ITEM_VEC& dupAConn = dupA->ConnectedItems( sheetPath );
    BOOST_CHECK( std::find( dupAConn.begin(), dupAConn.end(), dupB ) != dupAConn.end() );
    const SCH_ITEM_VEC& dupBConn = dupB->ConnectedItems( sheetPath );
    BOOST_CHECK( std::find( dupBConn.begin(), dupBConn.end(), dupA ) != dupBConn.end() );

    // Jumper group links pins
    const SCH_ITEM_VEC& jumpCConn = jumpC->ConnectedItems( sheetPath );
    BOOST_CHECK( std::find( jumpCConn.begin(), jumpCConn.end(), jumpD ) != jumpCConn.end() );
    const SCH_ITEM_VEC& jumpDConn = jumpD->ConnectedItems( sheetPath );
    BOOST_CHECK( std::find( jumpDConn.begin(), jumpDConn.end(), jumpC ) != jumpDConn.end() );

    // Connection map contains all pins
    std::set<SCH_PIN*> mapPins;
    for( const auto& [pos, items] : connection_map )
        for( SCH_ITEM* item : items )
            mapPins.insert( static_cast<SCH_PIN*>( item ) );
    BOOST_CHECK_EQUAL( mapPins.size(), 5 );

    // Item list contains all pins
    BOOST_CHECK_EQUAL( graph.m_items.size(), 5 );
    for( SCH_PIN* pin : { powerPin, dupA, dupB, jumpC, jumpD } )
    {
        BOOST_CHECK( std::find( graph.m_items.begin(), graph.m_items.end(), pin )
                     != graph.m_items.end() );
    }

    delete sheet; // deletes screen
}


void boost_test_update_generic_connectivity()
{
    std::map<VECTOR2I, std::vector<SCH_ITEM*>> cmap;

    // Create schematic
    SCHEMATIC schematic( nullptr );
    
    // Create root sheet and screen
    SCH_SCREEN* screen = new SCH_SCREEN( nullptr );
    SCH_SHEET*  sheet = new SCH_SHEET( nullptr, VECTOR2I( 0, 0 ), VECTOR2I( 1000, 1000 ) );
    sheet->SetScreen( screen );
    sheet->SetParent( &schematic );

    SCH_SHEET_PATH sheetPath;
    sheetPath.push_back( sheet );

    CONNECTION_GRAPH graph;

    // Wire line
    SCH_LINE wire( VECTOR2I( 0, 0 ), LAYER_WIRE );
    wire.SetEndPoint( VECTOR2I( 100, 0 ) );

    graph.updateGenericItemConnectivity( sheetPath, &wire, cmap );
    SCH_CONNECTION* wireConn = wire.Connection( &sheetPath );
    BOOST_REQUIRE( wireConn );
    BOOST_CHECK( wireConn->Type() == CONNECTION_TYPE::NET );
    BOOST_CHECK( cmap.count( wire.GetStartPoint() ) );
    BOOST_CHECK( cmap.count( wire.GetEndPoint() ) );

    // Bus line
    cmap.clear();
    SCH_LINE bus( VECTOR2I( 0, 0 ), LAYER_BUS );
    bus.SetEndPoint( VECTOR2I( 0, 100 ) );

    graph.updateGenericItemConnectivity( sheetPath, &bus, cmap );
    SCH_CONNECTION* busConn = bus.Connection( &sheetPath );
    BOOST_REQUIRE( busConn );
    BOOST_CHECK( busConn->Type() == CONNECTION_TYPE::BUS );
    BOOST_CHECK( cmap.count( bus.GetStartPoint() ) );
    BOOST_CHECK( cmap.count( bus.GetEndPoint() ) );

    // Bus-to-bus entry
    cmap.clear();
    SCH_BUS_BUS_ENTRY busEntry( VECTOR2I( 0, 0 ), false );
    SCH_LINE          dummy1( VECTOR2I( 0, 0 ), LAYER_BUS );
    SCH_LINE          dummy2( VECTOR2I( 0, 0 ), LAYER_BUS );
    busEntry.m_connected_bus_items[0] = &dummy1;
    busEntry.m_connected_bus_items[1] = &dummy2;

    graph.updateGenericItemConnectivity( sheetPath, &busEntry, cmap );
    SCH_CONNECTION* bbConn = busEntry.Connection( &sheetPath );
    BOOST_REQUIRE( bbConn );
    BOOST_CHECK( bbConn->Type() == CONNECTION_TYPE::BUS );
    BOOST_CHECK( busEntry.m_connected_bus_items[0] == nullptr );
    BOOST_CHECK( busEntry.m_connected_bus_items[1] == nullptr );
    auto ptsBB = busEntry.GetConnectionPoints();
    BOOST_CHECK( cmap.count( ptsBB[0] ) );
    BOOST_CHECK( cmap.count( ptsBB[1] ) );

    // Bus-wire entry
    cmap.clear();
    SCH_BUS_WIRE_ENTRY bwEntry( VECTOR2I( 0, 0 ), false );
    SCH_LINE           dummyBus( VECTOR2I( 0, 0 ), LAYER_BUS );
    bwEntry.m_connected_bus_item = &dummyBus;

    graph.updateGenericItemConnectivity( sheetPath, &bwEntry, cmap );
    SCH_CONNECTION* bwConn = bwEntry.Connection( &sheetPath );
    BOOST_REQUIRE( bwConn );
    BOOST_CHECK( bwConn->Type() == CONNECTION_TYPE::NET );
    BOOST_CHECK( bwEntry.m_connected_bus_item == nullptr );
    auto ptsBW = bwEntry.GetConnectionPoints();
    BOOST_CHECK( cmap.count( ptsBW[0] ) );
    BOOST_CHECK( cmap.count( ptsBW[1] ) );

    // Pin
    cmap.clear();
    LIB_SYMBOL libSymbol( "PWR", nullptr );
    SCH_PIN*   libPin = new SCH_PIN( &libSymbol );
    libPin->SetNumber( "1" );
    libPin->SetName( "P" );
    libPin->SetType( ELECTRICAL_PINTYPE::PT_POWER_IN );
    libPin->SetPosition( VECTOR2I( 10, 10 ) );
    libSymbol.AddDrawItem( libPin );
    libSymbol.SetGlobalPower();

    SCH_SYMBOL symbol( libSymbol, libSymbol.GetLibId(), &sheetPath, 0, 0, VECTOR2I( 0, 0 ) );
    symbol.SetRef( &sheetPath, "U1" );
    symbol.UpdatePins();
    SCH_PIN* pin = symbol.GetPins( &sheetPath )[0];

    graph.updateGenericItemConnectivity( sheetPath, pin, cmap );
    SCH_CONNECTION* pinConn = pin->Connection( &sheetPath );
    BOOST_REQUIRE( pinConn );
    BOOST_CHECK( pinConn->Type() == CONNECTION_TYPE::NET );
    BOOST_CHECK( graph.m_global_power_pins.size() == 1 );
    BOOST_CHECK( cmap.count( pin->GetPosition() ) );
}

BOOST_AUTO_TEST_SUITE( UpdateItemsConnectivity )
BOOST_AUTO_TEST_CASE( SymbolConnectivityLinksPins )
{
    boost_test_update_symbol_connectivity();
}
BOOST_AUTO_TEST_CASE( GenericItemConnectivity )
{
    boost_test_update_generic_connectivity();
}

BOOST_AUTO_TEST_SUITE_END()