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
/* routine de trace du pcb, avec selection des couches */
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
    DisplayOpt.DisplayZones = 1;
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

    // Draw tracks and flashes
    Draw_Track_Buffer( DrawPanel, DC, m_Pcb, draw_mode, printmasklayer );

    // Draw filled polygons
    #define NBMAX 20000
    int    nbpoints    = 0;
    int    nbpointsmax = NBMAX;
    int*   coord   = (int*) malloc( nbpointsmax * sizeof(int) * 2 );
    int*   ptcoord = coord;

    for( TRACK* track = m_Pcb->m_Zone;  track;  track = track->Next() )
    {
        if( printmasklayer != -1  &&  !(track->ReturnMaskLayer() & printmasklayer) )
            continue;

        if( track->GetNet() == 0 )  // StartPoint
        {
            if( nbpoints )			// we have found a new polygon: Draw the old polygon
            {
                int Color = g_DesignSettings.m_LayerColor[track->GetLayer()];
                int filled = (g_DisplayPolygonsModeSketch == 0) ? 1 : 0;

                GRClosedPoly( &DrawPanel->m_ClipBox, DC, nbpoints, coord,
                              filled, Color, Color );
            }

            nbpoints = 2;
            ptcoord  = coord;

            *ptcoord++ = track->m_Start.x;
            *ptcoord++ = track->m_Start.y;

            *ptcoord++ = track->m_End.x;
            *ptcoord++ = track->m_End.y;
        }
        else
        {
            if( nbpoints >= nbpointsmax )
            {
                nbpointsmax *= 2;
                coord   = (int*) realloc( coord, nbpointsmax * sizeof(int) * 2 );
                ptcoord = coord + nbpointsmax;
            }
            nbpoints++;

            *ptcoord++ = track->m_End.x;
            *ptcoord++ = track->m_End.y;
        }

        if( track->Next() == NULL )    // Last point
        {
            int Color = g_DesignSettings.m_LayerColor[track->GetLayer()];
            int filled = (g_DisplayPolygonsModeSketch == 0) ? 1 : 0;

            GRClosedPoly( &DrawPanel->m_ClipBox, DC, nbpoints, coord,
                          filled, Color, Color );
        }
    }

    free( coord );

    if( DisplayOpt.DisplayPadNum )
        Affiche_DCodes_Pistes( DrawPanel, DC, m_Pcb, GR_COPY );

    GetScreen()->ClrRefreshReq();
}
