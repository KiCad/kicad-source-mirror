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

#include <qa_utils/wx_utils/unit_test_utils.h>

#include <cmath>

#include <geometry/arc_construction.h>
#include <math/util.h>

using namespace KIGEOM;


namespace
{

double dist( const VECTOR2D& aA, const VECTOR2D& aB )
{
    return ( aA - aB ).EuclideanNorm();
}


/// Sweep of the solution in degrees, using the midpoint to pick between the two arcs
double sweepDegrees( const ARC_SOLUTION& aSol )
{
    const VECTOR2D s = VECTOR2D( aSol.start ) - aSol.center;
    const VECTOR2D m = VECTOR2D( aSol.mid ) - aSol.center;

    return 2.0 * std::abs( std::atan2( s.Cross( m ), s.Dot( m ) ) ) * 180.0 / M_PI;
}


/// +1 when the sweep from start through mid increases the raw angle, else -1
int orientation( const ARC_SOLUTION& aSol )
{
    const VECTOR2D s = VECTOR2D( aSol.start ) - aSol.center;
    const VECTOR2D m = VECTOR2D( aSol.mid ) - aSol.center;

    return s.Cross( m ) > 0.0 ? 1 : -1;
}


/// Every point of the solution lies on its circle and the midpoint bisects the chord
void checkConsistent( const ARC_SOLUTION& aSol, const VECTOR2I& aStart, const VECTOR2I& aEnd, double aTol = 1.5 )
{
    BOOST_REQUIRE( aSol.valid );
    BOOST_CHECK_EQUAL( aSol.start, aStart );
    BOOST_CHECK_EQUAL( aSol.end, aEnd );
    BOOST_CHECK_SMALL( dist( aSol.start, aSol.center ) - aSol.radius, aTol );
    BOOST_CHECK_SMALL( dist( aSol.end, aSol.center ) - aSol.radius, aTol );
    BOOST_CHECK_SMALL( dist( aSol.mid, aSol.center ) - aSol.radius, aTol );
    BOOST_CHECK_SMALL( dist( aSol.mid, aSol.start ) - dist( aSol.mid, aSol.end ), aTol );
}

} // namespace


BOOST_AUTO_TEST_SUITE( ArcConstruction )


BOOST_AUTO_TEST_CASE( ProjectLandsOnBisector )
{
    const VECTOR2I start( 0, 0 );
    const VECTOR2I end( 1000000, 0 );

    BOOST_CHECK_EQUAL( ProjectToChordBisector( start, end, { 700000, 300000 } ), VECTOR2I( 500000, 300000 ) );
    BOOST_CHECK_EQUAL( ProjectToChordBisector( start, end, { -40, -900000 } ), VECTOR2I( 500000, -900000 ) );

    // A diagonal chord, so the projection has to rotate
    const VECTOR2I p = ProjectToChordBisector( { 0, 0 }, { 1000000, 1000000 }, { 900000, 100000 } );
    BOOST_CHECK_SMALL( dist( p, VECTOR2D( 0, 0 ) ) - dist( p, VECTOR2D( 1000000, 1000000 ) ), 1.0 );
    BOOST_CHECK_EQUAL( ProjectToChordBisector( start, start, { 12, 34 } ), VECTOR2I( 12, 34 ) );
}


BOOST_AUTO_TEST_CASE( ThroughPointsQuarterCircle )
{
    const VECTOR2I start( 1000000, 0 );
    const VECTOR2I end( 0, 1000000 );

    ARC_SOLUTION sol = ArcThroughPoints( start, { 707107, 707107 }, end );

    checkConsistent( sol, start, end );
    BOOST_CHECK_SMALL( sol.center.x, 1.5 );
    BOOST_CHECK_SMALL( sol.center.y, 1.5 );
    BOOST_CHECK_SMALL( sol.radius - 1000000.0, 1.5 );
    BOOST_CHECK_LE( dist( sol.mid, VECTOR2D( 707107, 707107 ) ), 1.5 );
    BOOST_CHECK_SMALL( sweepDegrees( sol ) - 90.0, 1e-2 );
}


BOOST_AUTO_TEST_CASE( ThroughPointsMajorArcMidIsHalfway )
{
    const VECTOR2I start( 1000000, 0 );
    const VECTOR2I end( 0, 1000000 );

    // The on-arc point on the far side of the chord selects the 270 degree arc
    ARC_SOLUTION sol = ArcThroughPoints( start, { -707107, -707107 }, end );

    checkConsistent( sol, start, end );
    BOOST_CHECK_SMALL( sweepDegrees( sol ) - 270.0, 1e-2 );
    BOOST_CHECK_LE( dist( sol.mid, VECTOR2D( -707107, -707107 ) ), 1.5 );
}


BOOST_AUTO_TEST_CASE( ThroughPointsDirectionFollowsOnArcPoint )
{
    const VECTOR2I a( 1000000, 0 );
    const VECTOR2I b( 0, 1000000 );
    const VECTOR2I on( 707107, 707107 );

    ARC_SOLUTION fwd = ArcThroughPoints( a, on, b );
    ARC_SOLUTION rev = ArcThroughPoints( b, on, a );

    BOOST_REQUIRE( fwd.valid );
    BOOST_REQUIRE( rev.valid );
    BOOST_CHECK_EQUAL( orientation( fwd ), -orientation( rev ) );

    // Same circle and same halfway point either way round
    BOOST_CHECK_LE( dist( fwd.center, rev.center ), 1e-6 );
    BOOST_CHECK_EQUAL( fwd.mid, rev.mid );
    BOOST_CHECK_EQUAL( rev.start, b );
    BOOST_CHECK_EQUAL( rev.end, a );
}


BOOST_AUTO_TEST_CASE( ThroughPointsMirroredOnArcPointMirrorsMid )
{
    const VECTOR2I start( 0, 0 );
    const VECTOR2I end( 1000000, 0 );

    ARC_SOLUTION up = ArcThroughPoints( start, { 300000, 200000 }, end );
    ARC_SOLUTION down = ArcThroughPoints( start, { 300000, -200000 }, end );

    checkConsistent( up, start, end );
    checkConsistent( down, start, end );
    BOOST_CHECK_GT( up.mid.y, 0 );
    BOOST_CHECK_LT( down.mid.y, 0 );
    BOOST_CHECK_EQUAL( up.mid.x, down.mid.x );
    BOOST_CHECK_EQUAL( up.mid.y, -down.mid.y );
}


BOOST_AUTO_TEST_CASE( ThroughPointsDegenerateIsInvalid )
{
    BOOST_CHECK( !ArcThroughPoints( { 0, 0 }, { 500, 0 }, { 1000, 0 } ).valid );
    BOOST_CHECK( !ArcThroughPoints( { 0, 0 }, { 500, 500 }, { 1000, 1000 } ).valid );
    BOOST_CHECK( !ArcThroughPoints( { 0, 0 }, { 0, 0 }, { 1000, 1000 } ).valid );
    BOOST_CHECK( !ArcThroughPoints( { 5, 5 }, { 700, 900 }, { 5, 5 } ).valid );
    BOOST_CHECK( !ArcThroughPoints( { 5, 5 }, { 5, 5 }, { 5, 5 } ).valid );
}


BOOST_AUTO_TEST_CASE( ThroughPointsVeryShallowArc )
{
    // A 2 m chord with a 1 um sagitta, so the center is about 5e14 nm away
    const VECTOR2I start( 0, 0 );
    const VECTOR2I end( 2000000000, 0 );

    ARC_SOLUTION sol = ArcThroughPoints( start, { 1000000000, 1000 }, end );

    checkConsistent( sol, start, end );
    BOOST_CHECK_EQUAL( sol.mid, VECTOR2I( 1000000000, 1000 ) );
    BOOST_CHECK_CLOSE( sol.radius, 5.0e14, 1e-3 );
}


BOOST_AUTO_TEST_CASE( ThroughPointsOneNanometerSagitta )
{
    // The center is 2e18 nm away, beyond what a double resolves to the nanometer
    const VECTOR2I start( -2000000000, 0 );
    const VECTOR2I end( 2000000000, 0 );

    ARC_SOLUTION sol = ArcThroughPoints( start, { 0, 1 }, end );

    BOOST_REQUIRE( sol.valid );
    BOOST_CHECK_EQUAL( sol.mid, VECTOR2I( 0, 1 ) );
    BOOST_CHECK_CLOSE( sol.radius, 2.0e18, 1e-6 );
}


BOOST_AUTO_TEST_CASE( ThroughPointsNearIntegerLimits )
{
    const VECTOR2I start( -2000000000, 0 );
    const VECTOR2I end( 2000000000, 0 );

    ARC_SOLUTION sol = ArcThroughPoints( start, { 0, 1500000000 }, end );

    checkConsistent( sol, start, end, 4.0 );
    BOOST_CHECK_EQUAL( sol.mid, VECTOR2I( 0, 1500000000 ) );
    BOOST_CHECK_CLOSE( sol.radius, ( 4.0e18 + 2.25e18 ) / 3.0e9, 1e-6 );
}


BOOST_AUTO_TEST_CASE( OppositeCornersNearBillionUnits )
{
    // Squared lengths here exceed the range of 32 and 64 bit integers alike when computed carelessly
    const VECTOR2I a( -1000000000, -1000000000 );
    const VECTOR2I b( 1000000000, 1000000000 );

    ARC_SOLUTION through = ArcThroughPoints( a, { 1000000000, -1000000000 }, b );
    checkConsistent( through, a, b, 4.0 );
    BOOST_CHECK_SMALL( sweepDegrees( through ) - 180.0, 1e-2 );

    ARC_SOLUTION mid = ArcFromStartEndMidDrag( a, b, { 300000000, -700000000 } );
    checkConsistent( mid, a, b, 4.0 );

    ARC_SOLUTION center = ArcFromStartEndCenterDrag( a, b, { 0, 0 }, false );
    checkConsistent( center, a, b, 4.0 );
    BOOST_CHECK_SMALL( sweepDegrees( center ) - 180.0, 1e-2 );

    ARC_SOLUTION tangent = ArcFromStartTangentEnd( a, { 1, 0 }, b );
    checkConsistent( tangent, a, b, 4.0 );
    BOOST_CHECK( std::isfinite( tangent.center.x ) && std::isfinite( tangent.center.y ) );

    const VECTOR2I p = ProjectToChordBisector( a, b, { 1000000000, -1000000000 } );
    BOOST_CHECK_LE( std::abs( dist( p, VECTOR2D( a ) ) - dist( p, VECTOR2D( b ) ) ), 1.0 );
}


BOOST_AUTO_TEST_CASE( MidDragProjectsOntoBisector )
{
    const VECTOR2I start( 0, 0 );
    const VECTOR2I end( 1000000, 0 );

    ARC_SOLUTION sol = ArcFromStartEndMidDrag( start, end, { 700000, 300000 } );

    checkConsistent( sol, start, end );
    BOOST_CHECK_EQUAL( sol.mid, VECTOR2I( 500000, 300000 ) );
    BOOST_CHECK_CLOSE( sol.radius, ( 500000.0 * 500000.0 + 300000.0 * 300000.0 ) / 600000.0, 1e-6 );

    // Off the bisector on the other side of the chord
    sol = ArcFromStartEndMidDrag( start, end, { 100, -250000 } );

    checkConsistent( sol, start, end );
    BOOST_CHECK_EQUAL( sol.mid, VECTOR2I( 500000, -250000 ) );
}


BOOST_AUTO_TEST_CASE( MidDragDiagonalChordIsEquidistant )
{
    const VECTOR2I start( 123457, -98765 );
    const VECTOR2I end( 1000003, 777777 );

    ARC_SOLUTION sol = ArcFromStartEndMidDrag( start, end, { 100000, 900000 } );

    checkConsistent( sol, start, end );
}


BOOST_AUTO_TEST_CASE( MidDragOnChordIsInvalid )
{
    BOOST_CHECK( !ArcFromStartEndMidDrag( { 0, 0 }, { 1000000, 0 }, { 300000, 0 } ).valid );
    BOOST_CHECK( !ArcFromStartEndMidDrag( { 0, 0 }, { 1000000, 1000000 }, { 900000, 900000 } ).valid );
    BOOST_CHECK( !ArcFromStartEndMidDrag( { 7, 7 }, { 7, 7 }, { 900000, 900000 } ).valid );
}


BOOST_AUTO_TEST_CASE( CenterDragMinorIsDefault )
{
    const VECTOR2I start( 0, 0 );
    const VECTOR2I end( 1000000, 0 );

    ARC_SOLUTION sol = ArcFromStartEndCenterDrag( start, end, { 500000, 500000 }, false );

    checkConsistent( sol, start, end );
    BOOST_CHECK_LE( dist( sol.center, VECTOR2D( 500000, 500000 ) ), 1e-6 );
    BOOST_CHECK_SMALL( sweepDegrees( sol ) - 90.0, 1e-2 );
    BOOST_CHECK_LE( dist( sol.mid, VECTOR2D( 500000, 500000 - 707107 ) ), 1.5 );
}


BOOST_AUTO_TEST_CASE( CenterDragMajorSweepsPastHalfTurn )
{
    const VECTOR2I start( 0, 0 );
    const VECTOR2I end( 1000000, 0 );

    ARC_SOLUTION sol = ArcFromStartEndCenterDrag( start, end, { 500000, 500000 }, true );

    checkConsistent( sol, start, end );
    BOOST_CHECK_SMALL( sweepDegrees( sol ) - 270.0, 1e-2 );
    BOOST_CHECK_LE( dist( sol.mid, VECTOR2D( 500000, 500000 + 707107 ) ), 1.5 );

    // A center behind the chord swaps which side each posture bulges toward
    ARC_SOLUTION below = ArcFromStartEndCenterDrag( start, end, { 500000, -500000 }, false );

    checkConsistent( below, start, end );
    BOOST_CHECK_SMALL( sweepDegrees( below ) - 90.0, 1e-2 );
    BOOST_CHECK_GT( below.mid.y, 0 );
}


BOOST_AUTO_TEST_CASE( CenterDragOnChordIsSemicircle )
{
    const VECTOR2I start( 0, 0 );
    const VECTOR2I end( 1000000, 0 );

    ARC_SOLUTION minor = ArcFromStartEndCenterDrag( start, end, { 500000, 0 }, false );
    ARC_SOLUTION major = ArcFromStartEndCenterDrag( start, end, { 500000, 0 }, true );

    checkConsistent( minor, start, end );
    checkConsistent( major, start, end );
    BOOST_CHECK_SMALL( sweepDegrees( minor ) - 180.0, 1e-2 );
    BOOST_CHECK_SMALL( sweepDegrees( major ) - 180.0, 1e-2 );
    BOOST_CHECK_EQUAL( minor.mid.x, 500000 );
    BOOST_CHECK_EQUAL( major.mid.x, 500000 );
    BOOST_CHECK_EQUAL( minor.mid.y, -major.mid.y );
    BOOST_CHECK_EQUAL( std::abs( minor.mid.y ), 500000 );
}


BOOST_AUTO_TEST_CASE( CenterDragDegenerateAndOverflowAreInvalid )
{
    BOOST_CHECK( !ArcFromStartEndCenterDrag( { 3, 3 }, { 3, 3 }, { 500, 500 }, false ).valid );

    // The major arc apex lands outside the coordinate range, the minor arc still fits
    BOOST_CHECK( !ArcFromStartEndCenterDrag( { -2000000000, 0 }, { 2000000000, 0 }, { 0, 2000000000 }, true ).valid );
    BOOST_CHECK( ArcFromStartEndCenterDrag( { -2000000000, 0 }, { 2000000000, 0 }, { 0, 2000000000 }, false ).valid );
}


BOOST_AUTO_TEST_CASE( TangentStartDirectionIsParallelOverEndSweep )
{
    const VECTOR2I start( 250000, -130000 );

    for( const VECTOR2D& tangent : { VECTOR2D( 1, 0 ), VECTOR2D( 0, -1 ), VECTOR2D( 3, 4 ), VECTOR2D( -5, 2 ) } )
    {
        const VECTOR2D unit = tangent / tangent.EuclideanNorm();

        // Ends ahead of, beside and behind the start, at a spread of distances
        for( int degrees = 5; degrees < 360; degrees += 10 )
        {
            for( double radius : { 3000.0, 400000.0, 90000000.0 } )
            {
                const double   a = degrees * M_PI / 180.0;
                const VECTOR2I end( start.x + KiROUND( radius * ( unit.x * std::cos( a ) - unit.y * std::sin( a ) ) ),
                                    start.y + KiROUND( radius * ( unit.x * std::sin( a ) + unit.y * std::cos( a ) ) ) );

                ARC_SOLUTION sol = ArcFromStartTangentEnd( start, tangent, end );

                BOOST_TEST_CONTEXT( "tangent " << tangent << " degrees " << degrees << " radius " << radius )
                {
                    checkConsistent( sol, start, end, 2.0 );

                    const VECTOR2D radial = ( VECTOR2D( start ) - sol.center ) / sol.radius;
                    const VECTOR2D travel = static_cast<double>( orientation( sol ) ) * VECTOR2D( -radial.y, radial.x );

                    BOOST_CHECK_SMALL( radial.Dot( unit ) * sol.radius, 1.0 );
                    BOOST_CHECK_GT( travel.Dot( unit ), 0.999999 );
                }
            }
        }
    }
}


BOOST_AUTO_TEST_CASE( TangentSweepIsTwiceTheChordAngle )
{
    const VECTOR2I start( 0, 0 );

    // Ahead and to the side gives a quarter turn's chord at 45 degrees, so 90 degrees of sweep
    BOOST_CHECK_SMALL( sweepDegrees( ArcFromStartTangentEnd( start, { 1, 0 }, { 100000, 100000 } ) ) - 90.0, 1e-2 );

    // Behind the start the arc has to swing around the long way
    BOOST_CHECK_SMALL( sweepDegrees( ArcFromStartTangentEnd( start, { 1, 0 }, { -100000, 100000 } ) ) - 270.0, 1e-2 );
}


BOOST_AUTO_TEST_CASE( TangentSideFollowsEndSide )
{
    ARC_SOLUTION left = ArcFromStartTangentEnd( { 0, 0 }, { 1, 0 }, { 100000, 100000 } );
    ARC_SOLUTION right = ArcFromStartTangentEnd( { 0, 0 }, { 1, 0 }, { 100000, -100000 } );

    BOOST_REQUIRE( left.valid );
    BOOST_REQUIRE( right.valid );
    BOOST_CHECK_GT( left.center.y, 0.0 );
    BOOST_CHECK_LT( right.center.y, 0.0 );
    BOOST_CHECK_EQUAL( left.mid.x, right.mid.x );
    BOOST_CHECK_EQUAL( left.mid.y, -right.mid.y );
}


BOOST_AUTO_TEST_CASE( TangentDegenerateIsInvalid )
{
    // End on the tangent line, ahead of and behind the start
    BOOST_CHECK( !ArcFromStartTangentEnd( { 0, 0 }, { 1, 0 }, { 500000, 0 } ).valid );
    BOOST_CHECK( !ArcFromStartTangentEnd( { 0, 0 }, { 1, 0 }, { -500000, 0 } ).valid );
    BOOST_CHECK( !ArcFromStartTangentEnd( { 0, 0 }, { 0, 0 }, { 500000, 500000 } ).valid );
    BOOST_CHECK( !ArcFromStartTangentEnd( { 10, 10 }, { 1, 1 }, { 10, 10 } ).valid );
    BOOST_CHECK( !ArcFromStartTangentEnd( { 0, 0 }, { 1, 1 }, { 500000, 500000 } ).valid );
}


BOOST_AUTO_TEST_SUITE_END()
