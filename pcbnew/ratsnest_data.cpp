/*
 * This program source code file is part of KICAD, a free EDA CAD application.
 *
 * Copyright (C) 2013-2015 CERN
 * @author Maciej Suminski <maciej.suminski@cern.ch>
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

/**
 * @file ratsnest_data.cpp
 * @brief Class that computes missing connections on a PCB.
 */

#ifdef USE_OPENMP
#include <omp.h>
#endif /* USE_OPENMP */

#include <ratsnest_data.h>

#include <class_board.h>
#include <class_module.h>
#include <class_pad.h>
#include <class_track.h>
#include <class_zone.h>

#include <boost/range/adaptor/map.hpp>
#include <boost/scoped_ptr.hpp>
#include <boost/make_shared.hpp>
#include <boost/bind.hpp>

#include <geometry/shape_poly_set.h>

#include <cassert>
#include <algorithm>
#include <limits>

#ifdef PROFILE
#include <profile.h>
#endif

static uint64_t getDistance( const RN_NODE_PTR& aNode1, const RN_NODE_PTR& aNode2 )
{
    // Drop the least significant bits to avoid overflow
    int64_t x = ( aNode1->GetX() - aNode2->GetX() ) >> 16;
    int64_t y = ( aNode1->GetY() - aNode2->GetY() ) >> 16;

    // We do not need sqrt() here, as the distance is computed only for comparison
    return ( x * x + y * y );
}


static bool sortDistance( const RN_NODE_PTR& aOrigin, const RN_NODE_PTR& aNode1,
                   const RN_NODE_PTR& aNode2 )
{
    return getDistance( aOrigin, aNode1 ) < getDistance( aOrigin, aNode2 );
}


static bool sortWeight( const RN_EDGE_PTR& aEdge1, const RN_EDGE_PTR& aEdge2 )
{
    return aEdge1->GetWeight() < aEdge2->GetWeight();
}


bool sortArea( const RN_POLY& aP1, const RN_POLY& aP2 )
{
    return aP1.m_bbox.GetArea() < aP2.m_bbox.GetArea();
}


bool operator==( const RN_NODE_PTR& aFirst, const RN_NODE_PTR& aSecond )
{
    return aFirst->GetX() == aSecond->GetX() && aFirst->GetY() == aSecond->GetY();
}


bool operator!=( const RN_NODE_PTR& aFirst, const RN_NODE_PTR& aSecond )
{
    return aFirst->GetX() != aSecond->GetX() || aFirst->GetY() != aSecond->GetY();
}


RN_NODE_AND_FILTER operator&&( const RN_NODE_FILTER& aFilter1, const RN_NODE_FILTER& aFilter2 )
{
    return RN_NODE_AND_FILTER( aFilter1, aFilter2 );
}


RN_NODE_OR_FILTER operator||( const RN_NODE_FILTER& aFilter1, const RN_NODE_FILTER& aFilter2 )
{
    return RN_NODE_OR_FILTER( aFilter1, aFilter2 );
}


static bool isEdgeConnectingNode( const RN_EDGE_PTR& aEdge, const RN_NODE_PTR& aNode )
{
    return aEdge->GetSourceNode() == aNode || aEdge->GetTargetNode() == aNode;
}


static std::vector<RN_EDGE_MST_PTR>* kruskalMST( RN_LINKS::RN_EDGE_LIST& aEdges,
                                                 std::vector<RN_NODE_PTR>& aNodes )
{
    unsigned int nodeNumber = aNodes.size();
    unsigned int mstExpectedSize = nodeNumber - 1;
    unsigned int mstSize = 0;
    bool ratsnestLines = false;

    // The output
    std::vector<RN_EDGE_MST_PTR>* mst = new std::vector<RN_EDGE_MST_PTR>;
    mst->reserve( mstExpectedSize );

    // Set tags for marking cycles
    boost::unordered_map<RN_NODE_PTR, int> tags;
    unsigned int tag = 0;
    BOOST_FOREACH( RN_NODE_PTR& node, aNodes )
    {
        node->SetTag( tag );
        tags[node] = tag++;
    }

    // Lists of nodes connected together (subtrees) to detect cycles in the graph
    std::vector<std::list<int> > cycles( nodeNumber );
    for( unsigned int i = 0; i < nodeNumber; ++i )
        cycles[i].push_back( i );

    // Kruskal algorithm requires edges to be sorted by their weight
    aEdges.sort( sortWeight );

    while( mstSize < mstExpectedSize && !aEdges.empty() )
    {
        RN_EDGE_PTR& dt = aEdges.front();

        int srcTag = tags[dt->GetSourceNode()];
        int trgTag = tags[dt->GetTargetNode()];

        // Check if by adding this edge we are going to join two different forests
        if( srcTag != trgTag )
        {
            // Because edges are sorted by their weight, first we always process connected
            // items (weight == 0). Once we stumble upon an edge with non-zero weight,
            // it means that the rest of the lines are ratsnest.
            if( !ratsnestLines && dt->GetWeight() != 0 )
                ratsnestLines = true;

            // Update tags
            std::list<int>::iterator it, itEnd;

            if( ratsnestLines )
            {
                for( it = cycles[trgTag].begin(), itEnd = cycles[trgTag].end(); it != itEnd; ++it )
                    tags[aNodes[*it]] = srcTag;
            }
            else
            {
                for( it = cycles[trgTag].begin(), itEnd = cycles[trgTag].end(); it != itEnd; ++it ) {
                    tags[aNodes[*it]] = srcTag;
                    aNodes[*it]->SetTag( srcTag );
                }
            }

            // Move nodes that were marked with old tag to the list marked with the new tag
            cycles[srcTag].splice( cycles[srcTag].end(), cycles[trgTag] );

            if( ratsnestLines )
            {
                // Do a copy of edge, but make it RN_EDGE_MST. In contrary to RN_EDGE,
                // RN_EDGE_MST saves both source and target node and does not require any other
                // edges to exist for getting source/target nodes
                RN_EDGE_MST_PTR newEdge = boost::make_shared<RN_EDGE_MST>( dt->GetSourceNode(),
                                                                           dt->GetTargetNode(),
                                                                           dt->GetWeight() );
                mst->push_back( newEdge );
                ++mstSize;
            }
            else
            {
                // Processing a connection, decrease the expected size of the ratsnest MST
                --mstExpectedSize;
            }
        }

        // Remove the edge that was just processed
        aEdges.erase( aEdges.begin() );
    }

    // Probably we have discarded some of edges, so reduce the size
    mst->resize( mstSize );

    return mst;
}


void RN_NET::validateEdge( RN_EDGE_MST_PTR& aEdge )
{
    RN_NODE_PTR source = aEdge->GetSourceNode();
    RN_NODE_PTR target = aEdge->GetTargetNode();
    bool valid = true;

    // If any of nodes belonging to the edge has the flag set,
    // change it to the closest node that has flag cleared
    if( source->GetFlag() )
    {
        valid = false;

        std::list<RN_NODE_PTR> closest = GetClosestNodes( source, WITHOUT_FLAG() );
        BOOST_FOREACH( RN_NODE_PTR& node, closest )
        {
            if( node && node != target )
            {
                source = node;
                break;
            }
        }
    }

    if( target->GetFlag() )
    {
        valid = false;

        std::list<RN_NODE_PTR> closest = GetClosestNodes( target, WITHOUT_FLAG() );
        BOOST_FOREACH( RN_NODE_PTR& node, closest )
        {
            if( node && node != source )
            {
                target = node;
                break;
            }
        }
    }

    // Replace an invalid edge with new, valid one
    if( !valid )
        aEdge.reset( new RN_EDGE_MST( source, target ) );
}


void RN_NET::removeNode( RN_NODE_PTR& aNode, const BOARD_CONNECTED_ITEM* aParent )
{
    aNode->RemoveParent( aParent );

    if( m_links.RemoveNode( aNode ) )
    {
        clearNode( aNode );
        m_dirty = true;
    }
}


void RN_NET::removeEdge( RN_EDGE_MST_PTR& aEdge, const BOARD_CONNECTED_ITEM* aParent )
{
    // Save nodes, so they can be cleared later
    RN_NODE_PTR start = aEdge->GetSourceNode();
    RN_NODE_PTR end = aEdge->GetTargetNode();

    start->RemoveParent( aParent );
    end->RemoveParent( aParent );

    // Connection has to be removed before running RemoveNode(),
    // as RN_NODE influences the reference counter
    m_links.RemoveConnection( aEdge );

    // Remove nodes associated with the edge. It is done in a safe way, there is a check
    // if nodes are not used by other edges.
    if( m_links.RemoveNode( start ) )
        clearNode( start );

    if( m_links.RemoveNode( end ) )
        clearNode( end );

    m_dirty = true;
}


const RN_NODE_PTR& RN_LINKS::AddNode( int aX, int aY )
{
    RN_NODE_SET::iterator node;
    bool wasNewElement;

    boost::tie( node, wasNewElement ) = m_nodes.emplace( boost::make_shared<RN_NODE>( aX, aY ) );

    return *node;
}


bool RN_LINKS::RemoveNode( const RN_NODE_PTR& aNode )
{
    if( aNode->GetRefCount() == 0 )
    {
        m_nodes.erase( aNode );

        return true;
    }

    return false;
}


RN_EDGE_MST_PTR RN_LINKS::AddConnection( const RN_NODE_PTR& aNode1, const RN_NODE_PTR& aNode2,
                                         unsigned int aDistance )
{
    assert( aNode1 != aNode2 );
    RN_EDGE_MST_PTR edge = boost::make_shared<RN_EDGE_MST>( aNode1, aNode2, aDistance );
    m_edges.push_back( edge );

    return edge;
}


void RN_NET::compute()
{
    const RN_LINKS::RN_NODE_SET& boardNodes = m_links.GetNodes();
    const RN_LINKS::RN_EDGE_LIST& boardEdges = m_links.GetConnections();

    // Special cases do not need complicated algorithms
    if( boardNodes.size() <= 2 )
    {
        m_rnEdges.reset( new std::vector<RN_EDGE_MST_PTR>( 0 ) );

        // Check if the only possible connection exists
        if( boardEdges.size() == 0 && boardNodes.size() == 2 )
        {
            RN_LINKS::RN_NODE_SET::iterator last = ++boardNodes.begin();

            // There can be only one possible connection, but it is missing
            m_rnEdges->push_back( boost::make_shared<RN_EDGE_MST>( *boardNodes.begin(), *last ) );
        }

        // Set tags to nodes as connected
        BOOST_FOREACH( RN_NODE_PTR node, boardNodes )
            node->SetTag( 0 );

        return;
    }

    // Move and sort (sorting speeds up) all nodes to a vector for the Delaunay triangulation
    std::vector<RN_NODE_PTR> nodes( boardNodes.size() );
    std::partial_sort_copy( boardNodes.begin(), boardNodes.end(), nodes.begin(), nodes.end() );

    TRIANGULATOR triangulator;
    triangulator.CreateDelaunay( nodes.begin(), nodes.end() );
    boost::scoped_ptr<RN_LINKS::RN_EDGE_LIST> triangEdges( triangulator.GetEdges() );

    // Compute weight/distance for edges resulting from triangulation
    RN_LINKS::RN_EDGE_LIST::iterator eit, eitEnd;
    for( eit = (*triangEdges).begin(), eitEnd = (*triangEdges).end(); eit != eitEnd; ++eit )
        (*eit)->SetWeight( getDistance( (*eit)->GetSourceNode(), (*eit)->GetTargetNode() ) );

    // Add the currently existing connections list to the results of triangulation
    std::copy( boardEdges.begin(), boardEdges.end(), std::front_inserter( *triangEdges ) );

    // Get the minimal spanning tree
    m_rnEdges.reset( kruskalMST( *triangEdges, nodes ) );
}


void RN_NET::clearNode( const RN_NODE_PTR& aNode )
{
    if( !m_rnEdges )
        return;

    std::vector<RN_EDGE_MST_PTR>::iterator newEnd;

    // Remove all ratsnest edges for associated with the node
    newEnd = std::remove_if( m_rnEdges->begin(), m_rnEdges->end(),
                             boost::bind( isEdgeConnectingNode, _1, boost::cref( aNode ) ) );

    m_rnEdges->resize( std::distance( m_rnEdges->begin(), newEnd ) );
}


RN_POLY::RN_POLY( const SHAPE_POLY_SET* aParent,
                  int aSubpolygonIndex,
                  RN_LINKS& aConnections, const BOX2I& aBBox ) :
    m_subpolygonIndex( aSubpolygonIndex ),
    m_bbox( aBBox ),
    m_parentPolyset( aParent )
{
    const VECTOR2I& p = aParent->CVertex( aSubpolygonIndex, 0 );

    m_node = aConnections.AddNode( p.x, p.y );

    // Mark it as not appropriate as a destination of ratsnest edges
    // (edges coming out from a polygon vertex look weird)
    m_node->SetFlag( true );
}


bool RN_POLY::HitTest( const RN_NODE_PTR& aNode ) const
{
    VECTOR2I p( aNode->GetX(), aNode->GetY() );

    return m_parentPolyset->Contains( p, m_subpolygonIndex );
}


void RN_NET::Update()
{
    // Add edges resulting from nodes being connected by zones
    processZones();
    processPads();

    compute();

    BOOST_FOREACH( RN_EDGE_MST_PTR& edge, *m_rnEdges )
        validateEdge( edge );

    m_dirty = false;
}


void RN_NET::AddItem( const D_PAD* aPad )
{
    RN_NODE_PTR node = m_links.AddNode( aPad->GetPosition().x, aPad->GetPosition().y );
    node->AddParent( aPad );
    m_pads[aPad].m_Node = node;

    m_dirty = true;
}


void RN_NET::AddItem( const VIA* aVia )
{
    RN_NODE_PTR node = m_links.AddNode( aVia->GetPosition().x, aVia->GetPosition().y );
    node->AddParent( aVia );
    m_vias[aVia] = node;

    m_dirty = true;
}


void RN_NET::AddItem( const TRACK* aTrack )
{
    if( aTrack->GetStart() == aTrack->GetEnd() )
        return;

    RN_NODE_PTR start = m_links.AddNode( aTrack->GetStart().x, aTrack->GetStart().y );
    RN_NODE_PTR end = m_links.AddNode( aTrack->GetEnd().x, aTrack->GetEnd().y );

    start->AddParent( aTrack );
    end->AddParent( aTrack );
    m_tracks[aTrack] = m_links.AddConnection( start, end );

    m_dirty = true;
}


void RN_NET::AddItem( const ZONE_CONTAINER* aZone )
{
    // Prepare a list of polygons (every zone can contain one or more polygons)
    const SHAPE_POLY_SET& polySet = aZone->GetFilledPolysList();

    for( int i = 0; i < polySet.OutlineCount(); ++i )
    {
        const SHAPE_LINE_CHAIN& path = polySet.COutline( i );

        RN_POLY poly = RN_POLY( &polySet, i, m_links, path.BBox() );
        m_zones[aZone].m_Polygons.push_back( poly );
    }

    m_dirty = true;
}


void RN_NET::RemoveItem( const D_PAD* aPad )
{
    PAD_NODE_MAP::iterator it = m_pads.find( aPad );

    if( it == m_pads.end() )
        return;

    RN_PAD_DATA& pad_data = it->second;
    removeNode( pad_data.m_Node, aPad );

    BOOST_FOREACH( RN_EDGE_MST_PTR& edge, pad_data.m_Edges )
        removeEdge( edge, aPad );

    m_pads.erase( aPad );
}


void RN_NET::RemoveItem( const VIA* aVia )
{
    VIA_NODE_MAP::iterator it = m_vias.find( aVia );

    if( it == m_vias.end() )
        return;

    removeNode( it->second, aVia );
    m_vias.erase( it );
}


void RN_NET::RemoveItem( const TRACK* aTrack )
{
    TRACK_EDGE_MAP::iterator it = m_tracks.find( aTrack );

    if( it == m_tracks.end() )
        return;

    removeEdge( it->second, aTrack );
    m_tracks.erase( it );
}


void RN_NET::RemoveItem( const ZONE_CONTAINER* aZone )
{
    ZONE_DATA_MAP::iterator it = m_zones.find( aZone );

    if( it == m_zones.end() )
        return;

    RN_ZONE_DATA& zoneData = it->second;

    // Remove all subpolygons that make the zone
    std::deque<RN_POLY>& polygons = zoneData.m_Polygons;
    BOOST_FOREACH( RN_POLY& polygon, polygons )
        removeNode( polygon.GetNode(), aZone );
    polygons.clear();

    // Remove all connections added by the zone
    std::deque<RN_EDGE_MST_PTR>& edges = zoneData.m_Edges;
    BOOST_FOREACH( RN_EDGE_MST_PTR edge, edges )
        removeEdge( edge, aZone );
    edges.clear();

    m_zones.erase( it );
}


const RN_NODE_PTR RN_NET::GetClosestNode( const RN_NODE_PTR& aNode ) const
{
    const RN_LINKS::RN_NODE_SET& nodes = m_links.GetNodes();
    RN_LINKS::RN_NODE_SET::const_iterator it, itEnd;

    unsigned int minDistance = std::numeric_limits<unsigned int>::max();
    RN_NODE_PTR closest;

    for( it = nodes.begin(), itEnd = nodes.end(); it != itEnd; ++it )
    {
        RN_NODE_PTR node = *it;

        // Obviously the distance between node and itself is the shortest,
        // that's why we have to skip it
        if( node != aNode )
        {
            unsigned int distance = getDistance( node, aNode );
            if( distance < minDistance )
            {
                minDistance = distance;
                closest = node;
            }
        }
    }

    return closest;
}


const RN_NODE_PTR RN_NET::GetClosestNode( const RN_NODE_PTR& aNode,
                                          const RN_NODE_FILTER& aFilter ) const
{
    const RN_LINKS::RN_NODE_SET& nodes = m_links.GetNodes();
    RN_LINKS::RN_NODE_SET::const_iterator it, itEnd;

    unsigned int minDistance = std::numeric_limits<unsigned int>::max();
    RN_NODE_PTR closest;

    for( it = nodes.begin(), itEnd = nodes.end(); it != itEnd; ++it )
    {
        RN_NODE_PTR node = *it;

        // Obviously the distance between node and itself is the shortest,
        // that's why we have to skip it
        if( node != aNode && aFilter( node ) )
        {
            unsigned int distance = getDistance( node, aNode );

            if( distance < minDistance )
            {
                minDistance = distance;
                closest = node;
            }
        }
    }

    return closest;
}


std::list<RN_NODE_PTR> RN_NET::GetClosestNodes( const RN_NODE_PTR& aNode, int aNumber ) const
{
    std::list<RN_NODE_PTR> closest;
    const RN_LINKS::RN_NODE_SET& nodes = m_links.GetNodes();

    // Copy nodes
    BOOST_FOREACH( const RN_NODE_PTR& node, nodes )
        closest.push_back( node );

    // Sort by the distance from aNode
    closest.sort( boost::bind( sortDistance, boost::cref( aNode ), _1, _2 ) );

    // aNode should not be returned in the results
    closest.remove( aNode );

    // Trim the result to the asked size
    if( aNumber > 0 )
        closest.resize( std::min( (size_t)aNumber, nodes.size() ) );

    return closest;
}


std::list<RN_NODE_PTR> RN_NET::GetClosestNodes( const RN_NODE_PTR& aNode,
                                                const RN_NODE_FILTER& aFilter, int aNumber ) const
{
    std::list<RN_NODE_PTR> closest;
    const RN_LINKS::RN_NODE_SET& nodes = m_links.GetNodes();

    // Copy nodes
    BOOST_FOREACH( const RN_NODE_PTR& node, nodes )
        closest.push_back( node );

    // Sort by the distance from aNode
    closest.sort( boost::bind( sortDistance, boost::cref( aNode ), _1, _2 ) );

    // aNode should not be returned in the results
    closest.remove( aNode );

    // Filter out by condition
    std::remove_if( closest.begin(), closest.end(), aFilter );

    // Trim the result to the asked size
    if( aNumber > 0 )
        closest.resize( std::min( static_cast<size_t>( aNumber ), nodes.size() ) );

    return closest;
}


void RN_NET::AddSimple( const BOARD_CONNECTED_ITEM* aItem )
{
    BOOST_FOREACH( RN_NODE_PTR node, GetNodes( aItem ) )
    {
        // Block all nodes, so they do not become targets for dynamic ratsnest lines
        AddBlockedNode( node );

        // Filter out junctions
        if( node->GetRefCount() == 1 )
            m_simpleNodes.insert( node );
    }
}


std::list<RN_NODE_PTR> RN_NET::GetNodes( const BOARD_CONNECTED_ITEM* aItem ) const
{
    std::list<RN_NODE_PTR> nodes;

    try
    {
        switch( aItem->Type() )
        {
        case PCB_PAD_T:
        {
            const D_PAD* pad = static_cast<const D_PAD*>( aItem );
            nodes.push_back( m_pads.at( pad ).m_Node );
        }
        break;

        case PCB_VIA_T:
        {
            const VIA* via = static_cast<const VIA*>( aItem );
            nodes.push_back( m_vias.at( via ) );
        }
        break;

        case PCB_TRACE_T:
        {
            const TRACK* track = static_cast<const TRACK*>( aItem );
            const RN_EDGE_MST_PTR& edge = m_tracks.at( track );

            nodes.push_back( edge->GetSourceNode() );
            nodes.push_back( edge->GetTargetNode() );
        }
        break;

        case PCB_ZONE_AREA_T:
        {
            const ZONE_CONTAINER* zone = static_cast<const ZONE_CONTAINER*>( aItem );
            const std::deque<RN_POLY>& polys = m_zones.at( zone ).m_Polygons;

            for( std::deque<RN_POLY>::const_iterator it = polys.begin(); it != polys.end(); ++it )
                nodes.push_back( it->GetNode() );
        }
        break;

        default:
            break;
        }
    }
    catch( ... )
    {
        // It is fine, just return empty list of nodes
    }

    return nodes;
}


void RN_NET::GetAllItems( std::list<BOARD_CONNECTED_ITEM*>& aOutput, RN_ITEM_TYPE aType ) const
{
    if( aType & RN_PADS )
    {
        BOOST_FOREACH( const BOARD_CONNECTED_ITEM* item, m_pads | boost::adaptors::map_keys )
            aOutput.push_back( const_cast<BOARD_CONNECTED_ITEM*>( item ) );
    }

    if( aType & RN_VIAS )
    {
        BOOST_FOREACH( const BOARD_CONNECTED_ITEM* item, m_vias | boost::adaptors::map_keys )
            aOutput.push_back( const_cast<BOARD_CONNECTED_ITEM*>( item ) );
    }

    if( aType & RN_TRACKS )
    {
        BOOST_FOREACH( const BOARD_CONNECTED_ITEM* item, m_tracks | boost::adaptors::map_keys )
            aOutput.push_back( const_cast<BOARD_CONNECTED_ITEM*>( item ) );
    }

    if( aType & RN_ZONES )
    {
        BOOST_FOREACH( const BOARD_CONNECTED_ITEM* item, m_zones | boost::adaptors::map_keys )
            aOutput.push_back( const_cast<BOARD_CONNECTED_ITEM*>( item ) );
    }
}


void RN_NET::ClearSimple()
{
    BOOST_FOREACH( const RN_NODE_PTR& node, m_blockedNodes )
        node->SetFlag( false );

    m_blockedNodes.clear();
    m_simpleNodes.clear();
}


void RN_NET::GetConnectedItems( const BOARD_CONNECTED_ITEM* aItem,
                                std::list<BOARD_CONNECTED_ITEM*>& aOutput,
                                RN_ITEM_TYPE aTypes ) const
{
    std::list<RN_NODE_PTR> nodes = GetNodes( aItem );
    assert( !nodes.empty() );

    int tag = nodes.front()->GetTag();
    assert( tag >= 0 );

    if( aTypes & RN_PADS )
    {
        for( PAD_NODE_MAP::const_iterator it = m_pads.begin(); it != m_pads.end(); ++it )
        {
            if( it->second.m_Node->GetTag() == tag )
                aOutput.push_back( const_cast<D_PAD*>( it->first ) );
        }
    }

    if( aTypes & RN_VIAS )
    {
        for( VIA_NODE_MAP::const_iterator it = m_vias.begin(); it != m_vias.end(); ++it )
        {
            if( it->second->GetTag() == tag )
                aOutput.push_back( const_cast<VIA*>( it->first ) );
        }
    }

    if( aTypes & RN_TRACKS )
    {
        for( TRACK_EDGE_MAP::const_iterator it = m_tracks.begin(); it != m_tracks.end(); ++it )
        {
            if( it->second->GetTag() == tag )
                aOutput.push_back( const_cast<TRACK*>( it->first ) );
        }
    }

    if( aTypes & RN_ZONES )
    {
        for( ZONE_DATA_MAP::const_iterator it = m_zones.begin(); it != m_zones.end(); ++it )
        {
            BOOST_FOREACH( const RN_EDGE_MST_PTR& edge, it->second.m_Edges )
            {
                if( edge->GetTag() == tag )
                {
                    aOutput.push_back( const_cast<ZONE_CONTAINER*>( it->first ) );
                    break;
                }
            }
        }
    }
}


void RN_DATA::AddSimple( const BOARD_ITEM* aItem )
{
    int net;

    if( aItem->IsConnected() )
    {
        const BOARD_CONNECTED_ITEM* item = static_cast<const BOARD_CONNECTED_ITEM*>( aItem );
        net = item->GetNetCode();

        if( net < 1 )           // do not process unconnected items
            return;

        m_nets[net].AddSimple( item );
    }
    else if( aItem->Type() == PCB_MODULE_T )
    {
        const MODULE* module = static_cast<const MODULE*>( aItem );

        for( const D_PAD* pad = module->Pads().GetFirst(); pad; pad = pad->Next() )
            AddSimple( pad );

        return;
    }
    else
        return;
}


void RN_DATA::AddBlocked( const BOARD_ITEM* aItem )
{
    int net;

    if( aItem->IsConnected() )
    {
        const BOARD_CONNECTED_ITEM* item = static_cast<const BOARD_CONNECTED_ITEM*>( aItem );
        net = item->GetNetCode();

        if( net < 1 )           // do not process unconnected items
            return;

        // Block all nodes belonging to the item
        BOOST_FOREACH( RN_NODE_PTR node, m_nets[net].GetNodes( item ) )
            m_nets[net].AddBlockedNode( node );
    }
    else if( aItem->Type() == PCB_MODULE_T )
    {
        const MODULE* module = static_cast<const MODULE*>( aItem );

        for( const D_PAD* pad = module->Pads().GetFirst(); pad; pad = pad->Next() )
            AddBlocked( pad );

        return;
    }
    else
        return;
}


void RN_DATA::GetConnectedItems( const BOARD_CONNECTED_ITEM* aItem,
                                 std::list<BOARD_CONNECTED_ITEM*>& aOutput,
                                 RN_ITEM_TYPE aTypes ) const
{
    int net = aItem->GetNetCode();

    if( net < 1 )
        return;

    assert( net < (int) m_nets.size() );

    m_nets[net].GetConnectedItems( aItem, aOutput, aTypes );
}


void RN_DATA::GetNetItems( int aNetCode, std::list<BOARD_CONNECTED_ITEM*>& aOutput,
                           RN_ITEM_TYPE aTypes ) const
{
    if( aNetCode < 1 )
        return;

    assert( aNetCode < (int) m_nets.size() );

    m_nets[aNetCode].GetAllItems( aOutput, aTypes );
}


bool RN_DATA::AreConnected( const BOARD_CONNECTED_ITEM* aItem, const BOARD_CONNECTED_ITEM* aOther )
{
    int net1 = aItem->GetNetCode();
    int net2 = aOther->GetNetCode();

    if( net1 < 1 || net2 < 1 || net1 != net2 )
        return false;

    assert( net1 < (int) m_nets.size() && net2 < (int) m_nets.size() );

    // net1 == net2
    std::list<RN_NODE_PTR> items1 = m_nets[net1].GetNodes( aItem );
    std::list<RN_NODE_PTR> items2 = m_nets[net1].GetNodes( aOther );

    assert( !items1.empty() && !items2.empty() );

    return ( items1.front()->GetTag() == items2.front()->GetTag() );
}


int RN_DATA::GetUnconnectedCount() const
{
    int count = 0;

    for( unsigned i = 0; i < m_nets.size(); ++i )
    {
        const std::vector<RN_EDGE_MST_PTR>* unconnected = m_nets[i].GetUnconnected();

        if( unconnected )
            count += unconnected->size();
    }

    return count;
}


void RN_NET::processZones()
{
    for( ZONE_DATA_MAP::iterator it = m_zones.begin(); it != m_zones.end(); ++it )
    {
        const ZONE_CONTAINER* zone = it->first;
        RN_ZONE_DATA& zoneData = it->second;

        // Reset existing connections
        BOOST_FOREACH( RN_EDGE_MST_PTR edge, zoneData.m_Edges )
            m_links.RemoveConnection( edge );

        zoneData.m_Edges.clear();
        LSET layers = zone->GetLayerSet();

        // Compute new connections
        RN_LINKS::RN_NODE_SET candidates = m_links.GetNodes();
        RN_LINKS::RN_NODE_SET::iterator point, pointEnd;

        // Sorting by area should speed up the processing, as smaller polygons are computed
        // faster and may reduce the number of points for further checks
        std::sort( zoneData.m_Polygons.begin(), zoneData.m_Polygons.end(), sortArea );

        for( std::deque<RN_POLY>::iterator poly = zoneData.m_Polygons.begin(),
                polyEnd = zoneData.m_Polygons.end(); poly != polyEnd; ++poly )
        {
            const RN_NODE_PTR& node = poly->GetNode();

            point = candidates.begin();
            pointEnd = candidates.end();

            while( point != pointEnd )
            {
                if( *point != node && ( (*point)->GetLayers() & layers ).any()
                        && poly->HitTest( *point ) )
                {
                    //(*point)->AddParent( zone );  // do not assign parent for helper links

                    RN_EDGE_MST_PTR connection = m_links.AddConnection( node, *point );
                    zoneData.m_Edges.push_back( connection );

                    // This point already belongs to a polygon, we do not need to check it anymore
                    point = candidates.erase( point );
                    pointEnd = candidates.end();
                }
                else
                {
                    ++point;
                }
            }
        }
    }
}


void RN_NET::processPads()
{
    for( PAD_NODE_MAP::iterator it = m_pads.begin(); it != m_pads.end(); ++it )
    {
        const D_PAD* pad = it->first;
        RN_NODE_PTR node = it->second.m_Node;
        std::deque<RN_EDGE_MST_PTR>& edges = it->second.m_Edges;

        // Reset existing connections
        BOOST_FOREACH( RN_EDGE_MST_PTR edge, edges )
            m_links.RemoveConnection( edge );

        LSET layers = pad->GetLayerSet();
        RN_LINKS::RN_NODE_SET candidates = m_links.GetNodes();
        RN_LINKS::RN_NODE_SET::iterator point, pointEnd;

        point = candidates.begin();
        pointEnd = candidates.end();

        while( point != pointEnd )
        {
            if( *point != node && ( (*point)->GetLayers() & layers ).any() &&
                    pad->HitTest( wxPoint( (*point)->GetX(), (*point)->GetY() ) ) )
            {
                //(*point)->AddParent( pad );   // do not assign parent for helper links

                RN_EDGE_MST_PTR connection = m_links.AddConnection( node, *point );
                edges.push_back( connection );
            }

            ++point;
        }
    }
}


void RN_DATA::Add( const BOARD_ITEM* aItem )
{
    int net;

    if( aItem->IsConnected() )
    {
        net = static_cast<const BOARD_CONNECTED_ITEM*>( aItem )->GetNetCode();
        if( net < 1 )           // do not process unconnected items
            return;

        if( net >= (int) m_nets.size() )            // Autoresize
            m_nets.resize( net + 1 );
    }
    else if( aItem->Type() == PCB_MODULE_T )
    {
        const MODULE* module = static_cast<const MODULE*>( aItem );
        for( const D_PAD* pad = module->Pads().GetFirst(); pad; pad = pad->Next() )
        {
            net = pad->GetNetCode();

            if( net < 1 )       // do not process unconnected items
                continue;

            if( net >= (int) m_nets.size() )        // Autoresize
                m_nets.resize( net + 1 );

            m_nets[net].AddItem( pad );
        }

        return;
    }
    else
        return;

    switch( aItem->Type() )
    {
    case PCB_PAD_T:
        m_nets[net].AddItem( static_cast<const D_PAD*>( aItem ) );
        break;

    case PCB_TRACE_T:
        m_nets[net].AddItem( static_cast<const TRACK*>( aItem ) );
        break;

    case PCB_VIA_T:
        m_nets[net].AddItem( static_cast<const VIA*>( aItem ) );
        break;

    case PCB_ZONE_AREA_T:
        m_nets[net].AddItem( static_cast<const ZONE_CONTAINER*>( aItem ) );
        break;

    default:
        break;
    }
}


void RN_DATA::Remove( const BOARD_ITEM* aItem )
{
    int net;

    if( aItem->IsConnected() )
    {
        net = static_cast<const BOARD_CONNECTED_ITEM*>( aItem )->GetNetCode();

        if( net < 1 )           // do not process unconnected items
            return;

#ifdef NDEBUG
        if( net >= (int) m_nets.size() )        // Autoresize
        {
            m_nets.resize( net + 1 );

            return;     // if it was resized, then surely the item had not been added before
        }
#endif
        assert( net < (int) m_nets.size() );
    }
    else if( aItem->Type() == PCB_MODULE_T )
    {
        const MODULE* module = static_cast<const MODULE*>( aItem );
        for( const D_PAD* pad = module->Pads().GetFirst(); pad; pad = pad->Next() )
        {
            net = pad->GetNetCode();

            if( net < 1 )       // do not process unconnected items
                continue;

#ifdef NDEBUG
            if( net >= (int) m_nets.size() )    // Autoresize
            {
                m_nets.resize( net + 1 );

                return;     // if it was resized, then surely the item had not been added before
            }
#endif
            assert( net < (int) m_nets.size() );

            m_nets[net].RemoveItem( pad );
        }

        return;
    }
    else
        return;

    switch( aItem->Type() )
    {
    case PCB_PAD_T:
        m_nets[net].RemoveItem( static_cast<const D_PAD*>( aItem ) );
        break;

    case PCB_TRACE_T:
        m_nets[net].RemoveItem( static_cast<const TRACK*>( aItem ) );
        break;

    case PCB_VIA_T:
        m_nets[net].RemoveItem( static_cast<const VIA*>( aItem ) );
        break;

    case PCB_ZONE_AREA_T:
        m_nets[net].RemoveItem( static_cast<const ZONE_CONTAINER*>( aItem ) );
        break;

    default:
        break;
    }
}


void RN_DATA::Update( const BOARD_ITEM* aItem )
{
    Remove( aItem );
    Add( aItem );
}


void RN_DATA::ProcessBoard()
{
    int netCount = m_board->GetNetCount();
    m_nets.clear();
    m_nets.resize( netCount );
    int netCode;

    // Iterate over all items that may need to be connected
    for( MODULE* module = m_board->m_Modules; module; module = module->Next() )
    {
        for( D_PAD* pad = module->Pads().GetFirst(); pad; pad = pad->Next() )
        {
            netCode = pad->GetNetCode();

            assert( netCode >= 0 && netCode < netCount );

            if( netCode > 0 && netCode < netCount )
                m_nets[netCode].AddItem( pad );
        }
    }

    for( TRACK* track = m_board->m_Track; track; track = track->Next() )
    {
        netCode = track->GetNetCode();

        assert( netCode >= 0 && netCode < netCount );

        if( netCode > 0 && netCode < netCount )
        {
            if( track->Type() == PCB_VIA_T )
                m_nets[netCode].AddItem( static_cast<VIA*>( track ) );
            else if( track->Type() == PCB_TRACE_T )
                m_nets[netCode].AddItem( track );
        }
    }

    for( int i = 0; i < m_board->GetAreaCount(); ++i )
    {
        ZONE_CONTAINER* zone = m_board->GetArea( i );

        netCode = zone->GetNetCode();

        assert( netCode >= 0 && netCode < netCount );

        if( netCode > 0 && netCode < netCount )
            m_nets[netCode].AddItem( zone );
    }

    Recalculate();
}


void RN_DATA::Recalculate( int aNet )
{
    unsigned int netCount = m_board->GetNetCount();

    if( netCount > m_nets.size() )
        m_nets.resize( netCount );

    if( aNet < 0 && netCount > 1 )              // Recompute everything
    {
#ifdef PROFILE
    prof_counter totalRealTime;
    prof_start( &totalRealTime );
#endif

        unsigned int i;

#ifdef USE_OPENMP
        #pragma omp parallel shared(netCount) private(i)
        {
            #pragma omp for schedule(guided, 1)
#else /* USE_OPENMP */
        {
#endif
            // Start with net number 1, as 0 stands for not connected
            for( i = 1; i < netCount; ++i )
            {
                if( m_nets[i].IsDirty() )
                    updateNet( i );
            }
        }  /* end of parallel section */
#ifdef PROFILE
    prof_end( &totalRealTime );

    wxLogDebug( wxT( "Recalculate all nets: %.1f ms" ), totalRealTime.msecs() );
#endif /* PROFILE */
    }
    else if( aNet > 0 )         // Recompute only specific net
    {
        updateNet( aNet );
    }
}


void RN_DATA::updateNet( int aNetCode )
{
    assert( aNetCode < (int) m_nets.size() );

    if( aNetCode < 1 || aNetCode > (int) m_nets.size() )
        return;

    m_nets[aNetCode].ClearSimple();
    m_nets[aNetCode].Update();
}
