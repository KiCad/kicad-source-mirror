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
 * @file test_numeric_evaluator.cpp
 * Test suite for #NUMERIC_EVALUATOR
 */

#include <qa_utils/wx_utils/unit_test_utils.h>

#include <libeval/numeric_evaluator.h>

struct NUM_EVAL_FIXTURE
{
    NUM_EVAL_FIXTURE() :
            m_eval( EDA_UNITS::MM )
    {
    }

    NUMERIC_EVALUATOR m_eval;
};


/**
 * Declares the struct as the Boost test fixture.
 */
BOOST_FIXTURE_TEST_SUITE( NumericEvaluator, NUM_EVAL_FIXTURE )


/**
 * Struct representing a test case for #NUMERIC_EVALUATOR
 */
struct EVAL_CASE
{
    wxString input;
    wxString exp_result;
};


/**
 * Basic class ops: set one up, trivial input, tear it down
 */
BOOST_AUTO_TEST_CASE( Basic )
{
    m_eval.Process( "1" );
    BOOST_CHECK_EQUAL( m_eval.Result(), "1" );
}

/**
 * Check that getting/setting vars works
 */
BOOST_AUTO_TEST_CASE( SetVar )
{
    m_eval.SetVar( "MoL", 42 );

    m_eval.Process( "1 + MoL" );
    BOOST_CHECK_EQUAL( m_eval.GetVar( "MoL" ), 42 );
    BOOST_CHECK_EQUAL( m_eval.Result(), "43" );

    m_eval.SetVar( "MoL", 422 );

    // have to process again to re-evaluate
    m_eval.Process( "1 + MoL" );
    BOOST_CHECK_EQUAL( m_eval.Result(), "423" );

    // Can remove one var
    m_eval.SetVar( "pi", 3.14 );
    BOOST_CHECK_EQUAL( m_eval.GetVar( "pi" ), 3.14 );
    m_eval.RemoveVar( "pi" );
    BOOST_CHECK_EQUAL( m_eval.GetVar( "pi" ), 0.0 );

    // Other is still there
    BOOST_CHECK_EQUAL( m_eval.GetVar( "MoL" ), 422 );

    // Add another one back
    m_eval.SetVar( "piish", 3.1 );

    // String clear doesn't clear vars
    m_eval.Clear();
    m_eval.Process( "1 + MoL + piish" );
    BOOST_CHECK_EQUAL( m_eval.Result(), "426.1" );

    // Clear both
    m_eval.ClearVar();
    BOOST_CHECK_EQUAL( m_eval.GetVar( "MoL" ), 0.0 );
    BOOST_CHECK_EQUAL( m_eval.GetVar( "piish" ), 0.0 );
}

/**
 * A list of valid test strings and the expected results
 */
static const std::vector<EVAL_CASE> eval_cases_valid = {
    // Empty case
    { "", "0" },
    // Trivial eval
    { "1", "1" },
    // Decimal separators
    { "1.5", "1.5" },
    { "1,5", "1.5" },
    // Semicolon is valid, but the result is NaN
    { "1;", "NaN" },
    // With own unit
    { "1mm", "1" },
    // Unit that's not the evaluator's unit
    { "1in", "25.4" },
    // Unit with white-space
    { "1 in", "25.4" },
    // Unit-less arithmetic
    { "1+2", "3" },
    // Multiple units
    { "1 + 10mm + 1\" + 1.5in + 500mil", "87.2" },
    // Any White-space is OK
    { "   1 +     2    ", "3" },
    // Decimals are OK in expressions
    { "1.5 + 0.2 + .1", "1.8" },
    // Negatives are OK
    { "3 - 10", "-7" },
    // Lots of operands
    { "1 + 2 + 10 + 1000.05", "1013.05" },
    // Operator precedence
    { "1 + 2 - 4 * 20 / 2", "-37" },
    // Parens
    { "(1)", "1" },
    // Parens affect precedence
    { "-(1 + (2 - 4)) * 20.8 / 2", "10.4" },
    // Unary addition is a sign, not a leading operator
    { "+2 - 1", "1" },
    // Set var in-string
    { "x = 1; 1 + x", "2" },
    // Multiple set vars
    { "x = 1; y = 2; 10 + x - y", "9" },

    // Unicode units - these currently fail
//    { wxT( "1um" ), "0.001" },      // GREEK SMALL LETTER MU
//    { wxT( "1µm" ), "0.001" },      // GREEK SMALL LETTER MU
//    { wxT( "1 µm" ), "0.001" },     // GREEK SMALL LETTER MU
//    { wxT( "1µm" ), "0.001" },      // MICRO SIGN
//    { wxT( "1 µm" ), "0.001" },     // MICRO SIGN
};


/**
 * Run through a set of test strings, clearing in between
 */
BOOST_AUTO_TEST_CASE( Results )
{
    for( const auto& c : eval_cases_valid )
    {
        BOOST_TEST_CONTEXT( c.input + " -> " + c.exp_result )
        {
            // Clear for new string input
            m_eval.Clear();

            m_eval.Process( c.input );

            // These are all valid
            BOOST_CHECK_EQUAL( m_eval.IsValid(), true );
            BOOST_CHECK_EQUAL( m_eval.Result(), c.exp_result );

            // Does original text still match?
            BOOST_CHECK_EQUAL( m_eval.OriginalText(), c.input );
        }
    }
}


/**
 * Test unicode parsing of the degree symbol
 */
BOOST_AUTO_TEST_CASE( UnicodeDegree )
{
    wxString degreeInput = wxT( "1\u00B0" );

    // Set to degrees and make ready for input
    m_eval.SetDefaultUnits( EDA_UNITS::DEGREES );
    m_eval.Clear();

    m_eval.Process( degreeInput );

    // These are all valid
    BOOST_CHECK_EQUAL( m_eval.IsValid(), true );

// Currently disabled since the parser doesn't parse the unicode correctly
    //BOOST_CHECK_EQUAL( m_eval.Result(), degreeInput );

    // Does original text still match?
    BOOST_CHECK_EQUAL( m_eval.OriginalText(), degreeInput );
}


struct EVAL_INVALID_CASE
{
    wxString input;
};

/**
 * A list of invalid test strings
 */
static const std::vector<EVAL_INVALID_CASE> eval_cases_invalid = {
    // Trailing operator
    { "1+" },
    // Leading operator
    { "*2 + 1" },
    // No operator
    { "1 2" },
    { "(1)(2)" },
    // Unknown operator
    { "1 $ 2" },
    // Mismatched parens
    { "(1 + 2" },
    { "1 + 2)" },
    // random text
    { "sdfsdf sdfsd" },
    // Div by 0
    { "1 / 0" },
    // Unknown vars are errors
    { "1 + unknown" },
    // Semicolons can't be empty or redundant
    { ";" },
    { ";1" },
    { ";1;" },
};

/**
 * Run through a set of invalid test strings, clearing in between
 */
BOOST_AUTO_TEST_CASE( ResultsInvalid )
{
    for( const auto& c : eval_cases_invalid )
    {
        BOOST_TEST_CONTEXT( c.input )
        {
            // Clear for new string input
            m_eval.Clear();

            m_eval.Process( c.input );

            // These are all valid
            BOOST_CHECK_EQUAL( m_eval.IsValid(), false );

            // Does original text still match?
            BOOST_CHECK_EQUAL( m_eval.OriginalText(), c.input );
        }
    }
}

BOOST_AUTO_TEST_SUITE_END()
