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
 * You should have received a copy of the GNU General Public License along
 * with this program.  If not, see <http://www.gnu.org/licenses/>.
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

#include <wx/filefn.h>
#include <wx/filename.h>
#include <wx/ffile.h>


namespace
{
const char* kMinimalSymbolLib =
        "(kicad_symbol_lib (version 20220914) (generator kicad_symbol_editor)\n"
        "  (symbol \"TestResistor\" (in_bom yes) (on_board yes)\n"
        "    (property \"Reference\" \"R\" (at 0 0 0)\n"
        "      (effects (font (size 1.27 1.27)))\n"
        "    )\n"
        "    (property \"Value\" \"TestResistor\" (at 0 0 0)\n"
        "      (effects (font (size 1.27 1.27)))\n"
        "    )\n"
        "    (property \"Footprint\" \"\" (at 0 0 0)\n"
        "      (effects (font (size 1.27 1.27)) hide)\n"
        "    )\n"
        "    (property \"Datasheet\" \"\" (at 0 0 0)\n"
        "      (effects (font (size 1.27 1.27)) hide)\n"
        "    )\n"
        "    (symbol \"TestResistor_0_1\"\n"
        "      (rectangle (start -1.27 -1.27) (end 1.27 1.27)\n"
        "        (stroke (width 0) (type default))\n"
        "        (fill (type background))\n"
        "      )\n"
        "    )\n"
        "    (symbol \"TestResistor_1_1\"\n"
        "      (pin passive line (at -3.81 0 0) (length 2.54)\n"
        "        (name \"PIN\" (effects (font (size 1.27 1.27))))\n"
        "        (number \"1\" (effects (font (size 1.27 1.27))))\n"
        "      )\n"
        "    )\n"
        "  )\n"
        ")\n";


wxString writeTempSymbolLib()
{
    wxString path = wxFileName::CreateTempFileName( wxS( "qa_remote_sym" ) );
    wxFFile  file( path, wxS( "wb" ) );
    BOOST_REQUIRE( file.IsOpened() );
    file.Write( kMinimalSymbolLib, strlen( kMinimalSymbolLib ) );
    file.Close();
    return path;
}
} // namespace


BOOST_AUTO_TEST_SUITE( RemoteSymbolLoadFromPayload )


BOOST_AUTO_TEST_CASE( CopiedSymbolSurvivesPluginDestruction )
{
    const wxString tempPath = writeTempSymbolLib();
    std::unique_ptr<LIB_SYMBOL> symbol;

    {
        IO_RELEASER<SCH_IO> plugin( SCH_IO_MGR::FindPlugin( SCH_IO_MGR::SCH_KICAD ) );
        BOOST_REQUIRE( plugin );

        LIB_SYMBOL* loaded = plugin->LoadSymbol( tempPath, wxS( "TestResistor" ) );
        BOOST_REQUIRE( loaded );

        // The fix for #23286: copy the symbol so it is independent of the
        // plugin's cache, which is destroyed when the plugin goes out of scope.
        symbol = std::make_unique<LIB_SYMBOL>( *loaded );
    }

    // Plugin and its cache have been destroyed. The copied symbol must still
    // be valid and accessible without triggering a use-after-free.
    BOOST_CHECK( symbol );
    BOOST_CHECK( symbol->GetName() == wxS( "TestResistor" ) );

    wxRemoveFile( tempPath );
}


BOOST_AUTO_TEST_CASE( LoadSymbolReturnsNullForMissingName )
{
    const wxString tempPath = writeTempSymbolLib();

    IO_RELEASER<SCH_IO> plugin( SCH_IO_MGR::FindPlugin( SCH_IO_MGR::SCH_KICAD ) );
    BOOST_REQUIRE( plugin );

    LIB_SYMBOL* loaded = plugin->LoadSymbol( tempPath, wxS( "NonExistentSymbol" ) );
    BOOST_CHECK( loaded == nullptr );

    wxRemoveFile( tempPath );
}


BOOST_AUTO_TEST_SUITE_END()
