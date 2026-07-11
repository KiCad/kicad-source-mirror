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

#include <vector>

#include <qa_utils/wx_utils/unit_test_utils.h>
#include <tool/tool_manager.h>
#include <pcbnew_utils/board_test_utils.h>

#include <board.h>
#include <board_commit.h>
#include <pcb_shape.h>
#include <constraints/pcb_constraint.h>
#include <constraints/board_constraint_adapter.h>


BOOST_AUTO_TEST_SUITE( ConstraintSolverDrag )


namespace
{
constexpr int MM = 1000000;


struct DRAG_FIXTURE
{
    BOARD                board;
    TOOL_MANAGER         mgr;
    KI_TEST::DUMMY_TOOL* tool;

    DRAG_FIXTURE() : tool( new KI_TEST::DUMMY_TOOL() )
    {
        mgr.SetEnvironment( &board, nullptr, nullptr, nullptr, nullptr );
        mgr.RegisterTool( tool );
    }

    PCB_SHAPE* addSegment( const VECTOR2I& aStart, const VECTOR2I& aEnd )
    {
        PCB_SHAPE* seg = new PCB_SHAPE( &board, SHAPE_T::SEGMENT );
        seg->SetStart( aStart );
        seg->SetEnd( aEnd );
        board.Add( seg );
        return seg;
    }

    PCB_SHAPE* addCircle( const VECTOR2I& aCenter, int aRadius )
    {
        PCB_SHAPE* circle = new PCB_SHAPE( &board, SHAPE_T::CIRCLE );
        circle->SetCenter( aCenter );
        circle->SetRadius( aRadius );
        board.Add( circle );
        return circle;
    }
};


// Mirror PCB_POINT_EDITOR::updateItem: stage the dragged shape, move it, then re-derive the
// constrained cluster, staging each neighbor before it changes.
void simulateDrag( BOARD_COMMIT& aCommit, BOARD* aBoard, PCB_SHAPE* aShape,
                   CONSTRAINT_ANCHOR aAnchor, const VECTOR2I& aCursor,
                   std::vector<PCB_SHAPE*>* aModified )
{
    aCommit.Modify( aShape );

    if( aAnchor == CONSTRAINT_ANCHOR::START )
        aShape->SetStart( aCursor );
    else if( aAnchor == CONSTRAINT_ANCHOR::END )
        aShape->SetEnd( aCursor );
    else if( aAnchor == CONSTRAINT_ANCHOR::CENTER )
        aShape->SetCenter( aCursor );

    SolveCluster( aBoard, { aShape->m_Uuid, aAnchor }, aCursor, aModified,
                  [&]( PCB_SHAPE* aNeighbor ) { aCommit.Modify( aNeighbor ); } );
}
}


// Dragging one end of a corner re-derives the coincident neighbor; reverting the same commit
// restores both shapes (one undoable transaction).
BOOST_FIXTURE_TEST_CASE( DragReDerivesNeighborRevertRestores, DRAG_FIXTURE )
{
    PCB_SHAPE* a = addSegment( { 0, 0 }, { 10 * MM, 0 } );
    PCB_SHAPE* b = addSegment( { 10 * MM, 0 }, { 10 * MM, 10 * MM } );

    PCB_CONSTRAINT* c = new PCB_CONSTRAINT( &board, PCB_CONSTRAINT_TYPE::COINCIDENT );
    c->AddMember( a->m_Uuid, CONSTRAINT_ANCHOR::END );
    c->AddMember( b->m_Uuid, CONSTRAINT_ANCHOR::START );
    board.Add( c );

    const VECTOR2I aEnd0 = a->GetEnd();
    const VECTOR2I bStart0 = b->GetStart();

    std::vector<PCB_SHAPE*> modified;

    BOARD_COMMIT commit( tool );
    simulateDrag( commit, &board, a, CONSTRAINT_ANCHOR::END, { 12 * MM, 3 * MM }, &modified );

    // The neighbor followed the dragged corner and is reported as modified.
    BOOST_CHECK_LE( ( a->GetEnd() - b->GetStart() ).EuclideanNorm(), 100 );
    BOOST_CHECK( std::find( modified.begin(), modified.end(), b ) != modified.end() );
    BOOST_CHECK( a->GetEnd() != aEnd0 );

    // Revert restores every shape the transaction touched.
    commit.Revert();
    BOOST_CHECK_EQUAL( a->GetEnd(), aEnd0 );
    BOOST_CHECK_EQUAL( b->GetStart(), bStart0 );
}


// Dragging one circle's centre re-derives a concentric neighbor (centre-anchor drag on a
// non-segment shape).
BOOST_FIXTURE_TEST_CASE( DragCircleCentreMovesConcentricNeighbor, DRAG_FIXTURE )
{
    PCB_SHAPE* a = addCircle( { 0, 0 }, 5 * MM );
    PCB_SHAPE* b = addCircle( { 0, 0 }, 8 * MM );

    PCB_CONSTRAINT* c = new PCB_CONSTRAINT( &board, PCB_CONSTRAINT_TYPE::CONCENTRIC );
    c->AddMember( a->m_Uuid, CONSTRAINT_ANCHOR::WHOLE );
    c->AddMember( b->m_Uuid, CONSTRAINT_ANCHOR::WHOLE );
    board.Add( c );

    std::vector<PCB_SHAPE*> modified;

    BOARD_COMMIT commit( tool );
    simulateDrag( commit, &board, a, CONSTRAINT_ANCHOR::CENTER, { 4 * MM, 3 * MM }, &modified );

    // The concentric neighbor's centre followed the dragged centre.
    BOOST_CHECK_LE( ( a->GetCenter() - b->GetCenter() ).EuclideanNorm(), 2000 );
    BOOST_CHECK( std::find( modified.begin(), modified.end(), b ) != modified.end() );

    commit.Revert();
}


// When the cluster cannot be solved (here, an unmapped constraint family), neighbors are left
// untouched -- nothing is half-moved or staged.
BOOST_FIXTURE_TEST_CASE( UnsolvableClusterLeavesNeighborUntouched, DRAG_FIXTURE )
{
    PCB_SHAPE* a = addSegment( { 0, 0 }, { 10 * MM, 0 } );
    PCB_SHAPE* b = addSegment( { 10 * MM, 0 }, { 10 * MM, 10 * MM } );

    // Concentric needs circles; given two segments the adapter cannot map it, so Build() leaves it
    // unenforced and the solve is skipped, leaving the neighbor where it was.
    PCB_CONSTRAINT* c = new PCB_CONSTRAINT( &board, PCB_CONSTRAINT_TYPE::CONCENTRIC );
    c->AddMember( a->m_Uuid, CONSTRAINT_ANCHOR::WHOLE );
    c->AddMember( b->m_Uuid, CONSTRAINT_ANCHOR::WHOLE );
    board.Add( c );

    const VECTOR2I bStart0 = b->GetStart();
    const VECTOR2I bEnd0 = b->GetEnd();

    std::vector<PCB_SHAPE*> modified;

    BOARD_COMMIT commit( tool );
    simulateDrag( commit, &board, a, CONSTRAINT_ANCHOR::END, { 12 * MM, 3 * MM }, &modified );

    BOOST_CHECK( modified.empty() );
    BOOST_CHECK_EQUAL( b->GetStart(), bStart0 );
    BOOST_CHECK_EQUAL( b->GetEnd(), bEnd0 );

    commit.Revert();
}


// Deleting a referenced shape leaves the constraint in place (in an error state), it is not
// cascade-deleted (Zulip "Geometry Constraint Solver", 2026-06-18: deleting an object should put
// the constraint in an error state, not delete it).
BOOST_FIXTURE_TEST_CASE( DeleteShapeLeavesConstraintInErrorState, DRAG_FIXTURE )
{
    PCB_SHAPE* a = addSegment( { 0, 0 }, { 10 * MM, 0 } );
    PCB_SHAPE* b = addSegment( { 10 * MM, 0 }, { 10 * MM, 10 * MM } );

    KIID aId = a->m_Uuid;

    PCB_CONSTRAINT* c = new PCB_CONSTRAINT( &board, PCB_CONSTRAINT_TYPE::COINCIDENT );
    c->AddMember( aId, CONSTRAINT_ANCHOR::END );
    c->AddMember( b->m_Uuid, CONSTRAINT_ANCHOR::START );
    board.Add( c );

    BOOST_REQUIRE_EQUAL( board.Constraints().size(), 1 );

    BOARD_COMMIT commit( tool );
    commit.Remove( a );
    commit.Push( wxT( "delete segment" ) );

    // The constraint survives and still holds the now-dangling reference to the deleted shape.
    BOOST_REQUIRE_EQUAL( board.Constraints().size(), 1 );
    PCB_CONSTRAINT* survivor = board.Constraints().front();
    BOOST_REQUIRE_EQUAL( survivor->GetMembers().size(), 2 );

    bool referencesDeleted = false;

    for( const CONSTRAINT_MEMBER& m : survivor->GetMembers() )
    {
        if( m.m_item == aId )
            referencesDeleted = true;
    }

    BOOST_CHECK( referencesDeleted );
    BOOST_CHECK( board.ResolveItem( aId, true ) == nullptr );   // the shape is gone
}


// A whole-shape move breaks a point-on-line relation. ReSolveShapeClusters restores it with the
// moved shape pinned where it was dropped.
BOOST_FIXTURE_TEST_CASE( MoveReSolvesCluster, DRAG_FIXTURE )
{
    PCB_SHAPE* line = addSegment( { 0, 0 }, { 20 * MM, 0 } );
    PCB_SHAPE* seg = addSegment( { 2 * MM, 5 * MM }, { 5 * MM, 0 } );

    PCB_CONSTRAINT* c = new PCB_CONSTRAINT( &board, PCB_CONSTRAINT_TYPE::POINT_ON_LINE );
    c->AddMember( seg->m_Uuid, CONSTRAINT_ANCHOR::END );
    c->AddMember( line->m_Uuid, CONSTRAINT_ANCHOR::WHOLE );
    board.Add( c );

    PCB_CONSTRAINT* f1 = new PCB_CONSTRAINT( &board, PCB_CONSTRAINT_TYPE::FIXED_POSITION );
    f1->AddMember( line->m_Uuid, CONSTRAINT_ANCHOR::START );
    board.Add( f1 );

    PCB_CONSTRAINT* f2 = new PCB_CONSTRAINT( &board, PCB_CONSTRAINT_TYPE::FIXED_POSITION );
    f2->AddMember( line->m_Uuid, CONSTRAINT_ANCHOR::END );
    board.Add( f2 );

    seg->Move( { 0, 3 * MM } );
    BOOST_CHECK_EQUAL( seg->GetEnd().y, 3 * MM );

    std::vector<PCB_SHAPE*> modified;
    ReSolveShapeClusters( &board, { seg }, &modified );

    // The end is back on the line, the dropped start held, and the fixed line did not move.
    BOOST_CHECK_LE( std::abs( seg->GetEnd().y ), 5000 );
    BOOST_CHECK_LE( ( seg->GetStart() - VECTOR2I( 2 * MM, 8 * MM ) ).EuclideanNorm(), 5000.0 );
    BOOST_CHECK_EQUAL( line->GetStart(), VECTOR2I( 0, 0 ) );
    BOOST_CHECK_EQUAL( line->GetEnd(), VECTOR2I( 20 * MM, 0 ) );
    BOOST_CHECK( std::find( modified.begin(), modified.end(), seg ) != modified.end() );
}


// Dragging one corner of a fixed-length segment pivots about the far corner, which stays put.
BOOST_FIXTURE_TEST_CASE( DragCornerPivotsAboutFarEnd, DRAG_FIXTURE )
{
    PCB_SHAPE* seg = addSegment( { 0, 0 }, { 10 * MM, 0 } );

    PCB_CONSTRAINT* c = new PCB_CONSTRAINT( &board, PCB_CONSTRAINT_TYPE::FIXED_LENGTH );
    c->AddMember( seg->m_Uuid, CONSTRAINT_ANCHOR::WHOLE );
    c->SetValue( 10.0 * MM );
    board.Add( c );

    // Drag the START corner up toward (0, 5mm).
    SolveCluster( &board, { seg->m_Uuid, CONSTRAINT_ANCHOR::START }, { 0, 5 * MM }, nullptr, {},
                  /* aIncludeDragged */ true, /* aStabilize */ false, /* aHoldFarEnd */ true );

    BOOST_CHECK_EQUAL( seg->GetEnd(), VECTOR2I( 10 * MM, 0 ) ); // far corner held
    BOOST_CHECK_LE( std::abs( ( seg->GetEnd() - seg->GetStart() ).EuclideanNorm() - 10.0 * MM ),
                    5000.0 );                    // length preserved
    BOOST_CHECK_GT( seg->GetStart().y, 3 * MM ); // dragged corner followed the cursor
}


BOOST_AUTO_TEST_SUITE_END()
