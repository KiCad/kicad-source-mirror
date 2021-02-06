/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2020 KiCad Developers, see CHANGELOG.TXT for contributors.
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
#include <math/vector3.h>

/**
 * Declare the test suite
 */
BOOST_AUTO_TEST_SUITE( VECTOR3TESTS )

BOOST_AUTO_TEST_CASE( test_cross_product, *boost::unit_test::tolerance( 0.000001 ) )
{
    VECTOR3I v1( 0, 1, 2 );
    VECTOR3I v2( 2, 1, 0 );

    BOOST_CHECK( v1.Cross( v2 ) == VECTOR3I(-2, 4 ,-2) );
}

BOOST_AUTO_TEST_CASE( test_dot_product, *boost::unit_test::tolerance( 0.000001 ) )
{
    VECTOR3I v1( 0, 1, 2 );
    VECTOR3I v2( 2, 1, 0 );

    BOOST_CHECK( v1.Dot( v2 ) == 1 );
}


BOOST_AUTO_TEST_SUITE_END()
