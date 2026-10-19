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
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

/**
 * @file
 * Test suite for text_eval_parser VCS functionality.
 *
 * Creates a temporary git repository so tests are self-contained and work
 * regardless of whether the build directory is inside the source tree.
 */

#include <qa_utils/wx_utils/unit_test_utils.h>
#include <text_eval/text_eval_wrapper.h>
#include <text_eval/text_eval_vcs.h>
#include <qa_utils/git_test_fixture.h>
#include <pgm_base.h>
#include <settings/settings_manager.h>

#include <chrono>
#include <fstream>
#include <regex>

#include <wx/dir.h>
#include <wx/filename.h>
#include <wx/utils.h>


static const char* TEST_AUTHOR_NAME  = "Test Author";
static const char* TEST_AUTHOR_EMAIL = "test@kicad.example";
static const char* TEST_COMMIT_MSG   = "Initial test commit";


/**
 * Fixture that creates a temporary git repo with one committed file.
 * This gives every VCS function a known, deterministic environment.
 */
struct VCS_TEST_FIXTURE : KI_TEST::GIT_TEST_FIXTURE
{
    VCS_TEST_FIXTURE() :
            m_originalDir( wxGetCwd() )
    {
        KIGIT::GitRepositoryPtr repo =
                KI_TEST::InitGitRepository( TempPath(), "main", TEST_AUTHOR_NAME, TEST_AUTHOR_EMAIL );
        KI_TEST::WriteGitFile( TempPath(), "test.txt", "test content\n" );
        KI_TEST::CommitGitFile( repo.get(), "test.txt", TEST_COMMIT_MSG );
        BOOST_REQUIRE( wxSetWorkingDirectory( TempPath() ) );
    }

    ~VCS_TEST_FIXTURE() { wxSetWorkingDirectory( m_originalDir ); }

    const wxString& tempDir() const { return TempPath(); }

private:
    // These tests exercise cwd-based discovery independently of projects loaded by other tests.
    TEXT_EVAL_VCS::CONTEXT_PATH_SCOPE m_context{ wxString() };
    wxString                          m_originalDir;
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
    };

    const std::vector<TestCase> cases = {
        { "@{vcsidentifier()}", 40 },
        { "@{vcsidentifier(40)}", 40 },
        { "@{vcsidentifier(7)}", 7 },
        { "@{vcsidentifier(8)}", 8 },
        { "@{vcsidentifier(12)}", 12 },
        { "@{vcsidentifier(4)}", 4 },

        { "@{vcsfileidentifier(\".\")}", 40 },
        { "@{vcsfileidentifier(\".\", 8)}", 8 },
    };

    std::regex hexPattern( "^[0-9a-f]+$" );

    for( const auto& testCase : cases )
    {
        auto result = evaluator.Evaluate( wxString::FromUTF8( testCase.expression ) );

        BOOST_CHECK_MESSAGE( !evaluator.HasErrors(),
                             "Error in expression: " + testCase.expression + " Errors: "
                                     + evaluator.GetErrorSummary().ToStdString() );

        BOOST_CHECK_EQUAL( result.Length(), testCase.expectedLength );
        BOOST_CHECK( std::regex_match( result.ToStdString(), hexPattern ) );
    }
}

/**
 * Test VCS branch and author information against known fixture values
 */
BOOST_AUTO_TEST_CASE( VcsBranchAndAuthorInfo )
{
    EXPRESSION_EVALUATOR evaluator;

    auto branch = evaluator.Evaluate( "@{vcsbranch()}" );
    BOOST_CHECK( !evaluator.HasErrors() );
    BOOST_CHECK( !branch.IsEmpty() );

    auto authorEmail = evaluator.Evaluate( "@{vcsauthoremail()}" );
    BOOST_CHECK( !evaluator.HasErrors() );
    BOOST_CHECK_EQUAL( authorEmail, TEST_AUTHOR_EMAIL );

    auto committerEmail = evaluator.Evaluate( "@{vcscommitteremail()}" );
    BOOST_CHECK( !evaluator.HasErrors() );
    BOOST_CHECK_EQUAL( committerEmail, TEST_AUTHOR_EMAIL );

    auto author = evaluator.Evaluate( "@{vcsauthor()}" );
    BOOST_CHECK( !evaluator.HasErrors() );
    BOOST_CHECK_EQUAL( author, TEST_AUTHOR_NAME );

    auto committer = evaluator.Evaluate( "@{vcscommitter()}" );
    BOOST_CHECK( !evaluator.HasErrors() );
    BOOST_CHECK_EQUAL( committer, TEST_AUTHOR_NAME );

    // File variants should return the same values since there's only one commit
    auto fileAuthorEmail = evaluator.Evaluate( "@{vcsfileauthoremail(\".\")}" );
    BOOST_CHECK( !evaluator.HasErrors() );
    BOOST_CHECK_EQUAL( fileAuthorEmail, TEST_AUTHOR_EMAIL );

    auto fileCommitterEmail = evaluator.Evaluate( "@{vcsfilecommitteremail(\".\")}" );
    BOOST_CHECK( !evaluator.HasErrors() );
    BOOST_CHECK_EQUAL( fileCommitterEmail, TEST_AUTHOR_EMAIL );
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
    };

    const std::vector<TestCase> cases = {
        { "@{vcsdirty()}" },
        { "@{vcsdirty(0)}" },
        { "@{vcsdirty(1)}" },
    };

    for( const auto& testCase : cases )
    {
        auto result = evaluator.Evaluate( wxString::FromUTF8( testCase.expression ) );

        BOOST_CHECK( !evaluator.HasErrors() );
        BOOST_CHECK( result == "0" || result == "1" );
    }
}

/**
 * Test VCS dirty suffix functions
 */
BOOST_AUTO_TEST_CASE( VcsDirtySuffix )
{
    EXPRESSION_EVALUATOR evaluator;

    const std::vector<std::string> cases = {
        "@{vcsdirtysuffix()}",
        "@{vcsdirtysuffix(\"-modified\")}",
        "@{vcsdirtysuffix(\"+\", 1)}",
    };

    for( const auto& expr : cases )
    {
        evaluator.Evaluate( wxString::FromUTF8( expr ) );
        BOOST_CHECK( !evaluator.HasErrors() );
    }
}

/**
 * Test VCS label and distance functions
 */
BOOST_AUTO_TEST_CASE( VcsLabelsAndDistance )
{
    EXPRESSION_EVALUATOR evaluator;

    const std::vector<std::string> cases = {
        "@{vcsnearestlabel()}",
        "@{vcsnearestlabel(\"\")}",
        "@{vcsnearestlabel(\"v*\")}",
        "@{vcsnearestlabel(\"\", 0)}",
        "@{vcsnearestlabel(\"\", 1)}",

        "@{vcslabeldistance()}",
        "@{vcslabeldistance(\"v*\")}",
        "@{vcslabeldistance(\"\", 1)}",
    };

    std::regex numberPattern( "^[0-9]+$" );

    for( const auto& expr : cases )
    {
        auto result = evaluator.Evaluate( wxString::FromUTF8( expr ) );
        BOOST_CHECK( !evaluator.HasErrors() );

        if( !result.IsEmpty() && expr.find( "distance" ) != std::string::npos )
        {
            BOOST_CHECK( std::regex_match( result.ToStdString(), numberPattern ) );
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
    };

    const std::vector<TestCase> cases = {
        { "@{vcscommitdate()}", std::regex( "^[0-9]{4}-[0-9]{2}-[0-9]{2}$" ) },
        { "@{vcscommitdate(\"ISO\")}", std::regex( "^[0-9]{4}-[0-9]{2}-[0-9]{2}$" ) },
        { "@{vcscommitdate(\"US\")}", std::regex( "^[0-9]{2}/[0-9]{2}/[0-9]{4}$" ) },
        { "@{vcscommitdate(\"EU\")}", std::regex( "^[0-9]{2}/[0-9]{2}/[0-9]{4}$" ) },

        { "@{vcsfilecommitdate(\".\")}", std::regex( "^[0-9]{4}-[0-9]{2}-[0-9]{2}$" ) },
    };

    for( const auto& testCase : cases )
    {
        auto result = evaluator.Evaluate( wxString::FromUTF8( testCase.expression ) );

        BOOST_CHECK_MESSAGE( !evaluator.HasErrors(),
                             "Error in expression: " + testCase.expression + " Errors: "
                                     + evaluator.GetErrorSummary().ToStdString() );

        BOOST_CHECK_MESSAGE( std::regex_match( result.ToStdString(), testCase.pattern ),
                             "Bad date format for " + testCase.expression + ": "
                                     + result.ToStdString() );
    }
}

/**
 * Test performance of VCS operations
 */
BOOST_AUTO_TEST_CASE( VcsPerformance )
{
    EXPRESSION_EVALUATOR evaluator;

    auto start = std::chrono::high_resolution_clock::now();

    for( int i = 0; i < 100; ++i )
    {
        auto result = evaluator.Evaluate( "@{vcsidentifier(7)}" );
        BOOST_CHECK( !evaluator.HasErrors() );
    }

    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>( end - start );

    // Sanitizer instrumentation changes execution cost, not the evaluation contract
#if defined( KICAD_SANITIZE_THREADS ) || defined( KICAD_SANITIZE_ADDRESS )
    BOOST_TEST_MESSAGE( "Instrumented VCS evaluation: " << duration.count() << " ms" );
#else
    BOOST_CHECK_LT( duration.count(), 2000 );
#endif
}

/**
 * Test mixed VCS and non-VCS expressions
 */
BOOST_AUTO_TEST_CASE( VcsMixedExpressions )
{
    EXPRESSION_EVALUATOR evaluator;
    evaluator.SetVariable( wxString( "PROJECT" ), wxString( "MyProject" ) );

    const std::vector<std::string> cases = {
        "Version: @{vcsbranch()}",
        "Commit: @{vcsidentifier(7)}",
        "Author: @{vcsauthor()} <@{vcsauthoremail()}>",

        "${PROJECT} @{vcsbranch()}",
        "Built from @{vcsnearestlabel()}@{vcsdirtysuffix()}",

        "Distance: @{vcslabeldistance() + 0}",
    };

    for( const auto& expr : cases )
    {
        auto result = evaluator.Evaluate( wxString::FromUTF8( expr ) );

        BOOST_CHECK( !evaluator.HasErrors() );
        BOOST_CHECK( !result.IsEmpty() );
    }
}

/**
 * Regression test for issue #23959: kicad-cli does not properly expand VCS functions.
 *
 * When the process working directory is not inside the project's git repository (the
 * typical kicad-cli situation), repo-scoped VCS queries must still resolve correctly
 * via TEXT_EVAL_VCS::SetContextPath.
 */
BOOST_AUTO_TEST_CASE( VcsContextPathOverride )
{
    // Move the process cwd somewhere that is definitely not a git repo, mirroring the
    // kicad-cli situation where the user runs the binary from an arbitrary directory.
    wxString scratchDir = wxFileName::GetTempDir() + wxFileName::GetPathSeparator()
                          + wxString::Format( "kicad_vcs_cli_%ld", wxGetProcessId() );

    wxFileName::Mkdir( scratchDir, wxS_DIR_DEFAULT, wxPATH_MKDIR_FULL );
    wxSetWorkingDirectory( scratchDir );

    auto cleanupCwd = [&]()
    {
        wxSetWorkingDirectory( tempDir() );

        if( wxFileName::DirExists( scratchDir ) )
            wxFileName::Rmdir( scratchDir, wxPATH_RMDIR_RECURSIVE );
    };

    try
    {
        // Without the context path override, vcsidentifier() falls back to cwd and reports
        // the "not in a repository" sentinel.
        EXPRESSION_EVALUATOR evaluator;
        auto                 noContext = evaluator.Evaluate( "@{vcsidentifier(7)}" );
        BOOST_CHECK_EQUAL( noContext.ToStdString(), std::string( "<unknown>" ) );

        // With the override in place, the same expression resolves from the repo anchored
        // at the temporary repository regardless of cwd.
        {
            TEXT_EVAL_VCS::CONTEXT_PATH_SCOPE scope( tempDir() );

            auto hash = evaluator.Evaluate( "@{vcsidentifier(7)}" );
            BOOST_CHECK( !evaluator.HasErrors() );
            BOOST_CHECK_EQUAL( hash.Length(), 7 );

            std::regex hexPattern( "^[0-9a-f]+$" );
            BOOST_CHECK( std::regex_match( hash.ToStdString(), hexPattern ) );

            auto branch = evaluator.Evaluate( "@{vcsbranch()}" );
            BOOST_CHECK( !evaluator.HasErrors() );
            BOOST_CHECK( !branch.IsEmpty() );
            BOOST_CHECK( branch != wxS( "<unknown>" ) );

            auto author = evaluator.Evaluate( "@{vcsauthor()}" );
            BOOST_CHECK_EQUAL( author, TEST_AUTHOR_NAME );
        }

        // After the scope ends the override must be cleared and behavior returns to the
        // cwd-based fallback.
        auto afterScope = evaluator.Evaluate( "@{vcsidentifier(7)}" );
        BOOST_CHECK_EQUAL( afterScope.ToStdString(), std::string( "<unknown>" ) );
    }
    catch( ... )
    {
        cleanupCwd();
        throw;
    }

    cleanupCwd();
}

/**
 * Integration test for issue #23959: verify the production wiring in
 * SETTINGS_MANAGER::LoadProject / UnloadProject sets and clears the VCS context
 * correctly, so a kicad-cli-style invocation (cwd outside the repo) resolves VCS
 * functions against the loaded project directory.
 */
BOOST_AUTO_TEST_CASE( VcsContextPathSetByLoadProject )
{
    wxString scratchDir = wxFileName::GetTempDir() + wxFileName::GetPathSeparator()
                          + wxString::Format( "kicad_vcs_loadproject_%ld", wxGetProcessId() );

    wxFileName::Mkdir( scratchDir, wxS_DIR_DEFAULT, wxPATH_MKDIR_FULL );
    wxSetWorkingDirectory( scratchDir );

    // Drop a minimal .kicad_pro inside the git fixture so SETTINGS_MANAGER has something
    // real to load. The content does not matter for VCS resolution.
    wxString projectFile = tempDir() + wxFileName::GetPathSeparator()
                           + wxT( "issue23959.kicad_pro" );

    {
        std::ofstream f( projectFile.ToStdString() );
        f << "{ \"meta\": { \"filename\": \"issue23959.kicad_pro\", \"version\": 3 } }";
    }

    auto cleanup = [&]()
    {
        wxRemoveFile( projectFile );
        wxSetWorkingDirectory( tempDir() );

        if( wxFileName::DirExists( scratchDir ) )
            wxFileName::Rmdir( scratchDir, wxPATH_RMDIR_RECURSIVE );
    };

    try
    {
        // Baseline: with cwd outside the repo and no project loaded, lookups fail.
        EXPRESSION_EVALUATOR evaluator;
        auto                 baseline = evaluator.Evaluate( "@{vcsidentifier(7)}" );
        BOOST_CHECK_EQUAL( baseline.ToStdString(), std::string( "<unknown>" ) );

        BOOST_REQUIRE( Pgm().GetSettingsManager().LoadProject( projectFile, true ) );

        // LoadProject must anchor VCS lookups to the project dir even though cwd is elsewhere.
        auto loaded = evaluator.Evaluate( "@{vcsidentifier(7)}" );
        BOOST_CHECK( !evaluator.HasErrors() );
        BOOST_CHECK_EQUAL( loaded.Length(), 7 );

        std::regex hexPattern( "^[0-9a-f]+$" );
        BOOST_CHECK( std::regex_match( loaded.ToStdString(), hexPattern ) );

        Pgm().GetSettingsManager().UnloadProject( &Pgm().GetSettingsManager().Prj(), false );
        BOOST_REQUIRE( wxSetWorkingDirectory( scratchDir ) );

        // UnloadProject must clear the context so lookups fall back to cwd, which is
        // outside the repo, reproducing the sentinel state.
        auto afterUnload = evaluator.Evaluate( "@{vcsidentifier(7)}" );
        BOOST_CHECK_EQUAL( afterUnload.ToStdString(), std::string( "<unknown>" ) );
    }
    catch( ... )
    {
        cleanup();
        throw;
    }

    cleanup();
}


// The context is process-wide, so a manager that dies with a project loaded leaves it stale.
BOOST_AUTO_TEST_CASE( VcsContextClearedWhenSettingsManagerDies )
{
    wxString projectPath = tempDir() + wxFileName::GetPathSeparator() + wxT( "ctx.kicad_pro" );

    {
        std::ofstream f( projectPath.ToStdString() );
        f << "{\"meta\": {\"filename\": \"ctx.kicad_pro\", \"version\": 3}}";
    }

    {
        SETTINGS_MANAGER mgr;
        BOOST_TEST_REQUIRE( mgr.LoadProject( projectPath ) );
        BOOST_CHECK( TEXT_EVAL_VCS::GetContextPath() != wxT( "." ) );
    }

    BOOST_CHECK_EQUAL( TEXT_EVAL_VCS::GetContextPath().ToStdString(), std::string( "." ) );
}

BOOST_AUTO_TEST_SUITE_END()
