	/****************************************************************/
	/* Routines generales d'affichage des curseurs et des marqueurs */
	/****************************************************************/

	/* fichier curseurs.cpp */

#include "fctsys.h"
#include "gr_basic.h"

#include "common.h"
#include "pcbnew.h"


/* Routines Locales : */


	/* Forme (bit_map) du marqueur */
static char Default_MarkerBitmap[]=
	{
	12, 12,					/* Dimensions x et y du dessin de marqueurs*/
	1,1,1,1,1,1,1,1,0,0,0,0,	/* bitmap: 1 = color, 0 = notrace */
	1,1,1,0,1,0,1,1,0,0,0,0,
	1,1,1,1,0,0,0,1,0,0,0,0,
	1,0,1,1,1,0,0,0,0,0,0,0,
	1,1,0,1,1,1,0,0,0,0,0,0,
	1,1,0,0,1,1,1,0,0,0,0,0,
	1,1,1,0,0,1,1,1,0,0,0,0,
	0,0,0,0,0,0,1,1,1,0,0,0,
	0,0,0,0,0,0,0,1,1,1,0,0,
	0,0,0,0,0,0,0,0,1,1,1,0,
	0,0,0,0,0,0,0,0,0,1,1,1,
	0,0,0,0,0,0,0,0,0,0,1,0
	};


/**********************************************************************/
void MARQUEUR::Draw( WinEDA_DrawPanel * panel, wxDC * DC, int DrawMode)
/**********************************************************************/
/*
 Trace un repere sur l'ecran au point de coordonnees PCB pos
	Le marqueur est defini par un tableau de 2 + (lig*col) elements:
	 1er element: dim nbre ligne
	 2er element: dim nbre col
	 suite: lig * col elements a 0 ou 1 : si 1 mise a color du pixel
*/
{
int px, py;
int ii, ii_max, jj, jj_max;
char * pt_bitmap = m_Bitmap;
	
	if ( pt_bitmap == NULL ) pt_bitmap = Default_MarkerBitmap;

	GRSetDrawMode(DC, DrawMode);

	px = GRMapX(m_Pos.x); py = GRMapY(m_Pos.y);

	/* Lecture des dimensions */
	ii_max = *(pt_bitmap++); jj_max = *(pt_bitmap++);

	/* Trace du bitmap */
	for( ii = 0; ii < ii_max; ii++)
		{
		for( jj = 0; jj < jj_max; jj++, pt_bitmap++)
			{
			if(*pt_bitmap) GRSPutPixel(&panel->m_ClipBox, DC,
					px+ii , py+jj , m_Color);
			}
		}
}

