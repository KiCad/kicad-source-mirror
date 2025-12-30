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
 * @file test_bus_net_name_determinism.cpp
 * Test for issue 18606: net name of shorted nets should be deterministic
 * when part of a higher-level bus definition.
 *
 * When multiple nets from a bus (e.g., A0, A1, A2, A3 from bus A[0..3]) are
 * shorted together, the resulting combined net name should be deterministically
 * chosen (alphabetically first: A0).
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

struct BUS_NET_NAME_DETERMINISM_FIXTURE
{
    BUS_NET_NAME_DETERMINISM_FIXTURE()
    { }

    SETTINGS_MANAGER           m_settingsManager;
    std::unique_ptr<SCHEMATIC> m_schematic;
};


/**
 * Test that when bus member nets (A0, A1, A2, A3) are shorted together,
 * the resulting net name is deterministic (should be "A0" - alphabetically first).
 *
 * The test schematic has:
 * - Parent sheet with bus A[0..3] connected to a hierarchical sheet
 * - Child sheet with hierarchical label A[0..3], bus entries to A0, A1, A2, A3
 * - All four nets (A0, A1, A2, A3) are shorted together via junctions
 * - A resistor R201 is connected to the shorted nets
 */
BOOST_FIXTURE_TEST_CASE( ShortedBusNetsHaveDeterministicName, BUS_NET_NAME_DETERMINISM_FIXTURE )
{
    LOCALE_IO dummy;

    // Load the test schematic multiple times to verify determinism
    for( int iteration = 0; iteration < 3; ++iteration )
    {
        KI_TEST::LoadSchematic( m_settingsManager, "issue18606/issue18606", m_schematic );

        SCH_SHEET_LIST sheets = m_schematic->BuildSheetListSortedByPageNumbers();

        // Find the resistor R201 in the child sheet and check its pin's net name
        wxString foundNetName;
        bool     foundResistor = false;

        for( const SCH_SHEET_PATH& path : sheets )
        {
            SCH_SCREEN* screen = path.LastScreen();

            for( SCH_ITEM* item : screen->Items().OfType( SCH_SYMBOL_T ) )
            {
                SCH_SYMBOL* symbol = static_cast<SCH_SYMBOL*>( item );

                if( symbol->GetRef( &path ) == "R201" )
                {
                    foundResistor = true;

                    for( SCH_PIN* pin : symbol->GetPins( &path ) )
                    {
                        SCH_CONNECTION* conn = pin->Connection( &path );

                        if( conn )
                        {
                            foundNetName = conn->Name();
                            break;
                        }
                    }

                    break;
                }
            }

            if( foundResistor )
                break;
        }

        BOOST_CHECK_MESSAGE( foundResistor, "R201 should be found in the schematic" );

        // The net name should be deterministic - alphabetically "A0" should win
        // when A0, A1, A2, A3 are shorted together.
        // The path is "/" (not "/test/") because the net name is inherited from the
        // parent sheet's bus A[0..3] during hierarchical propagation.
        BOOST_CHECK_MESSAGE( foundNetName == "/A0",
                             "Net name should be '/A0' (alphabetically first bus member), "
                             "but got '" << foundNetName.ToStdString() << "' on iteration "
                             << iteration );
    }
}
