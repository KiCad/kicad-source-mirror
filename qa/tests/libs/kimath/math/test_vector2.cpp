/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2020 KiCad Developers, see AUTHORS.TXT for contributors.
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

BOOST_AUTO_TEST_CASE( test_cross_product, *boost::unit_test::tolerance( 0.000001 ) )
{
    VECTOR2I v1(0, 1);
    VECTOR2I v2(1, 0);

    BOOST_CHECK( v2.Cross( v1 ) == 1 );
}

BOOST_AUTO_TEST_CASE( test_dot_product, *boost::unit_test::tolerance( 0.000001 ) )
{
    VECTOR2I v1( 0, 1 );
    VECTOR2I v2( 1, 0 );

    BOOST_CHECK( v2.Dot( v1 ) == 0 );
}

BOOST_AUTO_TEST_CASE( test_resize, *boost::unit_test::tolerance( 0.000001 ) )
{
    // just some arbitrary vectors
    VECTOR2I v1( 4, 3 );
    VECTOR2I v2( 5, -1 );
    VECTOR2I v3( -2, 1 );
    VECTOR2I v4( 1, 1 );
    VECTOR2I v5( -70, -70 );

    BOOST_CHECK( v1.Resize( 8 ) == VECTOR2I( 6, 5 ) );
    BOOST_CHECK( v2.Resize( 10 ) == VECTOR2I( 10, -2 ) );
    BOOST_CHECK( v3.Resize( 4 ) == VECTOR2I( -4, 2 ) );
    BOOST_CHECK( v4.Resize( 1 ) == VECTOR2I( 1, 1 ) );
    BOOST_CHECK( v5.Resize( 100 ) == VECTOR2I( -71, -71 ) );
}

BOOST_AUTO_TEST_SUITE_END()
