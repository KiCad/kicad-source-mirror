/**
 * @file trigo.cpp
 * @brief Trigonometric and geometric basic functions.
 */

#include <fctsys.h>
#include <macros.h>
#include <trigo.h>
#include <common.h>
#include <math_for_graphics.h>

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
bool TestSegmentHit( wxPoint aRefPoint, wxPoint aStart, wxPoint aEnd, int aDist )
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
            EXCHG( aStart.y, aEnd.y );

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
            EXCHG( aStart.x, aEnd.x );

        if( aRefPoint.x <= aEnd.x && aRefPoint.x >= aStart.x )
            return true;

        // there is a special case: x,y near an end point (distance < dist )
        // the distance should be carefully calculated
        if( (aStart.x - aRefPoint.x) <= aDist )
        {
            double dd = square( aRefPoint.x - aStart.x) +
                        square( aRefPoint.y - aStart.y );
            if( dd <= square( aDist ) )
                return true;
        }

        if( (aRefPoint.x - aEnd.x) <= aDist )
        {
            double dd = square(aRefPoint.x - aEnd.x) +
                        square( aRefPoint.y - aEnd.y);
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


int ArcTangente( int dy, int dx )
{
    double fangle;

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

    fangle = atan2( (double) dy, (double) dx ) / M_PI * 1800;
    return KiROUND( fangle );
}


void RotatePoint( int* pX, int* pY, double angle )
{
    int tmp;

    while( angle < 0 )
        angle += 3600;

    while( angle >= 3600 )
        angle -= 3600;

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
        double fangle = DEG2RAD( angle / 10.0 );
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

    while( angle < 0 )
        angle += 3600;

    while( angle >= 3600 )
        angle -= 3600;

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
        double fangle = DEG2RAD( angle / 10.0 );
        double sinus = sin( fangle );
        double cosinus = cos( fangle );

        double fpx = (*pY * sinus ) + (*pX * cosinus );
        double fpy = (*pY * cosinus ) - (*pX * sinus );
        *pX = fpx;
        *pY = fpy;
    }
}


double EuclideanNorm( wxPoint vector )
{
    return hypot( (double) vector.x, (double) vector.y );
}

double DistanceLinePoint( wxPoint linePointA, wxPoint linePointB, wxPoint referencePoint )
{
    return fabs( (double) ( (linePointB.x - linePointA.x) * (linePointA.y - referencePoint.y) -
                 (linePointA.x - referencePoint.x ) * (linePointB.y - linePointA.y) )
                  / EuclideanNorm( linePointB - linePointA ) );
}


bool HitTestPoints( wxPoint pointA, wxPoint pointB, double threshold )
{
    wxPoint vectorAB = pointB - pointA;
    double  distance = EuclideanNorm( vectorAB );

    return distance < threshold;
}


double CrossProduct( wxPoint vectorA, wxPoint vectorB )
{
    return (double)vectorA.x * vectorB.y - (double)vectorA.y * vectorB.x;
}


double GetLineLength( const wxPoint& aPointA, const wxPoint& aPointB )
{
    return hypot( (double) aPointA.x - (double) aPointB.x,
                  (double) aPointA.y - (double) aPointB.y );
}
