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
 * @file test_text_eval_numeric_compat.cpp
 * Test suite for text_eval system using examples adapted from numeric_evaluator tests
 */

#include <qa_utils/wx_utils/unit_test_utils.h>

// Code under test
#include <text_eval/text_eval_wrapper.h>
#include <wx/wxcrt.h>

// Make EDA_UNITS printable for Boost.Test
std::ostream& operator<<( std::ostream& aStream, EDA_UNITS aUnits )
{
    wxString unitStr = EDA_UNIT_UTILS::GetText( aUnits );
    return aStream << unitStr.ToStdString();
}

/**
 * Declare the test suite
 */
BOOST_AUTO_TEST_SUITE( TextEvalNumericCompat )

/**
 * Struct representing a test case adapted from numeric evaluator
 */
struct TEXT_EVAL_CASE
{
    wxString input;        // Input expression wrapped in @{} for text_eval
    wxString exp_result;   // Expected result as string
    bool shouldError;      // Whether this case should produce an error
};

/**
 * Basic functionality test
 */
BOOST_AUTO_TEST_CASE( Basic )
{
    EXPRESSION_EVALUATOR evaluator;

    wxString result = evaluator.Evaluate("@{1}");
    BOOST_CHECK_EQUAL( result, "1" );
    BOOST_CHECK( !evaluator.HasErrors() );
}

/**
 * Variable setting and usage test
 */
BOOST_AUTO_TEST_CASE( SetVar )
{
    EXPRESSION_EVALUATOR evaluator;

    // Set variable and test usage
    evaluator.SetVariable( "MoL", 42.0 );
    wxString result = evaluator.Evaluate( "@{1 + ${MoL}}" );
    BOOST_CHECK_EQUAL( result, "43" );
    BOOST_CHECK( !evaluator.HasErrors() );

    // Change variable value
    evaluator.SetVariable( "MoL", 422.0 );
    result = evaluator.Evaluate( "@{1 + ${MoL}}" );
    BOOST_CHECK_EQUAL( result, "423" );
    BOOST_CHECK( !evaluator.HasErrors() );

    // Add another variable
    evaluator.SetVariable( "pi", 3.14 );
    BOOST_CHECK( evaluator.HasVariable( "pi" ) );

    // Remove one variable
    BOOST_CHECK( evaluator.RemoveVariable( "pi" ) );
    BOOST_CHECK( !evaluator.HasVariable( "pi" ) );

    // Other variable should still be there
    BOOST_CHECK( evaluator.HasVariable( "MoL" ) );

    // Add another variable back
    evaluator.SetVariable( "piish", 3.1 );

    // Test multiple variables
    result = evaluator.Evaluate( "@{1 + ${MoL} + ${piish}}" );
    BOOST_CHECK_EQUAL( result, "426.1" );
    BOOST_CHECK( !evaluator.HasErrors() );

    // Clear all variables
    evaluator.ClearVariables();
    BOOST_CHECK( !evaluator.HasVariable( "MoL" ) );
    BOOST_CHECK( !evaluator.HasVariable( "piish" ) );
}

/**
 * A list of valid test cases adapted from numeric evaluator
 * All expressions are wrapped in @{} to use the text_eval system
 */
static const std::vector<TEXT_EVAL_CASE> eval_cases_valid = {
    // Empty case - text_eval handles this differently than numeric evaluator
    { "@{}", "@{}", true },  // Empty expressions should error in text_eval

    // Trivial eval
    { "@{1}", "1", false },

    // Decimal separators (text_eval may handle differently)
    { "@{1.5}", "1.5", false },
    // Note: comma as decimal separator might not work in text_eval

    // Simple arithmetic
    { "@{1+2}", "3", false },
    { "@{1 + 2}", "3", false },
    { "@{1.5 + 0.2 + 0.1}", "1.8", false },
    { "@{3 - 10}", "-7", false },
    { "@{1 + 2 + 10 + 1000.05}", "1013.05", false },

    // Operator precedence
    { "@{1 + 2 - 4 * 20 / 2}", "-37", false },

    // Parentheses
    { "@{(1)}", "1", false },
    { "@{-(1 + (2 - 4)) * 20.8 / 2}", "10.4", false },

    // Unary operators
    { "@{+2 - 1}", "1", false },
};

/**
 * A list of invalid test cases adapted from numeric evaluator
 */
static const std::vector<TEXT_EVAL_CASE> eval_cases_invalid = {
    // Trailing operator
    { "@{1+}", "", true },

    // Leading operator (except unary)
    { "@{*2 + 1}", "", true },

    // Division by zero
    { "@{1 / 0}", "", true },

    // Unknown variables should preserve the original expression
    { "@{1 + ${unknown}}", "@{1 + ${unknown}}", true },

    // Mismatched parentheses
    { "@{(1 + 2}", "", true },
    { "@{1 + 2)}", "", true },

    // Invalid syntax
    { "@{1 $ 2}", "", true },
};

/**
 * Run through valid test cases
 */
BOOST_AUTO_TEST_CASE( ValidResults )
{
    EXPRESSION_EVALUATOR evaluator;

    for( const auto& testCase : eval_cases_valid )
    {
        BOOST_TEST_CONTEXT( testCase.input + " -> " + testCase.exp_result )
        {
            wxString result = evaluator.Evaluate( testCase.input );

            if( testCase.shouldError )
            {
                BOOST_CHECK( evaluator.HasErrors() );
            }
            else
            {
                BOOST_CHECK( !evaluator.HasErrors() );
                BOOST_CHECK_EQUAL( result, testCase.exp_result );
            }
        }
    }
}

/**
 * Run through invalid test cases
 */
BOOST_AUTO_TEST_CASE( InvalidResults )
{
    EXPRESSION_EVALUATOR evaluator;

    for( const auto& testCase : eval_cases_invalid )
    {
        BOOST_TEST_CONTEXT( testCase.input )
        {
            wxString result = evaluator.Evaluate( testCase.input );

            // All these cases should produce errors
            BOOST_CHECK( evaluator.HasErrors() );

            // For undefined variables, result should be the original expression
            if( testCase.input.Contains( "${unknown}" ) )
            {
                BOOST_CHECK_EQUAL( result, testCase.input );
            }
        }
    }
}

/**
 * Test variable usage with more complex expressions
 */
BOOST_AUTO_TEST_CASE( VariableExpressions )
{
    EXPRESSION_EVALUATOR evaluator;

    // Set up variables similar to numeric evaluator tests
    evaluator.SetVariable( "x", 10.0 );
    evaluator.SetVariable( "y", 5.0 );

    struct VarTestCase {
        wxString input;
        wxString expected;
        bool shouldError;
    };

    const std::vector<VarTestCase> varCases = {
        { "@{${x}}", "10", false },
        { "@{${y}}", "5", false },
        { "@{${x} + ${y}}", "15", false },
        { "@{${x} * ${y}}", "50", false },
        { "@{${x} - ${y}}", "5", false },
        { "@{${x} / ${y}}", "2", false },
        { "@{(${x} + ${y}) * 2}", "30", false },

        // Undefined variable should preserve expression
        { "@{${undefined}}", "@{${undefined}}", true },

        // Mixed defined and undefined
        { "@{${x} + ${undefined}}", "@{${x} + ${undefined}}", true },
    };

    for( const auto& testCase : varCases )
    {
        BOOST_TEST_CONTEXT( testCase.input + " -> " + testCase.expected )
        {
            wxString result = evaluator.Evaluate( testCase.input );

            if( testCase.shouldError )
            {
                BOOST_CHECK( evaluator.HasErrors() );
                BOOST_CHECK_EQUAL( result, testCase.input );  // Original expression preserved
            }
            else
            {
                BOOST_CHECK( !evaluator.HasErrors() );
                BOOST_CHECK_EQUAL( result, testCase.expected );
            }
        }
    }
}

/**
 * Test mathematical functions available in text_eval
 */
BOOST_AUTO_TEST_CASE( MathFunctions )
{
    EXPRESSION_EVALUATOR evaluator;

    struct MathTestCase {
        wxString input;
        wxString expected;
        bool shouldError;
    };

    const std::vector<MathTestCase> mathCases = {
        // Basic math functions that are confirmed to work
        { "@{abs(-5)}", "5", false },
        { "@{min(3, 7)}", "3", false },
        { "@{max(3, 7)}", "7", false },
        { "@{sqrt(16)}", "4", false },
        { "@{ceil(3.2)}", "4", false },
        { "@{floor(3.8)}", "3", false },
        { "@{round(3.6)}", "4", false },
        { "@{pow(2, 3)}", "8", false },

        // Sum and average functions
        { "@{sum(1, 2, 3)}", "6", false },
        { "@{avg(2, 4, 6)}", "4", false },
    };

    for( const auto& testCase : mathCases )
    {
        BOOST_TEST_CONTEXT( testCase.input + " -> " + testCase.expected )
        {
            wxString result = evaluator.Evaluate( testCase.input );

            if( testCase.shouldError )
            {
                BOOST_CHECK( evaluator.HasErrors() );
            }
            else
            {
                BOOST_CHECK( !evaluator.HasErrors() );
                BOOST_CHECK_EQUAL( result, testCase.expected );
            }
        }
    }
}

/**
 * Test unit support functionality
 */
BOOST_AUTO_TEST_CASE( UnitSupport )
{
    // Test basic unit constructor
    EXPRESSION_EVALUATOR evaluator_mm( EDA_UNITS::MM );
    EXPRESSION_EVALUATOR evaluator_inch( EDA_UNITS::INCH );
    EXPRESSION_EVALUATOR evaluator_mil( EDA_UNITS::MILS );

    // Test unit setting and getting
    BOOST_CHECK_EQUAL( evaluator_mm.GetDefaultUnits(), EDA_UNITS::MM );
    BOOST_CHECK_EQUAL( evaluator_inch.GetDefaultUnits(), EDA_UNITS::INCH );
    BOOST_CHECK_EQUAL( evaluator_mil.GetDefaultUnits(), EDA_UNITS::MILS );

    // Test unit change
    evaluator_mm.SetDefaultUnits( EDA_UNITS::INCH );
    BOOST_CHECK_EQUAL( evaluator_mm.GetDefaultUnits(), EDA_UNITS::INCH );

    // Test basic expressions work with unit-aware evaluator
    wxString result = evaluator_mm.Evaluate( "@{1 + 2}" );
    BOOST_CHECK_EQUAL( result, "3" );
    BOOST_CHECK( !evaluator_mm.HasErrors() );

    // Test unit constructor with variable callback
    auto callback = [](const std::string& varName) -> calc_parser::Result<calc_parser::Value> {
        if (varName == "width") {
            return calc_parser::MakeValue<calc_parser::Value>(10.0);
        }
        return calc_parser::MakeError<calc_parser::Value>("Variable not found: " + varName);
    };

    EXPRESSION_EVALUATOR evaluator_callback( EDA_UNITS::MM, callback, false );
    BOOST_CHECK_EQUAL( evaluator_callback.GetDefaultUnits(), EDA_UNITS::MM );
    BOOST_CHECK( evaluator_callback.HasVariableCallback() );

    result = evaluator_callback.Evaluate( "@{${width} * 2}" );
    BOOST_CHECK_EQUAL( result, "20" );
    BOOST_CHECK( !evaluator_callback.HasErrors() );
}

/**
 * Test unit conversion infrastructure readiness
 * Tests the unit support foundation without exposing internal functions
 */
BOOST_AUTO_TEST_CASE( UnitInfrastructureReadiness )
{
    // Test that different unit types can be set and retrieved
    EXPRESSION_EVALUATOR evaluator_mm( EDA_UNITS::MM );
    EXPRESSION_EVALUATOR evaluator_inch( EDA_UNITS::INCH );
    EXPRESSION_EVALUATOR evaluator_mil( EDA_UNITS::MILS );
    EXPRESSION_EVALUATOR evaluator_cm( EDA_UNITS::CM );
    EXPRESSION_EVALUATOR evaluator_um( EDA_UNITS::UM );

    // Verify unit storage works for all supported units
    BOOST_CHECK_EQUAL( evaluator_mm.GetDefaultUnits(), EDA_UNITS::MM );
    BOOST_CHECK_EQUAL( evaluator_inch.GetDefaultUnits(), EDA_UNITS::INCH );
    BOOST_CHECK_EQUAL( evaluator_mil.GetDefaultUnits(), EDA_UNITS::MILS );
    BOOST_CHECK_EQUAL( evaluator_cm.GetDefaultUnits(), EDA_UNITS::CM );
    BOOST_CHECK_EQUAL( evaluator_um.GetDefaultUnits(), EDA_UNITS::UM );

    // Test unit changes
    evaluator_mm.SetDefaultUnits( EDA_UNITS::INCH );
    BOOST_CHECK_EQUAL( evaluator_mm.GetDefaultUnits(), EDA_UNITS::INCH );

    evaluator_inch.SetDefaultUnits( EDA_UNITS::MILS );
    BOOST_CHECK_EQUAL( evaluator_inch.GetDefaultUnits(), EDA_UNITS::MILS );

    // Verify expressions still work with all unit types
    wxString result;

    result = evaluator_mm.Evaluate( "@{5 * 2}" );
    BOOST_CHECK_EQUAL( result, "10" );

    result = evaluator_inch.Evaluate( "@{3.5 + 1.5}" );
    BOOST_CHECK_EQUAL( result, "5" );

    result = evaluator_mil.Evaluate( "@{100 / 4}" );
    BOOST_CHECK_EQUAL( result, "25" );

    // Test complex expressions work with unit-aware evaluators
    result = evaluator_cm.Evaluate( "@{(10 + 5) * 2 - 1}" );
    BOOST_CHECK_EQUAL( result, "29" );

    // Test variable support with units
    evaluator_um.SetVariable( "length", 25.4 );
    result = evaluator_um.Evaluate( "@{${length} * 2}" );
    BOOST_CHECK_EQUAL( result, "50.8" );

    // Test that unit-aware evaluator preserves its unit setting across operations
    EXPRESSION_EVALUATOR persistent_eval( EDA_UNITS::MILS );
    BOOST_CHECK_EQUAL( persistent_eval.GetDefaultUnits(), EDA_UNITS::MILS );

    persistent_eval.Evaluate( "@{1 + 1}" );
    BOOST_CHECK_EQUAL( persistent_eval.GetDefaultUnits(), EDA_UNITS::MILS );

    persistent_eval.SetVariable( "test", 42 );
    BOOST_CHECK_EQUAL( persistent_eval.GetDefaultUnits(), EDA_UNITS::MILS );

    persistent_eval.Evaluate( "@{${test} + 8}" );
    BOOST_CHECK_EQUAL( persistent_eval.GetDefaultUnits(), EDA_UNITS::MILS );
    BOOST_CHECK_EQUAL( persistent_eval.Evaluate( "@{${test} + 8}" ), "50" );
}

/**
 * Test mixed unit arithmetic expectations using known conversion factors
 * Documents expected behavior for when unit parsing is integrated
 */
BOOST_AUTO_TEST_CASE( UnitMixingExpectations )
{
    EXPRESSION_EVALUATOR evaluator_mm( EDA_UNITS::MM );

    // Verify basic functionality works before discussing unit mixing
    BOOST_CHECK_EQUAL( evaluator_mm.GetDefaultUnits(), EDA_UNITS::MM );

    wxString result = evaluator_mm.Evaluate( "@{2 + 3}" );
    BOOST_CHECK_EQUAL( result, "5" );

    // Test complex arithmetic
    result = evaluator_mm.Evaluate( "@{(1 + 2) * 3}" );
    BOOST_CHECK_EQUAL( result, "9" );

    // Test with decimals (important for unit conversions)
    result = evaluator_mm.Evaluate( "@{25.4 + 12.7}" );
    // Use close comparison for floating point
    double numeric_result = wxAtof( result );
    BOOST_CHECK_CLOSE( numeric_result, 38.1, 0.01 );

    // Test with variables that could represent converted values
    evaluator_mm.SetVariable( "inch_in_mm", 25.4 );  // 1 inch = 25.4 mm
    evaluator_mm.SetVariable( "mil_in_mm", 0.0254 ); // 1 mil = 0.0254 mm

    result = evaluator_mm.Evaluate( "@{${inch_in_mm} + ${mil_in_mm}}" );
    BOOST_CHECK_EQUAL( result, "25.4254" );

    // Simulate what "1mm + 1in" should become when units are parsed
    evaluator_mm.SetVariable( "mm_part", 1.0 );
    evaluator_mm.SetVariable( "in_part", 25.4 );  // 1in converted to mm
    result = evaluator_mm.Evaluate( "@{${mm_part} + ${in_part}}" );
    BOOST_CHECK_EQUAL( result, "26.4" );

    // Simulate what "1in + 1000mil" should become
    evaluator_mm.SetVariable( "one_inch", 25.4 );
    evaluator_mm.SetVariable( "thousand_mils", 25.4 ); // 1000 mils = 1 inch = 25.4 mm
    result = evaluator_mm.Evaluate( "@{${one_inch} + ${thousand_mils}}" );
    BOOST_CHECK_EQUAL( result, "50.8" );

    // Test expressions that will be possible once unit parsing is integrated:
    // These would parse "1mm", "1in", "1mil" etc. and convert to default units


    // Basic unit expressions
    BOOST_CHECK_EQUAL( evaluator_mm.Evaluate( "@{1mm}" ), "1" );
    BOOST_CHECK_EQUAL( evaluator_mm.Evaluate( "@{1in}" ), "25.4" );
    BOOST_CHECK_EQUAL( evaluator_mm.Evaluate( "@{1000mil}" ), "25.4" );

    // Mixed unit arithmetic
    BOOST_CHECK_EQUAL( evaluator_mm.Evaluate( "@{1mm + 1in}" ), "26.4" );
    BOOST_CHECK_EQUAL( evaluator_mm.Evaluate( "@{1in + 1000mil}" ), "50.8" );
    BOOST_CHECK_EQUAL( evaluator_mm.Evaluate( "@{10mm + 0.5in + 500mil}" ), "35.4" );

    // Unit expressions with whitespace
    BOOST_CHECK_EQUAL( evaluator_mm.Evaluate( "@{1 mm}" ), "1" );
    BOOST_CHECK_EQUAL( evaluator_mm.Evaluate( "@{1 in}" ), "25.4" );

    // Complex mixed unit expressions with variables
    evaluator_mm.SetVariable( "width", 10 ); // 10mm
    BOOST_CHECK_EQUAL( evaluator_mm.Evaluate( "@{${width}mm + 1in}" ), "35.4" );
    // These two should both work the same
    BOOST_CHECK_EQUAL( evaluator_mm.Evaluate( "@{${width} * 1mm + 1in}" ), "35.4" );

    // Different evaluator units should convert appropriately
    EXPRESSION_EVALUATOR evaluator_inch( EDA_UNITS::INCH );
    BOOST_CHECK_EQUAL( evaluator_inch.Evaluate( "@{1in}" ), "1" );
    BOOST_CHECK_EQUAL( evaluator_inch.Evaluate( "@{25.4mm}" ), "1" );
}

/**
 * Test actual unit parsing integration (now that unit parsing is implemented)
 */
BOOST_AUTO_TEST_CASE( ActualUnitParsing )
{
    // Test MM evaluator with unit expressions
    EXPRESSION_EVALUATOR evaluator_mm( EDA_UNITS::MM );
    BOOST_CHECK_EQUAL( evaluator_mm.GetDefaultUnits(), EDA_UNITS::MM );

    // Test basic unit expressions (these should now work!)
    wxString result;

    // Debug: Test basic arithmetic first
    result = evaluator_mm.Evaluate( "@{2+3}" );
    BOOST_CHECK_EQUAL( result, "5" );

    // Debug: Test just number
    result = evaluator_mm.Evaluate( "@{1}" );
    BOOST_CHECK_EQUAL( result, "1" );

    // Debug: Test the working case
    result = evaluator_mm.Evaluate( "@{1in}" );
    if (result != "25.4") {
        std::cout << "DEBUG: @{1in} returned '" << result.ToStdString() << "'" << std::endl;
        if (evaluator_mm.HasErrors()) {
            std::cout << "DEBUG: @{1in} Errors: " << evaluator_mm.GetErrorSummary().ToStdString() << std::endl;
        }
    }
    BOOST_CHECK_EQUAL( result, "25.4" );

    result = evaluator_mm.Evaluate( "@{1mil}" );
    BOOST_CHECK_EQUAL( result, "0.0254" );

    result = evaluator_mm.Evaluate( "@{1mm}" );
    BOOST_CHECK_EQUAL( result, "1" );

    // 1 inch should convert to 25.4 mm
    result = evaluator_mm.Evaluate( "@{1in}" );
    BOOST_CHECK_EQUAL( result, "25.4" );

    // 1000 mils should convert to 25.4 mm (1000 mils = 1 inch)
    result = evaluator_mm.Evaluate( "@{1000mil}" );
    BOOST_CHECK_EQUAL( result, "25.4" );

    // Test mixed unit arithmetic
    result = evaluator_mm.Evaluate( "@{1mm + 1in}" );
    BOOST_CHECK_EQUAL( result, "26.4" );

    result = evaluator_mm.Evaluate( "@{1in + 1000mil}" );
    BOOST_CHECK_EQUAL( result, "50.8" );

    // Test more complex expressions
    result = evaluator_mm.Evaluate( "@{10mm + 0.5in + 500mil}" );
    BOOST_CHECK_EQUAL( result, "35.4" );

    // Test unit expressions with spaces (if supported)
    result = evaluator_mm.Evaluate( "@{1 mm}" );
    BOOST_CHECK_EQUAL( result, "1" );

    // Test with different default units
    EXPRESSION_EVALUATOR evaluator_inch( EDA_UNITS::INCH );

    // 1 inch should be 1 when default unit is inches
    result = evaluator_inch.Evaluate( "@{1in}" );
    BOOST_CHECK_EQUAL( result, "1" );

    // 25.4mm should convert to 1 inch (with floating point tolerance)
    result = evaluator_inch.Evaluate( "@{25.4mm}" );
    // Use approximate comparison for floating point
    double result_val = wxAtof(result);
    BOOST_CHECK( std::abs(result_val - 1.0) < 0.001 );

    // Test arithmetic with inch evaluator
    result = evaluator_inch.Evaluate( "@{1in + 1000mil}" );
    BOOST_CHECK_EQUAL( result, "2" );  // 1 inch + 1 inch = 2 inches

    // Test centimeters
    result = evaluator_mm.Evaluate( "@{1cm}" );
    BOOST_CHECK_EQUAL( result, "10" );  // 1 cm = 10 mm

    // Test micrometers
    result = evaluator_mm.Evaluate( "@{1000um}" );
    BOOST_CHECK_EQUAL( result, "1" );  // 1000 um = 1 mm

    // Test quotes for inches
    result = evaluator_mm.Evaluate( "@{1\"}" );
    BOOST_CHECK_EQUAL( result, "25.4" );  // 1" = 25.4 mm

    // Test complex mixed expressions with parentheses
    result = evaluator_mm.Evaluate( "@{(1in + 500mil) * 2}" );
    // Expected: (25.4 + 12.7) * 2 = 38.1 * 2 = 76.2mm
    double result_val2 = wxAtof(result);
    BOOST_CHECK( std::abs(result_val2 - 76.2) < 0.001 );
}

/**
 * Test unit parsing edge cases and error handling
 */
BOOST_AUTO_TEST_CASE( UnitParsingEdgeCases, * boost::unit_test::enabled() )
{
    EXPRESSION_EVALUATOR evaluator_mm( EDA_UNITS::MM );

    // Test invalid units (should be treated as plain numbers)
    wxString result = evaluator_mm.Evaluate( "@{1xyz}" );
    // Should parse as "1" followed by identifier "xyz", might be an error or treat as 1
    // This behavior depends on implementation details

    // Test numbers without units (should work normally)
    result = evaluator_mm.Evaluate( "@{25.4}" );
    BOOST_CHECK_EQUAL( result, "25.4" );

    // Test zero with units
    result = evaluator_mm.Evaluate( "@{0mm}" );
    BOOST_CHECK_EQUAL( result, "0" );

    result = evaluator_mm.Evaluate( "@{0in}" );
    BOOST_CHECK_EQUAL( result, "0" );

    // Test decimal values with units
    result = evaluator_mm.Evaluate( "@{2.54cm}" );
    BOOST_CHECK_EQUAL( result, "25.4" );  // 2.54 cm = 25.4 mm

    result = evaluator_mm.Evaluate( "@{0.5in}" );
    BOOST_CHECK_EQUAL( result, "12.7" );  // 0.5 inch = 12.7 mm

    // Test very small values
    result = evaluator_mm.Evaluate( "@{1um}" );
    BOOST_CHECK_EQUAL( result, "0.001" );  // 1 um = 0.001 mm
}

BOOST_AUTO_TEST_CASE( NumericEvaluatorCompatibility )
{
    // Test the NUMERIC_EVALUATOR_COMPAT wrapper class that provides
    // a drop-in replacement for NUMERIC_EVALUATOR using EXPRESSION_EVALUATOR backend

    NUMERIC_EVALUATOR_COMPAT eval( EDA_UNITS::MM );

    // Test basic arithmetic
    BOOST_CHECK( eval.Process( "1 + 2" ) );
    BOOST_CHECK( eval.IsValid() );
    BOOST_CHECK_EQUAL( eval.Result(), "3" );
    BOOST_CHECK_EQUAL( eval.OriginalText(), "1 + 2" );

    // Test variables
    eval.SetVar( "x", 5.0 );
    eval.SetVar( "y", 3.0 );

    BOOST_CHECK( eval.Process( "x + y" ) );
    BOOST_CHECK( eval.IsValid() );
    BOOST_CHECK_EQUAL( eval.Result(), "8" );

    // Test GetVar
    BOOST_CHECK_CLOSE( eval.GetVar( "x" ), 5.0, 0.001 );
    BOOST_CHECK_CLOSE( eval.GetVar( "y" ), 3.0, 0.001 );
    BOOST_CHECK_CLOSE( eval.GetVar( "undefined" ), 0.0, 0.001 );

    // Test units (should work seamlessly)
    BOOST_CHECK( eval.Process( "1in + 1mm" ) );
    BOOST_CHECK( eval.IsValid() );
    BOOST_CHECK_EQUAL( eval.Result(), "26.4" );  // 1 inch + 1mm in mm

    // Test mathematical functions
    BOOST_CHECK( eval.Process( "sqrt(16)" ) );
    BOOST_CHECK( eval.IsValid() );
    BOOST_CHECK_EQUAL( eval.Result(), "4" );

    // Test invalid expression - use something clearly invalid
    BOOST_CHECK( !eval.Process( "1 + * 2" ) );  // Clearly invalid: two operators in a row
    BOOST_CHECK( !eval.IsValid() );

    // Test Clear() - should reset state but keep variables
    eval.Clear();
    BOOST_CHECK_CLOSE( eval.GetVar( "x" ), 5.0, 0.001 );  // Variables should still be there

    BOOST_CHECK( eval.Process( "x * 2" ) );
    BOOST_CHECK( eval.IsValid() );
    BOOST_CHECK_EQUAL( eval.Result(), "10" );

    // Test variable removal
    eval.RemoveVar( "x" );
    BOOST_CHECK_CLOSE( eval.GetVar( "x" ), 0.0, 0.001 );  // Should be 0.0 for undefined
    BOOST_CHECK_CLOSE( eval.GetVar( "y" ), 3.0, 0.001 );  // y should still exist

    // Test ClearVar()
    eval.ClearVar();
    BOOST_CHECK_CLOSE( eval.GetVar( "y" ), 0.0, 0.001 );  // All variables should be gone

    // Test that we can still use the evaluator after clearing
    BOOST_CHECK( eval.Process( "42" ) );
    BOOST_CHECK( eval.IsValid() );
    BOOST_CHECK_EQUAL( eval.Result(), "42" );

    // Test LocaleChanged() - should be a no-op but not crash
    eval.LocaleChanged();
    BOOST_CHECK( eval.Process( "3.14" ) );
    BOOST_CHECK( eval.IsValid() );
    BOOST_CHECK_EQUAL( eval.Result(), "3.14" );
}

/**
 * Test unit parsing priority - mil/mm/um should be recognized before SI prefixes
 * This regression test verifies the fix for a bug where "40mil" was incorrectly
 * parsed as "40m" (milli) + "il" = 0.04 instead of 40 thousandths of an inch = 1.016mm
 */
BOOST_AUTO_TEST_CASE( UnitParsingPriority )
{
    EXPRESSION_EVALUATOR evaluator_mm( EDA_UNITS::MM );
    wxString result;

    // Test "mil" unit (thousandths of an inch)
    // 40mil should convert to 1.016mm (40 * 0.0254)
    result = evaluator_mm.Evaluate( "@{40mil}" );
    BOOST_CHECK_CLOSE( wxAtof( result ), 1.016, 0.01 );  // Allow 0.01% tolerance

    // Test "mm" unit (should not be affected by 'm' SI prefix)
    result = evaluator_mm.Evaluate( "@{10mm}" );
    BOOST_CHECK_CLOSE( wxAtof( result ), 10.0, 0.01 );

    // Test "um" unit (micrometers, should not be parsed as 'u' prefix + 'm')
    // 1000um should equal 1mm
    result = evaluator_mm.Evaluate( "@{1000um}" );
    BOOST_CHECK_CLOSE( wxAtof( result ), 1.0, 0.01 );

    // Test combined expression with multiple unit types
    // 1mm + 40mil should equal 2.016mm
    result = evaluator_mm.Evaluate( "@{1mm + 40mil}" );
    BOOST_CHECK_CLOSE( wxAtof( result ), 2.016, 0.01 );

    // Test that SI prefixes still work when not part of a unit
    // 1k should equal 1000
    result = evaluator_mm.Evaluate( "@{1k}" );
    BOOST_CHECK_CLOSE( wxAtof( result ), 1000.0, 0.01 );

    // Test that 'm' SI prefix works when followed by non-unit characters
    // 40m should equal 0.04 (40 milli)
    result = evaluator_mm.Evaluate( "@{40m}" );
    BOOST_CHECK_CLOSE( wxAtof( result ), 0.04, 0.01 );

    // Test "thou" unit (alternative name for mil)
    // 100thou should equal 2.54mm
    result = evaluator_mm.Evaluate( "@{100thou}" );
    BOOST_CHECK_CLOSE( wxAtof( result ), 2.54, 0.01 );
}

BOOST_AUTO_TEST_SUITE_END()
