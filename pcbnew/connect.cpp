/**
 * @file connect.cpp
 * @brief Functions to handle existing tracks in ratsnest calculations.
 */

/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2011 Jean-Pierre Charras, jaen-pierre.charras@gipsa-lab.inpg.com
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
static void Propagate_SubNet( TRACK* pt_start_conn, TRACK* pt_end_conn );
static void Build_Pads_Info_Connections_By_Tracks( TRACK* pt_start_conn, TRACK* pt_end_conn );
static void RebuildTrackChain( BOARD* pcb );


// A helper class to handle connection points
class CONNECTED_POINT
{
public:
    TRACK * m_Track;      // a link to the connected item (track or via)
    wxPoint m_Point;      // the connection point

    CONNECTED_POINT( TRACK * aTrack, wxPoint & aPoint)
    {
        m_Track = aTrack;
        m_Point = aPoint;
    }
};

// A helper class to handle connections calculations:
class CONNECTIONS
{
public:
    std::vector <TRACK*> m_Connected;           // List of connected tracks/vias
                                                // to a given track or via
    std::vector <CONNECTED_POINT> m_Candidates; // List of points to test
                                                // (end points of tracks or vias location )

private:
    BOARD * m_brd;                              // the master board.

public:
    CONNECTIONS( BOARD * aBrd );
    ~CONNECTIONS() {};

    /** Function BuildCandidatesList
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
};

/* sort function used to sort .m_Connected by X the Y values
 * items are sorted by X coordinate value,
 * and for same X value, by Y coordinate value.
 */
static bool sortConnectedPointByXthenYCoordinates( const CONNECTED_POINT & aRef,
                                                   const CONNECTED_POINT & aTst )
{
    if( aRef.m_Point.x == aTst.m_Point.x )
        return aRef.m_Point.y < aTst.m_Point.y;
    return aRef.m_Point.x < aTst.m_Point.x;
}

CONNECTIONS::CONNECTIONS( BOARD * aBrd )
{
    m_brd = aBrd;
}

void CONNECTIONS::BuildCandidatesList( TRACK * aBegin, TRACK * aEnd)
{
    m_Connected.clear();

    if( aBegin == NULL )
        aBegin = m_brd->m_Track;

    unsigned ii = 0;
    // Count candidates ( i.e. end points )
    for( const TRACK* track = aBegin; track; track = track->Next() )
    {
        if( track->Type() == PCB_VIA_T )
            ii++;
        else
            ii += 2;

        if( track == aEnd )
            break;
    }
    // Build candidate list
    m_Connected.reserve( ii );
    for( TRACK* track = aBegin; track != aEnd; track = track->Next() )
    {
        CONNECTED_POINT candidate( track, track->m_Start);
        m_Candidates.push_back( candidate );
        if( track->Type() != PCB_VIA_T )
        {
            candidate.m_Track = track;
            candidate.m_Point = track->m_End;
            m_Candidates.push_back( candidate );
        }

        if( track == aEnd )
            break;
    }

    // Sort list by increasing X coordinate,
    // and for increasing Y coordinate when items have the same X coordinate
    // So candidates to the same location are consecutive in list.
    sort( m_Candidates.begin(), m_Candidates.end(), sortConnectedPointByXthenYCoordinates );
}

int CONNECTIONS::SearchConnectedTracks( const TRACK * aTrack )
{
    int count = 0;
    m_Connected.clear();

    int layerMask = aTrack->ReturnMaskLayer();

    // Search for connections to starting point:
    wxPoint position = aTrack->m_Start;
    for( int kk = 0; kk < 2; kk++ )
    {
        int idx = searchEntryPoint( position );
        if ( idx >= 0 )
        {
            // search after:
            for ( unsigned ii = idx; ii < m_Candidates.size(); ii ++ )
            {
                if( m_Candidates[ii].m_Track == aTrack )
                    continue;
                if( m_Candidates[ii].m_Point != position )
                    break;
                if( (m_Candidates[ii].m_Track->ReturnMaskLayer() & layerMask ) != 0 )
                    m_Connected.push_back( m_Candidates[ii].m_Track );
            }
            // search before:
            for ( int ii = idx-1; ii >= 0; ii -- )
            {
                if( m_Candidates[ii].m_Track == aTrack )
                    continue;
                if( m_Candidates[ii].m_Point != position )
                    break;
                if( (m_Candidates[ii].m_Track->ReturnMaskLayer() & layerMask ) != 0 )
                    m_Connected.push_back( m_Candidates[ii].m_Track );
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
    int idxmax = m_Candidates.size()-1;

    int delta = m_Candidates.size();
    if( delta & 1 && delta > 1 )
        delta += 1;
    delta /= 2;
    int idx = delta;        // Starting index is the middle of list
    while( delta )
    {
        if( (delta & 1) && ( delta > 1 ) )
            delta++;
        delta /= 2;

        CONNECTED_POINT & candidate = m_Candidates[idx];
        if( candidate.m_Point == aPoint )   // candidate found
        {
            return idx;
        }

        // Not found: test the middle of the remaining sub list
        if( candidate.m_Point.x == aPoint.x )   // Must search considering Y coordinate
        {
            if(candidate.m_Point.y < aPoint.y)  // Must search after this item
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
        else if( candidate.m_Point.x < aPoint.x ) // Must search after this item
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


/**
 * Function Merge_Two_SubNets
 * Used by Propagate_SubNet()
 * Change a subnet value to a new value, for tracks ans pads which are connected to
 * corresponding track for pads and tracks, this is the .m_Subnet member that is tested
 * and modified these members are block numbers (or cluster numbers) for a given net
 * The result is merging 2 blocks (or subnets)
 * @return modification count
 * @param old_val = subnet value to modify
 * @param new_val = new subnet value for each item which have old_val as subnet value
 * @param pt_start_conn = first track segment to test
 * @param pt_end_conn = last track segment to test
 * If pt_end_conn = NULL: search is made from pt_start_conn to end of linked list
 */
static int Merge_Two_SubNets( TRACK* pt_start_conn, TRACK* pt_end_conn, int old_val, int new_val )
{
    TRACK* pt_conn;
    int    nb_change = 0;
    D_PAD* pt_pad;

    if( old_val == new_val )
        return 0;

    if( (old_val > 0) && (old_val < new_val) )
        EXCHG( old_val, new_val );

    pt_conn = pt_start_conn;

    for( ; pt_conn != NULL; pt_conn = pt_conn->Next() )
    {
        if( pt_conn->GetSubNet() != old_val )
        {
            if( pt_conn == pt_end_conn )
                break;

            continue;
        }

        nb_change++;
        pt_conn->SetSubNet( new_val );

        if( pt_conn->start && ( pt_conn->start->Type() == PCB_PAD_T) )
        {
            pt_pad = (D_PAD*) (pt_conn->start);

            if( pt_pad->GetSubNet() == old_val )
                pt_pad->SetSubNet( pt_conn->GetSubNet() );
        }

        if( pt_conn->end && (pt_conn->end->Type() == PCB_PAD_T) )
        {
            pt_pad = (D_PAD*) (pt_conn->end);

            if( pt_pad->GetSubNet() == old_val )
                pt_pad->SetSubNet( pt_conn->GetSubNet() );
        }

        if( pt_conn == pt_end_conn )
            break;
    }

    return nb_change;
}



/**
 * Function Propagate_SubNet
 * Test a list of track segments, to create or propagate a sub netcode to pads and
 * segments connected together the track list must be sorted by nets, and all segments
 * from pt_start_conn to pt_end_conn have the same net when 2 items are connected (a
 * track to a pad, or a track to an other track) they are grouped in a cluster.
 * for pads, this is the .m_physical_connexion member which is a cluster identifier
 * for tracks, this is the .m_Subnet member which is a cluster identifier
 * For a given net, if all tracks are created, there is only one cluster.
 * but if not all tracks are created, there are more than one cluster, and some ratsnest
 * will be shown.
 * @param pt_start_conn = first track to test
 * @param pt_end_conn = last segment to test
 */
static void Propagate_SubNet( TRACK* pt_start_conn, TRACK* pt_end_conn )
{
    TRACK*      pt_conn;
    int         sub_netcode;
    D_PAD*      pt_pad;
    TRACK*      pt_other_trace;
    BOARD_ITEM* PtStruct;

    /* Clear variables used in computations */
    pt_conn = pt_start_conn;

    for( ; pt_conn != NULL; pt_conn = pt_conn->Next() )
    {
        pt_conn->SetSubNet( 0 );
        PtStruct = pt_conn->start;

        if( PtStruct && (PtStruct->Type() == PCB_PAD_T) )
            ( (D_PAD*) PtStruct )->SetSubNet( 0 );

        PtStruct = pt_conn->end;

        if( PtStruct && (PtStruct->Type() == PCB_PAD_T) )
            ( (D_PAD*) PtStruct )->SetSubNet( 0 );

        if( pt_conn == pt_end_conn )
            break;
    }

    sub_netcode = 1;
    pt_start_conn->SetSubNet( sub_netcode );

    /* Start of calculation */
    pt_conn = pt_start_conn;

    for( ; pt_conn != NULL; pt_conn = pt_conn->Next() )
    {
        /* First: handling connections to pads */
        PtStruct = pt_conn->start;

        /* The segment starts on a pad */
        if( PtStruct && (PtStruct->Type() == PCB_PAD_T) )
        {
            pt_pad = (D_PAD*) PtStruct;

            if( pt_conn->GetSubNet() )        /* the track segment is already a cluster member */
            {
                if( pt_pad->GetSubNet() > 0 )
                {
                    /* The pad is already a cluster member, so we can merge the 2 clusters */
                    Merge_Two_SubNets( pt_start_conn, pt_end_conn,
                                       pt_pad->GetSubNet(), pt_conn->GetSubNet() );
                }
                else
                {
                    /* The pad is not yet attached to a cluster , so we can add this pad to
                     * the cluster */
                    pt_pad->SetSubNet( pt_conn->GetSubNet() );
                }
            }
            else                              /* the track segment is not attached to a cluster */
            {
                if( pt_pad->GetSubNet() > 0 )
                {
                    /* it is connected to a pad in a cluster, merge this track */
                    pt_conn->SetSubNet( pt_pad->GetSubNet() );
                }
                else
                {
                    /* it is connected to a pad not in a cluster, so we must create a new
                     * cluster (only with the 2 items: the track and the pad) */
                    sub_netcode++;
                    pt_conn->SetSubNet( sub_netcode );
                    pt_pad->SetSubNet( pt_conn->GetSubNet() );
                }
            }
        }

        PtStruct = pt_conn->end;

        /* The segment end on a pad */
        if( PtStruct && (PtStruct->Type() == PCB_PAD_T) )
        {
            pt_pad = (D_PAD*) PtStruct;

            if( pt_conn->GetSubNet() )
            {
                if( pt_pad->GetSubNet() > 0 )
                {
                    Merge_Two_SubNets( pt_start_conn, pt_end_conn,
                                       pt_pad->GetSubNet(), pt_conn->GetSubNet() );
                }
                else
                {
                    pt_pad->SetSubNet( pt_conn->GetSubNet() );
                }
            }
            else
            {
                if( pt_pad->GetSubNet() > 0 )
                {
                    pt_conn->SetSubNet( pt_pad->GetSubNet() );
                }
                else
                {
                    sub_netcode++;
                    pt_conn->SetSubNet( sub_netcode );
                    pt_pad->SetSubNet( pt_conn->GetSubNet() );
                }
            }
        }


        /* Test connections between segments */
        PtStruct = pt_conn->start;

        if( PtStruct && (PtStruct->Type() != PCB_PAD_T) )
        {
            /* The segment starts on an other track */
            pt_other_trace = (TRACK*) PtStruct;

            /* the track segment is already a cluster member */
            if( pt_conn->GetSubNet() )
            {
                /* The other track is already a cluster member, so we can merge the 2 clusters */
                if( pt_other_trace->GetSubNet() )
                {
                    Merge_Two_SubNets( pt_start_conn, pt_end_conn,
                                       pt_other_trace->GetSubNet(), pt_conn->GetSubNet() );
                }
                else
                {
                    /* The other track is not yet attached to a cluster , so we can add this
                     * other track to the cluster */
                    pt_other_trace->SetSubNet( pt_conn->GetSubNet() );
                }
            }
            else
            {
                /* the track segment is not yet attached to a cluster */
                if( pt_other_trace->GetSubNet() )
                {
                    /* The other track is already a cluster member, so we can add the segment
                     * to the cluster */
                    pt_conn->SetSubNet( pt_other_trace->GetSubNet() );
                }
                else
                {
                    /* it is connected to an other segment not in a cluster, so we must
                     * create a new cluster (only with the 2 track segments) */
                    sub_netcode++;
                    pt_conn->SetSubNet( sub_netcode );
                    pt_other_trace->SetSubNet( pt_conn->GetSubNet() );
                }
            }
        }

        PtStruct = pt_conn->end;    // Do the same calculations for the segment end point

        if( PtStruct && (PtStruct->Type() != PCB_PAD_T) )
        {
            pt_other_trace = (TRACK*) PtStruct;

            if( pt_conn->GetSubNet() )  /* the track segment is already a cluster member */
            {
                if( pt_other_trace->GetSubNet() )
                {
                    Merge_Two_SubNets( pt_start_conn, pt_end_conn,
                                       pt_other_trace->GetSubNet(), pt_conn->GetSubNet() );
                }
                else
                {
                    pt_other_trace->SetSubNet( pt_conn->GetSubNet() );
                }
            }
            else
            {
                /* the track segment is not yet attached to a cluster */
                if( pt_other_trace->GetSubNet() )
                {
                    pt_conn->SetSubNet( pt_other_trace->GetSubNet() );
                }
                else
                {
                    sub_netcode++;
                    pt_conn->SetSubNet( sub_netcode );
                    pt_other_trace->SetSubNet( pt_conn->GetSubNet() );
                }
            }
        }

        if( pt_conn == pt_end_conn )
            break;
    }
}


void PCB_BASE_FRAME::TestConnections( wxDC* aDC )
{
    // Clear the cluster identifier for all pads
    for( unsigned i = 0;  i< m_Pcb->GetPadsCount();  ++i )
    {
        D_PAD* pad = m_Pcb->m_NetInfo->GetPad(i);

        pad->SetZoneSubNet( 0 );
        pad->SetSubNet( 0 );
    }

    m_Pcb->Test_Connections_To_Copper_Areas();

    // Test existing connections net by net
    for( TRACK* track = m_Pcb->m_Track;  track; )
    {
        // this is the current net because pt_start_conn is the first segment of the net
        int    current_net_code = track->GetNet();

        // this is the last segment of the current net
        TRACK* pt_end_conn = track->GetEndNetCode( current_net_code );

        Build_Pads_Info_Connections_By_Tracks( track, pt_end_conn );

        track = pt_end_conn->Next();    // this is now the first segment of the next net
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

    for( unsigned i = 0;  i<m_Pcb->GetPadsCount();  ++i )
    {
        D_PAD* pad = m_Pcb->m_NetInfo->GetPad(i);

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
        TRACK* pt_start_conn;
        TRACK* pt_end_conn = NULL;
        pt_start_conn = m_Pcb->m_Track.GetFirst()->GetStartNetCode( aNetCode );

        if( pt_start_conn )
            pt_end_conn = pt_start_conn->GetEndNetCode( aNetCode );

        if( pt_start_conn && pt_end_conn ) // c.a.d. if there are segments
        {
            Build_Pads_Info_Connections_By_Tracks( pt_start_conn, pt_end_conn );
        }
    }

    Merge_SubNets_Connected_By_CopperAreas( m_Pcb, aNetCode );

    /* Test the ratsnest for this net */
    int nb_net_noconnect = TestOneRatsNest( aDC, aNetCode );

    /* Display results */
    msg.Printf( wxT( "links %d nc %d  net:nc %d" ),
                m_Pcb->GetRatsnestsCount(), m_Pcb->GetNoconnectCount(),
                nb_net_noconnect );

    SetStatusText( msg );
    return;
}


/**  Used after a track change (delete a track ou add a track)
 * Compute connections (initialize the .start and .end members) for a single net.
 * tracks must be sorted by net, as usual
 *  @param pt_start_conn = first segment of the net
 *  @param pt_end_conn = last segment of the net
 *  Connections to pads are assumed to be already initialized.
 *  If a track is deleted, the other pointers to pads do not change.
 *  When a track is added, its pointers to pads are already initialized
 */
static void Build_Pads_Info_Connections_By_Tracks( TRACK* pt_start_conn, TRACK* pt_end_conn )
{
    TRACK* Track;

    /* Reset the old connections type track to track */
    for( Track = pt_start_conn; Track != NULL; Track = Track->Next() )
    {
        Track->SetSubNet( 0 );

        if( Track->GetState( BEGIN_ONPAD ) == 0 )
            Track->start = NULL;

        if( Track->GetState( END_ONPAD ) == 0 )
            Track->end = NULL;

        if( Track == pt_end_conn )
            break;
    }

    /* Update connections type track to track */
    for( Track = pt_start_conn; Track != NULL; Track = Track->Next() )
    {
        if( Track->Type() == PCB_VIA_T )
        {
            // A via can connect many tracks, we must search for all track segments in this net
            TRACK* pt_segm;
            int    layermask = Track->ReturnMaskLayer();

            for( pt_segm = pt_start_conn; pt_segm != NULL; pt_segm = pt_segm->Next() )
            {
                int curlayermask = pt_segm->ReturnMaskLayer();

                if( !pt_segm->start && (pt_segm->m_Start == Track->m_Start)
                   && ( layermask & curlayermask ) )
                {
                    pt_segm->start = Track;
                }

                if( !pt_segm->end && (pt_segm->m_End == Track->m_Start)
                   && (layermask & curlayermask) )
                {
                    pt_segm->end = Track;
                }

                if( pt_segm == pt_end_conn )
                    break;
            }
        }

        if( Track->start == NULL )  // end track not already connected, search a connection
        {
            Track->start = Track->GetTrace( Track, pt_end_conn, START );
        }

        if( Track->end == NULL )    // end track not already connected, search a connection
        {
            Track->end = Track->GetTrace( Track, pt_end_conn, END );
        }

        if( Track == pt_end_conn )
            break;
    }

    /* Creates sub nets (cluster) for the current net: */
    Propagate_SubNet( pt_start_conn, pt_end_conn );
}


void PCB_BASE_FRAME::RecalculateAllTracksNetcode()
{
    TRACK*              curr_track;
    std::vector<D_PAD*> sortedPads;

    // Build the net info list
    GetBoard()->m_NetInfo->BuildListOfNets();

    if( m_Pcb->GetPadsCount() == 0 ) // If no pad, reset pointers and netcode, and do nothing else
    {
        curr_track = m_Pcb->m_Track;

        for( ; curr_track != NULL; curr_track = curr_track->Next() )
        {
            curr_track->start = NULL;
            curr_track->SetState( BEGIN_ONPAD | END_ONPAD, OFF );
            curr_track->SetNet( 0 );
            curr_track->end = NULL;
        }

        return;
    }

    /**************************************************************/
    /* Pass 1: search the connections between track ends and pads */
    /**************************************************************/

    m_Pcb->GetSortedPadListByXthenYCoord( sortedPads );

    /* Reset variables and flags used in computation */
    curr_track = m_Pcb->m_Track;

    for( ; curr_track != NULL; curr_track = curr_track->Next() )
    {
        curr_track->m_TracksConnected.clear();
        curr_track->SetState( BUSY | IN_EDIT | BEGIN_ONPAD | END_ONPAD, OFF );
        curr_track->SetZoneSubNet( 0 );
        curr_track->SetNet( 0 );  // net code = 0 means not connected
    }

    /* First pass: search connection between a track segment and a pad.
     * if found, set the track net code to the pad netcode
     */
    curr_track = m_Pcb->m_Track;

    for( ; curr_track != NULL; curr_track = curr_track->Next() )
    {
        int layerMask = g_TabOneLayerMask[curr_track->GetLayer()];

        /* Search for a pad on the segment starting point */
        curr_track->start = m_Pcb->GetPad( sortedPads, curr_track->m_Start, layerMask );

        if( curr_track->start != NULL )
        {
            curr_track->SetState( BEGIN_ONPAD, ON );
            curr_track->SetNet( ( (D_PAD*) (curr_track->start) )->GetNet() );
        }

        /* Search for a pad on the segment ending point */
        curr_track->end = m_Pcb->GetPad( sortedPads, curr_track->m_End, layerMask );

        if( curr_track->end != NULL )
        {
            curr_track->SetState( END_ONPAD, ON );
            curr_track->SetNet( ( (D_PAD*) (curr_track->end) )->GetNet() );
        }
    }

    /*****************************************************/
    /* Pass 2: search the connections between track ends */
    /*****************************************************/

    /* the .start and .end member pointers are updated, and point on connected pads
     * or are null for tracks whitch are not connection to pads
     * Now build connections lists to tracks
     */

    CONNECTIONS connections( m_Pcb );
    connections.BuildCandidatesList();
    for( curr_track = m_Pcb->m_Track; curr_track != NULL; curr_track = curr_track->Next() )
    {

        if( curr_track->start != NULL && curr_track->end != NULL )
            continue;

        connections.SearchConnectedTracks( curr_track );
        curr_track->m_TracksConnected = connections.m_Connected;
    }

    // Propagate net codes from a segment to other connected segments
    bool new_pass_request = true;   // is true if a track has its netcode changes from 0
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
 * Function RebuildTrackChain
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

    for( int i = 0; i<item_count;  ++i )
        trackList.push_back( pcb->m_Track.PopFront() );

    // the list is empty now
    wxASSERT( pcb->m_Track == NULL && pcb->m_Track.GetCount()==0 );

    sort( trackList.begin(), trackList.end(), SortTracksByNetCode );

    // add them back to the list
    for( int i = 0; i < item_count;  ++i )
    {
        pcb->m_Track.PushBack( trackList[i] );
    }
}
