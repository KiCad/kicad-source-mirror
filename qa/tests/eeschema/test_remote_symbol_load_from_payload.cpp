/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 3 of the License, or (at your
 * option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

/**
 * Regression test for https://gitlab.com/kicad/code/kicad/-/issues/23286
 *
 * SCH_IO_KICAD_SEXPR::LoadSymbol returns a non-owning pointer into the
 * plugin's internal cache. When the plugin is destroyed, the cache deletes
 * every symbol it owns. Code that wraps the returned pointer in a
 * unique_ptr without copying triggers a use-after-free.
 */

#include <qa_utils/wx_utils/unit_test_utils.h>

#include <lib_symbol.h>
#include <sch_io/kicad_sexpr/sch_io_kicad_sexpr.h>
#include <sch_io/sch_io_mgr.h>

#include <wx/filename.h>


namespace
{
// LoadSymbol only reads, so the library is used in place from qa/data
wxString symbolLibPath()
{
    return wxString::FromUTF8( KI_TEST::GetEeschemaTestDataDir() )
           + wxS( "remote_symbol_lib.kicad_sym" );
}
} // namespace


BOOST_AUTO_TEST_SUITE( RemoteSymbolLoadFromPayload )


BOOST_AUTO_TEST_CASE( CopiedSymbolSurvivesPluginDestruction )
{
    const wxString libPath = symbolLibPath();
    std::unique_ptr<LIB_SYMBOL> symbol;

    {
        IO_RELEASER<SCH_IO> plugin( SCH_IO_MGR::FindPlugin( SCH_IO_MGR::SCH_KICAD ) );
        BOOST_REQUIRE( plugin );

        LIB_SYMBOL* loaded = plugin->LoadSymbol( libPath, wxS( "TestResistor" ) );
        BOOST_REQUIRE( loaded );

        // The fix for #23286: copy the symbol so it is independent of the
        // plugin's cache, which is destroyed when the plugin goes out of scope.
        symbol = std::make_unique<LIB_SYMBOL>( *loaded );
    }

    // Plugin and its cache have been destroyed. The copied symbol must still
    // be valid and accessible without triggering a use-after-free.
    BOOST_CHECK( symbol );
    BOOST_CHECK( symbol->GetName() == wxS( "TestResistor" ) );
}


BOOST_AUTO_TEST_CASE( LoadSymbolReturnsNullForMissingName )
{
    const wxString libPath = symbolLibPath();

    IO_RELEASER<SCH_IO> plugin( SCH_IO_MGR::FindPlugin( SCH_IO_MGR::SCH_KICAD ) );
    BOOST_REQUIRE( plugin );

    LIB_SYMBOL* loaded = plugin->LoadSymbol( libPath, wxS( "NonExistentSymbol" ) );
    BOOST_CHECK( loaded == nullptr );
}


BOOST_AUTO_TEST_SUITE_END()
