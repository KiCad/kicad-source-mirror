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
 * @file test_altium_parser_sch.cpp
 * Test suite for #ALTIUM_PARSER_SCH
 */

#include <qa_utils/wx_utils/unit_test_utils.h>

#include <eeschema/sch_io/altium/altium_parser_sch.h>

// Function declarations of private methods to test
int ReadKiCadUnitFrac( const std::map<wxString, wxString>& aProps,
                       const wxString&                     aKey );


struct ALTIUM_PARSER_SCH_FIXTURE
{
    ALTIUM_PARSER_SCH_FIXTURE() {}
};


/**
 * Declares the struct as the Boost test fixture.
 */
BOOST_FIXTURE_TEST_SUITE( AltiumParserSch, ALTIUM_PARSER_SCH_FIXTURE )

struct ALTIUM_TO_KICAD_UNIT_FRAC_CASE
{
    wxString input;
    wxString input_frac;
    int      exp_result;
};

/**
 * A list of valid internal unit conversation factors
 */
static const std::vector<ALTIUM_TO_KICAD_UNIT_FRAC_CASE> altium_to_kicad_unit_frac = {
    // Some simple values
    { "0", "0", 0 },
    { "1", "0", 2540 },
    { "2", "0", 5080 },
    { "-1", "0", -2540 },
    { "-2", "0", -5080 },
    // Decimal Places
    { "0", "1", 0 },
    { "0", "10", 0 },
    { "0", "100", 0 },
    { "0", "1000", 30 },
    { "0", "10000", 250 },
    { "1", "10000", 2790 },
    { "0", "-1", 0 },
    { "0", "-10", 0 },
    { "0", "-100", 0 },
    { "0", "-1000", -30 },
    { "0", "-10000", -250 },
    { "-1", "-10000", -2790 },
    // Edge Cases
    // Clamp bigger values
    // imperial rounded units as input
    // metric rounded units as input
};

/**
 * Test conversation from Altium internal units into KiCad internal units using properties with FRAC
 */
BOOST_AUTO_TEST_CASE( PropertiesReadKiCadUnitFracConversation )
{
    for( const auto& c : altium_to_kicad_unit_frac )
    {
        BOOST_TEST_CONTEXT(
                wxString::Format( wxT( "%s FRAC %s -> %i" ), c.input, c.input_frac, c.exp_result ) )
        {
            std::map<wxString, wxString> properties = { { "TEST", c.input },
                                                        { "TEST_FRAC", c.input_frac } };

            int result = ReadKiCadUnitFrac( properties, "TEST" );

            // These are all valid
            BOOST_CHECK_EQUAL( result, c.exp_result );
        }
    }
}


BOOST_AUTO_TEST_SUITE_END()
