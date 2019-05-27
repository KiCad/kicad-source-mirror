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
 * @file move_or_drag_track.cpp
 * @brief Track editing routines to move and drag track segments or node.
 */

#include <fctsys.h>
#include <class_drawpanel.h>
#include <confirm.h>
#include <pcb_edit_frame.h>
#include <trigo.h>
#include <macros.h>
#include <gr_basic.h>

#include <class_board.h>

#include <pcbnew.h>
#include <drc.h>
#include <drag.h>
#include <pcbnew_id.h>


static PICKED_ITEMS_LIST s_ItemsListPicker;


// Place a dragged (or moved) track segment or via
bool PCB_EDIT_FRAME::PlaceDraggedOrMovedTrackSegment( TRACK* Track, wxDC* DC )
{
    int        errdrc;

    if( Track == NULL )
        return false;

    int current_net_code = Track->GetNetCode();

    // DRC control:
    if( Settings().m_legacyDrcOn )
    {
        errdrc = m_drc->DrcOnCreatingTrack( Track, GetBoard()->m_Track );

        if( errdrc == BAD_DRC )
            return false;

        // Test the dragged segments
        for( unsigned ii = 0; ii < g_DragSegmentList.size(); ii++ )
        {
            errdrc = m_drc->DrcOnCreatingTrack( g_DragSegmentList[ii].m_Track, GetBoard()->m_Track );

            if( errdrc == BAD_DRC )
                return false;
        }
    }

    // DRC Ok: place track segments
    Track->ClearFlags();
    Track->SetState( IN_EDIT, false );

    // Draw dragged tracks
    for( unsigned ii = 0; ii < g_DragSegmentList.size(); ii++ )
    {
        Track = g_DragSegmentList[ii].m_Track;
        Track->SetState( IN_EDIT, false );
        Track->ClearFlags();

        /* Test the connections modified by the move
         *  (only pad connection must be tested, track connection will be
         * tested by TestNetConnection() ) */
        LSET layerMask( Track->GetLayer() );

        Track->start = GetBoard()->GetPadFast( Track->GetStart(), layerMask );

        if( Track->start )
            Track->SetState( BEGIN_ONPAD, true );
        else
            Track->SetState( BEGIN_ONPAD, false );

        Track->end = GetBoard()->GetPadFast( Track->GetEnd(), layerMask );

        if( Track->end )
            Track->SetState( END_ONPAD, true );
        else
            Track->SetState( END_ONPAD, false );
    }

    EraseDragList();

    SaveCopyInUndoList( s_ItemsListPicker, UR_UNSPECIFIED );
    s_ItemsListPicker.ClearItemsList(); // s_ItemsListPicker is no more owner of picked items

    GetBoard()->PopHighLight();

    OnModify();
    m_canvas->SetMouseCapture( NULL, NULL );

    if( current_net_code > 0 )
        TestNetConnection( DC, current_net_code );

    m_canvas->Refresh();

    return true;
}
