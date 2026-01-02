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
 * @file test_issue18119_bus_hierarchy.cpp
 *
 * Test for issue #18119: Bus net connections not propagating correctly through hierarchy
 * when bus member wire names differ from parent bus wire names.
 *
 * The schematic has:
 * - Main sheet with BUS0, BUS1 labels connected via bus entries to bus BUS[0..1]
 * - Bus BUS[0..1] connected to sheet pin SUB_BUS[0..1]
 * - Sub sheet with hierarchical label SUB_BUS[0..1] broken out to SUB_BUS0 and SUB_BUS1
 * - SUB_BUS0 connected to hierarchical label OUT_0
 * - SUB_BUS1 connected to hierarchical label OUT_1
 * - On main sheet: OUT_0 -> R1 -> VCC, OUT_1 -> GND
 *
 * Expected: BUS0 should be connected to VCC (through OUT_0), BUS1 should be connected to GND
 * (through OUT_1).
 *
 * The bug: BUS1 does not connect to GND because the net name propagation through the bus
 * hierarchy doesn't work correctly when wire names (BUS0/BUS1) differ from bus member names
 * (SUB_BUS0/SUB_BUS1).
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

struct ISSUE18119_FIXTURE
{
    ISSUE18119_FIXTURE()
    { }

    SETTINGS_MANAGER           m_settingsManager;
    std::unique_ptr<SCHEMATIC> m_schematic;
};


/**
 * Test that bus member connections propagate correctly through hierarchy when wire names
 * differ on each side of the hierarchy.
 *
 * Expected connectivity:
 * - J4 (BUS1) should be on the same net as GND (via OUT_1) - this is the main bug
 * - J3 (BUS0) should be on the same net as R1.1 (via OUT_0) - different net from VCC due to R1
 *
 * Note: R1 is between OUT_0 and VCC, creating two separate nets:
 * - Net 1: OUT_0 side (includes J3/BUS0/SUB_BUS0/OUT_0/R1.1)
 * - Net 2: VCC side (includes R1.2/VCC)
 */
BOOST_FIXTURE_TEST_CASE( Issue18119BusHierarchy, ISSUE18119_FIXTURE )
{
    LOCALE_IO dummy;

    KI_TEST::LoadSchematic( m_settingsManager, "issue18119/issue18119", m_schematic );

    SCH_SHEET_LIST    sheets = m_schematic->BuildSheetListSortedByPageNumbers();
    CONNECTION_GRAPH* graph  = m_schematic->ConnectionGraph();

    // Find connectors and power symbols and their net codes
    std::map<wxString, int> connectorNetCodes;  // Reference -> net code
    std::map<wxString, int> resistorPinNetCodes; // R1.1, R1.2 -> net code

    for( const auto& [key, subgraphs] : graph->GetNetMap() )
    {
        for( CONNECTION_SUBGRAPH* subgraph : subgraphs )
        {
            for( SCH_ITEM* item : subgraph->GetItems() )
            {
                if( item->Type() == SCH_PIN_T )
                {
                    SCH_PIN*    pin    = static_cast<SCH_PIN*>( item );
                    SCH_SYMBOL* symbol = dynamic_cast<SCH_SYMBOL*>( pin->GetParentSymbol() );

                    if( symbol )
                    {
                        wxString ref = symbol->GetRef( &subgraph->GetSheet() );

                        // Track connectors J1-J4 and power symbols
                        if( ref == "J1" || ref == "J2" || ref == "J3" || ref == "J4" ||
                            ref == "#PWR01" || ref == "#PWR02" || ref == "#PWR03" || ref == "#PWR04" )
                        {
                            connectorNetCodes[ref] = key.Netcode;
                        }

                        // Track R1 pins
                        if( ref == "R1" )
                        {
                            wxString pinKey = ref + "." + pin->GetNumber();
                            resistorPinNetCodes[pinKey] = key.Netcode;
                        }
                    }
                }
            }
        }
    }

    // Verify we found all expected components
    BOOST_REQUIRE_MESSAGE( connectorNetCodes.count( "J1" ), "J1 (VCC connector) should be found" );
    BOOST_REQUIRE_MESSAGE( connectorNetCodes.count( "J2" ), "J2 (GND connector) should be found" );
    BOOST_REQUIRE_MESSAGE( connectorNetCodes.count( "J3" ), "J3 (BUS0) should be found" );
    BOOST_REQUIRE_MESSAGE( connectorNetCodes.count( "J4" ), "J4 (BUS1) should be found" );
    BOOST_REQUIRE_MESSAGE( connectorNetCodes.count( "#PWR02" ), "GND power symbol should be found" );

    // J1 should be on VCC net (through direct wire connection)
    BOOST_REQUIRE_MESSAGE( connectorNetCodes.count( "#PWR01" ), "VCC power symbol should be found" );
    BOOST_CHECK_MESSAGE( connectorNetCodes["J1"] == connectorNetCodes["#PWR01"],
                         "J1 should be on the same net as VCC" );

    // J2 should be on GND net (through direct wire connection)
    BOOST_CHECK_MESSAGE( connectorNetCodes["J2"] == connectorNetCodes["#PWR02"],
                         "J2 should be on the same net as GND" );

    // J3 (BUS0) should be on the same net as R1 pin 1 (through OUT_0)
    // Note: R1 separates the OUT_0 net from the VCC net, so J3 is NOT on VCC net
    BOOST_REQUIRE_MESSAGE( resistorPinNetCodes.count( "R1.1" ), "R1 pin 1 should be found" );
    BOOST_CHECK_MESSAGE( connectorNetCodes["J3"] == resistorPinNetCodes["R1.1"],
                         "J3 (BUS0) should be on the same net as R1.1 (via OUT_0)" );

    // J4 (BUS1) should be on GND net (through OUT_1 -> GND)
    // This tests that BUS1 properly connects through BUS[0..1] -> SUB_BUS[0..1] -> SUB_BUS1 -> OUT_1
    // This is the main bug - BUS1 does not connect to GND
    BOOST_CHECK_MESSAGE( connectorNetCodes["J4"] == connectorNetCodes["#PWR02"],
                         "J4 (BUS1) should be on the same net as GND (via OUT_1)" );
}
