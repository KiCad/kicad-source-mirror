/**
 * @file connect.cpp
 * @brief Functions to handle existing tracks in ratsnest calculations.
 */

/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2012 Jean-Pierre Charras, jean-pierre.charras@ujf-grenoble.fr
 * Copyright (C) 2012 SoftPLC Corporation, Dick Hollenbeck <dick@softplc.com>
 * Copyright (C) 1992-2015 KiCad Developers, see AUTHORS.txt for contributors.
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
#include <macros.h>
#include <wxBasePcbFrame.h>
#include <view/view.h>

#include <pcbnew.h>

// Helper classes to handle connection points
#include <connect.h>

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

void PCB_BASE_FRAME::TestNetConnection( wxDC* aDC, int aNetCode )
{
#if 0
    // Skip dummy net -1, and "not connected" net 0 (grouping all not connected pads)
    if( aNetCode <= 0 )
        return;

    if( (m_Pcb->m_Status_Pcb & LISTE_RATSNEST_ITEM_OK) == 0 )
        Compile_Ratsnest( aDC, true );

    // Clear the cluster identifier (subnet) of pads for this net
    // Pads are grouped by netcode (and in netname alphabetic order)
    for( unsigned i = 0; i < m_Pcb->GetPadCount(); ++i )
    {
        D_PAD* pad = m_Pcb->GetPad(i);

        if( m_Pcb->GetPad(i)->GetNetCode() == aNetCode )
            pad->SetSubNet( 0 );
    }

    m_Pcb->Test_Connections_To_Copper_Areas( aNetCode );

    // Search for the first and the last segment relative to the given net code
    if( m_Pcb->m_Track )
    {
        CONNECTIONS connections( m_Pcb );

        TRACK* lastTrack = NULL;
        TRACK* firstTrack = m_Pcb->m_Track.GetFirst()->GetStartNetCode( aNetCode );

        if( firstTrack )
            lastTrack = firstTrack->GetEndNetCode( aNetCode );

        if( firstTrack && lastTrack ) // i.e. if there are segments
        {
            connections.Build_CurrNet_SubNets_Connections( firstTrack, lastTrack, aNetCode );
        }
    }

    Merge_SubNets_Connected_By_CopperAreas( m_Pcb, aNetCode );

    // rebuild the active ratsnest for this net
    DrawGeneralRatsnest( aDC, aNetCode );
    TestForActiveLinksInRatsnest( aNetCode );
    DrawGeneralRatsnest( aDC, aNetCode );

    // Display results
    wxString msg;
    int net_notconnected_count = 0;
    NETINFO_ITEM* net = m_Pcb->FindNet( aNetCode );

    if( net )       // Should not occur, but ...
    {
        for( unsigned ii = net->m_RatsnestStartIdx; ii < net->m_RatsnestEndIdx; ii++ )
        {
            if( m_Pcb->m_FullRatsnest[ii].IsActive() )
                net_notconnected_count++;
        }

        msg.Printf( wxT( "links %d nc %d  net %d: not conn %d" ),
                    m_Pcb->GetRatsnestsCount(), m_Pcb->GetUnconnectedNetCount(), aNetCode,
                    net_notconnected_count );
    }
    else
        msg.Printf( wxT( "net not found: netcode %d" ), aNetCode );

    SetStatusText( msg );

    return;
#endif
}
