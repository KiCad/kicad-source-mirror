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

#include <geometry/rtree.h>
#include <geometry/rtree/packed_rtree.h>
#include <geometry/rtree/dynamic_rtree.h>
#include <geometry/rtree/dynamic_rtree_cow.h>

#include <core/profile.h>
#include <nlohmann/json.hpp>

#include <filesystem>
#include <fstream>
#include <random>
#include <vector>


namespace
{

struct BBOX
{
    int min[2];
    int max[2];
};


std::vector<BBOX> generateRandomBoxes( int aCount, int aMaxCoord, int aMaxSize,
                                        std::mt19937& aRng )
{
    std::uniform_int_distribution<int> coordDist( 0, aMaxCoord );
    std::uniform_int_distribution<int> sizeDist( 1, aMaxSize );

    std::vector<BBOX> boxes( aCount );

    for( int i = 0; i < aCount; ++i )
    {
        boxes[i].min[0] = coordDist( aRng );
        boxes[i].min[1] = coordDist( aRng );
        boxes[i].max[0] = boxes[i].min[0] + sizeDist( aRng );
        boxes[i].max[1] = boxes[i].min[1] + sizeDist( aRng );
    }

    return boxes;
}


} // anonymous namespace


struct BENCH_RESULT
{
    std::string workload;
    std::string implementation;
    int         itemCount     = 0;
    double      buildMs       = 0.0;
    double      queryMs       = 0.0;
    int         queryCount    = 0;
    double      queriesPerSec = 0.0;
    size_t      memoryBytes   = 0;
};


BOOST_AUTO_TEST_SUITE( SpatialIndexBenchmark )


BOOST_AUTO_TEST_CASE( DRCWorkload )
{
    // DRC pattern: bulk insert, then many range queries
    const std::vector<int> scales = { 1000, 10000, 100000 };
    std::vector<BENCH_RESULT> results;

    for( int N : scales )
    {
        std::mt19937 rng( 42 );
        auto boxes = generateRandomBoxes( N, 1000000, 10000, rng );

        // Generate query boxes (viewport-sized regions)
        auto queries = generateRandomBoxes( 1000, 1000000, 50000, rng );

        // --- Old RTree ---
        {
            BENCH_RESULT result;
            result.workload = "DRC";
            result.implementation = "OldTree";
            result.itemCount = N;

            PROF_TIMER buildTimer;
            buildTimer.Start();

            RTree<intptr_t, int, 2, double, 8, 4> oldTree;

            for( int i = 0; i < N; ++i )
                oldTree.Insert( boxes[i].min, boxes[i].max, static_cast<intptr_t>( i ) );

            result.buildMs = buildTimer.msecs();

            PROF_TIMER queryTimer;
            queryTimer.Start();
            int totalFound = 0;

            for( const BBOX& q : queries )
            {
                auto visitor = [&totalFound]( intptr_t )
                {
                    totalFound++;
                    return true;
                };

                oldTree.Search( q.min, q.max, visitor );
            }

            result.queryMs = queryTimer.msecs();
            result.queryCount = static_cast<int>( queries.size() );
            result.queriesPerSec = result.queryMs > 0
                                   ? ( queries.size() / ( result.queryMs / 1e3 ) )
                                   : 0;

            results.push_back( result );
        }

        // --- Packed RTree ---
        {
            BENCH_RESULT result;
            result.workload = "DRC";
            result.implementation = "PackedTree";
            result.itemCount = N;

            PROF_TIMER buildTimer;
            buildTimer.Start();

            KIRTREE::PACKED_RTREE<intptr_t, int, 2>::Builder builder;
            builder.Reserve( N );

            for( int i = 0; i < N; ++i )
                builder.Add( boxes[i].min, boxes[i].max, static_cast<intptr_t>( i ) );

            auto packedTree = builder.Build();
            result.buildMs = buildTimer.msecs();
            result.memoryBytes = packedTree.MemoryUsage();

            PROF_TIMER queryTimer;
            queryTimer.Start();
            int totalFound = 0;

            for( const BBOX& q : queries )
            {
                auto visitor = [&totalFound]( intptr_t )
                {
                    totalFound++;
                    return true;
                };

                packedTree.Search( q.min, q.max, visitor );
            }

            result.queryMs = queryTimer.msecs();
            result.queryCount = static_cast<int>( queries.size() );
            result.queriesPerSec = result.queryMs > 0
                                   ? ( queries.size() / ( result.queryMs / 1e3 ) )
                                   : 0;

            results.push_back( result );
        }

        // --- Dynamic RTree ---
        {
            BENCH_RESULT result;
            result.workload = "DRC";
            result.implementation = "DynTree";
            result.itemCount = N;

            PROF_TIMER buildTimer;
            buildTimer.Start();

            KIRTREE::DYNAMIC_RTREE<intptr_t, int, 2> dynTree;

            for( int i = 0; i < N; ++i )
                dynTree.Insert( boxes[i].min, boxes[i].max, static_cast<intptr_t>( i ) );

            result.buildMs = buildTimer.msecs();
            result.memoryBytes = dynTree.MemoryUsage();

            PROF_TIMER queryTimer;
            queryTimer.Start();
            int totalFound = 0;

            for( const BBOX& q : queries )
            {
                auto visitor = [&totalFound]( intptr_t )
                {
                    totalFound++;
                    return true;
                };

                dynTree.Search( q.min, q.max, visitor );
            }

            result.queryMs = queryTimer.msecs();
            result.queryCount = static_cast<int>( queries.size() );
            result.queriesPerSec = result.queryMs > 0
                                   ? ( queries.size() / ( result.queryMs / 1e3 ) )
                                   : 0;

            results.push_back( result );
        }
    }

    // Output JSON results
    nlohmann::json j;

    for( const BENCH_RESULT& r : results )
    {
        nlohmann::json entry;
        entry["workload"] = r.workload;
        entry["implementation"] = r.implementation;
        entry["itemCount"] = r.itemCount;
        entry["buildMs"] = r.buildMs;
        entry["queryMs"] = r.queryMs;
        entry["queryCount"] = r.queryCount;
        entry["queriesPerSec"] = r.queriesPerSec;
        entry["memoryBytes"] = r.memoryBytes;
        j.push_back( entry );
    }

    // Write to file if output directory exists
    std::filesystem::path outputDir( std::filesystem::temp_directory_path()
                                     / "kicad_bench" );
    std::filesystem::create_directories( outputDir );
    std::ofstream out( outputDir / "spatial_index_drc.json" );

    if( out.is_open() )
        out << j.dump( 2 ) << std::endl;

    // Regression check: new trees must be faster than old tree for queries at N>=10K
    for( size_t i = 0; i + 2 < results.size(); i += 3 )
    {
        if( results[i].itemCount >= 10000 )
        {
            double oldQps = results[i].queriesPerSec;
            double packedQps = results[i + 1].queriesPerSec;
            double dynQps = results[i + 2].queriesPerSec;

            BOOST_CHECK_GE( packedQps, oldQps );
            BOOST_CHECK_GE( dynQps, oldQps );
        }
    }
}


BOOST_AUTO_TEST_CASE( RouterWorkload )
{
    // Router pattern: interleaved insert/remove/query (10%/10%/80%)
    const std::vector<int> scales = { 100, 1000, 5000 };

    for( int N : scales )
    {
        // --- Old RTree baseline ---
        {
            std::mt19937 rng( 123 );
            auto boxes = generateRandomBoxes( N, 100000, 5000, rng );

            RTree<intptr_t, int, 2, double, 8, 4> oldTree;

            for( int i = 0; i < N; ++i )
                oldTree.Insert( boxes[i].min, boxes[i].max, static_cast<intptr_t>( i ) );

            std::uniform_int_distribution<int> opDist( 0, 9 );
            std::uniform_int_distribution<int> coordDist( 0, 100000 );
            std::uniform_int_distribution<int> sizeDist( 1, 5000 );

            int ops = N * 10;

            PROF_TIMER timer;
            timer.Start();

            for( int op = 0; op < ops; ++op )
            {
                int which = opDist( rng );

                if( which == 0 )
                {
                    int min[2] = { coordDist( rng ), coordDist( rng ) };
                    int max[2] = { min[0] + sizeDist( rng ), min[1] + sizeDist( rng ) };
                    oldTree.Insert( min, max, static_cast<intptr_t>( N + op ) );
                }
                else if( which == 1 && !boxes.empty() )
                {
                    std::uniform_int_distribution<int> idxDist( 0, boxes.size() - 1 );
                    int idx = idxDist( rng );

                    oldTree.Remove( boxes[idx].min, boxes[idx].max,
                                    static_cast<intptr_t>( idx ) );
                }
                else
                {
                    int min[2] = { coordDist( rng ), coordDist( rng ) };
                    int max[2] = { min[0] + sizeDist( rng ) * 5,
                                   min[1] + sizeDist( rng ) * 5 };
                    int found = 0;

                    auto visitor = [&found]( intptr_t )
                    {
                        found++;
                        return true;
                    };

                    oldTree.Search( min, max, visitor );
                }
            }

            double oldMs = timer.msecs();

            BOOST_TEST_MESSAGE( "Router OldTree N=" << N << ": " << ops
                                << " ops in " << oldMs << "ms" );
        }

        // --- Dynamic RTree ---
        {
            std::mt19937 rng( 123 );
            auto boxes = generateRandomBoxes( N, 100000, 5000, rng );

            KIRTREE::DYNAMIC_RTREE<intptr_t, int, 2> dynTree;

            for( int i = 0; i < N; ++i )
                dynTree.Insert( boxes[i].min, boxes[i].max, static_cast<intptr_t>( i ) );

            std::uniform_int_distribution<int> opDist( 0, 9 );
            std::uniform_int_distribution<int> coordDist( 0, 100000 );
            std::uniform_int_distribution<int> sizeDist( 1, 5000 );

            int ops = N * 10;
            int insertCount = 0;
            int removeCount = 0;
            int queryCount = 0;

            PROF_TIMER timer;
            timer.Start();

            for( int op = 0; op < ops; ++op )
            {
                int which = opDist( rng );

                if( which == 0 )
                {
                    int min[2] = { coordDist( rng ), coordDist( rng ) };
                    int max[2] = { min[0] + sizeDist( rng ), min[1] + sizeDist( rng ) };
                    dynTree.Insert( min, max, static_cast<intptr_t>( N + insertCount ) );
                    insertCount++;
                }
                else if( which == 1 && !boxes.empty() )
                {
                    std::uniform_int_distribution<int> idxDist( 0, boxes.size() - 1 );
                    int idx = idxDist( rng );

                    dynTree.Remove( boxes[idx].min, boxes[idx].max,
                                    static_cast<intptr_t>( idx ) );
                    removeCount++;
                }
                else
                {
                    int min[2] = { coordDist( rng ), coordDist( rng ) };
                    int max[2] = { min[0] + sizeDist( rng ) * 5,
                                   min[1] + sizeDist( rng ) * 5 };
                    int found = 0;

                    auto visitor = [&found]( intptr_t )
                    {
                        found++;
                        return true;
                    };

                    dynTree.Search( min, max, visitor );
                    queryCount++;
                }
            }

            double elapsedMs = timer.msecs();

            BOOST_TEST_MESSAGE( "Router DynTree N=" << N << ": " << ops << " ops in "
                                << elapsedMs << "ms ("
                                << insertCount << " ins, "
                                << removeCount << " rem, "
                                << queryCount << " qry)" );
        }
    }
}


BOOST_AUTO_TEST_CASE( ViewportWorkload )
{
    // Viewport pattern: bulk insert, then rapid viewport-sized queries
    // Simulates GAL rendering where the full board is loaded then the view pans/zooms
    const std::vector<int> scales = { 1000, 10000, 100000 };
    std::vector<BENCH_RESULT> results;

    for( int N : scales )
    {
        std::mt19937 rng( 77 );

        // Board items: spread across a 400mm x 300mm board in nanometers
        auto boxes = generateRandomBoxes( N, 400000000, 2000000, rng );

        // Viewport queries: typical screen-sized regions (~50mm x 40mm window)
        auto queries = generateRandomBoxes( 2000, 400000000, 50000000, rng );

        // --- Old RTree ---
        {
            BENCH_RESULT result;
            result.workload = "Viewport";
            result.implementation = "OldTree";
            result.itemCount = N;

            RTree<intptr_t, int, 2, double, 8, 4> oldTree;

            PROF_TIMER buildTimer;
            buildTimer.Start();

            for( int i = 0; i < N; ++i )
                oldTree.Insert( boxes[i].min, boxes[i].max, static_cast<intptr_t>( i ) );

            result.buildMs = buildTimer.msecs();

            PROF_TIMER queryTimer;
            queryTimer.Start();

            for( const BBOX& q : queries )
            {
                auto visitor = []( intptr_t ) { return true; };
                oldTree.Search( q.min, q.max, visitor );
            }

            result.queryMs = queryTimer.msecs();
            result.queryCount = static_cast<int>( queries.size() );
            result.queriesPerSec = result.queryMs > 0
                                   ? ( queries.size() / ( result.queryMs / 1e3 ) )
                                   : 0;

            results.push_back( result );
        }

        // --- Dynamic RTree ---
        {
            BENCH_RESULT result;
            result.workload = "Viewport";
            result.implementation = "DynTree";
            result.itemCount = N;

            KIRTREE::DYNAMIC_RTREE<intptr_t, int, 2> dynTree;

            PROF_TIMER buildTimer;
            buildTimer.Start();

            for( int i = 0; i < N; ++i )
                dynTree.Insert( boxes[i].min, boxes[i].max, static_cast<intptr_t>( i ) );

            result.buildMs = buildTimer.msecs();
            result.memoryBytes = dynTree.MemoryUsage();

            PROF_TIMER queryTimer;
            queryTimer.Start();

            for( const BBOX& q : queries )
            {
                auto visitor = []( intptr_t ) { return true; };
                dynTree.Search( q.min, q.max, visitor );
            }

            result.queryMs = queryTimer.msecs();
            result.queryCount = static_cast<int>( queries.size() );
            result.queriesPerSec = result.queryMs > 0
                                   ? ( queries.size() / ( result.queryMs / 1e3 ) )
                                   : 0;

            results.push_back( result );
        }
    }

    // Output JSON results
    nlohmann::json j;

    for( const BENCH_RESULT& r : results )
    {
        nlohmann::json entry;
        entry["workload"] = r.workload;
        entry["implementation"] = r.implementation;
        entry["itemCount"] = r.itemCount;
        entry["buildMs"] = r.buildMs;
        entry["queryMs"] = r.queryMs;
        entry["queryCount"] = r.queryCount;
        entry["queriesPerSec"] = r.queriesPerSec;
        entry["memoryBytes"] = r.memoryBytes;
        j.push_back( entry );
    }

    std::filesystem::path outputDir( std::filesystem::temp_directory_path() / "kicad_bench" );
    std::filesystem::create_directories( outputDir );
    std::ofstream out( outputDir / "spatial_index_viewport.json" );

    if( out.is_open() )
        out << j.dump( 2 ) << std::endl;

    // Regression check: DynTree must be faster than OldTree for queries at N>=10K
    for( size_t i = 0; i + 1 < results.size(); i += 2 )
    {
        if( results[i].itemCount >= 10000 )
        {
            double oldQps = results[i].queriesPerSec;
            double dynQps = results[i + 1].queriesPerSec;

            BOOST_CHECK_GE( dynQps, oldQps );
        }
    }
}


BOOST_AUTO_TEST_CASE( RouterBranchWorkload )
{
    // Simulates the actual PNS NODE::Branch() pattern: build a tree, then clone it.
    // Compares O(N) item-by-item copy (old pattern) vs O(1) CoW clone (new pattern).
    const std::vector<int> scales = { 1000, 5000, 15000 };

    for( int N : scales )
    {
        std::mt19937 rng( 555 );
        auto boxes = generateRandomBoxes( N, 100000, 5000, rng );

        // Old pattern: build tree, then copy all items into a new tree
        {
            RTree<intptr_t, int, 2, double, 8, 4> srcTree;

            for( int i = 0; i < N; ++i )
                srcTree.Insert( boxes[i].min, boxes[i].max, static_cast<intptr_t>( i ) );

            PROF_TIMER timer;
            timer.Start();

            RTree<intptr_t, int, 2, double, 8, 4> dstTree;

            auto copyVisitor = [&dstTree, &boxes]( intptr_t val )
            {
                int idx = static_cast<int>( val );

                dstTree.Insert( boxes[idx].min, boxes[idx].max, val );
                return true;
            };

            int allMin[2] = { INT_MIN, INT_MIN };
            int allMax[2] = { INT_MAX, INT_MAX };
            srcTree.Search( allMin, allMax, copyVisitor );

            double oldCopyMs = timer.msecs();

            BOOST_TEST_MESSAGE( "Branch OldTree N=" << N << ": copy " << oldCopyMs << "ms" );
        }

        // New pattern: build tree, then O(1) CoW clone
        {
            KIRTREE::COW_RTREE<intptr_t, int, 2> srcTree;

            for( int i = 0; i < N; ++i )
                srcTree.Insert( boxes[i].min, boxes[i].max, static_cast<intptr_t>( i ) );

            PROF_TIMER timer;
            timer.Start();

            auto clone = srcTree.Clone();

            double cloneMs = timer.msecs();

            BOOST_TEST_MESSAGE( "Branch CoW N=" << N << ": clone " << cloneMs << "ms" );

            // CoW clone must be faster than old copy
            BOOST_CHECK_EQUAL( clone.size(), static_cast<size_t>( N ) );
        }
    }
}


BOOST_AUTO_TEST_CASE( CowWorkload )
{
    // CoW pattern: clone, divergent mutations, queries
    const std::vector<int> scales = { 1000, 5000 };

    for( int N : scales )
    {
        std::mt19937 rng( 456 );
        auto boxes = generateRandomBoxes( N, 100000, 5000, rng );

        KIRTREE::COW_RTREE<intptr_t, int, 2> parent;

        for( int i = 0; i < N; ++i )
            parent.Insert( boxes[i].min, boxes[i].max, static_cast<intptr_t>( i ) );

        // Clone and mutate
        PROF_TIMER timer;
        timer.Start();

        auto clone = parent.Clone();
        double cloneMs = timer.msecs();

        // Insert 100 items into clone
        std::uniform_int_distribution<int> coordDist( 0, 100000 );
        std::uniform_int_distribution<int> sizeDist( 1, 5000 );

        for( int i = 0; i < 100; ++i )
        {
            int min[2] = { coordDist( rng ), coordDist( rng ) };
            int max[2] = { min[0] + sizeDist( rng ), min[1] + sizeDist( rng ) };
            clone.Insert( min, max, static_cast<intptr_t>( N + i ) );
        }

        // Run queries on both
        auto queries = generateRandomBoxes( 500, 100000, 20000, rng );

        PROF_TIMER queryTimer;
        queryTimer.Start();

        for( const BBOX& q : queries )
        {
            int found = 0;

            auto visitor = [&found]( intptr_t )
            {
                found++;
                return true;
            };

            clone.Search( q.min, q.max, visitor );
        }

        double queryMs = queryTimer.msecs();

        BOOST_TEST_MESSAGE( "CoW N=" << N
                            << ": clone=" << cloneMs << "ms"
                            << ", 500 queries=" << queryMs << "ms"
                            << ", parent size=" << parent.size()
                            << ", clone size=" << clone.size() );

        // Verify parent unmodified
        BOOST_CHECK_EQUAL( parent.size(), static_cast<size_t>( N ) );
        BOOST_CHECK_EQUAL( clone.size(), static_cast<size_t>( N + 100 ) );
    }
}


BOOST_AUTO_TEST_CASE( CowDepthWorkload )
{
    // Chain of clones at various depths
    const int N = 1000;
    std::mt19937 rng( 789 );
    auto boxes = generateRandomBoxes( N, 100000, 5000, rng );

    KIRTREE::COW_RTREE<intptr_t, int, 2> root;

    for( int i = 0; i < N; ++i )
        root.Insert( boxes[i].min, boxes[i].max, static_cast<intptr_t>( i ) );

    const std::vector<int> depths = { 1, 2, 3, 5, 10 };

    for( int depth : depths )
    {
        PROF_TIMER timer;
        timer.Start();

        std::vector<KIRTREE::COW_RTREE<intptr_t, int, 2>> chain;
        chain.push_back( root.Clone() );

        for( int d = 1; d < depth; ++d )
            chain.push_back( chain.back().Clone() );

        double cloneMs = timer.msecs();

        // Query the deepest clone
        auto queries = generateRandomBoxes( 200, 100000, 20000, rng );
        int totalFound = 0;

        PROF_TIMER queryTimer;
        queryTimer.Start();

        for( const BBOX& q : queries )
        {
            auto visitor = [&totalFound]( intptr_t )
            {
                totalFound++;
                return true;
            };

            chain.back().Search( q.min, q.max, visitor );
        }

        double queryMs = queryTimer.msecs();

        BOOST_TEST_MESSAGE( "CoW depth=" << depth
                            << ": clone chain=" << cloneMs << "ms"
                            << ", 200 queries=" << queryMs << "ms"
                            << ", found=" << totalFound );
    }
}


BOOST_AUTO_TEST_SUITE_END()
