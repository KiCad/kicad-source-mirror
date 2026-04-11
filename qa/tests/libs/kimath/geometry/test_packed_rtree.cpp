/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 3
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

#include <geometry/rtree/packed_rtree.h>

#include <algorithm>
#include <cstdlib>
#include <random>
#include <set>
#include <vector>

using namespace KIRTREE;

BOOST_AUTO_TEST_SUITE( PackedRTree )


BOOST_AUTO_TEST_CASE( EmptyTree )
{
    PACKED_RTREE<intptr_t, int, 2> tree;

    BOOST_CHECK( tree.empty() );
    BOOST_CHECK_EQUAL( tree.size(), 0 );

    int searchMin[2] = { 0, 0 };
    int searchMax[2] = { 100, 100 };

    auto visitor = []( intptr_t ) { return true; };

    BOOST_CHECK_EQUAL( tree.Search( searchMin, searchMax, visitor ), 0 );
    BOOST_CHECK( tree.begin() == tree.end() );
}


BOOST_AUTO_TEST_CASE( SingleItem )
{
    PACKED_RTREE<intptr_t, int, 2>::Builder builder;

    int min[2] = { 10, 20 };
    int max[2] = { 30, 40 };
    builder.Add( min, max, 42 );

    auto tree = builder.Build();

    BOOST_CHECK( !tree.empty() );
    BOOST_CHECK_EQUAL( tree.size(), 1 );

    // Search that overlaps
    int sMin[2] = { 0, 0 };
    int sMax[2] = { 15, 25 };
    std::vector<intptr_t> results;

    auto collect = [&results]( intptr_t val )
    {
        results.push_back( val );
        return true;
    };

    BOOST_CHECK_EQUAL( tree.Search( sMin, sMax, collect ), 1 );
    BOOST_CHECK_EQUAL( results.size(), 1 );
    BOOST_CHECK_EQUAL( results[0], 42 );

    // Search that doesn't overlap
    results.clear();
    int sMin2[2] = { 100, 100 };
    int sMax2[2] = { 200, 200 };
    BOOST_CHECK_EQUAL( tree.Search( sMin2, sMax2, collect ), 0 );
    BOOST_CHECK( results.empty() );
}


BOOST_AUTO_TEST_CASE( KnownGrid )
{
    // Create a 10x10 grid of non-overlapping 10x10 items
    PACKED_RTREE<intptr_t, int, 2>::Builder builder;
    builder.Reserve( 100 );

    for( int y = 0; y < 10; ++y )
    {
        for( int x = 0; x < 10; ++x )
        {
            int min[2] = { x * 10, y * 10 };
            int max[2] = { x * 10 + 10, y * 10 + 10 };
            builder.Add( min, max, static_cast<intptr_t>( y * 10 + x ) );
        }
    }

    auto tree = builder.Build();

    BOOST_CHECK_EQUAL( tree.size(), 100 );

    // Search a region that should contain exactly 4 items (2x2 in top-left)
    int sMin[2] = { 0, 0 };
    int sMax[2] = { 19, 19 };
    std::set<intptr_t> results;

    auto collect = [&results]( intptr_t val )
    {
        results.insert( val );
        return true;
    };

    tree.Search( sMin, sMax, collect );

    BOOST_CHECK_EQUAL( results.size(), 4 );
    BOOST_CHECK( results.count( 0 ) );   // (0,0)
    BOOST_CHECK( results.count( 1 ) );   // (1,0)
    BOOST_CHECK( results.count( 10 ) );  // (0,1)
    BOOST_CHECK( results.count( 11 ) );  // (1,1)

    // Search entire region should return all 100
    results.clear();
    int fullMin[2] = { 0, 0 };
    int fullMax[2] = { 100, 100 };
    tree.Search( fullMin, fullMax, collect );
    BOOST_CHECK_EQUAL( results.size(), 100 );
}


BOOST_AUTO_TEST_CASE( PointItems )
{
    // Zero-area bounding boxes (points)
    PACKED_RTREE<intptr_t, int, 2>::Builder builder;

    for( int i = 0; i < 50; ++i )
    {
        int min[2] = { i * 10, i * 10 };
        int max[2] = { i * 10, i * 10 };
        builder.Add( min, max, static_cast<intptr_t>( i ) );
    }

    auto tree = builder.Build();

    BOOST_CHECK_EQUAL( tree.size(), 50 );

    // Search a point
    int sMin[2] = { 100, 100 };
    int sMax[2] = { 100, 100 };
    std::vector<intptr_t> results;

    auto collect = [&results]( intptr_t val )
    {
        results.push_back( val );
        return true;
    };

    tree.Search( sMin, sMax, collect );
    BOOST_CHECK_EQUAL( results.size(), 1 );
    BOOST_CHECK_EQUAL( results[0], 10 );
}


BOOST_AUTO_TEST_CASE( OverlappingItems )
{
    PACKED_RTREE<intptr_t, int, 2>::Builder builder;

    // All items overlap at the origin region
    for( int i = 0; i < 20; ++i )
    {
        int min[2] = { -i, -i };
        int max[2] = { i + 10, i + 10 };
        builder.Add( min, max, static_cast<intptr_t>( i ) );
    }

    auto tree = builder.Build();

    // Search at origin should find all 20
    int sMin[2] = { 0, 0 };
    int sMax[2] = { 0, 0 };
    int count = 0;

    auto counter = [&count]( intptr_t )
    {
        count++;
        return true;
    };

    tree.Search( sMin, sMax, counter );
    BOOST_CHECK_EQUAL( count, 20 );
}


BOOST_AUTO_TEST_CASE( EarlyTermination )
{
    PACKED_RTREE<intptr_t, int, 2>::Builder builder;

    for( int i = 0; i < 100; ++i )
    {
        int min[2] = { 0, 0 };
        int max[2] = { 100, 100 };
        builder.Add( min, max, static_cast<intptr_t>( i ) );
    }

    auto tree = builder.Build();

    // Visitor that stops after 3 items
    int count = 0;

    auto stopAfter3 = [&count]( intptr_t )
    {
        count++;
        return count < 3;
    };

    int sMin[2] = { 0, 0 };
    int sMax[2] = { 100, 100 };
    tree.Search( sMin, sMax, stopAfter3 );

    BOOST_CHECK_EQUAL( count, 3 );
}


BOOST_AUTO_TEST_CASE( FullIteration )
{
    PACKED_RTREE<intptr_t, int, 2>::Builder builder;

    for( int i = 0; i < 100; ++i )
    {
        int min[2] = { i, i };
        int max[2] = { i + 1, i + 1 };
        builder.Add( min, max, static_cast<intptr_t>( i ) );
    }

    auto tree = builder.Build();

    std::set<intptr_t> iteratedValues;

    for( auto it = tree.begin(); it != tree.end(); ++it )
        iteratedValues.insert( *it );

    BOOST_CHECK_EQUAL( iteratedValues.size(), 100 );

    for( int i = 0; i < 100; ++i )
        BOOST_CHECK( iteratedValues.count( i ) );
}


BOOST_AUTO_TEST_CASE( LargeDatasetBruteForce )
{
    // Verify correctness against brute-force for 10K items
    const int N = 10000;
    std::mt19937 rng( 12345 );
    std::uniform_int_distribution<int> coordDist( -1000000, 1000000 );
    std::uniform_int_distribution<int> sizeDist( 1, 10000 );

    struct ITEM
    {
        int      min[2];
        int      max[2];
        intptr_t id;
    };

    std::vector<ITEM> items( N );
    PACKED_RTREE<intptr_t, int, 2>::Builder builder;
    builder.Reserve( N );

    for( int i = 0; i < N; ++i )
    {
        int x = coordDist( rng );
        int y = coordDist( rng );
        int w = sizeDist( rng );
        int h = sizeDist( rng );

        items[i].min[0] = x;
        items[i].min[1] = y;
        items[i].max[0] = x + w;
        items[i].max[1] = y + h;
        items[i].id = static_cast<intptr_t>( i );

        builder.Add( items[i].min, items[i].max, items[i].id );
    }

    auto tree = builder.Build();

    // Run 100 random queries, verify against brute-force
    for( int q = 0; q < 100; ++q )
    {
        int qx = coordDist( rng );
        int qy = coordDist( rng );
        int qw = sizeDist( rng ) * 10;
        int qh = sizeDist( rng ) * 10;

        int qMin[2] = { qx, qy };
        int qMax[2] = { qx + qw, qy + qh };

        // Brute-force
        std::set<intptr_t> expected;

        for( const ITEM& item : items )
        {
            if( item.min[0] <= qMax[0] && item.max[0] >= qMin[0]
                && item.min[1] <= qMax[1] && item.max[1] >= qMin[1] )
            {
                expected.insert( item.id );
            }
        }

        // R-tree
        std::set<intptr_t> actual;

        auto collect = [&actual]( intptr_t val )
        {
            actual.insert( val );
            return true;
        };

        tree.Search( qMin, qMax, collect );

        BOOST_CHECK_EQUAL( actual.size(), expected.size() );
        BOOST_CHECK( actual == expected );
    }
}


BOOST_AUTO_TEST_CASE( MemoryUsage )
{
    PACKED_RTREE<intptr_t, int, 2>::Builder builder;
    builder.Reserve( 1000 );

    for( int i = 0; i < 1000; ++i )
    {
        int min[2] = { i, i };
        int max[2] = { i + 1, i + 1 };
        builder.Add( min, max, static_cast<intptr_t>( i ) );
    }

    auto tree = builder.Build();

    size_t memUsage = tree.MemoryUsage();

    // Should be reasonable: data (8KB for pointers) + node overhead
    // With fanout=16, 1000 items need ~63 leaf nodes + ~5 internal nodes
    // Memory overhead should be modest relative to item storage
    BOOST_CHECK_GT( memUsage, 1000 * sizeof( intptr_t ) );
    BOOST_CHECK_LT( memUsage, 1000 * sizeof( intptr_t ) * 5 );  // < 5x overhead
}


BOOST_AUTO_TEST_CASE( MoveSemantics )
{
    PACKED_RTREE<intptr_t, int, 2>::Builder builder;

    for( int i = 0; i < 50; ++i )
    {
        int min[2] = { i, i };
        int max[2] = { i + 10, i + 10 };
        builder.Add( min, max, static_cast<intptr_t>( i ) );
    }

    auto tree1 = builder.Build();
    BOOST_CHECK_EQUAL( tree1.size(), 50 );

    // Move construct
    auto tree2 = std::move( tree1 );
    BOOST_CHECK_EQUAL( tree2.size(), 50 );
    BOOST_CHECK_EQUAL( tree1.size(), 0 );
    BOOST_CHECK( tree1.empty() );

    // Move assign
    PACKED_RTREE<intptr_t, int, 2> tree3;
    tree3 = std::move( tree2 );
    BOOST_CHECK_EQUAL( tree3.size(), 50 );
    BOOST_CHECK_EQUAL( tree2.size(), 0 );

    // Verify the moved-to tree still works
    int sMin[2] = { 0, 0 };
    int sMax[2] = { 100, 100 };
    int count = 0;

    auto counter = [&count]( intptr_t )
    {
        count++;
        return true;
    };

    tree3.Search( sMin, sMax, counter );
    BOOST_CHECK_EQUAL( count, 50 );
}


BOOST_AUTO_TEST_CASE( ThreeDimensional )
{
    PACKED_RTREE<intptr_t, int, 3>::Builder builder;

    for( int i = 0; i < 100; ++i )
    {
        int min[3] = { i, i, i };
        int max[3] = { i + 5, i + 5, i + 5 };
        builder.Add( min, max, static_cast<intptr_t>( i ) );
    }

    auto tree = builder.Build();

    BOOST_CHECK_EQUAL( tree.size(), 100 );

    // Search a region
    int sMin[3] = { 10, 10, 10 };
    int sMax[3] = { 20, 20, 20 };
    int count = 0;

    auto counter = [&count]( intptr_t )
    {
        count++;
        return true;
    };

    tree.Search( sMin, sMax, counter );
    BOOST_CHECK_GT( count, 0 );
}


BOOST_AUTO_TEST_SUITE_END()
