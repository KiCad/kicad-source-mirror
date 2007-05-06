/*****************************************************************/
/* Routines de tracage des pistes ( Toutes, 1 piste, 1 segment ) */
/*****************************************************************/

#include "fctsys.h"
#include "gr_basic.h"

#include "common.h"
#include "pcbnew.h"
#include "trigo.h"

#include "protos.h"

/* variables locales : */

/*********************************************************************************/
void Trace_Pistes(WinEDA_DrawPanel * panel, BOARD * Pcb, wxDC * DC, int drawmode)
/********************************************************************************/
/* Draw all tracks and zones.
*/
{
TRACK * track;

	track = Pcb->m_Track;
	for ( ; track != NULL ; track = track->Next() )
	{
		track->Draw(panel, DC, drawmode);
	}

	track = Pcb->m_Zone;
	for ( ; track != NULL ; track = track->Next() )
	{
		track->Draw(panel, DC, drawmode);
	}
}



/************************************************************************/
void Trace_Une_Piste(WinEDA_DrawPanel * panel, wxDC * DC, TRACK * Track,
		int nbsegment, int draw_mode)
/************************************************************************/
/* routine de trace de n segments consecutifs en memoire.
	Utile pour monter une piste en cours de trace car les segments de cette
	piste sont alors contigus en memoire
Parametres :
	pt_start_piste = adresse de depart de la liste des segments
	nbsegment = nombre de segments a tracer
	draw_mode = mode ( GR_XOR, GR_OR..)
ATTENTION:
	le point de depart d'une piste suivante DOIT exister: peut etre
	donc mis a 0 avant appel a la routine si la piste a tracer est la derniere
*/
{
	if ( Track == NULL ) return;
	for ( ;nbsegment > 0; nbsegment--, Track = (TRACK*)Track->Pnext)
		{
		if ( Track == NULL ) break;
		Track->Draw(panel, DC, draw_mode) ;
		}
}

/*************************************************************/
void Trace_DrawSegmentPcb(WinEDA_DrawPanel * panel, wxDC * DC,
		DRAWSEGMENT * PtDrawSegment, int draw_mode)
/*************************************************************/
/* Affichage d'un segment type drawing PCB:
	draw_mode = mode de trace ( GR_OR, GR_XOR, GrAND)
		Les contours sont de differents type:
		segment
		cercle
		arc
*/
{
int ux0, uy0, dx, dy;
int l_piste;
int color, mode;
int zoom;
int rayon;

	color = g_DesignSettings.m_LayerColor[PtDrawSegment->m_Layer];
	if(color & ITEM_NOT_SHOW ) return ;

	if ( panel ) zoom = panel->GetZoom();
	else zoom = ActiveScreen->GetZoom();

	GRSetDrawMode(DC, draw_mode);
	l_piste = PtDrawSegment->m_Width >> 1;  /* l_piste = demi largeur piste */

	/* coord de depart */
	ux0 = PtDrawSegment->m_Start.x;
	uy0 = PtDrawSegment->m_Start.y;
	/* coord d'arrivee */
	dx = PtDrawSegment->m_End.x;
	dy = PtDrawSegment->m_End.y;


	mode = DisplayOpt.DisplayDrawItems;
	if(PtDrawSegment->m_Flags & FORCE_SKETCH) mode = SKETCH;
	if ( l_piste < (L_MIN_DESSIN * zoom) ) mode = FILAIRE;

	switch (PtDrawSegment->m_Shape)
		{
		case S_CIRCLE:
			rayon = (int)hypot((double)(dx-ux0),(double)(dy-uy0) );
			if ( mode == FILAIRE)
				{
				GRCircle(&panel->m_ClipBox, DC, ux0, uy0, rayon, color) ;
				}
			else if( mode == SKETCH)
				{
				GRCircle(&panel->m_ClipBox, DC, ux0, uy0, rayon-l_piste, color);
				GRCircle(&panel->m_ClipBox, DC, ux0, uy0, rayon+l_piste, color);
				}
			else
				{
				GRCircle(&panel->m_ClipBox, DC, ux0, uy0, rayon, PtDrawSegment->m_Width,color);
				}
			break;

		case S_ARC:
			{
			int StAngle, EndAngle;
			rayon = (int)hypot((double)(dx-ux0),(double)(dy-uy0) );
			StAngle = (int) ArcTangente(dy-uy0, dx-ux0);
			EndAngle = StAngle + PtDrawSegment->m_Angle;
			if ( StAngle > EndAngle) EXCHG (StAngle, EndAngle);
			if ( mode == FILAIRE)
				GRArc(&panel->m_ClipBox, DC, ux0, uy0, StAngle, EndAngle, rayon, color);
			else if( mode == SKETCH)
				{
				GRArc(&panel->m_ClipBox, DC, ux0, uy0, StAngle, EndAngle,
					rayon - l_piste, color);
				GRArc(&panel->m_ClipBox, DC, ux0, uy0, StAngle, EndAngle,
					rayon + l_piste, color);
				}
			else
				{
				GRArc(&panel->m_ClipBox, DC, ux0, uy0, StAngle, EndAngle,
						rayon, PtDrawSegment->m_Width,color);
				}
			}
			break;

		default:
			if( mode == FILAIRE) GRLine(&panel->m_ClipBox, DC, ux0, uy0, dx, dy, color) ;
			else if( mode == SKETCH)
				{
				GRCSegm(&panel->m_ClipBox, DC, ux0, uy0, dx, dy,
						PtDrawSegment->m_Width, color) ;
				}
			else
				{
				GRFillCSegm(&panel->m_ClipBox, DC, ux0, uy0, dx, dy,
						 PtDrawSegment->m_Width, color) ;
				}
			break;
		}
}

