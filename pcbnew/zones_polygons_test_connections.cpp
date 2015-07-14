/**
 * @file zones_polygons_test_connections.cpp
 */

/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2012 Jean-Pierre Charras, jean-pierre.charras@ujf-grenoble.fr
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

#include <algorithm> // sort

#include <fctsys.h>
#include <common.h>
#include <macros.h>

#include <class_board.h>
#include <class_module.h>
#include <class_track.h>
#include <class_zone.h>

#include <pcbnew.h>
#include <zones.h>
#include <polygon_test_point_inside.h>

static bool CmpZoneSubnetValue( const BOARD_CONNECTED_ITEM* a, const BOARD_CONNECTED_ITEM* b );

void Merge_SubNets_Connected_By_CopperAreas( BOARD* aPcb, int aNetcode );

// This helper function sort a list of zones by netcode,
// and for a given netcode by zone size
// zone size = size of the m_FilledPolysList buffer
bool sort_areas( const ZONE_CONTAINER* ref, const ZONE_CONTAINER* tst )
{
    if( ref->GetNetCode() == tst->GetNetCode() )
        return ref->GetFilledPolysList().GetCornersCount() <
               tst->GetFilledPolysList().GetCornersCount();
    else
        return ref->GetNetCode() < tst->GetNetCode();
}

/**
 * Function Test_Connection_To_Copper_Areas
 * init .m_ZoneSubnet parameter in tracks and pads according to the connections to areas found
 * @param aNetcode = netcode to analyse. if -1, analyse all nets
 */
void BOARD::Test_Connections_To_Copper_Areas( int aNetcode )
{
    // list of pads and tracks candidates on this layer and on this net.
    // It is static to avoid multiple memory realloc.
    static std::vector <BOARD_CONNECTED_ITEM*> candidates;

    // clear .m_ZoneSubnet parameter for pads
    for( MODULE* module = m_Modules;  module;  module = module->Next() )
    {
        for( D_PAD* pad = module->Pads();  pad;  pad = pad->Next() )
            if( aNetcode < 0 || aNetcode == pad->GetNetCode() )
                pad->SetZoneSubNet( 0 );
    }

    // clear .m_ZoneSubnet parameter for tracks and vias
    for( TRACK* track = m_Track;  track;  track = track->Next() )
    {
        if( aNetcode < 0 || aNetcode == track->GetNetCode() )
            track->SetZoneSubNet( 0 );
    }

    // examine all zones, net by net:
    int subnet = 0;

    // Build zones candidates list
    std::vector<ZONE_CONTAINER*> zones_candidates;

    zones_candidates.reserve( GetAreaCount() );

    for( int index = 0; index < GetAreaCount(); index++ )
    {
        ZONE_CONTAINER* zone = GetArea( index );

        if( !zone->IsOnCopperLayer() )
            continue;

        if( aNetcode >= 0 &&  aNetcode != zone->GetNetCode() )
            continue;

        if( zone->GetFilledPolysList().GetCornersCount() == 0 )
            continue;

        zones_candidates.push_back( zone );
    }

    // sort them by netcode then vertices count.
    // For a given net, examine the smaller zones first slightly speed up calculation
    // (25% faster)
    // this is only noticeable with very large boards and depends on board zones topology
    // This is due to the fact some items are connected by small zones ares,
    // before examining large zones areas and these items are not tested after a connection is found
    sort( zones_candidates.begin(), zones_candidates.end(), sort_areas );

    int oldnetcode = -1;
    for( unsigned idx = 0; idx < zones_candidates.size(); idx++ )
    {
        ZONE_CONTAINER* zone = zones_candidates[idx];

        int netcode = zone->GetNetCode();

        // Build a list of candidates connected to the net:
        // At this point, layers are not considered, because areas on different layers can
        // be connected by a via or a pad.
        // (because zones are sorted by netcode, there is made only once per net)
        NETINFO_ITEM* net = FindNet( netcode );

        wxASSERT( net );
        if( net == NULL )
            continue;

        if( oldnetcode != netcode )
        {
            oldnetcode = netcode;
            candidates.clear();

            // Build the list of pads candidates connected to the net:
            candidates.reserve( net->m_PadInNetList.size() );

            for( unsigned ii = 0; ii < net->m_PadInNetList.size(); ii++ )
                candidates.push_back( net->m_PadInNetList[ii] );

            // Build the list of track candidates connected to the net:
            TRACK* track = m_Track.GetFirst()->GetStartNetCode( netcode );

            for( ; track; track = track->Next() )
            {
                if( track->GetNetCode() != netcode )
                    break;

                candidates.push_back( track );
            }
        }

        // test if a candidate is inside a filled area of this zone
        unsigned indexstart = 0, indexend;
        const CPOLYGONS_LIST& polysList = zone->GetFilledPolysList();

        for( indexend = 0; indexend < polysList.GetCornersCount(); indexend++ )
        {
            // end of a filled sub-area found
            if( polysList.IsEndContour( indexend ) )
            {
                subnet++;
                EDA_RECT bbox = zone->CalculateSubAreaBoundaryBox( indexstart, indexend );

                for( unsigned ic = 0; ic < candidates.size(); ic++ )
                {
                    // test if this area is connected to a board item:
                    BOARD_CONNECTED_ITEM* item = candidates[ic];

                    if( item->GetZoneSubNet() == subnet )   // Already merged
                        continue;

                   if( !item->IsOnLayer( zone->GetLayer() ) )
                        continue;

                    wxPoint pos1, pos2;

                    if( item->Type() == PCB_PAD_T )
                    {
                        // For pads we use the shape position instead of
                        // the pad position, because the zones are connected
                        // to the center of the shape, not the pad position
                        // (this is important for pads with thermal relief)
                        pos1 = pos2 = ( (D_PAD*) item )->ShapePos();
                    }
                    else if( item->Type() == PCB_VIA_T )
                    {
                        const VIA *via = static_cast<const VIA*>( item );
                        pos1 = via->GetStart();
                        pos2 = pos1;
                    }
                    else if( item->Type() == PCB_TRACE_T )
                    {
                        const TRACK *trk = static_cast<const TRACK*>( item );
                        pos1 = trk->GetStart();
                        pos2 = trk->GetEnd();
                    }
                    else
                    {
                        continue;
                    }

                    bool connected = false;

                    if( bbox.Contains( pos1 ) )
                    {
                        if( TestPointInsidePolygon( polysList, indexstart,
                                                    indexend, pos1.x, pos1.y ) )
                            connected = true;
                    }
                    if( !connected && (pos1 != pos2 ) )
                    {
                        if( bbox.Contains( pos2 ) )
                        {
                            if( TestPointInsidePolygon( polysList,
                                                        indexstart, indexend,
                                                        pos2.x, pos2.y ) )
                                connected = true;
                        }
                    }

                    if( connected )
                    {
                        // Set ZoneSubnet to the current subnet value.
                        // If the previous subnet is not 0, merge all items with old subnet
                        // to the new one
                        int old_subnet = item->GetZoneSubNet();
                        item->SetZoneSubNet( subnet );

                        // Merge previous subnet with the current
                        if( (old_subnet > 0) && (old_subnet != subnet) )
                        {
                            for( unsigned jj = 0; jj < candidates.size(); jj++ )
                            {
                                BOARD_CONNECTED_ITEM* item_to_merge = candidates[jj];

                                if( old_subnet == item_to_merge->GetZoneSubNet() )
                                {
                                    item_to_merge->SetZoneSubNet( subnet );
                                }
                            }
                        }   // End if ( old_subnet > 0 )
                    }       // End if( connected )
                }

                // End test candidates for the current filled area
                indexstart = indexend + 1;  // prepare test next area, starting at indexend+1
                                            // (if exists).  End read one area in
                                            // zone->m_FilledPolysList
            }
        } // End read all segments in zone
    } // End read all zones candidates
}


/**
 * Function Merge_SubNets_Connected_By_CopperAreas(BOARD* aPcb)
 * Calls Merge_SubNets_Connected_By_CopperAreas( BOARD* aPcb, int aNetcode ) for each
 * netcode found in zone list
 * @param aPcb = the current board
 */
void Merge_SubNets_Connected_By_CopperAreas( BOARD* aPcb )
{
    for( int index = 0; index < aPcb->GetAreaCount(); index++ )
    {
        ZONE_CONTAINER* zone = aPcb->GetArea( index );

        if ( ! zone->IsOnCopperLayer() )
            continue;

        if ( zone->GetNetCode() <= 0 )
            continue;

        Merge_SubNets_Connected_By_CopperAreas( aPcb, zone->GetNetCode() );
    }
}


/**
 * Function Merge_SubNets_Connected_By_CopperAreas(BOARD* aPcb, int aNetcode)
 * Used after connections by tracks calculations
 * Merge subnets, in tracks ans pads when they are connected by a filled copper area
 * for pads, this is the .m_physical_connexion member which is tested and modified
 * for tracks, this is the .m_Subnet member which is tested and modified
 * these members are block numbers (or cluster numbers) for a given net,
 * calculated by Build_Pads_Info_Connections_By_Tracks()
 * The result is merging 2 blocks (or subnets)
 * @param aPcb = the current board
 * @param aNetcode = netcode to consider
 */
void Merge_SubNets_Connected_By_CopperAreas( BOARD* aPcb, int aNetcode )
{
    // Ensure a zone with the given netcode exists: examine all zones:
    bool found = false;

    for( int index = 0; index < aPcb->GetAreaCount(); index++ )
    {
        ZONE_CONTAINER* zone = aPcb->GetArea( index );

        if( aNetcode == zone->GetNetCode() )
        {
            found = true;
            break;
        }
    }

    if( !found )  // No zone with this netcode, therefore no connection by zone
        return;

    // list of pads and tracks candidates to test:
    // It is static to avoid multiple memory realloc.
    static std::vector <BOARD_CONNECTED_ITEM*> Candidates;
    Candidates.clear();

    // Build the list of pads candidates connected to the net:
    NETINFO_ITEM* net = aPcb->FindNet( aNetcode );
    wxASSERT( net );
    Candidates.reserve( net->m_PadInNetList.size() );
    for( unsigned ii = 0; ii < net->m_PadInNetList.size(); ii++ )
        Candidates.push_back( net->m_PadInNetList[ii] );

    // Build the list of track candidates connected to the net:
    TRACK* track;
    track = aPcb->m_Track.GetFirst()->GetStartNetCode( aNetcode );
    for( ; track; track = track->Next() )
    {
        if( track->GetNetCode() != aNetcode )
            break;
        Candidates.push_back( track );
    }

    if( Candidates.size() == 0 )
        return;

    int next_subnet_free_number = 0;
    for( unsigned ii = 0; ii < Candidates.size(); ii++ )
    {
        int subnet = Candidates[ii]->GetSubNet();
        next_subnet_free_number = std::max( next_subnet_free_number, subnet );
    }

    next_subnet_free_number++;     // This is a subnet we can use with not connected items
                                   // by tracks, but connected by zone.

    // Sort by zone_subnet:
    sort( Candidates.begin(), Candidates.end(), CmpZoneSubnetValue );

    // Some items can be not connected, but they can be connected to a filled area:
    // give them a subnet common to these items connected only by the area,
    // and not already used.
    // a value like next_subnet_free_number+zone_subnet is right
    for( unsigned jj = 0; jj < Candidates.size(); jj++ )
    {
        BOARD_CONNECTED_ITEM* item = Candidates[jj];
        if ( item->GetSubNet() == 0 && (item->GetZoneSubNet() > 0) )
        {
            item->SetSubNet( next_subnet_free_number + item->GetZoneSubNet() );
        }
    }

    // Now, for each zone subnet, we search for 2 items with different subnets.
    // if found, the 2 subnet are merged in the whole candidate list.
    int old_subnet      = 0;
    int old_zone_subnet = 0;
    for( unsigned ii = 0; ii < Candidates.size(); ii++ )
    {
        BOARD_CONNECTED_ITEM* item = Candidates[ii];
        int zone_subnet = item->GetZoneSubNet();

        if( zone_subnet == 0 )  // Not connected by a filled area, skip it
            continue;

        int subnet = item->GetSubNet();

        if( zone_subnet != old_zone_subnet )  // a new zone subnet is found
        {
            old_subnet = subnet;
            old_zone_subnet = zone_subnet;
            continue;
        }

        zone_subnet = old_zone_subnet;

        // 2 successive items already from the same cluster: nothing to do
        if( subnet == old_subnet )
            continue;

        // Here we have 2 items connected by the same area have 2 differents subnets: merge subnets
        if( (subnet > old_subnet) || ( subnet <= 0) )
            std::swap( subnet, old_subnet );

        for( unsigned jj = 0; jj < Candidates.size(); jj++ )
        {
            BOARD_CONNECTED_ITEM * item_to_merge = Candidates[jj];

            if( item_to_merge->GetSubNet() == old_subnet )
                item_to_merge->SetSubNet( subnet );
        }

        old_subnet = subnet;
    }
}


/* Compare function used for sorting candidates  by increasing zone zubnet
 */
static bool CmpZoneSubnetValue( const BOARD_CONNECTED_ITEM* a, const BOARD_CONNECTED_ITEM* b )
{
    int asubnet, bsubnet;

    asubnet = a->GetZoneSubNet();
    bsubnet = b->GetZoneSubNet();

    return asubnet < bsubnet;
}
