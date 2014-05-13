/*
 * This program source code file is part of KICAD, a free EDA CAD application.
 *
 * Copyright (C) 2013 CERN
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

#include <cassert>
#include <algorithm>
#include <limits>

uint64_t getDistance( const RN_NODE_PTR& aNode1, const RN_NODE_PTR& aNode2 )
{
    // Drop the least significant bits to avoid overflow
    int64_t x = ( aNode1->GetX() - aNode2->GetX() ) >> 16;
    int64_t y = ( aNode1->GetY() - aNode2->GetY() ) >> 16;

    // We do not need sqrt() here, as the distance is computed only for comparison
    return ( x * x + y * y );
}


bool sortDistance( const RN_NODE_PTR& aOrigin, const RN_NODE_PTR& aNode1,
                   const RN_NODE_PTR& aNode2 )
{
    return getDistance( aOrigin, aNode1 ) < getDistance( aOrigin, aNode2 );
}


bool sortWeight( const RN_EDGE_PTR& aEdge1, const RN_EDGE_PTR& aEdge2 )
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


bool isEdgeConnectingNode( const RN_EDGE_PTR& aEdge, const RN_NODE_PTR& aNode )
{
    return aEdge->GetSourceNode() == aNode || aEdge->GetTargetNode() == aNode;
}


std::vector<RN_EDGE_PTR>* kruskalMST( RN_LINKS::RN_EDGE_LIST& aEdges,
                                      const std::vector<RN_NODE_PTR>& aNodes )
{
    unsigned int nodeNumber = aNodes.size();
    unsigned int mstExpectedSize = nodeNumber - 1;
    unsigned int mstSize = 0;

    // The output
    std::vector<RN_EDGE_PTR>* mst = new std::vector<RN_EDGE_PTR>;
    mst->reserve( mstExpectedSize );

    // Set tags for marking cycles
    boost::unordered_map<RN_NODE_PTR, int> tags;
    unsigned int tag = 0;
    BOOST_FOREACH( const RN_NODE_PTR& node, aNodes )
        tags[node] = tag++;

    // Lists of nodes connected together (subtrees) to detect cycles in the graph
    std::vector<std::list<int> > cycles( nodeNumber );
    for( unsigned int i = 0; i < nodeNumber; ++i )
        cycles[i].push_back( i );

    // Kruskal algorithm requires edges to be sorted by their weight
    aEdges.sort( sortWeight );

    while( mstSize < mstExpectedSize && !aEdges.empty() )
    {
        RN_EDGE_PTR& dt = *aEdges.begin();

        int srcTag = tags[dt->GetSourceNode()];
        int trgTag = tags[dt->GetTargetNode()];

        // Check if by adding this edge we are going to join two different forests
        if( srcTag != trgTag )
        {
            // Update tags
            std::list<int>::iterator it, itEnd;
            for( it = cycles[trgTag].begin(), itEnd = cycles[trgTag].end(); it != itEnd; ++it )
                tags[aNodes[*it]] = srcTag;

            // Move nodes that were marked with old tag to the list marked with the new tag
            cycles[srcTag].splice( cycles[srcTag].end(), cycles[trgTag] );

            if( dt->GetWeight() == 0 )      // Skip already existing connections (weight == 0)
            {
                mstExpectedSize--;
            }
            else
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
        }

        // Remove the edge that was just processed
        aEdges.erase( aEdges.begin() );
    }

    // Probably we have discarded some of edges, so reduce the size
    mst->resize( mstSize );

    return mst;
}


void RN_NET::validateEdge( RN_EDGE_PTR& aEdge )
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


const RN_NODE_PTR& RN_LINKS::AddNode( int aX, int aY )
{
    RN_NODE_SET::iterator node;
    bool wasNewElement;

    boost::tie( node, wasNewElement ) = m_nodes.emplace( boost::make_shared<RN_NODE>( aX, aY ) );
    (*node)->IncRefCount(); // TODO use the shared_ptr use_count

    return *node;
}


bool RN_LINKS::RemoveNode( const RN_NODE_PTR& aNode )
{
    aNode->DecRefCount(); // TODO use the shared_ptr use_count

    if( aNode->GetRefCount() == 0 )
    {
        m_nodes.erase( aNode );

        return true;
    }

    return false;
}


const RN_EDGE_PTR& RN_LINKS::AddConnection( const RN_NODE_PTR& aNode1, const RN_NODE_PTR& aNode2,
                                            unsigned int aDistance )
{
    m_edges.push_back( boost::make_shared<RN_EDGE_MST>( aNode1, aNode2, aDistance ) );

    return m_edges.back();
}


void RN_NET::compute()
{
    const RN_LINKS::RN_NODE_SET& boardNodes = m_links.GetNodes();
    const RN_LINKS::RN_EDGE_LIST& boardEdges = m_links.GetConnections();

    // Special case that does need so complicated algorithm
    if( boardNodes.size() == 2 )
    {
        m_rnEdges.reset( new std::vector<RN_EDGE_PTR>( 0 ) );

        // Check if the only possible connection exists
        if( boardEdges.size() == 0 )
        {
            RN_LINKS::RN_NODE_SET::iterator last = ++boardNodes.begin();

            // There can be only one possible connection, but it is missing
            m_rnEdges->push_back( boost::make_shared<RN_EDGE_MST>( *boardNodes.begin(), *last ) );
        }

        return;
    }
    else if( boardNodes.size() <= 1 )   // This case is even simpler
    {
        m_rnEdges.reset( new std::vector<RN_EDGE_PTR>( 0 ) );

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

    std::vector<RN_EDGE_PTR>::iterator newEnd;

    // Remove all ratsnest edges for associated with the node
    newEnd = std::remove_if( m_rnEdges->begin(), m_rnEdges->end(),
                             boost::bind( isEdgeConnectingNode, _1, aNode ) );

    m_rnEdges->resize( std::distance( m_rnEdges->begin(), newEnd ) );
}


RN_POLY::RN_POLY( const CPolyPt* aBegin, const CPolyPt* aEnd, const ZONE_CONTAINER* aParent,
            RN_LINKS& aConnections, const BOX2I& aBBox ) :
    m_parent( aParent), m_begin( aBegin ), m_end( aEnd ), m_bbox( aBBox )
{
    m_node = aConnections.AddNode( m_begin->x, m_begin->y );

    // Mark it as not feasible as a destination of ratsnest edges
    // (edges coming out from a polygon vertex look weird)
    m_node->SetFlag( true );
}


bool RN_POLY::HitTest( const RN_NODE_PTR& aNode ) const
{
    long xt = aNode->GetX();
    long yt = aNode->GetY();

    // If the point lies outside the bounding box, there is no point to check it further
    if( !m_bbox.Contains( xt, yt ) )
        return false;

    long xNew, yNew, xOld, yOld, x1, y1, x2, y2;
    bool inside = false;

    // For the first loop we have to use the last point as the previous point
    xOld = m_end->x;
    yOld = m_end->y;

    for( const CPolyPt* point = m_begin; point <= m_end; ++point )
    {
        xNew = point->x;
        yNew = point->y;

        // Swap points if needed, so always x2 >= x1
        if( xNew > xOld )
        {
            x1 = xOld; y1 = yOld;
            x2 = xNew; y2 = yNew;
        }
        else
        {
            x1 = xNew; y1 = yNew;
            x2 = xOld; y2 = yOld;
        }

        if( ( xNew < xt ) == ( xt <= xOld ) && /* edge "open" at left end */
          (double)( yt - y1 ) * (double)( x2 - x1 ) < (double)( y2 - y1 ) * (double)( xt - x1 ) )
        {
            inside = !inside;
        }

        xOld = xNew;
        yOld = yNew;
    }

    return inside;
}


void RN_NET::Update()
{
    // Add edges resulting from nodes being connected by zones
    processZones();

    compute();

    BOOST_FOREACH( RN_EDGE_PTR& edge, *m_rnEdges )
        validateEdge( edge );

    m_dirty = false;
}


void RN_NET::AddItem( const D_PAD* aPad )
{
    RN_NODE_PTR nodePtr = m_links.AddNode( aPad->GetPosition().x, aPad->GetPosition().y );
    m_pads[aPad] = nodePtr;

    m_dirty = true;
}


void RN_NET::AddItem( const VIA* aVia )
{
    m_vias[aVia] = m_links.AddNode( aVia->GetPosition().x, aVia->GetPosition().y );

    m_dirty = true;
}


void RN_NET::AddItem( const TRACK* aTrack )
{
    RN_NODE_PTR start = m_links.AddNode( aTrack->GetStart().x, aTrack->GetStart().y );
    RN_NODE_PTR end = m_links.AddNode( aTrack->GetEnd().x, aTrack->GetEnd().y );

    m_tracks[aTrack] = m_links.AddConnection( start, end );

    m_dirty = true;
}


void RN_NET::AddItem( const ZONE_CONTAINER* aZone )
{
    // Prepare a list of polygons (every zone can contain one or more polygons)
    const std::vector<CPolyPt>& polyPoints = aZone->GetFilledPolysList().GetList();
    if( polyPoints.size() == 0 )
        return;

    // Origin and end of bounding box for a polygon
    VECTOR2I origin( polyPoints[0].x, polyPoints[0].y );
    VECTOR2I end( polyPoints[0].x, polyPoints[0].y );
    unsigned int idxStart = 0;

    // Extract polygons from zones
    for( unsigned int i = 0; i < polyPoints.size(); ++i )
    {
        const CPolyPt& point = polyPoints[i];

        // Determine bounding box
        if( point.x < origin.x )
            origin.x = point.x;
        else if( point.x > end.x )
            end.x = point.x;

        if( point.y < origin.y )
            origin.y = point.y;
        else if( point.y > end.y )
            end.y = point.y;

        if( point.end_contour )
        {
            // The last vertex is enclosing the polygon (it repeats at the beginning and
            // at the end), so we skip it
            m_zonePolygons[aZone].push_back( RN_POLY( &polyPoints[idxStart], &point, aZone,
                                             m_links, BOX2I( origin, end - origin ) ) );

            idxStart = i + 1;

            if( idxStart < polyPoints.size() )
            {
                origin.x = polyPoints[idxStart].x;
                origin.y = polyPoints[idxStart].y;
                end.x = polyPoints[idxStart].x;
                end.y = polyPoints[idxStart].y;
            }
        }
    }

    m_dirty = true;
}


void RN_NET::RemoveItem( const D_PAD* aPad )
{
    try
    {
        RN_NODE_PTR node = m_pads.at( aPad );

        if( m_links.RemoveNode( node ) )
            clearNode( node );

        m_pads.erase( aPad );

        m_dirty = true;
    }
    catch( ... )
    {
    }
}


void RN_NET::RemoveItem( const VIA* aVia )
{
    try
    {
        RN_NODE_PTR node = m_vias.at( aVia );

        if( m_links.RemoveNode( node ) )
            clearNode( node );

        m_vias.erase( aVia );

        m_dirty = true;
    }
    catch( ... )
    {
    }
}


void RN_NET::RemoveItem( const TRACK* aTrack )
{
    try
    {
        RN_EDGE_PTR& edge = m_tracks.at( aTrack );

        // Save nodes, so they can be cleared later
        RN_NODE_PTR aBegin = edge->GetSourceNode();
        RN_NODE_PTR aEnd = edge->GetTargetNode();
        m_links.RemoveConnection( edge );

        // Remove nodes associated with the edge. It is done in a safe way, there is a check
        // if nodes are not used by other edges.
        if( m_links.RemoveNode( aBegin ) )
            clearNode( aBegin );

        if( m_links.RemoveNode( aEnd ) )
            clearNode( aEnd );

        m_tracks.erase( aTrack );

        m_dirty = true;
    }
    catch( ... )
    {
    }
}


void RN_NET::RemoveItem( const ZONE_CONTAINER* aZone )
{
    try
    {
        // Remove all subpolygons that make the zone
        std::deque<RN_POLY>& polygons = m_zonePolygons.at( aZone );
        BOOST_FOREACH( RN_POLY& polygon, polygons )
        {
            const RN_NODE_PTR node = polygon.GetNode();

            if( m_links.RemoveNode( node ) )
                clearNode( node );
        }
        polygons.clear();

        // Remove all connections added by the zone
        std::deque<RN_EDGE_PTR>& edges = m_zoneConnections.at( aZone );
        BOOST_FOREACH( RN_EDGE_PTR& edge, edges )
            m_links.RemoveConnection( edge );
        edges.clear();

        m_dirty = true;
    }
    catch( ... )
    {
    }
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
    closest.sort( boost::bind( sortDistance, aNode, _1, _2 ) );

    // Remove the first node (==aNode), as it is surely located within the smallest distance
    closest.pop_front();

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
    closest.sort( boost::bind( sortDistance, aNode, _1, _2 ) );

    // Remove the first node (==aNode), as it is surely located within the smallest distance
    closest.pop_front();

    // Filter out by condition
    std::remove_if( closest.begin(), closest.end(), aFilter );

    // Trim the result to the asked size
    if( aNumber > 0 )
        closest.resize( std::min( static_cast<size_t>( aNumber ), nodes.size() ) );

    return closest;
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
            nodes.push_back( m_pads.at( pad ) );
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
            RN_EDGE_PTR edge = m_tracks.at( track );

            nodes.push_back( edge->GetSourceNode() );
            nodes.push_back( edge->GetTargetNode() );
        }
        break;

        default:
            break;
        }
    }
    catch ( ... )
    {
        return nodes;
    }

    return nodes;
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

        // Add all nodes belonging to the item
        BOOST_FOREACH( RN_NODE_PTR node, m_nets[net].GetNodes( item ) )
            m_nets[net].AddSimpleNode( node );
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


void RN_DATA::AddSimple( const VECTOR2I& aPosition, int aNetCode )
{
    assert( aNetCode > 0 );

    RN_NODE_PTR newNode = boost::make_shared<RN_NODE>( aPosition.x, aPosition.y );

    m_nets[aNetCode].AddSimpleNode( newNode );
}


void RN_NET::processZones()
{
    BOOST_FOREACH( std::deque<RN_EDGE_PTR>& edges, m_zoneConnections | boost::adaptors::map_values )
    {
        BOOST_FOREACH( RN_EDGE_PTR& edge, edges )
            m_links.RemoveConnection( edge );

        edges.clear();
    }

    RN_LINKS::RN_NODE_SET candidates = m_links.GetNodes();

    BOOST_FOREACH( std::deque<RN_POLY>& polygons, m_zonePolygons | boost::adaptors::map_values )
    {
        RN_LINKS::RN_NODE_SET::iterator point, pointEnd;
        std::deque<RN_POLY>::iterator poly, polyEnd;

        // Sorting by area should speed up the processing, as smaller polygons are computed
        // faster and may reduce the number of points for further checks
        std::sort( polygons.begin(), polygons.end(), sortArea );

        for( poly = polygons.begin(), polyEnd = polygons.end(); poly != polyEnd; ++poly )
        {
            point = candidates.begin();
            pointEnd = candidates.end();

            while( point != pointEnd )
            {
                if( poly->HitTest( *point ) )
                {
                    const RN_EDGE_PTR& connection = m_links.AddConnection( poly->GetNode(), *point );
                    m_zoneConnections[poly->GetParent()].push_back( connection );

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


void RN_DATA::Add( const BOARD_ITEM* aItem )
{
    int net;

    if( aItem->IsConnected() )
    {
        net = static_cast<const BOARD_CONNECTED_ITEM*>( aItem )->GetNetCode();
        if( net < 1 )           // do not process unconnected items
            return;

        // Autoresize
        if( net >= (int) m_nets.size() )
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

            // Autoresize
            if( net >= (int) m_nets.size() )
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
    }
    else if( aItem->Type() == PCB_MODULE_T )
    {
        const MODULE* module = static_cast<const MODULE*>( aItem );
        for( const D_PAD* pad = module->Pads().GetFirst(); pad; pad = pad->Next() )
        {
            net = pad->GetNetCode();
            if( net < 1 )       // do not process unconnected items
                continue;

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
    m_nets.clear();
    m_nets.resize( m_board->GetNetCount() );
    int netCode;

    // Iterate over all items that may need to be connected
    for( MODULE* module = m_board->m_Modules; module; module = module->Next() )
    {
        for( D_PAD* pad = module->Pads().GetFirst(); pad; pad = pad->Next() )
        {
            netCode = pad->GetNetCode();

            if( netCode > 0 )
                m_nets[netCode].AddItem( pad );
        }
    }

    for( TRACK* track = m_board->m_Track; track; track = track->Next() )
    {
        netCode = track->GetNetCode();

        if( netCode > 0 )
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

        if( netCode > 0 )
            m_nets[netCode].AddItem( zone );
    }
}


void RN_DATA::Recalculate( int aNet )
{
    if( aNet < 0 )              // Recompute everything
    {
        unsigned int i, netCount;
        netCount = m_board->GetNetCount();

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
