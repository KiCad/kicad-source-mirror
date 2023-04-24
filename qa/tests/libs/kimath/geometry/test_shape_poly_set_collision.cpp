/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2017 CERN
 * @author Alejandro García Montoro <alejandro.garciamontoro@gmail.com>
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

#include <tuple>
#include <qa_utils/wx_utils/unit_test_utils.h>

#include <geometry/shape_line_chain.h>
#include <geometry/shape_poly_set.h>

#include "fixtures_geometry.h"

/**
 * Fixture for the Collision test suite. It contains an instance of the common data and two
 * vectors containing colliding and non-colliding points.
 */
struct CollisionFixture
{
    // Structure to store the common data.
    struct KI_TEST::CommonTestData common;

    // Vectors containing colliding and non-colliding points
    std::vector<VECTOR2I> collidingPoints, nonCollidingPoints;

    // tuple of segment under test, collision result, and intersection point
    typedef std::tuple<SEG, bool, VECTOR2I> SEG_CASE;

    std::vector<SEG_CASE> segs;

    /**
    * Constructor
    */
    CollisionFixture()
    {
        // Create points colliding with the poly set.

        // Inside the polygon
        collidingPoints.emplace_back( 10, 90 );

        // Inside the polygon, but on a re-entrant angle of a hole
        collidingPoints.emplace_back( 15, 16 );

        // On a hole edge => inside the polygon
        collidingPoints.emplace_back( 40, 25 );

        // On the outline edge => inside the polygon
        collidingPoints.emplace_back( 0, 10 );

        // Create points not colliding with the poly set.

        // Completely outside of the polygon
        nonCollidingPoints.emplace_back( 200, 200 );

        // Inside the outline and inside a hole => outside the polygon
        nonCollidingPoints.emplace_back( 15, 12 );

        // Seg crossing the edge
        segs.emplace_back( std::make_tuple( SEG( VECTOR2I( 90, 90 ), VECTOR2I( 110, 110 ) ),
                                            true, VECTOR2I( 100, 100 ) ) );
        segs.emplace_back( std::make_tuple( SEG( VECTOR2I( 110, 110 ), VECTOR2I( 90, 90 ) ),
                                            true, VECTOR2I( 100, 100 ) ) );
        segs.emplace_back( std::make_tuple( SEG( VECTOR2I( 50, -10 ), VECTOR2I( 50, 50 ) ),
                                            true, VECTOR2I( 50, 0 ) ) );
        segs.emplace_back( std::make_tuple( SEG( VECTOR2I( 50, 50 ), VECTOR2I( 50, -10 ) ),
                                            true, VECTOR2I( 50, 0 ) ) );

        // Seg fully inside
        segs.emplace_back( std::make_tuple( SEG( VECTOR2I( 80, 80 ), VECTOR2I( 90, 90 ) ),
                                            true, VECTOR2I( 85, 85 ) ) );
        segs.emplace_back( std::make_tuple( SEG( VECTOR2I( 90, 90 ), VECTOR2I( 80, 80 ) ),
                                            true, VECTOR2I( 85, 85 ) ) );

        // Seg fully outside
        segs.emplace_back( std::make_tuple( SEG( VECTOR2I( 110, 110 ), VECTOR2I( 120, 120 ) ),
                                            false, VECTOR2I() ) );

        // Seg touching
        segs.emplace_back( std::make_tuple( SEG( VECTOR2I( 100, 100 ), VECTOR2I( 110, 110 ) ),
                                            true, VECTOR2I( 100, 100 ) ) );
        segs.emplace_back( std::make_tuple( SEG( VECTOR2I( 110, 110 ), VECTOR2I( 100, 100 ) ),
                                            true, VECTOR2I( 100, 100 ) ) );
    }

    ~CollisionFixture()
    {
    }
};

/**
 * Declares the CollisionFixture as the boost test suite fixture.
 */
BOOST_FIXTURE_TEST_SUITE( SPSCollision, CollisionFixture )

/**
 * Simple dummy test to check that HasHoles() definition is right
 */
BOOST_AUTO_TEST_CASE( HasHoles )
{
    BOOST_CHECK( !common.solidPolySet.HasHoles() );
    BOOST_CHECK( common.holeyPolySet.HasHoles() );
}

/**
 * This test checks basic behaviour of PointOnEdge, testing if points on corners, outline edges
 * and hole edges are detected as colliding.
 */
BOOST_AUTO_TEST_CASE( PointOnEdge )
{
    // Check points on corners
    BOOST_CHECK( common.holeyPolySet.PointOnEdge( VECTOR2I( 0, 50 ) ) );

    // Check points on outline edges
    BOOST_CHECK( common.holeyPolySet.PointOnEdge( VECTOR2I( 0, 10 ) ) );

    // Check points on hole edges
    BOOST_CHECK( common.holeyPolySet.PointOnEdge( VECTOR2I( 10, 11 ) ) );

    // Check points inside a hole -> not in edge
    BOOST_CHECK( !common.holeyPolySet.PointOnEdge( VECTOR2I( 12, 12 ) ) );

    // Check points inside the polygon and outside any hole -> not on edge
    BOOST_CHECK( !common.holeyPolySet.PointOnEdge( VECTOR2I( 90, 90 ) ) );

    // Check points outside the polygon -> not on edge
    BOOST_CHECK( !common.holeyPolySet.PointOnEdge( VECTOR2I( 200, 200 ) ) );
}

/**
 * This test checks that the function Contains, whose behaviour has been updated to also manage
 * holey polygons, does the right work.
 */
BOOST_AUTO_TEST_CASE( pointInPolygonSet )
{
    // Check that the set contains the points that collide with it
    for( const VECTOR2I& point : collidingPoints )
    {
        std::stringstream ss;
        ss << "Point {" << point.x << ", " << point.y << " }";
        BOOST_TEST_INFO( ss.str() );

        BOOST_CHECK( common.holeyPolySet.Contains( point ) );
    }

    // Check that the set does not contain any point outside of it
    for( const VECTOR2I& point : nonCollidingPoints )
    {
        std::stringstream ss;
        ss << "Point {" << point.x << ", " << point.y << " }";
        BOOST_TEST_INFO( ss.str() );

        BOOST_CHECK( !common.holeyPolySet.Contains( point ) );
    }
}

/**
 * This test checks the behaviour of the Collide (with a point) method.
 */
BOOST_AUTO_TEST_CASE( Collide )
{
    // When clearance = 0, the behaviour should be the same as with Contains

    // Check that the set collides with the colliding points
    for( const VECTOR2I& point : collidingPoints )
    {
        std::stringstream ss;
        ss << "Point {" << point.x << ", " << point.y << " }";
        BOOST_TEST_INFO( ss.str() );

        BOOST_CHECK( common.holeyPolySet.Collide( point, 0 ) );
    }

    // Check that the set does not collide with the non colliding points
    for( const VECTOR2I& point : nonCollidingPoints )
    {
        std::stringstream ss;
        ss << "Point {" << point.x << ", " << point.y << " }";
        BOOST_TEST_INFO( ss.str() );

        BOOST_CHECK( !common.holeyPolySet.Collide( point, 0 ) );
    }

    // Checks with clearance > 0

    // Point at the offset zone outside of the outline => collision!
    BOOST_CHECK( common.holeyPolySet.Collide( VECTOR2I( -1, 10 ), 5 ) );

    // Point at the offset zone outside of a hole => collision!
    BOOST_CHECK( common.holeyPolySet.Collide( VECTOR2I( 11, 11 ), 5 ) );
}

/**
 * This test checks the behaviour of the CollideVertex method, testing whether the collision with
 * vertices is well detected
 */
BOOST_AUTO_TEST_CASE( CollideVertex )
{
    // Check that the set collides with the colliding points
    for( const VECTOR2I& point : common.holeyPoints )
    {
        BOOST_CHECK_MESSAGE( common.holeyPolySet.CollideVertex( point, nullptr, 0 ),
                             " Point " << point.x << ", " << point.y <<
                             " does not collide with holeyPolySet polygon" );
    }
}

/**
 * This test checks the behaviour of the CollideVertex method, testing whether the collision with
 * vertices is well detected
 */
BOOST_AUTO_TEST_CASE( CollideVertexWithClearance )
{
    // Check that the set collides with the colliding points
    for( const VECTOR2I& point : common.holeyPoints )
        BOOST_CHECK( common.holeyPolySet.CollideVertex( point + VECTOR2I( 1, 1 ), nullptr, 2 ) );
}


/**
 * Check that SHAPE_POLY_SET::Collide does the right thing for segments
 */
BOOST_AUTO_TEST_CASE( CollideSegments )
{
    for( const SEG_CASE& testCase : segs )
    {
        SEG seg;
        bool expectedResult;
        VECTOR2I expectedLocation;

        std::tie( seg, expectedResult, expectedLocation ) = testCase;

        VECTOR2I location;

        BOOST_CHECK( common.holeyPolySet.Collide( seg, 0, nullptr, &location ) == expectedResult );

        if( expectedResult )
            BOOST_REQUIRE_EQUAL( location, expectedLocation );
    }
}

BOOST_AUTO_TEST_SUITE_END()
