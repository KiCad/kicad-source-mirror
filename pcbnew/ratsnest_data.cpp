/*
 * This program source code file is part of KICAD, a free EDA CAD application.
 *
 * Copyright (C) 2013-2017 CERN
 * @author Maciej Suminski <maciej.suminski@cern.ch>
 * @author Tomasz Wlostowski <tomasz.wlostowski@cern.ch>
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

#ifdef PROFILE
#include <profile.h>
#endif

#include <ratsnest_data.h>
#include <functional>
using namespace std::placeholders;

#include <cassert>
#include <algorithm>
#include <limits>

#include <connectivity_algo.h>

static uint64_t getDistance( const CN_ANCHOR_PTR& aNode1, const CN_ANCHOR_PTR& aNode2 )
{
    double  dx = ( aNode1->Pos().x - aNode2->Pos().x );
    double  dy = ( aNode1->Pos().y - aNode2->Pos().y );

    return sqrt( dx * dx + dy * dy );
}

static bool sortWeight( const CN_EDGE& aEdge1, const CN_EDGE& aEdge2 )
{
    return aEdge1.GetWeight() < aEdge2.GetWeight();
}


/*bool operator==( const RN_NODE_PTR& aFirst, const RN_NODE_PTR& aSecond )
 *  {
 *   return aFirst->GetX() == aSecond->GetX() && aFirst->GetY() == aSecond->GetY();
 *  }
 *
 *  bool operator!=( const RN_NODE_PTR& aFirst, const RN_NODE_PTR& aSecond )
 *  {
 *   return aFirst->GetX() != aSecond->GetX() || aFirst->GetY() != aSecond->GetY();
 *  }
 *
 *  RN_NODE_AND_FILTER operator&&( const RN_NODE_FILTER& aFilter1, const RN_NODE_FILTER& aFilter2 )
 *  {
 *   return RN_NODE_AND_FILTER( aFilter1, aFilter2 );
 *  }
 *
 *  RN_NODE_OR_FILTER operator||( const RN_NODE_FILTER& aFilter1, const RN_NODE_FILTER& aFilter2 )
 *  {
 *   return RN_NODE_OR_FILTER( aFilter1, aFilter2 );
 *  }
 *
 *  static bool isEdgeConnectingNode( const RN_EDGE_PTR& aEdge, const RN_NODE_PTR& aNode )
 *  {
 *   return aEdge->GetSourceNode() == aNode || aEdge->GetTargetNode() == aNode;
 *  }
 */

static const std::vector<CN_EDGE> kruskalMST( std::list<CN_EDGE>& aEdges,
        std::vector<CN_ANCHOR_PTR>& aNodes )
{
    unsigned int    nodeNumber = aNodes.size();
    unsigned int    mstExpectedSize = nodeNumber - 1;
    unsigned int    mstSize = 0;
    bool ratsnestLines = false;

    //printf("mst nodes : %d edges : %d\n", aNodes.size(), aEdges.size () );
    // The output
    std::vector<CN_EDGE> mst;

    // Set tags for marking cycles
    std::unordered_map<CN_ANCHOR_PTR, int> tags;
    unsigned int tag = 0;

    for( auto& node : aNodes )
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
        //printf("mstSize %d %d\n", mstSize, mstExpectedSize);
        auto& dt = aEdges.front();

        int srcTag  = tags[dt.GetSourceNode()];
        int trgTag  = tags[dt.GetTargetNode()];

        // Check if by adding this edge we are going to join two different forests
        if( srcTag != trgTag )
        {
            // Because edges are sorted by their weight, first we always process connected
            // items (weight == 0). Once we stumble upon an edge with non-zero weight,
            // it means that the rest of the lines are ratsnest.
            if( !ratsnestLines && dt.GetWeight() != 0 )
                ratsnestLines = true;

            // Update tags
            if( ratsnestLines )
            {
                for( auto it = cycles[trgTag].begin(); it != cycles[trgTag].end(); ++it )
                {
                    tags[aNodes[*it]] = srcTag;
                }

                // Do a copy of edge, but make it RN_EDGE_MST. In contrary to RN_EDGE,
                // RN_EDGE_MST saves both source and target node and does not require any other
                // edges to exist for getting source/target nodes
                CN_EDGE newEdge ( dt.GetSourceNode(), dt.GetTargetNode(), dt.GetWeight() );

                assert( newEdge.GetSourceNode()->GetTag() != newEdge.GetTargetNode()->GetTag() );
                assert( newEdge.GetWeight() > 0 );

                mst.push_back( newEdge );
                ++mstSize;
            }
            else
            {
                // for( it = cycles[trgTag].begin(), itEnd = cycles[trgTag].end(); it != itEnd; ++it )
                // for( auto it : cycles[trgTag] )
                for( auto it = cycles[trgTag].begin(); it != cycles[trgTag].end(); ++it )
                {
                    tags[aNodes[*it]] = srcTag;
                    aNodes[*it]->SetTag( srcTag );
                }

                // Processing a connection, decrease the expected size of the ratsnest MST
                --mstExpectedSize;
            }

            // Move nodes that were marked with old tag to the list marked with the new tag
            cycles[srcTag].splice( cycles[srcTag].end(), cycles[trgTag] );
        }

        // Remove the edge that was just processed
        aEdges.erase( aEdges.begin() );
    }

    // Probably we have discarded some of edges, so reduce the size
    mst.resize( mstSize );

    return mst;
}

class RN_NET::TRIANGULATOR_STATE
{
private:
    std::vector<CN_ANCHOR_PTR>  m_allNodes;
    std::vector<hed::NODE_PTR>  m_triangulationNodes;

public:

    void Clear()
    {
        m_allNodes.clear();
    }

    void AddNode( CN_ANCHOR_PTR aNode )
    {
        m_allNodes.push_back( aNode );
    }

    const std::list<CN_EDGE> Triangulate()
    {
        std::list<CN_EDGE> mstEdges;
        std::list<hed::EDGE_PTR> triangEdges;
        std::vector<hed::NODE_PTR> triNodes;

        using ANCHOR_LIST = std::vector<CN_ANCHOR_PTR>;
        std::vector<ANCHOR_LIST> anchorChains;

        triNodes.reserve( m_allNodes.size() );
        anchorChains.reserve ( m_allNodes.size() );

        std::sort( m_allNodes.begin(), m_allNodes.end(),
                [] ( const CN_ANCHOR_PTR& aNode1, const CN_ANCHOR_PTR& aNode2 )
        {
            if( aNode1->Pos().y < aNode2->Pos().y )
                return true;
            else if( aNode1->Pos().y == aNode2->Pos().y )
            {
                return aNode1->Pos().x < aNode2->Pos().x;
            }

            return false;
        }
                );

        CN_ANCHOR_PTR prev, last;
        int id = 0;

        for( auto n : m_allNodes )
        {
            anchorChains.push_back( ANCHOR_LIST() );
        }

        for( auto n : m_allNodes )
        {
            if( !prev || prev->Pos() != n->Pos() )
            {
                auto tn = std::make_shared<hed::NODE> ( n->Pos().x, n->Pos().y );
                tn->SetId( id );
                triNodes.push_back( tn );
            }

            id++;
            prev = n;
        }

        int prevId = 0;

        for( auto n : triNodes )
        {
            for( int i = prevId; i < n->Id(); i++ )
                anchorChains[prevId].push_back( m_allNodes[ i ] );

            prevId = n->Id();
        }

        for( int i = prevId; i < id; i++ )
            anchorChains[prevId].push_back( m_allNodes[ i ] );


        hed::TRIANGULATION triangulator;
        triangulator.CreateDelaunay( triNodes.begin(), triNodes.end() );
        triangulator.GetEdges( triangEdges );

        for( auto e : triangEdges )
        {
            auto    src = m_allNodes[ e->GetSourceNode()->Id() ];
            auto    dst = m_allNodes[ e->GetTargetNode()->Id() ];

            mstEdges.emplace_back( src, dst, getDistance( src, dst ) );
        }

        for( int i = 0; i < anchorChains.size(); i++ )
        {
            auto& chain = anchorChains[i];

            if( chain.size() < 2 )
                continue;

            std::sort( chain.begin(), chain.end(),
                    [] ( const CN_ANCHOR_PTR& a, const CN_ANCHOR_PTR& b ) {
                return a->GetCluster().get() < b->GetCluster().get();
            } );

            for( auto j = 1; j < chain.size(); j++ )
            {
                const auto& prevNode    = chain[j - 1];
                const auto& curNode     = chain[j];
                int weight = prevNode->GetCluster() != curNode->GetCluster() ? 1 : 0;
                mstEdges.push_back( CN_EDGE ( prevNode, curNode, weight ) );
            }
        }

        return mstEdges;
    }
};

#include <profile.h>

RN_NET::RN_NET() : m_dirty( true ), m_visible( true )
{
    m_triangulator.reset( new TRIANGULATOR_STATE );
}

void RN_NET::compute()
{

    // Special cases do not need complicated algorithms (actually, it does not work well with
    // the Delaunay triangulator)
    //printf("compute nodes :  %d\n", m_nodes.size() );
    if( m_nodes.size() <= 2 )
    {
        m_rnEdges.clear();

        // Check if the only possible connection exists
        if( m_boardEdges.size() == 0 && m_nodes.size() == 2 )
        {
            auto last = ++m_nodes.begin();

            // There can be only one possible connection, but it is missing
            CN_EDGE edge (*m_nodes.begin(), *last );
            edge.GetSourceNode()->SetTag( 0 );
            edge.GetTargetNode()->SetTag( 1 );

            m_rnEdges.push_back( edge );
        }
        else
        {
            // Set tags to m_nodes as connected
            for( auto node : m_nodes )
                node->SetTag( 0 );
        }


        return;
    }


    m_triangulator->Clear();

    for( auto n : m_nodes )
    {
        m_triangulator->AddNode( n );
    }

    #ifdef PROFILE
    PROF_COUNTER cnt("triangulate");
    #endif
    auto triangEdges = m_triangulator->Triangulate();
    #ifdef PROFILE
    cnt.Show();
    #endif

    for( const auto& e : m_boardEdges )
        triangEdges.push_back( e );

// Get the minimal spanning tree
#ifdef PROFILE
    PROF_COUNTER cnt2("mst");
#endif
    m_rnEdges = kruskalMST( triangEdges, m_nodes );
#ifdef PROFILE
    cnt2.Show();
#endif
}



void RN_NET::Update()
{
    compute();

    m_dirty = false;
}


void RN_NET::Clear()
{
    m_rnEdges.clear();
    m_boardEdges.clear();
    m_nodes.clear();

    m_dirty = true;
}

#if 0

const RN_NODE_PTR RN_NET::GetClosestNode( const RN_NODE_PTR& aNode ) const
{
    /*const RN_LINKS::RN_NODE_SET& nodes = m_links.GetNodes();
     *  RN_LINKS::RN_NODE_LISt::const_iterator it, itEnd;
     *
     *  unsigned int minDistance = std::numeric_limits<unsigned int>::max();
     *  RN_NODE_PTR closest;
     *
     *  for( it = nodes.begin(), itEnd = nodes.end(); it != itEnd; ++it )
     *  {
     *   RN_NODE_PTR node = *it;
     *
     *   // Obviously the distance between node and itself is the shortest,
     *   // that's why we have to skip it
     *   if( node != aNode )
     *   {
     *       unsigned int distance = getDistance( node, aNode );
     *       if( distance < minDistance )
     *       {
     *           minDistance = distance;
     *           closest = node;
     *       }
     *   }
     *  }
     *
     *  return closest;*/
}


const RN_NODE_PTR RN_NET::GetClosestNode( const RN_NODE_PTR& aNode,
        const RN_NODE_FILTER& aFilter ) const
{
    /*const RN_LINKS::RN_NODE_SET& nodes = m_links.GetNodes();
     *  RN_LINKS::RN_NODE_SET::const_iterator it, itEnd;
     *
     *  unsigned int minDistance = std::numeric_limits<unsigned int>::max();
     *  RN_NODE_PTR closest;
     *
     *  for( it = nodes.begin(), itEnd = nodes.end(); it != itEnd; ++it )
     *  {
     *   RN_NODE_PTR node = *it;
     *
     *   // Obviously the distance between node and itself is the shortest,
     *   // that's why we have to skip it
     *   if( node != aNode && aFilter( node ) )
     *   {
     *       unsigned int distance = getDistance( node, aNode );
     *
     *       if( distance < minDistance )
     *       {
     *           minDistance = distance;
     *           closest = node;
     *       }
     *   }
     *  }
     *
     *  return closest;*/
}


std::list<RN_NODE_PTR> RN_NET::GetClosestNodes( const RN_NODE_PTR& aNode, int aNumber ) const
{
    /*std::list<RN_NODE_PTR> closest;
     *  const RN_LINKS::RN_NODE_SET& nodes = m_links.GetNodes();
     *
     *  // Copy nodes
     *  std::copy( nodes.begin(), nodes.end(), std::back_inserter( closest ) );
     *
     *  // Sort by the distance from aNode
     *  closest.sort( std::bind( sortDistance, std::cref( aNode ), _1, _2 ) );
     *
     *  // aNode should not be returned in the results
     *  closest.remove( aNode );
     *
     *  // Trim the result to the asked size
     *  if( aNumber > 0 )
     *   closest.resize( std::min( (size_t)aNumber, nodes.size() ) );
     *  return closest;*/
}


std::list<RN_NODE_PTR> RN_NET::GetClosestNodes( const RN_NODE_PTR& aNode,
        const RN_NODE_FILTER& aFilter, int aNumber ) const
{
    /*std::list<RN_NODE_PTR> closest;
     *  const RN_LINKS::RN_NODE_SET& nodes = m_links.GetNodes();
     *
     *  // Copy filtered nodes
     *  std::copy_if( nodes.begin(), nodes.end(), std::back_inserter( closest ), std::cref( aFilter ) );
     *
     *  // Sort by the distance from aNode
     *  closest.sort( std::bind( sortDistance, std::cref( aNode ), _1, _2 ) );
     *
     *  // aNode should not be returned in the results
     *  closest.remove( aNode );
     *
     *  // Trim the result to the asked size
     *  if( aNumber > 0 )
     *   closest.resize( std::min( static_cast<size_t>( aNumber ), nodes.size() ) );
     *  return closest;*/
}


std::list<RN_NODE_PTR> RN_NET::GetNodes( const BOARD_CONNECTED_ITEM* aItem ) const
{
    /*std::list<RN_NODE_PTR> nodes;
     *
     *  switch( aItem->Type() )
     *  {
     *  case PCB_PAD_T:
     *  {
     *   PAD_NODE_MAP::const_iterator it = m_pads.find( static_cast<const D_PAD*>( aItem ) );
     *
     *   if( it != m_pads.end() )
     *       nodes.push_back( it->second.m_Node );
     *  }
     *  break;
     *
     *  case PCB_VIA_T:
     *  {
     *   VIA_NODE_MAP::const_iterator it = m_vias.find( static_cast<const VIA*>( aItem ) );
     *
     *   if( it != m_vias.end() )
     *       nodes.push_back( it->second );
     *  }
     *  break;
     *
     *  case PCB_TRACE_T:
     *  {
     *   TRACK_EDGE_MAP::const_iterator it = m_tracks.find( static_cast<const TRACK*>( aItem ) );
     *
     *   if( it != m_tracks.end() )
     *   {
     *       nodes.push_back( it->second->GetSourceNode() );
     *       nodes.push_back( it->second->GetTargetNode() );
     *   }
     *  }
     *  break;
     *
     *  case PCB_ZONE_AREA_T:
     *  {
     *   ZONE_DATA_MAP::const_iterator itz = m_zones.find( static_cast<const ZONE_CONTAINER*>( aItem ) );
     *
     *   if( itz != m_zones.end() )
     *   {
     *       const std::deque<RN_POLY>& polys = itz->second.m_Polygons;
     *
     *       for( std::deque<RN_POLY>::const_iterator it = polys.begin(); it != polys.end(); ++it )
     *           nodes.push_back( it->GetNode() );
     *   }
     *  }
     *  break;
     *
     *  default:
     *   break;
     *  }
     *
     *  return nodes;*/
}


void RN_NET::GetAllItems( std::list<BOARD_CONNECTED_ITEM*>& aOutput, const KICAD_T aTypes[] ) const
{
/*    if( aType & RN_PADS )
 *   {
 *       for( auto it : m_pads )
 *           aOutput.push_back( const_cast<D_PAD*>( it.first ) );
 *   }
 *
 *   if( aType & RN_VIAS )
 *   {
 *       for( auto it : m_vias )
 *           aOutput.push_back( const_cast<VIA*>( it.first ) );
 *   }
 *
 *   if( aType & RN_TRACKS )
 *   {
 *       for( auto it : m_tracks )
 *           aOutput.push_back( const_cast<TRACK*>( it.first ) );
 *   }
 *
 *   if( aType & RN_ZONES )
 *   {
 *       for( auto it : m_zones )
 *           aOutput.push_back( const_cast<ZONE_CONTAINER*>( it.first ) );
 *   }*/
}


void RN_NET::GetConnectedItems( const BOARD_CONNECTED_ITEM* aItem,
        std::list<BOARD_CONNECTED_ITEM*>& aOutput,
        const KICAD_T aTypes[] ) const
{
/*    std::list<RN_NODE_PTR> nodes = GetNodes( aItem );
 *   assert( !nodes.empty() );
 *
 *   int tag = nodes.front()->GetTag();
 *   assert( tag >= 0 );
 *
 *   if( aTypes & RN_PADS )
 *   {
 *       for( PAD_NODE_MAP::const_iterator it = m_pads.begin(); it != m_pads.end(); ++it )
 *       {
 *           if( it->second.m_Node->GetTag() == tag )
 *               aOutput.push_back( const_cast<D_PAD*>( it->first ) );
 *       }
 *   }
 *
 *   if( aTypes & RN_VIAS )
 *   {
 *       for( VIA_NODE_MAP::const_iterator it = m_vias.begin(); it != m_vias.end(); ++it )
 *       {
 *           if( it->second->GetTag() == tag )
 *               aOutput.push_back( const_cast<VIA*>( it->first ) );
 *       }
 *   }
 *
 *   if( aTypes & RN_TRACKS )
 *   {
 *       for( TRACK_EDGE_MAP::const_iterator it = m_tracks.begin(); it != m_tracks.end(); ++it )
 *       {
 *           if( it->second->GetTag() == tag )
 *               aOutput.push_back( const_cast<TRACK*>( it->first ) );
 *       }
 *   }
 *
 *   if( aTypes & RN_ZONES )
 *   {
 *       for( ZONE_DATA_MAP::const_iterator it = m_zones.begin(); it != m_zones.end(); ++it )
 *       {
 *           for( const RN_EDGE_MST_PTR& edge : it->second.m_Edges )
 *           {
 *               if( edge->GetTag() == tag )
 *               {
 *                   aOutput.push_back( const_cast<ZONE_CONTAINER*>( it->first ) );
 *                   break;
 *               }
 *           }
 *       }
 *   }*/
}


// const RN_NODE_PTR& RN_NET::AddNode( int aX, int aY )
// {
// return m_links.AddNode( aX, aY );
// }

#endif

void RN_NET::AddCluster( CN_CLUSTER_PTR aCluster )
{
    CN_ANCHOR_PTR firstAnchor;

    for( auto item : *aCluster )
    {
        bool isZone = dynamic_cast<CN_ZONE*>(item) != nullptr;
        auto& anchors = item->Anchors();
        int nAnchors = isZone ? 1 : anchors.size();

        if ( nAnchors > anchors.size() )
            nAnchors = anchors.size();

        //printf("item %p anchors : %d\n", item, anchors.size() );
        //printf("add item %p anchors : %d net : %d\n", item, item->Anchors().size(), item->Parent()->GetNetCode() );

        for ( int i = 0; i < nAnchors; i++ )
        {
        //    printf("add anchor %p\n", anchors[i].get() );

            anchors[i]->SetCluster( aCluster );
            m_nodes.push_back(anchors[i]);

            if( firstAnchor )
            {
                if( firstAnchor != anchors[i] )
                {
                    m_boardEdges.emplace_back( firstAnchor, anchors[i], 0 );
                }
            }
            else
            {
                firstAnchor = anchors[i];
            }
        }
    }
}

bool RN_NET::NearestBicoloredPair( const RN_NET& aOtherNet, CN_ANCHOR_PTR& aNode1,
        CN_ANCHOR_PTR& aNode2 ) const
{
    bool rv = false;

    VECTOR2I::extended_type distMax = VECTOR2I::ECOORD_MAX;

    for( auto nodeA : m_nodes )
    {
        for( auto nodeB : aOtherNet.m_nodes )
        {
            if( !nodeA->GetNoLine() )
            {
                auto squaredDist = (nodeA->Pos() - nodeB->Pos() ).SquaredEuclideanNorm();

                if( squaredDist < distMax )
                {
                    rv = true;
                    distMax = squaredDist;
                    aNode1  = nodeA;
                    aNode2  = nodeB;
                }
            }
        }
    }

    return rv;
}


unsigned int RN_NET::GetNodeCount() const
{
    return m_nodes.size();
}
