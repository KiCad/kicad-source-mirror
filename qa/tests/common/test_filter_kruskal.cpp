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

#include <boost/test/unit_test.hpp>

#include <core/filter_kruskal.h>
#include <core/union_find.h>

#include <algorithm>
#include <random>
#include <tuple>
#include <vector>


namespace
{

struct TEST_EDGE
{
    size_t   u;
    size_t   v;
    int64_t  weight;
};


bool edgeOrder( const TEST_EDGE& aLhs, const TEST_EDGE& aRhs )
{
    return std::tie( aLhs.weight, aLhs.u, aLhs.v ) < std::tie( aRhs.weight, aRhs.u, aRhs.v );
}


std::pair<size_t, size_t> edgeEnds( const TEST_EDGE& aEdge )
{
    return { aEdge.u, aEdge.v };
}


/**
 * Plain sort-then-scan Kruskal.  Filter-Kruskal is only worth having if it agrees with this
 * on every input, so every case below is checked against it rather than against a hand-written
 * expectation.
 */
std::vector<TEST_EDGE> referenceKruskal( std::vector<TEST_EDGE> aEdges, size_t aNodes,
                                         const std::vector<std::pair<size_t, size_t>>& aSeeds )
{
    KI_UNION_FIND forest( aNodes );

    for( const auto& [a, b] : aSeeds )
        forest.Unite( a, b );

    std::sort( aEdges.begin(), aEdges.end(), edgeOrder );

    std::vector<TEST_EDGE> tree;

    for( const TEST_EDGE& edge : aEdges )
    {
        if( forest.Unite( edge.u, edge.v ) )
            tree.push_back( edge );
    }

    return tree;
}


std::vector<TEST_EDGE> filterKruskal( std::vector<TEST_EDGE> aEdges, size_t aNodes,
                                      const std::vector<std::pair<size_t, size_t>>& aSeeds,
                                      size_t& aReturnedCount )
{
    KI_UNION_FIND forest( aNodes );

    for( const auto& [a, b] : aSeeds )
        forest.Unite( a, b );

    std::vector<TEST_EDGE> tree;

    aReturnedCount = KI_MST::FilterKruskal<TEST_EDGE>(
            aEdges, forest, edgeOrder, edgeEnds,
            [&]( const TEST_EDGE& aEdge ) { tree.push_back( aEdge ); } );

    return tree;
}


int64_t totalWeight( const std::vector<TEST_EDGE>& aTree )
{
    int64_t total = 0;

    for( const TEST_EDGE& edge : aTree )
        total += edge.weight;

    return total;
}


/**
 * Compare against the reference on total weight, on the edge set itself, on the emission
 * order, and on the count the algorithm reports.  Weight alone would not catch an
 * implementation that picked a different tree of equal cost.
 */
void checkAgainstReference( const std::vector<TEST_EDGE>& aEdges, size_t aNodes,
                            const std::vector<std::pair<size_t, size_t>>& aSeeds = {} )
{
    size_t                 reportedCount = 0;
    std::vector<TEST_EDGE> got = filterKruskal( aEdges, aNodes, aSeeds, reportedCount );
    std::vector<TEST_EDGE> want = referenceKruskal( aEdges, aNodes, aSeeds );

    BOOST_REQUIRE_EQUAL( reportedCount, got.size() );
    BOOST_REQUIRE_EQUAL( got.size(), want.size() );
    BOOST_REQUIRE_EQUAL( totalWeight( got ), totalWeight( want ) );

    // Emission must be in non-decreasing order; a caller relying on Kruskal semantics may
    // depend on that even though the set is what matters.
    for( size_t ii = 1; ii < got.size(); ++ii )
        BOOST_REQUIRE( !edgeOrder( got[ii], got[ii - 1] ) );

    std::vector<TEST_EDGE> sortedGot = got;
    std::vector<TEST_EDGE> sortedWant = want;
    std::sort( sortedGot.begin(), sortedGot.end(), edgeOrder );
    std::sort( sortedWant.begin(), sortedWant.end(), edgeOrder );

    for( size_t ii = 0; ii < sortedGot.size(); ++ii )
    {
        BOOST_REQUIRE_EQUAL( sortedGot[ii].u, sortedWant[ii].u );
        BOOST_REQUIRE_EQUAL( sortedGot[ii].v, sortedWant[ii].v );
        BOOST_REQUIRE_EQUAL( sortedGot[ii].weight, sortedWant[ii].weight );
    }
}


std::vector<TEST_EDGE> randomEdges( std::mt19937& aRng, size_t aNodes, size_t aCount,
                                    int64_t aWeightRange )
{
    std::vector<TEST_EDGE> edges;
    edges.reserve( aCount );

    for( size_t ii = 0; ii < aCount; ++ii )
    {
        edges.push_back( { aRng() % aNodes, aRng() % aNodes,
                           (int64_t) ( aRng() % (uint32_t) aWeightRange ) } );
    }

    return edges;
}

} // namespace


BOOST_AUTO_TEST_SUITE( FilterKruskal )


/**
 * Sizes are chosen to straddle KRUSKAL_THRESHOLD so both the recursive partitioning path and
 * the direct sort path are exercised.
 */
BOOST_AUTO_TEST_CASE( MatchesPlainKruskal )
{
    std::mt19937 rng( 555 );

    for( int trial = 0; trial < 60; ++trial )
    {
        const size_t nodes = 2 + rng() % 2000;
        const size_t count = rng() % ( 5 * nodes );

        checkAgainstReference( randomEdges( rng, nodes, count, 100000 ), nodes );
    }
}


/**
 * Every weight identical is the case that breaks a naive pivot: the light partition can come
 * back empty and the recursion never shrinks.
 */
BOOST_AUTO_TEST_CASE( AllWeightsEqual )
{
    std::mt19937 rng( 12 );

    for( int trial = 0; trial < 8; ++trial )
    {
        const size_t nodes = 500 + rng() % 2000;
        std::vector<TEST_EDGE> edges = randomEdges( rng, nodes, 5 * nodes, 100000 );

        for( TEST_EDGE& edge : edges )
            edge.weight = 0;

        checkAgainstReference( edges, nodes );
    }
}


BOOST_AUTO_TEST_CASE( FewDistinctWeights )
{
    std::mt19937 rng( 606 );

    for( int trial = 0; trial < 8; ++trial )
    {
        const size_t nodes = 1000 + rng() % 1500;

        checkAgainstReference( randomEdges( rng, nodes, 6 * nodes, 2 ), nodes );
    }
}


/**
 * A disconnected graph yields a spanning forest, not a tree, so the ComponentCount()-based
 * early exit never fires and the recursion has to bottom out on its own.
 */
BOOST_AUTO_TEST_CASE( DisconnectedGraphYieldsForest )
{
    std::mt19937 rng( 808 );

    const size_t nodes = 4000;
    const size_t islands = 8;
    const size_t islandSize = nodes / islands;

    std::vector<TEST_EDGE> edges;

    for( size_t ii = 0; ii < 20000; ++ii )
    {
        size_t island = rng() % islands;
        edges.push_back( { island * islandSize + rng() % islandSize,
                           island * islandSize + rng() % islandSize,
                           (int64_t) ( rng() % 1000 ) } );
    }

    checkAgainstReference( edges, nodes );
}


/**
 * RN_NET seeds the forest with the connections the board already makes before handing over the
 * candidate edges, so pre-united endpoints have to be filtered rather than selected.
 */
BOOST_AUTO_TEST_CASE( HonoursPreSeededForest )
{
    std::mt19937 rng( 909 );

    const size_t nodes = 3000;

    std::vector<std::pair<size_t, size_t>> seeds;

    for( size_t ii = 0; ii < 2000; ++ii )
        seeds.emplace_back( rng() % nodes, rng() % nodes );

    std::vector<TEST_EDGE> edges = randomEdges( rng, nodes, 15000, 5000 );

    for( TEST_EDGE& edge : edges )
        edge.weight += 1;

    checkAgainstReference( edges, nodes, seeds );
}


/**
 * Running the same input twice must give the same tree.  A randomised pivot would satisfy the
 * weight check above but not this, and the ratsnest is drawn from the result.
 */
BOOST_AUTO_TEST_CASE( SelectionIsReproducible )
{
    std::mt19937 rng( 246 );

    const size_t           nodes = 5000;
    std::vector<TEST_EDGE> edges = randomEdges( rng, nodes, 25000, 50 );

    size_t                 count = 0;
    std::vector<TEST_EDGE> first = filterKruskal( edges, nodes, {}, count );

    for( int repeat = 0; repeat < 4; ++repeat )
    {
        std::vector<TEST_EDGE> again = filterKruskal( edges, nodes, {}, count );

        BOOST_REQUIRE_EQUAL( again.size(), first.size() );

        for( size_t ii = 0; ii < again.size(); ++ii )
        {
            BOOST_REQUIRE_EQUAL( again[ii].u, first[ii].u );
            BOOST_REQUIRE_EQUAL( again[ii].v, first[ii].v );
            BOOST_REQUIRE_EQUAL( again[ii].weight, first[ii].weight );
        }
    }
}


/**
 * Reproducibility only follows if the comparator is a total order on *undirected* edges, which
 * takes a canonical endpoint key.  Feed the same graph with the edge list shuffled and
 * individual edges flipped end for end; the selected tree must not move.  Rerunning an
 * identical input, as the case above does, cannot detect the difference.
 */
BOOST_AUTO_TEST_CASE( SelectionIgnoresInputOrderAndOrientation )
{
    auto canonical =
            []( const TEST_EDGE& aEdge )
            {
                size_t low = std::min( aEdge.u, aEdge.v );
                size_t high = std::max( aEdge.u, aEdge.v );

                return std::tuple<int64_t, size_t, size_t>( aEdge.weight, low, high );
            };

    auto canonicalOrder =
            [&canonical]( const TEST_EDGE& aLhs, const TEST_EDGE& aRhs )
            {
                return canonical( aLhs ) < canonical( aRhs );
            };

    std::mt19937 rng( 1357 );

    const size_t           nodes = 4000;
    std::vector<TEST_EDGE> edges = randomEdges( rng, nodes, 20000, 40 );

    auto select =
            [&]( std::vector<TEST_EDGE> aInput )
            {
                KI_UNION_FIND          forest( nodes );
                std::vector<TEST_EDGE> tree;

                KI_MST::FilterKruskal<TEST_EDGE>(
                        aInput, forest, canonicalOrder, edgeEnds,
                        [&]( const TEST_EDGE& aEdge ) { tree.push_back( aEdge ); } );

                std::vector<std::tuple<int64_t, size_t, size_t>> keys;

                for( const TEST_EDGE& edge : tree )
                    keys.push_back( canonical( edge ) );

                std::sort( keys.begin(), keys.end() );

                return keys;
            };

    std::vector<std::tuple<int64_t, size_t, size_t>> expected = select( edges );

    for( int trial = 0; trial < 8; ++trial )
    {
        std::vector<TEST_EDGE> permuted = edges;

        std::shuffle( permuted.begin(), permuted.end(), rng );

        for( TEST_EDGE& edge : permuted )
        {
            if( rng() & 1u )
                std::swap( edge.u, edge.v );
        }

        BOOST_REQUIRE( select( permuted ) == expected );
    }
}


BOOST_AUTO_TEST_CASE( DegenerateInputs )
{
    checkAgainstReference( {}, 1 );
    checkAgainstReference( { { 0, 1, 5 } }, 2 );

    // Self loops can never be selected.
    checkAgainstReference( { { 0, 0, 1 }, { 1, 1, 2 }, { 0, 1, 3 } }, 2 );

    // Parallel edges: only the lightest of each pair may be taken.
    size_t                 count = 0;
    std::vector<TEST_EDGE> tree =
            filterKruskal( { { 0, 1, 9 }, { 0, 1, 2 }, { 0, 1, 7 } }, 2, {}, count );

    BOOST_CHECK_EQUAL( tree.size(), 1 );
    BOOST_CHECK_EQUAL( tree[0].weight, 2 );
}


BOOST_AUTO_TEST_SUITE_END()
