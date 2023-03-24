/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2004-2018 Jean-Pierre Charras, jp.charras at wanadoo.fr
 * Copyright (C) 2011 Wayne Stambaugh <stambaughw@verizon.net>
 * Copyright (C) 1992-2022 KiCad Developers, see AUTHORS.txt for contributors.
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
        m_itemsList( nullptr ),
        m_reporter( nullptr )
{
}


/* Main cleaning function.
 *  Delete
 * - Redundant points on tracks (merge aligned segments)
 * - vias on pad
 * - null length segments
 */
void TRACKS_CLEANER::CleanupBoard( bool aDryRun,
                                   std::vector<std::shared_ptr<CLEANUP_ITEM> >* aItemsList,
                                   bool aRemoveMisConnected, bool aCleanVias, bool aMergeSegments,
                                   bool aDeleteUnconnected, bool aDeleteTracksinPad,
                                   bool aDeleteDanglingVias, REPORTER* aReporter )
{
    m_reporter = aReporter;
    bool has_deleted = false;

    m_dryRun = aDryRun;
    m_itemsList = aItemsList;

    if( m_reporter )
    {
        if( aDryRun )
            m_reporter->Report( _( "Checking null tracks and vias..." ) );
        else
            m_reporter->Report( _( "Removing null tracks and vias..." ) );

        wxSafeYield();      // Timeslice to update UI
    }

    bool removeNullSegments = aMergeSegments || aRemoveMisConnected;
    cleanup( aCleanVias, removeNullSegments, aMergeSegments /* dup segments*/, aMergeSegments );

    if( m_reporter )
    {
        if( aDryRun )
            m_reporter->Report( _( "Checking redundant tracks..." ) );
        else
            m_reporter->Report( _( "Removing redundant tracks..." ) );

        wxSafeYield();      // Timeslice to update UI
    }

    cleanup( false, false, true, aMergeSegments );

    if( aRemoveMisConnected )
    {
        if( m_reporter )
        {
            if( aDryRun )
                m_reporter->Report( _( "Checking shorting tracks..." ) );
            else
                m_reporter->Report( _( "Removing shorting tracks..." ) );

            wxSafeYield();      // Timeslice to update UI
        }

        removeShortingTrackSegments();
    }

    if( aDeleteTracksinPad )
    {
        if( m_reporter )
        {
            if( aDryRun )
                m_reporter->Report( _( "Checking tracks in pads..." ) );
            else
                m_reporter->Report( _( "Removing tracks in pads..." ) );

            wxSafeYield();      // Timeslice to update UI
        }

        deleteTracksInPads();
    }

    if( aDeleteUnconnected || aDeleteDanglingVias )
    {
        if( m_reporter )
        {
            if( aDryRun )
            {
                m_reporter->Report( _( "Checking dangling tracks and vias..." ) );
            }
            else
            {
                if( aDeleteUnconnected )
                    m_reporter->Report( _( "Removing dangling tracks..." ) );

                if( aDeleteDanglingVias )
                    m_reporter->Report( _( "Removing dangling vias..." ) );
            }

            wxSafeYield();      // Timeslice to update UI
        }

        has_deleted = deleteDanglingTracks( aDeleteUnconnected, aDeleteDanglingVias );
    }

    if( has_deleted && aMergeSegments )
    {
        if( m_reporter )
        {
            if( aDryRun )
                m_reporter->Report( _( "Checking collinear tracks..." ) );
            else
                m_reporter->Report( _( "Merging collinear tracks..." ) );

            wxSafeYield();      // Timeslice to update UI
        }

        cleanup( false, false, false, true );
    }
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

    const std::list<CN_ITEM*>& items =
                m_brd->GetConnectivity()->GetConnectivityAlgo()->ItemEntry( aTrack ).GetItems();

    if( items.empty() )
        return false;

    CN_ITEM* citem = items.front();

    if( !citem->Valid() )
        return false;

    const std::vector<std::shared_ptr<CN_ANCHOR>>& anchors = citem->Anchors();

    VECTOR2I refpoint = aTstStart ? aTrack->GetStart() : aTrack->GetEnd();

    for( const std::shared_ptr<CN_ANCHOR>& anchor : anchors )
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
        // Ensure the connectivity is up to date, especially after removing a dangling segment
        m_brd->BuildConnectivity();

        // Keep a duplicate deque to all deleting in the primary
        std::deque<PCB_TRACK*> temp_tracks( m_brd->Tracks() );

        for( PCB_TRACK* track : temp_tracks )
        {
            if( track->IsLocked() || ( track->GetFlags() & IS_DELETED ) > 0 )
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
                track->SetFlags( IS_DELETED );

                // keep iterating, because a track connected to the deleted track
                // now perhaps is not connected and should be deleted
                item_erased = true;

                if( !m_dryRun )
                {
                    m_brd->Remove( track );
                    m_commit.Removed( track );
                    modified = true;
                }
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
                track->TransformShapeToPolygon( poly, track->GetLayer(), 0, ARC_HIGH_DEF,
                                                ERROR_INSIDE );

                poly.BooleanSubtract( *pad->GetEffectivePolygon(), SHAPE_POLY_SET::PM_FAST );

                if( poly.IsEmpty() )
                {
                    auto item = std::make_shared<CLEANUP_ITEM>( CLEANUP_TRACK_IN_PAD );
                    item->SetItems( track );
                    m_itemsList->push_back( item );

                    toRemove.insert( track );
                    track->SetFlags( IS_DELETED );
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

        if( aDeleteDuplicateSegments && track->Type() == PCB_TRACE_T && !track->IsNull() )
        {
            rtree.QueryColliding( track, track->GetLayer(), track->GetLayer(),
                    // Filter:
                    [&]( BOARD_ITEM* aItem ) -> bool
                    {
                        return aItem->Type() == PCB_TRACE_T
                                  && !aItem->HasFlag( SKIP_STRUCT )
                                  && !aItem->HasFlag( IS_DELETED )
                                  && !static_cast<PCB_TRACK*>( aItem )->IsNull();
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

    auto mergeSegments =
            [&]( std::shared_ptr<CN_CONNECTIVITY_ALGO> connectivity ) -> bool
            {
                for( PCB_TRACK* segment : m_brd->Tracks() )
                {
                    // one can merge only collinear segments, not vias or arcs.
                    if( segment->Type() != PCB_TRACE_T )
                        continue;

                    if( segment->HasFlag( IS_DELETED ) )  // already taken into account
                        continue;

                    // for each end of the segment:
                    for( CN_ITEM* citem : connectivity->ItemEntry( segment ).GetItems() )
                    {
                        // Do not merge an end which has different width tracks attached -- it's a
                        // common use-case for necking-down a track between pads.
                        std::vector<PCB_TRACK*> sameWidthCandidates;
                        std::vector<PCB_TRACK*> differentWidthCandidates;

                        for( CN_ITEM* connected : citem->ConnectedItems() )
                        {
                            if( !connected->Valid() )
                                continue;

                            BOARD_CONNECTED_ITEM* candidate = connected->Parent();

                            if( candidate->Type() == PCB_TRACE_T
                                    && !candidate->HasFlag( IS_DELETED ) )
                            {
                                PCB_TRACK* candidateSegment = static_cast<PCB_TRACK*>( candidate );

                                if( candidateSegment->GetWidth() == segment->GetWidth() )
                                {
                                    sameWidthCandidates.push_back( candidateSegment );
                                }
                                else
                                {
                                    differentWidthCandidates.push_back( candidateSegment );
                                    break;
                                }
                            }
                        }

                        if( !differentWidthCandidates.empty() )
                            continue;

                        for( PCB_TRACK* candidate : sameWidthCandidates )
                        {
                            if( segment->ApproxCollinear( *candidate )
                                    && mergeCollinearSegments( segment, candidate ) )
                            {
                                return true;
                            }
                        }
                    }
                }

                return false;
            };

    if( aMergeSegments )
    {
        do
        {
            while( !m_brd->BuildConnectivity() )
                wxSafeYield();

            m_connectedItemsCache.clear();
        } while( mergeSegments( m_brd->GetConnectivity()->GetConnectivityAlgo() ) );
    }

    for( PCB_TRACK* track : m_brd->Tracks() )
        track->ClearFlags( IS_DELETED | SKIP_STRUCT );
}


const std::vector<BOARD_CONNECTED_ITEM*>& TRACKS_CLEANER::getConnectedItems( PCB_TRACK* aTrack )
{
    const std::shared_ptr<CONNECTIVITY_DATA>& connectivity = m_brd->GetConnectivity();

    if( m_connectedItemsCache.count( aTrack ) == 0 )
    {
        m_connectedItemsCache[ aTrack ] =
                connectivity->GetConnectedItems( aTrack, { PCB_TRACE_T, PCB_ARC_T, PCB_VIA_T,
                                                           PCB_PAD_T, PCB_ZONE_T } );
    }

    return m_connectedItemsCache[ aTrack ];
}


bool TRACKS_CLEANER::mergeCollinearSegments( PCB_TRACK* aSeg1, PCB_TRACK* aSeg2 )
{
    if( aSeg1->IsLocked() || aSeg2->IsLocked() )
        return false;

    // Collect the unique points where the two tracks are connected to other items
    std::set<VECTOR2I> pts;

    auto collectPts =
            [&]( BOARD_CONNECTED_ITEM* citem )
            {
                if( citem->Type() == PCB_TRACE_T || citem->Type() == PCB_ARC_T
                        || citem->Type() == PCB_VIA_T )
                {
                    PCB_TRACK* track = static_cast<PCB_TRACK*>( citem );

                    if( track->IsPointOnEnds( aSeg1->GetStart() ) )
                        pts.emplace( aSeg1->GetStart() );

                    if( track->IsPointOnEnds( aSeg1->GetEnd() ) )
                        pts.emplace( aSeg1->GetEnd() );

                    if( track->IsPointOnEnds( aSeg2->GetStart() ) )
                        pts.emplace( aSeg2->GetStart() );

                    if( track->IsPointOnEnds( aSeg2->GetEnd() ) )
                        pts.emplace( aSeg2->GetEnd() );
                }
                else
                {
                    if( citem->HitTest( aSeg1->GetStart(), ( aSeg1->GetWidth() + 1 ) / 2 ) )
                        pts.emplace( aSeg1->GetStart() );

                    if( citem->HitTest( aSeg1->GetEnd(), ( aSeg1->GetWidth() + 1 ) / 2  ) )
                        pts.emplace( aSeg1->GetEnd() );

                    if( citem->HitTest( aSeg2->GetStart(), ( aSeg2->GetWidth() + 1 ) / 2  ) )
                        pts.emplace( aSeg2->GetStart() );

                    if( citem->HitTest( aSeg2->GetEnd(), ( aSeg2->GetWidth() + 1 ) / 2  ) )
                        pts.emplace( aSeg2->GetEnd() );
                }
            };

    for( BOARD_CONNECTED_ITEM* item : getConnectedItems( aSeg1 ) )
    {
        if( item != aSeg1 && item != aSeg2 )
            collectPts( item );
    }

    for( BOARD_CONNECTED_ITEM* item : getConnectedItems( aSeg2 ) )
    {
        if( item != aSeg1 && item != aSeg2 )
            collectPts( item );
    }

    // This means there is a node in the center
    if( pts.size() > 2 )
        return false;

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
        dummy_seg.SetStart( VECTOR2I( min_x, min_y ) );
        dummy_seg.SetEnd( VECTOR2I( max_x, max_y ) );
    }
    else
    {
        dummy_seg.SetStart( VECTOR2I( min_x, max_y ) );
        dummy_seg.SetEnd( VECTOR2I( max_x, min_y ) );
    }

    // If the existing connected points are not the same as the points generated by our
    // min/max alg, then assign the missing points to the end closest.  This ensures that
    // our replacement track is still connected
    for( auto& pt : pts )
    {
        if( !dummy_seg.IsPointOnEnds( VECTOR2I( pt.x, pt.y ) ) )
        {
            if( ( VECTOR2I( dummy_seg.GetStart() ) - pt ).SquaredEuclideanNorm() <
                    ( VECTOR2I( dummy_seg.GetEnd() ) - pt ).SquaredEuclideanNorm() )
                dummy_seg.SetStart( VECTOR2I( pt.x, pt.y ) );
            else
                dummy_seg.SetEnd( VECTOR2I( pt.x, pt.y ) );
        }
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

        m_brd->GetConnectivity()->Update( aSeg1 );

        // Merge successful, seg2 has to go away
        m_brd->Remove( aSeg2 );
        m_commit.Removed( aSeg2 );
    }

    return true;
}


void TRACKS_CLEANER::removeItems( std::set<BOARD_ITEM*>& aItems )
{
    for( BOARD_ITEM* item : aItems )
    {
        m_brd->Remove( item );
        m_commit.Removed( item );
    }
}
