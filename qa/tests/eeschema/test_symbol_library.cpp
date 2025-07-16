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

#include <fmt/format.h>
#include <qa_utils/wx_utils/unit_test_utils.h>
#include <pgm_base.h>
#include "eeschema_test_utils.h"

#include <libraries/symbol_library_adapter.h>

// NOTE: this is for the new symbol library manager adapter and
// library manager system.  There is also test_symbol_library_manager.cpp
// which is for the old code; that one can be removed once the old
// library manager doesn't exist anymore


class TEST_SYMBOL_LIBRARY_FIXTURE : public KI_TEST::SCHEMATIC_TEST_FIXTURE
{
};
 
BOOST_FIXTURE_TEST_SUITE( SymbolLibrary, TEST_SYMBOL_LIBRARY_FIXTURE )

BOOST_AUTO_TEST_CASE( ProjectLibraryTable )
{
    if( !wxGetEnv( wxT( "KICAD_CONFIG_HOME_IS_QA" ), nullptr ) )
    {
        BOOST_TEST_MESSAGE( "QA test is running using unknown config home; skipping" );
        return;
    }

    LIBRARY_MANAGER manager;
    manager.LoadGlobalTables();

    wxFileName fn( KI_TEST::GetTestDataRootDir(), "test_project.kicad_sch" );
    fn.AppendDir( "libraries" );
    fn.AppendDir( "test_project" );

    std::vector<LIBRARY_TABLE_ROW*> rows = manager.Rows( LIBRARY_TABLE_TYPE::SYMBOL );

    BOOST_REQUIRE( rows.size() == 3 );
    BOOST_REQUIRE( rows[0]->Nickname() == "Device" );

    LoadSchematic( fn.GetFullPath() );
    PROJECT& project = SettingsManager().Prj();
    manager.LoadProjectTables( project.GetProjectDirectory() );

    rows = manager.Rows( LIBRARY_TABLE_TYPE::SYMBOL );

    BOOST_REQUIRE( rows.size() == 4 );
    BOOST_REQUIRE( rows[0]->Nickname() == "Device" );
    BOOST_REQUIRE( rows[0]->URI() == "${KIPRJMOD}/Device.kicad_sym" );

    SYMBOL_LIBRARY_ADAPTER adapter( manager );
    adapter.LoadOne( "Device" );

    std::vector<LIB_SYMBOL*> symbols = adapter.GetSymbols( "Device" );

    // Check that the project's copy of Device is the one being returned
    BOOST_REQUIRE( symbols.size() == 3 );

    symbols = adapter.GetSymbols( "power" );
    BOOST_TEST_MESSAGE( symbols.size() );

    BOOST_REQUIRE( adapter.LoadSymbol( "Device", "R" ) );
}


// If you want to do benchmarking, it can be useful to run this testcase standalone with
// the KICAD_CONFIG_HOME and KICAD9_SYMBOL_DIR environment variables set to an actual KiCad
// installation so that you can benchmark a typical load
BOOST_AUTO_TEST_CASE( AsyncLoad )
{
    LIBRARY_MANAGER manager;
    manager.LoadGlobalTables();

    Pgm().GetSettingsManager().LoadProject( "" );
    SYMBOL_LIBRARY_ADAPTER adapter( manager );

    auto tstart = std::chrono::high_resolution_clock::now();
    adapter.AsyncLoad();

    constexpr static int interval = 250;
    constexpr static int timeLimit = 20000;
    int elapsed = 0;

    while( true )
    {
        std::this_thread::sleep_for( std::chrono::milliseconds( interval ) );

        if( std::optional<float> loadStatus = adapter.AsyncLoadProgress(); loadStatus.has_value() )
        {
            BOOST_TEST_MESSAGE( fmt::format( "Loading libraries: ({:.1f}%)", *loadStatus * 100 ) );

            if( loadStatus >= 1 )
                break;
        }
        else
        {
            // Just informational; this could be fine when running in a QA context
            BOOST_TEST_MESSAGE( "Async load not in progress" );
            break;
        }

        elapsed += interval;

        if( elapsed > timeLimit )
        {
            BOOST_TEST_FAIL( "Exceeded timeout" );
            break;
        }
    }

    auto duration = std::chrono::high_resolution_clock::now() - tstart;

    BOOST_TEST_MESSAGE(
        fmt::format( "took {}ms",
        std::chrono::duration_cast<std::chrono::milliseconds>( duration ).count() ) );

    adapter.BlockUntilLoaded();

    std::vector<LIBRARY_TABLE_ROW*> rows = manager.Rows( LIBRARY_TABLE_TYPE::SYMBOL );

    BOOST_REQUIRE_GE( rows.size(), 2 );

    for( auto& [nickname, status] : adapter.GetLibraryStatuses() )
    {
        wxString msg = nickname;
        BOOST_REQUIRE( status.load_status != LOAD_STATUS::LOADING );

        switch( status.load_status )
        {
        default:
        case LOAD_STATUS::LOADED:
            msg << ": loaded OK";
            BOOST_REQUIRE( !status.error.has_value() );
            break;

        case LOAD_STATUS::ERROR:
            BOOST_REQUIRE( status.error.has_value() );
            msg << ": error: " << status.error->message;
            break;
        }

        BOOST_TEST_MESSAGE( msg );
    }
}

BOOST_AUTO_TEST_SUITE_END()
