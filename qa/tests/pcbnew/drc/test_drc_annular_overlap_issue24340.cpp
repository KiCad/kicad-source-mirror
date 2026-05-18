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
 * @file test_drc_annular_overlap_issue24340.cpp
 *
 * Regression test for issue #24340: false annular-width DRC violations on PTH
 * pads that partially overlap a same-number SMD pad.
 *
 * The bbox overlap forces the polygon path in DRC_TEST_PROVIDER_ANNULAR_WIDTH,
 * which polygonised the THT pad with ERROR_INSIDE. The inscribed-polygon
 * outline lies ~maxError (5 µm default) inside the true circle, so DRC
 * reported ~0.1484 mm and flagged a violation that does not exist in the
 * true geometry.
 *
 * Fixture (qa/data/pcbnew/issue24340/) -- two modified R_0805 footprints,
 * each with same-number THT + SMD overlap, sharing the same project rule
 * min_via_annular_width = 0.1524 mm:
 *
 *   Bug-case footprint at board (130, 95.5):
 *     THT circle size 0.508 mm, drill 0.2032 mm -- exact annular 0.1524 mm
 *     (equal to the constraint). Pre-fix: polygon path with ERROR_INSIDE
 *     under-reports the annular and falsely flags a violation. Post-fix:
 *     fast path is used, reports 0.1524 mm, passes.
 *
 *   Control footprint at board (176, 95.5):
 *     THT circle size 0.4 mm, drill 0.2 mm -- annular 0.1 mm, well below
 *     the 0.1524 mm constraint. A genuine violation that the fix must
 *     still report (guards against accidentally suppressing real failures).
 *
 * Expected: exactly 1 annular_width violation, on the control footprint.
 */

#include <qa_utils/wx_utils/unit_test_utils.h>
#include <pcbnew_utils/board_test_utils.h>

#include <board.h>
#include <board_design_settings.h>
#include <base_units.h>
#include <drc/drc_item.h>
#include <drc/drc_engine.h>
#include <settings/settings_manager.h>
#include <widgets/report_severity.h>


struct DRC_ANNULAR_OVERLAP_FIXTURE
{
    DRC_ANNULAR_OVERLAP_FIXTURE() = default;

    ~DRC_ANNULAR_OVERLAP_FIXTURE()
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


BOOST_FIXTURE_TEST_CASE( AnnularOverlapIssue24340, DRC_ANNULAR_OVERLAP_FIXTURE )
{
    KI_TEST::LoadBoard( m_settingsManager, "issue24340/issue24340", m_board );

    BOOST_REQUIRE_MESSAGE( m_board, "Failed to load board issue24340" );

    struct Violation
    {
        std::shared_ptr<DRC_ITEM> item;
        VECTOR2I                  pos;
    };

    std::vector<Violation> annularViolations;
    BOARD_DESIGN_SETTINGS& bds = m_board->GetDesignSettings();

    BOOST_REQUIRE_MESSAGE( bds.m_DRCEngine, "DRC engine not initialized" );

    for( int ii = DRCE_FIRST; ii <= DRCE_LAST; ++ii )
        bds.m_DRCSeverities[ii] = SEVERITY::RPT_SEVERITY_IGNORE;

    bds.m_DRCSeverities[DRCE_ANNULAR_WIDTH] = SEVERITY::RPT_SEVERITY_ERROR;

    bds.m_DRCEngine->SetViolationHandler(
            [&]( const std::shared_ptr<DRC_ITEM>& aItem, const VECTOR2I& aPos, int /*aLayer*/,
                 const std::function<void( PCB_MARKER* )>& /*aPathGen*/ )
            {
                if( aItem->GetErrorCode() == DRCE_ANNULAR_WIDTH )
                    annularViolations.push_back( { aItem, aPos } );
            } );

    bds.m_DRCEngine->RunTests( EDA_UNITS::MM, true, false );

    bds.m_DRCEngine->ClearViolationHandler();

    BOOST_TEST_MESSAGE( wxString::Format( "Annular violations: %d", (int) annularViolations.size() ) );

    // Bug-case THT board position: footprint (130, 95.5) + pad (-1.4, 0) = (128.6, 95.5).
    // Control THT board position:   footprint (176, 95.5) + pad (-1.4, 0) = (174.6, 95.5).
    const VECTOR2I bugPos = VECTOR2I( pcbIUScale.mmToIU( 128.6 ), pcbIUScale.mmToIU( 95.5 ) );
    const VECTOR2I controlPos = VECTOR2I( pcbIUScale.mmToIU( 174.6 ), pcbIUScale.mmToIU( 95.5 ) );
    const int      tol = pcbIUScale.mmToIU( 0.5 );

    int onBugFootprint = 0;
    int onControlFootprint = 0;

    for( const Violation& v : annularViolations )
    {
        if( std::abs( v.pos.x - bugPos.x ) < tol && std::abs( v.pos.y - bugPos.y ) < tol )
            ++onBugFootprint;
        else if( std::abs( v.pos.x - controlPos.x ) < tol && std::abs( v.pos.y - controlPos.y ) < tol )
            ++onControlFootprint;
    }

    // Bug-case pad: exact annular 0.1524 mm meets the 0.1524 mm constraint. The
    // pre-fix polygon path under-reported it by ~maxError and falsely flagged.
    BOOST_CHECK_EQUAL( onBugFootprint, 0 );

    // Control pad: 0.1 mm annular is genuinely below the constraint. The fix
    // must still report this -- guards against the optimization swallowing
    // real violations.
    BOOST_CHECK_EQUAL( onControlFootprint, 1 );
}
