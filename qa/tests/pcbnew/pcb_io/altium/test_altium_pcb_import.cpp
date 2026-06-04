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
#include <board_stackup_manager/board_stackup.h>
#include <common.h>
#include <core/utf8.h>
#include <eda_text.h>
#include <netinfo.h>
#include <netclass.h>
#include <pcb_track.h>
#include <project.h>
#include <pcb_text.h>
#include <project/net_settings.h>
#include <settings/settings_manager.h>
#include <zone.h>

#include <map>
#include <string>
#include <vector>


struct ALTIUM_PCB_IMPORT_FIXTURE
{
    ALTIUM_PCB_IMPORT_FIXTURE() = default;

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


/**
 * Regression test for https://gitlab.com/kicad/code/kicad/-/issues/24458
 *
 * The Fastino board references every via's solder mask expansion from the hole edge.  KiCad vias
 * cannot represent a hole-referenced opening, so when the resulting opening does not clear the via
 * land the importer must tent the side instead of leaving the copper exposed.  Before the fix these
 * vias were imported untented, so verify the imported board tents both sides of every via.
 *
 * This is the same board used by the issue 24456 stackup test.
 */
BOOST_AUTO_TEST_CASE( Via_HoleReferencedMaskTenting )
{
    std::string dataPath = KI_TEST::GetPcbnewTestDataDir()
                           + "plugins/altium/issue24456/Fastino_Ground_Isolator.PcbDoc";

    std::unique_ptr<BOARD> board = std::make_unique<BOARD>();

    m_altiumPlugin.LoadBoard( dataPath, board.get(), nullptr );

    BOOST_REQUIRE( board );

    int viaCount = 0;
    int frontExposed = 0;
    int backExposed = 0;

    for( PCB_TRACK* track : board->Tracks() )
    {
        if( track->Type() != PCB_VIA_T )
            continue;

        const PCB_VIA* via = static_cast<const PCB_VIA*>( track );
        viaCount++;

        if( via->GetFrontTentingMode() != TENTING_MODE::TENTED )
            frontExposed++;

        if( via->GetBackTentingMode() != TENTING_MODE::TENTED )
            backExposed++;
    }

    BOOST_REQUIRE_GT( viaCount, 0 );

    // Every via on this board carries a hole-referenced mask opening narrower than its land, so the
    // importer must tent both sides of all of them.
    BOOST_CHECK_MESSAGE( frontExposed == 0,
                         wxString::Format( "%d of %d vias left front-exposed despite a "
                                           "hole-referenced mask",
                                           frontExposed, viaCount ) );
    BOOST_CHECK_MESSAGE( backExposed == 0,
                         wxString::Format( "%d of %d vias left back-exposed despite a "
                                           "hole-referenced mask",
                                           backExposed, viaCount ) );

    // Guard the tenting heuristic's boundary cases directly.  A wide hole-referenced opening that
    // clears the land must NOT tent, a land-referenced via must NOT be silently tented, and an
    // explicit Altium tent flag must always tent regardless of expansion mode.
    const uint32_t holeSize = 300000;     // 0.3mm
    const int      landWidth = 600000;    // 0.6mm

    BOOST_CHECK( !altiumViaSideIsTented( /*tentFlag*/ false, /*manual*/ true, /*fromHole*/ true,
                                         holeSize, /*expansion*/ 500000, landWidth ) );
    BOOST_CHECK( !altiumViaSideIsTented( /*tentFlag*/ false, /*manual*/ true, /*fromHole*/ false,
                                         holeSize, /*expansion*/ 30000, landWidth ) );
    BOOST_CHECK( altiumViaSideIsTented( /*tentFlag*/ true, /*manual*/ false, /*fromHole*/ false,
                                        holeSize, /*expansion*/ 0, landWidth ) );

    // A narrow hole-referenced opening (hole + 2 * expansion <= land) tents the side.
    BOOST_CHECK( altiumViaSideIsTented( /*tentFlag*/ false, /*manual*/ true, /*fromHole*/ true,
                                        holeSize, /*expansion*/ 30000, landWidth ) );
}


/**
 * Verify that the dielectric loss tangent is imported from the modern Altium physical stackup
 * (LAYER_V8_/V9_STACK_LAYER keys). The legacy LAYER<n> records used to build the stackup do not
 * carry the loss tangent, so it must be backfilled from the modern keys.
 *
 * Regression test for https://gitlab.com/kicad/code/kicad/-/issues/24456
 */
BOOST_AUTO_TEST_CASE( StackupDielectricLossTangent )
{
    std::string dataPath = KI_TEST::GetPcbnewTestDataDir()
                           + "plugins/altium/issue24456/Fastino_Ground_Isolator.PcbDoc";

    std::unique_ptr<BOARD> board = std::make_unique<BOARD>();

    m_altiumPlugin.LoadBoard( dataPath, board.get(), nullptr );

    BOOST_REQUIRE( board );

    const BOARD_STACKUP& stackup = board->GetDesignSettings().GetStackupDescriptor();

    int dielectricCount = 0;
    int dielectricWithTangent = 0;

    for( const BOARD_STACKUP_ITEM* item : stackup.GetList() )
    {
        if( item->GetType() != BS_ITEM_TYPE_DIELECTRIC )
            continue;

        for( int sub = 0; sub < item->GetSublayersCount(); sub++ )
        {
            // Only count dielectric sublayers that carry a real dielectric (non-zero thickness)
            if( item->GetThickness( sub ) <= 0 )
                continue;

            dielectricCount++;

            double tangent = item->GetLossTangent( sub );

            if( tangent > 0. )
            {
                dielectricWithTangent++;

                // Every prepreg/core dielectric in this board uses a 0.020 loss tangent.
                BOOST_CHECK_CLOSE( tangent, 0.020, 1e-6 );
            }
        }
    }

    BOOST_REQUIRE_GT( dielectricCount, 0 );

    // All of the board's substantive dielectrics carry a loss tangent in the Altium source, so
    // every imported dielectric sublayer must receive it.
    BOOST_CHECK_MESSAGE( dielectricWithTangent == dielectricCount,
                         wxString::Format( "Only %d of %d dielectric sublayers received a loss "
                                           "tangent from the Altium stackup",
                                           dielectricWithTangent, dielectricCount ) );
}


/**
 * Test that Altium project parameters (special strings) are imported as KiCad project text
 * variables so that board text such as ".PCB_Revision" resolves to its value.
 *
 * Regression test for https://gitlab.com/kicad/code/kicad/-/issues/24455
 */
BOOST_AUTO_TEST_CASE( ProjectParametersToTextVars )
{
    std::string dataDir = KI_TEST::GetPcbnewTestDataDir() + "plugins/altium/issue24456/";
    std::string pcbDoc = dataDir + "Fastino_Ground_Isolator.PcbDoc";
    std::string prjPcb = dataDir + "Fastino_Ground_Isolator.PrjPcb";

    SETTINGS_MANAGER settingsManager;
    settingsManager.LoadProject( "" );
    PROJECT& project = settingsManager.Prj();

    std::map<std::string, UTF8> props;
    props["project_file"] = prjPcb;

    std::unique_ptr<BOARD> board = std::make_unique<BOARD>();

    m_altiumPlugin.LoadBoard( pcbDoc, board.get(), &props, &project );

    const std::map<wxString, wxString>& textVars = project.GetTextVars();

    // The parameter that the issue reports as broken must resolve to its value.
    BOOST_REQUIRE( textVars.count( wxS( "PCB_REVISION" ) ) );
    BOOST_CHECK_EQUAL( textVars.at( wxS( "PCB_REVISION" ) ), wxS( "A" ) );

    // A representative sample of the remaining project parameters must all be present.
    BOOST_CHECK_EQUAL( textVars.at( wxS( "COMPANY_NAME" ) ), wxS( "ETH Zurich" ) );
    BOOST_CHECK_EQUAL( textVars.at( wxS( "PROJECT_NAME" ) ), wxS( "Fastino Ground Isolator" ) );
    BOOST_CHECK_EQUAL( textVars.at( wxS( "REVISION_MAJOR" ) ), wxS( "1" ) );
    BOOST_CHECK_EQUAL( textVars.at( wxS( "YEAR" ) ), wxS( "2026" ) );

    // Board text referencing the special string now resolves through the project variable.
    wxString resolved = ExpandTextVars( wxS( "${PCB_REVISION}" ), &project );
    BOOST_CHECK_EQUAL( resolved, wxS( "A" ) );

    // End-to-end: an actual imported board text that references ${PCB_REVISION} must render its
    // value once the board is linked to the project carrying the variable. This guards against a
    // regression in the Altium special-string conversion as well as the variable registration.
    board->SetProject( &project, true /* reference only */ );

    bool sawResolvedBoardText = false;

    for( BOARD_ITEM* item : board->Drawings() )
    {
        const EDA_TEXT* text = dynamic_cast<const EDA_TEXT*>( item );

        if( text && text->GetText().Contains( wxS( "${PCB_REVISION}" ) ) )
        {
            wxString shown = text->GetShownText( false );
            BOOST_CHECK( !shown.Contains( wxS( "${PCB_REVISION}" ) ) );
            BOOST_CHECK( shown.Contains( wxS( "A" ) ) );
            sawResolvedBoardText = true;
        }
    }

    BOOST_CHECK( sawResolvedBoardText );
}


/**
 * Test that importing project parameters does not overwrite text variables that already exist
 * on the project, and that names reserved for KiCad's contextual resolution are not imported.
 */
BOOST_AUTO_TEST_CASE( ProjectParametersPreserveExisting )
{
    std::string dataDir = KI_TEST::GetPcbnewTestDataDir() + "plugins/altium/issue24456/";
    std::string pcbDoc = dataDir + "Fastino_Ground_Isolator.PcbDoc";
    std::string prjPcb = dataDir + "Fastino_Ground_Isolator.PrjPcb";

    SETTINGS_MANAGER settingsManager;
    settingsManager.LoadProject( "" );
    PROJECT& project = settingsManager.Prj();
    project.GetTextVars()[wxS( "PCB_REVISION" )] = wxS( "user-set" );

    std::map<std::string, UTF8> props;
    props["project_file"] = prjPcb;

    std::unique_ptr<BOARD> board = std::make_unique<BOARD>();

    m_altiumPlugin.LoadBoard( pcbDoc, board.get(), &props, &project );

    // A pre-existing variable wins over the imported parameter.
    BOOST_CHECK_EQUAL( project.GetTextVars().at( wxS( "PCB_REVISION" ) ), wxS( "user-set" ) );

    // Other parameters are still imported.
    BOOST_CHECK_EQUAL( project.GetTextVars().at( wxS( "COMPANY_NAME" ) ), wxS( "ETH Zurich" ) );
}


/**
 * Regression test for https://gitlab.com/kicad/code/kicad/-/issues/24504
 *
 * The Fastino board places several labels as a coincident pair of free strings, one on a copper
 * layer and one on the matching soldermask layer.  Altium auto-sizes each string's bounding box
 * from its own rasterizer and may store a different box width (and stroke width) for the two
 * copies, so anchoring the imported KiCad text to that per-record box width pushed the copper and
 * mask copies apart.  Verify that every copper-layer free string that has a matching soldermask
 * free string (same text, mirroring and rotation) is imported to the same position.
 */
BOOST_AUTO_TEST_CASE( CopperAndMaskTextCoincide )
{
    std::string dataPath = KI_TEST::GetPcbnewTestDataDir()
                           + "plugins/altium/issue24456/Fastino_Ground_Isolator.PcbDoc";

    std::unique_ptr<BOARD> board = std::make_unique<BOARD>();
    m_altiumPlugin.LoadBoard( dataPath, board.get(), nullptr );
    BOOST_REQUIRE( board );

    std::vector<PCB_TEXT*> copperTexts;
    std::vector<PCB_TEXT*> maskTexts;

    for( BOARD_ITEM* item : board->Drawings() )
    {
        if( item->Type() != PCB_TEXT_T )
            continue;

        PCB_TEXT*    text = static_cast<PCB_TEXT*>( item );
        PCB_LAYER_ID layer = text->GetLayer();

        if( IsCopperLayer( layer ) )
            copperTexts.push_back( text );
        else if( layer == F_Mask || layer == B_Mask )
            maskTexts.push_back( text );
    }

    BOOST_REQUIRE_MESSAGE( !copperTexts.empty(), "Board must contain copper-layer text" );
    BOOST_REQUIRE_MESSAGE( !maskTexts.empty(), "Board must contain soldermask-layer text" );

    // A copper string and a mask string are the same logical label when they share text, rotation
    // and mirroring.  KiCad's IU tolerance for "coincident" is tight; before the fix the gap was
    // 50000-75000 IU (0.05-0.075 mm).
    const int tolerance = 1000; // 1 micron

    int matchedPairs = 0;

    for( PCB_TEXT* copper : copperTexts )
    {
        for( PCB_TEXT* mask : maskTexts )
        {
            if( copper->GetText() != mask->GetText()
                || copper->GetTextAngle() != mask->GetTextAngle()
                || copper->IsMirrored() != mask->IsMirrored() )
            {
                continue;
            }

            // Only treat them as the same label when they are already near each other; distinct
            // labels that happen to share a glyph (e.g. several "+" pads) must not be cross-matched.
            VECTOR2I delta = copper->GetTextPos() - mask->GetTextPos();

            if( std::abs( delta.x ) > 500000 || std::abs( delta.y ) > 500000 )
                continue;

            matchedPairs++;

            BOOST_CHECK_MESSAGE(
                    std::abs( delta.x ) <= tolerance && std::abs( delta.y ) <= tolerance,
                    wxString::Format( "Copper/mask copies of '%s' diverge by (%d, %d) IU",
                                      copper->GetText(), delta.x, delta.y ) );
        }
    }

    BOOST_CHECK_MESSAGE( matchedPairs > 0,
                         "Expected at least one coincident copper/soldermask text pair" );
}


BOOST_AUTO_TEST_SUITE_END()
