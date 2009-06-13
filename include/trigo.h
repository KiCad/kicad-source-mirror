/****************************************************/
/*  TRIGO.H :  Tables de fonctions trigonometriques */
/*      utilisees dans les rotations d'axes         */
/****************************************************/

#ifndef TRIGO_H
#define TRIGO_H


/* Prototype des fonctions de trigo.cpp */
void RotatePoint( int *pX, int *pY, int angle );
void RotatePoint( int *pX, int *pY, int cx, int cy, int angle );
void RotatePoint( wxPoint* point, int angle );
void RotatePoint( wxPoint *point, const wxPoint & centre, int angle );
void RotatePoint( double *pX, double *pY, int angle );
void RotatePoint( double *pX, double *pY, double cx, double cy, int angle );

/** function ArcTangente
Retourne l'arc tangente en 0.1 degres du vecteur de coord dx, dy
   entre -1800 et 1800
   Analogue a atan2 ( mais plus rapide pour les caculs si
   l'angle est souvent 0, -1800, ou +- 900  */
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
bool TestSegmentHit( wxPoint aRefPoint, wxPoint aStart, wxPoint aEnd, int aDist );

/*******************/
/* Macro NEW_COORD */
/*******************/

/* Macro de calcul de novelles coordonnees par rotation d'axe
             coord :  xrot = y*sin + x*cos
                         yrot = y*cos - x*sin
             soit :      xrot = (y*tg + x)*cos
                         yrot = (y - x*tg)*cos

            les coeffs COS sont tabules en fct de tg sur 16 valeurs.
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
