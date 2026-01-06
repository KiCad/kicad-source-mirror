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
 * @file test_drc_creepage_issue21482.cpp
 * @brief Performance regression test for creepage DRC check.
 *
 * This test verifies that the creepage DRC check completes in a reasonable time
 * even with many copper elements that are not in the creepage checking area.
 *
 * Takes 11 seconds on my machine.  Hopefully faster on better machines.
 * Shooting for <15 seconds on boards with thousands of elements
 */

#include <qa_utils/wx_utils/unit_test_utils.h>
#include <pcbnew_utils/board_test_utils.h>

#include <board.h>
#include <board_design_settings.h>
#include <footprint.h>
#include <pcb_track.h>
#include <pcb_marker.h>
#include <drc/drc_item.h>
#include <drc/drc_engine.h>
#include <settings/settings_manager.h>
#include <widgets/report_severity.h>

#include <chrono>
#include <cstdlib>
#include <fstream>


struct DRC_CREEPAGE_PERF_TEST_FIXTURE
{
    DRC_CREEPAGE_PERF_TEST_FIXTURE()
    { }

    ~DRC_CREEPAGE_PERF_TEST_FIXTURE()
    {
        if( m_board && m_board->GetDesignSettings().m_DRCEngine )
        {
            m_board->GetDesignSettings().m_DRCEngine->ClearViolationHandler();
        }

        if( m_board )
        {
            m_board->SetProject( nullptr );
            m_board = nullptr;
        }
    }

    SETTINGS_MANAGER       m_settingsManager;
    std::unique_ptr<BOARD> m_board;
};


/**
 * Test that creepage DRC check completes in a reasonable time.
 *
 * This test uses a board with:
 * - Components with creepage constraints (BI/RI netclasses)
 * - Many vias/tracks that are NOT in the creepage checking area
 *
 * The creepage check should NOT scale linearly with the number of
 * unrelated copper elements.
 */
BOOST_FIXTURE_TEST_CASE( CreepagePerformanceIssue21482, DRC_CREEPAGE_PERF_TEST_FIXTURE )
{
    // Load the test board with custom creepage rules
    KI_TEST::LoadBoard( m_settingsManager, "issue21482/issue21482", m_board );

    BOOST_REQUIRE_MESSAGE( m_board, "Failed to load board issue21482" );

    // Count board elements for reporting
    int trackCount = m_board->Tracks().size();
    int padCount = 0;
    for( FOOTPRINT* fp : m_board->Footprints() )
        padCount += fp->Pads().size();

    BOOST_TEST_MESSAGE( wxString::Format( "Board has %d tracks and %d pads", trackCount, padCount ) );

    std::vector<DRC_ITEM>  violations;
    BOARD_DESIGN_SETTINGS& bds = m_board->GetDesignSettings();

    BOOST_REQUIRE_MESSAGE( bds.m_DRCEngine, "DRC engine not initialized" );

    // Disable all DRC tests except creepage
    for( int ii = DRCE_FIRST; ii <= DRCE_LAST; ++ii )
        bds.m_DRCSeverities[ii] = SEVERITY::RPT_SEVERITY_IGNORE;

    bds.m_DRCSeverities[DRCE_CREEPAGE] = SEVERITY::RPT_SEVERITY_ERROR;

    bds.m_DRCEngine->SetViolationHandler(
            [&]( const std::shared_ptr<DRC_ITEM>& aItem, const VECTOR2I& aPos, int aLayer,
                 const std::function<void( PCB_MARKER* )>& aPathGenerator )
            {
                if( bds.GetSeverity( aItem->GetErrorCode() ) == SEVERITY::RPT_SEVERITY_ERROR )
                    violations.push_back( *aItem );
            } );

    BOOST_TEST_CHECKPOINT( "Running creepage DRC" );

    // Time the DRC run
    auto start = std::chrono::high_resolution_clock::now();

    try
    {
        bds.m_DRCEngine->RunTests( EDA_UNITS::MM, true, false );
    }
    catch( const std::exception& e )
    {
        BOOST_ERROR( wxString::Format( "DRC creepage exception: %s", e.what() ) );
        return;
    }

    auto end = std::chrono::high_resolution_clock::now();
    double elapsedSeconds = std::chrono::duration<double>( end - start ).count();

    BOOST_TEST_MESSAGE( wxString::Format( "DRC creepage completed in %.2f seconds", elapsedSeconds ) );
    BOOST_TEST_MESSAGE( wxString::Format( "Found %d creepage violations", (int) violations.size() ) );

    // Clear the violation handler
    bds.m_DRCEngine->ClearViolationHandler();

    // Check if running on GitLab CI
    bool onGitLabCI = std::getenv( "GITLAB_CI" ) != nullptr;

    if( onGitLabCI )
    {
        // On GitLab CI, write metrics to a file for collection as an artifact.
        // The timeout varies too much between runners, so we only record the time
        // without asserting on it.
        std::ofstream metricsFile( "creepage_perf_metrics.txt" );

        if( metricsFile.is_open() )
        {
            metricsFile << "creepage_drc_seconds " << elapsedSeconds << "\n";
            metricsFile << "creepage_violations " << violations.size() << "\n";
            metricsFile.close();
            BOOST_TEST_MESSAGE( "Metrics written to creepage_perf_metrics.txt" );
        }
    }
    else
    {
        // On local machines, enforce the timeout constraint
        BOOST_CHECK_MESSAGE( elapsedSeconds < 20.0,
                             wxString::Format( "Creepage DRC too slow: %.2f seconds (target: <20s)",
                                               elapsedSeconds ) );
    }

    // Performance tier feedback
    if( elapsedSeconds < 5.0 )
    {
        BOOST_TEST_MESSAGE( "Excellent performance: <5 seconds" );
    }
    else if( elapsedSeconds < 10.0 )
    {
        BOOST_TEST_MESSAGE( "Good performance: <10 seconds" );
    }
    else if( elapsedSeconds < 15.0 )
    {
        BOOST_TEST_MESSAGE( "Acceptable performance: <15 seconds" );
    }
}
