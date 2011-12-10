/**
 * @file connect.cpp
 * @brief Functions to handle existing tracks in ratsnest calculations.
 */

/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2011 Jean-Pierre Charras, jean-pierre.charras@gipsa-lab.inpg.com
 * Copyright (C) 2004-2011 KiCad Developers, see change_log.txt for contributors.
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

#include "fctsys.h"
#include "common.h"
#include "pcbcommon.h"
#include "macros.h"
#include "wxBasePcbFrame.h"

#include "class_track.h"
#include "class_board.h"

#include "pcbnew.h"


extern void Merge_SubNets_Connected_By_CopperAreas( BOARD* aPcb );
extern void Merge_SubNets_Connected_By_CopperAreas( BOARD* aPcb, int aNetcode );

/* Local functions */
static void RebuildTrackChain( BOARD* pcb );


// A helper class to handle connection points (i.e. candidates) for tracks
class CONNECTED_POINT
{
private:
    TRACK * m_track;      // a link to the parent item (track or via)
    wxPoint m_point;      // the connection point

public:
    CONNECTED_POINT( TRACK * aTrack, wxPoint & aPoint)
    {
        m_track = aTrack;
        m_point = aPoint;
    }

    TRACK * GetTrack() const { return m_track; }
    const wxPoint & GetPoint() const { return m_point; }
};

// A helper class to handle connections calculations:
class CONNECTIONS
{
private:
    std::vector <TRACK*> m_connected;           // List of connected tracks/vias
                                                // to a given track or via
    std::vector <CONNECTED_POINT> m_candidates; // List of points to test
                                                // (end points of tracks or vias location )
    BOARD * m_brd;                              // the master board.
    const TRACK * m_firstTrack;                 // The first track used to build m_Candidates
    const TRACK * m_lastTrack;                  // The last track used to build m_Candidates
    std::vector<D_PAD*> m_sortedPads;           // list of sorted pads by X (then Y) coordinate

public:
    CONNECTIONS( BOARD * aBrd );
    ~CONNECTIONS() {};

    /** Function BuildPadsList
     * Fills m_sortedPads with all pads that be connected to tracks
     * pads are sorted by > then Y coordinates to allow fast binary search in list
     * @param aNetcode = net code to use to filter pads
     * if  aNetcode < 0, all pads will be put in list (default)
     */
    void BuildPadsList( int aNetcode = -1 );

    /**
     * Function Build_CurrNet_SubNets_Connections
     * Connections to pads are assumed to be already initialized,
     * and are not recalculated
     * An be called after a track change (delete or add a track):
     *   If a track is deleted, the other pointers to pads do not change.
     *   When a new track is added in track list, its pointers to pads are already initialized
     * Builds the subnets inside a net (tracks from aFirstTrack to aFirstTrack).
     * subnets are clusters of pads and tracks that are connected together.
     * When all tracks are created relative to the net, there is only a cluster
     * when not tracks there are a cluster per pad
     * @param aFirstTrack = first track of the given net
     * @param aLastTrack = last track of the given net
     */
    void Build_CurrNet_SubNets_Connections( TRACK* aFirstTrack, TRACK* aLastTrack );

    /**
     * Function BuildCandidatesList
     * Fills m_Candidates with all connecting points (track ends or via location)
     * with tracks from aBegin to aEnd.
     * if aBegin == NULL, use first track in brd list
     * if aEnd == NULL, uses all tracks from aBegin in brd list
     */
    void BuildCandidatesList( TRACK * aBegin = NULL, TRACK * aEnd = NULL);

    /**
     * function SearchConnectedTracks
     * Fills m_Connected with tracks/vias connected to aTrack
     * @param aTrack = track or via to use as reference
     */
    int SearchConnectedTracks( const TRACK * aTrack );

    /**
     * Function GetConnectedTracks
     * Copy m_Connected that contains the list of tracks connected
     * calculated by SearchConnectedTracks
     * in aTrack->m_TracksConnected
     * @param aTrack = track or via to fill with connected tracks
     */
    void GetConnectedTracks(TRACK * aTrack)
    {
        aTrack->m_TracksConnected = m_connected;
    }

    /**
     * function SearchConnectedToPads
     * Explores the list of pads and adds to m_PadsConnected member
     * of each track connected the pad(s) connected to
     */
    void SearchConnectedToPads();

    /**
     * function CollectItemsNearTo
     * Used by SearchConnectedToPads
     * Fills aList with pads near to aPosition
     * near means aPosition to pad position <= aDistMax
     * @param aList = list to fill
     * @param aPosition = aPosition to use as reference
     * @param aDistMax = dist max from aPosition to a candidate to select it
     */
    void CollectItemsNearTo( std::vector<CONNECTED_POINT*>& aList,
                            const wxPoint& aPosition, int aDistMax );

    /**
     * Function Propagate_SubNets
     * Test a list of tracks, to create or propagate a sub netcode to pads and
     * segments connected together.
     * The track list must be sorted by nets, and all segments
     * from m_firstTrack to m_lastTrack have the same net.
     * When 2 items are connected (a track to a pad, or a track to an other track),
     * they are grouped in a cluster.
     * For pads, this is the .m_physical_connexion member which is a cluster identifier
     * For tracks, this is the .m_Subnet member which is a cluster identifier
     * For a given net, if all tracks are created, there is only one cluster.
     * but if not all tracks are created, there are more than one cluster,
     * and some ratsnests will be left active.
     */
    void Propagate_SubNets();

private:
    /**
     * function searchEntryPoint
     * Search an item in m_Connected connected to aPoint
     * note m_Connected containts usually more than one candidate
     * and searchEntryPoint returns an index to one of these candidates
     * Others are neightbor of the indexed item.
     * @param aPoint is the reference coordinates
     * @return the index of item found or -1 if no candidate
     */
    int searchEntryPoint( const wxPoint & aPoint);

    /**
     * Function Merge_SubNets
     * Change a subnet value to a new value, for tracks ans pads which are connected to
     * corresponding track for pads and tracks, this is the .m_Subnet member that is tested
     * and modified these members are block numbers (or cluster numbers) for a given net
     * The result is 2 cluster (or subnets) are merged into only one.
     * Note: the resulting sub net value is the smallest between aOldSubNet et aNewSubNet
     * @return modification count
     * @param aOldSubNet = subnet value to modify
     * @param aNewSubNet = new subnet value for each item which have old_val as subnet value
     */
    int Merge_SubNets( int aOldSubNet, int aNewSubNet );

};


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
    if( aNetcode < 0 )
        m_brd->GetSortedPadListByXthenYCoord( m_sortedPads );
    else
    {
        std::vector<D_PAD*> buffer;
        m_brd->GetSortedPadListByXthenYCoord( buffer );
        int icnt = 0;
        for( unsigned ii = 0; ii < buffer.size(); ii++ )
        {
            if( buffer[ii]->GetNet() == aNetcode )
                icnt++;
        }
        m_sortedPads.reserve(icnt);
        for( unsigned ii = 0; ii < buffer.size(); ii++ )
        {
            if( buffer[ii]->GetNet() == aNetcode )
                m_sortedPads.push_back( buffer[ii] );
        }
    }
}


void CONNECTIONS::SearchConnectedToPads()
{
    std::vector<CONNECTED_POINT*> candidates;

    for( unsigned ii = 0; ii < m_sortedPads.size(); ii++ )
    {
        D_PAD * pad = m_sortedPads[ii];
        candidates.clear();
        CollectItemsNearTo( candidates, pad->ReturnShapePos(), pad->m_ShapeMaxRadius );
        // add this pad to track.m_PadsConnected, if it is connected
        for( unsigned jj = 0; jj < candidates.size(); jj++ )
        {
            CONNECTED_POINT * item = candidates[jj];
            if( (pad->m_layerMask & item->GetTrack()->ReturnMaskLayer()) == 0 )
                continue;
            if( pad->HitTest( item->GetPoint() ) )
            {
                item->GetTrack()->m_PadsConnected.push_back( pad );
                pad->m_TracksConnected.push_back( item->GetTrack() );
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
    if( delta & 1 && delta > 1 )
        delta += 1;
    delta /= 2;
    int idx = delta;        // Starting index is the middle of list
    while( delta )
    {
        if( (delta & 1) && ( delta > 1 ) )
            delta++;
        delta /= 2;

        CONNECTED_POINT& item = m_candidates[idx];

        if( item.GetPoint().x == aPosition.x )
            break;   // A good entry point is found. The list can be scanned from this point.

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

void CONNECTIONS::BuildCandidatesList( TRACK * aBegin, TRACK * aEnd)
{
    m_candidates.clear();

    if( aBegin == NULL )
        aBegin = m_brd->m_Track;

    m_firstTrack = aBegin;

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
        CONNECTED_POINT candidate( track, track->m_Start);
        m_candidates.push_back( candidate );
        if( track->Type() != PCB_VIA_T )
        {
            CONNECTED_POINT candidate2( track, track->m_End);
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

int CONNECTIONS::SearchConnectedTracks( const TRACK * aTrack )
{
    int count = 0;
    m_connected.clear();

    int layerMask = aTrack->ReturnMaskLayer();

    // Search for connections to starting point:
    wxPoint position = aTrack->m_Start;
    for( int kk = 0; kk < 2; kk++ )
    {
        int idx = searchEntryPoint( position );
        if ( idx >= 0 )
        {
            // search after:
            for ( unsigned ii = idx; ii < m_candidates.size(); ii ++ )
            {
                if( m_candidates[ii].GetTrack() == aTrack )
                    continue;
                if( m_candidates[ii].GetPoint() != position )
                    break;
                if( (m_candidates[ii].GetTrack()->ReturnMaskLayer() & layerMask ) != 0 )
                    m_connected.push_back( m_candidates[ii].GetTrack() );
            }
            // search before:
            for ( int ii = idx-1; ii >= 0; ii -- )
            {
                if( m_candidates[ii].GetTrack() == aTrack )
                    continue;
                if( m_candidates[ii].GetPoint() != position )
                    break;
                if( (m_candidates[ii].GetTrack()->ReturnMaskLayer() & layerMask ) != 0 )
                    m_connected.push_back( m_candidates[ii].GetTrack() );
            }
        }

        // Search for connections to ending point:
        if( aTrack->Type() == PCB_VIA_T )
            break;

        position = aTrack->m_End;
    }

    return count;
}

int CONNECTIONS::searchEntryPoint( const wxPoint & aPoint)
{
    // Search the aPoint coordinates in m_Candidates
    // m_Candidates is sorted by X then Y values, and a fast binary search is used
    int idxmax = m_candidates.size()-1;

    int delta = m_candidates.size();
    if( delta & 1 && delta > 1 )
        delta += 1;
    delta /= 2;
    int idx = delta;        // Starting index is the middle of list
    while( delta )
    {
        if( (delta & 1) && ( delta > 1 ) )
            delta++;
        delta /= 2;

        CONNECTED_POINT & candidate = m_candidates[idx];
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
 * Connections to pads are assumed to be already initialized.
 * and are not recalculated
 */
void CONNECTIONS::Build_CurrNet_SubNets_Connections( TRACK* aFirstTrack, TRACK* aLastTrack )
{

    m_firstTrack = aFirstTrack;     // The first track used to build m_Candidates
    m_lastTrack = aLastTrack;       // The last track used to build m_Candidates

    TRACK* curr_track;

    // Pads subnets are expected already cleared, because this function
    // does not know the full list of pads
    BuildCandidatesList( aFirstTrack, aLastTrack );
    for( curr_track = aFirstTrack; curr_track != NULL; curr_track = curr_track->Next() )
    {
        // Clear track subnet id (Pads subnets are cleared outside this function)
        curr_track->SetSubNet( 0 );
        curr_track->m_TracksConnected.clear();

        // Update connections between tracks:
        SearchConnectedTracks( curr_track );
        curr_track->m_TracksConnected = m_connected;

        if( curr_track == aLastTrack )
            break;
    }

    // Creates sub nets (clusters) for the current net:

    Propagate_SubNets();
}


/*
 * Change a subnet value to a new value, for tracks and pads which are connected to.
 * The result is merging 2 clusters (or subnets) into only one cluster.
 * Note: the resultig sub net value is the smallest between aOldSubNet et aNewSubNet
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
                pad->SetSubNet( curr_track->GetSubNet() );
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
 * For pads, this is the .m_physical_connexion member which is a cluster identifier
 * For tracks, this is the .m_Subnet member which is a cluster identifier
 * For a given net, if all tracks are created, there is only one cluster.
 * but if not all tracks are created, there are more than one cluster,
 * and some ratsnests will be left active.
 */
void CONNECTIONS::Propagate_SubNets()
{
    TRACK*      curr_track;
    int         sub_netcode;

    curr_track = (TRACK*)m_firstTrack;
    sub_netcode = 1;
    curr_track->SetSubNet( sub_netcode );

    for( ; curr_track != NULL; curr_track = curr_track->Next() )
    {
        /* First: handling connections to pads */
        for( unsigned ii = 0; ii < curr_track->m_PadsConnected.size(); ii++ )
        {
            D_PAD * pad = curr_track->m_PadsConnected[ii];

            if( curr_track->GetSubNet() )        /* the track segment is already a cluster member */
            {
                if( pad->GetSubNet() > 0 )
                {
                    /* The pad is already a cluster member, so we can merge the 2 clusters */
                    Merge_SubNets( pad->GetSubNet(), curr_track->GetSubNet() );
                }
                else
                {
                    /* The pad is not yet attached to a cluster , so we can add this pad to
                     * the cluster */
                    pad->SetSubNet( curr_track->GetSubNet() );
                }
            }
            else                              /* the track segment is not attached to a cluster */
            {
                if( pad->GetSubNet() > 0 )
                {
                    /* it is connected to a pad in a cluster, merge this track */
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

        /* Test connections between segments */
        for( unsigned ii = 0; ii < curr_track->m_TracksConnected.size(); ii++ )
        {
            BOARD_CONNECTED_ITEM* track = curr_track->m_TracksConnected[ii];
            if( curr_track->GetSubNet() )   // The current track is already a cluster member
            {
                /* The other track is already a cluster member, so we can merge the 2 clusters */
                if( track->GetSubNet() )
                {
                    Merge_SubNets( track->GetSubNet(), curr_track->GetSubNet() );
                }
                else
                {
                    /* The other track is not yet attached to a cluster , so we can add this
                     * other track to the cluster */
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
                    /* it is connected to an other segment not in a cluster, so we must
                     * create a new cluster (only with the 2 track segments) */
                    sub_netcode++;
                    curr_track->SetSubNet( sub_netcode );
                    track->SetSubNet( curr_track->GetSubNet() );
                }
            }
        }

        if( curr_track == m_lastTrack )
            break;
    }
}

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
    CONNECTIONS connections( m_Pcb );
    for( TRACK* track = m_Pcb->m_Track; track; )
    {
        // At this point, track is the first track of a given net
        int    current_net_code = track->GetNet();

        // Get last track of the current net
        TRACK* lastTrack = track->GetEndNetCode( current_net_code );

        if( current_net_code )  // do not spend time if net code = 0 ( dummy net )
            connections.Build_CurrNet_SubNets_Connections( track, lastTrack );

        track = lastTrack->Next();    // this is now the first track of the next net
    }

    Merge_SubNets_Connected_By_CopperAreas( m_Pcb );

    return;
}


void PCB_BASE_FRAME::TestNetConnection( wxDC* aDC, int aNetCode )
{
    wxString msg;

    if( aNetCode == 0 )
        return;

    if( (m_Pcb->m_Status_Pcb & LISTE_RATSNEST_ITEM_OK) == 0 )
        Compile_Ratsnest( aDC, true );

    // Clear the cluster identifier (subnet) of pads for this net
    for( unsigned i = 0; i < m_Pcb->GetPadCount(); ++i )
    {
        D_PAD* pad = m_Pcb->GetPad(i);
        int    pad_net_code = pad->GetNet();

        if( pad_net_code < aNetCode )
            continue;

        if( pad_net_code > aNetCode )
            break;

        pad->SetSubNet( 0 );
    }

    m_Pcb->Test_Connections_To_Copper_Areas( aNetCode );

    /* Search for the first and the last segment relative to the given net code */
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
            connections.Build_CurrNet_SubNets_Connections( firstTrack, lastTrack );
        }
    }

    Merge_SubNets_Connected_By_CopperAreas( m_Pcb, aNetCode );

    /* rebuild the active ratsnest for this net */
    DrawGeneralRatsnest( aDC, aNetCode );
    TestForActiveLinksInRatsnest( aNetCode );
    DrawGeneralRatsnest( aDC, aNetCode );

    /* Display results */
    int net_notconnected_count = 0;
    NETINFO_ITEM* net = m_Pcb->FindNet( aNetCode );
    for( unsigned ii = net->m_RatsnestStartIdx; ii < net->m_RatsnestEndIdx; ii++ )
    {
        if( m_Pcb->m_FullRatsnest[ii].IsActive() )
            net_notconnected_count++;
    }
    msg.Printf( wxT( "links %d nc %d  net:nc %d" ),
                m_Pcb->GetRatsnestsCount(), m_Pcb->GetNoconnectCount(),
                net_notconnected_count );

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
        curr_track->SetState( BUSY | IN_EDIT | BEGIN_ONPAD | END_ONPAD, OFF );
        curr_track->SetZoneSubNet( 0 );
        curr_track->SetNet( 0 );    // net code = 0 means not connected
    }

    // If no pad, reset pointers and netcode, and do nothing else
    if( m_Pcb->GetPadCount() == 0 )
        return;

    CONNECTIONS connections( m_Pcb );
    connections.BuildPadsList();
    connections.BuildCandidatesList();

    // First pass: build connections between track segments and pads.
    connections.SearchConnectedToPads();

    /* For tracks connected to at least one pad,
     * set the track net code to the pad netcode
     */
    curr_track = m_Pcb->m_Track;
    for( ; curr_track != NULL; curr_track = curr_track->Next() )
    {
        if( curr_track->m_PadsConnected.size() )
            curr_track->SetNet( curr_track->m_PadsConnected[0]->GetNet() );
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
            int netcode = curr_track->GetNet();
            if( netcode == 0 )
            {   // try to find a connected item having a netcode
                for( unsigned kk = 0; kk < curr_track->m_TracksConnected.size(); kk++ )
                {
                    int altnetcode = curr_track->m_TracksConnected[kk]->GetNet();
                    if( altnetcode )
                    {
                        new_pass_request = true;
                        netcode = altnetcode;
                        curr_track->SetNet(netcode);
                        break;
                    }
                }
            }
            if( netcode )    // this track has a netcode
            {   // propagate this netcode to connected tracks having no netcode
                for( unsigned kk = 0; kk < curr_track->m_TracksConnected.size(); kk++ )
                {
                    int altnetcode = curr_track->m_TracksConnected[kk]->GetNet();
                    if( altnetcode == 0 )
                    {
                        curr_track->m_TracksConnected[kk]->SetNet(netcode);
                        new_pass_request = true;
                    }
                }
            }
        }
    }

    /* Sort the track list by net codes: */
    RebuildTrackChain( m_Pcb );
}



/*
 * Function SortTracksByNetCode used in RebuildTrackChain()
 * to sort track segments by net code.
 */
static bool SortTracksByNetCode( const TRACK* const & ref, const TRACK* const & compare )
{
    return ref->GetNet() < compare->GetNet();
}

/**
 * Helper function RebuildTrackChain
 * rebuilds the track segment linked list in order to have a chain
 * sorted by increasing netcodes.
 * @param pcb = board to rebuild
 */
static void RebuildTrackChain( BOARD* pcb )
{
    if( pcb->m_Track == NULL )
        return;

    int item_count = pcb->m_Track.GetCount();

    std::vector<TRACK*> trackList;
    trackList.reserve( item_count );

    for( int i = 0; i < item_count; ++i )
        trackList.push_back( pcb->m_Track.PopFront() );

    // the list is empty now
    wxASSERT( pcb->m_Track == NULL && pcb->m_Track.GetCount()==0 );

    sort( trackList.begin(), trackList.end(), SortTracksByNetCode );

    // add them back to the list
    for( int i = 0; i < item_count;  ++i )
        pcb->m_Track.PushBack( trackList[i] );
}
