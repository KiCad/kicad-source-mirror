/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2004-2018 Jean-Pierre Charras, jp.charras at wanadoo.fr
 * Copyright (C) 2011 Wayne Stambaugh <stambaughw@verizon.net>
 * Copyright (C) 1992-2020 KiCad Developers, see AUTHORS.txt for contributors.
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
#include <reporter.h>
#include <board_commit.h>
#include <cleanup_item.h>
#include <connectivity/connectivity_algo.h>
#include <connectivity/connectivity_data.h>
#include <tool/tool_manager.h>
#include <tools/pcb_actions.h>
#include <tools/global_edit_tool.h>
#include <tracks_cleaner.h>


TRACKS_CLEANER::TRACKS_CLEANER( BOARD* aPcb, BOARD_COMMIT& aCommit ) :
        m_brd( aPcb ),
        m_commit( aCommit ),
        m_dryRun( true ),
        m_itemsList( nullptr )
{
}


/* Main cleaning function.
 *  Delete
 * - Redundant points on tracks (merge aligned segments)
 * - vias on pad
 * - null length segments
 */
void TRACKS_CLEANER::CleanupBoard( bool aDryRun, std::vector<CLEANUP_ITEM*>* aItemsList,
                                   bool aRemoveMisConnected, bool aCleanVias, bool aMergeSegments,
                                   bool aDeleteUnconnected, bool aDeleteTracksinPad )
{
    m_dryRun = aDryRun;
    m_itemsList = aItemsList;

    // Clear the flag used to mark some segments as deleted, in dry run:
    for( TRACK* segment : m_brd->Tracks() )
        segment->ClearFlags( IS_DELETED );

    // delete redundant vias
    if( aCleanVias )
        cleanupVias();

    // Remove null segments and intermediate points on aligned segments
    // If not asked, remove null segments only if remove misconnected is asked
    if( aMergeSegments )
        cleanupSegments();
    else if( aRemoveMisConnected )
        deleteNullSegments( m_brd->Tracks() );

    if( aRemoveMisConnected )
        removeBadTrackSegments();

    if( aDeleteTracksinPad )
        deleteTracksInPads();

    // Delete dangling tracks
    if( aDeleteUnconnected )
    {
        if( deleteDanglingTracks() )
        {
            // Removed tracks can leave aligned segments
            // (when a T was formed by tracks and the "vertical" segment is removed)
            if( aMergeSegments )
                cleanupSegments();
        }
    }

    // Clear the flag used to mark some segments:
    for( TRACK* segment : m_brd->Tracks() )
        segment->ClearFlags( IS_DELETED );
}


void TRACKS_CLEANER::removeBadTrackSegments()
{
    std::shared_ptr<CONNECTIVITY_DATA> connectivity = m_brd->GetConnectivity();

    std::set<BOARD_ITEM *> toRemove;

    for( TRACK* segment : m_brd->Tracks() )
    {
        segment->SetState( FLAG0, false );

        for( auto testedPad : connectivity->GetConnectedPads( segment ) )
        {
            if( segment->GetNetCode() != testedPad->GetNetCode() )
            {
                CLEANUP_ITEM* item = new CLEANUP_ITEM( CLEANUP_SHORT );
                item->SetItems( segment );
                m_itemsList->push_back( item );

                toRemove.insert( segment );
            }
        }

        for( TRACK* testedTrack : connectivity->GetConnectedTracks( segment ) )
        {
            if( segment->GetNetCode() != testedTrack->GetNetCode() && !testedTrack->GetState( FLAG0 ) )
            {
                CLEANUP_ITEM* item = new CLEANUP_ITEM( CLEANUP_SHORT );
                item->SetItems( segment );
                m_itemsList->push_back( item );

                toRemove.insert( segment );
            }
        }
    }

    if( !m_dryRun )
        removeItems( toRemove );
}


void TRACKS_CLEANER::cleanupVias()
{
    std::set<BOARD_ITEM*> toRemove;
    std::vector<VIA*>     vias;

    for( TRACK* track : m_brd->Tracks() )
    {
        if( auto via = dyn_cast<VIA*>( track ) )
            vias.push_back( via );
    }

    for( auto via1_it = vias.begin(); via1_it != vias.end(); via1_it++ )
    {
        VIA* via1 = *via1_it;

        if( via1->IsLocked() )
            continue;

        if( via1->GetStart() != via1->GetEnd() )
            via1->SetEnd( via1->GetStart() );

        // To delete through Via on THT pads at same location
        // Examine the list of connected pads:
        // if a through pad is found, the via can be removed

        const std::vector<D_PAD*> pads = m_brd->GetConnectivity()->GetConnectedPads( via1 );

        for( D_PAD* pad : pads )
        {
            const LSET all_cu = LSET::AllCuMask();

            if( ( pad->GetLayerSet() & all_cu ) == all_cu )
            {
                CLEANUP_ITEM* item = new CLEANUP_ITEM( CLEANUP_REDUNDANT_VIA );
                item->SetItems( via1, pad );
                m_itemsList->push_back( item );

                // redundant: delete the via
                toRemove.insert( via1 );
                break;
            }
        }

        for( auto via2_it = via1_it + 1; via2_it != vias.end(); via2_it++ )
        {
            VIA* via2 = *via2_it;

            if( via1->GetPosition() != via2->GetPosition() || via2->IsLocked() )
                continue;

            if( via1->GetViaType() == via2->GetViaType() )
            {
                CLEANUP_ITEM* item = new CLEANUP_ITEM( CLEANUP_REDUNDANT_VIA );
                item->SetItems( via1, via2 );
                m_itemsList->push_back( item );

                toRemove.insert( via2 );
                break;
            }
        }
    }

    if( !m_dryRun )
        removeItems( toRemove );
}


bool TRACKS_CLEANER::testTrackEndpointIsNode( TRACK* aTrack, bool aTstStart )
{
    // A node is a point where more than 2 items are connected.

    auto connectivity = m_brd->GetConnectivity();
    auto items = connectivity->GetConnectivityAlgo()->ItemEntry( aTrack ).GetItems();

    if( items.empty() )
        return false;

    auto citem = items.front();

    if( !citem->Valid() )
        return false;

    auto anchors = citem->Anchors();

    VECTOR2I refpoint = aTstStart ? aTrack->GetStart() : aTrack->GetEnd();

    for( const auto& anchor : anchors )
    {
        if( anchor->Pos() != refpoint )
            continue;

        // The right anchor point is found: if more than one other item
        // (pad, via, track...) is connected, it is a node:
        return anchor->ConnectedItemsCount() > 1;
    }

    return false;
}


bool TRACKS_CLEANER::deleteDanglingTracks()
{
    bool item_erased = false;
    bool modified = false;

    do // Iterate when at least one track is deleted
    {
        item_erased = false;
        // Ensure the connectivity is up to date, especially after removind a dangling segment
        m_brd->BuildConnectivity();

        // Keep a duplicate deque to all deleting in the primary
        std::deque<TRACK*> temp_tracks( m_brd->Tracks() );

        for( TRACK* track : temp_tracks )
        {
            bool    flag_erase = false; // Start without a good reason to erase it
            wxPoint pos;

            // Tst if a track (or a via) endpoint is not connected to another track or to a zone.
            if( m_brd->GetConnectivity()->TestTrackEndpointDangling( track, &pos ) )
                flag_erase = true;

            if( flag_erase )
            {
                int errorCode = track->IsTrack() ? CLEANUP_DANGLING_TRACK : CLEANUP_DANGLING_VIA;
                CLEANUP_ITEM* item = new CLEANUP_ITEM( errorCode );
                item->SetItems( track );
                m_itemsList->push_back( item );

                if( !m_dryRun )
                {
                    m_brd->Remove( track );
                    m_commit.Removed( track );

                    /* keep iterating, because a track connected to the deleted track
                     * now perhaps is not connected and should be deleted */
                    item_erased = true;
                    modified = true;
                }
                // Fix me: In dry run we should disable the track to erase and retry with this disabled track
                // However the connectivity algo does not handle disabled items.
            }
        }
    } while( item_erased ); // A segment was erased: test for some new dangling segments

    return modified;
}


// Delete null length track segments
void TRACKS_CLEANER::deleteNullSegments( TRACKS& aTracks )
{
    std::set<BOARD_ITEM *> toRemove;

    for( TRACK* segment : aTracks )
    {
        if( segment->IsNull() && segment->Type() == PCB_TRACE_T && !segment->IsLocked() )
        {
            CLEANUP_ITEM* item = new CLEANUP_ITEM( CLEANUP_ZERO_LENGTH_TRACK );
            item->SetItems( segment );
            m_itemsList->push_back( item );

            toRemove.insert( segment );
        }
    }

    if( !m_dryRun )
        removeItems( toRemove );
}


void TRACKS_CLEANER::deleteTracksInPads()
{
    std::set<BOARD_ITEM*> toRemove;

    // Delete tracks that start and end on the same pad
    std::shared_ptr<CONNECTIVITY_DATA> connectivity = m_brd->GetConnectivity();

    for( TRACK* track : m_brd->Tracks() )
    {
        // Mark track if connected to pads
        for( D_PAD* pad : connectivity->GetConnectedPads( track ) )
        {
            if( pad->HitTest( track->GetStart() ) && pad->HitTest( track->GetEnd() ) )
            {
                CLEANUP_ITEM* item = new CLEANUP_ITEM( CLEANUP_TRACK_IN_PAD );
                item->SetItems( track );
                m_itemsList->push_back( item );

                toRemove.insert( track );
            }
        }
    }

    if( !m_dryRun )
        removeItems( toRemove );
}


// Delete null length segments, and intermediate points ..
void TRACKS_CLEANER::cleanupSegments()
{
    // Easy things first
    deleteNullSegments( m_brd->Tracks() );

    std::set<BOARD_ITEM*> toRemove;

    // Remove duplicate segments (2 superimposed identical segments):
    for( auto it = m_brd->Tracks().begin(); it != m_brd->Tracks().end(); it++ )
    {
        TRACK* track1 = *it;

        if( track1->Type() != PCB_TRACE_T || track1->HasFlag( IS_DELETED ) || track1->IsLocked() )
            continue;

        for( auto it2 = it + 1; it2 != m_brd->Tracks().end(); it2++ )
        {
            TRACK* track2 = *it2;

            if( track2->HasFlag( IS_DELETED ) )
                continue;

            if( track1->IsPointOnEnds( track2->GetStart() )
                    && track1->IsPointOnEnds( track2->GetEnd() )
                    && track1->GetWidth() == track2->GetWidth()
                    && track1->GetLayer() == track2->GetLayer() )
            {
                CLEANUP_ITEM* item = new CLEANUP_ITEM( CLEANUP_DUPLICATE_TRACK );
                item->SetItems( track2 );
                m_itemsList->push_back( item );

                track2->SetFlags( IS_DELETED );
                toRemove.insert( track2 );
            }
        }
    }

    if( !m_dryRun )
        removeItems( toRemove );

    // Keep a duplicate deque to all deleting in the primary
    std::deque<TRACK*> temp_segments( m_brd->Tracks() );

    // merge collinear segments:
    for( TRACK* segment : temp_segments )
    {
        if( segment->Type() != PCB_TRACE_T )    // one can merge only track collinear segments, not vias.
            continue;

        if( segment->HasFlag( IS_DELETED ) )  // already taken in account
            continue;

        auto connectivity = m_brd->GetConnectivity();

        auto& entry = connectivity->GetConnectivityAlgo()->ItemEntry( segment );

        for( CN_ITEM* citem : entry.GetItems() )
        {
            for( CN_ITEM* connected : citem->ConnectedItems() )
            {
                if( !connected->Valid() )
                    continue;

                BOARD_CONNECTED_ITEM* candidateItem = connected->Parent();

                if( candidateItem->Type() == PCB_TRACE_T && !candidateItem->HasFlag( IS_DELETED ) )
                {
                    TRACK* candidateSegment = static_cast<TRACK*>( candidateItem );

                    // Do not merge segments having different widths: it is a frequent case
                    // to draw a track between 2 pads:
                    if( candidateSegment->GetWidth() != segment->GetWidth() )
                        continue;

                    if( segment->ApproxCollinear( *candidateSegment ) )
                        mergeCollinearSegments( segment, candidateSegment );
                }
            }
        }
    }
}


void TRACKS_CLEANER::mergeCollinearSegments( TRACK* aSeg1, TRACK* aSeg2 )
{
    if( aSeg1->IsLocked() || aSeg2->IsLocked() )
        return;

    auto connectivity = m_brd->GetConnectivity();

    // Verify the removed point after merging is not a node.
    // If it is a node (i.e. if more than one other item is connected, the segments cannot be merged
    TRACK dummy_seg( *aSeg1 );

    // Calculate the new ends of the segment to merge, and store them to dummy_seg:
    int min_x = std::min( aSeg1->GetStart().x,
            std::min( aSeg1->GetEnd().x, std::min( aSeg2->GetStart().x, aSeg2->GetEnd().x ) ) );
    int min_y = std::min( aSeg1->GetStart().y,
            std::min( aSeg1->GetEnd().y, std::min( aSeg2->GetStart().y, aSeg2->GetEnd().y ) ) );
    int max_x = std::max( aSeg1->GetStart().x,
            std::max( aSeg1->GetEnd().x, std::max( aSeg2->GetStart().x, aSeg2->GetEnd().x ) ) );
    int max_y = std::max( aSeg1->GetStart().y,
            std::max( aSeg1->GetEnd().y, std::max( aSeg2->GetStart().y, aSeg2->GetEnd().y ) ) );

    if( ( aSeg1->GetStart().x > aSeg1->GetEnd().x )
            == ( aSeg1->GetStart().y > aSeg1->GetEnd().y ) )
    {
        dummy_seg.SetStart( wxPoint( min_x, min_y ) );
        dummy_seg.SetEnd( wxPoint( max_x, max_y ) );
    }
    else
    {
        dummy_seg.SetStart( wxPoint( min_x, max_y ) );
        dummy_seg.SetEnd( wxPoint( max_x, min_y ) );
    }

    // Now find the removed end(s) and stop merging if it is a node:
    if( aSeg1->GetStart() != dummy_seg.GetStart() && aSeg1->GetStart() != dummy_seg.GetEnd() )
    {
        if( testTrackEndpointIsNode( aSeg1, true ) )
            return;
    }

    if( aSeg1->GetEnd() != dummy_seg.GetStart() && aSeg1->GetEnd() != dummy_seg.GetEnd() )
    {
        if( testTrackEndpointIsNode( aSeg1, false ) )
            return;
    }

    CLEANUP_ITEM* item = new CLEANUP_ITEM( CLEANUP_MERGE_TRACKS );
    item->SetItems( aSeg1, aSeg2 );
    m_itemsList->push_back( item );

    aSeg2->SetFlags( IS_DELETED );

    if( !m_dryRun )
    {
        m_commit.Modify( aSeg1 );
        *aSeg1 = dummy_seg;

        connectivity->Update( aSeg1 );

        // Clear the status flags here after update.
        for( auto pad : connectivity->GetConnectedPads( aSeg1 ) )
        {
            aSeg1->SetState( BEGIN_ONPAD, pad->HitTest( aSeg1->GetStart() ) );
            aSeg1->SetState( END_ONPAD, pad->HitTest( aSeg1->GetEnd() ) );
        }

        // Merge succesful, seg2 has to go away
        m_brd->Remove( aSeg2 );
        m_commit.Removed( aSeg2 );
    }
}


void TRACKS_CLEANER::removeItems( std::set<BOARD_ITEM*>& aItems )
{
    for( auto item : aItems )
    {
        m_brd->Remove( item );
        m_commit.Removed( item );
    }
}
