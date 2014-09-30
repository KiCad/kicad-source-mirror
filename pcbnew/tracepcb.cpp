/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2012 Jean-Pierre Charras, jean-pierre.charras@ujf-grenoble.fr
 * Copyright (C) 2012 SoftPLC Corporation, Dick Hollenbeck <dick@softplc.com>
 * Copyright (C) 2012 Wayne Stambaugh <stambaughw@verizon.net>
 * Copyright (C) 1992-2012 KiCad Developers, see AUTHORS.txt for contributors.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, you may find one here:
 * http://www.gnu.org/licenses/old-licenses/gpl-2.0.html
 * or you may search the http://www.gnu.org website for the version 2 license,
 * or you may write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
 */

/**
 * @file tracepcb.cpp
 * @brief Functions to redraw the current board.
 */

#include <fctsys.h>
#include <class_drawpanel.h>
#include <wxPcbStruct.h>
#include <base_units.h>

#include <class_board.h>
#include <class_module.h>
#include <class_track.h>
#include <class_zone.h>
#include <class_marker_pcb.h>

#include <pcbnew.h>
#include <module_editor_frame.h>
#include <pcbplot.h>

#include <wx/overlay.h>


// Local functions:
/* Trace the pads of a module in sketch mode.
 * Used to display pads when when the module visibility is set to not visible
 * and we want to see pad through.
 * The pads must appear on the layers selected in LayerMask
 */
static void Trace_Pads_Only( EDA_DRAW_PANEL* panel, wxDC* DC, MODULE* Module,
                             int ox, int oy, LSET LayerMask, GR_DRAWMODE draw_mode );


void FOOTPRINT_EDIT_FRAME::RedrawActiveWindow( wxDC* DC, bool EraseBg )
{
    PCB_SCREEN* screen = (PCB_SCREEN*) GetScreen();

    if( !GetBoard() || !screen )
        return;

    GRSetDrawMode( DC, GR_COPY );

    m_canvas->DrawBackGround( DC );
    DrawWorkSheet( DC, screen, 0, IU_PER_MILS, wxEmptyString );

    // Redraw the footprints
    for( MODULE* module = GetBoard()->m_Modules; module; module = module->Next() )
    {
        module->Draw( m_canvas, DC, GR_OR );
    }

#ifdef USE_WX_OVERLAY

    if( IsShown() )
    {
        m_overlay.Reset();
        wxDCOverlay overlaydc( m_overlay, (wxWindowDC*) DC );
        overlaydc.Clear();
    }

#endif

    if( m_canvas->IsMouseCaptured() )
        m_canvas->CallMouseCapture( DC, wxDefaultPosition, false );

    // Redraw the cursor
    m_canvas->DrawCrossHair( DC );
}


/* Draw the BOARD, and others elements : axis, grid ..
 */
void PCB_EDIT_FRAME::RedrawActiveWindow( wxDC* DC, bool EraseBg )
{
    PCB_SCREEN* screen = GetScreen();

    if( !GetBoard() || !screen )
        return;

    GRSetDrawMode( DC, GR_COPY );

    m_canvas->DrawBackGround( DC );

    DrawWorkSheet( DC, GetScreen(), g_DrawDefaultLineThickness,
                    IU_PER_MILS, GetBoard()->GetFileName() );

    GetBoard()->Draw( m_canvas, DC, GR_OR | GR_ALLOW_HIGHCONTRAST );

    DrawGeneralRatsnest( DC );

#ifdef USE_WX_OVERLAY

    if( IsShown() )
    {
        m_overlay.Reset();
        wxDCOverlay overlaydc( m_overlay, (wxWindowDC*) DC );
        overlaydc.Clear();
    }

#endif

    if( m_canvas->IsMouseCaptured() )
        m_canvas->CallMouseCapture( DC, wxDefaultPosition, false );

    // Redraw the cursor
    m_canvas->DrawCrossHair( DC );
}


// Redraw the BOARD items but not cursors, axis or grid
void BOARD::Draw( EDA_DRAW_PANEL* aPanel, wxDC* DC, GR_DRAWMODE aDrawMode, const wxPoint& offset )
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
    for( TRACK* track = m_Track; track; track = track->Next() )
    {
        if( track->IsMoving() )
            continue;

        track->Draw( aPanel, DC, aDrawMode );
    }

    // SEGZONE is outdated, only her for compatibility with
    // very old designs
    for( SEGZONE* zone = m_Zone; zone; zone = zone->Next() )
    {
        if( zone->IsMoving() )
            continue;

        zone->Draw( aPanel, DC, aDrawMode );
    }

    // Draw the graphic items
    for( BOARD_ITEM* item = m_Drawings; item; item = item->Next() )
    {
        if( item->IsMoving() )
            continue;

        switch( item->Type() )
        {
        case PCB_DIMENSION_T:
        case PCB_TEXT_T:
        case PCB_TARGET_T:
        case PCB_LINE_T:
            item->Draw( aPanel, DC, aDrawMode );
            break;

        default:
            break;
        }
    }

    // Draw areas (i.e. zones)
    for( int ii = 0; ii < GetAreaCount(); ii++ )
    {
        ZONE_CONTAINER* zone = GetArea( ii );

        // Areas must be drawn here only if not moved or dragged,
        // because these areas are drawn by ManageCursor() in a specific manner
        if( ( zone->GetFlags() & (IN_EDIT | IS_DRAGGED | IS_MOVED) ) == 0 )
        {
            zone->Draw( aPanel, DC, aDrawMode );
            zone->DrawFilledArea( aPanel, DC, aDrawMode );
        }
    }

    LSET all_cu = LSET::AllCuMask();

    for( MODULE* module = m_Modules; module; module = module->Next() )
    {
        bool    display = true;
        LSET    layerMask = all_cu;

        if( module->IsMoving() )
            continue;

        if( !IsElementVisible( PCB_VISIBLE( MOD_FR_VISIBLE ) ) )
        {
            if( module->GetLayer() == F_Cu )
                display = false;

            layerMask.set( F_Cu, false );
        }

        if( !IsElementVisible( PCB_VISIBLE( MOD_BK_VISIBLE ) ) )
        {
            if( module->GetLayer() == B_Cu )
                display = false;

            layerMask.set( B_Cu, false );
        }

        if( display )
            module->Draw( aPanel, DC, aDrawMode );
        else
            Trace_Pads_Only( aPanel, DC, module, 0, 0, layerMask, aDrawMode );
    }

    if( IsHighLightNetON() )
        DrawHighLight( aPanel, DC, GetHighLightNetCode() );

    // draw the BOARD's markers last, otherwise the high light will erase any marker on a pad
    for( unsigned i = 0; i < m_markers.size(); ++i )
    {
        m_markers[i]->Draw( aPanel, DC, aDrawMode );
    }
}


void BOARD::DrawHighLight( EDA_DRAW_PANEL* am_canvas, wxDC* DC, int aNetCode )
{
    GR_DRAWMODE draw_mode;

    if( IsHighLightNetON() )
        draw_mode = GR_HIGHLIGHT | GR_OR;
    else
        draw_mode = GR_AND | GR_HIGHLIGHT;

    // Redraw zones
    for( int ii = 0; ii < GetAreaCount(); ii++ )
    {
        ZONE_CONTAINER* zone = GetArea( ii );

        if( zone->GetNetCode() == aNetCode )
        {
            zone->Draw( am_canvas, DC, draw_mode );
        }
    }

    // Redraw any pads that have aNetCode
    for( MODULE* module = m_Modules; module; module = module->Next() )
    {
        for( D_PAD* pad = module->Pads(); pad; pad = pad->Next() )
        {
            if( pad->GetNetCode() == aNetCode )
            {
                pad->Draw( am_canvas, DC, draw_mode );
            }
        }
    }

    // Redraw track and vias that have aNetCode
    for( TRACK* seg = m_Track; seg; seg = seg->Next() )
    {
        if( seg->GetNetCode() == aNetCode )
        {
            seg->Draw( am_canvas, DC, draw_mode );
        }
    }
}


/* Trace the pads of a module in sketch mode.
 * Used to display pads when when the module visibility is set to not visible
 * and we want to see pad through.
 * The pads must appear on the layers selected in LayerMask
 */
static void Trace_Pads_Only( EDA_DRAW_PANEL* panel, wxDC* DC, MODULE* aModule,
                             int ox, int oy, LSET aLayerMask, GR_DRAWMODE draw_mode )
{
    PCB_BASE_FRAME* frame = (PCB_BASE_FRAME*) panel->GetParent();

    int             tmp = frame->m_DisplayPadFill;

    frame->m_DisplayPadFill = false;

    // Draw pads.
    for( D_PAD* pad = aModule->Pads(); pad; pad = pad->Next() )
    {
        if( (pad->GetLayerSet() & aLayerMask) == 0 )
            continue;

        pad->Draw( panel, DC, draw_mode, wxPoint( ox, oy ) );
    }

    frame->m_DisplayPadFill = tmp;
}
