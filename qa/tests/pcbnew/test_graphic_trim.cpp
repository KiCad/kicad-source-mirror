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

#include <board.h>
#include <geometry/shape_arc.h>
#include <layer_ids.h>
#include <pcb_shape.h>
#include <pcb_track.h>
#include <tools/graphic_trim.h>

#include <limits>


BOOST_AUTO_TEST_SUITE( GraphicTrim )


static PCB_SHAPE makeLine( const VECTOR2I& aStart, const VECTOR2I& aEnd, PCB_LAYER_ID aLayer = Dwgs_User )
{
    PCB_SHAPE line( nullptr, SHAPE_T::SEGMENT );
    line.SetStart( aStart );
    line.SetEnd( aEnd );
    line.SetLayer( aLayer );
    return line;
}


static PCB_SHAPE makeArc( const VECTOR2I& aCenter, const VECTOR2I& aStart, double aAngle,
                          PCB_LAYER_ID aLayer = Dwgs_User )
{
    SHAPE_ARC geometry( aCenter, aStart, EDA_ANGLE( aAngle, DEGREES_T ) );
    PCB_SHAPE arc( nullptr, SHAPE_T::ARC );
    arc.SetArcGeometry( geometry.GetP0(), geometry.GetArcMid(), geometry.GetP1() );
    arc.SetLayer( aLayer );
    return arc;
}


// One cut keeps the side the pointer is not on.
BOOST_AUTO_TEST_CASE( OneBoundaryRemovesThePointerSide )
{
    PCB_SHAPE source = makeLine( { 0, 0 }, { 100, 0 } );
    PCB_SHAPE boundary = makeLine( { 70, -10 }, { 70, 10 } );

    GRAPHIC_EDIT_RESULT afterCut = GRAPHIC_TRIM_PLANNER::Plan( source, { 90, 2 }, { &boundary } );

    BOOST_REQUIRE( afterCut );
    BOOST_REQUIRE_EQUAL( afterCut.m_Geometry.size(), 1 );
    BOOST_CHECK_EQUAL( afterCut.m_Geometry[0].m_Start, VECTOR2I( 0, 0 ) );
    BOOST_CHECK_EQUAL( afterCut.m_Geometry[0].m_End, VECTOR2I( 70, 0 ) );

    GRAPHIC_EDIT_RESULT beforeCut = GRAPHIC_TRIM_PLANNER::Plan( source, { 10, -2 }, { &boundary } );

    BOOST_REQUIRE( beforeCut );
    BOOST_REQUIRE_EQUAL( beforeCut.m_Geometry.size(), 1 );
    BOOST_CHECK_EQUAL( beforeCut.m_Geometry[0].m_Start, VECTOR2I( 70, 0 ) );
    BOOST_CHECK_EQUAL( beforeCut.m_Geometry[0].m_End, VECTOR2I( 100, 0 ) );
}


BOOST_AUTO_TEST_CASE( TwoBoundariesRemoveMiddleAndRetainExteriorPieces )
{
    PCB_SHAPE source = makeLine( { 0, 0 }, { 100, 0 } );
    PCB_SHAPE left = makeLine( { 30, -10 }, { 30, 10 } );
    PCB_SHAPE right = makeLine( { 70, -10 }, { 70, 10 } );

    GRAPHIC_EDIT_RESULT result = GRAPHIC_TRIM_PLANNER::Plan( source, { 50, 3 }, { &right, &left } );

    BOOST_REQUIRE( result );
    BOOST_REQUIRE_EQUAL( result.m_Geometry.size(), 2 );
    BOOST_CHECK_EQUAL( result.m_Geometry[0].m_Start, VECTOR2I( 0, 0 ) );
    BOOST_CHECK_EQUAL( result.m_Geometry[0].m_End, VECTOR2I( 30, 0 ) );
    BOOST_CHECK_EQUAL( result.m_Geometry[1].m_Start, VECTOR2I( 70, 0 ) );
    BOOST_CHECK_EQUAL( result.m_Geometry[1].m_End, VECTOR2I( 100, 0 ) );
}


BOOST_AUTO_TEST_CASE( ChoosesNearestIntersectionsAroundClickedInterval )
{
    PCB_SHAPE source = makeLine( { 0, 0 }, { 100, 0 } );
    PCB_SHAPE cut10 = makeLine( { 10, -10 }, { 10, 10 } );
    PCB_SHAPE cut30 = makeLine( { 30, -10 }, { 30, 10 } );
    PCB_SHAPE cut70 = makeLine( { 70, -10 }, { 70, 10 } );
    PCB_SHAPE cut90 = makeLine( { 90, -10 }, { 90, 10 } );

    GRAPHIC_EDIT_RESULT result = GRAPHIC_TRIM_PLANNER::Plan( source, { 50, 0 }, { &cut90, &cut10, &cut70, &cut30 } );

    BOOST_REQUIRE( result );
    BOOST_REQUIRE_EQUAL( result.m_Geometry.size(), 2 );
    BOOST_CHECK_EQUAL( result.m_Geometry[0].m_End, VECTOR2I( 30, 0 ) );
    BOOST_CHECK_EQUAL( result.m_Geometry[1].m_Start, VECTOR2I( 70, 0 ) );
}


BOOST_AUTO_TEST_CASE( TrimsLineAtFiniteArcBoundary )
{
    PCB_SHAPE source = makeLine( { 0, 0 }, { 100, 0 } );
    PCB_SHAPE boundary = makeArc( { 70, 0 }, { 60, 0 }, 180.0 );

    GRAPHIC_EDIT_RESULT result = GRAPHIC_TRIM_PLANNER::Plan( source, { 95, 0 }, { &boundary } );

    BOOST_REQUIRE( result );
    BOOST_REQUIRE_EQUAL( result.m_Geometry.size(), 1 );
    BOOST_CHECK_EQUAL( result.m_Geometry[0].m_End, VECTOR2I( 80, 0 ) );
}


// Board scale.  At a 100 IU radius the rounding is percent-level and swamps the cut.
BOOST_AUTO_TEST_CASE( TrimsArcAtFiniteLineBoundaries )
{
    PCB_SHAPE source = makeArc( { 0, 0 }, { 100000, 0 }, 180.0 );
    VECTOR2I  firstCut = makeArc( { 0, 0 }, { 100000, 0 }, 45.0 ).GetEnd();
    VECTOR2I  secondCut = makeArc( { 0, 0 }, { 100000, 0 }, 135.0 ).GetEnd();
    PCB_SHAPE firstBoundary = makeLine( { 0, 0 }, firstCut * 2 );
    PCB_SHAPE secondBoundary = makeLine( { 0, 0 }, secondCut * 2 );

    GRAPHIC_EDIT_RESULT result =
            GRAPHIC_TRIM_PLANNER::Plan( source, source.GetArcMid(), { &secondBoundary, &firstBoundary } );

    BOOST_REQUIRE( result );
    BOOST_REQUIRE_EQUAL( result.m_Geometry.size(), 2 );
    BOOST_CHECK_EQUAL( result.m_Geometry[0].m_Start, source.GetStart() );
    BOOST_CHECK_LE( result.m_Geometry[0].m_End.Distance( firstCut ), 2 );
    BOOST_CHECK_LE( result.m_Geometry[1].m_Start.Distance( secondCut ), 2 );
    BOOST_CHECK_EQUAL( result.m_Geometry[1].m_End, source.GetEnd() );
}


BOOST_AUTO_TEST_CASE( TrimsArcAtFiniteArcBoundaryAndPreservesWinding )
{
    // Board scale again.  At 100 IU the rounding says nothing about the winding.
    PCB_SHAPE source = makeArc( { 0, 0 }, { 100000, 0 }, -180.0 );
    SHAPE_ARC sourceGeometry( source.GetStart(), source.GetArcMid(), source.GetEnd(), 0 );
    VECTOR2I  cut = sourceGeometry.GetArcMid();
    VECTOR2I  radial = cut - sourceGeometry.GetCenter();
    VECTOR2I  tangent( -radial.y / 5, radial.x / 5 );
    PCB_SHAPE boundary = makeArc( cut + tangent, cut, 270.0 );

    GRAPHIC_EDIT_RESULT result = GRAPHIC_TRIM_PLANNER::Plan( source, source.GetEnd(), { &boundary } );

    BOOST_REQUIRE( result );
    BOOST_REQUIRE_EQUAL( result.m_Geometry.size(), 1 );
    SHAPE_ARC retained( result.m_Geometry[0].m_Start, result.m_Geometry[0].m_Mid, result.m_Geometry[0].m_End, 0 );
    BOOST_CHECK_EQUAL( result.m_Geometry[0].m_Start, source.GetStart() );
    BOOST_CHECK_NE( result.m_Geometry[0].m_End, source.GetEnd() );
    BOOST_CHECK_EQUAL( retained.IsClockwise(), sourceGeometry.IsClockwise() );
    BOOST_CHECK_CLOSE( retained.GetRadius(), sourceGeometry.GetRadius(), 0.01 );
}


BOOST_AUTO_TEST_CASE( RejectsTrackSourceAndIgnoresTracksAndWrongLayerBoundaries )
{
    PCB_TRACK trackSource( nullptr );
    trackSource.SetStart( { 0, 0 } );
    trackSource.SetEnd( { 100, 0 } );
    PCB_SHAPE source = makeLine( { 0, 0 }, { 100, 0 } );
    PCB_TRACK trackBoundary( nullptr );
    trackBoundary.SetStart( { 50, -10 } );
    trackBoundary.SetEnd( { 50, 10 } );
    PCB_SHAPE wrongLayer = makeLine( { 70, -10 }, { 70, 10 }, Cmts_User );

    BOOST_CHECK( !GRAPHIC_TRIM_PLANNER::Plan( trackSource, { 50, 0 }, { &source } ) );
    BOOST_CHECK( !GRAPHIC_TRIM_PLANNER::Plan( source, { 90, 0 }, { &trackBoundary, &wrongLayer } ) );
}


BOOST_AUTO_TEST_CASE( LockedSourceIsRejectedButLockedBoundaryIsEligible )
{
    BOARD     board;
    PCB_SHAPE source( &board, SHAPE_T::SEGMENT );
    source.SetStart( { 0, 0 } );
    source.SetEnd( { 100, 0 } );
    source.SetLayer( Dwgs_User );
    PCB_SHAPE boundary( &board, SHAPE_T::SEGMENT );
    boundary.SetStart( { 70, -10 } );
    boundary.SetEnd( { 70, 10 } );
    boundary.SetLayer( Dwgs_User );
    boundary.SetLocked( true );
    BOOST_CHECK( GRAPHIC_TRIM_PLANNER::Plan( source, { 90, 0 }, { &boundary } ) );

    source.SetLocked( true );
    GRAPHIC_EDIT_RESULT result = GRAPHIC_TRIM_PLANNER::Plan( source, { 90, 0 }, { &boundary } );
    BOOST_CHECK( !result );
    BOOST_CHECK( result.m_Refusal == GRAPHIC_EDIT_REFUSAL::LOCKED_SOURCE );
}


BOOST_AUTO_TEST_CASE( RefusesCollinearOverlapAndPointerOnCut )
{
    PCB_SHAPE source = makeLine( { 0, 0 }, { 100, 0 } );
    PCB_SHAPE collinear = makeLine( { 20, 0 }, { 80, 0 } );
    PCB_SHAPE middle = makeLine( { 50, -10 }, { 50, 10 } );

    // A shared run has no one point to cut at.
    GRAPHIC_EDIT_RESULT overlap = GRAPHIC_TRIM_PLANNER::Plan( source, { 50, 0 }, { &collinear } );
    BOOST_CHECK( !overlap );
    BOOST_CHECK( overlap.m_Refusal == GRAPHIC_EDIT_REFUSAL::AMBIGUOUS );

    // The pointer is on the cut, so neither side is the one meant.
    GRAPHIC_EDIT_RESULT onCut = GRAPHIC_TRIM_PLANNER::Plan( source, { 50, 0 }, { &middle } );
    BOOST_CHECK( !onCut );
    BOOST_CHECK( onCut.m_Refusal == GRAPHIC_EDIT_REFUSAL::AMBIGUOUS );
}


// A graze meets the source at one point, which is all a cut needs.
BOOST_AUTO_TEST_CASE( TangentBoundaryStillCuts )
{
    PCB_SHAPE source = makeLine( { 0, 0 }, { 100000, 0 } );
    PCB_SHAPE tangent = makeArc( { 50000, 10000 }, { 50000, 0 }, 180.0 );

    GRAPHIC_EDIT_RESULT result = GRAPHIC_TRIM_PLANNER::Plan( source, { 90000, 0 }, { &tangent } );

    BOOST_REQUIRE( result );
    BOOST_REQUIRE_EQUAL( result.m_Geometry.size(), 1 );
    BOOST_CHECK_LE( result.m_Geometry[0].m_End.Distance( VECTOR2I( 50000, 0 ) ), 2 );
}


// The stem of a T is bounded at one end and free at the other.  Trimming it removes all of it.
BOOST_AUTO_TEST_CASE( ShapeBoundedOnlyAtAnEndIsRemovedWhole )
{
    PCB_SHAPE stem = makeLine( { 50000, 0 }, { 50000, 100000 } );
    PCB_SHAPE crossbar = makeLine( { 0, 0 }, { 100000, 0 } );

    GRAPHIC_EDIT_RESULT result = GRAPHIC_TRIM_PLANNER::Plan( stem, { 50000, 50000 }, { &crossbar } );

    BOOST_REQUIRE( result );
    BOOST_CHECK( result.m_Geometry.empty() );
    BOOST_REQUIRE_EQUAL( result.m_Preview.size(), 1 );
    BOOST_CHECK_EQUAL( result.m_Preview[0].m_Start, VECTOR2I( 50000, 0 ) );
    BOOST_CHECK_EQUAL( result.m_Preview[0].m_End, VECTOR2I( 50000, 100000 ) );
}


// The other two arms of the same T cut normally, either side of the stem.
BOOST_AUTO_TEST_CASE( EitherArmOfATeeTrimsAtTheStem )
{
    PCB_SHAPE crossbar = makeLine( { 0, 0 }, { 100000, 0 } );
    PCB_SHAPE stem = makeLine( { 50000, 0 }, { 50000, 100000 } );

    GRAPHIC_EDIT_RESULT left = GRAPHIC_TRIM_PLANNER::Plan( crossbar, { 10000, 0 }, { &stem } );

    BOOST_REQUIRE( left );
    BOOST_REQUIRE_EQUAL( left.m_Geometry.size(), 1 );
    BOOST_CHECK_EQUAL( left.m_Geometry[0].m_Start, VECTOR2I( 50000, 0 ) );
    BOOST_CHECK_EQUAL( left.m_Geometry[0].m_End, VECTOR2I( 100000, 0 ) );

    GRAPHIC_EDIT_RESULT right = GRAPHIC_TRIM_PLANNER::Plan( crossbar, { 90000, 0 }, { &stem } );

    BOOST_REQUIRE( right );
    BOOST_REQUIRE_EQUAL( right.m_Geometry.size(), 1 );
    BOOST_CHECK_EQUAL( right.m_Geometry[0].m_Start, VECTOR2I( 0, 0 ) );
    BOOST_CHECK_EQUAL( right.m_Geometry[0].m_End, VECTOR2I( 50000, 0 ) );
}


// The preview is the span that goes, not what is left.
BOOST_AUTO_TEST_CASE( PreviewCarriesTheRemovedSpan )
{
    PCB_SHAPE source = makeLine( { 0, 0 }, { 100000, 0 } );
    PCB_SHAPE left = makeLine( { 30000, -10000 }, { 30000, 10000 } );
    PCB_SHAPE right = makeLine( { 70000, -10000 }, { 70000, 10000 } );

    GRAPHIC_EDIT_RESULT result = GRAPHIC_TRIM_PLANNER::Plan( source, { 50000, 0 }, { &left, &right } );

    BOOST_REQUIRE( result );
    BOOST_REQUIRE_EQUAL( result.m_Preview.size(), 1 );
    BOOST_CHECK_EQUAL( result.m_Preview[0].m_Start, VECTOR2I( 30000, 0 ) );
    BOOST_CHECK_EQUAL( result.m_Preview[0].m_End, VECTOR2I( 70000, 0 ) );
}


BOOST_AUTO_TEST_CASE( IgnoresUnrelatedCollinearAndTangentGeometry )
{
    PCB_SHAPE source = makeLine( { 0, 0 }, { 100, 0 } );
    PCB_SHAPE remoteCollinear = makeLine( { 200, 0 }, { 300, 0 } );
    PCB_SHAPE remoteTangent = makeArc( { 200, 10 }, { 200, 0 }, 180.0 );
    PCB_SHAPE validCut = makeLine( { 70, -10 }, { 70, 10 } );

    GRAPHIC_EDIT_RESULT result =
            GRAPHIC_TRIM_PLANNER::Plan( source, { 90, 0 }, { &remoteCollinear, &remoteTangent, &validCut } );

    BOOST_REQUIRE( result );
    BOOST_CHECK_EQUAL( result.m_Geometry[0].m_End, VECTOR2I( 70, 0 ) );
}


// An overlap contributes no cut of its own, but it does not spoil one that another shape gives.
BOOST_AUTO_TEST_CASE( OverlapDoesNotSpoilAUsableCut )
{
    PCB_SHAPE source = makeLine( { 0, 0 }, { 100, 0 } );
    PCB_SHAPE overlap = makeLine( { 20, 0 }, { 80, 0 } );
    PCB_SHAPE validCut = makeLine( { 70, -10 }, { 70, 10 } );

    GRAPHIC_EDIT_RESULT result = GRAPHIC_TRIM_PLANNER::Plan( source, { 90, 0 }, { &overlap, &validCut } );

    BOOST_REQUIRE( result );
    BOOST_REQUIRE_EQUAL( result.m_Geometry.size(), 1 );
    BOOST_CHECK_EQUAL( result.m_Geometry[0].m_End, VECTOR2I( 70, 0 ) );
}


// SEG::TCoef() wraps on a span wider than the coordinate range, so a metre is the most the
// parameter can describe.
BOOST_AUTO_TEST_CASE( HandlesBoardScaleParameter )
{
    constexpr int reach = 1000000000;
    PCB_SHAPE     source = makeLine( { -reach, -100 }, { reach, 100 } );
    PCB_SHAPE     boundary = makeLine( { 0, -1000 }, { 0, 1000 } );

    GRAPHIC_EDIT_RESULT result = GRAPHIC_TRIM_PLANNER::Plan( source, { reach / 2, 50 }, { &boundary } );

    BOOST_REQUIRE( result );
    BOOST_REQUIRE_EQUAL( result.m_Geometry.size(), 1 );
    BOOST_CHECK_SMALL( result.m_Geometry[0].m_End.x, 2 );
}


BOOST_AUTO_TEST_CASE( DegenerateSourcesReportWhyTheyWereRefused )
{
    PCB_SHAPE zeroLength = makeLine( { 1000, 1000 }, { 1000, 1000 } );
    PCB_SHAPE boundary = makeLine( { 1000, 0 }, { 1000, 2000 } );

    BOOST_CHECK( GRAPHIC_TRIM_PLANNER::Plan( zeroLength, { 1000, 1000 }, { &boundary } ).m_Refusal
                 == GRAPHIC_EDIT_REFUSAL::DEGENERATE );
}


// Two shapes meeting on the source cross it once.  Not two cuts.
BOOST_AUTO_TEST_CASE( BoundariesCrossingAtOnePointYieldOneCut )
{
    PCB_SHAPE source = makeLine( { 0, 0 }, { 100000, 0 } );
    PCB_SHAPE first = makeLine( { 50000, -10000 }, { 50000, 10000 } );
    PCB_SHAPE second = makeLine( { 40000, -10000 }, { 60000, 10000 } );

    GRAPHIC_EDIT_RESULT result = GRAPHIC_TRIM_PLANNER::Plan( source, { 90000, 0 }, { &first, &second } );

    BOOST_REQUIRE( result );
    BOOST_REQUIRE_EQUAL( result.m_Geometry.size(), 1 );
    BOOST_CHECK_EQUAL( result.m_Geometry[0].m_End, VECTOR2I( 50000, 0 ) );
    BOOST_CHECK_EQUAL( result.m_Boundaries.size(), 1 );
}


static PCB_SHAPE makeCircle( const VECTOR2I& aCenter, int aRadius, PCB_LAYER_ID aLayer = Dwgs_User )
{
    PCB_SHAPE circle( nullptr, SHAPE_T::CIRCLE );
    circle.SetCenter( aCenter );
    circle.SetEnd( aCenter + VECTOR2I( aRadius, 0 ) );
    circle.SetLayer( aLayer );
    return circle;
}


static PCB_SHAPE makeRectangle( const VECTOR2I& aCorner, const VECTOR2I& aOpposite,
                                PCB_LAYER_ID aLayer = Dwgs_User )
{
    PCB_SHAPE rectangle( nullptr, SHAPE_T::RECTANGLE );
    rectangle.SetStart( aCorner );
    rectangle.SetEnd( aOpposite );
    rectangle.SetLayer( aLayer );
    return rectangle;
}


// Two cuts open a circle up.  What is left is one arc, the long way round from the pointer.
BOOST_AUTO_TEST_CASE( CircleTrimsIntoAnArc )
{
    PCB_SHAPE source = makeCircle( { 0, 0 }, 100000 );
    PCB_SHAPE boundary = makeLine( { 0, -200000 }, { 0, 200000 } );

    GRAPHIC_EDIT_RESULT result = GRAPHIC_TRIM_PLANNER::Plan( source, { 100000, 0 }, { &boundary } );

    BOOST_REQUIRE( result );
    BOOST_REQUIRE_EQUAL( result.m_Geometry.size(), 1 );
    BOOST_CHECK( result.m_Geometry[0].m_Shape == SHAPE_T::ARC );

    SHAPE_ARC kept( result.m_Geometry[0].m_Start, result.m_Geometry[0].m_Mid, result.m_Geometry[0].m_End, 0 );

    BOOST_CHECK_CLOSE( kept.GetRadius(), 100000.0, 0.01 );

    // The half the pointer was on has gone, so the middle of what is left is the other side.
    BOOST_CHECK_LE( kept.GetArcMid().Distance( VECTOR2I( -100000, 0 ) ), 2 );
}


BOOST_AUTO_TEST_CASE( CircleNeedsTwoCutsToOpen )
{
    PCB_SHAPE source = makeCircle( { 0, 0 }, 100000 );

    // Ends on the circle, so it meets it once and no more.
    PCB_SHAPE boundary = makeLine( { 100000, 0 }, { 300000, 0 } );

    GRAPHIC_EDIT_RESULT result = GRAPHIC_TRIM_PLANNER::Plan( source, { 0, 100000 }, { &boundary } );

    BOOST_CHECK( !result );
    BOOST_CHECK( result.m_Refusal == GRAPHIC_EDIT_REFUSAL::NO_INTERSECTION );
}


// The side under the pointer is the one that gets cut.  The other three come through as lines.
BOOST_AUTO_TEST_CASE( RectangleTrimsIntoLines )
{
    PCB_SHAPE source = makeRectangle( { 0, 0 }, { 100000, 100000 } );
    PCB_SHAPE boundary = makeLine( { 50000, -10000 }, { 50000, 10000 } );

    GRAPHIC_EDIT_RESULT result = GRAPHIC_TRIM_PLANNER::Plan( source, { 90000, 0 }, { &boundary } );

    BOOST_REQUIRE( result );
    BOOST_REQUIRE_EQUAL( result.m_Geometry.size(), 4 );

    for( const GRAPHIC_EDIT_GEOMETRY& piece : result.m_Geometry )
        BOOST_CHECK( piece.m_Shape == SHAPE_T::SEGMENT );

    // The remnant of the trimmed side runs from the far corner to the cut.
    BOOST_CHECK_EQUAL( result.m_Geometry[0].m_Start, VECTOR2I( 0, 0 ) );
    BOOST_CHECK_EQUAL( result.m_Geometry[0].m_End, VECTOR2I( 50000, 0 ) );

    BOOST_REQUIRE_EQUAL( result.m_Preview.size(), 1 );
    BOOST_CHECK_EQUAL( result.m_Preview[0].m_Start, VECTOR2I( 50000, 0 ) );
    BOOST_CHECK_EQUAL( result.m_Preview[0].m_End, VECTOR2I( 100000, 0 ) );
}


// Nothing crosses the side the pointer is on, so the rectangle is left alone.
// GetRectCorners() answers for a flat rectangle too, with two zero-length sides and two that
// run opposite ways.  Trimming one put zero-length segments on the board.
BOOST_AUTO_TEST_CASE( FlatRectangleIsRefused )
{
    PCB_SHAPE source = makeRectangle( { 0, 0 }, { 0, 100000 } );
    PCB_SHAPE boundary = makeLine( { -10000, 50000 }, { 10000, 50000 } );

    GRAPHIC_EDIT_RESULT result = GRAPHIC_TRIM_PLANNER::Plan( source, { 0, 40000 }, { &boundary } );

    BOOST_CHECK( !result );
    BOOST_CHECK( result.m_Refusal == GRAPHIC_EDIT_REFUSAL::DEGENERATE );
}


BOOST_AUTO_TEST_CASE( RectangleSideWithNoCrossingIsRefused )
{
    PCB_SHAPE source = makeRectangle( { 0, 0 }, { 100000, 100000 } );
    PCB_SHAPE boundary = makeLine( { 50000, -10000 }, { 50000, 10000 } );

    GRAPHIC_EDIT_RESULT result = GRAPHIC_TRIM_PLANNER::Plan( source, { 90000, 100000 }, { &boundary } );

    BOOST_CHECK( !result );
    BOOST_CHECK( result.m_Refusal == GRAPHIC_EDIT_REFUSAL::NO_INTERSECTION );
}


BOOST_AUTO_TEST_CASE( CirclesAndRectanglesAreBoundariesToo )
{
    PCB_SHAPE source = makeLine( { -200000, 0 }, { 200000, 0 } );
    PCB_SHAPE circle = makeCircle( { 0, 0 }, 100000 );

    GRAPHIC_EDIT_RESULT byCircle = GRAPHIC_TRIM_PLANNER::Plan( source, { 150000, 0 }, { &circle } );

    BOOST_REQUIRE( byCircle );
    BOOST_REQUIRE_EQUAL( byCircle.m_Geometry.size(), 1 );
    BOOST_CHECK_LE( byCircle.m_Geometry[0].m_End.Distance( VECTOR2I( 100000, 0 ) ), 2 );

    PCB_SHAPE rectangle = makeRectangle( { -50000, -50000 }, { 50000, 50000 } );

    GRAPHIC_EDIT_RESULT byRectangle = GRAPHIC_TRIM_PLANNER::Plan( source, { 150000, 0 }, { &rectangle } );

    BOOST_REQUIRE( byRectangle );
    BOOST_REQUIRE_EQUAL( byRectangle.m_Geometry.size(), 1 );
    BOOST_CHECK_LE( byRectangle.m_Geometry[0].m_End.Distance( VECTOR2I( 50000, 0 ) ), 2 );
}


BOOST_AUTO_TEST_SUITE_END()
