/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2014 Jean-Pierre Charras, jp.charras at wanadoo.fr
 * Copyright (C) 2014 KiCad Developers, see CHANGELOG.TXT for contributors.
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
                               const wxPoint &a_p1_l2, const wxPoint &a_p2_l2 )
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

    //We wont calculate directly the u_k of the intersection point to avoid floating point
    // division but they could be calculated with:
    // u_a = (float) num_a / (float) den;
    // u_b = (float) num_b / (float) den;

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


/* Function TestSegmentHit
 * test for hit on line segment
 * i.e. a reference point is within a given distance from segment
 * aRefPoint = reference point to test
 * aStart, aEnd are coordinates of end points segment
 * aDist = maximum distance for hit
 * Note: for calculation time reasons, the distance between the ref point
 * and the segment is not always exactly calculated
 * (we only know if the actual dist is < aDist, not exactly know this dist.
 * Because many times we have horizontal or vertical segments,
 * a special calcultaion is made for them
 * Note: sometimes we need to calculate the distande between 2 points
 * A square root should be calculated.
 * However, because we just compare 2 distnaces, to avoid calculating square root,
 * the square of distances are compared.
*/
static inline double square( int x )    // helper function to calculate x*x
{
    return (double) x * x;
}
bool TestSegmentHit( const wxPoint &aRefPoint, wxPoint aStart,
                     wxPoint aEnd, int aDist )
{
    // test for vertical or horizontal segment
    if( aEnd.x == aStart.x )
    {
        // vertical segment
        int ll = abs( aRefPoint.x - aStart.x );

        if( ll > aDist )
            return false;

        // To have only one case to examine, ensure aEnd.y > aStart.y
        if( aEnd.y < aStart.y )
            std::swap( aStart.y, aEnd.y );

        if( aRefPoint.y <= aEnd.y && aRefPoint.y >= aStart.y )
            return true;

        // there is a special case: x,y near an end point (distance < dist )
        // the distance should be carefully calculated
        if( (aStart.y - aRefPoint.y) < aDist )
        {
            double dd = square( aRefPoint.x - aStart.x) +
                 square( aRefPoint.y - aStart.y );
            if( dd <= square( aDist ) )
                return true;
        }

        if( (aRefPoint.y - aEnd.y) < aDist )
        {
            double dd = square( aRefPoint.x - aEnd.x ) +
                 square( aRefPoint.y - aEnd.y );
            if( dd <= square( aDist ) )
                return true;
        }
    }
    else if( aEnd.y == aStart.y )
    {
        // horizontal segment
        int ll = abs( aRefPoint.y - aStart.y );

        if( ll > aDist )
            return false;

        // To have only one case to examine, ensure xf > xi
        if( aEnd.x < aStart.x )
            std::swap( aStart.x, aEnd.x );

        if( aRefPoint.x <= aEnd.x && aRefPoint.x >= aStart.x )
            return true;

        // there is a special case: x,y near an end point (distance < dist )
        // the distance should be carefully calculated
        if( (aStart.x - aRefPoint.x) <= aDist )
        {
            double dd = square( aRefPoint.x - aStart.x ) +
                        square( aRefPoint.y - aStart.y );
            if( dd <= square( aDist ) )
                return true;
        }

        if( (aRefPoint.x - aEnd.x) <= aDist )
        {
            double dd = square( aRefPoint.x - aEnd.x ) +
                        square( aRefPoint.y - aEnd.y );
            if( dd <= square( aDist ) )
                return true;
        }
    }
    else
    {
        // oblique segment:
        // First, we need to calculate the distance between the point
        // and the line defined by aStart and aEnd
        // this dist should be < dist
        //
        // find a,slope such that aStart and aEnd lie on y = a + slope*x
        double  slope   = (double) (aEnd.y - aStart.y) / (aEnd.x - aStart.x);
        double  a   = (double) aStart.y - slope * aStart.x;
        // find c,orthoslope such that (x,y) lies on y = c + orthoslope*x,
        // where orthoslope=(-1/slope)
        // to calculate xp, yp = near point from aRefPoint
        // which is on the line defined by aStart, aEnd
        double  orthoslope   = -1.0 / slope;
        double  c   = (double) aRefPoint.y - orthoslope * aRefPoint.x;
        // find nearest point to (x,y) on line defined by aStart, aEnd
        double  xp  = (a - c) / (orthoslope - slope);
        double  yp  = a + slope * xp;
        // find distance to line, in fact the square of dist,
        // because we just know if it is > or < aDist
        double dd = square( aRefPoint.x - xp ) + square( aRefPoint.y - yp );
        double dist = square( aDist );

        if( dd > dist )    // this reference point is not a good candiadte.
            return false;

        // dd is < dist, therefore we should make a fine test
        if( fabs( slope ) > 0.7 )
        {
            // line segment more vertical than horizontal
            if( (aEnd.y > aStart.y && yp <= aEnd.y && yp >= aStart.y) ||
                (aEnd.y < aStart.y && yp >= aEnd.y && yp <= aStart.y) )
                return true;
        }
        else
        {
            // line segment more horizontal than vertical
            if( (aEnd.x > aStart.x && xp <= aEnd.x && xp >= aStart.x) ||
                (aEnd.x < aStart.x && xp >= aEnd.x && xp <= aStart.x) )
                return true;
        }

        // Here, the test point is still a good candidate,
        // however it is not "between" the end points of the segment.
        // It is "outside" the segment, but it could be near a segment end point
        // Therefore, we test the dist from the test point to each segment end point
        dd = square( aRefPoint.x - aEnd.x ) + square( aRefPoint.y - aEnd.y );
        if( dd <= dist )
            return true;
        dd = square( aRefPoint.x - aStart.x ) + square( aRefPoint.y - aStart.y );
        if( dd <= dist )
            return true;
    }

    return false;    // no hit
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
    return RAD2DECIDEG( atan2( dy, dx ) );
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
