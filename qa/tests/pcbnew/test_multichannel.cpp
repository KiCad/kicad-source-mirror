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
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
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


int countZonesByNamePrefixInRuleArea( BOARD* aBoard, const wxString& aBaseName, const RULE_AREA& aRuleArea )
{
    int count = 0;

    for( const ZONE* zone : aBoard->Zones() )
    {
        if( zone == aRuleArea.m_zone )
            continue;

        const wxString& name = zone->GetZoneName();

        // A copied zone may get a _<n> suffix for uniqueness (issue 23131).
        if( name != aBaseName && !name.StartsWith( aBaseName + wxT( "_" ) ) )
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
    // The copy is renamed for uniqueness (issue 23131), so match the base name as a prefix.
    BOOST_CHECK_EQUAL( countZonesByNamePrefixInRuleArea( m_board.get(), wxT( "MultilayerZoneFrontAndOne" ), *destA ),
                       1 );

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
 * Apply Design Block Layout maps block footprints to their placed instances by symbol instance
 * UUID when the board's net topology no longer matches the block (issue: a board wire makes the
 * two non isomorphic, which used to abort with "No compatible component found in the target area").
 */
BOOST_FIXTURE_TEST_CASE( ApplyDesignBlockLayoutMatchesBySymbolPathWhenTopologyDiffers, MULTICHANNEL_TEST_FIXTURE )
{
    using TMATCH::CONNECTION_GRAPH;

    m_board = std::make_unique<BOARD>();
    m_board->SetEnabledLayers( LSET::AllCuMask() );

    NETINFO_ITEM* shared = new NETINFO_ITEM( m_board.get(), wxT( "Net-(J1-Pin_1)" ) );
    m_board->Add( shared );

    auto makeFootprint = [&]( const wxString& aRef, const VECTOR2I& aPos, const KIID& aSymbolUuid,
                              NETINFO_ITEM* aNet ) -> FOOTPRINT*
    {
        FOOTPRINT* fp = new FOOTPRINT( m_board.get() );
        fp->SetFPID( LIB_ID( wxT( "TestLib" ), wxT( "Receptacle" ) ) );
        fp->SetReference( aRef );
        fp->SetPosition( aPos );

        // Symbol instance UUID, the link between a block footprint and its placed instance
        KIID_PATH path;
        path.push_back( aSymbolUuid );
        fp->SetPath( path );

        PAD* pad = new PAD( fp );
        pad->SetNumber( wxT( "1" ) );
        pad->SetShape( PADSTACK::ALL_LAYERS, PAD_SHAPE::CIRCLE );
        pad->SetPosition( aPos );
        pad->SetSize( PADSTACK::ALL_LAYERS, VECTOR2I( pcbIUScale.mmToIU( 1 ), pcbIUScale.mmToIU( 1 ) ) );
        pad->SetLayerSet( LSET( { F_Cu } ) );

        if( aNet )
            pad->SetNet( aNet );

        fp->Add( pad );
        m_board->Add( fp );
        return fp;
    };

    KIID symA, symB;

    // Block source: both receptacles unconnected, so each pad has zero connections
    FOOTPRINT* refFpA = makeFootprint( wxT( "J5" ), VECTOR2I( 0, 0 ), symA, nullptr );
    FOOTPRINT* refFpB = makeFootprint( wxT( "J6" ), VECTOR2I( pcbIUScale.mmToIU( 10 ), 0 ), symB, nullptr );

    // Board instance: same symbol instances, but a board wire ties both pads onto one net
    FOOTPRINT* destFpA =
            makeFootprint( wxT( "J1" ), VECTOR2I( pcbIUScale.mmToIU( 50 ), pcbIUScale.mmToIU( 50 ) ), symA, shared );
    FOOTPRINT* destFpB =
            makeFootprint( wxT( "J2" ), VECTOR2I( pcbIUScale.mmToIU( 80 ), pcbIUScale.mmToIU( 80 ) ), symB, shared );

    // Precondition: topology matching fails, so the symbol path fallback is what rescues the apply
    {
        std::set<FOOTPRINT*>      refSet{ refFpA, refFpB };
        std::set<FOOTPRINT*>      destSet{ destFpA, destFpB };
        auto                      cgRef = CONNECTION_GRAPH::BuildFromFootprintSet( refSet, destSet );
        auto                      cgTarget = CONNECTION_GRAPH::BuildFromFootprintSet( destSet, refSet );
        TMATCH::COMPONENT_MATCHES topoOnly;
        std::vector<TMATCH::TOPOLOGY_MISMATCH_REASON> details;
        BOOST_REQUIRE_MESSAGE( !cgRef->FindIsomorphism( cgTarget.get(), topoOnly, details ),
                               "Test precondition broken: areas are net isomorphic" );
    }

    PCB_GROUP* destGroup = new PCB_GROUP( m_board.get() );
    destGroup->SetName( wxT( "design-block-dest" ) );
    destGroup->AddItem( destFpA );
    destGroup->AddItem( destFpB );
    m_board->Add( destGroup );

    RULE_AREA dbRA;
    dbRA.m_sourceType = PLACEMENT_SOURCE_T::DESIGN_BLOCK;
    dbRA.m_components.insert( refFpA );
    dbRA.m_components.insert( refFpB );
    dbRA.m_designBlockItems.insert( refFpA );
    dbRA.m_designBlockItems.insert( refFpB );
    dbRA.m_zone = new ZONE( m_board.get() );
    dbRA.m_zone->SetIsRuleArea( true );
    dbRA.m_zone->SetLayerSet( LSET::AllCuMask() );
    dbRA.m_zone->AddPolygon(
            KIGEOM::BoxToLineChain( BOX2I::ByCorners( VECTOR2I( pcbIUScale.mmToIU( -5 ), pcbIUScale.mmToIU( -5 ) ),
                                                      VECTOR2I( pcbIUScale.mmToIU( 15 ), pcbIUScale.mmToIU( 5 ) ) ) ) );

    RULE_AREA destRA;
    destRA.m_sourceType = PLACEMENT_SOURCE_T::GROUP_PLACEMENT;
    destRA.m_components.insert( destFpA );
    destRA.m_components.insert( destFpB );
    destRA.m_group = destGroup;
    destRA.m_zone = new ZONE( m_board.get() );
    destRA.m_zone->SetIsRuleArea( true );
    destRA.m_zone->SetLayerSet( LSET::AllCuMask() );
    destRA.m_zone->AddPolygon( KIGEOM::BoxToLineChain(
            BOX2I::ByCorners( VECTOR2I( pcbIUScale.mmToIU( 40 ), pcbIUScale.mmToIU( 40 ) ),
                              VECTOR2I( pcbIUScale.mmToIU( 90 ), pcbIUScale.mmToIU( 90 ) ) ) ) );

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

    delete dbRA.m_zone;
    delete destRA.m_zone;

    BOOST_CHECK_MESSAGE( result >= 0, "Apply Design Block Layout aborted even though the symbol instance paths "
                                      "give an unambiguous mapping (No compatible component regression)" );

    // Correct pairing (J5->J1, J6->J2) reproduces the source 10mm spacing, a swap would give -10mm
    VECTOR2I delta = destFpB->GetPosition() - destFpA->GetPosition();
    BOOST_CHECK_MESSAGE( delta == VECTOR2I( pcbIUScale.mmToIU( 10 ), 0 ),
                         wxString::Format( "Destination relative placement wrong, expected "
                                           "(10mm,0) got (%d,%d) IU",
                                           delta.x, delta.y ) );
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


/**
 * Applying a design block layout must not delete routing owned by another group. When several
 * instances of the same block overlap (e.g. stacked right after Update PCB) the target area
 * encloses a sibling instance's traces. Removing them made each apply wipe the previously applied
 * instance's routing, so only the last instance kept its layout (issue 24767). Loose, ungrouped
 * routing in the target area must still be replaced.
 */
BOOST_FIXTURE_TEST_CASE( ApplyDesignBlockLayoutKeepsOtherGroupRouting, MULTICHANNEL_TEST_FIXTURE )
{
    m_board = std::make_unique<BOARD>();
    m_board->SetEnabledLayers( LSET::AllCuMask() | LSET::AllTechMask() );

    NETINFO_ITEM* net = new NETINFO_ITEM( m_board.get(), wxT( "NET1" ), 1 );
    m_board->Add( net );

    // Source block: one track centered on origin.
    PCB_TRACK* srcTrack = new PCB_TRACK( m_board.get() );
    srcTrack->SetLayer( F_Cu );
    srcTrack->SetWidth( pcbIUScale.mmToIU( 0.25 ) );
    srcTrack->SetStart( VECTOR2I( pcbIUScale.mmToIU( -2 ), 0 ) );
    srcTrack->SetEnd( VECTOR2I( pcbIUScale.mmToIU( 2 ), 0 ) );
    srcTrack->SetNet( net );
    m_board->Add( srcTrack );

    // Destination group with a placeholder so the group exists.
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

    // A sibling instance's track, inside the destination area but owned by another group.
    PCB_TRACK* siblingTrack = new PCB_TRACK( m_board.get() );
    siblingTrack->SetLayer( F_Cu );
    siblingTrack->SetWidth( pcbIUScale.mmToIU( 0.25 ) );
    siblingTrack->SetStart( VECTOR2I( pcbIUScale.mmToIU( 47 ), pcbIUScale.mmToIU( 47 ) ) );
    siblingTrack->SetEnd( VECTOR2I( pcbIUScale.mmToIU( 49 ), pcbIUScale.mmToIU( 47 ) ) );
    m_board->Add( siblingTrack );

    PCB_GROUP* siblingGroup = new PCB_GROUP( m_board.get() );
    siblingGroup->SetName( wxT( "design-block-sibling" ) );
    siblingGroup->AddItem( siblingTrack );
    m_board->Add( siblingGroup );

    // Loose, ungrouped routing inside the destination area, which still gets replaced.
    PCB_TRACK* looseTrack = new PCB_TRACK( m_board.get() );
    looseTrack->SetLayer( F_Cu );
    looseTrack->SetWidth( pcbIUScale.mmToIU( 0.25 ) );
    looseTrack->SetStart( VECTOR2I( pcbIUScale.mmToIU( 47 ), pcbIUScale.mmToIU( 53 ) ) );
    looseTrack->SetEnd( VECTOR2I( pcbIUScale.mmToIU( 49 ), pcbIUScale.mmToIU( 53 ) ) );
    m_board->Add( looseTrack );

    RULE_AREA dbRA;
    dbRA.m_sourceType = PLACEMENT_SOURCE_T::DESIGN_BLOCK;
    dbRA.m_designBlockItems.insert( srcTrack );

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

    BOOST_REQUIRE_MESSAGE( result >= 0, wxString::Format( "RepeatLayout failed: %s", err ) );

    // Pointers to removed tracks are deleted, so count survivors by location instead.
    int siblingTracks = 0;
    int looseTracks = 0;

    for( PCB_TRACK* track : m_board->Tracks() )
    {
        int y = track->GetStart().y;

        if( y > pcbIUScale.mmToIU( 46 ) && y < pcbIUScale.mmToIU( 48 ) )
            siblingTracks++;
        else if( y > pcbIUScale.mmToIU( 52 ) && y < pcbIUScale.mmToIU( 54 ) )
            looseTracks++;
    }

    BOOST_CHECK_MESSAGE( siblingTracks == 1,
                         wxString::Format( "Sibling group routing was deleted (issue 24767): found %d, expected 1",
                                           siblingTracks ) );
    BOOST_CHECK_MESSAGE( looseTracks == 0,
                         wxString::Format( "Loose routing in the target area should be replaced: found %d, expected 0",
                                           looseTracks ) );
}


/**
 * Repeat layout must refuse a target Rule Area that resolves to the same components as the
 * reference area (issue 22318). The repro board has four placement rule areas all bound to the
 * same sheet "/test sheet 1/", so each resolves to the identical footprint set; copying one onto
 * another would move and delete the reference's own items.
 */
BOOST_FIXTURE_TEST_CASE( RepeatLayoutRefusesDuplicatePlacementAreas, MULTICHANNEL_TEST_FIXTURE )
{
    KI_TEST::LoadBoard( m_settingsManager, "issue22318/issue22318", m_board );

    TOOL_MANAGER       toolMgr;
    MOCK_TOOLS_HOLDER* toolsHolder = new MOCK_TOOLS_HOLDER;

    toolMgr.SetEnvironment( m_board.get(), nullptr, nullptr, nullptr, toolsHolder );

    MULTICHANNEL_TOOL* mtTool = new MULTICHANNEL_TOOL;
    toolMgr.RegisterTool( mtTool );

    mtTool->FindExistingRuleAreas();

    auto ruleData = mtTool->GetData();

    BOOST_TEST_MESSAGE( wxString::Format( "Found %d rule areas",
                                          static_cast<int>( ruleData->m_areas.size() ) ) );

    BOOST_REQUIRE_EQUAL( ruleData->m_areas.size(), 4 );

    // The shared membership, not the exact count, is what makes the areas invalid targets.
    const size_t sharedCount = ruleData->m_areas.front().m_components.size();

    BOOST_REQUIRE( sharedCount > 0 );

    for( const RULE_AREA& ra : ruleData->m_areas )
        BOOST_CHECK_EQUAL( ra.m_components.size(), sharedCount );

    RULE_AREA* refArea = &ruleData->m_areas.front();

    BOOST_REQUIRE( mtTool->CheckRACompatibility( refArea->m_zone ) >= 0 );

    BOOST_REQUIRE_EQUAL( ruleData->m_compatMap.size(), ruleData->m_areas.size() - 1 );

    for( const auto& [targetArea, compatData] : ruleData->m_compatMap )
    {
        BOOST_CHECK_MESSAGE( !compatData.m_isOk,
                             "Duplicate placement area was wrongly reported as a valid copy target "
                             "(issue 22318)" );
        BOOST_CHECK( !compatData.m_mismatchReasons.empty() );
    }

    // The single-target overload must also refuse rather than corrupt the board.
    RULE_AREA* targetArea = &ruleData->m_areas[1];

    REPEAT_LAYOUT_OPTIONS opts;
    wxString              err;

    int result = mtTool->RepeatLayout( TOOL_EVENT(), *refArea, *targetArea, opts, nullptr, &err );

    BOOST_CHECK_MESSAGE( result < 0,
                         "RepeatLayout copied a Rule Area onto an identical-component area "
                         "(issue 22318)" );
    BOOST_CHECK( !err.IsEmpty() );
}


/**
 * Repeat layout with "group items with their target rule areas" must not duplicate unrelated
 * user groups that merely overlap the source rule area (issue 22316).
 *
 * The fixture has four sheet-based rule areas ("test 1".."test 4") and three user groups
 * ("test group A/B/C") that contain footprints and tracks.  Some of those grouped tracks fall
 * geometrically inside the source rule area, so the copy-routing pass duplicates them.  The bug
 * was that cloning the parent group of any single copied item produced partial "phantom" clones
 * of the unrelated user groups.  After the fix, a source group is only reconstructed in the
 * target when all of its members are part of the copied layout.
 */
BOOST_FIXTURE_TEST_CASE( RepeatLayoutDoesNotDuplicateUnrelatedGroups, MULTICHANNEL_TEST_FIXTURE )
{
    KI_TEST::LoadBoard( m_settingsManager, "issue22316/issue22316", m_board );

    TOOL_MANAGER       toolMgr;
    MOCK_TOOLS_HOLDER* toolsHolder = new MOCK_TOOLS_HOLDER;

    toolMgr.SetEnvironment( m_board.get(), nullptr, nullptr, nullptr, toolsHolder );

    MULTICHANNEL_TOOL* mtTool = new MULTICHANNEL_TOOL;
    toolMgr.RegisterTool( mtTool );

    mtTool->FindExistingRuleAreas();

    auto ruleData = mtTool->GetData();

    BOOST_TEST_MESSAGE( wxString::Format( "Found %d rule areas",
                                          static_cast<int>( ruleData->m_areas.size() ) ) );

    BOOST_REQUIRE_EQUAL( ruleData->m_areas.size(), 4 );

    RULE_AREA* refArea = nullptr;

    for( RULE_AREA& ra : ruleData->m_areas )
    {
        if( ra.m_ruleName == wxT( "test 1" ) )
            refArea = &ra;
    }

    BOOST_REQUIRE( refArea != nullptr );

    std::set<KIID>     groupUuidsBefore;
    std::set<wxString> userGroupNamesBefore;

    for( PCB_GROUP* group : m_board->Groups() )
    {
        groupUuidsBefore.insert( group->m_Uuid );

        if( !group->GetName().IsEmpty() )
            userGroupNamesBefore.insert( group->GetName() );
    }

    mtTool->CheckRACompatibility( refArea->m_zone );

    for( auto& [targetArea, compatData] : ruleData->m_compatMap )
        compatData.m_doCopy = true;

    ruleData->m_options.m_copyPlacement = true;
    ruleData->m_options.m_copyRouting = true;
    ruleData->m_options.m_copyOtherItems = true;
    ruleData->m_options.m_groupItems = true;
    ruleData->m_options.m_includeLockedItems = true;

    int result = mtTool->RepeatLayout( TOOL_EVENT(), refArea->m_zone );

    BOOST_REQUIRE( result >= 0 );

    int clonedUserGroups = 0;

    for( PCB_GROUP* group : m_board->Groups() )
    {
        bool isNew = !groupUuidsBefore.contains( group->m_Uuid );

        if( isNew && userGroupNamesBefore.contains( group->GetName() ) )
            clonedUserGroups++;
    }

    BOOST_CHECK_MESSAGE( clonedUserGroups == 0,
                         wxString::Format( "Repeat layout cloned %d unrelated user groups "
                                           "(issue 22316)",
                                           clonedUserGroups ) );
}


/**
 * Two channels of the same hierarchical sheet can have genuinely different internal connectivity
 * when one channel's hierarchical pins are externally looped together on the parent sheet
 * (issue 24192).  In that case the topology match correctly fails, but the failure message must
 * explain the real connectivity difference instead of a generic "no compatible component found"
 * message that wrongly implies a missing or differently-counted part.
 */
BOOST_FIXTURE_TEST_CASE( TopoMatchExternalLoopReportsConnectivity, MULTICHANNEL_TEST_FIXTURE )
{
    using TMATCH::CONNECTION_GRAPH;

    KI_TEST::LoadBoard( m_settingsManager, "issue24192/issue24192", m_board );

    TOOL_MANAGER       toolMgr;
    MOCK_TOOLS_HOLDER* toolsHolder = new MOCK_TOOLS_HOLDER;
    toolMgr.SetEnvironment( m_board.get(), nullptr, nullptr, nullptr, toolsHolder );

    MULTICHANNEL_TOOL* mtTool = new MULTICHANNEL_TOOL;
    toolMgr.RegisterTool( mtTool );

    mtTool->FindExistingRuleAreas();

    auto ruleData = mtTool->GetData();

    BOOST_REQUIRE_EQUAL( ruleData->m_areas.size(), 2 );

    RULE_AREA* refArea = nullptr;
    RULE_AREA* targetArea = nullptr;

    for( RULE_AREA& ra : ruleData->m_areas )
    {
        if( ra.m_ruleName.Contains( wxT( "Untitled Sheet/" ) ) )
            refArea = &ra;
        else if( ra.m_ruleName.Contains( wxT( "Untitled Sheet1/" ) ) )
            targetArea = &ra;
    }

    BOOST_REQUIRE( refArea != nullptr );
    BOOST_REQUIRE( targetArea != nullptr );

    // The component counts are identical; only the internal connectivity differs.
    BOOST_CHECK_EQUAL( refArea->m_components.size(), targetArea->m_components.size() );

    auto cgRef = CONNECTION_GRAPH::BuildFromFootprintSet( refArea->m_components,
                                                          targetArea->m_components );
    auto cgTarget = CONNECTION_GRAPH::BuildFromFootprintSet( targetArea->m_components,
                                                             refArea->m_components );

    TMATCH::COMPONENT_MATCHES result;
    std::vector<TMATCH::TOPOLOGY_MISMATCH_REASON> details;
    bool status = cgRef->FindIsomorphism( cgTarget.get(), result, details );

    // The topology genuinely differs, so matching must fail and produce a reason.
    BOOST_CHECK( !status );
    BOOST_REQUIRE( !details.empty() );

    // Collect the reference designators that belong to each channel so we can verify the
    // reason's reference/candidate orientation matches the ref/target areas.
    std::set<wxString> refRefDes;
    std::set<wxString> targetRefDes;

    for( FOOTPRINT* fp : refArea->m_components )
        refRefDes.insert( fp->GetReference() );

    for( FOOTPRINT* fp : targetArea->m_components )
        targetRefDes.insert( fp->GetReference() );

    bool sawConnectivityReason = false;

    for( const auto& reason : details )
    {
        BOOST_TEST_MESSAGE( wxString::Format( "reason: %s <-> %s: %s", reason.m_reference,
                                              reason.m_candidate, reason.m_reason ) );

        // The generic fallback message must no longer be the only thing reported; instead the
        // connectivity difference detected by the per-pad isomorphism check must surface.
        BOOST_CHECK( !reason.m_reason.Contains( wxT( "No compatible component found" ) ) );

        if( reason.m_reason.Contains( wxT( "connects to" ) )
            || reason.m_reason.Contains( wxT( "connectivity" ) ) )
        {
            sawConnectivityReason = true;

            // The reference designator in the reason must come from the reference channel and
            // the candidate from the target channel, not the other way around (codex-review).
            BOOST_CHECK_MESSAGE( refRefDes.count( reason.m_reference ) > 0,
                                 wxString::Format( "Reason reference '%s' should be a reference-area "
                                                   "component", reason.m_reference ) );
            BOOST_CHECK_MESSAGE( targetRefDes.count( reason.m_candidate ) > 0,
                                 wxString::Format( "Reason candidate '%s' should be a target-area "
                                                   "component", reason.m_candidate ) );
        }
    }

    BOOST_CHECK_MESSAGE( sawConnectivityReason,
                         "Topology mismatch message should explain the connectivity difference "
                         "rather than report a generic missing-component error (issue 24192)" );
}


/**
 * Issue 24767: three copies of one design block, but apply works for only two of them.
 * The block's auto net name Net-(D3-A) clashes with a same name net on a different part in
 * the copy. They get merged, the matcher then drops them, and the wiring no longer matches.
 * Isolating the block's auto nets before matching lets every copy apply.
 */
BOOST_FIXTURE_TEST_CASE( TopoMatchCollidingAutoNetName, MULTICHANNEL_TEST_FIXTURE )
{
    using TMATCH::CONNECTION_GRAPH;

    std::unique_ptr<BOARD> board = std::make_unique<BOARD>();

    auto addNet = [&]( const wxString& aName ) -> int
    {
        NETINFO_ITEM* net = new NETINFO_ITEM( board.get(), aName );
        board->Add( net );
        return net->GetNetCode();
    };

    const int netGnd = addNet( wxT( "GND" ) );
    const int netP3V3 = addNet( wxT( "+3V3" ) );
    const int netCollide = addNet( wxT( "Net-(D3-A)" ) ); // shared name, lands on a different part
    const int netRefY = addNet( wxT( "Net-(D4-A)" ) );    // block other LED anode
    const int netTgtX = addNet( wxT( "Net-(D2-A)" ) );    // copy other LED anode

    LIB_ID ledAId( wxT( "TestLib" ), wxT( "LED_A" ) );
    LIB_ID ledBId( wxT( "TestLib" ), wxT( "LED_B" ) );
    LIB_ID resId( wxT( "TestLib" ), wxT( "R" ) );

    auto addPad = [&]( FOOTPRINT* fp, const wxString& aNum, int aNet )
    {
        PAD* pad = new PAD( fp );
        pad->SetNumber( aNum );
        pad->SetShape( PADSTACK::ALL_LAYERS, PAD_SHAPE::CIRCLE );
        pad->SetSize( PADSTACK::ALL_LAYERS, VECTOR2I( pcbIUScale.mmToIU( 1 ), pcbIUScale.mmToIU( 1 ) ) );
        pad->SetLayerSet( LSET( { F_Cu } ) );
        pad->SetNetCode( aNet );
        fp->Add( pad );
    };

    auto addFp = [&]( const LIB_ID& aId, const wxString& aRef ) -> FOOTPRINT*
    {
        FOOTPRINT* fp = new FOOTPRINT( board.get() );
        fp->SetFPID( aId );
        fp->SetReference( aRef );
        board->Add( fp );
        return fp;
    };

    // One LED plus its series resistor. The LED anode net is the one that can clash.
    auto addLedAndRes = [&]( const LIB_ID& aLedId, const wxString& aLedRef, const wxString& aResRef, int aAnodeNet,
                             std::set<FOOTPRINT*>& aSet )
    {
        FOOTPRINT* led = addFp( aLedId, aLedRef );
        addPad( led, wxT( "1" ), netGnd );
        addPad( led, wxT( "2" ), aAnodeNet );
        aSet.insert( led );

        FOOTPRINT* res = addFp( resId, aResRef );
        addPad( res, wxT( "1" ), aAnodeNet );
        addPad( res, wxT( "2" ), netP3V3 );
        aSet.insert( res );
    };

    // The block: LED_A is D3 on the clashing net, LED_B is D4 on its own net.
    std::set<FOOTPRINT*> refFps;
    addLedAndRes( ledAId, wxT( "D3" ), wxT( "R27" ), netCollide, refFps );
    addLedAndRes( ledBId, wxT( "D4" ), wxT( "R30" ), netRefY, refFps );

    // The copy: same parts, but the clashing name now sits on LED_B (D3), a different part.
    std::set<FOOTPRINT*> tgtFps;
    addLedAndRes( ledAId, wxT( "D2" ), wxT( "R1" ), netTgtX, tgtFps );
    addLedAndRes( ledBId, wxT( "D3" ), wxT( "R2" ), netCollide, tgtFps );

    // The fix gives the block's auto nets private names so they cannot clash with the copy.
    std::unordered_set<EDA_ITEM*> refItems( refFps.begin(), refFps.end() );
    MULTICHANNEL_TOOL::IsolateDesignBlockAutoNets( board.get(), refFps, refItems );

    auto cgRef = CONNECTION_GRAPH::BuildFromFootprintSet( refFps, tgtFps );
    auto cgTarget = CONNECTION_GRAPH::BuildFromFootprintSet( tgtFps, refFps );

    TMATCH::COMPONENT_MATCHES                     result;
    std::vector<TMATCH::TOPOLOGY_MISMATCH_REASON> details;
    bool                                          status = cgRef->FindIsomorphism( cgTarget.get(), result, details );

    if( !status )
    {
        for( const auto& reason : details )
        {
            BOOST_TEST_MESSAGE( wxString::Format( "Mismatch: %s to %s: %s", reason.m_reference, reason.m_candidate,
                                                  reason.m_reason ) );
        }
    }

    BOOST_CHECK_MESSAGE( status, "Topology match failed even after isolating the block's auto nets "
                                 "(issue 24767)" );
    BOOST_CHECK_EQUAL( result.size(), refFps.size() );
}


/**
 * Four single-pad footprints, identical except for their value (NC_0 / NC_1 / NO_0 / NO_1), used
 * to pair arbitrarily and swap NC with NO. The value breaks the tie. Regression test for the
 * "design blocks swap the schematic-to-PCB link" report.
 */
BOOST_FIXTURE_TEST_CASE( TopoMatchTieBreaksIdenticalPartsByValue, MULTICHANNEL_TEST_FIXTURE )
{
    using TMATCH::CONNECTION_GRAPH;

    std::unique_ptr<BOARD> board = std::make_unique<BOARD>();

    const LIB_ID receptacleId( wxT( "Don-Con" ), wxT( "Mill-Max-Pin_Receptacle" ) );

    // Unique net per footprint, so the four are a topological tie and only the value differs.
    auto makeReceptacle = [&]( const wxString& aRef, const wxString& aValue, bool aWithSymbolPath ) -> FOOTPRINT*
    {
        FOOTPRINT* fp = new FOOTPRINT( board.get() );
        fp->SetFPID( receptacleId );
        fp->SetReference( aRef );
        fp->SetValue( aValue );

        if( aWithSymbolPath )
        {
            KIID_PATH path;
            path.push_back( KIID() );
            fp->SetPath( path );
        }

        NETINFO_ITEM* net = new NETINFO_ITEM( board.get(), wxString::Format( "net_%s", aRef ) );
        board->Add( net );

        PAD* pad = new PAD( fp );
        pad->SetNumber( wxT( "1" ) );
        pad->SetShape( PADSTACK::ALL_LAYERS, PAD_SHAPE::CIRCLE );
        pad->SetSize( PADSTACK::ALL_LAYERS, VECTOR2I( pcbIUScale.mmToIU( 1 ), pcbIUScale.mmToIU( 1 ) ) );
        pad->SetLayerSet( LSET( { F_Cu } ) );
        pad->SetNet( net );
        fp->Add( pad );

        board->Add( fp );
        return fp;
    };

    // Block source: no symbol path (as after AppendBoard). Fixed order keeps the test deterministic.
    FOOTPRINT* refNC0 = makeReceptacle( wxT( "J3" ), wxT( "NC_0" ), false );
    FOOTPRINT* refNC1 = makeReceptacle( wxT( "J5" ), wxT( "NC_1" ), false );
    FOOTPRINT* refNO0 = makeReceptacle( wxT( "J6" ), wxT( "NO_0" ), false );
    FOOTPRINT* refNO1 = makeReceptacle( wxT( "J7" ), wxT( "NO_1" ), false );

    // Target refdes order is the reverse of value order, so a refdes fallback would swap NC and NO.
    FOOTPRINT* tgtNO1 = makeReceptacle( wxT( "J20" ), wxT( "NO_1" ), true );
    FOOTPRINT* tgtNO0 = makeReceptacle( wxT( "J21" ), wxT( "NO_0" ), true );
    FOOTPRINT* tgtNC1 = makeReceptacle( wxT( "J22" ), wxT( "NC_1" ), true );
    FOOTPRINT* tgtNC0 = makeReceptacle( wxT( "J23" ), wxT( "NC_0" ), true );

    auto cgRef = std::make_unique<CONNECTION_GRAPH>();
    cgRef->AddFootprint( refNC0, VECTOR2I( 0, 0 ) );
    cgRef->AddFootprint( refNC1, VECTOR2I( 0, 0 ) );
    cgRef->AddFootprint( refNO0, VECTOR2I( 0, 0 ) );
    cgRef->AddFootprint( refNO1, VECTOR2I( 0, 0 ) );
    cgRef->BuildConnectivity();

    auto cgTarget = std::make_unique<CONNECTION_GRAPH>();
    cgTarget->AddFootprint( tgtNO1, VECTOR2I( 0, 0 ) );
    cgTarget->AddFootprint( tgtNO0, VECTOR2I( 0, 0 ) );
    cgTarget->AddFootprint( tgtNC1, VECTOR2I( 0, 0 ) );
    cgTarget->AddFootprint( tgtNC0, VECTOR2I( 0, 0 ) );
    cgTarget->BuildConnectivity();

    TMATCH::COMPONENT_MATCHES                     result;
    std::vector<TMATCH::TOPOLOGY_MISMATCH_REASON> details;
    bool                                          status = cgRef->FindIsomorphism( cgTarget.get(), result, details );

    BOOST_CHECK( status );
    BOOST_CHECK_EQUAL( result.size(), 4 );

    // Each reference must match the same-value target.
    for( const auto& [refFp, targetFp] : result )
    {
        BOOST_TEST_MESSAGE( wxString::Format( "%s (%s) -> %s (%s)", refFp->GetReference(), refFp->GetValue(),
                                              targetFp->GetReference(), targetFp->GetValue() ) );

        BOOST_CHECK_EQUAL( refFp->GetValue(), targetFp->GetValue() );
    }
}


/**
 * Same identical footprints, but run through the full Apply Design Block Layout. The destination
 * is placed as a mirror of the block, and applying must un-mirror it rather than keep the swap.
 */
BOOST_FIXTURE_TEST_CASE( ApplyDesignBlockLayoutUnmirrorsIdenticalReceptacles, MULTICHANNEL_TEST_FIXTURE )
{
    m_board = std::make_unique<BOARD>();
    m_board->SetEnabledLayers( LSET::AllCuMask() | LSET::AllTechMask() );

    auto makeReceptacle = [&]( const wxString& aRef, const wxString& aValue, const VECTOR2I& aPos,
                               bool aWithSymbolPath ) -> FOOTPRINT*
    {
        FOOTPRINT* fp = new FOOTPRINT( m_board.get() );
        fp->SetFPID( LIB_ID( wxT( "Don-Con" ), wxT( "Mill-Max-Pin_Receptacle" ) ) );
        fp->SetReference( aRef );
        fp->SetValue( aValue );
        fp->SetPosition( aPos );

        if( aWithSymbolPath )
        {
            KIID_PATH path;
            path.push_back( KIID() );
            fp->SetPath( path );
        }

        NETINFO_ITEM* net = new NETINFO_ITEM( m_board.get(), wxString::Format( "net_%s", aRef ) );
        m_board->Add( net );

        PAD* pad = new PAD( fp );
        pad->SetNumber( wxT( "1" ) );
        pad->SetShape( PADSTACK::ALL_LAYERS, PAD_SHAPE::CIRCLE );
        pad->SetSize( PADSTACK::ALL_LAYERS, VECTOR2I( pcbIUScale.mmToIU( 1 ), pcbIUScale.mmToIU( 1 ) ) );
        pad->SetLayerSet( LSET( { F_Cu } ) );
        pad->SetNet( net );
        pad->SetPosition( aPos );
        fp->Add( pad );

        m_board->Add( fp );
        return fp;
    };

    auto mm = []( double a, double b )
    {
        return VECTOR2I( pcbIUScale.mmToIU( a ), pcbIUScale.mmToIU( b ) );
    };

    // Signed area of NC_0, NC_1, NO_0. Same sign as the block is correct, opposite sign is a mirror.
    auto chirality = []( const VECTOR2I& aNC0, const VECTOR2I& aNC1, const VECTOR2I& aNO0 ) -> double
    {
        return (double) ( aNC1.x - aNC0.x ) * ( aNO0.y - aNC0.y ) - (double) ( aNC1.y - aNC0.y ) * ( aNO0.x - aNC0.x );
    };

    // Block source: the reference arrangement, no symbol path (as after AppendBoard).
    FOOTPRINT* srcNC0 = makeReceptacle( wxT( "J3" ), wxT( "NC_0" ), mm( 0, 0 ), false );
    FOOTPRINT* srcNC1 = makeReceptacle( wxT( "J5" ), wxT( "NC_1" ), mm( 0, 7.9 ), false );
    FOOTPRINT* srcNO0 = makeReceptacle( wxT( "J6" ), wxT( "NO_0" ), mm( -3.58, 2.28 ), false );
    FOOTPRINT* srcNO1 = makeReceptacle( wxT( "J7" ), wxT( "NO_1" ), mm( 3.58, 5.62 ), false );

    const double srcChir = chirality( srcNC0->GetPosition(), srcNC1->GetPosition(), srcNO0->GetPosition() );

    // Destination placed as a mirror (NO_0 / NO_1 reflected across the NC axis).
    FOOTPRINT* dstNC0 = makeReceptacle( wxT( "J8" ), wxT( "NC_0" ), mm( 40, 40 ), true );
    FOOTPRINT* dstNC1 = makeReceptacle( wxT( "J9" ), wxT( "NC_1" ), mm( 40, 47.9 ), true );
    FOOTPRINT* dstNO0 = makeReceptacle( wxT( "J10" ), wxT( "NO_0" ), mm( 43.58, 42.28 ), true );
    FOOTPRINT* dstNO1 = makeReceptacle( wxT( "J11" ), wxT( "NO_1" ), mm( 36.42, 45.62 ), true );

    BOOST_REQUIRE_LT( srcChir * chirality( dstNC0->GetPosition(), dstNC1->GetPosition(), dstNO0->GetPosition() ), 0.0 );

    PCB_GROUP* destGroup = new PCB_GROUP( m_board.get() );
    destGroup->SetName( wxT( "Pin receptacles for safety switch" ) );

    for( FOOTPRINT* fp : { dstNC0, dstNC1, dstNO0, dstNO1 } )
        destGroup->AddItem( fp );

    m_board->Add( destGroup );

    RULE_AREA dbRA;
    dbRA.m_sourceType = PLACEMENT_SOURCE_T::DESIGN_BLOCK;

    for( FOOTPRINT* fp : { srcNC0, srcNC1, srcNO0, srcNO1 } )
    {
        dbRA.m_components.insert( fp );
        dbRA.m_designBlockItems.insert( fp );
    }

    dbRA.m_zone = new ZONE( m_board.get() );
    dbRA.m_zone->SetIsRuleArea( true );
    dbRA.m_zone->SetLayerSet( LSET::AllCuMask() );
    dbRA.m_zone->AddPolygon( KIGEOM::BoxToLineChain( BOX2I::ByCorners( mm( -6, -3 ), mm( 6, 11 ) ) ) );

    RULE_AREA destRA;
    destRA.m_sourceType = PLACEMENT_SOURCE_T::GROUP_PLACEMENT;

    for( FOOTPRINT* fp : { dstNC0, dstNC1, dstNO0, dstNO1 } )
        destRA.m_components.insert( fp );

    destRA.m_zone = new ZONE( m_board.get() );
    destRA.m_zone->SetIsRuleArea( true );
    destRA.m_zone->SetLayerSet( LSET::AllCuMask() );
    destRA.m_zone->AddPolygon( KIGEOM::BoxToLineChain( BOX2I::ByCorners( mm( 33, 37 ), mm( 47, 51 ) ) ) );

    TOOL_MANAGER       toolMgr;
    MOCK_TOOLS_HOLDER* toolsHolder = new MOCK_TOOLS_HOLDER;
    toolMgr.SetEnvironment( m_board.get(), nullptr, nullptr, nullptr, toolsHolder );

    MULTICHANNEL_TOOL* mtTool = new MULTICHANNEL_TOOL;
    toolMgr.RegisterTool( mtTool );

    REPEAT_LAYOUT_OPTIONS opts = { .m_copyRouting = false,
                                   .m_connectedRoutingOnly = false,
                                   .m_copyPlacement = true,
                                   .m_copyOtherItems = false,
                                   .m_groupItems = false,
                                   .m_includeLockedItems = true,
                                   .m_anchorFp = nullptr };

    int result = mtTool->RepeatLayout( TOOL_EVENT(), dbRA, destRA, opts );
    BOOST_REQUIRE_MESSAGE( result >= 0, "RepeatLayout failed" );

    delete dbRA.m_zone;
    delete destRA.m_zone;

    const double dstChir = chirality( dstNC0->GetPosition(), dstNC1->GetPosition(), dstNO0->GetPosition() );

    BOOST_TEST_MESSAGE(
            wxString::Format( "block chirality %.0f, destination chirality after apply %.0f", srcChir, dstChir ) );

    BOOST_CHECK_MESSAGE( srcChir * dstChir > 0.0, "Applied layout left the receptacles mirrored (NC/NO swapped)" );
}


/**
 * Issue 24019: a global rail with an uneven per-channel pad count (a config strap tied to GND in
 * some instances, another rail in others) must be excluded from the topology comparison for every
 * pairing, not just the targets that carry two or more pads on it, or isomorphic channels fail to
 * match.  Channel 3 straps to +3V3, leaving one lone GND pad; channel 2 must still match it.
 */
BOOST_FIXTURE_TEST_CASE( CheckRACompatGlobalRailAcrossChannels, MULTICHANNEL_TEST_FIXTURE )
{
    m_board = std::make_unique<BOARD>();
    m_board->SetEnabledLayers( LSET::AllCuMask() | LSET::AllTechMask() );

    auto addNet = [&]( const wxString& aName ) -> int
    {
        NETINFO_ITEM* net = new NETINFO_ITEM( m_board.get(), aName );
        m_board->Add( net );
        return net->GetNetCode();
    };

    const int netGnd = addNet( wxT( "GND" ) );
    const int netVcc = addNet( wxT( "+3V3" ) );

    const LIB_ID icId( wxT( "TestLib" ), wxT( "SOT-23-6" ) );
    const LIB_ID rId( wxT( "TestLib" ), wxT( "R_0402" ) );

    // Each channel is an IC (OUT + GND) and a config resistor strapped to a rail: GND in channels
    // 1 and 2, +3V3 in channel 3.  That strap difference is intentional and must not defeat the match.
    auto makeChannel = [&]( int aIdx, int aStrapNet ) -> ZONE*
    {
        const int netOut = addNet( wxString::Format( wxT( "Net-(U%d-OUT)" ), aIdx ) );
        const int netCfg = addNet( wxString::Format( wxT( "Net-(U%d-CFG)" ), aIdx ) );

        auto addPad = [&]( FOOTPRINT* aFp, const wxString& aNumber, int aNetCode )
        {
            PAD* pad = new PAD( aFp );
            pad->SetNumber( aNumber );
            pad->SetShape( PADSTACK::ALL_LAYERS, PAD_SHAPE::CIRCLE );
            pad->SetSize( PADSTACK::ALL_LAYERS,
                          VECTOR2I( pcbIUScale.mmToIU( 1 ), pcbIUScale.mmToIU( 1 ) ) );
            pad->SetLayerSet( LSET( { F_Cu } ) );
            pad->SetNetCode( aNetCode );
            aFp->Add( pad );
        };

        FOOTPRINT* ic = new FOOTPRINT( m_board.get() );
        ic->SetFPID( icId );
        ic->SetReference( wxString::Format( wxT( "U%d" ), aIdx ) );
        m_board->Add( ic );
        addPad( ic, wxT( "1" ), netOut );
        addPad( ic, wxT( "2" ), netGnd );

        FOOTPRINT* r = new FOOTPRINT( m_board.get() );
        r->SetFPID( rId );
        r->SetReference( wxString::Format( wxT( "R%d" ), aIdx ) );
        m_board->Add( r );
        addPad( r, wxT( "1" ), netCfg );
        addPad( r, wxT( "2" ), aStrapNet );

        ZONE* zone = new ZONE( m_board.get() );
        m_board->Add( zone );

        return zone;
    };

    std::vector<std::pair<int, int>> channelStraps = { { 1, netGnd }, { 2, netGnd }, { 3, netVcc } };
    std::vector<ZONE*>               zones;

    TOOL_MANAGER       toolMgr;
    MOCK_TOOLS_HOLDER* toolsHolder = new MOCK_TOOLS_HOLDER;
    toolMgr.SetEnvironment( m_board.get(), nullptr, nullptr, nullptr, toolsHolder );

    MULTICHANNEL_TOOL* mtTool = new MULTICHANNEL_TOOL;
    toolMgr.RegisterTool( mtTool );

    RULE_AREAS_DATA* ruleData = mtTool->GetData();
    ruleData->m_areas.reserve( channelStraps.size() );

    for( const auto& [idx, strap] : channelStraps )
    {
        ZONE* zone = makeChannel( idx, strap );
        zones.push_back( zone );

        RULE_AREA ra;
        ra.m_zone = zone;
        ra.m_ruleName = wxString::Format( wxT( "Channel %d" ), idx );

        for( FOOTPRINT* fp : m_board->Footprints() )
        {
            if( fp->GetReference() == wxString::Format( wxT( "U%d" ), idx )
                || fp->GetReference() == wxString::Format( wxT( "R%d" ), idx ) )
            {
                ra.m_components.insert( fp );
            }
        }

        ruleData->m_areas.push_back( ra );
    }

    // Before the fix, channel 3's lone GND pad hid the rail from that pairing and the match failed.
    int status = mtTool->CheckRACompatibility( zones[1] );
    BOOST_REQUIRE_EQUAL( status, 0 );

    for( RULE_AREA& ra : ruleData->m_areas )
    {
        if( ra.m_zone == zones[1] )
            continue;

        auto it = ruleData->m_compatMap.find( &ra );
        BOOST_REQUIRE( it != ruleData->m_compatMap.end() );

        for( const wxString& reason : it->second.m_mismatchReasons )
            BOOST_TEST_MESSAGE( wxString::Format( "%s: %s", ra.m_ruleName, reason ) );

        BOOST_CHECK_MESSAGE( it->second.m_isOk,
                             wxString::Format( "Channel 2 must match %s", ra.m_ruleName ) );
    }
}


/**
 * A duplicate rule area over a channel's footprints must not make that channel's own nets look
 * global.  Channels 1 and 2 genuinely differ on a local net, so the match must still fail even with
 * a second area covering channel 1.
 */
BOOST_FIXTURE_TEST_CASE( CheckRACompatOverlappingAreaKeepsLocalNet, MULTICHANNEL_TEST_FIXTURE )
{
    m_board = std::make_unique<BOARD>();
    m_board->SetEnabledLayers( LSET::AllCuMask() | LSET::AllTechMask() );

    auto addNet = [&]( const wxString& aName ) -> int
    {
        NETINFO_ITEM* net = new NETINFO_ITEM( m_board.get(), aName );
        m_board->Add( net );
        return net->GetNetCode();
    };

    const int netGnd = addNet( wxT( "GND" ) );

    const LIB_ID icId( wxT( "TestLib" ), wxT( "SOT-23-6" ) );
    const LIB_ID rId( wxT( "TestLib" ), wxT( "R_0402" ) );

    auto addPad = [&]( FOOTPRINT* aFp, const wxString& aNumber, int aNetCode )
    {
        PAD* pad = new PAD( aFp );
        pad->SetNumber( aNumber );
        pad->SetShape( PADSTACK::ALL_LAYERS, PAD_SHAPE::CIRCLE );
        pad->SetSize( PADSTACK::ALL_LAYERS, VECTOR2I( pcbIUScale.mmToIU( 1 ), pcbIUScale.mmToIU( 1 ) ) );
        pad->SetLayerSet( LSET( { F_Cu } ) );
        pad->SetNetCode( aNetCode );
        aFp->Add( pad );
    };

    auto makeFp = [&]( const LIB_ID& aFpId, const wxString& aRef ) -> FOOTPRINT*
    {
        FOOTPRINT* fp = new FOOTPRINT( m_board.get() );
        fp->SetFPID( aFpId );
        fp->SetReference( aRef );
        m_board->Add( fp );
        return fp;
    };

    // Channel 1: U1 pad 1 and R1 pad 1 share a per-channel net, giving that net two internal pads.
    const int netLocal1 = addNet( wxT( "Net-(U1-SIG)" ) );
    FOOTPRINT* u1 = makeFp( icId, wxT( "U1" ) );
    addPad( u1, wxT( "1" ), netLocal1 );
    addPad( u1, wxT( "2" ), netGnd );
    FOOTPRINT* r1 = makeFp( rId, wxT( "R1" ) );
    addPad( r1, wxT( "1" ), netLocal1 );
    addPad( r1, wxT( "2" ), netGnd );

    // Channel 2: U2 pad 1 is on its own net, so R2 does not share it -- a real difference from
    // channel 1 that must be reported as a mismatch.
    const int netLocal2a = addNet( wxT( "Net-(U2-SIG)" ) );
    const int netLocal2b = addNet( wxT( "Net-(R2-SIG)" ) );
    FOOTPRINT* u2 = makeFp( icId, wxT( "U2" ) );
    addPad( u2, wxT( "1" ), netLocal2a );
    addPad( u2, wxT( "2" ), netGnd );
    FOOTPRINT* r2 = makeFp( rId, wxT( "R2" ) );
    addPad( r2, wxT( "1" ), netLocal2b );
    addPad( r2, wxT( "2" ), netGnd );

    ZONE* zone1 = new ZONE( m_board.get() );
    m_board->Add( zone1 );
    ZONE* zone2 = new ZONE( m_board.get() );
    m_board->Add( zone2 );
    ZONE* zoneDup = new ZONE( m_board.get() );
    m_board->Add( zoneDup );

    TOOL_MANAGER       toolMgr;
    MOCK_TOOLS_HOLDER* toolsHolder = new MOCK_TOOLS_HOLDER;
    toolMgr.SetEnvironment( m_board.get(), nullptr, nullptr, nullptr, toolsHolder );

    MULTICHANNEL_TOOL* mtTool = new MULTICHANNEL_TOOL;
    toolMgr.RegisterTool( mtTool );

    RULE_AREAS_DATA* ruleData = mtTool->GetData();
    ruleData->m_areas.reserve( 3 );

    RULE_AREA ra1;
    ra1.m_zone = zone1;
    ra1.m_ruleName = wxT( "Channel 1" );
    ra1.m_components = { u1, r1 };
    ruleData->m_areas.push_back( ra1 );

    RULE_AREA ra2;
    ra2.m_zone = zone2;
    ra2.m_ruleName = wxT( "Channel 2" );
    ra2.m_components = { u2, r2 };
    ruleData->m_areas.push_back( ra2 );

    // A second rule area over channel 1's footprints.  Counting it as a distinct channel would make
    // Net-(U1-SIG) look global (two areas carry it) and hide the difference from channel 2.
    RULE_AREA raDup;
    raDup.m_zone = zoneDup;
    raDup.m_ruleName = wxT( "Channel 1 duplicate" );
    raDup.m_components = { u1, r1 };
    ruleData->m_areas.push_back( raDup );

    int status = mtTool->CheckRACompatibility( zone1 );
    BOOST_REQUIRE_EQUAL( status, 0 );

    RULE_AREA* channel2 = nullptr;

    for( RULE_AREA& ra : ruleData->m_areas )
    {
        if( ra.m_zone == zone2 )
            channel2 = &ra;
    }

    BOOST_REQUIRE( channel2 != nullptr );

    auto it = ruleData->m_compatMap.find( channel2 );
    BOOST_REQUIRE( it != ruleData->m_compatMap.end() );

    BOOST_CHECK_MESSAGE( !it->second.m_isOk,
                         "Channel 1 and channel 2 differ on a local net and must not match; an "
                         "overlapping rule area must not hide that net by marking it global" );
}


BOOST_AUTO_TEST_SUITE_END()
