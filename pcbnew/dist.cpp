	/***************************************************************/
	/* EDITEUR de PCB: AUTOROUTAGE: routine de calcul de distances */
	/***************************************************************/

#include "fctsys.h"
#include "gr_basic.h"

#include "common.h"
#include "pcbnew.h"
#include "autorout.h"
#include "cell.h"

/* Routines exportees : */
int GetApxDist( int, int, int, int );
int CalcDist( int, int, int );

/* Les tables de distances et penalites sont etablies sur la base du pas 
de routage de 50 unites(le pas entre les cellules est 50 unites)
La distance vraie est calculee par un facteur d'echelle
*/


	/************************************************/
	/* int GetApxDist(int r1,int c1,int r2,int c2 ) */
	/************************************************/

 /* calculate approximate distance */

int GetApxDist(int r1,int c1,int r2,int c2 )
{
int d1, d2; /* row and column deltas */

	if ((d1 = r1-r2) < 0) /* get absolute row delta */
		d1 = -d1;
	if ((d2 = c1-c2) < 0) /* get absolute column delta */
		d2 = -d2;

return( (d1+d2) * 50 * E_scale);

	if (!d1) /* in same row? */
		return( (d2*50*E_scale) );	 /* 50 mils per cell */

	if (!d2) /* in same column? */
		return( (d1*50*E_scale) ); /* 50 mils per cell */

	if (d1 > d2) /* get smaller into d1 */
		{
		EXCHG(d1,d2);
		}
	d2 -= d1; /* get non-diagonal part of approximate "route" */
	return( ((d1*71)+(d2*50))*E_scale ); /* 71 mils diagonally per cell */
	}

/* distance to go thru a cell (en mils) */
static int dist[10][10] = { /* OT=Otherside, OR=Origin (source) cell */
/*..........N, NE,  E, SE,  S, SW,  W, NW,	 OT, OR */
/* N  */ { 50, 60, 35, 60, 99, 60, 35, 60,	 12, 12 },
/* NE */ { 60, 71, 60, 71, 60, 99, 60, 71,	 23, 23 },
/* E  */ { 35, 60, 50, 60, 35, 60, 99, 60,	 12, 12 },
/* SE */ { 60, 71, 60, 71, 60, 71, 60, 99,	 23, 23 },
/* S  */ { 99, 60, 35, 60, 50, 60, 35, 60,	 12, 12 },
/* SW */ { 60, 99, 60, 71, 60, 71, 60, 71,	 23, 23 },
/* W  */ { 35, 60, 99, 60, 35, 60, 50, 60,	 12, 12 },
/* NW */ { 60, 71, 60, 99, 60, 71, 60, 71,	 23, 23 },

/* OT */ { 12, 23, 12, 23, 12, 23, 12, 23,	 99, 99 },
/* OR */ { 99, 99, 99, 99, 99, 99, 99, 99,	 99, 99 }
	};

/* penalty for extraneous holes and corners, scaled by sharpness of turn */
static int penalty[10][10] = { /* OT=Otherside, OR=Origin (source) cell */
/*......... N, NE,  E, SE,  S, SW,  W, NW,	 OT, OR */
/* N  */ {  0,  5, 10, 15, 20, 15, 10,  5,	 50, 0 },
/* NE */ {  5,  0,  5, 10, 15, 20, 15, 10,	 50, 0 },
/* E  */ { 10,  5,  0,  5, 10, 15, 20, 15,	 50, 0 },
/* SE */ { 15, 10,  5,  0,  5, 10, 15, 20,	 50, 0 },
/* S  */ { 20, 15, 10,  5,  0,  5, 10, 15,	 50, 0 },
/* SW */ { 15, 20, 15, 10,  5,  0,  5, 10,	 50, 0 },
/* W  */ { 10, 15, 20, 15, 10,  5,  0,  5,	 50, 0 },
/* NW */ {  5, 10, 15, 20, 15, 10,  5,  0,	 50, 0 },

/* OT */ { 50, 50, 50, 50, 50, 50, 50, 50,  100, 0 },
/* OR */ {  0,  0,  0,  0,  0,  0,  0,  0,	  0, 0 }
	};

/* penalty pour directions preferencielles */
#define PN 20
static int dir_penalty_TOP[10][10] = {
/* OT=Otherside, OR=Origin (source) cell */
/*......... N, NE,  E, SE,  S, SW,  W, NW,	 OT, OR */
/* N  */ { PN,  0,  0,  0, PN,  0,  0,  0,	  0, 0 },
/* NE */ { PN,  0,  0,  0, PN,  0,  0,  0,	  0, 0 },
/* E  */ { PN,  0,  0,  0, PN,  0,  0,  0,	  0, 0 },
/* SE */ { PN,  0,  0,  0, PN,  0,  0,  0,	  0, 0 },
/* S  */ { PN,  0,  0,  0, PN,  0,  0,  0,	  0, 0 },
/* SW */ { PN,  0,  0,  0, PN,  0,  0,  0,	  0, 0 },
/* W  */ { PN,  0,  0,  0, PN,  0,  0,  0,	  0, 0 },
/* NW */ { PN,  0,  0,  0, PN,  0,  0,  0,	  0, 0 },

/* OT */ { PN,  0,  0,  0, PN,  0,  0,  0,	  0, 0 },
/* OR */ { PN,  0,  0,  0, PN,  0,  0,  0,	  0, 0 }
	};

static int dir_penalty_BOTTOM[10][10] = {
/* OT=Otherside, OR=Origin (source) cell */
/*......... N, NE,  E, SE,  S, SW,  W, NW,	 OT, OR */
/* N  */ {  0,  0, PN,  0,  0,  0, PN,  0,	  0, 0 },
/* NE */ {  0,  0, PN,  0,  0,  0, PN,  0,	  0, 0 },
/* E  */ {  0,  0, PN,  0,  0,  0, PN,  0,	  0, 0 },
/* SE */ {  0,  0, PN,  0,  0,  0, PN,  0,	  0, 0 },
/* S  */ {  0,  0, PN,  0,  0,  0, PN,  0,	  0, 0 },
/* SW */ {  0,  0, PN,  0,  0,  0, PN,  0,	  0, 0 },
/* W  */ {  0,  0, PN,  0,  0,  0, PN,  0,	  0, 0 },
/* NW */ {  0,  0, PN,  0,  0,  0, PN,  0,	  0, 0 },

/* OT */ {  0,  0, PN,  0,  0,  0, PN,  0,	  0, 0 },
/* OR */ {  0,  0, PN,  0,  0,  0, PN,  0,	  0, 0 }
	};

/*
** x is the direction to enter the cell of interest.
** y is the direction to exit the cell of interest.
** z is the direction to really exit the cell, if y=FROM_OTHERSIDE.
**
** return the distance of the trace through the cell of interest.
** the calculation is driven by the tables above.
*/

	/************************************/
	/* int CalcDist(int x,int y,int z ) */
	/************************************/

 /* calculate distance of a trace through a cell */
int CalcDist(int x,int y,int z ,int side )
{
int adjust, ldist;

	adjust = 0; /* set if hole is encountered */
	if (x == EMPTY) x = 10;
	if (y == EMPTY) y = 10;
	else if (y == FROM_OTHERSIDE)
		{
		if (z == EMPTY) z = 10;
		adjust = penalty[x-1][z-1];
		}
	ldist = dist[x-1][y-1] + penalty[x-1][y-1] + adjust ;

	if(Nb_Sides)
		{
		if(side == BOTTOM) ldist += dir_penalty_TOP[x-1][y-1];
		if(side == TOP) ldist += dir_penalty_BOTTOM[x-1][y-1];
		}
	return(ldist * 10);
}


