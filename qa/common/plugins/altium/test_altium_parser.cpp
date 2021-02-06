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
 * @file test_altium_parser.cpp
 * Test suite for #ALTIUM_PARSER
 */

#include <qa_utils/wx_utils/unit_test_utils.h>

#include <common/plugins/altium/altium_parser.h>

struct ALTIUM_PARSER_FIXTURE
{
    ALTIUM_PARSER_FIXTURE() {}
};


/**
 * Declares the struct as the Boost test fixture.
 */
BOOST_FIXTURE_TEST_SUITE( AltiumParser, ALTIUM_PARSER_FIXTURE )

struct ALTIUM_TO_KICAD_UNIT_CASE
{
    int input;
    int exp_result;
};

/**
 * A list of valid internal unit conversation factors
 */
static const std::vector<ALTIUM_TO_KICAD_UNIT_CASE> altium_to_kicad_unit = {
    // Some simple values
    { 0, 0 },
    { 1, 3 },
    { 2, 5 },
    { 3, 8 },
    { 10, 25 },
    { 20, 51 },
    { 30, 76 },
    // Edge Cases
    { 845466002, 2147483645 },
    { -845466002, -2147483645 },
    // Clamp bigger values
    { 845466003, 2147483646 },
    { -845466003, -2147483646 },
    { 1000000000, 2147483646 },
    { -1000000000, -2147483646 },
    // imperial rounded units as input
    { 100, 254 },
    { 200, 508 },
    { 300, 762 },
    { 400, 1016 },
    { 500, 1270 },
    { 600, 1524 },
    { 700, 1778 },
    { 800, 2032 },
    { 900, 2286 },
    { 1000, 2540 },
    // metric rounded units as input
    { 394, 1000 },
    { 787, 2000 },
    { 1181, 3000 },
    { 1575, 4000 },
    { 1969, 5000 },
    { 2362, 6000 },
    { 2756, 7000 },
    { 3150, 8000 },
    { 3543, 9000 },
    { 3937, 10000 },
    { 39370, 100000 },
    { 78740, 200000 },
    { 118110, 300000 },
    { 157480, 400000 },
    { 196850, 500000 },
    { 236220, 600000 },
    { 275591, 700000 },
    { 314961, 800000 },
    { 354331, 900000 },
    { 393701, 1000000 },
    { -394, -1000 },
    { -787, -2000 },
    { -1181, -3000 },
    { -1575, -4000 },
    { -1969, -5000 },
    { -2362, -6000 },
    { -2756, -7000 },
    { -3150, -8000 },
    { -3543, -9000 },
    { -3937, -10000 },
    { -39370, -100000 },
    { -78740, -200000 },
    { -118110, -300000 },
    { -157480, -400000 },
    { -196850, -500000 },
    { -236220, -600000 },
    { -275591, -700000 },
    { -314961, -800000 },
    { -354331, -900000 },
    { -393701, -1000000 },
};

/**
 * Test conversation from Altium internal units into KiCad internal units
 */
BOOST_AUTO_TEST_CASE( ConvertToKicadUnit )
{
    for( const auto& c : altium_to_kicad_unit )
    {
        BOOST_TEST_CONTEXT( wxString::Format( wxT( "%i -> %i" ), c.input, c.exp_result ) )
        {
            int result = ALTIUM_PARSER::ConvertToKicadUnit( c.input );

            // These are all valid
            BOOST_CHECK_EQUAL( result, c.exp_result );
        }
    }
}

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
 * Test conversation from Unit property into KiCad internal units
 */
BOOST_AUTO_TEST_CASE( PropertiesReadKicadUnit )
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

struct READ_PROPERTIES_CASE
{
    std::string                  input;
    std::map<wxString, wxString> exp_result;
};

/**
 * A list of valid test strings and the expected result map
 */
static const std::vector<READ_PROPERTIES_CASE> read_properties = {
    // Empty
    { "", {} },
    { "\0", {} },
    { "|", {} },
    { "|\0", {} },
    { "||", {} },
    { "||\0", {} },
    // Empty key-value pair
    { "|=", { { "", "" } } },
    { "|=\0", { { "", "" } } },
    { "|  =  ", { { "", "" } } },
    { "|  =  \0", { { "", "" } } },
    // Single key-value pair
    { "|A=\0", { { "A", "" } } },
    { "|A=B", { { "A", "B" } } },
    { "|A=B\0", { { "A", "B" } } },
    { "|A=B|", { { "A", "B" } } },
    { "|A=B|\0", { { "A", "B" } } },
    // Multiple key-value pairs
    { "|A=B|C=D|\0", { { "A", "B" }, { "C", "D" } } },
    // Same key multiple times
    { "|A=B|A=C\0", { { "A", "B" } } },
    { "|A=B|A=C|A=D|A=E|A=F\0", { { "A", "B" } } },
    // Always upper case key
    { "|a=b\0", { { "A", "b" } } },
    { "|abc123=b\0", { { "ABC123", "b" } } },
    // Trim whitespaces, TODO: correct?
    { "|A=  B\0", { { "A", "  B" } } },
    { "|A=B  \0", { { "A", "B" } } },
    { "|  A=B\0", { { "A", "B" } } },
    { "|A  =B\0", { { "A", "B" } } },
    { "|A=\nB\n\0", { { "A", "\nB" } } },
    // Escaping and other special cases, TODO: extend
    //{ "|A=||\0", {{"A", "|"}} },
    { "|A==\0", { { "A", "=" } } },
    { "|A=a\na\0", { { "A", "a\na" } } },
    { "|A=a\ta\0", { { "A", "a\ta" } } },
    // Encoding, TODO: extend
    // Correct reading errors
    { "|A|B=C\0", { { "B", "C" } } },
    { "|A=B|C\0", { { "A", "B" } } },
};

/**
 * Test conversation from binary to properties
 */
BOOST_AUTO_TEST_CASE( ReadProperties )
{
    for( const auto& c : read_properties )
    {
        BOOST_TEST_CONTEXT( wxString::Format( wxT( "'%s'" ), c.input ) )
        {
            size_t                  size = 4 + c.input.size();
            std::unique_ptr<char[]> content = std::make_unique<char[]>( size );

            *content.get() = c.input.size();
            std::memcpy( content.get() + 4, c.input.c_str(), c.input.size() );

            ALTIUM_PARSER parser( content, size );

            std::map<wxString, wxString> result = parser.ReadProperties();

            BOOST_CHECK_EQUAL( parser.HasParsingError(), false );
            BOOST_CHECK_EQUAL( parser.GetRemainingBytes(), 0 );

            BOOST_CHECK_EQUAL( result.size(), c.exp_result.size() );
            for( const auto& kv : c.exp_result )
            {
                BOOST_CHECK_EQUAL( 1, result.count( kv.first ) );
                BOOST_CHECK_EQUAL( result.at( kv.first ), kv.second );
            }
        }
    }
}

BOOST_AUTO_TEST_SUITE_END()
