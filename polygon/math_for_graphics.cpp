// math for graphics utility routines and RC, from FreePCB

#include <vector>

#include <cmath>
#include <float.h>
#include <limits.h>
#include <common.h>

#include <math_for_graphics.h>

static bool InRange( double x, double xi, double xf );

/* Function FindSegmentIntersections
 * find intersections between line segment (xi,yi) to (xf,yf)
 * and line segment (xi2,yi2) to (xf2,yf2)
 * returns true if intersection found
 */
bool FindSegmentIntersections( int xi, int yi, int xf, int yf,
                              int xi2, int yi2, int xf2, int yf2  )
{
    if( std::max( xi, xf ) < std::min( xi2, xf2 )
        || std::min( xi, xf ) > std::max( xi2, xf2 )
        || std::max( yi, yf ) < std::min( yi2, yf2 )
        || std::min( yi, yf ) > std::max( yi2, yf2 ) )
        return false;

    return TestForIntersectionOfStraightLineSegments( xi, yi, xf, yf,
                                                      xi2, yi2, xf2, yf2 );
}


/* Function FindLineSegmentIntersection
 * find intersection between line y = a + bx and line segment (xi,yi) to (xf,yf)
 * if b > DBL_MAX/10, assume vertical line at x = a
 * return false if no intersection or true if intersect
 * return coords of intersections in *x1, *y1, *x2, *y2
 * if no intersection, returns min distance in dist
 */
bool FindLineSegmentIntersection( double a, double b, int xi, int yi, int xf, int yf,
                                 double* x1, double* y1, double* x2, double* y2,
                                 double* dist )
{
    double  xx = 0, yy = 0; // Init made to avoid C compil "uninitialized" warning
    bool    bVert = false;

    if( b > DBL_MAX / 10.0 )
        bVert = true;

    if( xf != xi )      // non-vertical segment, get intersection
    {
        // horizontal or oblique straight segment
        // put into form y = c + dx;
        double  d   = (double) (yf - yi) / (double) (xf - xi);
        double  c   = yf - d * xf;

        if( bVert )
        {
            // if vertical line, easy
            if( InRange( a, xi, xf ) )
            {
                *x1 = a;
                *y1 = c + d * a;
                return 1;
            }
            else
            {
                if( dist )
                    *dist = std::min( std::abs( a - xi ), std::abs( a - xf ) );

                return false;
            }
        }

        if( std::abs( b - d ) < 1E-12 )
        {
            // parallel lines
            if( dist )
            {
                *dist = GetPointToLineDistance( a, b, xi, xf );
            }

            return false;    // lines parallel
        }

        // calculate intersection
        xx  = (c - a) / (b - d);
        yy  = a + b * (xx);

        // see if intersection is within the line segment
        if( yf == yi )
        {
            // horizontal line
            if( (xx>=xi && xx>xf) || (xx<=xi && xx<xf) )
                return false;
        }
        else
        {
            // oblique line
            if( (xx>=xi && xx>xf) || (xx<=xi && xx<xf)
                || (yy>yi && yy>yf) || (yy<yi && yy<yf) )
                return false;
        }
    }
    else
    {
        // vertical line segment
        if( bVert )
            return false;

        xx  = xi;
        yy  = a + b * xx;

        if( (yy>=yi && yy>yf) || (yy<=yi && yy<yf) )
            return 0;
    }

    *x1 = xx;
    *y1 = yy;
    return true;
}


/*
 * Function TestForIntersectionOfStraightLineSegments
 * Test for intersection of line segments
 * If lines are parallel, returns false
 * If true, returns also intersection coords in x, y
 * if false, returns min. distance in dist (may be 0.0 if parallel)
 */
bool TestForIntersectionOfStraightLineSegments( int x1i, int y1i, int x1f, int y1f,
                                                int x2i, int y2i, int x2f, int y2f,
                                                int* x, int* y, double* d )
{
    double a, b, dist;

    // first, test for intersection
    if( x1i == x1f && x2i == x2f )
    {
        // both segments are vertical, can't intersect
    }
    else if( y1i == y1f && y2i == y2f )
    {
        // both segments are horizontal, can't intersect
    }
    else if( x1i == x1f && y2i == y2f )
    {
        // first seg. vertical, second horizontal, see if they cross
        if( InRange( x1i, x2i, x2f )
            && InRange( y2i, y1i, y1f ) )
        {
            if( x )
                *x = x1i;

            if( y )
                *y = y2i;

            if( d )
                *d = 0.0;

            return true;
        }
    }
    else if( y1i == y1f && x2i == x2f )
    {
        // first seg. horizontal, second vertical, see if they cross
        if( InRange( y1i, y2i, y2f )
            && InRange( x2i, x1i, x1f ) )
        {
            if( x )
                *x = x2i;

            if( y )
                *y = y1i;

            if( d )
                *d = 0.0;

            return true;
        }
    }
    else if( x1i == x1f )
    {
        // first segment vertical, second oblique
        // get a and b for second line segment, so that y = a + bx;
        b   = double( y2f - y2i ) / (x2f - x2i);
        a   = (double) y2i - b * x2i;

        double  x1, y1, x2, y2;
        int     test = FindLineSegmentIntersection( a, b, x1i, y1i, x1f, y1f,
                                                    &x1, &y1, &x2, &y2 );

        if( test )
        {
            if( InRange( y1, y1i, y1f ) && InRange( x1, x2i, x2f ) && InRange( y1, y2i, y2f ) )
            {
                if( x )
                    *x = KiROUND( x1 );

                if( y )
                    *y = KiROUND( y1 );

                if( d )
                    *d = 0.0;

                return true;
            }
        }
    }
    else if( y1i == y1f )
    {
        // first segment horizontal, second oblique
        // get a and b for second line segment, so that y = a + bx;
        b   = double( y2f - y2i ) / (x2f - x2i);
        a   = (double) y2i - b * x2i;

        double  x1, y1, x2, y2;
        int     test = FindLineSegmentIntersection( a, b, x1i, y1i, x1f, y1f,
                                                    &x1, &y1, &x2, &y2 );

        if( test )
        {
            if( InRange( x1, x1i, x1f ) && InRange( x1, x2i, x2f ) && InRange( y1, y2i, y2f ) )
            {
                if( x )
                    *x = KiROUND( x1 );

                if( y )
                    *y = KiROUND( y1 );

                if( d )
                    *d = 0.0;

                return true;
            }
        }
    }
    else if( x2i == x2f )
    {
        // second segment vertical, first oblique
        // get a and b for first line segment, so that y = a + bx;
        b   = double( y1f - y1i ) / (x1f - x1i);
        a   = (double) y1i - b * x1i;

        double  x1, y1, x2, y2;
        int     test = FindLineSegmentIntersection( a, b, x2i, y2i, x2f, y2f,
                                                    &x1, &y1, &x2, &y2 );

        if( test )
        {
            if( InRange( x1, x1i, x1f ) &&  InRange( y1, y1i, y1f ) && InRange( y1, y2i, y2f ) )
            {
                if( x )
                    *x = KiROUND( x1 );

                if( y )
                    *y = KiROUND( y1 );

                if( d )
                    *d = 0.0;

                return true;
            }
        }
    }
    else if( y2i == y2f )
    {
        // second segment horizontal, first oblique
        // get a and b for second line segment, so that y = a + bx;
        b   = double( y1f - y1i ) / (x1f - x1i);
        a   = (double) y1i - b * x1i;

        double  x1, y1, x2, y2;
        int     test = FindLineSegmentIntersection( a, b, x2i, y2i, x2f, y2f,
                                                    &x1, &y1, &x2, &y2 );

        if( test )
        {
            if( InRange( x1, x1i, x1f ) && InRange( y1, y1i, y1f ) )
            {
                if( x )
                    *x = KiROUND( x1 );

                if( y )
                    *y = KiROUND( y1 );

                if( d )
                    *d = 0.0;

                return true;
            }
        }
    }
    else
    {
        // both segments oblique
        if( long( y1f - y1i ) * (x2f - x2i) != long( y2f - y2i ) * (x1f - x1i) )
        {
            // not parallel, get a and b for first line segment, so that y = a + bx;
            b   = double( y1f - y1i ) / (x1f - x1i);
            a   = (double) y1i - b * x1i;

            double  x1, y1, x2, y2;
            int     test = FindLineSegmentIntersection( a, b, x2i, y2i, x2f, y2f,
                                                        &x1, &y1, &x2, &y2 );

            // both segments oblique
            if( test )
            {
                if( InRange( x1, x1i, x1f ) && InRange( y1, y1i, y1f ) )
                {
                    if( x )
                        *x = KiROUND( x1 );

                    if( y )
                        *y = KiROUND( y1 );

                    if( d )
                        *d = 0.0;

                    return true;
                }
            }
        }
    }

    // don't intersect, get shortest distance between each endpoint and the other line segment
    dist = GetPointToLineSegmentDistance( x1i, y1i, x2i, y2i, x2f, y2f );

    double  xx  = x1i;
    double  yy  = y1i;
    double  dd  = GetPointToLineSegmentDistance( x1f, y1f, x2i, y2i, x2f, y2f );

    if( dd < dist )
    {
        dist    = dd;
        xx      = x1f;
        yy      = y1f;
    }

    dd = GetPointToLineSegmentDistance( x2i, y2i, x1i, y1i, x1f, y1f );

    if( dd < dist )
    {
        dist    = dd;
        xx      = x2i;
        yy      = y2i;
    }

    dd = GetPointToLineSegmentDistance( x2f, y2f, x1i, y1i, x1f, y1f );

    if( dd < dist )
    {
        dist    = dd;
        xx      = x2f;
        yy      = y2f;
    }

    if( x )
        *x = KiROUND( xx );

    if( y )
        *y = KiROUND( yy );

    if( d )
        *d = dist;

    return false;
}


/* Function GetClearanceBetweenSegments
 * Get clearance between 2 segments
 * Returns coordinates of the closest point between these 2 segments in x, y
 * If clearance > max_cl, just returns max_cl+1 and doesn't return x,y
 */
int GetClearanceBetweenSegments( int x1i, int y1i, int x1f, int y1f, int w1,
                                 int x2i, int y2i, int x2f, int y2f, int w2,
                                 int max_cl, int* x, int* y )
{
    // check clearance between bounding rectangles
    int min_dist = max_cl + ( (w1 + w2) / 2 );

    if( std::min( x1i, x1f ) - std::max( x2i, x2f ) > min_dist )
        return max_cl+1;

    if( std::min( x2i, x2f ) - std::max( x1i, x1f ) > min_dist )
        return max_cl+1;

    if( std::min( y1i, y1f ) - std::max( y2i, y2f ) > min_dist )
        return max_cl+1;

    if( std::min( y2i, y2f ) - std::max( y1i, y1f ) > min_dist )
        return max_cl+1;

    int     xx, yy;
    double  dist;
    TestForIntersectionOfStraightLineSegments( x1i, y1i, x1f, y1f,
                                               x2i, y2i, x2f, y2f, &xx, &yy, &dist );
    int d = KiROUND( dist ) - ((w1 + w2) / 2);
    if( d < 0 )
        d = 0;

    if( x )
        *x = xx;

    if( y )
        *y = yy;

    return d;
}


/* Function GetPointToLineDistance
 * Get min. distance from (x,y) to line y = a + bx
 * if b > DBL_MAX/10, assume vertical line at x = a
 * returns closest point on line in xpp, ypp
 */
double GetPointToLineDistance( double a, double b, int x, int y, double* xpp, double* ypp )
{
    if( b > DBL_MAX / 10 )
    {
        // vertical line
        if( xpp && ypp )
        {
            *xpp    = a;
            *ypp    = y;
        }

        return std::abs( a - x );
    }

    // find c,d such that (x,y) lies on y = c + dx where d=(-1/b)
    double  d   = -1.0 / b;
    double  c   = (double) y - d * x;

    // find nearest point to (x,y) on line through (xi,yi) to (xf,yf)
    double  xp  = (a - c) / (d - b);
    double  yp  = a + b * xp;

    if( xpp && ypp )
    {
        *xpp    = xp;
        *ypp    = yp;
    }

    // find distance
    return Distance( x, y, xp, yp );
}


/**
 * Function GetPointToLineSegmentDistance
 * Get distance between line segment and point
 * @param x,y = point
 * @param xi,yi Start point of the line segament
 * @param xf,yf End point of the line segment
 * @return the distance
 */
double GetPointToLineSegmentDistance( int x, int y, int xi, int yi, int xf, int yf )
{
    // test for vertical or horizontal segment
    if( xf==xi )
    {
        // vertical line segment
        if( InRange( y, yi, yf ) )
            return std::abs( x - xi );
        else
            return std::min( Distance( x, y, xi, yi ), Distance( x, y, xf, yf ) );
    }
    else if( yf==yi )
    {
        // horizontal line segment
        if( InRange( x, xi, xf ) )
            return std::abs( y - yi );
        else
            return std::min( Distance( x, y, xi, yi ), Distance( x, y, xf, yf ) );
    }
    else
    {
        // oblique segment
        // find a,b such that (xi,yi) and (xf,yf) lie on y = a + bx
        double  b   = (double) (yf - yi) / (xf - xi);
        double  a   = (double) yi - b * xi;

        // find c,d such that (x,y) lies on y = c + dx where d=(-1/b)
        double  d   = -1.0 / b;
        double  c   = (double) y - d * x;

        // find nearest point to (x,y) on line through (xi,yi) to (xf,yf)
        double  xp  = (a - c) / (d - b);
        double  yp  = a + b * xp;

        // find distance
        if( InRange( xp, xi, xf ) && InRange( yp, yi, yf ) )
            return Distance( x, y, xp, yp );
        else
            return std::min( Distance( x, y, xi, yi ), Distance( x, y, xf, yf ) );
    }
}


// test for value within range
bool InRange( double x, double xi, double xf )
{
    if( xf > xi )
    {
        if( x >= xi && x <= xf )
            return true;
    }
    else
    {
        if( x >= xf && x <= xi )
            return true;
    }

    return false;
}
