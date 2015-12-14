#ifndef MATH_FOR_GRAPHICS_H
#define MATH_FOR_GRAPHICS_H
// math stuff for graphics, from FreePCB

/* Function FindLineSegmentIntersection
 * find intersection between line y = a + bx and line segment (xi,yi) to (xf,yf)
 * if b > DBL_MAX/10, assume vertical line at x = a
 * return false if no intersection or true if intersect
 * return coords of intersections in *x1, *y1, *x2, *y2
 * if no intersection, returns min distance in dist
 */
bool FindLineSegmentIntersection( double a, double b, int xi, int yi, int xf, int yf,
                double * x1, double * y1, double * x2, double * y2, double * dist=NULL );

/* Function FindSegmentIntersections
 * find intersections between line segment (xi,yi) to (xf,yf)
 * and line segment (xi2,yi2) to (xf2,yf2)
 * returns true if intersection found
 */
bool FindSegmentIntersections( int xi, int yi, int xf, int yf,
                              int xi2, int yi2, int xf2, int yf2 );

/**
 * Function TestForIntersectionOfStraightLineSegments
 * Test for intersection of line segments
 * If lines are parallel, returns false
 * If true, returns also intersection coords in x, y
 * if false, returns min. distance in dist (may be 0.0 if parallel)
 * and coords on nearest point in one of the segments in (x,y)
 * @param x1i, y1i, x1f, y1f = integer coordinates of the first segment
 * @param x2i, y2i, x2f, y2f = integer coordinates of the other segment
 * @param x, y  = pointers on 2 integer to store the intersection coordinates (can be NULL)
 * @param dist  = pointeur on a double to store the dist.
 * @return true if intersect.
 */
bool TestForIntersectionOfStraightLineSegments( int x1i, int y1i, int x1f, int y1f,
                                       int x2i, int y2i, int x2f, int y2f,
                                       int * x=NULL, int * y=NULL, double * dist=NULL );

/* Function GetClearanceBetweenSegments
 * Get clearance between 2 segments
 * Returns coordinates of the closest point between these 2 segments in x, y
 * If clearance > max_cl, just returns max_cl+1 and doesn't return x,y
 */
int GetClearanceBetweenSegments( int x1i, int y1i, int x1f, int y1f, int w1,
                                   int x2i, int y2i, int x2f, int y2f, int w2,
                                   int max_cl, int * x, int * y );

/**
 * Function GetPointToLineSegmentDistance
 * Get distance between line segment and point
 * @param x,y = point
 * @param xi,yi, xf,yf = the end-points of the line segment
 * @return the distance
 */
double GetPointToLineSegmentDistance( int x, int y, int xi, int yi, int xf, int yf );

/* Function GetPointToLineDistance
 * Get min. distance from (x,y) to line y = a + bx
 * if b > DBL_MAX/10, assume vertical line at x = a
 * returns closest point on line in xpp, ypp
 */
double GetPointToLineDistance( double a, double b, int x, int y,
                               double * xp=NULL, double * yp=NULL );

inline double Distance( double x1, double y1, double x2, double y2 )
{
    return hypot( x1 - x2, y1 - y2 );
}

#endif
