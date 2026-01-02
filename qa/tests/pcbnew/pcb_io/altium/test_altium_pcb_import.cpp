/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, you may find one here:
 * http://www.gnu.org/licenses/old-licenses/gpl-2.0.html
 * or you may search the http://www.gnu.org website for the version 2 license,
 * or you may write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
 */

/**
 * @file test_altium_pcb_import.cpp
 * Test suite for import of Altium PCB files
 */

#include <pcbnew_utils/board_test_utils.h>
#include <pcbnew_utils/board_file_utils.h>
#include <qa_utils/wx_utils/unit_test_utils.h>

#include <pcbnew/pcb_io/altium/pcb_io_altium_designer.h>

#include <board.h>
#include <board_design_settings.h>
#include <netinfo.h>
#include <netclass.h>
#include <project/net_settings.h>


struct ALTIUM_PCB_IMPORT_FIXTURE
{
    ALTIUM_PCB_IMPORT_FIXTURE() {}

    PCB_IO_ALTIUM_DESIGNER m_altiumPlugin;
};


BOOST_FIXTURE_TEST_SUITE( AltiumPcbImport, ALTIUM_PCB_IMPORT_FIXTURE )


/**
 * Test basic board loading - verifies that the Altium import doesn't trigger any assertions
 * during the load process. This catches regressions in layer mapping and other import issues.
 */
BOOST_AUTO_TEST_CASE( BoardLoadNoAssertions )
{
    std::string dataPath = KI_TEST::GetPcbnewTestDataDir()
                           + "plugins/altium/eDP_adapter_dvt1_source/eDP_adapter_dvt1.PcbDoc";

    std::unique_ptr<BOARD> board = std::make_unique<BOARD>();

    // Load the board - should not trigger any assertions
    m_altiumPlugin.LoadBoard( dataPath, board.get(), nullptr );

    BOOST_REQUIRE( board );

    // Basic sanity checks
    BOOST_CHECK( board->GetNetCount() > 0 );
    BOOST_CHECK( board->Footprints().size() > 0 );
}


/**
 * Test that netclass pattern assignments result in direct netclass assignments on nets.
 * This is a regression test for https://gitlab.com/kicad/code/kicad/-/issues/15584
 *
 * Validates that when an Altium board with netclasses is imported, the nets are directly
 * assigned to their netclasses (not just through pattern resolution).
 */
BOOST_AUTO_TEST_CASE( NetclassAssignment )
{
    // HiFive1.B01.PcbDoc has Altium netclass definitions
    std::string dataPath = KI_TEST::GetPcbnewTestDataDir() + "plugins/altium/HiFive/HiFive1.B01.PcbDoc";

    std::unique_ptr<BOARD> board = std::make_unique<BOARD>();

    m_altiumPlugin.LoadBoard( dataPath, board.get(), nullptr );

    BOOST_REQUIRE( board );

    // Get the net settings which contains pattern assignments
    std::shared_ptr<NET_SETTINGS> netSettings = board->GetDesignSettings().m_NetSettings;

    BOOST_REQUIRE( netSettings );

    // Check if there are any pattern assignments in the board
    auto& patternAssignments = netSettings->GetNetclassPatternAssignments();

    // The HiFive board should have netclass definitions - require this for the test to be meaningful
    BOOST_REQUIRE_MESSAGE( patternAssignments.size() > 0,
                           "Test file must have netclass pattern assignments" );

    // For each net that has a pattern assignment, verify that the NETINFO_ITEM
    // has a netclass directly assigned (not just through pattern resolution)
    bool foundAssignedNet = false;

    for( NETINFO_ITEM* net : board->GetNetInfo() )
    {
        if( net->GetNetCode() <= 0 )
            continue;

        // Get the netclass directly from the NETINFO_ITEM
        NETCLASS* directNetclass = net->GetNetClass();

        // Get the effective netclass from pattern resolution
        std::shared_ptr<NETCLASS> effectiveNetclass =
                netSettings->GetEffectiveNetClass( net->GetNetname() );

        // If this net has a non-default effective netclass, the direct assignment
        // should also be non-default (this is what the fix ensures)
        if( effectiveNetclass && effectiveNetclass->GetName() != NETCLASS::Default )
        {
            BOOST_CHECK_MESSAGE(
                    directNetclass != nullptr,
                    wxString::Format( "Net '%s' should have a direct netclass assignment",
                                      net->GetNetname() ) );

            if( directNetclass )
            {
                foundAssignedNet = true;

                // The direct netclass should match what effective resolution returns
                // (or be part of the effective class for multi-netclass scenarios)
                BOOST_CHECK_MESSAGE(
                        directNetclass->GetName() != NETCLASS::Default,
                        wxString::Format( "Net '%s' should not have default netclass, "
                                          "expected effective class or component",
                                          net->GetNetname() ) );
            }
        }
    }

    // If there were pattern assignments, we should have found at least one assigned net
    BOOST_CHECK_MESSAGE( foundAssignedNet,
                         "At least one net should have a non-default netclass assigned" );
}


BOOST_AUTO_TEST_SUITE_END()
