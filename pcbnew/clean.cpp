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


#include <fctsys.h>
#include <class_drawpanel.h>
#include <pcbcommon.h>
#include <wxPcbStruct.h>
#include <pcbnew.h>
#include <class_board.h>
#include <class_track.h>
#include <connect.h>
#include <dialog_cleaning_options.h>

// Helper class used to clean tracks and vias
class TRACKS_CLEANER: CONNECTIONS
{
private:
    BOARD * m_Brd;
    bool m_deleteUnconnectedTracks;
    bool m_mergeSegments;
    bool m_cleanVias;

public:
    TRACKS_CLEANER( BOARD * aPcb );

    /**
     * the cleanup function.
     * return true if some item was modified
     */
    bool CleanupBoard();

    void SetdeleteUnconnectedTracksOpt( bool aDelete )
    {
        m_deleteUnconnectedTracks = aDelete;
    }

    void SetMergeSegmentsOpt( bool aMerge )
    {
        m_mergeSegments = aMerge;
    }

    void SetCleanViasOpt( bool aClean )
    {
        m_cleanVias = aClean;
    }

private:

    /**
     * Removes redundant vias like vias at same location
     * or on pad through
     */
    bool clean_vias();

    /**
     * Removes dangling tracks
     */
    bool deleteUnconnectedTracks();

    /**
     * Merge colinear segments and remove null len segments
     */
    bool  clean_segments();

    /**
     * helper function
     * Rebuild list of tracks, and connected tracks
     * this info must be rebuilt when tracks are erased
     */
    void buildTrackConnectionInfo();

    /**
     * helper function
     * merge aTrackRef and aCandidate, when possible,
     * i.e. when they are colinear, same width, and obviously same layer
     */
    TRACK* mergeCollinearSegmentIfPossible( TRACK* aTrackRef,
                                           TRACK* aCandidate, int aEndType );
};

/* Install the cleanup dialog frame to know what should be cleaned
*/
void PCB_EDIT_FRAME::Clean_Pcb()
{
    DIALOG_CLEANING_OPTIONS dlg( this );

    if( dlg.ShowModal() != wxID_OK )
        return;

    wxBusyCursor( dummy );
    TRACKS_CLEANER cleaner( GetBoard() );
    cleaner.SetdeleteUnconnectedTracksOpt( dlg.deleteUnconnectedSegm );
    cleaner.SetMergeSegmentsOpt( dlg.mergeSegments );
    cleaner.SetCleanViasOpt( dlg.cleanVias );

    if( cleaner.CleanupBoard() )
    {
        // Clear undo and redo lists to avoid inconsistencies between lists
        GetScreen()->ClearUndoRedoList();
        SetCurItem( NULL );
        Compile_Ratsnest( NULL, true );
        OnModify();
    }

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
bool TRACKS_CLEANER::CleanupBoard()
{
    bool modified = false;

    // delete redundant vias
    if( m_cleanVias && clean_vias() )
        modified = true;

    // Remove null segments and intermediate points on aligned segments
    if( m_mergeSegments && clean_segments() )
        modified = true;

    // Delete dangling tracks
    if( m_deleteUnconnectedTracks && deleteUnconnectedTracks() )
        modified = true;

    return modified;
}

TRACKS_CLEANER::TRACKS_CLEANER( BOARD * aPcb ): CONNECTIONS( aPcb )
{
    m_Brd = aPcb;
    m_deleteUnconnectedTracks = false;
    m_mergeSegments = false;

    // Build connections info
    BuildPadsList();
    buildTrackConnectionInfo();
}

void TRACKS_CLEANER::buildTrackConnectionInfo()
{
    BuildTracksCandidatesList( m_Brd->m_Track, NULL);

    // clear flags and variables used in cleanup
    for( TRACK * track = m_Brd->m_Track; track; track = track->Next() )
    {
        track->start = NULL;
        track->end = NULL;
        track->m_PadsConnected.clear();
        track->SetState( START_ON_PAD|END_ON_PAD|BUSY, OFF );
    }

    // Build connections info tracks to pads
    SearchTracksConnectedToPads();
    for( TRACK * track = m_Brd->m_Track; track; track = track->Next() )
    {
        // Mark track if connected to pads
        for( unsigned jj = 0; jj < track->m_PadsConnected.size(); jj++ )
        {
            D_PAD * pad = track->m_PadsConnected[jj];

            if( pad->HitTest( track->GetStart() ) )
            {
                track->start = pad;
                track->SetState( START_ON_PAD, ON );
            }

            if( pad->HitTest( track->GetEnd() ) )
            {
                track->end = pad;
                track->SetState( END_ON_PAD, ON );
            }
        }
    }
}

bool TRACKS_CLEANER::clean_vias()
{
    TRACK* next_track;
    bool modified = false;

    for( TRACK* track = m_Brd->m_Track; track; track = track->Next() )
    {
        // Correct via m_End defects (if any)
        if( track->Type() == PCB_VIA_T )
        {
            if( track->GetStart() != track->GetEnd() )
                track->SetEnd( track->GetStart() );
        }

        if( track->GetShape() != VIA_THROUGH )
            continue;

        // Search and delete others vias at same location
        TRACK* alt_track = track->Next();

        for( ; alt_track != NULL; alt_track = next_track )
        {
            next_track = alt_track->Next();

            if( alt_track->GetShape() != VIA_THROUGH )
                continue;

            if( alt_track->GetStart() != track->GetStart() )
                continue;

            // delete via
            alt_track->UnLink();
            delete alt_track;
            modified = true;
        }
    }

    // Delete Via on pads at same location
    for( TRACK* track = m_Brd->m_Track; track != NULL; track = next_track )
    {
        next_track = track->Next();

        if( track->GetShape() != VIA_THROUGH )
            continue;

        // Examine the list of connected pads:
        // if one pad through is found, the via can be removed
        for( unsigned ii = 0; ii < track->m_PadsConnected.size(); ii++ )
        {
            D_PAD * pad = track->m_PadsConnected[ii];

            if( (pad->GetLayerMask() & ALL_CU_LAYERS) == ALL_CU_LAYERS )
            {
                // redundant: via delete it
                track->UnLink();
                delete track;
                modified = true;
                break;
            }
        }
    }

    return modified;
}


/*
 *  Delete dangling tracks
 *  Vias:
 *  If a via is only connected to a dangling track, it also will be removed
 */
bool TRACKS_CLEANER::deleteUnconnectedTracks()
{
    if( m_Brd->m_Track == NULL )
        return false;

    bool modified = false;
    bool item_erased = true;
    while( item_erased )    // Iterate when at least one track is deleted
    {
        item_erased = false;
        TRACK* next_track;
        for( TRACK * track = m_Brd->m_Track; track ; track = next_track )
        {
            next_track = track->Next();

            int flag_erase = 0; //Not connected indicator
            int type_end = 0;

            if( track->GetState( START_ON_PAD ) )
                type_end |= START_ON_PAD;

            if( track->GetState( END_ON_PAD ) )
                type_end |= END_ON_PAD;

            // if the track start point is not connected to a pad,
            // test if this track start point is connected to another track
            // For via test, an enhancement could be to test if connected
            // to 2 items on different layers.
            // Currently a via must be connected to 2 items, that can be on the same layer
            int top_layer, bottom_layer;
            ZONE_CONTAINER* zone;

            if( (type_end & START_ON_PAD ) == 0 )
            {
                TRACK* other = track->GetTrace( m_Brd->m_Track, NULL, FLG_START );

                if( other == NULL )     // Test a connection to zones
                {
                    if( track->Type() != PCB_VIA_T )
                    {
                        zone = m_Brd->HitTestForAnyFilledArea( track->GetStart(),
                                                               track->GetLayer() );
                    }
                    else
                    {
                        ((SEGVIA*)track)->ReturnLayerPair( &top_layer, &bottom_layer );
                        zone = m_Brd->HitTestForAnyFilledArea( track->GetStart(),
                                                               top_layer, bottom_layer );
                    }
                }

                if( (other == NULL) && (zone == NULL) )
                {
                    flag_erase |= 1;
                }
                else    // segment, via or zone connected to this end
                {
                    track->start = other;
                    // If a via is connected to this end,
                    // test if this via has a second item connected.
                    // If no, remove it with the current segment

                    if( other && other->Type() == PCB_VIA_T )
                    {
                        // search for another segment following the via
                        track->SetState( BUSY, ON );

                        SEGVIA* via = (SEGVIA*) other;
                        other = via->GetTrace( m_Brd->m_Track, NULL, FLG_START );

                        if( other == NULL )
                        {
                            via->ReturnLayerPair( &top_layer, &bottom_layer );
                            zone = m_Brd->HitTestForAnyFilledArea( via->GetStart(),
                                                                   bottom_layer, top_layer );
                        }

                        if( (other == NULL) && (zone == NULL) )
                            flag_erase |= 2;

                        track->SetState( BUSY, OFF );
                    }
                }
            }

            // if track end point is not connected to a pad,
            // test if this track end point is connected to an other track
            if( (type_end & END_ON_PAD ) == 0 )
            {
                TRACK* other = track->GetTrace( m_Brd->m_Track, NULL, FLG_END );

                if( other == NULL )     // Test a connection to zones
                {
                    if( track->Type() != PCB_VIA_T )
                    {
                        zone = m_Brd->HitTestForAnyFilledArea( track->GetEnd(),
                                                               track->GetLayer() );
                    }
                    else
                    {
                        ((SEGVIA*)track)->ReturnLayerPair( &top_layer, &bottom_layer );
                        zone = m_Brd->HitTestForAnyFilledArea( track->GetEnd(),
                                                               top_layer, bottom_layer );
                    }
                }

                if ( (other == NULL) && (zone == NULL) )
                {
                    flag_erase |= 0x10;
                }
                else     // segment, via or zone connected to this end
                {
                    track->end = other;

                    // If a via is connected to this end, test if this via has a second item connected
                    // if no, remove it with the current segment

                    if( other && other->Type() == PCB_VIA_T )
                    {
                        // search for another segment following the via

                        track->SetState( BUSY, ON );

                        SEGVIA* via = (SEGVIA*) other;
                        other = via->GetTrace( m_Brd->m_Track, NULL, FLG_END );

                        if( other == NULL )
                        {
                            via->ReturnLayerPair( &top_layer, &bottom_layer );
                            zone = m_Brd->HitTestForAnyFilledArea( via->GetEnd(),
                                                                   bottom_layer, top_layer );
                        }

                        if( (other == NULL) && (zone == NULL) )
                            flag_erase |= 0x20;

                        track->SetState( BUSY, OFF );
                    }
                }
            }

            if( flag_erase )
            {
                // remove segment from board
                track->DeleteStructure();
                // iterate, because a track connected to the deleted track
                // is now perhaps now not connected and should be deleted
                item_erased = true;
                modified = true;
            }
        }
    }

    return modified;
}


// Delete null length segments, and intermediate points ..
bool TRACKS_CLEANER::clean_segments()
{
    bool modified = false;
    TRACK*          segment, * nextsegment;
    TRACK*          other;
    int             flag, no_inc;


    // Delete null segments
    for( segment = m_Brd->m_Track; segment; segment = nextsegment )
    {
        nextsegment = segment->Next();

        if( segment->IsNull() )     // Length segment = 0; delete it
            segment->DeleteStructure();
    }

    // Delete redundant segments, i.e. segments having the same end points
    // and layers
    for( segment  = m_Brd->m_Track; segment; segment = segment->Next() )
    {
        for( other = segment->Next(); other; other = nextsegment )
        {
            nextsegment = other->Next();
            bool erase = false;

            if( segment->Type() != other->Type() )
                continue;

            if( segment->GetLayer() != other->GetLayer() )
                continue;

            if( segment->GetNet() != other->GetNet() )
                break;

            if( ( segment->GetStart() == other->GetStart() ) &&
                ( segment->GetEnd() == other->GetEnd() ) )
                erase = true;

            if( ( segment->GetStart() == other->GetEnd() ) &&
                ( segment->GetEnd() == other->GetStart() ) )
                erase = true;

            // Delete redundant point
            if( erase )
            {
                other->DeleteStructure();
                modified = true;
            }
        }
    }

    // merge collinear segments:
    for( segment = m_Brd->m_Track; segment; segment = nextsegment )
    {
        TRACK*  segStart;
        TRACK*  segEnd;
        TRACK*  segDelete;

        nextsegment = segment->Next();

        if( segment->Type() != PCB_TRACE_T )
            continue;

        flag = no_inc = 0;

        // search for a possible point connected to the START point of the current segment
        for( segStart = segment->Next(); ; )
        {
            segStart = segment->GetTrace( segStart, NULL, FLG_START );

            if( segStart )
            {
                // the two segments must have the same width
                if( segment->GetWidth() != segStart->GetWidth() )
                    break;

                // it cannot be a via
                if( segStart->Type() != PCB_TRACE_T )
                    break;

                // We must have only one segment connected
                segStart->SetState( BUSY, ON );
                other = segment->GetTrace( m_Brd->m_Track, NULL, FLG_START );
                segStart->SetState( BUSY, OFF );

                if( other == NULL )
                    flag = 1;           // OK

                break;
            }
            break;
        }

        if( flag )   // We have the starting point of the segment is connected to an other segment
        {
            segDelete = mergeCollinearSegmentIfPossible( segment, segStart, FLG_START );

            if( segDelete )
            {
                no_inc = 1;
                segDelete->DeleteStructure();
                modified = true;
            }
        }

        // search for a possible point connected to the END point of the current segment:
        for( segEnd = segment->Next(); ; )
        {
            segEnd = segment->GetTrace( segEnd, NULL, FLG_END );

            if( segEnd )
            {
                if( segment->GetWidth() != segEnd->GetWidth() )
                    break;

                if( segEnd->Type() != PCB_TRACE_T )
                    break;

                // We must have only one segment connected
                segEnd->SetState( BUSY, ON );
                other = segment->GetTrace( m_Brd->m_Track, NULL, FLG_END );
                segEnd->SetState( BUSY, OFF );

                if( other == NULL )
                    flag |= 2;          // Ok

                break;
            }
            else
            {
                break;
            }
        }

        if( flag & 2 )  // We have the ending point of the segment is connected to an other segment
        {
            segDelete = mergeCollinearSegmentIfPossible( segment, segEnd, FLG_END );

            if( segDelete )
            {
                no_inc = 1;
                segDelete->DeleteStructure();
                modified = true;
            }
        }

        if( no_inc ) // The current segment was modified, retry to merge it
            nextsegment = segment->Next();
    }

    return modified;
}


/** Function used by clean_segments.
 *  Test if aTrackRef and aCandidate (which must have a common end) are collinear.
 *  and see if the common point is not on a pad (i.e. if this common point can be removed).
 *  the ending point of aTrackRef is the start point (aEndType == START)
 *  or the end point (aEndType != START)
 *  flags START_ON_PAD and END_ON_PAD must be set before calling this function
 *  if the common point can be deleted, this function
 *    change the common point coordinate of the aTrackRef segm
 *   (and therefore connect the 2 other ending points)
 *    and return aCandidate (which can be deleted).
 *  else return NULL
 */
TRACK* TRACKS_CLEANER::mergeCollinearSegmentIfPossible( TRACK* aTrackRef, TRACK* aCandidate,
                                       int aEndType )
{
    if( aTrackRef->GetWidth() != aCandidate->GetWidth() )
        return NULL;

    bool is_colinear = false;

    // Trivial case: superimposed tracks ( tracks, not vias ):
    if( aTrackRef->Type() == PCB_TRACE_T && aCandidate->Type() == PCB_TRACE_T )
    {
        if( ( aTrackRef->GetStart() == aCandidate->GetStart() ) &&
            ( aTrackRef->GetEnd() == aCandidate->GetEnd() ) )
            return aCandidate;

        if( ( aTrackRef->GetStart() == aCandidate->GetEnd() ) &&
            ( aTrackRef->GetEnd() == aCandidate->GetStart() ) )
            return aCandidate;
    }

    int refdx = aTrackRef->GetEnd().x - aTrackRef->GetStart().x;
    int refdy = aTrackRef->GetEnd().y - aTrackRef->GetStart().y;

    int segmdx = aCandidate->GetEnd().x - aCandidate->GetStart().x;
    int segmdy = aCandidate->GetEnd().y - aCandidate->GetStart().y;

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
    if( aEndType == FLG_START )
    {
        // We do not have a pad, which is a always terminal point for a track
        if( aTrackRef->GetState( START_ON_PAD) )
            return NULL;

        /* change the common point coordinate of pt_segm to use the other point
         * of pt_segm (pt_segm will be removed later) */
        if( aTrackRef->GetStart() == aCandidate->GetStart() )
        {
            aTrackRef->SetStart( aCandidate->GetEnd());
            aTrackRef->start = aCandidate->end;
            aTrackRef->SetState( START_ON_PAD, aCandidate->GetState( END_ON_PAD) );
            return aCandidate;
        }
        else
        {
            aTrackRef->SetStart( aCandidate->GetStart() );
            aTrackRef->start = aCandidate->start;
            aTrackRef->SetState( START_ON_PAD, aCandidate->GetState( START_ON_PAD) );
            return aCandidate;
        }
    }
    else    // aEndType == END
    {
        // We do not have a pad, which is a always terminal point for a track
        if( aTrackRef->GetState( END_ON_PAD) )
            return NULL;

        /* change the common point coordinate of pt_segm to use the other point
         * of pt_segm (pt_segm will be removed later) */
        if( aTrackRef->GetEnd() == aCandidate->GetStart() )
        {
            aTrackRef->SetEnd( aCandidate->GetEnd() );
            aTrackRef->end = aCandidate->end;
            aTrackRef->SetState( END_ON_PAD, aCandidate->GetState( END_ON_PAD) );
            return aCandidate;
        }
        else
        {
            aTrackRef->SetEnd( aCandidate->GetStart() );
            aTrackRef->end = aCandidate->start;
            aTrackRef->SetState( END_ON_PAD, aCandidate->GetState( START_ON_PAD) );
            return aCandidate;
        }
    }

    return NULL;
}


bool PCB_EDIT_FRAME::RemoveMisConnectedTracks()
{
     /* finds all track segments which are mis-connected (to more than one net).
     * When such a bad segment is found, it is flagged to be removed.
     * All tracks having at least one flagged segment are removed.
     */
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

        if( segment->start && segment->start->Type()==PCB_PAD_T )
        {
            // get the netcode of the pad to propagate.
            net_code_s = ((D_PAD*)(segment->start))->GetNet();
        }
        else
        {
            other = segment->GetTrace( GetBoard()->m_Track, NULL, FLG_START );

            if( other )
                net_code_s = other->GetNet();
        }

        if( net_code_s < 0 )
            continue;           // the "start" of segment is not connected

        // find the netcode for segment using anything connected to the "end" of "segment"
        net_code_e = -1;

        if( segment->end && segment->end->Type()==PCB_PAD_T )
        {
            net_code_e = ((D_PAD*)(segment->end))->GetNet();
        }
        else
        {
            other = segment->GetTrace( GetBoard()->m_Track, NULL, FLG_END );

            if( other )
                net_code_e = other->GetNet();
        }

        if( net_code_e < 0 )
            continue;           // the "end" of segment is not connected

        // Netcodes do not agree, so mark the segment as "to be removed"
        if( net_code_s != net_code_e )
        {
            segment->SetState( FLAG0, ON );
        }
    }

    // Remove tracks having a flagged segment
    for( segment = GetBoard()->m_Track; segment; segment = next )
    {
        next = (TRACK*) segment->Next();

        if( segment->GetState( FLAG0 ) )    // Segment is flagged to be removed
        {
            segment->SetState( FLAG0, OFF );
            isModified = true;
            GetBoard()->m_Status_Pcb = 0;
            Remove_One_Track( NULL, segment );

            // the current segment is deleted,
            // we do not know the next "not yet tested" segment,
            // so restart to the beginning
            next = GetBoard()->m_Track;
        }
    }

    return isModified;
}
