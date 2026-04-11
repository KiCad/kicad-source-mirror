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
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

/**
 * Test suite for KiCad math code.
 */

#include <qa_utils/wx_utils/unit_test_utils.h>

// Code under test
#include <math/matrix3x3.h>

/**
 * Declare the test suite
 */
BOOST_AUTO_TEST_SUITE( MATRIX3X3TESTS )


BOOST_AUTO_TEST_CASE( test_equality_ops, *boost::unit_test::tolerance( 0.000001 ) )
{
    MATRIX3x3D m1( VECTOR3I{ 1, 1, 1 }, { 2, 2, 2 }, { 3, 3, 3 } );
    MATRIX3x3D m2( VECTOR3I{ 6, 6, 6 }, { 1, 1, 1 }, { 3, 3, 3 } );
    MATRIX3x3D m3( VECTOR3I{ 1, 1, 1 }, { 2, 2, 2 }, { 3, 3, 3 } );

    BOOST_TEST( m1 == m3 );
    BOOST_TEST( m2 != m1 );
}

BOOST_AUTO_TEST_CASE( test_matrix_multiply_vector, *boost::unit_test::tolerance( 0.000001 ) )
{
    MATRIX3x3 m1( VECTOR3I{ 1, 1, 1 }, { 2, 2, 2 }, { 3, 3, 3 } );
    VECTOR3I v1( 5, 5, 5 );

    VECTOR3I res = m1 * v1;

    VECTOR3I expected( 15, 30, 45 );

    BOOST_TEST( res == expected );
}


BOOST_AUTO_TEST_CASE( test_matrix_multiply_scalar, *boost::unit_test::tolerance( 0.000001 ) )
{
    MATRIX3x3 m1( VECTOR3I{ 1, 1, 1 }, { 2, 2, 2 }, { 3, 3, 3 } );

    MATRIX3x3 res = m1 * 5;

    MATRIX3x3 expected( VECTOR3I{ 5, 5, 5 }, { 10, 10, 10 }, { 15, 15, 15 } );

    BOOST_TEST( res == expected );
}


BOOST_AUTO_TEST_SUITE_END()
