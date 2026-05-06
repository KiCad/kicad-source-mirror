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
#include <vector>

#include <wx/string.h>

#include <board.h>
#include <footprint.h>
#include <math/vector2d.h>
#include <netinfo.h>
#include <pad.h>


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


#endif // PCBNEW_NET_CHAIN_BRIDGING_H
