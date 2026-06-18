/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
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

#include <set>
#include <vector>

#include <boost/test/unit_test.hpp>

#include <database/database_lib_settings.h>
#include <lib_symbol.h>
#include <libraries/library_manager.h>
#include <libraries/symbol_library_adapter.h>
#include <sch_io/database/sch_io_database.h>


BOOST_AUTO_TEST_SUITE( SchIoDatabase )


/**
 * SCH_IO_DATABASE::cacheLib() must not rebuild its materialized LIB_SYMBOL cache when a re-query
 * returns the same rows. Before the signature-comparison fix, the cache was rebuilt on every
 * max_age expiry and on any change to the global library modify hash; the async library loader
 * bumps that hash whenever an unrelated library finishes loading, freezing the symbol chooser for
 * seconds at a time on large database libraries.
 *
 * Forcing max_age to zero makes every EnumerateSymbolLib call re-query the database, so the
 * signature-comparison path is exercised. With the fix, unchanged data leaves the existing
 * LIB_SYMBOL objects in place and the returned pointers are stable; with the fix reverted the
 * cache is rebuilt and the pointers change.
 *
 * See https://gitlab.com/kicad/code/kicad/-/issues/23509
 */
BOOST_AUTO_TEST_CASE( UnchangedDataReusesCache )
{
    LIBRARY_MANAGER        manager;
    SYMBOL_LIBRARY_ADAPTER adapter( manager );

    SCH_IO_DATABASE plugin;
    plugin.SetLibraryManagerAdapter( &adapter );

    wxString libPath( QA_DBLIB_SETTINGS_PATH );

    std::vector<LIB_SYMBOL*> first;
    BOOST_REQUIRE_NO_THROW( plugin.EnumerateSymbolLib( first, libPath ) );
    BOOST_REQUIRE( !first.empty() );

    // Defeat the age throttle so the next call actually re-queries and compares signatures rather
    // than short-circuiting on the cache timestamp.
    plugin.Settings()->m_Cache.max_age = 0;

    std::vector<LIB_SYMBOL*> second;
    BOOST_REQUIRE_NO_THROW( plugin.EnumerateSymbolLib( second, libPath ) );

    BOOST_REQUIRE_EQUAL( first.size(), second.size() );

    // Identical data must yield the same materialized symbols. Pointer identity proves the cache
    // was reused; a rebuild would hand back freshly allocated LIB_SYMBOLs.
    std::set<LIB_SYMBOL*> firstSet( first.begin(), first.end() );

    for( LIB_SYMBOL* symbol : second )
        BOOST_CHECK( firstSet.count( symbol ) );
}


BOOST_AUTO_TEST_SUITE_END()
