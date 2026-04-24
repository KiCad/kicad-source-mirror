/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright The KiCad Developers, see AUTHORS.TXT for contributors.
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
 * along with this program; if not, you may find one here:
 * http://www.gnu.org/licenses/old-licenses/gpl-2.0.html
 * or you may search the http://www.gnu.org website for the version 2 license,
 * or you may write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
 */

#include <qa_utils/wx_utils/unit_test_utils.h>

#include <cmath>
#include <limits>
#include <chrono>
#include <random>
#include <stdexcept>

#include <geometry/eda_angle.h>
#include <geometry/shape_ellipse.h>
#include <geometry/shape_line_chain.h>
#include <geometry/shape_circle.h>
#include <geometry/shape_rect.h>
#include <geometry/shape_arc.h>

BOOST_AUTO_TEST_SUITE( ShapeEllipse )


/**
 * Compute a bounding box by sampling 10,000 points around the ellipse
 * Used as ground truth to verify BBox() implementation
 */
static BOX2I bruteForceEllipseBBox( const VECTOR2I& aCenter, int aMajorR, int aMinorR, const EDA_ANGLE& aRotation,
                                    const EDA_ANGLE& aStartAngle, const EDA_ANGLE& aEndAngle, bool aIsArc,
                                    int aNSamples = 10000 )
{
    const double a = aMajorR;
    const double b = aMinorR;
    const double cp = std::cos( aRotation.AsRadians() );
    const double sp = std::sin( aRotation.AsRadians() );

    double tStart = aIsArc ? aStartAngle.AsRadians() : 0.0;
    double tEnd = aIsArc ? aEndAngle.AsRadians() : 2.0 * M_PI;

    if( tEnd < tStart )
        tEnd += 2.0 * M_PI;

    double minX = std::numeric_limits<double>::max();
    double maxX = std::numeric_limits<double>::lowest();
    double minY = std::numeric_limits<double>::max();
    double maxY = std::numeric_limits<double>::lowest();

    for( int i = 0; i <= aNSamples; ++i )
    {
        const double t = tStart + ( tEnd - tStart ) * i / aNSamples;
        const double ct = std::cos( t );
        const double st = std::sin( t );
        const double x = aCenter.x + a * ct * cp - b * st * sp;
        const double y = aCenter.y + a * ct * sp + b * st * cp;

        minX = std::min( minX, x );
        maxX = std::max( maxX, x );
        minY = std::min( minY, y );
        maxY = std::max( maxY, y );
    }

    const int ix = static_cast<int>( std::floor( minX ) );
    const int iy = static_cast<int>( std::floor( minY ) );
    const int ex = static_cast<int>( std::ceil( maxX ) );
    const int ey = static_cast<int>( std::ceil( maxY ) );

    return BOX2I( VECTOR2I( ix, iy ), VECTOR2I( ex - ix, ey - iy ) );
}


BOOST_AUTO_TEST_CASE( ConstructorSwapsMajorMinor )
{
    // Pass major=100, minor=300 (wrong order). Constructor should swap them
    // to major=300, minor=100 and add 90° to rotation
    SHAPE_ELLIPSE e( VECTOR2I( 0, 0 ), 100, 300, EDA_ANGLE( 0.0, DEGREES_T ) );

    BOOST_CHECK_EQUAL( e.GetMajorRadius(), 300 );
    BOOST_CHECK_EQUAL( e.GetMinorRadius(), 100 );
    BOOST_CHECK_CLOSE( e.GetRotation().AsDegrees(), 90.0, 1e-6 );
}


BOOST_AUTO_TEST_CASE( ConstructorClampsDegenerate )
{
    // Zero/negative radii are clamped to 1 IU
    SHAPE_ELLIPSE e1( VECTOR2I( 0, 0 ), 0, 100, EDA_ANGLE( 0, DEGREES_T ) );
    BOOST_CHECK_GE( e1.GetMajorRadius(), 1 );
    BOOST_CHECK_GE( e1.GetMinorRadius(), 1 );

    SHAPE_ELLIPSE e2( VECTOR2I( 0, 0 ), 100, 0, EDA_ANGLE( 0, DEGREES_T ) );
    BOOST_CHECK_GE( e2.GetMajorRadius(), 1 );
    BOOST_CHECK_GE( e2.GetMinorRadius(), 1 );

    SHAPE_ELLIPSE e3( VECTOR2I( 0, 0 ), -10, 100, EDA_ANGLE( 0, DEGREES_T ) );
    BOOST_CHECK_GE( e3.GetMajorRadius(), 1 );
    BOOST_CHECK_GE( e3.GetMinorRadius(), 1 );
}


BOOST_AUTO_TEST_CASE( AxisAlignedEllipseBBox )
{
    // Axis-aligned ellipse at (1000, 2000) with major=500 along X, minor=300 along Y.
    // No rotation, so the bbox is center.x +/- 500 and center.y +/- 300.
    SHAPE_ELLIPSE e( VECTOR2I( 1000, 2000 ), 500, 300, EDA_ANGLE( 0.0, DEGREES_T ) );
    BOX2I         bbox = e.BBox();

    BOOST_CHECK_EQUAL( bbox.GetLeft(), 500 );
    BOOST_CHECK_EQUAL( bbox.GetRight(), 1500 );
    BOOST_CHECK_EQUAL( bbox.GetTop(), 1700 );
    BOOST_CHECK_EQUAL( bbox.GetBottom(), 2300 );
}


BOOST_AUTO_TEST_CASE( RotatedEllipseBBoxAt45Degrees )
{
    // At 45 degree rotation, the ellipse extends equally in both directions. The
    // half extent on each axis is sqrt((a^2 + b^2) / 2).
    const int     a = 500;
    const int     b = 300;
    SHAPE_ELLIPSE e( VECTOR2I( 0, 0 ), a, b, EDA_ANGLE( 45.0, DEGREES_T ) );
    BOX2I         bbox = e.BBox();

    const double expected = std::sqrt( ( double( a ) * a + double( b ) * b ) / 2.0 );
    BOOST_CHECK_LE( std::abs( bbox.GetWidth() / 2.0 - expected ), 2.0 );
    BOOST_CHECK_LE( std::abs( bbox.GetHeight() / 2.0 - expected ), 2.0 );
}


BOOST_AUTO_TEST_CASE( RandomClosedEllipseBBoxVsBruteForce )
{
    // Generate 1000 random closed ellipses with varying centers, radii, and
    // rotations. Compare the analytical BBox against the brute-force sampled
    // bbox. They must agree within +/- 2 IU (integer rounding tolerance).
    std::mt19937                           rng( 42 );
    std::uniform_int_distribution<int>     centerDist( -10000, 10000 );
    std::uniform_int_distribution<int>     radiusDist( 50, 2000 );
    std::uniform_real_distribution<double> angleDist( 0.0, 360.0 );

    const int N = 1000;
    for( int i = 0; i < N; ++i )
    {
        const VECTOR2I center( centerDist( rng ), centerDist( rng ) );
        int            r1 = radiusDist( rng );
        int            r2 = radiusDist( rng );

        if( r1 < r2 )
            std::swap( r1, r2 );

        const EDA_ANGLE rot( angleDist( rng ), DEGREES_T );

        SHAPE_ELLIPSE e( center, r1, r2, rot );
        BOX2I         analytic = e.BBox();
        BOX2I         brute = bruteForceEllipseBBox( center, r1, r2, rot, EDA_ANGLE( 0, DEGREES_T ),
                                                     EDA_ANGLE( 360, DEGREES_T ), false );

        // Allow +/- 2 IU for integer rounding on either side.
        BOOST_CHECK_LE( std::abs( analytic.GetLeft() - brute.GetLeft() ), 2 );
        BOOST_CHECK_LE( std::abs( analytic.GetRight() - brute.GetRight() ), 2 );
        BOOST_CHECK_LE( std::abs( analytic.GetTop() - brute.GetTop() ), 2 );
        BOOST_CHECK_LE( std::abs( analytic.GetBottom() - brute.GetBottom() ), 2 );
    }
}


BOOST_AUTO_TEST_CASE( RandomEllipticalArcBBoxVsBruteForce )
{
    // 1000 random elliptical arcs with varying start angles and sweep lengths.
    // The arc bbox logic has to check whether each axis extremum falls inside
    // the angular range
    std::mt19937                           rng( 1337 );
    std::uniform_int_distribution<int>     centerDist( -10000, 10000 );
    std::uniform_int_distribution<int>     radiusDist( 50, 2000 );
    std::uniform_real_distribution<double> angleDist( 0.0, 360.0 );
    std::uniform_real_distribution<double> sweepDist( 10.0, 350.0 );

    const int N = 1000;
    int       failures = 0;

    for( int i = 0; i < N; ++i )
    {
        const VECTOR2I center( centerDist( rng ), centerDist( rng ) );
        int            r1 = radiusDist( rng );
        int            r2 = radiusDist( rng );

        if( r1 < r2 )
            std::swap( r1, r2 );

        const EDA_ANGLE rot( angleDist( rng ), DEGREES_T );
        const EDA_ANGLE start( angleDist( rng ), DEGREES_T );
        const EDA_ANGLE end( start.AsDegrees() + sweepDist( rng ), DEGREES_T );

        SHAPE_ELLIPSE e( center, r1, r2, rot, start, end );
        BOX2I         analytic = e.BBox();
        BOX2I         brute = bruteForceEllipseBBox( center, r1, r2, rot, start, end, true );

        if( std::abs( analytic.GetLeft() - brute.GetLeft() ) > 2
            || std::abs( analytic.GetRight() - brute.GetRight() ) > 2
            || std::abs( analytic.GetTop() - brute.GetTop() ) > 2
            || std::abs( analytic.GetBottom() - brute.GetBottom() ) > 2 )
        {
            ++failures;
        }
    }

    BOOST_CHECK_EQUAL( failures, 0 );
}


BOOST_AUTO_TEST_CASE( CirclePerimeterViaRamanujan )
{
    // When both radii are equal it's a circle. Ramanujan's formula should
    // give exactly 2πr in that case.
    const int     r = 1000;
    SHAPE_ELLIPSE e( VECTOR2I( 0, 0 ), r, r, EDA_ANGLE( 0.0, DEGREES_T ) );
    BOOST_CHECK_CLOSE( e.GetLength(), 2.0 * M_PI * r, 1e-9 );
}


BOOST_AUTO_TEST_CASE( EllipsePerimeterMatchesHighResIntegration )
{
    // Cross-check Ramanujan's approximation against a high-resolution numerical
    // integration (200,000 steps) for a 2:1 ellipse. They should agree to
    // within 10 parts per million
    const double a = 1000.0;
    const double b = 500.0;

    const int    N = 200000;
    double       sum = 0.0;
    const double h = 2.0 * M_PI / N;

    for( int i = 0; i < N; ++i )
    {
        const double t0 = i * h;
        const double t1 = ( i + 1 ) * h;
        auto         f = [a, b]( double t )
        {
            const double s = std::sin( t ), c = std::cos( t );
            return std::sqrt( a * a * s * s + b * b * c * c );
        };
        sum += ( t1 - t0 ) * ( f( t0 ) + 4 * f( 0.5 * ( t0 + t1 ) ) + f( t1 ) ) / 6.0;
    }

    SHAPE_ELLIPSE e( VECTOR2I( 0, 0 ), static_cast<int>( a ), static_cast<int>( b ), EDA_ANGLE( 30.0, DEGREES_T ) );

    BOOST_CHECK_CLOSE( e.GetLength(), sum, 1e-4 );
}


BOOST_AUTO_TEST_CASE( SemiCircleArcLength )
{
    // Semicircular arc of radius 1000. Length should be 1000 * pi.
    const int     r = 1000;
    SHAPE_ELLIPSE arc( VECTOR2I( 0, 0 ), r, r, EDA_ANGLE( 0.0, DEGREES_T ), EDA_ANGLE( 0.0, DEGREES_T ),
                       EDA_ANGLE( 180.0, DEGREES_T ) );
    BOOST_CHECK_CLOSE( arc.GetLength(), M_PI * r, 1e-6 );
}


BOOST_AUTO_TEST_CASE( QuarterCircleArcLength )
{
    // Quarter of a circle (equal radii, 0 degrees to 90 degrees). Arc length must be pi * r/2.
    const int     r = 1000;
    SHAPE_ELLIPSE arc( VECTOR2I( 0, 0 ), r, r, EDA_ANGLE( 0.0, DEGREES_T ), EDA_ANGLE( 0.0, DEGREES_T ),
                       EDA_ANGLE( 90.0, DEGREES_T ) );
    BOOST_CHECK_CLOSE( arc.GetLength(), 0.5 * M_PI * r, 1e-6 );
}


BOOST_AUTO_TEST_CASE( PointInsideClosedAxisAligned )
{
    // Check points inside, outside, and on the boundary of an unrotated ellipse.
    SHAPE_ELLIPSE e( VECTOR2I( 0, 0 ), 500, 300, EDA_ANGLE( 0.0, DEGREES_T ) );

    BOOST_CHECK( e.PointInside( VECTOR2I( 0, 0 ) ) );      // center
    BOOST_CHECK( e.PointInside( VECTOR2I( 400, 0 ) ) );    // inside on major
    BOOST_CHECK( e.PointInside( VECTOR2I( 0, 200 ) ) );    // inside on minor
    BOOST_CHECK( !e.PointInside( VECTOR2I( 501, 0 ) ) );   // just outside
    BOOST_CHECK( !e.PointInside( VECTOR2I( 0, 301 ) ) );   // just outside
    BOOST_CHECK( !e.PointInside( VECTOR2I( 500, 300 ) ) ); // bbox corner, outside
}


BOOST_AUTO_TEST_CASE( PointInsideRotatedEllipse )
{
    // Ellipse rotated 90 degrees. Major axis runs along Y.
    SHAPE_ELLIPSE e( VECTOR2I( 0, 0 ), 500, 300, EDA_ANGLE( 90.0, DEGREES_T ) );

    BOOST_CHECK( e.PointInside( VECTOR2I( 0, 499 ) ) ); // now tall on Y
    BOOST_CHECK( e.PointInside( VECTOR2I( 299, 0 ) ) ); // now short on X
    BOOST_CHECK( !e.PointInside( VECTOR2I( 0, 501 ) ) );
    BOOST_CHECK( !e.PointInside( VECTOR2I( 301, 0 ) ) );
}


BOOST_AUTO_TEST_CASE( PointInsideArcAlwaysFalse )
{
    // Arcs are open curves with no interior. PointInside must always return false.
    SHAPE_ELLIPSE arc( VECTOR2I( 0, 0 ), 500, 300, EDA_ANGLE( 0.0, DEGREES_T ), EDA_ANGLE( 0.0, DEGREES_T ),
                       EDA_ANGLE( 180.0, DEGREES_T ) );
    BOOST_CHECK( !arc.PointInside( VECTOR2I( 0, 0 ) ) );
    BOOST_CHECK( !arc.PointInside( VECTOR2I( 100, 50 ) ) );
}


BOOST_AUTO_TEST_CASE( SquaredDistanceCircleAgreesWithRadialDistance )
{
    // For a circle (equal radii), distance from a point to the boundary is
    // |distance_to_center - radius|. Verify SquaredDistance agrees.
    const int     r = 1000;
    SHAPE_ELLIPSE c( VECTOR2I( 0, 0 ), r, r, EDA_ANGLE( 0.0, DEGREES_T ) );

    const VECTOR2I p( 2000, 0 );
    const double   expected = 1000.0; // |2000−1000|
    const double   got = std::sqrt( static_cast<double>( c.SquaredDistance( p, true ) ) );
    BOOST_CHECK_LE( std::abs( got - expected ), 1.0 );
}


BOOST_AUTO_TEST_CASE( SquaredDistanceZeroInsideClosedEllipse )
{
    // Point inside the center of a closed ellipse means SquaredDistance should be 0
    SHAPE_ELLIPSE e( VECTOR2I( 0, 0 ), 500, 300, EDA_ANGLE( 30.0, DEGREES_T ) );
    BOOST_CHECK_EQUAL( e.SquaredDistance( VECTOR2I( 0, 0 ), false ), 0 );
}


BOOST_AUTO_TEST_CASE( SquaredDistanceOutlineOnlyReturnsBoundaryDist )
{
    // From the center, outline-only distance should be the minor radius (the
    // closest point on the boundary).
    const int     a = 500, b = 300;
    SHAPE_ELLIPSE e( VECTOR2I( 0, 0 ), a, b, EDA_ANGLE( 0.0, DEGREES_T ) );

    const double d = std::sqrt( static_cast<double>( e.SquaredDistance( VECTOR2I( 0, 0 ), true ) ) );
    BOOST_CHECK_LE( std::abs( d - b ), 2.0 );
}


BOOST_AUTO_TEST_CASE( ConvertToPolylineClosedEllipseIsClosed )
{
    // A closed ellipse's polyline must be marked closed and have enough points
    // to approximate the curve.
    SHAPE_ELLIPSE    e( VECTOR2I( 0, 0 ), 500, 300, EDA_ANGLE( 0.0, DEGREES_T ) );
    SHAPE_LINE_CHAIN chain = e.ConvertToPolyline( 5 );
    BOOST_CHECK( chain.IsClosed() );
    BOOST_CHECK_GT( chain.PointCount(), 8 ); // at least 8 segments
}


BOOST_AUTO_TEST_CASE( ConvertToPolylineArcIsOpenWithCorrectEndpoints )
{
    // An arc's polyline must be open. First point should be at
    // the start angle (1000, 0) and last point at the end angle (-1000, 0).
    SHAPE_ELLIPSE    arc( VECTOR2I( 0, 0 ), 1000, 500, EDA_ANGLE( 0.0, DEGREES_T ), EDA_ANGLE( 0.0, DEGREES_T ),
                          EDA_ANGLE( 180.0, DEGREES_T ) );
    SHAPE_LINE_CHAIN chain = arc.ConvertToPolyline( 5 );

    BOOST_CHECK( !chain.IsClosed() );
    BOOST_CHECK_LE( ( chain.CPoint( 0 ) - VECTOR2I( 1000, 0 ) ).EuclideanNorm(), 2 );
    BOOST_CHECK_LE( ( chain.CPoint( -1 ) - VECTOR2I( -1000, 0 ) ).EuclideanNorm(), 2 );
}


BOOST_AUTO_TEST_CASE( ConvertToPolylineAllPointsWithinMaxError )
{
    // Every tessellated point must lie on the true ellipse.
    // Verify no point deviates more than maxError + rounding tolerance.
    const int     maxErr = 10;
    SHAPE_ELLIPSE e( VECTOR2I( 0, 0 ), 2000, 800, EDA_ANGLE( 37.5, DEGREES_T ) );

    SHAPE_LINE_CHAIN chain = e.ConvertToPolyline( maxErr );

    int maxObserved = 0;
    for( int i = 0; i < chain.PointCount(); ++i )
    {
        const double d = std::sqrt( static_cast<double>( e.SquaredDistance( chain.CPoint( i ), true ) ) );
        maxObserved = std::max( maxObserved, static_cast<int>( std::ceil( d ) ) );
    }
    // Rounding adds up to ~1 IU per axis, so permit maxErr + 2.
    BOOST_CHECK_LE( maxObserved, maxErr + 2 );
}


BOOST_AUTO_TEST_CASE( ConvertToPolylineTighterErrorYieldsMorePoints )
{
    // Tighter error tolerance must produce more points.
    SHAPE_ELLIPSE e( VECTOR2I( 0, 0 ), 1000, 600, EDA_ANGLE( 0.0, DEGREES_T ) );

    const int coarse = e.ConvertToPolyline( 50 ).PointCount();
    const int fine = e.ConvertToPolyline( 5 ).PointCount();

    BOOST_CHECK_GT( fine, coarse );
}


BOOST_AUTO_TEST_CASE( CollideSegmentThroughClosedEllipse )
{
    // Segment passes straight through the ellipse along the major axis.
    // Collision with distance 0 (the segment crosses the boundary).
    SHAPE_ELLIPSE e( VECTOR2I( 0, 0 ), 500, 300, EDA_ANGLE( 0.0, DEGREES_T ) );
    SEG           s( VECTOR2I( -1000, 0 ), VECTOR2I( 1000, 0 ) );

    int actual = -1;
    BOOST_CHECK( e.Collide( s, 0, &actual, nullptr ) );
    BOOST_CHECK_EQUAL( actual, 0 );
}


BOOST_AUTO_TEST_CASE( CollideSegmentEndpointInsideClosedEllipse )
{
    // One endpoint is at the center (inside the ellipse). Collision with
    // distance 0 expected
    SHAPE_ELLIPSE e( VECTOR2I( 0, 0 ), 500, 300, EDA_ANGLE( 0.0, DEGREES_T ) );
    SEG           s( VECTOR2I( 0, 0 ), VECTOR2I( 2000, 0 ) );

    int actual = -1;
    BOOST_CHECK( e.Collide( s, 0, &actual, nullptr ) );
    BOOST_CHECK_EQUAL( actual, 0 );
}


BOOST_AUTO_TEST_CASE( CollideSegmentOutsideFar )
{
    // Segment is far away from the ellipse. No collision even with 100 IU clearance.
    SHAPE_ELLIPSE e( VECTOR2I( 0, 0 ), 500, 300, EDA_ANGLE( 0.0, DEGREES_T ) );
    SEG           s( VECTOR2I( 10000, 10000 ), VECTOR2I( 20000, 20000 ) );

    BOOST_CHECK( !e.Collide( s, 100, nullptr, nullptr ) );
}


BOOST_AUTO_TEST_CASE( CollideSegmentNearMissWithinClearance )
{
    // Segment runs 50 IU above the ellipse's top. With clearance 40 it's a miss,
    // with clearance 60 it's a hit. Reported distance should be close to 50.
    SHAPE_ELLIPSE e( VECTOR2I( 0, 0 ), 500, 300, EDA_ANGLE( 0.0, DEGREES_T ) );
    SEG           s( VECTOR2I( -1000, 350 ), VECTOR2I( 1000, 350 ) );

    BOOST_CHECK( !e.Collide( s, 40, nullptr, nullptr ) ); // 50 > 40

    int actual = -1;
    BOOST_CHECK( e.Collide( s, 60, &actual, nullptr ) ); // 50 < 60
    BOOST_CHECK_LE( std::abs( actual - 50 ), 2 );
}


BOOST_AUTO_TEST_CASE( CollideDegenerateSegmentAtCenter )
{
    // Zero length segment at the center is treated as a point. Inside the
    // ellipse, so collision with distance 0.
    SHAPE_ELLIPSE e( VECTOR2I( 0, 0 ), 500, 300, EDA_ANGLE( 0.0, DEGREES_T ) );
    SEG           s( VECTOR2I( 0, 0 ), VECTOR2I( 0, 0 ) );

    BOOST_CHECK( e.Collide( s, 0, nullptr, nullptr ) );
}


BOOST_AUTO_TEST_CASE( CollideDegenerateSegmentFarAway )
{
    // Zero length segment far away. No collision.
    SHAPE_ELLIPSE e( VECTOR2I( 0, 0 ), 500, 300, EDA_ANGLE( 0.0, DEGREES_T ) );
    SEG           s( VECTOR2I( 5000, 5000 ), VECTOR2I( 5000, 5000 ) );

    BOOST_CHECK( !e.Collide( s, 100, nullptr, nullptr ) );
}


BOOST_AUTO_TEST_CASE( CollideSegmentVsRotatedEllipse )
{
    // Ellipse rotated 90 degrees. Major axis now runs along Y. A vertical
    // segment through center and a horizontal segment at y=350
    // should collide
    SHAPE_ELLIPSE e( VECTOR2I( 0, 0 ), 500, 300, EDA_ANGLE( 90.0, DEGREES_T ) );

    SEG yAxis( VECTOR2I( 0, -1000 ), VECTOR2I( 0, 1000 ) );
    BOOST_CHECK( e.Collide( yAxis, 0, nullptr, nullptr ) );

    SEG crossing( VECTOR2I( -1000, 350 ), VECTOR2I( 1000, 350 ) );
    BOOST_CHECK( e.Collide( crossing, 0, nullptr, nullptr ) );
}


BOOST_AUTO_TEST_CASE( CollideSegmentVsArcUpperHalf )
{
    // Upper-half arc (0 to 180 degrees). A vertical segment crossing the arc
    // should collide. A horizontal segment in the lower half should miss the
    // arc even though it would hit the full ellipse
    SHAPE_ELLIPSE arc( VECTOR2I( 0, 0 ), 500, 300, EDA_ANGLE( 0.0, DEGREES_T ), EDA_ANGLE( 0.0, DEGREES_T ),
                       EDA_ANGLE( 180.0, DEGREES_T ) );

    // Vertical segment at x=300 crosses the arc around (300, 240).
    SEG crossing( VECTOR2I( 300, -500 ), VECTOR2I( 300, 500 ) );
    BOOST_CHECK( arc.Collide( crossing, 0, nullptr, nullptr ) );

    // Horizontal segment at y=−100 stays in the lower half.
    // The full ellipse would intersect it, but the arc sweep does not.
    SEG lower( VECTOR2I( -600, -100 ), VECTOR2I( 600, -100 ) );
    BOOST_CHECK( !arc.Collide( lower, 50, nullptr, nullptr ) ); // 100 > 50
    BOOST_CHECK( arc.Collide( lower, 150, nullptr, nullptr ) ); // 100 < 150
}


BOOST_AUTO_TEST_CASE( CollideCircleDegenerateAgreesWithShapeCircle )
{
    // Equal radii ellipse is a circle. Collide results must match

    const int     r = 1000;
    SHAPE_ELLIPSE e( VECTOR2I( 0, 0 ), r, r, EDA_ANGLE( 0.0, DEGREES_T ) );
    SHAPE_CIRCLE  c( VECTOR2I( 0, 0 ), r );

    const SEG segments[] = {
        SEG( VECTOR2I( 2000, 0 ), VECTOR2I( 2500, 0 ) ),       // far right
        SEG( VECTOR2I( -2000, 0 ), VECTOR2I( 2000, 0 ) ),      // through center
        SEG( VECTOR2I( 1100, 1100 ), VECTOR2I( 2000, 2000 ) ), // outside diagonal
        SEG( VECTOR2I( 500, 500 ), VECTOR2I( 800, 800 ) ),     // inside
    };

    for( const SEG& s : segments )
    {
        BOOST_CHECK_EQUAL( e.Collide( s, 0, nullptr, nullptr ), c.Collide( s, 0, nullptr, nullptr ) );
        BOOST_CHECK_EQUAL( e.Collide( s, 100, nullptr, nullptr ), c.Collide( s, 100, nullptr, nullptr ) );
    }
}


BOOST_AUTO_TEST_CASE( CollideEllipseVsCircleOverlap )
{
    // Ellipse and circle overlap. They should collide.
    SHAPE_ELLIPSE e( VECTOR2I( 0, 0 ), 500, 300, EDA_ANGLE( 0.0, DEGREES_T ) );
    SHAPE_CIRCLE  c( VECTOR2I( 400, 0 ), 200 );

    BOOST_CHECK( e.Collide( &c, 0, nullptr, nullptr ) );
}


BOOST_AUTO_TEST_CASE( CollideEllipseVsCircleDisjoint )
{
    // Ellipse and circle far apart. There is no collision.
    SHAPE_ELLIPSE e( VECTOR2I( 0, 0 ), 500, 300, EDA_ANGLE( 0.0, DEGREES_T ) );
    SHAPE_CIRCLE  c( VECTOR2I( 2000, 2000 ), 100 );

    BOOST_CHECK( !e.Collide( &c, 0, nullptr, nullptr ) );
}


BOOST_AUTO_TEST_CASE( CollideEllipseVsRectContainsEllipse )
{
    // Large rectangle entirely contains the ellipse. Collision detected
    // because the ellipse center is inside the rect.
    SHAPE_ELLIPSE e( VECTOR2I( 0, 0 ), 500, 300, EDA_ANGLE( 0.0, DEGREES_T ) );
    SHAPE_RECT    r( VECTOR2I( -2000, -2000 ), 4000, 4000 );

    BOOST_CHECK( e.Collide( &r, 0, nullptr, nullptr ) );
}


BOOST_AUTO_TEST_CASE( CollideEllipseVsRectDisjoint )
{
    // Small rectangle far from the ellipse. No collision.
    SHAPE_ELLIPSE e( VECTOR2I( 0, 0 ), 500, 300, EDA_ANGLE( 0.0, DEGREES_T ) );
    SHAPE_RECT    r( VECTOR2I( 2000, 2000 ), 100, 100 );

    BOOST_CHECK( !e.Collide( &r, 0, nullptr, nullptr ) );
}


BOOST_AUTO_TEST_CASE( CollideEllipseVsRectEdgeIntersects )
{
    // Rectangle straddles the ellipse's right edge. We have partial overlap.
    SHAPE_ELLIPSE e( VECTOR2I( 0, 0 ), 500, 300, EDA_ANGLE( 0.0, DEGREES_T ) );
    SHAPE_RECT    r( VECTOR2I( 400, -100 ), 400, 200 );

    BOOST_CHECK( e.Collide( &r, 0, nullptr, nullptr ) );
}


BOOST_AUTO_TEST_CASE( CollideEllipseVsLineChainIntersects )
{
    // Open polyline passes through the ellipse. It should collide.
    SHAPE_ELLIPSE    e( VECTOR2I( 0, 0 ), 500, 300, EDA_ANGLE( 0.0, DEGREES_T ) );
    SHAPE_LINE_CHAIN chain;
    chain.Append( VECTOR2I( -1000, 100 ) );
    chain.Append( VECTOR2I( 1000, 100 ) ); // horizontal line piercing the ellipse
    chain.Append( VECTOR2I( 1000, 500 ) );

    BOOST_CHECK( e.Collide( &chain, 0, nullptr, nullptr ) );
}


BOOST_AUTO_TEST_CASE( CollideEllipseInsideClosedChain )
{
    // Closed square chain entirely contains the ellipse. Collision detected
    SHAPE_LINE_CHAIN chain;
    chain.Append( VECTOR2I( -2000, -2000 ) );
    chain.Append( VECTOR2I( 2000, -2000 ) );
    chain.Append( VECTOR2I( 2000, 2000 ) );
    chain.Append( VECTOR2I( -2000, 2000 ) );
    chain.SetClosed( true );

    SHAPE_ELLIPSE e( VECTOR2I( 0, 0 ), 500, 300, EDA_ANGLE( 0.0, DEGREES_T ) );

    BOOST_CHECK( e.Collide( &chain, 0, nullptr, nullptr ) );
}


BOOST_AUTO_TEST_CASE( CollideEllipseVsArcOverlap )
{
    // Circular arc overlaps the ellipse. It should collide.
    SHAPE_ELLIPSE e( VECTOR2I( 0, 0 ), 500, 300, EDA_ANGLE( 0.0, DEGREES_T ) );
    SHAPE_ARC     arc( VECTOR2I( 0, 0 ), VECTOR2I( 400, 0 ), EDA_ANGLE( 180.0, DEGREES_T ), 0 );

    BOOST_CHECK( e.Collide( &arc, 0, nullptr, nullptr ) );
}


BOOST_AUTO_TEST_CASE( CollideEllipseVsEllipseOverlap )
{
    // Two overlapping ellipses. They should collide.
    SHAPE_ELLIPSE a( VECTOR2I( 0, 0 ), 500, 300, EDA_ANGLE( 0.0, DEGREES_T ) );
    SHAPE_ELLIPSE b( VECTOR2I( 400, 0 ), 400, 200, EDA_ANGLE( 0.0, DEGREES_T ) );

    BOOST_CHECK( a.Collide( &b, 0, nullptr, nullptr ) );
}


BOOST_AUTO_TEST_CASE( CollideEllipseVsEllipseDisjoint )
{
    // Two ellipses far apart. There is no collision.
    SHAPE_ELLIPSE a( VECTOR2I( 0, 0 ), 500, 300, EDA_ANGLE( 0.0, DEGREES_T ) );
    SHAPE_ELLIPSE b( VECTOR2I( 3000, 3000 ), 400, 200, EDA_ANGLE( 0.0, DEGREES_T ) );

    BOOST_CHECK( !a.Collide( &b, 0, nullptr, nullptr ) );
}


BOOST_AUTO_TEST_CASE( CollideEllipseInsideAnotherEllipse )
{
    // Small ellipse inside a larger one. Collision should be detected.
    SHAPE_ELLIPSE big( VECTOR2I( 0, 0 ), 2000, 1500, EDA_ANGLE( 0.0, DEGREES_T ) );
    SHAPE_ELLIPSE small( VECTOR2I( 0, 0 ), 200, 100, EDA_ANGLE( 0.0, DEGREES_T ) );

    BOOST_CHECK( big.Collide( &small, 0, nullptr, nullptr ) );
}


BOOST_AUTO_TEST_CASE( CollideEllipseVsRect )
{
    SHAPE_ELLIPSE e( { 0, 0 }, 500, 300, ANGLE_0 );
    SHAPE_RECT    r( { 400, 0 }, 200, 200 ); // straddles the ellipse boundary
    int           actual = -1;
    BOOST_CHECK( e.Collide( &r, 0, &actual ) );
    BOOST_CHECK_EQUAL( actual, 0 );
}


BOOST_AUTO_TEST_CASE( RotateByFullTurnPreservesBBox )
{
    // Rotating 360 degrees about any point should leave the bbox unchanged
    SHAPE_ELLIPSE e( VECTOR2I( 100, 200 ), 500, 300, EDA_ANGLE( 30.0, DEGREES_T ) );
    const BOX2I   before = e.BBox();

    e.Rotate( EDA_ANGLE( 360.0, DEGREES_T ), VECTOR2I( 0, 0 ) );

    const BOX2I after = e.BBox();

    // Full 360° rotation: bbox position and size should match (integer rounding <= 2 IU).
    BOOST_CHECK_LE( std::abs( before.GetLeft() - after.GetLeft() ), 2 );
    BOOST_CHECK_LE( std::abs( before.GetRight() - after.GetRight() ), 2 );
    BOOST_CHECK_LE( std::abs( before.GetTop() - after.GetTop() ), 2 );
    BOOST_CHECK_LE( std::abs( before.GetBottom() - after.GetBottom() ), 2 );
}


BOOST_AUTO_TEST_CASE( RotateCircleAboutCenterIsBBoxInvariant )
{
    // A circle's bbox doesn't change under any rotation about its center.
    SHAPE_ELLIPSE c( VECTOR2I( 0, 0 ), 1000, 1000, EDA_ANGLE( 0.0, DEGREES_T ) );
    const BOX2I   before = c.BBox();

    c.Rotate( EDA_ANGLE( 47.5, DEGREES_T ), VECTOR2I( 0, 0 ) );

    const BOX2I after = c.BBox();

    BOOST_CHECK_LE( std::abs( before.GetLeft() - after.GetLeft() ), 2 );
    BOOST_CHECK_LE( std::abs( before.GetRight() - after.GetRight() ), 2 );
    BOOST_CHECK_LE( std::abs( before.GetTop() - after.GetTop() ), 2 );
    BOOST_CHECK_LE( std::abs( before.GetBottom() - after.GetBottom() ), 2 );
}


BOOST_AUTO_TEST_CASE( RotateAboutNonCenterTranslatesAndRotates )
{
    // Rotating 180 degrees about (1000, 0) moves the center from (0, 0) to
    // (2000, 0) and subtracts 180 from the ellipse's internal rotation.
    SHAPE_ELLIPSE e( VECTOR2I( 0, 0 ), 500, 300, EDA_ANGLE( 0.0, DEGREES_T ) );
    e.Rotate( EDA_ANGLE( 180.0, DEGREES_T ), VECTOR2I( 1000, 0 ) );

    BOOST_CHECK_LE( std::abs( e.GetCenter().x - 2000 ), 1 );
    BOOST_CHECK_LE( std::abs( e.GetCenter().y - 0 ), 1 );

    BOOST_CHECK_CLOSE( e.GetRotation().AsDegrees(), -180.0, 1e-6 );
}


BOOST_AUTO_TEST_CASE( MirrorLeftRightFlipsCenter )
{
    // Left-right mirror across x=1000: center.x flips from 100 to 1900,
    // rotation negates from 30 to -30 degrees.
    SHAPE_ELLIPSE e( VECTOR2I( 100, 200 ), 500, 300, EDA_ANGLE( 30.0, DEGREES_T ) );
    e.Mirror( VECTOR2I( 1000, 0 ), FLIP_DIRECTION::LEFT_RIGHT );

    BOOST_CHECK_EQUAL( e.GetCenter().x, 1900 );
    BOOST_CHECK_EQUAL( e.GetCenter().y, 200 );

    BOOST_CHECK_CLOSE( e.GetRotation().AsDegrees(), -30.0, 1e-6 );
}


BOOST_AUTO_TEST_CASE( MirrorTopBottomFlipsCenter )
{
    // Top-bottom mirror across y=500: center.y flips from 200 to 800,
    // rotation negates
    SHAPE_ELLIPSE e( VECTOR2I( 100, 200 ), 500, 300, EDA_ANGLE( 30.0, DEGREES_T ) );
    e.Mirror( VECTOR2I( 0, 500 ), FLIP_DIRECTION::TOP_BOTTOM );

    BOOST_CHECK_EQUAL( e.GetCenter().x, 100 );
    BOOST_CHECK_EQUAL( e.GetCenter().y, 800 );

    BOOST_CHECK_CLOSE( e.GetRotation().AsDegrees(), -30.0, 1e-6 );
}


BOOST_AUTO_TEST_CASE( MirrorLeftRightTwiceIsIdentity )
{
    // Mirroring left-right twice with the same axis returns to the original
    // center and rotation.
    SHAPE_ELLIPSE  e( VECTOR2I( 100, 200 ), 500, 300, EDA_ANGLE( 45.0, DEGREES_T ) );
    const VECTOR2I origCenter = e.GetCenter();
    const double   origRotation = e.GetRotation().AsDegrees();

    e.Mirror( VECTOR2I( 1234, 5678 ), FLIP_DIRECTION::LEFT_RIGHT );
    e.Mirror( VECTOR2I( 1234, 5678 ), FLIP_DIRECTION::LEFT_RIGHT );

    BOOST_CHECK_EQUAL( e.GetCenter().x, origCenter.x );
    BOOST_CHECK_EQUAL( e.GetCenter().y, origCenter.y );
    BOOST_CHECK_CLOSE( e.GetRotation().AsDegrees(), origRotation, 1e-6 );
}


BOOST_AUTO_TEST_CASE( MirrorTopBottomTwiceIsIdentity )
{
    // Mirroring top-bottom twice returns to the original state.
    SHAPE_ELLIPSE  e( VECTOR2I( 100, 200 ), 500, 300, EDA_ANGLE( 45.0, DEGREES_T ) );
    const VECTOR2I origCenter = e.GetCenter();
    const double   origRotation = e.GetRotation().AsDegrees();

    e.Mirror( VECTOR2I( 1234, 5678 ), FLIP_DIRECTION::TOP_BOTTOM );
    e.Mirror( VECTOR2I( 1234, 5678 ), FLIP_DIRECTION::TOP_BOTTOM );

    BOOST_CHECK_EQUAL( e.GetCenter().x, origCenter.x );
    BOOST_CHECK_EQUAL( e.GetCenter().y, origCenter.y );
    BOOST_CHECK_CLOSE( e.GetRotation().AsDegrees(), origRotation, 1e-6 );
}


BOOST_AUTO_TEST_CASE( MirrorArcSwapsAndReflectsSweep )
{
    // Symmetric arc [30, 150] mirrored left-right. The sweep [30, 150] is
    // symmetric about 90 degrees, so start and end angles stay the same.
    SHAPE_ELLIPSE arc( VECTOR2I( 0, 0 ), 500, 300, EDA_ANGLE( 0.0, DEGREES_T ), EDA_ANGLE( 30.0, DEGREES_T ),
                       EDA_ANGLE( 150.0, DEGREES_T ) );

    arc.Mirror( VECTOR2I( 0, 0 ), FLIP_DIRECTION::LEFT_RIGHT );

    BOOST_CHECK_CLOSE( arc.GetStartAngle().AsDegrees(), 30.0, 1e-6 );
    BOOST_CHECK_CLOSE( arc.GetEndAngle().AsDegrees(), 150.0, 1e-6 );
}


BOOST_AUTO_TEST_CASE( MirrorArcAsymmetricSwapsSweep )
{
    // Asymmetric arc [20, 80] mirrored left-right. Start becomes 180-80=100,
    // end becomes 180-20=160.
    SHAPE_ELLIPSE arc( VECTOR2I( 0, 0 ), 500, 300, EDA_ANGLE( 0.0, DEGREES_T ), EDA_ANGLE( 20.0, DEGREES_T ),
                       EDA_ANGLE( 80.0, DEGREES_T ) );

    arc.Mirror( VECTOR2I( 0, 0 ), FLIP_DIRECTION::LEFT_RIGHT );

    BOOST_CHECK_CLOSE( arc.GetStartAngle().AsDegrees(), 100.0, 1e-6 ); // 180 - 80
    BOOST_CHECK_CLOSE( arc.GetEndAngle().AsDegrees(), 160.0, 1e-6 );   // 180 - 20
}


BOOST_AUTO_TEST_CASE( FormatContainsExpectedFields )
{
    SHAPE_ELLIPSE e( VECTOR2I( 100, 200 ), 500, 300, EDA_ANGLE( 30.0, DEGREES_T ) );

    const std::string cpp = e.Format( true );
    // output mentions all the key numeric values.
    BOOST_CHECK( cpp.find( "SHAPE_ELLIPSE" ) != std::string::npos );
    BOOST_CHECK( cpp.find( "100" ) != std::string::npos );
    BOOST_CHECK( cpp.find( "200" ) != std::string::npos );
    BOOST_CHECK( cpp.find( "500" ) != std::string::npos );
    BOOST_CHECK( cpp.find( "300" ) != std::string::npos );

    const std::string plain = e.Format( false );
    BOOST_CHECK( plain.find( "500" ) != std::string::npos );
    BOOST_CHECK( plain.find( "300" ) != std::string::npos );
}


BOOST_AUTO_TEST_CASE( FuzzRandomEllipsesInvariantsAndDeterminism )
{
    // 10,000 random ellipses. Check that major >= minor after construction
    // bbox is non-degenerate, SquaredDistance gives the same result when
    // called multiple times and PointInside is consistent
    // with SquaredDistance returning 0 for interior points.
    std::mt19937                           rng( 12345 );
    std::uniform_int_distribution<int>     centerDist( -10000, 10000 );
    std::uniform_int_distribution<int>     radiusDist( 50, 2000 );
    std::uniform_real_distribution<double> angleDist( 0.0, 360.0 );
    std::uniform_int_distribution<int>     ptDist( -15000, 15000 );

    const int N = 10000;
    int       determinismFailures = 0;

    for( int i = 0; i < N; ++i )
    {
        const VECTOR2I  center( centerDist( rng ), centerDist( rng ) );
        const int       r1 = radiusDist( rng );
        const int       r2 = radiusDist( rng );
        const EDA_ANGLE rot( angleDist( rng ), DEGREES_T );

        SHAPE_ELLIPSE e( center, r1, r2, rot );

        // major >= minor > 0 after normalize
        BOOST_CHECK_GE( e.GetMajorRadius(), e.GetMinorRadius() );
        BOOST_CHECK_GT( e.GetMinorRadius(), 0 );

        // BBox is non-degenerate
        const BOX2I bbox = e.BBox();
        BOOST_CHECK_GT( bbox.GetWidth(), 0 );
        BOOST_CHECK_GT( bbox.GetHeight(), 0 );

        // SquaredDistance and PointInside are functions of their inputs
        const VECTOR2I    testPt( ptDist( rng ), ptDist( rng ) );
        const SEG::ecoord d1 = e.SquaredDistance( testPt, false );
        const SEG::ecoord d2 = e.SquaredDistance( testPt, false );
        const SEG::ecoord d3 = e.SquaredDistance( testPt, false );

        if( d1 != d2 || d2 != d3 )
            ++determinismFailures;

        // If PointInside is true for a closed ellipse, non-outline
        // SquaredDistance must be 0.
        if( !e.IsArc() && e.PointInside( testPt ) )
            BOOST_CHECK_EQUAL( e.SquaredDistance( testPt, false ), 0 );
    }

    BOOST_CHECK_EQUAL( determinismFailures, 0 );
}


BOOST_AUTO_TEST_CASE( CrossCheckSegmentCollideAgainstTessellation )
{
    std::mt19937                           rng( 9999 );
    std::uniform_int_distribution<int>     centerDist( -5000, 5000 );
    std::uniform_int_distribution<int>     radiusDist( 100, 1500 );
    std::uniform_real_distribution<double> angleDist( 0.0, 360.0 );
    std::uniform_int_distribution<int>     segDist( -8000, 8000 );

    const int N = 5000;
    const int clearance = 50;
    int       mismatches = 0;

    for( int i = 0; i < N; ++i )
    {
        const VECTOR2I  ec( centerDist( rng ), centerDist( rng ) );
        const int       r1 = radiusDist( rng );
        const int       r2 = radiusDist( rng );
        const EDA_ANGLE rot( angleDist( rng ), DEGREES_T );

        SHAPE_ELLIPSE e( ec, r1, r2, rot );
        const SEG     s( VECTOR2I( segDist( rng ), segDist( rng ) ), VECTOR2I( segDist( rng ), segDist( rng ) ) );

        const bool analyticCollide = e.Collide( s, clearance, nullptr, nullptr );

        // Brute force: very fine tessellation + SHAPE_LINE_CHAIN::Collide.
        // Tessellation error 2 IU << clearance 50 IU, so boundary ambiguity is small.
        const SHAPE_LINE_CHAIN chain = e.ConvertToPolyline( 2 );
        bool                   bruteCollide = chain.Collide( s, clearance, nullptr, nullptr );

        // The tessellated chain's Collide does not model the closed ellipse interior.
        // Compensate: for a closed ellipse, a segment endpoint inside the interior is
        // a collision too.
        if( !bruteCollide && !e.IsArc() )
        {
            if( e.PointInside( s.A ) || e.PointInside( s.B ) )
                bruteCollide = true;
        }

        if( analyticCollide != bruteCollide )
            ++mismatches;
    }

    BOOST_TEST_MESSAGE( "cross-check mismatches: " << mismatches << " / " << N );

    // Near-threshold cases may disagree within the 2 IU tessellation budget.
    // Allow up to 1% boundary mismatches.
    BOOST_CHECK_LE( mismatches, N / 100 );
}


/**
 * Test that normalize() adjusts arc angles when swapping major/minor radii.
 * When minor > major, radii swap and rotation += 90. For arcs, angles must
 * shift by -90 to keep the same physical arc.
 */
BOOST_AUTO_TEST_CASE( NormalizeArcAnglesOnSwap )
{
    // Construct with minor > major to force a swap in normalize().
    // Major=20, Minor=50 → after normalize: Major=50, Minor=20, Rotation += 90
    SHAPE_ELLIPSE e( VECTOR2I( 0, 0 ), 20, 50, EDA_ANGLE( 0, DEGREES_T ), EDA_ANGLE( 30.0, DEGREES_T ),
                     EDA_ANGLE( 120.0, DEGREES_T ) );

    // After swap: major=50, minor=20
    BOOST_CHECK_EQUAL( e.GetMajorRadius(), 50 );
    BOOST_CHECK_EQUAL( e.GetMinorRadius(), 20 );

    // Rotation should be 0 + 90 = 90
    BOOST_CHECK_CLOSE( e.GetRotation().AsDegrees(), 90.0, 1e-6 );

    // Angles should shift by -90: 30-90=-60, 120-90=30
    BOOST_CHECK_CLOSE( e.GetStartAngle().AsDegrees(), -60.0, 1e-6 );
    BOOST_CHECK_CLOSE( e.GetEndAngle().AsDegrees(), 30.0, 1e-6 );

    // Sweep should be preserved: was 90, still 90
    double sweep = e.GetEndAngle().AsDegrees() - e.GetStartAngle().AsDegrees();
    BOOST_CHECK_CLOSE( sweep, 90.0, 1e-6 );
}


/**
 * Test that normalize() does NOT adjust angles for closed ellipses (only arcs).
 */
BOOST_AUTO_TEST_CASE( NormalizeClosedEllipseNoAngleShift )
{
    // Closed ellipse with minor > major.
    SHAPE_ELLIPSE e( VECTOR2I( 0, 0 ), 20, 50, EDA_ANGLE( 0, DEGREES_T ) );

    BOOST_CHECK_EQUAL( e.GetMajorRadius(), 50 );
    BOOST_CHECK_EQUAL( e.GetMinorRadius(), 20 );
    BOOST_CHECK_CLOSE( e.GetRotation().AsDegrees(), 90.0, 1e-6 );
    BOOST_CHECK( !e.IsArc() );
}


/**
 * Test that isAngleInSweep handles full-sweep (start=0, end=360) correctly.
 */
BOOST_AUTO_TEST_CASE( FullSweepAngleInSweep )
{
    SHAPE_ELLIPSE e( VECTOR2I( 0, 0 ), 100, 50, EDA_ANGLE( 0, DEGREES_T ), EDA_ANGLE( 0, DEGREES_T ),
                     EDA_ANGLE( 360.0, DEGREES_T ) );

    // Every angle should be in the sweep for a full 360 arc.
    BOOST_CHECK( e.Collide( SEG( VECTOR2I( 100, 0 ), VECTOR2I( 100, 0 ) ), 1 ) );   // 0 degrees
    BOOST_CHECK( e.Collide( SEG( VECTOR2I( 0, 50 ), VECTOR2I( 0, 50 ) ), 1 ) );     // 90 degrees
    BOOST_CHECK( e.Collide( SEG( VECTOR2I( -100, 0 ), VECTOR2I( -100, 0 ) ), 1 ) ); // 180 degrees
    BOOST_CHECK( e.Collide( SEG( VECTOR2I( 0, -50 ), VECTOR2I( 0, -50 ) ), 1 ) );   // 270 degrees
}


/**
 * Test that the cache is consistent after SetRotation.
 */
BOOST_AUTO_TEST_CASE( CacheConsistencyAfterSetRotation )
{
    SHAPE_ELLIPSE e( VECTOR2I( 0, 0 ), 100, 50, EDA_ANGLE( 0, DEGREES_T ) );

    // Get BBox at rotation=0
    BOX2I box0 = e.BBox( 0 );

    // Rotate 90 degrees so major and minor visual extents swap.
    e.SetRotation( EDA_ANGLE( 90.0, DEGREES_T ) );
    BOX2I box90 = e.BBox( 0 );

    // At 0 degrees: width dominated by major (100), height by minor (50)
    // At 90 degrees: width dominated by minor (50), height by major (100)
    BOOST_CHECK_EQUAL( box0.GetWidth(), box90.GetHeight() );
    BOOST_CHECK_EQUAL( box0.GetHeight(), box90.GetWidth() );
}


/**
 * Test that the cache is consistent after SetMajorRadius.
 */
BOOST_AUTO_TEST_CASE( CacheConsistencyAfterSetMajorRadius )
{
    SHAPE_ELLIPSE e( VECTOR2I( 0, 0 ), 100, 50, EDA_ANGLE( 0, DEGREES_T ) );

    BOX2I box1 = e.BBox( 0 );

    e.SetMajorRadius( 200 );
    BOX2I box2 = e.BBox( 0 );

    // Width should double (major axis is along X at rotation=0).
    BOOST_CHECK_EQUAL( box2.GetWidth(), box1.GetWidth() * 2 );
    // Height should stay the same (minor unchanged).
    BOOST_CHECK_EQUAL( box2.GetHeight(), box1.GetHeight() );
}


/**
 * Test PointInside after rotation change. Test if cache is stale.
 */
BOOST_AUTO_TEST_CASE( PointInsideAfterRotationChange )
{
    SHAPE_ELLIPSE e( VECTOR2I( 0, 0 ), 100, 30, EDA_ANGLE( 0, DEGREES_T ) );

    // Point at (80, 0) is inside when major axis is along X.
    BOOST_CHECK( e.PointInside( VECTOR2I( 80, 0 ) ) );
    // Point at (0, 80) is outside (minor = 30).
    BOOST_CHECK( !e.PointInside( VECTOR2I( 0, 80 ) ) );

    // Rotate 90, major axis now along Y.
    e.SetRotation( EDA_ANGLE( 90.0, DEGREES_T ) );

    // Now (80, 0) should be outside and (0, 80) inside.
    BOOST_CHECK( !e.PointInside( VECTOR2I( 80, 0 ) ) );
    BOOST_CHECK( e.PointInside( VECTOR2I( 0, 80 ) ) );
}


BOOST_AUTO_TEST_SUITE_END()