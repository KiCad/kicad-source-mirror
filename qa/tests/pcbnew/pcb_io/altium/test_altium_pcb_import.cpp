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
#include <pcbnew/pcb_io/altium/altium_parser_pcb.h>

#include <board.h>
#include <board_design_settings.h>
#include <netinfo.h>
#include <netclass.h>
#include <project/net_settings.h>
#include <zone.h>


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


/**
 * Verify that copper zones in imported Altium boards have non-zero local clearance values
 * derived from rules whose scope expressions match polygons.
 * Regression test for https://gitlab.com/kicad/code/kicad/-/issues/18408
 */
static void checkAllCopperFillZonesHaveClearance( PCB_IO_ALTIUM_DESIGNER& aPlugin,
                                                  const std::string& aRelativePath )
{
    std::string dataPath = KI_TEST::GetPcbnewTestDataDir() + aRelativePath;

    std::unique_ptr<BOARD> board = std::make_unique<BOARD>();
    aPlugin.LoadBoard( dataPath, board.get(), nullptr );

    BOOST_REQUIRE( board );

    int fillZoneCount = 0;
    int fillZonesWithClearance = 0;

    for( ZONE* zone : board->Zones() )
    {
        if( !zone->IsOnCopperLayer() || zone->GetIsRuleArea() || zone->IsTeardropArea() )
            continue;

        fillZoneCount++;

        if( zone->GetLocalClearance().has_value() && zone->GetLocalClearance().value() > 0 )
            fillZonesWithClearance++;
    }

    BOOST_CHECK_GT( fillZoneCount, 0 );

    BOOST_CHECK_MESSAGE( fillZonesWithClearance == fillZoneCount,
                         wxString::Format( "%s: %d/%d copper fill zones have clearance set",
                                           aRelativePath, fillZonesWithClearance,
                                           fillZoneCount ) );
}


BOOST_AUTO_TEST_CASE( ZoneClearances_eDP )
{
    checkAllCopperFillZonesHaveClearance(
            m_altiumPlugin, "plugins/altium/eDP_adapter_dvt1_source/eDP_adapter_dvt1.PcbDoc" );
}


BOOST_AUTO_TEST_CASE( ZoneClearances_HiFive )
{
    checkAllCopperFillZonesHaveClearance( m_altiumPlugin,
                                          "plugins/altium/HiFive/HiFive1.B01.PcbDoc" );
}


/**
 * Test altiumScopeExprMatchesPolygon utility function.
 * Regression test for https://gitlab.com/kicad/code/kicad/-/issues/18408
 *
 * Validates that the scope expression matching correctly identifies polygon-related
 * Altium rule scope expressions used for zone clearance rules.
 */
BOOST_AUTO_TEST_CASE( ScopeExprMatchesPolygon )
{
    // Positive matches: expressions that reference polygons
    BOOST_CHECK( altiumScopeExprMatchesPolygon( wxT( "InPolygon" ) ) );
    BOOST_CHECK( altiumScopeExprMatchesPolygon( wxT( "InPoly" ) ) );
    BOOST_CHECK( altiumScopeExprMatchesPolygon( wxT( "IsPolygon" ) ) );
    BOOST_CHECK( altiumScopeExprMatchesPolygon( wxT( "IsPoly" ) ) );

    // Case insensitivity
    BOOST_CHECK( altiumScopeExprMatchesPolygon( wxT( "inpolygon" ) ) );
    BOOST_CHECK( altiumScopeExprMatchesPolygon( wxT( "INPOLYGON" ) ) );
    BOOST_CHECK( altiumScopeExprMatchesPolygon( wxT( "inPOLY" ) ) );

    // Contained within longer expressions
    BOOST_CHECK( altiumScopeExprMatchesPolygon( wxT( "InPolygon And InNet('GND')" ) ) );
    BOOST_CHECK( altiumScopeExprMatchesPolygon( wxT( "(InPoly) Or IsVia" ) ) );

    // Negative matches: expressions that don't reference polygons
    BOOST_CHECK( !altiumScopeExprMatchesPolygon( wxT( "All" ) ) );
    BOOST_CHECK( !altiumScopeExprMatchesPolygon( wxT( "IsVia" ) ) );
    BOOST_CHECK( !altiumScopeExprMatchesPolygon( wxT( "IsTrack" ) ) );
    BOOST_CHECK( !altiumScopeExprMatchesPolygon( wxT( "InNet('GND')" ) ) );
    BOOST_CHECK( !altiumScopeExprMatchesPolygon( wxT( "InComponent('U1')" ) ) );
    BOOST_CHECK( !altiumScopeExprMatchesPolygon( wxT( "" ) ) );
}


/**
 * Verify selectAltiumPolygonRule honours Altium priority order (1 = highest).
 * Regression guard for selecting a less-specific polygon rule when multiple
 * polygon-scoped rules exist alongside default (All/All) rules.
 */
BOOST_AUTO_TEST_CASE( SelectAltiumPolygonRule_PriorityOrder )
{
    auto makeRule = []( int aPriority, const wxString& aScope1, const wxString& aScope2,
                        int aClearance )
    {
        ARULE6 rule;
        rule.priority = aPriority;
        rule.scope1expr = aScope1;
        rule.scope2expr = aScope2;
        rule.clearanceGap = aClearance;
        return rule;
    };

    // Sorted by priority ascending, matching the order produced by ParseRules6Data.
    std::vector<ARULE6> rules = {
        makeRule( 1, wxT( "InPolygon And InNet('GND')" ), wxT( "All" ), 100 ),
        makeRule( 2, wxT( "InPolygon" ), wxT( "All" ), 200 ),
        makeRule( 3, wxT( "All" ), wxT( "All" ), 300 ),
        makeRule( 4, wxT( "All" ), wxT( "All" ), 400 ),
    };

    const ARULE6* selected = selectAltiumPolygonRule( rules );
    BOOST_REQUIRE( selected != nullptr );
    BOOST_CHECK_EQUAL( selected->priority, 1 );
    BOOST_CHECK_EQUAL( selected->clearanceGap, 100 );

    rules.erase( rules.begin() );
    selected = selectAltiumPolygonRule( rules );
    BOOST_REQUIRE( selected != nullptr );
    BOOST_CHECK_EQUAL( selected->priority, 2 );
    BOOST_CHECK_EQUAL( selected->clearanceGap, 200 );

    rules.erase( rules.begin() );
    BOOST_CHECK( selectAltiumPolygonRule( rules ) == nullptr );

    BOOST_CHECK( selectAltiumPolygonRule( {} ) == nullptr );
}


BOOST_AUTO_TEST_SUITE_END()
