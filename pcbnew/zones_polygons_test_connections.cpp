/////////////////////////////////////////////////////////////////////////////

// Name:        zones_polygons_test_connections.cpp
// Licence:     GPL License
/////////////////////////////////////////////////////////////////////////////

#ifndef WX_PRECOMP
#include "wx/wx.h"
#endif


// For compilers that support precompilation, includes "wx/wx.h".
#include "wx/wxprec.h"

#ifdef __BORLANDC__
#pragma hdrstop
#endif

using namespace std;
#include <algorithm> // sort
#include <vector>


#include "fctsys.h"

#include "common.h"
#include "pcbnew.h"
#include "PolyLine.h"

#include "zones.h"

static bool CmpZoneSubnetValue( const BOARD_CONNECTED_ITEM* a, const BOARD_CONNECTED_ITEM* b );
void Merge_SubNets_Connected_By_CopperAreas( BOARD* aPcb, int aNetcode );


/***********************************************************/
void BOARD::Test_Connections_To_Copper_Areas( int aNetcode )
/***********************************************************/

/**
 * Function Test_Connection_To_Copper_Areas
 * init .m_ZoneSubnet parameter in tracks and pads according to the connections to areas found
 * @param aNetcode = netcode to analyse. if -1, analyse all nets
 */
{
    std::vector <BOARD_CONNECTED_ITEM*> Candidates;  // list of pads and tracks candidates on this layer and on this net.
    int                   subnet = 0;
    int                   netcode;
    ZONE_CONTAINER*       curr_zone;
    BOARD_CONNECTED_ITEM* item;

    // clear .m_ZoneSubnet parameter for pads
    for( MODULE* module = m_Modules;  module;  module = module->Next() )
    {
        for( D_PAD* pad = module->m_Pads; pad != NULL; pad = pad->Next() )
            if( (aNetcode < 0) || ( aNetcode == pad->GetNet() ) )
                pad->SetZoneSubNet( 0 );
    }

    // clear .m_ZoneSubnet parameter for tracks and vias
    for( TRACK* track = m_Track;  track;  track = track->Next() )
    {
        if( (aNetcode < 0) || ( aNetcode == track->GetNet() ) )
            track->SetZoneSubNet( 0 );
    }

    // examine all zones, net by net:
    for( int index = 0; index < GetAreaCount(); index++ )
    {
        curr_zone = GetArea( index );
        if( !curr_zone->IsOnCopperLayer() )
            continue;

        netcode = curr_zone->GetNet();
        if( (aNetcode >= 0) && !( aNetcode == netcode ) )
            continue;

        if( curr_zone->m_FilledPolysList.size() == 0 )
            continue;

        // Build a list of candidates connected to the net:
        Candidates.clear();

        // At this point, layers are not considered, because areas on different layers can be connected by a via or a pad.
        for( MODULE* module = m_Modules;  module;  module = module->Next() )
        {
            for( D_PAD* pad = module->m_Pads; pad != NULL; pad = pad->Next() )
            {
                if( pad->GetNet() != curr_zone->GetNet() )
                    continue;

                Candidates.push_back( pad );
            }
        }

        for( TRACK* track = m_Track; track;  track = track->Next() )
        {
            if( track->GetNet() != netcode )
                continue;
            Candidates.push_back( track );
        }

        // test if a candidate is inside a filled area of this zone
        unsigned indexstart = 0, indexend;
        for( indexend = 0; indexend < curr_zone->m_FilledPolysList.size(); indexend++ )
        {
            if( curr_zone->m_FilledPolysList[indexend].end_contour ) // end of a filled sub-area found
            {
                subnet++;
                EDA_Rect bbox = curr_zone->CalculateSubAreaBoundaryBox( indexstart, indexend );
                for( unsigned ic = 0; ic < Candidates.size(); ic++ )
                { // test if this area is connected to a board item:
                    item = Candidates[ic];
                    if( !item->IsOnLayer( curr_zone->GetLayer() ) )
                        continue;
                    wxPoint pos1, pos2;
                    if( item->Type() == TYPE_PAD )
                    {
                        pos1 = pos2 = ( (D_PAD*) item )->m_Pos;
                    }
                    else if( item->Type() == TYPE_VIA )
                    {
                        pos1 = pos2 = ( (SEGVIA*) item )->m_Start;
                    }
                    else if( item->Type() == TYPE_TRACK )
                    {
                        pos1 = ( (TRACK*) item )->m_Start;
                        pos2 = ( (TRACK*) item )->m_End;
                    }
                    else
                        continue;
                    bool connected = false;
                    if( bbox.Inside( pos1 ) )
                    {
                        if( TestPointInsidePolygon( curr_zone->m_FilledPolysList, indexstart,
                                                    indexend, pos1.x, pos1.y ) )
                            connected = true;
                    }
                    if( !connected && (pos1 != pos2 ) )
                    {
                        if( bbox.Inside( pos2 ) )
                            if( TestPointInsidePolygon( curr_zone->m_FilledPolysList, indexstart,
                                                        indexend, pos2.x, pos2.y ) )
                                connected = true;
                    }

                    if( connected )
                    {   // Set ZoneSubnet to the current subnet value.
                        // If the previous subnet is not 0, merge all items with old subnet to the new one
                        int old_subnet = 0;
                        old_subnet = item->GetZoneSubNet();
                        item->SetZoneSubNet( subnet );

                        if( (old_subnet > 0) && (old_subnet != subnet) )      // Merge previous subnet with the current
                        {
                            for( unsigned jj = 0; jj < Candidates.size(); jj++ )
                            {
                                BOARD_CONNECTED_ITEM* item_to_merge = Candidates[jj];

                                if( old_subnet == item_to_merge->GetZoneSubNet() )
                                    item_to_merge->SetZoneSubNet( subnet );
                            }
                        }   // End if ( old_subnet > 0 )
                    }       // End if( connected )
                }

                // End test candidates for the current filled area
                indexstart = indexend + 1;  // prepare test next area, starting at indexend+1 (if exists)
            }                               // End read one area in curr_zone->m_FilledPolysList
        }

        // End read full curr_zone->m_FilledPolysList
    }

    // End read all zones in board
}


/**************************************************************************************************/
void Merge_SubNets_Connected_By_CopperAreas( BOARD* aPcb )
/**************************************************************************************************/

/** Function Merge_SubNets_Connected_By_CopperAreas(BOARD* aPcb)
 * Calls Merge_SubNets_Connected_By_CopperAreas( BOARD* aPcb, int aNetcode ) for each netcode found in zone list
 * @param aPcb = the current board
 */
{
    for( int index = 0; index < aPcb->GetAreaCount(); index++ )
    {
        ZONE_CONTAINER* curr_zone = aPcb->GetArea( index );
        if ( ! curr_zone->IsOnCopperLayer() )
            continue;
        if ( curr_zone->GetNet() <= 0 )
            continue;
        Merge_SubNets_Connected_By_CopperAreas( aPcb, curr_zone->GetNet() );
    }
}


/**************************************************************************************************/
void Merge_SubNets_Connected_By_CopperAreas( BOARD* aPcb, int aNetcode )
/**************************************************************************************************/

/** Function Merge_SubNets_Connected_By_CopperAreas(BOARD* aPcb, int aNetcode)
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
{
    BOARD_CONNECTED_ITEM* item;
    int  old_subnet, subnet, next_subnet_free_number;
    int  old_zone_subnet, zone_subnet;

    // Ensure a zone with the given netcode exists: examine all zones:
    bool found = false;

    for( int index = 0; index < aPcb->GetAreaCount(); index++ )
    {
        ZONE_CONTAINER* curr_zone = aPcb->GetArea( index );
        if( aNetcode == curr_zone->GetNet() )
        {
            found = true;
            break;
        }
    }

    if( !found )  // No zone with this netcode, therefore no connection by zone
        return;

    std::vector <BOARD_CONNECTED_ITEM*> Candidates;  // list of pads and tracks candidates to test.
    // Build a list of candidates connected to the net:
    next_subnet_free_number = 0;
    for( MODULE* module = aPcb->m_Modules;  module;  module = module->Next() )
    {
        for( D_PAD* pad = module->m_Pads; pad != NULL; pad = pad->Next() )
        {
            if( pad->GetNet() == aNetcode )
            {
                Candidates.push_back( pad );
                next_subnet_free_number = MAX( next_subnet_free_number, pad->GetSubNet() );
            }
        }
    }

    for( TRACK* track = aPcb->m_Track; track;  track = track->Next() )
    {
        if( track->GetNet() == aNetcode )
        {
            Candidates.push_back( track );
            next_subnet_free_number = MAX( next_subnet_free_number, track->GetSubNet() );
        }
    }

    if( Candidates.size() == 0 )
        return;

    next_subnet_free_number++;     // This is a subnet we can use with not connected items by tracks, but connected by zone.

    // Sort by zone_subnet:
    sort( Candidates.begin(), Candidates.end(), CmpZoneSubnetValue );

    // Some items can be not connected, but they can be connected to a filled area:
    // give them a subnet common to these items connected only by the area, and not already used.
    // a value like next_subnet_free_number+zone_subnet is right
    for( unsigned jj = 0; jj < Candidates.size(); jj++ )
    {
        item = Candidates[jj];
        if ( item->GetSubNet() == 0 && (item->GetZoneSubNet() > 0) )
        {
            item->SetSubNet( next_subnet_free_number + item->GetZoneSubNet() );
        }
    }


    // Now, for each zone subnet, we search for 2 items with different subnets.
    // if found, the 2 subnet are merged in the whole candidate list.
    old_subnet      = 0;
    old_zone_subnet = 0;

    for( unsigned ii = 0; ii < Candidates.size(); ii++ )
    {
        item = Candidates[ii];
        zone_subnet = item->GetZoneSubNet();

        if( zone_subnet == 0 )  // Not connected by a filled area, skip it
            continue;

        subnet = item->GetSubNet();

        if( zone_subnet != old_zone_subnet )  // a new zone subnet is found
        {
            old_subnet = subnet;
            old_zone_subnet = zone_subnet;
            continue;
        }

        zone_subnet = old_zone_subnet;

        if( subnet == old_subnet )  // 2 successive items already from the same cluster: nothing to do
            continue;

        // Here we have 2 items connected by the same area have 2 differents subnets: merge subnets
        if( (subnet > old_subnet) || ( subnet <= 0) )
            EXCHG( subnet, old_subnet );

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
