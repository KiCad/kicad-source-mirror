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
 * @file
 * Test suite for refdes functions
 */

#include <qa_utils/wx_utils/unit_test_utils.h>

// Code under test
#include <refdes_utils.h>

/**
 * Declare the test suite
 */
BOOST_AUTO_TEST_SUITE( RefdesUtils )


/**
 * Test the #UTIL::GetRefDesPrefix function
 */
BOOST_AUTO_TEST_CASE( GetPrefix )
{
    using CASE = std::pair<std::string, std::string>;

    const std::vector<CASE> cases = {
        { "", "" },        // empty
        { "U", "U" },      // no number
        { "1", "" },       // only number
        { "IC", "IC" },    // >1 char prefix, no number
        { "U1", "U" },     // single digit
        { "IC21", "IC" },  // >1 char prefix + number
        { "U10", "U" },    // double digit
        { "U1000", "U" },  // multi digit
        { "U1U2", "U1U" }, // prefix contains digit
    };

    for( const auto& c : cases )
    {
        BOOST_TEST_CONTEXT( "Testing: " << c.first )
        {
            BOOST_CHECK_EQUAL( UTIL::GetRefDesPrefix( c.first ), c.second );
        }
    }
}


BOOST_AUTO_TEST_SUITE_END()
