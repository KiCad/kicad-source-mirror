/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2014 Jean-Pierre Charras, jp.charras at wanadoo.fr
 * Copyright (C) 2014 KiCad Developers, see AUTHORS.txt for contributors.
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

#include <fctsys.h>
#include <macros.h>
#include <trigo.h>
#include <common.h>
#include <math_for_graphics.h>

// Returns true if the point P is on the segment S.
// faster than TestSegmentHit() because P should be exactly on S
// therefore works fine only for H, V and 45 deg segm (suitable for wires in eeschema)
bool IsPointOnSegment( const wxPoint& aSegStart, const wxPoint& aSegEnd,
                       const wxPoint& aTestPoint )
{
    wxPoint vectSeg   = aSegEnd - aSegStart;    // Vector from S1 to S2
    wxPoint vectPoint = aTestPoint - aSegStart; // Vector from S1 to P

    // Use long long here to avoid overflow in calculations
    if( (long long) vectSeg.x * vectPoint.y - (long long) vectSeg.y * vectPoint.x )
        return false;        /* Cross product non-zero, vectors not parallel */

    if( ( (long long) vectSeg.x * vectPoint.x + (long long) vectSeg.y * vectPoint.y ) <
        ( (long long) vectPoint.x * vectPoint.x + (long long) vectPoint.y * vectPoint.y ) )
        return false;          /* Point not on segment */

    return true;
}


// Returns true if the segment 1 intersectd the segment 2.
bool SegmentIntersectsSegment( const wxPoint &a_p1_l1, const wxPoint &a_p2_l1,
                               const wxPoint &a_p1_l2, const wxPoint &a_p2_l2,
                               wxPoint* aIntersectionPoint )
{

    //We are forced to use 64bit ints because the internal units can oveflow 32bit ints when
    // multiplied with each other, the alternative would be to scale the units down (i.e. divide
    // by a fixed number).
    long long dX_a, dY_a, dX_b, dY_b, dX_ab, dY_ab;
    long long num_a, num_b, den;

    //Test for intersection within the bounds of both line segments using line equations of the
    // form:
    // x_k(u_k) = u_k * dX_k + x_k(0)
    // y_k(u_k) = u_k * dY_k + y_k(0)
    // with  0 <= u_k <= 1 and k = [ a, b ]

    dX_a  = a_p2_l1.x - a_p1_l1.x;
    dY_a  = a_p2_l1.y - a_p1_l1.y;
    dX_b  = a_p2_l2.x - a_p1_l2.x;
    dY_b  = a_p2_l2.y - a_p1_l2.y;
    dX_ab = a_p1_l2.x - a_p1_l1.x;
    dY_ab = a_p1_l2.y - a_p1_l1.y;

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


bool TestSegmentHit( const wxPoint &aRefPoint, wxPoint aStart, wxPoint aEnd, int aDist )
{
    int xmin = aStart.x;
    int xmax = aEnd.x;
    int ymin = aStart.y;
    int ymax = aEnd.y;
    wxPoint delta = aStart - aRefPoint;

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

    // Special case for a segment with start == end (equal to a circle)
    if ( aStart == aEnd )
    {
        long double length_square = (long double) delta.x * delta.x + (long double) delta.y * delta.y;
        long double dist_square = (long double) aDist * aDist;

        return ( length_square <= dist_square );
    }

    wxPoint len = aEnd - aStart;
    // Precision note here:
    // These are 32-bit integers, so squaring requires 64 bits to represent
    // exactly.  64-bit Doubles have only 52 bits in the mantissa, so we start to lose
    // precision at 2^53, which corresponds to ~ ±1nm @ 9.5cm, 2nm at 90cm, etc...
    // Long doubles avoid this ambiguity as well as the more expensive denormal double calc
    // Long doubles usually (sometimes more if SIMD) have at least 64 bits in the mantissa
    long double length_square = (long double) len.x * len.x + (long double) len.y * len.y;
    long double cross = std::abs( (long double) len.x * delta.y - (long double) len.y * delta.x );
    long double dist_square = (long double) aDist * aDist;

    // The perpendicular distance to a line is the vector magnitude of the line from
    // a test point to the test line.  That is the 2d determinant.  Because we handled
    // the zero length case above, so we are guaranteed a unique solution.

    return ( ( length_square >= cross && dist_square >= cross ) ||
             ( length_square * dist_square >= cross * cross ) );
}


double ArcTangente( int dy, int dx )
{

    /* gcc is surprisingly smart in optimizing these conditions in
       a tree! */

    if( dx == 0 && dy == 0 )
        return 0;

    if( dy == 0 )
    {
        if( dx >= 0 )
            return 0;
        else
            return -1800;
    }

    if( dx == 0 )
    {
        if( dy >= 0 )
            return 900;
        else
            return -900;
    }

    if( dx == dy )
    {
        if( dx >= 0 )
            return 450;
        else
            return -1800 + 450;
    }

    if( dx == -dy )
    {
        if( dx >= 0 )
            return -450;
        else
            return 1800 - 450;
    }

    // Of course dy and dx are treated as double
    return RAD2DECIDEG( atan2( (double) dy, (double) dx ) );
}


void RotatePoint( int* pX, int* pY, double angle )
{
    int tmp;

    NORMALIZE_ANGLE_POS( angle );

    // Cheap and dirty optimizations for 0, 90, 180, and 270 degrees.
    if( angle == 0 )
        return;

    if( angle == 900 )          /* sin = 1, cos = 0 */
    {
        tmp = *pX;
        *pX = *pY;
        *pY = -tmp;
    }
    else if( angle == 1800 )    /* sin = 0, cos = -1 */
    {
        *pX = -*pX;
        *pY = -*pY;
    }
    else if( angle == 2700 )    /* sin = -1, cos = 0 */
    {
        tmp = *pX;
        *pX = -*pY;
        *pY = tmp;
    }
    else
    {
        double fangle = DECIDEG2RAD( angle );
        double sinus = sin( fangle );
        double cosinus = cos( fangle );
        double fpx = (*pY * sinus ) + (*pX * cosinus );
        double fpy = (*pY * cosinus ) - (*pX * sinus );
        *pX = KiROUND( fpx );
        *pY = KiROUND( fpy );
    }
}


void RotatePoint( int* pX, int* pY, int cx, int cy, double angle )
{
    int ox, oy;

    ox = *pX - cx;
    oy = *pY - cy;

    RotatePoint( &ox, &oy, angle );

    *pX = ox + cx;
    *pY = oy + cy;
}


void RotatePoint( wxPoint* point, const wxPoint& centre, double angle )
{
    int ox, oy;

    ox = point->x - centre.x;
    oy = point->y - centre.y;

    RotatePoint( &ox, &oy, angle );
    point->x = ox + centre.x;
    point->y = oy + centre.y;
}

void RotatePoint( VECTOR2I& point, const VECTOR2I& centre, double angle )
{
    wxPoint c( centre.x, centre.y );
    wxPoint p( point.x, point.y );

    RotatePoint(&p, c, angle);

    point.x = p.x;
    point.y = p.y;
}


void RotatePoint( double* pX, double* pY, double cx, double cy, double angle )
{
    double ox, oy;

    ox = *pX - cx;
    oy = *pY - cy;

    RotatePoint( &ox, &oy, angle );

    *pX = ox + cx;
    *pY = oy + cy;
}


void RotatePoint( double* pX, double* pY, double angle )
{
    double tmp;

    NORMALIZE_ANGLE_POS( angle );

    // Cheap and dirty optimizations for 0, 90, 180, and 270 degrees.
    if( angle == 0 )
        return;

    if( angle == 900 )          /* sin = 1, cos = 0 */
    {
        tmp = *pX;
        *pX = *pY;
        *pY = -tmp;
    }
    else if( angle == 1800 )    /* sin = 0, cos = -1 */
    {
        *pX = -*pX;
        *pY = -*pY;
    }
    else if( angle == 2700 )    /* sin = -1, cos = 0 */
    {
        tmp = *pX;
        *pX = -*pY;
        *pY = tmp;
    }
    else
    {
        double fangle = DECIDEG2RAD( angle );
        double sinus = sin( fangle );
        double cosinus = cos( fangle );

        double fpx = (*pY * sinus ) + (*pX * cosinus );
        double fpy = (*pY * cosinus ) - (*pX * sinus );
        *pX = fpx;
        *pY = fpy;
    }
}


const VECTOR2I GetArcCenter( const VECTOR2I& aStart, const VECTOR2I& aMid, const VECTOR2I& aEnd )
{
    VECTOR2I center;
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
        center.x = KiROUND( ( aStart.x + aEnd.x ) / 2.0 );
        center.y = KiROUND( ( aStart.y + aEnd.y ) / 2.0 );
        return center;
    }

    // Prevent div=0 errors
    if( xDelta_21 == 0.0 )
        xDelta_21 = std::numeric_limits<double>::epsilon();

    if( xDelta_32 == 0.0 )
        xDelta_32 = -std::numeric_limits<double>::epsilon();

    double aSlope = yDelta_21 / xDelta_21;
    double bSlope = yDelta_32 / xDelta_32;

    // If the points are colinear, the center is at infinity, so offset
    // the slope by a minimal amount
    // Warning: This will induce a small error in the center location
    if( yDelta_32 * xDelta_21 == yDelta_21 * xDelta_32 )
    {
        aSlope += std::numeric_limits<double>::epsilon();
        bSlope -= std::numeric_limits<double>::epsilon();
    }

    if( aSlope == 0.0 )
        aSlope = std::numeric_limits<double>::epsilon();

    if( bSlope == 0.0 )
        bSlope = -std::numeric_limits<double>::epsilon();


    double result = ( aSlope * bSlope * ( aStart.y - aEnd.y ) +
                      bSlope * ( aStart.x + aMid.x ) -
                      aSlope * ( aMid.x + aEnd.x ) ) / ( 2 * ( bSlope - aSlope ) );

    center.x = KiROUND( Clamp<double>( double( std::numeric_limits<int>::min() / 2 ),
                                       result,
                                       double( std::numeric_limits<int>::max() / 2 ) ) );

    result = ( ( ( aStart.x + aMid.x ) / 2.0 - center.x ) / aSlope +
                 ( aStart.y + aMid.y ) / 2.0 );

    center.y = KiROUND( Clamp<double>( double( std::numeric_limits<int>::min() / 2 ),
                                       result,
                                       double( std::numeric_limits<int>::max() / 2 ) ) );

    return center;
}
