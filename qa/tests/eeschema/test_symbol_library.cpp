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
#include <mock_pgm_base.h>
#include <qa_utils/wx_utils/unit_test_utils.h>
#include <pgm_base.h>
#include "eeschema_test_utils.h"

#include <libraries/symbol_library_adapter.h>

// NOTE: this is for the new symbol library manager adapter and
// library manager system.  There is also test_symbol_library_manager.cpp
// which is for the old code; that one can be removed once the old
// library manager doesn't exist anymore


namespace
{
constexpr const char* kDeviceLibNickname = "Device";
}


class TEST_SYMBOL_LIBRARY_FIXTURE : public KI_TEST::SCHEMATIC_TEST_FIXTURE
{
protected:
    /// Register a fresh SYMBOL_LIBRARY_ADAPTER on the manager and return it.
    /// Shared by the regression tests below so the cast-and-assert boilerplate
    /// only lives in one place.
    SYMBOL_LIBRARY_ADAPTER* RegisterSymbolAdapter( LIBRARY_MANAGER& aManager )
    {
        aManager.RegisterAdapter( LIBRARY_TABLE_TYPE::SYMBOL,
                                  std::make_unique<SYMBOL_LIBRARY_ADAPTER>( aManager ) );

        std::optional<LIBRARY_MANAGER_ADAPTER*> adapterOpt =
                aManager.Adapter( LIBRARY_TABLE_TYPE::SYMBOL );
        BOOST_REQUIRE( adapterOpt.has_value() );
        return static_cast<SYMBOL_LIBRARY_ADAPTER*>( *adapterOpt );
    }

    wxFileName GetTestProjectSchPath() const
    {
        wxFileName fn( KI_TEST::GetTestDataRootDir(), "test_project.kicad_sch" );
        fn.AppendDir( "libraries" );
        fn.AppendDir( "test_project" );
        return fn;
    }

    /// Ensure KICAD9_SYMBOL_DIR points at the QA data libraries so the global
    /// sym-lib-table's ${KICAD9_SYMBOL_DIR}/Device.kicad_sym URI resolves.
    /// Only needed by tests that load a global library before any project is open.
    void EnsureGlobalSymbolDir()
    {
        if( !wxGetEnv( wxT( "KICAD9_SYMBOL_DIR" ), nullptr ) )
        {
            wxString path( KI_TEST::GetTestDataRootDir() );
            path += wxT( "/libraries" );
            wxSetEnv( wxT( "KICAD9_SYMBOL_DIR" ), path );
        }
    }
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

        case LOAD_STATUS::LOAD_ERROR:
            BOOST_REQUIRE( status.error.has_value() );
            msg << ": error: " << status.error->message;
            break;
        }

        BOOST_TEST_MESSAGE( msg );
    }
}

// Regression test for https://gitlab.com/kicad/code/kicad/-/issues/23756
// Verifies that LoadProjectTables() clears the adapter's cached LIB_DATA entries
// before destroying the backing LIBRARY_TABLE objects. Without this, LIB_DATA::row
// raw pointers dangle across a reload and later calls such as GetLibraryDescription()
// crash when dereferencing freed LIBRARY_TABLE_ROW memory. The user-visible trigger
// was creating a new library through File -> Export -> Symbols, which re-enters the
// SelectLibrary loop and iterates all loaded libraries to show their descriptions.
BOOST_AUTO_TEST_CASE( LoadProjectTablesClearsAdapterCache )
{
    if( !wxGetEnv( wxT( "KICAD_CONFIG_HOME_IS_QA" ), nullptr ) )
    {
        BOOST_TEST_MESSAGE( "QA test is running using unknown config home; skipping" );
        return;
    }

    LIBRARY_MANAGER manager;
    manager.LoadGlobalTables();

    LoadSchematic( GetTestProjectSchPath().GetFullPath() );
    PROJECT& project = SettingsManager().Prj();
    manager.LoadProjectTables( project.GetProjectDirectory() );

    SYMBOL_LIBRARY_ADAPTER* adapter = RegisterSymbolAdapter( manager );

    // Force a project-scope library into m_libraries so that LIB_DATA::row holds
    // a raw pointer into the project LIBRARY_TABLE's deque. The project-scope Device
    // row shadows any global Device entry thanks to LoadProjectTables search order.
    adapter->LoadOne( kDeviceLibNickname );
    BOOST_REQUIRE( adapter->IsLibraryLoaded( kDeviceLibNickname ) );

    // Reload the project tables. Before the fix, this destroyed the old project
    // LIBRARY_TABLE (via loadTables()'s aTarget.erase(type)) while leaving the stale
    // LIB_DATA entry with its dangling row pointer in the adapter's m_libraries cache.
    manager.LoadProjectTables( project.GetProjectDirectory() );

    // After the fix, the project cache must be cleared. IsLibraryLoaded() reads only
    // status (no row deref) so it is safe to call even if the cache was not cleared,
    // which makes it a reliable regression assertion: pre-fix it returns true
    // (stale LOADED entry), post-fix it returns false (cache emptied).
    BOOST_REQUIRE_MESSAGE( !adapter->IsLibraryLoaded( kDeviceLibNickname ),
                           "Project library cache must be cleared by LoadProjectTables "
                           "to prevent dangling LIB_DATA::row pointers" );

    // Reloading the library should succeed and produce a readable description using
    // the fresh row pointer from the new project table.
    adapter->LoadOne( kDeviceLibNickname );
    BOOST_REQUIRE( adapter->IsLibraryLoaded( kDeviceLibNickname ) );
    BOOST_REQUIRE( adapter->GetLibraryDescription( kDeviceLibNickname ).has_value() );
}


// Follow-up regression for https://gitlab.com/kicad/code/kicad/-/issues/23756
// Verifies that LoadProjectTables() preserves project-over-global shadowing for
// cached nicknames. SYMBOL_LIBRARY_ADAPTER::GlobalLibraries is a process-wide
// static, so a global library loaded by an earlier code path can persist as
// LOADED while a later project introduces a same-named project row that should
// shadow it. Erasing the project cache on reload would expose the stale global
// entry until the project library is explicitly reloaded; resetting in place
// (LIB_DATA{}) keeps the project nickname as a sentinel so fetchIfLoaded() and
// IsLibraryLoaded() correctly report it as not loaded.
BOOST_AUTO_TEST_CASE( ProjectReloadPreservesShadowing )
{
    if( !wxGetEnv( wxT( "KICAD_CONFIG_HOME_IS_QA" ), nullptr ) )
    {
        BOOST_TEST_MESSAGE( "QA test is running using unknown config home; skipping" );
        return;
    }

    EnsureGlobalSymbolDir();

    LIBRARY_MANAGER manager;

    // Register the adapter BEFORE loading tables. LoadGlobalTables() routes
    // GlobalTablesChanged() through registered adapters to clear the static
    // GlobalLibraries cache. Without this, stale LIB_DATA entries left over
    // from earlier test cases (AsyncLoad populates the process-wide
    // GlobalLibraries with LIB_DATA::row pointers into a LIBRARY_MANAGER that
    // is subsequently destroyed) would dangle into this test run.
    SYMBOL_LIBRARY_ADAPTER* adapter = RegisterSymbolAdapter( manager );

    manager.LoadGlobalTables();

    // Force the global "Device" into the process-wide globalLibs cache before
    // the project is loaded. With no project tables present, loadIfNeeded()
    // falls through PROJECT scope and populates GLOBAL scope. After this call
    // the static GlobalLibraries map holds Device with status LOADED.
    adapter->LoadOne( kDeviceLibNickname );
    BOOST_REQUIRE( adapter->IsLibraryLoaded( kDeviceLibNickname ) );

    // Now load the project. Its symbol library table contains a "Device" row
    // that shadows the global Device (verified by ProjectLibraryTable above).
    LoadSchematic( GetTestProjectSchPath().GetFullPath() );
    PROJECT& project = SettingsManager().Prj();
    manager.LoadProjectTables( project.GetProjectDirectory() );

    // LoadOne now resolves through PROJECT scope first and populates m_libraries
    // with the project Device row, masking the global Device entry.
    adapter->LoadOne( kDeviceLibNickname );
    BOOST_REQUIRE( adapter->IsLibraryLoaded( kDeviceLibNickname ) );

    // Reload the project tables to fire ProjectTablesChanged. The project cache
    // entry must be invalidated in place rather than erased so it continues to
    // mask the still-LOADED global Device entry until the project library is
    // explicitly reloaded.
    manager.LoadProjectTables( project.GetProjectDirectory() );

    // Pre-fix (m_libraries.clear()): the project Device entry is gone, so
    // IsLibraryLoaded() falls through to globalLibs and returns true.
    // Post-fix (in-place reset): the project Device entry remains but is no
    // longer LOADED, so IsLibraryLoaded() returns false immediately.
    BOOST_REQUIRE_MESSAGE( !adapter->IsLibraryLoaded( kDeviceLibNickname ),
                           "Project library reload must preserve project-over-global "
                           "shadowing for cached nicknames" );

    // Reloading repopulates the entry from the rebuilt project table.
    adapter->LoadOne( kDeviceLibNickname );
    BOOST_REQUIRE( adapter->IsLibraryLoaded( kDeviceLibNickname ) );
    BOOST_REQUIRE( adapter->GetLibraryDescription( kDeviceLibNickname ).has_value() );
}


// Follow-up regression for https://gitlab.com/kicad/code/kicad/-/issues/23756
// Complements ProjectReloadPreservesShadowing: if a project library is removed
// (so the rebuilt project table no longer contains its nickname), the cached
// sentinel installed by ProjectTablesChanged() must not permanently mask a
// same-named global library. ProjectTablesReloaded() is responsible for erasing
// sentinels that no longer have a backing project row.
BOOST_AUTO_TEST_CASE( ProjectReloadReleasesRemovedShadow )
{
    if( !wxGetEnv( wxT( "KICAD_CONFIG_HOME_IS_QA" ), nullptr ) )
    {
        BOOST_TEST_MESSAGE( "QA test is running using unknown config home; skipping" );
        return;
    }

    EnsureGlobalSymbolDir();

    LIBRARY_MANAGER manager;

    // Register adapter first so LoadGlobalTables() clears any stale static
    // GlobalLibraries entries left over from earlier test cases.
    SYMBOL_LIBRARY_ADAPTER* adapter = RegisterSymbolAdapter( manager );

    manager.LoadGlobalTables();

    // Warm the global cache with Device.
    adapter->LoadOne( kDeviceLibNickname );
    BOOST_REQUIRE( adapter->IsLibraryLoaded( kDeviceLibNickname ) );

    // Load the project and materialise the project-scope Device entry in
    // m_libraries (shadows the global Device).
    LoadSchematic( GetTestProjectSchPath().GetFullPath() );
    PROJECT& project = SettingsManager().Prj();
    manager.LoadProjectTables( project.GetProjectDirectory() );

    adapter->LoadOne( kDeviceLibNickname );
    BOOST_REQUIRE( adapter->IsLibraryLoaded( kDeviceLibNickname ) );

    // Now simulate the project losing its Device row. Pointing LoadProjectTables
    // at a non-readable path tears down every project-scope LIBRARY_TABLE so the
    // rebuilt state has no project Device row. ProjectTablesChanged() installs
    // the sentinel in m_libraries, and ProjectTablesReloaded() must erase it
    // because GetRow(PROJECT, "Device") no longer resolves.
    manager.LoadProjectTables( wxString( wxS( "/nonexistent/path/for/test" ) ) );

    // Pre-fix (sentinel left in place): the stale INVALID entry in m_libraries
    // short-circuits IsLibraryLoaded() before it consults globalLibs(), so it
    // returns false even though the global Device is still LOADED.
    // Post-fix (ProjectTablesReloaded prunes the sentinel): IsLibraryLoaded()
    // falls through to globalLibs() and finds the still-LOADED global Device.
    BOOST_REQUIRE_MESSAGE( adapter->IsLibraryLoaded( kDeviceLibNickname ),
                           "Removing a project library must expose the same-named global "
                           "library rather than leave a permanent sentinel mask" );
}


// Regression test for https://gitlab.com/kicad/code/kicad/-/issues/23480
// Verifies that reloading project tables during an async library load does not
// crash due to dangling LIBRARY_TABLE_ROW pointers held by background workers.
BOOST_AUTO_TEST_CASE( ProjectChangeDuringAsyncLoad )
{
    if( !wxGetEnv( wxT( "KICAD_CONFIG_HOME_IS_QA" ), nullptr ) )
    {
        BOOST_TEST_MESSAGE( "QA test is running using unknown config home; skipping" );
        return;
    }

    LIBRARY_MANAGER manager;
    manager.LoadGlobalTables();

    LoadSchematic( GetTestProjectSchPath().GetFullPath() );
    PROJECT& project = SettingsManager().Prj();
    manager.LoadProjectTables( project.GetProjectDirectory() );

    SYMBOL_LIBRARY_ADAPTER* adapter = RegisterSymbolAdapter( manager );

    adapter->AsyncLoad();

    // Immediately trigger a project table reload while async workers are running.
    // Before the fix, this destroyed table rows that workers were still using.
    manager.ProjectChanged();

    adapter->BlockUntilLoaded();

    BOOST_TEST_MESSAGE( "ProjectChanged during async load completed without crash" );
}


BOOST_AUTO_TEST_SUITE_END()
