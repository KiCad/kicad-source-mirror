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
 * @file deltrack.cpp
 */

#include <fctsys.h>
#include <class_drawpanel.h>
#include <confirm.h>
#include <wxPcbStruct.h>
#include <macros.h>
#include <pcbcommon.h>
#include <ratsnest_data.h>

#include <class_board.h>
#include <class_track.h>

#include <pcbnew.h>
#include <protos.h>


TRACK* PCB_EDIT_FRAME::Delete_Segment( wxDC* DC, TRACK* aTrack )
{
    int current_net_code;

    if( aTrack == NULL )
        return NULL;

    if( aTrack->IsNew() )  // Trace in progress, erase the last segment
    {
        if( g_CurrentTrackList.GetCount() > 0 )
        {
            LAYER_NUM previous_layer = GetActiveLayer();

            DBG( g_CurrentTrackList.VerifyListIntegrity(); )

            // Delete the current trace
            ShowNewTrackWhenMovingCursor( m_canvas, DC, wxDefaultPosition, false );

            // delete the most recently entered
            delete g_CurrentTrackList.PopBack();

            if( g_TwoSegmentTrackBuild )
            {
                // if in 2 track mode, and the next most recent is a segment
                // not a via, and the one previous to that is a via, then
                // delete up to the via.
                if( g_CurrentTrackList.GetCount() >= 2
                    && g_CurrentTrackSegment->Type() != PCB_VIA_T
                    && g_CurrentTrackSegment->Back()->Type() == PCB_VIA_T )
                {
                    delete g_CurrentTrackList.PopBack();
                }
            }

            while( g_CurrentTrackSegment && g_CurrentTrackSegment->Type() == PCB_VIA_T )
            {
                delete g_CurrentTrackList.PopBack();

                if( g_CurrentTrackSegment && g_CurrentTrackSegment->Type() != PCB_VIA_T )
                    previous_layer = g_CurrentTrackSegment->GetLayer();
            }

            // Correct active layer which could change if a via
            // has been erased
            SetActiveLayer( previous_layer );

            UpdateStatusBar();

            if( g_TwoSegmentTrackBuild )   // We must have 2 segments or more, or 0
            {
                if( g_CurrentTrackList.GetCount() == 1
                    && g_CurrentTrackSegment->Type() != PCB_VIA_T )
                {
                    delete g_CurrentTrackList.PopBack();
                }
            }

            if( g_CurrentTrackList.GetCount() == 0 )
            {
                m_canvas->SetMouseCapture( NULL, NULL );

                if( GetBoard()->IsHighLightNetON() )
                    HighLight( DC );

                SetCurItem( NULL );
                return NULL;
            }
            else
            {
                if( m_canvas->IsMouseCaptured() )
                    m_canvas->CallMouseCapture( DC, wxDefaultPosition, false );

                return g_CurrentTrackSegment;
            }
        }
        return NULL;
    }

    current_net_code = aTrack->GetNetCode();

    DLIST<TRACK>* container = (DLIST<TRACK>*)aTrack->GetList();
    wxASSERT( container );
    GetBoard()->GetRatsnest()->Remove( aTrack );
    aTrack->ViewRelease();
    container->Remove( aTrack );

    // redraw the area where the track was
    m_canvas->RefreshDrawingRect( aTrack->GetBoundingBox() );

    SaveCopyInUndoList( aTrack, UR_DELETED );
    OnModify();
    TestNetConnection( DC, current_net_code );
    SetMsgPanel( GetBoard() );

    return NULL;
}


void PCB_EDIT_FRAME::Delete_Track( wxDC* DC, TRACK* aTrack )
{
    if( aTrack != NULL )
    {
        int current_net_code = aTrack->GetNetCode();
        Remove_One_Track( DC, aTrack );
        OnModify();
        TestNetConnection( DC, current_net_code );
    }
}


void PCB_EDIT_FRAME::Delete_net( wxDC* DC, TRACK* aTrack )
{
    if( aTrack == NULL )
        return;

    if( !IsOK( this, _( "Delete NET?" ) ) )
        return;

    PICKED_ITEMS_LIST itemsList;
    ITEM_PICKER       picker( NULL, UR_DELETED );
    int    net_code_delete = aTrack->GetNetCode();

    /* Search the first item for the given net code */
    TRACK* trackList = GetBoard()->m_Track->GetStartNetCode( net_code_delete );

    /* Remove all segments having the given net code */
    int    ii = 0;
    TRACK* next_track;
    for( TRACK* segm = trackList;  segm; segm = next_track, ++ii )
    {
        next_track = segm->Next();
        if( segm->GetNetCode() != net_code_delete )
            break;

        GetBoard()->GetRatsnest()->Remove( segm );
        segm->ViewRelease();
        GetBoard()->m_Track.Remove( segm );

        // redraw the area where the track was
        m_canvas->RefreshDrawingRect( segm->GetBoundingBox() );
        picker.SetItem( segm );
        itemsList.PushItem( picker );
    }

    SaveCopyInUndoList( itemsList, UR_DELETED );
    OnModify();
    TestNetConnection( DC, net_code_delete );
    SetMsgPanel( GetBoard() );
}


void PCB_EDIT_FRAME::Remove_One_Track( wxDC* DC, TRACK* pt_segm )
{
    int segments_to_delete_count;

    if( pt_segm == NULL )
        return;

    TRACK* trackList = GetBoard()->MarkTrace( pt_segm, &segments_to_delete_count,
                                              NULL, NULL, true );

    if( segments_to_delete_count == 0 )
        return;

    int net_code = pt_segm->GetNetCode();
    PICKED_ITEMS_LIST itemsList;
    ITEM_PICKER       picker( NULL, UR_DELETED );

    int               ii = 0;
    TRACK*            tracksegment = trackList;
    TRACK*            next_track;

    for( ; ii < segments_to_delete_count; ii++, tracksegment = next_track )
    {
        next_track = tracksegment->Next();
        tracksegment->SetState( BUSY, false );

        DBG( std::cout << __func__ << ": track " << tracksegment << " status=" \
                     << TO_UTF8( TRACK::ShowState( tracksegment->GetStatus() ) ) \
                     << std::endl; )

        GetBoard()->GetRatsnest()->Remove( tracksegment );
        tracksegment->ViewRelease();
        GetBoard()->m_Track.Remove( tracksegment );

        // redraw the area where the track was
        m_canvas->RefreshDrawingRect( tracksegment->GetBoundingBox() );
        picker.SetItem( tracksegment );
        itemsList.PushItem( picker );
    }

    SaveCopyInUndoList( itemsList, UR_DELETED );

    if( net_code > 0 )
        TestNetConnection( DC, net_code );
}
