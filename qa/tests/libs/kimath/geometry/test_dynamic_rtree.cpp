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

#include <geometry/rtree/dynamic_rtree.h>
#include <geometry/rtree/dynamic_rtree_cow.h>

#include <algorithm>
#include <random>
#include <set>
#include <vector>

using namespace KIRTREE;

BOOST_AUTO_TEST_SUITE( DynamicRTree )


BOOST_AUTO_TEST_CASE( EmptyTree )
{
    DYNAMIC_RTREE<intptr_t, int, 2> tree;

    BOOST_CHECK( tree.empty() );
    BOOST_CHECK_EQUAL( tree.size(), 0 );

    int searchMin[2] = { 0, 0 };
    int searchMax[2] = { 100, 100 };

    auto visitor = []( intptr_t ) { return true; };

    BOOST_CHECK_EQUAL( tree.Search( searchMin, searchMax, visitor ), 0 );
    BOOST_CHECK( tree.begin() == tree.end() );
}


BOOST_AUTO_TEST_CASE( SingleInsert )
{
    DYNAMIC_RTREE<intptr_t, int, 2> tree;

    int min[2] = { 10, 20 };
    int max[2] = { 30, 40 };
    tree.Insert( min, max, 42 );

    BOOST_CHECK_EQUAL( tree.size(), 1 );
    BOOST_CHECK( !tree.empty() );

    // Search overlapping
    int sMin[2] = { 0, 0 };
    int sMax[2] = { 15, 25 };
    std::vector<intptr_t> results;

    auto collect = [&results]( intptr_t val )
    {
        results.push_back( val );
        return true;
    };

    BOOST_CHECK_EQUAL( tree.Search( sMin, sMax, collect ), 1 );
    BOOST_CHECK_EQUAL( results[0], 42 );

    // Search disjoint
    results.clear();
    int sMin2[2] = { 100, 100 };
    int sMax2[2] = { 200, 200 };
    BOOST_CHECK_EQUAL( tree.Search( sMin2, sMax2, collect ), 0 );
}


BOOST_AUTO_TEST_CASE( BulkInsert1K )
{
    DYNAMIC_RTREE<intptr_t, int, 2> tree;

    for( int i = 0; i < 1000; ++i )
    {
        int min[2] = { i * 10, i * 10 };
        int max[2] = { i * 10 + 9, i * 10 + 9 };
        tree.Insert( min, max, static_cast<intptr_t>( i ) );
    }

    BOOST_CHECK_EQUAL( tree.size(), 1000 );

    // Search for specific item
    int sMin[2] = { 500 * 10, 500 * 10 };
    int sMax[2] = { 500 * 10 + 9, 500 * 10 + 9 };
    std::vector<intptr_t> results;

    auto collect = [&results]( intptr_t val )
    {
        results.push_back( val );
        return true;
    };

    tree.Search( sMin, sMax, collect );
    BOOST_CHECK( std::find( results.begin(), results.end(), 500 ) != results.end() );
}


BOOST_AUTO_TEST_CASE( RemoveExisting )
{
    DYNAMIC_RTREE<intptr_t, int, 2> tree;

    int min1[2] = { 0, 0 };
    int max1[2] = { 10, 10 };
    tree.Insert( min1, max1, 1 );

    int min2[2] = { 20, 20 };
    int max2[2] = { 30, 30 };
    tree.Insert( min2, max2, 2 );

    BOOST_CHECK_EQUAL( tree.size(), 2 );

    // Remove first item
    BOOST_CHECK( tree.Remove( min1, max1, 1 ) );
    BOOST_CHECK_EQUAL( tree.size(), 1 );

    // Verify it's gone
    std::vector<intptr_t> results;

    auto collect = [&results]( intptr_t val )
    {
        results.push_back( val );
        return true;
    };

    int fullMin[2] = { -100, -100 };
    int fullMax[2] = { 100, 100 };
    tree.Search( fullMin, fullMax, collect );

    BOOST_CHECK_EQUAL( results.size(), 1 );
    BOOST_CHECK_EQUAL( results[0], 2 );
}


BOOST_AUTO_TEST_CASE( RemoveNonExistent )
{
    DYNAMIC_RTREE<intptr_t, int, 2> tree;

    int min[2] = { 0, 0 };
    int max[2] = { 10, 10 };
    tree.Insert( min, max, 1 );

    int fakeMin[2] = { 100, 100 };
    int fakeMax[2] = { 200, 200 };
    BOOST_CHECK( !tree.Remove( fakeMin, fakeMax, 999 ) );
    BOOST_CHECK_EQUAL( tree.size(), 1 );
}


BOOST_AUTO_TEST_CASE( SearchOverlapDisjointAll )
{
    DYNAMIC_RTREE<intptr_t, int, 2> tree;

    // Insert items in distinct quadrants
    int min1[2] = { 0, 0 };
    int max1[2] = { 10, 10 };
    tree.Insert( min1, max1, 1 );

    int min2[2] = { 100, 0 };
    int max2[2] = { 110, 10 };
    tree.Insert( min2, max2, 2 );

    int min3[2] = { 0, 100 };
    int max3[2] = { 10, 110 };
    tree.Insert( min3, max3, 3 );

    int min4[2] = { 100, 100 };
    int max4[2] = { 110, 110 };
    tree.Insert( min4, max4, 4 );

    // Search left side only
    std::set<intptr_t> results;

    auto collect = [&results]( intptr_t val )
    {
        results.insert( val );
        return true;
    };

    int sMin[2] = { -10, -10 };
    int sMax[2] = { 50, 200 };
    tree.Search( sMin, sMax, collect );
    BOOST_CHECK_EQUAL( results.size(), 2 );
    BOOST_CHECK( results.count( 1 ) );
    BOOST_CHECK( results.count( 3 ) );

    // Search disjoint area
    results.clear();
    int dMin[2] = { 50, 50 };
    int dMax[2] = { 90, 90 };
    tree.Search( dMin, dMax, collect );
    BOOST_CHECK( results.empty() );

    // Search all
    results.clear();
    int aMin[2] = { -1000, -1000 };
    int aMax[2] = { 1000, 1000 };
    tree.Search( aMin, aMax, collect );
    BOOST_CHECK_EQUAL( results.size(), 4 );
}


BOOST_AUTO_TEST_CASE( BulkInsertAndBruteForce )
{
    const int N = 5000;
    std::mt19937 rng( 67890 );
    std::uniform_int_distribution<int> coordDist( -500000, 500000 );
    std::uniform_int_distribution<int> sizeDist( 1, 5000 );

    struct ITEM
    {
        int      min[2];
        int      max[2];
        intptr_t id;
    };

    std::vector<ITEM> items( N );
    DYNAMIC_RTREE<intptr_t, int, 2> tree;

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

        tree.Insert( items[i].min, items[i].max, items[i].id );
    }

    BOOST_CHECK_EQUAL( tree.size(), N );

    // Verify with random queries
    for( int q = 0; q < 50; ++q )
    {
        int qx = coordDist( rng );
        int qy = coordDist( rng );
        int qw = sizeDist( rng ) * 10;
        int qh = sizeDist( rng ) * 10;

        int qMin[2] = { qx, qy };
        int qMax[2] = { qx + qw, qy + qh };

        std::set<intptr_t> expected;

        for( const ITEM& item : items )
        {
            if( item.min[0] <= qMax[0] && item.max[0] >= qMin[0]
                && item.min[1] <= qMax[1] && item.max[1] >= qMin[1] )
            {
                expected.insert( item.id );
            }
        }

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


BOOST_AUTO_TEST_CASE( RemoveAndVerify )
{
    const int N = 200;
    DYNAMIC_RTREE<intptr_t, int, 2> tree;

    struct ITEM
    {
        int      min[2];
        int      max[2];
        intptr_t id;
    };

    std::vector<ITEM> items( N );

    for( int i = 0; i < N; ++i )
    {
        items[i].min[0] = i * 5;
        items[i].min[1] = i * 5;
        items[i].max[0] = i * 5 + 4;
        items[i].max[1] = i * 5 + 4;
        items[i].id = static_cast<intptr_t>( i );

        tree.Insert( items[i].min, items[i].max, items[i].id );
    }

    // Remove every other item
    for( int i = 0; i < N; i += 2 )
        BOOST_CHECK( tree.Remove( items[i].min, items[i].max, items[i].id ) );

    BOOST_CHECK_EQUAL( tree.size(), N / 2 );

    // Verify remaining items are all odd
    std::set<intptr_t> remaining;

    auto collect = [&remaining]( intptr_t val )
    {
        remaining.insert( val );
        return true;
    };

    int fullMin[2] = { -10000, -10000 };
    int fullMax[2] = { 10000, 10000 };
    tree.Search( fullMin, fullMax, collect );

    BOOST_CHECK_EQUAL( remaining.size(), N / 2 );

    for( int i = 1; i < N; i += 2 )
        BOOST_CHECK( remaining.count( i ) );
}


BOOST_AUTO_TEST_CASE( MoveSemantics )
{
    DYNAMIC_RTREE<intptr_t, int, 2> tree;

    for( int i = 0; i < 50; ++i )
    {
        int min[2] = { i, i };
        int max[2] = { i + 5, i + 5 };
        tree.Insert( min, max, static_cast<intptr_t>( i ) );
    }

    BOOST_CHECK_EQUAL( tree.size(), 50 );

    // Move construct
    DYNAMIC_RTREE<intptr_t, int, 2> tree2( std::move( tree ) );
    BOOST_CHECK_EQUAL( tree2.size(), 50 );
    BOOST_CHECK_EQUAL( tree.size(), 0 );

    // Move assign
    DYNAMIC_RTREE<intptr_t, int, 2> tree3;
    tree3 = std::move( tree2 );
    BOOST_CHECK_EQUAL( tree3.size(), 50 );
    BOOST_CHECK_EQUAL( tree2.size(), 0 );

    // Verify the moved-to tree works
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


BOOST_AUTO_TEST_CASE( RemoveAll )
{
    DYNAMIC_RTREE<intptr_t, int, 2> tree;

    for( int i = 0; i < 100; ++i )
    {
        int min[2] = { i, i };
        int max[2] = { i + 1, i + 1 };
        tree.Insert( min, max, static_cast<intptr_t>( i ) );
    }

    BOOST_CHECK_EQUAL( tree.size(), 100 );

    tree.RemoveAll();
    BOOST_CHECK_EQUAL( tree.size(), 0 );
    BOOST_CHECK( tree.empty() );

    // Tree should be usable after RemoveAll
    int min[2] = { 0, 0 };
    int max[2] = { 10, 10 };
    tree.Insert( min, max, 999 );
    BOOST_CHECK_EQUAL( tree.size(), 1 );
}


BOOST_AUTO_TEST_CASE( ThreeDimensional )
{
    DYNAMIC_RTREE<intptr_t, int, 3> tree;

    for( int i = 0; i < 100; ++i )
    {
        int min[3] = { i, i, i };
        int max[3] = { i + 5, i + 5, i + 5 };
        tree.Insert( min, max, static_cast<intptr_t>( i ) );
    }

    BOOST_CHECK_EQUAL( tree.size(), 100 );

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


BOOST_AUTO_TEST_CASE( NearestNeighbors )
{
    DYNAMIC_RTREE<intptr_t, int, 2> tree;

    // Insert items at known positions
    for( int i = 0; i < 20; ++i )
    {
        int min[2] = { i * 100, 0 };
        int max[2] = { i * 100 + 10, 10 };
        tree.Insert( min, max, static_cast<intptr_t>( i ) );
    }

    // Find 3 nearest to origin
    int point[2] = { 0, 0 };
    std::vector<std::pair<int64_t, intptr_t>> results;
    tree.NearestNeighbors( point, 3, results );

    BOOST_CHECK_EQUAL( results.size(), 3 );

    // First result should be item 0 (at origin)
    BOOST_CHECK_EQUAL( results[0].second, 0 );

    // Results should be in ascending distance order
    for( size_t i = 1; i < results.size(); ++i )
        BOOST_CHECK_GE( results[i].first, results[i - 1].first );
}


BOOST_AUTO_TEST_CASE( IteratorFullCoverage )
{
    DYNAMIC_RTREE<intptr_t, int, 2> tree;

    for( int i = 0; i < 200; ++i )
    {
        int min[2] = { i * 3, i * 7 };
        int max[2] = { i * 3 + 2, i * 7 + 6 };
        tree.Insert( min, max, static_cast<intptr_t>( i ) );
    }

    std::set<intptr_t> iterated;

    for( const auto& val : tree )
        iterated.insert( val );

    BOOST_CHECK_EQUAL( iterated.size(), 200 );

    for( int i = 0; i < 200; ++i )
        BOOST_CHECK( iterated.count( i ) );
}


BOOST_AUTO_TEST_CASE( EarlyTermination )
{
    DYNAMIC_RTREE<intptr_t, int, 2> tree;

    for( int i = 0; i < 100; ++i )
    {
        int min[2] = { 0, 0 };
        int max[2] = { 100, 100 };
        tree.Insert( min, max, static_cast<intptr_t>( i ) );
    }

    int count = 0;

    auto stopAfter5 = [&count]( intptr_t )
    {
        count++;
        return count < 5;
    };

    int sMin[2] = { 0, 0 };
    int sMax[2] = { 100, 100 };
    tree.Search( sMin, sMax, stopAfter5 );

    BOOST_CHECK_EQUAL( count, 5 );
}


BOOST_AUTO_TEST_CASE( RemoveMovedItem )
{
    // Simulate an item whose bbox changes after insertion (e.g., VIEW_ITEM that moves).
    // DYNAMIC_RTREE::Remove falls back to full-tree search when the provided bbox
    // doesn't match the stored insertion bbox.
    DYNAMIC_RTREE<intptr_t, int, 2> tree;

    int origMin[2] = { 100, 100 };
    int origMax[2] = { 200, 200 };
    tree.Insert( origMin, origMax, 42 );

    int otherMin[2] = { 500, 500 };
    int otherMax[2] = { 600, 600 };
    tree.Insert( otherMin, otherMax, 99 );

    BOOST_CHECK_EQUAL( tree.size(), 2 );

    // Try to remove item 42 using a DIFFERENT bbox (simulating the item having moved).
    // The primary search won't find it, but the fallback full-tree search should.
    int movedMin[2] = { 300, 300 };
    int movedMax[2] = { 400, 400 };
    BOOST_CHECK( tree.Remove( movedMin, movedMax, 42 ) );
    BOOST_CHECK_EQUAL( tree.size(), 1 );

    // Verify item 99 is still there
    std::vector<intptr_t> results;

    auto collect = [&results]( intptr_t val )
    {
        results.push_back( val );
        return true;
    };

    int fullMin[2] = { -1000, -1000 };
    int fullMax[2] = { 1000, 1000 };
    tree.Search( fullMin, fullMax, collect );

    BOOST_CHECK_EQUAL( results.size(), 1 );
    BOOST_CHECK_EQUAL( results[0], 99 );
}


BOOST_AUTO_TEST_CASE( StressInterleavedInsertRemoveQuery )
{
    const int N = 1000;
    std::mt19937 rng( 11111 );
    std::uniform_int_distribution<int> coordDist( 0, 100000 );
    std::uniform_int_distribution<int> sizeDist( 100, 5000 );
    std::uniform_int_distribution<int> opDist( 0, 9 );

    DYNAMIC_RTREE<intptr_t, int, 2> tree;

    struct ITEM
    {
        int      min[2];
        int      max[2];
        intptr_t id;
    };

    std::vector<ITEM> liveItems;

    for( int op = 0; op < N; ++op )
    {
        int which = opDist( rng );

        if( which < 5 || liveItems.empty() )
        {
            // Insert
            ITEM item;
            item.min[0] = coordDist( rng );
            item.min[1] = coordDist( rng );
            item.max[0] = item.min[0] + sizeDist( rng );
            item.max[1] = item.min[1] + sizeDist( rng );
            item.id = static_cast<intptr_t>( op );

            tree.Insert( item.min, item.max, item.id );
            liveItems.push_back( item );
        }
        else if( which < 7 )
        {
            // Remove
            std::uniform_int_distribution<int> idxDist( 0, liveItems.size() - 1 );
            int idx = idxDist( rng );
            ITEM& item = liveItems[idx];

            BOOST_CHECK( tree.Remove( item.min, item.max, item.id ) );

            liveItems.erase( liveItems.begin() + idx );
        }
        else
        {
            // Query
            int qMin[2] = { coordDist( rng ), coordDist( rng ) };
            int qMax[2] = { qMin[0] + sizeDist( rng ) * 5, qMin[1] + sizeDist( rng ) * 5 };

            std::set<intptr_t> expected;

            for( const ITEM& item : liveItems )
            {
                if( item.min[0] <= qMax[0] && item.max[0] >= qMin[0]
                    && item.min[1] <= qMax[1] && item.max[1] >= qMin[1] )
                {
                    expected.insert( item.id );
                }
            }

            std::set<intptr_t> actual;

            auto collect = [&actual]( intptr_t val )
            {
                actual.insert( val );
                return true;
            };

            tree.Search( qMin, qMax, collect );
            BOOST_CHECK_EQUAL( actual.size(), expected.size() );
        }
    }

    BOOST_CHECK_EQUAL( tree.size(), liveItems.size() );
}


BOOST_AUTO_TEST_CASE( BulkLoadEmpty )
{
    DYNAMIC_RTREE<int, int, 2> tree;
    std::vector<DYNAMIC_RTREE<int, int, 2>::BULK_ENTRY> entries;
    tree.BulkLoad( entries );

    BOOST_CHECK_EQUAL( tree.size(), 0u );
    BOOST_CHECK( tree.empty() );
}


BOOST_AUTO_TEST_CASE( BulkLoadSingleItem )
{
    DYNAMIC_RTREE<int, int, 2> tree;
    std::vector<DYNAMIC_RTREE<int, int, 2>::BULK_ENTRY> entries;
    entries.push_back( { { 10, 20 }, { 30, 40 }, 42 } );
    tree.BulkLoad( entries );

    BOOST_CHECK_EQUAL( tree.size(), 1u );

    std::vector<int> results;
    auto collect = [&results]( int val ) { results.push_back( val ); return true; };
    int qMin[2] = { 0, 0 };
    int qMax[2] = { 100, 100 };
    tree.Search( qMin, qMax, collect );

    BOOST_CHECK_EQUAL( results.size(), 1u );
    BOOST_CHECK_EQUAL( results[0], 42 );
}


BOOST_AUTO_TEST_CASE( BulkLoadCorrectnessVsBruteForce )
{
    const int N = 5000;
    std::mt19937 rng( 99999 );
    std::uniform_int_distribution<int> coordDist( 0, 100000 );
    std::uniform_int_distribution<int> sizeDist( 100, 5000 );

    struct ITEM
    {
        int min[2];
        int max[2];
        int id;
    };

    std::vector<ITEM> items( N );

    for( int i = 0; i < N; ++i )
    {
        items[i].min[0] = coordDist( rng );
        items[i].min[1] = coordDist( rng );
        items[i].max[0] = items[i].min[0] + sizeDist( rng );
        items[i].max[1] = items[i].min[1] + sizeDist( rng );
        items[i].id = i;
    }

    // Build tree via BulkLoad
    DYNAMIC_RTREE<int, int, 2> tree;
    std::vector<DYNAMIC_RTREE<int, int, 2>::BULK_ENTRY> entries;
    entries.reserve( N );

    for( const auto& item : items )
        entries.push_back( { { item.min[0], item.min[1] }, { item.max[0], item.max[1] }, item.id } );

    tree.BulkLoad( entries );

    BOOST_CHECK_EQUAL( tree.size(), static_cast<size_t>( N ) );

    // Verify with random queries against brute force
    const int QUERIES = 200;

    for( int q = 0; q < QUERIES; ++q )
    {
        int qMin[2] = { coordDist( rng ), coordDist( rng ) };
        int qMax[2] = { qMin[0] + sizeDist( rng ) * 5, qMin[1] + sizeDist( rng ) * 5 };

        std::set<int> expected;

        for( const auto& item : items )
        {
            if( item.min[0] <= qMax[0] && item.max[0] >= qMin[0]
                && item.min[1] <= qMax[1] && item.max[1] >= qMin[1] )
            {
                expected.insert( item.id );
            }
        }

        std::set<int> actual;
        auto collect = [&actual]( int val ) { actual.insert( val ); return true; };
        tree.Search( qMin, qMax, collect );

        BOOST_CHECK_EQUAL( actual.size(), expected.size() );
        BOOST_CHECK( actual == expected );
    }

    // Verify full iteration returns all items
    std::set<int> allViaIter;

    for( int val : tree )
        allViaIter.insert( val );

    BOOST_CHECK_EQUAL( allViaIter.size(), static_cast<size_t>( N ) );
}


BOOST_AUTO_TEST_CASE( BulkLoadThenInsertAndRemove )
{
    const int N = 500;
    std::mt19937 rng( 77777 );
    std::uniform_int_distribution<int> coordDist( 0, 100000 );
    std::uniform_int_distribution<int> sizeDist( 100, 5000 );

    // Build initial tree via BulkLoad
    DYNAMIC_RTREE<int, int, 2> tree;
    std::vector<DYNAMIC_RTREE<int, int, 2>::BULK_ENTRY> entries;

    struct ITEM
    {
        int min[2];
        int max[2];
        int id;
    };

    std::vector<ITEM> liveItems;

    for( int i = 0; i < N; ++i )
    {
        ITEM item;
        item.min[0] = coordDist( rng );
        item.min[1] = coordDist( rng );
        item.max[0] = item.min[0] + sizeDist( rng );
        item.max[1] = item.min[1] + sizeDist( rng );
        item.id = i;
        entries.push_back( { { item.min[0], item.min[1] }, { item.max[0], item.max[1] }, item.id } );
        liveItems.push_back( item );
    }

    tree.BulkLoad( entries );
    BOOST_CHECK_EQUAL( tree.size(), static_cast<size_t>( N ) );

    // Now do incremental inserts
    for( int i = N; i < N + 100; ++i )
    {
        ITEM item;
        item.min[0] = coordDist( rng );
        item.min[1] = coordDist( rng );
        item.max[0] = item.min[0] + sizeDist( rng );
        item.max[1] = item.min[1] + sizeDist( rng );
        item.id = i;
        tree.Insert( item.min, item.max, item.id );
        liveItems.push_back( item );
    }

    BOOST_CHECK_EQUAL( tree.size(), static_cast<size_t>( N + 100 ) );

    // Remove some items
    for( int i = 0; i < 50; ++i )
    {
        std::uniform_int_distribution<int> idxDist( 0, liveItems.size() - 1 );
        int idx = idxDist( rng );
        ITEM& item = liveItems[idx];

        BOOST_CHECK( tree.Remove( item.min, item.max, item.id ) );
        liveItems.erase( liveItems.begin() + idx );
    }

    BOOST_CHECK_EQUAL( tree.size(), liveItems.size() );

    // Verify a query
    int qMin[2] = { 0, 0 };
    int qMax[2] = { 200000, 200000 };

    std::set<int> expected;

    for( const auto& item : liveItems )
    {
        if( item.min[0] <= qMax[0] && item.max[0] >= qMin[0]
            && item.min[1] <= qMax[1] && item.max[1] >= qMin[1] )
        {
            expected.insert( item.id );
        }
    }

    std::set<int> actual;
    auto collect = [&actual]( int val ) { actual.insert( val ); return true; };
    tree.Search( qMin, qMax, collect );

    BOOST_CHECK_EQUAL( actual.size(), expected.size() );
}


BOOST_AUTO_TEST_SUITE_END()


// --- CoW R-tree tests ---

BOOST_AUTO_TEST_SUITE( CowRTree )


BOOST_AUTO_TEST_CASE( CloneSharesData )
{
    COW_RTREE<intptr_t, int, 2> tree;

    for( int i = 0; i < 100; ++i )
    {
        int min[2] = { i * 10, i * 10 };
        int max[2] = { i * 10 + 9, i * 10 + 9 };
        tree.Insert( min, max, static_cast<intptr_t>( i ) );
    }

    BOOST_CHECK_EQUAL( tree.size(), 100 );

    // Clone should be O(1) and share data
    auto clone = tree.Clone();
    BOOST_CHECK_EQUAL( clone.size(), 100 );

    // Both should return the same search results
    int sMin[2] = { 0, 0 };
    int sMax[2] = { 50, 50 };

    std::set<intptr_t> origResults;
    std::set<intptr_t> cloneResults;

    auto collectOrig = [&origResults]( intptr_t val )
    {
        origResults.insert( val );
        return true;
    };

    auto collectClone = [&cloneResults]( intptr_t val )
    {
        cloneResults.insert( val );
        return true;
    };

    tree.Search( sMin, sMax, collectOrig );
    clone.Search( sMin, sMax, collectClone );

    BOOST_CHECK( origResults == cloneResults );
}


BOOST_AUTO_TEST_CASE( CloneMutateInsert )
{
    COW_RTREE<intptr_t, int, 2> tree;

    for( int i = 0; i < 50; ++i )
    {
        int min[2] = { i * 10, 0 };
        int max[2] = { i * 10 + 9, 9 };
        tree.Insert( min, max, static_cast<intptr_t>( i ) );
    }

    auto clone = tree.Clone();

    // Insert into clone only
    int newMin[2] = { 1000, 1000 };
    int newMax[2] = { 1010, 1010 };
    clone.Insert( newMin, newMax, 999 );

    BOOST_CHECK_EQUAL( tree.size(), 50 );
    BOOST_CHECK_EQUAL( clone.size(), 51 );

    // Verify parent doesn't see the new item
    std::vector<intptr_t> origResults;

    auto collectOrig = [&origResults]( intptr_t val )
    {
        origResults.push_back( val );
        return true;
    };

    tree.Search( newMin, newMax, collectOrig );
    BOOST_CHECK( origResults.empty() );

    // Clone should see it
    std::vector<intptr_t> cloneResults;

    auto collectClone = [&cloneResults]( intptr_t val )
    {
        cloneResults.push_back( val );
        return true;
    };

    clone.Search( newMin, newMax, collectClone );
    BOOST_CHECK_EQUAL( cloneResults.size(), 1 );
    BOOST_CHECK_EQUAL( cloneResults[0], 999 );
}


BOOST_AUTO_TEST_CASE( CloneMutateRemove )
{
    COW_RTREE<intptr_t, int, 2> tree;

    for( int i = 0; i < 50; ++i )
    {
        int min[2] = { i * 10, 0 };
        int max[2] = { i * 10 + 9, 9 };
        tree.Insert( min, max, static_cast<intptr_t>( i ) );
    }

    auto clone = tree.Clone();

    // Remove from clone only
    int rmMin[2] = { 0, 0 };
    int rmMax[2] = { 9, 9 };
    BOOST_CHECK( clone.Remove( rmMin, rmMax, 0 ) );

    BOOST_CHECK_EQUAL( tree.size(), 50 );
    BOOST_CHECK_EQUAL( clone.size(), 49 );

    // Parent should still have item 0
    std::vector<intptr_t> origResults;

    auto collectOrig = [&origResults]( intptr_t val )
    {
        origResults.push_back( val );
        return true;
    };

    tree.Search( rmMin, rmMax, collectOrig );
    BOOST_CHECK( std::find( origResults.begin(), origResults.end(), 0 )
                 != origResults.end() );
}


BOOST_AUTO_TEST_CASE( MultiLevelClone )
{
    COW_RTREE<intptr_t, int, 2> root;

    for( int i = 0; i < 30; ++i )
    {
        int min[2] = { i * 10, 0 };
        int max[2] = { i * 10 + 9, 9 };
        root.Insert( min, max, static_cast<intptr_t>( i ) );
    }

    auto child1 = root.Clone();

    int newMin1[2] = { 500, 500 };
    int newMax1[2] = { 510, 510 };
    child1.Insert( newMin1, newMax1, 100 );

    auto grandchild = child1.Clone();

    int newMin2[2] = { 600, 600 };
    int newMax2[2] = { 610, 610 };
    grandchild.Insert( newMin2, newMax2, 200 );

    BOOST_CHECK_EQUAL( root.size(), 30 );
    BOOST_CHECK_EQUAL( child1.size(), 31 );
    BOOST_CHECK_EQUAL( grandchild.size(), 32 );

    // Root should not see either new item
    std::vector<intptr_t> rootResults;

    auto collectRoot = [&rootResults]( intptr_t val )
    {
        rootResults.push_back( val );
        return true;
    };

    root.Search( newMin1, newMax1, collectRoot );
    BOOST_CHECK( rootResults.empty() );

    root.Search( newMin2, newMax2, collectRoot );
    BOOST_CHECK( rootResults.empty() );
}


BOOST_AUTO_TEST_CASE( CloneIterator )
{
    COW_RTREE<intptr_t, int, 2> tree;

    for( int i = 0; i < 50; ++i )
    {
        int min[2] = { i, i };
        int max[2] = { i + 1, i + 1 };
        tree.Insert( min, max, static_cast<intptr_t>( i ) );
    }

    auto clone = tree.Clone();

    std::set<intptr_t> cloneItems;

    for( const auto& val : clone )
        cloneItems.insert( val );

    BOOST_CHECK_EQUAL( cloneItems.size(), 50 );
}


BOOST_AUTO_TEST_CASE( MoveSemantics )
{
    COW_RTREE<intptr_t, int, 2> tree;

    for( int i = 0; i < 30; ++i )
    {
        int min[2] = { i, i };
        int max[2] = { i + 1, i + 1 };
        tree.Insert( min, max, static_cast<intptr_t>( i ) );
    }

    auto clone = tree.Clone();

    COW_RTREE<intptr_t, int, 2> moved( std::move( clone ) );
    BOOST_CHECK_EQUAL( moved.size(), 30 );
    BOOST_CHECK_EQUAL( clone.size(), 0 );

    // Original tree should be unaffected
    BOOST_CHECK_EQUAL( tree.size(), 30 );
}


BOOST_AUTO_TEST_SUITE_END()
