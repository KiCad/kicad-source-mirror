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
#include <set>
#include <tuple>
#include <vector>

#include <wx/string.h>

class BOARD;
class FOOTPRINT;
class PAD;


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
double FootprintChainBridgingLength( const FOOTPRINT* aFootprint, const wxString& aNetChain );


/**
 * Sum chain bridging length across every footprint on the board.
 */
double BoardChainBridgingLength( const BOARD* aBoard, const wxString& aNetChain );


/**
 * Pick a single per-IU-per-mm delay for a given chain.  Walks the chain's tracks until it
 * finds one with a measurable per-mm propagation delay; falls back to the default constant
 * if none.  Shared between the aggregate and per-edge bridge helpers so they always
 * apply the same scaling.
 */
double ChainBridgingDelayPerMm( const BOARD* aBoard, const wxString& aNetChain );


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
std::tuple<double, double> BoardChainBridging( const BOARD* aBoard, const wxString& aNetChain );


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
 * Enumerate every per-pad-pair bridge edge contributed by every footprint on the board to
 * the named chain.  Unlike FootprintChainBridgingLength (which returns a single max-span
 * scalar per footprint), this returns one edge per qualifying cross-net pad pair, which is
 * what the CHAIN_TOPOLOGY graph builder needs to compute trunk paths through series
 * passives.
 */
std::vector<CHAIN_BRIDGE> EnumerateChainBridges( const BOARD* aBoard, const wxString& aNetChain );


/**
 * Status returned by PartitionNetChainAroundNet().
 */
enum class NET_CHAIN_PARTITION_STATUS
{
    OK,
    INVALID_INPUT,            // null board, equal pads, etc.
    QUERY_NET_NOT_IN_CHAIN,   // aQueryNet has no chain assignment
    START_PAD_NOT_ON_QUERY,   // aStartPad's netcode != aQueryNet
    END_PAD_NOT_ON_QUERY,
    NO_CHAIN_BRIDGES,         // the query net has no series-passive bridge in the chain
    AMBIGUOUS_OVERLAP         // the two sides share at least one netcode (cycle)
};


/**
 * Result of PartitionNetChainAroundNet().
 *
 * On AMBIGUOUS_OVERLAP both sets are populated so the caller can inspect the
 * overlap; on all other non-OK statuses both sets are empty.
 */
struct NET_CHAIN_PARTITION
{
    NET_CHAIN_PARTITION_STATUS status = NET_CHAIN_PARTITION_STATUS::INVALID_INPUT;
    std::set<int>              beforeStart;
    std::set<int>              afterEnd;
};


/**
 * Partition the chain containing @p aQueryNet around it, cut at the bridges incident on
 * @p aStartPad and @p aEndPad.
 *
 * The chain bridge graph (nodes = chain netcodes, edges = CHAIN_BRIDGE) is built from
 * EnumerateChainBridges() with aQueryNet's node and every incident edge removed.  Multi-
 * source BFS seeds from every non-query bridge neighbor of aStartPad to fill
 * `beforeStart`, and from every non-query bridge neighbor of aEndPad to fill `afterEnd`.
 * Footprints that place several chain pads on distinct nets therefore contribute every
 * such neighbor on the seeded side, not just one.
 *
 * If aQueryNet is the chain terminal, one side may legitimately be empty; status is
 * still OK.  If the two sets overlap, the chain has a cycle that does not pass through
 * aQueryNet and the partition is ambiguous; both sets are still populated and status is
 * AMBIGUOUS_OVERLAP.
 *
 * Multi-pad parts (transformers, beads) are represented by EnumerateChainBridges() as a
 * complete cross-net pad-pair graph per footprint.  When the chain spans more than two
 * nets through one footprint the resulting clique is treated as fully connected, which
 * is consistent with how the DRC trunk-length topology consumes the same edges.
 */
NET_CHAIN_PARTITION PartitionNetChainAroundNet( const BOARD* aBoard, int aQueryNet,
                                                const PAD* aStartPad, const PAD* aEndPad );


#endif // PCBNEW_NET_CHAIN_BRIDGING_H
