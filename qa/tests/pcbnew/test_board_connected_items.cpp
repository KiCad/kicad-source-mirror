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
 * You should have received a copy of the GNU General Public License along
 * with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <boost/test/unit_test.hpp>

#include <board.h>
#include <board_connected_item.h>
#include <footprint.h>
#include <pad.h>
#include <pcb_shape.h>
#include <pcb_track.h>
#include <zone.h>
#include <netinfo.h>
#include <pcb_io/kicad_sexpr/pcb_io_kicad_sexpr.h>


struct BOARD_CONNECTED_ITEMS_FIXTURE
{
    BOARD_CONNECTED_ITEMS_FIXTURE()
    {
        m_board = std::make_unique<BOARD>();
    }

    std::unique_ptr<BOARD> m_board;
};


BOOST_FIXTURE_TEST_SUITE( BoardConnectedItems, BOARD_CONNECTED_ITEMS_FIXTURE )


/**
 * Verify that AllConnectedItems includes copper shapes inside footprints.
 * This is a regression test for https://gitlab.com/kicad/code/kicad/-/issues/23093
 * where footprint-level shapes on copper were missed by AllConnectedItems, causing
 * MapNets to leave dangling NETINFO_ITEM pointers after paste.
 */
BOOST_AUTO_TEST_CASE( AllConnectedItems_IncludesFootprintShapes )
{
    FOOTPRINT* fp = new FOOTPRINT( m_board.get() );
    m_board->Add( fp );

    PAD* pad = new PAD( fp );
    pad->SetShape( PADSTACK::ALL_LAYERS, PAD_SHAPE::CIRCLE );
    pad->SetSize( PADSTACK::ALL_LAYERS, VECTOR2I( pcbIUScale.mmToIU( 1 ), pcbIUScale.mmToIU( 1 ) ) );
    fp->Add( pad );

    PCB_SHAPE* shape = new PCB_SHAPE( fp, SHAPE_T::SEGMENT );
    shape->SetLayer( F_Cu );
    shape->SetStart( VECTOR2I( 0, 0 ) );
    shape->SetEnd( VECTOR2I( pcbIUScale.mmToIU( 5 ), 0 ) );
    fp->Add( shape );

    auto items = m_board->AllConnectedItems();

    bool foundPad = false;
    bool foundShape = false;

    for( BOARD_CONNECTED_ITEM* item : items )
    {
        if( item == pad )
            foundPad = true;

        if( item == shape )
            foundShape = true;
    }

    BOOST_CHECK_MESSAGE( foundPad, "AllConnectedItems should include footprint pads" );
    BOOST_CHECK_MESSAGE( foundShape, "AllConnectedItems should include footprint copper shapes" );
}


/**
 * Verify that AllConnectedItems includes zones inside footprints.
 */
BOOST_AUTO_TEST_CASE( AllConnectedItems_IncludesFootprintZones )
{
    FOOTPRINT* fp = new FOOTPRINT( m_board.get() );
    m_board->Add( fp );

    ZONE* zone = new ZONE( fp );
    zone->SetLayer( F_Cu );
    zone->AppendCorner( VECTOR2I( 0, 0 ), -1 );
    zone->AppendCorner( VECTOR2I( pcbIUScale.mmToIU( 5 ), 0 ), -1 );
    zone->AppendCorner( VECTOR2I( pcbIUScale.mmToIU( 5 ), pcbIUScale.mmToIU( 5 ) ), -1 );
    zone->AppendCorner( VECTOR2I( 0, pcbIUScale.mmToIU( 5 ) ), -1 );
    fp->Add( zone );

    auto items = m_board->AllConnectedItems();

    bool foundZone = false;

    for( BOARD_CONNECTED_ITEM* item : items )
    {
        if( item == zone )
            foundZone = true;
    }

    BOOST_CHECK_MESSAGE( foundZone, "AllConnectedItems should include footprint zones" );
}


/**
 * Verify that MapNets remaps net pointers for shapes inside footprints.
 * Simulates the paste scenario where items from a clipboard board need their
 * net pointers remapped to the destination board's NETINFO_LIST.
 */
BOOST_AUTO_TEST_CASE( MapNets_RemapsFootprintShapes )
{
    // Source board simulates the clipboard board
    std::unique_ptr<BOARD> srcBoard = std::make_unique<BOARD>();

    NETINFO_ITEM* srcNet = new NETINFO_ITEM( srcBoard.get(), wxT( "TestNet" ), 1 );
    srcBoard->Add( srcNet );

    FOOTPRINT* fp = new FOOTPRINT( srcBoard.get() );
    srcBoard->Add( fp );

    PAD* pad = new PAD( fp );
    pad->SetShape( PADSTACK::ALL_LAYERS, PAD_SHAPE::CIRCLE );
    pad->SetSize( PADSTACK::ALL_LAYERS, VECTOR2I( pcbIUScale.mmToIU( 1 ), pcbIUScale.mmToIU( 1 ) ) );
    pad->SetNet( srcNet );
    fp->Add( pad );

    PCB_SHAPE* shape = new PCB_SHAPE( fp, SHAPE_T::SEGMENT );
    shape->SetLayer( F_Cu );
    shape->SetStart( VECTOR2I( 0, 0 ) );
    shape->SetEnd( VECTOR2I( pcbIUScale.mmToIU( 5 ), 0 ) );
    shape->SetNet( srcNet );
    fp->Add( shape );

    // Destination board
    NETINFO_ITEM* destNet = new NETINFO_ITEM( m_board.get(), wxT( "TestNet" ), 1 );
    m_board->Add( destNet );

    // Remap nets from source to destination
    srcBoard->MapNets( m_board.get() );

    // After MapNets, all connected items should point to destination board nets
    BOOST_CHECK_MESSAGE( pad->GetNet() != srcNet,
                         "Pad net should be remapped away from source net" );
    BOOST_CHECK_MESSAGE( shape->GetNet() != srcNet,
                         "Footprint shape net should be remapped away from source net" );
    BOOST_CHECK( pad->GetNetname() == wxT( "TestNet" ) );
    BOOST_CHECK( shape->GetNetname() == wxT( "TestNet" ) );
}


/**
 * Verify that after MapNets and source board destruction, pasted items can still be
 * serialized without crashing. This is the exact scenario from issue 23093.
 */
BOOST_AUTO_TEST_CASE( MapNets_FootprintShapeSurvivesSourceBoardDeletion )
{
    FOOTPRINT* fp = nullptr;

    {
        // Source board (clipboard) - will be destroyed at end of this scope
        std::unique_ptr<BOARD> srcBoard = std::make_unique<BOARD>();

        NETINFO_ITEM* srcNet = new NETINFO_ITEM( srcBoard.get(), wxT( "GND" ), 1 );
        srcBoard->Add( srcNet );

        fp = new FOOTPRINT( srcBoard.get() );
        srcBoard->Add( fp );

        PAD* pad = new PAD( fp );
        pad->SetShape( PADSTACK::ALL_LAYERS, PAD_SHAPE::CIRCLE );
        pad->SetSize( PADSTACK::ALL_LAYERS,
                      VECTOR2I( pcbIUScale.mmToIU( 1 ), pcbIUScale.mmToIU( 1 ) ) );
        pad->SetNet( srcNet );
        fp->Add( pad );

        PCB_SHAPE* shape = new PCB_SHAPE( fp, SHAPE_T::SEGMENT );
        shape->SetLayer( F_Cu );
        shape->SetStart( VECTOR2I( 0, 0 ) );
        shape->SetEnd( VECTOR2I( pcbIUScale.mmToIU( 5 ), 0 ) );
        shape->SetNet( srcNet );
        fp->Add( shape );

        // Remap nets to destination board before source goes away
        srcBoard->MapNets( m_board.get() );

        // Remove footprint from source board (simulates placeBoardItems calling RemoveAll)
        srcBoard->Remove( fp );
    }

    // Source board and its NETINFO_LIST are now destroyed.
    // Move the footprint to the destination board.
    fp->SetParent( m_board.get() );
    m_board->Add( fp );

    // Attempt to format the board. Before the fix, GetNetname() on the footprint
    // shape would dereference a dangling pointer and crash.
    STRING_FORMATTER formatter;
    PCB_IO_KICAD_SEXPR io;
    io.SetOutputFormatter( &formatter );
    BOOST_CHECK_NO_THROW( io.Format( static_cast<const BOARD_ITEM*>( fp ) ) );

    // Verify the formatted output contains the net name
    std::string output = formatter.GetString();
    BOOST_CHECK_MESSAGE( output.find( "GND" ) != std::string::npos,
                         "Formatted footprint should contain the remapped net name" );
}


BOOST_AUTO_TEST_SUITE_END()
