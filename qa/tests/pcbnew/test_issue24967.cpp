/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright The KiCad Developers, see AUTHORS.TXT for contributors.
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
#include <pcb_group.h>
#include <pcb_track.h>
#include <pcb_view.h>
#include <tool/tool_manager.h>
#include <tools/pcb_selection_tool.h>

// Regression test for https://gitlab.com/kicad/code/kicad/-/work_items/24967
//
// Expanding a connection from a segment inside a group walked the whole net and called select()
// on segments outside the group. select() reacts to an out-of-scope item by ExitGroup(), which
// clears the selection and drops the entered-group context mid-walk, and a later Delete then
// promoted the survivors back to the whole group and wiped it. Expansion must stay within the
// entered group.

BOOST_AUTO_TEST_SUITE( Issue24967 )


static PCB_TRACK* addSegment( BOARD& aBoard, const VECTOR2I& aStart, const VECTOR2I& aEnd )
{
    PCB_TRACK* track = new PCB_TRACK( &aBoard );
    track->SetLayer( F_Cu );
    track->SetWidth( 250000 );
    track->SetStart( aStart );
    track->SetEnd( aEnd );
    aBoard.Add( track );
    return track;
}


BOOST_AUTO_TEST_CASE( ExpansionStaysInsideEnteredGroup )
{
    // view must outlive board so board items unregister from a live view at teardown.
    KIGFX::PCB_VIEW view;
    BOARD           board;
    TOOL_MANAGER    mgr;
    mgr.SetEnvironment( &board, &view, nullptr, nullptr, nullptr );

    PCB_SELECTION_TOOL* selTool = new PCB_SELECTION_TOOL;
    mgr.RegisterTool( selTool );

    // A-B-C-D chain, only the middle segment is grouped.
    PCB_TRACK* segAB = addSegment( board, { 0, 0 }, { 1000000, 0 } );
    PCB_TRACK* segBC = addSegment( board, { 1000000, 0 }, { 2000000, 0 } );
    PCB_TRACK* segCD = addSegment( board, { 2000000, 0 }, { 3000000, 0 } );

    PCB_GROUP* group = new PCB_GROUP( &board );
    group->AddItem( segBC );
    board.Add( group );

    board.BuildConnectivity();

    for( PCB_TRACK* track : { segAB, segBC, segCD } )
        view.Add( track );

    view.Add( group );

    // Enter the group and start from the grouped segment, as a double-click + U would.
    selTool->AddItemToSel( group, true );
    selTool->EnterGroup();
    BOOST_REQUIRE_EQUAL( selTool->GetEnteredGroup(), group );

    std::vector<BOARD_CONNECTED_ITEM*> startItems = { segBC };
    selTool->selectAllConnectedTracks( startItems, PCB_SELECTION_TOOL::STOP_NEVER );

    // Expansion must not escape the group: it stays entered and the ungrouped ends are untouched.
    BOOST_CHECK_EQUAL( selTool->GetEnteredGroup(), group );
    BOOST_CHECK( !segAB->IsSelected() );
    BOOST_CHECK( !segCD->IsSelected() );
}


BOOST_AUTO_TEST_CASE( ExpansionFromOutsideLeavesGroupIntact )
{
    // view must outlive board so board items unregister from a live view at teardown.
    KIGFX::PCB_VIEW view;
    BOARD           board;
    TOOL_MANAGER    mgr;
    mgr.SetEnvironment( &board, &view, nullptr, nullptr, nullptr );

    PCB_SELECTION_TOOL* selTool = new PCB_SELECTION_TOOL;
    mgr.RegisterTool( selTool );

    // A-B-C-D chain, only the middle segment is grouped.
    PCB_TRACK* segAB = addSegment( board, { 0, 0 }, { 1000000, 0 } );
    PCB_TRACK* segBC = addSegment( board, { 1000000, 0 }, { 2000000, 0 } );
    PCB_TRACK* segCD = addSegment( board, { 2000000, 0 }, { 3000000, 0 } );

    PCB_GROUP* group = new PCB_GROUP( &board );
    group->AddItem( segBC );
    board.Add( group );

    board.BuildConnectivity();

    for( PCB_TRACK* track : { segAB, segBC, segCD } )
        view.Add( track );

    view.Add( group );

    // Not entered: expand from an ungrouped segment, as a single-click + U would.
    std::vector<BOARD_CONNECTED_ITEM*> startItems = { segAB };
    selTool->selectAllConnectedTracks( startItems, PCB_SELECTION_TOOL::STOP_NEVER );

    // Expansion must not reach into the group. The ungrouped ends are selected, but the grouped
    // segment stays unselected so a following delete can't take the whole group.
    BOOST_CHECK_EQUAL( selTool->GetEnteredGroup(), nullptr );
    BOOST_CHECK( segAB->IsSelected() );
    BOOST_CHECK( segCD->IsSelected() );
    BOOST_CHECK( !segBC->IsSelected() );
}


BOOST_AUTO_TEST_SUITE_END()
