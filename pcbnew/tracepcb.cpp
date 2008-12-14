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


/**********************************************************************/
void WinEDA_ModuleEditFrame::RedrawActiveWindow( wxDC* DC, bool EraseBg )
/**********************************************************************/

/* Draw the footprint editor BOARD, and others elements : axis, grid ..
 */

{
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
    for( MODULE* module = m_Pcb->m_Modules;  module;  module = module->Next() )
    {
        module->Draw( DrawPanel, DC, GR_OR );
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

    m_Pcb->Draw( DrawPanel, DC, GR_OR );
    if( g_HightLigt_Status )
        DrawHightLight( DC, g_HightLigth_NetCode );

    DrawGeneralRatsnest( DC );

    GetScreen()->ClrRefreshReq();

    Affiche_Status_Box();

    if( DrawPanel->ManageCurseur )
        DrawPanel->ManageCurseur( DrawPanel, DC, FALSE );

    // Redraw the cursor
    DrawPanel->Trace_Curseur( DC );
}


/********************************************************************/
void BOARD::Draw( WinEDA_DrawPanel* aPanel, wxDC* DC,
                  int aDrawMode, const wxPoint& offset )
/********************************************************************/
/* Redraw the BOARD items but not cursors, axis or grid */
{
    for( MODULE* module = m_Modules;  module;  module = module->Next() )
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
            module->Draw( aPanel, DC, aDrawMode );
        else
            Trace_Pads_Only( aPanel, DC, module, 0, 0, layerMask, aDrawMode );
    }


    // Draw the graphic items
    for( BOARD_ITEM* item = m_Drawings;  item;  item = item->Next() )
    {
        if( item->m_Flags & IS_MOVED )
            continue;

        switch( item->Type() )
        {
        case TYPE_COTATION:
        case TYPE_TEXTE:
        case TYPE_MIRE:
        case TYPE_DRAWSEGMENT:
            item->Draw( aPanel, DC, aDrawMode );
            break;

       default:
            break;
        }
    }

    /* Draw all tracks and zones.  As long as dark colors are used for the tracks,
     * Then the OR draw mode should show tracks underneath other tracks.  But a white
     * track will cover any other color since it has more bits to OR in.
     */
    for( TRACK* track = m_Track;  track;   track = track->Next() )
    {
        track->Draw( aPanel, DC, aDrawMode );
    }

    for( SEGZONE* zone = m_Zone;  zone;   zone = zone->Next() )
    {
        zone->Draw( aPanel, DC, aDrawMode );
    }

    /* Draw areas (i.e. zones) */
    for( int ii = 0; ii < GetAreaCount(); ii++ )
    {
        ZONE_CONTAINER* zone = GetArea(ii);

        // Areas must be drawn here only if not moved or dragged,
        // because these areas are drawn by ManageCursor() in a specific manner
        if ( (zone->m_Flags & (IN_EDIT | IS_DRAGGED | IS_MOVED)) == 0 )
        {
            zone->Draw( aPanel, DC, aDrawMode );
            zone->DrawFilledArea( aPanel, DC, aDrawMode );
        }
    }


    // draw the BOARD's markers.
    for( unsigned i=0; i < m_markers.size();  ++i )
    {
        m_markers[i]->Draw( aPanel, DC, aDrawMode );
    }

    // Draw equipots info
    for( EQUIPOT* net = m_Equipots;  net;  net = net->Next() )
    {
        if ( net->GetNet() != 0 )   // no net if 0
            net->Draw( aPanel, DC, aDrawMode );
    }
}


