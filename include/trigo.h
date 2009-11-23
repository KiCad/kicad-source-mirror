/*************/
/*  trigo.h  */
/*************/

#ifndef TRIGO_H
#define TRIGO_H


void RotatePoint( int *pX, int *pY, int angle );
void RotatePoint( int *pX, int *pY, int cx, int cy, int angle );
void RotatePoint( wxPoint* point, int angle );
void RotatePoint( wxPoint *point, const wxPoint & centre, int angle );
void RotatePoint( double *pX, double *pY, int angle );
void RotatePoint( double *pX, double *pY, double cx, double cy, int angle );

/* Return the arc tangent of 0.1 degrees coord vector dx, dy
 * between -1800 and 1800
 * Equivalent to atan2 (but faster for calculations if
 * the angle is 0 to -1800, or + - 900
 */
int ArcTangente( int dy, int dx );

bool DistanceTest( int seuil, int dx, int dy, int spot_cX, int spot_cY );


/** Function TestSegmentHit
 * test for hit on line segment
 * i.e. cursor within a given distance from segment
 * @param aRefPoint = cursor (point to test) coords
 * @param aStart is the first end-point of the line segment
 * @param aEnd is the second end-point of the line segment
 * @param aDist = maximum distance for hit
*/
bool TestSegmentHit( wxPoint aRefPoint, wxPoint aStart, wxPoint aEnd,
                     int aDist );

/*******************/
/* Macro NEW_COORD */
/*******************/

/* Calculate coordinates to rotate around an axis
 *           coord:  xrot = y + x * sin * cos
 *                   yrot = y * cos - sin * x
 *           either: xrot = (y + x * tg) * cos
 *                   yrot = (y - x * tg) * cos
 *
 * Cosine coefficients are loaded from a trigometric table by 16 bit values.
 */
#define NEW_COORD( x0, y0 )                       \
    do {                                          \
        int itmp;                                 \
        itmp = x0;                                \
        x0 = x0 + (int)( y0 * tg );               \
        y0 = y0 - (int)( itmp * tg );             \
        x0 = ( x0 * cosinus ) >> 8;               \
        y0 = ( y0 * cosinus ) >> 8;               \
    } while( 0 );


extern double fsinus[];
extern double fcosinus[];

#endif
