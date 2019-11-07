/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2012 Jean-Pierre Charras, jean-pierre.charras@ujf-grenoble.fr
 * Copyright (C) 2012 SoftPLC Corporation, Dick Hollenbeck <dick@softplc.com>
 * Copyright (C) 2012 Wayne Stambaugh <stambaughw@verizon.net>
 * Copyright (C) 1992-2019 KiCad Developers, see AUTHORS.txt for contributors.
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

#include <fctsys.h>
#include <pcb_edit_frame.h>
#include <base_units.h>
#include <class_board.h>
#include <class_module.h>
#include <class_track.h>
#include <class_zone.h>
#include <class_marker_pcb.h>
#include <pcbnew.h>
#include <footprint_edit_frame.h>
#include <pcbplot.h>
#include <wx/overlay.h>


// Local functions:
/* Trace the pads of a module in sketch mode.
 * Used to display pads when when the module visibility is set to not visible
 * and we want to see pad through.
 * The pads must appear on the layers selected in LayerMask
 */
static void Trace_Pads_Only( PCB_BASE_FRAME* aFrame, wxDC* DC, MODULE* Module,
                             int ox, int oy, LSET LayerMask );


// Redraw the BOARD items but not cursors, axis or grid
void BOARD::Print( PCB_BASE_FRAME* aFrame, wxDC* DC, const wxPoint& offset )
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
    for( auto track : m_tracks )
    {
        if( track->IsMoving() )
            continue;

        track->Print( aFrame, DC );
    }

    // Draw areas (i.e. zones)
    for( int ii = 0; ii < GetAreaCount(); ii++ )
    {
        ZONE_CONTAINER* zone = GetArea( ii );

        // Areas must be drawn here only if not moved or dragged,
        // because these areas are drawn by ManageCursor() in a specific manner
        if( ( zone->GetEditFlags() & (IN_EDIT | IS_DRAGGED | IS_MOVED) ) == 0 )
        {
            zone->Print( aFrame, DC );
            zone->PrintFilledArea( aFrame, DC );
        }
    }

    // Draw the graphic items
    for( auto item : m_drawings )
    {
        if( item->IsMoving() )
            continue;

        switch( item->Type() )
        {
        case PCB_DIMENSION_T:
        case PCB_TEXT_T:
        case PCB_TARGET_T:
        case PCB_LINE_T:
            item->Print( aFrame, DC );
            break;

        default:
            break;
        }
    }

    LSET all_cu = LSET::AllCuMask();

    for( auto module : m_modules )
    {
        bool    display = true;
        LSET    layerMask = all_cu;

        if( module->IsMoving() )
            continue;

        if( !IsElementVisible( LAYER_MOD_FR ) )
        {
            if( module->GetLayer() == F_Cu )
                display = false;

            layerMask.set( F_Cu, false );
        }

        if( !IsElementVisible( LAYER_MOD_BK ) )
        {
            if( module->GetLayer() == B_Cu )
                display = false;

            layerMask.set( B_Cu, false );
        }

        if( display )
            module->Print( aFrame, DC );
        else
            Trace_Pads_Only( aFrame, DC, module, 0, 0, layerMask );
    }

    // draw the BOARD's markers last, otherwise the high light will erase any marker on a pad
    for( unsigned i = 0; i < m_markers.size(); ++i )
    {
        m_markers[i]->Print( aFrame, DC );
    }
}


/* Trace the pads of a module in sketch mode.
 * Used to display pads when when the module visibility is set to not visible
 * and we want to see pad through.
 * The pads must appear on the layers selected in LayerMask
 */
static void Trace_Pads_Only( PCB_BASE_FRAME* aFrame, wxDC* DC, MODULE* aModule,
                             int ox, int oy, LSET aLayerMask )
{
    PCB_DISPLAY_OPTIONS  displ_opts = aFrame->GetDisplayOptions();
    int                  tmp = displ_opts.m_DisplayPadFill;

    displ_opts.m_DisplayPadFill = false;
    aFrame->SetDisplayOptions( displ_opts );

    // Draw pads.
    for( auto pad : aModule->Pads() )
    {
        if( (pad->GetLayerSet() & aLayerMask) == 0 )
            continue;

        pad->Print( aFrame, DC, wxPoint( ox, oy ) );
    }

    displ_opts.m_DisplayPadFill = tmp;
    aFrame->SetDisplayOptions( displ_opts );
}
