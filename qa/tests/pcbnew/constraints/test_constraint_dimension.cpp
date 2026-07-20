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

#include <algorithm>
#include <vector>

#include <board.h>
#include <footprint.h>
#include <geometry/shape_arc.h>
#include <geometry/shape_line_chain.h>
#include <i18n_utility.h>
#include <pcb_dimension.h>
#include <pcb_shape.h>
#include <properties/property.h>
#include <properties/property_mgr.h>
#include <trigo.h>

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


PCB_DIM_RADIAL* addRadialDim( BOARD& aBoard, const VECTOR2I& aCenter, const VECTOR2I& aRim )
{
    PCB_DIM_RADIAL* dim = new PCB_DIM_RADIAL( &aBoard );
    dim->SetStart( aCenter );
    dim->SetEnd( aRim );
    dim->Update();
    aBoard.Add( dim );
    return dim;
}


PCB_DIM_ORTHOGONAL* addOrthogonalDim( BOARD& aBoard, const VECTOR2I& aStart, const VECTOR2I& aEnd,
                                      PCB_DIM_ORTHOGONAL::DIR aOrientation )
{
    PCB_DIM_ORTHOGONAL* dim = new PCB_DIM_ORTHOGONAL( &aBoard );
    dim->SetOrientation( aOrientation );
    dim->SetStart( aStart );
    dim->SetEnd( aEnd );
    dim->Update();
    aBoard.Add( dim );
    return dim;
}


PCB_CONSTRAINT* addDrivingLength( BOARD& aBoard, const KIID& aDimension, double aLengthIU )
{
    PCB_CONSTRAINT* c = addConstraint(
            aBoard, PCB_CONSTRAINT_TYPE::FIXED_LENGTH,
            { { aDimension, CONSTRAINT_ANCHOR::START }, { aDimension, CONSTRAINT_ANCHOR::END } }, aLengthIU );
    c->SetDriving( true );
    return c;
}


// Dim-first member order matches draw-time auto-constrain
PCB_CONSTRAINT* bindEndpoint( BOARD& aBoard, const KIID& aDimension, CONSTRAINT_ANCHOR aDimAnchor,
                              const CONSTRAINT_MEMBER& aTarget )
{
    return addConstraint( aBoard, PCB_CONSTRAINT_TYPE::COINCIDENT, { { aDimension, aDimAnchor }, aTarget } );
}


// Physical TL and BR corners regardless of stored start end order
VECTOR2I rectTopLeft( const PCB_SHAPE* aRect )
{
    return VECTOR2I( std::min( aRect->GetStart().x, aRect->GetEnd().x ),
                     std::min( aRect->GetStart().y, aRect->GetEnd().y ) );
}


VECTOR2I rectBottomRight( const PCB_SHAPE* aRect )
{
    return VECTOR2I( std::max( aRect->GetStart().x, aRect->GetEnd().x ),
                     std::max( aRect->GetStart().y, aRect->GetEnd().y ) );
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


// Driving length on centre-to-rim radial drives circle radius centre pinned
BOOST_AUTO_TEST_CASE( RadialDrivingLengthDrivesCircleRadius )
{
    BOARD      board;
    PCB_SHAPE* circle = addCircle( board, { 0, 0 }, 5 * MM );

    PCB_DIM_RADIAL* dim = addRadialDim( board, { 0, 0 }, { 5 * MM, 0 } );

    // Bind as draw-time auto-constrain does centre coincident rim on outline
    addConstraint( board, PCB_CONSTRAINT_TYPE::COINCIDENT,
                   { { dim->m_Uuid, CONSTRAINT_ANCHOR::START }, { circle->m_Uuid, CONSTRAINT_ANCHOR::CENTER } } );
    addConstraint( board, PCB_CONSTRAINT_TYPE::POINT_ON_LINE,
                   { { dim->m_Uuid, CONSTRAINT_ANCHOR::END }, { circle->m_Uuid, CONSTRAINT_ANCHOR::WHOLE } } );

    PCB_CONSTRAINT* driving = addDrivingLength( board, dim->m_Uuid, 8 * MM );

    ApplyConstraintImmediately( &board, driving );

    BOOST_CHECK_LE( std::abs( circle->GetRadius() - 8 * MM ), 5000 );
    BOOST_CHECK_LE( ( circle->GetCenter() - VECTOR2I( 0, 0 ) ).EuclideanNorm(), 5000.0 );
}


// Horizontal driving orthogonal fixes x gap only y stays free
BOOST_AUTO_TEST_CASE( OrthogonalHorizontalDrivingSetsXAxisOnly )
{
    BOARD board;

    PCB_DIM_ORTHOGONAL* dim =
            addOrthogonalDim( board, { 0, 0 }, { 4 * MM, 3 * MM }, PCB_DIM_ORTHOGONAL::DIR::HORIZONTAL );

    PCB_CONSTRAINT* driving = addDrivingLength( board, dim->m_Uuid, 10 * MM );

    ApplyConstraintImmediately( &board, driving );

    BOOST_CHECK_LE( std::abs( ( dim->GetEnd().x - dim->GetStart().x ) - 10 * MM ), 5000 );
    BOOST_CHECK_LE( std::abs( dim->GetEnd().y - 3 * MM ), 5000 );   // y untouched
}


// Vertical driving orthogonal fixes y gap only x stays free
BOOST_AUTO_TEST_CASE( OrthogonalVerticalDrivingSetsYAxisOnly )
{
    BOARD board;

    PCB_DIM_ORTHOGONAL* dim =
            addOrthogonalDim( board, { 0, 0 }, { 3 * MM, 4 * MM }, PCB_DIM_ORTHOGONAL::DIR::VERTICAL );

    PCB_CONSTRAINT* driving = addDrivingLength( board, dim->m_Uuid, 10 * MM );

    ApplyConstraintImmediately( &board, driving );

    BOOST_CHECK_LE( std::abs( ( dim->GetEnd().y - dim->GetStart().y ) - 10 * MM ), 5000 );
    BOOST_CHECK_LE( std::abs( dim->GetEnd().x - 3 * MM ), 5000 );   // x untouched
}


// Radial only binds when one circle or arc holds both centre and rim
BOOST_AUTO_TEST_CASE( RadialBindingRequiresSameArcCenterAndOutline )
{
    BOARD        board;
    PCB_SHAPE*   circle = addCircle( board, { 0, 0 }, 5 * MM );
    const double tol = 10000;   // 0.01 mm

    PCB_DIM_RADIAL* dim = addRadialDim( board, { 0, 0 }, { 5 * MM, 0 } );

    std::optional<KIID> hit =
            SelectRadialDimensionTarget( &board, dim->m_Uuid, { 0, 0 }, { 5 * MM, 0 }, tol );
    BOOST_REQUIRE( hit.has_value() );
    BOOST_CHECK( *hit == circle->m_Uuid );

    // Rim 3mm off a 5mm circle no binding
    BOOST_CHECK( !SelectRadialDimensionTarget( &board, dim->m_Uuid, { 0, 0 }, { 3 * MM, 0 }, tol ) );

    // Centre off circle centre no binding
    BOOST_CHECK( !SelectRadialDimensionTarget( &board, dim->m_Uuid, { 2 * MM, 0 }, { 7 * MM, 0 }, tol ) );
}


// Centre on one circle rim on another binds neither
BOOST_AUTO_TEST_CASE( RadialBindingRejectsCrossObjectCenterAndOutline )
{
    BOARD        board;
    const double tol = 10000;

    addCircle( board, { 0, 0 }, 5 * MM );          // centre object
    addCircle( board, { 50 * MM, 0 }, 5 * MM );    // outline object 55mm from first centre

    BOOST_CHECK( !SelectRadialDimensionTarget( &board, KIID(), { 0, 0 }, { 55 * MM, 0 }, tol ) );
}


// Corner to corner dim binds both endpoints as indexed VERTEX anchors in canonical TL to BL order
BOOST_AUTO_TEST_CASE( DimensionBindsRectCornerToCorner )
{
    BOARD      board;
    PCB_SHAPE* rect = addRect( board, { 0, 0 }, { 10 * MM, 10 * MM } );

    KIID fakeDim;   // dimension uuid not on the board as during interactive draw
    std::vector<DIMENSION_ENDPOINT_BINDING> bindings =
            SelectDimensionEndpointBindings( &board, fakeDim, { 0, 0 }, VECTOR2I( 10 * MM, 10 * MM ), 1000.0 );

    BOOST_REQUIRE_EQUAL( bindings.size(), 2 );
    BOOST_CHECK( bindings[0].dimAnchor == CONSTRAINT_ANCHOR::START );
    BOOST_CHECK( bindings[0].target == CONSTRAINT_MEMBER( rect->m_Uuid, CONSTRAINT_ANCHOR::VERTEX, 0 ) );
    BOOST_CHECK( bindings[1].dimAnchor == CONSTRAINT_ANCHOR::END );
    BOOST_CHECK( bindings[1].target == CONSTRAINT_MEMBER( rect->m_Uuid, CONSTRAINT_ANCHOR::VERTEX, 2 ) );

    // Rect stored end before start still enumerates canonically index 0 always TL
    PCB_SHAPE* reversed = addRect( board, { 30 * MM, 30 * MM }, { 20 * MM, 20 * MM } );
    std::vector<CONSTRAINT_ANCHOR_POINT> anchors = ConstraintShapeAnchors( reversed );

    BOOST_REQUIRE_EQUAL( anchors.size(), 4 );
    BOOST_CHECK_EQUAL( anchors[0].index, 0 );
    BOOST_CHECK_EQUAL( anchors[0].pos, VECTOR2I( 20 * MM, 20 * MM ) );
    BOOST_CHECK_EQUAL( anchors[2].index, 2 );
    BOOST_CHECK_EQUAL( anchors[2].pos, VECTOR2I( 30 * MM, 30 * MM ) );
}


// Dim across pentagon vertices binds by outline order VERTEX resolves to own index not vertex 0
BOOST_AUTO_TEST_CASE( DimensionBindsPolygonVertices )
{
    BOARD board;

    std::vector<VECTOR2I> pts = { { 0, 0 },
                                  { 20 * MM, 5 * MM },
                                  { 15 * MM, 20 * MM },
                                  { 5 * MM, 20 * MM },
                                  { -5 * MM, 10 * MM } };
    PCB_SHAPE*            poly = addPoly( board, pts );

    KIID fakeDim;
    std::vector<DIMENSION_ENDPOINT_BINDING> bindings =
            SelectDimensionEndpointBindings( &board, fakeDim, pts[1], pts[3], 1000.0 );

    BOOST_REQUIRE_EQUAL( bindings.size(), 2 );
    BOOST_CHECK( bindings[0].dimAnchor == CONSTRAINT_ANCHOR::START );
    BOOST_CHECK( bindings[0].target == CONSTRAINT_MEMBER( poly->m_Uuid, CONSTRAINT_ANCHOR::VERTEX, 1 ) );
    BOOST_CHECK( bindings[1].dimAnchor == CONSTRAINT_ANCHOR::END );
    BOOST_CHECK( bindings[1].target == CONSTRAINT_MEMBER( poly->m_Uuid, CONSTRAINT_ANCHOR::VERTEX, 3 ) );

    // Anchor lookup must honour vertex index not just the VERTEX tag
    std::optional<VECTOR2I> pos =
            ConstraintAnchorPosition( &board, { poly->m_Uuid, CONSTRAINT_ANCHOR::VERTEX, 2 } );

    BOOST_REQUIRE( pos.has_value() );
    BOOST_CHECK_EQUAL( *pos, pts[2] );
}


// Single polygon reaching both ends preferred over splitting onto a closer segment
BOOST_AUTO_TEST_CASE( DimensionBindingPrefersSinglePolygonOverSplit )
{
    BOARD board;

    PCB_SHAPE* poly = addPoly( board, { { 0, 0 },
                                        { 10 * MM, 0 },
                                        { 10 * MM, 10 * MM },
                                        { 0, 10 * MM } } );
    PCB_SHAPE* b = addSegment( board, { 2 * MM, 1 * MM }, { 2 * MM, 20 * MM } );

    KIID fakeDim;
    std::vector<DIMENSION_ENDPOINT_BINDING> bindings =
            SelectDimensionEndpointBindings( &board, fakeDim, { 0, 0 }, VECTOR2I( 2 * MM, 0 ), 12.0 * MM );

    BOOST_REQUIRE_EQUAL( bindings.size(), 2 );
    BOOST_CHECK( bindings[0].target == CONSTRAINT_MEMBER( poly->m_Uuid, CONSTRAINT_ANCHOR::VERTEX, 0 ) );
    BOOST_CHECK( bindings[1].target == CONSTRAINT_MEMBER( poly->m_Uuid, CONSTRAINT_ANCHOR::VERTEX, 1 ) );

    // Decoy segment never enters binding despite its start being nearest END
    for( const DIMENSION_ENDPOINT_BINDING& binding : bindings )
        BOOST_CHECK( binding.target.m_item != b->m_Uuid );
}


// Arc-bearing outline not ingested by adapter so it offers no anchors
BOOST_AUTO_TEST_CASE( ArcOutlinePolyOffersNoAnchors )
{
    BOARD board;

    PCB_SHAPE* poly = new PCB_SHAPE( &board, SHAPE_T::POLY );

    SHAPE_LINE_CHAIN chain;
    chain.Append( VECTOR2I( 10 * MM, 10 * MM ) );
    chain.Append( VECTOR2I( 50 * MM, 10 * MM ) );
    chain.Append( SHAPE_ARC( { 50 * MM, 10 * MM }, { 55 * MM, 25 * MM }, { 50 * MM, 40 * MM }, 0 ) );
    chain.Append( VECTOR2I( 10 * MM, 40 * MM ) );
    chain.SetClosed( true );

    poly->GetPolyShape().AddOutline( chain );
    board.Add( poly );

    BOOST_REQUIRE_GT( poly->GetPolyShape().COutline( 0 ).ArcCount(), 0 );

    BOOST_CHECK( ConstraintShapeAnchors( poly ).empty() );
    BOOST_CHECK( !NearestConstraintAnchor( &board, VECTOR2I( 10 * MM, 10 * MM ), MM ) );
}


// Driving aligned dim widens rect to entered length height untouched endpoints ride corners
BOOST_AUTO_TEST_CASE( AlignedDrivingLengthDrivesRectWidth )
{
    BOARD      board;
    PCB_SHAPE* rect = addRect( board, { 0, 0 }, { 10 * MM, 10 * MM } );

    PCB_DIM_ALIGNED* dim = addAlignedDim( board, { 0, 0 }, { 10 * MM, 0 } );   // TL to TR

    bindEndpoint( board, dim->m_Uuid, CONSTRAINT_ANCHOR::START,
                  { rect->m_Uuid, CONSTRAINT_ANCHOR::VERTEX, 0 } );
    bindEndpoint( board, dim->m_Uuid, CONSTRAINT_ANCHOR::END,
                  { rect->m_Uuid, CONSTRAINT_ANCHOR::VERTEX, 1 } );

    PCB_CONSTRAINT* driving = addDrivingLength( board, dim->m_Uuid, 15 * MM );

    CONSTRAINT_DIAGNOSIS diag = ApplyConstraintImmediately( &board, driving );
    BOOST_REQUIRE( diag.solved );

    const VECTOR2I tl = rectTopLeft( rect );
    const VECTOR2I br = rectBottomRight( rect );

    BOOST_CHECK_LE( std::abs( ( br.x - tl.x ) - 15 * MM ), 5000 );   // width driven to the value
    BOOST_CHECK_LE( std::abs( ( br.y - tl.y ) - 10 * MM ), 5000 );   // height untouched
    BOOST_CHECK_LE( tl.EuclideanNorm(), 5000.0 );                    // pinned TL corner held

    // All four enumerated corners agree with solved extremes rectangle stayed coherent
    std::vector<CONSTRAINT_ANCHOR_POINT> corners = ConstraintShapeAnchors( rect );
    BOOST_REQUIRE_EQUAL( corners.size(), 4 );
    BOOST_CHECK_EQUAL( corners[0].pos, tl );
    BOOST_CHECK_EQUAL( corners[1].pos, VECTOR2I( br.x, tl.y ) );
    BOOST_CHECK_EQUAL( corners[2].pos, br );
    BOOST_CHECK_EQUAL( corners[3].pos, VECTOR2I( tl.x, br.y ) );

    // Dimension endpoints followed corners through coincident bindings
    BOOST_CHECK_LE( ( dim->GetStart() - corners[0].pos ).EuclideanNorm(), 5000.0 );
    BOOST_CHECK_LE( ( dim->GetEnd() - corners[1].pos ).EuclideanNorm(), 5000.0 );
}


// Driving horizontal orthogonal on pentagon vertices forces x separation y and unbound vertices held
BOOST_AUTO_TEST_CASE( OrthogonalDrivingLengthDrivesPolygonAxis )
{
    BOARD board;

    std::vector<VECTOR2I> pts = { { 0, 0 },
                                  { 20 * MM, 5 * MM },
                                  { 15 * MM, 20 * MM },
                                  { 5 * MM, 20 * MM },
                                  { -5 * MM, 10 * MM } };
    PCB_SHAPE*            poly = addPoly( board, pts );

    PCB_DIM_ORTHOGONAL* dim =
            addOrthogonalDim( board, pts[1], pts[3], PCB_DIM_ORTHOGONAL::DIR::HORIZONTAL );

    bindEndpoint( board, dim->m_Uuid, CONSTRAINT_ANCHOR::START,
                  { poly->m_Uuid, CONSTRAINT_ANCHOR::VERTEX, 1 } );
    bindEndpoint( board, dim->m_Uuid, CONSTRAINT_ANCHOR::END,
                  { poly->m_Uuid, CONSTRAINT_ANCHOR::VERTEX, 3 } );

    PCB_CONSTRAINT* driving = addDrivingLength( board, dim->m_Uuid, 20 * MM );

    CONSTRAINT_DIAGNOSIS diag = ApplyConstraintImmediately( &board, driving );
    BOOST_REQUIRE( diag.solved );

    const SHAPE_LINE_CHAIN& outline = poly->GetPolyShape().COutline( 0 );

    BOOST_CHECK_LE( std::abs( std::abs( outline.CPoint( 3 ).x - outline.CPoint( 1 ).x ) - 20 * MM ), 5000 );
    BOOST_CHECK_LE( std::abs( outline.CPoint( 1 ).y - pts[1].y ), 5000 );   // y is not the driven axis
    BOOST_CHECK_LE( std::abs( outline.CPoint( 3 ).y - pts[3].y ), 5000 );

    for( int i : { 0, 2, 4 } )
        BOOST_CHECK_LE( ( outline.CPoint( i ) - pts[i] ).EuclideanNorm(), 5000.0 );   // unbound vertices held

    // Dimension endpoints followed the vertices they bind
    BOOST_CHECK_LE( ( dim->GetStart() - outline.CPoint( 1 ) ).EuclideanNorm(), 5000.0 );
    BOOST_CHECK_LE( ( dim->GetEnd() - outline.CPoint( 3 ) ).EuclideanNorm(), 5000.0 );
}


// Driven reference dim follows rect resize through move-tool re-solve length re-measures
BOOST_AUTO_TEST_CASE( DrivenDimensionRemeasuresAfterRectResize )
{
    BOARD      board;
    PCB_SHAPE* rect = addRect( board, { 0, 0 }, { 10 * MM, 10 * MM } );

    PCB_DIM_ALIGNED* dim = addAlignedDim( board, { 0, 0 }, { 10 * MM, 0 } );

    bindEndpoint( board, dim->m_Uuid, CONSTRAINT_ANCHOR::START,
                  { rect->m_Uuid, CONSTRAINT_ANCHOR::VERTEX, 0 } );
    bindEndpoint( board, dim->m_Uuid, CONSTRAINT_ANCHOR::END,
                  { rect->m_Uuid, CONSTRAINT_ANCHOR::VERTEX, 1 } );

    // Driven mode carries a non-driving length that only mirrors measured geometry
    PCB_CONSTRAINT* reference = addConstraint(
            board, PCB_CONSTRAINT_TYPE::FIXED_LENGTH,
            { { dim->m_Uuid, CONSTRAINT_ANCHOR::START }, { dim->m_Uuid, CONSTRAINT_ANCHOR::END } },
            10.0 * MM );
    reference->SetDriving( false );

    rect->SetEnd( { 15 * MM, 10 * MM } );   // widen by 5mm edited outside the solver
    ReSolveShapeClusters( &board, { rect } );

    // Resize survived re-solve dimension endpoints tracked corners
    BOOST_CHECK_LE( ( rectBottomRight( rect ) - VECTOR2I( 15 * MM, 10 * MM ) ).EuclideanNorm(), 5000.0 );
    BOOST_CHECK_LE( ( dim->GetStart() - VECTOR2I( 0, 0 ) ).EuclideanNorm(), 5000.0 );
    BOOST_CHECK_LE( ( dim->GetEnd() - VECTOR2I( 15 * MM, 0 ) ).EuclideanNorm(), 5000.0 );

    BOOST_REQUIRE( reference->GetValue().has_value() );
    BOOST_CHECK_LE( std::abs( *reference->GetValue() - 15.0 * MM ), 5000.0 );
}


// Properties edit is authoritative hold-edited re-solve keeps typed width and pulls bound items along
BOOST_AUTO_TEST_CASE( HoldingEditedRectResizeFollowsBindings )
{
    BOARD      board;
    PCB_SHAPE* rect = addRect( board, { 0, 0 }, { 10 * MM, 10 * MM } );
    PCB_SHAPE* seg = addSegment( board, { 10 * MM, 0 }, { 20 * MM, -5 * MM } );

    addConstraint( board, PCB_CONSTRAINT_TYPE::COINCIDENT,
                   { { seg->m_Uuid, CONSTRAINT_ANCHOR::START }, { rect->m_Uuid, CONSTRAINT_ANCHOR::VERTEX, 1 } } );

    PCB_DIM_ALIGNED* dim = addAlignedDim( board, { 0, 0 }, { 10 * MM, 0 } );   // TL to TR

    bindEndpoint( board, dim->m_Uuid, CONSTRAINT_ANCHOR::START,
                  { rect->m_Uuid, CONSTRAINT_ANCHOR::VERTEX, 0 } );
    bindEndpoint( board, dim->m_Uuid, CONSTRAINT_ANCHOR::END,
                  { rect->m_Uuid, CONSTRAINT_ANCHOR::VERTEX, 1 } );

    PCB_CONSTRAINT* reference = addConstraint(
            board, PCB_CONSTRAINT_TYPE::FIXED_LENGTH,
            { { dim->m_Uuid, CONSTRAINT_ANCHOR::START }, { dim->m_Uuid, CONSTRAINT_ANCHOR::END } },
            10.0 * MM );
    reference->SetDriving( false );

    rect->SetEnd( { 15 * MM, 10 * MM } );   // widen by 5mm as a typed width edit would

    std::vector<PCB_SHAPE*> modified;
    ReSolveShapeClustersHoldingEdited( &board, { rect }, &modified );

    // Edited geometry survived corner-bound segment start followed TR corner
    BOOST_CHECK_LE( ( rectBottomRight( rect ) - VECTOR2I( 15 * MM, 10 * MM ) ).EuclideanNorm(), 5000.0 );
    BOOST_CHECK_LE( ( rectTopLeft( rect ) - VECTOR2I( 0, 0 ) ).EuclideanNorm(), 5000.0 );
    BOOST_CHECK_LE( ( seg->GetStart() - VECTOR2I( 15 * MM, 0 ) ).EuclideanNorm(), 5000.0 );
    BOOST_CHECK( std::find( modified.begin(), modified.end(), seg ) != modified.end() );

    // Both dimension endpoints ride the corners they bind Driven value re-measured
    BOOST_CHECK_LE( ( dim->GetStart() - VECTOR2I( 0, 0 ) ).EuclideanNorm(), 5000.0 );
    BOOST_CHECK_LE( ( dim->GetEnd() - VECTOR2I( 15 * MM, 0 ) ).EuclideanNorm(), 5000.0 );

    BOOST_REQUIRE( reference->GetValue().has_value() );
    BOOST_CHECK_LE( std::abs( *reference->GetValue() - 15.0 * MM ), 5000.0 );
}


// Stale vertex index left unmapped by Build does not poison solve Driving gate reports undrivable
BOOST_AUTO_TEST_CASE( StaleVertexBindingDegradesAndGatesDriving )
{
    BOARD board;

    std::vector<VECTOR2I> pts = { { 0, 0 },
                                  { 20 * MM, 5 * MM },
                                  { 15 * MM, 20 * MM },
                                  { 5 * MM, 20 * MM },
                                  { -5 * MM, 10 * MM } };
    PCB_SHAPE*            poly = addPoly( board, pts );

    PCB_DIM_ALIGNED* dim = addAlignedDim( board, pts[1], pts[4] );

    PCB_CONSTRAINT* startBinding = bindEndpoint( board, dim->m_Uuid, CONSTRAINT_ANCHOR::START,
                                                 { poly->m_Uuid, CONSTRAINT_ANCHOR::VERTEX, 1 } );
    PCB_CONSTRAINT* endBinding = bindEndpoint( board, dim->m_Uuid, CONSTRAINT_ANCHOR::END,
                                               { poly->m_Uuid, CONSTRAINT_ANCHOR::VERTEX, 4 } );

    BOOST_CHECK( DimensionEndpointsBound( &board, dim ) );

    poly->SetPolyPoints( { pts[0], pts[1], pts[2] } );   // vertex 4 no longer exists

    std::vector<PCB_CONSTRAINT*> constraints( board.Constraints().begin(), board.Constraints().end() );
    BOARD_CONSTRAINT_ADAPTER     adapter;

    BOOST_REQUIRE( adapter.Build( { poly }, constraints, nullptr, { dim } ) );

    const std::vector<KIID>& unmapped = adapter.UnmappedConstraints();
    BOOST_CHECK( std::find( unmapped.begin(), unmapped.end(), endBinding->m_Uuid ) != unmapped.end() );
    BOOST_CHECK( std::find( unmapped.begin(), unmapped.end(), startBinding->m_Uuid ) == unmapped.end() );

    // Surviving binding still solves settled cluster stays put
    BOOST_CHECK( adapter.Solve() );
    BOOST_CHECK( adapter.Apply().empty() );

    const SHAPE_LINE_CHAIN& outline = poly->GetPolyShape().COutline( 0 );

    for( int i : { 0, 1, 2 } )
        BOOST_CHECK_EQUAL( outline.CPoint( i ), pts[i] );

    // Dialog offers Driving only through this predicate stale binding must fail it
    BOOST_CHECK( !DimensionEndpointsBound( &board, dim ) );

    // Restoring outline revives binding gate discriminates stale index from structurally broken
    poly->SetPolyPoints( pts );
    BOOST_CHECK( DimensionEndpointsBound( &board, dim ) );

    // Deleting the bound item entirely fails gate through deleted-target leg
    board.Remove( poly );
    delete poly;
    BOOST_CHECK( !DimensionEndpointsBound( &board, dim ) );
}


// Radial Driving gate needs centre and rim on one live circle or arc split legs never offer Driving
BOOST_AUTO_TEST_CASE( RadialDrivingGateRequiresSingleMappableTarget )
{
    BOARD      board;
    PCB_SHAPE* circleA = addCircle( board, { 0, 0 }, 5 * MM );
    PCB_SHAPE* circleB = addCircle( board, { 50 * MM, 0 }, 5 * MM );
    PCB_SHAPE* seg = addSegment( board, { 0, 30 * MM }, { 10 * MM, 30 * MM } );

    // Centre and rim both on circle A exactly as authored gates drivable
    PCB_DIM_RADIAL* bound = addRadialDim( board, { 0, 0 }, { 5 * MM, 0 } );
    addConstraint( board, PCB_CONSTRAINT_TYPE::COINCIDENT,
                   { { bound->m_Uuid, CONSTRAINT_ANCHOR::START }, { circleA->m_Uuid, CONSTRAINT_ANCHOR::CENTER } } );
    addConstraint( board, PCB_CONSTRAINT_TYPE::POINT_ON_LINE,
                   { { bound->m_Uuid, CONSTRAINT_ANCHOR::END }, { circleA->m_Uuid, CONSTRAINT_ANCHOR::WHOLE } } );

    BOOST_CHECK( DimensionEndpointsBound( &board, bound ) );

    // Centre on A rim on B is not a radius of either object
    PCB_DIM_RADIAL* split = addRadialDim( board, { 0, 0 }, { 55 * MM, 0 } );
    addConstraint( board, PCB_CONSTRAINT_TYPE::COINCIDENT,
                   { { split->m_Uuid, CONSTRAINT_ANCHOR::START }, { circleA->m_Uuid, CONSTRAINT_ANCHOR::CENTER } } );
    addConstraint( board, PCB_CONSTRAINT_TYPE::POINT_ON_LINE,
                   { { split->m_Uuid, CONSTRAINT_ANCHOR::END }, { circleB->m_Uuid, CONSTRAINT_ANCHOR::WHOLE } } );

    BOOST_CHECK( !DimensionEndpointsBound( &board, split ) );

    // Both legs share a segment but a segment carries no radius to drive
    PCB_DIM_RADIAL* onSegment = addRadialDim( board, { 5 * MM, 30 * MM }, { 10 * MM, 30 * MM } );
    addConstraint( board, PCB_CONSTRAINT_TYPE::COINCIDENT,
                   { { onSegment->m_Uuid, CONSTRAINT_ANCHOR::START }, { seg->m_Uuid, CONSTRAINT_ANCHOR::CENTER } } );
    addConstraint( board, PCB_CONSTRAINT_TYPE::POINT_ON_LINE,
                   { { onSegment->m_Uuid, CONSTRAINT_ANCHOR::END }, { seg->m_Uuid, CONSTRAINT_ANCHOR::WHOLE } } );

    BOOST_CHECK( !DimensionEndpointsBound( &board, onSegment ) );
}


// Rect stored end before start still binds VERTEX 0 to physical top-left not stored-start corner
BOOST_AUTO_TEST_CASE( SwappedRectVertexZeroIsPhysicalTopLeft )
{
    BOARD      board;
    PCB_SHAPE* rect = addRect( board, { 10 * MM, 10 * MM }, { 0, 0 } );   // start is BR

    PCB_SHAPE* seg = addSegment( board, { 20 * MM, 20 * MM }, { 2 * MM, 1 * MM } );

    PCB_CONSTRAINT* c = addConstraint(
            board, PCB_CONSTRAINT_TYPE::COINCIDENT,
            { { seg->m_Uuid, CONSTRAINT_ANCHOR::END }, { rect->m_Uuid, CONSTRAINT_ANCHOR::VERTEX, 0 } } );

    CONSTRAINT_DIAGNOSIS diag = ApplyConstraintImmediately( &board, c );
    BOOST_REQUIRE( diag.solved );

    // First member the segment end is pinned rect physical top-left snapped onto it opposite corner held
    BOOST_CHECK_LE( ( seg->GetEnd() - VECTOR2I( 2 * MM, 1 * MM ) ).EuclideanNorm(), 5000.0 );
    BOOST_CHECK_LE( ( rectTopLeft( rect ) - VECTOR2I( 2 * MM, 1 * MM ) ).EuclideanNorm(), 5000.0 );
    BOOST_CHECK_LE( ( rectBottomRight( rect ) - VECTOR2I( 10 * MM, 10 * MM ) ).EuclideanNorm(), 5000.0 );
}


// Insert ahead of bound vertex shifts persisted index member keeps its corner footprint constraints remap too
BOOST_AUTO_TEST_CASE( VertexInsertShiftsMembersPastInsertionPoint )
{
    BOARD board;

    std::vector<VECTOR2I> pts = { { 0, 0 },
                                  { 20 * MM, 5 * MM },
                                  { 15 * MM, 20 * MM },
                                  { 5 * MM, 20 * MM },
                                  { -5 * MM, 10 * MM } };
    PCB_SHAPE*            poly = addPoly( board, pts );
    PCB_SHAPE*            seg = addSegment( board, pts[3], { 30 * MM, 0 } );

    PCB_CONSTRAINT* onBoard = addConstraint( board, PCB_CONSTRAINT_TYPE::COINCIDENT,
                                             { { seg->m_Uuid, CONSTRAINT_ANCHOR::START },
                                               { poly->m_Uuid, CONSTRAINT_ANCHOR::VERTEX, 3 } } );

    FOOTPRINT* fp = new FOOTPRINT( &board );
    board.Add( fp );

    PCB_CONSTRAINT* onFootprint = new PCB_CONSTRAINT( fp, PCB_CONSTRAINT_TYPE::FIXED_POSITION );
    onFootprint->AddMember( poly->m_Uuid, CONSTRAINT_ANCHOR::VERTEX, 4 );
    fp->Add( onFootprint );

    // Mutate outline the way addCorner does then remap with same edit description
    poly->GetPolyShape().InsertVertex( 2, ( pts[1] + pts[2] ) / 2 );

    std::vector<BOARD_ITEM*> modified;
    std::vector<BOARD_ITEM*> removed;

    RemapPolygonVertexMembers( &board, poly->m_Uuid, 2, 1,
                               [&]( BOARD_ITEM* aItem ) { modified.push_back( aItem ); },
                               [&]( BOARD_ITEM* aItem ) { removed.push_back( aItem ); } );

    BOOST_CHECK( onBoard->GetMembers()[1] == CONSTRAINT_MEMBER( poly->m_Uuid, CONSTRAINT_ANCHOR::VERTEX, 4 ) );
    BOOST_CHECK( onFootprint->GetMembers()[0] == CONSTRAINT_MEMBER( poly->m_Uuid, CONSTRAINT_ANCHOR::VERTEX, 5 ) );

    // Remapped members resolve to the corners they were authored on
    std::optional<VECTOR2I> boardPos = ConstraintAnchorPosition( &board, onBoard->GetMembers()[1] );
    std::optional<VECTOR2I> fpPos = ConstraintAnchorPosition( &board, onFootprint->GetMembers()[0] );

    BOOST_REQUIRE( boardPos.has_value() );
    BOOST_CHECK_EQUAL( *boardPos, pts[3] );
    BOOST_REQUIRE( fpPos.has_value() );
    BOOST_CHECK_EQUAL( *fpPos, pts[4] );

    // Both re-indexed constraints were staged an insertion never retires one
    BOOST_CHECK_EQUAL( modified.size(), 2 );
    BOOST_CHECK( removed.empty() );
}


// Delete vertex below bound one shifts index down still same physical corner
BOOST_AUTO_TEST_CASE( VertexDeleteBelowShiftsMemberDown )
{
    BOARD board;

    std::vector<VECTOR2I> pts = { { 0, 0 },
                                  { 20 * MM, 5 * MM },
                                  { 15 * MM, 20 * MM },
                                  { 5 * MM, 20 * MM },
                                  { -5 * MM, 10 * MM } };
    PCB_SHAPE*            poly = addPoly( board, pts );
    PCB_SHAPE*            seg = addSegment( board, pts[3], { 30 * MM, 0 } );

    PCB_CONSTRAINT* c = addConstraint( board, PCB_CONSTRAINT_TYPE::COINCIDENT,
                                       { { seg->m_Uuid, CONSTRAINT_ANCHOR::START },
                                         { poly->m_Uuid, CONSTRAINT_ANCHOR::VERTEX, 3 } } );

    // Mutate outline the way removeCorner does
    poly->GetPolyShape().RemoveVertex( 1 );

    std::vector<BOARD_ITEM*> removed;

    RemapPolygonVertexMembers( &board, poly->m_Uuid, 1, -1,
                               []( BOARD_ITEM* ) {},
                               [&]( BOARD_ITEM* aItem ) { removed.push_back( aItem ); } );

    BOOST_CHECK( c->GetMembers()[1] == CONSTRAINT_MEMBER( poly->m_Uuid, CONSTRAINT_ANCHOR::VERTEX, 2 ) );

    std::optional<VECTOR2I> pos = ConstraintAnchorPosition( &board, c->GetMembers()[1] );

    BOOST_REQUIRE( pos.has_value() );
    BOOST_CHECK_EQUAL( *pos, pts[3] );
    BOOST_CHECK( removed.empty() );
}


// Delete bound vertex itself leaves nothing to express whole constraint staged for removal
BOOST_AUTO_TEST_CASE( VertexDeleteOfBoundVertexRetiresConstraint )
{
    BOARD board;

    std::vector<VECTOR2I> pts = { { 0, 0 },
                                  { 20 * MM, 5 * MM },
                                  { 15 * MM, 20 * MM },
                                  { 5 * MM, 20 * MM },
                                  { -5 * MM, 10 * MM } };
    PCB_SHAPE*            poly = addPoly( board, pts );
    PCB_SHAPE*            seg = addSegment( board, pts[3], { 30 * MM, 0 } );

    PCB_CONSTRAINT* c = addConstraint( board, PCB_CONSTRAINT_TYPE::COINCIDENT,
                                       { { seg->m_Uuid, CONSTRAINT_ANCHOR::START },
                                         { poly->m_Uuid, CONSTRAINT_ANCHOR::VERTEX, 3 } } );

    poly->GetPolyShape().RemoveVertex( 3 );

    std::vector<BOARD_ITEM*> modified;
    std::vector<BOARD_ITEM*> removed;

    RemapPolygonVertexMembers( &board, poly->m_Uuid, 3, -1,
                               [&]( BOARD_ITEM* aItem ) { modified.push_back( aItem ); },
                               [&]( BOARD_ITEM* aItem ) { removed.push_back( aItem ); } );

    // Retiring constraint staged whole never edited undo keeps authored members
    BOOST_REQUIRE_EQUAL( removed.size(), 1 );
    BOOST_CHECK( removed[0] == c );
    BOOST_CHECK( modified.empty() );
    BOOST_CHECK( c->GetMembers()[1] == CONSTRAINT_MEMBER( poly->m_Uuid, CONSTRAINT_ANCHOR::VERTEX, 3 ) );

    // Complete the edit the way the commit would
    board.Remove( c );
    delete c;

    BOOST_CHECK( board.Constraints().empty() );
}


// Chamfer composes insert then delete order is load-bearing reversed order nets wrong shift
BOOST_AUTO_TEST_CASE( ChamferRemapOrderNetsOneHigherPastTheCorner )
{
    BOARD board;

    std::vector<VECTOR2I> pts = { { 0, 0 },
                                  { 20 * MM, 5 * MM },
                                  { 15 * MM, 20 * MM },
                                  { 5 * MM, 20 * MM },
                                  { -5 * MM, 10 * MM } };
    PCB_SHAPE*            poly = addPoly( board, pts );

    const int k = 2;

    PCB_CONSTRAINT* below = addConstraint( board, PCB_CONSTRAINT_TYPE::FIXED_POSITION,
                                           { { poly->m_Uuid, CONSTRAINT_ANCHOR::VERTEX, k - 1 } } );
    PCB_CONSTRAINT* onCorner = addConstraint( board, PCB_CONSTRAINT_TYPE::FIXED_POSITION,
                                              { { poly->m_Uuid, CONSTRAINT_ANCHOR::VERTEX, k } } );
    PCB_CONSTRAINT* above = addConstraint( board, PCB_CONSTRAINT_TYPE::FIXED_POSITION,
                                           { { poly->m_Uuid, CONSTRAINT_ANCHOR::VERTEX, k + 1 } } );
    PCB_CONSTRAINT* both = addConstraint( board, PCB_CONSTRAINT_TYPE::COINCIDENT,
                                          { { poly->m_Uuid, CONSTRAINT_ANCHOR::VERTEX, k },
                                            { poly->m_Uuid, CONSTRAINT_ANCHOR::VERTEX, k + 2 } } );

    // Mutate outline the way chamferCorner does replacing corner k with two chamfer points
    VECTOR2I a = pts[k] + ( pts[k - 1] - pts[k] ) / 4;
    VECTOR2I b = pts[k] + ( pts[k + 1] - pts[k] ) / 4;

    poly->GetPolyShape().RemoveVertex( k );
    poly->GetPolyShape().InsertVertex( k, b );
    poly->GetPolyShape().InsertVertex( k, a );

    std::vector<BOARD_ITEM*> modified;
    std::vector<BOARD_ITEM*> removed;

    auto modify = [&]( BOARD_ITEM* aItem ) { modified.push_back( aItem ); };
    auto remove = [&]( BOARD_ITEM* aItem ) { removed.push_back( aItem ); };

    RemapPolygonVertexMembers( &board, poly->m_Uuid, k + 1, 2, modify, remove );
    RemapPolygonVertexMembers( &board, poly->m_Uuid, k, -1, modify, remove );

    BOOST_CHECK( below->GetMembers()[0] == CONSTRAINT_MEMBER( poly->m_Uuid, CONSTRAINT_ANCHOR::VERTEX, k - 1 ) );
    BOOST_CHECK( above->GetMembers()[0] == CONSTRAINT_MEMBER( poly->m_Uuid, CONSTRAINT_ANCHOR::VERTEX, k + 2 ) );

    // Netted member still resolves to its own corner one past the chamfer points
    std::optional<VECTOR2I> pos = ConstraintAnchorPosition( &board, above->GetMembers()[0] );

    BOOST_REQUIRE( pos.has_value() );
    BOOST_CHECK_EQUAL( *pos, pts[k + 1] );

    // Both constraints on the chamfered corner retire mixed one was Modify-staged first by insert pass
    BOOST_REQUIRE_EQUAL( removed.size(), 2 );
    BOOST_CHECK_EQUAL( std::ranges::count( removed, onCorner ), 1 );
    BOOST_CHECK_EQUAL( std::ranges::count( removed, both ), 1 );
    BOOST_CHECK_EQUAL( std::ranges::count( modified, both ), 1 );
    BOOST_CHECK_EQUAL( std::ranges::count( modified, onCorner ), 0 );
    BOOST_CHECK_EQUAL( std::ranges::count( modified, below ), 0 );
    BOOST_CHECK( both->GetMembers()[0] == CONSTRAINT_MEMBER( poly->m_Uuid, CONSTRAINT_ANCHOR::VERTEX, k ) );
    BOOST_CHECK( both->GetMembers()[1] == CONSTRAINT_MEMBER( poly->m_Uuid, CONSTRAINT_ANCHOR::VERTEX, k + 4 ) );
}


// Remapping scoped to edited polygon other shapes and lower indices never move
BOOST_AUTO_TEST_CASE( VertexRemapLeavesOtherShapesAndLowerIndicesAlone )
{
    BOARD board;

    std::vector<VECTOR2I> pts = { { 0, 0 },
                                  { 20 * MM, 5 * MM },
                                  { 15 * MM, 20 * MM },
                                  { 5 * MM, 20 * MM },
                                  { -5 * MM, 10 * MM } };
    PCB_SHAPE*            edited = addPoly( board, pts );

    std::vector<VECTOR2I> otherPts = { { 40 * MM, 0 }, { 60 * MM, 0 }, { 60 * MM, 20 * MM }, { 40 * MM, 20 * MM } };
    PCB_SHAPE*            other = addPoly( board, otherPts );
    PCB_SHAPE*            rect = addRect( board, { 0, 40 * MM }, { 10 * MM, 50 * MM } );
    PCB_SHAPE*            seg = addSegment( board, { 0, 60 * MM }, { 10 * MM, 60 * MM } );

    PCB_CONSTRAINT* onOther = addConstraint( board, PCB_CONSTRAINT_TYPE::FIXED_POSITION,
                                             { { other->m_Uuid, CONSTRAINT_ANCHOR::VERTEX, 3 } } );
    PCB_CONSTRAINT* onRect = addConstraint( board, PCB_CONSTRAINT_TYPE::COINCIDENT,
                                            { { seg->m_Uuid, CONSTRAINT_ANCHOR::START },
                                              { rect->m_Uuid, CONSTRAINT_ANCHOR::VERTEX, 2 } } );
    PCB_CONSTRAINT* below = addConstraint( board, PCB_CONSTRAINT_TYPE::COINCIDENT,
                                           { { seg->m_Uuid, CONSTRAINT_ANCHOR::END },
                                             { edited->m_Uuid, CONSTRAINT_ANCHOR::VERTEX, 1 } } );

    edited->GetPolyShape().InsertVertex( 2, ( pts[1] + pts[2] ) / 2 );

    std::vector<BOARD_ITEM*> modified;
    std::vector<BOARD_ITEM*> removed;

    RemapPolygonVertexMembers( &board, edited->m_Uuid, 2, 1,
                               [&]( BOARD_ITEM* aItem ) { modified.push_back( aItem ); },
                               [&]( BOARD_ITEM* aItem ) { removed.push_back( aItem ); } );

    BOOST_CHECK( onOther->GetMembers()[0] == CONSTRAINT_MEMBER( other->m_Uuid, CONSTRAINT_ANCHOR::VERTEX, 3 ) );
    BOOST_CHECK( onRect->GetMembers()[1] == CONSTRAINT_MEMBER( rect->m_Uuid, CONSTRAINT_ANCHOR::VERTEX, 2 ) );
    BOOST_CHECK( below->GetMembers()[1] == CONSTRAINT_MEMBER( edited->m_Uuid, CONSTRAINT_ANCHOR::VERTEX, 1 ) );

    // No constraint needed re-indexing so nothing was staged
    BOOST_CHECK( modified.empty() );
    BOOST_CHECK( removed.empty() );
}


// Cardinal rotation keeps RECTANGLE corner roles re-canonicalize VERTEX 0 resolves to new top-left
BOOST_AUTO_TEST_CASE( CardinalRotationRecanonicalizesRectRoles )
{
    BOARD      board;
    PCB_SHAPE* rect = addRect( board, { 2 * MM, 1 * MM }, { 12 * MM, 5 * MM } );

    std::vector<CONSTRAINT_ANCHOR_POINT> before = ConstraintShapeAnchors( rect );
    BOOST_REQUIRE_EQUAL( before.size(), 4 );

    const VECTOR2I  pivot( 0, 0 );
    const EDA_ANGLE angle( 90, DEGREES_T );

    rect->Rotate( pivot, angle );
    BOOST_REQUIRE( rect->GetShape() == SHAPE_T::RECTANGLE );

    // Physical corners after the turn from pre-rotation enumeration
    std::vector<VECTOR2I> rotated;

    for( const CONSTRAINT_ANCHOR_POINT& a : before )
    {
        VECTOR2I p = a.pos;
        RotatePoint( p, pivot, angle );
        rotated.push_back( p );
    }

    VECTOR2I tl = rotated[0];
    VECTOR2I br = rotated[0];

    for( const VECTOR2I& p : rotated )
    {
        tl.x = std::min( tl.x, p.x );
        tl.y = std::min( tl.y, p.y );
        br.x = std::max( br.x, p.x );
        br.y = std::max( br.y, p.y );
    }

    std::vector<CONSTRAINT_ANCHOR_POINT> after = ConstraintShapeAnchors( rect );
    BOOST_REQUIRE_EQUAL( after.size(), 4 );
    BOOST_CHECK_EQUAL( after[0].pos, tl );
    BOOST_CHECK_EQUAL( after[1].pos, VECTOR2I( br.x, tl.y ) );
    BOOST_CHECK_EQUAL( after[2].pos, br );
    BOOST_CHECK_EQUAL( after[3].pos, VECTOR2I( tl.x, br.y ) );

    // Role semantics not corner tracking member resolves to new top-left a different physical corner
    std::optional<VECTOR2I> v0 =
            ConstraintAnchorPosition( &board, { rect->m_Uuid, CONSTRAINT_ANCHOR::VERTEX, 0 } );

    BOOST_REQUIRE( v0.has_value() );
    BOOST_CHECK_EQUAL( *v0, tl );
    BOOST_CHECK( *v0 != rotated[0] );
}


// Non-cardinal rotation converts rect to a 4-vertex POLY under same KIID canonical order keeps bindings
BOOST_AUTO_TEST_CASE( NonCardinalRotationKeepsRectVertexBindings )
{
    BOARD board;

    auto checkRect = [&]( PCB_SHAPE* rect )
    {
        std::vector<CONSTRAINT_ANCHOR_POINT> before = ConstraintShapeAnchors( rect );
        BOOST_REQUIRE_EQUAL( before.size(), 4 );

        PCB_DIM_ALIGNED* dim = addAlignedDim( board, before[0].pos, before[1].pos );

        bindEndpoint( board, dim->m_Uuid, CONSTRAINT_ANCHOR::START,
                      { rect->m_Uuid, CONSTRAINT_ANCHOR::VERTEX, 0 } );
        bindEndpoint( board, dim->m_Uuid, CONSTRAINT_ANCHOR::END,
                      { rect->m_Uuid, CONSTRAINT_ANCHOR::VERTEX, 1 } );

        BOOST_CHECK( DimensionEndpointsBound( &board, dim ) );

        const VECTOR2I  pivot = rect->GetCenter();
        const EDA_ANGLE angle( 30, DEGREES_T );

        rect->Rotate( pivot, angle );

        BOOST_REQUIRE( rect->GetShape() == SHAPE_T::POLY );

        const SHAPE_LINE_CHAIN& outline = rect->GetPolyShape().COutline( 0 );
        BOOST_REQUIRE_EQUAL( outline.PointCount(), 4 );
        BOOST_CHECK_EQUAL( outline.ArcCount(), 0 );

        for( int i = 0; i < 4; ++i )
        {
            VECTOR2I expected = before[i].pos;
            RotatePoint( expected, pivot, angle );

            std::optional<VECTOR2I> pos =
                    ConstraintAnchorPosition( &board, { rect->m_Uuid, CONSTRAINT_ANCHOR::VERTEX, i } );

            BOOST_REQUIRE( pos.has_value() );
            BOOST_CHECK_LE( ( *pos - expected ).EuclideanNorm(), 2.0 );   // integer rounding only
        }

        BOOST_CHECK( DimensionEndpointsBound( &board, dim ) );
    };

    // One rect stored start at TL one end before start conversion normalizes both to same order
    checkRect( addRect( board, { 0, 0 }, { 10 * MM, 4 * MM } ) );
    checkRect( addRect( board, { 40 * MM, 4 * MM }, { 30 * MM, 0 } ) );
}


// Rounded rect rotated off-cardinal becomes arc-bearing outline gate rejects members degrade to unmapped
BOOST_AUTO_TEST_CASE( RoundedRectNonCardinalRotationDegradesToUnmapped )
{
    BOARD      board;
    PCB_SHAPE* rect = addRect( board, { 0, 0 }, { 10 * MM, 6 * MM } );
    rect->SetCornerRadius( 1 * MM );

    std::vector<CONSTRAINT_ANCHOR_POINT> before = ConstraintShapeAnchors( rect );
    BOOST_REQUIRE_EQUAL( before.size(), 4 );

    PCB_DIM_ALIGNED* dim = addAlignedDim( board, before[0].pos, before[1].pos );

    PCB_CONSTRAINT* startBinding = bindEndpoint( board, dim->m_Uuid, CONSTRAINT_ANCHOR::START,
                                                 { rect->m_Uuid, CONSTRAINT_ANCHOR::VERTEX, 0 } );
    PCB_CONSTRAINT* endBinding = bindEndpoint( board, dim->m_Uuid, CONSTRAINT_ANCHOR::END,
                                               { rect->m_Uuid, CONSTRAINT_ANCHOR::VERTEX, 1 } );

    BOOST_CHECK( DimensionEndpointsBound( &board, dim ) );

    rect->Rotate( { 0, 0 }, EDA_ANGLE( 30, DEGREES_T ) );

    BOOST_REQUIRE( rect->GetShape() == SHAPE_T::POLY );
    BOOST_CHECK_GT( rect->GetPolyShape().COutline( 0 ).ArcCount(), 0 );

    // No anchors no resolution no Driving offer
    BOOST_CHECK( ConstraintShapeAnchors( rect ).empty() );
    BOOST_CHECK( !ConstraintAnchorPosition( &board, { rect->m_Uuid, CONSTRAINT_ANCHOR::VERTEX, 0 } ) );
    BOOST_CHECK( !DimensionEndpointsBound( &board, dim ) );

    // Adapter leaves both bindings unmapped rather than poisoning the build
    std::vector<PCB_CONSTRAINT*> constraints( board.Constraints().begin(), board.Constraints().end() );
    BOARD_CONSTRAINT_ADAPTER     adapter;

    BOOST_REQUIRE( adapter.Build( { rect }, constraints, nullptr, { dim } ) );

    const std::vector<KIID>& unmapped = adapter.UnmappedConstraints();
    BOOST_CHECK( std::find( unmapped.begin(), unmapped.end(), startBinding->m_Uuid ) != unmapped.end() );
    BOOST_CHECK( std::find( unmapped.begin(), unmapped.end(), endBinding->m_Uuid ) != unmapped.end() );
}


// Driving gate scans footprint-parented constraints too stale footprint binding revokes it
BOOST_AUTO_TEST_CASE( FootprintParentedBindingGatesDriving )
{
    BOARD      board;
    FOOTPRINT* fp = new FOOTPRINT( &board );
    board.Add( fp );

    PCB_SHAPE* rect = new PCB_SHAPE( fp, SHAPE_T::RECTANGLE );
    rect->SetStart( { 0, 0 } );
    rect->SetEnd( { 10 * MM, 10 * MM } );
    fp->Add( rect );

    PCB_DIM_ALIGNED* dim = addAlignedDim( board, { 0, 0 }, { 10 * MM, 0 } );

    auto bindOnFootprint = [&]( CONSTRAINT_ANCHOR aDimAnchor, int aCorner )
    {
        PCB_CONSTRAINT* c = new PCB_CONSTRAINT( fp, PCB_CONSTRAINT_TYPE::COINCIDENT );
        c->AddMember( dim->m_Uuid, aDimAnchor );
        c->AddMember( rect->m_Uuid, CONSTRAINT_ANCHOR::VERTEX, aCorner );
        fp->Add( c );
        return c;
    };

    bindOnFootprint( CONSTRAINT_ANCHOR::START, 0 );
    PCB_CONSTRAINT* endBinding = bindOnFootprint( CONSTRAINT_ANCHOR::END, 1 );

    // Nothing lives at board level drivable verdict can only come from footprint scan
    BOOST_CHECK( board.Constraints().empty() );
    BOOST_CHECK( DimensionEndpointsBound( &board, dim ) );

    // Stale vertex index on footprint-parented binding no longer resolves gate reports undrivable
    endBinding->Members()[1].m_index = 7;
    BOOST_CHECK( !DimensionEndpointsBound( &board, dim ) );
}


namespace
{
// Staging callbacks a live commit would provide recording touched items and completing add remove
struct MODE_STAGING
{
    BOARD&                   board;
    std::vector<BOARD_ITEM*> modified;
    std::vector<BOARD_ITEM*> added;
    std::vector<BOARD_ITEM*> removed;

    std::function<void( BOARD_ITEM* )> modify()
    {
        return [this]( BOARD_ITEM* aItem ) { modified.push_back( aItem ); };
    }

    std::function<void( BOARD_ITEM* )> add()
    {
        return [this]( BOARD_ITEM* aItem )
        {
            added.push_back( aItem );
            board.Add( aItem );
        };
    }

    std::function<void( BOARD_ITEM* )> remove()
    {
        return [this]( BOARD_ITEM* aItem )
        {
            removed.push_back( aItem );
            board.Remove( aItem );
            delete aItem;
        };
    }
};
}


// Value mode derives from board state Driven default Arbitrary on override Driving needs self length
// Only aligned orthogonal radial dimensions carry a mode
BOOST_AUTO_TEST_CASE( ValueModeDerivation )
{
    BOARD            board;
    PCB_DIM_ALIGNED* dim = addAlignedDim( board, { 0, 0 }, { 10 * MM, 0 } );

    BOOST_CHECK( DimensionValueMode( &board, dim ) == DIM_VALUE_MODE::DRIVEN );
    BOOST_CHECK( dim->GetValueMode() == DIM_VALUE_MODE::DRIVEN );

    dim->ChangeOverrideText( wxS( "custom" ) );
    BOOST_CHECK( DimensionValueMode( &board, dim ) == DIM_VALUE_MODE::ARBITRARY );

    PCB_CONSTRAINT* reference = addConstraint(
            board, PCB_CONSTRAINT_TYPE::FIXED_LENGTH,
            { { dim->m_Uuid, CONSTRAINT_ANCHOR::START }, { dim->m_Uuid, CONSTRAINT_ANCHOR::END } },
            10.0 * MM );
    reference->SetDriving( false );

    BOOST_CHECK( FindDimensionLengthConstraint( &board, dim ) == reference );
    BOOST_CHECK( DimensionValueMode( &board, dim ) == DIM_VALUE_MODE::ARBITRARY );

    dim->SetOverrideTextEnabled( false );
    BOOST_CHECK( DimensionValueMode( &board, dim ) == DIM_VALUE_MODE::DRIVEN );

    reference->SetDriving( true );
    BOOST_CHECK( DimensionValueMode( &board, dim ) == DIM_VALUE_MODE::DRIVING );

    // Only value-bearing families carry the mode
    PCB_DIM_LEADER* leader = new PCB_DIM_LEADER( &board );
    board.Add( leader );

    BOOST_CHECK( DimensionHasValueMode( dim ) );
    BOOST_CHECK( !DimensionHasValueMode( leader ) );
    BOOST_CHECK( !DimensionHasValueMode( nullptr ) );
}


// Mode transitions through shared setter Driven to Driving creates length Arbitrary installs text
// Driven again clears override every touched item goes through staging callbacks
BOOST_AUTO_TEST_CASE( SetValueModeTransitions )
{
    BOARD      board;
    PCB_SHAPE* rect = addRect( board, { 0, 0 }, { 10 * MM, 10 * MM } );

    PCB_DIM_ALIGNED* dim = addAlignedDim( board, { 0, 0 }, { 10 * MM, 0 } );

    bindEndpoint( board, dim->m_Uuid, CONSTRAINT_ANCHOR::START,
                  { rect->m_Uuid, CONSTRAINT_ANCHOR::VERTEX, 0 } );
    bindEndpoint( board, dim->m_Uuid, CONSTRAINT_ANCHOR::END,
                  { rect->m_Uuid, CONSTRAINT_ANCHOR::VERTEX, 1 } );

    BOOST_REQUIRE( DimensionCanDrive( &board, dim ) );

    // Driven to Driving creates the self driving length with the seeded value
    MODE_STAGING    toDriving{ board };
    PCB_CONSTRAINT* driving =
            SetDimensionValueMode( &board, dim, DIM_VALUE_MODE::DRIVING, 12 * MM, std::nullopt,
                                   toDriving.modify(), toDriving.add(), toDriving.remove() );

    BOOST_REQUIRE( driving );
    BOOST_CHECK( FindDimensionLengthConstraint( &board, dim ) == driving );
    BOOST_CHECK( driving->IsDriving() );
    BOOST_REQUIRE( driving->GetValue().has_value() );
    BOOST_CHECK_EQUAL( *driving->GetValue(), 12 * MM );
    BOOST_CHECK( DimensionValueMode( &board, dim ) == DIM_VALUE_MODE::DRIVING );
    BOOST_CHECK_EQUAL( toDriving.added.size(), 1 );
    BOOST_CHECK( toDriving.added[0] == driving );
    BOOST_CHECK( toDriving.removed.empty() );

    // Driving to Arbitrary retires the driving length and installs custom text
    MODE_STAGING toArbitrary{ board };
    PCB_CONSTRAINT* result =
            SetDimensionValueMode( &board, dim, DIM_VALUE_MODE::ARBITRARY, std::nullopt,
                                   wxString( wxS( "REF" ) ), toArbitrary.modify(), toArbitrary.add(),
                                   toArbitrary.remove() );

    BOOST_CHECK( !result );
    BOOST_CHECK( !FindDimensionLengthConstraint( &board, dim ) );
    BOOST_CHECK( DimensionValueMode( &board, dim ) == DIM_VALUE_MODE::ARBITRARY );
    BOOST_CHECK_EQUAL( dim->GetOverrideText(), wxS( "REF" ) );
    BOOST_CHECK_EQUAL( toArbitrary.removed.size(), 1 );

    // Arbitrary to Driven just clears the override bindings stay
    MODE_STAGING toDriven{ board };
    SetDimensionValueMode( &board, dim, DIM_VALUE_MODE::DRIVEN, std::nullopt, std::nullopt,
                           toDriven.modify(), toDriven.add(), toDriven.remove() );

    BOOST_CHECK( DimensionValueMode( &board, dim ) == DIM_VALUE_MODE::DRIVEN );
    BOOST_CHECK( !dim->GetOverrideTextEnabled() );
    BOOST_CHECK( toDriven.added.empty() );
    BOOST_CHECK( toDriven.removed.empty() );
    BOOST_CHECK( DimensionEndpointsBound( &board, dim ) );
}


// Driving transition rejected board untouched when unbound or length absent or non-positive
BOOST_AUTO_TEST_CASE( SetValueModeDrivingRejections )
{
    BOARD            board;
    PCB_DIM_ALIGNED* unbound = addAlignedDim( board, { 0, 0 }, { 10 * MM, 0 } );

    MODE_STAGING staging{ board };

    BOOST_CHECK( !SetDimensionValueMode( &board, unbound, DIM_VALUE_MODE::DRIVING, 12 * MM,
                                         std::nullopt, staging.modify(), staging.add(),
                                         staging.remove() ) );

    PCB_SHAPE*       rect = addRect( board, { 0, 0 }, { 10 * MM, 10 * MM } );
    PCB_DIM_ALIGNED* bound = addAlignedDim( board, { 0, 0 }, { 10 * MM, 0 } );

    bindEndpoint( board, bound->m_Uuid, CONSTRAINT_ANCHOR::START,
                  { rect->m_Uuid, CONSTRAINT_ANCHOR::VERTEX, 0 } );
    bindEndpoint( board, bound->m_Uuid, CONSTRAINT_ANCHOR::END,
                  { rect->m_Uuid, CONSTRAINT_ANCHOR::VERTEX, 1 } );

    BOOST_CHECK( !SetDimensionValueMode( &board, bound, DIM_VALUE_MODE::DRIVING, std::nullopt,
                                         std::nullopt, staging.modify(), staging.add(),
                                         staging.remove() ) );
    BOOST_CHECK( !SetDimensionValueMode( &board, bound, DIM_VALUE_MODE::DRIVING, 0, std::nullopt,
                                         staging.modify(), staging.add(), staging.remove() ) );
    BOOST_CHECK( !SetDimensionValueMode( &board, bound, DIM_VALUE_MODE::DRIVING, -5 * MM,
                                         std::nullopt, staging.modify(), staging.add(),
                                         staging.remove() ) );

    // A leader never takes a value mode at all
    PCB_DIM_LEADER* leader = new PCB_DIM_LEADER( &board );
    board.Add( leader );

    BOOST_CHECK( !SetDimensionValueMode( &board, leader, DIM_VALUE_MODE::DRIVING, 12 * MM,
                                         std::nullopt, staging.modify(), staging.add(),
                                         staging.remove() ) );

    BOOST_CHECK( staging.modified.empty() );
    BOOST_CHECK( staging.added.empty() );
    BOOST_CHECK( staging.removed.empty() );
    BOOST_CHECK( !FindDimensionLengthConstraint( &board, unbound ) );
    BOOST_CHECK( !FindDimensionLengthConstraint( &board, bound ) );
    BOOST_CHECK( DimensionValueMode( &board, unbound ) == DIM_VALUE_MODE::DRIVEN );
    BOOST_CHECK( DimensionValueMode( &board, bound ) == DIM_VALUE_MODE::DRIVEN );
}


// Panel Driving flow end to end shared setter authors length solving drives rect to width
BOOST_AUTO_TEST_CASE( SetValueModeDrivingResolvesGeometry )
{
    BOARD      board;
    PCB_SHAPE* rect = addRect( board, { 0, 0 }, { 10 * MM, 10 * MM } );

    PCB_DIM_ALIGNED* dim = addAlignedDim( board, { 0, 0 }, { 10 * MM, 0 } );   // TL to TR

    bindEndpoint( board, dim->m_Uuid, CONSTRAINT_ANCHOR::START,
                  { rect->m_Uuid, CONSTRAINT_ANCHOR::VERTEX, 0 } );
    bindEndpoint( board, dim->m_Uuid, CONSTRAINT_ANCHOR::END,
                  { rect->m_Uuid, CONSTRAINT_ANCHOR::VERTEX, 1 } );

    MODE_STAGING    staging{ board };
    PCB_CONSTRAINT* driving =
            SetDimensionValueMode( &board, dim, DIM_VALUE_MODE::DRIVING, 15 * MM, std::nullopt,
                                   staging.modify(), staging.add(), staging.remove() );

    BOOST_REQUIRE( driving );

    CONSTRAINT_DIAGNOSIS diag = ApplyConstraintImmediately( &board, driving );
    BOOST_REQUIRE( diag.solved );

    const VECTOR2I tl = rectTopLeft( rect );
    const VECTOR2I br = rectBottomRight( rect );

    BOOST_CHECK_LE( std::abs( ( br.x - tl.x ) - 15 * MM ), 5000 );   // width driven to the value
    BOOST_CHECK_LE( std::abs( ( br.y - tl.y ) - 10 * MM ), 5000 );   // height untouched
    BOOST_CHECK_LE( tl.EuclideanNorm(), 5000.0 );                    // pinned TL corner held

    // Dimension endpoints ride the corners value field text reflects driving length not stale measurement
    BOOST_CHECK_LE( ( dim->GetStart() - tl ).EuclideanNorm(), 5000.0 );
    BOOST_CHECK_LE( ( dim->GetEnd() - VECTOR2I( br.x, tl.y ) ).EuclideanNorm(), 5000.0 );
    BOOST_CHECK( dim->GetValueMode() == DIM_VALUE_MODE::DRIVING );
}


// Property surface the docked panel drives Value Mode and Value exist only for value-bearing dims
// Value read-only while Driven validators gate an unbound Driving switch and non-positive length
BOOST_AUTO_TEST_CASE( ValueModePropertySurface )
{
    BOARD      board;
    PCB_SHAPE* rect = addRect( board, { 0, 0 }, { 10 * MM, 10 * MM } );

    PCB_DIM_ALIGNED* dim = addAlignedDim( board, { 0, 0 }, { 10 * MM, 0 } );

    bindEndpoint( board, dim->m_Uuid, CONSTRAINT_ANCHOR::START,
                  { rect->m_Uuid, CONSTRAINT_ANCHOR::VERTEX, 0 } );
    bindEndpoint( board, dim->m_Uuid, CONSTRAINT_ANCHOR::END,
                  { rect->m_Uuid, CONSTRAINT_ANCHOR::VERTEX, 1 } );

    PCB_DIM_LEADER* leader = new PCB_DIM_LEADER( &board );
    board.Add( leader );

    PROPERTY_MANAGER& propMgr = PROPERTY_MANAGER::Instance();
    propMgr.Rebuild();

    PROPERTY_BASE* modeProp = propMgr.GetProperty( TYPE_HASH( PCB_DIMENSION_BASE ), _HKI( "Value Mode" ) );
    PROPERTY_BASE* valueProp = propMgr.GetProperty( TYPE_HASH( PCB_DIMENSION_BASE ), _HKI( "Value" ) );

    BOOST_REQUIRE( modeProp );
    BOOST_REQUIRE( valueProp );

    BOOST_CHECK( propMgr.IsAvailableFor( TYPE_HASH( PCB_DIM_ALIGNED ), modeProp, dim ) );
    BOOST_CHECK( propMgr.IsAvailableFor( TYPE_HASH( PCB_DIM_ALIGNED ), valueProp, dim ) );
    BOOST_CHECK( !propMgr.IsAvailableFor( TYPE_HASH( PCB_DIM_LEADER ), modeProp, leader ) );
    BOOST_CHECK( !propMgr.IsAvailableFor( TYPE_HASH( PCB_DIM_LEADER ), valueProp, leader ) );

    // Driven shows the measured value read-only
    BOOST_CHECK( !propMgr.IsWriteableFor( TYPE_HASH( PCB_DIM_ALIGNED ), valueProp, dim ) );
    BOOST_CHECK_EQUAL( dim->GetValueFieldText(), dim->GetValueText() );

    // A bound dimension may switch to Driving same switch on an unbound one is vetoed
    BOOST_CHECK( !modeProp->Validate( wxAny( static_cast<int>( DIM_VALUE_MODE::DRIVING ) ), dim ) );

    PCB_DIM_ALIGNED* unbound = addAlignedDim( board, { 0, 20 * MM }, { 10 * MM, 20 * MM } );

    BOOST_CHECK( modeProp->Validate( wxAny( static_cast<int>( DIM_VALUE_MODE::DRIVING ) ), unbound ) );
    BOOST_CHECK( !modeProp->Validate( wxAny( static_cast<int>( DIM_VALUE_MODE::ARBITRARY ) ), unbound ) );

    // Arbitrary makes the value writable and verbatim through the property setter
    dim->ChangeValueMode( DIM_VALUE_MODE::ARBITRARY );

    BOOST_CHECK( propMgr.IsWriteableFor( TYPE_HASH( PCB_DIM_ALIGNED ), valueProp, dim ) );

    dim->ChangeValueFieldText( wxS( "10 mm MAX" ) );
    BOOST_CHECK_EQUAL( dim->GetValueFieldText(), wxS( "10 mm MAX" ) );
    BOOST_CHECK_EQUAL( dim->GetOverrideText(), wxS( "10 mm MAX" ) );

    // Driving gates the value to a positive length and shows the constraint value
    dim->ChangeValueMode( DIM_VALUE_MODE::DRIVEN );

    MODE_STAGING staging{ board };
    SetDimensionValueMode( &board, dim, DIM_VALUE_MODE::DRIVING, 12 * MM, std::nullopt,
                           staging.modify(), staging.add(), staging.remove() );

    BOOST_CHECK( valueProp->Validate( wxAny( wxString( wxS( "0" ) ) ), dim ) );
    BOOST_CHECK( valueProp->Validate( wxAny( wxString( wxS( "-3" ) ) ), dim ) );
    BOOST_CHECK( valueProp->Validate( wxAny( wxString( wxS( "text" ) ) ), dim ) );
    BOOST_CHECK( !valueProp->Validate( wxAny( wxString( wxS( "15" ) ) ), dim ) );

    // A Driven dimension accepts anything the setter will ignore so no veto there
    BOOST_CHECK( !valueProp->Validate( wxAny( wxString( wxS( "0" ) ) ), unbound ) );
}


BOOST_AUTO_TEST_SUITE_END()
