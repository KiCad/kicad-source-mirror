/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright The KiCad Developers, see AUTHORS.TXT for contributors.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 3
 * of the License, or (at your option) any later version.
 */

#include <geometry/shape_poly_set.h>
#include <geometry/shape_line_chain.h>
#include <geometry/polygon_triangulation.h>
#include <geometry/geometry_predicates.h>
#include <trigo.h>
#include <thread>
#include <chrono>
#include <future>
#include <filesystem>
#include <fstream>

#include <qa_utils/geometry/geometry.h>
#include <qa_utils/numeric.h>
#include <qa_utils/wx_utils/unit_test_utils.h>

#include "geom_test_utils.h"

BOOST_AUTO_TEST_SUITE( PolygonTriangulation )

struct POLYGON_TRIANGULATION_TEST_ACCESS
{
    static std::vector<double> PartitionAreaFractions( POLYGON_TRIANGULATION& aTriangulator,
                                                       const SHAPE_LINE_CHAIN& aPoly,
                                                       size_t aTargetLeaves )
    {
        return aTriangulator.PartitionAreaFractionsForTesting( aPoly, aTargetLeaves );
    }
};

namespace fs = std::filesystem;

// Helper class to properly manage TRIANGULATED_POLYGON lifecycle
class TRIANGULATION_TEST_FIXTURE
{
public:
    TRIANGULATION_TEST_FIXTURE() : m_result( std::make_unique<SHAPE_POLY_SET::TRIANGULATED_POLYGON>(0) )
    {
    }

    SHAPE_POLY_SET::TRIANGULATED_POLYGON& GetResult() { return *m_result; }

    std::unique_ptr<POLYGON_TRIANGULATION> CreateTriangulator()
    {
        return std::make_unique<POLYGON_TRIANGULATION>( *m_result );
    }

private:
    std::unique_ptr<SHAPE_POLY_SET::TRIANGULATED_POLYGON> m_result;
};

// Helper function to create a simple square
SHAPE_LINE_CHAIN createSquare( int size = 100, VECTOR2I offset = VECTOR2I(0, 0) )
{
    SHAPE_LINE_CHAIN chain;
    chain.Append( offset.x, offset.y );
    chain.Append( offset.x + size, offset.y );
    chain.Append( offset.x + size, offset.y + size );
    chain.Append( offset.x, offset.y + size );
    chain.SetClosed( true );
    return chain;
}

// Helper function to create a triangle
SHAPE_LINE_CHAIN createTriangle( int size = 100, VECTOR2I offset = VECTOR2I(0, 0) )
{
    SHAPE_LINE_CHAIN chain;
    chain.Append( offset.x, offset.y );
    chain.Append( offset.x + size, offset.y );
    chain.Append( offset.x + size/2, offset.y + size );
    chain.SetClosed( true );
    return chain;
}

// Helper function to create a complex concave polygon
SHAPE_LINE_CHAIN createConcavePolygon( int size = 100 )
{
    SHAPE_LINE_CHAIN chain;
    chain.Append( 0, 0 );
    chain.Append( size, 0 );
    chain.Append( size, size/2 );
    chain.Append( size/2, size/2 );  // Create concave section
    chain.Append( size/2, size );
    chain.Append( 0, size );
    chain.SetClosed( true );
    return chain;
}

SHAPE_LINE_CHAIN createSerpentinePolygon( int step = 20000, int teeth = 16 )
{
    SHAPE_LINE_CHAIN chain;
    chain.Append( 0, 0 );

    int x = 0;

    for( int ii = 0; ii < teeth; ++ii )
    {
        x += step;
        chain.Append( x, 0 );
        chain.Append( x, step * 3 );
        x += step;
        chain.Append( x, step * 3 );
        chain.Append( x, step * 4 );
    }

    chain.Append( 0, step * 4 );
    chain.SetClosed( true );
    return chain;
}

// Helper function to validate triangulation result with comprehensive checks
bool validateTriangulation( const SHAPE_POLY_SET::TRIANGULATED_POLYGON& result,
                           const SHAPE_LINE_CHAIN& original, bool strict = true )
{
    // Basic validation
    if( result.GetVertexCount() == 0 )
        return false;

    size_t triangleCount = result.GetTriangleCount();
    if( triangleCount == 0 )
        return false;

    // Validate triangle topology
    for( size_t i = 0; i < triangleCount; i++ )
    {
        const auto& triangle = result.Triangles()[i];

        // Check valid vertex indices
        if( triangle.a >= (int)result.GetVertexCount() ||
            triangle.b >= (int)result.GetVertexCount() ||
            triangle.c >= (int)result.GetVertexCount() )
        {
            return false;
        }

        // Triangle vertices should not be the same
        if( triangle.a == triangle.b || triangle.b == triangle.c || triangle.a == triangle.c )
            return false;

        // Check triangle area is positive (counter-clockwise orientation)
        if( strict && triangle.Area() <= 0 )
            return false;
    }

    // Validate that original vertices are preserved
    if( strict && result.GetVertexCount() >= original.PointCount() )
    {
        const auto& vertices = result.Vertices();
        for( int i = 0; i < original.PointCount(); i++ )
        {
            bool found = false;
            for( size_t j = 0; j < vertices.size(); j++ )
            {
                if( vertices[j] == original.CPoint( i ) )
                {
                    found = true;
                    break;
                }
            }
            if( !found )
                return false;
        }
    }

    return true;
}

int countSpikeyTriangles( const SHAPE_POLY_SET::TRIANGULATED_POLYGON& aResult )
{
    int count = 0;

    for( const auto& tri : aResult.Triangles() )
    {
        VECTOR2I pa = tri.GetPoint( 0 );
        VECTOR2I pb = tri.GetPoint( 1 );
        VECTOR2I pc = tri.GetPoint( 2 );

        double ab = pa.Distance( pb );
        double bc = pb.Distance( pc );
        double ca = pc.Distance( pa );

        double longest = std::max( { ab, bc, ca } );
        double shortest = std::min( { ab, bc, ca } );

        if( shortest > 0.0 && longest / shortest > 10.0 )
            ++count;
    }

    return count;
}

bool parsePolyFileForTest( const fs::path& aPath, std::vector<SHAPE_POLY_SET>& aZones )
{
    std::ifstream file( aPath );

    if( !file.is_open() )
        return false;

    std::string content( ( std::istreambuf_iterator<char>( file ) ),
                         std::istreambuf_iterator<char>() );

    size_t zonePos = 0;

    while( ( zonePos = content.find( "(zone (layer \"", zonePos ) ) != std::string::npos )
    {
        size_t polysetStart = content.find( "polyset ", zonePos );

        if( polysetStart != std::string::npos )
        {
            SHAPE_POLY_SET polySet;
            std::string    remainder = content.substr( polysetStart );
            std::stringstream ss( remainder );

            if( polySet.Parse( ss ) )
                aZones.push_back( std::move( polySet ) );
        }

        size_t layerEnd = content.find( "\")", zonePos + 14 );

        if( layerEnd == std::string::npos )
            break;

        zonePos = layerEnd + 1;
    }

    return !aZones.empty();
}

double computeBoardSpikeyRatio( const fs::path& aPath )
{
    std::vector<SHAPE_POLY_SET> zones;

    if( !parsePolyFileForTest( aPath, zones ) )
        return 1.0;

    int totalTriangles = 0;
    int totalSpikey = 0;

    for( SHAPE_POLY_SET& polySet : zones )
    {
        polySet.CacheTriangulation();

        for( unsigned int i = 0; i < polySet.TriangulatedPolyCount(); ++i )
        {
            const auto* triPoly = polySet.TriangulatedPolygon( static_cast<int>( i ) );
            totalTriangles += triPoly->GetTriangleCount();
            totalSpikey += countSpikeyTriangles( *triPoly );
        }
    }

    return totalTriangles > 0 ? static_cast<double>( totalSpikey ) / totalTriangles : 0.0;
}

// Core functionality tests
BOOST_AUTO_TEST_CASE( BasicTriangleTriangulation )
{
    TRIANGULATION_TEST_FIXTURE fixture;
    auto triangulator = fixture.CreateTriangulator();

    SHAPE_LINE_CHAIN triangle = createTriangle();

    bool success = triangulator->TesselatePolygon( triangle, nullptr );

    BOOST_TEST( success );
    BOOST_TEST( fixture.GetResult().GetVertexCount() == 3 );
    BOOST_TEST( fixture.GetResult().GetTriangleCount() == 1 );
    BOOST_TEST( validateTriangulation( fixture.GetResult(), triangle ) );
}

BOOST_AUTO_TEST_CASE( BasicSquareTriangulation )
{
    TRIANGULATION_TEST_FIXTURE fixture;
    auto triangulator = fixture.CreateTriangulator();

    SHAPE_LINE_CHAIN square = createSquare();

    bool success = triangulator->TesselatePolygon( square, nullptr );

    BOOST_TEST( success );
    BOOST_TEST( fixture.GetResult().GetVertexCount() == 4 );
    BOOST_TEST( fixture.GetResult().GetTriangleCount() == 2 );
    BOOST_TEST( validateTriangulation( fixture.GetResult(), square ) );
}

BOOST_AUTO_TEST_CASE( SplitFirstFracturePartitionProducesMultipleLeaves )
{
    TRIANGULATION_TEST_FIXTURE fixture;
    auto triangulator = fixture.CreateTriangulator();
    SHAPE_LINE_CHAIN serpentine = createSerpentinePolygon();
    std::vector<double> fractions =
            POLYGON_TRIANGULATION_TEST_ACCESS::PartitionAreaFractions( *triangulator,
                                                                       serpentine, 4 );

    BOOST_TEST( fractions.size() == 4 );

    for( double fraction : fractions )
        BOOST_TEST( fraction > 0.15 );
}

BOOST_AUTO_TEST_CASE( EarLookaheadImprovesBadTriangulationCase )
{
    fs::path polyPath = fs::path( __FILE__ ).parent_path().parent_path().parent_path().parent_path()
                        .parent_path() / "data/triangulation/bad_triangulation_case.kicad_polys";

    BOOST_TEST( fs::exists( polyPath ) );
    BOOST_TEST( computeBoardSpikeyRatio( polyPath ) < 0.47 );
}

BOOST_AUTO_TEST_CASE( ConcavePolygonTriangulation )
{
    TRIANGULATION_TEST_FIXTURE fixture;
    auto triangulator = fixture.CreateTriangulator();

    SHAPE_LINE_CHAIN concave = createConcavePolygon(100000);

    bool success = triangulator->TesselatePolygon( concave, nullptr );

    BOOST_TEST( success );

    const auto& result = fixture.GetResult();
    bool isValid = validateTriangulation( result, concave );
    size_t triangleCount = result.GetTriangleCount();

    // Print diagnostic information if validation fails or triangle count is unexpected
    if( !success || !isValid || triangleCount < 4 )
    {
        std::cout << "\n=== ConcavePolygonTriangulation Diagnostic Output ===" << std::endl;
        std::cout << "Success: " << (success ? "true" : "false") << std::endl;
        std::cout << "Validation: " << (isValid ? "true" : "false") << std::endl;
        std::cout << "Triangle count: " << triangleCount << " (expected >= 4)" << std::endl;
        std::cout << "Vertex count: " << result.GetVertexCount() << std::endl;

        // Print input polygon vertices
        std::cout << "\nInput polygon vertices (" << concave.PointCount() << " points):" << std::endl;
        for( int i = 0; i < concave.PointCount(); i++ )
        {
            VECTOR2I pt = concave.CPoint( i );
            std::cout << "  [" << i << "]: (" << pt.x << ", " << pt.y << ")" << std::endl;
        }

        // Print result vertices
        std::cout << "\nResult vertices (" << result.GetVertexCount() << " points):" << std::endl;
        const auto& vertices = result.Vertices();
        for( size_t i = 0; i < vertices.size(); i++ )
        {
            std::cout << "  [" << i << "]: (" << vertices[i].x << ", " << vertices[i].y << ")" << std::endl;
        }

        // Print triangles
        std::cout << "\nTriangles found (" << triangleCount << " triangles):" << std::endl;
        const auto& triangles = result.Triangles();
        for( size_t i = 0; i < triangles.size(); i++ )
        {
            const auto& tri = triangles[i];
            VECTOR2I va = vertices[tri.a];
            VECTOR2I vb = vertices[tri.b];
            VECTOR2I vc = vertices[tri.c];
            double area = tri.Area();

            std::cout << "  Triangle[" << i << "]: indices(" << tri.a << "," << tri.b << "," << tri.c << ")" << std::endl;
            std::cout << "    A: (" << va.x << ", " << va.y << ")" << std::endl;
            std::cout << "    B: (" << vb.x << ", " << vb.y << ")" << std::endl;
            std::cout << "    C: (" << vc.x << ", " << vc.y << ")" << std::endl;
            std::cout << "    Area: " << area << std::endl;

            // Check for degenerate triangles
            if( area <= 0 )
                std::cout << "    *** DEGENERATE TRIANGLE (area <= 0) ***" << std::endl;
            if( tri.a == tri.b || tri.b == tri.c || tri.a == tri.c )
                std::cout << "    *** INVALID TRIANGLE (duplicate vertex indices) ***" << std::endl;
        }

        // Additional diagnostic information
        if( triangleCount > 0 )
        {
            double totalArea = 0.0;
            for( const auto& tri : triangles )
                totalArea += tri.Area();
            std::cout << "\nTotal triangulated area: " << totalArea << std::endl;

            // Calculate expected area of concave polygon for comparison
            double originalArea = std::abs( concave.Area() );
            std::cout << "Original polygon area: " << originalArea << std::endl;
            std::cout << "Area difference: " << std::abs( totalArea - originalArea ) << std::endl;
        }

        std::cout << "================================================\n" << std::endl;
    }

    BOOST_TEST( success );
    BOOST_TEST( isValid );
    // L-shaped concave polygons should have 4 triangles
    BOOST_TEST( triangleCount == 4 );
}


BOOST_AUTO_TEST_CASE( HintDataOptimization )
{
    // First triangulation without hint
    TRIANGULATION_TEST_FIXTURE fixture1;
    auto triangulator1 = fixture1.CreateTriangulator();
    SHAPE_LINE_CHAIN square = createSquare();

    bool success1 = triangulator1->TesselatePolygon( square, nullptr );
    BOOST_TEST( success1 );

    // Second triangulation with hint data from first
    TRIANGULATION_TEST_FIXTURE fixture2;
    auto triangulator2 = fixture2.CreateTriangulator();

    bool success2 = triangulator2->TesselatePolygon( square, &fixture1.GetResult() );
    BOOST_TEST( success2 );

    // Results should be identical when hint is applicable
    BOOST_TEST( fixture1.GetResult().GetVertexCount() == fixture2.GetResult().GetVertexCount() );
    BOOST_TEST( fixture1.GetResult().GetTriangleCount() == fixture2.GetResult().GetTriangleCount() );
}

BOOST_AUTO_TEST_CASE( HintDataOptimizationWithSimplifiedInput )
{
    SHAPE_LINE_CHAIN noisySquare;
    noisySquare.Append( 0, 0 );
    noisySquare.Append( 100, 0 );
    noisySquare.Append( 100, 10 );
    noisySquare.Append( 100, 100 );
    noisySquare.Append( 0, 100 );
    noisySquare.SetClosed( true );

    TRIANGULATION_TEST_FIXTURE hintFixture;
    auto hintTriangulator = hintFixture.CreateTriangulator();

    bool success1 = hintTriangulator->TesselatePolygon( noisySquare, nullptr );
    BOOST_TEST( success1 );

    auto poisonedTriangles = hintFixture.GetResult().Triangles();
    BOOST_REQUIRE_GE( poisonedTriangles.size(), 2U );
    std::reverse( poisonedTriangles.begin(), poisonedTriangles.end() );
    hintFixture.GetResult().SetTriangles( poisonedTriangles );

    TRIANGULATION_TEST_FIXTURE fixture;
    auto triangulator = fixture.CreateTriangulator();

    bool success2 = triangulator->TesselatePolygon( noisySquare, &hintFixture.GetResult() );
    BOOST_TEST( success2 );
    BOOST_TEST( fixture.GetResult().GetTriangleCount() == poisonedTriangles.size() );

    for( size_t i = 0; i < poisonedTriangles.size(); ++i )
    {
        BOOST_TEST( fixture.GetResult().Triangles()[i].a == poisonedTriangles[i].a );
        BOOST_TEST( fixture.GetResult().Triangles()[i].b == poisonedTriangles[i].b );
        BOOST_TEST( fixture.GetResult().Triangles()[i].c == poisonedTriangles[i].c );
    }
}

BOOST_AUTO_TEST_CASE( HintDataInvalidation )
{
    // Create hint data with different vertex count
    TRIANGULATION_TEST_FIXTURE hintFixture;
    auto hintTriangulator = hintFixture.CreateTriangulator();
    SHAPE_LINE_CHAIN triangle = createTriangle();
    hintTriangulator->TesselatePolygon( triangle, nullptr );

    // Try to use hint with different polygon (should ignore hint)
    TRIANGULATION_TEST_FIXTURE fixture;
    auto triangulator = fixture.CreateTriangulator();
    SHAPE_LINE_CHAIN square = createSquare();

    bool success = triangulator->TesselatePolygon( square, &hintFixture.GetResult() );
    BOOST_TEST( success );
    BOOST_TEST( validateTriangulation( fixture.GetResult(), square ) );
}

// Degenerate case handling
BOOST_AUTO_TEST_CASE( DegeneratePolygons )
{
    TRIANGULATION_TEST_FIXTURE fixture;
    auto triangulator = fixture.CreateTriangulator();

    // Test empty polygon
    SHAPE_LINE_CHAIN empty;
    bool success = triangulator->TesselatePolygon( empty, nullptr );
    BOOST_TEST( success ); // Should handle gracefully

    // Test single point
    TRIANGULATION_TEST_FIXTURE fixture2;
    auto triangulator2 = fixture2.CreateTriangulator();
    SHAPE_LINE_CHAIN singlePoint;
    singlePoint.Append( 0, 0 );
    singlePoint.SetClosed( true );
    success = triangulator2->TesselatePolygon( singlePoint, nullptr );
    BOOST_TEST( success ); // Should handle gracefully

    // Test two points (line segment)
    TRIANGULATION_TEST_FIXTURE fixture3;
    auto triangulator3 = fixture3.CreateTriangulator();
    SHAPE_LINE_CHAIN line;
    line.Append( 0, 0 );
    line.Append( 100, 0 );
    line.SetClosed( true );
    success = triangulator3->TesselatePolygon( line, nullptr );
    BOOST_TEST( success ); // Should handle gracefully
}

BOOST_AUTO_TEST_CASE( ZeroAreaPolygon )
{
    TRIANGULATION_TEST_FIXTURE fixture;
    auto triangulator = fixture.CreateTriangulator();

    // Create a polygon with zero area (all points collinear)
    SHAPE_LINE_CHAIN zeroArea;
    zeroArea.Append( 0, 0 );
    zeroArea.Append( 100, 0 );
    zeroArea.Append( 50, 0 );
    zeroArea.Append( 25, 0 );
    zeroArea.SetClosed( true );

    bool success = triangulator->TesselatePolygon( zeroArea, nullptr );

    BOOST_TEST( success ); // Should handle gracefully without crashing
}

// Memory management and lifecycle tests
BOOST_AUTO_TEST_CASE( MemoryManagement )
{
    // Test that multiple triangulations properly manage memory
    for( int i = 0; i < 100; i++ )
    {
        TRIANGULATION_TEST_FIXTURE fixture;
        auto triangulator = fixture.CreateTriangulator();

        SHAPE_LINE_CHAIN poly = createSquare( 100 + i, VECTOR2I( i, i ) );
        bool success = triangulator->TesselatePolygon( poly, nullptr );

        BOOST_TEST( success );
        BOOST_TEST( validateTriangulation( fixture.GetResult(), poly, false ) );
    }
}

BOOST_AUTO_TEST_CASE( LargePolygonStressTest )
{
    TRIANGULATION_TEST_FIXTURE fixture;
    auto triangulator = fixture.CreateTriangulator();

    // Create a large polygon (regular polygon with many vertices)
    SHAPE_LINE_CHAIN largePoly;
    int numVertices = 1000;
    int radius = 10000;

    for( int i = 0; i < numVertices; i++ )
    {
        double angle = 2.0 * M_PI * i / numVertices;
        int x = static_cast<int>( radius * cos( angle ) );
        int y = static_cast<int>( radius * sin( angle ) );
        largePoly.Append( x, y );
    }
    largePoly.SetClosed( true );

    auto start = std::chrono::high_resolution_clock::now();
    bool success = triangulator->TesselatePolygon( largePoly, nullptr );
    auto end = std::chrono::high_resolution_clock::now();

    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>( end - start );

    BOOST_TEST( success );
    if( success )
    {
        BOOST_TEST( validateTriangulation( fixture.GetResult(), largePoly, false ) );
        BOOST_TEST( fixture.GetResult().GetTriangleCount() > 0 );
    }

    // Performance check - should complete in reasonable time
    BOOST_TEST( duration.count() < 10000 ); // Less than 10 seconds
}

// Thread safety tests (following SHAPE_POLY_SET patterns)
BOOST_AUTO_TEST_CASE( ConcurrentTriangulation )
{
    const int numThreads = 4;
    const int numTriangulationsPerThread = 10;

    std::vector<std::future<bool>> futures;

    for( int t = 0; t < numThreads; t++ )
    {
        futures.push_back( std::async( std::launch::async, [t, numTriangulationsPerThread]()
        {
            for( int i = 0; i < numTriangulationsPerThread; i++ )
            {
                TRIANGULATION_TEST_FIXTURE fixture;
                auto triangulator = fixture.CreateTriangulator();

                // Create unique polygon for each thread/iteration
                SHAPE_LINE_CHAIN poly = createSquare( 100 + t * 10 + i, VECTOR2I( t * 100, i * 100 ) );

                bool success = triangulator->TesselatePolygon( poly, nullptr );
                if( !success || !validateTriangulation( fixture.GetResult(), poly, false ) )
                    return false;
            }
            return true;
        }));
    }

    // Wait for all threads and check results
    for( auto& future : futures )
    {
        BOOST_TEST( future.get() );
    }
}

// Edge case and robustness tests
BOOST_AUTO_TEST_CASE( SelfIntersectingPolygon )
{
    TRIANGULATION_TEST_FIXTURE fixture;
    auto triangulator = fixture.CreateTriangulator();

    // Create a bowtie (self-intersecting polygon)
    SHAPE_LINE_CHAIN bowtie;
    bowtie.Append( 0, 0 );
    bowtie.Append( 100, 100 );
    bowtie.Append( 100, 0 );
    bowtie.Append( 0, 100 );
    bowtie.SetClosed( true );

    bool success = triangulator->TesselatePolygon( bowtie, nullptr );

    // Algorithm should handle self-intersecting polygons
    BOOST_TEST( success );
    if( success )
    {
        BOOST_TEST( validateTriangulation( fixture.GetResult(), bowtie, false ) );
    }
}


/**
 * Test case for GitLab issue #18083: Self-intersecting filled shape is not completely filled.
 *
 * A self-touching polygon where one vertex lies on a non-adjacent edge creates a "pinch point".
 * The polygon appears to form a figure-8 shape that should be fully filled on both sides.
 *
 * The polygon from the issue has points: (165,87), (179,87), (174,94), (169,87), (167,94)
 * where vertex (169,87) lies on the segment from (165,87) to (179,87).
 *
 * This test verifies that the triangulation correctly fills both regions of the self-touching
 * polygon by checking that the total triangulated area equals the sum of both triangular lobes.
 */
BOOST_AUTO_TEST_CASE( Issue18083_SelfIntersectingPolygonArea )
{
    SHAPE_POLY_SET polySet;
    SHAPE_LINE_CHAIN outline;

    // Coordinates from the issue (converted to internal units: 1mm = 1000000)
    const int SCALE = 1000000;
    outline.Append( 165 * SCALE, 87 * SCALE );
    outline.Append( 179 * SCALE, 87 * SCALE );
    outline.Append( 174 * SCALE, 94 * SCALE );
    outline.Append( 169 * SCALE, 87 * SCALE );
    outline.Append( 167 * SCALE, 94 * SCALE );
    outline.SetClosed( true );

    polySet.AddOutline( outline );

    // Verify the polygon is detected as self-intersecting
    BOOST_TEST( polySet.IsSelfIntersecting() );

    // Triangulate via SHAPE_POLY_SET
    polySet.CacheTriangulation();
    BOOST_TEST( polySet.IsTriangulationUpToDate() );

    // Calculate the triangulated area
    double triangulatedArea = 0.0;

    for( int ii = 0; ii < polySet.TriangulatedPolyCount(); ii++ )
    {
        const auto triPoly = polySet.TriangulatedPolygon( ii );

        for( const auto& tri : triPoly->Triangles() )
            triangulatedArea += std::abs( tri.Area() );
    }

    // The expected total area is 49 mm² (14 mm² + 35 mm² for the two triangular lobes)
    // Triangle 1: (165,87) - (169,87) - (167,94) = base 4mm, height 7mm = 14 mm²
    // Triangle 2: (169,87) - (179,87) - (174,94) = base 10mm, height 7mm = 35 mm²
    double expectedAreaMmSq = 49.0 * SCALE * SCALE;

    // The triangulated area should match the expected area
    BOOST_TEST( std::abs( triangulatedArea - expectedAreaMmSq ) < expectedAreaMmSq * 0.01,
                "Triangulated area should match expected area of 49 mm²" );
}

BOOST_AUTO_TEST_CASE( NearlyCollinearVertices )
{
    TRIANGULATION_TEST_FIXTURE fixture;
    auto triangulator = fixture.CreateTriangulator();

    // Create a polygon with vertices that are nearly collinear
    SHAPE_LINE_CHAIN nearlyCollinear;
    nearlyCollinear.Append( 0, 0 );
    nearlyCollinear.Append( 1000000, 0 );
    nearlyCollinear.Append( 2000000, 1 ); // Very small deviation
    nearlyCollinear.Append( 3000000, 0 );
    nearlyCollinear.Append( 1500000, 1000000 );
    nearlyCollinear.SetClosed( true );

    bool success = triangulator->TesselatePolygon( nearlyCollinear, nullptr );

    BOOST_TEST( success );
    if( success )
    {
        BOOST_TEST( validateTriangulation( fixture.GetResult(), nearlyCollinear, false ) );
    }
}

BOOST_AUTO_TEST_CASE( DuplicateVertices )
{
    TRIANGULATION_TEST_FIXTURE fixture;
    auto triangulator = fixture.CreateTriangulator();

    // Create a square with duplicate vertices
    SHAPE_LINE_CHAIN duplicate;
    duplicate.Append( 0, 0 );
    duplicate.Append( 0, 0 );  // Duplicate
    duplicate.Append( 100, 0 );
    duplicate.Append( 100, 0 ); // Duplicate
    duplicate.Append( 100, 100 );
    duplicate.Append( 100, 100 ); // Duplicate
    duplicate.Append( 0, 100 );
    duplicate.Append( 0, 100 ); // Duplicate
    duplicate.SetClosed( true );

    bool success = triangulator->TesselatePolygon( duplicate, nullptr );

    BOOST_TEST( success );
    if( success )
    {
        BOOST_TEST( validateTriangulation( fixture.GetResult(), duplicate, false ) );
    }
}

BOOST_AUTO_TEST_CASE( ExtremeCoordinates )
{
    TRIANGULATION_TEST_FIXTURE fixture;
    auto triangulator = fixture.CreateTriangulator();

    // Test with very large coordinates
    SHAPE_LINE_CHAIN extreme;
    int large = 1000000000; // 1 billion
    extreme.Append( 0, 0 );
    extreme.Append( large, 0 );
    extreme.Append( large, large );
    extreme.Append( 0, large );
    extreme.SetClosed( true );

    bool success = triangulator->TesselatePolygon( extreme, nullptr );

    BOOST_TEST( success );
    if( success )
    {
        BOOST_TEST( validateTriangulation( fixture.GetResult(), extreme, false ) );
    }
}

// Error recovery and cleanup tests
BOOST_AUTO_TEST_CASE( ErrorRecoveryAndCleanup )
{
    TRIANGULATION_TEST_FIXTURE fixture;
    auto triangulator = fixture.CreateTriangulator();

    // Try a series of operations, some of which might fail
    std::vector<SHAPE_LINE_CHAIN> testPolygons;

    // Valid polygon
    testPolygons.push_back( createSquare() );

    // Degenerate polygon
    SHAPE_LINE_CHAIN degenerate;
    degenerate.Append( 0, 0 );
    degenerate.SetClosed( true );
    testPolygons.push_back( degenerate );

    // Another valid polygon
    testPolygons.push_back( createTriangle() );

    for( const auto& poly : testPolygons )
    {
        // Each triangulation should start with a clean state
        bool success = triangulator->TesselatePolygon( poly, nullptr );
        // Even if triangulation fails, it should not crash
        success |= fixture.GetResult().GetTriangleCount() > 0;
        BOOST_TEST( success );
    }
}

// Integration tests with TRIANGULATED_POLYGON interface
BOOST_AUTO_TEST_CASE( TriangulatedPolygonInterface )
{
    TRIANGULATION_TEST_FIXTURE fixture;
    auto triangulator = fixture.CreateTriangulator();

    SHAPE_LINE_CHAIN square = createSquare();
    bool success = triangulator->TesselatePolygon( square, nullptr );

    BOOST_TEST( success );

    const auto& result = fixture.GetResult();

    // Test GetTriangle method
    if( result.GetTriangleCount() > 0 )
    {
        VECTOR2I a, b, c;
        result.GetTriangle( 0, a, b, c );

        // Vertices should be valid points from the square
        BOOST_TEST( (a.x >= 0 && a.x <= 100 && a.y >= 0 && a.y <= 100) );
        BOOST_TEST( (b.x >= 0 && b.x <= 100 && b.y >= 0 && b.y <= 100) );
        BOOST_TEST( (c.x >= 0 && c.x <= 100 && c.y >= 0 && c.y <= 100) );
    }

    // Test triangle iteration
    for( const auto& tri : result.Triangles() )
    {
        BOOST_TEST( tri.GetPointCount() == 3 );
        BOOST_TEST( tri.GetSegmentCount() == 3 );
        BOOST_TEST( tri.Area() > 0 );
        BOOST_TEST( tri.IsClosed() );
        BOOST_TEST( tri.IsSolid() );
    }
}

BOOST_AUTO_TEST_CASE( SourceOutlineIndexTracking )
{
    TRIANGULATION_TEST_FIXTURE fixture;
    auto triangulator = fixture.CreateTriangulator();

    // Test that source outline index is properly maintained
    const int expectedOutlineIndex = 5;

    // Create triangulated polygon with specific source index
    SHAPE_POLY_SET::TRIANGULATED_POLYGON result( expectedOutlineIndex );
    POLYGON_TRIANGULATION localTriangulator( result );

    SHAPE_LINE_CHAIN triangle = createTriangle();
    bool success = localTriangulator.TesselatePolygon( triangle, nullptr );

    BOOST_TEST( success );
    BOOST_TEST( result.GetSourceOutlineIndex() == expectedOutlineIndex );
}

// Performance regression tests
BOOST_AUTO_TEST_CASE( PerformanceRegression )
{
    // Test various polygon sizes to ensure performance scales reasonably
    std::vector<int> testSizes = { 10, 50, 100, 500, 1000 };
    std::vector<long long> durations;

    for( int size : testSizes )
    {
        TRIANGULATION_TEST_FIXTURE fixture;
        auto triangulator = fixture.CreateTriangulator();

        // Create regular polygon
        SHAPE_LINE_CHAIN poly;
        for( int i = 0; i < size; i++ )
        {
            double angle = 2.0 * M_PI * i / size;
            int x = static_cast<int>( 1000 * cos( angle ) );
            int y = static_cast<int>( 1000 * sin( angle ) );
            poly.Append( x, y );
        }
        poly.SetClosed( true );

        auto start = std::chrono::high_resolution_clock::now();
        bool success = triangulator->TesselatePolygon( poly, nullptr );
        auto end = std::chrono::high_resolution_clock::now();

        BOOST_TEST( success );

        auto duration = std::chrono::duration_cast<std::chrono::microseconds>( end - start );
        durations.push_back( duration.count() );
    }

    // Check that performance scales reasonably (shouldn't be exponential)
    // This is a basic sanity check - actual performance will vary by hardware
    for( size_t i = 1; i < durations.size(); i++ )
    {
        double scaleFactor = static_cast<double>( durations[i] ) / durations[i-1];
        double sizeFactor = static_cast<double>( testSizes[i] ) / testSizes[i-1];

        // Performance shouldn't be worse than O(n^2) in most cases
        BOOST_TEST( scaleFactor < sizeFactor * sizeFactor * 2 );
    }
}

/**
 * Verify that the parallel partition triangulation path produces correct results
 * by registering a task submitter that uses std::async and triangulating a polygon
 * large enough to trigger balanced partitioning (>50K vertices produces 4 leaves).
 */
BOOST_AUTO_TEST_CASE( ParallelPartitionTriangulation )
{
    SHAPE_POLY_SET polySet;
    SHAPE_LINE_CHAIN outline;
    constexpr int vertexCount = 120000;
    constexpr int centerX = 5000000;
    constexpr int centerY = 5000000;
    constexpr int radius  = 4000000;

    for( int i = 0; i < vertexCount; ++i )
    {
        double angle = 2.0 * M_PI * i / vertexCount;
        int x = centerX + static_cast<int>( radius * cos( angle ) );
        int y = centerY + static_cast<int>( radius * sin( angle ) );
        outline.Append( x, y );
    }

    outline.SetClosed( true );
    polySet.AddOutline( outline );

    std::atomic<int> tasksSubmitted( 0 );

    SHAPE_POLY_SET::TASK_SUBMITTER submitter =
            [&tasksSubmitted]( std::function<void()> aTask )
            {
                tasksSubmitted++;
                std::thread( std::move( aTask ) ).detach();
            };

    polySet.CacheTriangulation( false, submitter );

    BOOST_TEST( polySet.IsTriangulationUpToDate() );
    BOOST_TEST( tasksSubmitted.load() > 0 );

    double originalArea = std::abs( outline.Area() );
    double triArea = 0.0;

    for( unsigned int i = 0; i < polySet.TriangulatedPolyCount(); i++ )
    {
        const auto* triPoly = polySet.TriangulatedPolygon( static_cast<int>( i ) );

        for( const auto& tri : triPoly->Triangles() )
            triArea += std::abs( tri.Area() );
    }

    // Partitioning may clip edges, so allow a few percent tolerance
    if( originalArea > 0.0 )
    {
        double coverage = triArea / originalArea;
        BOOST_TEST( coverage > 0.90 );
        BOOST_TEST( coverage < 1.10 );
    }
}

/**
 * Regression test for issue #24059: infinite loop in eliminateHoles()
 * when filterPoints() removes the vertex used as its loop sentinel.
 *
 * The hole's leftmost vertex coincides with an outer ring vertex (a zero-length bridge).
 * After split(), the two bridge copies (b2 and a2)
 * are adjacent and coordinate-equal.  filterPoints(b2, a2) then tries to
 * remove a2, which is the loop sentinel. This caused an infinite loop.
 */
BOOST_AUTO_TEST_CASE( Issue24059_HoleEliminationSentinelRemoval )
{
    SHAPE_LINE_CHAIN outer;
    outer.Append( 0, 0 );
    outer.Append( 10000, 0 );
    outer.Append( 10000, 10000 );
    outer.Append( 0, 10000 );
    outer.SetClosed( true );

    SHAPE_LINE_CHAIN hole;
    hole.Append( 0, 0 );
    hole.Append( 1000, 1000 );
    hole.Append( 0, 2000 );
    hole.SetClosed( true );

    SHAPE_POLY_SET::POLYGON polygon;
    polygon.push_back( outer );
    polygon.push_back( hole );

    TRIANGULATION_TEST_FIXTURE fixture;
    auto triangulator = fixture.CreateTriangulator();

    std::atomic<bool> finished( false );
    bool result = false;

    std::thread worker( [&]()
    {
        result = triangulator->TesselatePolygon( polygon, nullptr );
        finished.store( true );
    } );

    worker.detach();

    auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds( 5 );

    while( !finished.load() && std::chrono::steady_clock::now() < deadline )
        std::this_thread::sleep_for( std::chrono::milliseconds( 50 ) );

    BOOST_CHECK_MESSAGE( finished.load(), "TesselatePolygon hung (issue #24059)" );

    if( finished.load() )
    {
        BOOST_TEST( result );
        BOOST_TEST( fixture.GetResult().GetTriangleCount() > 0 );
    }
}

/**
 * Regression test for issue #24121: loading a custom drawing sheet whose
 * outline-font glyphs contain tiny holes (small details on letters such as
 * 'B', 'P', 'O') crashed in eliminateHoles().
 *
 * Root cause: when a hole's points all collapsed below the triangulation
 * simplification distance, createRing() would self-remove the surviving
 * vertex via the duplicate-tail cleanup, leaving the caller with a vertex
 * whose next/prev pointers were null.  The hole-ring acceptance check
 * (holeRing->next != holeRing) silently passed nullptr through, and
 * eliminateHoles() then dereferenced p->next == nullptr.
 *
 * The fix tightens the cleanup in createRing() (do not self-remove a
 * single-vertex ring) and tightens the acceptance check
 * (holeRing->prev != holeRing->next) so degenerate holes never reach
 * eliminateHoles().
 */
BOOST_AUTO_TEST_CASE( Issue24121_DegenerateHoleAllDuplicates )
{
    SHAPE_LINE_CHAIN outer;
    outer.Append( 0, 0 );
    outer.Append( 10000, 0 );
    outer.Append( 10000, 10000 );
    outer.Append( 0, 10000 );
    outer.SetClosed( true );

    // Hole with all coincident points: simplification leaves one vertex, then the
    // duplicate-tail cleanup formerly self-removed it and produced a nullptr-next
    // ring that crashed eliminateHoles().
    SHAPE_LINE_CHAIN hole;
    hole.Append( 5000, 5000 );
    hole.Append( 5000, 5000 );
    hole.Append( 5000, 5000 );
    hole.Append( 5000, 5000 );
    hole.SetClosed( true );

    SHAPE_POLY_SET::POLYGON polygon;
    polygon.push_back( outer );
    polygon.push_back( hole );

    TRIANGULATION_TEST_FIXTURE fixture;
    auto triangulator = fixture.CreateTriangulator();

    bool result = triangulator->TesselatePolygon( polygon, nullptr );

    BOOST_TEST( result );
    BOOST_TEST( fixture.GetResult().GetTriangleCount() > 0 );
}

/**
 * Companion regression for #24121: hole vertices spaced below the
 * simplification level (50 internal units by default) all collapse during
 * createRing(), reproducing the same degenerate state via a more font-like
 * point cloud rather than literal duplicates.
 */
BOOST_AUTO_TEST_CASE( Issue24121_DegenerateHoleBelowSimplification )
{
    SHAPE_LINE_CHAIN outer;
    outer.Append( 0, 0 );
    outer.Append( 10000, 0 );
    outer.Append( 10000, 10000 );
    outer.Append( 0, 10000 );
    outer.SetClosed( true );

    // All four points lie within a 4-unit box, well below the default 50-unit
    // simplification threshold, so addVertex() in createRing() collapses them
    // to a single vertex.
    SHAPE_LINE_CHAIN tinyHole;
    tinyHole.Append( 5000, 5000 );
    tinyHole.Append( 5002, 5000 );
    tinyHole.Append( 5002, 5002 );
    tinyHole.Append( 5000, 5002 );
    tinyHole.SetClosed( true );

    SHAPE_POLY_SET::POLYGON polygon;
    polygon.push_back( outer );
    polygon.push_back( tinyHole );

    TRIANGULATION_TEST_FIXTURE fixture;
    auto triangulator = fixture.CreateTriangulator();

    bool result = triangulator->TesselatePolygon( polygon, nullptr );

    BOOST_TEST( result );
    BOOST_TEST( fixture.GetResult().GetTriangleCount() > 0 );
}

BOOST_AUTO_TEST_CASE( CacheTriangulation_AfterMove_FreshPoly )
{
    SHAPE_LINE_CHAIN outline;
    outline.Append( 0, 0 );
    outline.Append( 10000, 0 );
    outline.Append( 10000, 10000 );
    outline.Append( 0, 10000 );
    outline.SetClosed( true );

    SHAPE_POLY_SET polySet;
    polySet.AddOutline( outline );

    BOOST_TEST( !polySet.IsTriangulationUpToDate() );

    // Move() sets m_hashValid=true without triangulating
    polySet.Move( { 1, 1 } );
    polySet.CacheTriangulation();

    BOOST_TEST( polySet.IsTriangulationUpToDate() );
    BOOST_TEST( polySet.TriangulatedPolyCount() > 0 );
}

BOOST_AUTO_TEST_CASE( CacheTriangulation_AfterUpdateTriangulationDataHash )
{
    SHAPE_LINE_CHAIN outline;
    outline.Append( 0, 0 );
    outline.Append( 10000, 0 );
    outline.Append( 10000, 10000 );
    outline.Append( 0, 10000 );
    outline.SetClosed( true );

    SHAPE_POLY_SET polySet;
    polySet.AddOutline( outline );

    polySet.UpdateTriangulationDataHash();
    polySet.CacheTriangulation();

    BOOST_TEST( polySet.IsTriangulationUpToDate() );
    BOOST_TEST( polySet.TriangulatedPolyCount() > 0 );
}

namespace
{
double meshMinAngleDeg( const SHAPE_POLY_SET::TRIANGULATED_POLYGON& aTri )
{
    double minAngle = 180.0;

    for( const auto& tri : aTri.Triangles() )
    {
        minAngle = std::min( minAngle, GEOM_TEST::TriangleMinAngleDeg( tri.GetPoint( 0 ),
                                                                       tri.GetPoint( 1 ),
                                                                       tri.GetPoint( 2 ) ) );
    }

    return minAngle;
}


double meshArea( const SHAPE_POLY_SET::TRIANGULATED_POLYGON& aTri )
{
    double area = 0.0;

    for( const auto& tri : aTri.Triangles() )
        area += tri.Area();

    return area;
}


bool meshHasEdge( const SHAPE_POLY_SET::TRIANGULATED_POLYGON& aTri, int aU, int aV )
{
    for( const auto& tri : aTri.Triangles() )
    {
        bool hasU = tri.a == aU || tri.b == aU || tri.c == aU;
        bool hasV = tri.a == aV || tri.b == aV || tri.c == aV;

        if( hasU && hasV )
            return true;
    }

    return false;
}


int meshSpikeyCount( const SHAPE_POLY_SET::TRIANGULATED_POLYGON& aTri )
{
    int count = 0;

    for( const auto& tri : aTri.Triangles() )
    {
        VECTOR2I a = tri.GetPoint( 0 ), b = tri.GetPoint( 1 ), c = tri.GetPoint( 2 );

        if( KIGEOM::IsSliverTriangle( a, b, c ) )
            count++;
    }

    return count;
}
} // namespace


BOOST_AUTO_TEST_CASE( RefineFlipsNonDelaunayDiagonal )
{
    // Vertex 3 lies inside the circumcircle of 0-1-2, so diagonal 0-2 is non-Delaunay and must
    // flip to 1-3.
    SHAPE_POLY_SET::TRIANGULATED_POLYGON tp( 0 );
    tp.AddVertex( VECTOR2I( 0, 0 ) );
    tp.AddVertex( VECTOR2I( 10000, 0 ) );
    tp.AddVertex( VECTOR2I( 10000, 10000 ) );
    tp.AddVertex( VECTOR2I( 2000, 8000 ) );
    tp.AddTriangle( 0, 1, 2 );
    tp.AddTriangle( 0, 2, 3 );

    double beforeAngle = meshMinAngleDeg( tp );
    double beforeArea  = meshArea( tp );

    tp.Refine();

    BOOST_CHECK_EQUAL( tp.GetTriangleCount(), 2u );
    BOOST_CHECK_CLOSE( meshArea( tp ), beforeArea, 0.001 );
    BOOST_CHECK_GT( meshMinAngleDeg( tp ), beforeAngle );

    BOOST_CHECK( meshHasEdge( tp, 0, 1 ) );
    BOOST_CHECK( meshHasEdge( tp, 1, 2 ) );
    BOOST_CHECK( meshHasEdge( tp, 2, 3 ) );
    BOOST_CHECK( meshHasEdge( tp, 0, 3 ) );

    BOOST_CHECK( !meshHasEdge( tp, 0, 2 ) );
    BOOST_CHECK( meshHasEdge( tp, 1, 3 ) );
}


BOOST_AUTO_TEST_CASE( RefinePrefersFewerSliversOverDelaunay )
{
    // Diagonal 0-2 is Delaunay-legal but produces a sliver; 1-3 produces none. Refine must prefer
    // the fewer-sliver diagonal over the Delaunay one.
    SHAPE_POLY_SET::TRIANGULATED_POLYGON tp( 0 );
    tp.AddVertex( VECTOR2I( 0, 0 ) );
    tp.AddVertex( VECTOR2I( 284000, 42000 ) );
    tp.AddVertex( VECTOR2I( 276000, 68000 ) );
    tp.AddVertex( VECTOR2I( 38000, 126000 ) );
    tp.AddTriangle( 0, 1, 2 );
    tp.AddTriangle( 0, 2, 3 );

    // Precondition: 0-2 is Delaunay-legal, so a Delaunay-only refine would not touch it.
    BOOST_REQUIRE( KIGEOM::InCircleDelaunayLegal( VECTOR2I( 0, 0 ), VECTOR2I( 284000, 42000 ),
                                                  VECTOR2I( 276000, 68000 ),
                                                  VECTOR2I( 38000, 126000 ) ) );
    BOOST_REQUIRE_EQUAL( meshSpikeyCount( tp ), 1 );

    double beforeArea = meshArea( tp );

    tp.Refine();

    BOOST_CHECK_EQUAL( tp.GetTriangleCount(), 2u );
    BOOST_CHECK_CLOSE( meshArea( tp ), beforeArea, 0.001 );
    BOOST_CHECK_EQUAL( meshSpikeyCount( tp ), 0 );
    BOOST_CHECK( !meshHasEdge( tp, 0, 2 ) );
    BOOST_CHECK( meshHasEdge( tp, 1, 3 ) );
}


BOOST_AUTO_TEST_CASE( RefineLeavesDelaunayMeshUnchanged )
{
    // An already-Delaunay square triangulation must be a fixed point: no spurious flips that
    // would churn the mesh or, worse, oscillate.
    SHAPE_POLY_SET::TRIANGULATED_POLYGON tp( 0 );
    tp.AddVertex( VECTOR2I( 0, 0 ) );
    tp.AddVertex( VECTOR2I( 10000, 0 ) );
    tp.AddVertex( VECTOR2I( 10000, 10000 ) );
    tp.AddVertex( VECTOR2I( 0, 10000 ) );
    tp.AddTriangle( 0, 1, 2 );
    tp.AddTriangle( 0, 2, 3 );

    double beforeArea = meshArea( tp );

    tp.Refine();

    BOOST_CHECK_EQUAL( tp.GetTriangleCount(), 2u );
    BOOST_CHECK_CLOSE( meshArea( tp ), beforeArea, 0.001 );
    BOOST_CHECK( meshHasEdge( tp, 0, 2 ) );
}


BOOST_AUTO_TEST_CASE( PredicatesHandleFullCoordinateRange )
{
    // The 4e9 nm width exceeds a 32-bit difference; a->b->c is counter-clockwise and must read so.
    VECTOR2I a( -2000000000, 0 );
    VECTOR2I b( 2000000000, 0 );
    VECTOR2I c( 2000000000, 1000 );

    BOOST_CHECK_EQUAL( KIGEOM::OrientationSign( a, b, c ), 1 );
    BOOST_CHECK_EQUAL( KIGEOM::OrientationSign( a, c, b ), -1 );

    // The same triangle is a needle; the sliver must still register at this span.
    BOOST_CHECK( KIGEOM::IsSliverTriangle( a, b, c ) );
}


BOOST_AUTO_TEST_CASE( RefinePreservesNonManifoldEdge )
{
    // Three triangles share edge 0-1, as a hole-bridge pinch produces. That non-manifold edge must
    // act as a boundary and never flip.
    SHAPE_POLY_SET::TRIANGULATED_POLYGON tp( 0 );
    tp.AddVertex( VECTOR2I( 0, 0 ) );
    tp.AddVertex( VECTOR2I( 100000, 0 ) );
    tp.AddVertex( VECTOR2I( 50000, 30000 ) );
    tp.AddVertex( VECTOR2I( 50000, -30000 ) );
    tp.AddVertex( VECTOR2I( 50000, 60000 ) );
    tp.AddTriangle( 0, 1, 2 );
    tp.AddTriangle( 0, 1, 3 );
    tp.AddTriangle( 0, 1, 4 );

    std::vector<int> before;

    for( const auto& tri : tp.Triangles() )
    {
        before.push_back( tri.a );
        before.push_back( tri.b );
        before.push_back( tri.c );
    }

    tp.Refine();

    std::vector<int> after;

    for( const auto& tri : tp.Triangles() )
    {
        after.push_back( tri.a );
        after.push_back( tri.b );
        after.push_back( tri.c );
    }

    BOOST_CHECK( before == after );
    BOOST_CHECK( meshHasEdge( tp, 0, 1 ) );
}

BOOST_AUTO_TEST_SUITE_END()
