/*****************************************/
/* Routines generales d'affichage du PCB */
/*****************************************/

/* fichier tracepcb.cpp */

/*
 *  Routines d'affichage grille, Boite de coordonnees, Curseurs, marqueurs ...
 */

#include <vector>

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
    PCB_SCREEN* screen = (PCB_SCREEN*)GetScreen();

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
        Module->Draw( DrawPanel, DC, GR_OR );
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
    PCB_SCREEN* screen = GetScreen();

    if( !m_Pcb || !screen )
        return;

    ActiveScreen = screen;
    GRSetDrawMode( DC, GR_COPY );

    if( EraseBg )
        DrawPanel->EraseScreen( DC );

    DrawPanel->DrawBackGround( DC );

    TraceWorkSheet( DC, GetScreen(), 0 );

    Trace_Pcb( DC, GR_OR );

    Affiche_Status_Box();

    if( DrawPanel->ManageCurseur )
        DrawPanel->ManageCurseur( DrawPanel, DC, FALSE );

    // Redraw the cursor
    DrawPanel->Trace_Curseur( DC );
}


/* should make the function below this one:
void BOARD::Draw( WinEDA_DrawPanel* panel, wxDC* DC,
                  int aDrawMode, const wxPoint& offset = ZeroOffset );
*/


/****************************************************/
void WinEDA_PcbFrame::Trace_Pcb( wxDC* DC, int mode )
/****************************************************/
/* Redraw the BOARD items but not cursors, axis or grid */
{
    if( !m_Pcb )
        return;

    for( MODULE* module = m_Pcb->m_Modules;  module;  module = module->Next() )
    {
        bool display = true;
        int  layerMask = ALL_CU_LAYERS;

        if( module->m_Flags & IS_MOVED )
            continue;

        if( !DisplayOpt.Show_Modules_Cmp )
        {
            if( module->GetLayer() == CMP_N )
                display = FALSE;
            layerMask &= ~CMP_LAYER;
        }

        if( !DisplayOpt.Show_Modules_Cu )
        {
            if( module->GetLayer() == COPPER_LAYER_N )
                display = FALSE;
            layerMask &= ~CUIVRE_LAYER;
        }

        if( display )
            module->Draw( DrawPanel, DC, mode );
        else
            Trace_Pads_Only( DrawPanel, DC, module, 0, 0, layerMask, mode );
    }


    // Draw the graphic items
    for( BOARD_ITEM* item = m_Pcb->m_Drawings;  item;  item = item->Next() )
    {
        if( item->m_Flags & IS_MOVED )
            continue;

        switch( item->Type() )
        {
        case TYPECOTATION:
        case TYPETEXTE:
        case TYPEMIRE:
        case TYPEDRAWSEGMENT:
            item->Draw( DrawPanel, DC, mode );
            break;

       default:
            break;
        }
    }

    Trace_Pistes( DrawPanel, m_Pcb, DC, mode );
    if( g_HightLigt_Status )
        DrawHightLight( DC, g_HightLigth_NetCode );

    for( int ii = 0; ii < m_Pcb->GetAreaCount(); ii++ )
    {
        ZONE_CONTAINER* edge_zone =  m_Pcb->GetArea(ii);

        // Areas must be drawn here only if not moved or dragged,
        // because these areas are drawn by ManageCursor() in a specific manner
        if ( (edge_zone->m_Flags & (IN_EDIT | IS_DRAGGED | IS_MOVED)) == 0 )
            edge_zone->Draw( DrawPanel, DC, mode );
    }

    // draw the BOARD's markers.
    for( unsigned i=0; i<m_Pcb->m_markers.size();  ++i )
    {
        m_Pcb->m_markers[i]->Draw( DrawPanel, DC, mode );
    }

    DrawGeneralRatsnest( DC );

    GetScreen()->ClrRefreshReq();
}

