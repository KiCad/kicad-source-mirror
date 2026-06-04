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
 * @file test_drc_creepage_issue24523.cpp
 *
 * Regression test for issue #24523: creepage DRC fails to find the shortest
 * surface path when it has to wind around two parallel NPTH slots.
 *
 * The repro board has two capacitors (C4, C5), each with a 10 mm x 1 mm NPTH
 * oval slot, placed side by side. Two nets ('Sitove' netclass, 5 mm creepage
 * rule) snake around both slots. The true creepage path between the nets wraps
 * around the slot end caps and threads the gap between the two slots, measuring
 * roughly 3 mm - well below the 5 mm requirement. Before the fix the path
 * search could not assemble this multi-leg route, so it overestimated the
 * creepage distance and reported no violation (a false negative).
 */

#include <qa_utils/wx_utils/unit_test_utils.h>
#include <pcbnew_utils/board_test_utils.h>

#include <board.h>
#include <board_design_settings.h>
#include <layer_ids.h>
#include <drc/drc_item.h>
#include <drc/drc_engine.h>
#include <footprint.h>
#include <pad.h>
#include <pcb_marker.h>
#include <settings/settings_manager.h>
#include <widgets/report_severity.h>


struct DRC_CREEPAGE_TWO_SLOTS_FIXTURE
{
    DRC_CREEPAGE_TWO_SLOTS_FIXTURE() = default;

    ~DRC_CREEPAGE_TWO_SLOTS_FIXTURE()
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


BOOST_FIXTURE_TEST_CASE( CreepageTwoNPTHSlotsIssue24523, DRC_CREEPAGE_TWO_SLOTS_FIXTURE )
{
    KI_TEST::LoadBoard( m_settingsManager, "issue24523/issue24523", m_board );

    BOOST_REQUIRE_MESSAGE( m_board, "Failed to load board issue24523" );

    struct ViolationInfo
    {
        std::shared_ptr<DRC_ITEM> item;
        VECTOR2I                  pos;
        std::vector<PCB_SHAPE>    pathShapes;
        int                       layer = 0;
        double                    reportedActual = -1.0;
    };

    std::vector<ViolationInfo> violations;
    BOARD_DESIGN_SETTINGS&     bds = m_board->GetDesignSettings();

    BOOST_REQUIRE_MESSAGE( bds.m_DRCEngine, "DRC engine not initialized" );

    for( int ii = DRCE_FIRST; ii <= DRCE_LAST; ++ii )
        bds.m_DRCSeverities[ii] = SEVERITY::RPT_SEVERITY_IGNORE;

    bds.m_DRCSeverities[DRCE_CREEPAGE] = SEVERITY::RPT_SEVERITY_ERROR;

    auto parseActual =
            []( const wxString& aMsg ) -> double
            {
                int actualPos = aMsg.Find( wxT( "actual " ) );

                if( actualPos == wxNOT_FOUND )
                    return -1.0;

                wxString tail = aMsg.Mid( actualPos + 7 );
                int      spacePos = tail.Find( ' ' );

                if( spacePos != wxNOT_FOUND )
                    tail = tail.Left( spacePos );

                double value = -1.0;
                tail.ToDouble( &value );
                return value;
            };

    bds.m_DRCEngine->SetViolationHandler(
            [&]( const std::shared_ptr<DRC_ITEM>& aItem, const VECTOR2I& aPos, int aLayer,
                 const std::function<void( PCB_MARKER* )>& aPathGenerator )
            {
                if( bds.GetSeverity( aItem->GetErrorCode() ) != SEVERITY::RPT_SEVERITY_ERROR )
                    return;

                ViolationInfo vi;
                vi.item = aItem;
                vi.pos = aPos;
                vi.layer = aLayer;
                vi.reportedActual = parseActual( aItem->GetErrorMessage( false ) );

                if( aPathGenerator )
                {
                    PCB_MARKER marker( aItem, aPos, aLayer );
                    aPathGenerator( &marker );
                    vi.pathShapes = marker.GetPath();
                }

                violations.push_back( vi );
            } );

    bds.m_DRCEngine->RunTests( EDA_UNITS::MM, true, false );

    bds.m_DRCEngine->ClearViolationHandler();

    BOOST_TEST_MESSAGE( wxString::Format( "Found %d creepage violations",
                                          (int) violations.size() ) );

    double shortestActual = std::numeric_limits<double>::max();

    for( const ViolationInfo& vi : violations )
    {
        BOOST_TEST_MESSAGE( wxString::Format( "  layer=%d arrow=(%.4f,%.4f) shapes=%d actual=%.4f",
                                              vi.layer, vi.pos.x / 1e6, vi.pos.y / 1e6,
                                              (int) vi.pathShapes.size(), vi.reportedActual ) );

        if( vi.reportedActual >= 0.0 )
            shortestActual = std::min( shortestActual, vi.reportedActual );
    }

    // The two nets violate the 5 mm rule because the true surface path that winds
    // around the two NPTH slots is only ~3 mm. The path search must find it.
    BOOST_REQUIRE_MESSAGE( !violations.empty(),
            "No creepage violation reported; the path search failed to find the ~3 mm route "
            "winding around the two NPTH slots and overestimated the creepage distance." );

    BOOST_TEST_MESSAGE( wxString::Format( "Shortest reported creepage actual: %.4f mm",
                                          shortestActual ) );

    // A violation is only emitted when the computed distance is below the 5 mm rule, so any
    // reported actual is already < 5 mm. Guard against a future regression that lets a grossly
    // overestimated (but still sub-5 mm) path through: the real path is ~3 mm.
    BOOST_CHECK_MESSAGE( shortestActual < 4.0,
            wxString::Format( "Shortest reported creepage %.4f mm is far longer than the true "
                              "~3 mm winding path, indicating the search is still missing the "
                              "shortest route around the two slots.", shortestActual ) );
}
