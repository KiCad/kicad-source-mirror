/*****************************************/
/* Routines generales d'affichage du PCB */
/*****************************************/

/* fichier TRACEPCB.CPP */

/*
 *  Routines d'affichage grille, Boite de coordonnees, Curseurs, marqueurs ...
 */

#include "fctsys.h"

#include "common.h"
#include "gerbview.h"
#include "pcbplot.h"

#include "protos.h"


/* Externes */

/* Variables Locales */


/************************************************************************************************************/
void WinEDA_DrawPanel::PrintPage( wxDC* DC, bool Print_Sheet_Ref, int printmasklayer, bool aPrintMirrorMode )
/*************************************************************************************************************/
/* Draw gerbview layers, for printing
*/
{
	DISPLAY_OPTIONS save_opt;
	int DisplayPolygonsModeImg;

	save_opt = DisplayOpt;
	if( printmasklayer & ALL_CU_LAYERS )
		DisplayOpt.DisplayPadFill = FILLED;
	else
		DisplayOpt.DisplayPadFill = SKETCH;
	DisplayOpt.DisplayPadNum       = 0;
	DisplayOpt.DisplayPadNoConn    = 0;
	DisplayOpt.DisplayPadIsol      = 0;
	DisplayOpt.DisplayModEdge      = FILLED;
	DisplayOpt.DisplayModText      = FILLED;
	DisplayOpt.DisplayPcbTrackFill = FILLED;
	DisplayOpt.DisplayTrackIsol    = 0;
	DisplayOpt.DisplayDrawItems    = FILLED;
	DisplayOpt.DisplayZonesMode = 0;
	DisplayPolygonsModeImg = g_DisplayPolygonsModeSketch;
	g_DisplayPolygonsModeSketch = 0;

	m_PrintIsMirrored = aPrintMirrorMode;

	( (WinEDA_GerberFrame*) m_Parent )->Trace_Gerber( DC, GR_COPY, printmasklayer );

	if( Print_Sheet_Ref )
		m_Parent->TraceWorkSheet( DC, GetScreen(), 0 );

	m_PrintIsMirrored = false;

	DisplayOpt = save_opt;
	g_DisplayPolygonsModeSketch = DisplayPolygonsModeImg;
}


/*******************************************************************/
void WinEDA_GerberFrame::RedrawActiveWindow( wxDC* DC, bool EraseBg )
/*******************************************************************/

/* Trace le PCB, et les elements complementaires ( axes, grille .. )
 */
{
	PCB_SCREEN* screen = (PCB_SCREEN*)GetScreen();

	if( !m_Pcb )
		return;
	ActiveScreen = screen;
	GRSetDrawMode( DC, GR_COPY );

	if( EraseBg )
		DrawPanel->EraseScreen( DC );

	DrawPanel->DrawBackGround( DC );

	Trace_Gerber( DC, GR_COPY, -1 );
	TraceWorkSheet( DC, screen, 0 );
	Affiche_Status_Box();

	if( DrawPanel->ManageCurseur )
		DrawPanel->ManageCurseur( DrawPanel, DC, FALSE );

	DrawPanel->Trace_Curseur( DC );
}

/********************************************************************/
void BOARD::Draw( WinEDA_DrawPanel* aPanel, wxDC* DC,
				  int aDrawMode, const wxPoint& offset )
/********************************************************************/
/* Redraw the BOARD items but not cursors, axis or grid */
// @todo: replace WinEDA_GerberFrame::Trace_Gerber() by this function
{
}

/***********************************************************************************/
void WinEDA_GerberFrame::Trace_Gerber( wxDC* DC, int draw_mode, int printmasklayer )
/***********************************************************************************/
/*
* Trace l'ensemble des elements du PCB sur l'ecran actif
* @param DC = device context to draw
* @param draw_mode = draw mode for the device context (GR_COPY, GR_OR, GR_XOR ..)
* @param printmasklayer = mask for allowed layer (=-1 to draw all layers)
*/
{
	if( !m_Pcb )
		return;

    bool    erase;
    
	// Draw filled polygons
    std::vector<wxPoint>    coords;

    // minimize reallocations of the vector's internal array by starting with a good sized one.
    coords.reserve(20000);
    
	for( TRACK* track = m_Pcb->m_Zone;  track;  track = track->Next() )
	{
		if( !(track->ReturnMaskLayer() & printmasklayer) )
			continue;

		if( track->GetNet() == 0 )  // StartPoint
		{
			if( coords.size() )			// we have found a new polygon: Draw the old polygon
			{
				int Color;
				int filled;
                
                if( erase )
                {
                    D(printf("erase\n");)
                    Color = g_DrawBgColor;
                    filled = true;
                }
                else
                {
                    D(printf("NO erase\n");)
                    Color = g_DesignSettings.m_LayerColor[track->GetLayer()];
                    filled = (g_DisplayPolygonsModeSketch == 0) ? 1 : 0;
                }

				GRClosedPoly( &DrawPanel->m_ClipBox, DC, coords.size(), &coords[0],
							  filled, Color, Color );
			}

            erase = ( track->m_Flags & DRAW_ERASED ) ? true : false;
    
            coords.clear();
			coords.push_back( track->m_Start );
            coords.push_back( track->m_End );
		}
		else
		{
			coords.push_back( track->m_End );
		}

		if( track->Next() == NULL )    // Last point
		{
			int Color = g_DesignSettings.m_LayerColor[track->GetLayer()];
			int filled = (g_DisplayPolygonsModeSketch == 0) ? 1 : 0;

			GRClosedPoly( &DrawPanel->m_ClipBox, DC, coords.size(), &coords[0],
						  filled, Color, Color );
		}
	}

	// Draw tracks and flashes down here.  This will probably not be a final solution to drawing order issues
	Draw_Track_Buffer( DrawPanel, DC, m_Pcb, draw_mode, printmasklayer );

	if( DisplayOpt.DisplayPadNum )
		Affiche_DCodes_Pistes( DrawPanel, DC, m_Pcb, GR_COPY );

	GetScreen()->ClrRefreshReq();
}
