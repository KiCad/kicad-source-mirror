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
#include <tool/tool_manager.h>
#include <pcbnew_utils/board_test_utils.h>
#include <board.h>
#include <board_commit.h>
#include <footprint.h>
#include <pad.h>
#include <pcb_shape.h>
#include <pcb_text.h>
#include <pcb_group.h>
#include <lset.h>
#include <generators/pcb_via_stack.h>
#include <pcb_view.h>
#include <tools/pcb_selection_tool.h>

BOOST_AUTO_TEST_SUITE( BoardCommit )

BOOST_AUTO_TEST_CASE( RecursesThroughGroups )
{
    BOARD board;
    TOOL_MANAGER mgr;
    mgr.SetEnvironment( &board, nullptr, nullptr, nullptr, nullptr );
    KI_TEST::DUMMY_TOOL *dummyTool = new KI_TEST::DUMMY_TOOL();
    mgr.RegisterTool( dummyTool );
    BOARD_COMMIT commit( dummyTool );

    PCB_SHAPE s1( nullptr, SHAPE_T::SEGMENT );
    PCB_SHAPE s2( nullptr, SHAPE_T::SEGMENT );
    PCB_GROUP group( nullptr );
    group.AddItem( &s1 );
    group.AddItem( &s2 );

    commit.Stage( &group, CHT_MODIFY, nullptr, RECURSE_MODE::RECURSE );

    BOOST_CHECK_EQUAL( commit.GetStatus( &s1 ), CHT_MODIFY );
    BOOST_CHECK_EQUAL( commit.GetStatus( &s2 ), CHT_MODIFY );
}

BOOST_AUTO_TEST_CASE( MakeImageCreatesTransientCopy )
{
    PCB_SHAPE shape( nullptr, SHAPE_T::SEGMENT );
    EDA_ITEM* copy = BOARD_COMMIT::MakeImage( &shape );

    BOOST_REQUIRE( copy );
    BOOST_CHECK( copy != &shape );
    BOOST_CHECK( copy->HasFlag( UR_TRANSIENT ) );

    delete copy;
}

BOOST_AUTO_TEST_CASE( ReturnsBoardFromManager )
{
    BOARD board;
    TOOL_MANAGER mgr;
    mgr.SetEnvironment( &board, nullptr, nullptr, nullptr, nullptr );
    KI_TEST::DUMMY_TOOL* dummyTool = new KI_TEST::DUMMY_TOOL();
    mgr.RegisterTool( dummyTool );

    BOARD_COMMIT commit( dummyTool );

    BOOST_CHECK_EQUAL( commit.GetBoard(), &board );
}

BOOST_AUTO_TEST_CASE( RemoveFootprintTextFromBoardEditor )
{
    BOARD        board;
    TOOL_MANAGER mgr;
    mgr.SetEnvironment( &board, nullptr, nullptr, nullptr, nullptr );
    KI_TEST::DUMMY_TOOL* dummyTool = new KI_TEST::DUMMY_TOOL();
    mgr.RegisterTool( dummyTool );

    FOOTPRINT* fp = new FOOTPRINT( &board );
    PCB_TEXT*  text = new PCB_TEXT( fp );
    text->SetText( wxT( "${REFERENCE}" ) );
    text->SetLayer( F_Fab );
    fp->Add( text );
    board.Add( fp );

    BOOST_REQUIRE_EQUAL( fp->GraphicalItems().size(), 1 );

    {
        BOARD_COMMIT commit( &mgr, true, false );
        commit.Remove( text );
        commit.Push( wxT( "Delete" ), SKIP_UNDO );
    }

    BOOST_CHECK_EQUAL( fp->GraphicalItems().size(), 0 );
}

// A COMMIT object reused across Push() calls (such as the group tool's persistent commit)
// must not carry m_addedItems from one commit into the next.  If it does, modifying a
// previously-added item in a later commit is silently dropped and no undo entry is created.
// This is the root cause of nested-group undo corruption (work item 24146).
BOOST_AUTO_TEST_CASE( ReusedCommitModifyAfterAdd )
{
    BOARD        board;
    TOOL_MANAGER mgr;
    mgr.SetEnvironment( &board, nullptr, nullptr, nullptr, nullptr );
    KI_TEST::DUMMY_TOOL* dummyTool = new KI_TEST::DUMMY_TOOL();
    mgr.RegisterTool( dummyTool );

    BOARD_COMMIT commit( &mgr, true, false );

    PCB_SHAPE* shape = new PCB_SHAPE( &board, SHAPE_T::SEGMENT );

    // First commit adds the shape.  After Push the commit is reused.
    commit.Add( shape );
    commit.Push( wxT( "Add" ), SKIP_UNDO );

    // Modifying the already-added shape in the next commit must record a change.
    commit.Modify( shape );
    BOOST_CHECK_EQUAL( commit.GetStatus( shape ), CHT_MODIFY );
}

// Removing a footprint frees its pads, fields and other owned children with it.  A child that
// sits in the selection on its own (the footprint itself unselected) must be pruned as well, or
// PCB_SELECTION::updateDrawList() dereferences the freed child on the next repaint.
BOOST_AUTO_TEST_CASE( RemoveFootprintPrunesSelectedChildren )
{
    BOARD           board;
    KIGFX::PCB_VIEW view;
    TOOL_MANAGER    mgr;
    mgr.SetEnvironment( &board, &view, nullptr, nullptr, nullptr );

    PCB_SELECTION_TOOL* selTool = new PCB_SELECTION_TOOL;
    mgr.RegisterTool( selTool );

    FOOTPRINT* fp = new FOOTPRINT( &board );
    PAD*       pad = new PAD( fp );
    fp->Add( pad );
    board.Add( fp );

    selTool->AddItemToSel( pad, true );

    BOOST_REQUIRE( selTool->GetSelection().Contains( pad ) );
    BOOST_REQUIRE( !fp->IsSelected() );

    BOARD_COMMIT commit( &mgr, true, false );
    commit.Remove( fp );
    commit.Push( wxT( "Delete footprint" ), SKIP_UNDO | SKIP_TEARDROPS );

    BOOST_CHECK( !selTool->GetSelection().Contains( pad ) );

    // With SKIP_UNDO the removed footprint is ours to free
    delete fp;
}

// Undo after a drag must put the hops back with the stack.
BOOST_AUTO_TEST_CASE( RevertAfterDraggingAViaStackRestoresItsHops )
{
    BOARD        board;
    TOOL_MANAGER mgr;

    board.SetCopperLayerCount( 4 );
    board.SetEnabledLayers( LSET::AllCuMask( 4 ) | LSET::AllTechMask() );
    mgr.SetEnvironment( &board, nullptr, nullptr, nullptr, nullptr );

    KI_TEST::DUMMY_TOOL* dummyTool = new KI_TEST::DUMMY_TOOL();
    mgr.RegisterTool( dummyTool );

    VECTOR2I origin( 10000000, 10000000 );

    PCB_VIA_STACK* stack = new PCB_VIA_STACK( &board, F_Cu );
    stack->SetStartLayer( F_Cu );
    stack->SetEndLayer( In2_Cu );
    stack->SetViaSize( 300000 );
    stack->SetViaDrill( 150000 );
    stack->SetPosition( origin );
    board.Add( stack );
    stack->Regenerate( &board, nullptr );

    std::map<BOARD_ITEM*, VECTOR2I> before;

    for( BOARD_ITEM* item : stack->GetBoardItems() )
        before[item] = item->GetPosition();

    BOOST_REQUIRE_EQUAL( before.size(), 2u );

    BOARD_COMMIT commit( &mgr, true, false );

    stack->EditStart( nullptr, &board, &commit );

    // A drag is a stream of motion events.
    for( const VECTOR2I& step : { VECTOR2I( 500000, 0 ), VECTOR2I( 500000, 250000 ) } )
    {
        stack->Move( step );
        stack->Update( nullptr, &board, &commit );
    }

    stack->EditFinish( nullptr, &board, &commit );

    commit.Revert();

    BOOST_CHECK_EQUAL( stack->GetPosition(), origin );

    for( const auto& [item, pos] : before )
    {
        BOOST_CHECK_MESSAGE( item->GetPosition() == pos,
                             "hop left behind at " + item->GetPosition().Format() + " instead of " + pos.Format() );
    }
}


// Editing a stack stages its members, then rebuilds them. A member must not carry both a
// modify and a remove line, or redo trips over the pair.
BOOST_AUTO_TEST_CASE( RegeneratingAViaStackDoesNotDoubleStageItsHops )
{
    BOARD        board;
    TOOL_MANAGER mgr;

    board.SetCopperLayerCount( 6 );
    board.SetEnabledLayers( LSET::AllCuMask( 6 ) | LSET::AllTechMask() );
    mgr.SetEnvironment( &board, nullptr, nullptr, nullptr, nullptr );

    KI_TEST::DUMMY_TOOL* dummyTool = new KI_TEST::DUMMY_TOOL();
    mgr.RegisterTool( dummyTool );

    PCB_VIA_STACK* stack = new PCB_VIA_STACK( &board, F_Cu );
    stack->SetStartLayer( F_Cu );
    stack->SetEndLayer( In2_Cu );
    stack->SetViaSize( 300000 );
    stack->SetViaDrill( 150000 );
    stack->SetPosition( VECTOR2I( 10000000, 10000000 ) );
    board.Add( stack );
    stack->Regenerate( &board, nullptr );

    std::vector<BOARD_ITEM*> original( stack->GetBoardItems().begin(), stack->GetBoardItems().end() );
    BOOST_REQUIRE_EQUAL( original.size(), 2u );

    BOARD_COMMIT commit( &mgr, true, false );

    stack->EditStart( nullptr, &board, &commit );

    // Widening the span changes the hop set, so the members are replaced rather than reused.
    stack->SetEndLayer( In3_Cu );
    stack->Update( nullptr, &board, &commit );
    stack->EditFinish( nullptr, &board, &commit );

    for( BOARD_ITEM* item : original )
    {
        BOOST_CHECK_MESSAGE( commit.GetStatus( item ) == CHT_REMOVE,
                             "replaced hop is staged as " << commit.GetStatus( item ) << ", expected CHT_REMOVE only" );
    }

    commit.Revert();
}


// Update outside an edit must do nothing. Regenerating there would delete and rebuild the
// hops behind the back of whatever holds them.
BOOST_AUTO_TEST_CASE( UpdateOutsideAnEditIsInert )
{
    BOARD        board;
    TOOL_MANAGER mgr;

    board.SetCopperLayerCount( 4 );
    board.SetEnabledLayers( LSET::AllCuMask( 4 ) | LSET::AllTechMask() );
    mgr.SetEnvironment( &board, nullptr, nullptr, nullptr, nullptr );

    KI_TEST::DUMMY_TOOL* dummyTool = new KI_TEST::DUMMY_TOOL();
    mgr.RegisterTool( dummyTool );

    PCB_VIA_STACK* stack = new PCB_VIA_STACK( &board, F_Cu );
    stack->SetStartLayer( F_Cu );
    stack->SetEndLayer( In2_Cu );
    stack->SetViaSize( 300000 );
    stack->SetViaDrill( 150000 );
    stack->SetPosition( VECTOR2I( 10000000, 10000000 ) );
    board.Add( stack );
    stack->Regenerate( &board, nullptr );

    std::vector<BOARD_ITEM*> before( stack->GetBoardItems().begin(), stack->GetBoardItems().end() );
    BOOST_REQUIRE_EQUAL( before.size(), 2u );

    BOARD_COMMIT commit( &mgr, true, false );

    // No EditStart, so IN_EDIT is not set.
    BOOST_CHECK( !stack->Update( nullptr, &board, &commit ) );

    std::vector<BOARD_ITEM*> after( stack->GetBoardItems().begin(), stack->GetBoardItems().end() );

    BOOST_REQUIRE_EQUAL( after.size(), before.size() );
    BOOST_CHECK_MESSAGE( std::set<BOARD_ITEM*>( before.begin(), before.end() )
                                 == std::set<BOARD_ITEM*>( after.begin(), after.end() ),
                         "the hops were rebuilt outside an edit" );
}


BOOST_AUTO_TEST_SUITE_END()

