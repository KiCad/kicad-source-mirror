/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright The KiCad Developers, see AUTHORS.TXT for contributors.
 *
 * This program is free softwar        { "@{datestring('2023年12月25日')}", "19716", false },  // Christmas 2023       { "@{datestring('2023年12月25日')}", "19716", false },  // Christmas 2023       { "@{datestring('2023年12월25일')}", "19716", false },  // Christmas 2023; you can redistribute it and/or
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
 * Test suite for text_eval_parser date and time functionality
 */

#include <qa_utils/wx_utils/unit_test_utils.h>
#include <text_eval/text_eval_wrapper.h>

#include <chrono>
#include <regex>

BOOST_AUTO_TEST_SUITE( TextEvalParserDateTime )

BOOST_AUTO_TEST_CASE( DateFormatting )
{
    EXPRESSION_EVALUATOR evaluator;

    struct TestCase {
        std::string expression;
        std::string expected;
        bool shouldError;
    };

    const std::vector<TestCase> cases = {
        // Test Unix epoch date (1970-01-01)
        { "@{dateformat(0)}", "1970-01-01", false },
        { "@{dateformat(0, \"ISO\")}", "1970-01-01", false },
        { "@{dateformat(0, \"iso\")}", "1970-01-01", false },
        { "@{dateformat(0, \"US\")}", "01/01/1970", false },
        { "@{dateformat(0, \"us\")}", "01/01/1970", false },
        { "@{dateformat(0, \"EU\")}", "01/01/1970", false },
        { "@{dateformat(0, \"european\")}", "01/01/1970", false },
        { "@{dateformat(0, \"long\")}", "January 1, 1970", false },
        { "@{dateformat(0, \"short\")}", "Jan 1, 1970", false },

        // Test some known dates
        { "@{dateformat(365)}", "1971-01-01", false },  // One year after epoch
        { "@{dateformat(1000)}", "1972-09-27", false }, // 1000 days after epoch

        // Test weekday names
        { "@{weekdayname(0)}", "Thursday", false },  // Unix epoch was Thursday
        { "@{weekdayname(1)}", "Friday", false },    // Next day
        { "@{weekdayname(2)}", "Saturday", false },  // Weekend
        { "@{weekdayname(3)}", "Sunday", false },
        { "@{weekdayname(4)}", "Monday", false },
        { "@{weekdayname(5)}", "Tuesday", false },
        { "@{weekdayname(6)}", "Wednesday", false },
        { "@{weekdayname(7)}", "Thursday", false },  // Week cycles

        // Test negative dates (before epoch)
        { "@{dateformat(-1)}", "1969-12-31", false },
        { "@{weekdayname(-1)}", "Wednesday", false },
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
            BOOST_CHECK_MESSAGE( !evaluator.HasErrors(),
                                "Error in expression: " + testCase.expression +
                                " Errors: " + evaluator.GetErrorSummary().ToStdString() );
            BOOST_CHECK_EQUAL( result.ToStdString( wxConvUTF8 ), testCase.expected );
        }
    }
}

/**
 * Test CJK (Chinese, Japanese, Korean) date formatting
 */
BOOST_AUTO_TEST_CASE( CJKDateFormatting )
{
    EXPRESSION_EVALUATOR evaluator;

    struct TestCase {
        std::string expression;
        std::string expected;
        bool shouldError;
    };

    const std::vector<TestCase> cases = {
        // Test Unix epoch date (1970-01-01) in CJK formats
        { "@{dateformat(0, \"Chinese\")}", "1970年01月01日", false },
        { "@{dateformat(0, \"chinese\")}", "1970年01月01日", false },
        { "@{dateformat(0, \"CN\")}", "1970年01月01日", false },
        { "@{dateformat(0, \"cn\")}", "1970年01月01日", false },

        { "@{dateformat(0, \"Japanese\")}", "1970年01月01日", false },
        { "@{dateformat(0, \"japanese\")}", "1970年01月01日", false },
        { "@{dateformat(0, \"JP\")}", "1970年01月01日", false },
        { "@{dateformat(0, \"jp\")}", "1970年01月01日", false },

        { "@{dateformat(0, \"Korean\")}", "1970년 01월 01일", false },
        { "@{dateformat(0, \"korean\")}", "1970년 01월 01일", false },
        { "@{dateformat(0, \"KR\")}", "1970년 01월 01일", false },
        { "@{dateformat(0, \"kr\")}", "1970년 01월 01일", false },

        // Test some other dates in CJK formats
        { "@{dateformat(365, \"Chinese\")}", "1971年01月01日", false },    // One year after epoch
        { "@{dateformat(365, \"Japanese\")}", "1971年01月01日", false },   // One year after epoch
        { "@{dateformat(365, \"Korean\")}", "1971년 01월 01일", false },   // One year after epoch

        { "@{dateformat(1000, \"Chinese\")}", "1972年09月27日", false },   // 1000 days after epoch
        { "@{dateformat(1000, \"Japanese\")}", "1972年09月27日", false },  // 1000 days after epoch
        { "@{dateformat(1000, \"Korean\")}", "1972년 09월 27일", false },  // 1000 days after epoch

        // Test negative dates (before epoch) in CJK formats
        { "@{dateformat(-1, \"Chinese\")}", "1969年12月31日", false },
        { "@{dateformat(-1, \"Japanese\")}", "1969年12月31日", false },
        { "@{dateformat(-1, \"Korean\")}", "1969년 12월 31일", false },

        // Test leap year date (Feb 29, 1972) in CJK formats
        { "@{dateformat(789, \"Chinese\")}", "1972年02月29日", false },    // Feb 29, 1972 (leap year)
        { "@{dateformat(789, \"Japanese\")}", "1972年02月29日", false },   // Feb 29, 1972 (leap year)
        { "@{dateformat(789, \"Korean\")}", "1972년 02월 29일", false },   // Feb 29, 1972 (leap year)
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
            BOOST_CHECK_MESSAGE( !evaluator.HasErrors(),
                                "Error in expression: " + testCase.expression +
                                " Errors: " + evaluator.GetErrorSummary().ToStdString() );
            BOOST_CHECK_EQUAL( result.ToStdString( wxConvUTF8 ), testCase.expected );
        }
    }
}

/**
 * Test CJK (Chinese, Japanese, Korean) date parsing with datestring function
 */
BOOST_AUTO_TEST_CASE( CJKDateParsing )
{
    EXPRESSION_EVALUATOR evaluator;

    struct TestCase {
        std::string expression;
        std::string expected;
        bool shouldError;
    };

    const std::vector<TestCase> cases = {
        // Test if basic functions work first
        { "@{dateformat(0)}", "1970-01-01", false },            // Test basic dateformat
        { "@{upper(\"test\")}", "TEST", false },                // Test basic string function

        // Test ASCII date parsing first to see if datestring function works
        { "@{datestring(\"2024-03-15\")}", "19797", false },    // Test ASCII date
        { "@{datestring(\"1970-01-01\")}", "0", false },        // Unix epoch

        // Test Chinese date parsing (年月日)
        { "@{datestring('2024年03月15日')}", "19797", false },  // Days since epoch for 2024-03-15
        { "@{datestring('1970年01月01日')}", "0", false },      // Unix epoch
        { "@{datestring('2024年01月01日')}", "19723", false },  // New Year 2024
        { "@{datestring('1972年02月29日')}", "789", false },    // Leap year date
        { "@{datestring('1969年12月31日')}", "-1", false },     // Day before epoch

        // Test Korean date parsing (년월일) with spaces
        { "@{datestring(\"2024년 03월 15일\")}", "19797", false }, // Days since epoch for 2024-03-15
        { "@{datestring(\"1970년 01월 01일\")}", "0", false },     // Unix epoch
        { "@{datestring(\"2024년 01월 01일\")}", "19723", false }, // New Year 2024
        { "@{datestring(\"1972년 02월 29일\")}", "789", false },   // Leap year date
        { "@{datestring(\"1969년 12월 31일\")}", "-1", false },    // Day before epoch

        // Test Korean date parsing (년월일) without spaces
        { "@{datestring(\"2024년03월15일\")}", "19797", false },   // Days since epoch for 2024-03-15
        { "@{datestring(\"1970년01월01일\")}", "0", false },       // Unix epoch

        // Test integration: parse CJK date and format in different style
        { "@{dateformat(datestring('2024年03월15일'), 'ISO')}", "2024-03-15", false },
        { "@{dateformat(datestring('2024년 03월 15일'), 'ISO')}", "2024-03-15", false },
        { "@{dateformat(datestring('1970年01月01日'), 'US')}", "01/01/1970", false },
        { "@{dateformat(datestring('1970년 01월 01일'), 'EU')}", "01/01/1970", false },

        // Test round-trip: CJK -> parse -> format back to CJK
        { "@{dateformat(datestring('2024年03월15日'), 'Chinese')}", "2024年03月15日", false },
        { "@{dateformat(datestring('2024년 03월 15일'), 'Korean')}", "2024년 03월 15일", false },

        // Test invalid CJK dates (should error)
        { "@{datestring('2024年13月15日')}", "", true },         // Invalid month
        { "@{datestring('2024년 02월 30일')}", "", true },       // Invalid day for February
        { "@{datestring('2024年02月')}", "", true },             // Missing day
        { "@{datestring('2024년')}", "", true },                 // Missing month and day
    };

    for( const auto& testCase : cases )
    {
        auto result = evaluator.Evaluate( wxString::FromUTF8( testCase.expression ) );

        if( testCase.shouldError )
        {
            BOOST_CHECK_MESSAGE( evaluator.HasErrors(),
                                "Expected error but got result: " +
                                result.ToStdString( wxConvUTF8 ) +
                                " for expression: " + testCase.expression );
        }
        else
        {
            BOOST_CHECK_MESSAGE( !evaluator.HasErrors(),
                                "Error in expression: " + testCase.expression +
                                " Errors: " + evaluator.GetErrorSummary().ToStdString() );
            BOOST_CHECK_EQUAL( result.ToStdString( wxConvUTF8 ), testCase.expected );
        }
    }
}

/**
 * Test current date/time functions
 */
BOOST_AUTO_TEST_CASE( CurrentDateTime )
{
    EXPRESSION_EVALUATOR evaluator;

    auto todayResult = evaluator.Evaluate( "@{today()}" );
    BOOST_CHECK( !evaluator.HasErrors() );

    // Should return a number (days since epoch)
    double todayDays = std::stod( todayResult.ToStdString() );

    // Should be a reasonable number of days since 1970
    // As of 2024, this should be over 19,000 days
    BOOST_CHECK_GT( todayDays, 19000 );
    BOOST_CHECK_LT( todayDays, 50000 );  // Reasonable upper bound

    // Test now() function
    auto nowResult = evaluator.Evaluate( "@{now()}" );
    BOOST_CHECK( !evaluator.HasErrors() );

    // Should return a timestamp (seconds since epoch)
    double nowTimestamp = std::stod( nowResult.ToStdString() );

    // Should be a reasonable timestamp
    auto currentTime = std::chrono::system_clock::now();
    auto currentTimestamp = std::chrono::system_clock::to_time_t( currentTime );
    double currentTimestampDouble = static_cast<double>( currentTimestamp );

    // Should be within a few seconds of current time
    BOOST_CHECK_CLOSE( nowTimestamp, currentTimestampDouble, 1.0 );  // Within 1%

    // Test that consecutive calls to today() return the same value
    auto todayResult2 = evaluator.Evaluate( "@{today()}" );
    BOOST_CHECK_EQUAL( todayResult, todayResult2 );

    // Test formatting current date
    auto formattedToday = evaluator.Evaluate( "@{dateformat(today(), \"ISO\")}" );
    BOOST_CHECK( !evaluator.HasErrors() );

    // Should be in ISO format: YYYY-MM-DD
    std::regex isoDateRegex( R"(\d{4}-\d{2}-\d{2})" );
    BOOST_CHECK( std::regex_match( formattedToday.ToStdString(), isoDateRegex ) );
}

/**
 * Test date arithmetic and calculations
 */
BOOST_AUTO_TEST_CASE( DateArithmetic )
{
    EXPRESSION_EVALUATOR evaluator;

    struct TestCase {
        std::string expression;
        std::string expected;
        bool shouldError;
    };

    const std::vector<TestCase> cases = {
        // Date arithmetic
        { "@{dateformat(0 + 1)}", "1970-01-02", false },  // Add one day
        { "@{dateformat(0 + 7)}", "1970-01-08", false },  // Add one week
        { "@{dateformat(0 + 30)}", "1970-01-31", false }, // Add 30 days
        { "@{dateformat(0 + 365)}", "1971-01-01", false }, // Add one year (1970 was not leap)

        // Leap year test
        { "@{dateformat(365 + 365 + 366)}", "1973-01-01", false }, // 1972 was leap year

        // Date differences
        { "@{365 - 0}", "365", false },  // Days between dates

        // Complex date expressions
        { "@{weekdayname(today())}", "", false },  // Should return a weekday name (we can't predict which)
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

            if( !testCase.expected.empty() )
            {
                BOOST_CHECK_EQUAL( result.ToStdString( wxConvUTF8 ), testCase.expected );
            }
            else
            {
                // For dynamic results like weekday names, just check it's not empty
                BOOST_CHECK( !result.empty() );
            }
        }
    }
}

/**
 * Test date edge cases and boundary conditions
 */
BOOST_AUTO_TEST_CASE( DateEdgeCases )
{
    EXPRESSION_EVALUATOR evaluator;

    struct TestCase {
        std::string expression;
        std::string expected;
        bool shouldError;
    };

    const std::vector<TestCase> cases = {
        // Leap year boundaries
        { "@{dateformat(365 + 365 + 59)}", "1972-02-29", false },  // Feb 29, 1972 (leap year)
        { "@{dateformat(365 + 365 + 60)}", "1972-03-01", false },  // Mar 1, 1972

        // Year boundaries
        { "@{dateformat(365 - 1)}", "1970-12-31", false },  // Last day of 1970
        { "@{dateformat(365)}", "1971-01-01", false },      // First day of 1971

        // Month boundaries
        { "@{dateformat(30)}", "1970-01-31", false },   // Last day of January
        { "@{dateformat(31)}", "1970-02-01", false },   // First day of February
        { "@{dateformat(58)}", "1970-02-28", false },   // Last day of February 1970 (not leap)
        { "@{dateformat(59)}", "1970-03-01", false },   // First day of March 1970

        // Large date values
        { "@{dateformat(36525)}", "2070-01-01", false }, // 100 years after epoch

        // Negative dates (before epoch)
        { "@{dateformat(-365)}", "1969-01-01", false },  // One year before epoch
        { "@{dateformat(-1)}", "1969-12-31", false },    // One day before epoch

        // Weekday wrap-around
        { "@{weekdayname(-1)}", "Wednesday", false },    // Day before Thursday
        { "@{weekdayname(-7)}", "Thursday", false },     // One week before

        // Edge case: very large weekday values
        { "@{weekdayname(7000)}", "Thursday", false },   // Should still work
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
            BOOST_CHECK_MESSAGE( !evaluator.HasErrors(),
                                "Error in expression: " + testCase.expression );
            BOOST_CHECK_EQUAL( result.ToStdString( wxConvUTF8 ), testCase.expected );
        }
    }
}

/**
 * Test date formatting with mixed expressions
 */
BOOST_AUTO_TEST_CASE( DateFormattingMixed )
{
    EXPRESSION_EVALUATOR evaluator;
    evaluator.SetVariable( "days_offset", 100.0 );

    struct TestCase {
        std::string expression;
        bool shouldWork;
    };

    const std::vector<TestCase> cases = {
        // Complex expressions combining dates and variables
        { "Today is @{dateformat(today())} which is @{weekdayname(today())}", true },
        { "Date: @{dateformat(0 + ${days_offset}, \"long\")}", true },
        { "In @{format(${days_offset})} days: @{dateformat(today() + ${days_offset})}", true },

        // Nested function calls
        { "@{upper(weekdayname(today()))}", true },
        { "@{lower(dateformat(today(), \"long\"))}", true },

        // Multiple date calculations
        { "Start: @{dateformat(0)} End: @{dateformat(365)} Duration: @{365 - 0} days", true },
    };

    for( const auto& testCase : cases )
    {
        auto result = evaluator.Evaluate( wxString::FromUTF8( testCase.expression ) );

        if( testCase.shouldWork )
        {
            BOOST_CHECK_MESSAGE( !evaluator.HasErrors(),
                                "Error in expression: " + testCase.expression +
                                " Result: " + result.ToStdString( wxConvUTF8 ) );
            BOOST_CHECK( !result.empty() );
        }
        else
        {
            BOOST_CHECK( evaluator.HasErrors() );
        }
    }
}

/**
 * Test performance of date operations
 */
BOOST_AUTO_TEST_CASE( DatePerformance )
{
    EXPRESSION_EVALUATOR evaluator;

    // Test that date operations are reasonably fast
    auto start = std::chrono::high_resolution_clock::now();

    // Perform many date operations
    for( int i = 0; i < 1000; ++i )
    {
        auto result = evaluator.Evaluate( "@{dateformat(" + std::to_string(i) + ")}" );
        BOOST_CHECK( !evaluator.HasErrors() );
    }

    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>( end - start );

    // Should complete in reasonable time (less than 100 milliseconds for 1000 operations)
    BOOST_CHECK_LT( duration.count(), 100 );
}

BOOST_AUTO_TEST_SUITE_END()
