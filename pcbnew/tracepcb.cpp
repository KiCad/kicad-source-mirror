/****************/
/* tracepcb.cpp */
/****************/

/*
 * Functions to redraw the current board ...
 */

#include <vector>

#include "fctsys.h"
#include "gr_basic.h"
#include "common.h"
#include "class_drawpanel.h"

#include "pcbnew.h"
#include "wxPcbStruct.h"
#include "pcbplot.h"
#include "protos.h"

extern int g_DrawDefaultLineThickness; // Default line thickness, used to draw Frame references


// Local functions:
/* Trace the pads of a module in sketch mode.
 * Used to display pads when when the module visibility is set to not visible
 * and we want to see pad through.
 * The pads must appear on the layers selected in MasqueLayer
 */
static void Trace_Pads_Only( WinEDA_DrawPanel* panel, wxDC* DC, MODULE* Module,
                      int ox, int oy, int MasqueLayer, int draw_mode );


/* Draw the footprint editor BOARD, and others elements : axis, grid ..
 */
void WinEDA_ModuleEditFrame::RedrawActiveWindow( wxDC* DC, bool EraseBg )
{
    PCB_SCREEN* screen = (PCB_SCREEN*)GetScreen();

    if( !GetBoard() || !screen )
        return;

    ActiveScreen = screen;
    GRSetDrawMode( DC, GR_COPY );

    DrawPanel->DrawBackGround( DC );
    TraceWorkSheet( DC, screen, 0 );

    /* Redraw the footprints */
    for( MODULE* module = GetBoard()->m_Modules;  module;  module = module->Next() )
    {
        module->Draw( DrawPanel, DC, GR_OR );
    }

    screen->ClrRefreshReq();

    if( DrawPanel->ManageCurseur )
        DrawPanel->ManageCurseur( DrawPanel, DC, FALSE );

    /* Redraw the cursor */
    DrawPanel->DrawCursor( DC );
}


/* Draw the BOARD, and others elements : axis, grid ..
 */
void WinEDA_PcbFrame::RedrawActiveWindow( wxDC* DC, bool EraseBg )
{
    PCB_SCREEN* screen = GetScreen();

    if( !GetBoard() || !screen )
        return;

    ActiveScreen = screen;
    GRSetDrawMode( DC, GR_COPY );

    DrawPanel->DrawBackGround( DC );

    TraceWorkSheet( DC, GetScreen(), g_DrawDefaultLineThickness );

    GetBoard()->Draw( DrawPanel, DC, GR_OR );

    DrawGeneralRatsnest( DC );

    GetScreen()->ClrRefreshReq();

    if( DrawPanel->ManageCurseur )
        DrawPanel->ManageCurseur( DrawPanel, DC, FALSE );

    // Redraw the cursor
    DrawPanel->DrawCursor( DC );
}


/* Redraw the BOARD items but not cursors, axis or grid */
void BOARD::Draw( WinEDA_DrawPanel* aPanel, wxDC* DC,
                  int aDrawMode, const wxPoint& offset )
{
    /* The order of drawing is flexible on some systems and not on others.  For
     * OSes which use OR to draw, the order is not important except for the
     * effect of the highlight and its relationship to markers. See comment
     * below.
     * This order independence comes from the fact that a binary OR operation is
     * commutative in nature.
     * However on the OSX, the OR operation is not used, and so this sequence
     * below is chosen to give MODULEs the highest visible priority.
     */

    /* Draw all tracks and zones.  As long as dark colors are used for the
     * tracks,  Then the OR draw mode should show tracks underneath other
     * tracks.  But a white track will cover any other color since it has
     * more bits to OR in.
     */
    for( TRACK* track = m_Track;  track;   track = track->Next() )
    {
        track->Draw( aPanel, DC, aDrawMode );
    }

    for( SEGZONE* zone = m_Zone;  zone;   zone = zone->Next() )
    {
        zone->Draw( aPanel, DC, aDrawMode );
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

    for( MODULE* module = m_Modules;  module;  module = module->Next() )
    {
        bool display = true;
        int  layerMask = ALL_CU_LAYERS;

        if( module->m_Flags & IS_MOVED )
            continue;

        if( !IsElementVisible( PCB_VISIBLE(MOD_FR_VISIBLE) ) )
        {
            if( module->GetLayer() == LAYER_N_FRONT )
                display = FALSE;
            layerMask &= ~LAYER_FRONT;
        }

        if( !IsElementVisible( PCB_VISIBLE(MOD_BK_VISIBLE) ) )
        {
            if( module->GetLayer() == LAYER_N_BACK )
                display = FALSE;
            layerMask &= ~LAYER_BACK;
        }

        if( display )
            module->Draw( aPanel, DC, aDrawMode );
        else
            Trace_Pads_Only( aPanel, DC, module, 0, 0, layerMask, aDrawMode );
    }

    // @todo: this high-light functionality could be built into me.
    if( g_HighLight_Status )
        DrawHighLight( aPanel, DC, g_HighLight_NetCode );

    // draw the BOARD's markers last, otherwise the high light will erase
    // any marker on a pad
    for( unsigned i=0; i < m_markers.size();  ++i )
    {
        m_markers[i]->Draw( aPanel, DC, aDrawMode );
    }
}


void BOARD::DrawHighLight( WinEDA_DrawPanel* aDrawPanel, wxDC* DC, int aNetCode )
{
    int draw_mode;

    if( g_HighLight_Status )
        draw_mode = GR_SURBRILL | GR_OR;
    else
        draw_mode = GR_AND | GR_SURBRILL;

#if 0   // does not unhighlight properly
    // redraw the zones with the aNetCode
    for( SEGZONE* zone = m_Zone;   zone;   zone = zone->Next() )
    {
        if( zone->GetNet() == aNetCode )
        {
            zone->Draw( aDrawPanel, DC, draw_mode );
        }
    }
#endif

    // Redraw ZONE_CONTAINERS
    BOARD::ZONE_CONTAINERS& zones = m_ZoneDescriptorList;
    for( BOARD::ZONE_CONTAINERS::iterator zc = zones.begin();  zc!=zones.end();  ++zc )
    {
        if( (*zc)->GetNet() == aNetCode )
        {
            (*zc)->Draw( aDrawPanel, DC, draw_mode );
        }
    }

    // Redraw any pads that have aNetCode
    for( MODULE* module = m_Modules;  module;   module = module->Next() )
    {
        for( D_PAD* pad = module->m_Pads;  pad;  pad = pad->Next() )
        {
            if( pad->GetNet() == aNetCode )
            {
                pad->Draw( aDrawPanel, DC, draw_mode );
            }
        }
    }

    // Redraw track and vias that have aNetCode
    for( TRACK* seg = m_Track;   seg;   seg = seg->Next() )
    {
        if( seg->GetNet() == aNetCode )
        {
            seg->Draw( aDrawPanel, DC, draw_mode );
        }
    }
}


/* Trace the pads of a module in sketch mode.
 * Used to display pads when when the module visibility is set to not visible
 * and we want to see pad through.
 * The pads must appear on the layers selected in MasqueLayer
 */
void Trace_Pads_Only( WinEDA_DrawPanel* panel, wxDC* DC, MODULE* Module,
                      int ox, int oy, int MasqueLayer, int draw_mode )
{
    int                  tmp;
    PCB_SCREEN*          screen;
    WinEDA_BasePcbFrame* frame;

    screen = (PCB_SCREEN*) panel->GetScreen();
    frame  = (WinEDA_BasePcbFrame*) panel->GetParent();

    tmp = frame->m_DisplayPadFill;
    frame->m_DisplayPadFill = FALSE;

    /* Draw pads. */
    for( D_PAD* pad = Module->m_Pads;  pad;  pad = pad->Next() )
    {
        if( (pad->m_Masque_Layer & MasqueLayer) == 0 )
            continue;

        pad->Draw( panel, DC, draw_mode, wxPoint( ox, oy ) );
    }

    frame->m_DisplayPadFill = tmp;
}

