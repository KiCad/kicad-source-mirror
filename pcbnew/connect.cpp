/**
 * @file connect.cpp
 * @brief Functions to handle existing tracks in ratsnest calculations.
 */

/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2012 Jean-Pierre Charras, jean-pierre.charras@ujf-grenoble.fr
 * Copyright (C) 2012 SoftPLC Corporation, Dick Hollenbeck <dick@softplc.com>
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

#include <fctsys.h>
#include <common.h>
#include <pcbcommon.h>
#include <macros.h>
#include <wxBasePcbFrame.h>

#include <pcbnew.h>

// Helper classes to handle connection points
#include <connect.h>

extern void Merge_SubNets_Connected_By_CopperAreas( BOARD* aPcb );
extern void Merge_SubNets_Connected_By_CopperAreas( BOARD* aPcb, int aNetcode );

// Local functions
static void RebuildTrackChain( BOARD* pcb );


CONNECTIONS::CONNECTIONS( BOARD * aBrd )
{
    m_brd = aBrd;
}


/* Fills m_sortedPads with all pads that be connected to tracks
 * pads are sorted by X coordinate ( and Y coordinates for same X value )
 * aNetcode = net code to filter pads or < 0 to put all pads in list
 */
void CONNECTIONS::BuildPadsList( int aNetcode )
{
    // Creates sorted pad list if not exists
    m_sortedPads.clear();
    m_brd->GetSortedPadListByXthenYCoord( m_sortedPads, aNetcode < 0 ? -1 : aNetcode );
}

/* Explores the list of pads and adds to m_PadsConnected member
 * of each pad pads connected to
 * Here, connections are due to intersecting pads, not tracks
 */
void CONNECTIONS::SearchConnectionsPadsToIntersectingPads()
{
    std::vector<CONNECTED_POINT*> candidates;

    BuildPadsCandidatesList();

    for( unsigned ii = 0; ii < m_sortedPads.size(); ii++ )
    {
        D_PAD* pad = m_sortedPads[ii];

        pad->m_PadsConnected.clear();
        candidates.clear();

        CollectItemsNearTo( candidates, pad->ShapePos(), pad->GetBoundingRadius() );

        // add pads to pad.m_PadsConnected, if they are connected
        for( unsigned jj = 0; jj < candidates.size(); jj++ )
        {
            CONNECTED_POINT* item = candidates[jj];

            D_PAD* candidate_pad = item->GetPad();

            if( pad == candidate_pad )
                continue;

            if( !( pad->GetLayerSet() & candidate_pad->GetLayerSet() ).any() )
                continue;
            if( pad->HitTest( item->GetPoint() ) )
            {
                pad->m_PadsConnected.push_back( candidate_pad );
            }
        }
    }
}

/* Explores the list of pads
 * Adds to m_PadsConnected member of each track the pad(s) connected to
 * Adds to m_TracksConnected member of each pad the track(s) connected to
 * D_PAD::m_TracksConnected is cleared before adding items
 * TRACK::m_PadsConnected is not cleared
 */
void CONNECTIONS::SearchTracksConnectedToPads( bool add_to_padlist, bool add_to_tracklist)
{
    std::vector<CONNECTED_POINT*> candidates;

    for( unsigned ii = 0; ii < m_sortedPads.size(); ii++ )
    {
        D_PAD * pad = m_sortedPads[ii];
        pad->m_TracksConnected.clear();
        candidates.clear();

        CollectItemsNearTo( candidates, pad->GetPosition(), pad->GetBoundingRadius() );

        // add this pad to track.m_PadsConnected, if it is connected
        for( unsigned jj = 0; jj < candidates.size(); jj++ )
        {
            CONNECTED_POINT* cp_item = candidates[jj];

            if( !( pad->GetLayerSet() & cp_item->GetTrack()->GetLayerSet() ).any() )
                continue;

            if( pad->HitTest( cp_item->GetPoint() ) )
            {
                if( add_to_padlist )
                    cp_item->GetTrack()->m_PadsConnected.push_back( pad );

                if( add_to_tracklist )
                    pad->m_TracksConnected.push_back( cp_item->GetTrack() );
            }
        }
    }
}

void CONNECTIONS::CollectItemsNearTo( std::vector<CONNECTED_POINT*>& aList,
                                       const wxPoint& aPosition, int aDistMax )
{
    /* Search items in m_Candidates that position is <= aDistMax from aPosition
     * (Rectilinear distance)
     * m_Candidates is sorted by X then Y values, so a fast binary search is used
     * to locate the "best" entry point in list
     * The best entry is a pad having its m_Pos.x == (or near) aPosition.x
     * All candidates are near this candidate in list
     * So from this entry point, a linear search is made to find all candidates
     */
    int idxmax = m_candidates.size()-1;

    int delta = m_candidates.size();

    int idx = 0;        // Starting index is the beginning of list
    while( delta )
    {
        // Calculate half size of remaining interval to test.
        // Ensure the computed value is not truncated (too small)
        if( (delta & 1) && ( delta > 1 ) )
            delta++;
        delta /= 2;

        CONNECTED_POINT& item = m_candidates[idx];

        int dist = item.GetPoint().x - aPosition.x;
        if( abs(dist) <= aDistMax )
        {
            break;   // A good entry point is found. The list can be scanned from this point.
        }

        else if( item.GetPoint().x < aPosition.x ) // We should search after this item
        {
            idx += delta;
            if( idx > idxmax )
                idx = idxmax;
        }
        else    // We should search before this item
        {
            idx -= delta;
            if( idx < 0 )
                idx = 0;
        }
    }

    /* Now explore the candidate list from the "best" entry point found
     * (candidate "near" aPosition.x)
     * We explore the list until abs(candidate->m_Point.x - aPosition.x) > aDistMax
     * because the list is sorted by X position (and for a given X pos, by Y pos)
     * Currently a linear search is made because the number of candidates
     * having the right X position is usually small
     */
    // search next candidates in list
    wxPoint diff;
    for( int ii = idx; ii <= idxmax; ii++ )
    {
        CONNECTED_POINT* item = &m_candidates[ii];
        diff = item->GetPoint() - aPosition;
        if( abs(diff.x) > aDistMax )
            break;    // Exit: the distance is to long, we cannot find other candidates
        if( abs(diff.y) > aDistMax )
            continue;    // the y distance is to long, but we can find other candidates
        // We have here a good candidate: add it
        aList.push_back( item );
    }
    // search previous candidates in list
    for(  int ii = idx-1; ii >=0; ii-- )
    {
        CONNECTED_POINT * item = &m_candidates[ii];
        diff = item->GetPoint() - aPosition;
        if( abs(diff.x) > aDistMax )
            break;
        if( abs(diff.y) > aDistMax )
            continue;
        // We have here a good candidate:add it
        aList.push_back( item );
    }
}


void CONNECTIONS::BuildPadsCandidatesList()
{
    m_candidates.clear();
    m_candidates.reserve( m_sortedPads.size() );
    for( unsigned ii = 0; ii < m_sortedPads.size(); ii++ )
    {
        D_PAD * pad = m_sortedPads[ii];
        CONNECTED_POINT candidate( pad, pad->GetPosition() );
        m_candidates.push_back( candidate );
    }
}

/* sort function used to sort .m_Connected by X the Y values
 * items are sorted by X coordinate value,
 * and for same X value, by Y coordinate value.
 */
static bool sortConnectedPointByXthenYCoordinates( const CONNECTED_POINT & aRef,
                                                   const CONNECTED_POINT & aTst )
{
    if( aRef.GetPoint().x == aTst.GetPoint().x )
        return aRef.GetPoint().y < aTst.GetPoint().y;
    return aRef.GetPoint().x < aTst.GetPoint().x;
}

void CONNECTIONS::BuildTracksCandidatesList( TRACK * aBegin, TRACK * aEnd)
{
    m_candidates.clear();
    m_firstTrack = m_lastTrack = aBegin;

    unsigned ii = 0;
    // Count candidates ( i.e. end points )
    for( const TRACK* track = aBegin; track; track = track->Next() )
    {
        if( track->Type() == PCB_VIA_T )
            ii++;
        else
            ii += 2;

        m_lastTrack = track;
        if( track == aEnd )
            break;
    }
    // Build candidate list
    m_candidates.reserve( ii );
    for( TRACK* track = aBegin; track; track = track->Next() )
    {
        CONNECTED_POINT candidate( track, track->GetStart());
        m_candidates.push_back( candidate );
        if( track->Type() != PCB_VIA_T )
        {
            CONNECTED_POINT candidate2( track, track->GetEnd());
            m_candidates.push_back( candidate2 );
        }

        if( track == aEnd )
            break;
    }

    // Sort list by increasing X coordinate,
    // and for increasing Y coordinate when items have the same X coordinate
    // So candidates to the same location are consecutive in list.
    sort( m_candidates.begin(), m_candidates.end(), sortConnectedPointByXthenYCoordinates );
}

/* Populates .m_connected with tracks/vias connected to aTrack
 * param aTrack = track or via to use as reference
 * For calculation time reason, an exhaustive search cannot be made
 * and a proximity search is made:
 * Only tracks with one end near one end of aTrack are collected.
 * near means dist <= aTrack width / 2
 * because with this constraint we can make a fast search in track list
 * m_candidates is expected to be populated by the track candidates ends list
 */
int CONNECTIONS::SearchConnectedTracks( const TRACK * aTrack )
{
    int count = 0;
    m_connected.clear();

    LSET layerMask = aTrack->GetLayerSet();

    // Search for connections to starting point:
#define USE_EXTENDED_SEARCH

#ifdef USE_EXTENDED_SEARCH
    int dist_max = aTrack->GetWidth() / 2;
    static std::vector<CONNECTED_POINT*> tracks_candidates;
#endif
    wxPoint position = aTrack->GetStart();
    for( int kk = 0; kk < 2; kk++ )
    {
#ifndef USE_EXTENDED_SEARCH
        int idx = searchEntryPointInCandidatesList( position );

        if( idx >= 0 )
        {
            // search after:
            for( unsigned ii = idx; ii < m_candidates.size(); ii ++ )
            {
                if( m_candidates[ii].GetTrack() == aTrack )
                    continue;
                if( m_candidates[ii].GetPoint() != position )
                    break;
                if( ( m_candidates[ii].GetTrack()->GetLayerSet() & layerMask ).any() )
                    m_connected.push_back( m_candidates[ii].GetTrack() );
            }

            // search before:
            for( int ii = idx-1; ii >= 0; ii -- )
            {
                if( m_candidates[ii].GetTrack() == aTrack )
                    continue;
                if( m_candidates[ii].GetPoint() != position )
                    break;
                if( ( m_candidates[ii].GetTrack()->GetLayerSet() & layerMask ).any() )
                    m_connected.push_back( m_candidates[ii].GetTrack() );
            }
        }
#else
        tracks_candidates.clear();
        CollectItemsNearTo( tracks_candidates, position, dist_max );
        for ( unsigned ii = 0; ii < tracks_candidates.size(); ii ++ )
        {
            TRACK * ctrack = tracks_candidates[ii]->GetTrack();

            if( !( ctrack->GetLayerSet() & layerMask ).any() )
                continue;

            if( ctrack == aTrack )
                continue;

            // We have a good candidate: calculate the actual distance
            // between ends, which should be <= dist max.
            wxPoint delta = tracks_candidates[ii]->GetPoint() - position;
            int dist = KiROUND( EuclideanNorm( delta ) );

            if( dist > dist_max )
                continue;

            m_connected.push_back( ctrack );
        }
#endif

        // Search for connections to ending point:
        if( aTrack->Type() == PCB_VIA_T )
            break;

        position = aTrack->GetEnd();
    }

    return count;
}


int CONNECTIONS::searchEntryPointInCandidatesList( const wxPoint& aPoint )
{
    // Search the aPoint coordinates in m_Candidates
    // m_Candidates is sorted by X then Y values, and a fast binary search is used
    int idxmax = m_candidates.size()-1;

    int delta = m_candidates.size();

    int idx = 0;        // Starting index is the beginning of list

    while( delta )
    {
        // Calculate half size of remaining interval to test.
        // Ensure the computed value is not truncated (too small)
        if( ( delta & 1 ) && ( delta > 1 ) )
            delta++;

        delta /= 2;

        CONNECTED_POINT& candidate = m_candidates[idx];

        if( candidate.GetPoint() == aPoint )   // candidate found
        {
            return idx;
        }

        // Not found: test the middle of the remaining sub list
        if( candidate.GetPoint().x == aPoint.x )   // Must search considering Y coordinate
        {
            if(candidate.GetPoint().y < aPoint.y)  // Must search after this item
            {
                idx += delta;
                if( idx > idxmax )
                    idx = idxmax;
            }
            else // Must search before this item
            {
                idx -= delta;
                if( idx < 0 )
                    idx = 0;
            }
        }
        else if( candidate.GetPoint().x < aPoint.x ) // Must search after this item
        {
            idx += delta;
            if( idx > idxmax )
                idx = idxmax;
        }
        else // Must search before this item
        {
            idx -= delta;
            if( idx < 0 )
                idx = 0;
        }
    }

    return -1;
}

/* Used after a track change (delete a track ou add a track)
 * Connections to pads are recalculated
 * Note also aFirstTrack (and aLastTrack ) can be NULL
 */
void CONNECTIONS::Build_CurrNet_SubNets_Connections( TRACK* aFirstTrack, TRACK* aLastTrack, int aNetcode )
{
    m_firstTrack = aFirstTrack;     // The first track used to build m_Candidates
    m_lastTrack = aLastTrack;       // The last track used to build m_Candidates

    // Pads subnets are expected already cleared, because this function
    // does not know the full list of pads
    BuildTracksCandidatesList( aFirstTrack, aLastTrack );
    TRACK* curr_track;
    for( curr_track = aFirstTrack; curr_track != NULL; curr_track = curr_track->Next() )
    {
        // Clear track subnet id (Pads subnets are cleared outside this function)
        curr_track->SetSubNet( 0 );
        curr_track->m_TracksConnected.clear();
        curr_track->m_PadsConnected.clear();

        // Update connections between tracks:
        SearchConnectedTracks( curr_track );
        curr_track->m_TracksConnected = m_connected;

        if( curr_track == aLastTrack )
            break;
    }

    // Update connections between tracks and pads
    BuildPadsList( aNetcode );
    SearchTracksConnectedToPads();

    // Update connections between intersecting pads (no tracks)
    SearchConnectionsPadsToIntersectingPads();

    // Creates sub nets (clusters) for the current net:
    Propagate_SubNets();
}


/**
 * Change a subnet value to a new value, in m_sortedPads pad list
 * After that, 2 cluster (or subnets) are merged into only one.
 * Note: the resulting subnet value is the smallest between aOldSubNet et aNewSubNet
 */
int CONNECTIONS::Merge_PadsSubNets( int aOldSubNet, int aNewSubNet )
{
    int    change_count = 0;

    if( aOldSubNet == aNewSubNet )
        return 0;

    if( (aOldSubNet > 0) && (aOldSubNet < aNewSubNet) )
        EXCHG( aOldSubNet, aNewSubNet );

    // Examine connections between intersecting pads
    for( unsigned ii = 0; ii < m_sortedPads.size(); ii++ )
    {
        D_PAD * curr_pad = m_sortedPads[ii];
        if( curr_pad->GetSubNet() != aOldSubNet )
            continue;

        change_count++;
        curr_pad->SetSubNet( aNewSubNet );
    }

    return change_count;
}

/*
 * Change a subnet value to a new value, for tracks and pads which are connected to.
 * The result is merging 2 clusters (or subnets) into only one cluster.
 * Note: the resulting sub net value is the smallest between aOldSubNet et aNewSubNet
 */
int CONNECTIONS::Merge_SubNets( int aOldSubNet, int aNewSubNet )
{
    TRACK* curr_track;
    int    change_count = 0;

    if( aOldSubNet == aNewSubNet )
        return 0;

    if( (aOldSubNet > 0) && (aOldSubNet < aNewSubNet) )
        EXCHG( aOldSubNet, aNewSubNet );

    curr_track = (TRACK*)m_firstTrack;

    for( ; curr_track != NULL; curr_track = curr_track->Next() )
    {
        if( curr_track->GetSubNet() != aOldSubNet )
        {
            if( curr_track == m_lastTrack )
                break;

            continue;
        }

        change_count++;
        curr_track->SetSubNet( aNewSubNet );

        for( unsigned ii = 0; ii < curr_track->m_PadsConnected.size(); ii++ )
        {
            D_PAD * pad = curr_track->m_PadsConnected[ii];
            if( pad->GetSubNet() == aOldSubNet )
            {
                pad->SetSubNet( curr_track->GetSubNet() );
            }
        }

        if( curr_track == m_lastTrack )
            break;
    }

    return change_count;
}


/* Test a list of track segments, to create or propagate a sub netcode to pads and
 * segments connected together.
 * The track list must be sorted by nets, and all segments
 * from m_firstTrack to m_lastTrack have the same net
 * When 2 items are connected (a track to a pad, or a track to an other track),
 * they are grouped in a cluster.
 * The .m_Subnet member is the cluster identifier (subnet id)
 * For a given net, if all tracks are created, there is only one cluster.
 * but if not all tracks are created, there are more than one cluster,
 * and some ratsnests will be left active.
 * A ratsnest is active when it "connect" 2 items having different subnet id
 */
void CONNECTIONS::Propagate_SubNets()
{
    int sub_netcode = 1;

    TRACK* curr_track = (TRACK*)m_firstTrack;
    if( curr_track )
        curr_track->SetSubNet( sub_netcode );

    // Examine connections between tracks and pads
    for( ; curr_track != NULL; curr_track = curr_track->Next() )
    {
        // First: handling connections to pads
        for( unsigned ii = 0; ii < curr_track->m_PadsConnected.size(); ii++ )
        {
            D_PAD * pad = curr_track->m_PadsConnected[ii];

            if( curr_track->GetSubNet() )        // the track segment is already a cluster member
            {
                if( pad->GetSubNet() > 0 )
                {
                    // The pad is already a cluster member, so we can merge the 2 clusters
                    Merge_SubNets( pad->GetSubNet(), curr_track->GetSubNet() );
                }
                else
                {
                    /* The pad is not yet attached to a cluster , so we can add this pad to
                     * the cluster */
                    pad->SetSubNet( curr_track->GetSubNet() );
                }
            }
            else                              // the track segment is not attached to a cluster
            {
                if( pad->GetSubNet() > 0 )
                {
                    // it is connected to a pad in a cluster, merge this track
                    curr_track->SetSubNet( pad->GetSubNet() );
                }
                else
                {
                    /* it is connected to a pad not in a cluster, so we must create a new
                     * cluster (only with the 2 items: the track and the pad) */
                    sub_netcode++;
                    curr_track->SetSubNet( sub_netcode );
                    pad->SetSubNet( curr_track->GetSubNet() );
                }
            }
        }

        // Test connections between segments
        for( unsigned ii = 0; ii < curr_track->m_TracksConnected.size(); ii++ )
        {
            BOARD_CONNECTED_ITEM* track = curr_track->m_TracksConnected[ii];

            if( curr_track->GetSubNet() )   // The current track is already a cluster member
            {
                // The other track is already a cluster member, so we can merge the 2 clusters
                if( track->GetSubNet() )
                {
                    Merge_SubNets( track->GetSubNet(), curr_track->GetSubNet() );
                }
                else
                {
                    // The other track is not yet attached to a cluster , so we can add this
                    // other track to the cluster
                    track->SetSubNet( curr_track->GetSubNet() );
                }
            }
            else        // the current track segment is not yet attached to a cluster
            {
                if( track->GetSubNet() )
                {
                    // The other track is already a cluster member, so we can add
                    // the current segment to the cluster
                    curr_track->SetSubNet( track->GetSubNet() );
                }
                else
                {
                    // it is connected to an other segment not in a cluster, so we must
                    // create a new cluster (only with the 2 track segments)
                    sub_netcode++;
                    curr_track->SetSubNet( sub_netcode );
                    track->SetSubNet( curr_track->GetSubNet() );
                }
            }
        }

        if( curr_track == m_lastTrack )
            break;
    }

    // Examine connections between intersecting pads, and propagate
    // sub_netcodes to intersecting pads
    for( unsigned ii = 0; ii < m_sortedPads.size(); ii++ )
    {
        D_PAD* curr_pad = m_sortedPads[ii];

        for( unsigned jj = 0; jj < curr_pad->m_PadsConnected.size(); jj++ )
        {
            D_PAD* pad = curr_pad->m_PadsConnected[jj];

            if( curr_pad->GetSubNet() )   // the current pad is already attached to a cluster
            {
                if( pad->GetSubNet() > 0 )
                {
                    // The pad is already a cluster member, so we can merge the 2 clusters
                    // Store the initial subnets, which will be modified by Merge_PadsSubNets
                    int subnet1 = pad->GetSubNet();
                    int subnet2 = curr_pad->GetSubNet();

                    // merge subnets of pads only, even those not connected by tracks
                    Merge_PadsSubNets( subnet1, subnet2 );

                    // merge subnets of tracks (and pads, which are already merged)
                    Merge_SubNets( subnet1, subnet2 );
                }
                else
                {
                    // The pad is not yet attached to a cluster,
                    // so we can add this pad to the cluster
                    pad->SetSubNet( curr_pad->GetSubNet() );
                }
            }
            else   // the current pad is not attached to a cluster
            {
                if( pad->GetSubNet() > 0 )
                {
                    // the connected pad is in a cluster,
                    // so we can add the current pad to the cluster
                    curr_pad->SetSubNet( pad->GetSubNet() );
                }
                else
                {
                    // the connected pad is not in a cluster,
                    // so we must create a new cluster, with the 2 pads.
                    sub_netcode++;
                    curr_pad->SetSubNet( sub_netcode );
                    pad->SetSubNet( curr_pad->GetSubNet() );
                }
            }
        }
    }
}

/*
 * Test all connections of the board,
 * and update subnet variable of pads and tracks
 * TestForActiveLinksInRatsnest must be called after this function
 * to update active/inactive ratsnest items status
 */
void PCB_BASE_FRAME::TestConnections()
{
    // Clear the cluster identifier for all pads
    for( unsigned i = 0;  i< m_Pcb->GetPadCount();  ++i )
    {
        D_PAD* pad = m_Pcb->GetPad(i);

        pad->SetZoneSubNet( 0 );
        pad->SetSubNet( 0 );
    }

    m_Pcb->Test_Connections_To_Copper_Areas();

    // Test existing connections net by net
    // note some nets can have no tracks, and pads intersecting
    // so Build_CurrNet_SubNets_Connections must be called for each net
    CONNECTIONS connections( m_Pcb );

    int last_net_tested = 0;
    int current_net_code = 0;

    for( TRACK* track = m_Pcb->m_Track; track; )
    {
        // At this point, track is the first track of a given net
        current_net_code = track->GetNetCode();

        // Get last track of the current net
        TRACK* lastTrack = track->GetEndNetCode( current_net_code );

        if( current_net_code > 0 )  // do not spend time if net code = 0 ( dummy net )
        {
            // Test all previous nets having no tracks
            for( int net = last_net_tested+1; net < current_net_code; net++ )
                connections.Build_CurrNet_SubNets_Connections( NULL, NULL, net );

            connections.Build_CurrNet_SubNets_Connections( track, lastTrack, current_net_code );
            last_net_tested = current_net_code;
        }

        track = lastTrack->Next();    // this is now the first track of the next net
    }

    // Test last nets without tracks, if any
    int netsCount = m_Pcb->GetNetCount();
    for( int net = last_net_tested+1; net < netsCount; net++ )
        connections.Build_CurrNet_SubNets_Connections( NULL, NULL, net );

    Merge_SubNets_Connected_By_CopperAreas( m_Pcb );

    return;
}


void PCB_BASE_FRAME::TestNetConnection( wxDC* aDC, int aNetCode )
{
    wxString msg;

    if( aNetCode <= 0 ) // -1 = not existing net, 0 = dummy net
        return;

    if( (m_Pcb->m_Status_Pcb & LISTE_RATSNEST_ITEM_OK) == 0 )
        Compile_Ratsnest( aDC, true );

    // Clear the cluster identifier (subnet) of pads for this net
    for( unsigned i = 0; i < m_Pcb->GetPadCount(); ++i )
    {
        D_PAD* pad = m_Pcb->GetPad(i);
        int    pad_net_code = pad->GetNetCode();

        if( pad_net_code < aNetCode )
            continue;

        if( pad_net_code > aNetCode )
            break;

        pad->SetSubNet( 0 );
    }

    m_Pcb->Test_Connections_To_Copper_Areas( aNetCode );

    // Search for the first and the last segment relative to the given net code
    if( m_Pcb->m_Track )
    {
        CONNECTIONS connections( m_Pcb );

        TRACK* firstTrack;
        TRACK* lastTrack = NULL;
        firstTrack = m_Pcb->m_Track.GetFirst()->GetStartNetCode( aNetCode );

        if( firstTrack )
            lastTrack = firstTrack->GetEndNetCode( aNetCode );

        if( firstTrack && lastTrack ) // i.e. if there are segments
        {
            connections.Build_CurrNet_SubNets_Connections( firstTrack, lastTrack, firstTrack->GetNetCode() );
        }
    }

    Merge_SubNets_Connected_By_CopperAreas( m_Pcb, aNetCode );

    // rebuild the active ratsnest for this net
    DrawGeneralRatsnest( aDC, aNetCode );
    TestForActiveLinksInRatsnest( aNetCode );
    DrawGeneralRatsnest( aDC, aNetCode );

    // Display results
    int net_notconnected_count = 0;
    NETINFO_ITEM* net = m_Pcb->FindNet( aNetCode );

    if( net )       // Should not occur, but ...
    {
        for( unsigned ii = net->m_RatsnestStartIdx; ii < net->m_RatsnestEndIdx; ii++ )
        {
            if( m_Pcb->m_FullRatsnest[ii].IsActive() )
                net_notconnected_count++;
        }

        msg.Printf( wxT( "links %d nc %d  net:nc %d" ),
                    m_Pcb->GetRatsnestsCount(), m_Pcb->GetUnconnectedNetCount(),
                    net_notconnected_count );
    }
    else
        msg.Printf( wxT( "net not found: netcode %d" ),aNetCode );

    SetStatusText( msg );
    return;
}


/* search connections between tracks and pads and propagate pad net codes to the track
 * segments.
 * Pads netcodes are assumed to be up to date.
 */
void PCB_BASE_FRAME::RecalculateAllTracksNetcode()
{
    TRACK*              curr_track;

    // Build the net info list
    GetBoard()->BuildListOfNets();

    // Reset variables and flags used in computation
    curr_track = m_Pcb->m_Track;
    for( ; curr_track != NULL; curr_track = curr_track->Next() )
    {
        curr_track->m_TracksConnected.clear();
        curr_track->m_PadsConnected.clear();
        curr_track->start = NULL;
        curr_track->end = NULL;
        curr_track->SetState( BUSY | IN_EDIT | BEGIN_ONPAD | END_ONPAD, false );
        curr_track->SetZoneSubNet( 0 );
        curr_track->SetNetCode( NETINFO_LIST::UNCONNECTED );
    }

    // If no pad, reset pointers and netcode, and do nothing else
    if( m_Pcb->GetPadCount() == 0 )
        return;

    CONNECTIONS connections( m_Pcb );
    connections.BuildPadsList();
    connections.BuildTracksCandidatesList(m_Pcb->m_Track);

    // First pass: build connections between track segments and pads.
    connections.SearchTracksConnectedToPads();

    /* For tracks connected to at least one pad,
     * set the track net code to the pad netcode
     */
    curr_track = m_Pcb->m_Track;
    for( ; curr_track != NULL; curr_track = curr_track->Next() )
    {
        if( curr_track->m_PadsConnected.size() )
            curr_track->SetNetCode( curr_track->m_PadsConnected[0]->GetNetCode() );
    }

    // Pass 2: build connections between track ends
    for( curr_track = m_Pcb->m_Track; curr_track != NULL; curr_track = curr_track->Next() )
    {
        connections.SearchConnectedTracks( curr_track );
        connections.GetConnectedTracks( curr_track );
    }

    // Propagate net codes from a segment to other connected segments
    bool new_pass_request = true;   // set to true if a track has its netcode changed from 0
                                    // to a known netcode to re-evaluate netcodes
                                    // of connected items
    while( new_pass_request )
    {
        new_pass_request = false;

        for( curr_track = m_Pcb->m_Track; curr_track; curr_track = curr_track->Next() )
        {
            int netcode = curr_track->GetNetCode();

            if( netcode == 0 )
            {
                // try to find a connected item having a netcode
                for( unsigned kk = 0; kk < curr_track->m_TracksConnected.size(); kk++ )
                {
                    int altnetcode = curr_track->m_TracksConnected[kk]->GetNetCode();
                    if( altnetcode )
                    {
                        new_pass_request = true;
                        netcode = altnetcode;
                        curr_track->SetNetCode(netcode);
                        break;
                    }
                }
            }

            if( netcode )    // this track has a netcode
            {
                // propagate this netcode to connected tracks having no netcode
                for( unsigned kk = 0; kk < curr_track->m_TracksConnected.size(); kk++ )
                {
                    int altnetcode = curr_track->m_TracksConnected[kk]->GetNetCode();
                    if( altnetcode == 0 )
                    {
                        curr_track->m_TracksConnected[kk]->SetNetCode(netcode);
                        new_pass_request = true;
                    }
                }
            }
        }
    }

    // Sort the track list by net codes:
    RebuildTrackChain( m_Pcb );
}



/*
 * Function SortTracksByNetCode used in RebuildTrackChain()
 * to sort track segments by net code.
 */
static bool SortTracksByNetCode( const TRACK* const & ref, const TRACK* const & compare )
{
    // For items having the same Net, keep the order in list
    if( ref->GetNetCode() == compare->GetNetCode())
        return ref->m_Param < compare->m_Param;

    return ref->GetNetCode() < compare->GetNetCode();
}

/**
 * Helper function RebuildTrackChain
 * rebuilds the track segment linked list in order to have a chain
 * sorted by increasing netcodes.
 * We try to keep order of track segments in list, when possible
 * @param pcb = board to rebuild
 */
static void RebuildTrackChain( BOARD* pcb )
{
    if( pcb->m_Track == NULL )
        return;

    int item_count = pcb->m_Track.GetCount();

    std::vector<TRACK*> trackList;
    trackList.reserve( item_count );

    // Put track list in a temporary list to sort tracks by netcode
    // We try to keep the initial order of track segments in list, when possible
    // so we use m_Param (a member variable used for temporary storage)
    // to temporary keep trace of the order of segments
    // The sort function uses this variable to sort items that
    // have the same net code.
    // Without this, during sorting, the initial order is sometimes lost
    // by the sort algorithm
    for( int ii = 0; ii < item_count; ++ii )
    {
        pcb->m_Track->m_Param = ii;
        trackList.push_back( pcb->m_Track.PopFront() );
    }

    // the list is empty now
    wxASSERT( pcb->m_Track == NULL && pcb->m_Track.GetCount()==0 );

    sort( trackList.begin(), trackList.end(), SortTracksByNetCode );

    // add them back to the list
    for( int i = 0; i < item_count;  ++i )
        pcb->m_Track.PushBack( trackList[i] );
}
