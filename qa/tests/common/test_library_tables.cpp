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

#include <richio.h>
#include <io/kicad/kicad_io_utils.h>
#include <qa_utils/wx_utils/unit_test_utils.h>
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
    };

    std::vector<TESTCASE> cases = {
        { .filename = "sym-lib-table", .expected_rows = 224 },
        { .filename = "fp-lib-table", .expected_rows = 146 },
        { .filename = "nested-symbols", .expected_rows = 6 },
        { .filename = "cycle", .expected_rows = 2 },
        { .filename = "corrupted", .expected_error = "Syntax error at line 6, column 9" },
        { .filename = "truncated", .expected_error = "Syntax error at line 11, column 1" }
    };

    wxFileName fn( KI_TEST::GetTestDataRootDir(), wxEmptyString );
    fn.AppendDir( "libraries" );

    for( const auto& [filename, expected_error, expected_rows] : cases )
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

            // TODO this should move to manager test suite
            table.LoadNestedTables();

            // Non-parsed tables can't be formatted
            if( !table.IsOk() )
                continue;

            std::ifstream inFp;
            inFp.open( fn.GetFullPath() );
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

    BOOST_REQUIRE( manager.Rows( LIBRARY_TABLE_TYPE::SYMBOL ).size() == 2 );
    BOOST_REQUIRE( manager.Rows( LIBRARY_TABLE_TYPE::FOOTPRINT ).size() == 146 );
}

BOOST_AUTO_TEST_SUITE_END()
