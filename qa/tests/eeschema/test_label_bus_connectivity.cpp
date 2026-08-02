/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright The KiCad Developers, see AUTHORS.TXT for contributors.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 3
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

/**
 * @file test_label_bus_connectivity.cpp
 * Test for issue #18605: Changing a label from non-bus to bus does not enable bus
 * unfolding until the label is moved.
 *
 * This test verifies that when a label's text changes from a net name to a bus name,
 * the connectivity is properly recalculated so that the connected bus wire gets the
 * correct bus members.
 */

#include <qa_utils/wx_utils/unit_test_utils.h>

#include <connection_graph.h>
#include <schematic.h>
#include <sch_label.h>
#include <sch_line.h>
#include <sch_screen.h>
#include <sch_sheet.h>
#include <sch_sheet_pin.h>
#include <settings/settings_manager.h>


BOOST_AUTO_TEST_SUITE( LabelBusConnectivity )


struct LABEL_BUS_CONNECTIVITY_FIXTURE
{
    LABEL_BUS_CONNECTIVITY_FIXTURE() :
            m_mgr()
    {
        m_mgr.LoadProject( "" );
        m_schematic = std::make_unique<SCHEMATIC>( &m_mgr.Prj() );
        m_schematic->Reset();
        SCH_SHEET* defaultSheet = m_schematic->GetTopLevelSheet( 0 );

        m_screen = new SCH_SCREEN( m_schematic.get() );
        m_sheet = new SCH_SHEET( m_schematic.get() );
        m_sheet->SetScreen( m_screen );
        m_schematic->AddTopLevelSheet( m_sheet );
        m_schematic->RemoveTopLevelSheet( defaultSheet );
        delete defaultSheet;
    }

    SETTINGS_MANAGER           m_mgr;
    std::unique_ptr<SCHEMATIC> m_schematic;
    SCH_SCREEN*                m_screen;
    SCH_SHEET*                 m_sheet;
};


/**
 * Test that changing a label from a net name to a bus name properly updates the
 * connected bus wire's connection members.
 *
 * Issue #18605: When a label attached to a bus wire is edited from a non-bus name
 * (like "test") to a bus name (like "test[0..7]"), the bus unfolding menu should
 * work and the bus wire should have the correct members.
 */
BOOST_FIXTURE_TEST_CASE( LabelNetToBusConnectivity, LABEL_BUS_CONNECTIVITY_FIXTURE )
{
    // Create a bus wire
    SCH_LINE* busWire = new SCH_LINE( VECTOR2I( 0, 0 ), LAYER_BUS );
    busWire->SetEndPoint( VECTOR2I( 5000000, 0 ) );
    m_screen->Append( busWire );

    // Create a label with a non-bus name, positioned on the bus wire
    SCH_LABEL* label = new SCH_LABEL( VECTOR2I( 2500000, 0 ), wxT( "test" ) );
    m_screen->Append( label );

    // Build initial connectivity
    SCH_SHEET_LIST sheets = m_schematic->BuildSheetListSortedByPageNumbers();
    m_schematic->ConnectionGraph()->Recalculate( sheets, true );

    SCH_SHEET_PATH path = sheets[0];

    // Verify initial state: bus wire should have BUS connection type but no members
    // (because "test" is not a bus name)
    SCH_CONNECTION* busConn = busWire->Connection( &path );
    BOOST_REQUIRE( busConn != nullptr );
    BOOST_CHECK( busConn->IsBus() );
    BOOST_CHECK( busConn->Members().empty() );

    // Now change the label text to a bus name
    label->SetText( wxT( "test[0..7]" ) );

    // Mark items dirty and recalculate connectivity (simulating incremental update)
    label->SetConnectivityDirty( true );
    busWire->SetConnectivityDirty( true );

    m_schematic->ConnectionGraph()->Recalculate( sheets, false );

    // Verify: bus wire should now have members from the bus label
    busConn = busWire->Connection( &path );
    BOOST_REQUIRE( busConn != nullptr );
    BOOST_CHECK( busConn->IsBus() );
    BOOST_CHECK_MESSAGE( !busConn->Members().empty(),
                         "Bus wire should have members after label changed to bus name" );

    // Verify the correct number of members (test[0..7] = 8 members)
    if( !busConn->Members().empty() )
    {
        BOOST_CHECK_EQUAL( busConn->Members().size(), 8 );
    }
}


/**
 * Test that a bus label properly provides members to a connected bus wire
 * in a full recalculation scenario.
 */
BOOST_FIXTURE_TEST_CASE( BusLabelFullRecalculation, LABEL_BUS_CONNECTIVITY_FIXTURE )
{
    // Create a bus wire
    SCH_LINE* busWire = new SCH_LINE( VECTOR2I( 0, 0 ), LAYER_BUS );
    busWire->SetEndPoint( VECTOR2I( 5000000, 0 ) );
    m_screen->Append( busWire );

    // Create a label with a bus name, positioned on the bus wire
    SCH_LABEL* label = new SCH_LABEL( VECTOR2I( 2500000, 0 ), wxT( "DATA[0..3]" ) );
    m_screen->Append( label );

    // Build connectivity
    SCH_SHEET_LIST sheets = m_schematic->BuildSheetListSortedByPageNumbers();
    m_schematic->ConnectionGraph()->Recalculate( sheets, true );

    SCH_SHEET_PATH path = sheets[0];

    // Verify: bus wire should have members from the bus label
    SCH_CONNECTION* busConn = busWire->Connection( &path );
    BOOST_REQUIRE( busConn != nullptr );
    BOOST_CHECK( busConn->IsBus() );
    BOOST_CHECK_MESSAGE( !busConn->Members().empty(),
                         "Bus wire should have members from bus label" );

    // Verify the correct number of members (DATA[0..3] = 4 members)
    if( !busConn->Members().empty() )
    {
        BOOST_CHECK_EQUAL( busConn->Members().size(), 4 );
    }
}


/**
 * Test that a hierarchical sheet pin with bus syntax keeps its bus connection after a move.
 *
 * Issue #25103: an unconditional rebuild left the moved pin's connectivity flag dirty, so
 * SCH_PAINTER ignored the resolved connection and dropped the bus colour.
 */
BOOST_FIXTURE_TEST_CASE( SheetPinBusConnectionSurvivesMove, LABEL_BUS_CONNECTIVITY_FIXTURE )
{
    const wxString busName( wxT( "bus{A B}" ) );
    const int      sheetLeft = 10000000;

    SCH_SCREEN* childScreen = new SCH_SCREEN( m_schematic.get() );
    SCH_SHEET*  childSheet = new SCH_SHEET( m_schematic.get(), VECTOR2I( sheetLeft, 0 ),
                                            VECTOR2I( 20000000, 20000000 ) );

    childScreen->SetFileName( wxT( "child.kicad_sch" ) );
    childSheet->SetScreen( childScreen );
    childSheet->SetName( wxT( "child" ) );
    childSheet->SetFileName( wxT( "child.kicad_sch" ) );
    m_screen->Append( childSheet );

    childScreen->Append( new SCH_HIERLABEL( VECTOR2I( 0, 0 ), busName ) );

    // The pin rides the sheet's left edge, so its x has to track the sheet origin
    SCH_SHEET_PIN* pin = new SCH_SHEET_PIN( childSheet, VECTOR2I( sheetLeft, 5000000 ), busName );
    childSheet->AddPin( pin );

    SCH_LINE* busWire = new SCH_LINE( VECTOR2I( 5000000, 5000000 ), LAYER_BUS );
    busWire->SetEndPoint( pin->GetTextPos() );
    m_screen->Append( busWire );

    m_schematic->RefreshHierarchy();

    SCH_SHEET_LIST sheets = m_schematic->Hierarchy();
    m_schematic->ConnectionGraph()->Recalculate( sheets, true );

    SCH_SHEET_PATH  rootPath = sheets[0];
    SCH_CONNECTION* pinConn = pin->Connection( &rootPath );

    BOOST_REQUIRE_MESSAGE( pinConn != nullptr, "Sheet pin has no connection before the move" );
    BOOST_REQUIRE_MESSAGE( pinConn->IsBus(), "Sheet pin is not a bus before the move" );

    // Simulate dragging the pin along the sheet edge with its bus wire attached
    const VECTOR2I newPos( sheetLeft, 8000000 );

    pin->SetPosition( newPos );
    busWire->SetEndPoint( pin->GetTextPos() );
    pin->SetConnectivityDirty( true );
    busWire->SetConnectivityDirty( true );

    // A schematic this small is a minor graph, so SCHEMATIC::RecalculateConnections() rebuilds
    // unconditionally after the move
    BOOST_REQUIRE( m_schematic->ConnectionGraph()->IsMinor() );
    m_schematic->ConnectionGraph()->Recalculate( sheets, true );

    // SCH_PAINTER ignores the resolved connection while this flag is set, which is what drops
    // the bus colour
    BOOST_CHECK_MESSAGE( !pin->IsConnectivityDirty(),
                         "Sheet pin connectivity still dirty after recalculation" );

    pinConn = pin->Connection( &rootPath );

    BOOST_REQUIRE_MESSAGE( pinConn != nullptr, "Sheet pin lost its connection after the move" );
    BOOST_CHECK_MESSAGE( pinConn->IsBus(), "Sheet pin lost its bus connection after the move" );
}


BOOST_AUTO_TEST_SUITE_END()
