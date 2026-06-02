/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 3
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, you may find one at
 * http://www.gnu.org/licenses/
 */

#include <boost/test/unit_test.hpp>
#include <schematic_utils/schematic_file_util.h>
#include <settings/settings_manager.h>
#include <locale_io.h>
#include <lib_symbol.h>
#include <sch_screen.h>
#include <sch_symbol.h>
#include <reporter.h>
#include <sch_io/kicad_legacy/sch_io_kicad_legacy_lib_cache.h>
#include <qa_utils/wx_utils/unit_test_utils.h>
#include <wx/string.h>

struct ERC_LIB_SYMBOL_MISMATCH_FIXTURE
{
    ERC_LIB_SYMBOL_MISMATCH_FIXTURE() {}

    SETTINGS_MANAGER           m_settingsManager;
    std::unique_ptr<SCHEMATIC> m_schematic;
};


BOOST_AUTO_TEST_SUITE( ERCLibSymbolMismatch )


BOOST_FIXTURE_TEST_CASE( Issue22371LegacyLibrary, ERC_LIB_SYMBOL_MISMATCH_FIXTURE )
{
    LOCALE_IO dummy;

    KI_TEST::LoadSchematic( m_settingsManager, wxString( "issue22371/issue22371" ), m_schematic );

    std::string dataDir = KI_TEST::GetEeschemaTestDataDir();
    wxString legacyLibPath = wxString( dataDir + "issue22371/lib.sch/74xx.lib" );

    SCH_IO_KICAD_LEGACY_LIB_CACHE legacyCache( legacyLibPath );
    legacyCache.Load();

    const LIB_SYMBOL_MAP& symbols = legacyCache.GetSymbolMap();
    auto it = symbols.find( "74LS00" );
    BOOST_REQUIRE_MESSAGE( it != symbols.end(), "74LS00 not found in legacy library" );

    std::unique_ptr<LIB_SYMBOL> flattenedLibSymbol = it->second->Flatten();

    SCH_SCREENS screens( m_schematic->Root() );

    for( SCH_SCREEN* screen = screens.GetFirst(); screen; screen = screens.GetNext() )
    {
        for( SCH_ITEM* item : screen->Items().OfType( SCH_SYMBOL_T ) )
        {
            SCH_SYMBOL* symbol = static_cast<SCH_SYMBOL*>( item );
            LIB_SYMBOL* libSymbolInSchematic = symbol->GetLibSymbolRef().get();

            if( !libSymbolInSchematic )
                continue;

            if( symbol->GetLibId().GetLibItemName() != "74LS00" )
                continue;

            constexpr int flags = SCH_ITEM::COMPARE_FLAGS::EQUALITY
                                  | SCH_ITEM::COMPARE_FLAGS::ERC;

            int result = flattenedLibSymbol->Compare( *libSymbolInSchematic, flags );

            BOOST_CHECK_EQUAL( result, 0 );
            return;
        }
    }

    BOOST_FAIL( "No 74LS00 symbol found in schematic" );
}


BOOST_AUTO_TEST_SUITE_END()
