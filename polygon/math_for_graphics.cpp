// math for graphics utility routines and RC, from FreePCB

#include <vector>

#include <math.h>
#include <float.h>
#include <limits.h>

#include <fctsys.h>

#include <PolyLine.h>

#define NM_PER_MIL 25400

double Distance( double x1, double y1, double x2, double y2 )
{
    double dx = x1 - x2;
    double dy = y1 - y2;
    double d  = sqrt( dx * dx + dy * dy );
    return d;
}


/**
 * Function TestLineHit
 * test for hit on line segment i.e. a point within a given distance from segment
 * @param x, y = cursor coords
 * @param xi,yi,xf,yf = the end-points of the line segment
 * @param dist = maximum distance for hit
 * return true if dist < distance between the point and the segment
 */
bool TestLineHit( int xi, int yi, int xf, int yf, int x, int y, double dist )
{
    double dd;

    // test for vertical or horizontal segment
    if( xf==xi )
    {
        // vertical segment
        dd = fabs( (double) (x - xi) );

        if( dd<dist && ( (yf>yi && y<yf && y>yi) || (yf<yi && y>yf && y<yi) ) )
            return true;
    }
    else if( yf==yi )
    {
        // horizontal segment
        dd = fabs( (double) (y - yi) );

        if( dd<dist && ( (xf>xi && x<xf && x>xi) || (xf<xi && x>xf && x<xi) ) )
            return true;
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
        // find nearest point to (x,y) on line segment (xi,yi) to (xf,yf)
        double  xp  = (a - c) / (d - b);
        double  yp  = a + b * xp;
        // find distance
        dd = sqrt( (x - xp) * (x - xp) + (y - yp) * (y - yp) );

        if( fabs( b )>0.7 )
        {
            // line segment more vertical than horizontal
            if( dd<dist && ( (yf>yi && yp<yf && yp>yi) || (yf<yi && yp>yf && yp<yi) ) )
                return 1;
        }
        else
        {
            // line segment more horizontal than vertical
            if( dd<dist && ( (xf>xi && xp<xf && xp>xi) || (xf<xi && xp>xf && xp<xi) ) )
                return true;
        }
    }

    return false;    // no hit
}


// set EllipseKH struct to describe the ellipse for an arc
//
int MakeEllipseFromArc( int xi, int yi, int xf, int yf, int style, EllipseKH* el )
{
    // arc (quadrant of ellipse)
    // convert to clockwise arc
    int xxi, xxf, yyi, yyf;

    if( style == CPolyLine::ARC_CCW )
    {
        xxi = xf;
        xxf = xi;
        yyi = yf;
        yyf = yi;
    }
    else
    {
        xxi = xi;
        xxf = xf;
        yyi = yi;
        yyf = yf;
    }

    // find center and radii of ellipse
    double xo = 0, yo = 0;

    if( xxf > xxi && yyf > yyi )
    {
        xo  = xxf;
        yo  = yyi;
        el->theta1  = M_PI;
        el->theta2  = M_PI / 2.0;
    }
    else if( xxf < xxi && yyf > yyi )
    {
        xo  = xxi;
        yo  = yyf;
        el->theta1  = -M_PI / 2.0;
        el->theta2  = -M_PI;
    }
    else if( xxf < xxi && yyf < yyi )
    {
        xo  = xxf;
        yo  = yyi;
        el->theta1  = 0.0;
        el->theta2  = -M_PI / 2.0;
    }
    else if( xxf > xxi && yyf < yyi )
    {
        xo  = xxi;
        yo  = yyf;
        el->theta1  = M_PI / 2.0;
        el->theta2  = 0.0;
    }

    el->Center.X    = xo;
    el->Center.Y    = yo;
    el->xrad        = abs( xf - xi );
    el->yrad        = abs( yf - yi );
#if 0
    el->Phi     = 0.0;
    el->MaxRad  = el->xrad;
    el->MinRad  = el->yrad;

    if( el->MaxRad < el->MinRad )
    {
        el->MaxRad  = el->yrad;
        el->MinRad  = el->xrad;
        el->Phi     = M_PI / 2.0;
    }

#endif
    return 0;
}


// find intersections between line segment (xi,yi) to (xf,yf)
// and line segment (xi2,yi2) to (xf2,yf2)
// the line segments may be arcs (i.e. quadrant of an ellipse) or straight
// returns number of intersections found (max of 2)
// returns coords of intersections in arrays x[2], y[2]
//
int FindSegmentIntersections( int xi, int yi, int xf, int yf, int style,
                              int xi2, int yi2, int xf2, int yf2, int style2,
                              double x[], double y[] )
{
    double  xr[12], yr[12];
    int     iret = 0;

    if( max( xi, xf ) < min( xi2, xf2 )
        || min( xi, xf ) > max( xi2, xf2 )
        || max( yi, yf ) < min( yi2, yf2 )
        || min( yi, yf ) > max( yi2, yf2 ) )
        return 0;

    if( style != CPolyLine::STRAIGHT && style2 != CPolyLine::STRAIGHT )
    {
        // two identical arcs intersect
        if( style == style2 && xi == xi2 && yi == yi2 && xf == xf2 && yf == yf2 )
        {
            if( x && y )
            {
                x[0]    = xi;
                y[0]    = yi;
            }

            return 1;
        }
        else if( style != style2 && xi == xf2 && yi == yf2 && xf == xi2 && yf == yi2 )
        {
            if( x && y )
            {
                x[0]    = xi;
                y[0]    = yi;
            }

            return 1;
        }
    }

    if( style == CPolyLine::STRAIGHT && style2 == CPolyLine::STRAIGHT )
    {
        // both straight-line segments
        int     x, y;
        bool    bYes = TestForIntersectionOfStraightLineSegments( xi,
                                                                  yi,
                                                                  xf,
                                                                  yf,
                                                                  xi2,
                                                                  yi2,
                                                                  xf2,
                                                                  yf2,
                                                                  &x,
                                                                  &y );

        if( !bYes )
            return 0;

        xr[0]   = x;
        yr[0]   = y;
        iret    = 1;
    }
    else if( style == CPolyLine::STRAIGHT )
    {
        // first segment is straight, second segment is an arc
        int     ret;
        double  x1r, y1r, x2r, y2r;

        if( xf == xi )
        {
            // vertical first segment
            double  a   = xi;
            double  b   = DBL_MAX / 2.0;
            ret = FindLineSegmentIntersection( a, b, xi2, yi2, xf2, yf2, style2,
                                               &x1r, &y1r, &x2r, &y2r );
        }
        else
        {
            double  b   = (double) (yf - yi) / (double) (xf - xi);
            double  a   = yf - b * xf;
            ret = FindLineSegmentIntersection( a, b, xi2, yi2, xf2, yf2, style2,
                                               &x1r, &y1r, &x2r, &y2r );
        }

        if( ret == 0 )
            return 0;

        if( InRange( x1r, xi, xf ) && InRange( y1r, yi, yf ) )
        {
            xr[iret]    = x1r;
            yr[iret]    = y1r;
            iret++;
        }

        if( ret == 2 )
        {
            if( InRange( x2r, xi, xf ) && InRange( y2r, yi, yf ) )
            {
                xr[iret]    = x2r;
                yr[iret]    = y2r;
                iret++;
            }
        }
    }
    else if( style2 == CPolyLine::STRAIGHT )
    {
        // first segment is an arc, second segment is straight
        int     ret;
        double  x1r, y1r, x2r, y2r;

        if( xf2 == xi2 )
        {
            // vertical second segment
            double  a   = xi2;
            double  b   = DBL_MAX / 2.0;
            ret = FindLineSegmentIntersection( a, b, xi, yi, xf, yf, style,
                                               &x1r, &y1r, &x2r, &y2r );
        }
        else
        {
            double  b   = (double) (yf2 - yi2) / (double) (xf2 - xi2);
            double  a   = yf2 - b * xf2;
            ret = FindLineSegmentIntersection( a, b, xi, yi, xf, yf, style,
                                               &x1r, &y1r, &x2r, &y2r );
        }

        if( ret == 0 )
            return 0;

        if( InRange( x1r, xi2, xf2 ) && InRange( y1r, yi2, yf2 ) )
        {
            xr[iret]    = x1r;
            yr[iret]    = y1r;
            iret++;
        }

        if( ret == 2 )
        {
            if( InRange( x2r, xi2, xf2 ) && InRange( y2r, yi2, yf2 ) )
            {
                xr[iret]    = x2r;
                yr[iret]    = y2r;
                iret++;
            }
        }
    }
    else
    {
        // both segments are arcs
        EllipseKH   el1;
        EllipseKH   el2;
        MakeEllipseFromArc( xi, yi, xf, yf, style, &el1 );
        MakeEllipseFromArc( xi2, yi2, xf2, yf2, style2, &el2 );
        int         n;

        if( el1.xrad + el1.yrad > el2.xrad + el2.yrad )
            n = GetArcIntersections( &el1, &el2 );
        else
            n = GetArcIntersections( &el2, &el1 );

        iret = n;
    }

    if( x && y )
    {
        for( int i = 0; i<iret; i++ )
        {
            x[i]    = xr[i];
            y[i]    = yr[i];
        }
    }

    return iret;
}


// find intersection between line y = a + bx and line segment (xi,yi) to (xf,yf)
// if b > DBL_MAX/10, assume vertical line at x = a
// the line segment may be an arc (i.e. quadrant of an ellipse)
// return 0 if no intersection
// returns 1 or 2 if intersections found
// sets coords of intersections in *x1, *y1, *x2, *y2
// if no intersection, returns min distance in dist
//
int FindLineSegmentIntersection( double a, double b, int xi, int yi, int xf, int yf, int style,
                                 double* x1, double* y1, double* x2, double* y2,
                                 double* dist )
{
    double  xx = 0, yy = 0; // Init made to avoid C compil "uninitialized" warning
    bool    bVert = false;

    if( b > DBL_MAX / 10.0 )
        bVert = true;

    if( xf != xi )
    {
        // non-vertical segment, get intersection
        if( style == CPolyLine::STRAIGHT || yf == yi )
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
                        *dist = min( abs( a - xi ), abs( a - xf ) );

                    return 0;
                }
            }

            if( fabs( b - d ) < 1E-12 )
            {
                // parallel lines
                if( dist )
                {
                    *dist = GetPointToLineDistance( a, b, xi, xf );
                }

                return 0;    // lines parallel
            }

            // calculate intersection
            xx  = (c - a) / (b - d);
            yy  = a + b * (xx);

            // see if intersection is within the line segment
            if( yf == yi )
            {
                // horizontal line
                if( (xx>=xi && xx>xf) || (xx<=xi && xx<xf) )
                    return 0;
            }
            else
            {
                // oblique line
                if( (xx>=xi && xx>xf) || (xx<=xi && xx<xf)
                    || (yy>yi && yy>yf) || (yy<yi && yy<yf) )
                    return 0;
            }
        }
        else if( style == CPolyLine::ARC_CW || style == CPolyLine::ARC_CCW )
        {
            // arc (quadrant of ellipse)
            // convert to clockwise arc
            int xxi, xxf, yyi, yyf;

            if( style == CPolyLine::ARC_CCW )
            {
                xxi = xf;
                xxf = xi;
                yyi = yf;
                yyf = yi;
            }
            else
            {
                xxi = xi;
                xxf = xf;
                yyi = yi;
                yyf = yf;
            }

            // find center and radii of ellipse
            double xo = xxf, yo = yyi, rx, ry;      // Init made to avoid C compil warnings

            if( xxf > xxi && yyf > yyi )
            {
                xo  = xxf;
                yo  = yyi;
            }
            else if( xxf < xxi && yyf > yyi )
            {
                xo  = xxi;
                yo  = yyf;
            }
            else if( xxf < xxi && yyf < yyi )
            {
                xo  = xxf;
                yo  = yyi;
            }
            else if( xxf > xxi && yyf < yyi )
            {
                xo  = xxi;
                yo  = yyf;
            }

            rx  = fabs( (double) (xxi - xxf) );
            ry  = fabs( (double) (yyi - yyf) );
            bool    test;
            double  xx1, xx2, yy1, yy2, aa;

            if( bVert )
            {
                // shift vertical line to coordinate system of ellipse
                aa = a - xo;
                test = FindVerticalLineEllipseIntersections( rx, ry, aa, &yy1, &yy2 );

                if( !test )
                    return 0;

                // shift back to PCB coordinates
                yy1 += yo;
                yy2 += yo;
                xx1 = a;
                xx2 = a;
            }
            else
            {
                // shift line to coordinate system of ellipse
                aa = a + b * xo - yo;
                test = FindLineEllipseIntersections( rx, ry, aa, b, &xx1, &xx2 );

                if( !test )
                    return 0;

                // shift back to PCB coordinates
                yy1 = aa + b * xx1;
                xx1 += xo;
                yy1 += yo;
                yy2 = aa + b * xx2;
                xx2 += xo;
                yy2 += yo;
            }

            int npts = 0;

            if( (xxf>xxi && xx1<xxf && xx1>xxi) || (xxf<xxi && xx1<xxi && xx1>xxf) )
            {
                if( (yyf>yyi && yy1<yyf && yy1>yyi) || (yyf<yyi && yy1<yyi && yy1>yyf) )
                {
                    *x1     = xx1;
                    *y1     = yy1;
                    npts    = 1;
                }
            }

            if( (xxf>xxi && xx2<xxf && xx2>xxi) || (xxf<xxi && xx2<xxi && xx2>xxf) )
            {
                if( (yyf>yyi && yy2<yyf && yy2>yyi) || (yyf<yyi && yy2<yyi && yy2>yyf) )
                {
                    if( npts == 0 )
                    {
                        *x1     = xx2;
                        *y1     = yy2;
                        npts    = 1;
                    }
                    else
                    {
                        *x2     = xx2;
                        *y2     = yy2;
                        npts    = 2;
                    }
                }
            }

            return npts;
        }
        else
            wxASSERT( 0 );
    }
    else
    {
        // vertical line segment
        if( bVert )
            return 0;

        xx  = xi;
        yy  = a + b * xx;

        if( (yy>=yi && yy>yf) || (yy<=yi && yy<yf) )
            return 0;
    }

    *x1 = xx;
    *y1 = yy;
    return 1;
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
        int     test = FindLineSegmentIntersection( a, b, x1i, y1i, x1f, y1f, CPolyLine::STRAIGHT,
                                                    &x1, &y1, &x2, &y2 );

        if( test )
        {
            if( InRange( y1, y1i, y1f ) && InRange( x1, x2i, x2f ) && InRange( y1, y2i, y2f ) )
            {
                if( x )
                    *x = (int) x1;

                if( y )
                    *y = (int) y1;

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
        int     test = FindLineSegmentIntersection( a, b, x1i, y1i, x1f, y1f, CPolyLine::STRAIGHT,
                                                    &x1, &y1, &x2, &y2 );

        if( test )
        {
            if( InRange( x1, x1i, x1f ) && InRange( x1, x2i, x2f ) && InRange( y1, y2i, y2f ) )
            {
                if( x )
                    *x = (int) x1;

                if( y )
                    *y = (int) y1;

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
        int     test = FindLineSegmentIntersection( a, b, x2i, y2i, x2f, y2f, CPolyLine::STRAIGHT,
                                                    &x1, &y1, &x2, &y2 );

        if( test )
        {
            if( InRange( x1, x1i, x1f ) &&  InRange( y1, y1i, y1f ) && InRange( y1, y2i, y2f ) )
            {
                if( x )
                    *x = (int) x1;

                if( y )
                    *y = (int) y1;

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
        int     test = FindLineSegmentIntersection( a, b, x2i, y2i, x2f, y2f, CPolyLine::STRAIGHT,
                                                    &x1, &y1, &x2, &y2 );

        if( test )
        {
            if( InRange( x1, x1i, x1f ) && InRange( y1, y1i, y1f ) )
            {
                if( x )
                    *x = (int) x1;

                if( y )
                    *y = (int) y1;

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
            int     test = FindLineSegmentIntersection( a,
                                                        b,
                                                        x2i,
                                                        y2i,
                                                        x2f,
                                                        y2f,
                                                        CPolyLine::STRAIGHT,
                                                        &x1,
                                                        &y1,
                                                        &x2,
                                                        &y2 );

            // both segments oblique
            if( test )
            {
                if( InRange( x1, x1i, x1f ) && InRange( y1, y1i, y1f ) )
                {
                    if( x )
                        *x = (int) x1;

                    if( y )
                        *y = (int) y1;

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
        *x = (int) xx;

    if( y )
        *y = (int) yy;

    if( d )
        *d = dist;

    return false;
}


/*solves the Quadratic equation = a*x*x + b*x + c
 */
bool Quadratic( double a, double b, double c, double* x1, double* x2 )
{
    double root = b * b - 4.0 * a * c;

    if( root < 0.0 )
        return false;

    root    = sqrt( root );
    *x1     = (-b + root) / (2.0 * a);
    *x2     = (-b - root) / (2.0 * a);
    return true;
}


// finds intersections of vertical line at x
// with ellipse defined by (x^2)/(a^2) + (y^2)/(b^2) = 1;
// returns true if solution exist, with solutions in y1 and y2
// else returns false
//
bool FindVerticalLineEllipseIntersections( double a, double b, double x, double* y1, double* y2 )
{
    double y_sqr = ( 1.0 - (x * x) / (a * a) ) * b * b;

    if( y_sqr < 0.0 )
        return false;

    *y1 = sqrt( y_sqr );
    *y2 = -*y1;
    return true;
}


// finds intersections of straight line y = c + dx
// with ellipse defined by (x^2)/(a^2) + (y^2)/(b^2) = 1;
// returns true if solution exist, with solutions in x1 and x2
// else returns false
//
bool FindLineEllipseIntersections( double a, double b, double c, double d, double* x1, double* x2 )
{
    // quadratic terms
    double  A   = d * d + b * b / (a * a);
    double  B   = 2.0 * c * d;
    double  C   = c * c - b * b;

    return Quadratic( A, B, C, x1, x2 );
}


// Get clearance between 2 segments
// Returns point in segment closest to other segment in x, y
// in clearance > max_cl, just returns max_cl and doesn't return x,y
//
int GetClearanceBetweenSegments( int x1i, int y1i, int x1f, int y1f, int style1, int w1,
                                 int x2i, int y2i, int x2f, int y2f, int style2, int w2,
                                 int max_cl, int* x, int* y )
{
    // check clearance between bounding rectangles
    int test = max_cl + w1 / 2 + w2 / 2;

    if( min( x1i, x1f ) - max( x2i, x2f ) > test )
        return max_cl;

    if( min( x2i, x2f ) - max( x1i, x1f ) > test )
        return max_cl;

    if( min( y1i, y1f ) - max( y2i, y2f ) > test )
        return max_cl;

    if( min( y2i, y2f ) - max( y1i, y1f ) > test )
        return max_cl;

    if( style1 == CPolyLine::STRAIGHT && style1 == CPolyLine::STRAIGHT )
    {
        // both segments are straight lines
        int     xx, yy;
        double  dd;
        TestForIntersectionOfStraightLineSegments( x1i, y1i, x1f, y1f,
                                                   x2i, y2i, x2f, y2f, &xx, &yy, &dd );
        int     d = max( 0, (int) dd - w1 / 2 - w2 / 2 );

        if( x )
            *x = xx;

        if( y )
            *y = yy;

        return d;
    }

    // not both straight-line segments
    // see if segments intersect
    double  xr[2];
    double  yr[2];
    test =
        FindSegmentIntersections( x1i, y1i, x1f, y1f, style1, x2i, y2i, x2f, y2f, style2, xr, yr );

    if( test )
    {
        if( x )
            *x = (int) xr[0];

        if( y )
            *y = (int) yr[0];

        return 0;
    }

    // at least one segment is an arc
    EllipseKH   el1;
    EllipseKH   el2;
    bool        bArcs;
    int         xi = 0, yi = 0, xf = 0, yf = 0;

    if( style2 == CPolyLine::STRAIGHT )
    {
        // style1 = arc, style2 = straight
        MakeEllipseFromArc( x1i, y1i, x1f, y1f, style1, &el1 );
        xi = x2i;
        yi = y2i;
        xf = x2f;
        yf = y2f;
        bArcs = false;
    }
    else if( style1 == CPolyLine::STRAIGHT )
    {
        // style2 = arc, style1 = straight
        xi  = x1i;
        yi  = y1i;
        xf  = x1f;
        yf  = y1f;
        MakeEllipseFromArc( x2i, y2i, x2f, y2f, style2, &el1 );
        bArcs = false;
    }
    else
    {
        // style1 = arc, style2 = arc
        MakeEllipseFromArc( x1i, y1i, x1f, y1f, style1, &el1 );
        MakeEllipseFromArc( x2i, y2i, x2f, y2f, style2, &el2 );
        bArcs = true;
    }

    const int NSTEPS = 32;

    if( el1.theta2 > el1.theta1 )
    {
        wxASSERT( 0 );
    }

    if( bArcs && el2.theta2 > el2.theta1 )
    {
        wxASSERT( 0 );
    }

    // test multiple points in both segments
    double  th1;
    double  th2;
    double  len2;

    if( bArcs )
    {
        th1     = el2.theta1;
        th2     = el2.theta2;
        len2    = max( el2.xrad, el2.yrad );
    }
    else
    {
        th1     = 1.0;
        th2     = 0.0;
        len2    = abs( xf - xi ) + abs( yf - yi );
    }

    double  s_start     = el1.theta1;
    double  s_end       = el1.theta2;
    double  s_start2    = th1;
    double  s_end2      = th2;
    double  dmin        = DBL_MAX;
    double  xmin        = 0, ymin = 0, smin = 0, smin2 = 0; // Init made to avoid C compil warnings

    int     nsteps  = NSTEPS;
    int     nsteps2 = NSTEPS;
    double  step    = (s_start - s_end) / (nsteps - 1);
    double  step2   = (s_start2 - s_end2) / (nsteps2 - 1);

    while( ( step * max( el1.xrad, el1.yrad ) ) > 0.1 * NM_PER_MIL
           && (step2 * len2) > 0.1 * NM_PER_MIL )
    {
        step = (s_start - s_end) / (nsteps - 1);

        for( int i = 0; i<nsteps; i++ )
        {
            double s;

            if( i < nsteps - 1 )
                s = s_start - i * step;
            else
                s = s_end;

            double  x = el1.Center.X + el1.xrad * cos( s );

            double  y = el1.Center.Y + el1.yrad * sin( s );

            // if not an arc, use s2 as fractional distance along line
            step2 = (s_start2 - s_end2) / (nsteps2 - 1);

            for( int i2 = 0; i2<nsteps2; i2++ )
            {
                double s2;

                if( i2 < nsteps2 - 1 )
                    s2 = s_start2 - i2 * step2;
                else
                    s2 = s_end2;

                double x2, y2;

                if( !bArcs )
                {
                    x2  = xi + (xf - xi) * s2;
                    y2  = yi + (yf - yi) * s2;
                }
                else
                {
                    x2 = el2.Center.X + el2.xrad*   cos( s2 );

                    y2 = el2.Center.Y + el2.yrad*   sin( s2 );
                }

                double d = Distance( x, y, x2, y2 );

                if( d < dmin )
                {
                    dmin    = d;
                    xmin    = x;
                    ymin    = y;
                    smin    = s;
                    smin2   = s2;
                }
            }
        }

        if( step > step2 )
        {
            s_start = min( el1.theta1, smin + step );
            s_end   = max( el1.theta2, smin - step );
            step    = (s_start - s_end) / nsteps;
        }
        else
        {
            s_start2    = min( th1, smin2 + step2 );
            s_end2      = max( th2, smin2 - step2 );
            step2       = (s_start2 - s_end2) / nsteps2;
        }
    }

    if( x )
        *x = (int) xmin;

    if( y )
        *y = (int) ymin;

    return max( 0, (int) dmin - w1 / 2 - w2 / 2 );    // allow for widths
}


// Get min. distance from (x,y) to line y = a + bx
// if b > DBL_MAX/10, assume vertical line at x = a
// returns closest point on line in xp, yp
//
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

        return abs( a - x );
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


/***********************************************************************************/
double GetPointToLineSegmentDistance( int x, int y, int xi, int yi, int xf, int yf )
/***********************************************************************************/
/**
 * Function GetPointToLineSegmentDistance
 * Get distance between line segment and point
 * @param x,y = point
 * @param xi,yi Start point of the line segament
 * @param xf,yf End point of the line segment
 * @return the distance
 */
{
    // test for vertical or horizontal segment
    if( xf==xi )
    {
        // vertical line segment
        if( InRange( y, yi, yf ) )
            return abs( x - xi );
        else
            return min( Distance( x, y, xi, yi ), Distance( x, y, xf, yf ) );
    }
    else if( yf==yi )
    {
        // horizontal line segment
        if( InRange( x, xi, xf ) )
            return abs( y - yi );
        else
            return min( Distance( x, y, xi, yi ), Distance( x, y, xf, yf ) );
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
            return min( Distance( x, y, xi, yi ), Distance( x, y, xf, yf ) );
    }
}


// test for value within range
//
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


// this finds approximate solutions
// note: this works best if el2 is smaller than el1
//
int GetArcIntersections( EllipseKH* el1, EllipseKH* el2,
                         double* x1, double* y1, double* x2, double* y2 )
{
    if( el1->theta2 > el1->theta1 )
    {
        wxASSERT( 0 );
    }

    if( el2->theta2 > el2->theta1 )
    {
        wxASSERT( 0 );
    }

    const int   NSTEPS = 32;
    double      xret[2], yret[2];

    double      xscale  = 1.0 / el1->xrad;
    double      yscale  = 1.0 / el1->yrad;

    // now transform params of second ellipse into reference frame
    // with origin at center if first ellipse,
    // scaled so the first ellipse is a circle of radius = 1.0
    double  xo  = (el2->Center.X - el1->Center.X) * xscale;
    double  yo  = (el2->Center.Y - el1->Center.Y) * yscale;
    double  xr  = el2->xrad * xscale;
    double  yr  = el2->yrad * yscale;

    // now test NSTEPS positions in arc, moving clockwise (ie. decreasing theta)
    double  step    = M_PI / ( (NSTEPS - 1) * 2.0 );
    double  d_prev  = 0;
    double  th_interp;
    double  th1;

    int     n = 0;

    for( int i = 0; i<NSTEPS; i++ )
    {
        double theta;

        if( i < NSTEPS - 1 )
            theta = el2->theta1 - i * step;
        else
            theta = el2->theta2;

        double  x = xo + xr * cos( theta );

        double  y = yo + yr * sin( theta );

        double  d = 1.0 - sqrt( x * x + y * y );

        if( i>0 )
        {
            bool bInt = false;

            if( d >= 0.0 && d_prev <= 0.0 )
            {
                th_interp = theta + ( step * (-d_prev) ) / (d - d_prev);
                bInt = true;
            }
            else if( d <= 0.0 && d_prev >= 0.0 )
            {
                th_interp = theta + (step * d_prev) / (d_prev - d);
                bInt = true;
            }

            if( bInt )
            {
                x = xo + xr * cos( th_interp );

                y = yo + yr * sin( th_interp );

                th1 = atan2( y, x );

                if( th1 <= el1->theta1 && th1 >= el1->theta2 )
                {
                    xret[n] = x * el1->xrad + el1->Center.X;
                    yret[n] = y * el1->yrad + el1->Center.Y;
                    n++;

                    if( n > 2 )
                    {
                        wxASSERT( 0 );
                    }
                }
            }
        }

        d_prev = d;
    }

    if( x1 )
        *x1 = xret[0];

    if( y1 )
        *y1 = yret[0];

    if( x2 )
        *x2 = xret[1];

    if( y2 )
        *y2 = yret[1];

    return n;
}


// this finds approximate solution
//
// double GetSegmentClearance( EllipseKH * el1, EllipseKH * el2,
double GetArcClearance( EllipseKH* el1, EllipseKH* el2,
                        double* x1, double* y1 )
{
    const int NSTEPS = 32;

    if( el1->theta2 > el1->theta1 )
    {
        wxASSERT( 0 );
    }

    if( el2->theta2 > el2->theta1 )
    {
        wxASSERT( 0 );
    }

    // test multiple positions in both arcs, moving clockwise (ie. decreasing theta)
    double  th_start    = el1->theta1;
    double  th_end      = el1->theta2;
    double  th_start2   = el2->theta1;
    double  th_end2     = el2->theta2;
    double  dmin    = DBL_MAX;
    double  xmin    = 0, ymin = 0, thmin = 0, thmin2 = 0;

    int     nsteps  = NSTEPS;
    int     nsteps2 = NSTEPS;
    double  step    = (th_start - th_end) / (nsteps - 1);
    double  step2   = (th_start2 - th_end2) / (nsteps2 - 1);

    while( ( step * max( el1->xrad, el1->yrad ) ) > 1.0 * NM_PER_MIL
           && ( step2 * max( el2->xrad, el2->yrad ) ) > 1.0 * NM_PER_MIL )
    {
        step = (th_start - th_end) / (nsteps - 1);

        for( int i = 0; i<nsteps; i++ )
        {
            double theta;

            if( i < nsteps - 1 )
                theta = th_start - i * step;
            else
                theta = th_end;

            double  x = el1->Center.X + el1->xrad * cos( theta );

            double  y = el1->Center.Y + el1->yrad * sin( theta );

            step2 = (th_start2 - th_end2) / (nsteps2 - 1);

            for( int i2 = 0; i2<nsteps2; i2++ )
            {
                double theta2;

                if( i2 < nsteps2 - 1 )
                    theta2 = th_start2 - i2 * step2;
                else
                    theta2 = th_end2;

                double  x2 = el2->Center.X + el2->xrad * cos( theta2 );
                double  y2 = el2->Center.Y + el2->yrad * sin( theta2 );
                double  d = Distance( x,  y, x2, y2 );

                if( d < dmin )
                {
                    dmin    = d;
                    xmin    = x;
                    ymin    = y;
                    thmin   = theta;
                    thmin2  = theta2;
                }
            }
        }

        if( step > step2 )
        {
            th_start    = min( el1->theta1, thmin + step );
            th_end      = max( el1->theta2, thmin - step );
            step        = (th_start - th_end) / nsteps;
        }
        else
        {
            th_start2   = min( el2->theta1, thmin2 + step2 );
            th_end2     = max( el2->theta2, thmin2 - step2 );
            step2       = (th_start2 - th_end2) / nsteps2;
        }
    }

    if( x1 )
        *x1 = xmin;

    if( y1 )
        *y1 = ymin;

    return dmin;
}
