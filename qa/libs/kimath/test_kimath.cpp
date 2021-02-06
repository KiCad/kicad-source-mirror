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
#include <trigo.h>


/**
 * Declare the test suite
 */
BOOST_AUTO_TEST_SUITE( KiMath )


BOOST_AUTO_TEST_CASE( TestInterceptsPositiveX )
{
    BOOST_CHECK( !InterceptsPositiveX( 10.0, 20.0 ) );
    BOOST_CHECK( !InterceptsPositiveX( 10.0, 120.0 ) );
    BOOST_CHECK( !InterceptsPositiveX( 10.0, 220.0 ) );
    BOOST_CHECK( !InterceptsPositiveX( 10.0, 320.0 ) );
    BOOST_CHECK( InterceptsPositiveX( 20.0, 10.0 ) );
    BOOST_CHECK( InterceptsPositiveX( 345.0, 15.0 ) );
}


BOOST_AUTO_TEST_CASE( TestInterceptsNegativeX )
{
    BOOST_CHECK( !InterceptsNegativeX( 10.0, 20.0 ) );
    BOOST_CHECK( !InterceptsNegativeX( 10.0, 120.0 ) );
    BOOST_CHECK( InterceptsNegativeX( 10.0, 220.0 ) );
    BOOST_CHECK( InterceptsNegativeX( 10.0, 320.0 ) );
    BOOST_CHECK( InterceptsNegativeX( 20.0, 10.0 ) );
    BOOST_CHECK( !InterceptsNegativeX( 345.0, 15.0 ) );
    BOOST_CHECK( InterceptsNegativeX( 145.0, 225.0 ) );
}


BOOST_AUTO_TEST_SUITE_END()
