/************************************************************/
/* print_board_functions.cpp: some functions to plot boards */
/************************************************************/

#include "fctsys.h"
#include "gr_basic.h"
#include "common.h"
#include "class_drawpanel.h"

#include "pcbnew.h"
#include "pcbplot.h"
#include "protos.h"

/* Local functions */
static void Print_Module( WinEDA_DrawPanel* panel, wxDC* DC, MODULE* Module,
                         int draw_mode, int masklayer );


/************************************************************************************************************/
void WinEDA_DrawPanel::PrintPage( wxDC* aDC, bool aPrint_Sheet_Ref, int aPrintMaskLayer, bool aPrintMirrorMode )
/************************************************************************************************************/

/** Function PrintPage
 * Used to print the board (on printer, or when creating SVF files).
 *  Print the board, but only layers allowed by aPrintMaskLayer
 *  ( printmasklayer is a 32 bits mask: bit n = 1 -> layer n is printed)
 */
{
    MODULE*              Module;
    int                  drawmode = GR_COPY;
    DISPLAY_OPTIONS      save_opt;
    TRACK*               pt_piste;
    WinEDA_BasePcbFrame* frame = (WinEDA_BasePcbFrame*) m_Parent;
    BOARD*               Pcb   = frame->GetBoard();

    save_opt = DisplayOpt;
    if( aPrintMaskLayer & ALL_CU_LAYERS )
    {
        DisplayOpt.DisplayPadFill = true;
        DisplayOpt.DisplayViaFill = true;
    }
    else
    {
        DisplayOpt.DisplayPadFill = false;
        DisplayOpt.DisplayViaFill = false;
    }

    frame->m_DisplayPadFill      = DisplayOpt.DisplayPadFill;
    frame->m_DisplayViaFill      = DisplayOpt.DisplayViaFill;
    frame->m_DisplayPadNum       = DisplayOpt.DisplayPadNum = false;
    DisplayOpt.DisplayPadNoConn  = false;
    DisplayOpt.DisplayPadIsol    = false;
    DisplayOpt.DisplayModEdge    = FILLED;
    DisplayOpt.DisplayModText    = FILLED;
    frame->m_DisplayPcbTrackFill = DisplayOpt.DisplayPcbTrackFill = FILLED;
    DisplayOpt.ShowTrackClearanceMode = DO_NOT_SHOW_CLEARANCE;
    DisplayOpt.DisplayDrawItems  = FILLED;
    DisplayOpt.DisplayZonesMode      = 0;
    DisplayOpt.DisplayNetNamesMode  = 0;

    m_PrintIsMirrored = aPrintMirrorMode;

    // The OR mode is used in color mode, but be aware the backgroud *must be BLACK.
    // In print page dialog, we first plrint in BLACK, and after reprint in color,
    // on the black "local" backgroud, in OR mode
    // the black print is not made before, only a white page is printed
    if( GetGRForceBlackPenState( ) == false )
        drawmode = GR_OR;


    /* Print the pcb graphic items (texts, ...) */
    GRSetDrawMode( aDC, drawmode );
    for( BOARD_ITEM* item = Pcb->m_Drawings;  item;  item = item->Next() )
    {
        switch( item->Type() )
        {
        case TYPE_DRAWSEGMENT:
        case TYPE_COTATION:
        case TYPE_TEXTE:
        case TYPE_MIRE:
            if( ((1<<item->GetLayer()) & aPrintMaskLayer) == 0 )
                break;

            item->Draw( this, aDC, drawmode );
            break;

        case TYPE_MARKER_PCB:       /* Trace des marqueurs */
        default:
            break;
        }
    }

    /* Print tracks */
    pt_piste = Pcb->m_Track;
    for( ; pt_piste != NULL; pt_piste = pt_piste->Next() )
    {
        if( ( aPrintMaskLayer & pt_piste->ReturnMaskLayer() ) == 0 )
            continue;
        if( pt_piste->Type() == TYPE_VIA ) /* VIA rencontree */
        {
            int rayon = pt_piste->m_Width >> 1;
            int color = g_DesignSettings.m_ViaColor[pt_piste->m_Shape];
            GRSetDrawMode( aDC, drawmode );
            GRFilledCircle( &m_ClipBox, aDC, pt_piste->m_Start.x, pt_piste->m_Start.y,
                            rayon, 0, color, color );
        }
        else
            pt_piste->Draw( this, aDC, drawmode );
    }

    pt_piste = Pcb->m_Zone;
    for( ; pt_piste != NULL; pt_piste = pt_piste->Next() )
    {
        if( ( aPrintMaskLayer & pt_piste->ReturnMaskLayer() ) == 0 )
            continue;
        pt_piste->Draw( this, aDC, drawmode );
    }


    /* Draw filled areas (i.e. zones) */
    for( int ii = 0; ii < Pcb->GetAreaCount(); ii++ )
    {
        ZONE_CONTAINER* zone = Pcb->GetArea(ii);
        if( ( aPrintMaskLayer & (1 << zone->GetLayer()) ) == 0 )
            continue;

        zone->DrawFilledArea( this, aDC, drawmode );
    }

    // Draw footprints, this is done at last in order to print the pad holes in white (or g_DrawBgColor)
    // after the tracks and zones
    Module = (MODULE*) Pcb->m_Modules;
    for( ; Module != NULL; Module = Module->Next() )
    {
        Print_Module( this, aDC, Module, drawmode, aPrintMaskLayer );
    }

    /* Print via holes in bg color: Not sure it is good for buried or blind vias */
    pt_piste = Pcb->m_Track;
    int color = g_DrawBgColor;
    bool blackpenstate = GetGRForceBlackPenState( );
    GRForceBlackPen( false );
    GRSetDrawMode( aDC, GR_COPY );
    for( ; pt_piste != NULL; pt_piste = pt_piste->Next() )
    {
        if( ( aPrintMaskLayer & pt_piste->ReturnMaskLayer() ) == 0 )
            continue;
        if( pt_piste->Type() == TYPE_VIA ) /* VIA rencontree */
        {
            int rayon = g_DesignSettings.m_ViaDrill / 2;
            GRFilledCircle( &m_ClipBox, aDC, pt_piste->m_Start.x, pt_piste->m_Start.y,
                            rayon, 0, color, color );
        }
    }
    GRForceBlackPen( blackpenstate );

    if( aPrint_Sheet_Ref )
        m_Parent->TraceWorkSheet( aDC, GetScreen(), 10 );

    m_PrintIsMirrored = false;

    DisplayOpt = save_opt;
    frame->m_DisplayPcbTrackFill = DisplayOpt.DisplayPcbTrackFill;
    frame->m_DisplayPadFill = DisplayOpt.DisplayPadFill;
    frame->m_DisplayViaFill = DisplayOpt.DisplayViaFill;
    frame->m_DisplayPadNum  = DisplayOpt.DisplayPadNum;
}


/***********************************************************/
static void Print_Module( WinEDA_DrawPanel* panel, wxDC* DC,
                         MODULE* Module, int draw_mode, int masklayer )
/***********************************************************/
{
    D_PAD*          pt_pad;
    EDA_BaseStruct* PtStruct;
    TEXTE_MODULE*   TextMod;
    int             mlayer;

    /* Print pads */
    pt_pad = Module->m_Pads;
    for( ; pt_pad != NULL; pt_pad = pt_pad->Next() )
    {
        if( (pt_pad->m_Masque_Layer & masklayer ) == 0 )
            continue;
        // Usually we draw pads in sketch mode on non copper layers:
        if ( (masklayer & ALL_CU_LAYERS) == 0 )
        {
            int tmp_fill = ((WinEDA_BasePcbFrame*)panel->m_Parent)->m_DisplayPadFill;
            // Switch in sketch mode
            ((WinEDA_BasePcbFrame*)panel->m_Parent)->m_DisplayPadFill = 0;
            pt_pad->Draw( panel, DC, draw_mode );
            ((WinEDA_BasePcbFrame*)panel->m_Parent)->m_DisplayPadFill = tmp_fill;
        }
        else	// on copper layer, draw pads according to current options
            pt_pad->Draw( panel, DC, draw_mode );
    }

    /* Print footprint graphic shapes */
    PtStruct = Module->m_Drawings;
    mlayer   = g_TabOneLayerMask[Module->GetLayer()];
    if( Module->GetLayer() == COPPER_LAYER_N )
        mlayer = SILKSCREEN_LAYER_CU;
    else if( Module->GetLayer() == CMP_N )
        mlayer = SILKSCREEN_LAYER_CMP;

    if( mlayer & masklayer )
    {
        if( ! Module->m_Reference->m_NoShow )
            Module->m_Reference->Draw( panel, DC, draw_mode );
        if( ! Module->m_Value->m_NoShow )
            Module->m_Value->Draw( panel, DC, draw_mode );
    }

    for( ; PtStruct != NULL; PtStruct = PtStruct->Next() )
    {
        switch( PtStruct->Type() )
        {
        case TYPE_TEXTE_MODULE:
            if( (mlayer & masklayer ) == 0 )
                break;

            TextMod = (TEXTE_MODULE*) PtStruct;
            TextMod->Draw( panel, DC, draw_mode );
            break;

        case TYPE_EDGE_MODULE:
        {
            EDGE_MODULE* edge = (EDGE_MODULE*) PtStruct;
            if( (g_TabOneLayerMask[edge->GetLayer()] & masklayer ) == 0 )
                break;
            edge->Draw( panel, DC, draw_mode );
            break;
        }

        default:
            break;
        }
    }
}
