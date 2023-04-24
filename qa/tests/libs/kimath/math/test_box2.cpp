/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2022 KiCad Developers, see AUTHORS.TXT for contributors.
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

/**
 * Test suite for KiCad math code.
 */

#include <qa_utils/wx_utils/unit_test_utils.h>

// Code under test
#include <math/box2.h>

/**
 * Declare the test suite
 */
BOOST_AUTO_TEST_SUITE( BOX2TESTS )

BOOST_AUTO_TEST_CASE( test_closest_point_to, *boost::unit_test::tolerance( 0.000001 ) )
{
    BOX2D box( VECTOR2D( 1, 2 ), VECTOR2D( 3, 4 ) );

    // check all quadrants

    // top left
    BOOST_CHECK( box.ClosestPointTo( VECTOR2D( 0, 0 ) ) == VECTOR2D( 1, 2 ) );

    // top
    BOOST_CHECK( box.ClosestPointTo( VECTOR2D( 2, 0 ) ) == VECTOR2D( 2, 2 ) );

    // top right
    BOOST_CHECK( box.ClosestPointTo( VECTOR2D( 6, 0 ) ) == VECTOR2D( 4, 2 ) );

    // right
    BOOST_CHECK( box.ClosestPointTo( VECTOR2D( 6, 5 ) ) == VECTOR2D( 4, 5 ) );

    // bottom right
    BOOST_CHECK( box.ClosestPointTo( VECTOR2D( 6, 7 ) ) == VECTOR2D( 4, 6 ) );

    // bottom
    BOOST_CHECK( box.ClosestPointTo( VECTOR2D( 3, 7 ) ) == VECTOR2D( 3, 6 ) );

    // bottom left
    BOOST_CHECK( box.ClosestPointTo( VECTOR2D( 0, 7 ) ) == VECTOR2D( 1, 6 ) );

    // left
    BOOST_CHECK( box.ClosestPointTo( VECTOR2D( 0, 3 ) ) == VECTOR2D( 1, 3 ) );

    // inside
    BOOST_CHECK( box.ClosestPointTo( VECTOR2D( 2, 4 ) ) == VECTOR2D( 2, 4 ) );
}

BOOST_AUTO_TEST_CASE( test_farthest_point_to, *boost::unit_test::tolerance( 0.000001 ) )
{
    BOX2D box( VECTOR2D( 1, 2 ), VECTOR2D( 3, 4 ) );

    // note: the farthest point always is on a corner of the box

    // outside:

    // top left
    BOOST_CHECK( box.FarthestPointTo( VECTOR2D( 0, 0 ) ) == VECTOR2D( 4, 6 ) );

    // top right
    BOOST_CHECK( box.FarthestPointTo( VECTOR2D( 6, 0 ) ) == VECTOR2D( 1, 6 ) );

    // bottom right
    BOOST_CHECK( box.FarthestPointTo( VECTOR2D( 6, 7 ) ) == VECTOR2D( 1, 2 ) );

    // bottom left
    BOOST_CHECK( box.FarthestPointTo( VECTOR2D( 0, 7 ) ) == VECTOR2D( 4, 2 ) );

    // inside:

    // top left
    BOOST_CHECK( box.FarthestPointTo( VECTOR2D( 2, 3 ) ) == VECTOR2D( 4, 6 ) );

    // top right
    BOOST_CHECK( box.FarthestPointTo( VECTOR2D( 3, 3 ) ) == VECTOR2D( 1, 6 ) );

    // bottom right
    BOOST_CHECK( box.FarthestPointTo( VECTOR2D( 3, 5 ) ) == VECTOR2D( 1, 2 ) );

    // bottom left
    BOOST_CHECK( box.FarthestPointTo( VECTOR2D( 2, 5 ) ) == VECTOR2D( 4, 2 ) );
}

BOOST_AUTO_TEST_CASE( test_intersects_circle, *boost::unit_test::tolerance( 0.000001 ) )
{
    BOX2D box( VECTOR2D( 1, 2 ), VECTOR2D( 6, 8 ) );

    // box inside circle (touching corners)
    BOOST_CHECK( box.IntersectsCircle( VECTOR2D( 4, 6 ), 5 ) == true );

    // box completely inside circle
    BOOST_CHECK( box.IntersectsCircle( VECTOR2D( 4, 6 ), 6 ) == true );

    // circle completely inside box
    BOOST_CHECK( box.IntersectsCircle( VECTOR2D( 4, 6 ), 2 ) == true );

    // circle outside box
    BOOST_CHECK( box.IntersectsCircle( VECTOR2D( 14, 6 ), 5 ) == false );
}

BOOST_AUTO_TEST_CASE( test_intersects_circle_edge, *boost::unit_test::tolerance( 0.000001 ) )
{
    BOX2D box( VECTOR2D( 1, 2 ), VECTOR2D( 6, 8 ) );

    // box touching edge
    BOOST_CHECK( box.IntersectsCircleEdge( VECTOR2D( 4, 6 ), 5, 1 ) == true );

    // box completely inside circle
    BOOST_CHECK( box.IntersectsCircleEdge( VECTOR2D( 4, 6 ), 6, 1 ) == false );

    // circle completely inside box
    BOOST_CHECK( box.IntersectsCircleEdge( VECTOR2D( 4, 6 ), 2, 1 ) == true );

    // circle outside box
    BOOST_CHECK( box.IntersectsCircleEdge( VECTOR2D( 14, 6 ), 5, 1 ) == false );
}

BOOST_AUTO_TEST_SUITE_END()
