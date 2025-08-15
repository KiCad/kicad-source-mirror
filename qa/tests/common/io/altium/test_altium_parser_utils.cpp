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
 * @file test_altium_parser_utils.cpp
 * Test suite for #ALTIUM_PARSER
 */

#include <qa_utils/wx_utils/unit_test_utils.h>
#include <boost/test/data/test_case.hpp>

#include <common/io/altium/altium_parser_utils.h>

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

std::ostream & operator<<(std::ostream & strm, SPECIAL_STRINGS_TO_KICAD const & data) {
    return strm << "[" << data.input << " -> " << data.exp_result << "]";
}

/**
 * A list of valid test strings and the expected results
 */
static const std::vector<SPECIAL_STRINGS_TO_KICAD> sch_special_string_to_kicad_property = {
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
    { "+", "+", {} },
    { "'", "'", {} },
    { "'A'", "'A'", {} },
    { "A+B", "A+B", {} },
    { "A=B", "A=B", {} },
    { "$", "$", {} },
    { "{", "{", {} },
    { "}", "}", {} },
    { "${A}", "${A}", {} }, // TODO: correct substitution
    // Simple special strings
    { "=A", "${A}", {} },
    { "=A", "${C}", { { "A", "C" } } },
    { "=A_B", "${A_B}", {} },
    // Combined special strings
    { "=A+B", "${A}${B}", {} },
    { "=A+B", "${A}${B}", {} },
    { "=A+B", "${C}${B}", { { "A", "C" } } },
    { "=A+B", "${C}${D}", { { "A", "C" }, { "B", "D" } } },
    // Case-insensitive special strings
    { "=A", "${C}", { { "A", "C" } } },
    { "=a", "${C}", { { "A", "C" } } },
    { "=AB", "${C}", { { "AB", "C" } } },
    { "=aB", "${C}", { { "AB", "C" } } },
    { "=Ab", "${C}", { { "AB", "C" } } },
    { "=ab", "${C}", { { "AB", "C" } } },
    // Special strings with text
    { "='A'", "A", {} },
    { "='This is a long text with spaces'", "This is a long text with spaces", {} },
    { "='='", "=", {} },
    { "='+'", "+", {} },
    { "='$'", "$", {} },
    { "='{'", "{", {} },
    { "='}'", "}", {} },
    { "='${A}'", "${A}", {} }, // TODO: correct substitution
    { "='A'+'B'", "AB", {} },
    { "='A'+' '", "A ", {} },
    { "=' '+'B'", " B", {} },
    { "='A'+B", "A${B}", {} },
    { "='A'+\"B\"", "A${B}", {} },
    { "='A' + \"B\"", "A${B}", {} },
    { "=\"A\"+'B'", "${A}B", {} },
    { "=\"A\" + 'B'", "${A}B", {} },
    { "=A+'B'", "${A}B", {} },
    { "=A+' '+B", "${A} ${B}", {} },
    { "='A'+B+'C'+D", "A${B}C${D}", {} },
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
    { "= A", "${A}", {} },
    { "=A ", "${A}", {} },
    { "='A'B", "A", {} },
    { "=A'B'", "B", {} },
    { "=A'B", "B", {} },
    { "=A+ 'B'", "${A}B", {} },
};


/**
 * Test conversation from Altium Schematic Special String to a KiCad String with variables
 */
BOOST_DATA_TEST_CASE( AltiumSchSpecialStringsToKiCadVariablesProperties,
                      boost::unit_test::data::make(sch_special_string_to_kicad_property),
                      data )
{
    wxString result = AltiumSchSpecialStringsToKiCadVariables( data.input, data.override );

    // These are all valid
    BOOST_CHECK_EQUAL( result, data.exp_result );
}


/**
 * A list of valid test strings and the expected results
 */
static const std::vector<SPECIAL_STRINGS_TO_KICAD> pcb_special_string_to_kicad_property = {
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
    { "'", "'", {} },
    { "'A'", "'A'", {} },
    { "$", "$", {} },
    { "{", "{", {} },
    { "}", "}", {} },
    { "${A}", "${A}", {} }, // TODO: correct substitution
    // Simple special strings starting with dot
    { ".A", "${A}", {} },
    { ".A", "${C}", { { "A", "C" } } },
    { ".A_B", "${A_B}", {} },
    // Concatenated special strings
    { "'.A'", "${A}", {} },
    { "'.A''.B'", "${A}${B}", {} },
    { "'.A''.B'", "${C}${B}", { { "A", "C" } } },
    { "'.A''.B'", "${CC}${D}", { { "A", "CC" }, { "B", "D" } } },
    { "A='.A', B='.B'", "A=${A}, B=${B}", {} },
    // Case-insensitive special strings
    { ".A", "${C}", { { "A", "C" } } },
    { ".a", "${C}", { { "A", "C" } } },
    { ".AB", "${C}", { { "AB", "C" } } },
    { ".aB", "${C}", { { "AB", "C" } } },
    { ".Ab", "${C}", { { "AB", "C" } } },
    { ".ab", "${C}", { { "AB", "C" } } },
    // Some special cases we do not know yet how to handle correctly. But we should not crash ;)
    { "''", "''", {} },
    { " .", " .", {} },
    { ". ", "${ }", {} },
    { ". A", "${ A}", {} },
    { ".A ", "${A }", {} },
    { " .A", " .A", {} },
    { "...", "${..}", {} },
};


/**
 * Test conversation from Altium Board Special String to a KiCad String with variables
 */
BOOST_DATA_TEST_CASE( AltiumPcbSpecialStringsToKiCadStringsProperties,
                      boost::unit_test::data::make(pcb_special_string_to_kicad_property),
                      data )
{
    wxString result = AltiumPcbSpecialStringsToKiCadStrings( data.input, data.override );

    // These are all valid
    BOOST_CHECK_EQUAL( result, data.exp_result );
}

BOOST_AUTO_TEST_SUITE_END()
