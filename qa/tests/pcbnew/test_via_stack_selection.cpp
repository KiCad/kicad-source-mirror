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
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include <qa_utils/wx_utils/unit_test_utils.h>

#include <board.h>
#include <lset.h>
#include <pcb_group.h>
#include <pcb_track.h>
#include <pcb_view.h>
#include <generators/pcb_via_stack.h>
#include <tool/tool_manager.h>
#include <tools/pcb_selection_tool.h>

BOOST_AUTO_TEST_SUITE( ViaStackSelection )


static PCB_TRACK* addSegment( BOARD& aBoard, PCB_LAYER_ID aLayer, const VECTOR2I& aStart, const VECTOR2I& aEnd )
{
    PCB_TRACK* track = new PCB_TRACK( &aBoard );
    track->SetLayer( aLayer );
    track->SetWidth( 250000 );
    track->SetStart( aStart );
    track->SetEnd( aEnd );
    aBoard.Add( track );
    return track;
}


BOOST_AUTO_TEST_CASE( ExpansionCrossesAStackWithoutSelectingIt )
{
    // view must outlive board so board items unregister from a live view at teardown.
    KIGFX::PCB_VIEW view;
    BOARD           board;
    TOOL_MANAGER    mgr;

    board.SetCopperLayerCount( 4 );
    board.SetEnabledLayers( LSET::AllCuMask( 4 ) | LSET::AllTechMask() );
    mgr.SetEnvironment( &board, &view, nullptr, nullptr, nullptr );

    PCB_SELECTION_TOOL* selTool = new PCB_SELECTION_TOOL;
    mgr.RegisterTool( selTool );

    VECTOR2I stackPos( 1000000, 0 );

    PCB_VIA_STACK* stack = new PCB_VIA_STACK( &board, F_Cu );
    stack->SetStartLayer( F_Cu );
    stack->SetEndLayer( In1_Cu );
    stack->SetViaSize( 300000 );
    stack->SetViaDrill( 150000 );
    stack->SetPosition( stackPos );
    board.Add( stack );
    stack->Regenerate( &board, nullptr );

    PCB_TRACK* segIn = addSegment( board, F_Cu, { 0, 0 }, stackPos );
    PCB_TRACK* segOut = addSegment( board, In1_Cu, stackPos, { 2000000, 0 } );

    board.BuildConnectivity();

    for( PCB_TRACK* track : { segIn, segOut } )
        view.Add( track );

    view.Add( stack );

    std::vector<BOARD_CONNECTED_ITEM*> startItems = { segIn };
    selTool->selectAllConnectedTracks( startItems, PCB_SELECTION_TOOL::STOP_NEVER );

    BOOST_CHECK( segIn->IsSelected() );
    BOOST_CHECK_MESSAGE( segOut->IsSelected(), "expansion should reach across the stack" );

    BOOST_CHECK_MESSAGE( !stack->IsSelected(), "generator members stay out of the expansion" );
}


// A stacked stack puts every hop at one position, so expansion has to walk down all of them.
BOOST_AUTO_TEST_CASE( ExpansionCrossesAMultiHopStackedStack )
{
    KIGFX::PCB_VIEW view;
    BOARD           board;
    TOOL_MANAGER    mgr;

    board.SetCopperLayerCount( 4 );
    board.SetEnabledLayers( LSET::AllCuMask( 4 ) | LSET::AllTechMask() );
    mgr.SetEnvironment( &board, &view, nullptr, nullptr, nullptr );

    PCB_SELECTION_TOOL* selTool = new PCB_SELECTION_TOOL;
    mgr.RegisterTool( selTool );

    VECTOR2I stackPos( 1000000, 0 );

    PCB_VIA_STACK* stack = new PCB_VIA_STACK( &board, F_Cu );
    stack->SetStartLayer( F_Cu );
    stack->SetEndLayer( In2_Cu );
    stack->SetViaSize( 300000 );
    stack->SetViaDrill( 150000 );
    stack->SetPosition( stackPos );
    board.Add( stack );
    stack->Regenerate( &board, nullptr );

    PCB_TRACK* segIn = addSegment( board, F_Cu, { 0, 0 }, stackPos );
    PCB_TRACK* segOut = addSegment( board, In2_Cu, stackPos, { 2000000, 0 } );

    board.BuildConnectivity();

    for( PCB_TRACK* track : { segIn, segOut } )
        view.Add( track );

    view.Add( stack );

    std::vector<BOARD_CONNECTED_ITEM*> startItems = { segIn };
    selTool->selectAllConnectedTracks( startItems, PCB_SELECTION_TOOL::STOP_NEVER );

    BOOST_CHECK( segIn->IsSelected() );
    BOOST_CHECK_MESSAGE( segOut->IsSelected(), "expansion must reach the track on the layer the stack lands on" );
    BOOST_CHECK( !stack->IsSelected() );
}


// The conservative treatment of a plain group is deliberate (see Issue24967).
BOOST_AUTO_TEST_CASE( ExpansionStillLeavesAPlainGroupAlone )
{
    KIGFX::PCB_VIEW view;
    BOARD           board;
    TOOL_MANAGER    mgr;

    board.SetCopperLayerCount( 4 );
    board.SetEnabledLayers( LSET::AllCuMask( 4 ) | LSET::AllTechMask() );
    mgr.SetEnvironment( &board, &view, nullptr, nullptr, nullptr );

    PCB_SELECTION_TOOL* selTool = new PCB_SELECTION_TOOL;
    mgr.RegisterTool( selTool );

    PCB_TRACK* segAB = addSegment( board, F_Cu, { 0, 0 }, { 1000000, 0 } );
    PCB_TRACK* segBC = addSegment( board, F_Cu, { 1000000, 0 }, { 2000000, 0 } );
    PCB_TRACK* segCD = addSegment( board, F_Cu, { 2000000, 0 }, { 3000000, 0 } );

    PCB_GROUP* group = new PCB_GROUP( &board );
    group->AddItem( segBC );
    board.Add( group );

    board.BuildConnectivity();

    for( PCB_TRACK* track : { segAB, segBC, segCD } )
        view.Add( track );

    view.Add( group );

    std::vector<BOARD_CONNECTED_ITEM*> startItems = { segAB };
    selTool->selectAllConnectedTracks( startItems, PCB_SELECTION_TOOL::STOP_NEVER );

    BOOST_CHECK( segAB->IsSelected() );
    BOOST_CHECK( segCD->IsSelected() );
    BOOST_CHECK( !segBC->IsSelected() );
    BOOST_CHECK( !group->IsSelected() );
}


BOOST_AUTO_TEST_SUITE_END()
