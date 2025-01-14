/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright The KiCad Developers, see AUTHORS.TXT for contributors.
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
#include <qa_utils/geometry/geometry.h>

// Code under test
#include <math/box2.h>


/**
 * Declare the test suite
 */
BOOST_AUTO_TEST_SUITE( BOX2TESTS )

BOOST_AUTO_TEST_CASE( DefaultConstructor )
{
    const BOX2I box;
    BOOST_TEST( box.GetPosition() == VECTOR2I( 0, 0 ) );
    BOOST_TEST( box.GetSize() == VECTOR2L( 0, 0 ) );
}

BOOST_AUTO_TEST_CASE( BasicInt )
{
    const BOX2I box( VECTOR2I( 1, 2 ), VECTOR2I( 3, 4 ) );

    BOOST_TEST( box.GetPosition() == VECTOR2I( 1, 2 ) );
    BOOST_TEST( box.GetSize() == VECTOR2L( 3, 4 ) );

    // Check the equality operator
    BOOST_TEST( box == BOX2I( VECTOR2I( 1, 2 ), VECTOR2I( 3, 4 ) ) );

    // Inflate in-place
    BOX2I inflated = box;
    inflated.Inflate( 1 );
    BOOST_TEST( inflated.GetPosition() == VECTOR2I( 0, 1 ) );
    BOOST_TEST( inflated.GetSize() == VECTOR2L( 5, 6 ) );

    // GetInflated
    const BOX2I inflated2 = box.GetInflated( 1 );
    BOOST_TEST( inflated2 == inflated );
}

BOOST_AUTO_TEST_CASE( Constexpr )
{
    constexpr BOX2I box_1_2__3_4( VECTOR2I( 1, 2 ), VECTOR2I( 3, 4 ) );
    static_assert( box_1_2__3_4.GetPosition() == VECTOR2I( 1, 2 ) );
    static_assert( box_1_2__3_4.GetSize() == VECTOR2L( 3, 4 ) );

    constexpr BOX2I box0_1__5_6 = box_1_2__3_4.GetInflated( 1 );
    static_assert( box0_1__5_6.GetPosition() == VECTOR2I( 0, 1 ) );
    static_assert( box0_1__5_6.GetSize() == VECTOR2L( 5, 6 ) );

    static_assert( box_1_2__3_4.SquaredDiagonal() < box0_1__5_6.SquaredDiagonal() );

    constexpr BOX2I box1_2__100_4 =
            box_1_2__3_4.GetWithOffset( VECTOR2I( 100, 0 ) ).Merge( box_1_2__3_4 );
    static_assert( box1_2__100_4.GetPosition() == VECTOR2I( 1, 2 ) );
    static_assert( box1_2__100_4.GetSize() == VECTOR2L( 103, 4 ) );
}

BOOST_AUTO_TEST_CASE( BasicDouble )
{
    const double tol = 0.000001;
    const BOX2D  box( VECTOR2D( 1.0, 2.0 ), VECTOR2D( 3.0, 4.0 ) );

    // Inflate by non-integer amount
    const BOX2D inflated = BOX2D( box ).Inflate( 1.5 );
    BOOST_CHECK_PREDICATE( KI_TEST::IsVecWithinTol<VECTOR2I>,
                           ( inflated.GetPosition() )( VECTOR2D( -0.5, 0.5 ) )( tol ) );
    BOOST_CHECK_PREDICATE( KI_TEST::IsVecWithinTol<VECTOR2I>,
                           ( inflated.GetSize() )( VECTOR2D( 6.0, 7.0 ) )( tol ) );
}

BOOST_AUTO_TEST_CASE( ByCorners )
{
    const BOX2I boxByCorners = BOX2I::ByCorners( VECTOR2I( 1, 2 ), VECTOR2I( 3, 4 ) );
    const BOX2I boxByPosSize = BOX2I( VECTOR2I( 1, 2 ), VECTOR2I( 2, 2 ) );

    BOOST_TEST( boxByCorners == boxByPosSize );
}

BOOST_AUTO_TEST_CASE( ByCentre )
{
    const BOX2I boxByCenter = BOX2I::ByCenter( VECTOR2I( 100, 100 ), VECTOR2I( 20, 20 ) );
    const BOX2I boxByPosSize = BOX2I( VECTOR2I( 90, 90 ), VECTOR2I( 20, 20 ) );

    BOOST_TEST( boxByCenter == boxByPosSize );
}

BOOST_AUTO_TEST_CASE( test_closest_point_to, *boost::unit_test::tolerance( 0.000001 ) )
{
    BOX2D box( VECTOR2D( 1, 2 ), VECTOR2D( 3, 4 ) );

    // check all quadrants

    // top left
    BOOST_TEST( box.NearestPoint( VECTOR2D( 0, 0 ) ) == VECTOR2D( 1, 2 ) );

    // top
    BOOST_TEST( box.NearestPoint( VECTOR2D( 2, 0 ) ) == VECTOR2D( 2, 2 ) );

    // top right
    BOOST_TEST( box.NearestPoint( VECTOR2D( 6, 0 ) ) == VECTOR2D( 4, 2 ) );

    // right
    BOOST_TEST( box.NearestPoint( VECTOR2D( 6, 5 ) ) == VECTOR2D( 4, 5 ) );

    // bottom right
    BOOST_TEST( box.NearestPoint( VECTOR2D( 6, 7 ) ) == VECTOR2D( 4, 6 ) );

    // bottom
    BOOST_TEST( box.NearestPoint( VECTOR2D( 3, 7 ) ) == VECTOR2D( 3, 6 ) );

    // bottom left
    BOOST_TEST( box.NearestPoint( VECTOR2D( 0, 7 ) ) == VECTOR2D( 1, 6 ) );

    // left
    BOOST_TEST( box.NearestPoint( VECTOR2D( 0, 3 ) ) == VECTOR2D( 1, 3 ) );

    // inside
    BOOST_TEST( box.NearestPoint( VECTOR2D( 2, 4 ) ) == VECTOR2D( 2, 4 ) );
}

BOOST_AUTO_TEST_CASE( test_farthest_point_to, *boost::unit_test::tolerance( 0.000001 ) )
{
    BOX2D box( VECTOR2D( 1, 2 ), VECTOR2D( 3, 4 ) );

    // note: the farthest point always is on a corner of the box

    // outside:

    // top left
    BOOST_TEST( box.FarthestPointTo( VECTOR2D( 0, 0 ) ) == VECTOR2D( 4, 6 ) );

    // top right
    BOOST_TEST( box.FarthestPointTo( VECTOR2D( 6, 0 ) ) == VECTOR2D( 1, 6 ) );

    // bottom right
    BOOST_TEST( box.FarthestPointTo( VECTOR2D( 6, 7 ) ) == VECTOR2D( 1, 2 ) );

    // bottom left
    BOOST_TEST( box.FarthestPointTo( VECTOR2D( 0, 7 ) ) == VECTOR2D( 4, 2 ) );

    // inside:

    // top left
    BOOST_TEST( box.FarthestPointTo( VECTOR2D( 2, 3 ) ) == VECTOR2D( 4, 6 ) );

    // top right
    BOOST_TEST( box.FarthestPointTo( VECTOR2D( 3, 3 ) ) == VECTOR2D( 1, 6 ) );

    // bottom right
    BOOST_TEST( box.FarthestPointTo( VECTOR2D( 3, 5 ) ) == VECTOR2D( 1, 2 ) );

    // bottom left
    BOOST_TEST( box.FarthestPointTo( VECTOR2D( 2, 5 ) ) == VECTOR2D( 4, 2 ) );
}

BOOST_AUTO_TEST_CASE( test_intersects_circle, *boost::unit_test::tolerance( 0.000001 ) )
{
    BOX2D box( VECTOR2D( 1, 2 ), VECTOR2D( 6, 8 ) );

    // box inside circle (touching corners)
    BOOST_TEST( box.IntersectsCircle( VECTOR2D( 4, 6 ), 5 ) == true );

    // box completely inside circle
    BOOST_TEST( box.IntersectsCircle( VECTOR2D( 4, 6 ), 6 ) == true );

    // circle completely inside box
    BOOST_TEST( box.IntersectsCircle( VECTOR2D( 4, 6 ), 2 ) == true );

    // circle outside box
    BOOST_TEST( box.IntersectsCircle( VECTOR2D( 14, 6 ), 5 ) == false );
}

BOOST_AUTO_TEST_CASE( test_intersects_circle_edge, *boost::unit_test::tolerance( 0.000001 ) )
{
    BOX2D box( VECTOR2D( 1, 2 ), VECTOR2D( 6, 8 ) );

    // box touching edge
    BOOST_TEST( box.IntersectsCircleEdge( VECTOR2D( 4, 6 ), 5, 1 ) == true );

    // box completely inside circle
    BOOST_TEST( box.IntersectsCircleEdge( VECTOR2D( 4, 6 ), 6, 1 ) == false );

    // circle completely inside box
    BOOST_TEST( box.IntersectsCircleEdge( VECTOR2D( 4, 6 ), 2, 1 ) == true );

    // circle outside box
    BOOST_TEST( box.IntersectsCircleEdge( VECTOR2D( 14, 6 ), 5, 1 ) == false );
}

BOOST_AUTO_TEST_SUITE_END()
