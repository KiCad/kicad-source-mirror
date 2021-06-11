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

#include <reporter.h>
#include <board_commit.h>
#include <cleanup_item.h>
#include <connectivity/connectivity_algo.h>
#include <connectivity/connectivity_data.h>
#include <tool/tool_manager.h>
#include <tools/pcb_actions.h>
#include <tools/global_edit_tool.h>
#include <drc/drc_rtree.h>
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
void TRACKS_CLEANER::CleanupBoard( bool aDryRun, std::vector<std::shared_ptr<CLEANUP_ITEM> >* aItemsList,
                                   bool aRemoveMisConnected, bool aCleanVias, bool aMergeSegments,
                                   bool aDeleteUnconnected, bool aDeleteTracksinPad, bool aDeleteDanglingVias )
{
    bool has_deleted = false;

    m_dryRun = aDryRun;
    m_itemsList = aItemsList;

    cleanup( aCleanVias, aMergeSegments || aRemoveMisConnected, aMergeSegments, aMergeSegments );

    if( aRemoveMisConnected )
        removeShortingTrackSegments();

    if( aDeleteTracksinPad )
        deleteTracksInPads();

    has_deleted = deleteDanglingTracks( aDeleteUnconnected, aDeleteDanglingVias );

    if( has_deleted && aMergeSegments )
        cleanup( false, false, false, true );
}


void TRACKS_CLEANER::removeShortingTrackSegments()
{
    std::shared_ptr<CONNECTIVITY_DATA> connectivity = m_brd->GetConnectivity();

    std::set<BOARD_ITEM *> toRemove;

    for( PCB_TRACK* segment : m_brd->Tracks() )
    {
        // Assume that the user knows what they are doing
        if( segment->IsLocked() )
            continue;

        for( PAD* testedPad : connectivity->GetConnectedPads( segment ) )
        {
            if( segment->GetNetCode() != testedPad->GetNetCode() )
            {
                std::shared_ptr<CLEANUP_ITEM> item;

                if( segment->Type() == PCB_VIA_T )
                    item = std::make_shared<CLEANUP_ITEM>( CLEANUP_SHORTING_VIA );
                else
                    item = std::make_shared<CLEANUP_ITEM>( CLEANUP_SHORTING_TRACK );

                item->SetItems( segment );
                m_itemsList->push_back( item );

                toRemove.insert( segment );
            }
        }

        for( PCB_TRACK* testedTrack : connectivity->GetConnectedTracks( segment ) )
        {
            if( segment->GetNetCode() != testedTrack->GetNetCode() )
            {
                std::shared_ptr<CLEANUP_ITEM> item;

                if( segment->Type() == PCB_VIA_T )
                    item = std::make_shared<CLEANUP_ITEM>( CLEANUP_SHORTING_VIA );
                else
                    item = std::make_shared<CLEANUP_ITEM>( CLEANUP_SHORTING_TRACK );

                item->SetItems( segment );
                m_itemsList->push_back( item );

                toRemove.insert( segment );
            }
        }
    }

    if( !m_dryRun )
        removeItems( toRemove );
}


bool TRACKS_CLEANER::testTrackEndpointIsNode( PCB_TRACK* aTrack, bool aTstStart )
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


bool TRACKS_CLEANER::deleteDanglingTracks( bool aTrack, bool aVia )
{
    bool item_erased = false;
    bool modified = false;

    if( !aTrack && !aVia )
        return false;

    do // Iterate when at least one track is deleted
    {
        item_erased = false;
        // Ensure the connectivity is up to date, especially after removind a dangling segment
        m_brd->BuildConnectivity();

        // Keep a duplicate deque to all deleting in the primary
        std::deque<PCB_TRACK*> temp_tracks( m_brd->Tracks() );

        for( PCB_TRACK* track : temp_tracks )
        {
            if( track->IsLocked() )
                continue;

            if( !aVia && track->Type() == PCB_VIA_T )
                continue;

            if( !aTrack && ( track->Type() == PCB_TRACE_T || track->Type() == PCB_ARC_T ) )
                continue;

            // Test if a track (or a via) endpoint is not connected to another track or zone.
            if( m_brd->GetConnectivity()->TestTrackEndpointDangling( track ) )
            {
                std::shared_ptr<CLEANUP_ITEM> item;

                if( track->Type() == PCB_VIA_T )
                    item = std::make_shared<CLEANUP_ITEM>( CLEANUP_DANGLING_VIA );
                else
                    item = std::make_shared<CLEANUP_ITEM>( CLEANUP_DANGLING_TRACK );

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


void TRACKS_CLEANER::deleteTracksInPads()
{
    std::set<BOARD_ITEM*> toRemove;

    // Delete tracks that start and end on the same pad
    std::shared_ptr<CONNECTIVITY_DATA> connectivity = m_brd->GetConnectivity();

    for( PCB_TRACK* track : m_brd->Tracks() )
    {
        if( track->IsLocked() )
            continue;

        if( track->Type() == PCB_VIA_T )
            continue;

        // Mark track if connected to pads
        for( PAD* pad : connectivity->GetConnectedPads( track ) )
        {
            if( pad->HitTest( track->GetStart() ) && pad->HitTest( track->GetEnd() ) )
            {
                SHAPE_POLY_SET poly;
                track->TransformShapeWithClearanceToPolygon( poly, track->GetLayer(), 0,
                                                             ARC_HIGH_DEF, ERROR_INSIDE );

                poly.BooleanSubtract( *pad->GetEffectivePolygon(), SHAPE_POLY_SET::PM_FAST );

                if( poly.IsEmpty() )
                {
                    std::shared_ptr<CLEANUP_ITEM> item = std::make_shared<CLEANUP_ITEM>( CLEANUP_TRACK_IN_PAD );
                    item->SetItems( track );
                    m_itemsList->push_back( item );

                    toRemove.insert( track );
                }
            }
        }
    }

    if( !m_dryRun )
        removeItems( toRemove );
}


/**
 * Geometry-based cleanup: duplicate items, null items, colinear items.
 */
void TRACKS_CLEANER::cleanup( bool aDeleteDuplicateVias, bool aDeleteNullSegments,
                              bool aDeleteDuplicateSegments, bool aMergeSegments )
{
    DRC_RTREE rtree;

    for( PCB_TRACK* track : m_brd->Tracks() )
    {
        track->ClearFlags( IS_DELETED | SKIP_STRUCT );
        rtree.Insert( track, track->GetLayer() );
    }

    std::set<BOARD_ITEM*> toRemove;

    for( PCB_TRACK* track : m_brd->Tracks() )
    {
        if( track->HasFlag( IS_DELETED ) || track->IsLocked() )
            continue;

        if( aDeleteDuplicateVias && track->Type() == PCB_VIA_T )
        {
            PCB_VIA* via = static_cast<PCB_VIA*>( track );

            if( via->GetStart() != via->GetEnd() )
                via->SetEnd( via->GetStart() );

            rtree.QueryColliding( via, via->GetLayer(), via->GetLayer(),
                    // Filter:
                    [&]( BOARD_ITEM* aItem ) -> bool
                    {
                        return aItem->Type() == PCB_VIA_T
                                  && !aItem->HasFlag( SKIP_STRUCT )
                                  && !aItem->HasFlag( IS_DELETED );
                    },
                    // Visitor:
                    [&]( BOARD_ITEM* aItem ) -> bool
                    {
                        PCB_VIA* other = static_cast<PCB_VIA*>( aItem );

                        if( via->GetPosition() == other->GetPosition()
                                && via->GetViaType() == other->GetViaType()
                                && via->GetLayerSet() == other->GetLayerSet() )
                        {
                            auto item = std::make_shared<CLEANUP_ITEM>( CLEANUP_REDUNDANT_VIA );
                            item->SetItems( via );
                            m_itemsList->push_back( item );

                            via->SetFlags( IS_DELETED );
                            toRemove.insert( via );
                        }

                        return true;
                    } );

            // To delete through Via on THT pads at same location
            // Examine the list of connected pads: if a through pad is found, the via is redundant
            for( PAD* pad : m_brd->GetConnectivity()->GetConnectedPads( via ) )
            {
                const LSET all_cu = LSET::AllCuMask();

                if( ( pad->GetLayerSet() & all_cu ) == all_cu )
                {
                    auto item = std::make_shared<CLEANUP_ITEM>( CLEANUP_REDUNDANT_VIA );
                    item->SetItems( via, pad );
                    m_itemsList->push_back( item );

                    via->SetFlags( IS_DELETED );
                    toRemove.insert( via );
                    break;
                }
            }

            via->SetFlags( SKIP_STRUCT );
        }

        if( aDeleteNullSegments && track->Type() != PCB_VIA_T )
        {
            if( track->IsNull() )
            {
                auto item = std::make_shared<CLEANUP_ITEM>( CLEANUP_ZERO_LENGTH_TRACK );
                item->SetItems( track );
                m_itemsList->push_back( item );

                track->SetFlags( IS_DELETED );
                toRemove.insert( track );
            }
        }

        if( aDeleteDuplicateSegments && track->Type() == PCB_TRACE_T )
        {
            rtree.QueryColliding( track, track->GetLayer(), track->GetLayer(),
                    // Filter:
                    [&]( BOARD_ITEM* aItem ) -> bool
                    {
                        return aItem->Type() == PCB_TRACE_T
                                  && !aItem->HasFlag( SKIP_STRUCT )
                                  && !aItem->HasFlag( IS_DELETED );
                    },
                    // Visitor:
                    [&]( BOARD_ITEM* aItem ) -> bool
                    {
                        PCB_TRACK* other = static_cast<PCB_TRACK*>( aItem );

                        if( track->IsPointOnEnds( other->GetStart() )
                                && track->IsPointOnEnds( other->GetEnd() )
                                && track->GetWidth() == other->GetWidth()
                                && track->GetLayer() == other->GetLayer() )
                        {
                            auto item = std::make_shared<CLEANUP_ITEM>( CLEANUP_DUPLICATE_TRACK );
                            item->SetItems( track );
                            m_itemsList->push_back( item );

                            track->SetFlags( IS_DELETED );
                            toRemove.insert( track );
                        }

                        return true;
                    } );

            track->SetFlags( SKIP_STRUCT );
        }
    }

    if( !m_dryRun )
        removeItems( toRemove );

    if( aMergeSegments )
    {
        bool merged;

        do
        {
            merged = false;
            m_brd->BuildConnectivity();

            // Keep a duplicate deque to all deleting in the primary
            std::deque<PCB_TRACK*> temp_segments( m_brd->Tracks() );

            // merge collinear segments:
            for( PCB_TRACK* segment : temp_segments )
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
                            PCB_TRACK* candidateSegment = static_cast<PCB_TRACK*>( candidateItem );

                            // Do not merge segments having different widths: it is a frequent case
                            // to draw a track between 2 pads:
                            if( candidateSegment->GetWidth() != segment->GetWidth() )
                                continue;

                            if( segment->ApproxCollinear( *candidateSegment ) )
                                merged |= mergeCollinearSegments( segment, candidateSegment );
                        }
                    }
                }
            }
        } while( merged );
    }

    for( PCB_TRACK* track : m_brd->Tracks() )
        track->ClearFlags( IS_DELETED | SKIP_STRUCT );
}


bool TRACKS_CLEANER::mergeCollinearSegments( PCB_TRACK* aSeg1, PCB_TRACK* aSeg2 )
{
    if( aSeg1->IsLocked() || aSeg2->IsLocked() )
        return false;

    auto connectivity = m_brd->GetConnectivity();

    // Verify the removed point after merging is not a node.
    // If it is a node (i.e. if more than one other item is connected, the segments cannot be merged
    PCB_TRACK dummy_seg( *aSeg1 );

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
            return false;
    }

    if( aSeg1->GetEnd() != dummy_seg.GetStart() && aSeg1->GetEnd() != dummy_seg.GetEnd() )
    {
        if( testTrackEndpointIsNode( aSeg1, false ) )
            return false;
    }

    std::shared_ptr<CLEANUP_ITEM> item = std::make_shared<CLEANUP_ITEM>( CLEANUP_MERGE_TRACKS );
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

        // Merge successful, seg2 has to go away
        m_brd->Remove( aSeg2 );
        m_commit.Removed( aSeg2 );
    }

    return true;
}


void TRACKS_CLEANER::removeItems( std::set<BOARD_ITEM*>& aItems )
{
    for( auto item : aItems )
    {
        m_brd->Remove( item );
        m_commit.Removed( item );
    }
}
