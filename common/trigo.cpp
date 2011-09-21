/**
 * @file trigo.cpp
 * @brief Trigonometric functions.
 */

#include "fctsys.h"
#include "macros.h"
#include "trigo.h"


bool TestSegmentHit( wxPoint aRefPoint, wxPoint aStart, wxPoint aEnd, int aDist )
{
    // make coordinates relatives to aStart:
    aEnd -= aStart;
    aRefPoint -= aStart;
    return DistanceTest( aDist, aEnd.x, aEnd.y, aRefPoint.x, aRefPoint.y );
}


bool DistanceTest( int seuil, int dx, int dy, int spot_cX, int spot_cY )
{
    /*  We can have 4 cases::
     *      horizontal segment
     *      vertical segment
     *      45 degrees segment
     *      other slopes
     */
    int cXrot, cYrot, segX, segY;
    int pointX, pointY;

    segX   = dx;
    segY   = dy;
    pointX = spot_cX;
    pointY = spot_cY;

    /* Recalculating coord for the segment is in 1st quadrant (coord >= 0) */
    if( segX < 0 )   /* set > 0 by symmetry about the Y axis */
    {
        segX   = -segX;
        pointX = -pointX;
    }

    if( segY < 0 )   /* set > 0 by symmetry about the X axis */
    {
        segY   = -segY;
        pointY = -pointY;
    }


    if( segY == 0 ) /* horizontal */
    {
        if( abs( pointY ) <= seuil )
        {
            if( ( pointX >= 0 ) && ( pointX <= segX ) )
                return 1;

            if( ( pointX < 0 ) && ( pointX >= -seuil ) )
            {
                if( ( ( pointX * pointX ) + ( pointY * pointY ) ) <= ( seuil * seuil ) )
                    return true;
            }
            if( ( pointX > segX ) && ( pointX <= ( segX + seuil ) ) )
            {
                if( ( ( ( pointX - segX ) * ( pointX - segX ) )
                     + ( pointY * pointY ) ) <= ( seuil * seuil ) )
                    return true;
            }
        }
    }
    else if( segX == 0 ) /* vertical */
    {
        if( abs( pointX ) <= seuil )
        {
            if( ( pointY >= 0 ) && ( pointY <= segY ) )
                return true;

            if( ( pointY < 0 ) && ( pointY >= -seuil ) )
            {
                if( ( ( pointY * pointY ) + ( pointX * pointX ) ) <= ( seuil * seuil ) )
                    return true;
            }

            if( ( pointY > segY ) && ( pointY <= ( segY + seuil ) ) )
            {
                if( ( ( ( pointY - segY ) * ( pointY - segY ) )
                     + ( pointX * pointX ) ) <= ( seuil * seuil ) )
                    return true;
            }
        }
    }
    else if( segX == segY )    /* 45 degrees */
    {
        /* Rotate axes of 45 degrees. mouse was then
         * Coord: x1 = x * y * cos45 + sin45
         * y1 = y * cos45 - sin45 x *
         * And the segment of track is horizontal.
         * Coord recalculation of the mouse (sin45 = cos45 = .707 = 7 / 10
         * Note: sin or cos45 = .707, and when recalculating coord
         * dx45 and dy45, lect coeff .707 is neglected, dx and dy are
         * actually 0707 times
         * Too big. (security hole too small)
         * Spot_cX, Y * must be by .707 * .707 = 0.5
         */

        cXrot = (pointX + pointY) >> 1;
        cYrot = (pointY - pointX) >> 1;

        /* Recalculating coord of segment extremity, which will be vertical
         * following the orientation of axes on the screen: dx45 = pointx
         * (or pointy) and 1.414 is actually greater, and dy45 = 0
         */

        // * Threshold should be .707 to reflect the change in coeff dx, dy
        seuil *= 7;
        seuil /= 10;

        if( abs( cYrot ) <= seuil ) /* ok on vertical axis */
        {
            if( ( cXrot >= 0 ) && ( cXrot <= segX ) )
                return true;

            /* Check extremes using the radius of a circle. */
            if( ( cXrot < 0 ) && ( cXrot >= -seuil ) )
            {
                if( ( ( cXrot * cXrot ) + ( cYrot * cYrot ) ) <= ( seuil * seuil ) )
                    return true;
            }
            if( ( cXrot > segX ) && ( cXrot <= ( segX + seuil ) ) )
            {
                if( ( ( ( cXrot - segX ) * ( cXrot - segX ) )
                     + ( cYrot * cYrot ) ) <= ( seuil * seuil ) )
                    return true;
            }
        }
    }
    else    /* any orientation */
    {
        /* There is a change of axis (rotation), so that the segment
         * track is horizontal in the new reference
         */
        int angle;

        angle = wxRound( ( atan2( (double) segY, (double) segX ) * 1800.0 / M_PI ) );
        cXrot = pointX;
        cYrot = pointY;

        RotatePoint( &cXrot, &cYrot, angle );   /* Rotate the point to be tested */
        RotatePoint( &segX, &segY, angle );     /* Rotate the segment */

        /* The track is horizontal, following the amendments to coordinate
         * axis and, therefore segX = length of segment
         */
        if( abs( cYrot ) <= seuil ) /* vertical axis */
        {
            if( ( cXrot >= 0 ) && ( cXrot <= segX ) )
                return true;

            if( ( cXrot < 0 ) && ( cXrot >= -seuil ) )
            {
                if( ( ( cXrot * cXrot ) + ( cYrot * cYrot ) ) <= ( seuil * seuil ) )
                    return true;
            }

            if( ( cXrot > segX ) && ( cXrot <= ( segX + seuil ) ) )
            {
                if( ( ( ( cXrot - segX ) * ( cXrot - segX ) )
                    + ( cYrot * cYrot ) ) <= ( seuil * seuil ) )
                    return true;
            }
        }
    }

    return false;
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
    return wxRound( fangle );
}


void RotatePoint( int* pX, int* pY, int angle )
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
        double fangle = DEG2RAD( (double) angle / 10.0 );
        double fpx = (*pY * sin( fangle ) ) + (*pX * cos( fangle ) );
        double fpy = (*pY * cos( fangle ) ) - (*pX * sin( fangle ) );
        *pX = wxRound( fpx );
        *pY = wxRound( fpy );
    }
}


void RotatePoint( int* pX, int* pY, int cx, int cy, int angle )
{
    int ox, oy;

    ox = *pX - cx;
    oy = *pY - cy;

    RotatePoint( &ox, &oy, angle );

    *pX = ox + cx;
    *pY = oy + cy;
}


void RotatePoint( wxPoint* point, int angle )
{
    int ox, oy;

    ox = point->x;
    oy = point->y;

    RotatePoint( &ox, &oy, angle );
    point->x = ox;
    point->y = oy;
}


void RotatePoint( wxPoint* point, const wxPoint& centre, int angle )
{
    int ox, oy;

    ox = point->x - centre.x;
    oy = point->y - centre.y;

    RotatePoint( &ox, &oy, angle );
    point->x = ox + centre.x;
    point->y = oy + centre.y;
}


void RotatePoint( double* pX, double* pY, double cx, double cy, int angle )
{
    double ox, oy;

    ox = *pX - cx;
    oy = *pY - cy;

    RotatePoint( &ox, &oy, angle );

    *pX = ox + cx;
    *pY = oy + cy;
}


void RotatePoint( double* pX, double* pY, int angle )
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
        double fangle = DEG2RAD( (double) angle / 10.0 );

        double fpx = (*pY * sin( fangle ) ) + (*pX * cos( fangle ) );
        double fpy = (*pY * cos( fangle ) ) - (*pX * sin( fangle ) );
        *pX = fpx;
        *pY = fpy;
    }
}


double EuclideanNorm( wxPoint vector )
{
    return sqrt( (double) vector.x * (double) vector.x + (double) vector.y * (double) vector.y );
}


wxPoint TwoPointVector( wxPoint startPoint, wxPoint endPoint )
{
    return endPoint - startPoint;
}


double DistanceLinePoint( wxPoint linePointA, wxPoint linePointB, wxPoint referencePoint )
{
    return fabs( (double) ( (linePointB.x - linePointA.x) * (linePointA.y - referencePoint.y) -
                 (linePointA.x - referencePoint.x ) * (linePointB.y - linePointA.y) )
                  / EuclideanNorm( TwoPointVector( linePointA, linePointB ) ) );
}


bool HitTestPoints( wxPoint pointA, wxPoint pointB, double threshold )
{
    wxPoint vectorAB = TwoPointVector( pointA, pointB );
    double  distance = EuclideanNorm( vectorAB );

    return distance < threshold;
}


int CrossProduct( wxPoint vectorA, wxPoint vectorB )
{
    return vectorA.x * vectorB.y - vectorA.y * vectorB.x;
}


double GetLineLength( const wxPoint& aPointA, const wxPoint& aPointB )
{
    return sqrt( pow( (double) aPointA.x - (double) aPointB.x, 2 ) +
                 pow( (double) aPointA.y - (double) aPointB.y, 2 ) );
}
