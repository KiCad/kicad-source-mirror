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

struct ISSUE_22694_FIXTURE
{
    ISSUE_22694_FIXTURE()
    { }

    SETTINGS_MANAGER           m_settingsManager;
    std::unique_ptr<SCHEMATIC> m_schematic;
};


/**
 * Test for issue #22694: Global labels and hierarchical labels not connected.
 *
 * When a hierarchical label in a sub-sheet is connected to a global label on
 * the same sub-sheet, items connected through that hierarchical pin on the
 * parent sheet should also be part of the global net.
 *
 * Schematic structure:
 *   Root sheet:
 *     - Sheet "Subsheet 1" (S1.kicad_sch) with pins HL1, HL2
 *     - Sheet "Untitled Sheet" (untitled.kicad_sch) with no pins
 *     - Wire connecting HL1 sheet pin
 *     - Wire connecting HL2 sheet pin
 *     - Global label "Glob_Lab_Main" (floating, not connected to sheet pins)
 *   Subsheet S1:
 *     - J2 connector (8 pins), pins 1-6 connected to hierarchical labels
 *     - HL1 hierarchical label on J2-Pin1
 *     - HL2 hierarchical labels on J2-Pin2..Pin6
 *     - Additional HL2 hierarchical label connected to "Global Label"
 *   Subsheet untitled:
 *     - J3 connector, Pin1 connected to "Global Label"
 *
 * Expected: J2-Pin2..Pin6 (in S1) should be on the same net as J3-Pin1
 *   (in untitled sheet), because HL2 is connected to "Global Label" in S1,
 *   and "Global Label" connects globally to J3-Pin1 in the untitled sheet.
 */
BOOST_FIXTURE_TEST_CASE( Issue22694, ISSUE_22694_FIXTURE )
{
    LOCALE_IO dummy;

    KI_TEST::LoadSchematic( m_settingsManager, "issue22694/issue22694", m_schematic );

    CONNECTION_GRAPH* graph = m_schematic->ConnectionGraph();

    // Collect net codes for component pins we care about
    std::map<wxString, int> pinNetCodes;

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

                    if( !symbol )
                        continue;

                    wxString ref    = symbol->GetRef( &subgraph->GetSheet() );
                    wxString pinNum = pin->GetNumber();
                    wxString pinKey = ref + wxT( "-" ) + pinNum;

                    pinNetCodes[pinKey] = key.Netcode;
                }
            }
        }
    }

    // Verify we found the relevant pins
    BOOST_REQUIRE_MESSAGE( pinNetCodes.count( "J2-1" ), "J2 Pin 1 should be in netlist" );
    BOOST_REQUIRE_MESSAGE( pinNetCodes.count( "J2-2" ), "J2 Pin 2 should be in netlist" );
    BOOST_REQUIRE_MESSAGE( pinNetCodes.count( "J3-1" ), "J3 Pin 1 should be in netlist" );

    // J2-Pin1 connects through HL1, which is a separate net from HL2
    // J2-Pin2..Pin6 connect through HL2, which also connects to "Global Label"
    // J3-Pin1 connects to "Global Label" in the untitled sheet
    // So J2-Pin2 and J3-Pin1 should be on the same net
    BOOST_CHECK_MESSAGE( pinNetCodes["J2-2"] == pinNetCodes["J3-1"],
                         "J2-Pin2 and J3-Pin1 should be on the same net "
                         "(connected via HL2 + Global Label)" );

    // J2-Pin1 (via HL1) should NOT be on the same net as J2-Pin2 (via HL2)
    BOOST_CHECK_MESSAGE( pinNetCodes["J2-1"] != pinNetCodes["J2-2"],
                         "J2-Pin1 (HL1) and J2-Pin2 (HL2) should be on different nets" );

    // All J2 pins connected via HL2 should be on the same net
    for( int p = 3; p <= 6; p++ )
    {
        wxString pinKey = wxString::Format( "J2-%d", p );

        if( pinNetCodes.count( pinKey ) )
        {
            BOOST_CHECK_MESSAGE( pinNetCodes["J2-2"] == pinNetCodes[pinKey],
                                 wxString::Format( "J2-Pin2 and J2-Pin%d should be on "
                                                   "the same net (both via HL2)", p ) );
        }
    }
}
