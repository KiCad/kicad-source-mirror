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

/**
 * @file
 * Tests the dimension-as-constraint-member support (issue #2329, item 6 phase 1): a dimension's
 * feature point can be COINCIDENT with a shape point, so solving (or dragging the shape) pulls the
 * dimension along, while its other, unbound endpoint stays put.
 */

#include <qa_utils/wx_utils/unit_test_utils.h>

#include <vector>

#include <board.h>
#include <pcb_dimension.h>
#include <pcb_shape.h>

#include <constraints/pcb_constraint.h>
#include <constraints/board_constraint_adapter.h>
#include <constraints/constraint_builder.h>

#include "constraint_test_utils.h"

using namespace KI_TEST;

namespace
{
PCB_DIM_ALIGNED* addAlignedDim( BOARD& aBoard, const VECTOR2I& aStart, const VECTOR2I& aEnd )
{
    PCB_DIM_ALIGNED* dim = new PCB_DIM_ALIGNED( &aBoard, PCB_DIM_ALIGNED_T );
    dim->SetStart( aStart );
    dim->SetEnd( aEnd );
    dim->Update();
    aBoard.Add( dim );
    return dim;
}
}


BOOST_AUTO_TEST_SUITE( ConstraintDimension )


// An aligned dimension exposes its two feature points; a leader (a control-point family) exposes
// only its start.
BOOST_AUTO_TEST_CASE( DimensionAnchorsPerFamily )
{
    BOARD            board;
    PCB_DIM_ALIGNED* aligned = addAlignedDim( board, { 0, 0 }, { 10 * MM, 0 } );

    std::vector<CONSTRAINT_ANCHOR_POINT> anchors = ConstraintItemAnchors( aligned );
    BOOST_REQUIRE_EQUAL( anchors.size(), 2 );
    BOOST_CHECK( anchors[0].anchor == CONSTRAINT_ANCHOR::START );
    BOOST_CHECK( anchors[1].anchor == CONSTRAINT_ANCHOR::END );

    PCB_DIM_LEADER* leader = new PCB_DIM_LEADER( &board );
    leader->SetStart( { 0, 0 } );
    leader->SetEnd( { 5 * MM, 5 * MM } );
    board.Add( leader );

    BOOST_CHECK_EQUAL( ConstraintItemAnchors( leader ).size(), 1 );   // start only
}


// A coincident constraint binds a dimension's start to a segment endpoint; dragging that endpoint
// pulls the dimension start along, while the dimension's unbound end stays put.
BOOST_AUTO_TEST_CASE( CoincidentDimensionFollowsDraggedShape )
{
    BOARD      board;
    PCB_SHAPE* seg = addSegment( board, { 0, 0 }, { 10 * MM, 0 } );

    PCB_DIM_ALIGNED* dim = addAlignedDim( board, { 10 * MM, 0 }, { 10 * MM, 5 * MM } );

    addConstraint( board, PCB_CONSTRAINT_TYPE::COINCIDENT,
                   { { seg->m_Uuid, CONSTRAINT_ANCHOR::END }, { dim->m_Uuid, CONSTRAINT_ANCHOR::START } } );

    // A bound dimension carries its cluster's verdict, so the overlay can mark it.
    BOARD_CONSTRAINT_DIAGNOSTICS diag = DiagnoseBoardConstraints( &board );
    auto segState = diag.shapeStates.find( seg->m_Uuid );
    auto dimState = diag.shapeStates.find( dim->m_Uuid );

    BOOST_REQUIRE( segState != diag.shapeStates.end() );
    BOOST_REQUIRE( dimState != diag.shapeStates.end() );
    BOOST_CHECK( dimState->second == segState->second );

    const VECTOR2I dimEnd0 = dim->GetEnd();

    std::vector<BOARD_ITEM*> staged;
    SolveCluster( &board, { seg->m_Uuid, CONSTRAINT_ANCHOR::END }, { 12 * MM, 3 * MM }, nullptr,
                  [&]( BOARD_ITEM* i ) { staged.push_back( i ); } );

    // The dimension start tracked the segment end; its unbound end did not move.
    BOOST_CHECK_LE( ( dim->GetStart() - seg->GetEnd() ).EuclideanNorm(), 5000.0 );
    BOOST_CHECK( ( dim->GetStart() - VECTOR2I( 10 * MM, 0 ) ).EuclideanNorm() > 20000.0 );
    BOOST_CHECK_LE( ( dim->GetEnd() - dimEnd0 ).EuclideanNorm(), 5000.0 );
    BOOST_CHECK( std::find( staged.begin(), staged.end(), dim ) != staged.end() );
}


// A dimension whose bound shape is deleted leaves the constraint in an error state, not deleted.
BOOST_AUTO_TEST_CASE( DeletedShapeErrorsDimensionConstraint )
{
    BOARD      board;
    PCB_SHAPE* seg = addSegment( board, { 0, 0 }, { 10 * MM, 0 } );
    KIID       segId = seg->m_Uuid;

    PCB_DIM_ALIGNED* dim = addAlignedDim( board, { 10 * MM, 0 }, { 10 * MM, 5 * MM } );

    addConstraint( board, PCB_CONSTRAINT_TYPE::COINCIDENT,
                   { { segId, CONSTRAINT_ANCHOR::END }, { dim->m_Uuid, CONSTRAINT_ANCHOR::START } } );

    board.Remove( seg );
    delete seg;

    BOARD_CONSTRAINT_DIAGNOSTICS diag = DiagnoseBoardConstraints( &board );
    BOOST_CHECK_EQUAL( diag.errored.size(), 1 );
}


// The board-wide anchor picker offers a dimension's feature point, so coincident authoring can bind
// a dimension by clicking near it.
BOOST_AUTO_TEST_CASE( PickerOffersDimensionAnchor )
{
    BOARD            board;
    PCB_DIM_ALIGNED* dim = addAlignedDim( board, { 10 * MM, 0 }, { 10 * MM, 5 * MM } );

    std::optional<CONSTRAINT_MEMBER> hit =
            NearestConstraintAnchor( &board, VECTOR2I( 10 * MM, 100 ), MM );

    BOOST_REQUIRE( hit.has_value() );
    BOOST_CHECK( hit->m_item == dim->m_Uuid );
    BOOST_CHECK( hit->m_anchor == CONSTRAINT_ANCHOR::START );
}


// Authoring a coincident with the dimension as the first member (the apply-on-create path pins the
// first member) snaps the dimension's bound point onto the shape point.
BOOST_AUTO_TEST_CASE( ApplyCoincidentDimensionFirstMember )
{
    BOARD      board;
    PCB_SHAPE* seg = addSegment( board, { 0, 0 }, { 10 * MM, 0 } );

    // The dimension start sits 1 mm off the segment end it will bind to.
    PCB_DIM_ALIGNED* dim = addAlignedDim( board, { 11 * MM, 0 }, { 11 * MM, 5 * MM } );

    PCB_CONSTRAINT* c = addConstraint(
            board, PCB_CONSTRAINT_TYPE::COINCIDENT,
            { { dim->m_Uuid, CONSTRAINT_ANCHOR::START }, { seg->m_Uuid, CONSTRAINT_ANCHOR::END } } );

    std::vector<PCB_SHAPE*> modified;
    ApplyConstraintImmediately( &board, c, &modified );

    // The first member (the dimension start) is pinned, so the segment end snaps onto it.
    BOOST_CHECK_LE( ( seg->GetEnd() - dim->GetStart() ).EuclideanNorm(), 5000.0 );
}


// Moving a whole shape re-solves its cluster, so a coincident-bound dimension follows the shape.
BOOST_AUTO_TEST_CASE( WholeShapeMoveDragsBoundDimension )
{
    BOARD      board;
    PCB_SHAPE* seg = addSegment( board, { 0, 0 }, { 10 * MM, 0 } );

    PCB_DIM_ALIGNED* dim = addAlignedDim( board, { 10 * MM, 0 }, { 10 * MM, 5 * MM } );

    addConstraint( board, PCB_CONSTRAINT_TYPE::COINCIDENT,
                   { { seg->m_Uuid, CONSTRAINT_ANCHOR::END }, { dim->m_Uuid, CONSTRAINT_ANCHOR::START } } );

    seg->Move( { 0, 3 * MM } );   // whole-shape move
    ReSolveShapeClusters( &board, { seg } );

    // The dimension start followed the moved segment end.
    BOOST_CHECK_LE( ( dim->GetStart() - seg->GetEnd() ).EuclideanNorm(), 5000.0 );
    BOOST_CHECK( dim->GetStart().y > 1 * MM );   // it actually moved up
}


// A leader has no bindable END (its end is a control point), so a coincident on it cannot map and
// is flagged errored rather than moving the control point.
BOOST_AUTO_TEST_CASE( LeaderEndIsNotBindable )
{
    BOARD      board;
    PCB_SHAPE* seg = addSegment( board, { 0, 0 }, { 10 * MM, 0 } );

    PCB_DIM_LEADER* leader = new PCB_DIM_LEADER( &board );
    leader->SetStart( { 10 * MM, 0 } );
    leader->SetEnd( { 15 * MM, 5 * MM } );
    board.Add( leader );

    PCB_CONSTRAINT* c = addConstraint(
            board, PCB_CONSTRAINT_TYPE::COINCIDENT,
            { { seg->m_Uuid, CONSTRAINT_ANCHOR::END }, { leader->m_Uuid, CONSTRAINT_ANCHOR::END } } );

    std::vector<PCB_CONSTRAINT*> constraints = { c };
    BOARD_CONSTRAINT_ADAPTER     adapter;
    BOOST_REQUIRE( adapter.Build( { seg }, constraints, nullptr, { leader } ) );

    BOOST_CHECK( std::find( adapter.UnmappedConstraints().begin(), adapter.UnmappedConstraints().end(),
                            c->m_Uuid )
                 != adapter.UnmappedConstraints().end() );
}


BOOST_AUTO_TEST_SUITE_END()
