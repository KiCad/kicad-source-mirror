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
 * @file test_drc_creepage_issue23578.cpp
 *
 * Regression test for issue #23578: creepage DRC must correctly model rounded
 * rectangle slots on Edge.Cuts.
 *
 * The board contains a gr_rect with radius=0.5mm (a fully-rounded stadium slot)
 * on Edge.Cuts, with Sitove-class copper traces within 5mm creepage distance
 * measured around the slot boundary. Before the fix, TransformEdgeToCreepShapes
 * ignored the corner radius and placed BE_SHAPE_POINT nodes at the raw rectangle
 * corners instead of BE_SHAPE_ARC nodes at the true rounded corners. This caused
 * the DRC to report no violation (false negative) for the fully-rounded case.
 */

#include <qa_utils/wx_utils/unit_test_utils.h>
#include <pcbnew_utils/board_test_utils.h>

#include <board.h>
#include <board_design_settings.h>
#include <drc/drc_item.h>
#include <drc/drc_engine.h>
#include <settings/settings_manager.h>
#include <widgets/report_severity.h>


struct DRC_CREEPAGE_ROUNDRECT_FIXTURE
{
    DRC_CREEPAGE_ROUNDRECT_FIXTURE() = default;

    ~DRC_CREEPAGE_ROUNDRECT_FIXTURE()
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


BOOST_FIXTURE_TEST_CASE( CreepageRoundedRectSlotIssue23578, DRC_CREEPAGE_ROUNDRECT_FIXTURE )
{
    KI_TEST::LoadBoard( m_settingsManager, "issue23578/issue23578", m_board );

    BOOST_REQUIRE_MESSAGE( m_board, "Failed to load board issue23578" );

    std::vector<std::shared_ptr<DRC_ITEM>> violations;
    BOARD_DESIGN_SETTINGS&                 bds = m_board->GetDesignSettings();

    BOOST_REQUIRE_MESSAGE( bds.m_DRCEngine, "DRC engine not initialized" );

    for( int ii = DRCE_FIRST; ii <= DRCE_LAST; ++ii )
        bds.m_DRCSeverities[ii] = SEVERITY::RPT_SEVERITY_IGNORE;

    bds.m_DRCSeverities[DRCE_CREEPAGE] = SEVERITY::RPT_SEVERITY_ERROR;

    bds.m_DRCEngine->SetViolationHandler(
            [&]( const std::shared_ptr<DRC_ITEM>& aItem, const VECTOR2I& aPos, int aLayer,
                 const std::function<void( PCB_MARKER* )>& aPathGenerator )
            {
                if( bds.GetSeverity( aItem->GetErrorCode() ) == SEVERITY::RPT_SEVERITY_ERROR )
                    violations.push_back( aItem );
            } );

    bds.m_DRCEngine->RunTests( EDA_UNITS::MM, true, false );

    bds.m_DRCEngine->ClearViolationHandler();

    BOOST_TEST_MESSAGE( wxString::Format( "Found %d creepage violations", (int) violations.size() ) );

    for( const auto& v : violations )
        BOOST_TEST_MESSAGE( wxString::Format( "  Violation: %s", v->GetErrorMessage( false ) ) );

    // The board has a rounded rectangle slot (gr_rect with radius=0.5mm, effectively a
    // stadium shape) on Edge.Cuts, with Sitove-class traces within 5mm creepage distance
    // of each other measured around the slot boundary. Without the fix, the DRC reported
    // zero violations because rounded corners were not modeled. With the fix, at least
    // one violation must be detected.
    BOOST_CHECK_GE( violations.size(), 1u );
}
