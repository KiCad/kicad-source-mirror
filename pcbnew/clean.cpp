/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2004-2017 Jean-Pierre Charras, jp.charras at wanadoo.fr
 * Copyright (C) 2011 Wayne Stambaugh <stambaughw@verizon.net>
 * Copyright (C) 1992-2017 KiCad Developers, see AUTHORS.txt for contributors.
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
#include <wxPcbStruct.h>
#include <pcbnew.h>
#include <class_board.h>
#include <class_track.h>
#include <dialog_cleaning_options.h>
#include <board_commit.h>
#include <connectivity_data.h>
#include <connectivity_algo.h>

// Helper class used to clean tracks and vias
class TRACKS_CLEANER
{
public:
    TRACKS_CLEANER( BOARD* aPcb, BOARD_COMMIT& aCommit );

    /**
     * the cleanup function.
     * return true if some item was modified
     * @param aCleanVias = true to remove superimposed vias
     * @param aRemoveMisConnected = true to remove segments connecting 2 different nets
     * @param aMergeSegments = true to merge collinear segmenst and remove 0 len segm
     * @param aDeleteUnconnected = true to remove dangling tracks
     * (short circuits)
     */
    bool CleanupBoard( bool aCleanVias, bool aRemoveMisConnected,
                       bool aMergeSegments, bool aDeleteUnconnected );

private:
    /* finds and remove all track segments which are connected to more than one net.
     * (short circuits)
     */
    bool removeBadTrackSegments();

    /**
     * Removes redundant vias like vias at same location
     * or on pad through
     */
    bool cleanupVias();

    /**
     * Removes all the following THT vias on the same position of the
     * specified one
     */
    void removeDuplicatesOfVia( const VIA *aVia, std::set<BOARD_ITEM *>& aToRemove );

    /**
     * Removes all the following duplicates tracks of the specified one
     */
    void removeDuplicatesOfTrack( const TRACK* aTrack, std::set<BOARD_ITEM*>& aToRemove );

    /**
     * Removes dangling tracks
     */
    bool deleteDanglingTracks();

    /// Delete null length track segments
    bool deleteNullSegments();

    /// Try to merge the segment to a following collinear one
    bool MergeCollinearTracks( TRACK* aSegment );

    /**
     * Merge collinear segments and remove duplicated and null len segments
     */
    bool cleanupSegments();

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

    const ZONE_CONTAINER* zoneForTrackEndpoint( const TRACK* aTrack,
            ENDPOINT_T aEndPoint );

    bool testTrackEndpointDangling( TRACK* aTrack, ENDPOINT_T aEndPoint );

    BOARD* m_brd;
    BOARD_COMMIT& m_commit;

    bool removeItems(  std::set<BOARD_ITEM*>& aItems )
    {
        bool isModified = false;


        for( auto item : aItems )
        {
            isModified = true;
            m_brd->Remove( item );
            m_commit.Removed( item );
        }

        return isModified;
    }
};


/* Install the cleanup dialog frame to know what should be cleaned
*/
void PCB_EDIT_FRAME::Clean_Pcb()
{
    DIALOG_CLEANING_OPTIONS dlg( this );

    if( dlg.ShowModal() != wxID_OK )
        return;

    // Old model has to be refreshed, GAL normally does not keep updating it
    Compile_Ratsnest( NULL, false );

    wxBusyCursor( dummy );
    BOARD_COMMIT commit( this );
    TRACKS_CLEANER cleaner( GetBoard(), commit );

    bool modified = cleaner.CleanupBoard( dlg.m_deleteShortCircuits, dlg.m_cleanVias,
                            dlg.m_mergeSegments, dlg.m_deleteUnconnectedSegm );

    if( modified )
    {
        // Clear undo and redo lists to avoid inconsistencies between lists
        SetCurItem( NULL );
        commit.Push( _( "Board cleanup" ) );
    }

    m_canvas->Refresh( true );
}


void TRACKS_CLEANER::buildTrackConnectionInfo()
{
    auto connectivity = m_brd->GetConnectivity();

    connectivity->Build(m_brd);

    // clear flags and variables used in cleanup
    for( auto track : m_brd->Tracks() )
    {
        track->SetState( START_ON_PAD | END_ON_PAD | BUSY, false );
    }

    for( auto track : m_brd->Tracks() )
    {
        // Mark track if connected to pads
        for( auto pad : connectivity->GetConnectedPads( track ) )
        {
            if( pad->HitTest( track->GetStart() ) )
            {
                track->SetState( START_ON_PAD, true );
            }

            if( pad->HitTest( track->GetEnd() ) )
            {
                track->SetState( END_ON_PAD, true );
            }
        }
    }
}


/* Main cleaning function.
 *  Delete
 * - Redundant points on tracks (merge aligned segments)
 * - vias on pad
 * - null length segments
 */
bool TRACKS_CLEANER::CleanupBoard( bool aRemoveMisConnected,
                                   bool aCleanVias,
                                   bool aMergeSegments,
                                   bool aDeleteUnconnected )
{

    bool modified = false;

    // delete redundant vias
    if( aCleanVias )
        modified |= cleanupVias();

    // Remove null segments and intermediate points on aligned segments
    // If not asked, remove null segments only if remove misconnected is asked
    if( aMergeSegments )
        modified |= cleanupSegments();
    else if( aRemoveMisConnected )
        modified |= deleteNullSegments();

    buildTrackConnectionInfo();

    if( aRemoveMisConnected )
        modified |= removeBadTrackSegments();

    // Delete dangling tracks
    if( aDeleteUnconnected )
    {
        buildTrackConnectionInfo();

        if( deleteDanglingTracks() )
        {
            modified = true;

            // Removed tracks can leave aligned segments
            // (when a T was formed by tracks and the "vertical" segment
            // is removed)
            if( aMergeSegments )
                cleanupSegments();
        }
    }

    return modified;
}


TRACKS_CLEANER::TRACKS_CLEANER( BOARD* aPcb, BOARD_COMMIT& aCommit )
    : m_brd( aPcb ), m_commit( aCommit )
{

}


bool TRACKS_CLEANER::removeBadTrackSegments()
{
    auto connectivity = m_brd->GetConnectivity();

    std::set<BOARD_ITEM *> toRemove;

    for( auto segment : m_brd->Tracks() )
    {
        segment->SetState( FLAG0, false );

        for( auto testedPad : connectivity->GetConnectedPads( segment ) )
        {
            if( segment->GetNetCode() != testedPad->GetNetCode() )
                toRemove.insert( segment );
        }

        for( auto testedTrack : connectivity->GetConnectedTracks( segment ) )
        {
            if( segment->GetNetCode() != testedTrack->GetNetCode() && !testedTrack->GetState( FLAG0 ) )
                toRemove.insert( segment );
        }
    }

    return removeItems( toRemove );
}


void TRACKS_CLEANER::removeDuplicatesOfVia( const VIA *aVia, std::set<BOARD_ITEM *>& aToRemove )
{
    VIA* next_via;

    for( VIA* alt_via = GetFirstVia( aVia->Next() ); alt_via != NULL; alt_via = next_via )
    {
        next_via = GetFirstVia( alt_via->Next() );

        if( ( alt_via->GetViaType() == VIA_THROUGH ) &&
                ( alt_via->GetStart() == aVia->GetStart() ) )
            aToRemove.insert ( alt_via );
    }
}


bool TRACKS_CLEANER::cleanupVias()
{
    std::set<BOARD_ITEM*> toRemove;

    for( VIA* via = GetFirstVia( m_brd->m_Track ); via != NULL;
            via = GetFirstVia( via->Next() ) )
    {
        if( via->GetFlags() & TRACK_LOCKED )
            continue;

        // Correct via m_End defects (if any), should never happen
        if( via->GetStart() != via->GetEnd() )
        {
            wxFAIL_MSG( "Malformed via with mismatching ends" );
            via->SetEnd( via->GetStart() );
        }

        /* Important: these cleanups only do thru hole vias, they don't
         * (yet) handle high density interconnects */
        if( via->GetViaType() == VIA_THROUGH )
        {
            removeDuplicatesOfVia( via, toRemove );

            /* To delete through Via on THT pads at same location
             * Examine the list of connected pads:
             * if one through pad is found, the via can be removed */

            const auto pads = m_brd->GetConnectivity()->GetConnectedPads( via );
            for( const auto pad : pads )
            {
                const LSET all_cu = LSET::AllCuMask();

                if( ( pad->GetLayerSet() & all_cu ) == all_cu )
                {
                    // redundant: delete the via
                    toRemove.insert( via );
                    break;
                }
            }
        }
    }

    return removeItems( toRemove );
}


/** Utility: does the endpoint unconnected processed for one endpoint of one track
 * Returns true if the track must be deleted, false if not necessarily */
bool TRACKS_CLEANER::testTrackEndpointDangling( TRACK* aTrack, ENDPOINT_T aEndPoint )
{
    auto connectivity = m_brd->GetConnectivity();
    VECTOR2I endpoint ;

    if( aTrack->Type() == PCB_TRACE_T )
        endpoint = aTrack->GetEndPoint( aEndPoint );
    else
        endpoint = aTrack->GetStart( );

    //wxASSERT ( connectivity->GetConnectivityAlgo()->ItemEntry( aTrack ) != nullptr );
    wxASSERT ( connectivity->GetConnectivityAlgo()->ItemEntry( aTrack ).GetItems().size() != 0 );
    auto citem = connectivity->GetConnectivityAlgo()->ItemEntry( aTrack ).GetItems().front();

    if( !citem->Valid() )
        return false;

    auto anchors = citem->Anchors();

    for( auto anchor : anchors )
    {
        if( anchor->Pos() == endpoint && anchor->IsDangling() )
            return true;
    }

    return false;
}


/* Delete dangling tracks
 *  Vias:
 *  If a via is only connected to a dangling track, it also will be removed
 */
bool TRACKS_CLEANER::deleteDanglingTracks()
{
    bool item_erased = false;
    bool modified = false;

    do // Iterate when at least one track is deleted
    {
        item_erased = false;

        TRACK* next_track;

        for( TRACK *track = m_brd->m_Track; track != NULL; track = next_track )
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
            if( !( track->GetState( START_ON_PAD ) ) )
                flag_erase |= testTrackEndpointDangling( track, ENDPOINT_START );

            // If not sure about removal, then check if there is nothing attached on the end
            if( !flag_erase && !track->GetState( END_ON_PAD ) )
                flag_erase |= testTrackEndpointDangling( track, ENDPOINT_END );

            if( flag_erase )
            {
                m_brd->Remove( track );
                m_commit.Removed( track );

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
bool TRACKS_CLEANER::deleteNullSegments()
{
    std::set<BOARD_ITEM *> toRemove;

    for( auto segment : m_brd->Tracks() )
    {
        if( segment->IsNull() )     // Length segment = 0; delete it
            toRemove.insert( segment );
    }

    return removeItems( toRemove );
}

void TRACKS_CLEANER::removeDuplicatesOfTrack( const TRACK *aTrack, std::set<BOARD_ITEM*>& aToRemove )
{
    if( aTrack->GetFlags() & STRUCT_DELETED )
        return;

    for( auto other : m_brd->Tracks() )
    {
        // New netcode, break out (can't be there any other)
        if( aTrack->GetNetCode() != other->GetNetCode() )
            continue;

        if( aTrack == other )
            continue;

        if( other->GetFlags() & STRUCT_DELETED )
            continue;

        // Must be of the same type, on the same layer and the endpoints
        // must be the same (maybe swapped)
        if( ( aTrack->Type() == other->Type() ) &&
            ( aTrack->GetLayer() == other->GetLayer() ) )
        {
            if( ( ( aTrack->GetStart() == other->GetStart() ) &&
                 ( aTrack->GetEnd() == other->GetEnd() ) ) ||
                ( ( aTrack->GetStart() == other->GetEnd() ) &&
                 ( aTrack->GetEnd() == other->GetStart() ) ) )
            {
                other->SetFlags( STRUCT_DELETED );
                aToRemove.insert( other );
            }
        }
    }
}


bool TRACKS_CLEANER::MergeCollinearTracks( TRACK* aSegment )
{
    bool merged_this = false;


    for( ENDPOINT_T endpoint = ENDPOINT_START; endpoint <= ENDPOINT_END;
            endpoint = ENDPOINT_T( endpoint + 1 ) )
    {
        // search for a possible segment connected to the current endpoint of the current one
        TRACK* other = aSegment->Next();

        if( other )
        {
            other = aSegment->GetTrack( other, NULL, endpoint, true, false );

            if( other )
            {
                // the two segments must have the same width and the other
                // cannot be a via
                if( ( aSegment->GetWidth() == other->GetWidth() ) &&
                        ( other->Type() == PCB_TRACE_T ) )
                {
                    // There can be only one segment connected
                    other->SetState( BUSY, true );
                    TRACK* yet_another = aSegment->GetTrack( m_brd->m_Track, NULL,
                            endpoint, true, false );
                    other->SetState( BUSY, false );

                    if( !yet_another )
                    {
                        // Try to merge them
                        TRACK* segDelete = mergeCollinearSegmentIfPossible( aSegment,
                                other, endpoint );

                        // Merge succesful, the other one has to go away
                        if( segDelete )
                        {
                            m_brd->Remove( segDelete );
                            m_commit.Removed( segDelete );
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
bool TRACKS_CLEANER::cleanupSegments()
{
    bool modified = false;

    // Easy things first
    modified |= deleteNullSegments();

    buildTrackConnectionInfo();

    std::set<BOARD_ITEM*> toRemove;

    // Delete redundant segments, i.e. segments having the same end points and layers
    // (can happens when blocks are copied on themselve)
    for( auto segment : m_brd->Tracks() )
        removeDuplicatesOfTrack( segment, toRemove );

    modified |= removeItems( toRemove );
    modified = true;

    if( modified )
        buildTrackConnectionInfo();

    // merge collinear segments:
    TRACK* nextsegment;

	for( TRACK* segment = m_brd->m_Track; segment; segment = nextsegment )
    {
        nextsegment = segment->Next();

        if( segment->Type() == PCB_TRACE_T )
        {
            bool merged_this = MergeCollinearTracks( segment );

            if( merged_this ) // The current segment was modified, retry to merge it again
            {
                nextsegment = segment->Next();
                modified = true;
            }
        }
    }

    return modified;
}


/* Utility: check for parallelism between two segments */
static bool parallelismTest( int dx1, int dy1, int dx2, int dy2 )
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


/** Function used by cleanupSegments.
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

static void updateConn( TRACK *track, std::shared_ptr<CONNECTIVITY_DATA> connectivity )
{
    for( auto pad : connectivity->GetConnectedPads( track ) )
    {
        if( pad->HitTest( track->GetStart() ) )
        {
            track->SetState( START_ON_PAD, true );
        }

        if( pad->HitTest( track->GetEnd() ) )
        {
            track->SetState( END_ON_PAD, true );
        }
    }
}


TRACK* TRACKS_CLEANER::mergeCollinearSegmentIfPossible( TRACK* aTrackRef, TRACK* aCandidate,
                                       ENDPOINT_T aEndType )
{
    // First of all, they must be of the same width and must be both actual tracks
    if( ( aTrackRef->GetWidth() != aCandidate->GetWidth() ) ||
        ( aTrackRef->Type() != PCB_TRACE_T ) ||
        ( aCandidate->Type() != PCB_TRACE_T ) )
        return NULL;

    // Trivial case: exactly the same track
    if( ( aTrackRef->GetStart() == aCandidate->GetStart() ) &&
        ( aTrackRef->GetEnd() == aCandidate->GetEnd() ) )
        return aCandidate;

    if( ( aTrackRef->GetStart() == aCandidate->GetEnd() ) &&
        ( aTrackRef->GetEnd() == aCandidate->GetStart() ) )
        return aCandidate;

    // Weed out non-parallel tracks
    if( !parallelismTest( aTrackRef->GetEnd().x - aTrackRef->GetStart().x,
                aTrackRef->GetEnd().y - aTrackRef->GetStart().y,
                aCandidate->GetEnd().x - aCandidate->GetStart().x,
                aCandidate->GetEnd().y - aCandidate->GetStart().y ) )
        return NULL;

    auto connectivity = m_brd->GetConnectivity();

    updateConn( aTrackRef, connectivity );
    updateConn( aCandidate, connectivity );

    if( aEndType == ENDPOINT_START )
    {
        // We do not have a pad, which is a always terminal point for a track
        if( aTrackRef->GetState( START_ON_PAD ) )
            return NULL;

        /* change the common point coordinate of pt_segm to use the other point
         * of pt_segm (pt_segm will be removed later) */
        if( aTrackRef->GetStart() == aCandidate->GetStart() )
        {
            m_commit.Modify( aTrackRef );
            aTrackRef->SetStart( aCandidate->GetEnd() );
            aTrackRef->SetState( START_ON_PAD, aCandidate->GetState( END_ON_PAD ) );
            connectivity->Update( aTrackRef );
            return aCandidate;
        }
        else
        {
            m_commit.Modify( aTrackRef );
            aTrackRef->SetStart( aCandidate->GetStart() );
            aTrackRef->SetState( START_ON_PAD, aCandidate->GetState( START_ON_PAD ) );
            connectivity->Update( aTrackRef );
            return aCandidate;
        }
    }
    else    // aEndType == END
    {
        // We do not have a pad, which is a always terminal point for a track
        if( aTrackRef->GetState( END_ON_PAD ) )
            return NULL;

        /* change the common point coordinate of pt_segm to use the other point
         * of pt_segm (pt_segm will be removed later) */
        if( aTrackRef->GetEnd() == aCandidate->GetStart() )
        {
            m_commit.Modify( aTrackRef );
            aTrackRef->SetEnd( aCandidate->GetEnd() );
            aTrackRef->SetState( END_ON_PAD, aCandidate->GetState( END_ON_PAD ) );
            connectivity->Update( aTrackRef );

            return aCandidate;
        }
        else
        {
            m_commit.Modify( aTrackRef );
            aTrackRef->SetEnd( aCandidate->GetStart() );
            aTrackRef->SetState( END_ON_PAD, aCandidate->GetState( START_ON_PAD ) );
            connectivity->Update( aTrackRef );
            return aCandidate;
        }
    }

    return NULL;
}


bool PCB_EDIT_FRAME::RemoveMisConnectedTracks()
{
    // Old model has to be refreshed, GAL normally does not keep updating it
    Compile_Ratsnest( NULL, false );
    BOARD_COMMIT commit( this );

    TRACKS_CLEANER cleaner( GetBoard(), commit );
    bool isModified = cleaner.CleanupBoard( true, false, false, false );

    if( isModified )
    {
        // Clear undo and redo lists to avoid inconsistencies between lists
        SetCurItem( NULL );
        commit.Push( _( "Board cleanup" ) );
        Compile_Ratsnest( NULL, true );
    }

    m_canvas->Refresh( true );

    return isModified;
}
