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
 * Test suite for text_eval_parser routines
 */

#include <qa_utils/wx_utils/unit_test_utils.h>

// Code under test
#include <text_eval/text_eval_wrapper.h>

#include <chrono>
#include <cmath>
#include <regex>
#include <wx/wxcrt.h>

/**
 * Declare the test suite
 */
BOOST_AUTO_TEST_SUITE( TextEvalParser )

/**
 * Test basic arithmetic operations
 */
BOOST_AUTO_TEST_CASE( BasicArithmetic )
{
    EXPRESSION_EVALUATOR evaluator;

    struct TestCase {
        std::string expression;
        std::string expected;
        bool shouldError;
    };

    const std::vector<TestCase> cases = {
        // Basic operations
        { "Text @{2 + 3} more text", "Text 5 more text", false },
        { "@{10 - 4}", "6", false },
        { "@{7 * 8}", "56", false },
        { "@{15 / 3}", "5", false },
        { "@{17 % 5}", "2", false },
        { "@{2^3}", "8", false },

        // Order of operations
        { "@{2 + 3 * 4}", "14", false },
        { "@{(2 + 3) * 4}", "20", false },
        { "@{2^3^2}", "512", false },  // Right associative
        { "@{-5}", "-5", false },
        { "@{+5}", "5", false },

        // Floating point
        { "@{3.14 + 1.86}", "5", false },
        { "@{10.5 / 2}", "5.25", false },
        { "@{3.5 * 2}", "7", false },

        // Edge cases
        { "@{1 / 0}", "Text @{1 / 0} more text", true },  // Division by zero
        { "@{1 % 0}", "Text @{1 % 0} more text", true },  // Modulo by zero

        // Multiple calculations in one string
        { "@{2 + 2} and @{3 * 3}", "4 and 9", false },
    };

    for( const auto& testCase : cases )
    {
        auto result = evaluator.Evaluate( testCase.expression );

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

/**
 * Test variable substitution
 */
BOOST_AUTO_TEST_CASE( VariableSubstitution )
{
    EXPRESSION_EVALUATOR evaluator;

    // Set up some variables
    evaluator.SetVariable( "x", 10.0 );
    evaluator.SetVariable( "y", 5.0 );
    evaluator.SetVariable( wxString("name"), wxString("KiCad") );
    evaluator.SetVariable( "version", 8.0 );

    struct TestCase {
        std::string expression;
        std::string expected;
        bool shouldError;
    };

    const std::vector<TestCase> cases = {
        // Basic variable substitution
        { "@{${x}}", "10", false },
        { "@{${y}}", "5", false },
        { "Hello ${name}!", "Hello KiCad!", false },

        // Variables in calculations
        { "@{${x} + ${y}}", "15", false },
        { "@{${x} * ${y}}", "50", false },
        { "@{${x} - ${y}}", "5", false },
        { "@{${x} / ${y}}", "2", false },

        // Mixed text and variable calculations
        { "Product: @{${x} * ${y}} units", "Product: 50 units", false },
        { "Version ${version}.0", "Version 8.0", false },

        // Undefined variables
        { "@{${undefined}}", "@{${undefined}}", true },

        // String variables
        { "Welcome to ${name}", "Welcome to KiCad", false },
    };

    for( const auto& testCase : cases )
    {
        auto result = evaluator.Evaluate( testCase.expression );

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

/**
 * Test string operations and concatenation
 */
BOOST_AUTO_TEST_CASE( StringOperations )
{
    EXPRESSION_EVALUATOR evaluator;
    evaluator.SetVariable( wxString("prefix"), wxString("Hello") );
    evaluator.SetVariable( wxString("suffix"), wxString("World") );

    struct TestCase {
        std::string expression;
        std::string expected;
        bool shouldError;
    };

    const std::vector<TestCase> cases = {
        // String concatenation with +
        { "@{\"Hello\" + \" \" + \"World\"}", "Hello World", false },
        { "@{${prefix} + \" \" + ${suffix}}", "Hello World", false },

        // Mixed string and number concatenation
        { "@{\"Count: \" + 42}", "Count: 42", false },
        { "@{42 + \" items\"}", "42 items", false },

        // String literals
        { "@{\"Simple string\"}", "Simple string", false },
        { "Prefix @{\"middle\"} suffix", "Prefix middle suffix", false },
    };

    for( const auto& testCase : cases )
    {
        auto result = evaluator.Evaluate( testCase.expression );

        BOOST_CHECK( !evaluator.HasErrors() );
        BOOST_CHECK_EQUAL( result, testCase.expected );
    }
}

/**
 * Test mathematical functions
 */
BOOST_AUTO_TEST_CASE( MathematicalFunctions )
{
    EXPRESSION_EVALUATOR evaluator;

    struct TestCase {
        std::string expression;
        std::string expected;
        bool shouldError;
        double tolerance;
    };

    const std::vector<TestCase> cases = {
        // Basic math functions
        { "@{abs(-5)}", "5", false, 0.001 },
        { "@{abs(3.14)}", "3.14", false, 0.001 },
        { "@{sqrt(16)}", "4", false, 0.001 },
        { "@{sqrt(2)}", "1.414", false, 0.01 },
        { "@{pow(2, 3)}", "8", false, 0.001 },
        { "@{pow(3, 2)}", "9", false, 0.001 },

        // Rounding functions
        { "@{floor(3.7)}", "3", false, 0.001 },
        { "@{ceil(3.2)}", "4", false, 0.001 },
        { "@{round(3.7)}", "4", false, 0.001 },
        { "@{round(3.2)}", "3", false, 0.001 },
        { "@{round(3.14159, 2)}", "3.14", false, 0.001 },

        // Min/Max functions
        { "@{min(5, 3, 8, 1)}", "1", false, 0.001 },
        { "@{max(5, 3, 8, 1)}", "8", false, 0.001 },
        { "@{min(3.5, 3.1)}", "3.1", false, 0.001 },

        // Sum and average
        { "@{sum(1, 2, 3, 4)}", "10", false, 0.001 },
        { "@{avg(2, 4, 6)}", "4", false, 0.001 },

        // Error cases
        { "@{sqrt(-1)}", "Text @{sqrt(-1)} more text", true, 0 },
    };

    for( const auto& testCase : cases )
    {
        auto result = evaluator.Evaluate( testCase.expression );

        if( testCase.shouldError )
        {
            BOOST_CHECK( evaluator.HasErrors() );
        }
        else
        {
            BOOST_CHECK( !evaluator.HasErrors() );

            if( testCase.tolerance > 0 )
            {
                // For floating point comparisons
                double actualValue = wxStrtod( result, nullptr );
                double expectedValue = wxStrtod( testCase.expected, nullptr );
                BOOST_CHECK_CLOSE( actualValue, expectedValue, testCase.tolerance * 100 );
            }
            else
            {
                BOOST_CHECK_EQUAL( result, testCase.expected );
            }
        }
    }
}

/**
 * Test string manipulation functions
 */
BOOST_AUTO_TEST_CASE( StringFunctions )
{
    EXPRESSION_EVALUATOR evaluator;
    evaluator.SetVariable( wxString("text"), wxString("Hello World") );

    struct TestCase {
        std::string expression;
        std::string expected;
        bool shouldError;
    };

    const std::vector<TestCase> cases = {
        // Case conversion
        { "@{upper(\"hello world\")}", "HELLO WORLD", false },
        { "@{lower(\"HELLO WORLD\")}", "hello world", false },
        { "@{upper(${text})}", "HELLO WORLD", false },

        // String concatenation function
        { "@{concat(\"Hello\", \" \", \"World\")}", "Hello World", false },
        { "@{concat(\"Count: \", 42, \" items\")}", "Count: 42 items", false },

        { "@{beforefirst(\"hello.world.txt\", \".\")}", "hello", false },
        { "@{beforelast(\"hello.world.txt\", \".\")}", "hello.world", false },
        { "@{afterfirst(\"hello.world.txt\", \".\")}", "world.txt", false },
        { "@{afterlast(\"hello.world.txt\", \".\")}", "txt", false },
        { "@{beforefirst(${text}, \" \")}", "Hello", false },
    };

    for( const auto& testCase : cases )
    {
        auto result = evaluator.Evaluate( testCase.expression );

        BOOST_CHECK( !evaluator.HasErrors() );
        BOOST_CHECK_EQUAL( result, testCase.expected );
    }
}

/**
 * Test formatting functions
 */
BOOST_AUTO_TEST_CASE( FormattingFunctions )
{
    EXPRESSION_EVALUATOR evaluator;

    struct TestCase {
        std::string expression;
        std::string expected;
        bool shouldError;
    };

    const std::vector<TestCase> cases = {
        // Number formatting
        { "@{format(3.14159)}", "3.14", false },
        { "@{format(3.14159, 3)}", "3.142", false },
        { "@{format(1234.5)}", "1234.50", false },
        { "@{fixed(3.14159, 2)}", "3.14", false },

        // Currency formatting
        { "@{currency(1234.56)}", "$1234.56", false },
        { "@{currency(999.99, \"€\")}", "€999.99", false },
    };

    for( const auto& testCase : cases )
    {
        auto result = evaluator.Evaluate( testCase.expression );

        BOOST_CHECK( !evaluator.HasErrors() );
        BOOST_CHECK_EQUAL( result, testCase.expected );
    }
}

/**
 * Test date and time functions
 */
BOOST_AUTO_TEST_CASE( DateTimeFunctions )
{
    EXPRESSION_EVALUATOR evaluator;

    // Note: These tests will be time-sensitive. We test the functions exist
    // and return reasonable values rather than exact matches.

    struct TestCase {
        std::string expression;
        bool shouldContainNumbers;
        bool shouldError;
    };

    const std::vector<TestCase> cases = {
        // Date functions that return numbers (days since epoch)
        { "@{today()}", true, false },
        { "@{now()}", true, false },  // Returns timestamp

        // Date formatting (these return specific dates so we can test exactly)
        { "@{dateformat(0)}", false, false },  // Should format epoch date
        { "@{dateformat(0, \"ISO\")}", false, false },
        { "@{dateformat(0, \"US\")}", false, false },
        { "@{weekdayname(0)}", false, false },  // Should return weekday name
    };

    for( const auto& testCase : cases )
    {
        auto result = evaluator.Evaluate( testCase.expression );

        if( testCase.shouldError )
        {
            BOOST_CHECK( evaluator.HasErrors() );
        }
        else
        {
            BOOST_CHECK( !evaluator.HasErrors() );
            BOOST_CHECK( !result.empty() );

            if( testCase.shouldContainNumbers )
            {
                // Result should be a number
                BOOST_CHECK( std::all_of( result.begin(), result.end(),
                    []( char c ) { return std::isdigit( c ) || c == '.' || c == '-'; } ) );
            }
        }
    }

    // Test specific date formatting with known values
    auto result1 = evaluator.Evaluate( "@{dateformat(0, \"ISO\")}" );
    BOOST_CHECK_EQUAL( result1, "1970-01-01" );  // Unix epoch

    auto result2 = evaluator.Evaluate( "@{weekdayname(0)}" );
    BOOST_CHECK_EQUAL( result2, "Thursday" );  // Unix epoch was a Thursday
}

/**
 * Test conditional functions
 */
BOOST_AUTO_TEST_CASE( ConditionalFunctions )
{
    EXPRESSION_EVALUATOR evaluator;
    evaluator.SetVariable( "x", 10.0 );
    evaluator.SetVariable( "y", 5.0 );

    struct TestCase {
        std::string expression;
        std::string expected;
        bool shouldError;
    };

    const std::vector<TestCase> cases = {
        // Basic if function
        { "@{if(1, \"true\", \"false\")}", "true", false },
        { "@{if(0, \"true\", \"false\")}", "false", false },
        { "@{if(${x} > ${y}, \"greater\", \"not greater\")}", "greater", false },
        { "@{if(${x} < ${y}, \"less\", \"not less\")}", "not less", false },

        // Numeric if results
        { "@{if(1, 42, 24)}", "42", false },
        { "@{if(0, 42, 24)}", "24", false },
    };

    for( const auto& testCase : cases )
    {
        auto result = evaluator.Evaluate( testCase.expression );

        BOOST_CHECK( !evaluator.HasErrors() );
        BOOST_CHECK_EQUAL( result, testCase.expected );
    }
}

/**
 * Test random functions
 */
BOOST_AUTO_TEST_CASE( RandomFunctions )
{
    EXPRESSION_EVALUATOR evaluator;

    // Test that random function returns a value between 0 and 1
    auto result = evaluator.Evaluate( "@{random()}" );
    BOOST_CHECK( !evaluator.HasErrors() );

    double randomValue = wxStrtod( result, nullptr );
    BOOST_CHECK_GE( randomValue, 0.0 );
    BOOST_CHECK_LT( randomValue, 1.0 );

    // Test that consecutive calls return different values (with high probability)
    auto result2 = evaluator.Evaluate( "@{random()}" );
    double randomValue2 = wxStrtod( result2, nullptr );

    // It's theoretically possible these could be equal, but extremely unlikely
    BOOST_CHECK_NE( randomValue, randomValue2 );
}

/**
 * Test error handling and edge cases
 */
BOOST_AUTO_TEST_CASE( ErrorHandling )
{
    EXPRESSION_EVALUATOR evaluator;

    struct TestCase {
        std::string expression;
        bool shouldError;
        std::string description;
    };

    const std::vector<TestCase> cases = {
        // Syntax errors
        { "@{2 +}", true, "incomplete expression" },
        { "@{(2 + 3", true, "unmatched parenthesis" },
        { "@{2 + 3)}", true, "extra closing parenthesis" },
        { "@{}", true, "empty calculation" },

        // Unknown functions
        { "@{unknownfunc(1, 2)}", true, "unknown function" },

        // Wrong number of arguments
        { "@{abs()}", true, "abs with no arguments" },
        { "@{abs(1, 2)}", true, "abs with too many arguments" },
        { "@{sqrt()}", true, "sqrt with no arguments" },

        // Runtime errors
        { "@{1 / 0}", true, "division by zero" },
        { "@{sqrt(-1)}", true, "square root of negative" },

        // Valid expressions that should not error
        { "Plain text", false, "plain text should not error" },
        { "@{2 + 2}", false, "simple calculation should work" },
    };

    for( const auto& testCase : cases )
    {
        auto result = evaluator.Evaluate( testCase.expression );

        if( testCase.shouldError )
        {
            BOOST_CHECK_MESSAGE( evaluator.HasErrors(),
                                 "Expected error for: " + testCase.description );
        }
        else
        {
            BOOST_CHECK_MESSAGE( !evaluator.HasErrors(),
                                 "Unexpected error for: " + testCase.description );
        }
    }
}

/**
 * Test complex nested expressions
 */
BOOST_AUTO_TEST_CASE( ComplexExpressions )
{
    EXPRESSION_EVALUATOR evaluator;
    evaluator.SetVariable( "pi", 3.14159 );
    evaluator.SetVariable( "radius", 5.0 );

    struct TestCase {
        std::string expression;
        std::string expected;
        double tolerance;
    };

    const std::vector<TestCase> cases = {
        // Complex mathematical expressions
        { "@{2 * ${pi} * ${radius}}", "31.42", 0.01 },  // Circumference
        { "@{${pi} * pow(${radius}, 2)}", "78.54", 0.01 },  // Area
        { "@{sqrt(pow(3, 2) + pow(4, 2))}", "5", 0.001 },  // Pythagorean theorem

        // Nested function calls
        { "@{max(abs(-5), sqrt(16), floor(3.7))}", "5", 0.001 },
        { "@{round(avg(1.1, 2.2, 3.3), 1)}", "2.2", 0.001 },

        // Mixed string and math
        { "Circle with radius @{${radius}} has area @{format(${pi} * pow(${radius}, 2), 1)}",
          "Circle with radius 5 has area 78.5", 0 },
    };

    for( const auto& testCase : cases )
    {
        auto result = evaluator.Evaluate( testCase.expression );
        std::string resultStr = result.ToStdString();
        BOOST_CHECK( !evaluator.HasErrors() );

        if( testCase.tolerance > 0 )
        {
            // Extract numeric part for comparison
            std::regex numberRegex( R"([\d.]+)" );
            std::smatch match;

            if( std::regex_search( resultStr, match, numberRegex ) )
            {
                double actualValue = std::stod( match[0].str() );
                double expectedValue = std::stod( testCase.expected );
                BOOST_CHECK_CLOSE( actualValue, expectedValue, testCase.tolerance * 100 );
            }
        }
        else
        {
            BOOST_CHECK_EQUAL( result, testCase.expected );
        }
    }
}

/**
 * Test performance with large expressions
 */
BOOST_AUTO_TEST_CASE( Performance )
{
    EXPRESSION_EVALUATOR evaluator;

    // Build a large expression with many calculations
    std::string largeExpression = "Result: ";
    for( int i = 0; i < 50; ++i )
    {
        largeExpression += "@{" + std::to_string(i) + " * 2} ";
    }

    auto start = std::chrono::high_resolution_clock::now();
    auto result = evaluator.Evaluate( largeExpression );
    auto end = std::chrono::high_resolution_clock::now();

    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>( end - start );

    BOOST_CHECK( !evaluator.HasErrors() );
    BOOST_CHECK( !result.empty() );

    // Should complete in reasonable time (less than 1 second)
    BOOST_CHECK_LT( duration.count(), 1000 );
}

BOOST_AUTO_TEST_SUITE_END()
