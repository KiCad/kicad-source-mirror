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
 * @file test_drc_npth_slot_clearance_issue24799.cpp
 *
 * Regression test for issue #24799: oblong NPTH holes picked up the
 * copper-to-edge clearance instead of the copper-to-hole clearance.
 *
 * The fixture has a round and an oblong NPTH mounting hole well away from
 * the board edge, an F.Cu zone over the whole board, and a track 0.5 mm
 * from the slot wall.  Copper-to-hole is 0.25 mm, copper-to-edge 1 mm.
 * Both holes must get the same 0.25 mm fill standoff and DRC must not
 * report the track.
 */

#include <qa_utils/wx_utils/unit_test_utils.h>
#include <pcbnew_utils/board_test_utils.h>

#include <base_units.h>
#include <board.h>
#include <geometry/shape_segment.h>
#include <board_design_settings.h>
#include <footprint.h>
#include <pad.h>
#include <zone.h>
#include <drc/drc_item.h>
#include <drc/drc_engine.h>
#include <settings/settings_manager.h>
#include <widgets/report_severity.h>


struct NPTH_SLOT_CLEARANCE_FIXTURE
{
    NPTH_SLOT_CLEARANCE_FIXTURE() = default;

    ~NPTH_SLOT_CLEARANCE_FIXTURE()
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


BOOST_FIXTURE_TEST_CASE( NpthSlotClearanceIssue24799, NPTH_SLOT_CLEARANCE_FIXTURE )
{
    KI_TEST::LoadBoard( m_settingsManager, "issue24799/issue24799", m_board );

    BOOST_REQUIRE_MESSAGE( m_board, "Failed to load board issue24799" );

    PAD* slotPad = nullptr;
    PAD* roundPad = nullptr;

    for( FOOTPRINT* footprint : m_board->Footprints() )
    {
        for( PAD* pad : footprint->Pads() )
        {
            if( pad->GetAttribute() != PAD_ATTRIB::NPTH || !pad->HasHole() )
                continue;

            if( pad->GetDrillSizeX() != pad->GetDrillSizeY() )
                slotPad = pad;
            else
                roundPad = pad;
        }
    }

    BOOST_REQUIRE_MESSAGE( slotPad, "Fixture must contain an oblong NPTH pad" );
    BOOST_REQUIRE_MESSAGE( roundPad, "Fixture must contain a round NPTH pad" );

    KI_TEST::FillZones( m_board.get() );

    BOOST_REQUIRE_EQUAL( m_board->Zones().size(), 1 );

    const std::shared_ptr<SHAPE_POLY_SET>& fill = m_board->Zones().front()->GetFilledPolysList( F_Cu );

    BOOST_REQUIRE( fill && fill->OutlineCount() > 0 );

    const int searchDist = pcbIUScale.mmToIU( 2.0 );
    const int minGap = pcbIUScale.mmToIU( 0.2 );
    const int maxGap = pcbIUScale.mmToIU( 0.5 );

    for( PAD* pad : { roundPad, slotPad } )
    {
        int actual = 0;

        BOOST_REQUIRE_MESSAGE( fill->Collide( pad->GetEffectiveHoleShape().get(), searchDist, &actual ),
                               "No fill within 2 mm of the hole, standoff is far too large" );

        BOOST_TEST_MESSAGE( wxString::Format( "Fill standoff from %s hole: %.3f mm",
                                              pad->GetDrillSizeX() == pad->GetDrillSizeY() ? wxString( "round" )
                                                                                           : wxString( "oblong" ),
                                              pcbIUScale.IUTomm( actual ) ) );

        // both holes should get the 0.25 mm hole clearance, not the 1 mm edge clearance
        BOOST_CHECK_GE( actual, minGap );
        BOOST_CHECK_LE( actual, maxGap );
    }

    BOARD_DESIGN_SETTINGS& bds = m_board->GetDesignSettings();

    BOOST_REQUIRE_MESSAGE( bds.m_DRCEngine, "DRC engine not initialized" );

    for( int ii = DRCE_FIRST; ii <= DRCE_LAST; ++ii )
        bds.m_DRCSeverities[ii] = SEVERITY::RPT_SEVERITY_IGNORE;

    bds.m_DRCSeverities[DRCE_EDGE_CLEARANCE] = SEVERITY::RPT_SEVERITY_ERROR;
    bds.m_DRCSeverities[DRCE_HOLE_CLEARANCE] = SEVERITY::RPT_SEVERITY_ERROR;

    int edgeClearanceCount = 0;
    int holeClearanceCount = 0;

    bds.m_DRCEngine->SetViolationHandler(
            [&]( const std::shared_ptr<DRC_ITEM>& aItem, const VECTOR2I& /*aPos*/, int /*aLayer*/,
                 const std::function<void( PCB_MARKER* )>& /*aPathGen*/ )
            {
                if( aItem->GetErrorCode() == DRCE_EDGE_CLEARANCE )
                    ++edgeClearanceCount;
                else if( aItem->GetErrorCode() == DRCE_HOLE_CLEARANCE )
                    ++holeClearanceCount;
            } );

    bds.m_DRCEngine->RunTests( EDA_UNITS::MM, true, false );

    bds.m_DRCEngine->ClearViolationHandler();

    BOOST_TEST_MESSAGE( wxString::Format( "Edge clearance violations: %d, hole clearance: %d", edgeClearanceCount,
                                          holeClearanceCount ) );

    // the track is 0.5 mm from the slot wall, legal against the 0.25 mm hole clearance
    BOOST_CHECK_EQUAL( edgeClearanceCount, 0 );
    BOOST_CHECK_EQUAL( holeClearanceCount, 0 );
}
