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

#include <boost/test/unit_test.hpp>

#include <database/database_lib_settings.h>
#include <lib_symbol.h>
#include <libraries/library_manager.h>
#include <libraries/symbol_library_adapter.h>

// We need access to private members of SCH_IO_DATABASE to drive the cycle-detection
// state directly. The plugin declares SCH_IO_DATABASE_CYCLE_DETECTION_FIXTURE as a friend.
#include <eeschema/sch_io/database/sch_io_database.h>


/// Regression test for https://gitlab.com/kicad/code/kicad/-/issues/24249
///
/// When a database row's Symbols column references the database library itself,
/// SCH_IO_DATABASE::loadSymbolFromRow used to recurse through the adapter back into its own
/// LoadSymbol entry point and blow the stack. We drive loadSymbolFromRow directly through a
/// friend fixture; an end-to-end test would need a live Pgm()/project loader and a SQLite ODBC
/// driver, neither of which the unit harness provides.
class SCH_IO_DATABASE_CYCLE_DETECTION_FIXTURE
{
public:
    SCH_IO_DATABASE_CYCLE_DETECTION_FIXTURE() = default;

    /// Drive loadSymbolFromRow with aInProgressKey already marked in-progress, simulating the
    /// second entry of a recursive load.
    std::unique_ptr<LIB_SYMBOL> InvokeWithCycle( SCH_IO_DATABASE& aPlugin,
                                                 const wxString&  aSymbolName,
                                                 const DATABASE_LIB_TABLE& aTable,
                                                 const DATABASE_CONNECTION::ROW& aRow,
                                                 const wxString& aInProgressKey )
    {
        aPlugin.m_inProgressLoads.insert( aInProgressKey );
        return aPlugin.loadSymbolFromRow( aSymbolName, aTable, aRow );
    }

    /// Drive loadSymbolFromRow without pre-seeding the in-progress set; the plugin inserts and
    /// (via its RAII guard) removes its own entry.
    std::unique_ptr<LIB_SYMBOL> Invoke( SCH_IO_DATABASE& aPlugin,
                                        const wxString& aSymbolName,
                                        const DATABASE_LIB_TABLE& aTable,
                                        const DATABASE_CONNECTION::ROW& aRow )
    {
        return aPlugin.loadSymbolFromRow( aSymbolName, aTable, aRow );
    }

    bool IsInProgressEmpty( const SCH_IO_DATABASE& aPlugin ) const
    {
        return aPlugin.m_inProgressLoads.empty();
    }

    std::size_t InProgressSize( const SCH_IO_DATABASE& aPlugin ) const
    {
        return aPlugin.m_inProgressLoads.size();
    }
};


BOOST_AUTO_TEST_SUITE( SchIoDatabaseCycle )


// m_adapter is deliberately left null: if the cycle short-circuit fails, loadSymbolFromRow
// dereferences the null adapter and crashes the test. So this fails on revert of the fix.
BOOST_AUTO_TEST_CASE( DetectsAlreadyInProgressLibId )
{
    SCH_IO_DATABASE plugin;

    DATABASE_LIB_TABLE table;
    table.name        = "SelfRefDB";
    table.table       = "Parts";
    table.key_col     = "Part ID";
    table.symbols_col = "Symbols";

    DATABASE_CONNECTION::ROW row;
    row[table.symbols_col] = std::string( "SelfRefDB:R-001" );

    SCH_IO_DATABASE_CYCLE_DETECTION_FIXTURE fixture;

    std::unique_ptr<LIB_SYMBOL> symbol =
            fixture.InvokeWithCycle( plugin, wxS( "R-001" ), table, row, wxS( "SelfRefDB:R-001" ) );

    BOOST_REQUIRE( symbol );
    BOOST_CHECK( symbol->GetName() == wxS( "R-001" ) );

    // The re-insert of the already-seeded key must observe it as present and so must not erase
    // the fixture's entry on the way out.
    BOOST_CHECK_EQUAL( fixture.InProgressSize( plugin ), 1u );
}


// A leaked in-progress entry would make a later load of the same LIB_ID a false positive cycle.
// Drive two loads through a real (empty) adapter and require the set to be empty after each, so
// a broken RAII removal is caught.
BOOST_AUTO_TEST_CASE( RaiiGuardBalancesInProgressSet )
{
    LIBRARY_MANAGER        libManager;
    SYMBOL_LIBRARY_ADAPTER adapter( libManager );

    SCH_IO_DATABASE plugin;
    plugin.SetLibraryManagerAdapter( &adapter );

    DATABASE_LIB_TABLE table;
    table.name        = "SelfRefDB";
    table.table       = "Parts";
    table.key_col     = "Part ID";
    table.symbols_col = "Symbols";

    DATABASE_CONNECTION::ROW row;
    // Valid LIB_ID that will be tracked. The adapter has no libraries loaded so its LoadSymbol
    // returns null without ever re-entering SCH_IO_DATABASE.
    row[table.symbols_col] = std::string( "SomeLib:R-001" );

    SCH_IO_DATABASE_CYCLE_DETECTION_FIXTURE fixture;
    BOOST_REQUIRE( fixture.IsInProgressEmpty( plugin ) );

    std::unique_ptr<LIB_SYMBOL> symbol = fixture.Invoke( plugin, wxS( "R-001" ), table, row );

    BOOST_REQUIRE( symbol );
    BOOST_CHECK( fixture.IsInProgressEmpty( plugin ) );

    // A second call on the same row must not see leftover state from the first.
    symbol = fixture.Invoke( plugin, wxS( "R-001" ), table, row );
    BOOST_REQUIRE( symbol );
    BOOST_CHECK( fixture.IsInProgressEmpty( plugin ) );
}


BOOST_AUTO_TEST_SUITE_END()
