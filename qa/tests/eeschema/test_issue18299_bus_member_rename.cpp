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
 * @file test_issue18299_bus_member_rename.cpp
 *
 * Test for issue #18299: Bus member falls off bus when also individually renamed at a sheet pin.
 *
 * The test schematic has:
 * - Main sheet with two sub-sheet instances (test1, test2) of the same schematic (test.kicad_sch)
 * - Bus {a} connects test1 to test2 via sheet pins
 * - Bus alias "a" has members: a, b, c
 * - Net "b" is a member of bus {a} and is broken out via bus entry in the sub-sheet
 * - Net "b" is ALSO individually connected to a sheet pin on test1 (renamed to "bbb" on parent)
 *
 * Expected: Net "b" inside test1 should be connected to net "b" inside test2 through the bus,
 * even though test1's "b" is also individually renamed at a sheet pin.
 *
 * The bug: When a bus member net is also individually renamed at a sheet pin, the bus member
 * connection through the bus is lost.
 */

#include <qa_utils/wx_utils/unit_test_utils.h>
#include <schematic_utils/schematic_file_util.h>

#include <connection_graph.h>
#include <schematic.h>
#include <sch_sheet.h>
#include <sch_screen.h>
#include <sch_label.h>
#include <settings/settings_manager.h>
#include <locale_io.h>

struct ISSUE18299_FIXTURE
{
    ISSUE18299_FIXTURE()
    { }

    SETTINGS_MANAGER           m_settingsManager;
    std::unique_ptr<SCHEMATIC> m_schematic;
};


/**
 * Test that bus member connections are maintained through hierarchy even when the member
 * is also individually renamed at a sheet pin.
 *
 * Expected connectivity:
 * - Net "b" in test1 sub-sheet should be connected to net "b" in test2 sub-sheet via the bus
 * - The individual sheet pin rename ("b" -> "bbb") should not break the bus member connection
 */
BOOST_FIXTURE_TEST_CASE( Issue18299BusMemberRename, ISSUE18299_FIXTURE )
{
    LOCALE_IO dummy;

    KI_TEST::LoadSchematic( m_settingsManager, "issue18299/issue18299", m_schematic );

    SCH_SHEET_LIST    sheets = m_schematic->BuildSheetListSortedByPageNumbers();
    CONNECTION_GRAPH* graph  = m_schematic->ConnectionGraph();

    // Find the "b" label net codes in each sub-sheet instance
    // We're looking for local labels named "b" in the sub-sheets
    std::map<wxString, int> labelNetCodes;  // "sheetname:labelname" -> net code

    for( const auto& [key, subgraphs] : graph->GetNetMap() )
    {
        for( CONNECTION_SUBGRAPH* subgraph : subgraphs )
        {
            const SCH_SHEET_PATH& sheetPath = subgraph->GetSheet();

            for( SCH_ITEM* item : subgraph->GetItems() )
            {
                if( item->Type() == SCH_LABEL_T )
                {
                    SCH_LABEL* label = static_cast<SCH_LABEL*>( item );
                    wxString   labelText = label->GetText();

                    // Get the sheet instance name from the path
                    wxString sheetName;

                    if( sheetPath.size() > 1 )
                    {
                        // Get the last sheet in the path (the containing sheet)
                        const SCH_SHEET* lastSheet = sheetPath.Last();

                        if( lastSheet )
                            sheetName = lastSheet->GetName();
                    }

                    if( !sheetName.IsEmpty() && labelText == "b" )
                    {
                        wxString mapKey = sheetName + ":" + labelText;
                        labelNetCodes[mapKey] = key.Netcode;
                    }
                }
            }
        }
    }

    // Verify we found "b" labels in both sub-sheet instances
    BOOST_REQUIRE_MESSAGE( labelNetCodes.count( "test1:b" ),
                           "Label 'b' should be found in test1 sub-sheet" );
    BOOST_REQUIRE_MESSAGE( labelNetCodes.count( "test2:b" ),
                           "Label 'b' should be found in test2 sub-sheet" );

    // The main test: "b" in test1 and "b" in test2 should be on the same net
    // because they are both members of bus {a} which connects the two sheets.
    // This tests that the individual sheet pin rename ("b" -> "bbb") does not
    // break the bus member connection through the bus.
    BOOST_CHECK_MESSAGE( labelNetCodes["test1:b"] == labelNetCodes["test2:b"],
                         "Label 'b' in test1 should be on the same net as label 'b' in test2 "
                         "(connected through bus {a})" );
}
