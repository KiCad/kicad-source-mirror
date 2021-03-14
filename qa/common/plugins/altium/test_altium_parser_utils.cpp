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
 * @file test_altium_parser_utils.cpp
 * Test suite for #ALTIUM_PARSER
 */

#include <unit_test_utils/unit_test_utils.h>

#include <common/plugins/altium/altium_parser_utils.h>

struct ALTIUM_PARSER_UTILS_FIXTURE
{
    ALTIUM_PARSER_UTILS_FIXTURE() {}
};


/**
 * Declares the struct as the Boost test fixture.
 */
BOOST_FIXTURE_TEST_SUITE( AltiumParserUtils, ALTIUM_PARSER_UTILS_FIXTURE )


struct SPECIAL_STRINGS_TO_KICAD
{
    wxString                     input;
    wxString                     exp_result;
    std::map<wxString, wxString> override;
};

/**
 * A list of valid test strings and the expected results
 */
static const std::vector<SPECIAL_STRINGS_TO_KICAD> special_string_to_kicad_property = {
    // Empty
    { "", "", {} },
    // No Special Strings
    { "A", "A", {} },
    { "  A", "  A", {} },
    { "A  ", "A  ", {} },
    { "A=B", "A=B", {} },
    { "A=B+C", "A=B+C", {} },
    { "A\nB", "A\nB", {} },
    { "A\tB", "A\tB", {} },
    { "This is a long text with spaces", "This is a long text with spaces", {} },
    // Text format (underscore,...), TODO: add
    // Escaping, TODO: add
    { "A=B", "A=B", {} },
    // Simple special strings
    { "=A", "${A}", {} },
    { "=A", "C", { { "A", "C" } } },
    { "=A", "${C}", { { "A", "${C}" } } },
    { "=A_B", "${A_B}", {} },
    // Combined special strings
    { "=A+B", "${A}${B}", {} },
    { "=A+B", "${A}${B}", {} },
    { "=A+B", "C${B}", { { "A", "C" } } },
    { "=A+B", "CD", { { "A", "C" }, { "B", "D" } } },
    // Some special cases we do not know yet how to handle correctly. But we should not crash ;)
    { "=+", "", {} },
    { "=++", "", {} },
    { "=+++", "", {} },
    { "=B+", "${B}", {} },
    { "=+B", "${B}", {} },
    { "=B++", "${B}", {} },
    { "=+B+", "${B}", {} },
    { "=++B", "${B}", {} },
    { " =", " =", {} },
    { "= ", "", {} },
    { "= A", "${ A}", {} },
    { "=A ", "${A}", {} },
};


/**
 * Test conversation from Altium Special String to a KiCad String with variables
 */
BOOST_AUTO_TEST_CASE( AltiumSpecialStringsToKiCadVariablesProperties )
{
    for( const auto& c : special_string_to_kicad_property )
    {
        BOOST_TEST_CONTEXT( wxString::Format( wxT( "'%s' -> '%s'" ), c.input, c.exp_result ) )
        {
            wxString result = AltiumSpecialStringsToKiCadVariables( c.input, c.override );

            // These are all valid
            BOOST_CHECK_EQUAL( result, c.exp_result );
        }
    }
}

BOOST_AUTO_TEST_SUITE_END()
