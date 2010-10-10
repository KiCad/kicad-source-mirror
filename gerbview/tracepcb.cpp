/****************/
/* tracepcb.cpp */
/****************/

/*
 *  Redraw the screen.
 */

#include "fctsys.h"
#include "gr_basic.h"
#include "common.h"
#include "class_drawpanel.h"
#include "drawtxt.h"

#include "gerbview.h"
#include "class_board_design_settings.h"
#include "colors_selection.h"
#include "class_gerber_draw_item.h"
#include "class_GERBER.h"

static void Show_Items_DCode_Value( WinEDA_DrawPanel* panel, wxDC* DC,
                            BOARD* Pcb, int drawmode );

/** virtual Function PrintPage
 * Used to print the board (on printer, or when creating SVF files).
 * Print the board, but only layers allowed by aPrintMaskLayer
 * @param aDC = the print device context
 * @param aPrint_Sheet_Ref = true to print frame references
 * @param aPrint_Sheet_Ref = a 32 bits mask: bit n = 1 -> layer n is printed
 * @param aPrintMirrorMode = true to plot mirrored
 * @param aData = a pointer to an optional data (not used here: can be NULL)
 */
void WinEDA_GerberFrame::PrintPage( wxDC* aDC, bool aPrint_Sheet_Ref, int aPrintMasklayer,
                                bool aPrintMirrorMode, void * aData )
{
    DISPLAY_OPTIONS save_opt;
    int DisplayPolygonsModeImg;

    save_opt = DisplayOpt;

    DisplayOpt.DisplayPcbTrackFill = FILLED;
    DisplayOpt.ShowTrackClearanceMode = DO_NOT_SHOW_CLEARANCE;
    DisplayOpt.DisplayDrawItems    = FILLED;
    DisplayOpt.DisplayZonesMode = 0;

    DisplayPolygonsModeImg = g_DisplayPolygonsModeSketch;
    g_DisplayPolygonsModeSketch = 0;

    DrawPanel->m_PrintIsMirrored = aPrintMirrorMode;

    Trace_Gerber( aDC, GR_COPY, aPrintMasklayer );

    if( aPrint_Sheet_Ref )
        TraceWorkSheet( aDC, GetScreen(), 0 );

    DrawPanel->m_PrintIsMirrored = false;

    DisplayOpt = save_opt;
    g_DisplayPolygonsModeSketch = DisplayPolygonsModeImg;
}


/*******************************************************************/
void WinEDA_GerberFrame::RedrawActiveWindow( wxDC* DC, bool EraseBg )
/*******************************************************************/
/* Redraws the full screen, including axis and grid
 */
{
    PCB_SCREEN* screen = (PCB_SCREEN*)GetScreen();

    if( !GetBoard() )
        return;
    ActiveScreen = screen;
    GRSetDrawMode( DC, GR_COPY );

    DrawPanel->DrawBackGround( DC );

    //buid mask layer :
    int masklayer = 0;
    for( int layer = 0; layer < 32; layer++ )
        if( GetBoard()->IsLayerVisible( layer ) )
            masklayer |= 1 << layer;

    Trace_Gerber( DC, GR_COPY, masklayer );
    TraceWorkSheet( DC, screen, 0 );

    if( DrawPanel->ManageCurseur )
        DrawPanel->ManageCurseur( DrawPanel, DC, FALSE );

    DrawPanel->DrawCursor( DC );

    // Display the filename and the layer name (found in the gerber files, if any)
    // relative to the active layer
    UpdateTitleAndInfo();
}

/********************************************************************/
void BOARD::Draw( WinEDA_DrawPanel* aPanel, wxDC* aDC, int aDrawMode, const wxPoint& aOffset )
/********************************************************************/
/* Redraw the BOARD items but not cursors, axis or grid */
// @todo: replace WinEDA_GerberFrame::Trace_Gerber() by this function
{
}


/***********************************************************************************/
void WinEDA_GerberFrame::Trace_Gerber( wxDC* aDC, int aDraw_mode, int aPrintMasklayer )
/***********************************************************************************/
/* Trace all elements of PCBs (i.e Spots, filled polygons or lines) on the active screen
* @param aDC = current device context
* @param aDraw_mode = draw mode for the device context (GR_COPY, GR_OR, GR_XOR ..)
* @param aPrintMasklayer = mask for allowed layer (=-1 to draw all layers)
*/
{
    if( !GetBoard() )
        return;

    int     layer = GetScreen()->m_Active_Layer;
    GERBER*       gerber = g_GERBER_List[layer];
    int           dcode_hightlight = 0;

    if( gerber )
        dcode_hightlight = gerber->m_Selected_Tool;

    BOARD_ITEM* item = GetBoard()->m_Drawings;
    for( ; item; item = item->Next() )
    {
        GERBER_DRAW_ITEM* gerb_item = (GERBER_DRAW_ITEM*) item;
        if( !(gerb_item->ReturnMaskLayer() & aPrintMasklayer) )
            continue;
        if( dcode_hightlight == gerb_item->m_DCode && item->GetLayer()==layer )
            gerb_item->Draw( DrawPanel, aDC, aDraw_mode | GR_SURBRILL );
        else
            gerb_item->Draw( DrawPanel, aDC, aDraw_mode );
    }

    if( IsElementVisible( DCODES_VISIBLE) )
        Show_Items_DCode_Value( DrawPanel, aDC, GetBoard(), GR_COPY );

    GetScreen()->ClrRefreshReq();
}


/*****************************************************************************************/
void Show_Items_DCode_Value( WinEDA_DrawPanel* aPanel, wxDC* aDC, BOARD* aPcb, int aDrawMode )
/*****************************************************************************************/
{
    wxPoint  pos;
    int      width, orient;
    wxString Line;

    GRSetDrawMode( aDC, aDrawMode );
    BOARD_ITEM* item = aPcb->m_Drawings;
    for( ; item != NULL; item = item->Next() )
    {
        GERBER_DRAW_ITEM* gerb_item = (GERBER_DRAW_ITEM*) item;
        if( aPcb->IsLayerVisible( gerb_item->GetLayer() ) == false )
            continue;
        if( gerb_item->m_DCode <= 0 )
            continue;

        if( gerb_item->m_Flashed )
            pos = gerb_item->m_Start;
        else
        {
            pos.x = (gerb_item->m_Start.x + gerb_item->m_End.x) / 2;
            pos.y = (gerb_item->m_Start.y + gerb_item->m_End.y) / 2;
        }

        pos = gerb_item->GetABPosition( pos );

        Line.Printf( wxT( "D%d" ), gerb_item->m_DCode );

        width  = MIN( gerb_item->m_Size.x, gerb_item->m_Size.y );
        orient = TEXT_ORIENT_HORIZ;
        if( gerb_item->m_Flashed )
        {
            width /= 3;
        }
        else        // lines
        {
            int dx, dy;
            dx = gerb_item->m_Start.x - gerb_item->m_End.x;
            dy = gerb_item->m_Start.y - gerb_item->m_End.y;
            if( abs( dx ) < abs( dy ) )
                orient = TEXT_ORIENT_VERT;
            width /= 2;
        }

        int color = g_ColorsSettings.GetItemColor(DCODES_VISIBLE);

        DrawGraphicText( aPanel, aDC,
                         pos, (EDA_Colors) color, Line,
                         orient, wxSize( width, width ),
                         GR_TEXT_HJUSTIFY_CENTER, GR_TEXT_VJUSTIFY_CENTER,
			  0, false, false, false);
    }
}


/* Virtual fonction needed by the PCB_SCREEN class derived from BASE_SCREEN
* this is a virtual pure function in BASE_SCREEN
* do nothing in gerbview
* could be removed later
*/
void PCB_SCREEN::ClearUndoORRedoList(UNDO_REDO_CONTAINER&, int )
{
}
