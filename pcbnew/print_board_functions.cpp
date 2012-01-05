/**
 * @file print_board_functions.cpp
 * @brief Functions to print boards.
 */

#include "fctsys.h"
#include "class_drawpanel.h"
#include "wxPcbStruct.h"
#include "printout_controler.h"
#include "colors_selection.h"
#include "pcbcommon.h"

#include "class_board.h"
#include "class_module.h"
#include "class_edge_mod.h"
#include "class_track.h"
#include "class_zone.h"

#include "pcbnew.h"
#include "protos.h"
#include "pcbplot.h"
#include "module_editor_frame.h"


static void Print_Module( EDA_DRAW_PANEL* aPanel, wxDC* aDC, MODULE* aModule,
                          int aDraw_mode, int aMasklayer,
                          PRINT_PARAMETERS::DrillShapeOptT aDrillShapeOpt );

void FOOTPRINT_EDIT_FRAME::PrintPage( wxDC* aDC,
                                      int   aPrintMaskLayer,
                                      bool  aPrintMirrorMode,
                                      void * aData)
{
    int     drawmode = GR_COPY;
    int     defaultPenSize = 50;

    DISPLAY_OPTIONS save_opt;

    PRINT_PARAMETERS * printParameters = (PRINT_PARAMETERS*) aData; // can be null
    PRINT_PARAMETERS::DrillShapeOptT drillShapeOpt = PRINT_PARAMETERS::FULL_DRILL_SHAPE;

    if( printParameters )
         defaultPenSize = printParameters->m_PenDefaultSize;

    save_opt = DisplayOpt;

    DisplayOpt.ContrastModeDisplay = false;
    DisplayOpt.DisplayPadFill = true;
    DisplayOpt.DisplayViaFill = true;

    m_DisplayPadFill = DisplayOpt.DisplayPadFill;
    m_DisplayViaFill = DisplayOpt.DisplayViaFill;
    m_DisplayPadNum = DisplayOpt.DisplayPadNum = false;
    bool nctmp = GetBoard()->IsElementVisible(NO_CONNECTS_VISIBLE);
    GetBoard()->SetElementVisibility(NO_CONNECTS_VISIBLE, false);
    DisplayOpt.DisplayPadIsol    = false;
    DisplayOpt.DisplayModEdge    = FILLED;
    DisplayOpt.DisplayModText    = FILLED;
    m_DisplayPcbTrackFill = DisplayOpt.DisplayPcbTrackFill = FILLED;
    DisplayOpt.ShowTrackClearanceMode = DO_NOT_SHOW_CLEARANCE;
    DisplayOpt.DisplayDrawItems    = FILLED;
    DisplayOpt.DisplayZonesMode    = 0;
    DisplayOpt.DisplayNetNamesMode = 0;

    m_canvas->SetPrintMirrored( aPrintMirrorMode );

    // The OR mode is used in color mode, but be aware the background *must be
    // BLACK.  In the print page dialog, we first print in BLACK, and after
    // reprint in color, on the black "local" background, in OR mode the black
    // print is not made before, only a white page is printed
    if( GetGRForceBlackPenState() == false )
        drawmode = GR_OR;

    // Draw footprints, this is done at last in order to print the pad holes in
    // white (or g_DrawBgColor) after the tracks and zones
    int tmp = D_PAD::m_PadSketchModePenSize;
    D_PAD::m_PadSketchModePenSize = defaultPenSize;

    wxSize  pageSizeIU = GetPageSizeIU() / 2;
    wxPoint offset( pageSizeIU.x, pageSizeIU.y );

    for( MODULE* module = GetBoard()->m_Modules;  module;  module = module->Next() )
    {
        module->Move( offset );
        Print_Module( m_canvas, aDC, module, drawmode, aPrintMaskLayer, drillShapeOpt );
        module->Move( -offset );
    }

    D_PAD::m_PadSketchModePenSize = tmp;

    m_canvas->SetPrintMirrored( false );

    DisplayOpt = save_opt;
    m_DisplayPcbTrackFill = DisplayOpt.DisplayPcbTrackFill;
    m_DisplayPadFill = DisplayOpt.DisplayPadFill;
    m_DisplayViaFill = DisplayOpt.DisplayViaFill;
    m_DisplayPadNum  = DisplayOpt.DisplayPadNum;
    GetBoard()->SetElementVisibility( NO_CONNECTS_VISIBLE, nctmp );
}


/**
 * Function PrintPage
 * is used to print the board (on printer, or when creating SVF files).
 * Print the board, but only layers allowed by aPrintMaskLayer
 * @param aDC = the print device context
 * @param aPrintMaskLayer = a 32 bits mask: bit n = 1 -> layer n is printed
 * @param aPrintMirrorMode = true to plot mirrored
 * @param aData = a pointer to an optional data (NULL if not used)
 */
void PCB_EDIT_FRAME::PrintPage( wxDC* aDC,
                                int   aPrintMaskLayer,
                                bool  aPrintMirrorMode,
                                void* aData)
{
    MODULE* Module;
    int drawmode = GR_COPY;
    DISPLAY_OPTIONS save_opt;
    TRACK*          pt_trace;
    BOARD*          Pcb   = GetBoard();
    int             defaultPenSize = 50;
    bool            onePagePerLayer = false;

    PRINT_PARAMETERS * printParameters = (PRINT_PARAMETERS*) aData; // can be null

    if( printParameters && printParameters->m_OptionPrintPage == 0 )
        onePagePerLayer = true;

    PRINT_PARAMETERS::DrillShapeOptT drillShapeOpt = PRINT_PARAMETERS::FULL_DRILL_SHAPE;

    if( printParameters )
    {
        drillShapeOpt = printParameters->m_DrillShapeOpt;
        defaultPenSize = printParameters->m_PenDefaultSize;
    }

    save_opt = DisplayOpt;
    int activeLayer = GetScreen()->m_Active_Layer;

    DisplayOpt.ContrastModeDisplay = false;
    DisplayOpt.DisplayPadFill = true;
    DisplayOpt.DisplayViaFill = true;

    if( (aPrintMaskLayer & ALL_CU_LAYERS) == 0 )
    {
        if( onePagePerLayer )
        {   // We can print mask layers (solder mask and solder paste) with the actual
            // pad sizes.  To do that, we must set ContrastModeDisplay to true and set
            //the GetScreen()->m_Active_Layer to the current printed layer
            DisplayOpt.ContrastModeDisplay = true;
            DisplayOpt.DisplayPadFill = true;

            // Calculate the active layer number to print from its mask layer:
            GetScreen()->m_Active_Layer = 0;

            for(int kk = 0; kk < 32; kk ++ )
            {
                if( ((1 << kk) & aPrintMaskLayer) != 0 )
                {
                    GetScreen()->m_Active_Layer = kk;
                    break;
                }
            }

            // pads on Silkscreen layer are usually plot in sketch mode:
            if( (GetScreen()->m_Active_Layer == SILKSCREEN_N_BACK)
                || (GetScreen()->m_Active_Layer == SILKSCREEN_N_FRONT) )
                DisplayOpt.DisplayPadFill = false;

        }
        else
        {
            DisplayOpt.DisplayPadFill = false;
        }
    }


    m_DisplayPadFill = DisplayOpt.DisplayPadFill;
    m_DisplayViaFill = DisplayOpt.DisplayViaFill;
    m_DisplayPadNum = DisplayOpt.DisplayPadNum = false;
    bool nctmp = GetBoard()->IsElementVisible( NO_CONNECTS_VISIBLE );
    GetBoard()->SetElementVisibility( NO_CONNECTS_VISIBLE, false );
    DisplayOpt.DisplayPadIsol    = false;
    m_DisplayModEdge = DisplayOpt.DisplayModEdge    = FILLED;
    m_DisplayModText = DisplayOpt.DisplayModText    = FILLED;
    m_DisplayPcbTrackFill = DisplayOpt.DisplayPcbTrackFill = FILLED;
    DisplayOpt.ShowTrackClearanceMode = DO_NOT_SHOW_CLEARANCE;
    DisplayOpt.DisplayDrawItems    = FILLED;
    DisplayOpt.DisplayZonesMode    = 0;
    DisplayOpt.DisplayNetNamesMode = 0;

    m_canvas->SetPrintMirrored( aPrintMirrorMode );

    // The OR mode is used in color mode, but be aware the background *must be
    // BLACK.  In the print page dialog, we first print in BLACK, and after
    // reprint in color, on the black "local" background, in OR mode the black
    // print is not made before, only a white page is printed
    if( GetGRForceBlackPenState() == false )
        drawmode = GR_OR;

    /* Print the pcb graphic items (texts, ...) */
    GRSetDrawMode( aDC, drawmode );

    for( BOARD_ITEM* item = Pcb->m_Drawings; item; item = item->Next() )
    {
        switch( item->Type() )
        {
        case PCB_LINE_T:
        case PCB_DIMENSION_T:
        case PCB_TEXT_T:
        case PCB_TARGET_T:
            if( ( ( 1 << item->GetLayer() ) & aPrintMaskLayer ) == 0 )
                break;

            item->Draw( m_canvas, aDC, drawmode );
            break;

        case PCB_MARKER_T:
        default:
            break;
        }
    }

    /* Print tracks */
    pt_trace = Pcb->m_Track;

    for( ; pt_trace != NULL; pt_trace = pt_trace->Next() )
    {
        if( ( aPrintMaskLayer & pt_trace->ReturnMaskLayer() ) == 0 )
            continue;

        if( pt_trace->Type() == PCB_VIA_T ) /* VIA encountered. */
        {
            int radius = pt_trace->m_Width >> 1;
            int color = g_ColorsSettings.GetItemColor( VIAS_VISIBLE + pt_trace->m_Shape );
            GRSetDrawMode( aDC, drawmode );
            GRFilledCircle( m_canvas->GetClipBox(), aDC,
                            pt_trace->m_Start.x,
                            pt_trace->m_Start.y,
                            radius,
                            0, color, color );
        }
        else
        {
            pt_trace->Draw( m_canvas, aDC, drawmode );
        }
    }

    pt_trace = Pcb->m_Zone;

    for( ; pt_trace != NULL; pt_trace = pt_trace->Next() )
    {
        if( ( aPrintMaskLayer & pt_trace->ReturnMaskLayer() ) == 0 )
            continue;

        pt_trace->Draw( m_canvas, aDC, drawmode );
    }

    /* Draw filled areas (i.e. zones) */
    for( int ii = 0; ii < Pcb->GetAreaCount(); ii++ )
    {
        ZONE_CONTAINER* zone = Pcb->GetArea( ii );

        if( ( aPrintMaskLayer & ( 1 << zone->GetLayer() ) ) == 0 )
            continue;

        zone->DrawFilledArea( m_canvas, aDC, drawmode );
    }

    // Draw footprints, this is done at last in order to print the pad holes in
    // white (or g_DrawBgColor) after the tracks and zones
    Module = (MODULE*) Pcb->m_Modules;
    int tmp = D_PAD::m_PadSketchModePenSize;
    D_PAD::m_PadSketchModePenSize = defaultPenSize;

    for( ; Module != NULL; Module = Module->Next() )
    {
        Print_Module( m_canvas, aDC, Module, drawmode, aPrintMaskLayer, drillShapeOpt );
    }

    D_PAD::m_PadSketchModePenSize = tmp;

    /* Print via holes in bg color: Not sure it is good for buried or blind
     * vias */
    if( drillShapeOpt != PRINT_PARAMETERS::NO_DRILL_SHAPE )
    {
        pt_trace = Pcb->m_Track;
        int  color = g_DrawBgColor;
        bool blackpenstate = GetGRForceBlackPenState();
        GRForceBlackPen( false );
        GRSetDrawMode( aDC, GR_COPY );

        for( ; pt_trace != NULL; pt_trace = pt_trace->Next() )
        {
            if( ( aPrintMaskLayer & pt_trace->ReturnMaskLayer() ) == 0 )
                continue;

            if( pt_trace->Type() == PCB_VIA_T ) /* VIA encountered. */
            {
                int diameter;

                if( drillShapeOpt == PRINT_PARAMETERS::SMALL_DRILL_SHAPE )
                    diameter = min( SMALL_DRILL, pt_trace->GetDrillValue() );
                else
                    diameter = pt_trace->GetDrillValue();

                GRFilledCircle( m_canvas->GetClipBox(), aDC,
                                pt_trace->m_Start.x, pt_trace->m_Start.y,
                                diameter/2,
                                0, color, color );
            }
        }

        GRForceBlackPen( blackpenstate );
    }

    m_canvas->SetPrintMirrored( false );

    DisplayOpt = save_opt;
    GetScreen()->m_Active_Layer = activeLayer;
    m_DisplayPcbTrackFill = DisplayOpt.DisplayPcbTrackFill;
    m_DisplayPadFill = DisplayOpt.DisplayPadFill;
    m_DisplayViaFill = DisplayOpt.DisplayViaFill;
    m_DisplayPadNum  = DisplayOpt.DisplayPadNum;
    m_DisplayModEdge = DisplayOpt.DisplayModEdge;
    m_DisplayModText = DisplayOpt.DisplayModText;
    GetBoard()->SetElementVisibility(NO_CONNECTS_VISIBLE, nctmp);
}


static void Print_Module( EDA_DRAW_PANEL* aPanel, wxDC* aDC, MODULE* aModule,
                          int aDraw_mode, int aMasklayer,
                          PRINT_PARAMETERS::DrillShapeOptT aDrillShapeOpt )
{
    D_PAD*        pt_pad;
    EDA_ITEM*     PtStruct;
    TEXTE_MODULE* TextMod;
    int           mlayer;

    /* Print pads */
    pt_pad = aModule->m_Pads;

    for( ; pt_pad != NULL; pt_pad = pt_pad->Next() )
    {
        if( (pt_pad->m_layerMask & aMasklayer ) == 0 )
            continue;

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

    /* Print footprint graphic shapes */
    PtStruct = aModule->m_Drawings;
    mlayer   = GetLayerMask( aModule->GetLayer() );

    if( aModule->GetLayer() == LAYER_N_BACK )
        mlayer = SILKSCREEN_LAYER_BACK;
    else if( aModule->GetLayer() == LAYER_N_FRONT )
        mlayer = SILKSCREEN_LAYER_FRONT;

    if( mlayer & aMasklayer )
    {
        if( aModule->m_Reference->IsVisible() )
            aModule->m_Reference->Draw( aPanel, aDC, aDraw_mode );

        if( aModule->m_Value->IsVisible() )
            aModule->m_Value->Draw( aPanel, aDC, aDraw_mode );
    }

    for( ; PtStruct != NULL; PtStruct = PtStruct->Next() )
    {
        switch( PtStruct->Type() )
        {
        case PCB_MODULE_TEXT_T:
            if( ( mlayer & aMasklayer ) == 0 )
                break;

            TextMod = (TEXTE_MODULE*) PtStruct;
            TextMod->Draw( aPanel, aDC, aDraw_mode );
            break;

        case PCB_MODULE_EDGE_T:
        {
            EDGE_MODULE* edge = (EDGE_MODULE*) PtStruct;

            if( ( GetLayerMask( edge->GetLayer() ) & aMasklayer ) == 0 )
                break;

            edge->Draw( aPanel, aDC, aDraw_mode );
            break;
        }

        default:
            break;
        }
    }
}
