/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright The KiCad Developers.
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

#include <drc/drc_creepage_engine.h>

#include <board.h>
#include <core/kicad_algo.h>
#include <footprint.h>
#include <netinfo.h>
#include <pad.h>


CREEPAGE_ENGINE::CREEPAGE_ENGINE( BOARD& aBoard ) :
        m_board( aBoard )
{
}


void CREEPAGE_ENGINE::populateBoardEdgeGraph( CREEPAGE_GRAPH& aGraph, SHAPE_POLY_SET& aOutline )
{
    aGraph.m_minGrooveWidth = m_minGrooveWidth;

    // Subtract NPTH holes from the outline so paths through slot interiors are rejected (#24286)
    bool hasOutline = m_board.GetBoardPolygonOutlines( aOutline, false, nullptr, false, true );

    BuildCreepageBoardEdges( m_board, aGraph.m_boardEdge, aGraph.m_ownedBoardEdges, nullptr );
    aGraph.m_boardOutline = hasOutline ? &aOutline : nullptr;

    aGraph.TransformEdgeToCreepShapes();
    aGraph.RemoveDuplicatedShapes();
    aGraph.TransformCreepShapesToNodes( aGraph.m_shapeCollection );
}


void CREEPAGE_ENGINE::connectChildren( CREEPAGE_GRAPH& aGraph )
{
    std::vector<std::shared_ptr<GRAPH_NODE>> temp_nodes;

    std::copy_if( aGraph.m_nodes.begin(), aGraph.m_nodes.end(), std::back_inserter( temp_nodes ),
                  []( const std::shared_ptr<GRAPH_NODE>& aNode )
                  {
                      return !!aNode && aNode->m_parent && !aNode->m_parent->IsConductive()
                             && !aNode->m_connectDirectly && aNode->m_type == GRAPH_NODE::POINT;
                  } );

    alg::for_all_pairs( temp_nodes.begin(), temp_nodes.end(),
                        [&]( const std::shared_ptr<GRAPH_NODE>& aN1,
                             const std::shared_ptr<GRAPH_NODE>& aN2 )
                        {
                            if( aN1 == aN2 || !aN1 || !aN2 )
                                return;

                            if( !aN1->m_parent || !aN2->m_parent )
                                return;

                            if( aN1->m_parent != aN2->m_parent )
                                return;

                            aN1->m_parent->ConnectChildren( const_cast<std::shared_ptr<GRAPH_NODE>&>( aN1 ),
                                                            const_cast<std::shared_ptr<GRAPH_NODE>&>( aN2 ),
                                                            aGraph );
                        } );
}


std::optional<CREEPAGE_RESULT>
CREEPAGE_ENGINE::extractResult( const std::vector<std::shared_ptr<GRAPH_CONNECTION>>& aPath,
                                double aDistance, int aNetA, int aNetB, double aConstraint,
                                int aNearMargin )
{
    if( aPath.empty() || aPath.size() < 4 )
        return std::nullopt;

    // aNearMargin == 0 reproduces the legacy "distance < constraint" test exactly; the overlay
    // passes a positive margin to also surface near misses
    if( !( aDistance - aConstraint < aNearMargin ) )
        return std::nullopt;

    CREEPAGE_RESULT result;
    result.m_netA = aNetA;
    result.m_netB = aNetB;
    result.m_distance = aDistance;
    result.m_constraint = aConstraint;
    result.m_violation = aDistance < aConstraint;

    std::shared_ptr<GRAPH_CONNECTION> gc1 = aPath[1];
    std::shared_ptr<GRAPH_CONNECTION> gc2 = aPath[aPath.size() - 2];

    result.m_start = gc1->m_path.a2;
    result.m_end = gc2->m_path.a2;

    // Connections [1] and [size-2] sit on real shape nodes; the path ends are zero-weight links
    // to the per-net virtual nodes
    if( gc1->n1 && gc1->n1->m_parent )
        result.m_itemA = gc1->n1->m_parent->GetParent();

    if( gc2->n2 && gc2->n2->m_parent )
        result.m_itemB = gc2->n2->m_parent->GetParent();

    for( const std::shared_ptr<GRAPH_CONNECTION>& gc : aPath )
        gc->GetShapes( result.m_path );

    return result;
}


std::optional<CREEPAGE_RESULT> CREEPAGE_ENGINE::SolveNetPairWholeBoard( int aNetA, int aNetB,
                                                                       PCB_LAYER_ID aLayer,
                                                                       double aConstraint )
{
    if( aConstraint <= 0 )
        return std::nullopt;

    NETINFO_ITEM* netA = m_board.FindNet( aNetA );
    NETINFO_ITEM* netB = m_board.FindNet( aNetB );

    if( !netA || !netB )
        return std::nullopt;

    if( netA->GetBoundingBox().Distance( netB->GetBoundingBox() ) > aConstraint )
        return std::nullopt;

    CREEPAGE_GRAPH graph( m_board );
    SHAPE_POLY_SET outline;
    populateBoardEdgeGraph( graph, outline );

    graph.GeneratePaths( aConstraint, Edge_Cuts );
    graph.SetTarget( aConstraint );

    std::shared_ptr<GRAPH_NODE> nodeA = graph.AddNetElements( aNetA, aLayer, aConstraint );
    std::shared_ptr<GRAPH_NODE> nodeB = graph.AddNetElements( aNetB, aLayer, aConstraint );

    graph.GeneratePaths( aConstraint, aLayer );

    connectChildren( graph );

    std::vector<std::shared_ptr<GRAPH_CONNECTION>> shortestPath;
    double distance = graph.Solve( nodeA, nodeB, shortestPath );

    return extractResult( shortestPath, distance, aNetA, aNetB, aConstraint, 0 );
}


std::map<int, std::shared_ptr<GRAPH_NODE>> CREEPAGE_ENGINE::addNetsInRegion( const BOX2I& aRegion )
{
    std::map<int, std::shared_ptr<GRAPH_NODE>> added;

    // Iterate in net-code order so node creation order matches the whole-board solve
    for( const auto& [netCode, net] : m_board.GetNetInfo().NetsByNetcode() )
    {
        if( !net || netCode <= 0 )
            continue;

        if( !m_affectedNets.count( netCode ) )
        {
            auto it = m_staticNetBBoxes.find( netCode );

            if( it == m_staticNetBBoxes.end() || !it->second.Intersects( aRegion ) )
                continue;
        }

        added[netCode] = m_graph->AddNetElements( netCode, m_layer, m_margin );
    }

    return added;
}


void CREEPAGE_ENGINE::BeginInteractive( PCB_LAYER_ID aLayer, const std::set<int>& aAffectedNets,
                                        const std::set<const BOARD_ITEM*>& aMovingItems, int aMargin,
                                        std::function<double( int, int )> aConstraintFn )
{
    m_interactive = true;
    m_layer = aLayer;
    m_affectedNets = aAffectedNets;
    m_movingItems = aMovingItems;
    m_constraintFn = std::move( aConstraintFn );
    m_margin = aMargin;

    m_staticNetBBoxes.clear();

    for( const auto& [netCode, net] : m_board.GetNetInfo().NetsByNetcode() )
    {
        if( net && netCode > 0 && !m_affectedNets.count( netCode ) )
            m_staticNetBBoxes.emplace( netCode, net->GetBoundingBox() );
    }

    m_dynamicEdges = movingItemsHaveNPTH();

    buildBoardEdgePrefix();
}


void CREEPAGE_ENGINE::buildBoardEdgePrefix()
{
    m_graph = std::make_unique<CREEPAGE_GRAPH>( m_board );
    m_outline = std::make_unique<SHAPE_POLY_SET>();
    populateBoardEdgeGraph( *m_graph, *m_outline );
    m_graph->SetTarget( m_margin );

    // The board-edge sub-graph is pure board geometry, independent of any track, so it is the
    // cacheable prefix.  Copper-layer paths depend on every track through the collision test and
    // must be rebuilt each frame.
    m_graph->GeneratePaths( m_margin, Edge_Cuts );
    connectChildren( *m_graph );

    m_staticNodeCount = m_graph->m_nodes.size();
    m_staticConnCount = m_graph->m_connections.size();
}


bool CREEPAGE_ENGINE::movingItemsHaveNPTH() const
{
    auto isNPTH = []( const PAD* aPad )
    {
        return aPad && aPad->GetAttribute() == PAD_ATTRIB::NPTH;
    };

    for( const BOARD_ITEM* item : m_movingItems )
    {
        if( !item )
            continue;

        if( item->Type() == PCB_PAD_T && isNPTH( static_cast<const PAD*>( item ) ) )
            return true;

        if( item->Type() == PCB_FOOTPRINT_T )
        {
            for( const PAD* pad : static_cast<const FOOTPRINT*>( item )->Pads() )
            {
                if( isNPTH( pad ) )
                    return true;
            }
        }
    }

    return false;
}


std::vector<CREEPAGE_RESULT> CREEPAGE_ENGINE::Update( int aNearMargin )
{
    std::vector<CREEPAGE_RESULT> results;

    if( !m_interactive || !m_graph )
        return results;

    if( m_dynamicEdges )
        buildBoardEdgePrefix();
    else
        m_graph->TruncateToPrefix( m_staticNodeCount, m_staticConnCount );

    BOX2I roi;

    for( const BOARD_ITEM* item : m_movingItems )
    {
        if( item )
            roi.Merge( item->GetBoundingBox() );
    }

    roi.Inflate( m_margin );

    std::map<int, std::shared_ptr<GRAPH_NODE>> netNodes = addNetsInRegion( roi );

    m_graph->GeneratePaths( m_margin, m_layer, &m_affectedNets );
    connectChildren( *m_graph );

    auto solve = [&]( int aNetA, std::shared_ptr<GRAPH_NODE>& aNodeA, int aNetB,
                      std::shared_ptr<GRAPH_NODE>& aNodeB )
    {
        if( !aNodeA || !aNodeB )
            return;

        double constraint = m_constraintFn ? m_constraintFn( aNetA, aNetB ) : 0.0;

        if( constraint <= 0 )
            return;

        std::vector<std::shared_ptr<GRAPH_CONNECTION>> path;
        double distance = m_graph->Solve( aNodeA, aNodeB, path );

        std::optional<CREEPAGE_RESULT> r =
                extractResult( path, distance, aNetA, aNetB, constraint, aNearMargin );

        if( r )
            results.push_back( *r );
    };

    for( int affected : m_affectedNets )
    {
        auto itA = netNodes.find( affected );

        if( itA == netNodes.end() )
            continue;

        for( auto& [netB, nodeB] : netNodes )
        {
            if( netB == affected )
                continue;

            // Avoid solving an affected-affected pair twice
            if( m_affectedNets.count( netB ) && netB < affected )
                continue;

            solve( affected, itA->second, netB, nodeB );
        }
    }

    return results;
}


void CREEPAGE_ENGINE::EndInteractive()
{
    m_interactive = false;
    m_dynamicEdges = false;
    m_graph.reset();
    m_outline.reset();
    m_affectedNets.clear();
    m_movingItems.clear();
    m_staticNetBBoxes.clear();
    m_constraintFn = nullptr;
    m_staticNodeCount = 0;
    m_staticConnCount = 0;
}
