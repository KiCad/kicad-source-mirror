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

#ifndef PCBNEW_NET_CHAIN_BRIDGING_H
#define PCBNEW_NET_CHAIN_BRIDGING_H

#include <algorithm>
#include <limits>
#include <tuple>
#include <vector>

#include <wx/string.h>

#include <base_units.h>
#include <board.h>
#include <footprint.h>
#include <math/vector2d.h>
#include <netinfo.h>
#include <pad.h>
#include <pcb_track.h>


constexpr double DEFAULT_PROPAGATION_DELAY_PS_PER_MM = 5.9;  // 150 ps/in fallback when no track delay sample is available


/**
 * Compute the bridging length contributed by a single footprint to a net chain.
 *
 * A footprint bridges two segments of the same chain when it has at least one chain pad
 * on each of two distinct nets.  The bridging contribution is the maximum pairwise
 * Euclidean distance between any two chain pads on different nets.  This is a
 * conservative upper bound that handles 3+ pad devices (centre-tap beads, transformers,
 * dual-winding parts) without silently dropping their contribution.
 *
 * Returns 0.0 when the footprint has fewer than two chain pads, or when all chain pads
 * share the same netcode (no cross-net bridging possible).
 *
 * The DRC matched-length provider and the tuning pattern generator share this predicate
 * so they always agree on which footprints bridge a chain and by how much.
 */
inline double FootprintChainBridgingLength( const FOOTPRINT* aFootprint, const wxString& aNetChain )
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


/**
 * Sum chain bridging length across every footprint on the board.
 */
inline double BoardChainBridgingLength( const BOARD* aBoard, const wxString& aNetChain )
{
    if( !aBoard || aNetChain.IsEmpty() )
        return 0.0;

    double total = 0.0;

    for( const FOOTPRINT* fp : aBoard->Footprints() )
        total += FootprintChainBridgingLength( fp, aNetChain );

    return total;
}


/**
 * Compute both the chain bridging length and its associated propagation delay (in internal
 * delay IU, i.e. attoseconds) in one pass.  The delay is derived from the first chain track
 * with a measurable per-mm propagation delay so the bridging contribution tracks the actual
 * stackup; if no such track exists the helper falls back to ~5.9 ps/mm (150 ps/in).
 *
 * The matched-length DRC provider and the tuning pattern generator share this helper so the
 * time-domain DRC verdict and the tuner's per-net budget agree on bridging delay.  The
 * returned tuple is (lengthIU, delayIU).
 */
// Forward declaration; defined below.
inline double ChainBridgingDelayPerMm( const BOARD* aBoard, const wxString& aNetChain );


inline std::tuple<double, double> BoardChainBridging( const BOARD* aBoard, const wxString& aNetChain )
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


/**
 * Saturating subtract for bridge-adjusting MINOPTMAX<int> bounds without overflow when the
 * delta is in attoseconds (time-domain bridging routinely exceeds INT_MAX).  Returns
 * max(0, aValue - aDelta), clamped to fit in int.
 */
inline int SubtractBridgingClamped( int aValue, long long aDelta )
{
    long long adjusted = static_cast<long long>( aValue ) - aDelta;

    return static_cast<int>( std::clamp( adjusted,
                                         0LL,
                                         static_cast<long long>( std::numeric_limits<int>::max() ) ) );
}


/**
 * One series-component bridge edge inside a chain graph.
 *
 * EnumerateChainBridges returns one CHAIN_BRIDGE per qualifying pad pair on a footprint
 * (rather than collapsing to the maximum span the way FootprintChainBridgingLength does).
 * The CHAIN_TOPOLOGY graph builder consumes these as edges so it can route the trunk path
 * through every series passive in the chain.
 */
struct CHAIN_BRIDGE
{
    PAD*   padA;
    PAD*   padB;
    double length;
    double delay;
};


/**
 * Pick a single per-IU-per-mm delay for a given chain.  Walks the chain's tracks until it
 * finds one with a measurable per-mm propagation delay; falls back to the default constant
 * if none.  Shared between the aggregate and per-edge bridge helpers so they always
 * apply the same scaling.
 */
inline double ChainBridgingDelayPerMm( const BOARD* aBoard, const wxString& aNetChain )
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


/**
 * Enumerate every per-pad-pair bridge edge contributed by every footprint on the board to
 * the named chain.  Unlike FootprintChainBridgingLength (which returns a single max-span
 * scalar per footprint), this returns one edge per qualifying cross-net pad pair, which is
 * what the CHAIN_TOPOLOGY graph builder needs to compute trunk paths through series
 * passives.
 */
inline std::vector<CHAIN_BRIDGE>
EnumerateChainBridges( const BOARD* aBoard, const wxString& aNetChain )
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


#endif // PCBNEW_NET_CHAIN_BRIDGING_H
