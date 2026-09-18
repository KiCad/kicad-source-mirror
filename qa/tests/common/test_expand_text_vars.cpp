/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation, either version 3 of the License, or (at your
 * option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */


#define BOOST_TEST_NO_MAIN
#include <boost/test/unit_test.hpp>
#include <qa_utils/file_utils.h>
#include <qa_utils/env_var_utils.h>

#include <atomic>
#include <optional>
#include <thread>
#include <vector>

#include <wx/filename.h>
#include <wx/utils.h>

#include <common.h>
#include <env_paths.h>
#include <env_vars.h>
#include <jobs/job_export_pcb_stats.h>
#include <pgm_base.h>
#include <settings/environment.h>
#include <title_block.h>

/**
 * Test fixture for ExpandTextVars tests
 */
struct ExpandTextVarsFixture
{
    // Simple resolver that maps VAR->value, X->5, Y->2
    std::function<bool( wxString* )> resolver =
            []( wxString* token ) -> bool
            {
                if( *token == wxT( "VAR" ) )
                {
                    *token = wxT( "value" );
                    return true;
                }
                else if( *token == wxT( "X" ) )
                {
                    *token = wxT( "5" );
                    return true;
                }
                else if( *token == wxT( "Y" ) )
                {
                    *token = wxT( "2" );
                    return true;
                }

                return false;
            };
};

BOOST_FIXTURE_TEST_SUITE( ExpandTextVarsTests, ExpandTextVarsFixture )

BOOST_AUTO_TEST_CASE( AssertionsRemainAvailableOnlyDuringRuleChecks )
{
    for( const wxString& prefix : { wxString( "ERC_WARNING" ), wxString( "ERC_ERROR" ),
                                    wxString( "DRC_WARNING" ), wxString( "DRC_ERROR" ) } )
    {
        const wxString assertion = "${" + prefix + " Check this item}";
        BOOST_CHECK_EQUAL( ExpandTextVars( assertion, &resolver, FOR_ERC_DRC ), assertion );
        BOOST_CHECK( ExpandTextVars( assertion, &resolver, FOR_CANVAS ).empty() );
        BOOST_CHECK_EQUAL( ExpandTextVars( assertion, &resolver, RAW_VALUE ), assertion );
    }
}


// Basic variable expansion
BOOST_AUTO_TEST_CASE( SimpleVariable )
{
    wxString result = ExpandTextVars( wxT( "${VAR}" ), &resolver, INTERNAL );
    BOOST_CHECK( result == wxT( "value" ) );
}

// Multiple variables in one string
BOOST_AUTO_TEST_CASE( MultipleVariables )
{
    wxString result = ExpandTextVars( wxT( "${X}+${Y}" ), &resolver, INTERNAL );
    BOOST_CHECK( result == wxT( "5+2" ) );
}

// Escaped variable should produce escape marker (not expanded)
BOOST_AUTO_TEST_CASE( EscapedVariable )
{
    wxString result = ExpandTextVars( wxT( "\\${VAR}" ), &resolver, INTERNAL );
    // The escape marker should be in the output
    BOOST_CHECK( result.Contains( wxT( "<<<ESC_DOLLAR:" ) ) );
}

// Escaped variable followed by regular variable - both should be processed correctly
BOOST_AUTO_TEST_CASE( EscapedThenRegularVariable )
{
    wxString result = ExpandTextVars( wxT( "\\${literal}${VAR}" ), &resolver, INTERNAL );
    // Should have escape marker for literal, and "value" for VAR
    BOOST_CHECK( result.Contains( wxT( "<<<ESC_DOLLAR:" ) ) );
    BOOST_CHECK( result.Contains( wxT( "value" ) ) );
}

// Regular variable followed by escaped variable
BOOST_AUTO_TEST_CASE( RegularThenEscapedVariable )
{
    wxString result = ExpandTextVars( wxT( "${VAR}\\${literal}" ), &resolver, INTERNAL );
    // Should have "value" for VAR and escape marker for literal
    BOOST_CHECK( result.StartsWith( wxT( "value" ) ) );
    BOOST_CHECK( result.Contains( wxT( "<<<ESC_DOLLAR:" ) ) );
}

// Issue 22497: Escaped variable inside math expression should not prevent other expansions
// This is the key test case for the bug fix
BOOST_AUTO_TEST_CASE( EscapedInsideMathExpression )
{
    // First pass: @{\${X}+${Y}} should become @{<<<ESC_DOLLAR:X}+2}
    // Second pass: the marker should be preserved and +2 should NOT be lost
    wxString result = ExpandTextVars( wxT( "@{\\${X}+${Y}}" ), &resolver, INTERNAL );

    // The result should contain the escape marker
    BOOST_CHECK_MESSAGE( result.Contains( wxT( "<<<ESC_DOLLAR:" ) ),
                         "Expected escape marker in result" );

    // The result should also contain +2 (the expanded Y variable)
    BOOST_CHECK_MESSAGE( result.Contains( wxT( "+2" ) ),
                         "Expected '+2' (from ${Y} expansion) in result" );

    // The result should be @{<<<ESC_DOLLAR:X}+2}
    BOOST_CHECK( result == wxT( "@{<<<ESC_DOLLAR:X}+2}" ) );
}

// Nested escaped variable in regular variable reference
BOOST_AUTO_TEST_CASE( EscapedInsideVariableReference )
{
    // ${prefix\${suffix}} - looking up variable with literal ${suffix} in name
    // This should try to resolve "prefix\${suffix}" which won't resolve,
    // but the recursive expansion should convert \${suffix} to the marker
    wxString result = ExpandTextVars( wxT( "${prefix\\${suffix}}" ), &resolver, INTERNAL );

    // The unresolved reference should be preserved with escape marker
    BOOST_CHECK( result.Contains( wxT( "<<<ESC_DOLLAR:" ) ) );
}

// Multiple escape markers in a math expression
BOOST_AUTO_TEST_CASE( MultipleEscapedInMathExpression )
{
    wxString result = ExpandTextVars( wxT( "@{\\${A}+\\${B}+${Y}}" ), &resolver, INTERNAL );

    // Should have two escape markers and the expanded Y (2)
    BOOST_CHECK_MESSAGE( result.Contains( wxT( "+2" ) ),
                         "Expected '+2' (from ${Y} expansion) in: " + result );

    // Count escape markers (should be 2)
    int dollarCount = 0;
    size_t pos = 0;

    while( ( pos = result.find( wxT( "<<<ESC_DOLLAR:" ), pos ) ) != wxString::npos )
    {
        dollarCount++;
        pos += 14;
    }

    BOOST_CHECK_EQUAL( dollarCount, 2 );
}

// Math expression with escaped @ sign
BOOST_AUTO_TEST_CASE( EscapedAtInExpression )
{
    wxString result = ExpandTextVars( wxT( "${VAR}\\@{literal}" ), &resolver, INTERNAL );

    // Should have "value" for VAR and escape marker for @{literal}
    BOOST_CHECK( result.StartsWith( wxT( "value" ) ) );
    BOOST_CHECK( result.Contains( wxT( "<<<ESC_AT:" ) ) );
}

// Escaped followed by escaped (both should be preserved)
BOOST_AUTO_TEST_CASE( ConsecutiveEscaped )
{
    wxString result = ExpandTextVars( wxT( "\\${A}\\${B}" ), &resolver, INTERNAL );

    // Should have two escape markers
    int dollarCount = 0;
    size_t pos = 0;

    while( ( pos = result.find( wxT( "<<<ESC_DOLLAR:" ), pos ) ) != wxString::npos )
    {
        dollarCount++;
        pos += 14;
    }

    BOOST_CHECK_EQUAL( dollarCount, 2 );
}

// Issue 23599: backslash path separator before text variable should NOT be treated as an escape.
// This test documents the ExpandTextVars behavior: \${ IS treated as an escape at this level.
// The fix is applied at call sites that deal with file paths, which normalize backslashes to
// forward slashes before calling ExpandTextVars.
BOOST_AUTO_TEST_CASE( BackslashBeforeVariableIsEscape )
{
    wxString result = ExpandTextVars( wxT( "subdir\\${VAR}_file.txt" ), &resolver, INTERNAL );

    // ExpandTextVars treats \${ as an escape, so VAR is NOT expanded
    BOOST_CHECK( result.Contains( wxT( "<<<ESC_DOLLAR:" ) ) );
}


// With forward slashes the variable is expanded normally
BOOST_AUTO_TEST_CASE( ForwardSlashBeforeVariableExpands )
{
    wxString result = ExpandTextVars( wxT( "subdir/${VAR}_file.txt" ), &resolver, INTERNAL );

    BOOST_CHECK( result == wxT( "subdir/value_file.txt" ) );
}

BOOST_AUTO_TEST_SUITE_END()


// Issue 24776: user-entered file paths with a text variable immediately after a backslash
// separator (e.g. the Symbol Fields Table BOM export path "Output\BoM\${PROJECTNAME}.csv")
// must expand. Callers route the path through NormalizeFilePathForTextVars before ExpandTextVars.
BOOST_FIXTURE_TEST_SUITE( NormalizeFilePathForTextVarsTests, ExpandTextVarsFixture )

BOOST_AUTO_TEST_CASE( BackslashSeparatorBeforeVarExpands )
{
    wxString path = NormalizeFilePathForTextVars( wxT( "Output\\BoM\\${VAR}_file.csv" ) );
    wxString result = ExpandTextVars( path, &resolver, INTERNAL );

    // Only the backslash immediately before the variable is rewritten to a separator; the
    // variable expands and the earlier literal backslash is preserved.
    BOOST_CHECK_MESSAGE( !result.Contains( wxT( "<<<ESC_DOLLAR:" ) ),
                         "Variable after backslash separator should expand, not escape. Got: " + result );
    BOOST_CHECK( result == wxT( "Output\\BoM/value_file.csv" ) );
}


BOOST_AUTO_TEST_CASE( MultipleVarsAfterBackslashSeparators )
{
    wxString path = NormalizeFilePathForTextVars( wxT( "Output\\BoM\\${X}_V${Y}.csv" ) );
    wxString result = ExpandTextVars( path, &resolver, INTERNAL );

    BOOST_CHECK( result == wxT( "Output\\BoM/5_V2.csv" ) );
}


BOOST_AUTO_TEST_CASE( ForwardSlashPathUnchanged )
{
    wxString path = NormalizeFilePathForTextVars( wxT( "Output/BoM/${VAR}.csv" ) );
    wxString result = ExpandTextVars( path, &resolver, INTERNAL );

    BOOST_CHECK( result == wxT( "Output/BoM/value.csv" ) );
}


// Backslashes that do not immediately precede a text variable are a legitimate part of the
// filename (notably on POSIX) and must survive normalization unchanged.
BOOST_AUTO_TEST_CASE( NonVariableBackslashesArePreserved )
{
    wxString path = NormalizeFilePathForTextVars( wxT( "Output\\BoM\\literal.csv" ) );

    BOOST_CHECK( path == wxT( "Output\\BoM\\literal.csv" ) );
}

BOOST_AUTO_TEST_SUITE_END()


// Issue 23599: JOB::ResolveOutputPath must expand text variables even when preceded by backslash
// path separators. This suite tests the fix in ResolveOutputPath that normalizes backslashes
// before calling ExpandTextVars.
BOOST_AUTO_TEST_SUITE( JobResolveOutputPath )

BOOST_AUTO_TEST_CASE( BackslashPathSeparatorBeforeTextVar )
{
    JOB_EXPORT_PCB_STATS job;

    TITLE_BLOCK titleBlock;
    titleBlock.SetRevision( wxT( "RevA" ) );
    job.SetTitleBlock( titleBlock );

    // Simulates Windows path with text variable immediately after backslash separator
    wxString path = wxT( "Board Stats\\${REVISION}_Stats.txt" );
    wxString result = job.ResolveOutputPath( path, false, nullptr );

    // The variable should be expanded, not escaped
    BOOST_CHECK_MESSAGE( !result.Contains( wxT( "<<<ESC_DOLLAR:" ) ),
                         "Text variable after backslash path separator should not be escaped. Got: "
                                 + result );
    BOOST_CHECK_MESSAGE( result.Contains( wxT( "RevA" ) ),
                         "Expected resolved REVISION in path. Got: " + result );
}


BOOST_AUTO_TEST_CASE( BackslashPathSeparatorBeforeMultipleTextVars )
{
    JOB_EXPORT_PCB_STATS job;

    TITLE_BLOCK titleBlock;
    titleBlock.SetRevision( wxT( "B" ) );
    titleBlock.SetComment( 0, wxT( "DWG-001" ) );
    job.SetTitleBlock( titleBlock );

    // The COMMENT1 variable maps to Comment(0) in TITLE_BLOCK
    wxString path = wxT( "Output\\${COMMENT1}_${REVISION}_file.txt" );
    wxString result = job.ResolveOutputPath( path, false, nullptr );

    BOOST_CHECK_MESSAGE( result.Contains( wxT( "DWG-001" ) ),
                         "Expected COMMENT1 expanded in path. Got: " + result );
    BOOST_CHECK_MESSAGE( result.Contains( wxT( "_B_" ) ),
                         "Expected REVISION expanded in path. Got: " + result );
}


BOOST_AUTO_TEST_CASE( TextVarNotFirstInFilename )
{
    JOB_EXPORT_PCB_STATS job;

    TITLE_BLOCK titleBlock;
    titleBlock.SetRevision( wxT( "C" ) );
    job.SetTitleBlock( titleBlock );

    // When there's literal text between the backslash and the variable, it always worked
    wxString path = wxT( "Output\\Generated_${REVISION}_file.txt" );
    wxString result = job.ResolveOutputPath( path, false, nullptr );

    BOOST_CHECK_MESSAGE( result.Contains( wxT( "Generated_C_file.txt" ) ),
                         "Expected variable expansion with preceding literal text. Got: " + result );
}


/**
 * Regression tests for overlapping-prefix environment variables. Reproduces the scenario
 * from a KiCad V10 bug report where users set both a versioned package path and a narrower
 * user library path that is a descendant of the package path, e.g.
 *
 *   KICAD10_3RD_PARTY = /tmp/kicadlib
 *   KICAD_USER_LIB    = /tmp/kicadlib/V10
 *
 * Creating a symbol library at `${KICAD_USER_LIB}/symbols/test.kicad_sym` round-trips
 * through NormalizePath (at InsertRow time) and ExpandEnvVarSubstitutions (at plugin
 * CreateLibrary time). If the round-trip loses fidelity, the plugin tries to open a
 * malformed path and fails with a generic "Could not create the library file" error.
 */
BOOST_AUTO_TEST_CASE( OverlappingPrefix_NormalizePicksLongestPrefix )
{
    // NormalizePath only considers env var paths that exist as readable directories.
    KI_TEST::SCOPED_TEMP_DIR tempDir( wxS( "overlapping-env-vars" ) );

    const wxString   innerDir = tempDir.CreateChildDirStr( "V10" );       //  .../V10
    const wxFileName targetFilename( innerDir, wxS( "test.kicad_sym" ) ); // ..../V10/test.kicad_sym

    KI_TEST::SCOPED_PGM_ENV_VAR scopedOuterEnvVar( wxS( "KICAD_QA_3RD_PARTY_OUTER" ), tempDir.PathStr() );
    KI_TEST::SCOPED_PGM_ENV_VAR scopedInnerEnvVar( wxS( "KICAD_QA_USER_LIB_INNER" ), innerDir );

    // ---- Setup done

    // NormalizePath should pick KICAD_QA_USER_LIB_INNER because it is a deeper match.
    const wxString normalized = NormalizePath( targetFilename, &Pgm().GetLocalEnvVariables(), wxEmptyString );
    const wxString expected = wxS( "${KICAD_QA_USER_LIB_INNER}/test.kicad_sym" );

    BOOST_CHECK_EQUAL( normalized, expected );
}


BOOST_AUTO_TEST_SUITE_END()


/**
 * Regression test for KiCad GitLab issue #23962.
 *
 * ResolveTextVars and ExpandTextVars used a function-local static EXPRESSION_EVALUATOR
 * shared across all threads. CONNECTION_GRAPH evaluates schematic labels in parallel
 * via a thread pool and label text may contain @{...} expressions, which caused the
 * shared evaluator's internal ERROR_COLLECTOR vector to be mutated concurrently, leading
 * to heap corruption and a segfault inside std::vector::clear.
 *
 * This test hammers ResolveTextVars from many threads to make the race observable,
 * primarily under TSan. It also serves as a smoke test that no thread crashes.
 */
BOOST_AUTO_TEST_SUITE( TextVarExpressionEvaluatorConcurrency )

BOOST_AUTO_TEST_CASE( ParallelResolveTextVarsWithMathExpressions )
{
    std::function<bool( wxString* )> resolver =
            []( wxString* token ) -> bool
            {
                if( *token == wxT( "#" ) )
                {
                    *token = wxT( "3" );
                    return true;
                }

                if( *token == wxT( "ROW" ) )
                {
                    *token = wxT( "4" );
                    return true;
                }

                return false;
            };

    const std::vector<wxString> inputs = {
        wxT( "Out@{(${#}-2)*8+0}" ),
        wxT( "Net_@{${ROW}*2+1}" ),
        wxT( "@{(2-2)*8+0}" ),
        wxT( "${ROW}:@{${ROW}*${ROW}}" ),
        wxT( "plain_label_no_expr" ),
        wxT( "@{1+1}_@{2+2}_@{3+3}" ),
    };

    const unsigned int numThreads = std::max( 4u, std::thread::hardware_concurrency() );
    const int          iterations = 2000;

    std::atomic<bool>        failed{ false };
    std::atomic<int>         totalRuns{ 0 };
    std::vector<std::thread> threads;
    threads.reserve( numThreads );

    for( unsigned int t = 0; t < numThreads; ++t )
    {
        threads.emplace_back(
                [&, t]()
                {
                    try
                    {
                        for( int i = 0; i < iterations; ++i )
                        {
                            const wxString& src = inputs[( t + i ) % inputs.size()];
                            int             depth = 0;
                            wxString        result = ResolveTextVars( src, &resolver, depth );
                            (void) result;
                            totalRuns.fetch_add( 1, std::memory_order_relaxed );
                        }
                    }
                    catch( ... )
                    {
                        failed.store( true, std::memory_order_relaxed );
                    }
                } );
    }

    for( auto& th : threads )
        th.join();

    BOOST_CHECK( !failed.load() );
    BOOST_CHECK_EQUAL( totalRuns.load(), static_cast<int>( numThreads ) * iterations );
}

BOOST_AUTO_TEST_SUITE_END()


BOOST_AUTO_TEST_SUITE( EnvVarResolution )


BOOST_AUTO_TEST_CASE( ExpandsNestedReferences )
{
    const wxString innerPath = "/inner/path";

    KI_TEST::SCOPED_PROCESS_ENV_VAR scopedInnerEnvVar( wxS( "KICAD_QA_INNER" ), innerPath );
    KI_TEST::SCOPED_PROCESS_ENV_VAR scopedOuterEnvVar( wxS( "KICAD_QA_OUTER" ), wxS( "${KICAD_QA_INNER}/templates" ) );

    wxString expanded = ExpandEnvVarSubstitutions( wxS( "${KICAD_QA_OUTER}" ), nullptr );
    wxString expected = innerPath + wxS( "/templates" );

    // After expansion the inner reference must be resolved to its concrete path.
    BOOST_CHECK_EQUAL( expanded, expected );
}


BOOST_AUTO_TEST_CASE( UndefinedReferenceLeavesLiteralMarker )
{
    // If a referenced variable is undefined, ExpandEnvVarSubstitutions preserves the
    // original token.  Callers that then mkdir the result would create a literal
    // "${MISSING}" directory; production code must detect this and bail out.

    const wxString innerPath = "/inner/path";

    KI_TEST::SCOPED_PROCESS_ENV_VAR scopedInnerEnvVar( wxS( "KICAD_QA_INNER" ), std::nullopt );
    KI_TEST::SCOPED_PROCESS_ENV_VAR scopedOuterEnvVar( wxS( "KICAD_QA_OUTER" ), wxS( "${KICAD_QA_INNER}/templates" ) );

    wxString expanded = ExpandEnvVarSubstitutions( wxS( "${KICAD_QA_OUTER}" ), nullptr );
    BOOST_CHECK_EQUAL( expanded, "${KICAD_QA_INNER}/templates" );
}


BOOST_AUTO_TEST_CASE( UserVarIsNotTreatedAsVersionedLibraryDir )
{
    const wxString stockLibPath = "/some/path/to/stock-libraries";
    const wxString versionedName = ENV_VAR::GetVersionedEnvVarName( wxS( "FOOTPRINT_DIR" ) );

    // Clear the user var
    KI_TEST::SCOPED_PROCESS_ENV_VAR scopedUserEnvVar( wxS( "KICAD_USER_FOOTPRINT_DIR" ), std::nullopt );
    // Set the versioned var to some specific path
    KI_TEST::SCOPED_PROCESS_ENV_VAR scopedVersionedEnvVar( versionedName, stockLibPath );

    const wxString uri = wxS( "${KICAD_USER_FOOTPRINT_DIR}/conn_custom.pretty" );

    wxString expanded = ExpandEnvVarSubstitutions( uri, nullptr );

    // An unresolved user var must stay literal, never fall back to the library directory.
    BOOST_CHECK_EQUAL( expanded, uri );
}


/*
 * A project or board from an older KiCad may reference an older versioned library variable
 * (for example ${KICAD5_FOOTPRINT_DIR} from a KiCad 5 project).  If that exact variable is not
 * set on this machine, the expander falls back to the current install's equivalent so the path
 * still resolves.
 */
BOOST_AUTO_TEST_CASE( LegacyVersionedVarStillResolvesToCurrentDir )
{
    const wxString libPath = "/some/path/to/stock-libraries";
    const wxString currentVersionedName = ENV_VAR::GetVersionedEnvVarName( wxS( "FOOTPRINT_DIR" ) );

    // Condition: the legacy var is not the same as the current versioned var.
    BOOST_REQUIRE( wxS( "KICAD5_FOOTPRINT_DIR" ) != currentVersionedName );

    // Ensure legacy var is unset so the direct lookup fails and the fallback runs.
    KI_TEST::SCOPED_PROCESS_ENV_VAR scopedLegacyEnvVar( wxS( "KICAD5_FOOTPRINT_DIR" ), std::nullopt );
    // Set the current versioned var, which is what the fallback resolves to.
    KI_TEST::SCOPED_PROCESS_ENV_VAR scopedVersionedEnvVar( currentVersionedName, libPath );

    wxString expanded =
            ExpandEnvVarSubstitutions( wxS( "${KICAD5_FOOTPRINT_DIR}/conn_custom.pretty" ), nullptr );

    // The missing fallback resolves to the current versioned footprint directory.
    BOOST_CHECK_EQUAL( expanded, libPath + wxS( "/conn_custom.pretty" ) );
}


BOOST_AUTO_TEST_CASE( DeprecatedUnversionedAliasStillResolves )
{
    // KICAD_SYMBOL_DIR is a documented deprecated alias for the versioned symbol dir.
    // It must still fall back to the current install even though it carries no
    // version info.
    const wxString libPath = "/some/path/to/stock-libraries";
    const wxString currentVersionedName = ENV_VAR::GetVersionedEnvVarName( wxS( "SYMBOL_DIR" ) );

    // Ensure the "correct" alias is unset so the direct lookup fails and the fallback runs.
    KI_TEST::SCOPED_PROCESS_ENV_VAR scopedAliasEnvVar( wxS( "KICAD_SYMBOL_DIR" ), std::nullopt );
    // Set the current versioned var, which is what the fallback resolves to.
    KI_TEST::SCOPED_PROCESS_ENV_VAR scopedVersionedEnvVar( currentVersionedName, libPath );

    // A user-provided path that uses the deprecated alias.
    wxString expanded =
            ExpandEnvVarSubstitutions( wxS( "${KICAD_SYMBOL_DIR}/Device.kicad_sym" ), nullptr );

    // Should have resolved to the current versioned var's value.
    BOOST_CHECK_EQUAL( expanded, libPath + wxS( "/Device.kicad_sym" ) );
}


BOOST_AUTO_TEST_CASE( IsVersionedEnvVarPredicate )
{
    BOOST_CHECK( ENV_VAR::IsVersionedEnvVar( wxS( "KICAD7_FOOTPRINT_DIR" ), wxS( "FOOTPRINT_DIR" ) ) );
    BOOST_CHECK( ENV_VAR::IsVersionedEnvVar( wxS( "KICAD10_FOOTPRINT_DIR" ), wxS( "FOOTPRINT_DIR" ) ) );

    BOOST_CHECK( !ENV_VAR::IsVersionedEnvVar( wxS( "KICAD_USER_FOOTPRINT_DIR" ), wxS( "FOOTPRINT_DIR" ) ) );
    BOOST_CHECK( !ENV_VAR::IsVersionedEnvVar( wxS( "KICAD_FOOTPRINT_DIR" ), wxS( "FOOTPRINT_DIR" ) ) );
    BOOST_CHECK( !ENV_VAR::IsVersionedEnvVar( wxS( "KICAD7_SYMBOL_DIR" ), wxS( "FOOTPRINT_DIR" ) ) );
}

BOOST_AUTO_TEST_SUITE_END()
