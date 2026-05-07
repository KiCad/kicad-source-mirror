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

#include "drc_chain_topology.h"

#include <algorithm>
#include <map>
#include <queue>
#include <stack>
#include <unordered_map>
#include <utility>

#include <board.h>
#include <board_connected_item.h>
#include <footprint.h>
#include <length_delay_calculation/length_delay_calculation.h>
#include <lset.h>
#include <net_chain_bridging.h>
#include <netinfo.h>
#include <pad.h>
#include <pcb_track.h>


namespace
{
// Coalesce coincident graph nodes by quantizing coordinates to this many IU
// (roughly 1 micron at typical pcbnew IU scale).  Track endpoints set by the
// router are pixel-exact so 1 IU is the right tolerance.
constexpr int NODE_COALESCE_EPSILON = 1;


struct NodeKey
{
    int x;
    int y;
    int layer;

    bool operator==( const NodeKey& aOther ) const
    {
        return x == aOther.x && y == aOther.y && layer == aOther.layer;
    }
};


struct NodeKeyHash
{
    std::size_t operator()( const NodeKey& aKey ) const noexcept
    {
        // Mix the three fields into a 64-bit hash; bits are well-distributed
        // for typical pcbnew coordinates.
        std::size_t h = static_cast<std::size_t>( static_cast<uint32_t>( aKey.x ) ) * 0x9E3779B185EBCA87ULL;
        h ^= static_cast<std::size_t>( static_cast<uint32_t>( aKey.y ) ) * 0xC2B2AE3D27D4EB4FULL;
        h ^= static_cast<std::size_t>( aKey.layer ) * 0x165667B19E3779F9ULL;
        return h;
    }
};


struct Edge
{
    int                   nodeA;
    int                   nodeB;
    double                length;
    double                delay;
    BOARD_CONNECTED_ITEM* sourceItem;  // can be null for synthetic split halves
    PCB_LAYER_ID          layer;       // signal layer for stub markers
};


struct Graph
{
    std::vector<VECTOR2I>     nodePos;
    std::vector<PCB_LAYER_ID> nodeLayer;
    std::vector<Edge>         edges;
    std::vector<std::vector<int>> adj;   // node -> list of edge indices

    // Pad anchors: nodes that originated from terminal pads.  Used to tag the
    // chain's two trunk endpoints.
    std::map<PAD*, std::vector<int>> padNodes;

    int addNode( const VECTOR2I& aPos, PCB_LAYER_ID aLayer,
                 std::unordered_map<NodeKey, int, NodeKeyHash>& aIndex )
    {
        int rx = ( aPos.x / NODE_COALESCE_EPSILON ) * NODE_COALESCE_EPSILON;
        int ry = ( aPos.y / NODE_COALESCE_EPSILON ) * NODE_COALESCE_EPSILON;
        NodeKey key { rx, ry, static_cast<int>( aLayer ) };

        auto it = aIndex.find( key );

        if( it != aIndex.end() )
            return it->second;

        int idx = static_cast<int>( nodePos.size() );
        nodePos.emplace_back( aPos );
        nodeLayer.emplace_back( aLayer );
        adj.emplace_back();
        aIndex.emplace( key, idx );
        return idx;
    }

    int addEdge( int aNodeA, int aNodeB, double aLength, double aDelay,
                 BOARD_CONNECTED_ITEM* aSrc, PCB_LAYER_ID aLayer )
    {
        if( aNodeA == aNodeB )
            return -1;

        int idx = static_cast<int>( edges.size() );
        edges.push_back( Edge{ aNodeA, aNodeB, aLength, aDelay, aSrc, aLayer } );
        adj[aNodeA].push_back( idx );
        adj[aNodeB].push_back( idx );
        return idx;
    }
};


double euclidean( const VECTOR2I& a, const VECTOR2I& b )
{
    return ( VECTOR2D( a ) - VECTOR2D( b ) ).EuclideanNorm();
}


// Per-track item delay: derive an IU-per-mm delay from the existing per-item
// length-calculation API.  Falls back to the chain-wide default if the
// per-item query reports no delay (e.g., uninitialized stackup).
double trackDelay( const PCB_TRACK* aTrack, double aLength,
                   LENGTH_DELAY_CALCULATION* aCalc, double aFallbackDelayPerMm )
{
    if( !aCalc || aLength <= 0.0 )
        return 0.0;

    LENGTH_DELAY_CALCULATION_ITEM lengthItem = aCalc->GetLengthCalculationItem( aTrack );

    if( lengthItem.Type() == LENGTH_DELAY_CALCULATION_ITEM::TYPE::UNKNOWN )
        return aFallbackDelayPerMm * aLength / pcbIUScale.IU_PER_MM;

    constexpr PATH_OPTIMISATIONS opts = {
        .OptimiseVias = false, .MergeTracks = false, .OptimiseTracesInPads = false,
        .InferViaInPad = false
    };

    std::vector<LENGTH_DELAY_CALCULATION_ITEM> items{ lengthItem };
    LENGTH_DELAY_STATS stats = aCalc->CalculateLengthDetails(
            items, opts, nullptr, nullptr,
            LENGTH_DELAY_LAYER_OPT::NO_LAYER_DETAIL,
            LENGTH_DELAY_DOMAIN_OPT::WITH_DELAY_DETAIL );

    return static_cast<double>( stats.TrackDelay );
}


// Build per-via per-adjacent-layer edges with apportioned delay per the plan.
// Returns the number of edges added.
int addViaEdges( Graph& aGraph, PCB_VIA* aVia,
                 LENGTH_DELAY_CALCULATION* aCalc,
                 std::unordered_map<NodeKey, int, NodeKeyHash>& aIndex )
{
    LSEQ stack = aVia->GetLayerSet().CuStack();

    if( stack.size() < 2 )
        return 0;

    // Compute total via length and delay once via the per-item query so per-via
    // overrides from GetPropagationDelays() are honored.
    double totalDelay = 0.0;
    double totalLength = 0.0;

    if( aCalc )
    {
        LENGTH_DELAY_CALCULATION_ITEM lengthItem = aCalc->GetLengthCalculationItem( aVia );

        if( lengthItem.Type() != LENGTH_DELAY_CALCULATION_ITEM::TYPE::UNKNOWN )
        {
            constexpr PATH_OPTIMISATIONS opts = {
                .OptimiseVias = false, .MergeTracks = false, .OptimiseTracesInPads = false,
                .InferViaInPad = false
            };

            std::vector<LENGTH_DELAY_CALCULATION_ITEM> items{ lengthItem };
            LENGTH_DELAY_STATS stats = aCalc->CalculateLengthDetails(
                    items, opts, nullptr, nullptr,
                    LENGTH_DELAY_LAYER_OPT::NO_LAYER_DETAIL,
                    LENGTH_DELAY_DOMAIN_OPT::WITH_DELAY_DETAIL );

            totalDelay = static_cast<double>( stats.ViaDelay );
            totalLength = static_cast<double>( stats.ViaLength );
        }
    }

    // If the per-item query reports no via length, fall back to summing
    // StackupHeight across the adjacent-layer pairs and apportion delay
    // equally.
    std::vector<int> heights;
    int              heightSum = 0;

    for( size_t i = 0; i + 1 < stack.size(); ++i )
    {
        int h = aCalc ? aCalc->StackupHeight( stack[i], stack[i + 1] ) : 0;
        heights.push_back( h );
        heightSum += h;
    }

    if( totalLength <= 0.0 )
        totalLength = static_cast<double>( heightSum );

    int edges = 0;

    for( size_t i = 0; i + 1 < stack.size(); ++i )
    {
        int nA = aGraph.addNode( aVia->GetPosition(), stack[i], aIndex );
        int nB = aGraph.addNode( aVia->GetPosition(), stack[i + 1], aIndex );

        double segLen = static_cast<double>( heights[i] );
        double segDelay = 0.0;

        if( totalLength > 0.0 )
            segDelay = totalDelay * ( segLen / totalLength );
        else if( !heights.empty() )
            segDelay = totalDelay / static_cast<double>( heights.size() );  // equal split fallback

        if( aGraph.addEdge( nA, nB, segLen, segDelay, aVia, stack[i] ) >= 0 )
            edges++;
    }

    return edges;
}


// Append nodes for a pad: one per copper layer in its LSET.  Returns the list
// of (layer, nodeIdx) pairs.
std::vector<std::pair<PCB_LAYER_ID, int>>
addPadNodes( Graph& aGraph, PAD* aPad,
             std::unordered_map<NodeKey, int, NodeKeyHash>& aIndex )
{
    std::vector<std::pair<PCB_LAYER_ID, int>> nodes;

    LSET cu = aPad->GetLayerSet() & LSET::AllCuMask();

    for( PCB_LAYER_ID layer : cu.CuStack() )
    {
        int n = aGraph.addNode( aPad->GetCenter(), layer, aIndex );
        nodes.emplace_back( layer, n );
    }

    return nodes;
}


// Returns true if the parametric position t (0..1) is strictly inside the
// open interval (epsilon, 1-epsilon) — i.e. the contact point is on the
// interior of the segment, not at an endpoint.
bool isInteriorParametric( double t )
{
    constexpr double kEdgeFraction = 1e-4;
    return t > kEdgeFraction && t < 1.0 - kEdgeFraction;
}


// Run T-junction detection on the current edge set: for each edge endpoint
// not coincident with another node, check whether it lies on the interior of
// any other track edge on the same layer; if so, split that edge.  Mutates
// `graph` in-place by adding nodes / edges.
void splitTrackTJunctions( Graph& aGraph,
                           std::unordered_map<NodeKey, int, NodeKeyHash>& aIndex )
{
    // Copy the current edge index list because we may mutate edges as we go.
    // We only consider track edges (sourceItem is a PCB_TRACK and not an arc)
    // because mid-segment T-joins on arcs are exotic and outside v1 scope.
    std::vector<int> trackEdges;

    for( int i = 0; i < static_cast<int>( aGraph.edges.size() ); ++i )
    {
        const Edge& e = aGraph.edges[i];

        if( e.sourceItem && e.sourceItem->Type() == PCB_TRACE_T )
            trackEdges.push_back( i );
    }

    for( int endpointEdge : trackEdges )
    {
        for( int endpointNodeSel = 0; endpointNodeSel < 2; ++endpointNodeSel )
        {
            const Edge& srcEdge = aGraph.edges[endpointEdge];
            int         endpointNode =
                    endpointNodeSel == 0 ? srcEdge.nodeA : srcEdge.nodeB;
            VECTOR2I    endpointPos = aGraph.nodePos[endpointNode];
            PCB_LAYER_ID endpointLayer = aGraph.nodeLayer[endpointNode];

            // Check this endpoint against every other track edge on the same
            // layer.  Iterate by index so any newly inserted edges from a
            // previous split are re-considered.
            for( int otherIdx = 0;
                 otherIdx < static_cast<int>( aGraph.edges.size() );
                 ++otherIdx )
            {
                if( otherIdx == endpointEdge )
                    continue;

                Edge& other = aGraph.edges[otherIdx];

                if( !other.sourceItem || other.sourceItem->Type() != PCB_TRACE_T )
                    continue;

                if( other.layer != endpointLayer )
                    continue;

                VECTOR2I aPos = aGraph.nodePos[other.nodeA];
                VECTOR2I bPos = aGraph.nodePos[other.nodeB];

                // Parametric projection of endpoint onto the other segment.
                VECTOR2D ab = VECTOR2D( bPos ) - VECTOR2D( aPos );
                double   len2 = ab.SquaredEuclideanNorm();

                if( len2 <= 0.0 )
                    continue;

                VECTOR2D ap = VECTOR2D( endpointPos ) - VECTOR2D( aPos );
                double   t = ap.Dot( ab ) / len2;

                if( !isInteriorParametric( t ) )
                    continue;

                VECTOR2D projD = VECTOR2D( aPos ) + ab * t;
                VECTOR2I proj { static_cast<int>( std::round( projD.x ) ),
                                static_cast<int>( std::round( projD.y ) ) };

                // Use the track's effective shape (which is width-aware) for
                // the actual contact test — KiCad's connectivity engine does
                // the same (connectivity_algo.cpp's
                // GetEffectiveShape->Collide pattern), so two tracks that
                // touch only by their copper width but not exactly on the
                // centerline are still recognized as a T-junction.  The
                // centerline projection above gave us the candidate split
                // point; Collide gates whether to actually split.
                if( PCB_TRACK* otherTrack =
                            dynamic_cast<PCB_TRACK*>( other.sourceItem ) )
                {
                    std::shared_ptr<SHAPE> shape =
                            otherTrack->GetEffectiveShape( endpointLayer );

                    if( !shape || !shape->Collide( endpointPos, 0 ) )
                    {
                        // Fall back to the centerline epsilon test for cases
                        // where GetEffectiveShape isn't available (synthetic
                        // edges from earlier splits with no source item).
                        if( euclidean( proj, endpointPos ) > NODE_COALESCE_EPSILON )
                            continue;
                    }
                }
                else if( euclidean( proj, endpointPos ) > NODE_COALESCE_EPSILON )
                {
                    continue;
                }

                // Split the other edge at the projected point.  Replace the
                // single Edge with two halves that share a new junction node,
                // each carrying half the length / delay scaled by t.
                double          fullLen = other.length;
                double          fullDelay = other.delay;
                int             oldA = other.nodeA;
                int             oldB = other.nodeB;
                BOARD_CONNECTED_ITEM* oldSrc = other.sourceItem;
                PCB_LAYER_ID    oldLayer = other.layer;

                int junctionNode =
                        aGraph.addNode( proj, endpointLayer, aIndex );

                if( junctionNode == oldA || junctionNode == oldB )
                    continue;  // would have collapsed back; skip

                // Remove the old edge from adjacency lists; we'll rewrite it.
                auto removeFromAdj = [&]( int node, int edgeIdx ) {
                    auto& v = aGraph.adj[node];
                    v.erase( std::remove( v.begin(), v.end(), edgeIdx ),
                             v.end() );
                };

                removeFromAdj( oldA, otherIdx );
                removeFromAdj( oldB, otherIdx );

                // Re-purpose `other` as the first half (oldA -> junction).
                aGraph.edges[otherIdx].nodeA = oldA;
                aGraph.edges[otherIdx].nodeB = junctionNode;
                aGraph.edges[otherIdx].length = fullLen * t;
                aGraph.edges[otherIdx].delay = fullDelay * t;
                aGraph.adj[oldA].push_back( otherIdx );
                aGraph.adj[junctionNode].push_back( otherIdx );

                // Add the second half as a new edge (junction -> oldB).
                aGraph.addEdge( junctionNode, oldB,
                                fullLen * ( 1.0 - t ),
                                fullDelay * ( 1.0 - t ),
                                oldSrc, oldLayer );

                // Connect the original endpoint to the junction explicitly.
                // (The endpoint's edge already terminates at endpointNode and
                // endpointNode == junctionNode after coalescing because they
                // share (x, y, layer); the addNode lookup above should have
                // returned the same index.  If not, we add a zero-length tie
                // to avoid a graph break.)
                if( endpointNode != junctionNode )
                {
                    aGraph.addEdge( endpointNode, junctionNode, 0.0, 0.0,
                                    nullptr, endpointLayer );
                }
            }
        }
    }
}


// Standard DFS that records parent edges and detects cycles by encountering a
// back-edge to a non-parent visited node.  Returns true if a cycle is
// detected.  Fills `parentNode[i]` and `parentEdge[i]` for every reachable
// node (or -1).
bool dfsSpanningTree( const Graph& aGraph, int aRoot,
                      std::vector<int>& aParentNode,
                      std::vector<int>& aParentEdge,
                      std::vector<bool>& aVisited )
{
    aParentNode.assign( aGraph.nodePos.size(), -1 );
    aParentEdge.assign( aGraph.nodePos.size(), -1 );
    aVisited.assign( aGraph.nodePos.size(), false );

    std::stack<std::pair<int, int>> stack;  // (node, parentEdgeIdx)
    stack.push( { aRoot, -1 } );

    bool cycle = false;

    while( !stack.empty() )
    {
        auto [node, fromEdge] = stack.top();
        stack.pop();

        if( aVisited[node] )
            continue;

        aVisited[node] = true;
        aParentEdge[node] = fromEdge;

        if( fromEdge >= 0 )
        {
            const Edge& e = aGraph.edges[fromEdge];
            aParentNode[node] = ( e.nodeA == node ) ? e.nodeB : e.nodeA;
        }

        for( int eIdx : aGraph.adj[node] )
        {
            if( eIdx == fromEdge )
                continue;

            const Edge& e = aGraph.edges[eIdx];
            int         next = ( e.nodeA == node ) ? e.nodeB : e.nodeA;

            if( aVisited[next] )
            {
                cycle = true;
                continue;
            }

            stack.push( { next, eIdx } );
        }
    }

    return cycle;
}

}  // namespace


CHAIN_TOPOLOGY::CHAIN_TOPOLOGY( BOARD* aBoard, const wxString& aChainName,
                                const std::set<BOARD_CONNECTED_ITEM*>& aChainItems ) :
    m_chainName( aChainName )
{
    if( !aBoard || aChainName.IsEmpty() || aChainItems.empty() )
    {
        m_status = STATUS::NO_ITEMS;
        return;
    }

    // Locate the two terminal pads from the first member-net we encounter
    // that has them set.  Per chain semantics every member-net carries the
    // same terminal-pad pair, so reading from any one is sufficient.
    PAD* terminalA = nullptr;
    PAD* terminalB = nullptr;

    for( BOARD_CONNECTED_ITEM* item : aChainItems )
    {
        NETINFO_ITEM* ni = item->GetNet();

        if( !ni )
            continue;

        if( PAD* p0 = ni->GetTerminalPad( 0 ) )
            terminalA = p0;

        if( PAD* p1 = ni->GetTerminalPad( 1 ) )
            terminalB = p1;

        if( terminalA && terminalB )
            break;
    }

    if( !terminalA || !terminalB )
    {
        m_status = STATUS::NO_TERMINAL_PADS;
        return;
    }

    // Build the graph.
    Graph                                          graph;
    std::unordered_map<NodeKey, int, NodeKeyHash> index;
    LENGTH_DELAY_CALCULATION*                      calc = aBoard->GetLengthCalculation();
    double fallbackDelayPerMm = ChainBridgingDelayPerMm( aBoard, m_chainName );

    // Collect pads belonging to chain-member nets — including ones not in
    // aChainItems (terminal pads sit on the chain but might not be in the
    // matched-length sweep's per-net set if they own no routing segments
    // attached to a tracked rule).  We need those pad nodes for the graph
    // anchors regardless.
    std::set<PAD*> seenPads;

    auto registerAllPadLayers = [&]( PAD* pad )
    {
        if( !pad )
            return;

        if( !seenPads.insert( pad ).second )
            return;

        addPadNodes( graph, pad, index );
        // Track which nodes belong to this pad for later terminal lookup.
        std::vector<int>& list = graph.padNodes[pad];
        list.clear();
        LSET cu = pad->GetLayerSet() & LSET::AllCuMask();

        for( PCB_LAYER_ID layer : cu.CuStack() )
        {
            int n = graph.addNode( pad->GetCenter(), layer, index );
            list.push_back( n );
        }
    };

    registerAllPadLayers( terminalA );
    registerAllPadLayers( terminalB );

    // Edges from tracks, arcs, vias, and pads in the matched item set.
    for( BOARD_CONNECTED_ITEM* item : aChainItems )
    {
        if( !item )
            continue;

        switch( item->Type() )
        {
        case PCB_TRACE_T:
        case PCB_ARC_T:
        {
            PCB_TRACK* track = static_cast<PCB_TRACK*>( item );
            int        nA = graph.addNode( track->GetStart(), track->GetLayer(), index );
            int        nB = graph.addNode( track->GetEnd(), track->GetLayer(), index );
            double     len = track->GetLength();
            double     dly = trackDelay( track, len, calc, fallbackDelayPerMm );
            graph.addEdge( nA, nB, len, dly, track, track->GetLayer() );
            break;
        }
        case PCB_VIA_T:
        {
            PCB_VIA* via = static_cast<PCB_VIA*>( item );
            addViaEdges( graph, via, calc, index );
            break;
        }
        case PCB_PAD_T:
        {
            PAD* pad = static_cast<PAD*>( item );
            registerAllPadLayers( pad );
            break;
        }
        default:
            break;
        }
    }

    // Bridge edges through series passives.
    std::vector<CHAIN_BRIDGE> bridges = EnumerateChainBridges( aBoard, m_chainName );

    for( const CHAIN_BRIDGE& br : bridges )
    {
        registerAllPadLayers( br.padA );
        registerAllPadLayers( br.padB );

        // Pick the bridge's anchor layer: footprint placement layer for SMD
        // pads, topmost shared copper layer for THT.
        FOOTPRINT* fp = br.padA->GetParentFootprint();
        PCB_LAYER_ID anchorLayer = fp ? fp->GetLayer() : F_Cu;

        LSET commonCu = ( br.padA->GetLayerSet() & br.padB->GetLayerSet() ) & LSET::AllCuMask();

        if( !commonCu.test( anchorLayer ) )
        {
            LSEQ stack = commonCu.CuStack();
            if( !stack.empty() )
                anchorLayer = stack[0];
            else
                continue;
        }

        int nA = graph.addNode( br.padA->GetCenter(), anchorLayer, index );
        int nB = graph.addNode( br.padB->GetCenter(), anchorLayer, index );
        graph.addEdge( nA, nB, br.length, br.delay, nullptr, anchorLayer );
    }

    // T-junction split pass on the populated graph.
    splitTrackTJunctions( graph, index );

    // Tag terminal nodes — pick any layer in the pad's LSET (the pad has zero
    // intrinsic length so any anchor layer works for path queries; tracks
    // terminating at the pad on layer L will share the same (x, y, L) node).
    auto firstPadNode = []( const std::vector<int>& nodes ) -> int
    {
        return nodes.empty() ? -1 : nodes.front();
    };

    int rootA = firstPadNode( graph.padNodes[terminalA] );
    int rootB = firstPadNode( graph.padNodes[terminalB] );

    if( rootA < 0 || rootB < 0 )
    {
        m_status = STATUS::NO_TERMINAL_PADS;
        return;
    }

    // Bridge zero-length edges between every pair of pad-anchor layers so
    // that a pad on multiple layers (e.g. THT) is one logical graph node.
    auto stitchPadLayers = [&]( PAD* aPad )
    {
        const std::vector<int>& nodes = graph.padNodes[aPad];

        for( size_t i = 1; i < nodes.size(); ++i )
            graph.addEdge( nodes[0], nodes[i], 0.0, 0.0, nullptr,
                           graph.nodeLayer[nodes[i]] );
    };

    for( auto& [pad, nodes] : graph.padNodes )
        stitchPadLayers( pad );

    // DFS spanning tree from terminalA.
    std::vector<int>  parentNode;
    std::vector<int>  parentEdge;
    std::vector<bool> visited;

    bool cycle = dfsSpanningTree( graph, rootA, parentNode, parentEdge, visited );

    if( cycle )
    {
        m_status = STATUS::CYCLE_DETECTED;
        return;
    }

    if( !visited[rootB] )
    {
        m_status = STATUS::DISCONNECTED;
        return;
    }

    // Recover trunk: walk from rootB back to rootA via parent pointers.
    std::vector<int> trunkEdges;
    std::vector<int> trunkNodes;
    {
        int cur = rootB;

        while( cur != rootA && cur >= 0 )
        {
            trunkNodes.push_back( cur );
            int e = parentEdge[cur];

            if( e < 0 )
                break;

            trunkEdges.push_back( e );
            cur = parentNode[cur];
        }

        if( cur != rootA )
        {
            m_status = STATUS::DISCONNECTED;
            return;
        }

        trunkNodes.push_back( rootA );
    }

    // Sum trunk length / delay.
    m_trunkLength = 0.0;
    m_trunkDelay = 0.0;

    for( int eIdx : trunkEdges )
    {
        m_trunkLength += graph.edges[eIdx].length;
        m_trunkDelay += graph.edges[eIdx].delay;
    }

    // Stub identification: BFS from each non-trunk node not yet on a stub
    // component, following non-trunk edges only, and find the unique
    // trunk-touching node.
    std::set<int> trunkNodeSet( trunkNodes.begin(), trunkNodes.end() );
    std::set<int> trunkEdgeSet( trunkEdges.begin(), trunkEdges.end() );
    std::vector<bool> nodeInStubComponent( graph.nodePos.size(), false );

    for( int startNode = 0;
         startNode < static_cast<int>( graph.nodePos.size() );
         ++startNode )
    {
        if( !visited[startNode] )
            continue;
        if( trunkNodeSet.count( startNode ) )
            continue;
        if( nodeInStubComponent[startNode] )
            continue;

        // BFS this stub component (via edges *not* in trunkEdgeSet) and
        // collect all of its nodes + items + length/delay + every trunk node
        // it touches.
        std::set<int>                       compNodes;
        std::set<int>                       compEdges;
        std::set<int>                       trunkTouches;
        std::vector<BOARD_CONNECTED_ITEM*>   items;
        double                              compLength = 0.0;
        double                              compDelay = 0.0;
        PCB_LAYER_ID                        anyLayer = UNDEFINED_LAYER;

        std::queue<int> q;
        q.push( startNode );
        compNodes.insert( startNode );

        while( !q.empty() )
        {
            int n = q.front();
            q.pop();

            for( int eIdx : graph.adj[n] )
            {
                const Edge& e = graph.edges[eIdx];
                int         other = ( e.nodeA == n ) ? e.nodeB : e.nodeA;

                if( trunkEdgeSet.count( eIdx ) )
                {
                    // Edge into the trunk — only record the touch.  The
                    // edge itself belongs to the trunk and is not part of
                    // the stub component.
                    if( trunkNodeSet.count( other ) )
                        trunkTouches.insert( other );

                    continue;
                }

                // Accumulate this stub edge exactly once across the BFS.
                // Tracking visited edges (not visited nodes) is the right
                // primitive: nothing about traversal order can correctly be
                // inferred from node-index comparisons because BFS may
                // reach a higher-index node from a lower one.
                if( compEdges.insert( eIdx ).second )
                {
                    compLength += e.length;
                    compDelay += e.delay;

                    if( e.sourceItem )
                        items.push_back( e.sourceItem );

                    if( anyLayer == UNDEFINED_LAYER )
                        anyLayer = e.layer;
                }

                if( trunkNodeSet.count( other ) )
                {
                    trunkTouches.insert( other );
                    continue;
                }

                if( compNodes.insert( other ).second )
                    q.push( other );
            }
        }

        for( int n : compNodes )
            nodeInStubComponent[n] = true;

        if( trunkTouches.empty() )
        {
            // Floating copper — DRCE_UNCONNECTED_ITEMS will catch it.
            continue;
        }

        if( trunkTouches.size() >= 2 )
        {
            // Two or more trunk-touching points imply a cycle that the
            // earlier DFS missed.  Defensive bail.
            m_status = STATUS::CYCLE_DETECTED;
            return;
        }

        int branchNode = *trunkTouches.begin();

        STUB stub;
        stub.branchPoint = graph.nodePos[branchNode];
        stub.branchLayer = anyLayer != UNDEFINED_LAYER ? anyLayer
                                                       : graph.nodeLayer[branchNode];
        stub.items = std::move( items );
        stub.length = compLength;
        stub.delay = compDelay;

        m_stubs.push_back( std::move( stub ) );
    }

    m_status = STATUS::OK;
}
