		/*************************/
		/* affichage des modules */
		/*************************/

#include "fctsys.h"
#include "gr_basic.h"

#include "common.h"
#include "pcbnew.h"
#include "trigo.h"

#ifdef PCBNEW
#include "drag.h"
#endif

#ifdef CVPCB
#include "cvpcb.h"
#endif

/* Police des caracteres de la routine de trace des textes */
extern char *graphic_fonte_shape[];

#include "protos.h"

#define L_MIN_DESSIN 1 /* seuil de largeur des segments pour trace autre que filaire */

/* fonctions locales : */



/******************************************************************/
void Trace_Pads_Only(WinEDA_DrawPanel * panel, wxDC * DC, MODULE * Module,
						int ox, int oy,
						int MasqueLayer,int draw_mode)
/******************************************************************/

/* Trace les pads d'un module en mode SKETCH.
	Utilisee pour afficher les pastilles d'un module lorsque celui ci n'est
	pas affiche par les options d'affichage des Modules

	Les pads affiches doivent apparaitre sur les couches donnees par
	MasqueLayer
*/
{
int tmp;
D_PAD * pt_pad ;
PCB_SCREEN * screen;
WinEDA_BasePcbFrame * frame;

	screen = (PCB_SCREEN *) panel->GetScreen();
	frame = (WinEDA_BasePcbFrame *) panel->m_Parent;

	tmp = frame->m_DisplayPadFill;
	frame->m_DisplayPadFill = FALSE;

	/* trace des pastilles */
	pt_pad = Module->m_Pads;
	for( ; pt_pad != NULL; pt_pad = (D_PAD*) pt_pad->Pnext )
		{
		if( (pt_pad->m_Masque_Layer & MasqueLayer) == 0 ) continue;
		pt_pad->Draw(panel, DC, wxPoint(ox,oy), draw_mode);
		}

	frame->m_DisplayPadFill = tmp;
}


/***********************************************************************/
void Affiche_1_Segment(WinEDA_DrawPanel * panel, wxDC * DC,
						int ux0, int uy0, int dx, int dy,
						int width, int mode, int color)
/***********************************************************************/
/* Trace un segment origine ux0,uy0, fin en dx, dy,
	largeur l_piste, mode FILL ou SKETCH, en couleur color
	Les coord sont en 1/10000 pouce
*/
{
int zoom = panel->GetZoom();

	if( (width/zoom) < L_MIN_DESSIN )
		GRLine(&panel->m_ClipBox, DC, ux0, uy0, dx, dy, color) ;

	else
		if( mode == SKETCH)
			GRCSegm(&panel->m_ClipBox, DC, ux0, uy0, dx, dy, width, color) ;

		else GRFillCSegm(&panel->m_ClipBox, DC, ux0, uy0, dx, dy, width, color) ;
}

