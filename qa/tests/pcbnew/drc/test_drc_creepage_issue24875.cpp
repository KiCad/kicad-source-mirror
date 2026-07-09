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
 * @file test_drc_creepage_issue24875.cpp
 *
 * Regression tests for issue #24875: two rounded Edge.Cuts slots whose caps overlap merge
 * into one void. The creepage path must wrap the merged end, not hug the far slot's corner
 * arc where it bulges into the near slot's void (which gave a path far too short).
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


struct DRC_CREEPAGE_ISSUE24875_FIXTURE
{
    DRC_CREEPAGE_ISSUE24875_FIXTURE() = default;

    ~DRC_CREEPAGE_ISSUE24875_FIXTURE()
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


// Overlapping caps. Was a 1.93mm shortcut across the void, correct ~3.93mm around the end.
BOOST_FIXTURE_TEST_CASE( CreepageOverlappingCapsIssue24875, DRC_CREEPAGE_ISSUE24875_FIXTURE )
{
    std::vector<double> actuals = runCreepage( "issue24875/issue24875" );

    BOOST_REQUIRE_MESSAGE( !actuals.empty(), "Expected a creepage violation around the slots" );

    for( double actual : actuals )
    {
        BOOST_TEST_MESSAGE( wxString::Format( "partial: actual = %.4f mm", actual ) );
        BOOST_CHECK_MESSAGE( actual > 3.0 && actual < 4.0,
                             "Creepage should wrap the merged slot end (actual " << actual << " mm)" );
    }
}


// Fully rounded (semicircle) cap overlapping. Was a 2.75mm phantom hug, correct ~3.90mm.
// Needs the whole sub-arc sampled, an empty result would mean arcs were over-dropped.
BOOST_FIXTURE_TEST_CASE( CreepageFullyRoundedCapIssue24875, DRC_CREEPAGE_ISSUE24875_FIXTURE )
{
    std::vector<double> actuals = runCreepage( "issue24875_fullround/issue24875_fullround" );

    BOOST_REQUIRE_MESSAGE( !actuals.empty(), "Expected a creepage violation around the slots" );

    for( double actual : actuals )
    {
        BOOST_TEST_MESSAGE( wxString::Format( "fullround: actual = %.4f mm", actual ) );
        BOOST_CHECK_MESSAGE( actual > 3.0 && actual < 4.5,
                             "Creepage should wrap the merged slot end (actual " << actual << " mm)" );
    }
}


// Cap overlaps the other slot's straight section, never a false path (~2.31mm). Guards
// against the overlap detection over-triggering.
BOOST_FIXTURE_TEST_CASE( CreepageCapOverStraightIssue24875, DRC_CREEPAGE_ISSUE24875_FIXTURE )
{
    std::vector<double> actuals = runCreepage( "issue24875_capstraight/issue24875_capstraight" );

    BOOST_REQUIRE_MESSAGE( !actuals.empty(), "Expected a creepage violation around the slots" );

    for( double actual : actuals )
    {
        BOOST_TEST_MESSAGE( wxString::Format( "capstraight: actual = %.4f mm", actual ) );
        BOOST_CHECK_MESSAGE( actual > 2.0 && actual < 2.6,
                             "Cap-over-straight creepage should stay correct (actual " << actual << " mm)" );
    }
}
