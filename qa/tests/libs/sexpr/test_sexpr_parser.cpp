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
 * Test suite for SEXPR::PARSER
 */

#include <qa_utils/wx_utils/unit_test_utils.h>

// Code under test
#include <sexpr/sexpr_parser.h>

#include "sexpr_test_utils.h"

class TEST_SEXPR_PARSER_FIXTURE
{
public:
    /**
     * Wrap the parser function with a unique_ptr
     */
    std::unique_ptr<SEXPR::SEXPR> Parse( const std::string& aIn )
    {
        return std::unique_ptr<SEXPR::SEXPR>( m_parser.Parse( aIn ) );
    }

    SEXPR::PARSER m_parser;
};


/**
 * Collection of test cases for use when multiple cases can be handled in the
 * same test case.
 */
struct TEST_SEXPR_CASE
{
    std::string m_case_name;
    std::string m_sexpr_data;
};


/**
 * Declare the test suite
 */
BOOST_FIXTURE_TEST_SUITE( SexprParser, TEST_SEXPR_PARSER_FIXTURE )

/**
 * Cases that result in no s-expr object
 */
BOOST_AUTO_TEST_CASE( Empty )
{
    const std::vector<TEST_SEXPR_CASE> cases = {
        {
            "Empty string",
            "",
        },
        {
            "whitespace",
            "  ",
        },
    };

    for( const auto& c : cases )
    {
        BOOST_TEST_CONTEXT( c.m_case_name )
        {
            const auto data = Parse( c.m_sexpr_data );
            BOOST_CHECK_EQUAL( data.get(), nullptr );
        }
    }
}

/**
 * This comes out as a single symbol
 */
BOOST_AUTO_TEST_CASE( Words )
{
    const std::string content{ "this is just writing" };
    const auto        sexp = Parse( content );

    BOOST_REQUIRE_NE( sexp.get(), nullptr );
    BOOST_CHECK_PREDICATE( KI_TEST::SexprIsSymbolWithValue, ( *sexp )( "this" ) );
}

/**
 * This comes out as a single symbol
 */
BOOST_AUTO_TEST_CASE( ParseExceptions )
{
    const std::vector<TEST_SEXPR_CASE> cases = {
        {
            "Unclosed (symbol",
            "(symbol",
        },
        {
            "Comma",
            ",",
        },
        {
            "Int only",
            "1",
        },
        {
            "Double only",
            "3.14",
        },
        {
            "Symbol only",
            "symbol",
        },
        // { // this is OK for some reason
        //     "String only",
        //     "\"string\"",
        // },
    };

    for( const auto& c : cases )
    {
        BOOST_TEST_CONTEXT( c.m_case_name )
        {
            BOOST_CHECK_THROW( Parse( c.m_sexpr_data ), SEXPR::PARSE_EXCEPTION );
        }
    }
}

/**
 * This comes out as an empty list
 */
BOOST_AUTO_TEST_CASE( EmptyParens )
{
    const std::string content{ "()" };
    const auto        sexp = Parse( content );

    BOOST_REQUIRE_NE( sexp.get(), nullptr );
    BOOST_CHECK_PREDICATE( KI_TEST::SexprIsListOfLength, ( *sexp )( 0 ) );
}

/**
 * Single symbol in parens
 */
BOOST_AUTO_TEST_CASE( SimpleSymbol )
{
    const std::string content{ "(symbol)" };
    const auto        sexp = Parse( content );

    BOOST_REQUIRE_NE( sexp.get(), nullptr );
    BOOST_REQUIRE_PREDICATE( KI_TEST::SexprIsListOfLength, ( *sexp )( 1 ) );

    const SEXPR::SEXPR& child = *sexp->GetChild( 0 );
    BOOST_CHECK_PREDICATE( KI_TEST::SexprIsSymbolWithValue, ( child )( "symbol" ) );
}

/**
 * Test several atoms in a list, including nested lists
 */
BOOST_AUTO_TEST_CASE( SymbolString )
{
    const std::string content{ "(symbol \"string\" 42 3.14 (nested 4 ()))" };
    const auto        sexp = Parse( content );

    BOOST_REQUIRE_NE( sexp.get(), nullptr );
    BOOST_REQUIRE_PREDICATE( KI_TEST::SexprIsListOfLength, ( *sexp )( 5 ) );

    BOOST_CHECK_PREDICATE( KI_TEST::SexprIsSymbolWithValue, ( *sexp->GetChild( 0 ) )( "symbol" ) );
    BOOST_CHECK_PREDICATE( KI_TEST::SexprIsStringWithValue, ( *sexp->GetChild( 1 ) )( "string" ) );
    BOOST_CHECK_PREDICATE( KI_TEST::SexprIsIntegerWithValue, ( *sexp->GetChild( 2 ) )( 42 ) );
    BOOST_CHECK_PREDICATE( KI_TEST::SexprIsDoubleWithValue, ( *sexp->GetChild( 3 ) )( 3.14 ) );

    // and child list
    const SEXPR::SEXPR& sublist = *sexp->GetChild( 4 );
    BOOST_REQUIRE_PREDICATE( KI_TEST::SexprIsListOfLength, ( sublist )( 3 ) );
    BOOST_CHECK_PREDICATE(
            KI_TEST::SexprIsSymbolWithValue, ( *sublist.GetChild( 0 ) )( "nested" ) );
    BOOST_CHECK_PREDICATE( KI_TEST::SexprIsIntegerWithValue, ( *sublist.GetChild( 1 ) )( 4 ) );
    BOOST_CHECK_PREDICATE( KI_TEST::SexprIsListOfLength, ( *sublist.GetChild( 2 ) )( 0 ) );
}


/**
 * Test for roundtripping (valid) s-expression back to strings
 *
 * Note: the whitespace has to be the same in this test.
 */
struct TEST_SEXPR_ROUNDTRIPPING
{
    std::string m_case_name;
    std::string m_input;
};

BOOST_AUTO_TEST_CASE( StringRoundtrip )
{
    const std::vector<TEST_SEXPR_ROUNDTRIPPING> cases = {
        {
            "empty list",
            "()",
        },
        {
            "simple list",
            "(42 3.14 \"string\")",
        },
        {
            "nested list", // REVIEW space after 42?
            "(42 \n  (1 2))",
        },
    };

    for( const auto& c : cases )
    {
        BOOST_TEST_CONTEXT( c.m_case_name )
        {
            const auto sexp = Parse( c.m_input );

            const std::string as_str = sexp->AsString();

            BOOST_CHECK_PREDICATE( KI_TEST::SexprConvertsToString, ( *sexp )( c.m_input ) );
        }
    }
}

BOOST_AUTO_TEST_SUITE_END()