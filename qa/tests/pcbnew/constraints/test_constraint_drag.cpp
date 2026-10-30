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
#include <array>
#include <cmath>
#include <vector>

#include <qa_utils/wx_utils/unit_test_utils.h>
#include <tool/tool_manager.h>
#include <pcbnew_utils/board_test_utils.h>

#include <board.h>
#include <board_commit.h>
#include <pcb_dimension.h>
#include <pcb_shape.h>
#include <geometry/shape_arc.h>
#include <constraints/pcb_constraint.h>
#include <constraints/board_constraint_adapter.h>
#include <constraints/constraint_builder.h>


BOOST_AUTO_TEST_SUITE( ConstraintSolverDrag )


namespace
{
constexpr int MM = 1000000;


SNAP_TARGET_ID targetId( const char* aName )
{
    return KIID::FromName( aName ).AsBytes();
}


SNAP_STABLE_ID testSnapId( const char* aName, SNAP_ID_KIND aKind = SNAP_ID_KIND::ITEM_GEOMETRY )
{
    return { aKind, targetId( aName ) };
}


struct DRAG_FIXTURE
{
    BOARD                board;
    TOOL_MANAGER         mgr;
    KI_TEST::DUMMY_TOOL* tool;

    DRAG_FIXTURE() :
            tool( new KI_TEST::DUMMY_TOOL() )
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

    PCB_CONSTRAINT* addHorizontal( PCB_SHAPE* aShape )
    {
        PCB_CONSTRAINT* c = new PCB_CONSTRAINT( &board, PCB_CONSTRAINT_TYPE::HORIZONTAL );
        c->AddMember( aShape->m_Uuid, CONSTRAINT_ANCHOR::WHOLE );
        board.Add( c );
        return c;
    }

    PCB_CONSTRAINT* addFixedPosition( PCB_SHAPE* aShape, CONSTRAINT_ANCHOR aAnchor )
    {
        PCB_CONSTRAINT* c = new PCB_CONSTRAINT( &board, PCB_CONSTRAINT_TYPE::FIXED_POSITION );
        c->AddMember( aShape->m_Uuid, aAnchor );
        board.Add( c );
        return c;
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
                  [&]( BOARD_ITEM* aNeighbor )
                  {
                      aCommit.Modify( aNeighbor );
                  } );
}
} // namespace


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


BOOST_FIXTURE_TEST_CASE( PersistentDragSessionWarmStartsSuccessivePreviewFrames, DRAG_FIXTURE )
{
    PCB_SHAPE* a = addSegment( { 0, 0 }, { 10 * MM, 0 } );
    PCB_SHAPE* b = addSegment( { 10 * MM, 0 }, { 10 * MM, 10 * MM } );
    addCoincident( a, CONSTRAINT_ANCHOR::END, b, CONSTRAINT_ANCHOR::START );

    CONSTRAINT_MEMBER             dragged( a->m_Uuid, CONSTRAINT_ANCHOR::END );
    BOARD_CONSTRAINT_DRAG_SESSION session;
    std::vector<PCB_SHAPE*>       modified;
    BOARD_COMMIT                  commit( tool );
    const VECTOR2I                originalAEnd = a->GetEnd();
    const VECTOR2I                originalBStart = b->GetStart();

    BOOST_REQUIRE( session.Build( &board, dragged ) );
    BOOST_CHECK( session.Matches( dragged ) );
    commit.Modify( a );

    CONSTRAINT_DIAGNOSIS first = session.Solve(
            { 12 * MM, 2 * MM }, &modified,
            [&]( BOARD_ITEM* aItem )
            {
                commit.Modify( aItem );
            },
            false, false );
    BOOST_REQUIRE( first.solved );
    BOOST_CHECK_LE( ( a->GetEnd() - b->GetStart() ).EuclideanNorm(), 100 );

    CONSTRAINT_DIAGNOSIS second = session.Solve(
            { 14 * MM, 4 * MM }, &modified,
            [&]( BOARD_ITEM* aItem )
            {
                commit.Modify( aItem );
            },
            false, false );
    BOOST_REQUIRE( second.solved );
    BOOST_CHECK_LE( ( a->GetEnd() - b->GetStart() ).EuclideanNorm(), 100 );
    BOOST_CHECK_LE( ( a->GetEnd() - VECTOR2I( 14 * MM, 4 * MM ) ).EuclideanNorm(), 100 );

    commit.Revert();
    BOOST_CHECK_EQUAL( a->GetEnd(), originalAEnd );
    BOOST_CHECK_EQUAL( b->GetStart(), originalBStart );
}


BOOST_FIXTURE_TEST_CASE( PersistentDragSessionRejectsInexactSnapTarget, DRAG_FIXTURE )
{
    PCB_SHAPE* seg = addSegment( { 0, 0 }, { 10 * MM, 0 } );

    PCB_CONSTRAINT* length = new PCB_CONSTRAINT( &board, PCB_CONSTRAINT_TYPE::FIXED_LENGTH );
    length->AddMember( seg->m_Uuid, CONSTRAINT_ANCHOR::WHOLE );
    length->SetValue( 10.0 * MM );
    board.Add( length );

    BOARD_CONSTRAINT_DRAG_SESSION session;
    BOOST_REQUIRE( session.Build( &board, CONSTRAINT_MEMBER( seg->m_Uuid, CONSTRAINT_ANCHOR::END ) ) );
    BOOST_CHECK( session.IsExactFeasible( { 10 * MM, 0 } ) );
    BOOST_CHECK( session.IsExactFeasible( { 10 * MM, 0 } ) );
    BOOST_CHECK( !session.IsExactFeasible( { 20 * MM, 0 } ) );
    BOOST_CHECK_EQUAL( seg->GetEnd(), VECTOR2I( 10 * MM, 0 ) );
}


BOOST_FIXTURE_TEST_CASE( PersistentDragSessionExactifiesEqualGapTarget, DRAG_FIXTURE )
{
    PCB_SHAPE* seg = addSegment( { 0, 0 }, { 10 * MM, 0 } );

    PCB_CONSTRAINT* length = new PCB_CONSTRAINT( &board, PCB_CONSTRAINT_TYPE::FIXED_LENGTH );
    length->AddMember( seg->m_Uuid, CONSTRAINT_ANCHOR::WHOLE );
    length->SetValue( 10.0 * MM );
    board.Add( length );

    PCB_CONSTRAINT* horizontal = new PCB_CONSTRAINT( &board, PCB_CONSTRAINT_TYPE::HORIZONTAL );
    horizontal->AddMember( seg->m_Uuid, CONSTRAINT_ANCHOR::WHOLE );
    board.Add( horizontal );

    BOARD_CONSTRAINT_DRAG_SESSION session;
    BOOST_REQUIRE( session.Build( &board, CONSTRAINT_MEMBER( seg->m_Uuid, CONSTRAINT_ANCHOR::END ) ) );

    SNAP_SOURCE_CONTEXT context;
    context.sourcePoint = { 10 * MM, 0 };
    SNAP_CANDIDATE gap =
            SNAP_CANDIDATE::AxisX( testSnapId( "gap", SNAP_ID_KIND::EQUAL_GAP_X ), SNAP_PRIORITY_TIER::OBJECT,
                                   SNAP_CANDIDATE_SUBTYPE::BBOX_LAYOUT, 10 * MM, 0.0 );
    gap.relation = SNAP_RELATION::BBOX_EQUAL_GAP;

    SNAP_RESULT result = session.ResolveCandidates( context, { gap } );
    BOOST_REQUIRE( result.status == SNAP_RESULT_STATUS::SUCCESS );
    BOOST_REQUIRE_EQUAL( result.quantizedResiduals.size(), 1 );
    BOOST_CHECK_EQUAL( result.quantizedResiduals.front(), 0.0 );
    BOOST_CHECK_EQUAL( seg->GetEnd(), VECTOR2I( 10 * MM, 0 ) );
}


BOOST_FIXTURE_TEST_CASE( PersistentMoveSessionArbitratesBeforeApplying, DRAG_FIXTURE )
{
    PCB_SHAPE* moved = addSegment( { 0, 0 }, { 10 * MM, 0 } );
    PCB_SHAPE* neighbor = addSegment( { 10 * MM, 0 }, { 20 * MM, 0 } );
    addCoincident( moved, CONSTRAINT_ANCHOR::END, neighbor, CONSTRAINT_ANCHOR::START );

    BOARD_CONSTRAINT_MOVE_SESSION session;
    BOOST_REQUIRE( session.Build( &board, { moved }, { 0, 0 } ) );

    SNAP_SOURCE_CONTEXT context;
    context.sourcePoint = { 5 * MM, 0 };
    SNAP_CANDIDATE target = SNAP_CANDIDATE::Point( testSnapId( "target" ), SNAP_PRIORITY_TIER::OBJECT,
                                                   SNAP_CANDIDATE_SUBTYPE::INTRINSIC_ANCHOR, context.sourcePoint, 0.0 );

    // A feasibility callback reports the residual and the freedom consumed; SNAP_RESOLVER owns
    // the accepted list, so honouring a two-degree candidate shows up as zero freedom left.
    SNAP_RESULT result = session.ResolveCandidates( context, { target } );
    BOOST_REQUIRE( result.status == SNAP_RESULT_STATUS::SUCCESS );
    BOOST_CHECK_EQUAL( result.position, context.sourcePoint );
    BOOST_REQUIRE_EQUAL( result.quantizedResiduals.size(), 1 );
    BOOST_CHECK_EQUAL( result.quantizedResiduals.front(), 0.0 );
    BOOST_CHECK_EQUAL( result.remainingDof, 0 );
    BOOST_CHECK_EQUAL( moved->GetStart(), VECTOR2I( 0, 0 ) );
    BOOST_CHECK_EQUAL( neighbor->GetStart(), VECTOR2I( 10 * MM, 0 ) );

    std::vector<PCB_SHAPE*> modified;
    BOOST_REQUIRE( session.Solve( result.position, &modified, {} ) );
    BOOST_CHECK_EQUAL( moved->GetStart(), VECTOR2I( 5 * MM, 0 ) );
    BOOST_CHECK_EQUAL( moved->GetEnd(), VECTOR2I( 15 * MM, 0 ) );
    BOOST_CHECK_EQUAL( neighbor->GetStart(), VECTOR2I( 15 * MM, 0 ) );
}


BOOST_FIXTURE_TEST_CASE( RigidTranslationPreservesRoundShapeRadii, DRAG_FIXTURE )
{
    PCB_SHAPE* circle = addCircle( { 0, 0 }, 4 * MM );
    PCB_SHAPE* arc = addArc( { 20 * MM, 0 }, { 17 * MM, 3 * MM }, { 14 * MM, 0 } );
    PCB_SHAPE* ellipse = new PCB_SHAPE( &board, SHAPE_T::ELLIPSE );
    ellipse->SetEllipseCenter( { 30 * MM, 0 } );
    ellipse->SetEllipseMajorRadius( 6 * MM );
    ellipse->SetEllipseMinorRadius( 3 * MM );
    board.Add( ellipse );

    const int circleRadius = circle->GetRadius();
    const int arcRadius = arc->GetRadius();
    const int ellipseMinorRadius = ellipse->GetEllipseMinorRadius();

    BOARD_CONSTRAINT_ADAPTER adapter;
    BOOST_REQUIRE( adapter.Build( { circle, arc, ellipse }, {} ) );
    BOOST_REQUIRE(
            adapter.SolveRigidTranslation( { circle->m_Uuid, arc->m_Uuid, ellipse->m_Uuid }, { 3 * MM, 2 * MM } ) );
    adapter.Apply();

    BOOST_CHECK_EQUAL( circle->GetRadius(), circleRadius );
    BOOST_CHECK_EQUAL( arc->GetRadius(), arcRadius );
    BOOST_CHECK_EQUAL( ellipse->GetEllipseMinorRadius(), ellipseMinorRadius );
}


BOOST_AUTO_TEST_CASE( UnbuiltMoveSessionRejectsCandidates )
{
    BOARD_CONSTRAINT_MOVE_SESSION session;
    SNAP_SOURCE_CONTEXT           context;
    context.sourcePoint = { 10, 20 };
    SNAP_CANDIDATE candidate = SNAP_CANDIDATE::Point( testSnapId( "point" ), SNAP_PRIORITY_TIER::OBJECT,
                                                      SNAP_CANDIDATE_SUBTYPE::INTRINSIC_ANCHOR, { 10, 20 }, 0.0 );

    SNAP_RESULT result = session.ResolveCandidates( context, { candidate } );

    BOOST_CHECK( result.status == SNAP_RESULT_STATUS::INCOMPATIBLE );
}


BOOST_FIXTURE_TEST_CASE( PersistentMoveSessionRejectsIncompatibleTarget, DRAG_FIXTURE )
{
    PCB_SHAPE* moved = addSegment( { 0, 0 }, { 10 * MM, 0 } );
    PCB_SHAPE* locked = addSegment( { 10 * MM, 0 }, { 20 * MM, 0 } );
    locked->SetLocked( true );
    addCoincident( moved, CONSTRAINT_ANCHOR::END, locked, CONSTRAINT_ANCHOR::START );

    BOARD_CONSTRAINT_MOVE_SESSION session;
    BOOST_REQUIRE( session.Build( &board, { moved }, { 0, 0 } ) );

    SNAP_SOURCE_CONTEXT context;
    context.sourcePoint = { 5 * MM, 0 };
    SNAP_CANDIDATE target = SNAP_CANDIDATE::Point( testSnapId( "blocked" ), SNAP_PRIORITY_TIER::OBJECT,
                                                   SNAP_CANDIDATE_SUBTYPE::INTRINSIC_ANCHOR, context.sourcePoint, 0.0 );

    BOOST_CHECK( session.ResolveCandidates( context, { target } ).status == SNAP_RESULT_STATUS::INCOMPATIBLE );
    BOOST_CHECK_EQUAL( moved->GetStart(), VECTOR2I( 0, 0 ) );
    BOOST_CHECK_EQUAL( locked->GetStart(), VECTOR2I( 10 * MM, 0 ) );
}


BOOST_FIXTURE_TEST_CASE( PersistentMoveSessionFallsBackAfterRejectedObject, DRAG_FIXTURE )
{
    PCB_SHAPE* moved = addSegment( { 0, 0 }, { 10 * MM, 0 } );
    PCB_SHAPE* locked = addSegment( { 10 * MM, 0 }, { 20 * MM, 0 } );
    locked->SetLocked( true );
    addCoincident( moved, CONSTRAINT_ANCHOR::END, locked, CONSTRAINT_ANCHOR::START );

    auto session = std::make_shared<BOARD_CONSTRAINT_MOVE_SESSION>();
    BOOST_REQUIRE( session->Build( &board, { moved }, { 0, 0 } ) );

    SNAP_SOURCE_CONTEXT context;
    context.sourcePoint = { 5 * MM, 0 };
    SNAP_STABLE_ID objectId = testSnapId( "blocked" );
    SNAP_STABLE_ID gridId = testSnapId( "grid-x", SNAP_ID_KIND::GRID_X );
    SNAP_RESOLVER  resolver;
    resolver.SetFeasibilityCallback(
            [session]( const SNAP_SOURCE_CONTEXT& aContext, const std::vector<SNAP_CANDIDATE>& aCandidates )
            {
                return session->ResolveCandidates( aContext, aCandidates );
            } );
    resolver.AddCandidate( SNAP_CANDIDATE::Point( objectId, SNAP_PRIORITY_TIER::OBJECT,
                                                  SNAP_CANDIDATE_SUBTYPE::INTRINSIC_ANCHOR, context.sourcePoint,
                                                  0.0 ) );
    resolver.AddCandidate(
            SNAP_CANDIDATE::AxisX( gridId, SNAP_PRIORITY_TIER::GRID, SNAP_CANDIDATE_SUBTYPE::GRID_AXIS, 0, 1.0 ) );

    SNAP_RESULT result = resolver.Resolve( context );
    BOOST_REQUIRE( result.status == SNAP_RESULT_STATUS::SUCCESS );
    BOOST_CHECK( !result.Accepted( objectId ) );
    BOOST_CHECK( result.Accepted( gridId ) );
    BOOST_CHECK_EQUAL( result.position, VECTOR2I( 0, 0 ) );
}


BOOST_FIXTURE_TEST_CASE( PersistentMoveSessionReportsEachAcceptedSnapOnce, DRAG_FIXTURE )
{
    PCB_SHAPE* moved = addSegment( { 0, 0 }, { 10 * MM, 0 } );
    PCB_SHAPE* neighbor = addSegment( { 10 * MM, 0 }, { 20 * MM, 0 } );
    addCoincident( moved, CONSTRAINT_ANCHOR::END, neighbor, CONSTRAINT_ANCHOR::START );

    auto session = std::make_shared<BOARD_CONSTRAINT_MOVE_SESSION>();
    BOOST_REQUIRE( session->Build( &board, { moved }, { 0, 0 } ) );

    SNAP_SOURCE_CONTEXT context;
    context.sourcePoint = { 5 * MM, 0 };
    SNAP_STABLE_ID targetId = testSnapId( "target" );
    SNAP_CANDIDATE target = SNAP_CANDIDATE::Point( targetId, SNAP_PRIORITY_TIER::OBJECT,
                                                   SNAP_CANDIDATE_SUBTYPE::INTRINSIC_ANCHOR, context.sourcePoint, 0.0 );
    target.guides.push_back( { { 0, 0 }, context.sourcePoint } );

    SNAP_RESOLVER resolver;
    resolver.SetFeasibilityCallback(
            [session]( const SNAP_SOURCE_CONTEXT& aContext, const std::vector<SNAP_CANDIDATE>& aCandidates )
            {
                return session->ResolveCandidates( aContext, aCandidates );
            } );
    resolver.AddCandidate( target );

    // The resolver appends the id and the guides itself.  A session that reported them too would
    // enter the snap in the sticky set twice and draw its guide twice every frame.
    SNAP_RESULT result = resolver.Resolve( context );
    BOOST_REQUIRE( result.status == SNAP_RESULT_STATUS::SUCCESS );
    BOOST_CHECK_EQUAL( std::count( result.accepted.begin(), result.accepted.end(), targetId ), 1 );
    BOOST_CHECK_EQUAL( result.guides.size(), 1 );
}


BOOST_FIXTURE_TEST_CASE( PersistentMoveSessionJointlySolvesAuthoredAndObjectManifolds, DRAG_FIXTURE )
{
    PCB_SHAPE* moved = addSegment( { 0, 0 }, { 10 * MM, 0 } );
    PCB_SHAPE* guide = addSegment( { -20 * MM, 0 }, { 20 * MM, 0 } );
    guide->SetLocked( true );

    PCB_CONSTRAINT* onLine = new PCB_CONSTRAINT( &board, PCB_CONSTRAINT_TYPE::POINT_ON_LINE );
    onLine->AddMember( moved->m_Uuid, CONSTRAINT_ANCHOR::START );
    onLine->AddMember( guide->m_Uuid, CONSTRAINT_ANCHOR::WHOLE );
    board.Add( onLine );

    BOARD_CONSTRAINT_MOVE_SESSION session;
    BOOST_REQUIRE( session.Build( &board, { moved }, { 0, 0 } ) );

    SNAP_SOURCE_CONTEXT context;
    context.sourcePoint = { 5 * MM, 3 * MM };
    SNAP_CANDIDATE vertical = SNAP_CANDIDATE::Line( testSnapId( "vertical" ), SNAP_PRIORITY_TIER::OBJECT,
                                                    SNAP_CANDIDATE_SUBTYPE::FINITE_MANIFOLD, { 5 * MM, -20 * MM },
                                                    { 0.0, 40.0 * MM }, 0.0 );

    SNAP_RESULT base = session.ResolveCandidates( context, {} );
    BOOST_REQUIRE( base.status == SNAP_RESULT_STATUS::SUCCESS );
    BOOST_CHECK_EQUAL( base.position, VECTOR2I( 5 * MM, 0 ) );

    SNAP_RESULT result = session.ResolveCandidates( context, { vertical } );
    BOOST_REQUIRE( result.status == SNAP_RESULT_STATUS::SUCCESS );
    BOOST_REQUIRE_EQUAL( result.quantizedResiduals.size(), 1 );
    BOOST_CHECK_EQUAL( result.quantizedResiduals.front(), 0.0 );
    BOOST_CHECK_EQUAL( result.remainingDof, 1 );
    BOOST_CHECK_EQUAL( result.position, VECTOR2I( 5 * MM, 0 ) );
}


BOOST_FIXTURE_TEST_CASE( PersistentMoveSessionJointlySolvesDisconnectedClusters, DRAG_FIXTURE )
{
    PCB_SHAPE* first = addSegment( { 0, 0 }, { 10 * MM, 0 } );
    PCB_SHAPE* firstGuide = addSegment( { -100 * MM, 0 }, { 100 * MM, 0 } );
    PCB_SHAPE* second = addSegment( { 0, 10 * MM }, { 10 * MM, 10 * MM } );
    PCB_SHAPE* secondGuide = addSegment( { -100 * MM, 9 * MM }, { 100 * MM, 11 * MM } );
    firstGuide->SetLocked( true );
    secondGuide->SetLocked( true );

    PCB_CONSTRAINT* firstOnLine = new PCB_CONSTRAINT( &board, PCB_CONSTRAINT_TYPE::POINT_ON_LINE );
    firstOnLine->AddMember( first->m_Uuid, CONSTRAINT_ANCHOR::START );
    firstOnLine->AddMember( firstGuide->m_Uuid, CONSTRAINT_ANCHOR::WHOLE );
    board.Add( firstOnLine );

    PCB_CONSTRAINT* secondOnLine = new PCB_CONSTRAINT( &board, PCB_CONSTRAINT_TYPE::POINT_ON_LINE );
    secondOnLine->AddMember( second->m_Uuid, CONSTRAINT_ANCHOR::START );
    secondOnLine->AddMember( secondGuide->m_Uuid, CONSTRAINT_ANCHOR::WHOLE );
    board.Add( secondOnLine );

    BOARD_CONSTRAINT_MOVE_SESSION session;
    BOOST_REQUIRE( session.Build( &board, { first, second }, { 0, 0 } ) );

    SNAP_SOURCE_CONTEXT context;
    context.sourcePoint = { 20 * MM, 5 * MM };
    SNAP_RESULT result = session.ResolveCandidates( context, {} );

    BOOST_REQUIRE( result.status == SNAP_RESULT_STATUS::SUCCESS );
    BOOST_CHECK_EQUAL( result.position, VECTOR2I( 0, 0 ) );
}


BOOST_FIXTURE_TEST_CASE( PersistentDragSessionFindsJointlyFeasibleManifoldPoint, DRAG_FIXTURE )
{
    PCB_SHAPE* seg = addSegment( { 0, 0 }, { 10 * MM, 0 } );

    PCB_CONSTRAINT* horizontal = new PCB_CONSTRAINT( &board, PCB_CONSTRAINT_TYPE::HORIZONTAL );
    horizontal->AddMember( seg->m_Uuid, CONSTRAINT_ANCHOR::WHOLE );
    board.Add( horizontal );

    BOARD_CONSTRAINT_DRAG_SESSION session;
    BOOST_REQUIRE( session.Build( &board, CONSTRAINT_MEMBER( seg->m_Uuid, CONSTRAINT_ANCHOR::END ) ) );

    SNAP_SOURCE_CONTEXT context;
    context.sourcePoint = { 6 * MM, 7 * MM };
    SNAP_CANDIDATE manifold = SNAP_CANDIDATE::Line( testSnapId( "vertical" ), SNAP_PRIORITY_TIER::OBJECT,
                                                    SNAP_CANDIDATE_SUBTYPE::FINITE_MANIFOLD, { 6 * MM, -20 * MM },
                                                    { 0.0, 40.0 * MM }, 0.0 );

    SNAP_RESULT result = session.ResolveCandidates( context, { manifold } );

    BOOST_REQUIRE( result.status == SNAP_RESULT_STATUS::SUCCESS );
    BOOST_CHECK_EQUAL( result.position.x, 6 * MM );
    BOOST_CHECK_LE( std::abs( result.position.y ), 1000 );
    BOOST_CHECK_EQUAL( seg->GetEnd(), VECTOR2I( 10 * MM, 0 ) );

    manifold.origin = { -20 * MM, 20 * MM };
    manifold.direction = { 40.0 * MM, 0.0 };
    BOOST_CHECK( session.ResolveCandidates( context, { manifold } ).status == SNAP_RESULT_STATUS::INCOMPATIBLE );
}


BOOST_FIXTURE_TEST_CASE( PersistentDragSessionPreservesGeometryOnBaseConflict, DRAG_FIXTURE )
{
    PCB_SHAPE* seg = addSegment( { 0, 0 }, { 12 * MM, 0 } );

    PCB_CONSTRAINT* length = new PCB_CONSTRAINT( &board, PCB_CONSTRAINT_TYPE::FIXED_LENGTH );
    length->AddMember( seg->m_Uuid, CONSTRAINT_ANCHOR::WHOLE );
    length->SetValue( 10.0 * MM );
    board.Add( length );

    BOARD_CONSTRAINT_DRAG_SESSION session;
    BOOST_REQUIRE( session.Build( &board, CONSTRAINT_MEMBER( seg->m_Uuid, CONSTRAINT_ANCHOR::END ) ) );

    seg->SetEnd( { 20 * MM, 0 } );
    CONSTRAINT_DIAGNOSIS diagnosis = session.Solve( { 20 * MM, 0 }, nullptr, {}, false, false );

    BOOST_CHECK( !diagnosis.solved );
    BOOST_CHECK_EQUAL( seg->GetEnd(), VECTOR2I( 12 * MM, 0 ) );
    BOOST_CHECK( !session.IsExactFeasible( { 12 * MM, 0 } ) );
}


// A whole cluster inside the moved selection travels rigidly, so a constraint set it already
// could not satisfy must not block the move.
BOOST_FIXTURE_TEST_CASE( WholeClusterMoveSurvivesPreExistingConflict, DRAG_FIXTURE )
{
    // Two sides of a closed outline, corner tied, both marked horizontal. Adjacent sides cannot
    // both be horizontal, which is the state this board arrived in
    PCB_SHAPE* bottom = addSegment( { 0, 0 }, { 10 * MM, 0 } );
    PCB_SHAPE* right = addSegment( { 10 * MM, 0 }, { 10 * MM, 10 * MM } );

    PCB_CONSTRAINT* corner = new PCB_CONSTRAINT( &board, PCB_CONSTRAINT_TYPE::COINCIDENT );
    corner->AddMember( bottom->m_Uuid, CONSTRAINT_ANCHOR::END );
    corner->AddMember( right->m_Uuid, CONSTRAINT_ANCHOR::START );
    board.Add( corner );

    for( PCB_SHAPE* seg : { bottom, right } )
    {
        PCB_CONSTRAINT* horizontal = new PCB_CONSTRAINT( &board, PCB_CONSTRAINT_TYPE::HORIZONTAL );
        horizontal->AddMember( seg->m_Uuid, CONSTRAINT_ANCHOR::WHOLE );
        board.Add( horizontal );
    }

    BOARD_CONSTRAINT_MOVE_SESSION session;
    BOOST_REQUIRE( session.Build( &board, { bottom, right }, { 0, 0 } ) );

    std::vector<PCB_SHAPE*> modified;
    BOOST_REQUIRE( session.Solve( { 5 * MM, 5 * MM }, &modified, {} ) );

    BOOST_CHECK_EQUAL( bottom->GetStart(), VECTOR2I( 5 * MM, 5 * MM ) );
    BOOST_CHECK_EQUAL( right->GetEnd(), VECTOR2I( 15 * MM, 15 * MM ) );
}


// A pinned point cannot travel, so a whole-cluster move that would carry it off is still refused
// and the geometry is left where it was.
BOOST_FIXTURE_TEST_CASE( WholeClusterMoveWithFixedPoint, DRAG_FIXTURE )
{
    PCB_SHAPE* bottom = addSegment( { 0, 0 }, { 10 * MM, 0 } );
    PCB_SHAPE* right = addSegment( { 10 * MM, 0 }, { 10 * MM, 10 * MM } );

    PCB_CONSTRAINT* corner = new PCB_CONSTRAINT( &board, PCB_CONSTRAINT_TYPE::COINCIDENT );
    corner->AddMember( bottom->m_Uuid, CONSTRAINT_ANCHOR::END );
    corner->AddMember( right->m_Uuid, CONSTRAINT_ANCHOR::START );
    board.Add( corner );

    PCB_CONSTRAINT* pinned = new PCB_CONSTRAINT( &board, PCB_CONSTRAINT_TYPE::FIXED_POSITION );
    pinned->AddMember( bottom->m_Uuid, CONSTRAINT_ANCHOR::START );
    board.Add( pinned );

    BOARD_CONSTRAINT_MOVE_SESSION session;
    BOOST_REQUIRE( session.Build( &board, { bottom, right }, { 0, 0 } ) );

    std::vector<PCB_SHAPE*> modified;

    BOOST_CHECK( !session.Solve( { 5 * MM, 5 * MM }, &modified, {} ) );
    BOOST_CHECK_EQUAL( bottom->GetStart(), VECTOR2I( 0, 0 ) );
    BOOST_CHECK_EQUAL( right->GetEnd(), VECTOR2I( 10 * MM, 10 * MM ) );
}


// A pin grounds its own point and nothing else, so a free corner in the same cluster still drags.
BOOST_FIXTURE_TEST_CASE( DragFreeCornerBesideFixedPoint, DRAG_FIXTURE )
{
    PCB_SHAPE* bottom = addSegment( { 0, 0 }, { 10 * MM, 0 } );
    PCB_SHAPE* right = addSegment( { 10 * MM, 0 }, { 10 * MM, 10 * MM } );

    addCoincident( bottom, CONSTRAINT_ANCHOR::END, right, CONSTRAINT_ANCHOR::START );

    PCB_CONSTRAINT* pinned = new PCB_CONSTRAINT( &board, PCB_CONSTRAINT_TYPE::FIXED_POSITION );
    pinned->AddMember( bottom->m_Uuid, CONSTRAINT_ANCHOR::START );
    board.Add( pinned );

    BOARD_CONSTRAINT_DRAG_SESSION session;
    BOOST_REQUIRE( session.Build( &board, CONSTRAINT_MEMBER( right->m_Uuid, CONSTRAINT_ANCHOR::START ) ) );
    BOOST_REQUIRE( !session.HasBaseConflict() );

    right->SetStart( { 12 * MM, 3 * MM } );
    CONSTRAINT_DIAGNOSIS diagnosis = session.Solve( { 12 * MM, 3 * MM }, nullptr, {}, false, false );

    BOOST_CHECK( diagnosis.solved );
    BOOST_CHECK( diagnosis.conflicting.empty() );
    BOOST_CHECK_LE( ( right->GetStart() - VECTOR2I( 12 * MM, 3 * MM ) ).EuclideanNorm(), 1000 );
    BOOST_CHECK_LE( ( bottom->GetEnd() - right->GetStart() ).EuclideanNorm(), 1000 );
    BOOST_CHECK_EQUAL( bottom->GetStart(), VECTOR2I( 0, 0 ) );
}


// Standing still is always feasible, so a pinned cluster keeps its snapping instead of reporting
// itself unusable on every frame.
BOOST_FIXTURE_TEST_CASE( PinnedClusterStaysSnappableAtRest, DRAG_FIXTURE )
{
    PCB_SHAPE* bottom = addSegment( { 0, 0 }, { 10 * MM, 0 } );
    PCB_SHAPE* right = addSegment( { 10 * MM, 0 }, { 10 * MM, 10 * MM } );

    addCoincident( bottom, CONSTRAINT_ANCHOR::END, right, CONSTRAINT_ANCHOR::START );

    PCB_CONSTRAINT* pinned = new PCB_CONSTRAINT( &board, PCB_CONSTRAINT_TYPE::FIXED_POSITION );
    pinned->AddMember( bottom->m_Uuid, CONSTRAINT_ANCHOR::START );
    board.Add( pinned );

    BOARD_CONSTRAINT_MOVE_SESSION session;
    BOOST_REQUIRE( session.Build( &board, { bottom, right }, { 0, 0 } ) );

    SNAP_SOURCE_CONTEXT context;
    context.sourcePoint = { 0, 0 };
    SNAP_RESULT result = session.ResolveCandidates( context, {} );

    BOOST_CHECK( result.status == SNAP_RESULT_STATUS::SUCCESS );
    BOOST_CHECK_EQUAL( result.position, VECTOR2I( 0, 0 ) );
}


// Moving part of a cluster still works when the pin sits on a shape left behind.
BOOST_FIXTURE_TEST_CASE( PartialClusterMoveBesideFixedPoint, DRAG_FIXTURE )
{
    PCB_SHAPE* bottom = addSegment( { 0, 0 }, { 10 * MM, 0 } );
    PCB_SHAPE* right = addSegment( { 10 * MM, 0 }, { 10 * MM, 10 * MM } );

    addCoincident( bottom, CONSTRAINT_ANCHOR::END, right, CONSTRAINT_ANCHOR::START );

    PCB_CONSTRAINT* pinned = new PCB_CONSTRAINT( &board, PCB_CONSTRAINT_TYPE::FIXED_POSITION );
    pinned->AddMember( bottom->m_Uuid, CONSTRAINT_ANCHOR::START );
    board.Add( pinned );

    BOARD_CONSTRAINT_MOVE_SESSION session;
    BOOST_REQUIRE( session.Build( &board, { right }, { 10 * MM, 0 } ) );

    std::vector<PCB_SHAPE*> moved;
    BOOST_REQUIRE( session.Solve( { 13 * MM, 4 * MM }, &moved, {} ) );

    BOOST_CHECK_EQUAL( right->GetStart(), VECTOR2I( 13 * MM, 4 * MM ) );
    BOOST_CHECK_EQUAL( bottom->GetStart(), VECTOR2I( 0, 0 ) );
    BOOST_CHECK_LE( ( bottom->GetEnd() - right->GetStart() ).EuclideanNorm(), 1000 );
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


// Mouse-up pushes the last painted preview without another solve.  The pushed transaction must
// therefore preserve every coordinate that was visible in the final preview frame.
BOOST_FIXTURE_TEST_CASE( MoveCommitEqualsLastPaintedPreview, DRAG_FIXTURE )
{
    PCB_SHAPE* a = addSegment( { 0, 0 }, { 10 * MM, 0 } );
    PCB_SHAPE* b = addSegment( { 10 * MM, 0 }, { 10 * MM, 10 * MM } );

    addCoincident( a, CONSTRAINT_ANCHOR::END, b, CONSTRAINT_ANCHOR::START );

    BOARD_COMMIT            commit( tool );
    std::vector<PCB_SHAPE*> modified;

    commit.Modify( a );
    a->Move( { 2 * MM, 4 * MM } );

    ReSolveShapeClusters( &board, { a }, &modified,
                          [&]( BOARD_ITEM* aItem )
                          {
                              commit.Modify( aItem );
                          } );

    const std::array<VECTOR2I, 4> painted = { a->GetStart(), a->GetEnd(), b->GetStart(), b->GetEnd() };

    commit.Push( wxT( "Move" ) );

    const std::array<VECTOR2I, 4> committed = { a->GetStart(), a->GetEnd(), b->GetStart(), b->GetEnd() };

    BOOST_CHECK_EQUAL_COLLECTIONS( committed.begin(), committed.end(), painted.begin(), painted.end() );
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
} // namespace


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
    const int      radius0 = arc->GetRadius();

    std::vector<PCB_SHAPE*> modified;
    BOARD_COMMIT            commit( tool );
    SolveCluster( &board, { arc->m_Uuid, CONSTRAINT_ANCHOR::CENTER }, { 1 * MM, 1 * MM }, &modified,
                  [&]( BOARD_ITEM* aItem )
                  {
                      commit.Modify( aItem );
                  } );

    BOOST_CHECK_LE( ( arc->GetStart() - start0 ).EuclideanNorm(), 20000.0 );
    BOOST_CHECK_LE( ( arc->GetEnd() - end0 ).EuclideanNorm(), 20000.0 );
    BOOST_CHECK( ( arc->GetCenter() - VECTOR2I( 0, 0 ) ).EuclideanNorm() > 100000.0 );
    BOOST_CHECK_GT( std::abs( arc->GetRadius() - radius0 ), 100000 );
}


// Picking the very point a FIXED_POSITION constraint nails down. The drag writes the cursor
// into the shape before the solve, so the solve has to put that point back.
BOOST_FIXTURE_TEST_CASE( BlockEditHoldsAFixedPositionAnchor, DRAG_FIXTURE )
{
    PCB_SHAPE* seg = addSegment( { 0, 0 }, { 10 * MM, 0 } );
    addFixedPosition( seg, CONSTRAINT_ANCHOR::START );

    BOARD_CONSTRAINT_BLOCK_SESSION session;
    BOOST_REQUIRE( session.Build( &board, { seg } ) );

    const VECTOR2I delta( 2 * MM, 3 * MM );
    BOARD_COMMIT   commit( tool );

    commit.Modify( seg );
    seg->SetStart( seg->GetStart() + delta );

    std::vector<PCB_SHAPE*> moved;
    CONSTRAINT_BLOCK_RESULT result =
            session.Solve( { { CONSTRAINT_MEMBER( seg->m_Uuid, CONSTRAINT_ANCHOR::START ), delta } }, {}, delta, &moved,
                           [&]( BOARD_ITEM* aItem )
                           {
                               commit.Modify( aItem );
                           } );

    BOOST_CHECK( result == CONSTRAINT_BLOCK_RESULT::SOLVED );
    BOOST_CHECK_EQUAL( seg->GetStart(), VECTOR2I( 0, 0 ) );
    BOOST_CHECK_EQUAL( seg->GetEnd(), VECTOR2I( 10 * MM, 0 ) );

    commit.Revert();
}


// The constraint holds one end, it does not freeze the shape. Picking both ends has to leave
// the free one exactly where the user put it.
BOOST_FIXTURE_TEST_CASE( BlockEditMovesAFreeEndWhileHoldingAFixedStart, DRAG_FIXTURE )
{
    PCB_SHAPE* seg = addSegment( { 0, 0 }, { 10 * MM, 0 } );
    addFixedPosition( seg, CONSTRAINT_ANCHOR::START );

    BOARD_CONSTRAINT_BLOCK_SESSION session;
    BOOST_REQUIRE( session.Build( &board, { seg } ) );

    const VECTOR2I delta( 2 * MM, 3 * MM );
    BOARD_COMMIT   commit( tool );

    commit.Modify( seg );
    seg->SetStart( seg->GetStart() + delta );
    seg->SetEnd( seg->GetEnd() + delta );

    std::vector<PCB_SHAPE*> moved;
    CONSTRAINT_BLOCK_RESULT result = session.Solve(
            { { CONSTRAINT_MEMBER( seg->m_Uuid, CONSTRAINT_ANCHOR::START ), delta },
              { CONSTRAINT_MEMBER( seg->m_Uuid, CONSTRAINT_ANCHOR::END ), VECTOR2I( 10 * MM, 0 ) + delta } },
            {}, delta, &moved,
            [&]( BOARD_ITEM* aItem )
            {
                commit.Modify( aItem );
            } );

    BOOST_CHECK( result == CONSTRAINT_BLOCK_RESULT::SOLVED );
    BOOST_CHECK_EQUAL( seg->GetStart(), VECTOR2I( 0, 0 ) );
    BOOST_CHECK_EQUAL( seg->GetEnd(), VECTOR2I( 12 * MM, 3 * MM ) );

    commit.Revert();
}


// A fixed point on one shape must not stop its coincident neighbour being dragged by a point
// of its own that nothing pins.
BOOST_FIXTURE_TEST_CASE( BlockEditStillMovesAFreeNeighborOfAFixedPoint, DRAG_FIXTURE )
{
    PCB_SHAPE* pinned = addSegment( { 0, 0 }, { 10 * MM, 0 } );
    PCB_SHAPE* free = addSegment( { 10 * MM, 0 }, { 20 * MM, 0 } );
    addFixedPosition( pinned, CONSTRAINT_ANCHOR::START );
    addCoincident( pinned, CONSTRAINT_ANCHOR::END, free, CONSTRAINT_ANCHOR::START );

    BOARD_CONSTRAINT_BLOCK_SESSION session;
    BOOST_REQUIRE( session.Build( &board, { free } ) );

    const VECTOR2I target( 22 * MM, 4 * MM );
    BOARD_COMMIT   commit( tool );

    commit.Modify( free );
    free->SetEnd( target );

    std::vector<PCB_SHAPE*> moved;
    CONSTRAINT_BLOCK_RESULT result =
            session.Solve( { { CONSTRAINT_MEMBER( free->m_Uuid, CONSTRAINT_ANCHOR::END ), target } }, {},
                           VECTOR2I( 2 * MM, 4 * MM ), &moved,
                           [&]( BOARD_ITEM* aItem )
                           {
                               commit.Modify( aItem );
                           } );

    BOOST_CHECK( result == CONSTRAINT_BLOCK_RESULT::SOLVED );
    BOOST_CHECK_EQUAL( free->GetEnd(), target );
    BOOST_CHECK_EQUAL( pinned->GetStart(), VECTOR2I( 0, 0 ) );

    commit.Revert();
}


// A shape that travels whole cannot take a pinned point along, so the gesture is refused
// rather than quietly dragging the point off its constraint. SolveRigidTranslation already
// refuses the same move for the stock drag.
BOOST_FIXTURE_TEST_CASE( BlockEditRefusesCarryingAFixedPoint, DRAG_FIXTURE )
{
    PCB_SHAPE* seg = addSegment( { 0, 0 }, { 10 * MM, 0 } );
    addFixedPosition( seg, CONSTRAINT_ANCHOR::START );

    BOARD_CONSTRAINT_BLOCK_SESSION session;
    BOOST_REQUIRE( session.Build( &board, { seg } ) );

    const VECTOR2I delta( 2 * MM, 3 * MM );
    BOARD_COMMIT   commit( tool );

    commit.Modify( seg );
    seg->Move( delta );

    std::vector<PCB_SHAPE*> moved;
    CONSTRAINT_BLOCK_RESULT result = session.Solve( {}, { seg->m_Uuid }, delta, &moved,
                                                    [&]( BOARD_ITEM* aItem )
                                                    {
                                                        commit.Modify( aItem );
                                                    } );

    BOOST_CHECK( result == CONSTRAINT_BLOCK_RESULT::REFUSED );

    commit.Revert();
}


// One relation the geometry already fails used to mark the whole cluster conflicted, and a
// conflicted cluster was not solved at all, so the relations beside it stopped holding too.
// Here the stale horizontal belongs to the second segment and the first one's has to survive.
BOOST_FIXTURE_TEST_CASE( BlockEditKeepsASatisfiedRelationBesideAStaleOne, DRAG_FIXTURE )
{
    PCB_SHAPE* holds = addSegment( { 0, 0 }, { 40 * MM, 0 } );
    PCB_SHAPE* stale = addSegment( { 40 * MM, 0 }, { 80 * MM, 10 * MM } );

    addCoincident( holds, CONSTRAINT_ANCHOR::END, stale, CONSTRAINT_ANCHOR::START );
    addHorizontal( holds );
    addHorizontal( stale ); // not satisfied, the segment rises 10 mm

    BOARD_CONSTRAINT_BLOCK_SESSION session;
    BOOST_REQUIRE( session.Build( &board, { holds, stale } ) );
    BOOST_REQUIRE( !session.HasBaseConflict() );

    const VECTOR2I target( 40 * MM, 8 * MM );
    BOARD_COMMIT   commit( tool );

    commit.Modify( holds );
    holds->SetEnd( target );

    std::vector<PCB_SHAPE*> moved;
    CONSTRAINT_BLOCK_RESULT result =
            session.Solve( { { CONSTRAINT_MEMBER( holds->m_Uuid, CONSTRAINT_ANCHOR::END ), target } }, {},
                           VECTOR2I( 0, 8 * MM ), &moved,
                           [&]( BOARD_ITEM* aItem )
                           {
                               commit.Modify( aItem );
                           } );

    // The cluster is modelled now rather than skipped
    BOOST_REQUIRE( result != CONSTRAINT_BLOCK_RESULT::NOT_MODELED );

    // The surviving horizontal held, so the unpicked start came along
    BOOST_CHECK_EQUAL( holds->GetStart().y, holds->GetEnd().y );

    commit.Revert();
}


// The stale relation is out of the system, so it must not refuse the drag either.
BOOST_FIXTURE_TEST_CASE( BlockEditDoesNotEnforceAStaleRelation, DRAG_FIXTURE )
{
    PCB_SHAPE* stale = addSegment( { 0, 0 }, { 40 * MM, 10 * MM } );
    addHorizontal( stale );

    BOARD_CONSTRAINT_BLOCK_SESSION session;
    BOOST_REQUIRE( session.Build( &board, { stale } ) );

    const VECTOR2I target( 40 * MM, 18 * MM );
    BOARD_COMMIT   commit( tool );

    commit.Modify( stale );
    stale->SetEnd( target );

    std::vector<PCB_SHAPE*> moved;
    CONSTRAINT_BLOCK_RESULT result =
            session.Solve( { { CONSTRAINT_MEMBER( stale->m_Uuid, CONSTRAINT_ANCHOR::END ), target } }, {},
                           VECTOR2I( 0, 8 * MM ), &moved,
                           [&]( BOARD_ITEM* aItem )
                           {
                               commit.Modify( aItem );
                           } );

    BOOST_CHECK( result != CONSTRAINT_BLOCK_RESULT::REFUSED );
    BOOST_CHECK_EQUAL( stale->GetEnd(), target );

    commit.Revert();
}


// A block drag reshapes the shape whose anchor was picked and carries the rest whole, which
// leaves the joint between them broken until the solver tidies up what the drag left free.
// Here only the loose end of the reshaped segment can move, and it has to follow the carried one.
BOOST_FIXTURE_TEST_CASE( BlockEditRelaxesUncommandedAnchorOfDeformedShape, DRAG_FIXTURE )
{
    PCB_SHAPE* deformed = addSegment( { 0, 0 }, { 10 * MM, 0 } );
    PCB_SHAPE* carried = addSegment( { 10 * MM, 0 }, { 20 * MM, 0 } );
    addCoincident( deformed, CONSTRAINT_ANCHOR::END, carried, CONSTRAINT_ANCHOR::START );

    BOARD_CONSTRAINT_BLOCK_SESSION session;
    BOOST_REQUIRE( session.Build( &board, { deformed, carried } ) );

    const VECTOR2I delta( 2 * MM, 3 * MM );
    BOARD_COMMIT   commit( tool );

    // What the drag writes before the solve. The picked anchor sits on its target, the carried
    // segment has moved whole, and the unpicked end has not moved
    commit.Modify( deformed );
    commit.Modify( carried );
    deformed->SetStart( deformed->GetStart() + delta );
    carried->Move( delta );

    std::vector<PCB_SHAPE*> moved;
    CONSTRAINT_BLOCK_RESULT result =
            session.Solve( { { CONSTRAINT_MEMBER( deformed->m_Uuid, CONSTRAINT_ANCHOR::START ), delta } },
                           { carried->m_Uuid }, delta, &moved,
                           [&]( BOARD_ITEM* aItem )
                           {
                               commit.Modify( aItem );
                           } );

    BOOST_CHECK( result == CONSTRAINT_BLOCK_RESULT::SOLVED );

    // Both placed points landed exactly where they were put
    BOOST_CHECK_EQUAL( deformed->GetStart(), VECTOR2I( 2 * MM, 3 * MM ) );
    BOOST_CHECK_EQUAL( carried->GetStart(), VECTOR2I( 12 * MM, 3 * MM ) );
    BOOST_CHECK_EQUAL( carried->GetEnd(), VECTOR2I( 22 * MM, 3 * MM ) );

    // The end nobody placed moved to the carried joint instead of staying where the reshape
    // left it, so the constraint holds and the reshaped segment ends up moved whole
    BOOST_CHECK_LE( ( deformed->GetEnd() - carried->GetStart() ).EuclideanNorm(), 100 );
    BOOST_CHECK( deformed->GetEnd() != VECTOR2I( 10 * MM, 0 ) );
    BOOST_CHECK( std::find( moved.begin(), moved.end(), deformed ) != moved.end() );

    commit.Revert();
}


// The common case. One picked anchor reshapes a shape whose constrained partner is not in the
// block at all, so the solver brings the partner along and reports it for the caller to stage.
BOOST_FIXTURE_TEST_CASE( BlockEditPullsAnUnselectedNeighborAlong, DRAG_FIXTURE )
{
    PCB_SHAPE* deformed = addSegment( { 0, 0 }, { 10 * MM, 0 } );
    PCB_SHAPE* neighbor = addSegment( { 10 * MM, 0 }, { 10 * MM, 10 * MM } );
    addCoincident( deformed, CONSTRAINT_ANCHOR::END, neighbor, CONSTRAINT_ANCHOR::START );

    BOARD_CONSTRAINT_BLOCK_SESSION session;
    BOOST_REQUIRE( session.Build( &board, { deformed } ) );

    const VECTOR2I target( 13 * MM, 4 * MM );
    const VECTOR2I neighborEnd0 = neighbor->GetEnd();
    BOARD_COMMIT   commit( tool );

    commit.Modify( deformed );
    deformed->SetEnd( target );

    std::vector<PCB_SHAPE*>  moved;
    std::vector<BOARD_ITEM*> staged;
    CONSTRAINT_BLOCK_RESULT  result =
            session.Solve( { { CONSTRAINT_MEMBER( deformed->m_Uuid, CONSTRAINT_ANCHOR::END ), target } }, {},
                           target - VECTOR2I( 10 * MM, 0 ), &moved,
                           [&]( BOARD_ITEM* aItem )
                           {
                               staged.push_back( aItem );
                               commit.Modify( aItem );
                           } );

    BOOST_CHECK( result == CONSTRAINT_BLOCK_RESULT::SOLVED );
    BOOST_CHECK_EQUAL( deformed->GetEnd(), target );
    BOOST_CHECK_LE( ( neighbor->GetStart() - target ).EuclideanNorm(), 100 );

    // Only the constrained end moved, because the weak pins hold the far end, and the caller
    // heard about the shape before it changed
    BOOST_CHECK( neighbor->GetStart() != VECTOR2I( 10 * MM, 0 ) );
    BOOST_CHECK_EQUAL( neighbor->GetEnd(), neighborEnd0 );
    BOOST_CHECK( std::find( moved.begin(), moved.end(), neighbor ) != moved.end() );
    BOOST_CHECK( std::find( staged.begin(), staged.end(), neighbor ) != staged.end() );

    commit.Revert();
}


// Both anchors of one segment picked while its constrained partner travels whole. Everything
// moves by the same offset, so the constraint moves with the block and the drag goes through.
BOOST_FIXTURE_TEST_CASE( BlockEditCarriesSpannedClusterWhole, DRAG_FIXTURE )
{
    PCB_SHAPE* picked = addSegment( { 0, 0 }, { 10 * MM, 0 } );
    PCB_SHAPE* carried = addSegment( { 10 * MM, 0 }, { 20 * MM, 0 } );
    addCoincident( picked, CONSTRAINT_ANCHOR::END, carried, CONSTRAINT_ANCHOR::START );

    BOARD_CONSTRAINT_BLOCK_SESSION session;
    BOOST_REQUIRE( session.Build( &board, { picked, carried } ) );

    const VECTOR2I delta( -4 * MM, 1 * MM );
    BOARD_COMMIT   commit( tool );

    commit.Modify( picked );
    commit.Modify( carried );
    picked->Move( delta );
    carried->Move( delta );

    std::vector<PCB_SHAPE*> moved;
    CONSTRAINT_BLOCK_RESULT result = session.Solve(
            { { CONSTRAINT_MEMBER( picked->m_Uuid, CONSTRAINT_ANCHOR::START ), delta },
              { CONSTRAINT_MEMBER( picked->m_Uuid, CONSTRAINT_ANCHOR::END ), VECTOR2I( 10 * MM, 0 ) + delta } },
            { carried->m_Uuid }, delta, &moved,
            [&]( BOARD_ITEM* aItem )
            {
                commit.Modify( aItem );
            } );

    BOOST_CHECK( result == CONSTRAINT_BLOCK_RESULT::SOLVED );
    BOOST_CHECK_EQUAL( picked->GetStart(), VECTOR2I( -4 * MM, 1 * MM ) );
    BOOST_CHECK_EQUAL( picked->GetEnd(), VECTOR2I( 6 * MM, 1 * MM ) );
    BOOST_CHECK_EQUAL( carried->GetStart(), VECTOR2I( 6 * MM, 1 * MM ) );

    commit.Revert();
}


// A placement the constraints forbid is refused and the geometry is left as it was. Here the
// carried segment's far end is tied to a locked neighbour the solver cannot move.
BOOST_FIXTURE_TEST_CASE( BlockEditRefusesCommandTiedToLockedNeighbor, DRAG_FIXTURE )
{
    PCB_SHAPE* carried = addSegment( { 0, 0 }, { 10 * MM, 0 } );
    PCB_SHAPE* anchored = addSegment( { 10 * MM, 0 }, { 10 * MM, 10 * MM } );
    anchored->SetLocked( true );
    addCoincident( carried, CONSTRAINT_ANCHOR::END, anchored, CONSTRAINT_ANCHOR::START );

    BOARD_CONSTRAINT_BLOCK_SESSION session;
    BOOST_REQUIRE( session.Build( &board, { carried } ) );

    const VECTOR2I delta( 3 * MM, 0 );
    const VECTOR2I anchoredStart0 = anchored->GetStart();
    BOARD_COMMIT   commit( tool );

    commit.Modify( carried );
    carried->Move( delta );

    std::vector<PCB_SHAPE*> moved;
    CONSTRAINT_BLOCK_RESULT result = session.Solve( {}, { carried->m_Uuid }, delta, &moved,
                                                    [&]( BOARD_ITEM* aItem )
                                                    {
                                                        commit.Modify( aItem );
                                                    } );

    BOOST_CHECK( result == CONSTRAINT_BLOCK_RESULT::REFUSED );
    BOOST_CHECK( moved.empty() );

    // The solve wrote nothing, so the locked neighbour is untouched and the carried geometry is
    // still exactly what the drag put on the board for the caller to undo
    BOOST_CHECK_EQUAL( anchored->GetStart(), anchoredStart0 );
    BOOST_CHECK_EQUAL( carried->GetStart(), VECTOR2I( 3 * MM, 0 ) );

    commit.Revert();
}


// The solver normally freezes a locked shape. With the lock override the drag carries it
// anyway, so its placed coordinates stand and the free neighbour follows them.
BOOST_FIXTURE_TEST_CASE( BlockEditCarriesALockedShapeUnderTheOverride, DRAG_FIXTURE )
{
    PCB_SHAPE* locked = addSegment( { 0, 0 }, { 10 * MM, 0 } );
    PCB_SHAPE* neighbor = addSegment( { 10 * MM, 0 }, { 10 * MM, 10 * MM } );
    locked->SetLocked( true );
    addCoincident( locked, CONSTRAINT_ANCHOR::END, neighbor, CONSTRAINT_ANCHOR::START );

    BOARD_CONSTRAINT_BLOCK_SESSION session;
    BOOST_REQUIRE( session.Build( &board, { locked } ) );

    const VECTOR2I delta( 3 * MM, 0 );
    BOARD_COMMIT   commit( tool );

    commit.Modify( locked );
    locked->Move( delta );

    std::vector<PCB_SHAPE*> moved;
    CONSTRAINT_BLOCK_RESULT result = session.Solve( {}, { locked->m_Uuid }, delta, &moved,
                                                    [&]( BOARD_ITEM* aItem )
                                                    {
                                                        commit.Modify( aItem );
                                                    } );

    BOOST_CHECK( result == CONSTRAINT_BLOCK_RESULT::SOLVED );
    BOOST_CHECK_EQUAL( locked->GetEnd(), VECTOR2I( 13 * MM, 0 ) );
    BOOST_CHECK_LE( ( neighbor->GetStart() - locked->GetEnd() ).EuclideanNorm(), 100 );

    commit.Revert();
}


// A dimension has no pickable handle, so a block carries it whole, and the solver freezes the
// dimension point no constraint binds. The carried offset still has to apply to both points, so
// the frame goes through and no point is left behind.
BOOST_FIXTURE_TEST_CASE( BlockEditCarriesABoundDimension, DRAG_FIXTURE )
{
    PCB_SHAPE* deformed = addSegment( { 0, 0 }, { 10 * MM, 0 } );

    PCB_DIM_ALIGNED* dim = new PCB_DIM_ALIGNED( &board, PCB_DIM_ALIGNED_T );
    dim->SetStart( { 10 * MM, 0 } );
    dim->SetEnd( { 10 * MM, 5 * MM } );
    dim->Update();
    board.Add( dim );

    PCB_CONSTRAINT* joint = new PCB_CONSTRAINT( &board, PCB_CONSTRAINT_TYPE::COINCIDENT );
    joint->AddMember( deformed->m_Uuid, CONSTRAINT_ANCHOR::END );
    joint->AddMember( dim->m_Uuid, CONSTRAINT_ANCHOR::START );
    board.Add( joint );

    BOARD_CONSTRAINT_BLOCK_SESSION session;
    BOOST_REQUIRE( session.Build( &board, { deformed } ) );

    const VECTOR2I delta( 2 * MM, 1 * MM );
    BOARD_COMMIT   commit( tool );

    commit.Modify( deformed );
    commit.Modify( dim );
    deformed->SetEnd( VECTOR2I( 10 * MM, 0 ) + delta );
    dim->Move( delta );

    CONSTRAINT_BLOCK_RESULT result = session.Solve(
            { { CONSTRAINT_MEMBER( deformed->m_Uuid, CONSTRAINT_ANCHOR::END ), VECTOR2I( 10 * MM, 0 ) + delta } },
            { dim->m_Uuid }, delta, nullptr,
            [&]( BOARD_ITEM* aItem )
            {
                commit.Modify( aItem );
            } );

    BOOST_CHECK( result == CONSTRAINT_BLOCK_RESULT::SOLVED );
    BOOST_CHECK_EQUAL( dim->GetStart(), VECTOR2I( 12 * MM, 1 * MM ) );
    BOOST_CHECK_EQUAL( dim->GetEnd(), VECTOR2I( 12 * MM, 6 * MM ) );

    commit.Revert();
}


// One picked polygon corner. The constrained corner follows the carried segment, the picked one
// lands exactly where it was put, and the corners nothing binds barely move.
BOOST_FIXTURE_TEST_CASE( BlockEditRelaxesOnlyBoundPolygonVertices, DRAG_FIXTURE )
{
    const std::vector<VECTOR2I> points{ { 0, 0 }, { 10 * MM, 0 }, { 10 * MM, 10 * MM }, { 0, 10 * MM } };

    PCB_SHAPE* poly = addPoly( points );
    PCB_SHAPE* carried = addSegment( { 0, 0 }, { 0, -10 * MM } );

    PCB_CONSTRAINT* joint = new PCB_CONSTRAINT( &board, PCB_CONSTRAINT_TYPE::COINCIDENT );
    joint->AddMember( poly->m_Uuid, CONSTRAINT_ANCHOR::VERTEX, 0 );
    joint->AddMember( carried->m_Uuid, CONSTRAINT_ANCHOR::START );
    board.Add( joint );

    BOARD_CONSTRAINT_BLOCK_SESSION session;
    BOOST_REQUIRE( session.Build( &board, { poly, carried } ) );

    const VECTOR2I delta( 2 * MM, 0 );
    BOARD_COMMIT   commit( tool );

    commit.Modify( poly );
    commit.Modify( carried );

    std::vector<VECTOR2I> deformed = points;
    deformed[2] += delta;
    poly->SetPolyPoints( deformed );
    carried->Move( delta );

    std::vector<PCB_SHAPE*> moved;
    CONSTRAINT_BLOCK_RESULT result =
            session.Solve( { { CONSTRAINT_MEMBER( poly->m_Uuid, CONSTRAINT_ANCHOR::VERTEX, 2 ), points[2] + delta } },
                           { carried->m_Uuid }, delta, &moved,
                           [&]( BOARD_ITEM* aItem )
                           {
                               commit.Modify( aItem );
                           } );

    BOOST_CHECK( result == CONSTRAINT_BLOCK_RESULT::SOLVED );

    const SHAPE_LINE_CHAIN& outline = poly->GetPolyShape().COutline( 0 );
    BOOST_REQUIRE_EQUAL( outline.PointCount(), 4 );

    BOOST_CHECK_EQUAL( outline.CPoint( 2 ), VECTOR2I( 12 * MM, 10 * MM ) );
    BOOST_CHECK_LE( ( outline.CPoint( 0 ) - carried->GetStart() ).EuclideanNorm(), 100 );
    BOOST_CHECK_LE( ( outline.CPoint( 0 ) - VECTOR2I( 2 * MM, 0 ) ).EuclideanNorm(), 100 );

    for( int i : { 1, 3 } )
        BOOST_CHECK_LE( ( outline.CPoint( i ) - points[i] ).EuclideanNorm(), 100 );

    commit.Revert();
}


// The session solves every frame from the geometry it was built on, so a cluster with room to
// move ends up the same whatever path the cursor took to get there.
BOOST_FIXTURE_TEST_CASE( BlockEditResultDoesNotDependOnThePath, DRAG_FIXTURE )
{
    const std::vector<VECTOR2I> path{ { 0, 2 * MM }, { 0, 5 * MM } };

    const auto run = [&]( const std::vector<VECTOR2I>& aDeltas ) -> std::pair<VECTOR2I, VECTOR2I>
    {
        BOARD      runBoard;
        PCB_SHAPE* deformed = new PCB_SHAPE( &runBoard, SHAPE_T::SEGMENT );
        deformed->SetStart( { 0, 0 } );
        deformed->SetEnd( { 10 * MM, 0 } );
        runBoard.Add( deformed );

        PCB_SHAPE* neighbor = new PCB_SHAPE( &runBoard, SHAPE_T::SEGMENT );
        neighbor->SetStart( { 20 * MM, 0 } );
        neighbor->SetEnd( { 20 * MM, 10 * MM } );
        runBoard.Add( neighbor );

        PCB_CONSTRAINT* square = new PCB_CONSTRAINT( &runBoard, PCB_CONSTRAINT_TYPE::PERPENDICULAR );
        square->AddMember( deformed->m_Uuid, CONSTRAINT_ANCHOR::WHOLE );
        square->AddMember( neighbor->m_Uuid, CONSTRAINT_ANCHOR::WHOLE );
        runBoard.Add( square );

        BOARD_CONSTRAINT_BLOCK_SESSION session;
        BOOST_REQUIRE( session.Build( &runBoard, { deformed } ) );

        for( const VECTOR2I& delta : aDeltas )
        {
            deformed->SetEnd( VECTOR2I( 10 * MM, 0 ) + delta );

            CONSTRAINT_BLOCK_RESULT result =
                    session.Solve( { { CONSTRAINT_MEMBER( deformed->m_Uuid, CONSTRAINT_ANCHOR::END ),
                                       VECTOR2I( 10 * MM, 0 ) + delta } },
                                   {}, delta, nullptr, {} );

            BOOST_REQUIRE( result == CONSTRAINT_BLOCK_RESULT::SOLVED );
        }

        return { neighbor->GetStart(), neighbor->GetEnd() };
    };

    const std::pair<VECTOR2I, VECTOR2I> wandered = run( path );
    const std::pair<VECTOR2I, VECTOR2I> direct = run( { path.back() } );

    BOOST_CHECK_EQUAL( wandered.first, direct.first );
    BOOST_CHECK_EQUAL( wandered.second, direct.second );
}


// A relation that was already broken is left alone. The block drag did not break it, so the
// drag neither repairs it nor is refused over it, and the neighbour it named stays put. The
// relation is dropped at build time, so the cluster itself is no longer counted as conflicted.
BOOST_FIXTURE_TEST_CASE( BlockEditLeavesAPreBrokenClusterAlone, DRAG_FIXTURE )
{
    PCB_SHAPE* carried = addSegment( { 0, 0 }, { 10 * MM, 0 } );
    PCB_SHAPE* neighbor = addSegment( { 12 * MM, 5 * MM }, { 20 * MM, 5 * MM } );
    addCoincident( carried, CONSTRAINT_ANCHOR::END, neighbor, CONSTRAINT_ANCHOR::START );

    BOARD_CONSTRAINT_BLOCK_SESSION session;
    BOOST_REQUIRE( session.Build( &board, { carried } ) );
    BOOST_REQUIRE( !session.HasBaseConflict() );

    const VECTOR2I neighborStart0 = neighbor->GetStart();

    std::vector<PCB_SHAPE*> moved;
    CONSTRAINT_BLOCK_RESULT result = session.Solve( {}, { carried->m_Uuid }, VECTOR2I( 3 * MM, 0 ), &moved, {} );

    BOOST_CHECK( result != CONSTRAINT_BLOCK_RESULT::REFUSED );
    BOOST_CHECK( std::find( moved.begin(), moved.end(), neighbor ) == moved.end() );
    BOOST_CHECK_EQUAL( neighbor->GetStart(), neighborStart0 );
}


// A block that touches a cluster but places none of its coordinates solves nothing and writes
// nothing, so picking a handle the solver cannot model still lets the drag run.
BOOST_FIXTURE_TEST_CASE( BlockEditWithNoModelledCommandWritesNothing, DRAG_FIXTURE )
{
    PCB_SHAPE*      a = addSegment( { 0, 0 }, { 10 * MM, 0 } );
    PCB_SHAPE*      b = addSegment( { 10 * MM, 0 }, { 20 * MM, 0 } );
    PCB_CONSTRAINT* joint = addCoincident( a, CONSTRAINT_ANCHOR::END, b, CONSTRAINT_ANCHOR::START );

    BOARD_CONSTRAINT_ADAPTER adapter;
    BOOST_REQUIRE( adapter.Build( { a, b }, { joint } ) );

    const VECTOR2I aEnd0 = a->GetEnd();
    const VECTOR2I bStart0 = b->GetStart();

    // A centre anchor on a segment maps to no parameter, so the cluster gets nothing to solve
    // even though the block carried one of its shapes
    CONSTRAINT_BLOCK_RESULT result = adapter.SolveBlockEdit(
            { { CONSTRAINT_MEMBER( a->m_Uuid, CONSTRAINT_ANCHOR::CENTER ), VECTOR2I( 5 * MM, 5 * MM ) } }, {},
            VECTOR2I( 0, 0 ) );

    BOOST_CHECK( result == CONSTRAINT_BLOCK_RESULT::NOT_MODELED );
    BOOST_CHECK( adapter.Apply().empty() );
    BOOST_CHECK_EQUAL( a->GetEnd(), aEnd0 );
    BOOST_CHECK_EQUAL( b->GetStart(), bStart0 );
}


// The drag reads each pin's target from the shape through ConstraintShapeAnchors, so the index
// that enumeration hands back has to be the index the solver's parameter mapping uses. A
// rectangle's four corners share two stored points and a polygon's corners are outline indices,
// so a mismatch here would move a different corner than the one the user dragged.
BOOST_AUTO_TEST_CASE( BlockEditHonoursEveryEnumeratedAnchor )
{
    const std::vector<VECTOR2I> polyPoints{ { 0, 0 }, { 10 * MM, 0 }, { 13 * MM, 8 * MM }, { -3 * MM, 8 * MM } };
    const VECTOR2I              nudge( 3 * MM, 2 * MM );

    const auto makeProbe = [&]( bool aRectangle, std::unique_ptr<BOARD>& aBoard ) -> PCB_SHAPE*
    {
        aBoard = std::make_unique<BOARD>();

        PCB_SHAPE* shape = new PCB_SHAPE( aBoard.get(), aRectangle ? SHAPE_T::RECTANGLE : SHAPE_T::POLY );

        if( aRectangle )
        {
            shape->SetStart( { 0, 0 } );
            shape->SetEnd( { 10 * MM, 8 * MM } );
        }
        else
        {
            shape->SetPolyPoints( polyPoints );
        }

        aBoard->Add( shape );
        return shape;
    };

    for( bool rectangle : { true, false } )
    {
        std::unique_ptr<BOARD> sizingBoard;
        const size_t           anchorCount = ConstraintShapeAnchors( makeProbe( rectangle, sizingBoard ) ).size();

        BOOST_REQUIRE_EQUAL( anchorCount, 4 );

        for( size_t ii = 0; ii < anchorCount; ++ii )
        {
            std::unique_ptr<BOARD> probeBoard;
            PCB_SHAPE*             probe = makeProbe( rectangle, probeBoard );

            std::vector<CONSTRAINT_ANCHOR_POINT> before = ConstraintShapeAnchors( probe );
            BOOST_REQUIRE_EQUAL( before.size(), anchorCount );

            const VECTOR2I target = before[ii].pos + nudge;

            BOARD_CONSTRAINT_ADAPTER adapter;
            BOOST_REQUIRE( adapter.Build( { probe }, {} ) );

            CONSTRAINT_BLOCK_RESULT result = adapter.SolveBlockEdit(
                    { { CONSTRAINT_MEMBER( probe->m_Uuid, before[ii].anchor, before[ii].index ), target } }, {},
                    VECTOR2I( 0, 0 ) );

            BOOST_REQUIRE( result == CONSTRAINT_BLOCK_RESULT::SOLVED );
            adapter.Apply();

            std::vector<CONSTRAINT_ANCHOR_POINT> after = ConstraintShapeAnchors( probe );
            BOOST_REQUIRE_EQUAL( after.size(), anchorCount );

            // The anchor that was placed is the one that landed on the target
            BOOST_CHECK_EQUAL( after[ii].pos, target );

            for( size_t jj = 0; jj < anchorCount; ++jj )
            {
                if( jj != ii && rectangle )
                    continue; // a rect corner shares each of its coordinates with a neighbour

                if( jj != ii )
                    BOOST_CHECK_EQUAL( after[jj].pos, before[jj].pos );
            }
        }
    }
}


BOOST_AUTO_TEST_SUITE_END()
