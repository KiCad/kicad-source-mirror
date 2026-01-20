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
 * Test suite for text_eval_parser VCS functionality
 */

#include <qa_utils/wx_utils/unit_test_utils.h>
#include <text_eval/text_eval_wrapper.h>
#include <git/git_backend.h>
#include <git/libgit_backend.h>

#include <chrono>
#include <regex>

/**
 * Fixture to set up and tear down the git backend for VCS tests.
 */
struct VCS_TEST_FIXTURE
{
    VCS_TEST_FIXTURE()
    {
        m_backend = new LIBGIT_BACKEND();
        m_backend->Init();
        SetGitBackend( m_backend );
    }

    ~VCS_TEST_FIXTURE()
    {
        SetGitBackend( nullptr );
        m_backend->Shutdown();
        delete m_backend;
    }

    LIBGIT_BACKEND* m_backend;
};

BOOST_FIXTURE_TEST_SUITE( TextEvalParserVcs, VCS_TEST_FIXTURE )

/**
 * Test VCS identifier functions with various lengths
 */
BOOST_AUTO_TEST_CASE( VcsIdentifierFormatting )
{
    EXPRESSION_EVALUATOR evaluator;

    struct TestCase
    {
        std::string expression;
        int         expectedLength;
        bool        shouldError;
    };

    const std::vector<TestCase> cases = {
        // Different identifier lengths
        { "@{vcsidentifier()}", 40, false },   // Full SHA-1
        { "@{vcsidentifier(40)}", 40, false }, // Explicit full
        { "@{vcsidentifier(7)}", 7, false },   // Short
        { "@{vcsidentifier(8)}", 8, false },   // 8 chars
        { "@{vcsidentifier(12)}", 12, false }, // Medium
        { "@{vcsidentifier(4)}", 4, false },   // Minimum (clamped)

        // File variants
        { "@{vcsfileidentifier(\".\")}", 40, false },
        { "@{vcsfileidentifier(\".\", 8)}", 8, false },
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
            BOOST_CHECK_MESSAGE( !evaluator.HasErrors(), "Error in expression: " + testCase.expression + " Errors: "
                                                                 + evaluator.GetErrorSummary().ToStdString() );

            // If in a git repo, validate format
            if( !result.IsEmpty() )
            {
                BOOST_CHECK_EQUAL( result.Length(), testCase.expectedLength );

                // Should be valid hex
                std::regex hexPattern( "^[0-9a-f]+$" );
                BOOST_CHECK( std::regex_match( result.ToStdString(), hexPattern ) );
            }
        }
    }
}

/**
 * Test VCS branch and author information
 */
BOOST_AUTO_TEST_CASE( VcsBranchAndAuthorInfo )
{
    EXPRESSION_EVALUATOR evaluator;

    auto branch = evaluator.Evaluate( "@{vcsbranch()}" );
    auto author = evaluator.Evaluate( "@{vcsauthor()}" );
    auto authorEmail = evaluator.Evaluate( "@{vcsauthoremail()}" );
    auto committer = evaluator.Evaluate( "@{vcscommitter()}" );
    auto committerEmail = evaluator.Evaluate( "@{vcscommitteremail()}" );

    BOOST_CHECK( !evaluator.HasErrors() );

    // If in a VCS repo, validate email format
    if( !authorEmail.IsEmpty() )
    {
        BOOST_CHECK( authorEmail.Contains( "@" ) );
    }

    if( !committerEmail.IsEmpty() )
    {
        BOOST_CHECK( committerEmail.Contains( "@" ) );
    }

    // File variants
    auto fileAuthor = evaluator.Evaluate( "@{vcsfileauthor(\".\")}" );
    auto fileAuthorEmail = evaluator.Evaluate( "@{vcsfileauthoremail(\".\")}" );
    auto fileCommitter = evaluator.Evaluate( "@{vcsfilecommitter(\".\")}" );
    auto fileCommitterEmail = evaluator.Evaluate( "@{vcsfilecommitteremail(\".\")}" );

    BOOST_CHECK( !evaluator.HasErrors() );

    // Validate file variant email formats
    if( !fileAuthorEmail.IsEmpty() )
    {
        BOOST_CHECK( fileAuthorEmail.Contains( "@" ) );
    }

    if( !fileCommitterEmail.IsEmpty() )
    {
        BOOST_CHECK( fileCommitterEmail.Contains( "@" ) );
    }
}

/**
 * Test VCS dirty status functions
 */
BOOST_AUTO_TEST_CASE( VcsDirtyStatus )
{
    EXPRESSION_EVALUATOR evaluator;

    struct TestCase
    {
        std::string expression;
        bool        shouldError;
    };

    const std::vector<TestCase> cases = {
        // Dirty status (returns 0 or 1)
        { "@{vcsdirty()}", false },
        { "@{vcsdirty(0)}", false }, // Exclude untracked
        { "@{vcsdirty(1)}", false }, // Include untracked
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

            // If in a repo, validate format - dirty status should be 0 or 1
            if( !result.IsEmpty() )
            {
                BOOST_CHECK( result == "0" || result == "1" );
            }
        }
    }
}

/**
 * Test VCS dirty suffix functions
 */
BOOST_AUTO_TEST_CASE( VcsDirtySuffix )
{
    EXPRESSION_EVALUATOR evaluator;

    struct TestCase
    {
        std::string expression;
        bool        shouldError;
    };

    const std::vector<TestCase> cases = {
        // Dirty suffix (returns string or empty)
        { "@{vcsdirtysuffix()}", false },
        { "@{vcsdirtysuffix(\"-modified\")}", false },
        { "@{vcsdirtysuffix(\"+\", 1)}", false },
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
            // Suffix can be any string or empty, just validate no errors
        }
    }
}

/**
 * Test VCS label and distance functions
 */
BOOST_AUTO_TEST_CASE( VcsLabelsAndDistance )
{
    EXPRESSION_EVALUATOR evaluator;

    struct TestCase
    {
        std::string expression;
        bool        shouldError;
    };

    const std::vector<TestCase> cases = {
        // Label functions
        { "@{vcsnearestlabel()}", false },
        { "@{vcsnearestlabel(\"\")}", false },
        { "@{vcsnearestlabel(\"v*\")}", false },  // Match v* labels
        { "@{vcsnearestlabel(\"\", 0)}", false }, // Annotated only
        { "@{vcsnearestlabel(\"\", 1)}", false }, // Any labels

        // Distance functions
        { "@{vcslabeldistance()}", false },
        { "@{vcslabeldistance(\"v*\")}", false },
        { "@{vcslabeldistance(\"\", 1)}", false },
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

            // Distance should be a valid number if not empty
            if( !result.IsEmpty() && testCase.expression.find( "distance" ) != std::string::npos )
            {
                std::regex numberPattern( "^[0-9]+$" );
                BOOST_CHECK( std::regex_match( result.ToStdString(), numberPattern ) );
            }
        }
    }
}

/**
 * Test VCS commit date formatting
 */
BOOST_AUTO_TEST_CASE( VcsCommitDate )
{
    EXPRESSION_EVALUATOR evaluator;

    struct TestCase
    {
        std::string expression;
        std::regex  pattern;
        bool        shouldError;
    };

    const std::vector<TestCase> cases = {
        // Different date formats
        { "@{vcscommitdate()}", std::regex( "^([0-9]{4}-[0-9]{2}-[0-9]{2}|)$" ), false },
        { "@{vcscommitdate(\"ISO\")}", std::regex( "^([0-9]{4}-[0-9]{2}-[0-9]{2}|)$" ), false },
        { "@{vcscommitdate(\"US\")}", std::regex( "^([0-9]{2}/[0-9]{2}/[0-9]{4}|)$" ), false },
        { "@{vcscommitdate(\"EU\")}", std::regex( "^([0-9]{2}/[0-9]{2}/[0-9]{4}|)$" ), false },

        // File variant
        { "@{vcsfilecommitdate(\".\")}", std::regex( "^([0-9]{4}-[0-9]{2}-[0-9]{2}|)$" ), false },
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
            BOOST_CHECK_MESSAGE( !evaluator.HasErrors(), "Error in expression: " + testCase.expression + " Errors: "
                                                                 + evaluator.GetErrorSummary().ToStdString() );

            // Validate format if result is not empty
            BOOST_CHECK( std::regex_match( result.ToStdString(), testCase.pattern ) );
        }
    }
}

/**
 * Test performance of VCS operations
 */
BOOST_AUTO_TEST_CASE( VcsPerformance )
{
    EXPRESSION_EVALUATOR evaluator;

    // Test that VCS operations are reasonably fast
    auto start = std::chrono::high_resolution_clock::now();

    // Perform many VCS operations
    for( int i = 0; i < 100; ++i )
    {
        auto result = evaluator.Evaluate( "@{vcsidentifier(7)}" );
        BOOST_CHECK( !evaluator.HasErrors() );
    }

    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>( end - start );

    // Should complete in reasonable time (less than 2 seconds for 100 operations)
    BOOST_CHECK_LT( duration.count(), 2000 );
}

/**
 * Test mixed VCS and non-VCS expressions
 */
BOOST_AUTO_TEST_CASE( VcsMixedExpressions )
{
    EXPRESSION_EVALUATOR evaluator;
    evaluator.SetVariable( wxString( "PROJECT" ), wxString( "MyProject" ) );

    struct TestCase
    {
        std::string expression;
        bool        shouldError;
    };

    const std::vector<TestCase> cases = {
        // Mixed with text
        { "Version: @{vcsbranch()}", false },
        { "Commit: @{vcsidentifier(7)}", false },
        { "Author: @{vcsauthor()} <@{vcsauthoremail()}>", false },

        // Mixed with variables
        { "${PROJECT} @{vcsbranch()}", false },
        { "Built from @{vcsnearestlabel()}@{vcsdirtysuffix()}", false },

        // Mixed with calculations
        { "Distance: @{vcslabeldistance() + 0}", false },
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
            BOOST_CHECK( !result.IsEmpty() );
        }
    }
}

BOOST_AUTO_TEST_SUITE_END()
