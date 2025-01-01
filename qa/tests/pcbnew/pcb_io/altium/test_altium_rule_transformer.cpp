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
 * @file test_altium_rule_transformer.cpp
 * Test suite for #ALTIUM_RULE_TOKENIZER
 */

#include <qa_utils/wx_utils/unit_test_utils.h>

#include <pcbnew/pcb_io/altium/altium_rule_transformer.h>

struct ALTIUM_RULE_TRANSFORMER_FIXTURE
{
    ALTIUM_RULE_TRANSFORMER_FIXTURE() {}
};


/**
 * Declares the struct as the Boost test fixture.
 */
BOOST_FIXTURE_TEST_SUITE( AltiumRuleTransformer, ALTIUM_RULE_TRANSFORMER_FIXTURE )


BOOST_AUTO_TEST_CASE( AltiumRuleTokenizerEmptyInput )
{
    ALTIUM_RULE_TOKENIZER tokenizer( "" );

    const ALTIUM_RULE_TOKEN& peek = tokenizer.Peek();
    BOOST_CHECK( ( ALTIUM_RULE_TOKEN_KIND::END_OF_EXPR == peek.kind ) );
    BOOST_CHECK_EQUAL( 0, peek.pos );

    const ALTIUM_RULE_TOKEN& next = tokenizer.Next();
    BOOST_CHECK( ( ALTIUM_RULE_TOKEN_KIND::END_OF_EXPR == next.kind ) );
    BOOST_CHECK_EQUAL( 0, next.pos );

    const ALTIUM_RULE_TOKEN& peek2 = tokenizer.Peek();
    BOOST_CHECK( ( ALTIUM_RULE_TOKEN_KIND::END_OF_EXPR == peek2.kind ) );
    BOOST_CHECK_EQUAL( 0, peek2.pos );
}

BOOST_AUTO_TEST_CASE( AltiumRuleTokenizerOnlySpaces )
{
    ALTIUM_RULE_TOKENIZER tokenizer( "   " );

    const ALTIUM_RULE_TOKEN& peek = tokenizer.Peek();
    BOOST_CHECK( ( ALTIUM_RULE_TOKEN_KIND::END_OF_EXPR == peek.kind ) );
    BOOST_CHECK_EQUAL( 3, peek.pos );

    const ALTIUM_RULE_TOKEN& next = tokenizer.Next();
    BOOST_CHECK( ( ALTIUM_RULE_TOKEN_KIND::END_OF_EXPR == next.kind ) );
    BOOST_CHECK_EQUAL( 3, next.pos );

    const ALTIUM_RULE_TOKEN& peek2 = tokenizer.Peek();
    BOOST_CHECK( ( ALTIUM_RULE_TOKEN_KIND::END_OF_EXPR == peek2.kind ) );
    BOOST_CHECK_EQUAL( 3, peek2.pos );
}

BOOST_AUTO_TEST_CASE( AltiumRuleTokenizerSingleCharIdentifier )
{
    ALTIUM_RULE_TOKENIZER tokenizer( "a" );

    const ALTIUM_RULE_TOKEN& next = tokenizer.Next();
    BOOST_CHECK( ( ALTIUM_RULE_TOKEN_KIND::IDENT == next.kind ) );
    BOOST_CHECK_EQUAL( 0, next.pos );
    BOOST_CHECK_EQUAL( "a", next.sValue );

    const ALTIUM_RULE_TOKEN& peek = tokenizer.Peek();
    BOOST_CHECK( ( ALTIUM_RULE_TOKEN_KIND::END_OF_EXPR == peek.kind ) );
    BOOST_CHECK_EQUAL( 1, peek.pos );

    const ALTIUM_RULE_TOKEN& next2 = tokenizer.Next();
    BOOST_CHECK( ( ALTIUM_RULE_TOKEN_KIND::END_OF_EXPR == next2.kind ) );
    BOOST_CHECK_EQUAL( 1, next2.pos );
}


struct ALTIUM_RULE_TOKENIZER_INPUT_OUTPUT
{
    wxString                       input;
    std::vector<ALTIUM_RULE_TOKEN> exp_token;
};

/**
 * A list of valid test strings and the expected results
 */
static const std::vector<ALTIUM_RULE_TOKENIZER_INPUT_OUTPUT> altium_rule_tokens_property = {
    // Empty string
    { "",
        {
            { ALTIUM_RULE_TOKEN_KIND::END_OF_EXPR, 0 },
            { ALTIUM_RULE_TOKEN_KIND::END_OF_EXPR, 0 }
        }
    },
    // Single Token
    { "All",
        {
            { ALTIUM_RULE_TOKEN_KIND::IDENT, 0, "All" },
            { ALTIUM_RULE_TOKEN_KIND::END_OF_EXPR, 3 }
        }
    },
    { "1234",
        {
            { ALTIUM_RULE_TOKEN_KIND::CONST_INT, 0, 1234L },
            { ALTIUM_RULE_TOKEN_KIND::END_OF_EXPR, 4 }
        }
    },
    { "+1234",
        {
            { ALTIUM_RULE_TOKEN_KIND::CONST_INT, 0, 1234L },
            { ALTIUM_RULE_TOKEN_KIND::END_OF_EXPR, 5 }
        }
    },
    { "-1234",
        {
            { ALTIUM_RULE_TOKEN_KIND::CONST_INT, 0, -1234L },
            { ALTIUM_RULE_TOKEN_KIND::END_OF_EXPR, 5 }
        }
    },
    { "'1234'",
        {
            { ALTIUM_RULE_TOKEN_KIND::CONST_STRING, 0, "1234" },
            { ALTIUM_RULE_TOKEN_KIND::END_OF_EXPR, 6 }
        }
    },
    { "True",
        {
            { ALTIUM_RULE_TOKEN_KIND::CONST_TRUE, 0 },
            { ALTIUM_RULE_TOKEN_KIND::END_OF_EXPR, 4 }
        }
    },
    { "true",
        {
            { ALTIUM_RULE_TOKEN_KIND::CONST_TRUE, 0 },
            { ALTIUM_RULE_TOKEN_KIND::END_OF_EXPR, 4 }
        }
    },
    { "False",
        {
            { ALTIUM_RULE_TOKEN_KIND::CONST_FALSE, 0 },
            { ALTIUM_RULE_TOKEN_KIND::END_OF_EXPR, 5 }
        }
    },
    { "false",
        {
            { ALTIUM_RULE_TOKEN_KIND::CONST_FALSE, 0 },
            { ALTIUM_RULE_TOKEN_KIND::END_OF_EXPR, 5 }
        }
    },
    { "+",
        {
            { ALTIUM_RULE_TOKEN_KIND::ADD, 0 },
            { ALTIUM_RULE_TOKEN_KIND::END_OF_EXPR, 1 }
        }
    },
    { "-",
        {
            { ALTIUM_RULE_TOKEN_KIND::SUB, 0 },
            { ALTIUM_RULE_TOKEN_KIND::END_OF_EXPR, 1 }
        }
    },
    { "*",
        {
            { ALTIUM_RULE_TOKEN_KIND::MUL, 0 },
            { ALTIUM_RULE_TOKEN_KIND::END_OF_EXPR, 1 }
        }
    },
    { "/",
        {
            { ALTIUM_RULE_TOKEN_KIND::DIV, 0 },
            { ALTIUM_RULE_TOKEN_KIND::END_OF_EXPR, 1 }
        }
    },
    { "Div",
        {
            { ALTIUM_RULE_TOKEN_KIND::INTEGRAL_DIV, 0 },
            { ALTIUM_RULE_TOKEN_KIND::END_OF_EXPR, 3 }
        }
    },
    { "div",
        {
            { ALTIUM_RULE_TOKEN_KIND::INTEGRAL_DIV, 0 },
            { ALTIUM_RULE_TOKEN_KIND::END_OF_EXPR, 3 }
        }
    },
    { "Mod",
        {
            { ALTIUM_RULE_TOKEN_KIND::MOD, 0 },
            { ALTIUM_RULE_TOKEN_KIND::END_OF_EXPR, 3 }
        }
    },
    { "mod",
        {
            { ALTIUM_RULE_TOKEN_KIND::MOD, 0 },
            { ALTIUM_RULE_TOKEN_KIND::END_OF_EXPR, 3 }
        }
    },
    { "And",
        {
            { ALTIUM_RULE_TOKEN_KIND::AND, 0 },
            { ALTIUM_RULE_TOKEN_KIND::END_OF_EXPR, 3 }
        }
    },
    { "and",
        {
            { ALTIUM_RULE_TOKEN_KIND::AND, 0 },
            { ALTIUM_RULE_TOKEN_KIND::END_OF_EXPR, 3 }
        }
    },
    { "&&",
        {
            { ALTIUM_RULE_TOKEN_KIND::LOW_AND, 0 },
            { ALTIUM_RULE_TOKEN_KIND::END_OF_EXPR, 2 }
        }
    },
    { "Or",
        {
            { ALTIUM_RULE_TOKEN_KIND::OR, 0 },
            { ALTIUM_RULE_TOKEN_KIND::END_OF_EXPR, 2 }
        }
    },
    { "or",
        {
            { ALTIUM_RULE_TOKEN_KIND::OR, 0 },
            { ALTIUM_RULE_TOKEN_KIND::END_OF_EXPR, 2 }
        }
    },
    { "||",
        {
            { ALTIUM_RULE_TOKEN_KIND::LOW_OR, 0 },
            { ALTIUM_RULE_TOKEN_KIND::END_OF_EXPR, 2 }
        }
    },
    { "Xor",
        {
            { ALTIUM_RULE_TOKEN_KIND::XOR, 0 },
            { ALTIUM_RULE_TOKEN_KIND::END_OF_EXPR, 3 }
        }
    },
    { "xor",
        {
            { ALTIUM_RULE_TOKEN_KIND::XOR, 0 },
            { ALTIUM_RULE_TOKEN_KIND::END_OF_EXPR, 3 }
        }
    },
    { "Not",
        {
            { ALTIUM_RULE_TOKEN_KIND::NOT, 0 },
            { ALTIUM_RULE_TOKEN_KIND::END_OF_EXPR, 3 }
        }
    },
    { "not",
        {
            { ALTIUM_RULE_TOKEN_KIND::NOT, 0 },
            { ALTIUM_RULE_TOKEN_KIND::END_OF_EXPR, 3 }
        }
    },
    { "<",
        {
            { ALTIUM_RULE_TOKEN_KIND::LESS, 0 },
            { ALTIUM_RULE_TOKEN_KIND::END_OF_EXPR, 1 }
        }
    },
    { "<=",
        {
            { ALTIUM_RULE_TOKEN_KIND::LESS_EQUAL, 0 },
            { ALTIUM_RULE_TOKEN_KIND::END_OF_EXPR, 2 }
        }
    },
    { ">",
        {
            { ALTIUM_RULE_TOKEN_KIND::GREATER, 0 },
            { ALTIUM_RULE_TOKEN_KIND::END_OF_EXPR, 1 }
        }
    },
    { ">=",
        {
            { ALTIUM_RULE_TOKEN_KIND::GREATER_EQUAL, 0 },
            { ALTIUM_RULE_TOKEN_KIND::END_OF_EXPR, 2 }
        }
    },
    { "<>",
        {
            { ALTIUM_RULE_TOKEN_KIND::NOT_EQUAL, 0 },
            { ALTIUM_RULE_TOKEN_KIND::END_OF_EXPR, 2 }
        }
    },
    { "=",
        {
            { ALTIUM_RULE_TOKEN_KIND::EQUAL, 0 },
            { ALTIUM_RULE_TOKEN_KIND::END_OF_EXPR, 1 }
        }
    },
    { "Between",
        {
            { ALTIUM_RULE_TOKEN_KIND::BETWEEN, 0 },
            { ALTIUM_RULE_TOKEN_KIND::END_OF_EXPR, 7 }
        }
    },
    { "between",
        {
            { ALTIUM_RULE_TOKEN_KIND::BETWEEN, 0 },
            { ALTIUM_RULE_TOKEN_KIND::END_OF_EXPR, 7 }
        }
    },
    { "Like",
        {
            { ALTIUM_RULE_TOKEN_KIND::LIKE, 0 },
            { ALTIUM_RULE_TOKEN_KIND::END_OF_EXPR, 4 }
        }
    },
    { "like",
        {
            { ALTIUM_RULE_TOKEN_KIND::LIKE, 0 },
            { ALTIUM_RULE_TOKEN_KIND::END_OF_EXPR, 4 }
        }
    },
    // Multiple tokens
    { "ab cd ef",
        {
            { ALTIUM_RULE_TOKEN_KIND::IDENT, 0, "ab" },
            { ALTIUM_RULE_TOKEN_KIND::IDENT, 3, "cd" },
            { ALTIUM_RULE_TOKEN_KIND::IDENT, 6, "ef" },
            { ALTIUM_RULE_TOKEN_KIND::END_OF_EXPR, 8 }
        }
    },
    // Complex tests
    { "InComponent('LEDS1') or InComponent('LEDS2')",
        {
            { ALTIUM_RULE_TOKEN_KIND::IDENT, 0, "InComponent" },
            { ALTIUM_RULE_TOKEN_KIND::LPAR, 11 },
            { ALTIUM_RULE_TOKEN_KIND::CONST_STRING, 12, "LEDS1" },
            { ALTIUM_RULE_TOKEN_KIND::RPAR, 19 },
            { ALTIUM_RULE_TOKEN_KIND::OR, 21 },
            { ALTIUM_RULE_TOKEN_KIND::IDENT, 24, "InComponent" },
            { ALTIUM_RULE_TOKEN_KIND::LPAR, 35 },
            { ALTIUM_RULE_TOKEN_KIND::CONST_STRING, 36, "LEDS2" },
            { ALTIUM_RULE_TOKEN_KIND::RPAR, 43 },
            { ALTIUM_RULE_TOKEN_KIND::END_OF_EXPR, 44 }
        }
    }
};

/**
 * Test conversation from wxString to Altium tokens
 */
BOOST_AUTO_TEST_CASE( AltiumRuleTokenizerParameterizedTest )
{
    for( const auto& c : altium_rule_tokens_property )
    {
        BOOST_TEST_CONTEXT( wxString::Format( wxT( "'%s'" ), c.input ) )
        {
            ALTIUM_RULE_TOKENIZER tokenizer( c.input );

            for( const auto& expected : c.exp_token )
            {
                const ALTIUM_RULE_TOKEN& token = tokenizer.Next();
                BOOST_CHECK( ( expected.kind == token.kind ) );
                BOOST_CHECK_EQUAL( expected.pos, token.pos );
                BOOST_CHECK_EQUAL( expected.iValue, token.iValue );
                BOOST_CHECK_EQUAL( expected.fValue, token.fValue );
                BOOST_CHECK_EQUAL( expected.sValue, token.sValue );
            }
        }
    }
}

BOOST_AUTO_TEST_SUITE_END()
