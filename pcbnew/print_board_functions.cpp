/************************************************************/
/* print_board_functions.cpp: some functions to plot boards */
/************************************************************/

#include "fctsys.h"
#include "gr_basic.h"
#include "common.h"
#include "class_drawpanel.h"

#include "pcbnew.h"
#include "class_board_design_settings.h"
#include "pcbplot.h"
#include "printout_controler.h"
#include "protos.h"


static void Print_Module( WinEDA_DrawPanel* aPanel, wxDC* aDC, MODULE* aModule,
                          int aDraw_mode, int aMasklayer,
                          PRINT_PARAMETERS::DrillShapeOptT aDrillShapeOpt );


/** Function PrintPage
 * Used to print the board (on printer, or when creating SVF files).
 * Print the board, but only layers allowed by aPrintMaskLayer
 * @param aDC = the print device context
 * @param aPrint_Sheet_Ref = true to print frame references
 * @param aPrint_Sheet_Ref = a 32 bits mask: bit n = 1 -> layer n is printed
 * @param aPrintMirrorMode = true to plot mirrored
 * @param aData = a pointer to an optional data (NULL if not used)
 */
void WinEDA_DrawPanel::PrintPage( wxDC* aDC,
                                  bool  aPrint_Sheet_Ref,
                                  int   aPrintMaskLayer,
                                  bool  aPrintMirrorMode,
                                  void * aData)
{
    MODULE* Module;
    int drawmode = GR_COPY;
    DISPLAY_OPTIONS      save_opt;
    TRACK*               pt_piste;
    WinEDA_BasePcbFrame* frame = (WinEDA_BasePcbFrame*) m_Parent;
    BOARD*               Pcb   = frame->GetBoard();
    PRINT_PARAMETERS * printParameters = (PRINT_PARAMETERS*) aData; // can be null

    PRINT_PARAMETERS::DrillShapeOptT drillShapeOpt = PRINT_PARAMETERS::FULL_DRILL_SHAPE;
    if( printParameters )
        drillShapeOpt = printParameters->m_DrillShapeOpt;

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

    frame->m_DisplayPadFill = DisplayOpt.DisplayPadFill;
    frame->m_DisplayViaFill = DisplayOpt.DisplayViaFill;
    frame->m_DisplayPadNum = DisplayOpt.DisplayPadNum = false;
    DisplayOpt.DisplayPadNoConn  = false;
    DisplayOpt.DisplayPadIsol    = false;
    DisplayOpt.DisplayModEdge    = FILLED;
    DisplayOpt.DisplayModText    = FILLED;
    frame->m_DisplayPcbTrackFill = DisplayOpt.DisplayPcbTrackFill = FILLED;
    DisplayOpt.ShowTrackClearanceMode = DO_NOT_SHOW_CLEARANCE;
    DisplayOpt.DisplayDrawItems    = FILLED;
    DisplayOpt.DisplayZonesMode    = 0;
    DisplayOpt.DisplayNetNamesMode = 0;

    m_PrintIsMirrored = aPrintMirrorMode;

    // The OR mode is used in color mode, but be aware the backgroud *must be
    // BLACK.  In the print page dialog, we first plrint in BLACK, and after
    // reprint in color, on the black "local" backgroud, in OR mode the black
    // print is not made before, only a white page is printed
    if( GetGRForceBlackPenState() == false )
        drawmode = GR_OR;

    /* Print the pcb graphic items (texts, ...) */
    GRSetDrawMode( aDC, drawmode );
    for( BOARD_ITEM* item = Pcb->m_Drawings; item; item = item->Next() )
    {
        switch( item->Type() )
        {
        case TYPE_DRAWSEGMENT:
        case TYPE_COTATION:
        case TYPE_TEXTE:
        case TYPE_MIRE:
            if( ( ( 1 << item->GetLayer() ) & aPrintMaskLayer ) == 0 )
                break;

            item->Draw( this, aDC, drawmode );
            break;

        case TYPE_MARKER_PCB:
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
        if( pt_piste->Type() == TYPE_VIA ) /* VIA encountered. */
        {
            int rayon = pt_piste->m_Width >> 1;
            int color = g_DesignSettings.m_ViaColor[pt_piste->m_Shape];
            GRSetDrawMode( aDC, drawmode );
            GRFilledCircle( &m_ClipBox, aDC,
                            pt_piste->m_Start.x,
                            pt_piste->m_Start.y,
                            rayon,
                            0, color, color );
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
        ZONE_CONTAINER* zone = Pcb->GetArea( ii );
        if( ( aPrintMaskLayer & ( 1 << zone->GetLayer() ) ) == 0 )
            continue;

        zone->DrawFilledArea( this, aDC, drawmode );
    }

    // Draw footprints, this is done at last in order to print the pad holes in
    // white (or g_DrawBgColor) after the tracks and zones
    Module = (MODULE*) Pcb->m_Modules;
    for( ; Module != NULL; Module = Module->Next() )
    {
        Print_Module( this, aDC, Module, drawmode, aPrintMaskLayer, drillShapeOpt );
    }

    /* Print via holes in bg color: Not sure it is good for buried or blind
     * vias */
    if( drillShapeOpt != PRINT_PARAMETERS::NO_DRILL_SHAPE )
    {
        pt_piste = Pcb->m_Track;
        int  color = g_DrawBgColor;
        bool blackpenstate = GetGRForceBlackPenState();
        GRForceBlackPen( false );
        GRSetDrawMode( aDC, GR_COPY );
        for( ; pt_piste != NULL; pt_piste = pt_piste->Next() )
        {
            if( ( aPrintMaskLayer & pt_piste->ReturnMaskLayer() ) == 0 )
                continue;
            if( pt_piste->Type() == TYPE_VIA ) /* VIA encountered. */
            {
                int diameter;
                if( drillShapeOpt == PRINT_PARAMETERS::SMALL_DRILL_SHAPE )
                    diameter = min( SMALL_DRILL, pt_piste->GetDrillValue());
                else
                    diameter = pt_piste->GetDrillValue();
                GRFilledCircle( &m_ClipBox, aDC,
                                pt_piste->m_Start.x, pt_piste->m_Start.y,
                                diameter/2,
                                0, color, color );
            }
        }
        GRForceBlackPen( blackpenstate );
    }

    if( aPrint_Sheet_Ref )
        m_Parent->TraceWorkSheet( aDC, GetScreen(), 10 );

    m_PrintIsMirrored = false;

    DisplayOpt = save_opt;
    frame->m_DisplayPcbTrackFill = DisplayOpt.DisplayPcbTrackFill;
    frame->m_DisplayPadFill = DisplayOpt.DisplayPadFill;
    frame->m_DisplayViaFill = DisplayOpt.DisplayViaFill;
    frame->m_DisplayPadNum  = DisplayOpt.DisplayPadNum;
}


static void Print_Module( WinEDA_DrawPanel* aPanel, wxDC* aDC, MODULE* aModule,
                          int aDraw_mode, int aMasklayer,
                          PRINT_PARAMETERS::DrillShapeOptT aDrillShapeOpt )
{
    D_PAD*          pt_pad;
    EDA_BaseStruct* PtStruct;
    TEXTE_MODULE*   TextMod;
    int             mlayer;

    /* Print pads */
    pt_pad = aModule->m_Pads;
    for( ; pt_pad != NULL; pt_pad = pt_pad->Next() )
    {
        if( (pt_pad->m_Masque_Layer & aMasklayer ) == 0 )
            continue;

        // Usually we draw pads in sketch mode on non copper layers:
        if( (aMasklayer & ALL_CU_LAYERS) == 0 )
        {
            int tmp_fill =
                ( (WinEDA_BasePcbFrame*) aPanel->GetParent() )->m_DisplayPadFill;

            // Switch in sketch mode
            ( (WinEDA_BasePcbFrame*) aPanel->GetParent() )->m_DisplayPadFill = 0;
            pt_pad->Draw( aPanel, aDC, aDraw_mode );
            ( (WinEDA_BasePcbFrame*) aPanel->GetParent() )->m_DisplayPadFill =
                tmp_fill;
        }
        else    // on copper layer, draw pads according to current options
        {
            // Manage hole according to the print drill option
            wxSize drill_tmp = pt_pad->m_Drill;
            switch ( aDrillShapeOpt )
            {
                case PRINT_PARAMETERS::NO_DRILL_SHAPE:
                    pt_pad->m_Drill = wxSize(0,0);
                    break;
                case PRINT_PARAMETERS::SMALL_DRILL_SHAPE:
                    pt_pad->m_Drill.x = MIN(SMALL_DRILL,pt_pad->m_Drill.x);
                    pt_pad->m_Drill.y = MIN(SMALL_DRILL,pt_pad->m_Drill.y);
                    break;
                case PRINT_PARAMETERS::FULL_DRILL_SHAPE:
                    // Do nothing
                    break;
            }
            pt_pad->Draw( aPanel, aDC, aDraw_mode );
            pt_pad->m_Drill = drill_tmp;
        }
    }

    /* Print footprint graphic shapes */
    PtStruct = aModule->m_Drawings;
    mlayer   = g_TabOneLayerMask[aModule->GetLayer()];
    if( aModule->GetLayer() == LAYER_N_BACK )
        mlayer = SILKSCREEN_LAYER_BACK;
    else if( aModule->GetLayer() == LAYER_N_FRONT )
        mlayer = SILKSCREEN_LAYER_FRONT;

    if( mlayer & aMasklayer )
    {
        if( !aModule->m_Reference->m_NoShow )
            aModule->m_Reference->Draw( aPanel, aDC, aDraw_mode );
        if( !aModule->m_Value->m_NoShow )
            aModule->m_Value->Draw( aPanel, aDC, aDraw_mode );
    }

    for( ; PtStruct != NULL; PtStruct = PtStruct->Next() )
    {
        switch( PtStruct->Type() )
        {
        case TYPE_TEXTE_MODULE:
            if( (mlayer & aMasklayer ) == 0 )
                break;

            TextMod = (TEXTE_MODULE*) PtStruct;
            TextMod->Draw( aPanel, aDC, aDraw_mode );
            break;

        case TYPE_EDGE_MODULE:
        {
            EDGE_MODULE* edge = (EDGE_MODULE*) PtStruct;
            if( ( g_TabOneLayerMask[edge->GetLayer()] & aMasklayer ) == 0 )
                break;
            edge->Draw( aPanel, aDC, aDraw_mode );
            break;
        }

        default:
            break;
        }
    }
}
