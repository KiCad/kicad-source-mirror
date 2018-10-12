/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2015 Jean-Pierre Charras, jean-pierre.charras@ujf-grenoble.fr
 * Copyright (C) 2012 SoftPLC Corporation, Dick Hollenbeck <dick@softplc.com>
 * Copyright (C) 2012 Wayne Stambaugh <stambaughw@verizon.net>
 * Copyright (C) 1992-2015 KiCad Developers, see AUTHORS.txt for contributors.
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
 * @file editrack-part2.cpp
 */


#include <fctsys.h>
#include <gr_basic.h>
#include <class_drawpanel.h>
#include <confirm.h>
#include <pcb_edit_frame.h>

#include <class_board.h>
#include <class_module.h>
#include <class_track.h>
#include <class_marker_pcb.h>

#include <pcbnew.h>
#include <drc.h>

#include <connectivity/connectivity_data.h>


bool PCB_EDIT_FRAME::Other_Layer_Route( TRACK* aTrack, wxDC* DC )
{
    unsigned    itmp;

    if( aTrack == NULL )
    {
        if( GetActiveLayer() != GetScreen()->m_Route_Layer_TOP )
            SetActiveLayer( GetScreen()->m_Route_Layer_TOP );
        else
            SetActiveLayer( GetScreen()->m_Route_Layer_BOTTOM );

        UpdateStatusBar();
        return true;
    }

    // Avoid more than one via on the current location:
    if( GetBoard()->GetViaByPosition( g_CurrentTrackSegment->GetEnd(),
                                      g_CurrentTrackSegment->GetLayer() ) )
        return false;

    for( TRACK* segm = g_FirstTrackSegment;  segm;  segm = segm->Next() )
    {
        if( segm->Type() == PCB_VIA_T && g_CurrentTrackSegment->GetEnd() == segm->GetStart() )
            return false;
    }

    // Is the current segment Ok (no DRC error) ?
    if( Settings().m_legacyDrcOn )
    {
        if( BAD_DRC==m_drc->DrcOnCreatingTrack( g_CurrentTrackSegment, GetBoard()->m_Track ) )
            // DRC error, the change layer is not made
            return false;

        // Handle 2 segments.
        if( Settings().m_legacyUseTwoSegmentTracks && g_CurrentTrackSegment->Back() )
        {
            if( BAD_DRC == m_drc->DrcOnCreatingTrack( g_CurrentTrackSegment->Back(),
                                                      GetBoard()->m_Track ) )
                return false;
        }
    }

    /* Save current state before placing a via.
     * If the via cannot be placed this current state will be reused
     */
    itmp = g_CurrentTrackList.GetCount();
    Begin_Route( g_CurrentTrackSegment, DC );

    m_canvas->CallMouseCapture( DC, wxDefaultPosition, false );

    // create the via
    VIA* via = new VIA( GetBoard() );
    via->SetFlags( IS_NEW );
    via->SetViaType( GetDesignSettings().m_CurrentViaType );
    via->SetNetCode( GetBoard()->GetHighLightNetCode() );
    via->SetPosition( g_CurrentTrackSegment->GetEnd() );

    // for microvias, the size and hole will be changed later.
    via->SetWidth( GetDesignSettings().GetCurrentViaSize());
    via->SetDrill( GetDesignSettings().GetCurrentViaDrill() );

    // Usual via is from copper to component.
    // layer pair is B_Cu and F_Cu.
    via->SetLayerPair( B_Cu, F_Cu );

    PCB_LAYER_ID first_layer = GetActiveLayer();
    PCB_LAYER_ID last_layer;

    // prepare switch to new active layer:
    if( first_layer != GetScreen()->m_Route_Layer_TOP )
        last_layer = GetScreen()->m_Route_Layer_TOP;
    else
        last_layer = GetScreen()->m_Route_Layer_BOTTOM;

    // Adjust the actual via layer pair
    switch( via->GetViaType() )
    {
    case VIA_BLIND_BURIED:
        via->SetLayerPair( first_layer, last_layer );
        break;

    case VIA_MICROVIA:  // from external to the near neighbor inner layer
        {
            PCB_LAYER_ID last_inner_layer = ToLAYER_ID( ( GetBoard()->GetCopperLayerCount() - 2 ) );

            if( first_layer == B_Cu )
                last_layer = last_inner_layer;
            else if( first_layer == F_Cu )
                last_layer = In1_Cu;
            else if( first_layer == last_inner_layer )
                last_layer = B_Cu;
            else if( first_layer == In1_Cu )
                last_layer = F_Cu;
            // else error: will be removed later
            via->SetLayerPair( first_layer, last_layer );

            // Update diameter and hole size, which where set previously
            // for normal vias
            NETINFO_ITEM* net = via->GetNet();
            via->SetWidth( net->GetMicroViaSize() );
            via->SetDrill( net->GetMicroViaDrillSize() );
        }
        break;

    default:
        break;
    }

    if( Settings().m_legacyDrcOn &&
        BAD_DRC == m_drc->DrcOnCreatingTrack( via, GetBoard()->m_Track ) )
    {
        // DRC fault: the Via cannot be placed here ...
        delete via;

        m_canvas->CallMouseCapture( DC, wxDefaultPosition, false );

        // delete the track(s) added in Begin_Route()
        while( g_CurrentTrackList.GetCount() > itmp )
        {
            Delete_Segment( DC, g_CurrentTrackSegment );
        }

         SetCurItem( g_CurrentTrackSegment, false );

        // Refresh DRC diag, erased by previous calls
        if( m_drc->GetCurrentMarker() )
            SetMsgPanel( m_drc->GetCurrentMarker() );

        return false;
    }

    SetActiveLayer( last_layer );

    TRACK*  lastNonVia = g_CurrentTrackSegment;

    /* A new via was created. It was Ok.
     */
    g_CurrentTrackList.PushBack( via );

    /* The via is now in linked list and we need a new track segment
     * after the via, starting at via location.
     * it will become the new current segment (from via to the mouse cursor)
     */

    TRACK* track = (TRACK*)lastNonVia->Clone();

    /* the above creates a new segment from the last entered segment, with the
     * current width, flags, netcode, etc... values.
     * layer, start and end point are not correct,
     * and will be modified next
     */

    // set the layer to the new value
    track->SetLayer( GetActiveLayer() );

    /* the start point is the via position and the end point is the cursor
     * which also is on the via (will change when moving mouse)
     */
    track->SetEnd( via->GetStart() );
    track->SetStart( via->GetStart() );

    g_CurrentTrackList.PushBack( track );

    if( Settings().m_legacyUseTwoSegmentTracks )
    {
        // Create a second segment (we must have 2 track segments to adjust)
        g_CurrentTrackList.PushBack( (TRACK*)g_CurrentTrackSegment->Clone() );
    }

    m_canvas->CallMouseCapture( DC, wxDefaultPosition, false );
    SetMsgPanel( via );
    UpdateStatusBar();

    return true;
}

static void ListSetState( EDA_ITEM* Start, int NbItem, STATUS_FLAGS State,
                          bool onoff );


void DrawTraces( EDA_DRAW_PANEL* panel, wxDC* DC, TRACK* aTrackList, int nbsegment,
                 GR_DRAWMODE draw_mode )
{
    // preserve the start of the list for debugging.
    for( TRACK* track = aTrackList; nbsegment > 0 && track; nbsegment--, track = track->Next() )
    {
        track->Draw( panel, DC, draw_mode );
    }
}


/*
 * This function try to remove an old track, when a new track is created,
 * and the old track is no more needed
 */
int PCB_EDIT_FRAME::EraseRedundantTrack( wxDC*              aDC,
                                         TRACK*             aNewTrack,
                                         int                aNewTrackSegmentsCount,
                                         PICKED_ITEMS_LIST* aItemsListPicker )
{
    TRACK*  StartTrack, * EndTrack;
    TRACK*  pt_segm;
    TRACK*  pt_del;
    int     nb_segm, nbconnect;
    wxPoint start;
    wxPoint end;
    LSET startmasklayer, endmasklayer;
    int     netcode = aNewTrack->GetNetCode();

    // Reconstruct the complete track (the new track has to start on a segment of track).
    // Note: aNewTrackSegmentsCount conatins the number of new track segments
    ListSetState( aNewTrack, aNewTrackSegmentsCount, BUSY, false );

    /* If the new track begins with a via, complete the track segment using
     * the following segment as a reference because a  via is often a hub of
     * segments, and does not characterize track.
     */
    if( aNewTrack->Type() == PCB_VIA_T && ( aNewTrackSegmentsCount > 1 ) )
        aNewTrack = aNewTrack->Next();

    // When MarkTrace try to find the entire track, if the starting segment
    // is fully inside a pad, MarkTrace does not find correctly the full trace,
    // because the entire track is the set of segments between 2 nodes
    // (pads or point connecting more than 2 items)
    // so use another (better) starting segment in this case
    TRACK* track_segment = aNewTrack;

    for( int ii = 0; ii < aNewTrackSegmentsCount; ii++ )
    {
        D_PAD* pad_st = m_Pcb->GetPad( aNewTrack->GetStart() );
        D_PAD* pad_end = m_Pcb->GetPad( aNewTrack->GetEnd() );

        if( pad_st && pad_st == pad_end )
            track_segment = aNewTrack->Next();
        else
            break;
    }

    // Mark the full trace containing track_segment, and recalculate the
    // beginning of the trace, and the number of segments, as the new trace
    // can contain also already existing track segments
    aNewTrack = GetBoard()->MarkTrace( GetBoard()->m_Track, track_segment,
                                       &aNewTrackSegmentsCount,
                                       nullptr, nullptr, true );
    wxASSERT( aNewTrack );

    TRACK* bufStart = m_Pcb->m_Track->GetStartNetCode( netcode ); // Beginning of tracks of the net
    TRACK* bufEnd = bufStart->GetEndNetCode( netcode );           // End of tracks of the net

    // Flags for cleaning the net.
    for( pt_del = bufStart;  pt_del;  pt_del = pt_del->Next() )
    {
        // printf( "track %p turning off BUSY | IN_EDIT | IS_LINKED\n", pt_del );
        pt_del->SetState( BUSY | IN_EDIT | IS_LINKED, false );

        if( pt_del == bufEnd )  // Last segment reached
            break;
    }

    if( aNewTrack->GetEndSegments( aNewTrackSegmentsCount, &StartTrack, &EndTrack ) == 0 )
        return 0;

    if( ( StartTrack == NULL ) || ( EndTrack == NULL ) )
        return 0;

    start = StartTrack->GetStart();
    end   = EndTrack->GetEnd();

    // The start and end points cannot be the same.
    if( start == end )
        return 0;

    // Determine layers interconnected these points.
    startmasklayer = StartTrack->GetLayerSet();
    endmasklayer   = EndTrack->GetLayerSet();

    // There may be a via or a pad on the end points.
    pt_segm = m_Pcb->m_Track->GetVia( NULL, start, startmasklayer );

    if( pt_segm )
        startmasklayer |= pt_segm->GetLayerSet();

    if( StartTrack->start && ( StartTrack->start->Type() == PCB_PAD_T ) )
    {
        // Start on pad.
        D_PAD* pad = (D_PAD*) StartTrack->start;
        startmasklayer |= pad->GetLayerSet();
    }

    pt_segm = m_Pcb->m_Track->GetVia( NULL, end, endmasklayer );

    if( pt_segm )
        endmasklayer |= pt_segm->GetLayerSet();

    if( EndTrack->end && ( EndTrack->end->Type() == PCB_PAD_T ) )
    {
        D_PAD* pad = (D_PAD*) EndTrack->end;
        endmasklayer |= pad->GetLayerSet();
    }

    // Mark as deleted a new track (which is not involved in the search for other connections)
    ListSetState( aNewTrack, aNewTrackSegmentsCount, IS_DELETED, true );

    /* A segment must be connected to the starting point, otherwise
     * it is unnecessary to analyze the other point
     */
    pt_segm = GetTrack( bufStart, bufEnd, start, startmasklayer );

    if( pt_segm == NULL )     // Not connected to the track starting point.
    {
        // Clear the delete flag.
        ListSetState( aNewTrack, aNewTrackSegmentsCount, IS_DELETED, false );
        return 0;
    }

    /* Marking a list of candidate segmented connect to endpoint
     * Note: the vias are not taken into account because they do
     * not define a track, since they are on an intersection.
     */
    for( pt_del = bufStart, nbconnect = 0; ; )
    {
        pt_segm = GetTrack( pt_del, bufEnd, end, endmasklayer );

        if( pt_segm == NULL )
            break;

        if( pt_segm->Type() != PCB_VIA_T )
        {
            if( pt_segm->GetState( IS_LINKED ) == 0 )
            {
                pt_segm->SetState( IS_LINKED, true );
                nbconnect++;
            }
        }

        if( pt_del == bufEnd )
            break;

        pt_del = pt_segm->Next();
    }

    if( nbconnect == 0 )
    {
        // Clear used flags
        for( pt_del = bufStart; pt_del; pt_del = pt_del->Next() )
        {
            pt_del->SetState( BUSY | IS_DELETED | IN_EDIT | IS_LINKED, false );

            if( pt_del == bufEnd )  // Last segment reached
                break;
        }

        return 0;
    }

    // Mark trace as edited (which does not involve searching for other tracks)
    ListSetState( aNewTrack, aNewTrackSegmentsCount, IS_DELETED, false );
    ListSetState( aNewTrack, aNewTrackSegmentsCount, IN_EDIT, true );

    // Test all marked segments.
    while( nbconnect )
    {
        for( pt_del = bufStart; pt_del; pt_del = pt_del->Next() )
        {
            if( pt_del->GetState( IS_LINKED ) )
                break;

            if( pt_del == bufEnd )  // Last segment reached
                break;
        }

        nbconnect--;

        if( pt_del )
            pt_del->SetState( IS_LINKED, false );

        pt_del = GetBoard()->MarkTrace( GetBoard()->m_Track, pt_del, &nb_segm,
                                        NULL, NULL, true );

        /* Test if the marked track is redundant, i.e. if one of marked segments
         * is connected to the starting point of the new track.
         */

        pt_segm = pt_del;

        for( int ii = 0; pt_segm && (ii < nb_segm); pt_segm = pt_segm->Next(), ii++ )
        {
            if( pt_segm->GetState( BUSY ) == 0 )
                break;

            if( pt_segm->GetStart() == start || pt_segm->GetEnd() == start )
            {
                // Marked track can be erased.
                TRACK* NextS;

                DrawTraces( m_canvas, aDC, pt_del, nb_segm, GR_XOR | GR_HIGHLIGHT );

                for( int jj = 0; jj < nb_segm; jj++, pt_del = NextS )
                {
                    NextS = pt_del->Next();

                    if( aItemsListPicker )
                    {
                        pt_del->UnLink();
                        pt_del->SetStatus( 0 );
                        pt_del->ClearFlags();
                        GetBoard()->GetConnectivity()->Remove( pt_del );
                        ITEM_PICKER picker( pt_del, UR_DELETED );
                        aItemsListPicker->PushItem( picker );
                    }
                    else
                    {
                        GetBoard()->GetConnectivity()->Remove( pt_del );
                        pt_del->DeleteStructure();
                    }
                }

                // Clean up flags.
                for( pt_del = m_Pcb->m_Track; pt_del != NULL; pt_del = pt_del->Next() )
                {
                    if( pt_del->GetState( IN_EDIT ) )
                    {
                        pt_del->SetState( IN_EDIT, false );

                        if( aDC )
                            pt_del->Draw( m_canvas, aDC, GR_OR );
                    }

                    pt_del->SetState( IN_EDIT | IS_LINKED, false );
                }

                return 1;
            }
        }

        // Clear BUSY flag here because the track did not get marked.
        ListSetState( pt_del, nb_segm, BUSY, false );
    }

    // Clear used flags
    for( pt_del = m_Pcb->m_Track; pt_del; pt_del = pt_del->Next() )
    {
        pt_del->SetState( BUSY | IS_DELETED | IN_EDIT | IS_LINKED, false );

        if( pt_del == bufEnd )  // Last segment reached
            break;
    }

    return 0;
}


/* Set the bits of .m_State member to on/off value, using bit mask State
 * of a list of EDA_ITEM
 */
static void ListSetState( EDA_ITEM* Start, int NbItem, STATUS_FLAGS State,
                          bool onoff )
{
    for( ; (Start != NULL ) && ( NbItem > 0 ); NbItem--, Start = Start->Next() )
    {
        Start->SetState( State, onoff );
    }
}
