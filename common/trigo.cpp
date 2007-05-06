		/************************/
		/* Routines de rotation */
		/************************/

		/* Fichier TRIGO.CPP */

#include "fctsys.h"
#define global extern
#include "trigo.h"


/***********************************/
int ArcTangente(int dy, int dx)
/***********************************/
/* Retourne l'arc tangente en 0.1 degres du vecteur de coord dx, dy
	entre -1800 et 1800
	Analogue a atan2 ( mais plus rapide pour les calculs si
	l'angle est souvent 0, -1800, ou +- 900
*/
{
double fangle;
	
	if(dy == 0)
		{
		if(dx >= 0 ) return(0);
		else return(-1800);
		}

	if(dx == 0)
		{
		if(dy >= 0 ) return(900);
		else return(-900);
		}

	if(dx == dy)
		{
		if(dx >= 0) return(450);
		else return(-1800+450);
		}

	if(dx == -dy)
		{
		if(dx >= 0) return(-450);
		else return(1800-450);
		}

	fangle = atan2( (double)dy, (double)dx ) / M_PI * 1800;
	return( (int) round(fangle) );
}


/*********************************************/
void RotatePoint(int *pX, int *pY, int angle)
/*********************************************/
/*
	Fonction surchargee!
	 calcule les nouvelles coord du point de coord pX, pY,
	pour une rotation de centre 0, 0, et d'angle angle ( en 1/10 degre)
*/
{
float fpx, fpy;
int tmp;

	while (angle < 0) angle += 3600;
	while (angle >= 3600) angle -= 3600;

	if (angle == 0) return;

	/* Calcul des coord :
		coord:  xrot = y*sin + x*cos
				yrot = y*cos - x*sin
	*/
	if( angle == 900 )		/* sin = 1, cos = 0 */
		{
		tmp = *pX;
		*pX = *pY;
		*pY = - tmp;
		}

	else if( angle == 1800 )	/* sin = 0, cos = -1 */
		{
		*pX = - *pX;
		*pY = - *pY;
		}

	else if( angle == 2700 )	/* sin = -1, cos = 0 */
		{
		tmp = *pX;
		*pX = - *pY;
		*pY = tmp;
		}

	else
		{
		fpx = (*pY * fsinus[angle]) + (*pX * fcosinus[angle]);
		fpy = (*pY * fcosinus[angle]) - (*pX * fsinus[angle]);
		*pX = (int)round(fpx); *pY = (int)round(fpy);
		}
}


/************************************************************/
void RotatePoint(int *pX, int *pY, int cx, int cy, int angle)
/*************************************************************/
/*
	Fonction surchargee!
	calcule les nouvelles coord du point de coord pX, pY,
	pour une rotation de centre cx, cy, et d'angle angle ( en 1/10 degre)
*/
{
int ox, oy;

	ox = *pX - cx; oy = *pY - cy;
	RotatePoint(&ox, &oy, angle);
	*pX = ox + cx;
	*pY = oy + cy;
}

/*****************************************************************/
void RotatePoint(wxPoint *point, const wxPoint & centre, int angle)
/*****************************************************************/
/*
	Fonction surchargee!
	calcule les nouvelles coord du point point,
	pour une rotation de centre centre, et d'angle angle ( en 1/10 degre)
*/
{
int ox, oy;

	ox = point->x - centre.x; oy = point->y - centre.y;
	RotatePoint(&ox, &oy, angle);
	point->x = ox + centre.x;
	point->y = oy + centre.y;
}


/*************************************************************************/
void RotatePoint(double *pX, double *pY, double cx, double cy, int angle)
/*************************************************************************/
{
double ox, oy;

	ox = *pX - cx; oy = *pY - cy;
	RotatePoint(&ox, &oy, angle);
	*pX = ox + cx;
	*pY = oy + cy;
}


/*************************************************/
void RotatePoint(double *pX, double *pY, int angle)
/*************************************************/
/* Calcul des coord :
		coord:  xrot = y*sin + x*cos
				yrot = y*cos - x*sin
*/
{
double tmp;
	while (angle < 0) angle += 3600;
	while (angle >= 3600) angle -= 3600;
	if (angle == 0) return;
	if( angle == 900 )		/* sin = 1, cos = 0 */
		{
		tmp = *pX;
		*pX = *pY;
		*pY = - tmp;
		}

	else if( angle == 1800 )	/* sin = 0, cos = -1 */
		{
		*pX = - *pX;
		*pY = - *pY;
		}

	else if( angle == 2700 )	/* sin = -1, cos = 0 */
		{
		tmp = *pX;
		*pX = - *pY;
		*pY = tmp;
		}

	else
		{
		double fpx = (*pY * fsinus[angle]) + (*pX * fcosinus[angle]);
		double fpy = (*pY * fcosinus[angle]) - (*pX * fsinus[angle]);
		*pX = fpx; *pY = fpy;
		}
}
