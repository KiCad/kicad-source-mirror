/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2004-2018 Jean-Pierre Charras, jp.charras at wanadoo.fr
 * Copyright (C) 2011 Wayne Stambaugh <stambaughw@verizon.net>
 * Copyright (C) 1992-2018 KiCad Developers, see AUTHORS.txt for contributors.
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
 * @file tracks_cleaner.cpp
 * @brief functions to clean tracks: remove null length and redundant segments
 */


#include <fctsys.h>
#include <class_drawpanel.h>
#include <pcb_edit_frame.h>
#include <pcbnew.h>
#include <class_board.h>
#include <class_track.h>
#include <dialog_cleanup_tracks_and_vias.h>
#include <reporter.h>
#include <board_commit.h>
#include <connectivity/connectivity_algo.h>
#include <connectivity/connectivity_data.h>

#include <tool/tool_manager.h>
#include <tools/pcb_actions.h>

#include <tracks_cleaner.h>


/* Install the cleanup dialog frame to know what should be cleaned
*/
void PCB_EDIT_FRAME::Clean_Pcb()
{
    DIALOG_CLEANUP_TRACKS_AND_VIAS dlg( this );

    dlg.ShowModal();
}


TRACKS_CLEANER::TRACKS_CLEANER( EDA_UNITS_T aUnits, BOARD* aPcb, BOARD_COMMIT& aCommit ) :
        m_units( aUnits ),
        m_brd( aPcb ),
        m_commit( aCommit ),
        m_dryRun( true ),
        m_itemsList( nullptr )
{
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
                track->SetState( START_ON_PAD, true );

            if( pad->HitTest( track->GetEnd() ) )
                track->SetState( END_ON_PAD, true );
        }
    }
}


/* Main cleaning function.
 *  Delete
 * - Redundant points on tracks (merge aligned segments)
 * - vias on pad
 * - null length segments
 */
bool TRACKS_CLEANER::CleanupBoard( bool aDryRun, DRC_LIST* aItemsList,
                                   bool aRemoveMisConnected,
                                   bool aCleanVias,
                                   bool aMergeSegments,
                                   bool aDeleteUnconnected )
{
    m_dryRun = aDryRun;
    m_itemsList = aItemsList;
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
            {
                if( m_itemsList )
                {
                    m_itemsList->emplace_back( new DRC_ITEM( m_units, DRCE_SHORT,
                                                             segment, segment->GetPosition(),
                                                             nullptr, wxPoint() ) );
                }

                toRemove.insert( segment );
            }
        }

        for( auto testedTrack : connectivity->GetConnectedTracks( segment ) )
        {
            if( segment->GetNetCode() != testedTrack->GetNetCode() && !testedTrack->GetState( FLAG0 ) )
            {
                if( m_itemsList )
                {
                    m_itemsList->emplace_back( new DRC_ITEM( m_units, DRCE_SHORT,
                                                             segment, segment->GetPosition(),
                                                             nullptr, wxPoint() ) );
                }

                toRemove.insert( segment );
            }
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

        if( ( alt_via->GetViaType() == VIA_THROUGH ) && alt_via->GetStart() == aVia->GetStart() )
        {
            if( m_itemsList )
            {
                m_itemsList->emplace_back( new DRC_ITEM( m_units, DRCE_REDUNDANT_VIA,
                                                         alt_via, alt_via->GetPosition(),
                                                         nullptr, wxPoint() ) );
            }

            aToRemove.insert ( alt_via );
        }
    }
}


bool TRACKS_CLEANER::cleanupVias()
{
    std::set<BOARD_ITEM*> toRemove;

    for( VIA* via = GetFirstVia( m_brd->m_Track ); via != NULL; via = GetFirstVia( via->Next() ) )
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
                    if( m_itemsList )
                    {
                        m_itemsList->emplace_back( new DRC_ITEM( m_units, DRCE_REDUNDANT_VIA,
                                                                 via, via->GetPosition(),
                                                                 nullptr, wxPoint() ) );
                    }

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

    for( const auto& anchor : anchors )
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
        buildTrackConnectionInfo();
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
                if( m_itemsList )
                {
                    int code = track->IsTrack() ? DRCE_DANGLING_TRACK : DRCE_DANGLING_VIA;
                    m_itemsList->emplace_back( new DRC_ITEM( m_units, code,
                                                             track, track->GetPosition(),
                                                             nullptr, wxPoint() ) );
                }

                if( !m_dryRun )
                {
                    m_brd->Remove( track );
                    m_commit.Removed( track );

                    /* keep iterating, because a track connected to the deleted track
                     * now perhaps is not connected and should be deleted */
                    item_erased = true;
                    modified = true;
                }
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
        {
            if( m_itemsList )
            {
                m_itemsList->emplace_back( new DRC_ITEM( m_units, DRCE_ZERO_LENGTH_TRACK,
                                                         segment, segment->GetPosition(),
                                                         nullptr, wxPoint() ) );
            }

            toRemove.insert( segment );
        }
    }

    return removeItems( toRemove );
}

void TRACKS_CLEANER::removeDuplicatesOfTrack( const TRACK *aSeg, std::set<BOARD_ITEM*>& aToRemove )
{
    if( aSeg->GetEditFlags() & STRUCT_DELETED )
        return;

    for( auto seg2 : m_brd->Tracks() )
    {
        // New netcode, break out (can't be there any seg2)
        if( aSeg->GetNetCode() != seg2->GetNetCode() )
            continue;

        if( aSeg == seg2 )
            continue;

        if( seg2->GetFlags() & STRUCT_DELETED )
            continue;

        // Must be of the same type, on the same layer and with the same endpoints (although
        // they might be swapped)
        if( aSeg->Type() == seg2->Type() && aSeg->GetLayer() == seg2->GetLayer() )
        {
            if( ( aSeg->GetStart() == seg2->GetStart() && aSeg->GetEnd() == seg2->GetEnd() ) ||
                ( aSeg->GetStart() == seg2->GetEnd() && aSeg->GetEnd() == seg2->GetStart() ) )
            {
                if( m_itemsList )
                {
                    m_itemsList->emplace_back( new DRC_ITEM( m_units, DRCE_DUPLICATE_TRACK,
                                                             seg2, seg2->GetPosition(),
                                                             nullptr, wxPoint() ) );
                }

                seg2->SetFlags( STRUCT_DELETED );
                aToRemove.insert( seg2 );
            }
        }
    }
}


bool TRACKS_CLEANER::MergeCollinearTracks( TRACK* aSegment )
{
    bool merged_this = false;

    if( !aSegment->Next() )
        return merged_this;

    for( ENDPOINT_T endpoint : { ENDPOINT_START, ENDPOINT_END } )
    {
        // search for a possible segment connected to the current endpoint of the current one
        TRACK* seg2 = aSegment->GetTrack( aSegment->Next(), NULL, endpoint, true, false );

        if( seg2 )
        {
            // the two segments must have the same width and seg2 cannot be a via
            if( aSegment->GetWidth() == seg2->GetWidth() && seg2->Type() == PCB_TRACE_T )
            {
                // There can be only one segment connected
                seg2->SetState( BUSY, true );
                TRACK* seg3 = aSegment->GetTrack( m_brd->m_Track, NULL, endpoint, true, false );
                seg2->SetState( BUSY, false );

                if( seg3 )
                    continue;

                // Try to merge them
                TRACK* segDelete = mergeCollinearSegments( aSegment, seg2, endpoint );

                // Merge succesful, seg2 has to go away
                if( !m_dryRun && segDelete )
                {
                    m_brd->Remove( segDelete );
                    m_commit.Removed( segDelete );
                    merged_this = true;
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
static void updateConn( TRACK *track, const std::shared_ptr<CONNECTIVITY_DATA>& connectivity )
{
    for( auto pad : connectivity->GetConnectedPads( track ) )
    {
        if( pad->HitTest( track->GetStart() ) )
            track->SetState( START_ON_PAD, true );

        if( pad->HitTest( track->GetEnd() ) )
            track->SetState( END_ON_PAD, true );
    }
}


TRACK* TRACKS_CLEANER::mergeCollinearSegments( TRACK* aSeg1, TRACK* aSeg2,
                                               ENDPOINT_T aEndType )
{
    // First of all, they must be of the same width and must be both actual tracks
    if( aSeg1->GetWidth() != aSeg2->GetWidth() ||
        aSeg1->Type() != PCB_TRACE_T || aSeg2->Type() != PCB_TRACE_T )
    {
        return NULL;
    }

    // Trivial case: exactly the same track
    if( ( aSeg1->GetStart() == aSeg2->GetStart() && aSeg1->GetEnd() == aSeg2->GetEnd() ) ||
        ( aSeg1->GetStart() == aSeg2->GetEnd() && aSeg1->GetEnd() == aSeg2->GetStart() ))
    {
        if( m_itemsList )
        {
            m_itemsList->emplace_back( new DRC_ITEM( m_units, DRCE_DUPLICATE_TRACK,
                                                     aSeg2, aSeg2->GetPosition(),
                                                     nullptr, wxPoint() ) );
        }

        return aSeg2;
    }

    // Weed out non-parallel tracks
    if( !parallelismTest( aSeg1->GetEnd().x - aSeg1->GetStart().x,
                          aSeg1->GetEnd().y - aSeg1->GetStart().y,
                          aSeg2->GetEnd().x - aSeg2->GetStart().x,
                          aSeg2->GetEnd().y - aSeg2->GetStart().y ) )
    {
        return NULL;
    }

    auto connectivity = m_brd->GetConnectivity();

    updateConn( aSeg1, connectivity );
    updateConn( aSeg2, connectivity );

    if( ( aEndType == ENDPOINT_START && aSeg1->GetState( START_ON_PAD ) ) ||
        ( aEndType == ENDPOINT_END && aSeg1->GetState( END_ON_PAD ) ) )
    {
        // We do not have a pad, which is a always terminal point for a track
        return NULL;
    }

    if( m_itemsList )
    {
        m_itemsList->emplace_back( new DRC_ITEM( m_units, DRCE_MERGE_TRACKS,
                                                 aSeg1, aSeg1->GetPosition(),
                                                 aSeg2, aSeg2->GetPosition() ) );
    }

    if( !m_dryRun )
    {
        m_commit.Modify( aSeg1 );

        if( aEndType == ENDPOINT_START )
        {
            /* change the common point coordinate of pt_segm to use the other point
             * of pt_segm (pt_segm will be removed later) */
            if( aSeg1->GetStart() == aSeg2->GetStart() )
            {
                aSeg1->SetStart( aSeg2->GetEnd() );
                aSeg1->SetState( START_ON_PAD, aSeg2->GetState( END_ON_PAD ) );
            }
            else
            {
                aSeg1->SetStart( aSeg2->GetStart() );
                aSeg1->SetState( START_ON_PAD, aSeg2->GetState( START_ON_PAD ) );
            }
        }
        else    // aEndType == END
        {
            /* change the common point coordinate of pt_segm to use the other point
             * of pt_segm (pt_segm will be removed later) */
            if( aSeg1->GetEnd() == aSeg2->GetStart() )
            {
                aSeg1->SetEnd( aSeg2->GetEnd() );
                aSeg1->SetState( END_ON_PAD, aSeg2->GetState( END_ON_PAD ) );
            }
            else
            {
                aSeg1->SetEnd( aSeg2->GetStart() );
                aSeg1->SetState( END_ON_PAD, aSeg2->GetState( START_ON_PAD ) );
            }
        }

        connectivity->Update( aSeg1 );
    }

    return aSeg2;
}


bool TRACKS_CLEANER::removeItems( std::set<BOARD_ITEM*>& aItems )
{
    if( m_dryRun )
        return false;

    bool isModified = false;

    for( auto item : aItems )
    {
        isModified = true;
        m_brd->Remove( item );
        m_commit.Removed( item );
    }

    return isModified;
}


bool PCB_EDIT_FRAME::RemoveMisConnectedTracks()
{
    // Old model has to be refreshed, GAL normally does not keep updating it
    Compile_Ratsnest( NULL, false );
    BOARD_COMMIT commit( this );

    TRACKS_CLEANER cleaner( m_UserUnits, GetBoard(), commit );
    bool isModified = cleaner.CleanupBoard( true, nullptr, true, false, false, false );

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
