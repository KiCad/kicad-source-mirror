/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2014 Jean-Pierre Charras, jp.charras at wanadoo.fr
 * Copyright (C) 2014-2021 KiCad Developers, see AUTHORS.txt for contributors.
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

#include <limits>           // for numeric_limits
#include <stdlib.h>         // for abs
#include <type_traits>      // for swap

#include <geometry/seg.h>
#include <math/util.h>
#include <math/vector2d.h>  // for VECTOR2I
#include <trigo.h>

// Returns true if the point P is on the segment S.
// faster than TestSegmentHit() because P should be exactly on S
// therefore works fine only for H, V and 45 deg segm (suitable for wires in eeschema)
bool IsPointOnSegment( const VECTOR2I& aSegStart, const VECTOR2I& aSegEnd,
                       const VECTOR2I& aTestPoint )
{
    VECTOR2I vectSeg = aSegEnd - aSegStart;      // Vector from S1 to S2
    VECTOR2I vectPoint = aTestPoint - aSegStart; // Vector from S1 to P

    // Use long long here to avoid overflow in calculations
    if( (long long) vectSeg.x * vectPoint.y - (long long) vectSeg.y * vectPoint.x )
        return false;        /* Cross product non-zero, vectors not parallel */

    if( ( (long long) vectSeg.x * vectPoint.x + (long long) vectSeg.y * vectPoint.y ) <
        ( (long long) vectPoint.x * vectPoint.x + (long long) vectPoint.y * vectPoint.y ) )
        return false;          /* Point not on segment */

    return true;
}


// Returns true if the segment 1 intersected the segment 2.
bool SegmentIntersectsSegment( const VECTOR2I& a_p1_l1, const VECTOR2I& a_p2_l1,
                               const VECTOR2I& a_p1_l2, const VECTOR2I& a_p2_l2,
                               VECTOR2I* aIntersectionPoint )
{

    //We are forced to use 64bit ints because the internal units can overflow 32bit ints when
    // multiplied with each other, the alternative would be to scale the units down (i.e. divide
    // by a fixed number).
    int64_t dX_a, dY_a, dX_b, dY_b, dX_ab, dY_ab;
    int64_t num_a, num_b, den;

    //Test for intersection within the bounds of both line segments using line equations of the
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

    //Check if lines are parallel
    if( den == 0 )
        return false;

    num_a = dY_ab * dX_b - dY_b * dX_ab;
    num_b = dY_ab * dX_a - dY_a * dX_ab;

    // Only compute the intersection point if requested
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

    //Test sign( u_a ) and return false if negative
    if( num_a < 0 )
        return false;

    //Test sign( u_b ) and return false if negative
    if( num_b < 0 )
        return false;

    //Test to ensure (u_a <= 1)
    if( num_a > den )
        return false;

    //Test to ensure (u_b <= 1)
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

    // First, check if we are outside of the bounding box
    if( ( ymin - aRefPoint.y > aDist ) || ( aRefPoint.y - ymax > aDist ) )
        return false;

    if( ( xmin - aRefPoint.x > aDist ) || ( aRefPoint.x - xmax > aDist ) )
        return false;

    // Next, eliminate easy cases
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


void RotatePoint( wxPoint* point, const wxPoint& centre, const EDA_ANGLE& angle )
{
    int ox, oy;

    ox = point->x - centre.x;
    oy = point->y - centre.y;

    RotatePoint( &ox, &oy, angle );

    point->x = ox + centre.x;
    point->y = oy + centre.y;
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
    VECTOR2I  start = aStart;
    VECTOR2I  end = aEnd;

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

    VECTOR2D vec = end - start;
    vec = vec.Resize( r );
    RotatePoint( vec, -( ANGLE_180 - angle ) / 2 );

    return VECTOR2D( start + vec );
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

    // Prevent div=0 errors
    if( xDelta_21 == 0.0 )
        xDelta_21 = std::numeric_limits<double>::epsilon();

    if( xDelta_32 == 0.0 )
        xDelta_32 = -std::numeric_limits<double>::epsilon();

    double aSlope = yDelta_21 / xDelta_21;
    double bSlope = yDelta_32 / xDelta_32;

    if( aSlope == bSlope )
    {
        if( aStart == aEnd )
        {
            // This is a special case for a 360 degrees arc.  In this case, the center is halfway between
            // the midpoint and either end point
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

    // Prevent divide by zero error
    if( aSlope == 0.0 )
        aSlope = std::numeric_limits<double>::epsilon();

    center.x = ( aSlope * bSlope * ( aStart.y - aEnd.y ) +
                 bSlope * ( aStart.x + aMid.x ) -
                 aSlope * ( aMid.x + aEnd.x ) ) / ( 2 * ( bSlope - aSlope ) );

    center.y = ( ( ( aStart.x + aMid.x ) / 2.0 - center.x ) / aSlope +
                 ( aStart.y + aMid.y ) / 2.0 );

    return center;
}


const VECTOR2I CalcArcCenter( const VECTOR2I& aStart, const VECTOR2I& aMid, const VECTOR2I& aEnd )
{
    VECTOR2D dStart( static_cast<double>( aStart.x ), static_cast<double>( aStart.y ) );
    VECTOR2D dMid( static_cast<double>( aMid.x ), static_cast<double>( aMid.y ) );
    VECTOR2D dEnd( static_cast<double>( aEnd.x ), static_cast<double>( aEnd.y ) );
    VECTOR2D dCenter = CalcArcCenter( dStart, dMid, dEnd );

    VECTOR2I iCenter;

    iCenter.x = KiROUND( Clamp<double>( double( std::numeric_limits<int>::min() / 2.0 ),
                                        dCenter.x,
                                        double( std::numeric_limits<int>::max() / 2.0 ) ) );

    iCenter.y = KiROUND( Clamp<double>( double( std::numeric_limits<int>::min() / 2.0 ),
                                        dCenter.y,
                                        double( std::numeric_limits<int>::max() / 2.0 ) ) );

    return iCenter;
}


