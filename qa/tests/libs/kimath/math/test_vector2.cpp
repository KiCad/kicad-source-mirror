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

// Code under test
#include <math/vector2d.h>

/**
 * Declare the test suite
 */
BOOST_AUTO_TEST_SUITE( VECTOR2TESTS )

BOOST_AUTO_TEST_CASE( Constexpr )
{
    constexpr VECTOR2I vi_1_2{ 1, 2 };
    // Something to test at runtime
    BOOST_TEST( vi_1_2 == VECTOR2I( 1, 2 ) );
    static_assert( vi_1_2.x == 1 );
    static_assert( vi_1_2.y == 2 );

    constexpr VECTOR2I vi_3_5 = vi_1_2 + VECTOR2I( 3, 4 ) - VECTOR2I( 1, 1 );

    static_assert( vi_3_5 == VECTOR2I( 3, 5 ) );

    static_assert( vi_3_5.SquaredEuclideanNorm() == 9 + 25 );
    static_assert( vi_3_5 > vi_1_2 );
    static_assert( vi_1_2 < vi_3_5 );
    static_assert( vi_1_2 != vi_3_5 );
    static_assert( vi_1_2 <= vi_1_2 );
    static_assert( vi_1_2 >= vi_1_2 );

    static_assert( vi_1_2.SquaredDistance( vi_3_5 ) == 4 + 9 );

    static_assert( LexicographicalCompare( vi_1_2, vi_3_5 ) < 0 );
    // Was the copy avoided?
    static_assert( &LexicographicalMin( vi_1_2, vi_3_5 ) == &vi_1_2 );
    static_assert( &LexicographicalMin( vi_1_2, vi_3_5 ) == &vi_1_2 );

    constexpr VECTOR2D vd_1_2{ 1.0, 2.0 };
    constexpr VECTOR2D vd_3_4{ 3.0, 4.0 };

    static_assert( vd_1_2 == VECTOR2D( 1.0, 2.0 ) );
    static_assert( vd_1_2 != vd_3_4 );
    static_assert( vd_1_2 < vd_3_4 );
    static_assert( vd_1_2 <= vd_3_4 );
    static_assert( vd_3_4 > vd_1_2 );

    // After C++23
    //static_assert( equals( vd_1_2, vd_3_4, 2.0 ) );
}

BOOST_AUTO_TEST_CASE( test_cross_product, *boost::unit_test::tolerance( 0.000001 ) )
{
    VECTOR2I v1(0, 1);
    VECTOR2I v2(1, 0);

    BOOST_TEST( v2.Cross( v1 ) == 1 );
}

BOOST_AUTO_TEST_CASE( test_dot_product, *boost::unit_test::tolerance( 0.000001 ) )
{
    VECTOR2I v1( 0, 1 );
    VECTOR2I v2( 1, 0 );

    BOOST_TEST( v2.Dot( v1 ) == 0 );
}

BOOST_AUTO_TEST_CASE( test_resize, *boost::unit_test::tolerance( 0.000001 ) )
{
    // just some arbitrary vectors
    VECTOR2I v1( 4, 3 );
    VECTOR2I v2( 5, -1 );
    VECTOR2I v3( -2, 1 );
    VECTOR2I v4( 1, 1 );
    VECTOR2I v5( -70, -70 );

    BOOST_TEST( v1.Resize( 8 ) == VECTOR2I( 6, 5 ) );
    BOOST_TEST( v2.Resize( 10 ) == VECTOR2I( 10, -2 ) );
    BOOST_TEST( v3.Resize( 4 ) == VECTOR2I( -4, 2 ) );
    BOOST_TEST( v4.Resize( 1 ) == VECTOR2I( 1, 1 ) );
    BOOST_TEST( v5.Resize( 100 ) == VECTOR2I( -71, -71 ) );
}

BOOST_AUTO_TEST_CASE( test_casting )
{
    VECTOR2I vint( 4, 3 );
    VECTOR2D vdouble( 4.0, 3.0 );
    VECTOR2L vlong( 4, 3 );
    VECTOR2<float> vfloat( 4.0f, 3.0f );
    VECTOR2<unsigned> vunsigned( 4, 3 );

    BOOST_CHECK_EQUAL( vint, VECTOR2I( vdouble ) );
    BOOST_CHECK_EQUAL( vint, VECTOR2I( vlong ) );
    BOOST_CHECK_EQUAL( vint, VECTOR2I( vfloat ) );
    BOOST_CHECK_EQUAL( vint, VECTOR2I( vunsigned ) );

    BOOST_CHECK_EQUAL( vdouble, VECTOR2D( vint ) );
    BOOST_CHECK_EQUAL( vdouble, VECTOR2D( vlong ) );
    BOOST_CHECK_EQUAL( vdouble, VECTOR2D( vfloat ) );
    BOOST_CHECK_EQUAL( vdouble, VECTOR2D( vunsigned ) );

    BOOST_CHECK_EQUAL( vlong, VECTOR2L( vint ) );
    BOOST_CHECK_EQUAL( vlong, VECTOR2L( vdouble ) );
    BOOST_CHECK_EQUAL( vlong, VECTOR2L( vfloat ) );
    BOOST_CHECK_EQUAL( vlong, VECTOR2L( vunsigned ) );

    BOOST_CHECK_EQUAL( vfloat, VECTOR2<float>( vint ) );
    BOOST_CHECK_EQUAL( vfloat, VECTOR2<float>( vdouble ) );
    BOOST_CHECK_EQUAL( vfloat, VECTOR2<float>( vlong ) );
    BOOST_CHECK_EQUAL( vfloat, VECTOR2<float>( vunsigned ) );

    BOOST_CHECK_EQUAL( vunsigned, VECTOR2<unsigned>( vint ) );
    BOOST_CHECK_EQUAL( vunsigned, VECTOR2<unsigned>( vdouble ) );
    BOOST_CHECK_EQUAL( vunsigned, VECTOR2<unsigned>( vlong ) );
    BOOST_CHECK_EQUAL( vunsigned, VECTOR2<unsigned>( vfloat ) );

    // Check that negative values are handled correctly
    vint = vint - 1;
    vdouble = vdouble - 1;
    vlong = vlong - 1;
    vfloat = vfloat - 1;
    vunsigned = vunsigned - 1;

    BOOST_CHECK_EQUAL( vint, VECTOR2I( 3, 2 ) );
    BOOST_CHECK_EQUAL( vdouble, VECTOR2D( 3.0, 2.0 ) );
    BOOST_CHECK_EQUAL( vlong, VECTOR2L( 3, 2 ) );
    BOOST_CHECK_EQUAL( vfloat, VECTOR2<float>( 3.0f, 2.0f ) );
    BOOST_CHECK_EQUAL( vunsigned, VECTOR2<unsigned>( 3, 2 ) );

    // Check that subtracting unsigned values works correctly
    vint = vint - (unsigned)1;
    vdouble = vdouble - (unsigned)1;
    vlong = vlong - (unsigned)1;
    vfloat = vfloat - (unsigned)1;
    vunsigned = vunsigned - (unsigned)1;

    BOOST_CHECK_EQUAL( vint, VECTOR2I( 2, 1 ) );
    BOOST_CHECK_EQUAL( vdouble, VECTOR2D( 2.0, 1.0 ) );
    BOOST_CHECK_EQUAL( vlong, VECTOR2L( 2, 1 ) );
    BOOST_CHECK_EQUAL( vfloat, VECTOR2<float>( 2.0f, 1.0f ) );
    BOOST_CHECK_EQUAL( vunsigned, VECTOR2<unsigned>( 2, 1 ) );

    vint = vint - 5.0;
    vdouble = vdouble - 5.0;
    vlong = vlong - 5.0;
    vfloat = vfloat - 5.0;
    vunsigned = vunsigned - 5.0;

    BOOST_CHECK_EQUAL( vint, VECTOR2I( -3, -4 ) );
    BOOST_CHECK_EQUAL( vdouble, VECTOR2D( -3.0, -4.0 ) );
    BOOST_CHECK_EQUAL( vlong, VECTOR2L( -3, -4 ) );
    BOOST_CHECK_EQUAL( vfloat, VECTOR2<float>( -3.0f, -4.0f ) );
    BOOST_CHECK_EQUAL( vunsigned, VECTOR2<unsigned>( 4294967293, 4294967292 ) ); // roll over unsigned when explicitly subtracting.

    // Check that negative initial values are handled correctly
    vint = VECTOR2I( -4, -3 );
    vdouble = VECTOR2D( -4.0, -3.0 );
    vlong = VECTOR2L( -4, -3 );
    vfloat = VECTOR2<float>( -4.0f, -3.0f );
    vunsigned = VECTOR2<unsigned>( -4, -3 );

    vint = vint - 1;
    vdouble = vdouble - 1;
    vlong = vlong - 1;
    vfloat = vfloat - 1;
    vunsigned = vunsigned - 1;

    BOOST_CHECK_EQUAL( vint, VECTOR2I( -5, -4 ) );
    BOOST_CHECK_EQUAL( vdouble, VECTOR2D( -5.0, -4.0 ) );
    BOOST_CHECK_EQUAL( vlong, VECTOR2L( -5, -4 ) );
    BOOST_CHECK_EQUAL( vfloat, VECTOR2<float>( -5.0f, -4.0f ) );
    BOOST_CHECK_EQUAL( vunsigned, VECTOR2<unsigned>( 4294967291, 4294967292 ) );

    vint = vint - 1u;
    vdouble = vdouble - 1u;
    vlong = vlong - 1u;
    vfloat = vfloat - 1u;

    BOOST_CHECK_EQUAL( vint, VECTOR2I( -6, -5 ) );
    BOOST_CHECK_EQUAL( vdouble, VECTOR2D( -6.0, -5.0 ) );
    BOOST_CHECK_EQUAL( vlong, VECTOR2L( -6, -5 ) );
    BOOST_CHECK_EQUAL( vfloat, VECTOR2<float>( -6.0f, -5.0f ) );

    auto add = vint + vdouble;
    BOOST_CHECK_EQUAL( add, VECTOR2D( -12.0, -10.0 ) );

    auto sub = vint - 2 * vlong;
    BOOST_CHECK_EQUAL( sub.x, 6 );
    BOOST_CHECK_EQUAL( sub.y, 5 );

    vunsigned = VECTOR2<unsigned>( std::numeric_limits<unsigned>::max(), std::numeric_limits<unsigned>::max() );
    vint = VECTOR2I( vunsigned );
    BOOST_CHECK_EQUAL( vint.x, std::numeric_limits<int>::max() );

    vunsigned += 1;
    BOOST_CHECK_EQUAL( vunsigned.x, 0 );
}

BOOST_AUTO_TEST_SUITE_END()
