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


BOOST_AUTO_TEST_SUITE_END()
