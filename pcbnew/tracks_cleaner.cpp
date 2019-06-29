/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2004-2018 Jean-Pierre Charras, jp.charras at wanadoo.fr
 * Copyright (C) 2011 Wayne Stambaugh <stambaughw@verizon.net>
 * Copyright (C) 1992-2019 KiCad Developers, see AUTHORS.txt for contributors.
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
#include <tools/global_edit_tool.h>
#include <tracks_cleaner.h>


/* Install the cleanup dialog frame to know what should be cleaned
*/
int GLOBAL_EDIT_TOOL::CleanupTracksAndVias( const TOOL_EVENT& aEvent )
{
    PCB_EDIT_FRAME* editFrame = getEditFrame<PCB_EDIT_FRAME>();
    DIALOG_CLEANUP_TRACKS_AND_VIAS dlg( editFrame );

    dlg.ShowModal();
    return 0;
}


TRACKS_CLEANER::TRACKS_CLEANER( EDA_UNITS_T aUnits, BOARD* aPcb, BOARD_COMMIT& aCommit ) :
        m_units( aUnits ),
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
bool TRACKS_CLEANER::CleanupBoard( bool aDryRun, DRC_LIST* aItemsList, bool aRemoveMisConnected,
        bool aCleanVias, bool aMergeSegments, bool aDeleteUnconnected, bool aDeleteTracksinPad )
{
    m_dryRun = aDryRun;
    m_itemsList = aItemsList;
    bool modified = false;

    // Clear the flag used to mark some segments as deleted, in dry run:
    for( auto segment : m_brd->Tracks() )
        segment->ClearFlags( IS_DELETED );

    // delete redundant vias
    if( aCleanVias )
        modified |= cleanupVias();

    // Remove null segments and intermediate points on aligned segments
    // If not asked, remove null segments only if remove misconnected is asked
    if( aMergeSegments )
        modified |= cleanupSegments();
    else if( aRemoveMisConnected )
        modified |= deleteNullSegments( m_brd->Tracks() );

    if( aRemoveMisConnected )
        modified |= removeBadTrackSegments();

    if( aDeleteTracksinPad )
        modified |= deleteTracksInPads();

    // Delete dangling tracks
    if( aDeleteUnconnected )
    {
        if( deleteDanglingTracks() )
        {
            modified = true;

            // Removed tracks can leave aligned segments
            // (when a T was formed by tracks and the "vertical" segment is removed)
            if( aMergeSegments )
                cleanupSegments();
        }
    }

    // Clear the flag used to mark some segments:
    for( auto segment : m_brd->Tracks() )
        segment->ClearFlags( IS_DELETED );

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
                    m_itemsList->emplace_back(
                            new DRC_ITEM( m_units, DRCE_SHORT, segment, segment->GetPosition() ) );
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


bool TRACKS_CLEANER::cleanupVias()
{
    std::set<BOARD_ITEM*> toRemove;
    std::vector<VIA*>     vias;

    for( auto track : m_brd->Tracks() )
    {
        if( auto via = dyn_cast<VIA*>( track ) )
            vias.push_back( via );
    }

    for( auto via1_it = vias.begin(); via1_it != vias.end(); via1_it++ )
    {
        auto via1 = *via1_it;

        if( via1->IsLocked() )
            continue;

        if( via1->GetStart() != via1->GetEnd() )
            via1->SetEnd( via1->GetStart() );

        // To delete through Via on THT pads at same location
        // Examine the list of connected pads:
        // if a through pad is found, the via can be removed

        const auto pads = m_brd->GetConnectivity()->GetConnectedPads( via1 );
        for( const auto pad : pads )
        {
            const LSET all_cu = LSET::AllCuMask();

            if( ( pad->GetLayerSet() & all_cu ) == all_cu )
            {
                if( m_itemsList )
                {
                    m_itemsList->emplace_back( new DRC_ITEM( m_units, DRCE_REDUNDANT_VIA, via1,
                            via1->GetPosition(), pad, pad->GetPosition() ) );
                }

                // redundant: delete the via
                toRemove.insert( via1 );
                break;
            }
        }

        for( auto via2_it = via1_it + 1; via2_it != vias.end(); via2_it++ )
        {
            auto via2 = *via2_it;

            if( via1->GetPosition() != via2->GetPosition() || via2->IsLocked() )
                continue;

            if( via1->GetViaType() == via2->GetViaType() )
            {
                if( m_itemsList )
                {
                    m_itemsList->emplace_back( new DRC_ITEM( m_units, DRCE_REDUNDANT_VIA, via1,
                            via1->GetPosition(), via2, via2->GetPosition() ) );
                }

                toRemove.insert( via2 );
                break;
            }
        }
    }


    return removeItems( toRemove );
}


bool TRACKS_CLEANER::testTrackEndpointDangling( TRACK* aTrack )
{
    auto connectivity = m_brd->GetConnectivity();
    auto items = connectivity->GetConnectivityAlgo()->ItemEntry( aTrack ).GetItems();

    // Not in the connectivity system.  This is a bug!
    if( items.empty() )
    {
        wxASSERT( !items.empty() );
        return false;
    }

    auto citem = items.front();

    if( !citem->Valid() )
        return false;

    auto anchors = citem->Anchors();

    for( const auto& anchor : anchors )
    {
        if( anchor->IsDangling() )
            return true;
    }

    return false;
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

        for( TRACK* track : m_brd->Tracks() )
        {
            bool flag_erase = false; // Start without a good reason to erase it

            // Tst if a track (or a via) endpoint is not connected to another track or to a zone.
            if( testTrackEndpointDangling( track ) )
                flag_erase = true;

            if( flag_erase )
            {
                if( m_itemsList )
                {
                    int code = track->IsTrack() ? DRCE_DANGLING_TRACK : DRCE_DANGLING_VIA;
                    m_itemsList->emplace_back(
                            new DRC_ITEM( m_units, code, track, track->GetPosition() ) );
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
                // Fix me: In dry run we should disable the track to erase and retry with this disabled track
                // However the connectivity algo does not handle disabled items.
            }
        }
    } while( item_erased ); // A segment was erased: test for some new dangling segments

    return modified;
}


// Delete null length track segments
bool TRACKS_CLEANER::deleteNullSegments( TRACKS& aTracks )
{
    std::set<BOARD_ITEM *> toRemove;

    for( auto segment : aTracks )
    {
        if( segment->IsNull() && segment->Type() == PCB_TRACE_T && !segment->IsLocked() )
        {
            if( m_itemsList )
            {
                m_itemsList->emplace_back( new DRC_ITEM(
                        m_units, DRCE_ZERO_LENGTH_TRACK, segment, segment->GetPosition() ) );
            }

            toRemove.insert( segment );
        }
    }

    return removeItems( toRemove );
}


bool TRACKS_CLEANER::deleteTracksInPads()
{
    std::set<BOARD_ITEM*> toRemove;

    // Delete tracks that start and end on the same pad
    auto connectivity = m_brd->GetConnectivity();

    for( auto track : m_brd->Tracks() )
    {
        // Mark track if connected to pads
        for( auto pad : connectivity->GetConnectedPads( track ) )
        {
            if( pad->HitTest( track->GetStart() ) && pad->HitTest( track->GetEnd() ) )
            {
                if( m_itemsList )
                {
                    m_itemsList->emplace_back( new DRC_ITEM(
                            m_units, DRCE_TRACK_IN_PAD, track, track->GetPosition() ) );
                }

                toRemove.insert( track );
            }
        }
    }

    return removeItems( toRemove );
}


// Delete null length segments, and intermediate points ..
bool TRACKS_CLEANER::cleanupSegments()
{
    bool modified = false;

    // Easy things first
    modified |= deleteNullSegments( m_brd->Tracks() );

    std::set<BOARD_ITEM*> toRemove;

    // Remove duplicate segments (2 superimposed identical segments):
    for( auto it = m_brd->Tracks().begin(); it != m_brd->Tracks().end(); it++ )
    {
        auto track1 = *it;

        if( track1->Type() != PCB_TRACE_T || ( track1->GetFlags() & IS_DELETED )
            || track1->IsLocked() )
            continue;

        for( auto it2 = it + 1; it2 != m_brd->Tracks().end(); it2++ )
        {
            auto track2 = *it2;

            if( track2->GetFlags() & IS_DELETED )
                continue;

            if( track1->IsPointOnEnds( track2->GetStart() )
                    && track1->IsPointOnEnds( track2->GetEnd() )
                    && ( track1->GetWidth() == track2->GetWidth() )
                    && ( track1->GetLayer() == track2->GetLayer() ) )
            {
                if( m_itemsList )
                {
                    m_itemsList->emplace_back( new DRC_ITEM( m_units, DRCE_DUPLICATE_TRACK, track2,
                            track2->GetPosition(), nullptr, wxPoint() ) );
                }

                track2->SetFlags( IS_DELETED );
                toRemove.insert( track2 );
            }
        }
    }

    modified |= removeItems( toRemove );

    // merge collinear segments:
    for( auto track_it = m_brd->Tracks().begin(); track_it != m_brd->Tracks().end(); track_it++ )
    {
        auto segment = *track_it;

        if( segment->Type() != PCB_TRACE_T )    // one can merge only track collinear segments, not vias.
            continue;

        if( segment->GetFlags() & IS_DELETED )  // already taken in account
            continue;

        auto connectivity = m_brd->GetConnectivity();

        auto& entry = connectivity->GetConnectivityAlgo()->ItemEntry( segment );

        for( auto citem : entry.GetItems() )
        {
            for( auto connected : citem->ConnectedItems() )
            {
                if( !connected->Valid() || connected->Parent()->Type() != PCB_TRACE_T ||
                    ( connected->Parent()->GetFlags() & IS_DELETED ) )
                    continue;

                if( !( connected->Parent()->GetFlags() & IS_DELETED ) )
                {
                    TRACK* candidate = static_cast<TRACK*>( connected->Parent() );

                    // Do not merge segments having different widths: it is a frequent case
                    // to draw a track between 2 pads:
                    if( candidate->GetWidth() != segment->GetWidth() )
                        continue;

                    if( segment->ApproxCollinear( *candidate ) )
                    {
                        modified |= mergeCollinearSegments(
                                segment, static_cast<TRACK*>( connected->Parent() ) );
                    }
                }
            }
        }
    }

    return modified;
}


bool TRACKS_CLEANER::mergeCollinearSegments( TRACK* aSeg1, TRACK* aSeg2 )
{
    if( aSeg1->IsLocked() || aSeg2->IsLocked() )
        return false;

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
            return false;
    }

    if( aSeg1->GetEnd() != dummy_seg.GetStart() && aSeg1->GetEnd() != dummy_seg.GetEnd() )
    {
        if( testTrackEndpointIsNode( aSeg1, false ) )
            return false;
    }

    if( m_itemsList )
    {
        m_itemsList->emplace_back( new DRC_ITEM( m_units, DRCE_MERGE_TRACKS,
                                                 aSeg1, aSeg1->GetPosition(),
                                                 aSeg2, aSeg2->GetPosition() ) );
    }

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

    return true;
}


bool TRACKS_CLEANER::removeItems( std::set<BOARD_ITEM*>& aItems )
{
    if( m_dryRun )
        return false;

    for( auto item : aItems )
    {
        m_brd->Remove( item );
        m_commit.Removed( item );
    }

    return !aItems.empty();
}
