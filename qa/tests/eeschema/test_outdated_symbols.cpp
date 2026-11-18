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
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include <qa_utils/wx_utils/unit_test_utils.h>
#include <schematic_utils/schematic_file_util.h>

#include <locale_io.h>
#include <sch_screen.h>
#include <sch_symbol.h>
#include <schematic.h>
#include <settings/settings_manager.h>

#include <map>
#include <set>


BOOST_AUTO_TEST_CASE( OutdatedSymbolsListsRevisedInstances )
{
    LOCALE_IO                  locale;
    SETTINGS_MANAGER           settingsManager;
    std::unique_ptr<SCHEMATIC> schematic;

    KI_TEST::LoadSchematic( settingsManager, "outdated_symbols/ctrl-board", schematic );

    // carol's Rev 2 passives revises R and leaves C untouched
    KI_TEST::SCOPED_PROJECT_SYMBOL_LIBRARY library( schematic->Project(), "acme-passives",
                                                    wxString( KI_TEST::GetEeschemaTestDataDir() )
                                                            + "outdated_symbols/passives.kicad_sym" );

    std::map<const SCH_SYMBOL*, wxString> references;
    std::set<wxString>                    expected;

    for( const SCH_SHEET_PATH& path : schematic->Hierarchy() )
    {
        for( SCH_ITEM* item : path.LastScreen()->Items().OfType( SCH_SYMBOL_T ) )
        {
            const SCH_SYMBOL* symbol = static_cast<const SCH_SYMBOL*>( item );
            references[symbol] = symbol->GetRef( &path );

            if( symbol->GetLibId() == LIB_ID( "acme-passives", "R" ) )
                expected.insert( references[symbol] );
        }
    }

    BOOST_REQUIRE_EQUAL( expected.size(), 21u );

    std::set<wxString> outdated;

    for( SCH_SYMBOL* symbol : SCH_SCREENS( schematic->Root() )
                                      .OutdatedSymbols( *library.adapter, schematic->Settings().SymbolCompareFlags() ) )
    {
        outdated.insert( references[symbol] );
    }

    BOOST_CHECK_EQUAL_COLLECTIONS( outdated.begin(), outdated.end(), expected.begin(), expected.end() );
}
