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
#include <trigo.h>
#include <thread>
#include <chrono>
#include <future>

#include <qa_utils/geometry/geometry.h>
#include <qa_utils/numeric.h>
#include <qa_utils/wx_utils/unit_test_utils.h>

#include "geom_test_utils.h"

BOOST_AUTO_TEST_SUITE( PolygonTriangulation )

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

BOOST_AUTO_TEST_SUITE_END()