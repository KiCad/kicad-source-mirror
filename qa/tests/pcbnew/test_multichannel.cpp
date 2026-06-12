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

#include <qa_utils/wx_utils/unit_test_utils.h>
#include <pcbnew_utils/board_test_utils.h>
#include <board.h>
#include <board_design_settings.h>
#include <pad.h>
#include <pcb_group.h>
#include <pcb_shape.h>
#include <pcb_track.h>
#include <pcb_text.h>
#include <pcb_field.h>
#include <footprint.h>
#include <zone.h>
#include <drc/drc_item.h>
#include <settings/settings_manager.h>
#include <tools/multichannel_tool.h>
#include <connectivity/topo_match.h>
#include <geometry/shape_line_chain.h>
#include <geometry/shape_utils.h>
#include <lib_id.h>
#include <atomic>

struct MULTICHANNEL_TEST_FIXTURE
{
    MULTICHANNEL_TEST_FIXTURE() {}

    SETTINGS_MANAGER       m_settingsManager;
    std::unique_ptr<BOARD> m_board;
};

class MOCK_TOOLS_HOLDER : public TOOLS_HOLDER
{
public:
     MOCK_TOOLS_HOLDER(){};
    ~MOCK_TOOLS_HOLDER(){};

    virtual wxWindow* GetToolCanvas() const override { return nullptr; }
};

BOOST_FIXTURE_TEST_SUITE( MultichannelTool, MULTICHANNEL_TEST_FIXTURE )

RULE_AREA* findRuleAreaByPartialName( MULTICHANNEL_TOOL* aTool, const wxString& aName )
{
    for( RULE_AREA& ra : aTool->GetData()->m_areas )
    {
        if( ra.m_ruleName.Contains( ( aName ) ) )
            return &ra;
    }

    return nullptr;
}

RULE_AREA* findRuleAreaByPlacementGroup( MULTICHANNEL_TOOL* aTool, const wxString& aGroupName )
{
    for( RULE_AREA& ra : aTool->GetData()->m_areas )
    {
        if( ra.m_zone && ra.m_zone->GetPlacementAreaSource() == aGroupName )
            return &ra;
    }

    return nullptr;
}

int countZonesByNameInRuleArea( BOARD* aBoard, const wxString& aZoneName, const RULE_AREA& aRuleArea )
{
    int count = 0;

    for( const ZONE* zone : aBoard->Zones() )
    {
        if( zone == aRuleArea.m_zone )
            continue;

        if( zone->GetZoneName() != aZoneName )
            continue;

        if( aRuleArea.m_zone->Outline()->Contains( zone->Outline()->COutline( 0 ).Centre() ) )
            count++;
    }

    return count;
}


BOOST_FIXTURE_TEST_CASE( MultichannelToolRegressions, MULTICHANNEL_TEST_FIXTURE )
{
    using TMATCH::CONNECTION_GRAPH;

    std::vector<wxString> tests = { "vme-wren" };

    for( const wxString& relPath : tests )
    {
        KI_TEST::LoadBoard( m_settingsManager, relPath, m_board );

        TOOL_MANAGER       toolMgr;
        MOCK_TOOLS_HOLDER* toolsHolder = new MOCK_TOOLS_HOLDER;

        toolMgr.SetEnvironment( m_board.get(), nullptr, nullptr, nullptr, toolsHolder );

        MULTICHANNEL_TOOL* mtTool = new MULTICHANNEL_TOOL; // TOOL_MANAGER owns the tools
        toolMgr.RegisterTool( mtTool );

        //RULE_AREAS_DATA* raData = m_parentTool->GetData();

        mtTool->GeneratePotentialRuleAreas();

        auto ruleData = mtTool->GetData();

        BOOST_TEST_MESSAGE( wxString::Format( "RA multichannel sheets = %d",
                                              static_cast<int>( ruleData->m_areas.size() ) ) );

        BOOST_CHECK_EQUAL( ruleData->m_areas.size(), 72 );

        int cnt = 0;

        ruleData->m_replaceExisting = true;

        for( RULE_AREA& ra : ruleData->m_areas )
        {
            if( ra.m_sheetName == wxT( "io_driver.kicad_sch" )
                || ra.m_sheetName == wxT( "pp_driver_2x.kicad_sch" ) )
            {
                ra.m_generateEnabled = true;
                cnt++;
            }
        }

        BOOST_TEST_MESSAGE( wxString::Format( "Autogenerating %d RAs", cnt ) );

        TOOL_EVENT dummyEvent;

        mtTool->AutogenerateRuleAreas( dummyEvent );
        mtTool->FindExistingRuleAreas();

        int n_areas_io = 0, n_areas_pp = 0, n_areas_other = 0;

        BOOST_TEST_MESSAGE( wxString::Format( "Found %d RAs after commit",
                                              static_cast<int>(ruleData->m_areas.size() ) ) );

        for( const RULE_AREA& ra : ruleData->m_areas )
        {
            BOOST_TEST_MESSAGE( wxString::Format( "SN '%s'", ra.m_ruleName ) );

            if( ra.m_ruleName.Contains( wxT( "io_drivers_fp" ) ) )
            {
                n_areas_io++;
                BOOST_CHECK_EQUAL( ra.m_components.size(), 31 );
            }
            else if( ra.m_ruleName.Contains( wxT( "io_drivers_pp" ) ) )
            {
                n_areas_pp++;
                BOOST_CHECK_EQUAL( ra.m_components.size(), 11 );
            }
            else
            {
                n_areas_other++;
            }
        }

        BOOST_TEST_MESSAGE( wxString::Format( "IO areas=%d, PP areas=%d, others=%d",
                                              n_areas_io, n_areas_pp, n_areas_other ) );

        BOOST_CHECK_EQUAL( n_areas_io, 16 );
        BOOST_CHECK_EQUAL( n_areas_pp, 16 );
        BOOST_CHECK_EQUAL( n_areas_other, 0 );

        const std::vector<wxString> rulesToTest = { wxT( "io_drivers_fp" ),
                                                    wxT( "io_drivers_pp" ) };

        for( const wxString& ruleName : rulesToTest )
        {
            for( const RULE_AREA& refArea : ruleData->m_areas )
            {
                if( !refArea.m_ruleName.Contains( ruleName ) )
                    continue;

                BOOST_TEST_MESSAGE( wxString::Format( "REF AREA: '%s'", refArea.m_ruleName ) );

                for( const RULE_AREA& targetArea : ruleData->m_areas )
                {
                    if( targetArea.m_zone == refArea.m_zone )
                        continue;

                    if( !targetArea.m_ruleName.Contains( ruleName ) )
                        continue;

                    auto cgRef = CONNECTION_GRAPH::BuildFromFootprintSet( refArea.m_components,
                                                                          targetArea.m_components );
                    auto cgTarget =
                        CONNECTION_GRAPH::BuildFromFootprintSet( targetArea.m_components,
                                                                  refArea.m_components );

                    TMATCH::COMPONENT_MATCHES result;

                    std::vector<TMATCH::TOPOLOGY_MISMATCH_REASON> details;
                    bool status = cgRef->FindIsomorphism( cgTarget.get(), result, details );

                    BOOST_TEST_MESSAGE( wxString::Format(
                            "topo match: '%s' [%d] -> '%s' [%d] result %d", refArea.m_ruleName.c_str().AsChar(),
                            static_cast<int>( refArea.m_components.size() ), targetArea.m_ruleName.c_str().AsChar(),
                            static_cast<int>( targetArea.m_components.size() ), status ? 1 : 0 ) );

                    for( const auto& iter : result )
                    {
                        BOOST_TEST_MESSAGE( wxString::Format( "%s : %s",
                                                              iter.second->GetReference(),
                                                              iter.first->GetReference() ) );
                    }

                    BOOST_CHECK( status );
                    BOOST_CHECK( details.empty() );
                }
            }
        }

        auto refArea = findRuleAreaByPartialName( mtTool, wxT( "io_drivers_fp/bank3/io78/" ) );

        BOOST_ASSERT( refArea );

        const std::vector<wxString> targetAreaNames( { wxT( "io_drivers_fp/bank2/io78/" ),
                                                       wxT( "io_drivers_fp/bank1/io78/" ),
                                                       wxT( "io_drivers_fp/bank0/io01/" ) } );

        for( const wxString& targetRaName : targetAreaNames )
        {
            auto targetRA = findRuleAreaByPartialName( mtTool, targetRaName );

            BOOST_ASSERT( targetRA != nullptr );

            BOOST_TEST_MESSAGE( wxString::Format( "Clone to: %s", targetRA->m_ruleName ) );

            ruleData->m_compatMap[targetRA].m_doCopy = true;
        }

        int result = mtTool->RepeatLayout( TOOL_EVENT(), refArea->m_zone );

        BOOST_ASSERT( result >= 0 );
    }
}


/**
 * Test that repeat layout copies footprint properties including field visibility,
 * text positions, and 3D models (issue 22548).
 */
BOOST_FIXTURE_TEST_CASE( RepeatLayoutCopiesFootprintProperties, MULTICHANNEL_TEST_FIXTURE )
{
    KI_TEST::LoadBoard( m_settingsManager, "issue22548/issue22548", m_board );

    TOOL_MANAGER       toolMgr;
    MOCK_TOOLS_HOLDER* toolsHolder = new MOCK_TOOLS_HOLDER;

    toolMgr.SetEnvironment( m_board.get(), nullptr, nullptr, nullptr, toolsHolder );

    MULTICHANNEL_TOOL* mtTool = new MULTICHANNEL_TOOL;
    toolMgr.RegisterTool( mtTool );

    mtTool->FindExistingRuleAreas();

    auto ruleData = mtTool->GetData();

    BOOST_TEST_MESSAGE( wxString::Format( "Found %d rule areas",
                                          static_cast<int>( ruleData->m_areas.size() ) ) );

    BOOST_CHECK( ruleData->m_areas.size() >= 2 );

    if( ruleData->m_areas.size() < 2 )
        return;

    RULE_AREA* refArea = nullptr;
    RULE_AREA* targetArea = nullptr;

    for( RULE_AREA& ra : ruleData->m_areas )
    {
        if( ra.m_ruleName.Contains( wxT( "Untitled Sheet/" ) ) )
            refArea = &ra;
        else if( ra.m_ruleName.Contains( wxT( "Untitled Sheet1/" ) ) )
            targetArea = &ra;
    }

    if( !refArea || !targetArea )
    {
        BOOST_TEST_MESSAGE( "Could not find Untitled Sheet and Untitled Sheet1 rule areas, skipping test" );
        return;
    }

    BOOST_TEST_MESSAGE( wxString::Format( "Reference area: %s, Target area: %s",
                                          refArea->m_ruleName, targetArea->m_ruleName ) );

    FOOTPRINT* refFP = nullptr;
    FOOTPRINT* targetFP = nullptr;

    for( FOOTPRINT* fp : refArea->m_components )
    {
        if( fp->GetReference().StartsWith( wxT( "U1" ) ) )
        {
            refFP = fp;
            break;
        }
    }

    for( FOOTPRINT* fp : targetArea->m_components )
    {
        if( fp->GetReference().StartsWith( wxT( "U2" ) ) )
        {
            targetFP = fp;
            break;
        }
    }

    if( !refFP || !targetFP )
    {
        BOOST_TEST_MESSAGE( "Could not find matching footprints in the rule areas, skipping test" );
        return;
    }

    PCB_FIELD* refValueField = refFP->GetField( FIELD_T::VALUE );
    bool refValueVisible = refValueField ? refValueField->IsVisible() : true;

    std::vector<FP_3DMODEL> refModels = refFP->Models();

    mtTool->CheckRACompatibility( refArea->m_zone );

    ruleData->m_compatMap[targetArea].m_doCopy = true;
    ruleData->m_options.m_copyPlacement = true;

    int result = mtTool->RepeatLayout( TOOL_EVENT(), refArea->m_zone );

    BOOST_CHECK( result >= 0 );

    PCB_FIELD* targetValueField = targetFP->GetField( FIELD_T::VALUE );

    if( targetValueField && refValueField )
    {
        BOOST_CHECK_EQUAL( targetValueField->IsVisible(), refValueVisible );
        BOOST_TEST_MESSAGE( wxString::Format( "Value field visibility: ref=%d, target=%d",
                                              refValueVisible, targetValueField->IsVisible() ) );
    }

    BOOST_CHECK_EQUAL( targetFP->Models().size(), refModels.size() );

    if( !refModels.empty() )
    {
        BOOST_TEST_MESSAGE( wxString::Format( "3D models: ref=%d, target=%d",
                                              static_cast<int>( refModels.size() ),
                                              static_cast<int>( targetFP->Models().size() ) ) );
    }
}


/**
 * Test that repeat layout does not remove vias from the reference area when a copper zone
 * has the same name as one of the rule areas (issue 21184).
 *
 * The bug occurred because enclosedByArea() matched zones by name, and when a copper fill zone
 * shared a name with a rule area, items enclosed by either zone could be incorrectly affected.
 */
BOOST_FIXTURE_TEST_CASE( RepeatLayoutDoesNotRemoveReferenceVias, MULTICHANNEL_TEST_FIXTURE )
{
    KI_TEST::LoadBoard( m_settingsManager, "issue21184/issue21184", m_board );

    TOOL_MANAGER       toolMgr;
    MOCK_TOOLS_HOLDER* toolsHolder = new MOCK_TOOLS_HOLDER;

    toolMgr.SetEnvironment( m_board.get(), nullptr, nullptr, nullptr, toolsHolder );

    MULTICHANNEL_TOOL* mtTool = new MULTICHANNEL_TOOL;
    toolMgr.RegisterTool( mtTool );

    mtTool->FindExistingRuleAreas();

    auto ruleData = mtTool->GetData();

    BOOST_TEST_MESSAGE( wxString::Format( "Found %d rule areas",
                                          static_cast<int>( ruleData->m_areas.size() ) ) );

    BOOST_CHECK_EQUAL( ruleData->m_areas.size(), 2 );

    if( ruleData->m_areas.size() < 2 )
        return;

    RULE_AREA* refArea = nullptr;
    RULE_AREA* targetArea = nullptr;

    for( RULE_AREA& ra : ruleData->m_areas )
    {
        if( ra.m_ruleName == wxT( "Test1" ) )
            refArea = &ra;
        else if( ra.m_ruleName == wxT( "Test2" ) )
            targetArea = &ra;
    }

    BOOST_REQUIRE( refArea != nullptr );
    BOOST_REQUIRE( targetArea != nullptr );

    int refViaCountBefore = 0;

    for( PCB_TRACK* track : m_board->Tracks() )
    {
        if( track->Type() == PCB_VIA_T )
        {
            PCB_VIA* via = static_cast<PCB_VIA*>( track );
            VECTOR2I viaPos = via->GetPosition();

            if( refArea->m_zone->Outline()->Contains( viaPos ) )
                refViaCountBefore++;
        }
    }

    BOOST_TEST_MESSAGE( wxString::Format( "Reference area vias before repeat: %d", refViaCountBefore ) );
    BOOST_CHECK( refViaCountBefore > 0 );

    mtTool->CheckRACompatibility( refArea->m_zone );

    ruleData->m_compatMap[targetArea].m_doCopy = true;
    ruleData->m_options.m_copyPlacement = true;
    ruleData->m_options.m_copyRouting = true;

    int result = mtTool->RepeatLayout( TOOL_EVENT(), refArea->m_zone );

    BOOST_CHECK( result >= 0 );

    int refViaCountAfter = 0;

    for( PCB_TRACK* track : m_board->Tracks() )
    {
        if( track->Type() == PCB_VIA_T )
        {
            PCB_VIA* via = static_cast<PCB_VIA*>( track );
            VECTOR2I viaPos = via->GetPosition();

            if( refArea->m_zone->Outline()->Contains( viaPos ) )
                refViaCountAfter++;
        }
    }

    BOOST_TEST_MESSAGE( wxString::Format( "Reference area vias after repeat: %d", refViaCountAfter ) );

    BOOST_CHECK_EQUAL( refViaCountAfter, refViaCountBefore );
}


/**
 * Test that "copy other items" only copies zones whose full layer sets are inside both
 * source and target rule area layer sets (issue 22983).
 */
BOOST_FIXTURE_TEST_CASE( RepeatLayoutRespectsZoneLayerSetsForOtherItems, MULTICHANNEL_TEST_FIXTURE )
{
    KI_TEST::LoadBoard( m_settingsManager, "issue22983/issue22983", m_board );

    TOOL_MANAGER       toolMgr;
    MOCK_TOOLS_HOLDER* toolsHolder = new MOCK_TOOLS_HOLDER;

    toolMgr.SetEnvironment( m_board.get(), nullptr, nullptr, nullptr, toolsHolder );

    MULTICHANNEL_TOOL* mtTool = new MULTICHANNEL_TOOL;
    toolMgr.RegisterTool( mtTool );

    mtTool->FindExistingRuleAreas();

    RULE_AREA* sourceA = findRuleAreaByPlacementGroup( mtTool, wxT( "SourceA" ) );
    RULE_AREA* destA = findRuleAreaByPlacementGroup( mtTool, wxT( "DestA" ) );
    RULE_AREA* sourceB = findRuleAreaByPlacementGroup( mtTool, wxT( "SourceB" ) );
    RULE_AREA* destB = findRuleAreaByPlacementGroup( mtTool, wxT( "DestB" ) );

    BOOST_REQUIRE( sourceA != nullptr );
    BOOST_REQUIRE( destA != nullptr );
    BOOST_REQUIRE( sourceB != nullptr );
    BOOST_REQUIRE( destB != nullptr );

    BOOST_CHECK_EQUAL( countZonesByNameInRuleArea( m_board.get(), wxT( "MultilayerZoneFrontAndOne" ), *sourceA ), 1 );
    BOOST_CHECK_EQUAL( countZonesByNameInRuleArea( m_board.get(), wxT( "MultilayerZoneFrontAndOne" ), *destA ), 0 );
    BOOST_CHECK_EQUAL(
            countZonesByNameInRuleArea( m_board.get(), wxT( "MultilayerZoneSourceBLayerMismatch" ), *sourceB ), 1 );
    BOOST_CHECK_EQUAL( countZonesByNameInRuleArea( m_board.get(), wxT( "MultilayerZoneSourceBLayerMismatch" ), *destB ),
                       0 );
    BOOST_CHECK_EQUAL( countZonesByNameInRuleArea( m_board.get(), wxT( "BottomZoneDontCopyMe" ), *sourceB ), 1 );
    BOOST_CHECK_EQUAL( countZonesByNameInRuleArea( m_board.get(), wxT( "BottomZoneDontCopyMe" ), *destB ), 0 );

    REPEAT_LAYOUT_OPTIONS options;
    options.m_copyPlacement = false;
    options.m_copyRouting = false;
    options.m_copyOtherItems = true;
    options.m_includeLockedItems = true;

    int copyAStatus = mtTool->RepeatLayout( TOOL_EVENT(), *sourceA, *destA, options );
    BOOST_REQUIRE( copyAStatus >= 0 );

    int copyBStatus = mtTool->RepeatLayout( TOOL_EVENT(), *sourceB, *destB, options );
    BOOST_REQUIRE( copyBStatus >= 0 );

    // SourceA and DestA both include F.Cu+B.Cu, so this multilayer zone should copy.
    BOOST_CHECK_EQUAL( countZonesByNameInRuleArea( m_board.get(), wxT( "MultilayerZoneFrontAndOne" ), *destA ), 1 );

    // SourceB only includes F.Cu, so this F.Cu+B.Cu zone should not copy to DestB.
    BOOST_CHECK_EQUAL( countZonesByNameInRuleArea( m_board.get(), wxT( "MultilayerZoneSourceBLayerMismatch" ), *destB ),
                       0 );

    // SourceB excludes B.Cu, so this B.Cu-only zone should not copy either.
    BOOST_CHECK_EQUAL( countZonesByNameInRuleArea( m_board.get(), wxT( "BottomZoneDontCopyMe" ), *destB ), 0 );
}


/**
 * Test that topology matching works correctly with dotted reference designators
 * used in multi-channel designs (issue 20058).
 *
 * Reference designators like TRIM_1.1 and TRIM_2.1 should be considered the same
 * kind because they share the base prefix "TRIM_".
 */
BOOST_FIXTURE_TEST_CASE( TopologyMatchDottedRefDes, MULTICHANNEL_TEST_FIXTURE )
{
    using TMATCH::CONNECTION_GRAPH;
    using TMATCH::COMPONENT;

    // Create two connection graphs with components that have dotted reference designators
    auto cgRef = std::make_unique<CONNECTION_GRAPH>();
    auto cgTarget = std::make_unique<CONNECTION_GRAPH>();

    // Create mock footprints with the same FPID
    LIB_ID fpid( wxT( "Package_SO" ), wxT( "SOIC-8_3.9x4.9mm_P1.27mm" ) );

    // Create reference footprint TRIM_1.1 and target footprint TRIM_2.1
    FOOTPRINT fpRef( nullptr );
    fpRef.SetFPID( fpid );
    fpRef.SetReference( wxT( "TRIM_1.1" ) );

    FOOTPRINT fpTarget( nullptr );
    fpTarget.SetFPID( fpid );
    fpTarget.SetReference( wxT( "TRIM_2.1" ) );

    // Create matching pad structures
    PAD padRef1( &fpRef );
    padRef1.SetNumber( wxT( "1" ) );
    padRef1.SetNetCode( 1 );
    fpRef.Add( &padRef1 );

    PAD padRef2( &fpRef );
    padRef2.SetNumber( wxT( "2" ) );
    padRef2.SetNetCode( 2 );
    fpRef.Add( &padRef2 );

    PAD padTarget1( &fpTarget );
    padTarget1.SetNumber( wxT( "1" ) );
    padTarget1.SetNetCode( 3 );
    fpTarget.Add( &padTarget1 );

    PAD padTarget2( &fpTarget );
    padTarget2.SetNumber( wxT( "2" ) );
    padTarget2.SetNetCode( 4 );
    fpTarget.Add( &padTarget2 );

    // Build connection graphs
    cgRef->AddFootprint( &fpRef, VECTOR2I( 0, 0 ) );
    cgTarget->AddFootprint( &fpTarget, VECTOR2I( 0, 0 ) );

    cgRef->BuildConnectivity();
    cgTarget->BuildConnectivity();

    // Check that the components are considered the same kind
    BOOST_CHECK_EQUAL( cgRef->Components().size(), 1 );
    BOOST_CHECK_EQUAL( cgTarget->Components().size(), 1 );

    COMPONENT* cmpRef = cgRef->Components()[0];
    COMPONENT* cmpTarget = cgTarget->Components()[0];

    bool sameKind = cmpRef->IsSameKind( *cmpTarget );

    BOOST_TEST_MESSAGE( wxString::Format( "TRIM_1.1 and TRIM_2.1 IsSameKind: %d", sameKind ? 1 : 0 ) );
    BOOST_CHECK( sameKind );

    // Test topology matching
    TMATCH::COMPONENT_MATCHES result;
    std::vector<TMATCH::TOPOLOGY_MISMATCH_REASON> details;
    bool status = cgRef->FindIsomorphism( cgTarget.get(), result, details );

    BOOST_TEST_MESSAGE( wxString::Format( "Topology match result: %d", status ? 1 : 0 ) );

    if( !status && !details.empty() )
    {
        for( const auto& reason : details )
        {
            BOOST_TEST_MESSAGE( wxString::Format( "Mismatch: %s <-> %s: %s",
                                                  reason.m_reference, reason.m_candidate, reason.m_reason ) );
        }
    }

    BOOST_CHECK( status );
    BOOST_CHECK( details.empty() );

    // Cleanup: remove pads before footprints go out of scope
    fpRef.Pads().clear();
    fpTarget.Pads().clear();
}


/**
 * Test that GeneratePotentialRuleAreas includes components from child sheets
 * when generating rule areas for a parent sheet (issue 20016).
 *
 * The vme-wren board has hierarchical sheets like /io_drivers_fp/bank0/ which
 * contains child sheets /io_drivers_fp/bank0/io01/ through io78/. The parent
 * sheet's rule area should include all components from child sheets.
 */
BOOST_FIXTURE_TEST_CASE( GenerateRuleAreasIncludesChildSheets, MULTICHANNEL_TEST_FIXTURE )
{
    KI_TEST::LoadBoard( m_settingsManager, "vme-wren", m_board );

    TOOL_MANAGER       toolMgr;
    MOCK_TOOLS_HOLDER* toolsHolder = new MOCK_TOOLS_HOLDER;

    toolMgr.SetEnvironment( m_board.get(), nullptr, nullptr, nullptr, toolsHolder );

    MULTICHANNEL_TOOL* mtTool = new MULTICHANNEL_TOOL;
    toolMgr.RegisterTool( mtTool );

    mtTool->GeneratePotentialRuleAreas();

    auto ruleData = mtTool->GetData();

    RULE_AREA* leafArea = nullptr;
    RULE_AREA* midArea = nullptr;
    RULE_AREA* topArea = nullptr;

    for( RULE_AREA& ra : ruleData->m_areas )
    {
        if( ra.m_sheetPath == wxT( "/io_drivers_fp/bank0/io01/" ) )
            leafArea = &ra;
        else if( ra.m_sheetPath == wxT( "/io_drivers_fp/bank0/" ) )
            midArea = &ra;
        else if( ra.m_sheetPath == wxT( "/io_drivers_fp/" ) )
            topArea = &ra;
    }

    BOOST_REQUIRE( leafArea != nullptr );
    BOOST_REQUIRE( midArea != nullptr );
    BOOST_REQUIRE( topArea != nullptr );

    BOOST_TEST_MESSAGE( wxString::Format( "Leaf /io_drivers_fp/bank0/io01/ components: %d",
                                          static_cast<int>( leafArea->m_components.size() ) ) );
    BOOST_TEST_MESSAGE( wxString::Format( "Mid /io_drivers_fp/bank0/ components: %d",
                                          static_cast<int>( midArea->m_components.size() ) ) );
    BOOST_TEST_MESSAGE( wxString::Format( "Top /io_drivers_fp/ components: %d",
                                          static_cast<int>( topArea->m_components.size() ) ) );

    // Leaf sheet has 31 direct components and no children
    BOOST_CHECK_EQUAL( leafArea->m_components.size(), 31 );

    // Mid-level sheet has 7 direct + 4 child sheets * 31 each = 131
    BOOST_CHECK_EQUAL( midArea->m_components.size(), 131 );

    // Top-level sheet has 3 direct + 4 banks * 131 each = 527
    BOOST_CHECK_EQUAL( topArea->m_components.size(), 527 );

    // Mid-level components must be a superset of leaf components
    for( FOOTPRINT* fp : leafArea->m_components )
        BOOST_CHECK( midArea->m_components.count( fp ) > 0 );

    // Top-level components must be a superset of mid-level components
    for( FOOTPRINT* fp : midArea->m_components )
        BOOST_CHECK( topArea->m_components.count( fp ) > 0 );
}


/**
 * Test that FindIsomorphism respects cancellation and reports progress
 * through the ISOMORPHISM_PARAMS interface.
 */
BOOST_FIXTURE_TEST_CASE( FindIsomorphismCancellation, MULTICHANNEL_TEST_FIXTURE )
{
    using TMATCH::CONNECTION_GRAPH;

    KI_TEST::LoadBoard( m_settingsManager, "vme-wren", m_board );

    TOOL_MANAGER       toolMgr;
    MOCK_TOOLS_HOLDER* toolsHolder = new MOCK_TOOLS_HOLDER;

    toolMgr.SetEnvironment( m_board.get(), nullptr, nullptr, nullptr, toolsHolder );

    MULTICHANNEL_TOOL* mtTool = new MULTICHANNEL_TOOL;
    toolMgr.RegisterTool( mtTool );

    mtTool->GeneratePotentialRuleAreas();

    auto ruleData = mtTool->GetData();

    ruleData->m_replaceExisting = true;

    for( RULE_AREA& ra : ruleData->m_areas )
    {
        if( ra.m_sheetName == wxT( "io_driver.kicad_sch" ) )
            ra.m_generateEnabled = true;
    }

    TOOL_EVENT dummyEvent;
    mtTool->AutogenerateRuleAreas( dummyEvent );
    mtTool->FindExistingRuleAreas();

    RULE_AREA* refArea = findRuleAreaByPartialName( mtTool, wxT( "io_drivers_fp/bank3/io78/" ) );
    RULE_AREA* targetArea = findRuleAreaByPartialName( mtTool, wxT( "io_drivers_fp/bank2/io78/" ) );

    BOOST_REQUIRE( refArea != nullptr );
    BOOST_REQUIRE( targetArea != nullptr );

    auto cgRef = CONNECTION_GRAPH::BuildFromFootprintSet( refArea->m_components,
                                                          targetArea->m_components );
    auto cgTarget = CONNECTION_GRAPH::BuildFromFootprintSet( targetArea->m_components,
                                                              refArea->m_components );

    // Pre-cancelled: should return false immediately with empty result
    {
        std::atomic<bool> cancelled( true );
        std::atomic<int>  matched( 0 );
        std::atomic<int>  total( 0 );

        TMATCH::ISOMORPHISM_PARAMS params;
        params.m_cancelled = &cancelled;
        params.m_matchedComponents = &matched;
        params.m_totalComponents = &total;

        TMATCH::COMPONENT_MATCHES result;
        std::vector<TMATCH::TOPOLOGY_MISMATCH_REASON> details;

        bool status = cgRef->FindIsomorphism( cgTarget.get(), result, details, params );

        BOOST_CHECK( !status );
        BOOST_CHECK( result.empty() );

        BOOST_TEST_MESSAGE( "Pre-cancelled FindIsomorphism correctly returned false" );
    }

    // Normal run with progress reporting
    {
        std::atomic<bool> cancelled( false );
        std::atomic<int>  matched( 0 );
        std::atomic<int>  total( 0 );

        TMATCH::ISOMORPHISM_PARAMS params;
        params.m_cancelled = &cancelled;
        params.m_matchedComponents = &matched;
        params.m_totalComponents = &total;

        TMATCH::COMPONENT_MATCHES result;
        std::vector<TMATCH::TOPOLOGY_MISMATCH_REASON> details;

        bool status = cgRef->FindIsomorphism( cgTarget.get(), result, details, params );

        BOOST_CHECK( status );

        int finalMatched = matched.load();
        int finalTotal = total.load();

        BOOST_TEST_MESSAGE( wxString::Format( "Progress: matched=%d, total=%d", finalMatched, finalTotal ) );

        BOOST_CHECK( finalTotal > 0 );
        BOOST_CHECK_EQUAL( finalMatched, finalTotal );
    }

    // Sanity check: same graphs without params still succeed
    {
        TMATCH::COMPONENT_MATCHES result;
        std::vector<TMATCH::TOPOLOGY_MISMATCH_REASON> details;

        bool status = cgRef->FindIsomorphism( cgTarget.get(), result, details );

        BOOST_CHECK( status );
        BOOST_CHECK( !result.empty() );

        BOOST_TEST_MESSAGE( "Default params FindIsomorphism still succeeds" );
    }
}


/**
 * Test that topology matching succeeds when hierarchical pins are connected directly
 * to global nets (issue 21739).
 *
 * When address-select pins (AD0, AD2) are tied directly to GND or +3V3 in the parent
 * schematic, those pads end up on a global net that also includes power-supply and
 * pull-up pads from other components in the channel.  Without the fix this creates
 * different intra-channel connection counts between channels, producing a false
 * topology-mismatch error.
 */
BOOST_FIXTURE_TEST_CASE( TopoMatchGlobalNetHierarchicalPins, MULTICHANNEL_TEST_FIXTURE )
{
    using TMATCH::CONNECTION_GRAPH;

    KI_TEST::LoadBoard( m_settingsManager, "issue21739/topology_mismatch", m_board );

    TOOL_MANAGER       toolMgr;
    MOCK_TOOLS_HOLDER* toolsHolder = new MOCK_TOOLS_HOLDER;

    toolMgr.SetEnvironment( m_board.get(), nullptr, nullptr, nullptr, toolsHolder );

    MULTICHANNEL_TOOL* mtTool = new MULTICHANNEL_TOOL;
    toolMgr.RegisterTool( mtTool );

    mtTool->FindExistingRuleAreas();

    auto ruleData = mtTool->GetData();

    BOOST_TEST_MESSAGE( wxString::Format( "Found %d rule areas",
                                          static_cast<int>( ruleData->m_areas.size() ) ) );

    BOOST_REQUIRE( ruleData->m_areas.size() >= 2 );

    RULE_AREA* ch0Area = nullptr;
    RULE_AREA* ch1Area = nullptr;

    for( RULE_AREA& ra : ruleData->m_areas )
    {
        if( !ra.m_zone )
            continue;

        wxString source = ra.m_zone->GetPlacementAreaSource();

        if( source == wxT( "/i2c_thingy_ch0/" ) )
            ch0Area = &ra;
        else if( source == wxT( "/i2c_thingy_ch1/" ) )
            ch1Area = &ra;
    }

    BOOST_REQUIRE_MESSAGE( ch0Area != nullptr, "Could not find i2c_thingy_ch0 rule area" );
    BOOST_REQUIRE_MESSAGE( ch1Area != nullptr, "Could not find i2c_thingy_ch1 rule area" );

    BOOST_TEST_MESSAGE( wxString::Format( "ch0 components: %d, ch1 components: %d",
                                          static_cast<int>( ch0Area->m_components.size() ),
                                          static_cast<int>( ch1Area->m_components.size() ) ) );

    BOOST_CHECK_EQUAL( ch0Area->m_components.size(), ch1Area->m_components.size() );

    auto cgRef = CONNECTION_GRAPH::BuildFromFootprintSet( ch0Area->m_components,
                                                          ch1Area->m_components );
    auto cgTarget = CONNECTION_GRAPH::BuildFromFootprintSet( ch1Area->m_components,
                                                              ch0Area->m_components );

    TMATCH::COMPONENT_MATCHES result;
    std::vector<TMATCH::TOPOLOGY_MISMATCH_REASON> details;
    bool status = cgRef->FindIsomorphism( cgTarget.get(), result, details );

    if( !status )
    {
        for( const auto& reason : details )
        {
            BOOST_TEST_MESSAGE( wxString::Format( "Mismatch: %s <-> %s: %s",
                                                  reason.m_reference, reason.m_candidate,
                                                  reason.m_reason ) );
        }
    }

    BOOST_CHECK_MESSAGE( status,
                         "Topology match failed for channels with hierarchical pins "
                         "tied to global nets (issue 21739)" );
}


/**
 * A design block's daisy-chain output net feeds the target instance's inputs, so it appears in
 * both footprint sets with one pad on the reference side and several on the target side.  It must
 * not be excluded as a global rail; doing so asymmetrically broke the isomorphism and reported
 * "No compatible component found in the target area."  Regression test for that case.
 */
BOOST_FIXTURE_TEST_CASE( TopoMatchBoundarySignalNetNotExcluded, MULTICHANNEL_TEST_FIXTURE )
{
    using TMATCH::CONNECTION_GRAPH;

    std::unique_ptr<BOARD> board = std::make_unique<BOARD>();

    // The board forces consecutive net codes, so register by name and use the assigned code.
    auto addNet = [&]( const wxString& aName ) -> int
    {
        NETINFO_ITEM* net = new NETINFO_ITEM( board.get(), aName );
        board->Add( net );
        return net->GetNetCode();
    };

    const int netGnd = addNet( wxT( "GND" ) );
    const int netVcc = addNet( wxT( "+5VP" ) );
    const int netChainInRef = addNet( wxT( "Net-(D-RefChainIn)" ) );
    const int netBridge = addNet( wxT( "Net-(D38-DOUT)" ) );  // reference output AND target input
    const int netChainOutTgt = addNet( wxT( "Net-(D43-DOUT)" ) );
    const int netD2Dout = addNet( wxT( "unconnected-(D2-DOUT)" ) );
    const int netD3Dout = addNet( wxT( "unconnected-(D3-DOUT)" ) );
    const int netD5Dout = addNet( wxT( "unconnected-(D5-DOUT)" ) );
    const int netD6Dout = addNet( wxT( "unconnected-(D6-DOUT)" ) );

    LIB_ID ledId( wxT( "TestLib" ), wxT( "WS2812" ) );

    // Four-pad addressable LED: pad 1 = DOUT, pad 2 = GND, pad 3 = DIN, pad 4 = VCC.
    auto makeLed = [&]( const wxString& aRef, int aDout, int aDin ) -> FOOTPRINT*
    {
        FOOTPRINT* fp = new FOOTPRINT( board.get() );
        fp->SetFPID( ledId );
        fp->SetReference( aRef );
        board->Add( fp );

        auto addPad = [&]( const wxString& aNumber, int aNetCode )
        {
            PAD* pad = new PAD( fp );
            pad->SetNumber( aNumber );
            pad->SetShape( PADSTACK::ALL_LAYERS, PAD_SHAPE::CIRCLE );
            pad->SetSize( PADSTACK::ALL_LAYERS,
                          VECTOR2I( pcbIUScale.mmToIU( 1 ), pcbIUScale.mmToIU( 1 ) ) );
            pad->SetLayerSet( LSET( { F_Cu } ) );
            pad->SetNetCode( aNetCode );
            fp->Add( pad );
        };

        addPad( wxT( "1" ), aDout );
        addPad( wxT( "2" ), netGnd );
        addPad( wxT( "3" ), aDin );
        addPad( wxT( "4" ), netVcc );

        return fp;
    };

    // Reference area (design block source instance): three LEDs share the DIN rail; only the
    // representative LED drives the chain output (the bridge net), the other two are unconnected.
    std::set<FOOTPRINT*> refFps;
    refFps.insert( makeLed( wxT( "D1" ), netBridge, netChainInRef ) );
    refFps.insert( makeLed( wxT( "D2" ), netD2Dout, netChainInRef ) );
    refFps.insert( makeLed( wxT( "D3" ), netD3Dout, netChainInRef ) );

    // Target area (downstream instance): the three LEDs' DIN rail IS the bridge net coming from
    // the design block source, and the representative LED drives a fresh chain output.
    std::set<FOOTPRINT*> tgtFps;
    tgtFps.insert( makeLed( wxT( "D4" ), netChainOutTgt, netBridge ) );
    tgtFps.insert( makeLed( wxT( "D5" ), netD5Dout, netBridge ) );
    tgtFps.insert( makeLed( wxT( "D6" ), netD6Dout, netBridge ) );

    auto cgRef = CONNECTION_GRAPH::BuildFromFootprintSet( refFps, tgtFps );
    auto cgTarget = CONNECTION_GRAPH::BuildFromFootprintSet( tgtFps, refFps );

    TMATCH::COMPONENT_MATCHES result;
    std::vector<TMATCH::TOPOLOGY_MISMATCH_REASON> details;
    bool status = cgRef->FindIsomorphism( cgTarget.get(), result, details );

    if( !status )
    {
        for( const auto& reason : details )
        {
            BOOST_TEST_MESSAGE( wxString::Format( "Mismatch: %s <-> %s: %s",
                                                  reason.m_reference, reason.m_candidate,
                                                  reason.m_reason ) );
        }
    }

    BOOST_CHECK_MESSAGE( status,
                         "Topology match failed because the design-block boundary signal net "
                         "was misclassified as a global rail and excluded asymmetrically" );
    BOOST_CHECK_EQUAL( result.size(), refFps.size() );
}


/**
 * Apply Design Block Layout must copy silkscreen graphics that are not associated with any
 * footprint from the design block source to the destination group (issue 24372).
 *
 * The bug was that copyRuleAreaContents filtered "other items" by the rule area zone's
 * LayerSet, but the synthetic rule area zones created for the design block / group placement
 * flow always use LSET::AllCuMask(). That filter rejected silkscreen, fab and user-layer
 * graphics even though they were explicitly enumerated in m_designBlockItems.
 */
BOOST_FIXTURE_TEST_CASE( ApplyDesignBlockLayoutCopiesSilkscreen, MULTICHANNEL_TEST_FIXTURE )
{
    m_board = std::make_unique<BOARD>();
    m_board->SetEnabledLayers( LSET::AllCuMask() | LSET::AllTechMask() );

    // Net for the single connection between the two footprints in each block.
    NETINFO_ITEM* net = new NETINFO_ITEM( m_board.get(), wxT( "NET1" ), 1 );
    m_board->Add( net );

    auto makeFootprint =
            [&]( const wxString& aRef, const VECTOR2I& aPos ) -> FOOTPRINT*
            {
                FOOTPRINT* fp = new FOOTPRINT( m_board.get() );
                fp->SetFPID( LIB_ID( wxT( "TestLib" ), wxT( "R" ) ) );
                fp->SetReference( aRef );
                fp->SetPosition( aPos );

                PAD* pad = new PAD( fp );
                pad->SetNumber( wxT( "1" ) );
                pad->SetNet( net );
                pad->SetPosition( aPos );
                pad->SetSize( F_Cu,
                              VECTOR2I( pcbIUScale.mmToIU( 1 ), pcbIUScale.mmToIU( 1 ) ) );
                pad->SetLayerSet( LSET( { F_Cu } ) );
                fp->Add( pad );

                m_board->Add( fp );
                return fp;
            };

    // Source: a "design block" with two matched footprints and a silkscreen rectangle that
    // is not associated with any footprint.
    FOOTPRINT* refFp1 = makeFootprint( wxT( "R1" ),
                                       VECTOR2I( pcbIUScale.mmToIU( 0 ), pcbIUScale.mmToIU( 0 ) ) );
    FOOTPRINT* refFp2 = makeFootprint( wxT( "R2" ),
                                       VECTOR2I( pcbIUScale.mmToIU( 10 ), pcbIUScale.mmToIU( 0 ) ) );

    PCB_SHAPE* silkRect = new PCB_SHAPE( m_board.get(), SHAPE_T::RECTANGLE );
    silkRect->SetStart( VECTOR2I( pcbIUScale.mmToIU( -2 ), pcbIUScale.mmToIU( -2 ) ) );
    silkRect->SetEnd( VECTOR2I( pcbIUScale.mmToIU( 12 ), pcbIUScale.mmToIU( 2 ) ) );
    silkRect->SetLayer( F_SilkS );
    silkRect->SetStroke( STROKE_PARAMS( pcbIUScale.mmToIU( 0.15 ), LINE_STYLE::SOLID ) );
    m_board->Add( silkRect );

    // Destination: a group containing the matched pair so RepeatLayout can find a parent group.
    FOOTPRINT* destFp1 = makeFootprint( wxT( "R3" ),
                                        VECTOR2I( pcbIUScale.mmToIU( 50 ), pcbIUScale.mmToIU( 50 ) ) );
    FOOTPRINT* destFp2 = makeFootprint( wxT( "R4" ),
                                        VECTOR2I( pcbIUScale.mmToIU( 60 ), pcbIUScale.mmToIU( 50 ) ) );

    PCB_GROUP* destGroup = new PCB_GROUP( m_board.get() );
    destGroup->SetName( wxT( "design-block-dest" ) );
    destGroup->AddItem( destFp1 );
    destGroup->AddItem( destFp2 );
    m_board->Add( destGroup );

    // Unrelated silkscreen drawing inside the destination bounding box but not part of the
    // destination group. Apply Design Block Layout must not delete or claim this item.
    PCB_SHAPE* unrelatedSilk = new PCB_SHAPE( m_board.get(), SHAPE_T::SEGMENT );
    unrelatedSilk->SetStart( VECTOR2I( pcbIUScale.mmToIU( 52 ), pcbIUScale.mmToIU( 52 ) ) );
    unrelatedSilk->SetEnd( VECTOR2I( pcbIUScale.mmToIU( 58 ), pcbIUScale.mmToIU( 52 ) ) );
    unrelatedSilk->SetLayer( F_SilkS );
    unrelatedSilk->SetStroke( STROKE_PARAMS( pcbIUScale.mmToIU( 0.15 ), LINE_STYLE::SOLID ) );
    m_board->Add( unrelatedSilk );

    int silkBefore = 0;

    for( BOARD_ITEM* item : m_board->Drawings() )
    {
        if( item->Type() == PCB_SHAPE_T && item->GetLayer() == F_SilkS )
            silkBefore++;
    }

    BOOST_REQUIRE_EQUAL( silkBefore, 2 );

    RULE_AREA dbRA;
    dbRA.m_sourceType = PLACEMENT_SOURCE_T::DESIGN_BLOCK;
    dbRA.m_components.insert( refFp1 );
    dbRA.m_components.insert( refFp2 );
    dbRA.m_designBlockItems.insert( refFp1 );
    dbRA.m_designBlockItems.insert( refFp2 );
    dbRA.m_designBlockItems.insert( silkRect );

    // The Apply Design Block flow uses a synthetic copper-only rule area zone. The destination
    // zone is a temporary zone never added to the board.
    dbRA.m_zone = new ZONE( m_board.get() );
    dbRA.m_zone->SetIsRuleArea( true );
    dbRA.m_zone->SetLayerSet( LSET::AllCuMask() );
    dbRA.m_zone->AddPolygon( KIGEOM::BoxToLineChain(
            BOX2I::ByCorners( VECTOR2I( pcbIUScale.mmToIU( -5 ), pcbIUScale.mmToIU( -5 ) ),
                              VECTOR2I( pcbIUScale.mmToIU( 15 ), pcbIUScale.mmToIU( 5 ) ) ) ) );

    RULE_AREA destRA;
    destRA.m_sourceType = PLACEMENT_SOURCE_T::GROUP_PLACEMENT;
    destRA.m_components.insert( destFp1 );
    destRA.m_components.insert( destFp2 );

    destRA.m_zone = new ZONE( m_board.get() );
    destRA.m_zone->SetIsRuleArea( true );
    destRA.m_zone->SetLayerSet( LSET::AllCuMask() );
    destRA.m_zone->AddPolygon( KIGEOM::BoxToLineChain(
            BOX2I::ByCorners( VECTOR2I( pcbIUScale.mmToIU( 45 ), pcbIUScale.mmToIU( 45 ) ),
                              VECTOR2I( pcbIUScale.mmToIU( 65 ), pcbIUScale.mmToIU( 55 ) ) ) ) );

    TOOL_MANAGER       toolMgr;
    MOCK_TOOLS_HOLDER* toolsHolder = new MOCK_TOOLS_HOLDER;
    toolMgr.SetEnvironment( m_board.get(), nullptr, nullptr, nullptr, toolsHolder );

    MULTICHANNEL_TOOL* mtTool = new MULTICHANNEL_TOOL;
    toolMgr.RegisterTool( mtTool );

    REPEAT_LAYOUT_OPTIONS opts = { .m_copyRouting = true,
                                   .m_connectedRoutingOnly = false,
                                   .m_copyPlacement = true,
                                   .m_copyOtherItems = true,
                                   .m_groupItems = false,
                                   .m_includeLockedItems = true,
                                   .m_anchorFp = nullptr };

    int result = mtTool->RepeatLayout( TOOL_EVENT(), dbRA, destRA, opts );
    BOOST_CHECK_MESSAGE( result >= 0, "RepeatLayout failed" );

    delete dbRA.m_zone;
    delete destRA.m_zone;

    // Verify a third silkscreen shape (the duplicated rectangle) was added and grouped under
    // the destination group, that the unrelated silk segment was preserved, and that the
    // duplicated rectangle landed inside the destination region.
    int        silkAfter = 0;
    int        silkInGroup = 0;
    bool       unrelatedSurvived = false;
    PCB_SHAPE* copiedRect = nullptr;

    for( BOARD_ITEM* item : m_board->Drawings() )
    {
        if( item->Type() != PCB_SHAPE_T || item->GetLayer() != F_SilkS )
            continue;

        silkAfter++;

        if( item == unrelatedSilk )
            unrelatedSurvived = true;

        if( item->GetParentGroup() == destGroup )
        {
            silkInGroup++;

            PCB_SHAPE* shape = static_cast<PCB_SHAPE*>( item );

            if( shape->GetShape() == SHAPE_T::RECTANGLE )
                copiedRect = shape;
        }
    }

    BOOST_CHECK_MESSAGE( silkAfter == 3,
                         wxString::Format( "Expected 3 silkscreen shapes after Apply Design Block "
                                           "Layout (2 original + 1 copy), found %d (issue 24372)",
                                           silkAfter ) );
    BOOST_CHECK_MESSAGE( silkInGroup == 1,
                         wxString::Format( "Expected 1 silkscreen shape in destination group, "
                                           "found %d (issue 24372)",
                                           silkInGroup ) );
    BOOST_CHECK_MESSAGE( unrelatedSurvived,
                         "Unrelated silkscreen drawing outside the design block group was "
                         "deleted by Apply Design Block Layout (issue 24372)" );
    BOOST_REQUIRE( copiedRect != nullptr );

    // The copied rectangle should sit near the destination footprints (offset by ~50mm from
    // the source position), not at the original source location.
    BOOST_CHECK_GT( copiedRect->GetStart().x, pcbIUScale.mmToIU( 30 ) );
}


/**
 * Apply Design Block Layout must keep a footprint's silkscreen text when the footprint has
 * several text items with the same string (issue 24583).
 *
 * A flippable switch footprint has two "${REFERENCE}" texts, one on F.SilkS and one on B.SilkS.
 * The old code matched footprint texts by string only and stopped at the first match, so both
 * source texts went to the same target item and the front silkscreen text was lost.
 */
BOOST_FIXTURE_TEST_CASE( ApplyDesignBlockLayoutKeepsDuplicateSilkText, MULTICHANNEL_TEST_FIXTURE )
{
    m_board = std::make_unique<BOARD>();
    m_board->SetEnabledLayers( LSET::AllCuMask() | LSET::AllTechMask() );

    NETINFO_ITEM* net = new NETINFO_ITEM( m_board.get(), wxT( "NET1" ), 1 );
    m_board->Add( net );

    auto makeFootprint = [&]( const wxString& aRef, const VECTOR2I& aPos ) -> FOOTPRINT*
    {
        FOOTPRINT* fp = new FOOTPRINT( m_board.get() );
        fp->SetFPID( LIB_ID( wxT( "TestLib" ), wxT( "SW" ) ) );
        fp->SetReference( aRef );
        fp->SetPosition( aPos );

        // Two silkscreen texts with the same content, one on each board side.
        for( PCB_LAYER_ID layer : { F_SilkS, B_SilkS } )
        {
            PCB_TEXT* txt = new PCB_TEXT( fp );
            txt->SetText( wxT( "${REFERENCE}" ) );
            txt->SetLayer( layer );
            txt->SetPosition( aPos + VECTOR2I( 0, pcbIUScale.mmToIU( -8 ) ) );
            fp->Add( txt );
        }

        PAD* pad = new PAD( fp );
        pad->SetNumber( wxT( "1" ) );
        pad->SetNet( net );
        pad->SetPosition( aPos );
        pad->SetSize( F_Cu, VECTOR2I( pcbIUScale.mmToIU( 1 ), pcbIUScale.mmToIU( 1 ) ) );
        pad->SetLayerSet( LSET( { F_Cu } ) );
        fp->Add( pad );

        m_board->Add( fp );
        return fp;
    };

    auto countSilkText = [&]( FOOTPRINT* fp, PCB_LAYER_ID layer ) -> int
    {
        int count = 0;

        for( BOARD_ITEM* item : fp->GraphicalItems() )
        {
            if( item->Type() == PCB_TEXT_T && item->GetLayer() == layer )
                count++;
        }

        return count;
    };

    FOOTPRINT* refFp1 = makeFootprint( wxT( "SW1" ), VECTOR2I( 0, 0 ) );
    FOOTPRINT* refFp2 = makeFootprint( wxT( "SW2" ), VECTOR2I( pcbIUScale.mmToIU( 10 ), 0 ) );

    FOOTPRINT* destFp1 = makeFootprint( wxT( "SW3" ), VECTOR2I( pcbIUScale.mmToIU( 50 ), pcbIUScale.mmToIU( 50 ) ) );
    FOOTPRINT* destFp2 = makeFootprint( wxT( "SW4" ), VECTOR2I( pcbIUScale.mmToIU( 60 ), pcbIUScale.mmToIU( 50 ) ) );

    PCB_GROUP* destGroup = new PCB_GROUP( m_board.get() );
    destGroup->SetName( wxT( "design-block-dest" ) );
    destGroup->AddItem( destFp1 );
    destGroup->AddItem( destFp2 );
    m_board->Add( destGroup );

    BOOST_REQUIRE_EQUAL( countSilkText( destFp1, F_SilkS ), 1 );
    BOOST_REQUIRE_EQUAL( countSilkText( destFp1, B_SilkS ), 1 );

    RULE_AREA dbRA;
    dbRA.m_sourceType = PLACEMENT_SOURCE_T::DESIGN_BLOCK;
    dbRA.m_components.insert( refFp1 );
    dbRA.m_components.insert( refFp2 );
    dbRA.m_designBlockItems.insert( refFp1 );
    dbRA.m_designBlockItems.insert( refFp2 );

    dbRA.m_zone = new ZONE( m_board.get() );
    dbRA.m_zone->SetIsRuleArea( true );
    dbRA.m_zone->SetLayerSet( LSET::AllCuMask() );
    dbRA.m_zone->AddPolygon(
            KIGEOM::BoxToLineChain( BOX2I::ByCorners( VECTOR2I( pcbIUScale.mmToIU( -5 ), pcbIUScale.mmToIU( -5 ) ),
                                                      VECTOR2I( pcbIUScale.mmToIU( 15 ), pcbIUScale.mmToIU( 5 ) ) ) ) );

    RULE_AREA destRA;
    destRA.m_sourceType = PLACEMENT_SOURCE_T::GROUP_PLACEMENT;
    destRA.m_components.insert( destFp1 );
    destRA.m_components.insert( destFp2 );

    destRA.m_zone = new ZONE( m_board.get() );
    destRA.m_zone->SetIsRuleArea( true );
    destRA.m_zone->SetLayerSet( LSET::AllCuMask() );
    destRA.m_zone->AddPolygon( KIGEOM::BoxToLineChain(
            BOX2I::ByCorners( VECTOR2I( pcbIUScale.mmToIU( 45 ), pcbIUScale.mmToIU( 45 ) ),
                              VECTOR2I( pcbIUScale.mmToIU( 65 ), pcbIUScale.mmToIU( 55 ) ) ) ) );

    TOOL_MANAGER       toolMgr;
    MOCK_TOOLS_HOLDER* toolsHolder = new MOCK_TOOLS_HOLDER;
    toolMgr.SetEnvironment( m_board.get(), nullptr, nullptr, nullptr, toolsHolder );

    MULTICHANNEL_TOOL* mtTool = new MULTICHANNEL_TOOL;
    toolMgr.RegisterTool( mtTool );

    REPEAT_LAYOUT_OPTIONS opts = { .m_copyRouting = true,
                                   .m_connectedRoutingOnly = false,
                                   .m_copyPlacement = true,
                                   .m_copyOtherItems = true,
                                   .m_groupItems = false,
                                   .m_includeLockedItems = true,
                                   .m_anchorFp = nullptr };

    wxString err;
    int      result = mtTool->RepeatLayout( TOOL_EVENT(), dbRA, destRA, opts, nullptr, &err );
    BOOST_CHECK_MESSAGE( result >= 0, wxString::Format( "RepeatLayout failed: %s", err ) );

    delete dbRA.m_zone;
    delete destRA.m_zone;

    for( FOOTPRINT* destFp : { destFp1, destFp2 } )
    {
        BOOST_CHECK_MESSAGE( countSilkText( destFp, F_SilkS ) == 1,
                             wxString::Format( "%s lost its F.Silkscreen reference text "
                                               "(issue 24583): F=%d B=%d",
                                               destFp->GetReference(), countSilkText( destFp, F_SilkS ),
                                               countSilkText( destFp, B_SilkS ) ) );
        BOOST_CHECK_EQUAL( countSilkText( destFp, B_SilkS ), 1 );
    }
}


/**
 * Apply Design Block Layout must work for a footprint-free block, e.g. graphics-only silkscreen
 * (issue 24592). With no footprints the copy is center-anchored and re-grouped under the target.
 */
BOOST_FIXTURE_TEST_CASE( ApplyDesignBlockLayoutGraphicsOnlyBlock, MULTICHANNEL_TEST_FIXTURE )
{
    m_board = std::make_unique<BOARD>();
    m_board->SetEnabledLayers( LSET::AllCuMask() | LSET::AllTechMask() );

    auto makeSilkRect = [&]( const VECTOR2I& aStart, const VECTOR2I& aEnd ) -> PCB_SHAPE*
    {
        PCB_SHAPE* s = new PCB_SHAPE( m_board.get(), SHAPE_T::RECTANGLE );
        s->SetStart( aStart );
        s->SetEnd( aEnd );
        s->SetLayer( F_SilkS );
        s->SetStroke( STROKE_PARAMS( pcbIUScale.mmToIU( 0.15 ), LINE_STYLE::SOLID ) );
        m_board->Add( s );
        return s;
    };

    // Source: footprint-free block with one silkscreen rectangle centered on origin.
    PCB_SHAPE* srcRect = makeSilkRect( VECTOR2I( pcbIUScale.mmToIU( -2 ), pcbIUScale.mmToIU( -2 ) ),
                                       VECTOR2I( pcbIUScale.mmToIU( 2 ), pcbIUScale.mmToIU( 2 ) ) );

    // Destination: footprint-free group with one silkscreen item so the group exists.
    PCB_SHAPE* destPlaceholder = new PCB_SHAPE( m_board.get(), SHAPE_T::SEGMENT );
    destPlaceholder->SetStart( VECTOR2I( pcbIUScale.mmToIU( 49 ), pcbIUScale.mmToIU( 49 ) ) );
    destPlaceholder->SetEnd( VECTOR2I( pcbIUScale.mmToIU( 51 ), pcbIUScale.mmToIU( 49 ) ) );
    destPlaceholder->SetLayer( F_SilkS );
    destPlaceholder->SetStroke( STROKE_PARAMS( pcbIUScale.mmToIU( 0.15 ), LINE_STYLE::SOLID ) );
    m_board->Add( destPlaceholder );

    PCB_GROUP* destGroup = new PCB_GROUP( m_board.get() );
    destGroup->SetName( wxT( "design-block-dest" ) );
    destGroup->AddItem( destPlaceholder );
    m_board->Add( destGroup );

    int silkBefore = 0;

    for( BOARD_ITEM* item : m_board->Drawings() )
    {
        if( item->Type() == PCB_SHAPE_T && item->GetLayer() == F_SilkS )
            silkBefore++;
    }

    BOOST_REQUIRE_EQUAL( silkBefore, 2 );

    RULE_AREA dbRA;
    dbRA.m_sourceType = PLACEMENT_SOURCE_T::DESIGN_BLOCK;
    dbRA.m_designBlockItems.insert( srcRect ); // no footprints -> m_components stays empty

    dbRA.m_zone = new ZONE( m_board.get() );
    dbRA.m_zone->SetIsRuleArea( true );
    dbRA.m_zone->SetLayerSet( LSET::AllCuMask() );
    dbRA.m_zone->AddPolygon(
            KIGEOM::BoxToLineChain( BOX2I::ByCorners( VECTOR2I( pcbIUScale.mmToIU( -5 ), pcbIUScale.mmToIU( -5 ) ),
                                                      VECTOR2I( pcbIUScale.mmToIU( 5 ), pcbIUScale.mmToIU( 5 ) ) ) ) );
    dbRA.m_center = dbRA.m_zone->Outline()->COutline( 0 ).Centre();

    RULE_AREA destRA;
    destRA.m_sourceType = PLACEMENT_SOURCE_T::GROUP_PLACEMENT;
    destRA.m_group = destGroup; // set explicitly: no footprint to recover it from

    destRA.m_zone = new ZONE( m_board.get() );
    destRA.m_zone->SetIsRuleArea( true );
    destRA.m_zone->SetLayerSet( LSET::AllCuMask() );
    destRA.m_zone->AddPolygon( KIGEOM::BoxToLineChain(
            BOX2I::ByCorners( VECTOR2I( pcbIUScale.mmToIU( 45 ), pcbIUScale.mmToIU( 45 ) ),
                              VECTOR2I( pcbIUScale.mmToIU( 55 ), pcbIUScale.mmToIU( 55 ) ) ) ) );
    destRA.m_center = destRA.m_zone->Outline()->COutline( 0 ).Centre();

    TOOL_MANAGER       toolMgr;
    MOCK_TOOLS_HOLDER* toolsHolder = new MOCK_TOOLS_HOLDER;
    toolMgr.SetEnvironment( m_board.get(), nullptr, nullptr, nullptr, toolsHolder );

    MULTICHANNEL_TOOL* mtTool = new MULTICHANNEL_TOOL;
    toolMgr.RegisterTool( mtTool );

    REPEAT_LAYOUT_OPTIONS opts = { .m_copyRouting = true,
                                   .m_connectedRoutingOnly = false,
                                   .m_copyPlacement = true,
                                   .m_copyOtherItems = true,
                                   .m_groupItems = false,
                                   .m_includeLockedItems = true,
                                   .m_anchorFp = nullptr };

    wxString err;
    int      result = mtTool->RepeatLayout( TOOL_EVENT(), dbRA, destRA, opts, nullptr, &err );

    delete dbRA.m_zone;
    delete destRA.m_zone;

    BOOST_REQUIRE_MESSAGE( result >= 0, wxString::Format( "RepeatLayout failed for a footprint-free "
                                                          "(graphics-only) design block (issue 24592): %s",
                                                          err ) );

    // The block's rectangle is copied into the target region and grouped, replacing the group's
    // prior placeholder graphic (source + 1 copy = 2 silkscreen shapes).
    int        silkAfter = 0;
    PCB_SHAPE* copiedRect = nullptr;

    for( BOARD_ITEM* item : m_board->Drawings() )
    {
        if( item->Type() != PCB_SHAPE_T || item->GetLayer() != F_SilkS )
            continue;

        silkAfter++;

        PCB_SHAPE* shape = static_cast<PCB_SHAPE*>( item );

        if( shape != srcRect && shape->GetShape() == SHAPE_T::RECTANGLE )
            copiedRect = shape;
    }

    BOOST_CHECK_MESSAGE( silkAfter == 2, wxString::Format( "Expected 2 silkscreen shapes after Apply Design Block "
                                                           "Layout (source + 1 copy, placeholder replaced), found "
                                                           "%d (issue 24592)",
                                                           silkAfter ) );
    BOOST_REQUIRE_MESSAGE( copiedRect != nullptr, "Footprint-free block graphic was not copied (issue 24592)" );
    BOOST_CHECK_MESSAGE( copiedRect->GetParentGroup() == destGroup,
                         "Copied block graphic was not added to the destination group (issue 24592)" );
    BOOST_CHECK_GT( copiedRect->GetStart().x, pcbIUScale.mmToIU( 30 ) );
}


/**
 * A footprint-free block may contain copper (e.g. an antenna). With no footprints there are no
 * pads to map nets through, so copied copper comes in as no-net (issue 24592).
 */
BOOST_FIXTURE_TEST_CASE( ApplyDesignBlockLayoutFootprintFreeCopperIsNoNet, MULTICHANNEL_TEST_FIXTURE )
{
    m_board = std::make_unique<BOARD>();
    m_board->SetEnabledLayers( LSET::AllCuMask() | LSET::AllTechMask() );

    NETINFO_ITEM* net = new NETINFO_ITEM( m_board.get(), wxT( "NET1" ), 1 );
    m_board->Add( net );

    // Source: footprint-free block with one track on a real net.
    PCB_TRACK* srcTrack = new PCB_TRACK( m_board.get() );
    srcTrack->SetLayer( F_Cu );
    srcTrack->SetWidth( pcbIUScale.mmToIU( 0.25 ) );
    srcTrack->SetStart( VECTOR2I( pcbIUScale.mmToIU( -2 ), pcbIUScale.mmToIU( 0 ) ) );
    srcTrack->SetEnd( VECTOR2I( pcbIUScale.mmToIU( 2 ), pcbIUScale.mmToIU( 0 ) ) );
    srcTrack->SetNet( net );
    m_board->Add( srcTrack );

    BOOST_REQUIRE_EQUAL( srcTrack->GetNetCode(), 1 );

    // Destination: footprint-free group with a placeholder so the group exists.
    PCB_SHAPE* destPlaceholder = new PCB_SHAPE( m_board.get(), SHAPE_T::SEGMENT );
    destPlaceholder->SetStart( VECTOR2I( pcbIUScale.mmToIU( 49 ), pcbIUScale.mmToIU( 49 ) ) );
    destPlaceholder->SetEnd( VECTOR2I( pcbIUScale.mmToIU( 51 ), pcbIUScale.mmToIU( 49 ) ) );
    destPlaceholder->SetLayer( F_SilkS );
    destPlaceholder->SetStroke( STROKE_PARAMS( pcbIUScale.mmToIU( 0.15 ), LINE_STYLE::SOLID ) );
    m_board->Add( destPlaceholder );

    PCB_GROUP* destGroup = new PCB_GROUP( m_board.get() );
    destGroup->SetName( wxT( "design-block-dest" ) );
    destGroup->AddItem( destPlaceholder );
    m_board->Add( destGroup );

    RULE_AREA dbRA;
    dbRA.m_sourceType = PLACEMENT_SOURCE_T::DESIGN_BLOCK;
    dbRA.m_designBlockItems.insert( srcTrack ); // no footprints -> m_components stays empty

    dbRA.m_zone = new ZONE( m_board.get() );
    dbRA.m_zone->SetIsRuleArea( true );
    dbRA.m_zone->SetLayerSet( LSET::AllCuMask() );
    dbRA.m_zone->AddPolygon(
            KIGEOM::BoxToLineChain( BOX2I::ByCorners( VECTOR2I( pcbIUScale.mmToIU( -5 ), pcbIUScale.mmToIU( -5 ) ),
                                                      VECTOR2I( pcbIUScale.mmToIU( 5 ), pcbIUScale.mmToIU( 5 ) ) ) ) );
    dbRA.m_center = dbRA.m_zone->Outline()->COutline( 0 ).Centre();

    RULE_AREA destRA;
    destRA.m_sourceType = PLACEMENT_SOURCE_T::GROUP_PLACEMENT;
    destRA.m_group = destGroup;

    destRA.m_zone = new ZONE( m_board.get() );
    destRA.m_zone->SetIsRuleArea( true );
    destRA.m_zone->SetLayerSet( LSET::AllCuMask() );
    destRA.m_zone->AddPolygon( KIGEOM::BoxToLineChain(
            BOX2I::ByCorners( VECTOR2I( pcbIUScale.mmToIU( 45 ), pcbIUScale.mmToIU( 45 ) ),
                              VECTOR2I( pcbIUScale.mmToIU( 55 ), pcbIUScale.mmToIU( 55 ) ) ) ) );
    destRA.m_center = destRA.m_zone->Outline()->COutline( 0 ).Centre();

    TOOL_MANAGER       toolMgr;
    MOCK_TOOLS_HOLDER* toolsHolder = new MOCK_TOOLS_HOLDER;
    toolMgr.SetEnvironment( m_board.get(), nullptr, nullptr, nullptr, toolsHolder );

    MULTICHANNEL_TOOL* mtTool = new MULTICHANNEL_TOOL;
    toolMgr.RegisterTool( mtTool );

    REPEAT_LAYOUT_OPTIONS opts = { .m_copyRouting = true,
                                   .m_connectedRoutingOnly = false,
                                   .m_copyPlacement = true,
                                   .m_copyOtherItems = true,
                                   .m_groupItems = false,
                                   .m_includeLockedItems = true,
                                   .m_anchorFp = nullptr };

    wxString err;
    int      result = mtTool->RepeatLayout( TOOL_EVENT(), dbRA, destRA, opts, nullptr, &err );

    delete dbRA.m_zone;
    delete destRA.m_zone;

    BOOST_REQUIRE_MESSAGE( result >= 0, wxString::Format( "RepeatLayout failed for a footprint-free copper "
                                                          "block (issue 24592): %s",
                                                          err ) );

    // Copied track should be in the target region, no-net, and grouped.
    PCB_TRACK* copiedTrack = nullptr;

    for( PCB_TRACK* track : m_board->Tracks() )
    {
        if( track != srcTrack )
            copiedTrack = track;
    }

    BOOST_REQUIRE_MESSAGE( copiedTrack != nullptr, "Footprint-free block track was not copied (issue 24592)" );
    BOOST_CHECK_MESSAGE( copiedTrack->GetNetCode() == 0,
                         wxString::Format( "Copied copper should be no-net, got net code %d "
                                           "(issue 24592)",
                                           copiedTrack->GetNetCode() ) );
    BOOST_CHECK_MESSAGE( copiedTrack->GetParentGroup() == destGroup,
                         "Copied copper was not added to the destination group (issue 24592)" );
    BOOST_CHECK_GT( copiedTrack->GetStart().x, pcbIUScale.mmToIU( 30 ) );

    // The original source track must keep its net.
    BOOST_CHECK_EQUAL( srcTrack->GetNetCode(), 1 );
}


/**
 * Re-applying a footprint-free block must replace the group's previously-copied content, not
 * stack a fresh copy on top of it (issue 24592). The group is found via the target rule area's
 * explicit group, since there is no member footprint to recover it from.
 */
BOOST_FIXTURE_TEST_CASE( ApplyDesignBlockLayoutFootprintFreeReapplyReplaces, MULTICHANNEL_TEST_FIXTURE )
{
    m_board = std::make_unique<BOARD>();
    m_board->SetEnabledLayers( LSET::AllCuMask() | LSET::AllTechMask() );

    // Source: footprint-free block with one silkscreen rectangle centered on origin.
    PCB_SHAPE* srcRect = new PCB_SHAPE( m_board.get(), SHAPE_T::RECTANGLE );
    srcRect->SetStart( VECTOR2I( pcbIUScale.mmToIU( -2 ), pcbIUScale.mmToIU( -2 ) ) );
    srcRect->SetEnd( VECTOR2I( pcbIUScale.mmToIU( 2 ), pcbIUScale.mmToIU( 2 ) ) );
    srcRect->SetLayer( F_SilkS );
    srcRect->SetStroke( STROKE_PARAMS( pcbIUScale.mmToIU( 0.15 ), LINE_STYLE::SOLID ) );
    m_board->Add( srcRect );

    // Destination: footprint-free group with a placeholder so the group exists.
    PCB_SHAPE* destPlaceholder = new PCB_SHAPE( m_board.get(), SHAPE_T::SEGMENT );
    destPlaceholder->SetStart( VECTOR2I( pcbIUScale.mmToIU( 49 ), pcbIUScale.mmToIU( 49 ) ) );
    destPlaceholder->SetEnd( VECTOR2I( pcbIUScale.mmToIU( 51 ), pcbIUScale.mmToIU( 49 ) ) );
    destPlaceholder->SetLayer( F_SilkS );
    destPlaceholder->SetStroke( STROKE_PARAMS( pcbIUScale.mmToIU( 0.15 ), LINE_STYLE::SOLID ) );
    m_board->Add( destPlaceholder );

    PCB_GROUP* destGroup = new PCB_GROUP( m_board.get() );
    destGroup->SetName( wxT( "design-block-dest" ) );
    destGroup->AddItem( destPlaceholder );
    m_board->Add( destGroup );

    TOOL_MANAGER       toolMgr;
    MOCK_TOOLS_HOLDER* toolsHolder = new MOCK_TOOLS_HOLDER;
    toolMgr.SetEnvironment( m_board.get(), nullptr, nullptr, nullptr, toolsHolder );

    MULTICHANNEL_TOOL* mtTool = new MULTICHANNEL_TOOL;
    toolMgr.RegisterTool( mtTool );

    auto applyOnce = [&]() -> int
    {
        RULE_AREA dbRA;
        dbRA.m_sourceType = PLACEMENT_SOURCE_T::DESIGN_BLOCK;
        dbRA.m_designBlockItems.insert( srcRect );

        dbRA.m_zone = new ZONE( m_board.get() );
        dbRA.m_zone->SetIsRuleArea( true );
        dbRA.m_zone->SetLayerSet( LSET::AllCuMask() );
        dbRA.m_zone->AddPolygon( KIGEOM::BoxToLineChain(
                BOX2I::ByCorners( VECTOR2I( pcbIUScale.mmToIU( -5 ), pcbIUScale.mmToIU( -5 ) ),
                                  VECTOR2I( pcbIUScale.mmToIU( 5 ), pcbIUScale.mmToIU( 5 ) ) ) ) );
        dbRA.m_center = dbRA.m_zone->Outline()->COutline( 0 ).Centre();

        RULE_AREA destRA;
        destRA.m_sourceType = PLACEMENT_SOURCE_T::GROUP_PLACEMENT;
        destRA.m_group = destGroup;

        destRA.m_zone = new ZONE( m_board.get() );
        destRA.m_zone->SetIsRuleArea( true );
        destRA.m_zone->SetLayerSet( LSET::AllCuMask() );
        destRA.m_zone->AddPolygon( KIGEOM::BoxToLineChain(
                BOX2I::ByCorners( VECTOR2I( pcbIUScale.mmToIU( 45 ), pcbIUScale.mmToIU( 45 ) ),
                                  VECTOR2I( pcbIUScale.mmToIU( 55 ), pcbIUScale.mmToIU( 55 ) ) ) ) );
        destRA.m_center = destRA.m_zone->Outline()->COutline( 0 ).Centre();

        REPEAT_LAYOUT_OPTIONS opts = { .m_copyRouting = true,
                                       .m_connectedRoutingOnly = false,
                                       .m_copyPlacement = true,
                                       .m_copyOtherItems = true,
                                       .m_groupItems = false,
                                       .m_includeLockedItems = true,
                                       .m_anchorFp = nullptr };

        int result = mtTool->RepeatLayout( TOOL_EVENT(), dbRA, destRA, opts );

        delete dbRA.m_zone;
        delete destRA.m_zone;
        return result;
    };

    BOOST_REQUIRE_MESSAGE( applyOnce() >= 0, "First apply failed (issue 24592)" );
    BOOST_REQUIRE_MESSAGE( applyOnce() >= 0, "Second apply failed (issue 24592)" );

    // After two applies there must be exactly the source rectangle plus one copy. A third
    // rectangle would mean the second apply stacked instead of replacing.
    int rects = 0;

    for( BOARD_ITEM* item : m_board->Drawings() )
    {
        if( item->Type() == PCB_SHAPE_T && item->GetLayer() == F_SilkS
            && static_cast<PCB_SHAPE*>( item )->GetShape() == SHAPE_T::RECTANGLE )
        {
            rects++;
        }
    }

    BOOST_CHECK_MESSAGE( rects == 2, wxString::Format( "Re-apply stacked copies: expected 2 rectangles "
                                                       "(source + 1 copy), found %d (issue 24592)",
                                                       rects ) );
}


BOOST_AUTO_TEST_SUITE_END()
