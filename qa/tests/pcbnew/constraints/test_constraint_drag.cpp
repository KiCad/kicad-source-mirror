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

#include <algorithm>
#include <cmath>
#include <vector>

#include <qa_utils/wx_utils/unit_test_utils.h>
#include <tool/tool_manager.h>
#include <pcbnew_utils/board_test_utils.h>

#include <board.h>
#include <board_commit.h>
#include <pcb_shape.h>
#include <geometry/shape_arc.h>
#include <constraints/pcb_constraint.h>
#include <constraints/board_constraint_adapter.h>
#include <constraints/constraint_builder.h>


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

    PCB_SHAPE* addArc( const VECTOR2I& aStart, const VECTOR2I& aMid, const VECTOR2I& aEnd )
    {
        PCB_SHAPE* arc = new PCB_SHAPE( &board, SHAPE_T::ARC );
        arc->SetArcGeometry( aStart, aMid, aEnd );
        board.Add( arc );
        return arc;
    }

    PCB_SHAPE* addRect( const VECTOR2I& aStart, const VECTOR2I& aEnd )
    {
        PCB_SHAPE* rect = new PCB_SHAPE( &board, SHAPE_T::RECTANGLE );
        rect->SetStart( aStart );
        rect->SetEnd( aEnd );
        board.Add( rect );
        return rect;
    }

    PCB_SHAPE* addPoly( const std::vector<VECTOR2I>& aPoints )
    {
        PCB_SHAPE* poly = new PCB_SHAPE( &board, SHAPE_T::POLY );
        poly->SetPolyPoints( aPoints );
        board.Add( poly );
        return poly;
    }

    PCB_CONSTRAINT* addCoincident( PCB_SHAPE* aA, CONSTRAINT_ANCHOR aAnchorA, PCB_SHAPE* aB,
                                   CONSTRAINT_ANCHOR aAnchorB )
    {
        PCB_CONSTRAINT* c = new PCB_CONSTRAINT( &board, PCB_CONSTRAINT_TYPE::COINCIDENT );
        c->AddMember( aA->m_Uuid, aAnchorA );
        c->AddMember( aB->m_Uuid, aAnchorB );
        board.Add( c );
        return c;
    }
};


// Mirrors PCB_POINT_EDITOR updateItem staging dragged shape moving it then rederiving constrained cluster
// staging each neighbor before it changes
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
                  [&]( BOARD_ITEM* aNeighbor ) { aCommit.Modify( aNeighbor ); } );
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


// The live move drag stages every neighbor it pulls along into the drag commit, so cancelling the
// move (localCommit.Revert) must restore both the moved shape and its solved neighbor.
BOOST_FIXTURE_TEST_CASE( MoveReSolveStagesNeighborsRevertRestores, DRAG_FIXTURE )
{
    PCB_SHAPE* a = addSegment( { 0, 0 }, { 10 * MM, 0 } );
    PCB_SHAPE* b = addSegment( { 10 * MM, 0 }, { 10 * MM, 10 * MM } );

    addCoincident( a, CONSTRAINT_ANCHOR::END, b, CONSTRAINT_ANCHOR::START );

    const VECTOR2I aStart0 = a->GetStart();
    const VECTOR2I aEnd0 = a->GetEnd();
    const VECTOR2I bStart0 = b->GetStart();

    // Mirrors drag loop staging moved shape translating it then resolving cluster into same commit
    // staging each neighbor the solver touches
    BOARD_COMMIT            commit( tool );
    std::vector<PCB_SHAPE*> modified;

    commit.Modify( a );
    a->Move( { 0, 4 * MM } );

    ReSolveShapeClusters( &board, { a }, &modified,
                          [&]( BOARD_ITEM* aItem ) { commit.Modify( aItem ); } );

    // The neighbor followed the moved corner.
    BOOST_CHECK_LE( ( a->GetEnd() - b->GetStart() ).EuclideanNorm(), 100 );
    BOOST_CHECK( b->GetStart() != bStart0 );

    // Cancelling the move restores every shape the drag commit staged.
    commit.Revert();
    BOOST_CHECK_EQUAL( a->GetStart(), aStart0 );
    BOOST_CHECK_EQUAL( a->GetEnd(), aEnd0 );
    BOOST_CHECK_EQUAL( b->GetStart(), bStart0 );
}


// The caller-owned, Pack and Duplicate move paths pass no constraint shapes, so the drag collects
// none and the settle-solve runs over an empty set.  That must stage nothing and touch no neighbor,
// mirroring the guard that keeps those commits free of constraint side effects.
BOOST_FIXTURE_TEST_CASE( MoveEmptyShapesStagesNoNeighbor, DRAG_FIXTURE )
{
    PCB_SHAPE* a = addSegment( { 0, 0 }, { 10 * MM, 0 } );
    PCB_SHAPE* b = addSegment( { 10 * MM, 0 }, { 10 * MM, 10 * MM } );

    addCoincident( a, CONSTRAINT_ANCHOR::END, b, CONSTRAINT_ANCHOR::START );

    const VECTOR2I bStart0 = b->GetStart();

    BOARD_COMMIT            commit( tool );
    std::vector<PCB_SHAPE*> modified;
    int                     staged = 0;

    ReSolveShapeClusters( &board, {}, &modified,
                          [&]( BOARD_ITEM* ) { staged++; } );

    BOOST_CHECK_EQUAL( staged, 0 );
    BOOST_CHECK( modified.empty() );
    BOOST_CHECK_EQUAL( b->GetStart(), bStart0 );
    BOOST_CHECK( commit.Empty() );
}


// A fixed-length segment dragged by one endpoint rotates about the other end, which stays put.
// Without pinning the far end the solver is free to translate the whole segment (Zulip "Constraint
// solver", 2026-07-10: "we need to pin the other end of the line when moving, not only the length").
BOOST_FIXTURE_TEST_CASE( FixedLengthDragPinsFarEnd, DRAG_FIXTURE )
{
    PCB_SHAPE* seg = addSegment( { 0, 0 }, { 10 * MM, 0 } );

    PCB_CONSTRAINT* len = new PCB_CONSTRAINT( &board, PCB_CONSTRAINT_TYPE::FIXED_LENGTH );
    len->AddMember( seg->m_Uuid, CONSTRAINT_ANCHOR::WHOLE );
    len->SetValue( 10.0 * MM );
    board.Add( len );

    const VECTOR2I start0 = seg->GetStart();

    std::vector<PCB_SHAPE*> modified;

    BOARD_COMMIT commit( tool );

    // Drag END to a 6-8-10 point, so the held 10 mm length lands it exactly on the cursor.
    simulateDrag( commit, &board, seg, CONSTRAINT_ANCHOR::END, { 6 * MM, 8 * MM }, &modified );

    // The far (start) end held where it was, and the length is unchanged.
    BOOST_CHECK_LE( ( seg->GetStart() - start0 ).EuclideanNorm(), 5000.0 );
    BOOST_CHECK_LE( std::abs( ( seg->GetEnd() - seg->GetStart() ).EuclideanNorm() - 10.0 * MM ), 5000.0 );

    // The dragged end reached the cursor, which sat on the length circle.
    BOOST_CHECK_LE( ( seg->GetEnd() - VECTOR2I( 6 * MM, 8 * MM ) ).EuclideanNorm(), 20000.0 );

    commit.Revert();
}


// A fixed-length segment dragged by one end holds the far end even when the cursor is off the length
// circle.  The far end used to drift to split the pin error between the two ends.
BOOST_FIXTURE_TEST_CASE( FixedLengthDragOffCircleHoldsFarEnd, DRAG_FIXTURE )
{
    PCB_SHAPE* seg = addSegment( { 0, 0 }, { 10 * MM, 0 } );

    PCB_CONSTRAINT* len = new PCB_CONSTRAINT( &board, PCB_CONSTRAINT_TYPE::FIXED_LENGTH );
    len->AddMember( seg->m_Uuid, CONSTRAINT_ANCHOR::WHOLE );
    len->SetValue( 10.0 * MM );
    board.Add( len );

    const VECTOR2I start0 = seg->GetStart();

    std::vector<PCB_SHAPE*> modified;
    BOARD_COMMIT            commit( tool );

    // Cursor 20 mm out on +x, off the 10 mm circle.  Far end holds, dragged end lands at {10 mm, 0}.
    simulateDrag( commit, &board, seg, CONSTRAINT_ANCHOR::END, { 20 * MM, 0 }, &modified );

    BOOST_CHECK_LE( ( seg->GetStart() - start0 ).EuclideanNorm(), 5000.0 );
    BOOST_CHECK_LE( std::abs( ( seg->GetEnd() - seg->GetStart() ).EuclideanNorm() - 10.0 * MM ), 5000.0 );
    BOOST_CHECK_LE( ( seg->GetEnd() - VECTOR2I( 10 * MM, 0 ) ).EuclideanNorm(), 20000.0 );

    commit.Revert();
}


// Dragging one endpoint of a constrained arc holds the circle (centre + radius) and the far
// endpoint, so only the dragged endpoint sweeps -- the arc does not drift or balloon.
BOOST_FIXTURE_TEST_CASE( ArcEndpointDragHoldsCircleAndFarEnd, DRAG_FIXTURE )
{
    PCB_SHAPE* arc = addArc( { 10 * MM, 0 }, { 7071068, 7071068 }, { 0, 10 * MM } );   // 90 deg, r 10
    PCB_SHAPE* seg = addSegment( { 0, 10 * MM }, { 5 * MM, 15 * MM } );
    addCoincident( arc, CONSTRAINT_ANCHOR::END, seg, CONSTRAINT_ANCHOR::START );

    const VECTOR2I center0 = arc->GetCenter();
    const int      radius0 = arc->GetRadius();
    const VECTOR2I end0 = arc->GetEnd();

    // Even an off-circle target is projected onto the held circle inside the adapter, so the centre,
    // radius and far end stay put and only the dragged endpoint sweeps.
    std::vector<PCB_SHAPE*> modified;
    BOARD_COMMIT            commit( tool );
    SolveCluster( &board, { arc->m_Uuid, CONSTRAINT_ANCHOR::START }, { 12 * MM, 3 * MM }, &modified,
                  [&]( BOARD_ITEM* aItem ) { commit.Modify( aItem ); } );

    BOOST_CHECK_LE( ( arc->GetCenter() - center0 ).EuclideanNorm(), 20000.0 );
    BOOST_CHECK_LE( std::abs( arc->GetRadius() - radius0 ), 20000 );
    BOOST_CHECK_LE( ( arc->GetEnd() - end0 ).EuclideanNorm(), 20000.0 );
    BOOST_CHECK( ( arc->GetStart() - VECTOR2I( 10 * MM, 0 ) ).EuclideanNorm() > 20000.0 );
}


// Real FIXED_RADIUS on arc overrides temporary radius hold
// Dragging endpoint keeps driven radius so far end moves off old spot to stay on the now larger circle
BOOST_FIXTURE_TEST_CASE( ArcEndpointDragYieldsToFixedRadius, DRAG_FIXTURE )
{
    PCB_SHAPE* arc = addArc( { 10 * MM, 0 }, { 7071068, 7071068 }, { 0, 10 * MM } );   // r 10

    PCB_CONSTRAINT* r = new PCB_CONSTRAINT( &board, PCB_CONSTRAINT_TYPE::FIXED_RADIUS );
    r->AddMember( arc->m_Uuid, CONSTRAINT_ANCHOR::WHOLE );
    r->SetValue( 12.0 * MM );   // drive the radius larger than the current 10 mm
    board.Add( r );

    std::vector<PCB_SHAPE*> modified;
    BOARD_COMMIT            commit( tool );
    SolveCluster( &board, { arc->m_Uuid, CONSTRAINT_ANCHOR::START }, { 12 * MM, 0 }, &modified,
                  [&]( BOARD_ITEM* aItem ) { commit.Modify( aItem ); } );

    // The driving radius wins over the temporary hold.
    BOOST_CHECK_LE( std::abs( arc->GetRadius() - 12 * MM ), 20000 );
}


// Dragging a rectangle corner resizes about the diagonally opposite corner, which
// pinDraggedShapeRest holds.  The rect stores its corners swapped (start is the bottom-right), so a
// broken canonical corner-role mapping would drive or hold the wrong corner and fail both checks.
BOOST_FIXTURE_TEST_CASE( RectCornerDragHoldsOppositeCorner, DRAG_FIXTURE )
{
    PCB_SHAPE* rect = addRect( { 10 * MM, 10 * MM }, { 0, 0 } );   // swapped storage
    PCB_SHAPE* seg = addSegment( { 10 * MM, 0 }, { 15 * MM, -5 * MM } );

    // Tie the dragged corner (canonical index 1, the corner at (10mm, 0)) to a segment end so the
    // cluster contains a mappable constraint and the solve runs.
    PCB_CONSTRAINT* c = new PCB_CONSTRAINT( &board, PCB_CONSTRAINT_TYPE::COINCIDENT );
    c->AddMember( rect->m_Uuid, CONSTRAINT_ANCHOR::VERTEX, 1 );
    c->AddMember( seg->m_Uuid, CONSTRAINT_ANCHOR::START );
    board.Add( c );

    const VECTOR2I cursor( 12 * MM, -2 * MM );

    std::vector<PCB_SHAPE*> modified;
    BOARD_COMMIT            commit( tool );
    SolveCluster( &board, { rect->m_Uuid, CONSTRAINT_ANCHOR::VERTEX, 1 }, cursor, &modified,
                  [&]( BOARD_ITEM* aItem ) { commit.Modify( aItem ); } );

    VECTOR2I tl( std::min( rect->GetStart().x, rect->GetEnd().x ),
                 std::min( rect->GetStart().y, rect->GetEnd().y ) );
    VECTOR2I br( std::max( rect->GetStart().x, rect->GetEnd().x ),
                 std::max( rect->GetStart().y, rect->GetEnd().y ) );

    // The dragged top-right corner landed on the cursor and pulled the coincident segment along.
    BOOST_CHECK_LE( ( VECTOR2I( br.x, tl.y ) - cursor ).EuclideanNorm(), 5000.0 );
    BOOST_CHECK_LE( ( seg->GetStart() - cursor ).EuclideanNorm(), 5000.0 );

    // The opposite (bottom-left) corner held its position.
    BOOST_CHECK_LE( ( VECTOR2I( tl.x, br.y ) - VECTOR2I( 0, 10 * MM ) ).EuclideanNorm(), 5000.0 );
}


// Dragging one polygon vertex moves only that vertex; pinDraggedShapeRest holds every other vertex
// so the rest of the outline does not drift.
BOOST_FIXTURE_TEST_CASE( PolyVertexDragHoldsOtherVertices, DRAG_FIXTURE )
{
    const std::vector<VECTOR2I> points{ { 0, 0 },
                                        { 10 * MM, 0 },
                                        { 13 * MM, 8 * MM },
                                        { 5 * MM, 14 * MM },
                                        { -3 * MM, 8 * MM } };

    PCB_SHAPE* poly = addPoly( points );
    PCB_SHAPE* seg = addSegment( { 0, 0 }, { -5 * MM, -5 * MM } );

    // Tie one vertex to a segment start so the cluster contains a mappable constraint.
    PCB_CONSTRAINT* c = new PCB_CONSTRAINT( &board, PCB_CONSTRAINT_TYPE::COINCIDENT );
    c->AddMember( poly->m_Uuid, CONSTRAINT_ANCHOR::VERTEX, 0 );
    c->AddMember( seg->m_Uuid, CONSTRAINT_ANCHOR::START );
    board.Add( c );

    const VECTOR2I cursor( 16 * MM, 9 * MM );

    std::vector<PCB_SHAPE*> modified;
    BOARD_COMMIT            commit( tool );
    SolveCluster( &board, { poly->m_Uuid, CONSTRAINT_ANCHOR::VERTEX, 2 }, cursor, &modified,
                  [&]( BOARD_ITEM* aItem ) { commit.Modify( aItem ); } );

    const SHAPE_LINE_CHAIN& outline = poly->GetPolyShape().COutline( 0 );

    BOOST_REQUIRE_EQUAL( outline.PointCount(), 5 );
    BOOST_CHECK_LE( ( outline.CPoint( 2 ) - cursor ).EuclideanNorm(), 5000.0 );

    for( int i : { 0, 1, 3, 4 } )
        BOOST_CHECK_LE( ( outline.CPoint( i ) - points[i] ).EuclideanNorm(), 5000.0 );
}


// The point-editor bridge maps a dragged rectangle corner handle to its canonical min/max corner,
// independent of which diagonal the shape stores; ordinals past the corners map to nothing, so the
// centre and radius handles fall through to the authoritative-shape re-solve instead.
BOOST_FIXTURE_TEST_CASE( VertexForRectCornerMapsCanonicalIndex, DRAG_FIXTURE )
{
    PCB_SHAPE* rect = addRect( { 0, 0 }, { 10 * MM, 10 * MM } );
    PCB_SHAPE* swapped = addRect( { 10 * MM, 10 * MM }, { 0, 0 } );   // swapped storage

    for( PCB_SHAPE* shape : { rect, swapped } )
    {
        const std::vector<VECTOR2I> corners{ { 0, 0 },
                                             { 10 * MM, 0 },
                                             { 10 * MM, 10 * MM },
                                             { 0, 10 * MM } };

        for( size_t i = 0; i < corners.size(); ++i )
        {
            std::optional<CONSTRAINT_ANCHOR_POINT> vertex = ConstraintShapeVertex( shape, (int) i );

            BOOST_REQUIRE( vertex.has_value() );
            BOOST_CHECK( vertex->anchor == CONSTRAINT_ANCHOR::VERTEX );
            BOOST_CHECK_EQUAL( vertex->index, (int) i );
            BOOST_CHECK( vertex->pos == corners[i] );
        }
    }

    BOOST_CHECK( !ConstraintShapeVertex( rect, 4 ) );
    BOOST_CHECK( !ConstraintShapeVertex( rect, -1 ) );

    // A segment exposes endpoint anchors, not vertices, so an ordinal on it maps to nothing.
    PCB_SHAPE* seg = addSegment( { 0, 0 }, { 10 * MM, 0 } );
    BOOST_CHECK( !ConstraintShapeVertex( seg, 0 ) );
}


// The point-editor bridge maps a dragged polygon vertex handle to its outline ordinal; an ordinal
// past the outline, or any ordinal on an arc-bearing polygon, maps to nothing.
BOOST_FIXTURE_TEST_CASE( VertexForPolyMapsOrdinal, DRAG_FIXTURE )
{
    const std::vector<VECTOR2I> points{ { 0, 0 },
                                        { 10 * MM, 0 },
                                        { 13 * MM, 8 * MM },
                                        { 5 * MM, 14 * MM },
                                        { -3 * MM, 8 * MM } };

    PCB_SHAPE* poly = addPoly( points );

    for( size_t i = 0; i < points.size(); ++i )
    {
        std::optional<CONSTRAINT_ANCHOR_POINT> vertex = ConstraintShapeVertex( poly, (int) i );

        BOOST_REQUIRE( vertex.has_value() );
        BOOST_CHECK( vertex->anchor == CONSTRAINT_ANCHOR::VERTEX );
        BOOST_CHECK_EQUAL( vertex->index, (int) i );
        BOOST_CHECK( vertex->pos == points[i] );
    }

    BOOST_CHECK( !ConstraintShapeVertex( poly, (int) points.size() ) );

    PCB_SHAPE* arcPoly = new PCB_SHAPE( &board, SHAPE_T::POLY );

    SHAPE_LINE_CHAIN chain;
    chain.Append( VECTOR2I( 10 * MM, 10 * MM ) );
    chain.Append( VECTOR2I( 50 * MM, 10 * MM ) );
    chain.Append( SHAPE_ARC( { 50 * MM, 10 * MM }, { 55 * MM, 25 * MM }, { 50 * MM, 40 * MM }, 0 ) );
    chain.Append( VECTOR2I( 10 * MM, 40 * MM ) );
    chain.SetClosed( true );

    arcPoly->GetPolyShape().AddOutline( chain );
    board.Add( arcPoly );

    BOOST_REQUIRE_GT( arcPoly->GetPolyShape().COutline( 0 ).ArcCount(), 0 );

    // The solver never ingests an arc-bearing outline, so its vertices must not map to members.
    BOOST_CHECK( !ConstraintShapeVertex( arcPoly, 0 ) );
}


// Mimics point editor end to end for rectangle corner
// Behavior moves corner first bridge maps dragged handle ordinal to canonical corner and solve pulls coincident segment along
BOOST_FIXTURE_TEST_CASE( RectCornerDragMapsThenSolves, DRAG_FIXTURE )
{
    PCB_SHAPE* rect = addRect( { 0, 0 }, { 10 * MM, 10 * MM } );
    PCB_SHAPE* seg = addSegment( { 10 * MM, 0 }, { 15 * MM, -5 * MM } );

    PCB_CONSTRAINT* c = new PCB_CONSTRAINT( &board, PCB_CONSTRAINT_TYPE::COINCIDENT );
    c->AddMember( rect->m_Uuid, CONSTRAINT_ANCHOR::VERTEX, 1 );
    c->AddMember( seg->m_Uuid, CONSTRAINT_ANCHOR::START );
    board.Add( c );

    // Drag the top-right corner as RECTANGLE_POINT_EDIT_BEHAVIOR::UpdateItem would.
    const VECTOR2I cursor( 12 * MM, -2 * MM );
    rect->SetTop( cursor.y );
    rect->SetRight( cursor.x );

    std::optional<CONSTRAINT_ANCHOR_POINT> vertex = ConstraintShapeVertex( rect, 1 );

    BOOST_REQUIRE( vertex.has_value() );
    BOOST_CHECK( vertex->pos == cursor );

    std::vector<PCB_SHAPE*> modified;
    BOARD_COMMIT            commit( tool );
    SolveCluster( &board, { rect->m_Uuid, vertex->anchor, vertex->index }, vertex->pos, &modified,
                  [&]( BOARD_ITEM* aItem ) { commit.Modify( aItem ); } );

    BOOST_CHECK_LE( ( seg->GetStart() - cursor ).EuclideanNorm(), 5000.0 );
}


// PinEditedCorner clamps a corner drag at minimum size so shape holds clamped corner while edit point keeps raw cursor
// Mapping by handle ordinal and solving toward post clamp corner keeps neighbor riding its real position
BOOST_FIXTURE_TEST_CASE( ClampedRectCornerDragSolvesToClampedCorner, DRAG_FIXTURE )
{
    PCB_SHAPE* rect = addRect( { 0, 0 }, { 10 * MM, 10 * MM } );
    PCB_SHAPE* seg = addSegment( { 10 * MM, 0 }, { 15 * MM, -5 * MM } );

    PCB_CONSTRAINT* c = new PCB_CONSTRAINT( &board, PCB_CONSTRAINT_TYPE::COINCIDENT );
    c->AddMember( rect->m_Uuid, CONSTRAINT_ANCHOR::VERTEX, 1 );
    c->AddMember( seg->m_Uuid, CONSTRAINT_ANCHOR::START );
    board.Add( c );

    // Drag the top-right corner far past the left edge; the behavior clamps x to the minimum
    // width while y follows the cursor, so the shape corner and the raw cursor diverge.
    const VECTOR2I rawCursor( -5 * MM, -2 * MM );
    const int      minWidth = 25400;   // 1 mil floor applied by PinEditedCorner
    const VECTOR2I clamped( minWidth, rawCursor.y );

    rect->SetTop( clamped.y );
    rect->SetRight( clamped.x );

    std::optional<CONSTRAINT_ANCHOR_POINT> vertex = ConstraintShapeVertex( rect, 1 );

    BOOST_REQUIRE( vertex.has_value() );
    BOOST_CHECK( vertex->pos == clamped );
    BOOST_CHECK( vertex->pos != rawCursor );

    std::vector<PCB_SHAPE*> modified;
    BOARD_COMMIT            commit( tool );
    SolveCluster( &board, { rect->m_Uuid, vertex->anchor, vertex->index }, vertex->pos, &modified,
                  [&]( BOARD_ITEM* aItem ) { commit.Modify( aItem ); } );

    BOOST_CHECK_LE( ( seg->GetStart() - clamped ).EuclideanNorm(), 5000.0 );
}


namespace
{
VECTOR2I dragRectTopLeft( const PCB_SHAPE* aRect )
{
    return VECTOR2I( std::min( aRect->GetStart().x, aRect->GetEnd().x ),
                     std::min( aRect->GetStart().y, aRect->GetEnd().y ) );
}


VECTOR2I dragRectBotRight( const PCB_SHAPE* aRect )
{
    return VECTOR2I( std::max( aRect->GetStart().x, aRect->GetEnd().x ),
                     std::max( aRect->GetStart().y, aRect->GetEnd().y ) );
}


PCB_CONSTRAINT* addDrivingVertexLength( BOARD& aBoard, PCB_SHAPE* aShape, int aIndexA, int aIndexB,
                                        double aLengthIU )
{
    PCB_CONSTRAINT* c = new PCB_CONSTRAINT( &aBoard, PCB_CONSTRAINT_TYPE::FIXED_LENGTH );
    c->AddMember( aShape->m_Uuid, CONSTRAINT_ANCHOR::VERTEX, aIndexA );
    c->AddMember( aShape->m_Uuid, CONSTRAINT_ANCHOR::VERTEX, aIndexB );
    c->SetValue( aLengthIU );
    c->SetDriving( true );
    aBoard.Add( c );
    return c;
}
}


// Driving width on top side must survive adjacent side drag where side handles used to bypass solver and violate length
// Pinning side canonical corner routes it through same drag solve as corner drag so hard length wins and rectangle translates
BOOST_FIXTURE_TEST_CASE( RectSideDragEnforcesDrivingWidth, DRAG_FIXTURE )
{
    PCB_SHAPE* rect = addRect( { 0, 0 }, { 10 * MM, 10 * MM } );

    addDrivingVertexLength( board, rect, 0, 1, 10.0 * MM );   // TL-TR width

    // Drag the right side 5 mm out, as RECTANGLE_POINT_EDIT_BEHAVIOR::UpdateItem would.
    rect->SetRight( 15 * MM );

    // Side RECT_RIGHT (1) maps to canonical corner 1 (TR), read back post-move.
    std::optional<CONSTRAINT_ANCHOR_POINT> corner = ConstraintShapeVertex( rect, 1 );

    BOOST_REQUIRE( corner.has_value() );
    BOOST_CHECK( corner->pos == VECTOR2I( 15 * MM, 0 ) );

    std::vector<PCB_SHAPE*> modified;
    BOARD_COMMIT            commit( tool );
    SolveCluster( &board, { rect->m_Uuid, corner->anchor, corner->index }, corner->pos, &modified,
                  [&]( BOARD_ITEM* aItem ) { commit.Modify( aItem ); } );

    const VECTOR2I tl = dragRectTopLeft( rect );
    const VECTOR2I br = dragRectBotRight( rect );

    // The driving width held against the drag and the height stayed untouched.
    BOOST_CHECK_LE( std::abs( ( br.x - tl.x ) - 10 * MM ), 20000 );
    BOOST_CHECK_LE( std::abs( ( br.y - tl.y ) - 10 * MM ), 20000 );

    // Drag was not simply refused rect moved toward cursor strictly between original and requested span
    BOOST_CHECK_GT( tl.x, 1 * MM );
    BOOST_CHECK_LT( br.x, 14 * MM );

    BOOST_TEST_MESSAGE( "side drag under driving width settled TL at " << tl.x << "," << tl.y );

    // The four enumerated corners agree with the solved extremes, so the rect stayed axis-aligned.
    std::vector<CONSTRAINT_ANCHOR_POINT> corners = ConstraintShapeAnchors( rect );

    BOOST_REQUIRE_EQUAL( corners.size(), 4 );
    BOOST_CHECK_EQUAL( corners[0].pos, tl );
    BOOST_CHECK_EQUAL( corners[2].pos, br );
}


// Dragging bottom side leaves top side width constraint satisfiable so resize applies exactly
// Height follows handle while held top left corner and driven width stay intact
BOOST_FIXTURE_TEST_CASE( RectSideDragOffConstrainedAxisResizes, DRAG_FIXTURE )
{
    PCB_SHAPE* rect = addRect( { 0, 0 }, { 10 * MM, 10 * MM } );

    addDrivingVertexLength( board, rect, 0, 1, 10.0 * MM );   // TL-TR width

    // Drag the bottom side 5 mm down; side RECT_BOT (2) maps to canonical corner 2 (BR).
    rect->SetBottom( 15 * MM );

    std::optional<CONSTRAINT_ANCHOR_POINT> corner = ConstraintShapeVertex( rect, 2 );

    BOOST_REQUIRE( corner.has_value() );
    BOOST_CHECK( corner->pos == VECTOR2I( 10 * MM, 15 * MM ) );

    std::vector<PCB_SHAPE*> modified;
    BOARD_COMMIT            commit( tool );
    SolveCluster( &board, { rect->m_Uuid, corner->anchor, corner->index }, corner->pos, &modified,
                  [&]( BOARD_ITEM* aItem ) { commit.Modify( aItem ); } );

    const VECTOR2I tl = dragRectTopLeft( rect );
    const VECTOR2I br = dragRectBotRight( rect );

    BOOST_CHECK_LE( tl.EuclideanNorm(), 5000.0 );
    BOOST_CHECK_LE( std::abs( ( br.x - tl.x ) - 10 * MM ), 5000 );
    BOOST_CHECK_LE( std::abs( ( br.y - tl.y ) - 15 * MM ), 5000 );
}


// A side drag on a rectangle whose own dimensions are unconstrained resizes exactly as the handle
// placed it; the coincident neighbor on an unmoved corner stays put.
BOOST_FIXTURE_TEST_CASE( UnconstrainedRectSideDragResizesExactly, DRAG_FIXTURE )
{
    PCB_SHAPE* rect = addRect( { 0, 0 }, { 10 * MM, 10 * MM } );
    PCB_SHAPE* seg = addSegment( { 0, 0 }, { -5 * MM, -5 * MM } );

    PCB_CONSTRAINT* c = new PCB_CONSTRAINT( &board, PCB_CONSTRAINT_TYPE::COINCIDENT );
    c->AddMember( rect->m_Uuid, CONSTRAINT_ANCHOR::VERTEX, 0 );
    c->AddMember( seg->m_Uuid, CONSTRAINT_ANCHOR::START );
    board.Add( c );

    rect->SetRight( 15 * MM );

    std::optional<CONSTRAINT_ANCHOR_POINT> corner = ConstraintShapeVertex( rect, 1 );

    BOOST_REQUIRE( corner.has_value() );

    std::vector<PCB_SHAPE*> modified;
    BOARD_COMMIT            commit( tool );
    SolveCluster( &board, { rect->m_Uuid, corner->anchor, corner->index }, corner->pos, &modified,
                  [&]( BOARD_ITEM* aItem ) { commit.Modify( aItem ); } );

    BOOST_CHECK_LE( dragRectTopLeft( rect ).EuclideanNorm(), 5000.0 );
    BOOST_CHECK_LE( ( dragRectBotRight( rect ) - VECTOR2I( 15 * MM, 10 * MM ) ).EuclideanNorm(), 5000.0 );
    BOOST_CHECK_LE( seg->GetStart().EuclideanNorm(), 5000.0 );
}


// Driving length on polygon edge must survive a drag of that same edge
// Both endpoints ride co dragged pins that yield to hard length so edge lands at driven length and unbound vertices hold
BOOST_FIXTURE_TEST_CASE( PolyEdgeDragEnforcesDrivingLength, DRAG_FIXTURE )
{
    const std::vector<VECTOR2I> points{ { 0, 0 },
                                        { 10 * MM, 0 },
                                        { 13 * MM, 8 * MM },
                                        { 5 * MM, 14 * MM },
                                        { -3 * MM, 8 * MM } };

    PCB_SHAPE*   poly = addPoly( points );
    const double edgeLen = std::hypot( 3.0, 8.0 ) * MM;   // the 1-2 edge's current length

    addDrivingVertexLength( board, poly, 1, 2, edgeLen );

    // Drag the 1-2 edge so both vertices move and its length would stretch.
    std::vector<VECTOR2I> dragged = points;
    dragged[1] = { 8 * MM, -2 * MM };
    dragged[2] = { 16 * MM, 9 * MM };
    poly->SetPolyPoints( dragged );

    std::vector<PCB_SHAPE*> modified;
    BOARD_COMMIT            commit( tool );
    SolveCluster( &board, { poly->m_Uuid, CONSTRAINT_ANCHOR::VERTEX, 1 }, dragged[1], &modified,
                  [&]( BOARD_ITEM* aItem ) { commit.Modify( aItem ); },
                  /* aIncludeDragged */ false, /* aStabilize */ false, {},
                  std::pair{ CONSTRAINT_MEMBER( poly->m_Uuid, CONSTRAINT_ANCHOR::VERTEX, 2 ), dragged[2] } );

    const SHAPE_LINE_CHAIN& outline = poly->GetPolyShape().COutline( 0 );

    double solvedLen = ( outline.CPoint( 2 ) - outline.CPoint( 1 ) ).EuclideanNorm();

    BOOST_CHECK_LE( std::abs( solvedLen - edgeLen ), 20000.0 );

    // Drag was not simply refused edge midpoint moved out of solver noise toward requested midpoint
    // Length constrains only vertex separation so midpoint is free to follow landing closer than start
    const VECTOR2I midBefore = ( points[1] + points[2] ) / 2;
    const VECTOR2I midTarget = ( dragged[1] + dragged[2] ) / 2;
    const VECTOR2I midSolved = ( outline.CPoint( 1 ) + outline.CPoint( 2 ) ) / 2;

    BOOST_CHECK_LT( ( midSolved - midTarget ).EuclideanNorm(), ( midBefore - midTarget ).EuclideanNorm() );
    BOOST_CHECK_GT( ( midSolved - midBefore ).EuclideanNorm(), 250000.0 );

    for( int i : { 0, 3, 4 } )
        BOOST_CHECK_LE( ( outline.CPoint( i ) - points[i] ).EuclideanNorm(), 5000.0 );
}


// An unconstrained polygon edge drag lands both vertices exactly where the handle placed them; the
// vertex-bound segment follows and the unbound vertices hold.
BOOST_FIXTURE_TEST_CASE( UnconstrainedPolyEdgeDragMovesBothVertices, DRAG_FIXTURE )
{
    const std::vector<VECTOR2I> points{ { 0, 0 },
                                        { 10 * MM, 0 },
                                        { 13 * MM, 8 * MM },
                                        { 5 * MM, 14 * MM },
                                        { -3 * MM, 8 * MM } };

    PCB_SHAPE* poly = addPoly( points );
    PCB_SHAPE* seg = addSegment( points[1], { 25 * MM, -5 * MM } );

    PCB_CONSTRAINT* c = new PCB_CONSTRAINT( &board, PCB_CONSTRAINT_TYPE::COINCIDENT );
    c->AddMember( poly->m_Uuid, CONSTRAINT_ANCHOR::VERTEX, 1 );
    c->AddMember( seg->m_Uuid, CONSTRAINT_ANCHOR::START );
    board.Add( c );

    // Translate the 1-2 edge 3 mm right, as the edge handle would.
    std::vector<VECTOR2I> dragged = points;
    dragged[1] += VECTOR2I( 3 * MM, 0 );
    dragged[2] += VECTOR2I( 3 * MM, 0 );
    poly->SetPolyPoints( dragged );

    std::vector<PCB_SHAPE*> modified;
    BOARD_COMMIT            commit( tool );
    SolveCluster( &board, { poly->m_Uuid, CONSTRAINT_ANCHOR::VERTEX, 1 }, dragged[1], &modified,
                  [&]( BOARD_ITEM* aItem ) { commit.Modify( aItem ); },
                  /* aIncludeDragged */ false, /* aStabilize */ false, {},
                  std::pair{ CONSTRAINT_MEMBER( poly->m_Uuid, CONSTRAINT_ANCHOR::VERTEX, 2 ), dragged[2] } );

    const SHAPE_LINE_CHAIN& outline = poly->GetPolyShape().COutline( 0 );

    BOOST_CHECK_LE( ( outline.CPoint( 1 ) - dragged[1] ).EuclideanNorm(), 5000.0 );
    BOOST_CHECK_LE( ( outline.CPoint( 2 ) - dragged[2] ).EuclideanNorm(), 5000.0 );
    BOOST_CHECK_LE( ( seg->GetStart() - dragged[1] ).EuclideanNorm(), 5000.0 );
    BOOST_CHECK( std::find( modified.begin(), modified.end(), seg ) != modified.end() );

    for( int i : { 0, 3, 4 } )
        BOOST_CHECK_LE( ( outline.CPoint( i ) - points[i] ).EuclideanNorm(), 5000.0 );
}


// Dragging an arc's centre holds both endpoints, so the centre moves and the radius adapts.
BOOST_FIXTURE_TEST_CASE( ArcCentreDragHoldsEndpoints, DRAG_FIXTURE )
{
    PCB_SHAPE* arc = addArc( { 10 * MM, 0 }, { 7071068, 7071068 }, { 0, 10 * MM } );
    PCB_SHAPE* seg = addSegment( { 0, 10 * MM }, { 5 * MM, 15 * MM } );
    addCoincident( arc, CONSTRAINT_ANCHOR::END, seg, CONSTRAINT_ANCHOR::START );

    const VECTOR2I start0 = arc->GetStart();
    const VECTOR2I end0 = arc->GetEnd();

    std::vector<PCB_SHAPE*> modified;
    BOARD_COMMIT            commit( tool );
    SolveCluster( &board, { arc->m_Uuid, CONSTRAINT_ANCHOR::CENTER }, { 1 * MM, 1 * MM }, &modified,
                  [&]( BOARD_ITEM* aItem ) { commit.Modify( aItem ); } );

    BOOST_CHECK_LE( ( arc->GetStart() - start0 ).EuclideanNorm(), 20000.0 );
    BOOST_CHECK_LE( ( arc->GetEnd() - end0 ).EuclideanNorm(), 20000.0 );
    BOOST_CHECK( ( arc->GetCenter() - VECTOR2I( 0, 0 ) ).EuclideanNorm() > 100000.0 );
}


BOOST_AUTO_TEST_SUITE_END()
