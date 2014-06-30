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
 * @file tr_modif.cpp
 * @brief Trace editing: detects an removes a track which is become redunding,
 * after a new track is craeted.
 */

#include <fctsys.h>
#include <gr_basic.h>
#include <wxPcbStruct.h>

#include <class_board.h>
#include <class_track.h>

#include <pcbnew.h>
#include <protos.h>


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
    int     ii, jj, nb_segm, nbconnect;
    wxPoint start;
    wxPoint end;
    LSET startmasklayer, endmasklayer;

    int     netcode = aNewTrack->GetNetCode();

    /* Reconstruct the complete track (the new track has to start on a segment of track).
     */
    ListSetState( aNewTrack, aNewTrackSegmentsCount, BUSY, false );

    /* If the new track begins with a via, complete the track segment using
     * the following segment as a reference because a  via is often a hub of
     * segments, and does not characterize track.
     */
    if( aNewTrack->Type() == PCB_VIA_T && ( aNewTrackSegmentsCount > 1 ) )
        aNewTrack = aNewTrack->Next();

    aNewTrack = GetBoard()->MarkTrace( aNewTrack, &aNewTrackSegmentsCount, NULL, NULL, true );
    wxASSERT( aNewTrack );

#if 0 && defined(DEBUG)
    TRACK* EndNewTrack;      // The last segment of the list chained to the track

    EndNewTrack = aNewTrack;

    for( ii = 1;  ii < aNewTrackSegmentsCount; ii++ )
    {
        wxASSERT( EndNewTrack->GetState( -1 ) != 0 );
        D( printf( "track %p is newly part of net %d\n", EndNewTrack, netcode ); )
        EndNewTrack = EndNewTrack->Next();
    }

    wxASSERT( EndNewTrack->GetState( -1 ) != 0 );
    D( printf( "track %p is newly part of net %d\n", EndNewTrack, netcode ); )

    for( TRACK* track = m_Pcb->m_Track;  track;  track = track->Next() )
        track->Show( 0, std::cout );

#endif

    TRACK* bufStart = m_Pcb->m_Track->GetStartNetCode( netcode ); // Beginning of tracks of the net
    TRACK* bufEnd = bufStart->GetEndNetCode( netcode );           // End of tracks of the net

    // Flags for cleaning the net.
    for( pt_del = bufStart;  pt_del;  pt_del = pt_del->Next() )
    {
//        D( std::cout<<"track "<<pt_del<<" turning off BUSY | IN_EDIT | IS_LINKED"<<std::endl; )
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

        pt_del = GetBoard()->MarkTrace( pt_del, &nb_segm, NULL, NULL, true );

        /* Test if the marked track is redundant, i.e. if one of marked segments
         * is connected to the starting point of the new track.
         */
        ii = 0;
        pt_segm = pt_del;

        for( ; pt_segm && (ii < nb_segm); pt_segm = pt_segm->Next(), ii++ )
        {
            if( pt_segm->GetState( BUSY ) == 0 )
                break;

            if( pt_segm->GetStart() == start || pt_segm->GetEnd() == start )
            {
                // Marked track can be erased.
                TRACK* NextS;

                DrawTraces( m_canvas, aDC, pt_del, nb_segm, GR_XOR | GR_HIGHLIGHT );

                for( jj = 0; jj < nb_segm; jj++, pt_del = NextS )
                {
                    NextS = pt_del->Next();

                    if( aItemsListPicker )
                    {
                        pt_del->UnLink();
                        pt_del->SetStatus( 0 );
                        pt_del->ClearFlags();
                        ITEM_PICKER picker( pt_del, UR_DELETED );
                        aItemsListPicker->PushItem( picker );
                    }
                    else
                    {
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
