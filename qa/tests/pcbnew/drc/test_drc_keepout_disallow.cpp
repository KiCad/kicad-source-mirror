/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright The KiCad Developers.
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
#include <drc/drc_engine.h>
#include <drc/drc_item.h>
#include <pcb_marker.h>
#include <settings/settings_manager.h>


struct DRC_KEEPOUT_TEST_FIXTURE
{
    DRC_KEEPOUT_TEST_FIXTURE()
    {}

    SETTINGS_MANAGER       m_settingsManager;
    std::unique_ptr<BOARD> m_board;
};


// Regression test for GitLab #23911: Missing DRC notification for Rule Areas.
// The test board has a rule area that disallows tracks, vias, pads, zones, and
// footprints.  It contains two vias fully inside the keepout and a track that
// crosses into the keepout.  All three items must produce DRCE_ALLOWED_ITEMS
// violations.
BOOST_FIXTURE_TEST_CASE( DRCKeepoutDisallowViasAndTracks, DRC_KEEPOUT_TEST_FIXTURE )
{
    KI_TEST::LoadBoard( m_settingsManager, "keepout_disallow/keepout_disallow", m_board );

    std::vector<DRC_ITEM>  violations;
    BOARD_DESIGN_SETTINGS& bds = m_board->GetDesignSettings();

    // Suppress unrelated checks that fire on a bare-bones board.
    bds.m_DRCSeverities[DRCE_INVALID_OUTLINE] = SEVERITY::RPT_SEVERITY_IGNORE;
    bds.m_DRCSeverities[DRCE_UNCONNECTED_ITEMS] = SEVERITY::RPT_SEVERITY_IGNORE;
    bds.m_DRCSeverities[DRCE_LIB_FOOTPRINT_ISSUES] = SEVERITY::RPT_SEVERITY_IGNORE;
    bds.m_DRCSeverities[DRCE_LIB_FOOTPRINT_MISMATCH] = SEVERITY::RPT_SEVERITY_IGNORE;
    bds.m_DRCSeverities[DRCE_DRILL_OUT_OF_RANGE] = SEVERITY::RPT_SEVERITY_IGNORE;
    bds.m_DRCSeverities[DRCE_VIA_DIAMETER] = SEVERITY::RPT_SEVERITY_IGNORE;
    bds.m_DRCSeverities[DRCE_COPPER_SLIVER] = SEVERITY::RPT_SEVERITY_IGNORE;
    bds.m_DRCSeverities[DRCE_STARVED_THERMAL] = SEVERITY::RPT_SEVERITY_IGNORE;
    bds.m_DRCSeverities[DRCE_ALLOWED_ITEMS] = SEVERITY::RPT_SEVERITY_ERROR;

    bds.m_DRCEngine->SetViolationHandler(
            [&]( const std::shared_ptr<DRC_ITEM>& aItem, const VECTOR2I& aPos, int aLayer,
                 const std::function<void( PCB_MARKER* )>& aPathGenerator )
            {
                if( aItem->GetErrorCode() == DRCE_ALLOWED_ITEMS )
                    violations.push_back( *aItem );
            } );

    bds.m_DRCEngine->RunTests( EDA_UNITS::MM, true, false );

    int trackViolations = 0;
    int viaViolations = 0;

    std::map<KIID, EDA_ITEM*> itemMap;
    m_board->FillItemMap( itemMap );

    for( const DRC_ITEM& item : violations )
    {
        const KIID id = item.GetMainItemID();

        if( id == niluuid )
            continue;

        auto it = itemMap.find( id );

        if( it == itemMap.end() )
            continue;

        switch( it->second->Type() )
        {
        case PCB_VIA_T:   viaViolations++;   break;
        case PCB_TRACE_T: trackViolations++; break;
        default:                             break;
        }
    }

    // Expect exact counts so that duplicate-marker regressions are caught.
    BOOST_CHECK_MESSAGE( viaViolations == 2,
                        "Expected exactly 2 via keepout violations, got "
                                << viaViolations << " (total: " << violations.size() << ")" );
    BOOST_CHECK_MESSAGE( trackViolations == 1,
                        "Expected exactly 1 track keepout violation, got "
                                << trackViolations );
    BOOST_CHECK_MESSAGE( violations.size() == 3,
                        "Expected exactly 3 DRCE_ALLOWED_ITEMS violations, got "
                                << violations.size() );

    if( viaViolations != 2 || trackViolations != 1 || violations.size() != 3 )
    {
        UNITS_PROVIDER unitsProvider( pcbIUScale, EDA_UNITS::MM );

        for( const DRC_ITEM& item : violations )
            BOOST_TEST_MESSAGE( item.ShowReport( &unitsProvider, RPT_SEVERITY_ERROR, itemMap ) );
    }
}


// Regression test: a track outside a no-tracks keepout must not be flagged
// when board->m_DRCMaxClearance is large.  The antiTrackKeepouts Collide()
// call previously used m_DRCMaxClearance as the collision distance, silently
// inflating every keepout boundary by whatever the largest clearance on the
// board happened to be.
//
// Board layout:
//   - Keepout rule area (no tracks): (130,60)-(140,90) on F.Cu
//   - Track segment at x=144.018 — 4 mm outside the keepout right edge
//   - Copper zone with 20 mm local clearance at (176,55)-(196,95)
//   - physical_clearance rule of 20 mm in .kicad_dru to widen R-tree search
//
// The track is outside the keepout so no violation should be reported.
BOOST_FIXTURE_TEST_CASE( DRCKeepoutNoClearanceInflation, DRC_KEEPOUT_TEST_FIXTURE )
{
    KI_TEST::LoadBoard( m_settingsManager, "keepout_no_clearance/keepout_no_clearance", m_board );

    std::vector<DRC_ITEM>  violations;
    BOARD_DESIGN_SETTINGS& bds = m_board->GetDesignSettings();

    bds.m_DRCSeverities[DRCE_INVALID_OUTLINE] = SEVERITY::RPT_SEVERITY_IGNORE;
    bds.m_DRCSeverities[DRCE_UNCONNECTED_ITEMS] = SEVERITY::RPT_SEVERITY_IGNORE;
    bds.m_DRCSeverities[DRCE_LIB_FOOTPRINT_ISSUES] = SEVERITY::RPT_SEVERITY_IGNORE;
    bds.m_DRCSeverities[DRCE_LIB_FOOTPRINT_MISMATCH] = SEVERITY::RPT_SEVERITY_IGNORE;
    bds.m_DRCSeverities[DRCE_COPPER_SLIVER] = SEVERITY::RPT_SEVERITY_IGNORE;
    bds.m_DRCSeverities[DRCE_STARVED_THERMAL] = SEVERITY::RPT_SEVERITY_IGNORE;
    bds.m_DRCSeverities[DRCE_ALLOWED_ITEMS] = SEVERITY::RPT_SEVERITY_ERROR;

    bds.m_DRCEngine->SetViolationHandler(
            [&]( const std::shared_ptr<DRC_ITEM>& aItem, const VECTOR2I& aPos, int aLayer,
                 const std::function<void( PCB_MARKER* )>& aPathGenerator )
            {
                if( aItem->GetErrorCode() == DRCE_ALLOWED_ITEMS )
                    violations.push_back( *aItem );
            } );

    bds.m_DRCEngine->RunTests( EDA_UNITS::MM, true, false );

    if( !violations.empty() )
    {
        UNITS_PROVIDER            unitsProvider( pcbIUScale, EDA_UNITS::MM );
        std::map<KIID, EDA_ITEM*> itemMap;
        m_board->FillItemMap( itemMap );

        for( const DRC_ITEM& item : violations )
            BOOST_TEST_MESSAGE( item.ShowReport( &unitsProvider, RPT_SEVERITY_ERROR, itemMap ) );
    }

    BOOST_CHECK_MESSAGE( violations.empty(),
                         "Expected no keepout violations for track outside keepout, got " << violations.size() );
}
