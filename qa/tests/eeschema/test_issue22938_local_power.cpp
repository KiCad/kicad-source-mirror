/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright The KiCad Developers, see AUTHORS.TXT for contributors.
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
 * along with this program; if not, you may find one here:
 * https://www.gnu.org/licenses/gpl-3.0.en.html
 * or you may search the http://www.gnu.org website for the version 32 license,
 * or you may write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
 */

/**
 * Test for issue #22938: Local power port acts as global port
 *
 * A local power port symbol should only connect nets within the same sheet.
 * The bug causes local power ports with the same name on different sheets
 * to be connected together as if they were global power ports.
 */

#include <qa_utils/wx_utils/unit_test_utils.h>
#include <schematic_utils/schematic_file_util.h>

#include <connection_graph.h>
#include <schematic.h>
#include <sch_sheet.h>
#include <sch_screen.h>
#include <sch_symbol.h>
#include <sch_pin.h>
#include <settings/settings_manager.h>
#include <locale_io.h>

struct LOCAL_POWER_TEST_FIXTURE
{
    LOCAL_POWER_TEST_FIXTURE() = default;

    SETTINGS_MANAGER           m_settingsManager;
    std::unique_ptr<SCHEMATIC> m_schematic;
};


BOOST_FIXTURE_TEST_CASE( Issue22938_LocalPowerNotGlobal, LOCAL_POWER_TEST_FIXTURE )
{
    LOCALE_IO dummy;

    KI_TEST::LoadSchematic( m_settingsManager, "issue22938/issue22938", m_schematic );

    SCH_SHEET_LIST sheets = m_schematic->BuildSheetListSortedByPageNumbers();
    CONNECTION_GRAPH* graph = m_schematic->ConnectionGraph();

    // Map to track net codes for +48V local power pins on each sheet
    // Key: sheet path human readable, Value: net code
    std::map<wxString, int> localPower48VNetCodes;

    for( const auto& [key, subgraphs] : graph->GetNetMap() )
    {
        for( CONNECTION_SUBGRAPH* subgraph : subgraphs )
        {
            for( SCH_ITEM* item : subgraph->GetItems() )
            {
                if( item->Type() == SCH_PIN_T )
                {
                    SCH_PIN* pin = static_cast<SCH_PIN*>( item );

                    if( pin->IsLocalPower() )
                    {
                        wxString netName = pin->GetDefaultNetName( subgraph->GetSheet() );

                        if( netName == "+48V" )
                        {
                            wxString sheetPath = subgraph->GetSheet().PathHumanReadable();
                            localPower48VNetCodes[sheetPath] = key.Netcode;
                        }
                    }
                }
            }
        }
    }

    // We expect to find +48V local power pins on at least two different sheets
    BOOST_REQUIRE_MESSAGE( localPower48VNetCodes.size() >= 2,
                           "Expected +48V local power pins on multiple sheets" );

    // All local power pins with the same name but on DIFFERENT sheets
    // should have DIFFERENT net codes (they should NOT be connected)
    int firstNetCode = -1;
    bool allSame = true;

    for( const auto& [sheetPath, netCode] : localPower48VNetCodes )
    {
        if( firstNetCode == -1 )
        {
            firstNetCode = netCode;
        }
        else if( netCode != firstNetCode )
        {
            allSame = false;
            break;
        }
    }

    // The bug causes all local +48V nets to have the same net code
    // After the fix, they should have different net codes
    BOOST_CHECK_MESSAGE( !allSame,
                         "Local power ports +48V on different sheets should NOT be connected. "
                         "Each sheet should have its own isolated +48V net." );
}
