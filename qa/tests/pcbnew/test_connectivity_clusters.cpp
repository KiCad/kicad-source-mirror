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
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include <qa_utils/wx_utils/unit_test_utils.h>
#include <boost/test/data/test_case.hpp>

#include <pcbnew_utils/board_test_utils.h>

#include <board.h>
#include <connectivity/connectivity_algo.h>
#include <connectivity/connectivity_data.h>
#include <ratsnest/ratsnest_data.h>
#include <settings/settings_manager.h>

#include <deque>
#include <map>
#include <set>

/*
 * SearchClusters() computes connected components and RN_NET builds a minimum spanning forest
 * over them.  Both are stated as properties here rather than as recorded numbers, so the
 * checks stand on their own rather than on whatever the current implementation happens to
 * produce.
 */

namespace
{

struct CONNECTIVITY_CLUSTER_FIXTURE
{
    SETTINGS_MANAGER       m_settingsManager;
    std::unique_ptr<BOARD> m_board;
};


/// Boards carrying a mix of tracks, vias, zones and unrouted nets.
const std::vector<wxString> c_boards = {
    "issue22267",
    "issue17429",
    "issue16182",
    "issue14559",
    "issue8909",
};

} // namespace


BOOST_FIXTURE_TEST_SUITE( ConnectivityClusters, CONNECTIVITY_CLUSTER_FIXTURE )


/**
 * Every item must land in exactly one cluster, each cluster must be internally reachable over
 * the adjacency, and no adjacency may cross between two clusters.  That is the definition of a
 * connected component, so a union-find that links too much or too little cannot satisfy it.
 */
BOOST_DATA_TEST_CASE( ClustersAreTheConnectedComponents, boost::unit_test::data::make( c_boards ),
                      boardName )
{
    KI_TEST::LoadBoard( m_settingsManager, boardName, m_board );
    KI_TEST::FillZones( m_board.get() );
    m_board->BuildConnectivity();

    std::shared_ptr<CN_CONNECTIVITY_ALGO> algo =
            m_board->GetConnectivity()->GetConnectivityAlgo();

    CN_CONNECTIVITY_ALGO::CLUSTERS clusters =
            algo->SearchClusters( CN_CONNECTIVITY_ALGO::CSM_RATSNEST );

    BOOST_REQUIRE( !clusters.empty() );

    std::map<CN_ITEM*, size_t> clusterOf;
    size_t                     placed = 0;

    for( size_t ii = 0; ii < clusters.size(); ++ii )
    {
        for( CN_ITEM* item : *clusters[ii] )
        {
            BOOST_REQUIRE_MESSAGE( clusterOf.emplace( item, ii ).second,
                                   "item appears in more than one cluster" );
            placed++;
        }
    }

    BOOST_CHECK_EQUAL( placed, clusterOf.size() );

    // No adjacency between two items that both took part may span two clusters, or they would
    // be one component and the search split them.
    for( const auto& [item, cluster] : clusterOf )
    {
        for( CN_ITEM* neighbour : item->ConnectedItems() )
        {
            auto it = clusterOf.find( neighbour );

            if( it == clusterOf.end() )
                continue;

            if( neighbour->Net() != item->Net() )
                continue;

            BOOST_REQUIRE_MESSAGE( it->second == cluster,
                                   "connected items landed in different clusters" );
        }
    }

    // Conversely each cluster has to be reachable end to end, or the search merged components
    // that share no path.
    for( const std::shared_ptr<CN_CLUSTER>& cluster : clusters )
    {
        std::set<CN_ITEM*> members( cluster->begin(), cluster->end() );
        std::set<CN_ITEM*> seen;
        std::deque<CN_ITEM*> queue;

        queue.push_back( *cluster->begin() );
        seen.insert( *cluster->begin() );

        while( !queue.empty() )
        {
            CN_ITEM* current = queue.front();
            queue.pop_front();

            for( CN_ITEM* neighbour : current->ConnectedItems() )
            {
                if( members.count( neighbour ) && !seen.count( neighbour ) )
                {
                    seen.insert( neighbour );
                    queue.push_back( neighbour );
                }
            }
        }

        BOOST_REQUIRE_EQUAL( seen.size(), members.size() );
    }
}


/**
 * A ratsnest is a spanning forest over the clusters of a net, so it must hold exactly one edge
 * fewer than there are clusters.  Too many means a cycle survived, too few means the net was
 * left split.  That the forest is also of minimum weight is covered by the FilterKruskal suite,
 * which checks the selection against plain sort-then-scan Kruskal.
 */
BOOST_DATA_TEST_CASE( RatsnestSpansTheClustersOfEachNet,
                      boost::unit_test::data::make( c_boards ), boardName )
{
    KI_TEST::LoadBoard( m_settingsManager, boardName, m_board );
    KI_TEST::FillZones( m_board.get() );
    m_board->BuildConnectivity();

    std::shared_ptr<CONNECTIVITY_DATA> conn = m_board->GetConnectivity();
    std::shared_ptr<CN_CONNECTIVITY_ALGO> algo = conn->GetConnectivityAlgo();

    std::map<int, std::set<const CN_CLUSTER*>> clustersPerNet;

    for( const std::shared_ptr<CN_CLUSTER>& cluster :
         algo->SearchClusters( CN_CONNECTIVITY_ALGO::CSM_RATSNEST ) )
    {
        // A kept island contributes no anchor, matching what AddCluster() skips.
        if( cluster->IsOrphaned() && cluster->Size() == 1
            && dynamic_cast<CN_ZONE_LAYER*>( *cluster->begin() ) )
        {
            continue;
        }

        if( cluster->OriginNet() > 0 )
            clustersPerNet[cluster->OriginNet()].insert( cluster.get() );
    }

    size_t netsChecked = 0;
    size_t edgesSeen = 0;

    for( const auto& [net, netClusters] : clustersPerNet )
    {
        RN_NET* rn = conn->GetRatsnestForNet( net );

        if( !rn || rn->GetNodeCount() == 0 )
            continue;

        const std::vector<CN_EDGE>& edges = rn->GetEdges();

        BOOST_REQUIRE_MESSAGE( edges.size() == netClusters.size() - 1,
                               "net " << net << " has " << netClusters.size() << " clusters but "
                                      << edges.size() << " ratsnest edges" );

        for( const CN_EDGE& edge : edges )
            BOOST_REQUIRE( edge.GetSourceNode() && edge.GetTargetNode() );

        netsChecked++;
        edgesSeen += edges.size();
    }

    BOOST_REQUIRE_MESSAGE( netsChecked > 0, "no net on this board exercised the ratsnest" );
    BOOST_TEST_MESSAGE( "net count " << netsChecked << ", ratsnest edges " << edgesSeen );
}


BOOST_AUTO_TEST_SUITE_END()
