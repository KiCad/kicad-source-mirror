/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
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
 */

#include <net_chain_bridging.h>

#include <queue>
#include <unordered_map>

#include <base_units.h>
#include <board.h>
#include <footprint.h>
#include <math/vector2d.h>
#include <netinfo.h>
#include <pad.h>
#include <pcb_track.h>


double FootprintChainBridgingLength( const FOOTPRINT* aFootprint, const wxString& aNetChain )
{
    if( !aFootprint || aNetChain.IsEmpty() )
        return 0.0;

    std::vector<const PAD*> chainPads;

    for( const PAD* pad : aFootprint->Pads() )
    {
        const NETINFO_ITEM* pn = pad->GetNet();

        if( !pn || pn->GetNetChain() != aNetChain )
            continue;

        chainPads.push_back( pad );
    }

    if( chainPads.size() < 2 )
        return 0.0;

    int firstNet = chainPads.front()->GetNetCode();

    auto crossesNet = [firstNet]( const PAD* p ) { return p->GetNetCode() != firstNet; };

    if( !std::any_of( chainPads.begin() + 1, chainPads.end(), crossesNet ) )
        return 0.0;

    double maxSpan = 0.0;

    for( size_t i = 0; i < chainPads.size(); ++i )
    {
        for( size_t j = i + 1; j < chainPads.size(); ++j )
        {
            if( chainPads[i]->GetNetCode() == chainPads[j]->GetNetCode() )
                continue;

            VECTOR2D delta = VECTOR2D( chainPads[i]->GetCenter() )
                           - VECTOR2D( chainPads[j]->GetCenter() );
            maxSpan = std::max( maxSpan, delta.EuclideanNorm() );
        }
    }

    return maxSpan;
}


double BoardChainBridgingLength( const BOARD* aBoard, const wxString& aNetChain )
{
    if( !aBoard || aNetChain.IsEmpty() )
        return 0.0;

    double total = 0.0;

    for( const FOOTPRINT* fp : aBoard->Footprints() )
        total += FootprintChainBridgingLength( fp, aNetChain );

    return total;
}


double ChainBridgingDelayPerMm( const BOARD* aBoard, const wxString& aNetChain )
{
    double delayIUPerMm = DEFAULT_PROPAGATION_DELAY_PS_PER_MM * pcbIUScale.IU_PER_PS;

    if( !aBoard || aNetChain.IsEmpty() )
        return delayIUPerMm;

    for( const PCB_TRACK* track : aBoard->Tracks() )
    {
        const NETINFO_ITEM* ninfo = track->GetNet();

        if( !ninfo || ninfo->GetNetChain() != aNetChain )
            continue;

        double tLen = 0.0;
        double tDelay = 0.0;

        std::tie( std::ignore, tLen, std::ignore, tDelay, std::ignore ) =
                aBoard->GetTrackLength( *track );

        if( tLen > 0.0 && tDelay > 0.0 )
        {
            delayIUPerMm = tDelay / ( tLen / pcbIUScale.IU_PER_MM );
            break;
        }
    }

    return delayIUPerMm;
}


std::tuple<double, double> BoardChainBridging( const BOARD* aBoard, const wxString& aNetChain )
{
    if( !aBoard || aNetChain.IsEmpty() )
        return { 0.0, 0.0 };

    double lengthIU = BoardChainBridgingLength( aBoard, aNetChain );

    if( lengthIU <= 0.0 )
        return { 0.0, 0.0 };

    double delayIU = ChainBridgingDelayPerMm( aBoard, aNetChain ) * lengthIU
                     / pcbIUScale.IU_PER_MM;

    return { lengthIU, delayIU };
}


std::vector<CHAIN_BRIDGE> EnumerateChainBridges( const BOARD* aBoard, const wxString& aNetChain )
{
    std::vector<CHAIN_BRIDGE> bridges;

    if( !aBoard || aNetChain.IsEmpty() )
        return bridges;

    const double delayIUPerMm = ChainBridgingDelayPerMm( aBoard, aNetChain );

    for( FOOTPRINT* fp : aBoard->Footprints() )
    {
        if( !fp )
            continue;

        std::vector<PAD*> chainPads;

        for( PAD* pad : fp->Pads() )
        {
            const NETINFO_ITEM* pn = pad->GetNet();

            if( pn && pn->GetNetChain() == aNetChain )
                chainPads.push_back( pad );
        }

        if( chainPads.size() < 2 )
            continue;

        for( size_t i = 0; i < chainPads.size(); ++i )
        {
            for( size_t j = i + 1; j < chainPads.size(); ++j )
            {
                if( chainPads[i]->GetNetCode() == chainPads[j]->GetNetCode() )
                    continue;

                VECTOR2D delta = VECTOR2D( chainPads[i]->GetCenter() )
                               - VECTOR2D( chainPads[j]->GetCenter() );
                double   length = delta.EuclideanNorm();
                double   delay = delayIUPerMm * length / pcbIUScale.IU_PER_MM;

                bridges.push_back( CHAIN_BRIDGE{ chainPads[i], chainPads[j], length, delay } );
            }
        }
    }

    return bridges;
}


NET_CHAIN_PARTITION PartitionNetChainAroundNet( const BOARD* aBoard, int aQueryNet,
                                                const PAD* aStartPad, const PAD* aEndPad )
{
    NET_CHAIN_PARTITION result;

    if( !aBoard || !aStartPad || !aEndPad || aStartPad == aEndPad )
    {
        result.status = NET_CHAIN_PARTITION_STATUS::INVALID_INPUT;
        return result;
    }

    // Pads must live on this board.  Cross-board references would silently produce
    // incorrect partitions, since EnumerateChainBridges() walks aBoard's footprints.
    if( aStartPad->GetBoard() != aBoard || aEndPad->GetBoard() != aBoard )
    {
        result.status = NET_CHAIN_PARTITION_STATUS::INVALID_INPUT;
        return result;
    }

    NETINFO_ITEM* queryNetInfo = aBoard->FindNet( aQueryNet );

    if( !queryNetInfo || queryNetInfo->GetNetChain().IsEmpty() )
    {
        result.status = NET_CHAIN_PARTITION_STATUS::QUERY_NET_NOT_IN_CHAIN;
        return result;
    }

    // Compare by NETINFO pointer rather than netcode so a pad with a stale netcode (e.g.
    // copied from another board with the same code) is rejected.
    if( aStartPad->GetNet() != queryNetInfo )
    {
        result.status = NET_CHAIN_PARTITION_STATUS::START_PAD_NOT_ON_QUERY;
        return result;
    }

    if( aEndPad->GetNet() != queryNetInfo )
    {
        result.status = NET_CHAIN_PARTITION_STATUS::END_PAD_NOT_ON_QUERY;
        return result;
    }

    const wxString&           chainName = queryNetInfo->GetNetChain();
    std::vector<CHAIN_BRIDGE> bridges = EnumerateChainBridges( aBoard, chainName );

    // Cut-graph adjacency: every chain bridge becomes an undirected edge between its
    // two pads' netcodes, except edges incident on aQueryNet.  Removing aQueryNet
    // effectively partitions the chain at the inspected net.
    std::unordered_map<int, std::set<int>> adjacency;

    // Multi-pad parts can contribute several cross-net neighbors for one endpoint pad;
    // seed every such neighbor so BFS reaches all of them.
    std::set<int> startSeeds;
    std::set<int> endSeeds;
    bool          queryNetHasBridge = false;

    auto seedIfEndpoint = [&]( const PAD* aBridgePad, int aNeighborNet,
                               const PAD* aQueryPad, std::set<int>& aSeeds )
    {
        if( aBridgePad == aQueryPad && aNeighborNet != aQueryNet )
            aSeeds.insert( aNeighborNet );
    };

    for( const CHAIN_BRIDGE& br : bridges )
    {
        if( !br.padA || !br.padB )
            continue;

        int netA = br.padA->GetNetCode();
        int netB = br.padB->GetNetCode();

        if( netA == aQueryNet || netB == aQueryNet )
        {
            queryNetHasBridge = true;
            seedIfEndpoint( br.padA, netB, aStartPad, startSeeds );
            seedIfEndpoint( br.padB, netA, aStartPad, startSeeds );
            seedIfEndpoint( br.padA, netB, aEndPad, endSeeds );
            seedIfEndpoint( br.padB, netA, aEndPad, endSeeds );
            continue;
        }

        adjacency[netA].insert( netB );
        adjacency[netB].insert( netA );
    }

    // Distinguish "chain has nothing to partition" from "query net is unbridged in this
    // chain".  Both produce empty answers but mean different things to the caller.
    if( !queryNetHasBridge )
    {
        result.status = NET_CHAIN_PARTITION_STATUS::NO_CHAIN_BRIDGES;
        return result;
    }

    auto bfsFill = [&]( const std::set<int>& aSeeds, std::set<int>& aOut )
    {
        std::queue<int> frontier;

        for( int seed : aSeeds )
        {
            if( aOut.insert( seed ).second )
                frontier.push( seed );
        }

        while( !frontier.empty() )
        {
            int cur = frontier.front();
            frontier.pop();

            auto it = adjacency.find( cur );

            if( it == adjacency.end() )
                continue;

            for( int nbr : it->second )
            {
                if( aOut.insert( nbr ).second )
                    frontier.push( nbr );
            }
        }
    };

    bfsFill( startSeeds, result.beforeStart );
    bfsFill( endSeeds, result.afterEnd );

    for( int net : result.beforeStart )
    {
        if( result.afterEnd.count( net ) )
        {
            result.status = NET_CHAIN_PARTITION_STATUS::AMBIGUOUS_OVERLAP;
            return result;
        }
    }

    result.status = NET_CHAIN_PARTITION_STATUS::OK;
    return result;
}
