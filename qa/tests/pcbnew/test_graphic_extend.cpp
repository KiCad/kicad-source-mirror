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
#include <geometry/circle.h>
#include <geometry/seg.h>
#include <geometry/shape_arc.h>
#include <layer_ids.h>
#include <pcb_shape.h>
#include <pcb_track.h>
#include <tools/graphic_extend.h>

#include <limits>


BOOST_AUTO_TEST_SUITE( GraphicExtend )


static const GRAPHIC_EDIT_GEOMETRY& planned( const GRAPHIC_EDIT_RESULT& aResult )
{
    return aResult.m_Geometry.front();
}


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


BOOST_AUTO_TEST_CASE( ExtendsSelectedLineEndToFiniteLineBoundary )
{
    PCB_SHAPE source = makeLine( { 0, 0 }, { 10, 0 } );
    PCB_SHAPE boundary = makeLine( { 20, -10 }, { 20, 10 } );

    GRAPHIC_EDIT_RESULT result = GRAPHIC_EXTEND_PLANNER::Plan( source, GRAPHIC_ENDPOINT::END, { &boundary } );

    BOOST_REQUIRE( result );
    BOOST_CHECK_EQUAL( planned( result ).m_Start, VECTOR2I( 0, 0 ) );
    BOOST_CHECK_EQUAL( planned( result ).m_End, VECTOR2I( 20, 0 ) );
}


BOOST_AUTO_TEST_CASE( ExtendsSelectedLineStartInOppositeDirection )
{
    PCB_SHAPE source = makeLine( { 0, 0 }, { 10, 0 } );
    PCB_SHAPE boundary = makeLine( { -20, -10 }, { -20, 10 } );

    GRAPHIC_EDIT_RESULT result = GRAPHIC_EXTEND_PLANNER::Plan( source, GRAPHIC_ENDPOINT::START, { &boundary } );

    BOOST_REQUIRE( result );
    BOOST_CHECK_EQUAL( planned( result ).m_Start, VECTOR2I( -20, 0 ) );
    BOOST_CHECK_EQUAL( planned( result ).m_End, VECTOR2I( 10, 0 ) );
}


BOOST_AUTO_TEST_CASE( ChoosesNearestPositiveIntersection )
{
    PCB_SHAPE source = makeLine( { 0, 0 }, { 10, 0 } );
    PCB_SHAPE nearBoundary = makeLine( { 20, -10 }, { 20, 10 } );
    PCB_SHAPE farBoundary = makeLine( { 30, -10 }, { 30, 10 } );

    GRAPHIC_EDIT_RESULT result =
            GRAPHIC_EXTEND_PLANNER::Plan( source, GRAPHIC_ENDPOINT::END, { &farBoundary, &nearBoundary } );

    BOOST_REQUIRE( result );
    BOOST_CHECK_EQUAL( planned( result ).m_End, VECTOR2I( 20, 0 ) );
}


BOOST_AUTO_TEST_CASE( IgnoresWrongLayerAndNonGraphicItems )
{
    PCB_SHAPE source = makeLine( { 0, 0 }, { 10, 0 } );
    PCB_SHAPE wrongLayer = makeLine( { 20, -10 }, { 20, 10 }, Cmts_User );
    PCB_TRACK track( nullptr );
    track.SetStart( { 30, -10 } );
    track.SetEnd( { 30, 10 } );

    GRAPHIC_EDIT_RESULT result = GRAPHIC_EXTEND_PLANNER::Plan( source, GRAPHIC_ENDPOINT::END, { &wrongLayer, &track } );

    BOOST_CHECK( !result );
    BOOST_CHECK( result.m_Refusal == GRAPHIC_EDIT_REFUSAL::NO_INTERSECTION );
}


BOOST_AUTO_TEST_CASE( RejectsTrackSource )
{
    PCB_TRACK source( nullptr );
    source.SetStart( { 0, 0 } );
    source.SetEnd( { 10, 0 } );
    PCB_SHAPE boundary = makeLine( { 20, -10 }, { 20, 10 } );

    GRAPHIC_EDIT_RESULT result = GRAPHIC_EXTEND_PLANNER::Plan( source, GRAPHIC_ENDPOINT::END, { &boundary } );

    BOOST_CHECK( !result );
    BOOST_CHECK( result.m_Refusal == GRAPHIC_EDIT_REFUSAL::UNSUPPORTED_SOURCE );
}


BOOST_AUTO_TEST_CASE( ExtendsLineToFiniteArcBoundary )
{
    PCB_SHAPE source = makeLine( { 0, 0 }, { 10, 0 } );
    PCB_SHAPE boundary = makeArc( { 20, 0 }, { 15, 0 }, 180.0 );

    GRAPHIC_EDIT_RESULT result = GRAPHIC_EXTEND_PLANNER::Plan( source, GRAPHIC_ENDPOINT::END, { &boundary } );

    BOOST_REQUIRE( result );
    BOOST_CHECK_EQUAL( planned( result ).m_End, VECTOR2I( 15, 0 ) );
}


BOOST_AUTO_TEST_CASE( ExtendsArcEndToFiniteLineBoundary )
{
    PCB_SHAPE source = makeArc( { 0, 0 }, { 10, 0 }, 90.0 );
    SHAPE_ARC extended( { 0, 0 }, { 10, 0 }, EDA_ANGLE( 180.0, DEGREES_T ) );
    PCB_SHAPE boundary = makeLine( { -20, 0 }, { 20, 0 } );

    GRAPHIC_EDIT_RESULT result = GRAPHIC_EXTEND_PLANNER::Plan( source, GRAPHIC_ENDPOINT::END, { &boundary } );

    BOOST_REQUIRE( result );
    BOOST_CHECK_EQUAL( planned( result ).m_Start, source.GetStart() );
    BOOST_CHECK_EQUAL( planned( result ).m_End, extended.GetP1() );
    SHAPE_ARC resultGeometry( planned( result ).m_Start, planned( result ).m_Mid, planned( result ).m_End, 0 );
    BOOST_CHECK_CLOSE( resultGeometry.GetCentralAngle().AsDegrees(), 180.0, 0.01 );
}


BOOST_AUTO_TEST_CASE( ExtendsArcStartBackwardAlongCurvature )
{
    SHAPE_ARC extended( { 0, 0 }, { 10, 0 }, EDA_ANGLE( 180.0, DEGREES_T ) );
    PCB_SHAPE source = makeArc( extended.GetCenter(), extended.GetArcMid(), 90.0 );
    PCB_SHAPE boundary = makeLine( { -20, 0 }, { 20, 0 } );

    GRAPHIC_EDIT_RESULT result = GRAPHIC_EXTEND_PLANNER::Plan( source, GRAPHIC_ENDPOINT::START, { &boundary } );

    BOOST_REQUIRE( result );
    BOOST_CHECK_EQUAL( planned( result ).m_Start, extended.GetP0() );
    BOOST_CHECK_EQUAL( planned( result ).m_End, source.GetEnd() );
}


// Millimetre scale.  At a few-IU radius two rebuilt circles disagree by a whole IU.
BOOST_AUTO_TEST_CASE( ExtendsArcToFiniteArcBoundary )
{
    PCB_SHAPE source = makeArc( { 0, 0 }, { 10000000, 0 }, 90.0 );
    SHAPE_ARC expected( { 0, 0 }, { 10000000, 0 }, EDA_ANGLE( 180.0, DEGREES_T ) );
    PCB_SHAPE boundary = makeArc( expected.GetP1() + VECTOR2I( 0, 5000000 ), expected.GetP1(), 180.0 );

    GRAPHIC_EDIT_RESULT result = GRAPHIC_EXTEND_PLANNER::Plan( source, GRAPHIC_ENDPOINT::END, { &boundary } );

    BOOST_REQUIRE( result );
    SHAPE_ARC boundaryGeometry( boundary.GetStart(), boundary.GetArcMid(), boundary.GetEnd(), 0 );
    SHAPE_ARC resultGeometry( planned( result ).m_Start, planned( result ).m_Mid, planned( result ).m_End, 0 );
    BOOST_CHECK( boundaryGeometry.Collide( planned( result ).m_End ) );
    BOOST_CHECK_GT( std::abs( resultGeometry.GetCentralAngle().AsDegrees() ), 90.0 );
    BOOST_CHECK_LT( std::abs( resultGeometry.GetCentralAngle().AsDegrees() ), 180.0 );
}


// A graze is one well-defined place to stop, so the line grows until it touches.
BOOST_AUTO_TEST_CASE( ExtendsToATangentTouch )
{
    PCB_SHAPE source = makeLine( { 0, 10000000 }, { 10000000, 10000000 } );
    PCB_SHAPE boundary = makeArc( { 20000000, 0 }, { 20000000, 10000000 }, 180.0 );

    GRAPHIC_EDIT_RESULT result = GRAPHIC_EXTEND_PLANNER::Plan( source, GRAPHIC_ENDPOINT::END, { &boundary } );

    BOOST_REQUIRE( result );
    BOOST_CHECK_LE( planned( result ).m_End.Distance( VECTOR2I( 20000000, 10000000 ) ), 2 );
}


BOOST_AUTO_TEST_CASE( LockedSourceIsRejectedButLockedBoundaryIsEligible )
{
    BOARD     board;
    PCB_SHAPE source( &board, SHAPE_T::SEGMENT );
    source.SetStart( { 0, 0 } );
    source.SetEnd( { 10, 0 } );
    source.SetLayer( Dwgs_User );
    PCB_SHAPE boundary( &board, SHAPE_T::SEGMENT );
    boundary.SetStart( { 20, -10 } );
    boundary.SetEnd( { 20, 10 } );
    boundary.SetLayer( Dwgs_User );
    boundary.SetLocked( true );

    BOOST_CHECK( GRAPHIC_EXTEND_PLANNER::Plan( source, GRAPHIC_ENDPOINT::END, { &boundary } ) );

    source.SetLocked( true );
    GRAPHIC_EDIT_RESULT result = GRAPHIC_EXTEND_PLANNER::Plan( source, GRAPHIC_ENDPOINT::END, { &boundary } );
    BOOST_CHECK( !result );
    BOOST_CHECK( result.m_Refusal == GRAPHIC_EDIT_REFUSAL::LOCKED_SOURCE );
}


BOOST_AUTO_TEST_CASE( PointerChoosesNearestEndpoint )
{
    PCB_SHAPE line = makeLine( { 0, 0 }, { 100, 0 } );
    PCB_SHAPE wide = makeLine( { -1500000000, 0 }, { 0, 0 } );

    BOOST_CHECK( GRAPHIC_EXTEND_PLANNER::NearestEndpoint( line, { 10, 5 } ) == GRAPHIC_ENDPOINT::START );
    BOOST_CHECK( GRAPHIC_EXTEND_PLANNER::NearestEndpoint( line, { 90, -5 } ) == GRAPHIC_ENDPOINT::END );

    // Distance() must widen before subtracting or this pointer wraps.
    BOOST_CHECK( GRAPHIC_EXTEND_PLANNER::NearestEndpoint( wide, { 1500000000, 0 } ) == GRAPHIC_ENDPOINT::END );
}


BOOST_AUTO_TEST_CASE( QueryBoundsFollowTheExtensionRay )
{
    PCB_SHAPE line = makeLine( { 0, 0 }, { 10, 10 } );
    BOX2I     world( { -100, -100 }, { 200, 200 } );

    BOX2I endBounds = GRAPHIC_EXTEND_PLANNER::QueryBounds( line, GRAPHIC_ENDPOINT::END, world );
    BOX2I startBounds = GRAPHIC_EXTEND_PLANNER::QueryBounds( line, GRAPHIC_ENDPOINT::START, world );

    BOOST_CHECK_EQUAL( endBounds.GetPosition(), VECTOR2I( 10, 10 ) );
    BOOST_CHECK_EQUAL( endBounds.GetSize(), VECTOR2I( 90, 90 ) );
    BOOST_CHECK_EQUAL( startBounds.GetPosition(), VECTOR2I( -100, -100 ) );
    BOOST_CHECK_EQUAL( startBounds.GetSize(), VECTOR2I( 100, 100 ) );

    // Clipping must not overflow on a line spanning most of the range.
    PCB_SHAPE wide = makeLine( { -1000000000, 0 }, { 800000000, 0 } );
    BOX2I     wideWorld( { 0, -100 }, { 1000000000, 200 } );
    BOX2I     wideBounds = GRAPHIC_EXTEND_PLANNER::QueryBounds( wide, GRAPHIC_ENDPOINT::END, wideWorld );

    BOOST_CHECK_EQUAL( wideBounds.GetPosition(), VECTOR2I( 800000000, 0 ) );
    BOOST_CHECK_EQUAL( wideBounds.GetSize(), VECTOR2I( 200000000, 0 ) );
}


BOOST_AUTO_TEST_CASE( ExtendsBoardScaleLineForward )
{
    PCB_SHAPE source = makeLine( { -1000000000, 0 }, { 800000000, 0 } );
    PCB_SHAPE boundary = makeLine( { 900000000, -10 }, { 900000000, 10 } );

    GRAPHIC_EDIT_RESULT result = GRAPHIC_EXTEND_PLANNER::Plan( source, GRAPHIC_ENDPOINT::END, { &boundary } );

    BOOST_REQUIRE( result );
    BOOST_CHECK_EQUAL( planned( result ).m_End, VECTOR2I( 900000000, 0 ) );
}


BOOST_AUTO_TEST_CASE( IgnoresBoundaryBehindTheExtendedEndpoint )
{
    PCB_SHAPE source = makeLine( { 0, 0 }, { 10000000, 0 } );
    PCB_SHAPE behind = makeLine( { 5000000, -1000000 }, { 5000000, 1000000 } );
    PCB_SHAPE ahead = makeLine( { 20000000, -1000000 }, { 20000000, 1000000 } );

    // The ray is anchored on the fixed end.  It covers the shape's own span.
    BOOST_CHECK( !GRAPHIC_EXTEND_PLANNER::Plan( source, GRAPHIC_ENDPOINT::END, { &behind } ) );

    GRAPHIC_EDIT_RESULT result = GRAPHIC_EXTEND_PLANNER::Plan( source, GRAPHIC_ENDPOINT::END, { &behind, &ahead } );

    BOOST_REQUIRE( result );
    BOOST_CHECK_EQUAL( planned( result ).m_End, VECTOR2I( 20000000, 0 ) );
}


/// The circle an arc closed into, for checking.
static CIRCLE closedCircle( const GRAPHIC_EDIT_RESULT& aResult )
{
    const GRAPHIC_EDIT_GEOMETRY& geometry = planned( aResult );

    return CIRCLE( geometry.m_Start, geometry.m_Start.Distance( geometry.m_End ) );
}


// Everything the arc could reach is round past the point it closes on itself, so it closes.
BOOST_AUTO_TEST_CASE( ArcPastAFullTurnClosesIntoACircle )
{
    PCB_SHAPE source = makeArc( { 0, 0 }, { 10000000, 0 }, 300.0 );
    PCB_SHAPE boundary = makeLine( { -15000000, -15000000 }, { 15000000, 15000000 } );

    GRAPHIC_EDIT_RESULT result = GRAPHIC_EXTEND_PLANNER::Plan( source, GRAPHIC_ENDPOINT::END, { &boundary } );

    BOOST_REQUIRE( result );
    BOOST_REQUIRE_EQUAL( result.m_Geometry.size(), 1 );
    BOOST_CHECK( planned( result ).m_Shape == SHAPE_T::CIRCLE );
    BOOST_CHECK_EQUAL( closedCircle( result ).Center, VECTOR2I( 0, 0 ) );
    BOOST_CHECK_LE( std::abs( closedCircle( result ).Radius - 10000000 ), 2 );
    BOOST_CHECK( result.m_Boundaries.empty() );
}


// An arc on its own has nothing to stop it, so it goes all the way round.
BOOST_AUTO_TEST_CASE( ArcWithNothingToReachClosesIntoACircle )
{
    PCB_SHAPE source = makeArc( { 0, 0 }, { 10000000, 0 }, 90.0 );

    GRAPHIC_EDIT_RESULT result = GRAPHIC_EXTEND_PLANNER::Plan( source, GRAPHIC_ENDPOINT::END, {} );

    BOOST_REQUIRE( result );
    BOOST_CHECK( planned( result ).m_Shape == SHAPE_T::CIRCLE );
    BOOST_CHECK_EQUAL( closedCircle( result ).Center, VECTOR2I( 0, 0 ) );
    BOOST_CHECK_LE( std::abs( closedCircle( result ).Radius - 10000000 ), 2 );

    // Which end the pointer picked makes no difference to a closed circle.
    GRAPHIC_EDIT_RESULT fromStart = GRAPHIC_EXTEND_PLANNER::Plan( source, GRAPHIC_ENDPOINT::START, {} );

    BOOST_REQUIRE( fromStart );
    BOOST_CHECK_EQUAL( closedCircle( fromStart ).Center, closedCircle( result ).Center );
    BOOST_CHECK_EQUAL( closedCircle( fromStart ).Radius, closedCircle( result ).Radius );
}


// A boundary within reach still wins.  Closing is only what happens when nothing is.
BOOST_AUTO_TEST_CASE( ReachableBoundaryBeatsClosingTheCircle )
{
    PCB_SHAPE source = makeArc( { 0, 0 }, { 10000000, 0 }, 90.0 );
    PCB_SHAPE boundary = makeLine( { -15000000, 5000000 }, { 15000000, 5000000 } );

    GRAPHIC_EDIT_RESULT result = GRAPHIC_EXTEND_PLANNER::Plan( source, GRAPHIC_ENDPOINT::END, { &boundary } );

    BOOST_REQUIRE( result );
    BOOST_CHECK( planned( result ).m_Shape == SHAPE_T::ARC );
    BOOST_CHECK_EQUAL( result.m_Boundaries.size(), 1 );
}


// A line has no end to come back round to, so it still refuses.
BOOST_AUTO_TEST_CASE( LineWithNothingToReachStillRefuses )
{
    PCB_SHAPE source = makeLine( { 0, 0 }, { 10000000, 0 } );

    GRAPHIC_EDIT_RESULT result = GRAPHIC_EXTEND_PLANNER::Plan( source, GRAPHIC_ENDPOINT::END, {} );

    BOOST_CHECK( !result );
    BOOST_CHECK( result.m_Refusal == GRAPHIC_EDIT_REFUSAL::NO_INTERSECTION );
}


// The shape from the test board that reported "nothing to reach in that direction".  Its chord
// crosses the carrier circle, but only round past where the arc closes on itself.
BOOST_AUTO_TEST_CASE( WideArcWhoseChordIsOutOfReachCloses )
{
    PCB_SHAPE source( nullptr, SHAPE_T::ARC );
    source.SetArcGeometry( { 25000000, 87000000 }, { 35000000, 70000000 }, { 45000000, 87000000 } );
    source.SetLayer( Dwgs_User );

    PCB_SHAPE chord = makeLine( { 20000000, 80000000 }, { 50000000, 80000000 } );

    GRAPHIC_EDIT_RESULT result = GRAPHIC_EXTEND_PLANNER::Plan( source, GRAPHIC_ENDPOINT::END, { &chord } );

    BOOST_REQUIRE( result );
    BOOST_CHECK( planned( result ).m_Shape == SHAPE_T::CIRCLE );
    BOOST_CHECK_LE( closedCircle( result ).Center.Distance( VECTOR2I( 35000000, 81441176 ) ), 2 );
}


// Neither gives an unambiguous direction.  The radius also overflows the search box.
BOOST_AUTO_TEST_CASE( DegenerateSourcesReportWhyTheyWereRefused )
{
    PCB_SHAPE zeroLength = makeLine( { 1000, 1000 }, { 1000, 1000 } );
    int       radius = std::numeric_limits<int>::max() / 2 + 100;
    PCB_SHAPE hugeArc = makeArc( { 0, 0 }, { radius, 0 }, 90.0 );
    BOX2I     world( { std::numeric_limits<int>::min() / 2, std::numeric_limits<int>::min() / 2 },
                     { std::numeric_limits<int>::max(), std::numeric_limits<int>::max() } );

    BOOST_CHECK( GRAPHIC_EXTEND_PLANNER::Plan( zeroLength, GRAPHIC_ENDPOINT::END, {} ).m_Refusal
                 == GRAPHIC_EDIT_REFUSAL::DEGENERATE );
    BOOST_CHECK( !GRAPHIC_EXTEND_PLANNER::QueryBounds( hugeArc, GRAPHIC_ENDPOINT::END, world ).IsValid() );
    BOOST_CHECK( !GRAPHIC_EXTEND_PLANNER::Plan( hugeArc, GRAPHIC_ENDPOINT::END, {} ) );
}


// A large arc rebuilt from centre and sweep lands tens of IU off.  The intersection is rounded.
BOOST_AUTO_TEST_CASE( ExtendedLargeArcLandsOnItsBoundary )
{
    PCB_SHAPE source = makeArc( { 0, 0 }, { 100000000, 0 }, 10.0 );
    PCB_SHAPE boundary = makeLine( { 60000000, 60000000 }, { 120000000, 10000000 } );

    GRAPHIC_EDIT_RESULT result = GRAPHIC_EXTEND_PLANNER::Plan( source, GRAPHIC_ENDPOINT::END, { &boundary } );

    BOOST_REQUIRE( result );
    BOOST_CHECK( SEG( boundary.GetStart(), boundary.GetEnd() ).Contains( planned( result ).m_End ) );
}


BOOST_AUTO_TEST_CASE( CollinearBoundariesAreRejectedInBothEndpointOrders )
{
    PCB_SHAPE source = makeLine( { 0, 0 }, { 10, 0 } );
    PCB_SHAPE forward = makeLine( { 20, 0 }, { 30, 0 } );
    PCB_SHAPE reversed = makeLine( { 30, 0 }, { 20, 0 } );

    BOOST_CHECK( !GRAPHIC_EXTEND_PLANNER::Plan( source, GRAPHIC_ENDPOINT::END, { &forward } ) );
    BOOST_CHECK( !GRAPHIC_EXTEND_PLANNER::Plan( source, GRAPHIC_ENDPOINT::END, { &reversed } ) );
}


BOOST_AUTO_TEST_CASE( ExtendsNegativeSweepArc )
{
    SHAPE_ARC expected( { 0, 0 }, { 10, 0 }, EDA_ANGLE( -180.0, DEGREES_T ) );
    PCB_SHAPE source = makeArc( { 0, 0 }, { 10, 0 }, -90.0 );
    PCB_SHAPE boundary = makeLine( expected.GetP1() - VECTOR2I( 10, 0 ), expected.GetP1() + VECTOR2I( 10, 0 ) );

    BOOST_REQUIRE( source.EndsSwapped() );
    GRAPHIC_EDIT_RESULT result = GRAPHIC_EXTEND_PLANNER::Plan( source, GRAPHIC_ENDPOINT::START, { &boundary } );

    BOOST_REQUIRE( result );
    BOOST_CHECK_EQUAL( planned( result ).m_Start, expected.GetP1() );
    BOOST_CHECK_EQUAL( planned( result ).m_End, source.GetEnd() );
    SHAPE_ARC resultGeometry( planned( result ).m_Start, planned( result ).m_Mid, planned( result ).m_End, 0 );
    BOOST_CHECK_CLOSE( resultGeometry.GetCentralAngle().AsDegrees(), 180.0, 0.01 );
}


BOOST_AUTO_TEST_SUITE_END()
