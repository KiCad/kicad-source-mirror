/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
 * @author Jon Evans <jon@craftyjon.com>
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

#include <filesystem>
#include <fstream>

#include <mock_pgm_base.h>
#include <richio.h>
#include <io/kicad/kicad_io_utils.h>
#include <qa_utils/wx_utils/unit_test_utils.h>
#include <settings/settings_manager.h>
#include <pegtl/contrib/analyze.hpp>

#include <libraries/library_manager.h>
#include <libraries/library_table.h>
#include <libraries/library_table_parser.h>
#include <libraries/library_table_grammar.h>


BOOST_AUTO_TEST_SUITE( LibraryTables )


BOOST_AUTO_TEST_CASE( Grammar )
{
    BOOST_REQUIRE( tao::pegtl::analyze< LIBRARY_TABLE_GRAMMAR::LIB_TABLE_FILE >( 1 ) == 0 );
}


BOOST_AUTO_TEST_CASE( EmptyString )
{
    LIBRARY_TABLE_PARSER parser;
    tl::expected<LIBRARY_TABLE_IR, LIBRARY_PARSE_ERROR> result = parser.ParseBuffer( "" );
    BOOST_REQUIRE( !result.has_value() );
}


BOOST_AUTO_TEST_CASE( ParseFromFile )
{
    std::vector<std::string> cases = {
        "sym-lib-table",
        "fp-lib-table"
    };

    std::filesystem::path p( KI_TEST::GetTestDataRootDir() );
    p.append( "libraries/" );

    LIBRARY_TABLE_PARSER parser;

    for( const std::string& path : cases )
    {
        p.remove_filename();
        p.append( path );

        auto result = parser.Parse( p );

        BOOST_REQUIRE( result.has_value() );
    }
}


BOOST_AUTO_TEST_CASE( ParseAndConstruct )
{
    struct TESTCASE
    {
        wxString filename;
        wxString expected_error;
        size_t expected_rows;
        bool check_formatted = true;
    };

    std::vector<TESTCASE> cases = {
        { .filename = "sym-lib-table", .expected_rows = 224 },
        { .filename = "fp-lib-table", .expected_rows = 146 },
        { .filename = "nested-symbols", .expected_rows = 6 },
        { .filename = "nested-disabled", .expected_rows = 4 },
        { .filename = "nested-hidden", .expected_rows = 4 },
        { .filename = "cycle", .expected_rows = 2 },
        { .filename = "sym-hand-edited", .expected_rows = 2, .check_formatted = false },
        { .filename = "corrupted", .expected_error = "Syntax error at line 6, column 9" },
        { .filename = "truncated", .expected_error = "Syntax error at line 11, column 1" }
    };

    wxFileName fn( KI_TEST::GetTestDataRootDir(), wxEmptyString );
    fn.AppendDir( "libraries" );

    for( const auto& [filename, expected_error, expected_rows, check_formatted] : cases )
    {
        BOOST_TEST_CONTEXT( filename )
        {
            fn.SetName( filename );
            LIBRARY_TABLE table( fn, LIBRARY_TABLE_SCOPE::GLOBAL );

            BOOST_REQUIRE( table.IsOk() == ( expected_error.IsEmpty() ) );

            BOOST_REQUIRE_MESSAGE( table.Rows().size() == expected_rows,
                                   wxString::Format( "Expected %zu rows but got %zu",
                                                     expected_rows, table.Rows().size() ) );

            BOOST_REQUIRE_MESSAGE( table.ErrorDescription() == expected_error,
                                   wxString::Format( "Expected error '%s' but got '%s'",
                                                     expected_error, table.ErrorDescription() ) );

            // Non-parsed tables can't be formatted
            if( !table.IsOk() || !check_formatted )
                continue;

            std::ifstream inFp;
            inFp.open( fn.GetFullPath().fn_str() );
            BOOST_REQUIRE( inFp.is_open() );

            std::stringstream inBuf;
            inBuf << inFp.rdbuf();
            std::string inData = inBuf.str();

            STRING_FORMATTER formatter;
            table.Format( &formatter );
            KICAD_FORMAT::Prettify( formatter.MutableString(),
                                    KICAD_FORMAT::FORMAT_MODE::LIBRARY_TABLE );

            if( formatter.GetString().compare( inData ) != 0 )
            {
                BOOST_TEST_MESSAGE( "--- original ---" );
                BOOST_TEST_MESSAGE( inData );
                BOOST_TEST_MESSAGE( "--- formatted ---" );
                BOOST_TEST_MESSAGE( formatter.GetString() );
            }

            BOOST_REQUIRE( formatter.GetString().compare( inData ) == 0 );
        }
    }
}

BOOST_AUTO_TEST_CASE( Manager )
{
    LIBRARY_MANAGER manager;
    manager.LoadGlobalTables();

    BOOST_REQUIRE( manager.Rows( LIBRARY_TABLE_TYPE::SYMBOL ).size() == 3 );
    BOOST_REQUIRE( manager.Rows( LIBRARY_TABLE_TYPE::FOOTPRINT ).size() == 146 );
}


BOOST_AUTO_TEST_CASE( NestedTablesDisabledHidden )
{
    // Test that disabled and hidden nested library table rows are parsed correctly
    // This is a regression test for https://gitlab.com/kicad/code/kicad/-/issues/22784
    // Note that full end-to-end testing requires the library manager to process the tables,
    // but the parse test verifies the flag is correctly read from disk.

    wxFileName fn( KI_TEST::GetTestDataRootDir(), wxEmptyString );
    fn.AppendDir( "libraries" );

    // Test with the disabled nested table
    fn.SetName( "nested-disabled" );
    LIBRARY_TABLE disabledTable( fn, LIBRARY_TABLE_SCOPE::GLOBAL );
    BOOST_REQUIRE( disabledTable.IsOk() );
    BOOST_REQUIRE_MESSAGE( disabledTable.Rows().size() == 4,
                           wxString::Format( "Expected 4 rows but got %zu",
                                             disabledTable.Rows().size() ) );

    // Verify the disabled flag is parsed correctly on the nested table row
    bool foundDisabledRow = false;

    for( const LIBRARY_TABLE_ROW& row : disabledTable.Rows() )
    {
        if( row.Type() == LIBRARY_TABLE_ROW::TABLE_TYPE_NAME )
        {
            BOOST_REQUIRE_MESSAGE( row.Disabled(),
                                   "Nested table row should have disabled flag set" );
            foundDisabledRow = true;
        }
    }

    BOOST_REQUIRE_MESSAGE( foundDisabledRow,
                           "Disabled nested table row not found in parsed table" );

    // Test hidden nested table has same behavior
    fn.SetName( "nested-hidden" );
    LIBRARY_TABLE hiddenTable( fn, LIBRARY_TABLE_SCOPE::GLOBAL );
    BOOST_REQUIRE( hiddenTable.IsOk() );
    BOOST_REQUIRE_MESSAGE( hiddenTable.Rows().size() == 4,
                           wxString::Format( "Expected 4 rows but got %zu",
                                             hiddenTable.Rows().size() ) );

    bool foundHiddenRow = false;

    for( const LIBRARY_TABLE_ROW& row : hiddenTable.Rows() )
    {
        if( row.Type() == LIBRARY_TABLE_ROW::TABLE_TYPE_NAME )
        {
            BOOST_REQUIRE_MESSAGE( row.Hidden(),
                                   "Nested table row should have hidden flag set" );
            foundHiddenRow = true;
        }
    }

    BOOST_REQUIRE_MESSAGE( foundHiddenRow,
                           "Hidden nested table row not found in parsed table" );
}


/**
 * Regression test: inserting rows into a loaded LIBRARY_TABLE must not invalidate
 * pointers or references to previously-captured rows. The remote symbol import path
 * (EnsureRemoteLibraryEntry) calls InsertRow() at runtime while LIB_DATA instances
 * and LIBRARY_MANAGER::m_rowCache hold raw pointers into the rows container. A
 * std::vector-backed container would reallocate on growth and leave those pointers
 * dangling, which produced an intermittent std::bad_alloc crash deep in
 * KIwxExpandEnvVars when the next symbol placement tried to read the stale URI.
 */
BOOST_AUTO_TEST_CASE( InsertRowPreservesExistingRowPointers )
{
    LIBRARY_TABLE table( true, wxEmptyString, LIBRARY_TABLE_SCOPE::PROJECT );
    table.SetType( LIBRARY_TABLE_TYPE::SYMBOL );

    // Seed with a few rows and snapshot pointers plus the expected URIs.
    std::vector<const LIBRARY_TABLE_ROW*> seededPointers;
    std::vector<wxString>                 seededUris;

    for( int i = 0; i < 4; ++i )
    {
        LIBRARY_TABLE_ROW& row = table.InsertRow();
        row.SetNickname( wxString::Format( wxS( "seed_%d" ), i ) );
        row.SetURI( wxString::Format( wxS( "${KIPRJMOD}/libs/seed_%d.kicad_sym" ), i ) );
        row.SetType( wxS( "KiCad" ) );

        seededPointers.push_back( &row );
        seededUris.push_back( row.URI() );
    }

    // Insert additional rows to force container growth that would reallocate
    // a std::vector, and verify the seeded pointers continue to resolve to the
    // same logical rows (same nickname and URI).
    for( int i = 0; i < 64; ++i )
    {
        LIBRARY_TABLE_ROW& row = table.InsertRow();
        row.SetNickname( wxString::Format( wxS( "extra_%d" ), i ) );
        row.SetURI( wxString::Format( wxS( "${KIPRJMOD}/libs/extra_%d.kicad_sym" ), i ) );
        row.SetType( wxS( "KiCad" ) );

        for( size_t j = 0; j < seededPointers.size(); ++j )
        {
            BOOST_REQUIRE_MESSAGE(
                    seededPointers[j]->URI() == seededUris[j],
                    wxString::Format(
                            wxS( "Seed row %zu pointer was invalidated after inserting %d rows: "
                                 "expected URI '%s', got '%s'" ),
                            j, i + 1, seededUris[j], seededPointers[j]->URI() ) );
        }
    }
}


/**
 * Regression test for the PCM auto-remove identification predicate. Rows inserted by
 * the PCM traverser reference `${KICADn_3RD_PARTY}` directly, so matching on the URI
 * template uniquely identifies them and avoids cleaning up user-added libraries whose
 * expanded absolute paths happen to be descendants of the 3RD_PARTY directory via a
 * different env var (e.g. `${KICAD_USER_LIB}` pointing to `${KICAD10_3RD_PARTY}/V10`).
 *
 * Prior to this fix, `cleanupRemovedPCMLibraries` did a raw prefix check on the
 * expanded path, mis-identifying overlapping user libraries as PCM-managed and
 * silently deleting their rows any time the file was temporarily absent. The
 * user-visible symptom was "Could not create the library file" when adding a new
 * library through the symbol editor.
 */
BOOST_AUTO_TEST_CASE( IsPcmManagedRow_URITemplateMatching )
{
    struct CASE
    {
        wxString uri;
        bool     expectedPcmManaged;
        wxString description;
    };

    std::vector<CASE> cases = {
        { wxS( "${KICAD10_3RD_PARTY}/symbols/foo/foo.kicad_sym" ), true,
          wxS( "Versioned 3RD_PARTY template should be recognised as PCM-managed" ) },
        { wxS( "${KICAD9_3RD_PARTY}/symbols/legacy/legacy.kicad_sym" ), true,
          wxS( "Legacy versioned 3RD_PARTY template should still match the wildcard" ) },
        { wxS( "${KICAD10_3RD_PARTY}/footprints/bar/bar.pretty" ), true,
          wxS( "Footprint library using 3RD_PARTY template should match" ) },
        { wxS( "${KICAD_USER_LIB}/symbols/test.kicad_sym" ), false,
          wxS( "Row using a different env var must not be flagged as PCM-managed" ) },
        { wxS( "${KIPRJMOD}/libs/local.kicad_sym" ), false,
          wxS( "Project-relative row must not be flagged as PCM-managed" ) },
        { wxS( "/abs/path/to/lib.kicad_sym" ), false,
          wxS( "Absolute path row must not be flagged as PCM-managed" ) },
        { wxS( "${}" ), false,
          wxS( "Malformed empty var name must not match" ) },
        { wxS( "${KICAD10_3RD_PARTY_EXTRA}/foo" ), false,
          wxS( "Similar-but-different var name must not match" ) },
    };

    for( const CASE& c : cases )
    {
        LIBRARY_TABLE_ROW row;
        row.SetURI( c.uri );

        bool actual = LIBRARY_MANAGER::IsPcmManagedRow( row );

        BOOST_CHECK_MESSAGE(
                actual == c.expectedPcmManaged,
                wxString::Format( wxS( "%s: URI='%s' expected=%d actual=%d" ),
                                  c.description, c.uri, c.expectedPcmManaged ? 1 : 0,
                                  actual ? 1 : 0 ) );
    }
}


BOOST_AUTO_TEST_SUITE_END()
