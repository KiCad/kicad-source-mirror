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
 * Test suite for general string functions
 */

#include <qa_utils/wx_utils/unit_test_utils.h>

// Code under test
#include <string_utils.h>

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


/**
 * Test the #StrNumCmp method.
 */
BOOST_AUTO_TEST_CASE( NaturalNumberCompare )
{
    using CASE = std::pair<std::pair<std::string, std::string>, std::pair<int, int>>;

    const std::vector<CASE> cases = {
        { { "a", "b" }, { -1, -1 } },
        { { "b", "a" }, { 1, 1 } },
        { { "a", "a" }, { 0, 0 } },
        { { "a", "A" }, { 1, 0 } },
        { { "A", "a" }, { -1, 0 } },
        { { "a", "" }, { 1, 1 } },
        { { "", "a" }, { -1, -1 } },
        { { "1", "" }, { 1, 1 } },
        { { "", "1" }, { -1, -1 } },
        { { "10", "2" }, { 1, 1 } },
        { { "2", "10" }, { -1, -1 } },
        { { "2", "2" }, { 0, 0 } },
        { { "10", "10" }, { 0, 0 } },
        { { "01", "1" }, { 0, 0 } },
        { { "01a", "1a" }, { 0, 0 } },
        { { "01a", "1b" }, { -1, -1 } },
        { { "01b", "1a" }, { 1, 1 } },
        { { "10 ten", "2 two" }, { 1, 1 } },
        { { "SYM1", "sym2" }, { -1, -1 } },
        { { "sym2", "SYM1" }, { 1, 1 } },
        { { "a10b20c30", "a10b20c31" }, { -1, -1 } },
        { { "a10b20c31", "a10b20c30" }, { 1, 1 } },
        { { "10UF", "10UF" }, { 0, 0 } },
        { { "10uF", "10uF" }, { 0, 0 } },
        { { "u10", "u10" }, { 0, 0 } },
        { { "U10", "U10" }, { 0, 0 } },
        { { "u10", "U10" }, { 1, 0 } },
        { { "U10", "u10" }, { -1, 0 } },
        { { "U10.1", "U10.10" }, { -1, -1 } },
        { { "U10-1", "U10-10" }, { -1, -1 } },
        { { "U10,1", "U10,10" }, { -1, -1 } },
        { { "U10.A", "U10.a" }, { -1, 0 } },
        { { "U10-A", "U10-a" }, { -1, 0 } },
        { { "U10,A", "U10,a" }, { -1, 0 } },
    };

    for( const auto& c : cases )
    {
        BOOST_CHECK_MESSAGE( StrNumCmp( c.first.first, c.first.second ) == c.second.first,
                c.first.first + " AND " + c.first.second + " failed for case sensitive" );

        BOOST_CHECK_MESSAGE(
                StrNumCmp( c.first.first, c.first.second, true ) == c.second.second,
                c.first.first + " AND " + c.first.second + " failed for case insensitive" );
    }
}


BOOST_AUTO_TEST_CASE( ValueCompare )
{
    using CASE = std::pair<std::pair<wxString, wxString>, int>;

    const std::vector<CASE> cases = {
            { { wxT( "100" ),  wxT( "10" ) },     1 },
            { { wxT( "10K" ),  wxT( "1K" ) },     1 },
            { { wxT( "10K" ),  wxT( "1K5" ) },    1 },
            { { wxT( "10K" ),  wxT( "10,000" ) }, 0 },
            { { wxT( "1K5" ),  wxT( "1.5K" ) },   0 },
            { { wxT( "1K5" ),  wxT( "1,5K" ) },   0 },
            { { wxT( "K5" ),   wxT( "1K" ) },    -1 },
            { { wxT( "1K5" ),  wxT( "K55" ) },    1 },
            { { wxT( "1R5" ),  wxT( "1.5" ) },    0 },
            { { wxT( "1u5F" ), wxT( "1.5uF" ) },  0 },
            { { wxT( "1µ5" ),  wxT( "1u5" ) },    0 },
    };

    for( const auto& c : cases )
    {
        BOOST_CHECK_MESSAGE( ValueStringCompare( c.first.first, c.first.second ) == c.second,
                             c.first.first + " AND " + c.first.second + " failed" );
    }
}


/**
 * Test the #GetTrailingInt method.
 */
BOOST_AUTO_TEST_CASE( Double2Str )
{
    using CASE = std::pair<double, std::string>;

    // conceptually a little quirky because doubles do have all those pesky additional values
    const std::vector<CASE> cases = {
        { 0, "0" },
        { 1.000, "1" },
        { 1.050, "1.05" },                       // single trailing zero
        { 0.00001523, "0.00001523" },            // value less than the magic 0.0001 threshold
        { 0.00000000000000001523, "0" },         // really small decimal that gets cut off
        { 623523, "623523" },                    // large whole number
    };

    for( const auto& c : cases )
    {
        // Test both of these functions that work the same but the innards are different
        BOOST_CHECK_EQUAL( FormatDouble2Str( c.first ), c.second );
        BOOST_CHECK_EQUAL( UIDouble2Str( c.first ), c.second );
    }
}


/**
 * Test #EscapeHTML and #UnescapeHTML methods.
 */
BOOST_AUTO_TEST_CASE( HTMLEscape )
{
    using CASE = std::pair<wxString, wxString>;

    const std::vector<CASE> cases = {
        { wxS( "I will display € €" ), wxS( "I will display &#8364; &#x20AC;" ) },
        { wxS( "&lt;" ), wxS( "&amp;lt;" ) },
        { wxS( "Don't Ω" ), wxS( "Don&apos;t Ω" ) },
    };

    for( const auto& c : cases )
    {
        wxString original( c.first );
        wxString escaped = EscapeHTML( original );
        wxString unescaped = UnescapeHTML( escaped );

        wxString unescapedTest = UnescapeHTML( c.second );

        BOOST_CHECK_EQUAL( original.utf8_string(), unescaped.utf8_string() );
        BOOST_CHECK_EQUAL( original.utf8_string(), unescapedTest.utf8_string() );
    }
}


/**
 * Test #SortVariantNames().
 */
BOOST_AUTO_TEST_CASE( VariantNameSort )
{
    std::vector<wxString> variantNames;

    // Verify default variant name is always sorted to the beginning of the list.
    variantNames.emplace_back( wxS( "Variant1" ) );
    variantNames.emplace_back( GetDefaultVariantName() );
    std::sort( variantNames.begin(), variantNames.end(), SortVariantNames );

    BOOST_CHECK_EQUAL( variantNames[0], GetDefaultVariantName() );
    BOOST_CHECK_EQUAL( variantNames[1], wxS( "Variant1" ) );
}


BOOST_AUTO_TEST_SUITE_END()
