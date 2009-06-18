/*****************************************/
/* Routines generales d'affichage du PCB */
/*****************************************/

/* fichier TRACEPCB.CPP */

/*
 *  Routines d'affichage grille, Boite de coordonnees, Curseurs, marqueurs ...
 */

#include "fctsys.h"
#include "gr_basic.h"
#include "common.h"
#include "class_drawpanel.h"

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
        DisplayOpt.DisplayPadFill  = true;
    else
        DisplayOpt.DisplayPadFill  = false;
    DisplayOpt.DisplayPadNum       = 0;
    DisplayOpt.DisplayPadNoConn    = 0;
    DisplayOpt.DisplayPadIsol      = 0;
    DisplayOpt.DisplayModEdge      = FILLED;
    DisplayOpt.DisplayModText      = FILLED;
    DisplayOpt.DisplayPcbTrackFill = FILLED;
    DisplayOpt.ShowTrackClearanceMode = DO_NOT_SHOW_CLEARANCE;
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

    if( !GetBoard() )
        return;
    ActiveScreen = screen;
    GRSetDrawMode( DC, GR_COPY );

    if( EraseBg )
        DrawPanel->EraseScreen( DC );

    DrawPanel->DrawBackGround( DC );

    Trace_Gerber( DC, GR_COPY, -1 );
    TraceWorkSheet( DC, screen, 0 );
    UpdateStatusBar();

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
    if( !GetBoard() )
        return;

    bool    erase = false;
    int     Color;
    bool    filled;

    // Draw filled polygons
    std::vector<wxPoint>    points;

    // minimize reallocations of the vector's internal array by starting with a good sized one.
    points.reserve(10000);

    for( TRACK* track = GetBoard()->m_Zone;  track;  track = track->Next() )
    {
        if( !(track->ReturnMaskLayer() & printmasklayer) )
            continue;

        D(printf("D:%p\n", track );)

        if( track->GetNet() == 0 )  // StartPoint
        {
            if( points.size() )     // we have found a new polygon: Draw the old polygon
            {
                if( erase )
                {
                    Color = g_DrawBgColor;
                    filled = true;
                }
                else
                {
                    Color = g_DesignSettings.m_LayerColor[track->GetLayer()];
                    filled = (g_DisplayPolygonsModeSketch == 0);
                }

                GRClosedPoly( &DrawPanel->m_ClipBox, DC, points.size(), &points[0],
                              filled, Color, Color );
            }

            erase = ( track->m_Flags & DRAW_ERASED );

            points.clear();
            points.push_back( track->m_Start );
            points.push_back( track->m_End );
        }
        else
        {
            points.push_back( track->m_End );
        }

        if( track->Next() == NULL )    // Last point
        {
            if( erase )
            {
                Color = g_DrawBgColor;
                filled = true;
            }
            else
            {
                Color = g_DesignSettings.m_LayerColor[track->GetLayer()];
                filled = (g_DisplayPolygonsModeSketch == 0);
            }

            GRClosedPoly( &DrawPanel->m_ClipBox, DC, points.size(), &points[0],
                          filled, Color, Color );
        }
    }

    // Draw tracks and flashes down here.  This will probably not be a final solution to drawing order issues
    Draw_Track_Buffer( DrawPanel, DC, GetBoard(), draw_mode, printmasklayer );

    if( DisplayOpt.DisplayPadNum )
        Affiche_DCodes_Pistes( DrawPanel, DC, GetBoard(), GR_COPY );

    GetScreen()->ClrRefreshReq();
}
