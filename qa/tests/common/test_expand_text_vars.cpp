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
 * You should have received a copy of the GNU General Public License along
 * with this program.  If not, see <http://www.gnu.org/licenses/>.
 */


#define BOOST_TEST_NO_MAIN
#include <boost/test/unit_test.hpp>
#include <filesystem>
#include <common.h>
#include <env_paths.h>
#include <pgm_base.h>
#include <settings/environment.h>
#include <wx/filename.h>
#include <wx/utils.h>

/**
 * Test fixture for ExpandTextVars tests
 */
struct ExpandTextVarsFixture
{
    // Simple resolver that maps VAR->value, X->5, Y->2
    std::function<bool( wxString* )> resolver = []( wxString* token ) -> bool
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

// Basic variable expansion
BOOST_AUTO_TEST_CASE( SimpleVariable )
{
    wxString result = ExpandTextVars( wxT( "${VAR}" ), &resolver );
    BOOST_CHECK( result == wxT( "value" ) );
}

// Multiple variables in one string
BOOST_AUTO_TEST_CASE( MultipleVariables )
{
    wxString result = ExpandTextVars( wxT( "${X}+${Y}" ), &resolver );
    BOOST_CHECK( result == wxT( "5+2" ) );
}

// Escaped variable should produce escape marker (not expanded)
BOOST_AUTO_TEST_CASE( EscapedVariable )
{
    wxString result = ExpandTextVars( wxT( "\\${VAR}" ), &resolver );
    // The escape marker should be in the output
    BOOST_CHECK( result.Contains( wxT( "<<<ESC_DOLLAR:" ) ) );
}

// Escaped variable followed by regular variable - both should be processed correctly
BOOST_AUTO_TEST_CASE( EscapedThenRegularVariable )
{
    wxString result = ExpandTextVars( wxT( "\\${literal}${VAR}" ), &resolver );
    // Should have escape marker for literal, and "value" for VAR
    BOOST_CHECK( result.Contains( wxT( "<<<ESC_DOLLAR:" ) ) );
    BOOST_CHECK( result.Contains( wxT( "value" ) ) );
}

// Regular variable followed by escaped variable
BOOST_AUTO_TEST_CASE( RegularThenEscapedVariable )
{
    wxString result = ExpandTextVars( wxT( "${VAR}\\${literal}" ), &resolver );
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
    wxString result = ExpandTextVars( wxT( "@{\\${X}+${Y}}" ), &resolver );

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
    wxString result = ExpandTextVars( wxT( "${prefix\\${suffix}}" ), &resolver );

    // The unresolved reference should be preserved with escape marker
    BOOST_CHECK( result.Contains( wxT( "<<<ESC_DOLLAR:" ) ) );
}

// Multiple escape markers in a math expression
BOOST_AUTO_TEST_CASE( MultipleEscapedInMathExpression )
{
    wxString result = ExpandTextVars( wxT( "@{\\${A}+\\${B}+${Y}}" ), &resolver );

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
    wxString result = ExpandTextVars( wxT( "${VAR}\\@{literal}" ), &resolver );

    // Should have "value" for VAR and escape marker for @{literal}
    BOOST_CHECK( result.StartsWith( wxT( "value" ) ) );
    BOOST_CHECK( result.Contains( wxT( "<<<ESC_AT:" ) ) );
}

// Escaped followed by escaped (both should be preserved)
BOOST_AUTO_TEST_CASE( ConsecutiveEscaped )
{
    wxString result = ExpandTextVars( wxT( "\\${A}\\${B}" ), &resolver );

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

BOOST_AUTO_TEST_SUITE_END()


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
struct OverlappingEnvVarsFixture
{
    wxString rootDir;
    wxString outerDir;
    wxString innerDir;
    wxString targetDir;

    OverlappingEnvVarsFixture()
    {
        std::filesystem::path tmp = std::filesystem::temp_directory_path() /
                                     "kicad_qa_overlap_env_vars";
        std::error_code       ec;
        std::filesystem::remove_all( tmp, ec );
        std::filesystem::create_directories( tmp / "V10" / "symbols", ec );

        rootDir   = wxString::FromUTF8( tmp.string() );
        outerDir  = rootDir;
        innerDir  = wxString::FromUTF8( ( tmp / "V10" ).string() );
        targetDir = wxString::FromUTF8( ( tmp / "V10" / "symbols" ).string() );

        wxSetEnv( wxS( "KICAD_QA_3RD_PARTY_OUTER" ), outerDir );
        wxSetEnv( wxS( "KICAD_QA_USER_LIB_INNER" ), innerDir );
    }

    ~OverlappingEnvVarsFixture()
    {
        wxUnsetEnv( wxS( "KICAD_QA_3RD_PARTY_OUTER" ) );
        wxUnsetEnv( wxS( "KICAD_QA_USER_LIB_INNER" ) );

        std::filesystem::path tmp = std::filesystem::temp_directory_path() /
                                     "kicad_qa_overlap_env_vars";
        std::error_code ec;
        std::filesystem::remove_all( tmp, ec );
    }

    ENV_VAR_MAP BuildEnvMap() const
    {
        ENV_VAR_MAP map;
        map[wxS( "KICAD_QA_3RD_PARTY_OUTER" )] = ENV_VAR_ITEM( outerDir );
        map[wxS( "KICAD_QA_USER_LIB_INNER" )]  = ENV_VAR_ITEM( innerDir );
        return map;
    }
};


BOOST_FIXTURE_TEST_SUITE( OverlappingEnvVarPaths, OverlappingEnvVarsFixture )


BOOST_AUTO_TEST_CASE( NormalizePicksLongestPrefix )
{
    wxFileName target( targetDir, wxS( "test.kicad_sym" ) );
    ENV_VAR_MAP envMap = BuildEnvMap();

    wxString normalized = NormalizePath( target, &envMap, wxEmptyString );

    // NormalizePath should pick KICAD_QA_USER_LIB_INNER because it is a deeper match.
    BOOST_CHECK_MESSAGE(
            normalized == wxS( "${KICAD_QA_USER_LIB_INNER}/symbols/test.kicad_sym" ),
            wxString::Format( wxS( "Expected '%s' but got '%s'" ),
                              wxS( "${KICAD_QA_USER_LIB_INNER}/symbols/test.kicad_sym" ),
                              normalized ) );
}


BOOST_AUTO_TEST_CASE( RoundTripPreservesAbsolutePath )
{
    wxFileName target( targetDir, wxS( "test.kicad_sym" ) );
    ENV_VAR_MAP envMap = BuildEnvMap();

    wxString normalized = NormalizePath( target, &envMap, wxEmptyString );
    wxString expanded   = ExpandEnvVarSubstitutions( normalized, nullptr );

    wxFileName expandedFn( expanded );
    expandedFn.Normalize( wxPATH_NORM_DOTS | wxPATH_NORM_ABSOLUTE );

    wxFileName originalFn( target );
    originalFn.Normalize( wxPATH_NORM_DOTS | wxPATH_NORM_ABSOLUTE );

    BOOST_CHECK_MESSAGE(
            expandedFn.GetFullPath() == originalFn.GetFullPath(),
            wxString::Format(
                    wxS( "Round-trip mismatch: normalized='%s' expanded='%s' original='%s'" ),
                    normalized, expandedFn.GetFullPath(), originalFn.GetFullPath() ) );
}


BOOST_AUTO_TEST_SUITE_END()
