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
#include <pcb_track.h>
#include <footprint.h>
#include <zone.h>
#include <drc/drc_item.h>
#include <settings/settings_manager.h>
#include <tools/multichannel_tool.h>
#include <connectivity/topo_match.h>
#include <lib_id.h>

struct MULTICHANNEL_TEST_FIXTURE
{
    MULTICHANNEL_TEST_FIXTURE() : m_settingsManager( true /* headless */ ) {}

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

        mtTool->QuerySheetsAndComponentClasses();

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
                BOOST_CHECK_EQUAL( ra.m_raFootprints.size(), 31 );
            }
            else if( ra.m_ruleName.Contains( wxT( "io_drivers_pp" ) ) )
            {
                n_areas_pp++;
                BOOST_CHECK_EQUAL( ra.m_raFootprints.size(), 11 );
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
                    if( targetArea.m_area == refArea.m_area )
                        continue;

                    if( !targetArea.m_ruleName.Contains( ruleName ) )
                        continue;

                    auto cgRef = CONNECTION_GRAPH::BuildFromFootprintSet( refArea.m_raFootprints );
                    auto cgTarget =
                        CONNECTION_GRAPH::BuildFromFootprintSet( targetArea.m_raFootprints );

                    TMATCH::COMPONENT_MATCHES result;

                    CONNECTION_GRAPH::STATUS status =
                        cgRef->FindIsomorphism( cgTarget.get(), result );

                    BOOST_TEST_MESSAGE( wxString::Format(
                            "topo match: '%s' [%d] -> '%s' [%d] result %d",
                            refArea.m_ruleName.c_str().AsChar(),
                            static_cast<int>( refArea.m_raFootprints.size() ),
                            targetArea.m_ruleName.c_str().AsChar(),
                            static_cast<int>( targetArea.m_raFootprints.size() ), status ) );

                    for( const auto& iter : result )
                    {
                        BOOST_TEST_MESSAGE( wxString::Format( "%s : %s",
                                                              iter.second->GetReference(),
                                                              iter.first->GetReference() ) );
                    }

                    BOOST_CHECK_EQUAL( status, TMATCH::CONNECTION_GRAPH::ST_OK );
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

        int result = mtTool->RepeatLayout( TOOL_EVENT(), refArea->m_area );

        BOOST_ASSERT( result >= 0 );
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

            if( refArea->m_area->Outline()->Contains( viaPos ) )
                refViaCountBefore++;
        }
    }

    BOOST_TEST_MESSAGE( wxString::Format( "Reference area vias before repeat: %d", refViaCountBefore ) );
    BOOST_CHECK( refViaCountBefore > 0 );

    mtTool->CheckRACompatibility( refArea->m_area );

    ruleData->m_compatMap[targetArea].m_doCopy = true;
    ruleData->m_options.m_copyPlacement = true;
    ruleData->m_options.m_copyRouting = true;

    int result = mtTool->RepeatLayout( TOOL_EVENT(), refArea->m_area );

    BOOST_CHECK( result >= 0 );

    int refViaCountAfter = 0;

    for( PCB_TRACK* track : m_board->Tracks() )
    {
        if( track->Type() == PCB_VIA_T )
        {
            PCB_VIA* via = static_cast<PCB_VIA*>( track );
            VECTOR2I viaPos = via->GetPosition();

            if( refArea->m_area->Outline()->Contains( viaPos ) )
                refViaCountAfter++;
        }
    }

    BOOST_TEST_MESSAGE( wxString::Format( "Reference area vias after repeat: %d", refViaCountAfter ) );

    BOOST_CHECK_EQUAL( refViaCountAfter, refViaCountBefore );
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
    auto status = cgRef->FindIsomorphism( cgTarget.get(), result );

    BOOST_TEST_MESSAGE( wxString::Format( "Topology match result: %d", static_cast<int>( status ) ) );
    BOOST_CHECK_EQUAL( status, CONNECTION_GRAPH::ST_OK );

    // Cleanup: remove pads before footprints go out of scope
    fpRef.Pads().clear();
    fpTarget.Pads().clear();
}


BOOST_AUTO_TEST_SUITE_END()
