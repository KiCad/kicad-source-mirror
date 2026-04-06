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
#include <sch_pin.h>
#include <sch_screen.h>
#include <sch_sheet.h>
#include <sch_symbol.h>
#include <settings/settings_manager.h>
#include <locale_io.h>


struct ISSUE_23719_FIXTURE
{
    ISSUE_23719_FIXTURE()
    { }

    SETTINGS_MANAGER           m_settingsManager;
    std::unique_ptr<SCHEMATIC> m_schematic;
};


/**
 * Test for issue #23719 -- global labels not combining nets across more than two sheets.
 *
 * Schematic structure (3 sub-sheets, all globals).
 *
 *   Sheet 1 -- Connector J1 with two of its pins driven by two different
 *              global labels.
 *                one pin -> "P5V_IF_Rtn_Sense"
 *                one pin -> "P5V_IF_Rtn"
 *
 *   Sheet 2 -- TestPoint TP1 with two wires meeting at a junction; the
 *              junction is connected to both of these global labels.
 *                "P5V_IF_Rtn_Sense"
 *                "P5V_IF_Rtn"
 *
 *   Sheet 3 -- A single wire connecting two global labels.
 *                "P5V_IF_Rtn"
 *                "P4"
 *
 * Through transitivity, all four global names refer to the same logical net.
 *
 *     P5V_IF_Rtn_Sense  <-- shared via Sheet 2 -->  P5V_IF_Rtn
 *     P5V_IF_Rtn        <-- shared via Sheet 3 -->  P4
 *
 * Therefore both wired J1 pins and TP1's pin must all end up on the same net.
 *
 * The bug. When one of the multi-driver subgraphs (Sheet 3) gets an
 * alphabetically-lower primary name (here "P4"), the cross-sheet promotion
 * pass walked candidates by their *original* driver text instead of by their
 * already-promoted name. As a result, the J1 "P5V_IF_Rtn_Sense" subgraph,
 * which had been promoted to "P5V_IF_Rtn" by Sheet 2, was never re-promoted
 * to "P4" by Sheet 3 and ended up isolated on a stale net name.
 */
BOOST_FIXTURE_TEST_CASE( Issue23719, ISSUE_23719_FIXTURE )
{
    LOCALE_IO dummy;

    KI_TEST::LoadSchematic( m_settingsManager, "issue23719/issue23719", m_schematic );

    CONNECTION_GRAPH* graph = m_schematic->ConnectionGraph();

    struct PIN_NET
    {
        int      code;
        wxString name;
    };

    // For each (symbol-ref, pin-number) pair, capture the netcode and net name.
    // We don't assume which physical pin numbers wind up wired in the symbol's
    // local frame; only cross-sheet net consolidation matters here.
    std::map<wxString, std::map<wxString, PIN_NET>> symbolPins;

    for( const auto& [key, subgraphs] : graph->GetNetMap() )
    {
        for( CONNECTION_SUBGRAPH* subgraph : subgraphs )
        {
            for( SCH_ITEM* item : subgraph->GetItems() )
            {
                if( item->Type() != SCH_PIN_T )
                    continue;

                SCH_PIN*    pin    = static_cast<SCH_PIN*>( item );
                SCH_SYMBOL* symbol = dynamic_cast<SCH_SYMBOL*>( pin->GetParentSymbol() );

                if( !symbol )
                    continue;

                wxString ref    = symbol->GetRef( &subgraph->GetSheet() );
                wxString pinNum = pin->GetNumber();

                symbolPins[ref][pinNum] = { key.Netcode, key.Name };
            }
        }
    }

    BOOST_REQUIRE_MESSAGE( symbolPins.count( "J1" ),
                           "J1 connector should appear in the netlist" );
    BOOST_REQUIRE_MESSAGE( symbolPins.count( "TP1" ),
                           "TP1 testpoint should appear in the netlist" );

    const auto& j1Pins  = symbolPins["J1"];
    const auto& tp1Pins = symbolPins["TP1"];

    // J1 has two wired-up pins (the other two are NC / dangling). Both wired
    // pins live on globally-labeled sheets and must end up on the shared net.
    BOOST_REQUIRE_MESSAGE( j1Pins.size() >= 2,
                           wxString::Format( "J1 should have >= 2 connected pins, found %zu",
                                             j1Pins.size() ).ToStdString() );

    // TP1 has exactly one pin and it must be on the shared net.
    BOOST_REQUIRE_MESSAGE( tp1Pins.size() == 1,
                           wxString::Format( "TP1 should have exactly 1 connected pin, found %zu",
                                             tp1Pins.size() ).ToStdString() );

    const auto& tp1Pin = *tp1Pins.begin();
    const auto& j1PinA = *j1Pins.begin();
    const auto& j1PinB = *std::next( j1Pins.begin() );

    auto sameNetMsg =
            []( const wxString& aRefA, const auto& aPinA, const wxString& aRefB, const auto& aPinB )
            {
                return wxString::Format(
                               "%s-%s (%s, code=%d) and %s-%s (%s, code=%d) should share a net",
                               aRefA, aPinA.first, aPinA.second.name, aPinA.second.code,
                               aRefB, aPinB.first, aPinB.second.name, aPinB.second.code )
                        .ToStdString();
            };

    BOOST_CHECK_MESSAGE( j1PinA.second.code == tp1Pin.second.code,
                         sameNetMsg( "J1", j1PinA, "TP1", tp1Pin ) );

    BOOST_CHECK_MESSAGE( j1PinB.second.code == tp1Pin.second.code,
                         sameNetMsg( "J1", j1PinB, "TP1", tp1Pin ) );

    BOOST_CHECK_MESSAGE( j1PinA.second.code == j1PinB.second.code,
                         sameNetMsg( "J1", j1PinA, "J1", j1PinB ) );
}
