/*
 * This program source code file is part of KICAD, a free EDA CAD application.
 *
 * Copyright (C) 2013-2017 CERN
 * Copyright (C) 2019-2020 KiCad Developers, see AUTHORS.txt for contributors.
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
#include <queue>

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

void RN_NET::kruskalMST( std::priority_queue<CN_EDGE> &aEdges )
{
    disjoint_set dset( m_nodes.size() );

    m_rnEdges.clear();

    int i = 0;

    for( auto& node : m_nodes )
        node->SetTag( i++ );

    while( !aEdges.empty() )
    {
        auto& tmp = aEdges.top();

        int u = tmp.GetSourceNode()->GetTag();
        int v = tmp.GetTargetNode()->GetTag();

        if( dset.unite( u, v ) )
        {
            if( tmp.GetWeight() > 0 )
                m_rnEdges.push_back( tmp );
        }

        aEdges.pop();
    }
}


class RN_NET::TRIANGULATOR_STATE
{
private:
    std::vector<CN_ANCHOR_PTR>  m_allNodes;

    std::list<hed::EDGE_PTR> hedTriangulation( std::vector<hed::NODE_PTR>& aNodes )
    {
        hed::TRIANGULATION triangulator;
        triangulator.CreateDelaunay( aNodes.begin(), aNodes.end() );
        std::list<hed::EDGE_PTR> edges;
        triangulator.GetEdges( edges );

        return edges;
    }


    // Checks if all nodes in aNodes lie on a single line. Requires the nodes to
    // have unique coordinates!
    bool areNodesColinear( const std::vector<hed::NODE_PTR>& aNodes ) const
    {
        if ( aNodes.size() <= 2 )
            return true;

        const auto p0 = aNodes[0]->Pos();
        const auto v0 = aNodes[1]->Pos() - p0;

        for( unsigned i = 2; i < aNodes.size(); i++ )
        {
            const auto v1 = aNodes[i]->Pos() - p0;

            if( v0.Cross( v1 ) != 0 )
            {
                return false;
            }
        }

        return true;
    }

public:

    void Clear()
    {
        m_allNodes.clear();
    }

    void AddNode( CN_ANCHOR_PTR aNode )
    {
        m_allNodes.push_back( aNode );
    }

    const std::priority_queue<CN_EDGE> Triangulate()
    {
        std::priority_queue<CN_EDGE> mstEdges;
        std::list<hed::EDGE_PTR> triangEdges;
        std::vector<hed::NODE_PTR> triNodes;

        using ANCHOR_LIST = std::vector<CN_ANCHOR_PTR>;
        std::vector<ANCHOR_LIST> anchorChains;

        triNodes.reserve( m_allNodes.size() );
        anchorChains.resize( m_allNodes.size() );

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

        for( const auto& n : m_allNodes )
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

        for( const auto& n : triNodes )
        {
            for( int i = prevId; i < n->Id(); i++ )
                anchorChains[prevId].push_back( m_allNodes[ i ] );

            prevId = n->Id();
        }

        for( int i = prevId; i < id; i++ )
            anchorChains[prevId].push_back( m_allNodes[ i ] );

        if( triNodes.size() == 1 )
        {
            return mstEdges;
        }
        else if( areNodesColinear( triNodes ) )
        {
            // special case: all nodes are on the same line - there's no
            // triangulation for such set. In this case, we sort along any coordinate
            // and chain the nodes together.
            for(int i = 0; i < (int)triNodes.size() - 1; i++ )
            {
                auto src = m_allNodes[ triNodes[i]->Id() ];
                auto dst = m_allNodes[ triNodes[i + 1]->Id() ];
                mstEdges.emplace( src, dst, src->Dist( *dst ) );
            }
        }
        else
        {
            hed::TRIANGULATION triangulator;
            triangulator.CreateDelaunay( triNodes.begin(), triNodes.end() );
            triangulator.GetEdges( triangEdges );

            for( const auto& e : triangEdges )
            {
                auto    src = m_allNodes[ e->GetSourceNode()->Id() ];
                auto    dst = m_allNodes[ e->GetTargetNode()->Id() ];

                mstEdges.emplace( src, dst, src->Dist( *dst ) );
            }
        }

        for( unsigned int i = 0; i < anchorChains.size(); i++ )
        {
            auto& chain = anchorChains[i];

            if( chain.size() < 2 )
                continue;

            std::sort( chain.begin(), chain.end(),
                    [] ( const CN_ANCHOR_PTR& a, const CN_ANCHOR_PTR& b ) {
                return a->GetCluster().get() < b->GetCluster().get();
            } );

            for( unsigned int j = 1; j < chain.size(); j++ )
            {
                const auto& prevNode    = chain[j - 1];
                const auto& curNode     = chain[j];
                int weight = prevNode->GetCluster() != curNode->GetCluster() ? 1 : 0;
                mstEdges.emplace( prevNode, curNode, weight );
            }
        }

        return mstEdges;
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
            auto last = ++m_nodes.begin();

            // There can be only one possible connection, but it is missing
            CN_EDGE edge ( *m_nodes.begin(), *last );
            edge.GetSourceNode()->SetTag( 0 );
            edge.GetTargetNode()->SetTag( 1 );

            m_rnEdges.push_back( edge );
        }
        else
        {
            // Set tags to m_nodes as connected
            for( const auto& node : m_nodes )
                node->SetTag( 0 );
        }

        return;
    }


    m_triangulator->Clear();

    for( const auto& n : m_nodes )
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
        triangEdges.push( e );

// Get the minimal spanning tree
#ifdef PROFILE
    PROF_COUNTER cnt2("mst");
#endif
    kruskalMST( triangEdges );
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


void RN_NET::AddCluster( CN_CLUSTER_PTR aCluster )
{
    CN_ANCHOR_PTR firstAnchor;

    for( auto item : *aCluster )
    {
        bool isZone = dynamic_cast<CN_ZONE*>(item) != nullptr;
        auto& anchors = item->Anchors();
        unsigned int nAnchors = isZone ? 1 : anchors.size();

        if( nAnchors > anchors.size() )
            nAnchors = anchors.size();

        for( unsigned int i = 0; i < nAnchors; i++ )
        {
            anchors[i]->SetCluster( aCluster );
            m_nodes.insert( anchors[i] );

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

    auto verify = [&]( auto& aTestNode1, auto& aTestNode2 )
        {
            auto squaredDist = ( aTestNode1->Pos() - aTestNode2->Pos() ).SquaredEuclideanNorm();

            if( squaredDist < distMax )
            {
                rv      = true;
                distMax = squaredDist;
                aNode1  = aTestNode1;
                aNode2  = aTestNode2;
            }
        };

    for( const auto& nodeA : aOtherNet.m_nodes )
    {
        if( nodeA->GetNoLine() )
            continue;

        auto fwd_it = m_nodes.lower_bound( nodeA );
        auto rev_it = std::make_reverse_iterator( fwd_it );

        for( ; fwd_it != m_nodes.end(); ++fwd_it )
        {
            auto nodeB = *fwd_it;

            if( nodeB->GetNoLine() )
                continue;

            VECTOR2I::extended_type distX = nodeA->Pos().x - nodeB->Pos().x;

            if( distX * distX > distMax )
                break;

            verify( nodeA, nodeB );
        }

        if( rev_it != m_nodes.rend() )
            ++rev_it;

        for( ; rev_it != m_nodes.rend(); ++rev_it )
        {
            auto nodeB = *rev_it;

            if( nodeB->GetNoLine() )
                continue;

            VECTOR2I::extended_type distX = nodeA->Pos().x - nodeB->Pos().x;

            if( distX * distX > distMax )
                break;

            verify( nodeA, nodeB );
        }
    }

    return rv;
}


void RN_NET::SetVisible( bool aEnabled )
{
    for( auto& edge : m_rnEdges )
        edge.SetVisible( aEnabled );
}
