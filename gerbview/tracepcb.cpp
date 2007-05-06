		/*****************************************/
		/* Routines generales d'affichage du PCB */
		/*****************************************/

	/* fichier TRACEPCB.CPP */
/*
 Routines d'affichage grille, Boite de coordonnees, Curseurs, marqueurs ...
*/

#include "fctsys.h"

#include "common.h"
#include "gerbview.h"
#include "pcbplot.h"

#include "protos.h"


/* Externes */

/* Variables Locales */


/**********************************************************************************/
void WinEDA_DrawPanel::PrintPage(wxDC *DC, bool Print_Sheet_Ref, int printmasklayer)
/**********************************************************************************/
/* routine de trace du pcb, avec selection des couches */
{
DISPLAY_OPTIONS save_opt;

	save_opt = DisplayOpt;
	if( printmasklayer & ALL_CU_LAYERS ) DisplayOpt.DisplayPadFill = FILLED;
	else DisplayOpt.DisplayPadFill = SKETCH;
	DisplayOpt.DisplayPadNum = 0;
	DisplayOpt.DisplayPadNoConn = 0;
	DisplayOpt.DisplayPadIsol = 0;
	DisplayOpt.DisplayModEdge = FILLED;
	DisplayOpt.DisplayModText = FILLED;
	DisplayOpt.DisplayPcbTrackFill = FILLED;
	DisplayOpt.DisplayTrackIsol = 0;
	DisplayOpt.DisplayDrawItems = FILLED;
	DisplayOpt.DisplayZones = 1;

	((WinEDA_GerberFrame*)m_Parent)->Trace_Gerber(DC, GR_COPY);

	if ( Print_Sheet_Ref )
		m_Parent->TraceWorkSheet(DC, GetScreen());

	DisplayOpt = save_opt;
}

/*******************************************************************/
void WinEDA_GerberFrame::RedrawActiveWindow(wxDC * DC, bool EraseBg)
/*******************************************************************/
/* Trace le PCB, et les elements complementaires ( axes, grille .. )
*/
{
PCB_SCREEN * screen = GetScreen();
	
	if ( ! m_Pcb ) return;
	ActiveScreen = screen;
	GRSetDrawMode(DC, GR_COPY);

	if ( EraseBg ) DrawPanel->EraseScreen(DC);

	DrawPanel->DrawBackGround(DC);

	Trace_Gerber(DC, GR_OR);
	TraceWorkSheet(DC, screen);
	Affiche_Status_Box();

	if( DrawPanel->ManageCurseur )
		DrawPanel->ManageCurseur(DrawPanel, DC, FALSE);

	DrawPanel->Trace_Curseur(DC);
}

/********************************************************/
void WinEDA_GerberFrame::Trace_Gerber(wxDC * DC, int mode)
/********************************************************/
/* Trace l'ensemble des elements du PCB sur l'ecran actif*/
{

	if ( ! m_Pcb ) return;
		
	// Draw tracks and flashes
	Trace_Pistes(DrawPanel, DC, m_Pcb, mode);

	// Draw filled polygons
	#define NBMAX 2000
	TRACK * track;
	int nbpoints = 0;
	int nbpointsmax = NBMAX;
	int * coord = (int*) malloc(nbpointsmax * sizeof (int) * 2);
	int * ptcoord = coord;
	track = m_Pcb->m_Zone;
	for ( ; track != NULL ; track = track->Next() )
	{
		if ( track->m_NetCode == 0 )	// StartPoint
		{
			if ( nbpoints )
			{
				int Color = g_DesignSettings.m_LayerColor[track->m_Layer];
				GRClosedPoly(&DrawPanel->m_ClipBox, DC, nbpoints, coord,
					1, Color, Color);
			}
			nbpoints = 2;
			ptcoord = coord;
			* ptcoord = track->m_Start.x; ptcoord++;
			* ptcoord = track->m_Start.y; ptcoord++;
			* ptcoord = track->m_End.x; ptcoord++;
			* ptcoord = track->m_End.y; ptcoord++;
		}
		else
		{
			if ( nbpoints >= nbpointsmax )
			{
				nbpointsmax *= 2;
				coord = (int*) realloc(coord, nbpointsmax * sizeof (int) * 2);
				ptcoord = coord + nbpointsmax;
			}
			nbpoints++;
			* ptcoord = track->m_End.x; ptcoord++;
			* ptcoord = track->m_End.y; ptcoord++;
		}
		if ( track->Next() == NULL )	// Last point
		{
			int Color = g_DesignSettings.m_LayerColor[track->m_Layer];
			GRClosedPoly(&DrawPanel->m_ClipBox, DC, nbpoints, coord,
				1, Color, Color);
		}
	}

	free(coord);

	if ( DisplayOpt.DisplayPadNum )
		Affiche_DCodes_Pistes(DrawPanel, DC, m_Pcb, GR_COPY);

	GetScreen()->ClrRefreshReq();
}



