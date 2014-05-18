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
 * @brief functions to clean tracks: remove null length and redundant segments
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
#include <ratsnest_data.h>

// Helper class used to clean tracks and vias
class TRACKS_CLEANER: CONNECTIONS
{
private:
    BOARD *m_Brd;

public:
    TRACKS_CLEANER( BOARD * aPcb );

    /**
     * the cleanup function.
     * return true if some item was modified
     */
    bool CleanupBoard( PCB_EDIT_FRAME *aFrame, bool aCleanVias,
                       bool aMergeSegments, bool aDeleteUnconnected);

private:

    /**
     * Removes redundant vias like vias at same location
     * or on pad through
     */
    bool clean_vias();

    /**
     * Removes all the following THT vias on the same position of the
     * specified one
     */
    bool remove_duplicates_of_via( const VIA *aVia );

    /**
     * Removes all the following duplicates tracks of the specified one
     */
    bool remove_duplicates_of_track( const TRACK *aTrack );

    /**
     * Removes dangling tracks
     */
    bool deleteUnconnectedTracks();

    /// Delete null length track segments
    bool delete_null_segments();

    /// Try to merge the segment to a following collinear one
    bool merge_collinear_of_track( TRACK *aSegment );

    /**
     * Merge collinear segments and remove duplicated and null len segments
     */
    bool clean_segments();

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
                                           TRACK* aCandidate, ENDPOINT_T aEndType );

    const ZONE_CONTAINER* zoneForTrackEndpoint( const TRACK *aTrack,
            ENDPOINT_T aEndPoint );

    bool testTrackEndpointDangling( TRACK *aTrack, ENDPOINT_T aEndPoint );
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

    cleaner.CleanupBoard( this, dlg.m_cleanVias, dlg.m_mergeSegments,
                          dlg.m_deleteUnconnectedSegm );
    m_canvas->Refresh( true );
}


/* Main cleaning function.
 *  Delete
 * - Redundant points on tracks (merge aligned segments)
 * - vias on pad
 * - null length segments
 *  Create segments when track ends are incorrectly connected:
 *  i.e. when a track end covers a pad or a via but is not exactly on the pad or the via center
 */
bool TRACKS_CLEANER::CleanupBoard( PCB_EDIT_FRAME *aFrame,
                                   bool aCleanVias,
                                   bool aMergeSegments,
                                   bool aDeleteUnconnected )
{
    bool modified = false;

    // delete redundant vias
    modified |= (aCleanVias && clean_vias());

    // Remove null segments and intermediate points on aligned segments
    modified |= (aMergeSegments && clean_segments());

    // Delete dangling tracks
    modified |= (aDeleteUnconnected && deleteUnconnectedTracks());

    if( modified )
    {
        // Clear undo and redo lists to avoid inconsistencies between lists
        aFrame->GetScreen()->ClearUndoRedoList();
        aFrame->SetCurItem( NULL );
        aFrame->Compile_Ratsnest( NULL, true );
        aFrame->OnModify();
    }
    return modified;
}

TRACKS_CLEANER::TRACKS_CLEANER( BOARD * aPcb ): CONNECTIONS( aPcb )
{
    m_Brd = aPcb;

    // Build connections info
    BuildPadsList();
    buildTrackConnectionInfo();
}

void TRACKS_CLEANER::buildTrackConnectionInfo()
{
    BuildTracksCandidatesList( m_Brd->m_Track, NULL);

    // clear flags and variables used in cleanup
    for( TRACK *track = m_Brd->m_Track; track != NULL; track = track->Next() )
    {
        track->start = NULL;
        track->end = NULL;
        track->m_PadsConnected.clear();
        track->SetState( START_ON_PAD|END_ON_PAD|BUSY, false );
    }

    // Build connections info tracks to pads
    SearchTracksConnectedToPads();
    for( TRACK *track = m_Brd->m_Track; track != NULL; track = track->Next() )
    {
        // Mark track if connected to pads
        for( unsigned jj = 0; jj < track->m_PadsConnected.size(); jj++ )
        {
            D_PAD * pad = track->m_PadsConnected[jj];

            if( pad->HitTest( track->GetStart() ) )
            {
                track->start = pad;
                track->SetState( START_ON_PAD, true );
            }

            if( pad->HitTest( track->GetEnd() ) )
            {
                track->end = pad;
                track->SetState( END_ON_PAD, true );
            }
        }
    }
}

bool TRACKS_CLEANER::remove_duplicates_of_via( const VIA *aVia )
{
    bool modified = false;

    // Search and delete others vias at same location
    VIA* next_via;
    for( VIA* alt_via = GetFirstVia( aVia->Next() ); alt_via != NULL;
            alt_via = next_via )
    {
        next_via = GetFirstVia( alt_via->Next() );

        if( (alt_via->GetViaType() == VIA_THROUGH) &&
                (alt_via->GetStart() == aVia->GetStart()) )
        {
            // delete via
			m_Brd->GetRatsnest()->Remove( alt_via );
            alt_via->ViewRelease();
            alt_via->DeleteStructure();
            modified = true;
        }
    }
    return modified;
}

bool TRACKS_CLEANER::clean_vias()
{
    bool modified = false;

    for( VIA* via = GetFirstVia( m_Brd->m_Track ); via != NULL;
            via = GetFirstVia( via->Next() ) )
    {
        // Correct via m_End defects (if any), should never happen
        if( via->GetStart() != via->GetEnd() )
        {
            wxFAIL_MSG( wxT( "Via with mismatching ends" ) );
            via->SetEnd( via->GetStart() );
        }

        /* Important: these cleanups only do thru hole vias, they don't
         * (yet) handle high density interconnects */
        if( via->GetViaType() != VIA_THROUGH )
        {
            modified |= remove_duplicates_of_via( via );

            /* To delete through Via on THT pads at same location
             * Examine the list of connected pads:
             * if one through pad is found, the via can be removed */
            for( unsigned ii = 0; ii < via->m_PadsConnected.size(); ++ii )
            {
                const D_PAD *pad = via->m_PadsConnected[ii];

                if( (pad->GetLayerMask() & ALL_CU_LAYERS) == ALL_CU_LAYERS )
                {
                    // redundant: delete the via
                    m_Brd->GetRatsnest()->Remove( via );
                    via->ViewRelease();
                    via->DeleteStructure();
                    modified = true;
                    break;
                }
            }
        }
    }

    return modified;
}

/// Utility for checking if a track/via ends on a zone
const ZONE_CONTAINER* TRACKS_CLEANER::zoneForTrackEndpoint( const TRACK *aTrack,
        ENDPOINT_T aEndPoint )
{
    // Vias are special cased, since they get a layer range, not a single one
    LAYER_NUM top_layer, bottom_layer;
    const VIA *via = dynamic_cast<const VIA*>( aTrack );

    if( via )
        via->LayerPair( &top_layer, &bottom_layer );
    else
    {
        top_layer = aTrack->GetLayer();
        bottom_layer = top_layer;
    }
    return m_Brd->HitTestForAnyFilledArea( aTrack->GetEndPoint( aEndPoint ),
            top_layer, bottom_layer, aTrack->GetNetCode() );
}

/** Utility: does the endpoint unconnected processed for one endpoint of one track
 * Returns true if the track must be deleted, false if not necessarily */
bool TRACKS_CLEANER::testTrackEndpointDangling( TRACK *aTrack, ENDPOINT_T aEndPoint )
{
    bool flag_erase = false;

    TRACK* other = aTrack->GetTrack( m_Brd->m_Track, NULL, aEndPoint, true, false );
    if( (other == NULL) &&
            (zoneForTrackEndpoint( aTrack, aEndPoint ) == NULL) )
        flag_erase = true; // Start endpoint is neither on pad, zone or other track
    else    // segment, via or zone connected to this end
    {
        // Fill connectivity informations
        if( aEndPoint == ENDPOINT_START )
            aTrack->start = other;
        else
            aTrack->end = other;

        /* If a via is connected to this end, test if this via has a second item connected.
         * If not, remove the current segment (the via would then become
         * unconnected and remove on the following pass) */
        VIA* via = dynamic_cast<VIA*>( other );
        if( via )
        {
            // search for another segment following the via
            aTrack->SetState( BUSY, true );

            other = via->GetTrack( m_Brd->m_Track, NULL, aEndPoint, true, false );

            // There is a via on the start but it goes nowhere
            if( (other == NULL) &&
                    (zoneForTrackEndpoint( via, aEndPoint ) == NULL) )
                flag_erase = true;

            aTrack->SetState( BUSY, false );
        }
    }
    return flag_erase;
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
    bool item_erased;
    do // Iterate when at least one track is deleted
    {
        item_erased = false;
        TRACK* next_track;
        for( TRACK *track = m_Brd->m_Track; track != NULL; track = next_track )
        {
            next_track = track->Next();

            bool flag_erase = false; // Start without a good reason to erase it

            /* if a track endpoint is not connected to a pad, test if
             * the endpoint is connected to another track or to a zone.
             * For via test, an enhancement could be to test if
             * connected to 2 items on different layers. Currently
             * a via must be connected to 2 items, that can be on the
             * same layer */

            // Check if there is nothing attached on the start
            if( !(track->GetState( START_ON_PAD )) )
                flag_erase |= testTrackEndpointDangling( track, ENDPOINT_START );

            // Check if there is nothing attached on the end
            if( !(track->GetState( END_ON_PAD )) )
                flag_erase |= testTrackEndpointDangling( track, ENDPOINT_END );

            if( flag_erase )
            {
                // remove segment from board
                m_Brd->GetRatsnest()->Remove( track );
                track->ViewRelease();
                track->DeleteStructure();

                /* keep iterating, because a track connected to the deleted track
                 * now perhaps is not connected and should be deleted */
                item_erased = true;
                modified = true;
            }
        }
    } while( item_erased );

    return modified;
}

// Delete null length track segments
bool TRACKS_CLEANER::delete_null_segments()
{
    TRACK *nextsegment;
    bool modified = false;

    // Delete null segments
    for( TRACK *segment = m_Brd->m_Track; segment; segment = nextsegment )
    {
        nextsegment = segment->Next();

        if( segment->IsNull() )     // Length segment = 0; delete it
        {
            m_Brd->GetRatsnest()->Remove( segment );
            segment->ViewRelease();
            segment->DeleteStructure();
            modified = true;
        }
    }
    return modified;
}

bool TRACKS_CLEANER::remove_duplicates_of_track( const TRACK *aTrack )
{
    bool modified = false;

    TRACK *nextsegment;
    for( TRACK *other = aTrack->Next(); other; other = nextsegment )
    {
        nextsegment = other->Next();

        // New netcode, break out (can't be there any other)
        if( aTrack->GetNetCode() != other->GetNetCode() )
            break;

        // Must be of the same type, on the same layer and the endpoints
        // must be the same (maybe swapped)
        if( (aTrack->Type() != other->Type()) &&
            (aTrack->GetLayer() != other->GetLayer()) )
        {
            if( ((aTrack->GetStart() == other->GetStart()) &&
                 (aTrack->GetEnd() == other->GetEnd())) ||
                ((aTrack->GetStart() == other->GetEnd()) &&
                 (aTrack->GetEnd() == other->GetStart()))) 
            {
	            m_Brd->GetRatsnest()->Remove( other );
				other->ViewRelease();
                other->DeleteStructure();
                modified = true;
            }
        }
    }
    return modified;
}

bool TRACKS_CLEANER::merge_collinear_of_track( TRACK *aSegment )
{
    bool merged_this = false;

    // *WHY* doesn't C++ have prec and succ (or ++ --) like PASCAL?
    for( ENDPOINT_T endpoint = ENDPOINT_START; endpoint <= ENDPOINT_END;
            endpoint = ENDPOINT_T( endpoint + 1 ) )
    {
        // search for a possible segment connected to the current endpoint of the current one
        TRACK *other = aSegment->Next();
        if( other )
        {
            other = aSegment->GetTrack( other, NULL, endpoint, true, false );

            if( other )
            {
                // the two segments must have the same width and the other
                // cannot be a via
                if( (aSegment->GetWidth() == other->GetWidth()) && 
                        (other->Type() == PCB_TRACE_T) )
                {
                    // There can be only one segment connected
                    other->SetState( BUSY, true );
                    TRACK *yet_another = aSegment->GetTrack( m_Brd->m_Track, NULL,
                            endpoint, true, false );
                    other->SetState( BUSY, false );

                    if( !yet_another )
                    {
                        // Try to merge them
                        TRACK *segDelete = mergeCollinearSegmentIfPossible( aSegment,
                                other, endpoint );

                        // Merge succesful, the other one has to go away
                        if( segDelete )
                        {
                            m_Brd->GetRatsnest()->Remove( segDelete );
                            segDelete->ViewRelease();
                            segDelete->DeleteStructure();
                            merged_this = true;
                        }
                    }
                }
            }
        }
    }

    return merged_this;
}

// Delete null length segments, and intermediate points ..
bool TRACKS_CLEANER::clean_segments()
{
    bool modified = false;

    // Easy things first
    modified |= delete_null_segments();

    // Delete redundant segments, i.e. segments having the same end points and layers
    for( TRACK *segment = m_Brd->m_Track; segment; segment = segment->Next() )
        modified |= remove_duplicates_of_track( segment );

    // merge collinear segments:
    TRACK *nextsegment;
    for( TRACK *segment = m_Brd->m_Track; segment; segment = nextsegment )
    {
        nextsegment = segment->Next();

        if( segment->Type() == PCB_TRACE_T )
        {
            bool merged_this = merge_collinear_of_track( segment );
            modified |= merged_this;

            if( merged_this ) // The current segment was modified, retry to merge it again
                nextsegment = segment->Next();
        }
    }

    return modified;
}

/* Utility: check for parallelism between two segments */
static bool parallelism_test( int dx1, int dy1, int dx2, int dy2 )
{
    /* The following condition list is ugly and repetitive, but I have
     * not a better way to express clearly the trivial cases. Hope the
     * compiler optimize it better than always doing the product
     * below... */

    // test for vertical alignment (easy to handle)
    if( dx1 == 0 )
        return dx2 == 0;

    if( dx2 == 0 )
        return dx1 == 0;

    // test for horizontal alignment (easy to handle)
    if( dy1 == 0 )
        return dy2 == 0;

    if( dy2 == 0 )
        return dy1 == 0;

    /* test for alignment in other cases: Do the usual cross product test 
     * (the same as testing the slope, but without a division) */
    return ((double)dy1 * dx2 == (double)dx1 * dy2);
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
                                       ENDPOINT_T aEndType )
{
    // First of all, they must be of the same width and must be both actual tracks
    if( (aTrackRef->GetWidth() != aCandidate->GetWidth()) ||
        (aTrackRef->Type() != PCB_TRACE_T) ||
        (aCandidate->Type() != PCB_TRACE_T) )
        return NULL;

    // Trivial case: exactly the same track
    if( ( aTrackRef->GetStart() == aCandidate->GetStart() ) &&
        ( aTrackRef->GetEnd() == aCandidate->GetEnd() ) )
        return aCandidate;

    if( ( aTrackRef->GetStart() == aCandidate->GetEnd() ) &&
        ( aTrackRef->GetEnd() == aCandidate->GetStart() ) )
        return aCandidate;

    // Weed out non-parallel tracks
    if ( !parallelism_test( aTrackRef->GetEnd().x - aTrackRef->GetStart().x,
                aTrackRef->GetEnd().y - aTrackRef->GetStart().y,
                aCandidate->GetEnd().x - aCandidate->GetStart().x,
                aCandidate->GetEnd().y - aCandidate->GetStart().y ) )
        return NULL;

    /* Here we have 2 aligned segments:
     * We must change the pt_ref common point only if not on a pad
     * (this function) is called when there is only 2 connected segments,
     * and if this point is not on a pad, it can be removed and the 2 segments will be merged
     */
    if( aEndType == ENDPOINT_START )
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
            aTrackRef->ViewUpdate( KIGFX::VIEW_ITEM::GEOMETRY );
            return aCandidate;
        }
        else
        {
            aTrackRef->SetStart( aCandidate->GetStart() );
            aTrackRef->start = aCandidate->start;
            aTrackRef->SetState( START_ON_PAD, aCandidate->GetState( START_ON_PAD) );
            aTrackRef->ViewUpdate( KIGFX::VIEW_ITEM::GEOMETRY );
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
            aTrackRef->ViewUpdate( KIGFX::VIEW_ITEM::GEOMETRY );
            return aCandidate;
        }
        else
        {
            aTrackRef->SetEnd( aCandidate->GetStart() );
            aTrackRef->end = aCandidate->start;
            aTrackRef->SetState( END_ON_PAD, aCandidate->GetState( START_ON_PAD) );
            aTrackRef->ViewUpdate( KIGFX::VIEW_ITEM::GEOMETRY );
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
        segment->SetState( FLAG0, false );

        // find the netcode for segment using anything connected to the "start" of "segment"
        net_code_s = -1;

        if( segment->start && segment->start->Type()==PCB_PAD_T )
        {
            // get the netcode of the pad to propagate.
            net_code_s = ((D_PAD*)(segment->start))->GetNetCode();
        }
        else
        {
            other = segment->GetTrack( GetBoard()->m_Track, NULL, ENDPOINT_START, false, false );

            if( other )
                net_code_s = other->GetNetCode();
        }

        if( net_code_s < 0 )
            continue;           // the "start" of segment is not connected

        // find the netcode for segment using anything connected to the "end" of "segment"
        net_code_e = -1;

        if( segment->end && segment->end->Type()==PCB_PAD_T )
        {
            net_code_e = ((D_PAD*)(segment->end))->GetNetCode();
        }
        else
        {
            other = segment->GetTrack( GetBoard()->m_Track, NULL, ENDPOINT_END, false, false );

            if( other )
                net_code_e = other->GetNetCode();
        }

        if( net_code_e < 0 )
            continue;           // the "end" of segment is not connected

        // Netcodes do not agree, so mark the segment as "to be removed"
        if( net_code_s != net_code_e )
        {
            segment->SetState( FLAG0, true );
        }
    }

    // Remove tracks having a flagged segment
    for( segment = GetBoard()->m_Track; segment; segment = next )
    {
        next = (TRACK*) segment->Next();

        if( segment->GetState( FLAG0 ) )    // Segment is flagged to be removed
        {
            segment->SetState( FLAG0, false );
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
