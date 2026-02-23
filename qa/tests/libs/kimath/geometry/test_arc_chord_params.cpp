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
 * You should have received a copy of the GNU General Public License along
 * with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <qa_utils/wx_utils/unit_test_utils.h>
#include <boost/test/data/test_case.hpp>

#include <geometry/arc_chord_params.h>
#include <cmath>


BOOST_AUTO_TEST_SUITE( ArcChordParams )


/**
 * Test that degenerate cases (collinear points) return invalid.
 */
BOOST_AUTO_TEST_CASE( DegenerateCases )
{
    ARC_CHORD_PARAMS params;

    // Collinear points (mid on the line between start and end)
    BOOST_CHECK( !params.Compute( VECTOR2I( 0, 0 ), VECTOR2I( 500000, 0 ), VECTOR2I( 1000000, 0 ) ) );
    BOOST_CHECK( !params.IsValid() );

    // Same start and end point
    BOOST_CHECK( !params.Compute( VECTOR2I( 0, 0 ), VECTOR2I( 500000, 500000 ), VECTOR2I( 0, 0 ) ) );
    BOOST_CHECK( !params.IsValid() );

    // All three points the same
    BOOST_CHECK( !params.Compute( VECTOR2I( 100, 100 ), VECTOR2I( 100, 100 ), VECTOR2I( 100, 100 ) ) );
    BOOST_CHECK( !params.IsValid() );
}


/**
 * Test a simple 90-degree arc (quarter circle).
 */
BOOST_AUTO_TEST_CASE( QuarterCircle )
{
    ARC_CHORD_PARAMS params;

    // Quarter circle with radius 1000000, center at origin
    // Start at (1000000, 0), mid at approximately (707107, 707107), end at (0, 1000000)
    int radius = 1000000;
    int midCoord = KiROUND( radius * std::sqrt( 2.0 ) / 2.0 );

    VECTOR2I start( radius, 0 );
    VECTOR2I mid( midCoord, midCoord );
    VECTOR2I end( 0, radius );

    BOOST_REQUIRE( params.Compute( start, mid, end ) );
    BOOST_CHECK( params.IsValid() );

    // Check radius (should be close to 1000000)
    BOOST_CHECK_CLOSE( params.GetRadius(), (double)radius, 0.1 );

    // Check arc angle (should be close to 90 degrees = PI/2 radians)
    BOOST_CHECK_CLOSE( params.GetArcAngle(), M_PI / 2.0, 0.1 );

    // Check center point (should be close to origin)
    VECTOR2D center = params.GetCenterPoint();
    BOOST_CHECK_SMALL( center.x, 1.0 );
    BOOST_CHECK_SMALL( center.y, 1.0 );

    // Check chord length (should be sqrt(2) * radius)
    BOOST_CHECK_CLOSE( params.GetChordLength(), radius * std::sqrt( 2.0 ), 0.1 );

    // Check sagitta (perpendicular distance from chord midpoint to arc)
    // For 90-degree arc: sagitta = radius * (1 - cos(45)) = radius * (1 - sqrt(2)/2)
    double expectedSagitta = radius * ( 1.0 - std::sqrt( 2.0 ) / 2.0 );
    BOOST_CHECK_CLOSE( params.GetSagitta(), expectedSagitta, 0.1 );
}


/**
 * Test a semicircle (180-degree arc).
 */
BOOST_AUTO_TEST_CASE( Semicircle )
{
    ARC_CHORD_PARAMS params;

    int radius = 500000;
    VECTOR2I start( -radius, 0 );
    VECTOR2I mid( 0, radius );
    VECTOR2I end( radius, 0 );

    BOOST_REQUIRE( params.Compute( start, mid, end ) );
    BOOST_CHECK( params.IsValid() );

    // Check radius
    BOOST_CHECK_CLOSE( params.GetRadius(), (double)radius, 0.1 );

    // Check arc angle (should be PI radians = 180 degrees)
    BOOST_CHECK_CLOSE( params.GetArcAngle(), M_PI, 0.1 );

    // Check center point (should be at origin)
    VECTOR2D center = params.GetCenterPoint();
    BOOST_CHECK_SMALL( center.x, 1.0 );
    BOOST_CHECK_SMALL( center.y, 1.0 );

    // Check chord length (should be 2 * radius = diameter)
    BOOST_CHECK_CLOSE( params.GetChordLength(), 2.0 * radius, 0.1 );

    // Check sagitta (should equal radius for semicircle)
    BOOST_CHECK_CLOSE( params.GetSagitta(), (double)radius, 0.1 );

    // Check center offset (should be 0 for semicircle since center is on the chord)
    BOOST_CHECK_SMALL( params.GetCenterOffset(), 1.0 );
}


/**
 * Test a shallow arc (large radius, small arc angle).
 */
BOOST_AUTO_TEST_CASE( ShallowArc )
{
    ARC_CHORD_PARAMS params;

    // Create a shallow arc: 10-degree arc with radius 10mm
    // This tests numerical stability for cases where center is far from arc
    int radius = 10000000;  // 10mm in nm
    double arcAngleRad = 10.0 * M_PI / 180.0;  // 10 degrees
    double halfAngle = arcAngleRad / 2.0;

    // Start at angle -5 degrees, end at +5 degrees (from positive x-axis)
    VECTOR2I start( KiROUND( radius * std::cos( -halfAngle ) ),
                    KiROUND( radius * std::sin( -halfAngle ) ) );
    VECTOR2I mid( radius, 0 );  // Midpoint at 0 degrees
    VECTOR2I end( KiROUND( radius * std::cos( halfAngle ) ),
                  KiROUND( radius * std::sin( halfAngle ) ) );

    BOOST_REQUIRE( params.Compute( start, mid, end ) );
    BOOST_CHECK( params.IsValid() );

    // Check radius (allow 0.5% tolerance for shallow arc)
    BOOST_CHECK_CLOSE( params.GetRadius(), (double)radius, 0.5 );

    // Check arc angle
    BOOST_CHECK_CLOSE( params.GetArcAngle(), arcAngleRad, 1.0 );

    // Center should be at origin
    VECTOR2D center = params.GetCenterPoint();
    BOOST_CHECK_SMALL( center.x, 100.0 );  // Allow more tolerance for shallow arc
    BOOST_CHECK_SMALL( center.y, 100.0 );
}


/**
 * Test that start and end angles are correct.
 */
BOOST_AUTO_TEST_CASE( StartEndAngles )
{
    ARC_CHORD_PARAMS params;

    // Create arc from (1000, 0) through (0, 1000) to (-1000, 0)
    // This is a semicircle with center at origin
    int radius = 1000000;
    VECTOR2I start( radius, 0 );
    VECTOR2I mid( 0, radius );
    VECTOR2I end( -radius, 0 );

    BOOST_REQUIRE( params.Compute( start, mid, end ) );

    // Start angle should be 0 degrees (pointing right from center)
    EDA_ANGLE startAngle = params.GetStartAngle();
    BOOST_CHECK_CLOSE( startAngle.AsDegrees(), 0.0, 1.0 );

    // End angle should be 180 degrees (pointing left from center)
    EDA_ANGLE endAngle = params.GetEndAngle();
    BOOST_CHECK_CLOSE( std::abs( endAngle.AsDegrees() ), 180.0, 1.0 );
}


/**
 * Test arc with center not at origin.
 */
BOOST_AUTO_TEST_CASE( OffsetCenter )
{
    ARC_CHORD_PARAMS params;

    // Create quarter circle with center at (1000000, 1000000)
    int cx = 1000000;
    int cy = 1000000;
    int radius = 500000;

    VECTOR2I start( cx + radius, cy );
    int midCoord = KiROUND( radius * std::sqrt( 2.0 ) / 2.0 );
    VECTOR2I mid( cx + midCoord, cy + midCoord );
    VECTOR2I end( cx, cy + radius );

    BOOST_REQUIRE( params.Compute( start, mid, end ) );
    BOOST_CHECK( params.IsValid() );

    // Check radius
    BOOST_CHECK_CLOSE( params.GetRadius(), (double)radius, 0.1 );

    // Check center point
    VECTOR2D center = params.GetCenterPoint();
    BOOST_CHECK_CLOSE( center.x, (double)cx, 0.1 );
    BOOST_CHECK_CLOSE( center.y, (double)cy, 0.1 );
}


/**
 * Test clockwise vs counter-clockwise arcs.
 */
BOOST_AUTO_TEST_CASE( ArcDirection )
{
    ARC_CHORD_PARAMS paramsCCW;
    ARC_CHORD_PARAMS paramsCW;

    int radius = 1000000;

    // CCW arc: start at right, mid at top, end at left
    VECTOR2I start( radius, 0 );
    VECTOR2I midCCW( 0, radius );
    VECTOR2I end( -radius, 0 );

    BOOST_REQUIRE( paramsCCW.Compute( start, midCCW, end ) );

    // CW arc: same start and end, but mid at bottom
    VECTOR2I midCW( 0, -radius );

    BOOST_REQUIRE( paramsCW.Compute( start, midCW, end ) );

    // Both should have same radius
    BOOST_CHECK_CLOSE( paramsCCW.GetRadius(), (double)radius, 0.1 );
    BOOST_CHECK_CLOSE( paramsCW.GetRadius(), (double)radius, 0.1 );

    // Both should have same arc angle magnitude (PI)
    BOOST_CHECK_CLOSE( paramsCCW.GetArcAngle(), M_PI, 0.1 );
    BOOST_CHECK_CLOSE( paramsCW.GetArcAngle(), M_PI, 0.1 );

    // Centers should be on opposite sides of the chord
    VECTOR2D centerCCW = paramsCCW.GetCenterPoint();
    VECTOR2D centerCW = paramsCW.GetCenterPoint();

    // CCW center should be above the chord (positive y)
    // CW center should be below the chord (negative y)
    // For semicircles, centers are on the chord, so we check the normal direction
    VECTOR2D normalCCW = paramsCCW.GetNormalUnitVec();
    VECTOR2D normalCW = paramsCW.GetNormalUnitVec();

    // Normals should point in opposite directions (toward their respective centers)
    BOOST_CHECK_CLOSE( normalCCW.y, -normalCW.y, 0.1 );
}


/**
 * Test that GetChordMidpoint returns correct values.
 */
BOOST_AUTO_TEST_CASE( ChordMidpoint )
{
    ARC_CHORD_PARAMS params;

    VECTOR2I start( 0, 0 );
    VECTOR2I mid( 500000, 500000 );
    VECTOR2I end( 1000000, 0 );

    BOOST_REQUIRE( params.Compute( start, mid, end ) );

    VECTOR2D chordMid = params.GetChordMidpoint();

    // Chord midpoint should be average of start and end
    BOOST_CHECK_CLOSE( chordMid.x, 500000.0, 0.001 );
    BOOST_CHECK_CLOSE( chordMid.y, 0.0, 0.001 );
}


/**
 * Test unit vectors are normalized.
 */
BOOST_AUTO_TEST_CASE( UnitVectorsNormalized )
{
    ARC_CHORD_PARAMS params;

    VECTOR2I start( 123456, 789012 );
    VECTOR2I mid( 500000, 900000 );
    VECTOR2I end( 876543, 210987 );

    BOOST_REQUIRE( params.Compute( start, mid, end ) );

    // Check chord unit vector is normalized
    VECTOR2D u = params.GetChordUnitVec();
    double uLen = std::sqrt( u.x * u.x + u.y * u.y );
    BOOST_CHECK_CLOSE( uLen, 1.0, 0.0001 );

    // Check normal unit vector is normalized
    VECTOR2D n = params.GetNormalUnitVec();
    double nLen = std::sqrt( n.x * n.x + n.y * n.y );
    BOOST_CHECK_CLOSE( nLen, 1.0, 0.0001 );

    // Check u and n are perpendicular (dot product = 0)
    double dot = u.x * n.x + u.y * n.y;
    BOOST_CHECK_SMALL( dot, 0.0001 );
}


/**
 * Test a major arc (greater than 180 degrees).
 */
BOOST_AUTO_TEST_CASE( MajorArc )
{
    ARC_CHORD_PARAMS params;

    // Create a 270-degree arc (major arc)
    // Start at (1000, 0), go CCW through (0, -1000) to (-1000, 0)
    // The "mid" point is at 135 degrees from start, which is at (-707, -707)
    int radius = 1000000;
    int midCoord = KiROUND( radius * std::sqrt( 2.0 ) / 2.0 );

    VECTOR2I start( radius, 0 );
    VECTOR2I mid( -midCoord, -midCoord );  // At 225 degrees (halfway through 270-degree arc)
    VECTOR2I end( 0, radius );

    BOOST_REQUIRE( params.Compute( start, mid, end ) );
    BOOST_CHECK( params.IsValid() );

    // Check radius
    BOOST_CHECK_CLOSE( params.GetRadius(), (double)radius, 0.5 );

    // Check arc angle (should be 270 degrees = 3*PI/2 radians)
    BOOST_CHECK_CLOSE( params.GetArcAngle(), 3.0 * M_PI / 2.0, 1.0 );
}


BOOST_AUTO_TEST_SUITE_END()
