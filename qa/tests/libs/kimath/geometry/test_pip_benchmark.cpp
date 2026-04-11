/*
 * This program is part of KiCad, a free EDA CAD application.
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
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

#include <geometry/shape_poly_set.h>
#include <geometry/shape_line_chain.h>
#include <geometry/poly_containment_index.h>
#include <core/profile.h>
#include <nlohmann/json.hpp>

#include <algorithm>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <random>
#include <sstream>
#include <string>
#include <vector>

#if __has_include( <geometry/poly_ystripes_index.h> )
#include <geometry/poly_ystripes_index.h>
#define HAS_YSTRIPES_INDEX 1
#else
#define HAS_YSTRIPES_INDEX 0
#endif

namespace fs = std::filesystem;

namespace
{

// ---------------------------------------------------------------------------
// Pluggable strategy interface
// ---------------------------------------------------------------------------

class PIP_STRATEGY
{
public:
    virtual ~PIP_STRATEGY() = default;
    virtual std::string Name() const = 0;
    virtual void Build( const SHAPE_POLY_SET& aPolySet ) = 0;
    virtual bool Contains( const VECTOR2I& aPt ) const = 0;
};


// ---------------------------------------------------------------------------
// Concrete strategies
// ---------------------------------------------------------------------------

class RAYCASTING_STRATEGY : public PIP_STRATEGY
{
public:
    std::string Name() const override { return "Raycasting"; }

    void Build( const SHAPE_POLY_SET& aPolySet ) override
    {
        m_polySet = &aPolySet;
    }

    bool Contains( const VECTOR2I& aPt ) const override
    {
        return m_polySet->Contains( aPt );
    }

private:
    const SHAPE_POLY_SET* m_polySet = nullptr;
};


class RTREE_EDGE_STRATEGY : public PIP_STRATEGY
{
public:
    std::string Name() const override { return "RTreeEdge"; }

    void Build( const SHAPE_POLY_SET& aPolySet ) override
    {
        m_index.Build( aPolySet );
    }

    bool Contains( const VECTOR2I& aPt ) const override
    {
        return m_index.Contains( aPt );
    }

private:
    POLY_CONTAINMENT_INDEX m_index;
};


#if HAS_YSTRIPES_INDEX
class YSTRIPES_STRATEGY : public PIP_STRATEGY
{
public:
    std::string Name() const override { return "YStripes"; }

    void Build( const SHAPE_POLY_SET& aPolySet ) override
    {
        m_index.Build( aPolySet );
    }

    bool Contains( const VECTOR2I& aPt ) const override
    {
        return m_index.Contains( aPt );
    }

private:
    POLY_YSTRIPES_INDEX m_index;
};
#endif


// ---------------------------------------------------------------------------
// Synthetic polygon generators
// ---------------------------------------------------------------------------

SHAPE_POLY_SET makeSquare( int aSize )
{
    SHAPE_POLY_SET poly;
    SHAPE_LINE_CHAIN outline;
    outline.Append( 0, 0 );
    outline.Append( aSize, 0 );
    outline.Append( aSize, aSize );
    outline.Append( 0, aSize );
    outline.SetClosed( true );
    poly.AddOutline( outline );
    return poly;
}


SHAPE_POLY_SET makeRegularPolygon( int aRadius, int aVertexCount )
{
    SHAPE_POLY_SET poly;
    SHAPE_LINE_CHAIN outline;

    for( int i = 0; i < aVertexCount; i++ )
    {
        double angle = 2.0 * M_PI * i / aVertexCount;
        int    x = static_cast<int>( aRadius * std::cos( angle ) );
        int    y = static_cast<int>( aRadius * std::sin( angle ) );
        outline.Append( x, y );
    }

    outline.SetClosed( true );
    poly.AddOutline( outline );
    return poly;
}


SHAPE_POLY_SET makeStarPolygon( int aOuterRadius, int aInnerRadius, int aPoints )
{
    SHAPE_POLY_SET poly;
    SHAPE_LINE_CHAIN outline;

    for( int i = 0; i < aPoints * 2; i++ )
    {
        double angle = M_PI * i / aPoints;
        int    r = ( i % 2 == 0 ) ? aOuterRadius : aInnerRadius;
        int    x = static_cast<int>( r * std::cos( angle ) );
        int    y = static_cast<int>( r * std::sin( angle ) );
        outline.Append( x, y );
    }

    outline.SetClosed( true );
    poly.AddOutline( outline );
    return poly;
}


SHAPE_POLY_SET makeSerpentine( int aStep, int aTeeth )
{
    SHAPE_POLY_SET poly;
    SHAPE_LINE_CHAIN outline;
    int              width = aStep / 2;

    // Bottom edge going right
    for( int i = 0; i < aTeeth; i++ )
    {
        outline.Append( i * aStep, 0 );
        outline.Append( i * aStep, -width );
        outline.Append( i * aStep + width, -width );
        outline.Append( i * aStep + width, 0 );
    }

    outline.Append( aTeeth * aStep, 0 );

    // Top edge going left
    outline.Append( aTeeth * aStep, aStep );

    for( int i = aTeeth - 1; i >= 0; i-- )
    {
        outline.Append( i * aStep + width, aStep );
        outline.Append( i * aStep + width, aStep + width );
        outline.Append( i * aStep, aStep + width );
        outline.Append( i * aStep, aStep );
    }

    outline.Append( 0, aStep );
    outline.SetClosed( true );
    poly.AddOutline( outline );
    return poly;
}


SHAPE_POLY_SET makeSpiral( int aRadius, int aTurns, int aPointsPerTurn, int aWidth )
{
    SHAPE_POLY_SET poly;
    SHAPE_LINE_CHAIN outline;
    int              totalPoints = aTurns * aPointsPerTurn;

    // Outer trace going outward
    for( int i = 0; i <= totalPoints; i++ )
    {
        double angle = 2.0 * M_PI * i / aPointsPerTurn;
        double r = aWidth + ( static_cast<double>( aRadius ) * i / totalPoints );
        int    x = static_cast<int>( r * std::cos( angle ) );
        int    y = static_cast<int>( r * std::sin( angle ) );
        outline.Append( x, y );
    }

    // Inner trace going back inward
    for( int i = totalPoints; i >= 0; i-- )
    {
        double angle = 2.0 * M_PI * i / aPointsPerTurn;
        double r = ( static_cast<double>( aRadius ) * i / totalPoints );
        int    x = static_cast<int>( r * std::cos( angle ) );
        int    y = static_cast<int>( r * std::sin( angle ) );
        outline.Append( x, y );
    }

    outline.SetClosed( true );
    poly.AddOutline( outline );
    return poly;
}


// ---------------------------------------------------------------------------
// Point generators
// ---------------------------------------------------------------------------

std::vector<VECTOR2I> generateRandomPoints( const BOX2I& aBBox, int aCount, uint32_t aSeed )
{
    std::mt19937          gen( aSeed );
    std::vector<VECTOR2I> points;
    points.reserve( aCount );

    std::uniform_int_distribution<int> distX( aBBox.GetLeft(), aBBox.GetRight() );
    std::uniform_int_distribution<int> distY( aBBox.GetTop(), aBBox.GetBottom() );

    for( int i = 0; i < aCount; i++ )
        points.emplace_back( distX( gen ), distY( gen ) );

    return points;
}


std::vector<VECTOR2I> generateGridPoints( const BOX2I& aBBox, int aGridSize )
{
    std::vector<VECTOR2I> points;
    int64_t               rawStepX = aBBox.GetWidth() / aGridSize;
    int64_t               rawStepY = aBBox.GetHeight() / aGridSize;
    int                   stepX = static_cast<int>( rawStepX > 0 ? rawStepX : 1 );
    int                   stepY = static_cast<int>( rawStepY > 0 ? rawStepY : 1 );

    points.reserve( static_cast<size_t>( aGridSize ) * aGridSize );

    for( int gy = 0; gy < aGridSize; gy++ )
    {
        for( int gx = 0; gx < aGridSize; gx++ )
            points.emplace_back( aBBox.GetLeft() + gx * stepX, aBBox.GetTop() + gy * stepY );
    }

    return points;
}


// ---------------------------------------------------------------------------
// Benchmark infrastructure
// ---------------------------------------------------------------------------

struct STRATEGY_RESULT
{
    std::string name;
    int64_t     buildTimeUs  = 0;
    int64_t     queryTimeUs  = 0;
    int         queryCount   = 0;
    double      nsPerQuery   = 0.0;
    double      queriesPerSec = 0.0;
    int         insideCount  = 0;
};


STRATEGY_RESULT benchmarkStrategy( PIP_STRATEGY& aStrategy, const SHAPE_POLY_SET& aPolySet,
                                   const std::vector<VECTOR2I>& aPoints )
{
    STRATEGY_RESULT result;
    result.name = aStrategy.Name();

    // Measure build time
    {
        PROF_TIMER timer;
        aStrategy.Build( aPolySet );
        timer.Stop();
        result.buildTimeUs = static_cast<int64_t>( timer.msecs() * 1000.0 );
    }

    // Warmup pass
    int warmupInside = 0;

    for( const VECTOR2I& pt : aPoints )
    {
        if( aStrategy.Contains( pt ) )
            warmupInside++;
    }

    result.insideCount = warmupInside;

    // Self-calibrate iteration count to fill at least 100ms
    int iterations = 1;
    {
        PROF_TIMER calibrate;
        int calibrateInside = 0;

        for( const VECTOR2I& pt : aPoints )
        {
            if( aStrategy.Contains( pt ) )
                calibrateInside++;
        }

        calibrate.Stop();
        double onePassMs = calibrate.msecs();

        if( onePassMs > 0.0 )
            iterations = std::max( 1, static_cast<int>( 100.0 / onePassMs ) );
    }

    // Timed benchmark loop
    {
        PROF_TIMER timer;

        for( int iter = 0; iter < iterations; iter++ )
        {
            for( const VECTOR2I& pt : aPoints )
                aStrategy.Contains( pt );
        }

        timer.Stop();
        result.queryCount = static_cast<int>( aPoints.size() ) * iterations;
        result.queryTimeUs = static_cast<int64_t>( timer.msecs() * 1000.0 );
    }

    if( result.queryCount > 0 )
    {
        result.nsPerQuery = static_cast<double>( result.queryTimeUs ) * 1000.0 / result.queryCount;
        result.queriesPerSec = result.queryCount / ( static_cast<double>( result.queryTimeUs ) / 1e6 );
    }

    return result;
}


// ---------------------------------------------------------------------------
// Real-world polygon loading
// ---------------------------------------------------------------------------

struct POLYGON_CASE
{
    std::string    source;
    std::string    layer;
    std::string    net;
    int            vertexCount = 0;
    SHAPE_POLY_SET polySet;
};


bool loadPolyFile( const fs::path& aPath, std::vector<POLYGON_CASE>& aCases )
{
    std::ifstream file( aPath );

    if( !file.is_open() )
        return false;

    std::string content( ( std::istreambuf_iterator<char>( file ) ),
                         std::istreambuf_iterator<char>() );

    std::string source;
    size_t      srcStart = content.find( "(source \"" );

    if( srcStart != std::string::npos )
    {
        srcStart += 9;
        size_t srcEnd = content.find( "\")", srcStart );

        if( srcEnd != std::string::npos )
            source = content.substr( srcStart, srcEnd - srcStart );
    }

    size_t zonePos = 0;

    while( ( zonePos = content.find( "(zone (layer \"", zonePos ) ) != std::string::npos )
    {
        POLYGON_CASE entry;
        entry.source = source;

        size_t layerStart = zonePos + 14;
        size_t layerEnd = content.find( "\")", layerStart );
        entry.layer = content.substr( layerStart, layerEnd - layerStart );

        size_t netStart = content.find( "(net \"", layerEnd );

        if( netStart != std::string::npos )
        {
            netStart += 6;
            size_t netEnd = content.find( "\")", netStart );
            entry.net = content.substr( netStart, netEnd - netStart );
        }

        size_t vcStart = content.find( "(vertex_count ", layerEnd );

        if( vcStart != std::string::npos )
        {
            vcStart += 14;
            entry.vertexCount = std::stoi( content.substr( vcStart ) );
        }

        size_t polysetStart = content.find( "polyset ", zonePos );

        if( polysetStart != std::string::npos )
        {
            std::string       remainder = content.substr( polysetStart );
            std::stringstream ss( remainder );

            if( entry.polySet.Parse( ss ) )
                aCases.push_back( std::move( entry ) );
        }

        zonePos = layerEnd + 1;
    }

    return !aCases.empty();
}


std::vector<std::unique_ptr<PIP_STRATEGY>> makeAllStrategies()
{
    std::vector<std::unique_ptr<PIP_STRATEGY>> strategies;
    strategies.push_back( std::make_unique<RAYCASTING_STRATEGY>() );
    strategies.push_back( std::make_unique<RTREE_EDGE_STRATEGY>() );
#if HAS_YSTRIPES_INDEX
    strategies.push_back( std::make_unique<YSTRIPES_STRATEGY>() );
#endif
    return strategies;
}


std::string formatTable( const std::vector<std::vector<std::string>>& aRows )
{
    if( aRows.empty() )
        return {};

    size_t              cols = aRows[0].size();
    std::vector<size_t> widths( cols, 0 );

    for( const auto& row : aRows )
    {
        for( size_t c = 0; c < std::min( cols, row.size() ); c++ )
            widths[c] = std::max( widths[c], row[c].size() );
    }

    std::ostringstream out;

    for( size_t r = 0; r < aRows.size(); r++ )
    {
        for( size_t c = 0; c < cols; c++ )
        {
            const std::string& cell = ( c < aRows[r].size() ) ? aRows[r][c] : std::string();

            if( c == 0 )
                out << std::left << std::setw( static_cast<int>( widths[c] ) ) << cell;
            else
                out << "  " << std::right << std::setw( static_cast<int>( widths[c] ) ) << cell;
        }

        out << "\n";

        if( r == 0 )
        {
            for( size_t c = 0; c < cols; c++ )
            {
                if( c > 0 )
                    out << "  ";

                out << std::string( widths[c], '-' );
            }

            out << "\n";
        }
    }

    return out.str();
}


} // anonymous namespace


BOOST_AUTO_TEST_SUITE( PIPBenchmark )


BOOST_AUTO_TEST_CASE( CorrectnessAllStrategiesAgree )
{
    SHAPE_POLY_SET star = makeStarPolygon( 1000000, 400000, 6 );
    BOX2I          bbox = star.BBox();

    std::vector<VECTOR2I> testPoints = generateRandomPoints( bbox, 10000, 42 );

    auto strategies = makeAllStrategies();

    for( auto& strategy : strategies )
        strategy->Build( star );

    int mismatches = 0;

    for( const VECTOR2I& pt : testPoints )
    {
        bool refResult = strategies[0]->Contains( pt );

        for( size_t s = 1; s < strategies.size(); s++ )
        {
            if( strategies[s]->Contains( pt ) != refResult )
            {
                mismatches++;
                BOOST_TEST_MESSAGE( strategies[s]->Name() << " disagrees at ("
                                    << pt.x << ", " << pt.y << ")"
                                    << " ref=" << refResult );
            }
        }
    }

    BOOST_CHECK_MESSAGE( mismatches == 0,
                         std::to_string( mismatches ) + " mismatch(es) across strategies" );
}


BOOST_AUTO_TEST_CASE( CorrectnessEdgeCases )
{
    SHAPE_POLY_SET square = makeSquare( 1000000 );

    auto strategies = makeAllStrategies();

    for( auto& strategy : strategies )
        strategy->Build( square );

    struct TEST_POINT
    {
        VECTOR2I    pt;
        bool        expectedInside;
        std::string desc;
    };

    std::vector<TEST_POINT> cases = {
        { { 500000, 500000 },    true,  "center" },
        { { 100000, 100000 },    true,  "inside near corner" },
        { { 900000, 900000 },    true,  "inside far corner" },
        { { -100000, 500000 },   false, "outside left" },
        { { 500000, -100000 },   false, "outside above" },
        { { 1500000, 500000 },   false, "outside right" },
        { { 500000, 1500000 },   false, "outside below" },
        { { 500000, -1000000 },  false, "outside Y range above" },
        { { 500000, 3000000 },   false, "outside Y range below" },
    };

    for( const auto& tc : cases )
    {
        for( const auto& strategy : strategies )
        {
            bool result = strategy->Contains( tc.pt );

            BOOST_CHECK_MESSAGE(
                    result == tc.expectedInside,
                    strategy->Name() + " at " + tc.desc + " (" + std::to_string( tc.pt.x ) + ", "
                            + std::to_string( tc.pt.y ) + ") expected "
                            + ( tc.expectedInside ? "inside" : "outside" ) + " got "
                            + ( result ? "inside" : "outside" ) );
        }
    }
}


BOOST_AUTO_TEST_CASE( CorrectnessPolygonWithHoles )
{
    SHAPE_POLY_SET poly;
    SHAPE_LINE_CHAIN outline;
    outline.Append( 0, 0 );
    outline.Append( 1000, 0 );
    outline.Append( 1000, 1000 );
    outline.Append( 0, 1000 );
    outline.SetClosed( true );
    poly.AddOutline( outline );

    SHAPE_LINE_CHAIN hole;
    hole.Append( 400, 400 );
    hole.Append( 600, 400 );
    hole.Append( 600, 600 );
    hole.Append( 400, 600 );
    hole.SetClosed( true );
    poly.AddHole( hole );

    POLY_YSTRIPES_INDEX ystripes;
    ystripes.Build( poly );

    BOOST_CHECK_EQUAL( ystripes.Contains( VECTOR2I( 100, 100 ) ), true );
    BOOST_CHECK_EQUAL( ystripes.Contains( VECTOR2I( 500, 500 ) ), false );
    BOOST_CHECK_EQUAL( ystripes.Contains( VECTOR2I( 1500, 500 ) ), false );
    BOOST_CHECK_EQUAL( ystripes.Contains( VECTOR2I( 800, 200 ) ), true );

    // Point just outside the hole boundary must NOT be reported as inside
    // when using accuracy-based proximity fallback (regression for isHole fix)
    BOOST_CHECK( !ystripes.Contains( VECTOR2I( 500, 410 ), 20 ) );

    BOX2I bbox = poly.BBox();
    auto  points = generateRandomPoints( bbox, 10000, 42 );
    int   mismatches = 0;

    for( const VECTOR2I& pt : points )
    {
        bool yResult = ystripes.Contains( pt );
        bool refResult = poly.Contains( pt );

        if( yResult != refResult )
            mismatches++;
    }

    BOOST_CHECK_EQUAL( mismatches, 0 );
}


BOOST_AUTO_TEST_CASE( BenchmarkSyntheticPolygons )
{
    struct POLY_CASE
    {
        std::string    name;
        SHAPE_POLY_SET poly;
    };

    std::vector<POLY_CASE> polyCases;
    polyCases.push_back( { "square_4v", makeSquare( 1000000 ) } );
    polyCases.push_back( { "hex_6v", makeRegularPolygon( 1000000, 6 ) } );
    polyCases.push_back( { "ngon_12v", makeRegularPolygon( 1000000, 12 ) } );
    polyCases.push_back( { "ngon_32v", makeRegularPolygon( 1000000, 32 ) } );
    polyCases.push_back( { "ngon_64v", makeRegularPolygon( 1000000, 64 ) } );
    polyCases.push_back( { "ngon_256v", makeRegularPolygon( 1000000, 256 ) } );
    polyCases.push_back( { "star_12v", makeStarPolygon( 1000000, 400000, 6 ) } );
    polyCases.push_back( { "star_24v", makeStarPolygon( 1000000, 400000, 12 ) } );
    polyCases.push_back( { "star_100v", makeStarPolygon( 1000000, 400000, 50 ) } );
    polyCases.push_back( { "serpentine_200v", makeSerpentine( 100000, 20 ) } );
    polyCases.push_back( { "serpentine_1000v", makeSerpentine( 100000, 100 ) } );
    polyCases.push_back( { "spiral_2000v", makeSpiral( 1000000, 10, 100, 50000 ) } );

    const int queryPointCount = 10000;
    const uint32_t seed = 12345;

    std::vector<std::vector<std::string>> table;
    table.push_back( { "Polygon", "Vertices", "Strategy", "Build(us)", "ns/query", "Mquery/s",
                        "Inside" } );

    for( auto& pc : polyCases )
    {
        BOX2I                 bbox = pc.poly.BBox();
        std::vector<VECTOR2I> points = generateRandomPoints( bbox, queryPointCount, seed );
        auto                  strategies = makeAllStrategies();

        for( auto& strategy : strategies )
        {
            STRATEGY_RESULT r = benchmarkStrategy( *strategy, pc.poly, points );

            int vertexCount = 0;

            for( int i = 0; i < pc.poly.OutlineCount(); i++ )
                vertexCount += pc.poly.COutline( i ).PointCount();

            std::ostringstream nsStr, mqStr;
            nsStr << std::fixed << std::setprecision( 1 ) << r.nsPerQuery;
            mqStr << std::fixed << std::setprecision( 3 ) << ( r.queriesPerSec / 1e6 );

            table.push_back( { pc.name, std::to_string( vertexCount ), r.name,
                               std::to_string( r.buildTimeUs ), nsStr.str(), mqStr.str(),
                               std::to_string( r.insideCount ) } );
        }
    }

    BOOST_TEST_MESSAGE( "\n=== Synthetic Polygon PIP Benchmark ===\n" << formatTable( table ) );
}


BOOST_AUTO_TEST_CASE( BenchmarkRealWorldPolygons, * boost::unit_test::disabled() )
{
    std::string dataDir = KI_TEST::GetTestDataRootDir() + "triangulation/";

    if( !fs::exists( dataDir ) || fs::is_empty( dataDir ) )
    {
        BOOST_TEST_MESSAGE( "No triangulation data in " << dataDir << ", skipping benchmark" );
        return;
    }

    std::vector<fs::path> polyFiles;

    for( const auto& entry : fs::directory_iterator( dataDir ) )
    {
        if( entry.path().extension() == ".kicad_polys" )
            polyFiles.push_back( entry.path() );
    }

    std::sort( polyFiles.begin(), polyFiles.end() );

    // Load all zones, pick the one with the most vertices per board
    struct BOARD_BEST
    {
        std::string  source;
        POLYGON_CASE bestZone;
    };

    std::vector<BOARD_BEST> boards;

    for( const auto& polyFile : polyFiles )
    {
        std::vector<POLYGON_CASE> cases;

        if( !loadPolyFile( polyFile, cases ) )
            continue;

        auto it = std::max_element( cases.begin(), cases.end(),
                                    []( const POLYGON_CASE& a, const POLYGON_CASE& b )
                                    {
                                        return a.vertexCount < b.vertexCount;
                                    } );

        if( it != cases.end() )
            boards.push_back( { it->source, std::move( *it ) } );
    }

    std::sort( boards.begin(), boards.end(),
               []( const BOARD_BEST& a, const BOARD_BEST& b )
               {
                   return a.bestZone.vertexCount < b.bestZone.vertexCount;
               } );

    const int      queryPointCount = 5000;
    const uint32_t seed = 54321;

    std::vector<std::vector<std::string>> table;
    table.push_back( { "Source", "Layer", "Vertices", "Strategy", "Build(us)", "ns/query",
                        "Mquery/s", "Inside" } );

    nlohmann::json jsonResults = nlohmann::json::array();
    int            totalMismatches = 0;

    for( auto& board : boards )
    {
        POLYGON_CASE&         zone = board.bestZone;
        BOX2I                 bbox = zone.polySet.BBox();
        std::vector<VECTOR2I> points = generateRandomPoints( bbox, queryPointCount, seed );
        auto                  strategies = makeAllStrategies();

        std::vector<STRATEGY_RESULT> results;

        for( auto& strategy : strategies )
            results.push_back( benchmarkStrategy( *strategy, zone.polySet, points ) );

        // Verify agreement
        for( size_t s = 1; s < results.size(); s++ )
        {
            if( results[s].insideCount != results[0].insideCount )
            {
                totalMismatches++;
                BOOST_TEST_MESSAGE( "Mismatch on " << zone.source << " " << zone.layer
                                    << ": " << results[0].name << "=" << results[0].insideCount
                                    << " vs " << results[s].name << "=" << results[s].insideCount );
            }
        }

        nlohmann::json boardJson;
        boardJson["source"]   = zone.source;
        boardJson["layer"]    = zone.layer;
        boardJson["vertices"] = zone.vertexCount;

        for( const auto& r : results )
        {
            std::ostringstream nsStr, mqStr;
            nsStr << std::fixed << std::setprecision( 1 ) << r.nsPerQuery;
            mqStr << std::fixed << std::setprecision( 3 ) << ( r.queriesPerSec / 1e6 );

            std::string srcName = zone.source;

            if( srcName.size() > 30 )
                srcName = srcName.substr( 0, 27 ) + "...";

            table.push_back( { srcName, zone.layer, std::to_string( zone.vertexCount ), r.name,
                               std::to_string( r.buildTimeUs ), nsStr.str(), mqStr.str(),
                               std::to_string( r.insideCount ) } );

            nlohmann::json stratJson;
            stratJson["strategy"]       = r.name;
            stratJson["build_time_us"]  = r.buildTimeUs;
            stratJson["ns_per_query"]   = r.nsPerQuery;
            stratJson["queries_per_sec"] = r.queriesPerSec;
            stratJson["inside_count"]   = r.insideCount;
            boardJson["strategies"].push_back( stratJson );
        }

        jsonResults.push_back( boardJson );
    }

    BOOST_TEST_MESSAGE( "\n=== Real-World Polygon PIP Benchmark ===\n" << formatTable( table ) );

    BOOST_CHECK_MESSAGE( totalMismatches == 0,
                         std::to_string( totalMismatches )
                                 + " board(s) with strategy disagreements" );

    // Write JSON results
    nlohmann::json output;
    output["boards"]   = jsonResults;
    output["metadata"] = { { "query_points", queryPointCount }, { "seed", seed },
                           { "board_count", static_cast<int>( boards.size() ) } };

    std::ofstream jsonFile( "pip_benchmark_results.json" );

    if( jsonFile.is_open() )
    {
        jsonFile << output.dump( 2 ) << "\n";
        jsonFile.close();
        BOOST_TEST_MESSAGE( "Wrote results to pip_benchmark_results.json" );
    }
}


BOOST_AUTO_TEST_CASE( ScalingAnalysis, * boost::unit_test::disabled() )
{
    std::vector<int> vertexCounts = { 8, 16, 32, 64, 128, 256, 512, 1024, 2048, 4096, 8192,
                                      16384, 32768, 65536 };

    const int      queryPointCount = 5000;
    const uint32_t seed = 99999;

    auto strategyNames = makeAllStrategies();
    size_t numStrategies = strategyNames.size();

    std::vector<std::string> header = { "Vertices" };

    for( const auto& s : strategyNames )
        header.push_back( s->Name() + " ns/q" );

    std::vector<std::vector<std::string>> table;
    table.push_back( header );

    for( int vc : vertexCounts )
    {
        SHAPE_POLY_SET        poly = makeRegularPolygon( 10000000, vc );
        BOX2I                 bbox = poly.BBox();
        std::vector<VECTOR2I> points = generateRandomPoints( bbox, queryPointCount, seed );

        auto strategies = makeAllStrategies();
        std::vector<std::string> row;
        row.push_back( std::to_string( vc ) );

        for( size_t s = 0; s < numStrategies; s++ )
        {
            STRATEGY_RESULT r = benchmarkStrategy( *strategies[s], poly, points );
            std::ostringstream nsStr;
            nsStr << std::fixed << std::setprecision( 1 ) << r.nsPerQuery;
            row.push_back( nsStr.str() );
        }

        table.push_back( row );
    }

    BOOST_TEST_MESSAGE( "\n=== PIP Scaling Analysis (ns/query vs vertex count) ===\n"
                        << formatTable( table ) );
}


BOOST_AUTO_TEST_SUITE_END()
