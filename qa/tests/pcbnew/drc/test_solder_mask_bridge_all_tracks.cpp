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
 * @file test_solder_mask_bridge_all_tracks.cpp
 * Test that soldermask bridging reports all track combinations when "report all track errors"
 * is enabled.
 *
 * Bug description: When DRC checks for soldermask bridging between two nets, only one object
 * from net A is reported against all objects from net B. With "report all track errors" enabled,
 * all track combinations should be reported.
 */

#include <qa_utils/wx_utils/unit_test_utils.h>
#include <pcbnew_utils/board_test_utils.h>
#include <board.h>
#include <board_design_settings.h>
#include <pcb_marker.h>
#include <pcb_track.h>
#include <drc/drc_item.h>
#include <settings/settings_manager.h>

#include <set>


struct DRC_SOLDER_MASK_BRIDGE_ALL_TRACKS_FIXTURE
{
    DRC_SOLDER_MASK_BRIDGE_ALL_TRACKS_FIXTURE() :
            m_settingsManager( true /* headless */ )
    { }

    SETTINGS_MANAGER       m_settingsManager;
    std::unique_ptr<BOARD> m_board;
};


BOOST_FIXTURE_TEST_CASE( DRCSolderMaskBridgeAllTracksTest, DRC_SOLDER_MASK_BRIDGE_ALL_TRACKS_FIXTURE )
{
    // Test board has:
    // - Net "a" with 4 track segments
    // - Net "b" with 4 track segments
    // - A soldermask opening (gr_rect on F.Mask) covering where tracks from both nets pass through
    //
    // With "report all track errors" enabled, we expect all combinations of net A tracks vs net B
    // tracks to be reported. Each track from net A that bridges with each track from net B should
    // generate a violation.

    wxString brd_name( wxT( "soldermask_bridge_all_tracks" ) );
    KI_TEST::LoadBoard( m_settingsManager, brd_name, m_board );

    std::vector<DRC_ITEM>  violations;
    BOARD_DESIGN_SETTINGS& bds = m_board->GetDesignSettings();

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

    // Count tracks per net to understand expected violations
    int netA_tracks = 0;
    int netB_tracks = 0;

    for( PCB_TRACK* track : m_board->Tracks() )
    {
        if( track->GetNetname() == wxT( "a" ) )
            netA_tracks++;
        else if( track->GetNetname() == wxT( "b" ) )
            netB_tracks++;
    }

    BOOST_TEST_MESSAGE( wxString::Format( "Net A has %d tracks, Net B has %d tracks",
                                          netA_tracks, netB_tracks ) );

    // Run DRC with "report all track errors" = true
    bds.m_DRCEngine->RunTests( EDA_UNITS::MM, true /* aReportAllTrackErrors */, false );

    // With "report all track errors" enabled, we expect all track pair combinations to be reported.
    // In the test board, both nets have 4 track segments each that pass through the soldermask
    // opening. The soldermask aperture (gr_rect) bridges copper from different nets.
    //
    // Each track from net A that is close enough to each track from net B within the soldermask
    // opening should generate a violation. For this test case, we expect multiple violations,
    // not just one.
    //
    // The exact number depends on which track segments are actually within clearance distance
    // in the soldermask opening area.

    // We expect MORE violations when reporting all track errors than in single error mode.
    // With 4 tracks + 1 pad on each net passing through the soldermask opening:
    // - Non-track items (pads) always get all combinations reported
    // - Track items get all combinations only when "report all track errors" is enabled
    //
    // The original bug was that all violations showed the same track from net A (the first
    // one cached), regardless of how many tracks actually violated. Now:
    // - All non-track items from net A are reported against each net B collision
    // - All track items from net A are reported (when option is set) against each net B collision
    BOOST_CHECK_GT( violations.size(), 5 );

    BOOST_TEST_MESSAGE( wxString::Format( "Found %zu soldermask bridge violations",
                                          violations.size() ) );

    // Verify that we're reporting different items from net A (not just the first cached one)
    std::set<KIID> netAItemIds;

    for( const DRC_ITEM& item : violations )
    {
        if( item.GetAuxItemID() != niluuid )
            netAItemIds.insert( item.GetAuxItemID() );
    }

    // With 4 tracks + 1 pad on net A, we should see multiple different items reported
    BOOST_CHECK_GE( netAItemIds.size(), 4 );

    BOOST_TEST_MESSAGE( wxString::Format( "Found %zu different net A items in violations",
                                          netAItemIds.size() ) );
}


BOOST_FIXTURE_TEST_CASE( DRCSolderMaskBridgeSingleErrorTest, DRC_SOLDER_MASK_BRIDGE_ALL_TRACKS_FIXTURE )
{
    // When "report all track errors" is disabled, we should get fewer violations
    // (the original behavior - only report one error per track connection)

    wxString brd_name( wxT( "soldermask_bridge_all_tracks" ) );
    KI_TEST::LoadBoard( m_settingsManager, brd_name, m_board );

    std::vector<DRC_ITEM>  violations;
    BOARD_DESIGN_SETTINGS& bds = m_board->GetDesignSettings();

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

    // Run DRC with "report all track errors" = false
    bds.m_DRCEngine->RunTests( EDA_UNITS::MM, false /* aReportAllTrackErrors */, false );

    size_t singleErrorViolations = violations.size();

    BOOST_TEST_MESSAGE( wxString::Format( "Single error mode: Found %zu soldermask bridge violations",
                                          singleErrorViolations ) );

    // Run again with "report all track errors" = true for comparison
    violations.clear();
    bds.m_DRCEngine->RunTests( EDA_UNITS::MM, true /* aReportAllTrackErrors */, false );

    size_t allErrorsViolations = violations.size();

    BOOST_TEST_MESSAGE( wxString::Format( "All errors mode: Found %zu soldermask bridge violations",
                                          allErrorsViolations ) );

    // When reporting all errors, we should have at least as many (likely more) violations
    BOOST_CHECK_GE( allErrorsViolations, singleErrorViolations );
}
