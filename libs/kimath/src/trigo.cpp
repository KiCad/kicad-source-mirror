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
 * along with this program; if not, you may find one here:
 * http://www.gnu.org/licenses/old-licenses/gpl-2.0.html
 * or you may search the http://www.gnu.org website for the version 2 license,
 * or you may write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
 */

/**
 * @file trigo.cpp
 * @brief Trigonometric and geometric basic functions.
 */

#include <algorithm>        // for std::clamp
#include <limits>           // for numeric_limits
#include <cstdlib>         // for abs
#include <type_traits>      // for swap

#include <geometry/seg.h>
#include <math/util.h>
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
    double r = ( chord / 2.0 ) / ( angle / 2.0 ).Sin();
    double d_squared = r * r - chord*  chord / 4.0;
    double d = 0.0;

    if( d_squared > 0.0 )
        d = sqrt( d_squared );

    VECTOR2D vec2 = VECTOR2D(end - start).Resize( d );
    VECTOR2D vc = VECTOR2D(end - start).Resize( chord / 2 );

    RotatePoint( vec2, -ANGLE_90 );

    return VECTOR2D( start + vc + vec2 );
}


const VECTOR2D CalcArcCenter( const VECTOR2D& aStart, const VECTOR2D& aMid, const VECTOR2D& aEnd )
{
    VECTOR2D center;

    double yDelta_21 = aMid.y - aStart.y;
    double xDelta_21 = aMid.x - aStart.x;
    double yDelta_32 = aEnd.y - aMid.y;
    double xDelta_32 = aEnd.x - aMid.x;

    // This is a special case for aMid as the half-way point when aSlope = 0 and bSlope = inf
    // or the other way around.  In that case, the center lies in a straight line between
    // aStart and aEnd
    if( ( ( xDelta_21 == 0.0 ) && ( yDelta_32 == 0.0 ) ) ||
        ( ( yDelta_21 == 0.0 ) && ( xDelta_32 == 0.0 ) ) )
    {
        center.x = ( aStart.x + aEnd.x ) / 2.0;
        center.y = ( aStart.y + aEnd.y ) / 2.0 ;
        return center;
    }

    // Prevent div-by-0 errors
    if( xDelta_21 == 0.0 )
        xDelta_21 = std::numeric_limits<double>::epsilon();

    if( xDelta_32 == 0.0 )
        xDelta_32 = -std::numeric_limits<double>::epsilon();

    double aSlope = yDelta_21 / xDelta_21;
    double bSlope = yDelta_32 / xDelta_32;

    double daSlope = aSlope * VECTOR2D( 0.5 / yDelta_21, 0.5 / xDelta_21 ).EuclideanNorm();
    double dbSlope = bSlope * VECTOR2D( 0.5 / yDelta_32, 0.5 / xDelta_32 ).EuclideanNorm();

    if( aSlope == bSlope )
    {
        if( aStart == aEnd )
        {
            // This is a special case for a 360 degrees arc.  In this case, the center is
            // halfway between the midpoint and either end point.
            center.x = ( aStart.x + aMid.x ) / 2.0;
            center.y = ( aStart.y + aMid.y ) / 2.0 ;
            return center;
        }
        else
        {
            // If the points are colinear, the center is at infinity, so offset
            // the slope by a minimal amount
            // Warning: This will induce a small error in the center location
            aSlope += std::numeric_limits<double>::epsilon();
            bSlope -= std::numeric_limits<double>::epsilon();
        }
    }
#ifdef USE_ALTERNATE_CENTER_ALGO
    // We can call ArcCenterFrom3Points from here because special cases are filtered.
    CircleCenterFrom3Points( aStart, aMid, aEnd, &center );
    return center;
#endif

    // Prevent divide by zero error
    // a small value is used. std::numeric_limits<double>::epsilon() is too small and
    // generate false results
    if( aSlope == 0.0 )
        aSlope = 1e-10;
    if( bSlope == 0.0 )
        bSlope = 1e-10;

    // What follows is the calculation of the center using the slope of the two lines as well as
    // the propagated error that occurs when rounding to the nearest nanometer.  The error can be
    // ±0.5 units but can add up to multiple nanometers after the full calculation is performed.
    // All variables starting with `d` are the delta of that variable.  This is approximately equal
    // to the standard deviation.
    // We ignore the possible covariance between variables.  We also truncate our series expansion
    // at the first term.  These are reasonable assumptions as the worst-case scenario is that we
    // underestimate the potential uncertainty, which would potentially put us back at the status
    // quo.
    double abSlopeStartEndY = aSlope * bSlope * ( aStart.y - aEnd.y );
    double dabSlopeStartEndY = abSlopeStartEndY *
                               std::sqrt( ( daSlope / aSlope * daSlope / aSlope )
                                        + ( dbSlope / bSlope * dbSlope / bSlope )
                                        + ( M_SQRT1_2 / ( aStart.y - aEnd.y )
                                          * M_SQRT1_2 / ( aStart.y - aEnd.y ) ) );

    double bSlopeStartMidX = bSlope * ( aStart.x + aMid.x );
    double dbSlopeStartMidX = bSlopeStartMidX * std::sqrt( ( dbSlope / bSlope * dbSlope / bSlope )
                                                         + ( M_SQRT1_2 / ( aStart.x + aMid.x )
                                                           * M_SQRT1_2 / ( aStart.x + aMid.x ) ) );

    double aSlopeMidEndX = aSlope * ( aMid.x + aEnd.x );
    double daSlopeMidEndX = aSlopeMidEndX * std::sqrt( ( daSlope / aSlope * daSlope / aSlope )
                                                     + ( M_SQRT1_2 / ( aMid.x + aEnd.x )
                                                       * M_SQRT1_2 / ( aMid.x + aEnd.x ) ) );

    double twiceBASlopeDiff = 2 * ( bSlope - aSlope );
    double dtwiceBASlopeDiff = 2 * std::sqrt( dbSlope * dbSlope + daSlope * daSlope );

    double centerNumeratorX = abSlopeStartEndY + bSlopeStartMidX - aSlopeMidEndX;
    double dCenterNumeratorX = std::sqrt( dabSlopeStartEndY * dabSlopeStartEndY
                                       + dbSlopeStartMidX * dbSlopeStartMidX
                                       + daSlopeMidEndX * daSlopeMidEndX );

    double centerX = ( abSlopeStartEndY + bSlopeStartMidX - aSlopeMidEndX ) / twiceBASlopeDiff;
    double dCenterX = centerX * std::sqrt( ( dCenterNumeratorX / centerNumeratorX *
                                             dCenterNumeratorX / centerNumeratorX )
                                         + ( dtwiceBASlopeDiff / twiceBASlopeDiff *
                                             dtwiceBASlopeDiff / twiceBASlopeDiff ) );


    double centerNumeratorY = ( ( aStart.x + aMid.x ) / 2.0 - centerX );
    double dCenterNumeratorY = std::sqrt( 1.0 / 8.0 + dCenterX * dCenterX );

    double centerFirstTerm = centerNumeratorY / aSlope;
    double dcenterFirstTermY = centerFirstTerm * std::sqrt(
                                          ( dCenterNumeratorY/ centerNumeratorY *
                                            dCenterNumeratorY / centerNumeratorY )
                                        + ( daSlope / aSlope * daSlope / aSlope ) );

    double centerY = centerFirstTerm + ( aStart.y + aMid.y ) / 2.0;
    double dCenterY = std::sqrt( dcenterFirstTermY * dcenterFirstTermY + 1.0 / 8.0 );

    double rounded100CenterX = std::floor( ( centerX + 50.0 ) / 100.0 ) * 100.0;
    double rounded100CenterY = std::floor( ( centerY + 50.0 ) / 100.0 ) * 100.0;
    double rounded10CenterX = std::floor( ( centerX + 5.0 ) / 10.0 ) * 10.0;
    double rounded10CenterY = std::floor( ( centerY + 5.0 ) / 10.0 ) * 10.0;

    // The last step is to find the nice, round numbers near our baseline estimate and see if
    // they are within our uncertainty range.  If they are, then we use this round value as the
    // true value.  This is justified because ALL values within the uncertainty range are equally
    // true.  Using a round number will make sure that we are on a multiple of 1mil or 100nm
    // when calculating centers.
    if( std::abs( rounded100CenterX - centerX ) < dCenterX &&
        std::abs( rounded100CenterY - centerY ) < dCenterY )
    {
        center.x = rounded100CenterX;
        center.y = rounded100CenterY;
    }
    else if( std::abs( rounded10CenterX - centerX ) < dCenterX &&
             std::abs( rounded10CenterY - centerY ) < dCenterY )
    {
        center.x = rounded10CenterX;
        center.y = rounded10CenterY;
    }
    else
    {
        center.x = centerX;
        center.y = centerY;
    }


    return center;
}


const VECTOR2I CalcArcCenter( const VECTOR2I& aStart, const VECTOR2I& aMid, const VECTOR2I& aEnd )
{
    VECTOR2D dStart( static_cast<double>( aStart.x ), static_cast<double>( aStart.y ) );
    VECTOR2D dMid( static_cast<double>( aMid.x ), static_cast<double>( aMid.y ) );
    VECTOR2D dEnd( static_cast<double>( aEnd.x ), static_cast<double>( aEnd.y ) );
    VECTOR2D dCenter = CalcArcCenter( dStart, dMid, dEnd );

    VECTOR2I iCenter;

    iCenter.x = KiROUND( std::clamp( dCenter.x,
                                    double( std::numeric_limits<int>::min() + 100 ),
                                    double( std::numeric_limits<int>::max() - 100 ) ) );

    iCenter.y = KiROUND( std::clamp( dCenter.y,
                                    double( std::numeric_limits<int>::min() + 100 ),
                                    double( std::numeric_limits<int>::max() - 100 ) ) );

    return iCenter;
}