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
 * Integration tests for text_eval_parser functionality including real-world scenarios
 */

#include <qa_utils/wx_utils/unit_test_utils.h>

// Code under test
#include <text_eval/text_eval_wrapper.h>

#include <chrono>
#include <regex>

/**
 * Declare the test suite
 */
BOOST_AUTO_TEST_SUITE( TextEvalParserIntegration )

/**
 * Test real-world expression scenarios
 */
BOOST_AUTO_TEST_CASE( RealWorldScenarios )
{
    EXPRESSION_EVALUATOR evaluator;

    // Set up variables that might be used in actual KiCad projects
    evaluator.SetVariable( "board_width", 100.0 );
    evaluator.SetVariable( "board_height", 80.0 );
    evaluator.SetVariable( "trace_width", 0.2 );
    evaluator.SetVariable( "component_count", 45.0 );
    evaluator.SetVariable( "revision", 3.0 );
    evaluator.SetVariable( std::string("project_name"), std::string("My PCB Project") );
    evaluator.SetVariable( std::string("designer"), std::string("John Doe") );

    struct TestCase {
        std::string expression;
        std::string expectedPattern; // Can be exact match or regex pattern
        bool isRegex;
        bool shouldError;
        std::string description;
    };

    const std::vector<TestCase> cases = {
        // Board dimension calculations
        {
            "Board area: @{${board_width} * ${board_height}} mm²",
            "Board area: 8000 mm²",
            false, false,
            "Board area calculation"
        },
        {
            "Perimeter: @{2 * (${board_width} + ${board_height})} mm",
            "Perimeter: 360 mm",
            false, false,
            "Board perimeter calculation"
        },
        {
            "Diagonal: @{format(sqrt(pow(${board_width}, 2) + pow(${board_height}, 2)), 1)} mm",
            "Diagonal: 128.1 mm",
            false, false,
            "Board diagonal calculation"
        },

        // Text formatting scenarios
        {
            "Project: ${project_name} | Designer: ${designer} | Rev: @{${revision}}",
            "Project: My PCB Project | Designer: John Doe | Rev: 3",
            false, false,
            "Title block information"
        },
        {
            "Components: @{${component_count}} | Density: @{format(${component_count} / (${board_width} * ${board_height} / 10000), 2)} per cm²",
            "Components: 45 | Density: 56.25 per cm²",
            false, false,
            "Component density calculation"
        },

        // Date-based revision tracking
        {
            "Created: @{dateformat(today())} | Build: @{today()} days since epoch",
            R"(Created: \d{4}-\d{2}-\d{2} \| Build: \d+ days since epoch)",
            true, false,
            "Date-based tracking"
        },

        // Conditional formatting
        {
            "Status: @{if(${component_count} > 50, \"Complex\", \"Simple\")} design",
            "Status: Simple design",
            false, false,
            "Conditional design complexity"
        },
        {
            "Status: @{if(${trace_width} >= 0.2, \"Standard\", \"Fine pitch\")} (@{${trace_width}}mm)",
            "Status: Standard (0.2mm)",
            false, false,
            "Conditional trace width description"
        },

        // Multi-line documentation
        {
            "PCB Summary:\n- Size: @{${board_width}}×@{${board_height}}mm\n- Area: @{${board_width} * ${board_height}}mm²\n- Components: @{${component_count}}",
            "PCB Summary:\n- Size: 100×80mm\n- Area: 8000mm²\n- Components: 45",
            false, false,
            "Multi-line documentation"
        },

        // Error scenarios - undefined variables error and return unchanged
        {
            "Invalid: @{${undefined_var}} test",
            "Invalid: @{${undefined_var}} test",
            false, true,
            "Undefined variable behavior"
        },
    };

    for( const auto& testCase : cases )
    {
        auto result = evaluator.Evaluate( wxString::FromUTF8( testCase.expression ) );

        if( testCase.shouldError )
        {
            BOOST_CHECK_MESSAGE( evaluator.HasErrors(),
                                "Expected error for: " + testCase.description );
        }
        else
        {
            BOOST_CHECK_MESSAGE( !evaluator.HasErrors(),
                                "Unexpected error for: " + testCase.description +
                                " - " + evaluator.GetErrorSummary().ToStdString() );

            if( testCase.isRegex )
            {
                std::regex pattern( testCase.expectedPattern );
                BOOST_CHECK_MESSAGE( std::regex_match( result.ToStdString( wxConvUTF8 ), pattern ),
                                    "Result '" + result.ToStdString( wxConvUTF8 ) + "' doesn't match pattern '" +
                                    testCase.expectedPattern + "' for: " + testCase.description );
            }
            else
            {
                BOOST_CHECK_MESSAGE( result.ToStdString( wxConvUTF8 ) == testCase.expectedPattern,
                                    "Expected '" + testCase.expectedPattern + "' but got '" +
                                    result.ToStdString( wxConvUTF8 ) + "' for: " + testCase.description );
            }
        }
    }
}

/**
 * Test callback-based variable resolution
 */
BOOST_AUTO_TEST_CASE( CallbackVariableResolution )
{
    // Create evaluator with custom callback
    auto variableCallback = []( const std::string& varName ) -> calc_parser::Result<calc_parser::Value> {
        if( varName == "dynamic_value" )
            return calc_parser::MakeValue<calc_parser::Value>( 42.0 );
        else if( varName == "dynamic_string" )
            return calc_parser::MakeValue<calc_parser::Value>( std::string("Hello from callback") );
        else if( varName == "computed_value" )
            return calc_parser::MakeValue<calc_parser::Value>( std::sin( 3.14159 / 4 ) * 100.0 );  // Should be about 70.7
        else
            return calc_parser::MakeError<calc_parser::Value>( "Variable '" + varName + "' not found in callback" );
    };

    EXPRESSION_EVALUATOR evaluator( variableCallback, false );

    struct TestCase {
        std::string expression;
        std::string expected;
        double tolerance;
        bool shouldError;
    };

    const std::vector<TestCase> cases = {
        { "@{${dynamic_value}}", "42", 0, false },
        { "Message: ${dynamic_string}", "Message: Hello from callback", 0, false },
        { "@{format(${computed_value}, 1)}", "70.7", 0.1, false },
        { "@{${dynamic_value} + ${computed_value}}", "112.7", 0.1, false },
        { "${nonexistent}", "${nonexistent}", 0, true },
    };

    for( const auto& testCase : cases )
    {
        auto result = evaluator.Evaluate( wxString::FromUTF8( testCase.expression ) );

        if( testCase.shouldError )
        {
            BOOST_CHECK( evaluator.HasErrors() );
        }
        else
        {
            BOOST_CHECK( !evaluator.HasErrors() );

            if( testCase.tolerance > 0 )
            {
                // For floating point comparisons, extract the number
                std::regex numberRegex( R"([\d.]+)" );
                std::smatch match;
                std::string resultStr = result.ToStdString( wxConvUTF8 );
                if( std::regex_search( resultStr, match, numberRegex ) )
                {
                    double actualValue = std::stod( match[0].str() );
                    double expectedValue = std::stod( testCase.expected );
                    BOOST_CHECK_CLOSE( actualValue, expectedValue, testCase.tolerance * 100 );
                }
            }
            else
            {
                BOOST_CHECK_EQUAL( result.ToStdString( wxConvUTF8 ), testCase.expected );
            }
        }
    }
}

/**
 * Test concurrent/thread safety (basic test)
 */
BOOST_AUTO_TEST_CASE( ThreadSafety )
{
    // Create multiple evaluators that could be used in different threads
    std::vector<std::unique_ptr<EXPRESSION_EVALUATOR>> evaluators;

    for( int i = 0; i < 10; ++i )
    {
        auto evaluator = std::make_unique<EXPRESSION_EVALUATOR>();
        evaluator->SetVariable( "thread_id", static_cast<double>( i ) );
        evaluator->SetVariable( "multiplier", 5.0 );
        evaluators.push_back( std::move( evaluator ) );
    }

    // Test that each evaluator maintains its own state
    for( int i = 0; i < 10; ++i )
    {
        auto result = evaluators[i]->Evaluate( "@{${thread_id} * ${multiplier}}" );
        BOOST_CHECK( !evaluators[i]->HasErrors() );

        double expected = static_cast<double>( i * 5 );
        double actual = std::stod( result.ToStdString( wxConvUTF8 ) );
        BOOST_CHECK_CLOSE( actual, expected, 0.001 );
    }
}

/**
 * Test memory management and large expressions
 */
BOOST_AUTO_TEST_CASE( MemoryManagement )
{
    EXPRESSION_EVALUATOR evaluator;

    // Test large nested expressions
    std::string complexExpression = "@{";
    for( int i = 0; i < 100; ++i )
    {
        if( i > 0 ) complexExpression += " + ";
        complexExpression += std::to_string( i );
    }
    complexExpression += "}";

    auto result = evaluator.Evaluate( wxString::FromUTF8( complexExpression ) );
    BOOST_CHECK( !evaluator.HasErrors() );

    // Sum of 0..99 is 4950
    BOOST_CHECK_EQUAL( result.ToStdString( wxConvUTF8 ), std::string( "4950" ) );

    // Test many small expressions
    for( int i = 0; i < 1000; ++i )
    {
        auto expr = "@{" + std::to_string( i ) + " * 2}";
        auto result = evaluator.Evaluate( wxString::FromUTF8( expr ) );
        BOOST_CHECK( !evaluator.HasErrors() );
        BOOST_CHECK_EQUAL( result.ToStdString( wxConvUTF8 ), std::to_string( i * 2 ) );
    }
}

/**
 * Test edge cases in parsing and evaluation
 */
BOOST_AUTO_TEST_CASE( ParsingEdgeCases )
{
    EXPRESSION_EVALUATOR evaluator;

    struct TestCase {
        std::string expression;
        std::string expected;
        bool shouldError;
        double precision;
        std::string description;
    };

    const std::vector<TestCase> cases = {
        // Whitespace handling
        { "@{ 2 + 3 }", "5", false, 0.0, "Spaces in expression" },
        { "@{\t2\t+\t3\t}", "5", false, 0.0, "Tabs in expression" },
        { "@{\n2\n+\n3\n}", "5", false, 0.0, "Newlines in expression" },

        // String escaping and special characters
        { "@{\"Hello\\\"World\\\"\"}", "Hello\"World\"", false, 0.0, "Escaped quotes in string" },
        { "@{\"Line1\\nLine2\"}", "Line1\nLine2", false, 0.0, "Newline in string" },

        // Multiple calculations in complex text
        { "A: @{1+1}, B: @{2*2}, C: @{3^2}", "A: 2, B: 4, C: 9", false, 0.0, "Multiple calculations" },

        // Edge cases with parentheses
        { "@{((((2))))}", "2", false, 0.0, "Multiple nested parentheses" },
        { "@{(2 + 3) * (4 + 5)}", "45", false, 0.0, "Grouped operations" },

        // Empty and minimal expressions
        { "No calculations here", "No calculations here", false, 0.0, "Plain text" },
        { "", "", false, 0.0, "Empty string" },
        { "@{0}", "0", false, 0.0, "Zero value" },
        { "@{-0}", "0", false, 0.0, "Negative zero" },

        // Precision and rounding edge cases
        { "@{0.1 + 0.2}", "0.3", false, 0.01, "Floating point precision" },
        { "@{1.0 / 3.0}", "0.333333", false, 0.01, "Repeating decimal" },

        // Large numbers
        { "@{1000000 * 1000000}", "1e+12", false, 0.01, "Large number result" },

        // Error recovery - malformed expressions left unchanged, valid ones evaluated
        { "Good @{2+2} bad @{2+} good @{3+3}", "Good 4 bad @{2+} good 6", true, 0.0, "Error recovery" },
    };

    for( const auto& testCase : cases )
    {
        auto result = evaluator.Evaluate( wxString::FromUTF8( testCase.expression ) );

        if( testCase.shouldError )
        {
            BOOST_CHECK_MESSAGE( evaluator.HasErrors(), "Expected error for: " + testCase.description );
        }
        else
        {
            if( testCase.precision > 0.0 )
            {
                // For floating point comparisons, extract the number
                std::regex numberRegex( R"([\d.eE+-]+)" );
                std::smatch match;
                std::string resultStr = result.ToStdString( wxConvUTF8 );
                if( std::regex_search( resultStr, match, numberRegex ) )
                {
                    double actualValue = std::stod( match[0].str() );
                    double expectedValue = std::stod( testCase.expected );
                    BOOST_CHECK_CLOSE( actualValue, expectedValue, testCase.precision * 100 );
                }
            }
            else
            {
                BOOST_CHECK_MESSAGE( !evaluator.HasErrors(),
                                    "Unexpected error for: " + testCase.description +
                                    " - " + evaluator.GetErrorSummary().ToStdString() );
                BOOST_CHECK_MESSAGE( result.ToStdString( wxConvUTF8 ) == testCase.expected,
                                    "Expected '" + testCase.expected + "' but got '" +
                                    result.ToStdString( wxConvUTF8 ) + "' for: " + testCase.description );
            }
        }
    }
}

/**
 * Test performance with realistic workloads
 */
BOOST_AUTO_TEST_CASE( RealWorldPerformance )
{
    EXPRESSION_EVALUATOR evaluator;

    // Set up variables for a typical PCB project
    evaluator.SetVariable( "board_layers", 4.0 );
    evaluator.SetVariable( "component_count", 150.0 );
    evaluator.SetVariable( "net_count", 200.0 );
    evaluator.SetVariable( "via_count", 300.0 );
    evaluator.SetVariable( "board_width", 120.0 );
    evaluator.SetVariable( "board_height", 80.0 );

    // Simulate processing many text objects (like in a real PCB layout)
    std::vector<std::string> expressions = {
        "Layer @{${board_layers}}/4",
        "Components: @{${component_count}}",
        "Nets: @{${net_count}}",
        "Vias: @{${via_count}}",
        "Area: @{${board_width} * ${board_height}} mm²",
        "Density: @{format(${component_count} / (${board_width} * ${board_height} / 100), 1)} /cm²",
        "Via density: @{format(${via_count} / (${board_width} * ${board_height} / 100), 1)} /cm²",
        "Layer utilization: @{format(${net_count} / ${board_layers}, 1)} nets/layer",
        "Design complexity: @{if(${component_count} > 100, \"High\", \"Low\")}",
        "Board aspect ratio: @{format(${board_width} / ${board_height}, 2)}:1",
    };

    auto start = std::chrono::high_resolution_clock::now();

    // Process expressions many times (simulating real usage)
    for( int iteration = 0; iteration < 100; ++iteration )
    {
        for( const auto& expr : expressions )
        {
            auto result = evaluator.Evaluate( wxString::FromUTF8( expr ) );
            BOOST_CHECK( !evaluator.HasErrors() );
            BOOST_CHECK( !result.empty() );
        }
    }

    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>( end - start );

    // Should process 1000 expressions in reasonable time (less than 100 ms)
    BOOST_CHECK_LT( duration.count(), 100 );

    // Test that results are consistent
    for( auto& expr : expressions )
    {
        auto result1 = evaluator.Evaluate( wxString::FromUTF8( expr ) );
        auto result2 = evaluator.Evaluate( wxString::FromUTF8( expr ) );
        BOOST_CHECK_EQUAL( result1, result2 );
    }
}

BOOST_AUTO_TEST_SUITE_END()
