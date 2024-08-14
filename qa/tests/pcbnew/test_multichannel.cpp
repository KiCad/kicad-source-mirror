/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2021 KiCad Developers, see AUTHORS.txt for contributors.
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

struct MULTICHANNEL_TEST_FIXTURE
{
    MULTICHANNEL_TEST_FIXTURE() :
            m_settingsManager( true /* headless */ )
    { }

    SETTINGS_MANAGER       m_settingsManager;
    std::unique_ptr<BOARD> m_board;
};

class MOCK_TOOLS_HOLDER : public TOOLS_HOLDER
{
public:
    MOCK_TOOLS_HOLDER() {};
    ~MOCK_TOOLS_HOLDER() {};

    virtual wxWindow* GetToolCanvas() const override { return nullptr; }
};

BOOST_FIXTURE_TEST_SUITE( MultichannelTool, MULTICHANNEL_TEST_FIXTURE )

RULE_AREA* findRuleAreaByPartialName( MULTICHANNEL_TOOL* aTool, const wxString& aName )
{
    auto ruleData = aTool->GetData();

    for( auto& ra : ruleData->m_areas )
    {
        if( ra.m_ruleName.Contains(( aName ) ) )
            return &ra;
    }

    return nullptr;
}


BOOST_FIXTURE_TEST_CASE( MultichannelToolRegressions, MULTICHANNEL_TEST_FIXTURE )
{
    std::vector<wxString> tests = {
                                    "vme-wren"
                                };

    for( const wxString& relPath : tests )
    {
        KI_TEST::LoadBoard( m_settingsManager, relPath, m_board );

        BOARD_DESIGN_SETTINGS& bds = m_board->GetDesignSettings();

        TOOL_MANAGER toolMgr;
        MOCK_TOOLS_HOLDER *toolsHolder = new MOCK_TOOLS_HOLDER;

        toolMgr.SetEnvironment( m_board.get(), nullptr, nullptr, nullptr, toolsHolder );

        MULTICHANNEL_TOOL *mtTool = new MULTICHANNEL_TOOL; // TOOL_MANAGER owns the tools
        toolMgr.RegisterTool( mtTool );

        //RULE_AREAS_DATA* raData = m_parentTool->GetData();

        mtTool->QuerySheets();

        auto ruleData = mtTool->GetData();

        printf("RA multichannel sheets = %d\n", (int) ruleData->m_areas.size() );

        BOOST_CHECK_EQUAL( ruleData->m_areas.size(), 72 );

        int cnt = 0;

        ruleData->m_replaceExisting = true;
        for( auto& ra : ruleData->m_areas )
        {
            if ( ra.m_sheetName == wxT("io_driver.kicad_sch") || ra.m_sheetName == wxT("pp_driver_2x.kicad_sch") )
            {
                ra.m_generateEnabled = true;
                cnt++;
            }
        }

        printf("Autogenerating %d RAs\n", cnt );

        TOOL_EVENT dummyEvent;

        mtTool->AutogenerateRuleAreas( dummyEvent );
        mtTool->FindExistingRuleAreas();

        int n_areas_io = 0, n_areas_pp = 0, n_areas_other = 0;


        printf("Found %d RAs after commit\n", (int) ruleData->m_areas.size() );

        for( auto ra : ruleData->m_areas )
        {
            printf("SN '%s'\n", ra.m_ruleName.c_str().AsChar() );
            if( ra.m_ruleName.Contains( wxT("io_drivers_fp") ) )
            {
                n_areas_io++;
                BOOST_CHECK_EQUAL( ra.m_raFootprints.size(), 31 );
            }
            else if( ra.m_ruleName.Contains( wxT("io_drivers_pp") ) )
            {
                n_areas_pp++;
                BOOST_CHECK_EQUAL( ra.m_raFootprints.size(), 11 );
            }
            else
            {
                n_areas_other++;
            }
        }

        printf("IO areas=%d, PP areas=%d, others=%d\n", n_areas_io, n_areas_pp, n_areas_other );

        BOOST_CHECK_EQUAL( n_areas_io, 16 );
        BOOST_CHECK_EQUAL( n_areas_pp, 16 );
        BOOST_CHECK_EQUAL( n_areas_other, 0 );

        const std::vector<wxString> rulesToTest = { wxT("io_drivers_fp" ), wxT("io_drivers_pp" ) };

        for( auto ruleName : rulesToTest )
        {
            for( auto refArea : ruleData->m_areas )
            {
                if( !refArea.m_ruleName.Contains( ruleName ) )
                    continue;

                printf("REF AREA: '%s'\n", refArea.m_ruleName.c_str().AsChar() );

                for ( auto targetArea : ruleData->m_areas )
                {
                    if( targetArea.m_area == refArea.m_area )
                        continue;

                    if( !targetArea.m_ruleName.Contains( ruleName ) )
                        continue;

                    auto cgRef = TMATCH::CONNECTION_GRAPH::BuildFromFootprintSet( refArea.m_raFootprints );
                    auto cgTarget = TMATCH::CONNECTION_GRAPH::BuildFromFootprintSet( targetArea.m_raFootprints );

                    TMATCH::COMPONENT_MATCHES result;

                    auto status = cgRef->FindIsomorphism( cgTarget.get(), result );
                    printf( "topo match: '%s' [%d] -> '%s' [%d] result %d\n",
                            refArea.m_ruleName.c_str().AsChar(),
                            (int) refArea.m_raFootprints.size(),
                            targetArea.m_ruleName.c_str().AsChar(),
                            (int) targetArea.m_raFootprints.size(), status );

                    for( const auto& iter : result )
                    {
                        printf("%s : %s\n", iter.second->GetReference().c_str().AsChar(),
                         iter.first->GetReference().c_str().AsChar()
                         );
                    }

                    BOOST_CHECK_EQUAL( status, TMATCH::CONNECTION_GRAPH::ST_OK );
                }
            }
        }

        auto refArea = findRuleAreaByPartialName( mtTool, wxT("io_drivers_fp/bank3/io78/") );

        BOOST_ASSERT( refArea );

        const std::vector<wxString> targetAreaNames ( { wxT("io_drivers_fp/bank2/io78/"), wxT("io_drivers_fp/bank1/io78/"), wxT("io_drivers_fp/bank0/io01/") } );

        for( wxString targetRaName : targetAreaNames )
        {
            auto targetRA = findRuleAreaByPartialName( mtTool, targetRaName );

            BOOST_ASSERT( targetRA != nullptr );

            printf("Clone to: %s\n", targetRA->m_ruleName.c_str().AsChar() );

            ruleData->m_compatMap[ targetRA ].m_doCopy = true;
        }

        int result = mtTool->RepeatLayout( TOOL_EVENT(), refArea->m_area );

        BOOST_ASSERT( result >= 0 );
    }
}

BOOST_AUTO_TEST_SUITE_END()
