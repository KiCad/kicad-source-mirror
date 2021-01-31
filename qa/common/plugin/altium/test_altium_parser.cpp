/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2021 KiCad Developers, see CHANGELOG.TXT for contributors.
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
 * @file test_altium_parserr.cpp
 * Test suite for #ALTIUM_PARSER
 */

#include <unit_test_utils/unit_test_utils.h>

#include <common/plugins/altium/altium_parser.h>

struct ALTIUM_PARSER_FIXTURE
{
    ALTIUM_PARSER_FIXTURE() {}
};


/**
 * Declares the struct as the Boost test fixture.
 */
BOOST_FIXTURE_TEST_SUITE( AltiumParser, ALTIUM_PARSER_FIXTURE )

struct READ_KICAD_UNIT_CASE
{
    wxString input;
    int      exp_result;
};

/**
 * A list of valid test strings and the expected results
 */
static const std::vector<READ_KICAD_UNIT_CASE> read_kicad_unit_property = {
    // Empty (use default)
    { "", 0 },
    // Some simple cases
    { "0mil", 0 },
    { "1mil", 25400 },
    { "+1mil", 25400 },
    { "-1mil", -25400 },
    // Decimal Places
    { "0.1mil", 2540 },
    { "-0.1mil", -2540 },
    { "0.01mil", 254 },
    { "-0.01mil", -254 },
    { "0.001mil", 25 },
    { "-0.001mil", -25 },
    { "0.0001mil", 3 },
    { "-0.0001mil", -3 },
    { "0.00001mil", 0 },
    { "-0.00001mil", -0 },
    { "0.00002mil", 1 },
    { "-0.00002mil", -1 },
    // Big Numbers
    { "10mil", 254000 },
    { "-10mil", -254000 },
    { "100mil", 2540000 },
    { "-100mil", -2540000 },
    { "1000mil", 25400000 },
    { "-1000mil", -25400000 },
    { "10000mil", 254000000 },
    { "-10000mil", -254000000 },
    // Edge Cases
    { "84546.6002mil", 2147483645 },
    { "-84546.6002mil", -2147483645 },
    // Clamp bigger values
    { "84546.6003mil", 2147483646 },
    { "-84546.6003mil", -2147483646 },
    { "100000mil", 2147483646 },
    { "-100000mil", -2147483646 },
    { "1000000mil", 2147483646 },
    { "-1000000mil", -2147483646 },
    { "10000000mil", 2147483646 },
    { "-10000000mil", -2147483646 },
    // Incorrect suffix
    { "100", 0 },
    { "100mils", 0 },
    // Incorrect prefix
    { "+-100mil", 0 },
    { "a100mil", 0 },
};


/**
 * Run through a set of test strings, clearing in between
 */
BOOST_AUTO_TEST_CASE( Results )
{
    for( const auto& c : read_kicad_unit_property )
    {
        BOOST_TEST_CONTEXT( c.input + " -> " + wxString::Format( wxT( "%i" ), c.exp_result ) )
        {
            std::map<wxString, wxString> properties = { { "TEST", c.input } };

            int result = ALTIUM_PARSER::PropertiesReadKicadUnit( properties, "TEST", "0mil" );

            // These are all valid
            BOOST_CHECK_EQUAL( result, c.exp_result );
        }
    }
}

BOOST_AUTO_TEST_SUITE_END()
