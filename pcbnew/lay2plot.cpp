/**********************************************/
/* Routine de selection de couches pour trace */
/**********************************************/

#include "fctsys.h"
#include "gr_basic.h"

#include "common.h"
#include "pcbnew.h"
#include "pcbplot.h"

#include "protos.h"


/* Variables locales : */

/* Routines Locales */
static void Plot_Module( WinEDA_DrawPanel* panel, wxDC* DC, MODULE* Module,
                         int draw_mode, int masklayer );


/**********************************************************************************/
void WinEDA_DrawPanel::PrintPage( wxDC* DC, bool Print_Sheet_Ref, int printmasklayer )
/**********************************************************************************/

/* Used to print the board.
 *  Draw the board, but only layers allowed by printmasklayer
 *  ( printmasklayer is a 32 bits mask: bit n = 1 -> layer n is printed)
 */
{
    MODULE*              Module;
    BOARD_ITEM*          PtStruct;
    int                  drawmode = GR_COPY;
    DISPLAY_OPTIONS      save_opt;
    TRACK*               pt_piste;
    WinEDA_BasePcbFrame* frame = (WinEDA_BasePcbFrame*) m_Parent;
    BOARD*               Pcb   = frame->m_Pcb;

    save_opt = DisplayOpt;
    if( printmasklayer & ALL_CU_LAYERS )
        DisplayOpt.DisplayPadFill = FILLED;
    else
        DisplayOpt.DisplayPadFill = SKETCH;
    frame->m_DisplayPadFill      = DisplayOpt.DisplayPadFill;
    frame->m_DisplayPadNum       = DisplayOpt.DisplayPadNum = FALSE;
    DisplayOpt.DisplayPadNoConn  = FALSE;
    DisplayOpt.DisplayPadIsol    = FALSE;
    DisplayOpt.DisplayModEdge    = FILLED;
    DisplayOpt.DisplayModText    = FILLED;
    frame->m_DisplayPcbTrackFill = DisplayOpt.DisplayPcbTrackFill = FILLED;
    DisplayOpt.DisplayTrackIsol  = FALSE;
    DisplayOpt.DisplayDrawItems  = FILLED;
    DisplayOpt.DisplayZones      = TRUE;

    printmasklayer |= EDGE_LAYER;

    /* Draw the pcb graphic items (texts, ...) */
    PtStruct = Pcb->m_Drawings;
    for( ; PtStruct != NULL; PtStruct = PtStruct->Next() )
    {
        switch( PtStruct->Type() )
        {
        case TYPEDRAWSEGMENT:
            if( (g_TabOneLayerMask[ PtStruct->GetLayer()] & printmasklayer) == 0 )
                break;
            Trace_DrawSegmentPcb( this, DC, (DRAWSEGMENT*) PtStruct, drawmode );
            break;

        case TYPECOTATION:
            if( (g_TabOneLayerMask[ PtStruct->GetLayer()] & printmasklayer) == 0 )
                break;
            ( (COTATION*) PtStruct )->Draw( this, DC, drawmode );
            break;

        case TYPETEXTE:
        {
            if( (g_TabOneLayerMask[ PtStruct->GetLayer()] & printmasklayer) == 0 )
                break;
            ( (TEXTE_PCB*) PtStruct )->Draw( this, DC, drawmode );
            break;
        }

        case TYPEMIRE:
            if( (g_TabOneLayerMask[ PtStruct->GetLayer()] & printmasklayer) == 0 )
                break;
            ( (MIREPCB*) PtStruct )->Draw( this, DC, drawmode );
            break;

        case TYPEMARKER:       /* Trace des marqueurs */
            break;

        default:
            break;
        }
    }

    /* Draw the tracks */
    pt_piste = Pcb->m_Track;
    for( ; pt_piste != NULL; pt_piste = (TRACK*) pt_piste->Pnext )
    {
        if( ( printmasklayer & pt_piste->ReturnMaskLayer() ) == 0 )
            continue;
        if( pt_piste->Type() == TYPEVIA ) /* VIA rencontree */
        {
            int rayon = pt_piste->m_Width >> 1;
            int color = g_DesignSettings.m_ViaColor[pt_piste->m_Shape];
            GRSetDrawMode( DC, drawmode );
            GRFilledCircle( &m_ClipBox, DC, pt_piste->m_Start.x, pt_piste->m_Start.y,
                            rayon, 0, color, color );
        }
        else
            pt_piste->Draw( this, DC, drawmode );
    }

    pt_piste = Pcb->m_Zone;
    for( ; pt_piste != NULL; pt_piste = (TRACK*) pt_piste->Pnext )
    {
        if( ( printmasklayer & pt_piste->ReturnMaskLayer() ) == 0 )
            continue;
        pt_piste->Draw( this, DC, drawmode );
    }

    // Draw footprints, this is done at last in order to print the pad holes in while
    // after the tracks
    Module = (MODULE*) Pcb->m_Modules;
    for( ; Module != NULL; Module = (MODULE*) Module->Pnext )
    {
        Plot_Module( this, DC, Module, drawmode, printmasklayer );
    }

    /* draw the via holes */
    pt_piste = Pcb->m_Track;
    int rayon = g_DesignSettings.m_ViaDrill / 2;
    int color = WHITE;
    for( ; pt_piste != NULL; pt_piste = (TRACK*) pt_piste->Pnext )
    {
        if( ( printmasklayer & pt_piste->ReturnMaskLayer() ) == 0 )
            continue;
        if( pt_piste->Type() == TYPEVIA ) /* VIA rencontree */
        {
            GRSetDrawMode( DC, drawmode );
            GRFilledCircle( &m_ClipBox, DC, pt_piste->m_Start.x, pt_piste->m_Start.y,
                            rayon, 0, color, color );
        }
    }

    if( Print_Sheet_Ref )
        m_Parent->TraceWorkSheet( DC, ActiveScreen, 0 );

    DisplayOpt = save_opt;
    frame->m_DisplayPcbTrackFill = DisplayOpt.DisplayPcbTrackFill;
    frame->m_DisplayPadFill = DisplayOpt.DisplayPadFill;
    frame->m_DisplayPadNum  = DisplayOpt.DisplayPadNum;
}


/***********************************************************/
static void Plot_Module( WinEDA_DrawPanel* panel, wxDC* DC,
                         MODULE* Module, int draw_mode, int masklayer )
/***********************************************************/
{
    D_PAD*          pt_pad;
    EDA_BaseStruct* PtStruct;
    TEXTE_MODULE*   TextMod;
    int             mlayer;

    /* Draw pads */
    pt_pad = Module->m_Pads;
    for( ; pt_pad != NULL; pt_pad = (D_PAD*) pt_pad->Pnext )
    {
        if( (pt_pad->m_Masque_Layer & masklayer ) == 0 )
            continue;
        pt_pad->Draw( panel, DC, draw_mode );
    }

    /* draw footprint graphic shapes */
    PtStruct = Module->m_Drawings;
    mlayer   = g_TabOneLayerMask[Module->GetLayer()];
    if( Module->GetLayer() == COPPER_LAYER_N )
        mlayer = SILKSCREEN_LAYER_CU;
    else if( Module->GetLayer() == CMP_N )
        mlayer = SILKSCREEN_LAYER_CMP;

    if( mlayer & masklayer )
    {
        /* Analyse des autorisations de trace pour les textes VALEUR et REF */
        bool trace_val, trace_ref;
        trace_val = trace_ref = TRUE; // les 2 autorisations de tracer sont donnees
        if( Module->m_Reference->m_NoShow )
            trace_ref = FALSE;
        if( Module->m_Value->m_NoShow )
            trace_val = FALSE;

        if( trace_ref )
            Module->m_Reference->Draw( panel, DC, draw_mode );
        if( trace_val )
            Module->m_Value->Draw( panel, DC, draw_mode );
    }

    for( ; PtStruct != NULL; PtStruct = PtStruct->Next() )
    {
        switch( PtStruct->Type() )
        {
        case TYPETEXTEMODULE:
            if( (mlayer & masklayer ) == 0 )
                break;

            TextMod = (TEXTE_MODULE*) PtStruct;
            TextMod->Draw( panel, DC, draw_mode );
            break;

        case TYPEEDGEMODULE:
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
