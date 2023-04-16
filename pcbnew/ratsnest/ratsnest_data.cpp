/*
 * This program source code file is part of KICAD, a free EDA CAD application.
 *
 * Copyright (C) 2013-2017 CERN
 * Copyright (C) 2019-2023 KiCad Developers, see AUTHORS.txt for contributors.
 *
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

#ifdef PROFILE
#include <profile.h>
#endif

#include <ratsnest/ratsnest_data.h>
#include <functional>
using namespace std::placeholders;

#include <algorithm>
#include <cassert>
#include <limits>

#include <delaunator.hpp>

class disjoint_set
{

public:
    disjoint_set( size_t size )
    {
        m_data.resize( size );
        m_depth.resize( size, 0 );

        for( size_t i = 0; i < size; i++ )
            m_data[i]  = i;
    }

    int find( int aVal )
    {
        int root = aVal;

        while( m_data[root] != root )
            root = m_data[root];

        // Compress the path
        while( m_data[aVal] != aVal )
        {
            auto& tmp = m_data[aVal];
            aVal      = tmp;
            tmp       = root;
        }

        return root;
    }


    bool unite( int aVal1, int aVal2 )
    {
        aVal1 = find( aVal1 );
        aVal2 = find( aVal2 );

        if( aVal1 != aVal2 )
        {
            if( m_depth[aVal1] < m_depth[aVal2] )
            {
                m_data[aVal1] = aVal2;
            }
            else
            {
                m_data[aVal2] = aVal1;

                if( m_depth[aVal1] == m_depth[aVal2] )
                    m_depth[aVal1]++;
            }

            return true;
        }

        return false;
    }

private:
    std::vector<int> m_data;
    std::vector<int> m_depth;
};


void RN_NET::kruskalMST( const std::vector<CN_EDGE> &aEdges )
{
    disjoint_set dset( m_nodes.size() );

    m_rnEdges.clear();

    int i = 0;

    for( const std::shared_ptr<CN_ANCHOR>& node : m_nodes )
        node->SetTag( i++ );

    for( const CN_EDGE& tmp : aEdges )
    {
        const std::shared_ptr<const CN_ANCHOR>& source = tmp.GetSourceNode();
        const std::shared_ptr<const CN_ANCHOR>& target = tmp.GetTargetNode();

        if( dset.unite( source->GetTag(), target->GetTag() ) )
        {
            if( tmp.GetWeight() > 0 )
                m_rnEdges.push_back( tmp );
        }
    }
}


class RN_NET::TRIANGULATOR_STATE
{
private:
    std::multiset<std::shared_ptr<CN_ANCHOR>, CN_PTR_CMP> m_allNodes;


    // Checks if all nodes in aNodes lie on a single line. Requires the nodes to
    // have unique coordinates!
    bool areNodesColinear( const std::vector<std::shared_ptr<CN_ANCHOR>>& aNodes ) const
    {
        if ( aNodes.size() <= 2 )
            return true;

        const VECTOR2I p0( aNodes[0]->Pos() );
        const VECTOR2I v0( aNodes[1]->Pos() - p0 );

        for( unsigned i = 2; i < aNodes.size(); i++ )
        {
            const VECTOR2I v1 = aNodes[i]->Pos() - p0;

            if( v0.Cross( v1 ) != 0 )
                return false;
        }

        return true;
    }

public:

    void Clear()
    {
        m_allNodes.clear();
    }

    void AddNode( const std::shared_ptr<CN_ANCHOR>& aNode )
    {
        m_allNodes.insert( aNode );
    }

    void Triangulate( std::vector<CN_EDGE>& mstEdges )
    {
        std::vector<double>                                    node_pts;
        std::vector<std::shared_ptr<CN_ANCHOR>>                anchors;
        std::vector< std::vector<std::shared_ptr<CN_ANCHOR>> > anchorChains( m_allNodes.size() );

        node_pts.reserve( 2 * m_allNodes.size() );
        anchors.reserve( m_allNodes.size() );

        auto addEdge =
                [&]( const std::shared_ptr<CN_ANCHOR>& src, const std::shared_ptr<CN_ANCHOR>& dst )
                {
                    mstEdges.emplace_back( src, dst, src->Dist( *dst ) );
                };

        std::shared_ptr<CN_ANCHOR> prev = nullptr;

        for( const std::shared_ptr<CN_ANCHOR>& n : m_allNodes )
        {
            if( !prev || prev->Pos() != n->Pos() )
            {
                node_pts.push_back( n->Pos().x );
                node_pts.push_back( n->Pos().y );
                anchors.push_back( n );
                prev = n;
            }

            anchorChains[anchors.size() - 1].push_back( n );
        }

        if( anchors.size() < 2 )
        {
            return;
        }
        else if( areNodesColinear( anchors ) )
        {
            // special case: all nodes are on the same line - there's no
            // triangulation for such set. In this case, we sort along any coordinate
            // and chain the nodes together.
            for( size_t i = 0; i < anchors.size() - 1; i++ )
                addEdge( anchors[i], anchors[i + 1] );
        }
        else
        {
            delaunator::Delaunator delaunator( node_pts );
            auto& triangles = delaunator.triangles;

            for( size_t i = 0; i < triangles.size(); i += 3 )
            {
                addEdge( anchors[triangles[i]],     anchors[triangles[i + 1]] );
                addEdge( anchors[triangles[i + 1]], anchors[triangles[i + 2]] );
                addEdge( anchors[triangles[i + 2]], anchors[triangles[i]]     );
            }

            for( size_t i = 0; i < delaunator.halfedges.size(); i++ )
            {
                if( delaunator.halfedges[i] == delaunator::INVALID_INDEX )
                    continue;

                addEdge( anchors[triangles[i]], anchors[triangles[delaunator.halfedges[i]]] );
            }
        }

        for( size_t i = 0; i < anchorChains.size(); i++ )
        {
            std::vector<std::shared_ptr<CN_ANCHOR>>& chain = anchorChains[i];

            if( chain.size() < 2 )
                continue;

            std::sort( chain.begin(), chain.end(),
                    [] ( const std::shared_ptr<CN_ANCHOR>& a, const std::shared_ptr<CN_ANCHOR>& b )
                    {
                        return a->GetCluster().get() < b->GetCluster().get();
                    } );

            for( unsigned int j = 1; j < chain.size(); j++ )
            {
                const std::shared_ptr<CN_ANCHOR>& prevNode = chain[j - 1];
                const std::shared_ptr<CN_ANCHOR>& curNode  = chain[j];
                int weight = prevNode->GetCluster() != curNode->GetCluster() ? 1 : 0;
                mstEdges.emplace_back( prevNode, curNode, weight );
            }
        }
    }
};


RN_NET::RN_NET() : m_dirty( true )
{
    m_triangulator.reset( new TRIANGULATOR_STATE );
}


void RN_NET::compute()
{
    // Special cases do not need complicated algorithms (actually, it does not work well with
    // the Delaunay triangulator)
    if( m_nodes.size() <= 2 )
    {
        m_rnEdges.clear();

        // Check if the only possible connection exists
        if( m_boardEdges.size() == 0 && m_nodes.size() == 2 )
        {
            // There can be only one possible connection, but it is missing
            auto                              it = m_nodes.begin();
            const std::shared_ptr<CN_ANCHOR>& source = *it++;
            const std::shared_ptr<CN_ANCHOR>& target = *it;

            source->SetTag( 0 );
            target->SetTag( 1 );
            m_rnEdges.emplace_back( source, target );
        }
        else
        {
            // Set tags to m_nodes as connected
            for( const std::shared_ptr<CN_ANCHOR>& node : m_nodes )
                node->SetTag( 0 );
        }

        return;
    }


    m_triangulator->Clear();

    for( const std::shared_ptr<CN_ANCHOR>& n : m_nodes )
        m_triangulator->AddNode( n );

    std::vector<CN_EDGE> triangEdges;
    triangEdges.reserve( m_nodes.size() + m_boardEdges.size() );

#ifdef PROFILE
    PROF_TIMER cnt( "triangulate" );
#endif
    m_triangulator->Triangulate( triangEdges );
#ifdef PROFILE
    cnt.Show();
#endif

    for( const CN_EDGE& e : m_boardEdges )
        triangEdges.emplace_back( e );

    std::sort( triangEdges.begin(), triangEdges.end() );

// Get the minimal spanning tree
#ifdef PROFILE
    PROF_TIMER cnt2( "mst" );
#endif
    kruskalMST( triangEdges );
#ifdef PROFILE
    cnt2.Show();
#endif
}


void RN_NET::OptimizeRNEdges()
{
    auto optimizeZoneAnchor =
            [&]( const VECTOR2I& aPos, const LSET& aLayerSet,
                 const std::shared_ptr<const CN_ANCHOR>& aAnchor,
                 const std::function<void( std::shared_ptr<const CN_ANCHOR> )>& setOptimizedTo )
            {
                SEG::ecoord closest_dist_sq = ( aAnchor->Pos() - aPos ).SquaredEuclideanNorm();
                VECTOR2I    closest_pt;
                CN_ITEM*    closest_item = nullptr;

                for( CN_ITEM* item : aAnchor->Item()->ConnectedItems() )
                {
                    CN_ZONE_LAYER* zoneLayer = dynamic_cast<CN_ZONE_LAYER*>( item );

                    if( zoneLayer && aLayerSet.test( zoneLayer->Layer() ) )
                    {
                        const std::vector<VECTOR2I>& pts = zoneLayer->GetOutline().CPoints();

                        for( const VECTOR2I& pt : pts )
                        {
                            SEG::ecoord dist_sq = ( pt - aPos ).SquaredEuclideanNorm();

                            if( dist_sq < closest_dist_sq )
                            {
                                closest_pt = pt;
                                closest_item = zoneLayer;
                                closest_dist_sq = dist_sq;
                            }
                        }
                    }
                }

                if( closest_item )
                    setOptimizedTo( std::make_shared<CN_ANCHOR>( closest_pt, closest_item ) );
            };

    auto optimizeZoneToZoneAnchors =
            [&]( const std::shared_ptr<const CN_ANCHOR>& a,
                 const std::shared_ptr<const CN_ANCHOR>& b,
                 const std::function<void(const std::shared_ptr<const CN_ANCHOR>&)>& setOptimizedATo,
                 const std::function<void(const std::shared_ptr<const CN_ANCHOR>&)>& setOptimizedBTo )
            {
                for( CN_ITEM* itemA : a->Item()->ConnectedItems() )
                {
                    CN_ZONE_LAYER* zoneLayerA = dynamic_cast<CN_ZONE_LAYER*>( itemA );

                    if( !zoneLayerA )
                        continue;

                    for( CN_ITEM* itemB : b->Item()->ConnectedItems() )
                    {
                        CN_ZONE_LAYER* zoneLayerB = dynamic_cast<CN_ZONE_LAYER*>( itemB );

                        if( zoneLayerB && zoneLayerB->Layer() == zoneLayerA->Layer() )
                        {
                            // Process the first matching layer.  We don't really care if it's
                            // the "best" layer or not, as anything will be better than the
                            // original anchors (which are connected to the zone and so certainly
                            // don't look like they should have ratsnest lines coming off them).

                            VECTOR2I     startA = zoneLayerA->GetOutline().GetPoint( 0 );
                            VECTOR2I     startB = zoneLayerB->GetOutline().GetPoint( 0 );
                            const SHAPE* shapeA = &zoneLayerA->GetOutline();
                            const SHAPE* shapeB = &zoneLayerB->GetOutline();
                            int          startDist = ( startA - startB ).EuclideanNorm();

                            VECTOR2I ptA;
                            shapeA->Collide( shapeB, startDist + 10, nullptr, &ptA );
                            setOptimizedATo( std::make_shared<CN_ANCHOR>( ptA, zoneLayerA ) );

                            VECTOR2I ptB;
                            shapeB->Collide( shapeA, startDist + 10, nullptr, &ptB );
                            setOptimizedBTo( std::make_shared<CN_ANCHOR>( ptB, zoneLayerB ) );
                        }
                    }
                }
            };

    for( CN_EDGE& edge : m_rnEdges )
    {
        const std::shared_ptr<const CN_ANCHOR>& source = edge.GetSourceNode();
        const std::shared_ptr<const CN_ANCHOR>& target = edge.GetTargetNode();

        if( source->ConnectedItemsCount() == 0 )
        {
            optimizeZoneAnchor( source->Pos(), source->Parent()->GetLayerSet(), target,
                                [&]( const std::shared_ptr<const CN_ANCHOR>& optimized )
                                {
                                    edge.SetTargetNode( optimized );
                                } );
        }
        else if( target->ConnectedItemsCount() == 0 )
        {
            optimizeZoneAnchor( target->Pos(), target->Parent()->GetLayerSet(), source,
                                [&]( const std::shared_ptr<const CN_ANCHOR>& optimized )
                                {
                                    edge.SetSourceNode( optimized );
                                } );
        }
        else
        {
            optimizeZoneToZoneAnchors( source, target,
                                       [&]( const std::shared_ptr<const CN_ANCHOR>& optimized )
                                       {
                                           edge.SetSourceNode( optimized );
                                       },
                                       [&]( const std::shared_ptr<const CN_ANCHOR>& optimized )
                                       {
                                           edge.SetTargetNode( optimized );
                                       } );
        }
    }
}


void RN_NET::UpdateNet()
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


void RN_NET::AddCluster( std::shared_ptr<CN_CLUSTER> aCluster )
{
    std::shared_ptr<CN_ANCHOR> firstAnchor;

    for( CN_ITEM* item : *aCluster )
    {
        std::vector<std::shared_ptr<CN_ANCHOR>>& anchors = item->Anchors();
        unsigned int nAnchors = dynamic_cast<CN_ZONE_LAYER*>( item ) ? 1 : anchors.size();

        if( nAnchors > anchors.size() )
            nAnchors = anchors.size();

        for( unsigned int i = 0; i < nAnchors; i++ )
        {
            anchors[i]->SetCluster( aCluster );
            m_nodes.insert( anchors[i] );

            if( firstAnchor )
            {
                if( firstAnchor != anchors[i] )
                    m_boardEdges.emplace_back( firstAnchor, anchors[i], 0 );
            }
            else
            {
                firstAnchor = anchors[i];
            }
        }
    }
}


bool RN_NET::NearestBicoloredPair( RN_NET* aOtherNet, VECTOR2I& aPos1, VECTOR2I& aPos2 ) const
{
    bool rv = false;

    SEG::ecoord distMax_sq = VECTOR2I::ECOORD_MAX;

    auto verify =
            [&]( const std::shared_ptr<CN_ANCHOR>& aTestNode1,
                 const std::shared_ptr<CN_ANCHOR>& aTestNode2 )
            {
                VECTOR2I    diff = aTestNode1->Pos() - aTestNode2->Pos();
                SEG::ecoord dist_sq = diff.SquaredEuclideanNorm();

                if( dist_sq < distMax_sq )
                {
                    rv         = true;
                    distMax_sq = dist_sq;
                    aPos1     = aTestNode1->Pos();
                    aPos2     = aTestNode2->Pos();
                }
            };

    std::multiset<std::shared_ptr<CN_ANCHOR>, CN_PTR_CMP> nodes_b;

    std::copy_if( m_nodes.begin(), m_nodes.end(), std::inserter( nodes_b, nodes_b.end() ),
            []( const std::shared_ptr<CN_ANCHOR> &aVal )
            { return !aVal->GetNoLine(); } );

    /// Sweep-line algorithm to cut the number of comparisons to find the closest point
    ///
    /// Step 1: The outer loop needs to be the subset (selected nodes) as it is a linear search
    for( const std::shared_ptr<CN_ANCHOR>& nodeA : aOtherNet->m_nodes )
    {

        if( nodeA->GetNoLine() )
            continue;

        /// Step 2: O( log n ) search to identify a close element ordered by x
        /// The fwd_it iterator will move forward through the elements while
        /// the rev_it iterator will move backward through the same set
        auto fwd_it = nodes_b.lower_bound( nodeA );
        auto rev_it = std::make_reverse_iterator( fwd_it );

        for( ; fwd_it != nodes_b.end(); ++fwd_it )
        {
            const std::shared_ptr<CN_ANCHOR>& nodeB = *fwd_it;

            SEG::ecoord distX_sq = SEG::Square( nodeA->Pos().x - nodeB->Pos().x );

            /// As soon as the x distance (primary sort) is larger than the smallest distance,
            /// stop checking further elements
            if( distX_sq > distMax_sq )
                break;

            verify( nodeA, nodeB );
        }

        /// Step 3: using the same starting point, check points backwards for closer points
        for( ; rev_it != nodes_b.rend(); ++rev_it )
        {
            const std::shared_ptr<CN_ANCHOR>& nodeB = *rev_it;

            SEG::ecoord distX_sq = SEG::Square( nodeA->Pos().x - nodeB->Pos().x );

            if( distX_sq > distMax_sq )
                break;

            verify( nodeA, nodeB );
        }
    }

    return rv;
}

