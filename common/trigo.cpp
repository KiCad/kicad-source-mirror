/************************/
/* Routines de rotation */
/************************/

/* Fichier TRIGO.CPP */

#include "fctsys.h"
#include "trigo.h"




/************************************************************************/
bool DistanceTest( int seuil, int dx, int dy, int spot_cX, int spot_cY )
/************************************************************************/

/*
 *  Calcul de la distance du curseur souris a un segment de droite :
 *  ( piste, edge, contour module ..
 *  retourne:
 *      false si distance > seuil
 *      true  si distance <= seuil
 *  Variables utilisees ( doivent etre initialisees avant appel , et
 *  sont ramenees au repere centre sur l'origine du segment)
 *      dx, dy = coord de l'extremite segment.
 *      spot_cX,spot_cY = coord du curseur souris
 *  la recherche se fait selon 4 cas:
 *      segment horizontal
 *      segment vertical
 *      segment 45
 *      segment quelconque
 */
{
    int cXrot, cYrot,   /* coord du point (souris) dans le repere tourne */
        segX, segY;     /* coord extremite segment tj >= 0 */
    int pointX, pointY; /* coord point a tester dans repere modifie dans lequel
                         * segX et segY sont >=0 */

    segX = dx; segY = dy; pointX = spot_cX; pointY = spot_cY;

    /*Recalcul coord pour que le segment soit dans 1er quadrant (coord >= 0)*/
    if( segX < 0 )   /* mise en >0 par symetrie par rapport a l'axe Y */
    {
        segX = -segX; pointX = -pointX;
    }
    if( segY < 0 )   /* mise en > 0 par symetrie par rapport a l'axe X */
    {
        segY = -segY; pointY = -pointY;
    }


    if( segY == 0 ) /* piste Horizontale */
    {
        if( abs( pointY ) <= seuil )
        {
            if( (pointX >= 0) && (pointX <= segX) )
                return 1;
            /* Etude des extremites : cercle de rayon seuil */
            if( (pointX < 0) && (pointX >= -seuil) )
            {
                if( ( (pointX * pointX) + (pointY * pointY) ) <= (seuil * seuil) )
                    return true;
            }
            if( (pointX > segX) && ( pointX <= (segX + seuil) ) )
            {
                if( ( ( (pointX - segX) * (pointX - segX) ) + (pointY * pointY) ) <=
                   (seuil * seuil) )
                    return true;
            }
        }
    }
    else if( segX == 0 ) /* piste verticale */
    {
        if( abs( pointX ) <= seuil )
        {
            if( (pointY >= 0 ) && (pointY <= segY) )
                return true;
            if( (pointY < 0) && (pointY >= -seuil) )
            {
                if( ( (pointY * pointY) + (pointX * pointX) ) <= (seuil * seuil) )
                    return true;
            }
            if( (pointY > segY) && ( pointY <= (segY + seuil) ) )
            {
                if( ( ( (pointY - segY) * (pointY - segY) ) + (pointX * pointX) ) <=
                   (seuil * seuil) )
                    return true;
            }
        }
    }
    else if( segX == segY )    /* piste a 45 degre */
    {
        /* on fait tourner les axes de 45 degre. la souris a alors les
         *  coord : x1 = x*cos45 + y*sin45
         *      y1 = y*cos45 - x*sin45
         *  et le segment de piste est alors horizontal.
         *  recalcul des coord de la souris ( sin45 = cos45 = .707 = 7/10
         *  remarque : sin ou cos45 = .707, et lors du recalcul des coord
         *  dx45 et dy45, lec coeff .707 est neglige, dx et dy sont en fait .707 fois
         *  trop grands. (c.a.d trop petits)
         *  spot_cX,Y doit etre * par .707 * .707 = 0.5 */

        cXrot = (pointX + pointY) >> 1;
        cYrot = (pointY - pointX) >> 1;

        /* recalcul des coord de l'extremite du segment , qui sera vertical
         *  suite a l'orientation des axes sur l'ecran : dx45 = pointX (ou pointY)
         *  et est en fait 1,414 plus grand , et dy45 = 0 */

        // seuil doit etre * .707 pour tenir compte du coeff de reduction sur dx,dy
        seuil *= 7; seuil /= 10;
        if( abs( cYrot ) <= seuil ) /* ok sur axe vertical) */
        {
            if( (cXrot >= 0) && (cXrot <= segX) )
                return true;

            /* Etude des extremites : cercle de rayon seuil */
            if( (cXrot < 0) && (cXrot >= -seuil) )
            {
                if( ( (cXrot * cXrot) + (cYrot * cYrot) ) <= (seuil * seuil) )
                    return true;
            }
            if( (cXrot > segX) && ( cXrot <= (segX + seuil) ) )
            {
                if( ( ( (cXrot - segX) * (cXrot - segX) ) + (cYrot * cYrot) ) <= (seuil * seuil) )
                    return true;
            }
        }
    }
    else    /* orientation quelconque */
    {
        /* On fait un changement d'axe (rotation) de facon a ce que le segment
         * de piste soit horizontal dans le nouveau repere */
        int angle;

        angle = (int) ( atan2( (float) segY, (float) segX ) * 1800 / M_PI);
        cXrot = pointX; cYrot = pointY;

        RotatePoint( &cXrot, &cYrot, angle );   /* Rotation du point a tester */
        RotatePoint( &segX, &segY, angle );     /* Rotation du segment */

        /* la piste est Horizontale , par suite des modifs de coordonnes
         * et d'axe, donc segX = longueur du segment */

        if( abs( cYrot ) <= seuil ) /* ok sur axe vertical) */
        {
            if( (cXrot >= 0) && (cXrot <= segX) )
                return true;

            /* Etude des extremites : cercle de rayon seuil */
            if( (cXrot < 0) && (cXrot >= -seuil) )
            {
                if( ( (cXrot * cXrot) + (cYrot * cYrot) ) <= (seuil * seuil) )
                    return true;
            }
            if( (cXrot > segX) && ( cXrot <= (segX + seuil) ) )
            {
                if( ( ( (cXrot - segX) * (cXrot - segX) ) + (cYrot * cYrot) ) <= (seuil * seuil) )
                    return true;
            }
        }
    }
    return false;
}



/***********************************/
int ArcTangente( int dy, int dx )
/***********************************/

/* Retourne l'arc tangente en 0.1 degres du vecteur de coord dx, dy
 *  entre -1800 et 1800
 *  Analogue a atan2 ( mais plus rapide pour les calculs si
 *  l'angle est souvent 0, -1800, ou +- 900
 */
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
    return (int) round( fangle );
}


/*********************************************/
void RotatePoint( int* pX, int* pY, int angle )
/*********************************************/

/*
 *  Fonction surchargee!
 *  calcule les nouvelles coord du point de coord pX, pY,
 *  pour une rotation de centre 0, 0, et d'angle angle ( en 1/10 degre)
 */
{
    float fpx, fpy;
    int   tmp;

    while( angle < 0 )
        angle += 3600;

    while( angle >= 3600 )
        angle -= 3600;

    if( angle == 0 )
        return;

    /* Calcul des coord :
     *  coord:  xrot = y*sin + x*cos
     *          yrot = y*cos - x*sin
     */
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
        fpx = (*pY * fsinus[angle])   + (*pX * fcosinus[angle]);
        fpy = (*pY * fcosinus[angle]) - (*pX * fsinus[angle]);

        *pX = (int) round( fpx ); 
        *pY = (int) round( fpy );
    }
}


/************************************************************/
void RotatePoint( int* pX, int* pY, int cx, int cy, int angle )
/*************************************************************/

/*
 *  Fonction surchargee!
 *  calcule les nouvelles coord du point de coord pX, pY,
 *  pour une rotation de centre cx, cy, et d'angle angle ( en 1/10 degre)
 */
{
    int ox, oy;

    ox = *pX - cx; 
    oy = *pY - cy;
    
    RotatePoint( &ox, &oy, angle );

    *pX = ox + cx;
    *pY = oy + cy;
}


/********************************************/
void RotatePoint( wxPoint* point, int angle )
/********************************************/

/*
 *  Fonction surchargee!
 *  calcule les nouvelles coord du point point,
 *  pour une rotation d'angle angle ( en 1/10 degre)
 */
{
    int ox, oy;

    ox = point->x; 
    oy = point->y;
    
    RotatePoint( &ox, &oy, angle );
    point->x = ox;
    point->y = oy;
}


/*****************************************************************/
void RotatePoint( wxPoint* point, const wxPoint& centre, int angle )
/*****************************************************************/

/*
 *  Fonction surchargee!
 *  calcule les nouvelles coord du point point,
 *  pour une rotation de centre centre, et d'angle angle ( en 1/10 degre)
 */
{
    int ox, oy;

    ox = point->x - centre.x; 
    oy = point->y - centre.y;
    
    RotatePoint( &ox, &oy, angle );
    point->x = ox + centre.x;
    point->y = oy + centre.y;
}



/*************************************************************************/
void RotatePoint( double* pX, double* pY, double cx, double cy, int angle )
/*************************************************************************/
{
    double ox, oy;

    ox = *pX - cx; 
    oy = *pY - cy;
    
    RotatePoint( &ox, &oy, angle );
    
    *pX = ox + cx;
    *pY = oy + cy;
}


/*************************************************/
void RotatePoint( double* pX, double* pY, int angle )
/*************************************************/

/* Calcul des coord :
 *      coord:  xrot = y*sin + x*cos
 *              yrot = y*cos - x*sin
 */
{
    double tmp;

    while( angle < 0 )
        angle += 3600;

    while( angle >= 3600 )
        angle -= 3600;

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
        double fpx = (*pY * fsinus[angle])   + (*pX * fcosinus[angle]);
        double fpy = (*pY * fcosinus[angle]) - (*pX * fsinus[angle]);
        
        *pX = fpx; 
        *pY = fpy;
    }
}


