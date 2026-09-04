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
 * @file test_drc_creepage_issue25439.cpp
 *
 * Regression test for issue #25439: a 3 mm NPTH between two tracks shorter than the hole
 * diameter reported no creepage violation at all, because the segment to circle path builder
 * had no branch for a hole projecting past both track ends.
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


struct DRC_CREEPAGE_ISSUE25439_FIXTURE
{
    DRC_CREEPAGE_ISSUE25439_FIXTURE() = default;

    ~DRC_CREEPAGE_ISSUE25439_FIXTURE()
    {
        if( m_board && m_board->GetDesignSettings().m_DRCEngine )
            m_board->GetDesignSettings().m_DRCEngine->ClearViolationHandler();

        if( m_board )
        {
            m_board->SetProject( nullptr );
            m_board = nullptr;
        }
    }

    // Run creepage DRC and return the reported "actual" distances in mm.
    std::vector<double> runCreepage( const std::string& aRelPath )
    {
        KI_TEST::LoadBoard( m_settingsManager, aRelPath, m_board );
        BOOST_REQUIRE_MESSAGE( m_board, "Failed to load board " << aRelPath );

        BOARD_DESIGN_SETTINGS& bds = m_board->GetDesignSettings();
        BOOST_REQUIRE_MESSAGE( bds.m_DRCEngine, "DRC engine not initialized" );

        for( int ii = DRCE_FIRST; ii <= DRCE_LAST; ++ii )
            bds.m_DRCSeverities[ii] = SEVERITY::RPT_SEVERITY_IGNORE;

        bds.m_DRCSeverities[DRCE_CREEPAGE] = SEVERITY::RPT_SEVERITY_ERROR;

        std::vector<std::shared_ptr<DRC_ITEM>> violations;

        bds.m_DRCEngine->SetViolationHandler(
                [&]( const std::shared_ptr<DRC_ITEM>& aItem, const VECTOR2I&, int,
                     const std::function<void( PCB_MARKER* )>& )
                {
                    if( bds.GetSeverity( aItem->GetErrorCode() ) == SEVERITY::RPT_SEVERITY_ERROR )
                        violations.push_back( aItem );
                } );

        bds.m_DRCEngine->RunTests( EDA_UNITS::MM, true, false );
        bds.m_DRCEngine->ClearViolationHandler();

        std::regex          actualRe( R"(actual\s+([0-9]+\.[0-9]+)\s*mm)" );
        std::vector<double> actuals;

        for( const std::shared_ptr<DRC_ITEM>& v : violations )
        {
            std::string msg = v->GetErrorMessage( false ).ToStdString();
            std::smatch m;

            if( std::regex_search( msg, m, actualRe ) )
                actuals.push_back( std::stod( m[1].str() ) );
        }

        return actuals;
    }

    SETTINGS_MANAGER       m_settingsManager;
    std::unique_ptr<BOARD> m_board;
};


// Two 0.5mm tracks 3.5mm apart, straddled by a 3mm NPTH that overhangs both track ends, under
// an 8mm creepage rule. Reported nothing at all; the path has to wrap the hole, so it is longer
// than the straight copper gap but well short of a detour around the board edge.
BOOST_FIXTURE_TEST_CASE( CreepageOversizedHoleIssue25439, DRC_CREEPAGE_ISSUE25439_FIXTURE )
{
    std::vector<double> actuals = runCreepage( "issue25439/issue25439" );

    BOOST_REQUIRE_MESSAGE( actuals.size() == 1,
                           "Expected one creepage violation around the NPTH, got "
                                   << actuals.size() );

    BOOST_TEST_MESSAGE( wxString::Format( "npth: actual = %.4f mm", actuals[0] ) );
    BOOST_CHECK_MESSAGE( actuals[0] > 3.5 && actuals[0] < 4.0,
                         "Creepage should wrap the NPTH (actual " << actuals[0] << " mm)" );
}
