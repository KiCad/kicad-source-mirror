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
 * @file test_drc_creepage_issue24597.cpp
 *
 * Regression test for issue #24597: creepage path cut across a rounded Edge.Cuts slot.
 *
 * A rounded rectangle slot is a single PCB_SHAPE, so its four corner arcs and
 * four straight edges share one parent. When a candidate path ended on one corner
 * arc, GeneratePaths used to exempt that whole parent from the board-edge crossing
 * test, which let a copper-to-far-corner chord cut straight across the slot's near
 * edge. The path then hugged only one arc instead of wrapping the rounded end, and
 * the reported creepage came out too short (and differed front vs back).
 *
 * With the fix the path wraps both corner arcs of the end cap, so the reported
 * creepage grows from ~2.68mm (corner-cut) to ~2.82mm and matches on both layers.
 */

#include <regex>

#include <qa_utils/wx_utils/unit_test_utils.h>
#include <pcbnew_utils/board_test_utils.h>

#include <board.h>
#include <board_design_settings.h>
#include <drc/drc_item.h>
#include <drc/drc_engine.h>
#include <settings/settings_manager.h>
#include <widgets/report_severity.h>


struct DRC_CREEPAGE_ISSUE24597_FIXTURE
{
    DRC_CREEPAGE_ISSUE24597_FIXTURE() = default;

    ~DRC_CREEPAGE_ISSUE24597_FIXTURE()
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


BOOST_FIXTURE_TEST_CASE( CreepageSlotCornerIssue24597, DRC_CREEPAGE_ISSUE24597_FIXTURE )
{
    KI_TEST::LoadBoard( m_settingsManager, "issue24597/issue24597", m_board );

    BOOST_REQUIRE_MESSAGE( m_board, "Failed to load board issue24597" );

    std::vector<std::shared_ptr<DRC_ITEM>> violations;
    BOARD_DESIGN_SETTINGS&                 bds = m_board->GetDesignSettings();

    BOOST_REQUIRE_MESSAGE( bds.m_DRCEngine, "DRC engine not initialized" );

    for( int ii = DRCE_FIRST; ii <= DRCE_LAST; ++ii )
        bds.m_DRCSeverities[ii] = SEVERITY::RPT_SEVERITY_IGNORE;

    bds.m_DRCSeverities[DRCE_CREEPAGE] = SEVERITY::RPT_SEVERITY_ERROR;

    bds.m_DRCEngine->SetViolationHandler(
            [&]( const std::shared_ptr<DRC_ITEM>& aItem, const VECTOR2I&, int,
                 const std::function<void( PCB_MARKER* )>& )
            {
                if( bds.GetSeverity( aItem->GetErrorCode() ) == SEVERITY::RPT_SEVERITY_ERROR )
                    violations.push_back( aItem );
            } );

    bds.m_DRCEngine->RunTests( EDA_UNITS::MM, true, false );
    bds.m_DRCEngine->ClearViolationHandler();

    BOOST_REQUIRE_MESSAGE( !violations.empty(), "Expected a creepage violation around the slot" );

    // Pull the reported "actual" creepage out of each violation message. The corner-cutting
    // bug measured <= 2.76mm on this board, the correct path around both corner arcs is
    // ~2.82mm. Any value below the wrapped length means the path shortcut across the slot.
    std::regex actualRe( R"(actual\s+([0-9]+\.[0-9]+)\s*mm)" );

    for( const std::shared_ptr<DRC_ITEM>& v : violations )
    {
        std::string msg = v->GetErrorMessage( false ).ToStdString();
        std::smatch m;

        BOOST_REQUIRE_MESSAGE( std::regex_search( msg, m, actualRe ),
                               "Could not parse creepage distance from: " << msg );

        double actual = std::stod( m[1].str() );
        BOOST_TEST_MESSAGE( wxString::Format( "creepage actual = %.4f mm", actual ) );

        BOOST_CHECK_MESSAGE( actual > 2.78, "Creepage path cut across the slot (actual "
                                                    << actual << " mm <= 2.78 mm); it should wrap both corner arcs" );
    }
}
