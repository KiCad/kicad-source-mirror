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
 * @file test_issue23840_bus_group_to_vector.cpp
 *
 * Test for bus group to bus vector connections through hierarchical sheet pins.
 *
 * The schematic has two instances of the same sub-sheet (LEDs.kicad_sch) containing
 * 8 LEDs with resistors, driven by bus vector LEDs[0..7]:
 *
 * Instance 1 (LED_A): CON_A_[1..8] (bus vector) -> sheet pin LEDs[0...7]
 *   Components: D1-D8, R1-R8.  Standard vector-to-vector, already works.
 *
 * Instance 2 (LED_B): {CON_B_7 CON_B_8 CON_B_[1..3] CON_B_[4..6]} (bus group)
 *   -> sheet pin LEDs[0...7]
 *   Components: D9-D16, R9-R16.  Cross-type group-to-vector, positional mapping.
 *
 * Expected positional mapping for instance 2:
 *   LEDs0 -> CON_B_7,  LEDs1 -> CON_B_8,
 *   LEDs2 -> CON_B_1,  LEDs3 -> CON_B_2,  LEDs4 -> CON_B_3,
 *   LEDs5 -> CON_B_4,  LEDs6 -> CON_B_5,  LEDs7 -> CON_B_6
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


struct ISSUE23840_FIXTURE
{
    ISSUE23840_FIXTURE() {}

    SETTINGS_MANAGER           m_settingsManager;
    std::unique_ptr<SCHEMATIC> m_schematic;
};


BOOST_FIXTURE_TEST_CASE( Issue23840BusGroupToVector, ISSUE23840_FIXTURE )
{
    LOCALE_IO dummy;

    KI_TEST::LoadSchematic( m_settingsManager, "issue23840/BusAndVectors", m_schematic );

    SCH_SHEET_LIST    sheets = m_schematic->BuildSheetListSortedByPageNumbers();
    CONNECTION_GRAPH* graph = m_schematic->ConnectionGraph();

    // Collect net names keyed by "ref.pin" across all sheet instances.
    std::map<wxString, wxString> pinNetNames;

    for( const SCH_SHEET_PATH& path : sheets )
    {
        for( SCH_ITEM* item : path.LastScreen()->Items().OfType( SCH_SYMBOL_T ) )
        {
            SCH_SYMBOL* symbol = static_cast<SCH_SYMBOL*>( item );
            wxString    ref = symbol->GetRef( &path );

            for( SCH_PIN* pin : symbol->GetPins( &path ) )
            {
                SCH_CONNECTION* conn = pin->Connection( &path );

                if( conn )
                {
                    wxString pinKey = ref + "." + pin->GetNumber();
                    pinNetNames[pinKey] = conn->Name();
                }
            }
        }
    }

    // Instance 2 (LED_B): bus group {CON_B_7 CON_B_8 CON_B_[1..3] CON_B_[4..6]} -> LEDs[0..7]
    // Positional mapping: R9.1=CON_B_7, R10.1=CON_B_8, R11.1=CON_B_1, R12.1=CON_B_2,
    //                     R13.1=CON_B_3, R14.1=CON_B_4, R15.1=CON_B_5, R16.1=CON_B_6
    const std::vector<wxString> expectedNets = { "CON_B_7", "CON_B_8", "CON_B_1", "CON_B_2",
                                                 "CON_B_3", "CON_B_4", "CON_B_5", "CON_B_6" };

    for( int i = 0; i < 8; i++ )
    {
        wxString ref = wxString::Format( "R%d.1", i + 9 );
        BOOST_REQUIRE_MESSAGE( pinNetNames.count( ref ), wxString::Format( "%s should have a net assigned", ref ) );

        wxString netName = pinNetNames[ref];
        BOOST_CHECK_MESSAGE( netName == expectedNets[i],
                             wxString::Format( "%s expected %s, got %s", ref, expectedNets[i], netName ) );
    }
}
