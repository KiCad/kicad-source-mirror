/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 3
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, you may find one here:
 * https://www.gnu.org/licenses/gpl-3.0.html
 * or you may search the https://www.gnu.org website for the version 3 license,
 * or you may write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
 */

#include <qa_utils/wx_utils/unit_test_utils.h>

#include <geometry/shape.h>
#include <geometry/shape_arc.h>
#include <geometry/shape_circle.h>
#include <geometry/shape_line_chain.h>
#include <geometry/shape_rect.h>
#include <geometry/shape_segment.h>
#include <geometry/shape_compound.h>
#include <geometry/shape_poly_set.h>

#include "fixtures_geometry.h"


BOOST_AUTO_TEST_SUITE( SHAPE_NEAREST_POINTS_TEST )

// Circle to Circle tests
BOOST_AUTO_TEST_CASE( NearestPoints_CircleToCircle_Separate )
{
    SHAPE_CIRCLE circleA( VECTOR2I( 0, 0 ), 5 );
    SHAPE_CIRCLE circleB( VECTOR2I( 20, 0 ), 5 );

    VECTOR2I ptA, ptB;
    bool result = circleA.NearestPoints( &circleB, ptA, ptB );

    BOOST_CHECK( result );
    BOOST_CHECK_MESSAGE( ptA == VECTOR2I( 5, 0 ), "Expected: " << VECTOR2I( 5, 0 ) << " Actual: " << ptA );
    BOOST_CHECK_MESSAGE( ptB == VECTOR2I( 15, 0 ), "Expected: " << VECTOR2I( 15, 0 ) << " Actual: " << ptB );
}

BOOST_AUTO_TEST_CASE( NearestPoints_CircleToCircle_Concentric )
{
    SHAPE_CIRCLE circleA( VECTOR2I( 0, 0 ), 5 );
    SHAPE_CIRCLE circleB( VECTOR2I( 0, 0 ), 10 );

    VECTOR2I ptA, ptB;
    bool result = circleA.NearestPoints( &circleB, ptA, ptB );

    BOOST_CHECK( result );
    // For concentric circles, points should be on arbitrary direction (positive X by convention)
    BOOST_CHECK_MESSAGE( ptA == VECTOR2I( 5, 0 ), "Expected: " << VECTOR2I( 5, 0 ) << " Actual: " << ptA );
    BOOST_CHECK_MESSAGE( ptB == VECTOR2I( 10, 0 ), "Expected: " << VECTOR2I( 10, 0 ) << " Actual: " << ptB );
}

BOOST_AUTO_TEST_CASE( NearestPoints_CircleToCircle_Overlapping )
{
    SHAPE_CIRCLE circleA( VECTOR2I( 0, 0 ), 5 );
    SHAPE_CIRCLE circleB( VECTOR2I( 6, 0 ), 5 );

    VECTOR2I ptA, ptB;
    bool result = circleA.NearestPoints( &circleB, ptA, ptB );

    BOOST_CHECK( result );
    BOOST_CHECK_MESSAGE( ptA == VECTOR2I( 5, 0 ), "Expected: " << VECTOR2I( 5, 0 ) << " Actual: " << ptA );
    BOOST_CHECK_MESSAGE( ptB == VECTOR2I( 1, 0 ), "Expected: " << VECTOR2I( 1, 0 ) << " Actual: " << ptB );
}

// Circle to Rectangle tests
BOOST_AUTO_TEST_CASE( NearestPoints_CircleToRect_Outside )
{
    SHAPE_CIRCLE circle( VECTOR2I( -10, 5 ), 3 );
    SHAPE_RECT rect( VECTOR2I( 0, 0 ), VECTOR2I( 10, 10 ) );

    VECTOR2I ptA, ptB;
    bool result = circle.NearestPoints( &rect, ptA, ptB );

    BOOST_CHECK( result );
    BOOST_CHECK_MESSAGE( ptA == VECTOR2I( -7, 5 ), "Expected: " << VECTOR2I( -7, 5 ) << " Actual: " << ptA );
    BOOST_CHECK_MESSAGE( ptB == VECTOR2I( 0, 5 ), "Expected: " << VECTOR2I( 0, 5 ) << " Actual: " << ptB );
}

BOOST_AUTO_TEST_CASE( NearestPoints_CircleToRect_Inside )
{
    SHAPE_CIRCLE circle( VECTOR2I( 5, 5 ), 2 );
    SHAPE_RECT rect( VECTOR2I( 0, 0 ), VECTOR2I( 10, 10 ) );

    VECTOR2I ptA, ptB;
    bool result = circle.NearestPoints( &rect, ptA, ptB );

    BOOST_CHECK( result );
    // Circle center is inside rectangle, should find nearest edge
    // Center at (5,5) is equidistant from left/right edges (5 units) and top/bottom edges (5 units)
    // Implementation should choose one consistently - likely left edge based on min comparison
    BOOST_CHECK_MESSAGE( ptA == VECTOR2I( 3, 5 ), "Expected: " << VECTOR2I( 3, 5 ) << " Actual: " << ptA );
    BOOST_CHECK_MESSAGE( ptB == VECTOR2I( 0, 5 ), "Expected: " << VECTOR2I( 0, 5 ) << " Actual: " << ptB );
}

// Circle to Segment tests
BOOST_AUTO_TEST_CASE( NearestPoints_CircleToSegment )
{
    SHAPE_CIRCLE circle( VECTOR2I( 0, 5 ), 2 );
    SHAPE_SEGMENT segment( VECTOR2I( -5, 0 ), VECTOR2I( 5, 0 ) );

    VECTOR2I ptA, ptB;
    bool result = circle.NearestPoints( &segment, ptA, ptB );

    BOOST_CHECK( result );
    BOOST_CHECK_MESSAGE( ptA == VECTOR2I( 0, 3 ), "Expected: " << VECTOR2I( 0, 3 ) << " Actual: " << ptA );
    BOOST_CHECK_MESSAGE( ptB == VECTOR2I( 0, 0 ), "Expected: " << VECTOR2I( 0, 0 ) << " Actual: " << ptB );
}

BOOST_AUTO_TEST_CASE( NearestPoints_CircleToSegment_CenterOnSegment )
{
    SHAPE_CIRCLE circle( VECTOR2I( 0, 0 ), 3 );
    SHAPE_SEGMENT segment( VECTOR2I( -5, 0 ), VECTOR2I( 5, 0 ) );

    VECTOR2I ptA, ptB;
    bool result = circle.NearestPoints( &segment, ptA, ptB );

    BOOST_CHECK( result );
    // When center is on segment, should pick perpendicular direction
    BOOST_CHECK_MESSAGE( ptA == VECTOR2I( 0, 3 ), "Expected: " << VECTOR2I( 0, 3 ) << " Actual: " << ptA );
    BOOST_CHECK_MESSAGE( ptB == VECTOR2I( 0, 0 ), "Expected: " << VECTOR2I( 0, 0 ) << " Actual: " << ptB );
}

// Rectangle to Rectangle tests
BOOST_AUTO_TEST_CASE( NearestPoints_RectToRect_Separate )
{
    SHAPE_RECT rectA( VECTOR2I( 0, 0 ), VECTOR2I( 5, 5 ) );
    SHAPE_RECT rectB( VECTOR2I( 10, 0 ), VECTOR2I( 25, 25 ) );

    VECTOR2I ptA, ptB;
    bool result = rectA.NearestPoints( &rectB, ptA, ptB );

    BOOST_CHECK( result );
    BOOST_CHECK_MESSAGE( ptA == VECTOR2I( 5, 0 ), "Expected: " << VECTOR2I( 5, 0 ) << " Actual: " << ptA );
    BOOST_CHECK_MESSAGE( ptB == VECTOR2I( 10, 0 ), "Expected: " << VECTOR2I( 10, 0 ) << " Actual: " << ptB );
}

BOOST_AUTO_TEST_CASE( NearestPoints_RectToRect_Corner )
{
    SHAPE_RECT rectA( VECTOR2I( 0, 0 ), VECTOR2I( 5, 5 ) );
    SHAPE_RECT rectB( VECTOR2I( 7, 7 ), VECTOR2I( 25, 25 ) );

    VECTOR2I ptA, ptB;
    bool result = rectA.NearestPoints( &rectB, ptA, ptB );

    BOOST_CHECK( result );
    BOOST_CHECK_MESSAGE( ptA == VECTOR2I( 5, 5 ), "Expected: " << VECTOR2I( 5, 5 ) << " Actual: " << ptA );
    BOOST_CHECK_MESSAGE( ptB == VECTOR2I( 7, 7 ), "Expected: " << VECTOR2I( 7, 7 ) << " Actual: " << ptB );
}

// Line Chain tests
BOOST_AUTO_TEST_CASE( NearestPoints_LineChainToLineChain )
{
    SHAPE_LINE_CHAIN chainA;
    chainA.Append( VECTOR2I( 0, 0 ) );
    chainA.Append( VECTOR2I( 10, 0 ) );
    chainA.Append( VECTOR2I( 10, 10 ) );

    SHAPE_LINE_CHAIN chainB;
    chainB.Append( VECTOR2I( 5, 5 ) );
    chainB.Append( VECTOR2I( 15, 5 ) );

    VECTOR2I ptA, ptB;
    bool result = chainA.NearestPoints( &chainB, ptA, ptB );

    BOOST_CHECK( result );
    BOOST_CHECK_MESSAGE( ptA == VECTOR2I( 10, 5 ), "Expected: " << VECTOR2I( 10, 5 ) << " Actual: " << ptA );
    BOOST_CHECK_MESSAGE( ptB == VECTOR2I( 10, 5 ), "Expected: " << VECTOR2I( 10, 5 ) << " Actual: " << ptB );
}

BOOST_AUTO_TEST_CASE( NearestPoints_LineChainWithArc )
{
    SHAPE_LINE_CHAIN chainA;
    chainA.Append( VECTOR2I( 0, 0 ) );
    chainA.Append( VECTOR2I( 10, 0 ) );

    SHAPE_LINE_CHAIN chainB;
    chainB.Append( SHAPE_ARC( VECTOR2I( 5, 10 ), VECTOR2I( 10, 5 ), VECTOR2I( 15, 10 ), 0 ) );

    VECTOR2I ptA, ptB;
    bool result = chainA.NearestPoints( &chainB, ptA, ptB );

    BOOST_CHECK( result );
    BOOST_CHECK_MESSAGE( ptA == VECTOR2I( 10, 0 ), "Expected: " << VECTOR2I( 10, 0 ) << " Actual: " << ptA );
    BOOST_CHECK_MESSAGE( ptB == VECTOR2I( 10, 10 ), "Expected: " << VECTOR2I( 10, 10 ) << " Actual: " << ptB );
}

// Arc tests
BOOST_AUTO_TEST_CASE( NearestPoints_ArcToArc )
{
    SHAPE_ARC arcA( VECTOR2I( 0, 0 ), VECTOR2I( 5, 5 ), VECTOR2I( 10, 0 ), 0 );
    SHAPE_ARC arcB( VECTOR2I( 15, 0 ), VECTOR2I( 20, 5 ), VECTOR2I( 25, 0 ), 0 );

    VECTOR2I ptA, ptB;
    int64_t distSq;
    bool result = arcA.NearestPoints( arcB, ptA, ptB, distSq );

    BOOST_CHECK( result );
    // Points should be on the arcs closest to each other
    BOOST_CHECK( ptA.x <= 10 && ptA.x >= 0 );
    BOOST_CHECK( ptB.x >= 15 && ptB.x <= 25 );
}

BOOST_AUTO_TEST_CASE( NearestPoints_ArcToCircle )
{
    SHAPE_ARC arc( VECTOR2I( 0, 0 ), VECTOR2I( 5, 5 ), VECTOR2I( 10, 0 ), 0 );
    SHAPE_CIRCLE circle( VECTOR2I( 5, 10 ), 3 );

    VECTOR2I ptA, ptB;
    int64_t distSq;
    bool result = arc.NearestPoints( circle, ptA, ptB, distSq );

    BOOST_CHECK( result );
    // Arc point should be at or near the top of the arc
    BOOST_CHECK( ptA.y >= 0 );
    // Circle point should be at bottom of circle
    BOOST_CHECK_MESSAGE( ptB == VECTOR2I( 5, 7 ), "Expected: " << VECTOR2I( 5, 7 ) << " Actual: " << ptB );
}

// Segment tests
BOOST_AUTO_TEST_CASE( NearestPoints_SegmentToSegment )
{
    SHAPE_SEGMENT segA( SEG( VECTOR2I( 0, 0 ), VECTOR2I( 10, 0 ) ), 2 );
    SHAPE_SEGMENT segB( SEG( VECTOR2I( 5, 5 ), VECTOR2I( 5, 15 ) ), 2 );

    VECTOR2I ptA, ptB;
    bool result = segA.NearestPoints( &segB, ptA, ptB );

    BOOST_CHECK( result );
    // Points should account for segment width
    BOOST_CHECK_MESSAGE( ptA == VECTOR2I( 5, 1 ), "Expected: " << VECTOR2I( 5, 1 ) << " Actual: " << ptA );
    BOOST_CHECK_MESSAGE( ptB == VECTOR2I( 5, 4 ), "Expected: " << VECTOR2I( 5, 4 ) << " Actual: " << ptB );
}

BOOST_AUTO_TEST_CASE( NearestPoints_SegmentToCircle )
{
    SHAPE_SEGMENT segment( SEG( VECTOR2I( 0, 0 ), VECTOR2I( 10, 0 ) ), 4 );
    SHAPE_CIRCLE circle( VECTOR2I( 5, 10 ), 3 );

    VECTOR2I ptA, ptB;
    bool result = segment.NearestPoints( &circle, ptA, ptB );

    BOOST_CHECK( result );
    BOOST_CHECK_MESSAGE( ptA == VECTOR2I( 5, 2 ), "Expected: " << VECTOR2I( 5, 2 ) << " Actual: " << ptA );
    BOOST_CHECK_MESSAGE( ptB == VECTOR2I( 5, 7 ), "Expected: " << VECTOR2I( 5, 7 ) << " Actual: " << ptB );
}

// Compound Shape tests
BOOST_AUTO_TEST_CASE( NearestPoints_CompoundShapes )
{
    SHAPE_COMPOUND compoundA;
    compoundA.AddShape( new SHAPE_CIRCLE( VECTOR2I( 0, 0 ), 5 ) );
    compoundA.AddShape( new SHAPE_RECT( VECTOR2I( 10, 0 ), VECTOR2I( 5, 5 ) ) );

    SHAPE_COMPOUND compoundB;
    compoundB.AddShape( new SHAPE_CIRCLE( VECTOR2I( 20, 0 ), 3 ) );

    VECTOR2I ptA, ptB;
    bool result = compoundA.NearestPoints( &compoundB, ptA, ptB );

    BOOST_CHECK( result );
    // Should find nearest points between rectangle in A and circle in B
    BOOST_CHECK_MESSAGE( ptA == VECTOR2I( 10, 0 ), "Expected: " << VECTOR2I( 10, 0 ) << " Actual: " << ptA );
    BOOST_CHECK_MESSAGE( ptB == VECTOR2I( 17, 0 ), "Expected: " << VECTOR2I( 17, 0 ) << " Actual: " << ptB );
}

// Polygon Set tests
BOOST_AUTO_TEST_CASE( NearestPoints_PolySetToCircle )
{
    SHAPE_POLY_SET polySet;
    polySet.NewOutline();
    polySet.Append( VECTOR2I( 0, 0 ) );
    polySet.Append( VECTOR2I( 10, 0 ) );
    polySet.Append( VECTOR2I( 10, 10 ) );
    polySet.Append( VECTOR2I( 0, 10 ) );

    SHAPE_CIRCLE circle( VECTOR2I( 15, 5 ), 3 );

    VECTOR2I ptA, ptB;
    bool result = polySet.NearestPoints( &circle, ptA, ptB );

    BOOST_CHECK( result );
    BOOST_CHECK_MESSAGE( ptB == VECTOR2I( 12, 5 ), "Expected: " << VECTOR2I( 12, 5 ) << " Actual: " << ptB );
}

// Missing Basic Shape Combinations
BOOST_AUTO_TEST_CASE( NearestPoints_RectToSegment )
{
    SHAPE_RECT rect( VECTOR2I( 0, 0 ), VECTOR2I( 10, 10 ) );
    SHAPE_SEGMENT segment( VECTOR2I( 15, 5 ), VECTOR2I( 25, 5 ) );

    VECTOR2I ptA, ptB;
    bool result = rect.NearestPoints( &segment, ptA, ptB );

    BOOST_CHECK( result );
    BOOST_CHECK_MESSAGE( ptA == VECTOR2I( 10, 5 ), "Expected: " << VECTOR2I( 10, 5 ) << " Actual: " << ptA );
    BOOST_CHECK_MESSAGE( ptB == VECTOR2I( 15, 5 ), "Expected: " << VECTOR2I( 15, 5 ) << " Actual: " << ptB );
}

BOOST_AUTO_TEST_CASE( NearestPoints_RectToLineChain )
{
    SHAPE_RECT rect( VECTOR2I( 0, 0 ), VECTOR2I( 5, 5 ) );

    SHAPE_LINE_CHAIN chain;
    chain.Append( VECTOR2I( 10, 0 ) );
    chain.Append( VECTOR2I( 15, 5 ) );
    chain.Append( VECTOR2I( 10, 10 ) );

    VECTOR2I ptA, ptB;
    bool result = rect.NearestPoints( &chain, ptA, ptB );

    BOOST_CHECK( result );
    BOOST_CHECK_MESSAGE( ptA == VECTOR2I( 5, 0 ), "Expected: " << VECTOR2I( 5, 0 ) << " Actual: " << ptA );
    BOOST_CHECK_MESSAGE( ptB == VECTOR2I( 10, 0 ), "Expected: " << VECTOR2I( 10, 0 ) << " Actual: " << ptB );
}

BOOST_AUTO_TEST_CASE( NearestPoints_SegmentToLineChain )
{
    SHAPE_SEGMENT segment( SEG( VECTOR2I( 0, 0 ), VECTOR2I( 100, 0 ) ), 20 );

    SHAPE_LINE_CHAIN chain;
    chain.Append( VECTOR2I( 50, 100 ) );
    chain.Append( VECTOR2I( 150, 100 ) );
    chain.Append( VECTOR2I( 150, 50 ) );

    VECTOR2I ptA, ptB;
    bool result = segment.NearestPoints( &chain, ptA, ptB );

    BOOST_CHECK( result );
    BOOST_CHECK_MESSAGE( ptA == VECTOR2I( 107, 7 ), "Expected: " << VECTOR2I( 107, 7 ) << " Actual: " << ptA );
    BOOST_CHECK_MESSAGE( ptB == VECTOR2I( 150, 50 ), "Expected: " << VECTOR2I( 150, 50 ) << " Actual: " << ptB );
}

BOOST_AUTO_TEST_CASE( NearestPoints_CircleToLineChain )
{
    SHAPE_CIRCLE circle( VECTOR2I( 0, 0 ), 3 );

    SHAPE_LINE_CHAIN chain;
    chain.Append( VECTOR2I( 10, -5 ) );
    chain.Append( VECTOR2I( 10, 0 ) );
    chain.Append( VECTOR2I( 10, 5 ) );

    VECTOR2I ptA, ptB;
    bool result = circle.NearestPoints( &chain, ptA, ptB );

    BOOST_CHECK( result );
    BOOST_CHECK_MESSAGE( ptA == VECTOR2I( 3, 0 ), "Expected: " << VECTOR2I( 3, 0 ) << " Actual: " << ptA );
    BOOST_CHECK_MESSAGE( ptB == VECTOR2I( 10, 0 ), "Expected: " << VECTOR2I( 10, 0 ) << " Actual: " << ptB );
}

BOOST_AUTO_TEST_CASE( NearestPoints_ArcToRect )
{
    SHAPE_ARC arc( VECTOR2I( 0, 0 ), VECTOR2I( 5, 5 ), VECTOR2I( 10, 0 ), 0 );
    SHAPE_RECT rect( VECTOR2I( 150, 2 ), VECTOR2I( 50, 6 ) );

    VECTOR2I ptA, ptB;
    int64_t distSq;
    bool result = arc.NearestPoints( rect, ptA, ptB, distSq );

    BOOST_CHECK( result );
    BOOST_CHECK_MESSAGE( ptA == VECTOR2I( 10, 0 ), "Expected: " << VECTOR2I( 10, 0 ) << " Actual: " << ptA );
    BOOST_CHECK_MESSAGE( ptB == VECTOR2I( 50, 2 ), "Expected: " << VECTOR2I( 50, 2 ) << " Actual: " << ptB );

}

BOOST_AUTO_TEST_CASE( NearestPoints_ArcToSegment )
{
    SHAPE_ARC arc( VECTOR2I( 0, 0 ), VECTOR2I( 0, 10 ), VECTOR2I( 0, 20 ), 0 );  // Semicircle
    SEG segment( VECTOR2I( 15, 10 ), VECTOR2I( 25, 10 ) );

    VECTOR2I ptA, ptB;
    int64_t distSq;
    bool result = arc.NearestPoints( segment, ptA, ptB, distSq );

    BOOST_CHECK( result );
    // Arc point should be on the rightmost part of the arc
    BOOST_CHECK( ptA.x >= 0 );
    BOOST_CHECK_MESSAGE( ptB == VECTOR2I( 15, 10 ), "Expected: " << VECTOR2I( 15, 10 ) << " Actual: " << ptB );
}

BOOST_AUTO_TEST_CASE( NearestPoints_ArcToLineChain )
{
    SHAPE_ARC arc( VECTOR2I( 0, 0 ), VECTOR2I( 5, 5 ), VECTOR2I( 10, 0 ), 0 );

    SHAPE_LINE_CHAIN chain;
    chain.Append( VECTOR2I( 5, 15 ) );
    chain.Append( VECTOR2I( 5, 10 ) );
    chain.Append( VECTOR2I( 15, 10 ) );

    VECTOR2I ptA, ptB;
    int64_t distSq;
    bool result = chain.NearestPoints( &arc, ptB, ptA );

    BOOST_CHECK( result );
    // Arc point should be near the top of the arc
    BOOST_CHECK_MESSAGE( ptA == VECTOR2I( 5, 5 ), "Expected: " << VECTOR2I( 5, 5 ) << " Actual: " << ptA );
    BOOST_CHECK_MESSAGE( ptB == VECTOR2I( 5, 10 ), "Expected: " << VECTOR2I( 5, 10 ) << " Actual: " << ptB );
}

// Distance and Symmetry Validation Tests
BOOST_AUTO_TEST_CASE( NearestPoints_DistanceValidation )
{
    SHAPE_CIRCLE circleA( VECTOR2I( 0, 0 ), 5 );
    SHAPE_CIRCLE circleB( VECTOR2I( 20, 0 ), 3 );

    VECTOR2I ptA, ptB;
    bool result = circleA.NearestPoints( &circleB, ptA, ptB );

    BOOST_CHECK( result );

    // Calculate expected distance: center distance - both radii
    int expectedDistance = 20 - 5 - 3;  // 12
    int actualDistance = (ptB - ptA).EuclideanNorm();

    BOOST_CHECK_MESSAGE( actualDistance == expectedDistance,
                        "Expected distance: " << expectedDistance << " Actual: " << actualDistance );
}

BOOST_AUTO_TEST_CASE( NearestPoints_SymmetryTest )
{
    SHAPE_RECT rectA( VECTOR2I( 0, 0 ), VECTOR2I( 5, 5 ) );
    SHAPE_CIRCLE circleB( VECTOR2I( 10, 2 ), 2 );

    // Test A->B
    VECTOR2I ptA1, ptB1;
    bool result1 = rectA.NearestPoints( &circleB, ptA1, ptB1 );

    // Test B->A
    VECTOR2I ptA2, ptB2;
    bool result2 = circleB.NearestPoints( &rectA, ptA2, ptB2 );

    BOOST_CHECK( result1 && result2 );

    // Distance should be the same both ways
    int dist1 = (ptB1 - ptA1).EuclideanNorm();
    int dist2 = (ptB2 - ptA2).EuclideanNorm();

    BOOST_CHECK_MESSAGE( dist1 == dist2, "Distance A->B: " << dist1 << " Distance B->A: " << dist2 );

    // Points should be swapped (ptA1 should equal ptB2, ptB1 should equal ptA2)
    BOOST_CHECK_MESSAGE( ptA1 == ptB2, "Expected ptA1 == ptB2. ptA1: " << ptA1 << " ptB2: " << ptB2 );
    BOOST_CHECK_MESSAGE( ptB1 == ptA2, "Expected ptB1 == ptA2. ptB1: " << ptB1 << " ptA2: " << ptA2 );
}

BOOST_AUTO_TEST_CASE( NearestPoints_MinimumDistanceValidation )
{
    SHAPE_SEGMENT segment( SEG( VECTOR2I( 0, 0 ), VECTOR2I( 10, 0 ) ), 0 );
    SHAPE_CIRCLE circle( VECTOR2I( 5, 5 ), 2 );

    VECTOR2I ptA, ptB;
    bool result = segment.NearestPoints( &circle, ptA, ptB );

    BOOST_CHECK( result );

    // Verify that this is indeed the minimum distance by checking nearby points
    int minDist = (ptB - ptA).EuclideanNorm();

    // Check a few other points on the segment
    for( int x = 0; x <= 10; x += 2 )
    {
        VECTOR2I testPt( x, 0 );
        VECTOR2I circleClosest = circle.GetCenter() +
            (testPt - circle.GetCenter()).Resize( circle.GetRadius() );
        int testDist = (circleClosest - testPt).EuclideanNorm();

        BOOST_CHECK_MESSAGE( minDist <= testDist,
                            "Found shorter distance at x=" << x << ": " << testDist << " vs " << minDist );
    }
}

// Complex Line Chain Tests
BOOST_AUTO_TEST_CASE( NearestPoints_LineChainWithMixedArcs )
{
    SHAPE_LINE_CHAIN chainA;
    chainA.Append( VECTOR2I( 0, 0 ) );
    chainA.Append( VECTOR2I( 10, 0 ) );
    chainA.Append( SHAPE_ARC( VECTOR2I( 10, 0 ), VECTOR2I( 15, 5 ), VECTOR2I( 20, 0 ), 0 ) );
    chainA.Append( VECTOR2I( 30, 0 ) );

    SHAPE_CIRCLE circle( VECTOR2I( 15, 15 ), 3 );

    VECTOR2I ptA, ptB;
    bool result = chainA.NearestPoints( &circle, ptA, ptB );

    BOOST_CHECK( result );
    BOOST_CHECK_MESSAGE( ptB == VECTOR2I( 15, 12 ), "Expected: " << VECTOR2I( 15, 12 ) << " Actual: " << ptB );
}

// Degenerate Shape Tests
BOOST_AUTO_TEST_CASE( NearestPoints_DegenerateShapes_ZeroWidthRect )
{
    SHAPE_RECT rectA( VECTOR2I( 0, 0 ), VECTOR2I( 0, 10 ) );  // Zero width
    SHAPE_CIRCLE circle( VECTOR2I( 5, 5 ), 2 );

    VECTOR2I ptA, ptB;
    bool result = rectA.NearestPoints( &circle, ptA, ptB );

    BOOST_CHECK( result );
    BOOST_CHECK_MESSAGE( ptA == VECTOR2I( 0, 5 ), "Expected: " << VECTOR2I( 0, 5 ) << " Actual: " << ptA );
    BOOST_CHECK_MESSAGE( ptB == VECTOR2I( 3, 5 ), "Expected: " << VECTOR2I( 3, 5 ) << " Actual: " << ptB );
}

BOOST_AUTO_TEST_CASE( NearestPoints_DegenerateShapes_ZeroLengthSegment )
{
    SHAPE_SEGMENT zeroSeg( VECTOR2I( 5, 5 ), VECTOR2I( 5, 5 ) );  // Point segment
    SHAPE_CIRCLE circle( VECTOR2I( 10, 5 ), 3 );

    VECTOR2I ptA, ptB;
    bool result = circle.NearestPoints( &zeroSeg, ptA, ptB );

    BOOST_CHECK( result );
    BOOST_CHECK_MESSAGE( ptA == VECTOR2I( 7, 5 ), "Expected: " << VECTOR2I( 7, 5 ) << " Actual: " << ptA );
    BOOST_CHECK_MESSAGE( ptB == VECTOR2I( 5, 5 ), "Expected: " << VECTOR2I( 5, 5 ) << " Actual: " << ptB );
}

BOOST_AUTO_TEST_CASE( NearestPoints_SinglePointLineChain )
{
    SHAPE_LINE_CHAIN singlePoint;
    singlePoint.Append( VECTOR2I( 0, 0 ) );

    SHAPE_CIRCLE circle( VECTOR2I( 5, 0 ), 2 );

    VECTOR2I ptA, ptB;
    bool result = singlePoint.NearestPoints( &circle, ptA, ptB );

    BOOST_CHECK( !result );
}

// Negative Coordinate Tests
BOOST_AUTO_TEST_CASE( NearestPoints_NegativeCoordinates )
{
    SHAPE_CIRCLE circleA( VECTOR2I( -10, -5 ), 3 );
    SHAPE_RECT rect( VECTOR2I( 0, -2 ), VECTOR2I( 5, 4 ) );

    VECTOR2I ptA, ptB;
    bool result = circleA.NearestPoints( &rect, ptA, ptB );

    BOOST_CHECK( result );
    BOOST_CHECK_MESSAGE( ptA == VECTOR2I( -7, -4 ), "Expected: " << VECTOR2I( -7, -4 ) << " Actual: " << ptA );
    BOOST_CHECK_MESSAGE( ptB == VECTOR2I( 0, -2 ), "Expected: " << VECTOR2I( 0, -2 ) << " Actual: " << ptB );
}

// Edge case tests
BOOST_AUTO_TEST_CASE( NearestPoints_IdenticalShapes )
{
    SHAPE_CIRCLE circleA( VECTOR2I( 5, 5 ), 3 );
    SHAPE_CIRCLE circleB( VECTOR2I( 5, 5 ), 3 );

    VECTOR2I ptA, ptB;
    bool result = circleA.NearestPoints( &circleB, ptA, ptB );

    BOOST_CHECK( result );
    // Identical concentric circles - should return arbitrary points
    BOOST_CHECK_MESSAGE( ptA == VECTOR2I( 8, 5 ), "Expected: " << VECTOR2I( 8, 5 ) << " Actual: " << ptA );
    BOOST_CHECK_MESSAGE( ptB == VECTOR2I( 8, 5 ), "Expected: " << VECTOR2I( 8, 5 ) << " Actual: " << ptB );
}

BOOST_AUTO_TEST_CASE( NearestPoints_ZeroSizeShapes )
{
    SHAPE_CIRCLE circleA( VECTOR2I( 0, 0 ), 0 );
    SHAPE_CIRCLE circleB( VECTOR2I( 10, 0 ), 5 );

    VECTOR2I ptA, ptB;
    bool result = circleA.NearestPoints( &circleB, ptA, ptB );

    BOOST_CHECK( result );
    BOOST_CHECK_MESSAGE( ptA == VECTOR2I( 0, 0 ), "Expected: " << VECTOR2I( 0, 0 ) << " Actual: " << ptA );
    BOOST_CHECK_MESSAGE( ptB == VECTOR2I( 5, 0 ), "Expected: " << VECTOR2I( 5, 0 ) << " Actual: " << ptB );
}

BOOST_AUTO_TEST_CASE( NearestPoints_VeryCloseShapes )
{
    SHAPE_CIRCLE circleA( VECTOR2I( 0, 0 ), 5 );
    SHAPE_CIRCLE circleB( VECTOR2I( 10, 0 ), 5 );  // Just touching

    VECTOR2I ptA, ptB;
    bool result = circleA.NearestPoints( &circleB, ptA, ptB );

    BOOST_CHECK( result );
    BOOST_CHECK_MESSAGE( ptA == VECTOR2I( 5, 0 ), "Expected: " << VECTOR2I( 5, 0 ) << " Actual: " << ptA );
    BOOST_CHECK_MESSAGE( ptB == VECTOR2I( 5, 0 ), "Expected: " << VECTOR2I( 5, 0 ) << " Actual: " << ptB );
}

BOOST_AUTO_TEST_SUITE_END()