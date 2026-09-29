/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2014 Jean-Pierre Charras, jp.charras at wanadoo.fr
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
 * @file trigo.cpp
 * @brief Trigonometric and geometric basic functions.
 */

#include <algorithm>        // for std::clamp
#include <limits>           // for numeric_limits
#include <cmath>
#include <cstdlib>         // for abs
#include <type_traits>      // for swap

#include <geometry/seg.h>
#include <math/util.h>
#include <math/wide_int.h>
#include <math/vector2d.h>  // for VECTOR2I
#include <trigo.h>


/*
CircleCenterFrom3Points calculate the center of a circle defined by 3 points
It is similar to CalcArcCenter( const VECTOR2D& aStart, const VECTOR2D& aMid, const VECTOR2D& aEnd )
but it was needed to debug CalcArcCenter, so I keep it available for other issues in CalcArcCenter

The perpendicular bisector of the segment between two points is the
set of all points equidistant from both.  So if you take the
perpendicular bisector of (x1,y1) and (x2,y2) and the perpendicular
bisector of the segment from (x2,y2) to (x3,y3) and find the
intersection of those lines, that point will be the center.

To find the equation of the perpendicular bisector of (x1,y1) to (x2,y2),
you know that it passes through the midpoint of the segment:
((x1+x2)/2,(y1+y2)/2), and if the slope of the line
connecting (x1,y1) to (x2,y2) is m, the slope of the perpendicular
bisector is -1/m.  Work out the equations for the two lines, find
their intersection, and bingo!  You've got the coordinates of the center.

An error should occur if the three points lie on a line, and you'll
need special code to check for the case where one of the slopes is zero.

see https://web.archive.org/web/20171223103555/http://mathforum.org/library/drmath/view/54323.html
*/

//#define USE_ALTERNATE_CENTER_ALGO

#ifdef USE_ALTERNATE_CENTER_ALGO
bool CircleCenterFrom3Points( const VECTOR2D& p1, const VECTOR2D& p2,  const VECTOR2D& p3, VECTOR2D* aCenter )
{
    // Move coordinate origin to p2, to simplify calculations
    VECTOR2D b = p1 - p2;
    VECTOR2D d = p3 - p2;
    double bc = ( b.x*b.x + b.y*b.y ) / 2.0;
    double cd = ( -d.x*d.x - d.y*d.y ) / 2.0;
    double det = -b.x*d.y + d.x*b.y;

    if( fabs(det) < 1.0e-6 )     // arbitrary limit to avoid divide by 0
        return false;

    det = 1/det;
    aCenter->x = ( -bc*d.y - cd*b.y ) * det;
    aCenter->y = ( b.x*cd + d.x*bc ) * det;
    *aCenter += p2;

    return true;
}
#endif

bool IsPointOnSegment( const VECTOR2I& aSegStart, const VECTOR2I& aSegEnd,
                       const VECTOR2I& aTestPoint )
{
    VECTOR2I vectSeg = aSegEnd - aSegStart;      // Vector from S1 to S2
    VECTOR2I vectPoint = aTestPoint - aSegStart; // Vector from S1 to P

    // Use long long here to avoid overflow in calculations
    if( (long long) vectSeg.x * vectPoint.y - (long long) vectSeg.y * vectPoint.x )
        return false;         /* Cross product non-zero, vectors not parallel */

    if( ( (long long) vectSeg.x * vectPoint.x + (long long) vectSeg.y * vectPoint.y ) <
        ( (long long) vectPoint.x * vectPoint.x + (long long) vectPoint.y * vectPoint.y ) )
        return false;          /* Point not on segment */

    return true;
}


bool SegmentIntersectsSegment( const VECTOR2I& a_p1_l1, const VECTOR2I& a_p2_l1,
                               const VECTOR2I& a_p1_l2, const VECTOR2I& a_p2_l2,
                               VECTOR2I* aIntersectionPoint )
{

    // We are forced to use 64bit ints because the internal units can overflow 32bit ints when
    // multiplied with each other, the alternative would be to scale the units down (i.e. divide
    // by a fixed number).
    int64_t dX_a, dY_a, dX_b, dY_b, dX_ab, dY_ab;
    int64_t num_a, num_b, den;

    // Test for intersection within the bounds of both line segments using line equations of the
    // form:
    // x_k(u_k) = u_k * dX_k + x_k(0)
    // y_k(u_k) = u_k * dY_k + y_k(0)
    // with  0 <= u_k <= 1 and k = [ a, b ]

    dX_a  = int64_t{ a_p2_l1.x } - a_p1_l1.x;
    dY_a  = int64_t{ a_p2_l1.y } - a_p1_l1.y;
    dX_b  = int64_t{ a_p2_l2.x } - a_p1_l2.x;
    dY_b  = int64_t{ a_p2_l2.y } - a_p1_l2.y;
    dX_ab = int64_t{ a_p1_l2.x } - a_p1_l1.x;
    dY_ab = int64_t{ a_p1_l2.y } - a_p1_l1.y;

    den   = dY_a  * dX_b - dY_b * dX_a ;

    // Check if lines are parallel.
    if( den == 0 )
        return false;

    num_a = dY_ab * dX_b - dY_b * dX_ab;
    num_b = dY_ab * dX_a - dY_a * dX_ab;

    // Only compute the intersection point if requested.
    if( aIntersectionPoint )
    {
        *aIntersectionPoint = a_p1_l1;
        aIntersectionPoint->x += KiROUND( dX_a * ( double )num_a / ( double )den );
        aIntersectionPoint->y += KiROUND( dY_a * ( double )num_b / ( double )den );
    }

    if( den < 0 )
    {
        den   = -den;
        num_a = -num_a;
        num_b = -num_b;
    }

    // Test sign( u_a ) and return false if negative.
    if( num_a < 0 )
        return false;

    // Test sign( u_b ) and return false if negative.
    if( num_b < 0 )
        return false;

    // Test to ensure (u_a <= 1).
    if( num_a > den )
        return false;

    // Test to ensure (u_b <= 1).
    if( num_b > den )
        return false;

    return true;
}


bool TestSegmentHit( const VECTOR2I& aRefPoint, const VECTOR2I& aStart, const VECTOR2I& aEnd,
                     int aDist )
{
    int xmin = aStart.x;
    int xmax = aEnd.x;
    int ymin = aStart.y;
    int ymax = aEnd.y;
    VECTOR2I delta = aStart - aRefPoint;

    if( xmax < xmin )
        std::swap( xmax, xmin );

    if( ymax < ymin )
        std::swap( ymax, ymin );

    // Check if we are outside of the bounding box.
    if( ( ymin - aRefPoint.y > aDist ) || ( aRefPoint.y - ymax > aDist ) )
        return false;

    if( ( xmin - aRefPoint.x > aDist ) || ( aRefPoint.x - xmax > aDist ) )
        return false;

    // Eliminate easy cases.
    if( aStart.x == aEnd.x && aRefPoint.y > ymin && aRefPoint.y < ymax )
        return std::abs( delta.x ) <= aDist;

    if( aStart.y == aEnd.y && aRefPoint.x > xmin && aRefPoint.x < xmax )
        return std::abs( delta.y ) <= aDist;

    SEG segment( aStart, aEnd );
    return segment.SquaredDistance( aRefPoint ) < SEG::Square( aDist + 1 );
}


const VECTOR2I CalcArcMid( const VECTOR2I& aStart, const VECTOR2I& aEnd, const VECTOR2I& aCenter,
                           bool aMinArcAngle )
{
    VECTOR2I startVector = aStart - aCenter;
    VECTOR2I endVector = aEnd - aCenter;

    EDA_ANGLE startAngle( startVector );
    EDA_ANGLE endAngle( endVector );
    EDA_ANGLE midPointRotAngle = ( startAngle - endAngle ).Normalize180() / 2;

    if( !aMinArcAngle )
        midPointRotAngle += ANGLE_180;

    VECTOR2I newMid = aStart;
    RotatePoint( newMid, aCenter, midPointRotAngle );

    return newMid;
}


void RotatePoint( int* pX, int* pY, const EDA_ANGLE& aAngle )
{
    VECTOR2I  pt;
    EDA_ANGLE angle = aAngle;

    angle.Normalize();

    // Cheap and dirty optimizations for 0, 90, 180, and 270 degrees.
    if( angle == ANGLE_0 )
    {
        pt = VECTOR2I( *pX, *pY );
    }
    else if( angle == ANGLE_90 )          /* sin = 1, cos = 0 */
    {
        pt = VECTOR2I( *pY, -*pX );
    }
    else if( angle == ANGLE_180 )    /* sin = 0, cos = -1 */
    {
        pt = VECTOR2I( -*pX, -*pY );
    }
    else if( angle == ANGLE_270 )    /* sin = -1, cos = 0 */
    {
        pt = VECTOR2I( -*pY, *pX );
    }
    else
    {
        double sinus = angle.Sin();
        double cosinus = angle.Cos();

        pt.x = KiROUND( ( *pY * sinus ) + ( *pX * cosinus ) );
        pt.y = KiROUND( ( *pY * cosinus ) - ( *pX * sinus ) );
    }

    *pX = pt.x;
    *pY = pt.y;
}


void RotatePoint( int* pX, int* pY, int cx, int cy, const EDA_ANGLE& angle )
{
    int ox, oy;

    ox = *pX - cx;
    oy = *pY - cy;

    RotatePoint( &ox, &oy, angle );

    *pX = ox + cx;
    *pY = oy + cy;
}


void RotatePoint( double* pX, double* pY, double cx, double cy, const EDA_ANGLE& angle )
{
    double ox, oy;

    ox = *pX - cx;
    oy = *pY - cy;

    RotatePoint( &ox, &oy, angle );

    *pX = ox + cx;
    *pY = oy + cy;
}


void RotatePoint( double* pX, double* pY, const EDA_ANGLE& aAngle )
{
    EDA_ANGLE angle = aAngle;
    VECTOR2D  pt;

    angle.Normalize();

    // Cheap and dirty optimizations for 0, 90, 180, and 270 degrees.
    if( angle == ANGLE_0 )
    {
        pt = VECTOR2D( *pX, *pY );
    }
    else if( angle == ANGLE_90 )          /* sin = 1, cos = 0 */
    {
        pt = VECTOR2D( *pY, -*pX );
    }
    else if( angle == ANGLE_180 )    /* sin = 0, cos = -1 */
    {
        pt = VECTOR2D( -*pX, -*pY );
    }
    else if( angle == ANGLE_270 )    /* sin = -1, cos = 0 */
    {
        pt = VECTOR2D( -*pY, *pX );
    }
    else
    {
        double sinus = angle.Sin();
        double cosinus = angle.Cos();

        pt.x = ( *pY * sinus ) + ( *pX * cosinus );
        pt.y = ( *pY * cosinus ) - ( *pX * sinus );
    }

    *pX = pt.x;
    *pY = pt.y;
}


const VECTOR2D CalcArcCenter( const VECTOR2D& aStart, const VECTOR2D& aEnd,
                              const EDA_ANGLE& aAngle )
{
    EDA_ANGLE angle( aAngle );
    VECTOR2D  start = aStart;
    VECTOR2D  end = aEnd;

    if( angle < ANGLE_0 )
    {
        std::swap( start, end );
        angle = -angle;
    }

    if( angle > ANGLE_180 )
    {
        std::swap( start, end );
        angle = ANGLE_360 - angle;
    }

    double chord = ( start - end ).EuclideanNorm();
    double sinHalfAngle = ( angle / 2.0 ).Sin();

    // A zero arc angle has no defined center, so fall back to the chord midpoint
    if( sinHalfAngle == 0.0 )
        return VECTOR2D( ( start + end ) / 2.0 );

    // The center sits (chord/2) * cot(angle/2) off the chord; sqrt(r^2 - chord^2/4) cancels near 180 degrees
    double d = ( chord / 2.0 ) * ( angle / 2.0 ).Cos() / sinHalfAngle;

    if( !std::isfinite( d ) )
        d = 0.0;

    VECTOR2D vec2 = VECTOR2D(end - start).Resize( d );
    VECTOR2D vc = VECTOR2D(end - start).Resize( chord / 2 );

    RotatePoint( vec2, -ANGLE_90 );

    return VECTOR2D( start + vc + vec2 );
}


namespace
{
// No unique circumcircle exists if any two of the three points coincide
// Bbox below catches all three; pairwise checks below catch just one pair
constexpr double kClusterExtent = 5.0;

// A pair separated by more than integer rounding is a real, if small, arc.  This is not the
// cluster extent above: three points inside a 5 IU box are all noise, but two points 4 IU
// apart with a distant third still span a healthy triangle
constexpr double kCoincidentRadius        = 2.0;
constexpr double kCoincidentRadiusSquared = kCoincidentRadius * kCoincidentRadius;

// Radius agreement required of a snapped center, in IU
constexpr double kSnapRadiusTolerance = 1.0;

// Stand-in radius for collinear points, whose true center is at infinity
constexpr double kCollinearRadius = 1e17;

// Largest magnitude at which every integer is still a double
constexpr double kMaxExactInteger = 9007199254740992.0;


bool degenerateArcCenter( const VECTOR2D& aStart, const VECTOR2D& aMid, const VECTOR2D& aEnd,
                          VECTOR2D& aCenter )
{
    auto [minX, maxX] = std::minmax( { aStart.x, aMid.x, aEnd.x } );
    auto [minY, maxY] = std::minmax( { aStart.y, aMid.y, aEnd.y } );

    if( maxX - minX < kClusterExtent && maxY - minY < kClusterExtent )
    {
        aCenter = VECTOR2D( ( aStart.x + aMid.x + aEnd.x ) / 3.0, ( aStart.y + aMid.y + aEnd.y ) / 3.0 );
        return true;
    }

    auto coincident = []( const VECTOR2D& a, const VECTOR2D& b )
                      {
                          return ( a - b ).SquaredEuclideanNorm() < kCoincidentRadiusSquared;
                      };

    // Two distinct points fall back to the chord midpoint, same as the diameter-arc paths below
    if( coincident( aStart, aMid ) || coincident( aMid, aEnd ) )
    {
        aCenter = VECTOR2D( ( aStart.x + aEnd.x ) / 2.0, ( aStart.y + aEnd.y ) / 2.0 );
        return true;
    }

    if( coincident( aStart, aEnd ) )
    {
        aCenter = VECTOR2D( ( aStart.x + aMid.x ) / 2.0, ( aStart.y + aMid.y ) / 2.0 );
        return true;
    }

    return false;
}


VECTOR2D collinearArcCenter( const VECTOR2D& aStart, const VECTOR2D& aEnd )
{
    VECTOR2D chord = aEnd - aStart;
    VECTOR2D mid( ( aStart.x + aEnd.x ) / 2.0, ( aStart.y + aEnd.y ) / 2.0 );

    return mid + VECTOR2D( chord.y, -chord.x ).Resize( kCollinearRadius );
}


// Kahan's a*d - b*c, accurate to about one ulp despite the cancellation
double det2( double a, double b, double c, double d )
{
    double w = b * c;
    double e = std::fma( -b, c, w );
    double f = std::fma( a, d, -w );

    return f + e;
}


// Prefer a nice 100 nm or 10 nm center when it still lies on the circle to within a nanometer
VECTOR2D snapArcCenter( const VECTOR2D& aCenter, const VECTOR2D& aStart, const VECTOR2D& aMid,
                        const VECTOR2D& aEnd )
{
    if( !( std::abs( aCenter.x ) < kMaxExactInteger ) || !( std::abs( aCenter.y ) < kMaxExactInteger ) )
        return aCenter;

    auto radiiAgree = [&]( const VECTOR2D& aCandidate )
                      {
                          double rs = ( aCandidate - aStart ).EuclideanNorm();
                          double rm = ( aCandidate - aMid ).EuclideanNorm();
                          double re = ( aCandidate - aEnd ).EuclideanNorm();
                          auto [minR, maxR] = std::minmax( { rs, rm, re } );

                          return maxR - minR <= kSnapRadiusTolerance;
                      };

    for( double grid : { 100.0, 10.0 } )
    {
        VECTOR2D candidate( std::floor( aCenter.x / grid + 0.5 ) * grid,
                            std::floor( aCenter.y / grid + 0.5 ) * grid );

        if( radiiAgree( candidate ) )
            return candidate;
    }

    return aCenter;
}
} // namespace


const VECTOR2D CalcArcCenter( const VECTOR2D& aStart, const VECTOR2D& aMid, const VECTOR2D& aEnd )
{
    VECTOR2D center;

    if( degenerateArcCenter( aStart, aMid, aEnd, center ) )
        return center;

    // Work relative to start so the result cannot depend on where the arc sits
    double bx = aMid.x - aStart.x;
    double by = aMid.y - aStart.y;
    double cx = aEnd.x - aStart.x;
    double cy = aEnd.y - aStart.y;

    double b2 = std::fma( bx, bx, by * by );
    double c2 = std::fma( cx, cx, cy * cy );
    double d = 2.0 * det2( bx, by, cx, cy );

    if( d == 0.0 )
        return collinearArcCenter( aStart, aEnd );

    double ux = det2( b2, c2, by, cy ) / d;
    double uy = det2( c2, b2, cx, bx ) / d;

    return snapArcCenter( VECTOR2D( aStart.x + ux, aStart.y + uy ), aStart, aMid, aEnd );
}


const VECTOR2I CalcArcCenter( const VECTOR2I& aStart, const VECTOR2I& aMid, const VECTOR2I& aEnd )
{
    VECTOR2D dStart( static_cast<double>( aStart.x ), static_cast<double>( aStart.y ) );
    VECTOR2D dMid( static_cast<double>( aMid.x ), static_cast<double>( aMid.y ) );
    VECTOR2D dEnd( static_cast<double>( aEnd.x ), static_cast<double>( aEnd.y ) );
    VECTOR2D dCenter;

    if( !degenerateArcCenter( dStart, dMid, dEnd, dCenter ) )
    {
        // Deltas are exact in 64 bits and the numerators in 128, so only the final divisions round
        VECTOR2L b( int64_t( aMid.x ) - aStart.x, int64_t( aMid.y ) - aStart.y );
        VECTOR2L c( int64_t( aEnd.x ) - aStart.x, int64_t( aEnd.y ) - aStart.y );

        KI_INT128 b2 = KI_INT128( b.x ) * KI_INT128( b.x ) + KI_INT128( b.y ) * KI_INT128( b.y );
        KI_INT128 c2 = KI_INT128( c.x ) * KI_INT128( c.x ) + KI_INT128( c.y ) * KI_INT128( c.y );
        KI_INT128 d = CrossWide( b, c ) * KI_INT128( 2 );

        if( d == KI_INT128( 0 ) )
        {
            dCenter = collinearArcCenter( dStart, dEnd );
        }
        else
        {
            // Fold start into the numerator so the sum is rounded once
            double dd = ToDouble( d );
            double cx = ToDouble( KI_INT128( aStart.x ) * d + b2 * KI_INT128( c.y ) - c2 * KI_INT128( b.y ) ) / dd;
            double cy = ToDouble( KI_INT128( aStart.y ) * d + c2 * KI_INT128( b.x ) - b2 * KI_INT128( c.x ) ) / dd;

            dCenter = snapArcCenter( VECTOR2D( cx, cy ), dStart, dMid, dEnd );
        }
    }

    VECTOR2I iCenter;

    iCenter.x = KiROUND( std::clamp( dCenter.x,
                                    double( std::numeric_limits<int>::min() + 100 ),
                                    double( std::numeric_limits<int>::max() - 100 ) ) );

    iCenter.y = KiROUND( std::clamp( dCenter.y,
                                    double( std::numeric_limits<int>::min() + 100 ),
                                    double( std::numeric_limits<int>::max() - 100 ) ) );

    return iCenter;
}
