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

/**
 * @file test_drc_creepage_issue24853.cpp
 *
 * Issue #24853, creepage ignored positional rule conditions.
 *
 * Repro board: tracks 'a' and 'b' both cross rule area 'forcreepage'. Rule 'creep' matches
 * in-area, complementary rule 'creep2' matches out-of-area, both 25mm. Placeholders at the
 * origin made intersectsArea() false, so 'creep2' won. Discriminator is the rule name.
 */

#include <qa_utils/wx_utils/unit_test_utils.h>
#include <pcbnew_utils/board_test_utils.h>

#include <board.h>
#include <board_design_settings.h>
#include <layer_ids.h>
#include <netinfo.h>
#include <drc/drc_item.h>
#include <drc/drc_rule.h>
#include <drc/drc_engine.h>
#include <settings/settings_manager.h>
#include <widgets/report_severity.h>


struct DRC_CREEPAGE_RULE_AREA_FIXTURE
{
    DRC_CREEPAGE_RULE_AREA_FIXTURE() = default;

    ~DRC_CREEPAGE_RULE_AREA_FIXTURE()
    {
        if( m_board && m_board->GetDesignSettings().m_DRCEngine )
            m_board->GetDesignSettings().m_DRCEngine->ClearViolationHandler();

        if( m_board )
        {
            m_board->SetProject( nullptr );
            m_board = nullptr;
        }
    }

    SETTINGS_MANAGER       m_settingsManager;
    std::unique_ptr<BOARD> m_board;
};


BOOST_FIXTURE_TEST_CASE( CreepageRuleAreaConditionIssue24853, DRC_CREEPAGE_RULE_AREA_FIXTURE )
{
    KI_TEST::LoadBoard( m_settingsManager, "issue24853/issue24853", m_board );

    BOOST_REQUIRE_MESSAGE( m_board, "Failed to load board issue24853" );

    NETINFO_ITEM* netA = m_board->FindNet( wxT( "a" ) );
    NETINFO_ITEM* netB = m_board->FindNet( wxT( "b" ) );

    BOOST_REQUIRE_MESSAGE( netA && netB, "Nets 'a' and 'b' not found in board" );

    struct ViolationInfo
    {
        wxString rule;
        int      layer = 0;
    };

    std::vector<ViolationInfo> creepageViolations;
    BOARD_DESIGN_SETTINGS&     bds = m_board->GetDesignSettings();

    BOOST_REQUIRE_MESSAGE( bds.m_DRCEngine, "DRC engine not initialized" );

    for( int ii = DRCE_FIRST; ii <= DRCE_LAST; ++ii )
        bds.m_DRCSeverities[ii] = SEVERITY::RPT_SEVERITY_IGNORE;

    bds.m_DRCSeverities[DRCE_CREEPAGE] = SEVERITY::RPT_SEVERITY_ERROR;

    bds.m_DRCEngine->SetViolationHandler(
            [&]( const std::shared_ptr<DRC_ITEM>& aItem, const VECTOR2I& /* aPos */, int aLayer,
                 const std::function<void( PCB_MARKER* )>& /* aPathGenerator */ )
            {
                if( aItem->GetErrorCode() != DRCE_CREEPAGE )
                    return;

                ViolationInfo vi;
                vi.layer = aLayer;

                if( DRC_RULE* rule = aItem->GetViolatingRule() )
                    vi.rule = rule->m_Name;

                creepageViolations.push_back( vi );
            } );

    bds.m_DRCEngine->RunTests( EDA_UNITS::MM, true, false );

    bds.m_DRCEngine->ClearViolationHandler();

    BOOST_TEST_MESSAGE( wxString::Format( "Found %d creepage violations",
                                          (int) creepageViolations.size() ) );

    for( const ViolationInfo& vi : creepageViolations )
        BOOST_TEST_MESSAGE( wxString::Format( "  layer=%d rule=%s", vi.layer, vi.rule ) );

    BOOST_REQUIRE_MESSAGE( !creepageViolations.empty(),
                           "Expected a creepage violation between the two tracks inside the area" );

    // Both conductors cross the 'forcreepage' area, so every creepage violation between them
    // must resolve from the in-area rule 'creep'. The complementary 'creep2' rule requires both
    // conductors to be outside the area and must never fire here. Attribution to 'creep2' is the
    // exact #24853 failure mode where positional conditions were evaluated against origin
    // placeholders.
    for( const ViolationInfo& vi : creepageViolations )
    {
        BOOST_CHECK_MESSAGE( vi.rule == wxT( "creep" ),
                wxString::Format( "Creepage violation resolved from rule '%s'; the conductors are "
                                  "inside 'forcreepage' so it must resolve from 'creep'. Rule-area "
                                  "conditions are being ignored for creepage.",
                                  vi.rule ) );
    }
}
