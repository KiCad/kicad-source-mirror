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
 * @file test_solder_mask_track_to_pad.cpp
 * Test soldermask bridging detection between tracks and pads from different nets.
 *
 * Bug description: When a track passes near a pad from a different net, and that track
 * is within the pad's soldermask aperture + SolderMaskToCopperClearance, a bridging error
 * should be reported. This was not happening because the R-tree query clearance did not
 * include SolderMaskToCopperClearance.
 */

#include <qa_utils/wx_utils/unit_test_utils.h>
#include <pcbnew_utils/board_test_utils.h>
#include <board.h>
#include <board_design_settings.h>
#include <pcb_track.h>
#include <drc/drc_item.h>
#include <settings/settings_manager.h>
#include <kiid.h>


struct DRC_TRACK_TO_PAD_BRIDGE_FIXTURE
{
    DRC_TRACK_TO_PAD_BRIDGE_FIXTURE() :
            m_settingsManager( true /* headless */ )
    { }

    SETTINGS_MANAGER       m_settingsManager;
    std::unique_ptr<BOARD> m_board;
};


BOOST_FIXTURE_TEST_CASE( DRCTrackToPadBridgeTest, DRC_TRACK_TO_PAD_BRIDGE_FIXTURE )
{
    // Test board has:
    // - A pad on B.Cu with B.Mask at position (100, 100) on net "netA"
    // - A track on B.Cu at x=100.6 (0.6mm from pad center) on net "netB"
    // - SolderMaskToCopperClearance set to 1.0mm
    //
    // The track is 0.6mm from the pad center. With a 1mm pad and 0.05mm mask expansion,
    // the pad edge is at 100.5mm and mask aperture edge is at 100.55mm.
    // The track (0.2mm wide) has its nearest edge at 100.5mm.
    //
    // The track copper is within the SolderMaskToCopperClearance of the pad's mask aperture,
    // so a violation should be reported.

    wxString brd_name( wxT( "soldermask_track_to_pad" ) );
    KI_TEST::LoadBoard( m_settingsManager, brd_name, m_board );

    std::vector<DRC_ITEM>  violations;
    BOARD_DESIGN_SETTINGS& bds = m_board->GetDesignSettings();

    // Verify the clearance is set
    BOOST_TEST_MESSAGE( wxString::Format( "SolderMaskToCopperClearance: %d nm",
                                          bds.m_SolderMaskToCopperClearance ) );

    // Disable DRC tests not relevant to this test
    bds.m_DRCSeverities[ DRCE_LIB_FOOTPRINT_ISSUES ] = SEVERITY::RPT_SEVERITY_IGNORE;
    bds.m_DRCSeverities[ DRCE_LIB_FOOTPRINT_MISMATCH ] = SEVERITY::RPT_SEVERITY_IGNORE;
    bds.m_DRCSeverities[ DRCE_COPPER_SLIVER ] = SEVERITY::RPT_SEVERITY_IGNORE;
    bds.m_DRCSeverities[ DRCE_STARVED_THERMAL ] = SEVERITY::RPT_SEVERITY_IGNORE;
    bds.m_DRCSeverities[ DRCE_SILK_CLEARANCE ] = SEVERITY::RPT_SEVERITY_IGNORE;

    bds.m_DRCEngine->SetViolationHandler(
            [&]( const std::shared_ptr<DRC_ITEM>& aItem, VECTOR2I aPos, int aLayer,
                 DRC_CUSTOM_MARKER_HANDLER* aCustomHandler )
            {
                if( aItem->GetErrorCode() == DRCE_SOLDERMASK_BRIDGE )
                    violations.push_back( *aItem );
            } );

    // Run DRC
    bds.m_DRCEngine->RunTests( EDA_UNITS::MM, true /* aReportAllTrackErrors */, false );

    BOOST_TEST_MESSAGE( wxString::Format( "Found %zu soldermask bridge violations",
                                          violations.size() ) );

    // We expect at least one violation: track from netB is within clearance of pad from netA
    BOOST_CHECK_GE( violations.size(), 1 );

    // Verify the violation involves the track
    bool foundTrackViolation = false;

    for( const DRC_ITEM& item : violations )
    {
        KIID mainId = item.GetMainItemID();
        KIID auxId = item.GetAuxItemID();

        BOARD_ITEM* mainItem = mainId != niluuid ? m_board->GetItem( mainId ) : nullptr;
        BOARD_ITEM* auxItem = auxId != niluuid ? m_board->GetItem( auxId ) : nullptr;

        BOOST_TEST_MESSAGE( wxString::Format( "Violation: main=%s aux=%s",
                mainItem ? mainItem->GetClass() : wxS( "null" ),
                auxItem ? auxItem->GetClass() : wxS( "null" ) ) );

        if( ( mainItem && mainItem->Type() == PCB_TRACE_T ) ||
            ( auxItem && auxItem->Type() == PCB_TRACE_T ) )
        {
            foundTrackViolation = true;
        }
    }

    BOOST_CHECK_MESSAGE( foundTrackViolation, "Expected to find a track-to-pad soldermask violation" );
}
