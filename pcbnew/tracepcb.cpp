/*****************************************/
/* Routines generales d'affichage du PCB */
/*****************************************/

/* fichier tracepcb.cpp */

/*
 *  Routines d'affichage grille, Boite de coordonnees, Curseurs, marqueurs ...
 */

#include <vector.h>

#include "fctsys.h"
#include "gr_basic.h"

#include "common.h"
#include "pcbnew.h"
#include "autorout.h"
#include "pcbplot.h"

#include "protos.h"

using namespace std;


/**********************************************************************/
void WinEDA_ModuleEditFrame::RedrawActiveWindow( wxDC* DC, bool EraseBg )
/**********************************************************************/

/* Draw the footprint editor BOARD, and others elements : axis, grid ..
 */

{
    MODULE*     Module;
    PCB_SCREEN* screen = GetScreen();

    if( !m_Pcb || !screen )
        return;

    ActiveScreen = screen;
    GRSetDrawMode( DC, GR_COPY );

    if( EraseBg )
        DrawPanel->EraseScreen( DC );

    DrawPanel->DrawBackGround( DC );
    TraceWorkSheet( DC, screen, 0 );

	/* Redraw the footprint */
    Module = (MODULE*) m_Pcb->m_Modules;
    for( ; Module != NULL; Module = (MODULE*) Module->Pnext )
    {
        Module->Draw( DrawPanel, DC, wxPoint( 0, 0 ), GR_OR );
    }

    Affiche_Status_Box();

    if( DrawPanel->ManageCurseur )
        DrawPanel->ManageCurseur( DrawPanel, DC, FALSE );

    /* Redraw the cursor */
    DrawPanel->Trace_Curseur( DC );

    screen->ClrRefreshReq();
}


/****************************************************************/
void WinEDA_PcbFrame::RedrawActiveWindow( wxDC* DC, bool EraseBg )
/****************************************************************/

/* Draw the BOARD, and others elements : axis, grid ..
 */
{
    PCB_SCREEN* Screen = GetScreen();

    if( !m_Pcb || !Screen )
        return;

    ActiveScreen = GetScreen();
    GRSetDrawMode( DC, GR_COPY );

    if( EraseBg )
        DrawPanel->EraseScreen( DC );

    DrawPanel->DrawBackGround( DC );

    Trace_Pcb( DC, GR_OR );
    TraceWorkSheet( DC, GetScreen(), 0 );
    Affiche_Status_Box();

	if( DrawPanel->ManageCurseur )
		DrawPanel->ManageCurseur( DrawPanel, DC, FALSE );
    /* Redraw the cursor */
	DrawPanel->Trace_Curseur( DC );
}


/****************************************************/
void WinEDA_PcbFrame::Trace_Pcb( wxDC* DC, int mode )
/****************************************************/
/* Redraw the BOARD items but not cursors, axis or grid */
{
    MODULE*         Module;
    EDA_BaseStruct* PtStruct;

    if( !m_Pcb )
        return;

    Module = (MODULE*) m_Pcb->m_Modules;
    for( ; Module != NULL; Module = (MODULE*) Module->Pnext )
    {
        bool display = true;
        int  MaskLay = ALL_CU_LAYERS;
        
        if( Module->m_Flags & IS_MOVED )
            continue;

        if( !DisplayOpt.Show_Modules_Cmp )
        {
            if( Module->GetLayer() == CMP_N )
                display = FALSE;
            MaskLay &= ~CMP_LAYER;
        }
        if( !DisplayOpt.Show_Modules_Cu )
        {
            if( Module->GetLayer() == COPPER_LAYER_N )
                display = FALSE;
            MaskLay &= ~CUIVRE_LAYER;
        }

        if( display )
            Module->Draw( DrawPanel, DC, wxPoint( 0, 0 ), mode );
        else
            Trace_Pads_Only( DrawPanel, DC, Module, 0, 0, MaskLay, mode );
    }

    /* Draw the graphic items */

    PtStruct = m_Pcb->m_Drawings;
    for( ; PtStruct != NULL; PtStruct = PtStruct->Pnext )
    {
        if( PtStruct->m_Flags & IS_MOVED )
            continue;

        switch( PtStruct->Type() )
        {
        case TYPECOTATION:
            ( (COTATION*) PtStruct )->Draw( DrawPanel, DC, wxPoint( 0, 0 ), mode );
            break;

        case TYPETEXTE:
            ( (TEXTE_PCB*) PtStruct )->Draw( DrawPanel, DC, wxPoint( 0, 0 ), mode );
            break;

        case TYPEMIRE:
            ( (MIREPCB*) PtStruct )->Draw( DrawPanel, DC, wxPoint( 0, 0 ), mode );
            break;

       case TYPEDRAWSEGMENT:
			Trace_DrawSegmentPcb( DrawPanel, DC, (DRAWSEGMENT*) PtStruct, mode );
			break;

	   default:
            break;
        }
    }

    // draw the BOARD's markers.
    for( unsigned i=0; i<m_Pcb->m_markers.size();  ++i )
    {
        m_Pcb->m_markers[i]->Draw( DrawPanel, DC, mode );
    }

    Trace_Pistes( DrawPanel, m_Pcb, DC, mode );
    if( g_HightLigt_Status )
        DrawHightLight( DC, g_HightLigth_NetCode );

    EDGE_ZONE* segment;
    for( segment = m_Pcb->m_CurrentLimitZone; segment; segment = segment->Next() )
    {
        if( segment->m_Flags & IS_MOVED )
            continue;
        
        Trace_DrawSegmentPcb( DrawPanel, DC, segment, mode );
    }

	for( unsigned ii = 0; ii < m_Pcb->m_ZoneDescriptorList.size(); ii++ )
	{
		ZONE_CONTAINER* edge_zone =  m_Pcb->m_ZoneDescriptorList[ii];
		edge_zone->Draw( DrawPanel, DC, wxPoint(0,0), mode);
	}

    DrawGeneralRatsnest( DC );

    m_CurrentScreen->ClrRefreshReq();
}

