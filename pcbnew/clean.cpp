/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2004-2010 Jean-Pierre Charras, jean-pierre.charras@gpisa-lab.inpg.fr
 * Copyright (C) 2011 Wayne Stambaugh <stambaughw@verizon.net>
 * Copyright (C) 1992-2011 KiCad Developers, see change_log.txt for contributors.
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
 * @file clean.cpp
 * @brief functions to clean tracks: remove null lenght and redundant segments
 */


#include "fctsys.h"
#include "class_drawpanel.h"
#include "pcbcommon.h"
#include "wxPcbStruct.h"
#include "pcbnew.h"
#include "class_board.h"
#include "class_track.h"

/* local functions : */
static void     clean_segments( PCB_EDIT_FRAME* aFrame );
static void     clean_vias( BOARD* aPcb );
static void     DeleteUnconnectedTracks( PCB_EDIT_FRAME* aFrame );
static TRACK*   MergeColinearSegmentIfPossible( BOARD* aPcb, TRACK* aTrackRef,
                                                TRACK* aCandidate, int aEndType );
static void     CleanupTracks( PCB_EDIT_FRAME* aFrame,
                             bool aCleanVias, bool aMergeSegments,
                             bool aDeleteUnconnectedSegm, bool aConnectToPads );

#include "dialog_cleaning_options.h"

#define CONN2PAD_ENBL

#ifdef CONN2PAD_ENBL
static void ConnectDanglingEndToPad( PCB_EDIT_FRAME* frame );
static void ConnectDanglingEndToVia( BOARD* pcb );
#endif


/* Install the track operation dialog frame
*/
void PCB_EDIT_FRAME::Clean_Pcb( wxDC* DC )
{
    DIALOG_CLEANING_OPTIONS::connectToPads = false;
    DIALOG_CLEANING_OPTIONS dlg( this );

    if( dlg.ShowModal() == wxID_OK )
        CleanupTracks( this, dlg.cleanVias, dlg.mergeSegments,
                       dlg.deleteUnconnectedSegm, dlg.connectToPads );

    m_canvas->Refresh( true );
}


/* Main cleaning function.
 *  Delete
 * - Redundant points on tracks (merge aligned segments)
 * - vias on pad
 * - null lenght segments
 *  Create segments when track ends are incorrectly connected:
 *  i.e. when a track end covers a pad or a via but is not exactly on the pad or the via center
 */
void CleanupTracks( PCB_EDIT_FRAME* aFrame,
                    bool aCleanVias, bool aMergeSegments,
                    bool aDeleteUnconnectedSegm, bool aConnectToPads )
{
    wxBusyCursor( dummy );

    aFrame->ClearMsgPanel();
    aFrame->GetBoard()->GetNumSegmTrack();    // update the count

    // Clear undo and redo lists to avoid inconsistencies between lists
    aFrame->GetScreen()->ClearUndoRedoList();
    aFrame->SetCurItem( NULL );

    /* Rebuild the pad infos (pad list and netcodes) to ensure an up to date info */
    aFrame->GetBoard()->m_Status_Pcb = 0;
    aFrame->GetBoard()->BuildListOfNets();

    if( aCleanVias )       // delete redundant vias
    {
        aFrame->SetStatusText( _( "Clean vias" ) );
        clean_vias( aFrame->GetBoard() );
    }

#ifdef CONN2PAD_ENBL
    /* Create missing segments when a track end covers a pad or a via,
     * but is not on the pad  or the via center */
    if( aConnectToPads )
    {
        aFrame->SetStatusText( _( "Reconnect pads" ) );

        /* Create missing segments when a track end covers a pad, but is not on the pad center */
        ConnectDanglingEndToPad( aFrame );

        /* Create missing segments when a track end covers a via, but is not on the via center */
        ConnectDanglingEndToVia( aFrame->GetBoard() );
    }
#endif

    /* Remove null segments and intermediate points on aligned segments */
    if( aMergeSegments )
    {
        aFrame->SetStatusText( _( "Merge track segments" ) );
        clean_segments( aFrame );
    }

    /* Delete dangling tracks */
    if( aDeleteUnconnectedSegm )
    {
        aFrame->SetStatusText( _( "Delete unconnected tracks" ) );
        DeleteUnconnectedTracks( aFrame );
    }

    aFrame->SetStatusText( _( "Cleanup finished" ) );

    aFrame->Compile_Ratsnest( NULL, true );

    aFrame->OnModify();
}


void clean_vias( BOARD * aPcb )
{
    TRACK* track;
    TRACK* next_track;

    for( track = aPcb->m_Track; track; track = track->Next() )
    {
        if( track->GetShape() != VIA_THROUGH )
            continue;

        // Search and delete others vias at same location
        TRACK* alt_track = track->Next();

        for( ; alt_track != NULL; alt_track = next_track )
        {
            next_track = alt_track->Next();

            if( alt_track->m_Shape != VIA_THROUGH )
                continue;

            if( alt_track->m_Start != track->m_Start )
                continue;

            /* delete via */
            alt_track->UnLink();
            delete alt_track;
        }
    }

    /* Delete Via on pads at same location */
    for( track = aPcb->m_Track; track != NULL; track = next_track )
    {
        next_track = track->Next();

        if( track->m_Shape != VIA_THROUGH )
            continue;

        D_PAD* pad = aPcb->GetPadFast( track->m_Start, ALL_CU_LAYERS );

        if( pad && (pad->m_layerMask & EXTERNAL_LAYERS) == EXTERNAL_LAYERS )    // redundant Via
        {
            /* delete via */
            track->UnLink();
            delete track;
        }
    }
}


/*
 *  Delete dangling tracks
 *  Vias:
 *  If a via is only connected to a dangling track, it also will be removed
 */
static void DeleteUnconnectedTracks( PCB_EDIT_FRAME* aFrame )
{
    TRACK*          segment;
    TRACK*          other;
    TRACK*          startNetcode;
    TRACK*          next;
    ZONE_CONTAINER* zone;
    int             masklayer, oldnetcode;
    int             type_end, flag_erase;

    if( aFrame->GetBoard()->m_Track == NULL )
        return;

    aFrame->GetCanvas()->SetAbortRequest( false );

    // correct via m_End defects
    for( segment = aFrame->GetBoard()->m_Track;  segment;  segment = next )
    {
        next = segment->Next();

        if( segment->Type() == PCB_VIA_T )
        {
            if( segment->m_Start != segment->m_End )
                segment->m_End = segment->m_Start;

            continue;
        }
    }

    // removal of unconnected tracks
    segment = startNetcode = aFrame->GetBoard()->m_Track;
    oldnetcode = segment->GetNet();

    for( int ii = 0; segment ; segment = next, ii++ )
    {
        next = segment->Next();

        if( aFrame->GetCanvas()->GetAbortRequest() )
            break;

        if( segment->GetNet() != oldnetcode )
        {
            startNetcode = segment;
            oldnetcode = segment->GetNet();
        }

        flag_erase = 0; //Not connected indicator
        type_end = 0;

        /* Is a pad found on a track end ? */

        masklayer = segment->ReturnMaskLayer();

        D_PAD* pad;

        pad = aFrame->GetBoard()->GetPadFast( segment->m_Start, masklayer );

        if( pad != NULL )
        {
            segment->start = pad;
            type_end |= START_ON_PAD;
        }

        pad = aFrame->GetBoard()->GetPadFast( segment->m_End, masklayer );

        if( pad != NULL )
        {
            segment->end = pad;
            type_end |= END_ON_PAD;
        }

        // if not connected to a pad, test if segment's START is connected to another track
        // For via tests, an enhancement could to test if connected to 2 items on different layers.
        // Currently a via must be connected to 2 items, that can be on the same layer
        int top_layer, bottom_layer;

        if( (type_end & START_ON_PAD ) == 0 )
        {
            other = segment->GetTrace( aFrame->GetBoard()->m_Track, NULL, START );

            if( other == NULL )     // Test a connection to zones
            {
                if( segment->Type() != PCB_VIA_T )
                {
                    zone = aFrame->GetBoard()->HitTestForAnyFilledArea( segment->m_Start,
                                                                       segment->GetLayer() );
                }
                else
                {
                    ((SEGVIA*)segment)->ReturnLayerPair( &top_layer, &bottom_layer );
                    zone = aFrame->GetBoard()->HitTestForAnyFilledArea( segment->m_Start,
                                                                       top_layer, bottom_layer );
                }
            }

            if( (other == NULL) && (zone == NULL) )
            {
                flag_erase |= 1;
            }
            else    // segment, via or zone connected to this end
            {
                segment->start = other;
                // If a via is connected to this end, test if this via has a second item connected
                // if no, remove it with the current segment

                if( other && other->Type() == PCB_VIA_T )
                {
                    // search for another segment following the via

                    segment->SetState( BUSY, ON );

                    SEGVIA* via = (SEGVIA*) other;
                    other = via->GetTrace( aFrame->GetBoard()->m_Track, NULL, START );

                    if( other == NULL )
                    {
                        via->ReturnLayerPair( &top_layer, &bottom_layer );
                        zone = aFrame->GetBoard()->HitTestForAnyFilledArea( via->m_Start,
                                                                           bottom_layer,
                                                                           top_layer );
                    }

                    if( (other == NULL) && (zone == NULL) )
                        flag_erase |= 2;

                    segment->SetState( BUSY, OFF );
                }
            }
        }

        // if not connected to a pad, test if segment's END is connected to another track
        if( (type_end & END_ON_PAD ) == 0 )
        {
            other = segment->GetTrace( aFrame->GetBoard()->m_Track, NULL, END );

            if( other == NULL )     // Test a connection to zones
            {
                if( segment->Type() != PCB_VIA_T )
                {
                    zone = aFrame->GetBoard()->HitTestForAnyFilledArea( segment->m_End,
                                                                       segment->GetLayer() );
                }
                else
                {
                    ((SEGVIA*)segment)->ReturnLayerPair( &top_layer, &bottom_layer );
                    zone = aFrame->GetBoard()->HitTestForAnyFilledArea( segment->m_End,
                                                                       top_layer, bottom_layer );
                }
            }

            if ( (other == NULL) && (zone == NULL) )
            {
                flag_erase |= 0x10;
            }
            else     // segment, via or zone connected to this end
            {
                segment->end = other;

                // If a via is connected to this end, test if this via has a second item connected
                // if no, remove it with the current segment

                if( other && other->Type() == PCB_VIA_T )
                {
                    // search for another segment following the via

                    segment->SetState( BUSY, ON );

                    SEGVIA* via = (SEGVIA*) other;
                    other = via->GetTrace( aFrame->GetBoard()->m_Track, NULL, END );

                    if( other == NULL )
                    {
                        via->ReturnLayerPair( &top_layer, &bottom_layer );
                        zone = aFrame->GetBoard()->HitTestForAnyFilledArea( via->m_End,
                                                                           bottom_layer,
                                                                           top_layer );
                    }

                    if( (other == NULL) && (zone == NULL) )
                        flag_erase |= 0x20;

                    segment->SetState( BUSY, OFF );
                }
            }
        }

        if( flag_erase )
        {
            // update the pointer to start of the contiguous netcode group
            if( segment == startNetcode )
            {
                next = segment->Next();
                startNetcode = next;
            }
            else
            {
                next = startNetcode;
            }

            // remove segment from board
            segment->DeleteStructure();

            if( next == NULL )
                break;
        }
    }
}


/* Delete null length segments, and intermediate points .. */
static void clean_segments( PCB_EDIT_FRAME* aFrame )
{
    TRACK*          segment, * nextsegment;
    TRACK*          other;
    int             ii;
    int             flag, no_inc;
    wxString        msg;

    aFrame->GetCanvas()->SetAbortRequest( false );

    // Delete null segments
    for( segment = aFrame->GetBoard()->m_Track;  segment;  segment = nextsegment )
    {
        nextsegment = segment->Next();

        if( !segment->IsNull() )
            continue;

        /* Length segment = 0; delete it */
        segment->DeleteStructure();
    }

    /* Delete redundant segments */
    for( segment  = aFrame->GetBoard()->m_Track, ii = 0;  segment;  segment = segment->Next(), ii++ )
    {
        for( other = segment->Next(); other; other = nextsegment )
        {
            nextsegment = other->Next();
            int erase = 0;

            if( segment->Type() != other->Type() )
                continue;

            if( segment->GetLayer() != other->GetLayer() )
                continue;

            if( segment->GetNet() != other->GetNet() )
                break;

            if( segment->m_Start == other->m_Start )
            {
                if( segment->m_End == other->m_End )
                    erase = 1;
            }

            if( segment->m_Start == other->m_End )
            {
                if( segment->m_End == other->m_Start )
                    erase = 1;
            }

            /* Delete redundant point */
            if( erase )
            {
                ii--;
                other->DeleteStructure();
            }
        }
    }

    /* delete intermediate points  */
    ii = 0;

    for( segment = aFrame->GetBoard()->m_Track;  segment;  segment = nextsegment )
    {
        TRACK*  segStart;
        TRACK*  segEnd;
        TRACK*  segDelete;

        nextsegment = segment->Next();

        if( aFrame->GetCanvas()->GetAbortRequest() )
            return;

        if( segment->Type() != PCB_TRACE_T )
            continue;

        flag = no_inc = 0;

        // search for a possible point that connects on the START point of the segment
        for( segStart = segment->Next(); ; )
        {
            segStart = segment->GetTrace( segStart, NULL, START );

            if( segStart )
            {
                // the two segments must have the same width
                if( segment->m_Width != segStart->m_Width )
                    break;

                // it cannot be a via
                if( segStart->Type() != PCB_TRACE_T )
                    break;

                /* We must have only one segment connected */
                segStart->SetState( BUSY, ON );
                other = segment->GetTrace( aFrame->GetBoard()->m_Track, NULL, START );
                segStart->SetState( BUSY, OFF );

                if( other == NULL )
                    flag = 1;           /* OK */

                break;
            }
            break;
        }

        if( flag )   // We have the starting point of the segment is connected to an other segment
        {
            segDelete = MergeColinearSegmentIfPossible( aFrame->GetBoard(), segment, segStart,
                                                        START );

            if( segDelete )
            {
                no_inc = 1;
                segDelete->DeleteStructure();
            }
        }

        /* search for a possible point that connects on the END point of the segment: */
        for( segEnd = segment->Next(); ; )
        {
            segEnd = segment->GetTrace( segEnd, NULL, END );

            if( segEnd )
            {
                if( segment->m_Width != segEnd->m_Width )
                    break;

                if( segEnd->Type() != PCB_TRACE_T )
                    break;

                /* We must have only one segment connected */
                segEnd->SetState( BUSY, ON );
                other = segment->GetTrace( aFrame->GetBoard()->m_Track, NULL, END );
                segEnd->SetState( BUSY, OFF );

                if( other == NULL )
                    flag |= 2;          /* Ok */

                break;
            }
            else
            {
                break;
            }
        }

        if( flag & 2 )  // We have the ending point of the segment is connected to an other segment
        {
            segDelete = MergeColinearSegmentIfPossible( aFrame->GetBoard(), segment, segEnd, END );

            if( segDelete )
            {
                no_inc = 1;
                segDelete->DeleteStructure();
            }
        }

        if( no_inc ) /* The current segment was modified, retry to merge it */
            nextsegment = segment->Next();
    }

   return;
}


/* Function used by clean_segments.
 *  Test alignment of aTrackRef and aCandidate (which must have a common end).
 *  and see if the common point is not on a pad (i.e. if this common point can be removed).
 *  the ending point of pt_ref is the start point (aEndType == START)
 *  or the end point (aEndType != START)
 *  if the common point can be deleted, this function
 *    change the common point coordinate of the aTrackRef segm
 *   (and therefore connect the 2 other ending points)
 *    and return aCandidate (which can be deleted).
 *  else return NULL
 */
TRACK* MergeColinearSegmentIfPossible( BOARD* aPcb, TRACK* aTrackRef, TRACK* aCandidate,
                                       int aEndType )
{
    if( aTrackRef->m_Width != aCandidate->m_Width )
        return NULL;

    bool is_colinear = false;

    // Trivial case: superimposed tracks ( tracks, not vias ):
    if( aTrackRef->Type() == PCB_TRACE_T && aCandidate->Type() == PCB_TRACE_T )
    {
        if( aTrackRef->m_Start == aCandidate->m_Start )
            if( aTrackRef->m_End == aCandidate->m_End )
                return aCandidate;

        if( aTrackRef->m_Start == aCandidate->m_End )
            if( aTrackRef->m_End == aCandidate->m_Start )
                return aCandidate;
    }

    int refdx = aTrackRef->m_End.x - aTrackRef->m_Start.x;
    int refdy = aTrackRef->m_End.y - aTrackRef->m_Start.y;

    int segmdx = aCandidate->m_End.x - aCandidate->m_Start.x;
    int segmdy = aCandidate->m_End.y - aCandidate->m_Start.y;

    // test for vertical alignment (easy to handle)
    if( refdx == 0 )
    {
        if( segmdx != 0 )
            return NULL;
        else
            is_colinear = true;
    }

    // test for horizontal alignment (easy to handle)
    if( refdy == 0 )
    {
        if( segmdy != 0 )
            return NULL;
        else
            is_colinear = true;
    }

    /* test if alignment in other cases
     *  We must have refdy/refdx == segmdy/segmdx, (i.e. same slope)
     *   or refdy * segmdx == segmdy * refdx
     */
    if( is_colinear == false )
    {
        if( ( double)refdy * segmdx != (double)refdx * segmdy )
            return NULL;

        is_colinear = true;
    }

    /* Here we have 2 aligned segments:
     * We must change the pt_ref common point only if not on a pad
     * (this function) is called when there is only 2 connected segments,
     *and if this point is not on a pad, it can be removed and the 2 segments will be merged
     */
    if( aEndType == START )
    {
        // We must not have a pad, which is a always terminal point for a track
        if( aPcb->GetPadFast( aTrackRef->m_Start, aTrackRef->ReturnMaskLayer() ) )
            return NULL;

        /* change the common point coordinate of pt_segm to use the other point
         * of pt_segm (pt_segm will be removed later) */
        if( aTrackRef->m_Start == aCandidate->m_Start )
        {
            aTrackRef->m_Start = aCandidate->m_End;
            return aCandidate;
        }
        else
        {
            aTrackRef->m_Start = aCandidate->m_Start;
            return aCandidate;
        }
    }
    else    // aEndType == END
    {
        // We must not have a pad, which is a always terminal point for a track
        if( aPcb->GetPadFast( aTrackRef->m_End, aTrackRef->ReturnMaskLayer() ) )
            return NULL;

        /* change the common point coordinate of pt_segm to use the other point
         * of pt_segm (pt_segm will be removed later) */
        if( aTrackRef->m_End == aCandidate->m_Start )
        {
            aTrackRef->m_End = aCandidate->m_End;
            return aCandidate;
        }
        else
        {
            aTrackRef->m_End = aCandidate->m_Start;
            return aCandidate;
        }
    }

    return NULL;
}


bool PCB_EDIT_FRAME::RemoveMisConnectedTracks( wxDC* aDC )
{
    TRACK*          segment;
    TRACK*          other;
    TRACK*          next;
    int             net_code_s, net_code_e;
    bool            isModified = false;

    for( segment = GetBoard()->m_Track;  segment;  segment = (TRACK*) segment->Next() )
    {
        segment->SetState( FLAG0, OFF );

        // find the netcode for segment using anything connected to the "start" of "segment"
        net_code_s = -1;

        if( segment->start  &&  segment->start->Type()==PCB_PAD_T )
        {
            // get the netcode of the pad to propagate.
            net_code_s = ((D_PAD*)(segment->start))->GetNet();
        }
        else
        {
            other = segment->GetTrace( GetBoard()->m_Track, NULL, START );

            if( other )
                net_code_s = other->GetNet();
        }

        if( net_code_s < 0 )
            continue;           // the "start" of segment is not connected

        // find the netcode for segment using anything connected to the "end" of "segment"
        net_code_e = -1;

        if( segment->end  &&  segment->end->Type()==PCB_PAD_T )
        {
            net_code_e = ((D_PAD*)(segment->end))->GetNet();
        }
        else
        {
            other = segment->GetTrace( GetBoard()->m_Track, NULL, END );

            if( other )
                net_code_e = other->GetNet();
        }

        if( net_code_e < 0 )
            continue;           // the "end" of segment is not connected

        // Netcodes do not agree, so mark the segment as needed to be removed
        if( net_code_s != net_code_e )
        {
            segment->SetState( FLAG0, ON );
        }
    }

    // Remove flagged segments
    for( segment = GetBoard()->m_Track;  segment;  segment = next )
    {
        next = (TRACK*) segment->Next();

        if( segment->GetState( FLAG0 ) )    // Segment is flagged to be removed
        {
            segment->SetState( FLAG0, OFF );
            isModified = true;
            GetBoard()->m_Status_Pcb = 0;
            Remove_One_Track( aDC, segment );

            // the current segment could be deleted, so restart to the beginning
            next = GetBoard()->m_Track;
        }
    }

    return isModified;
}


#if defined(CONN2PAD_ENBL)

/**
 * Function ConnectDanglingEndToPad
 * looks for vias which have no netcode and which are in electrical contact
 * with a track to the degree that the track's end point falls on the via.
 * Note that this is not a rigorous electrical check, but is better than
 * testing for the track endpoint equaling the via center.  When such a via
 * is found, then add a small track to bridge from the overlapping track to
 * the via and change the via's netcode so that subsequent continuity checks
 * can be done with the faster equality algorithm.
 */
static void ConnectDanglingEndToVia( BOARD* pcb )
{
    for( TRACK* track = pcb->m_Track;  track;  track = track->Next() )
    {
        SEGVIA* via;

        if( track->Type()!=PCB_VIA_T  || (via = (SEGVIA*)track)->GetNet()!=0 )
            continue;

        for( TRACK* other = pcb->m_Track;  other;  other = other->Next() )
        {
            if( other == track )
                continue;

            if( !via->IsOnLayer( other->GetLayer() ) )
                continue;

            // if the other track's m_End does not match the via position, and the track's
            // m_Start is within the bounds of the via, and the other track has no start
            if( other->m_End != via->GetPosition() && via->HitTest( other->m_Start )
             && !other->start )
            {
                TRACK* newTrack = other->Copy();

                pcb->m_Track.Insert( newTrack, other->Next() );

                newTrack->m_End = via->GetPosition();

                newTrack->start = other;
                newTrack->end   = via;
                other->start = newTrack;

                via->SetNet( other->GetNet() );

                if( !via->start )
                    via->start = other;

                if( !via->end )
                    via->end = other;
            }

            // if the other track's m_Start does not match the via position, and the track's
            // m_End is within the bounds of the via, and the other track has no end
            else if( other->m_Start != via->GetPosition() && via->HitTest( other->m_End )
                  && !other->end )
            {
                TRACK* newTrack = other->Copy();

                pcb->m_Track.Insert( newTrack, other->Next() );

                newTrack->m_Start = via->GetPosition();

                newTrack->start = via;
                newTrack->end   = other;
                other->end = newTrack;

                via->SetNet( other->GetNet() );

                if( !via->start )
                    via->start = other;

                if( !via->end )
                    via->end = other;
            }
        }
    }
}


/**
 * Function ConnectDanglingEndToPad
 * possibly adds a segment to the end of any and all tracks if their end is not exactly
 * connected into the center of the pad.  This allows faster control of
 * connections.
 */
void ConnectDanglingEndToPad( PCB_EDIT_FRAME* aFrame )
{
    TRACK*          segment;
    int             nb_new_trace = 0;
    wxString        msg;

    aFrame->GetCanvas()->SetAbortRequest( false );

    for( segment = aFrame->GetBoard()->m_Track;  segment;  segment = segment->Next() )
    {
        D_PAD*          pad;

        if( aFrame->GetCanvas()->GetAbortRequest() )
            return;

        pad = aFrame->GetBoard()->GetPad( segment, START );

        if( pad )
        {
            // test if the track start point is not exactly starting on the pad
            if( segment->m_Start != pad->m_Pos )
            {
                if( segment->GetTrace( aFrame->GetBoard()->m_Track, NULL, START ) == NULL )
                {
                    TRACK* newTrack = segment->Copy();

                    aFrame->GetBoard()->m_Track.Insert( newTrack, segment->Next() );

                    newTrack->m_End = pad->m_Pos;
                    newTrack->start = segment;
                    newTrack->end   = pad;

                    nb_new_trace++;
                }
            }
        }

        pad = aFrame->GetBoard()->GetPad( segment, END );

        if( pad )
        {
            // test if the track end point is not exactly on the pad
            if( segment->m_End != pad->m_Pos )
            {
                if( segment->GetTrace( aFrame->GetBoard()->m_Track, NULL, END ) == NULL )
                {
                    TRACK* newTrack = segment->Copy();

                    aFrame->GetBoard()->m_Track.Insert( newTrack, segment->Next() );

                    newTrack->m_Start = pad->m_Pos;

                    newTrack->start = pad;
                    newTrack->end   = segment;
                    nb_new_trace++;
                }
            }
        }
    }
}

#endif
