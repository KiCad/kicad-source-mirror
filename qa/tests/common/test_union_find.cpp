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

#include <core/union_find.h>

#include <algorithm>
#include <atomic>
#include <map>
#include <random>
#include <thread>
#include <vector>


namespace
{

/**
 * Textbook union-by-size with full path compression.  Deliberately a different algorithm from
 * the one under test so that agreement means something.
 */
class REFERENCE_SET
{
public:
    explicit REFERENCE_SET( size_t aCount ) : m_parent( aCount ), m_size( aCount, 1 )
    {
        for( size_t ii = 0; ii < aCount; ++ii )
            m_parent[ii] = ii;
    }

    size_t Find( size_t aX )
    {
        while( m_parent[aX] != aX )
        {
            m_parent[aX] = m_parent[m_parent[aX]];
            aX = m_parent[aX];
        }

        return aX;
    }

    bool Unite( size_t aA, size_t aB )
    {
        aA = Find( aA );
        aB = Find( aB );

        if( aA == aB )
            return false;

        if( m_size[aA] < m_size[aB] )
            std::swap( aA, aB );

        m_parent[aB] = aA;
        m_size[aA] += m_size[aB];

        return true;
    }

private:
    std::vector<size_t> m_parent;
    std::vector<size_t> m_size;
};


std::vector<std::pair<size_t, size_t>> randomEdges( std::mt19937& aRng, size_t aNodes,
                                                    size_t aCount )
{
    std::vector<std::pair<size_t, size_t>> edges;
    edges.reserve( aCount );

    for( size_t ii = 0; ii < aCount; ++ii )
        edges.emplace_back( aRng() % aNodes, aRng() % aNodes );

    return edges;
}

} // namespace


BOOST_AUTO_TEST_SUITE( UnionFind )


/**
 * The partition, the merge count and the component count must all track a reference
 * implementation exactly, across a spread of graph densities.
 */
BOOST_AUTO_TEST_CASE( MatchesReferenceImplementation )
{
    std::mt19937 rng( 4242 );

    for( int trial = 0; trial < 100; ++trial )
    {
        const size_t nodes = 1 + rng() % 300;
        const size_t count = rng() % ( 3 * nodes + 1 );

        std::vector<std::pair<size_t, size_t>> edges = randomEdges( rng, nodes, count );

        KI_UNION_FIND under( nodes );
        REFERENCE_SET reference( nodes );

        size_t merges = 0;
        size_t referenceMerges = 0;

        for( const auto& [a, b] : edges )
        {
            merges += under.Unite( a, b ) ? 1 : 0;
            referenceMerges += reference.Unite( a, b ) ? 1 : 0;
        }

        BOOST_REQUIRE_EQUAL( merges, referenceMerges );
        BOOST_REQUIRE_EQUAL( under.ComponentCount(), nodes - referenceMerges );

        for( size_t i = 0; i < nodes; ++i )
        {
            for( size_t j = 0; j < nodes; ++j )
            {
                BOOST_REQUIRE_EQUAL( under.Connected( i, j ),
                                     reference.Find( i ) == reference.Find( j ) );
            }
        }
    }
}


/**
 * Rem's algorithm roots every tree at the smallest index it holds.  SearchClusters() relies on
 * this only through Find() agreeing with itself, but a broken link direction would show up
 * here first and would silently corrupt the clustering rather than crash.
 */
BOOST_AUTO_TEST_CASE( RootIsMinimumOfComponent )
{
    std::mt19937  rng( 99 );
    const size_t  nodes = 2000;
    KI_UNION_FIND under( nodes );
    REFERENCE_SET reference( nodes );

    for( const auto& [a, b] : randomEdges( rng, nodes, 5000 ) )
    {
        under.Unite( a, b );
        reference.Unite( a, b );
    }

    std::map<size_t, size_t> minimumOf;

    for( size_t ii = 0; ii < nodes; ++ii )
    {
        size_t root = reference.Find( ii );
        auto   it = minimumOf.find( root );

        if( it == minimumOf.end() )
            minimumOf[root] = ii;
    }

    for( size_t ii = 0; ii < nodes; ++ii )
        BOOST_REQUIRE_EQUAL( under.Find( ii ), minimumOf[reference.Find( ii )] );
}


/**
 * FindCompress() rewrites the tree as it walks.  It must return what Find() would have, and
 * must not disturb the partition.
 */
BOOST_AUTO_TEST_CASE( CompressionPreservesPartition )
{
    std::mt19937  rng( 7 );
    const size_t  nodes = 4000;
    KI_UNION_FIND under( nodes );
    REFERENCE_SET reference( nodes );

    for( const auto& [a, b] : randomEdges( rng, nodes, 9000 ) )
    {
        under.Unite( a, b );
        reference.Unite( a, b );
    }

    std::vector<size_t> before( nodes );

    for( size_t ii = 0; ii < nodes; ++ii )
        before[ii] = under.Find( ii );

    for( size_t ii = 0; ii < nodes; ++ii )
        BOOST_REQUIRE_EQUAL( under.FindCompress( ii ), before[ii] );

    for( size_t ii = 0; ii < nodes; ++ii )
        BOOST_REQUIRE_EQUAL( under.Find( ii ), before[ii] );
}


/**
 * Unite() is the one entry point SearchClusters() calls from several threads at once.  The
 * final partition must not depend on how the edges were divided up, and exactly one caller may
 * see true per merge or Kruskal would emit a cycle.
 */
BOOST_AUTO_TEST_CASE( ConcurrentUniteMatchesSerial )
{
    std::mt19937 rng( 31337 );
    const size_t nodes = 20000;

    std::vector<std::pair<size_t, size_t>> edges = randomEdges( rng, nodes, 50000 );

    KI_UNION_FIND    under( nodes );
    std::atomic<int> merges{ 0 };

    const unsigned threadCount = std::max( 2u, std::thread::hardware_concurrency() );
    std::vector<std::thread> threads;

    for( unsigned t = 0; t < threadCount; ++t )
    {
        threads.emplace_back(
                [&, t]
                {
                    int local = 0;

                    for( size_t ii = t; ii < edges.size(); ii += threadCount )
                        local += under.Unite( edges[ii].first, edges[ii].second ) ? 1 : 0;

                    merges += local;
                } );
    }

    for( std::thread& thread : threads )
        thread.join();

    REFERENCE_SET reference( nodes );
    int           referenceMerges = 0;

    for( const auto& [a, b] : edges )
        referenceMerges += reference.Unite( a, b ) ? 1 : 0;

    BOOST_CHECK_EQUAL( merges.load(), referenceMerges );
    BOOST_CHECK_EQUAL( under.ComponentCount(), nodes - referenceMerges );

    for( size_t ii = 0; ii < nodes; ++ii )
    {
        size_t other = ( ii * 7919 ) % nodes;
        BOOST_REQUIRE_EQUAL( under.Connected( ii, other ),
                             reference.Find( ii ) == reference.Find( other ) );
    }
}


/**
 * A chain united back to front is the worst case for Rem's interleaved climb: every union
 * walks the length of the tree built so far.  It must still terminate and collapse to one
 * component.
 */
BOOST_AUTO_TEST_CASE( ReverseChainUnderContention )
{
    const size_t     nodes = 50000;
    KI_UNION_FIND    under( nodes );
    std::atomic<int> merges{ 0 };

    const unsigned threadCount = std::max( 2u, std::thread::hardware_concurrency() );
    std::vector<std::thread> threads;

    for( unsigned t = 0; t < threadCount; ++t )
    {
        threads.emplace_back(
                [&, t]
                {
                    int local = 0;

                    for( long ii = (long) nodes - 2 - t; ii >= 0; ii -= threadCount )
                        local += under.Unite( ii, ii + 1 ) ? 1 : 0;

                    merges += local;
                } );
    }

    for( std::thread& thread : threads )
        thread.join();

    BOOST_CHECK_EQUAL( merges.load(), (int) nodes - 1 );
    BOOST_CHECK_EQUAL( under.ComponentCount(), 1 );

    for( size_t ii = 0; ii < nodes; ii += 997 )
        BOOST_REQUIRE_EQUAL( under.Find( ii ), 0 );
}


BOOST_AUTO_TEST_CASE( DegenerateSizes )
{
    KI_UNION_FIND empty( 0 );
    BOOST_CHECK_EQUAL( empty.Size(), 0 );
    BOOST_CHECK_EQUAL( empty.ComponentCount(), 0 );

    KI_UNION_FIND singleton( 1 );
    BOOST_CHECK( !singleton.Unite( 0, 0 ) );
    BOOST_CHECK_EQUAL( singleton.ComponentCount(), 1 );
    BOOST_CHECK( singleton.Connected( 0, 0 ) );

    KI_UNION_FIND pair( 2 );
    BOOST_CHECK( pair.Unite( 1, 0 ) );
    BOOST_CHECK( !pair.Unite( 0, 1 ) );
    BOOST_CHECK_EQUAL( pair.ComponentCount(), 1 );
    BOOST_CHECK_EQUAL( pair.Find( 1 ), 0 );
}


BOOST_AUTO_TEST_CASE( ResetClearsState )
{
    KI_UNION_FIND under( 10 );

    for( size_t ii = 1; ii < 10; ++ii )
        under.Unite( 0, ii );

    BOOST_CHECK_EQUAL( under.ComponentCount(), 1 );

    under.Reset( 5 );

    BOOST_CHECK_EQUAL( under.Size(), 5 );
    BOOST_CHECK_EQUAL( under.ComponentCount(), 5 );

    for( size_t ii = 0; ii < 5; ++ii )
        BOOST_CHECK_EQUAL( under.Find( ii ), ii );
}


BOOST_AUTO_TEST_SUITE_END()
