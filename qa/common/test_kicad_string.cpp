/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2018 KiCad Developers, see CHANGELOG.TXT for contributors.
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
 * Test suite for general string functions
 */

#include <unit_test_utils/unit_test_utils.h>

// Code under test
#include <kicad_string.h>

/**
 * Declare the test suite
 */
BOOST_AUTO_TEST_SUITE( KicadString )

/**
 * Test the #GetTrailingInt method.
 */
BOOST_AUTO_TEST_CASE( TrailingInt )
{
    using CASE = std::pair<std::string, int>;

    const std::vector<CASE> cases = {
        { "", 0 }, { "foo", 0 },            // no int
        { "0", 0 },                         // only int
        { "42", 42 },                       // only int
        { "1001", 1001 },                   // only int
        { "Foo42", 42 }, { "12Foo42", 42 }, // only the trailing
        { "12Foo4.2", 2 },                  // no dots
    };

    for( const auto& c : cases )
    {
        BOOST_CHECK_EQUAL( GetTrailingInt( c.first ), c.second );
    }
}

BOOST_AUTO_TEST_SUITE_END()