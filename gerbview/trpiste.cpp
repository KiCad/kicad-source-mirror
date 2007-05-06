		/*****************************************************************/
		/* Routines de tracage des pistes ( Toutes, 1 piste, 1 segment ) */
		/*****************************************************************/

#include "fctsys.h"

#include "common.h"
#include "gerbview.h"
#include "pcbplot.h"
#include "trigo.h"

#include "protos.h"

/* Definition des cas ou l'on force l'affichage en SKETCH (membre .flags) */
#define FORCE_SKETCH (DRAG | EDIT )

/* variables locales : */

/********************************************************************************/
void Trace_Pistes(WinEDA_DrawPanel * panel, wxDC * DC, BOARD * Pcb, int drawmode)
/********************************************************************************/
/* Routine de trace des pistes et zones */
{
TRACK * pt_piste;
int layer = ((PCB_SCREEN*)panel->GetScreen())->m_Active_Layer;
GERBER_Descr * gerber_layer	= g_GERBER_Descr_List[layer];
int dcode_hightlight = 0;

	if ( gerber_layer )
		dcode_hightlight = gerber_layer->m_Selected_Tool;

	pt_piste = Pcb->m_Track;
	for ( ; pt_piste != NULL ; pt_piste = (TRACK*) pt_piste->Pnext )
	{
		if ( (dcode_hightlight == pt_piste->m_NetCode) &&
			 (pt_piste->m_Layer == layer) )
			Trace_Segment(panel, DC, pt_piste, drawmode | GR_SURBRILL);
		else Trace_Segment(panel, DC, pt_piste, drawmode );
	}
}


/***********************************************************************************/
void Trace_Segment(WinEDA_DrawPanel * panel, wxDC * DC, TRACK* track, int draw_mode)
/***********************************************************************************/
/* routine de trace de 1 segment de piste.
Parametres :
	pt_piste = adresse de la description de la piste en buflib
	draw_mode = mode ( GR_XOR, GR_OR..)
*/
{
int l_piste;
int color;
int zoom;
int rayon;
int fillopt;
static bool show_err;

	color = g_DesignSettings.m_LayerColor[track->m_Layer];
	if(color & ITEM_NOT_SHOW ) return ;

	zoom = panel->GetZoom();

	GRSetDrawMode(DC, draw_mode);
	if( draw_mode & GR_SURBRILL)
		{
		if( draw_mode & GR_AND)	color &= ~HIGHT_LIGHT_FLAG;
		else color |= HIGHT_LIGHT_FLAG;
		}
	if ( color & HIGHT_LIGHT_FLAG)
		color = ColorRefs[color & MASKCOLOR].m_LightColor;

	rayon = l_piste = track->m_Width >> 1;

	fillopt = DisplayOpt.DisplayPcbTrackFill ? FILLED : SKETCH;

	switch (track->m_Shape)
		{
		case S_CIRCLE:
			rayon = (int)hypot((double)(track->m_End.x-track->m_Start.x),
						(double)(track->m_End.y-track->m_Start.y) );
			if ( (l_piste/zoom) < L_MIN_DESSIN)
				{
				GRCircle(&panel->m_ClipBox, DC, track->m_Start.x, track->m_Start.y,
					rayon , color) ;
				}

			if( fillopt == SKETCH)
				{
				GRCircle(&panel->m_ClipBox, DC, track->m_Start.x, track->m_Start.y,
						rayon-l_piste, color);
				GRCircle(&panel->m_ClipBox, DC, track->m_Start.x, track->m_Start.y,
						rayon+l_piste, color);
				}
			else
				{
				GRCircle(&panel->m_ClipBox, DC, track->m_Start.x, track->m_Start.y,
					rayon, track->m_Width,color);
				}
			break;

		case S_ARC:
			{
			if( fillopt == SKETCH)
				{
				GRArc1(&panel->m_ClipBox, DC, track->m_Start.x, track->m_Start.y,
						track->m_End.x, track->m_End.y,
						track->m_Param, track->m_Sous_Netcode, color);
				}
			else
				{
				GRArc1(&panel->m_ClipBox, DC, track->m_Start.x, track->m_Start.y,
						track->m_End.x, track->m_End.y,
						track->m_Param,track->m_Sous_Netcode,
						track->m_Width, color);
				}
			}
			break;

		case S_SPOT_CIRCLE:
			fillopt = DisplayOpt.DisplayPadFill ? FILLED : SKETCH;
			if ( (rayon/zoom) < L_MIN_DESSIN)
				{
				GRCircle(&panel->m_ClipBox, DC, track->m_Start.x, track->m_Start.y,
					rayon , color) ;
				}

			else if( fillopt == SKETCH )
				{
				GRCircle(&panel->m_ClipBox, DC, track->m_Start.x, track->m_Start.y,
									rayon, color);
				}
			else
				{
				GRFilledCircle(&panel->m_ClipBox, DC, track->m_Start.x, track->m_Start.y,
						rayon, color, color);
				}
			break;

		case  S_SPOT_RECT:
		case  S_RECT:
			fillopt = DisplayOpt.DisplayPadFill ? FILLED : SKETCH;
			if ( (l_piste/zoom) < L_MIN_DESSIN)
				{
				GRLine(&panel->m_ClipBox, DC, track->m_Start.x, track->m_Start.y,
						 track->m_End.x, track->m_End.y, color);
				}
			else if( fillopt == SKETCH )
				{
				GRRect(&panel->m_ClipBox, DC,
						track->m_Start.x - l_piste,
						track->m_Start.y - l_piste,
						track->m_End.x + l_piste,
						track->m_End.y + l_piste,
						color) ;
				}
			else
				{
				GRFilledRect(&panel->m_ClipBox, DC,
						track->m_Start.x - l_piste,
						track->m_Start.y - l_piste,
						track->m_End.x + l_piste,
						track->m_End.y + l_piste,
						color, color) ;
				}
			break;

		case  S_SPOT_OVALE:
			fillopt = DisplayOpt.DisplayPadFill ? FILLED : SKETCH;
		case  S_SEGMENT:
			if ( (l_piste/zoom) < L_MIN_DESSIN)
				{
				GRLine(&panel->m_ClipBox, DC, track->m_Start.x, track->m_Start.y,
					 track->m_End.x, track->m_End.y, color);
				break;
				}

			if( fillopt == SKETCH )
				{
				GRCSegm(&panel->m_ClipBox, DC, track->m_Start.x, track->m_Start.y,
						track->m_End.x, track->m_End.y,
				track->m_Width, color) ;
				}
			else
				{
				GRFillCSegm(&panel->m_ClipBox, DC, track->m_Start.x, track->m_Start.y,
						track->m_End.x, track->m_End.y,
						track->m_Width, color) ;
				}
			break;

		default:
			if ( ! show_err )
				{
				DisplayError(panel, wxT("Trace_Segment() type error"));
				show_err = TRUE;
				}
			break;
		}
}

/**************************************************************************/
void Trace_DrawSegmentPcb(WinEDA_DrawPanel * panel, wxDC * DC,
		DRAWSEGMENT * PtDrawSegment, int draw_mode)
/**************************************************************************/
/* Affichage d'un segment type drawing PCB:
	Entree : ox, oy = offset de trace
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
int zoom = panel->GetZoom();
int rayon;

	color = g_DesignSettings.m_LayerColor[PtDrawSegment->m_Layer];
	if(color & ITEM_NOT_SHOW ) return ;

	GRSetDrawMode(DC, draw_mode);
	l_piste = PtDrawSegment->m_Width >> 1;  /* l_piste = demi largeur piste */

	/* coord de depart */
	ux0 = PtDrawSegment->m_Start.x;
	uy0 = PtDrawSegment->m_Start.y;
	/* coord d'arrivee */
	dx = PtDrawSegment->m_End.x;
	dy = PtDrawSegment->m_End.y;


	mode = DisplayOpt.DisplayPcbTrackFill ? FILLED : SKETCH;
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
			if( mode == FILAIRE)
				GRLine(&panel->m_ClipBox, DC, ux0, uy0, dx, dy, color) ;
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


/*****************************************************************************************/
void Affiche_DCodes_Pistes(WinEDA_DrawPanel * panel, wxDC * DC, BOARD * Pcb, int drawmode)
/*****************************************************************************************/
{
TRACK * track;
wxPoint pos;
int width, orient;
wxString Line;

	GRSetDrawMode(DC, drawmode);
	track = Pcb->m_Track;
	for ( ; track != NULL ; track = (TRACK*) track->Pnext )
		{
		if ( (track->m_Shape == S_ARC) ||
			 (track->m_Shape == S_CIRCLE) ||
			 (track->m_Shape == S_ARC_RECT) )
			{
			pos.x = track->m_Start.x;
			pos.y = track->m_Start.y;
			}

		else
			{
			pos.x = (track->m_Start.x + track->m_End.x) / 2;
			pos.y = (track->m_Start.y + track->m_End.y) / 2;
			}
		Line.Printf( wxT("D%d"), track->m_NetCode);
		width = track->m_Width;
		orient = TEXT_ORIENT_HORIZ;
		if ( track->m_Shape >= S_SPOT_CIRCLE)	// forme flash
			{
			width /= 3;
			}

		else		// lines
			{
			int dx, dy;
			dx = track->m_Start.x - track->m_End.x;
			dy = track->m_Start.y - track->m_End.y;
			if ( abs(dx) < abs(dy) ) orient = TEXT_ORIENT_VERT;
			width /= 2;
			}

		DrawGraphicText(panel, DC,
				pos, g_DCodesColor, Line,
				orient, wxSize(width, width),
				GR_TEXT_HJUSTIFY_CENTER, GR_TEXT_VJUSTIFY_CENTER);
		}
}

