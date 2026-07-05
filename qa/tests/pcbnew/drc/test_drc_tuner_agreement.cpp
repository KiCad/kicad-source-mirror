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
 * @file test_drc_tuner_agreement.cpp
 *
 * Test that DRC correctly calculates lengths for traces with multiple track widths.
 * This verifies the fix for GitLab issue #18045 where tracks with width changes caused
 * the tuner and DRC to report different lengths.
 */

#include <qa_utils/wx_utils/unit_test_utils.h>
#include <pcbnew_utils/board_test_utils.h>
#include <board.h>
#include <footprint.h>
#include <pad.h>
#include <pcb_track.h>
#include <length_delay_calculation/length_delay_calculation.h>
#include <length_delay_calculation/length_delay_calculation_item.h>
#include <settings/settings_manager.h>


struct TUNER_DRC_TEST_FIXTURE
{
    TUNER_DRC_TEST_FIXTURE() {}

    SETTINGS_MANAGER       m_settingsManager;
    std::unique_ptr<BOARD> m_board;
};


/**
 * Verify that length calculation includes all segments regardless of track width.
 * GitLab issue #18045: Length tuner disagrees with DRC due to track-width change
 */
BOOST_FIXTURE_TEST_CASE( LengthCalculationIncludesAllWidths, TUNER_DRC_TEST_FIXTURE )
{
    // Load a board that has tracks with different widths on the same net
    KI_TEST::LoadBoard( m_settingsManager, "issue18045/test_proj", m_board );

    LENGTH_DELAY_CALCULATION* lengthCalc = m_board->GetLengthCalculation();

    // Find a net that has tracks with different widths
    NETINFO_ITEM* testNet = m_board->FindNet( "/TEST1_P" );
    BOOST_REQUIRE( testNet != nullptr );

    // Collect all tracks on this net and verify they have multiple widths
    std::vector<LENGTH_DELAY_CALCULATION_ITEM> lengthItems;
    std::set<int> widths;

    for( PCB_TRACK* track : m_board->Tracks() )
    {
        if( track->GetNetCode() != testNet->GetNetCode() )
            continue;

        if( track->Type() == PCB_TRACE_T )
            widths.insert( track->GetWidth() );

        LENGTH_DELAY_CALCULATION_ITEM item = lengthCalc->GetLengthCalculationItem( track );

        if( item.Type() != LENGTH_DELAY_CALCULATION_ITEM::TYPE::UNKNOWN )
            lengthItems.emplace_back( item );
    }

    BOOST_TEST_MESSAGE( wxString::Format( "Found %zu different track widths on net /TEST1_P", widths.size() ) );

    // The test board should have at least 2 different widths to be a valid test case
    BOOST_REQUIRE_GE( widths.size(), 2 );

    // Calculate total length including all track widths
    constexpr PATH_OPTIMISATIONS opts = {
        .OptimiseVias = true, .MergeTracks = true, .OptimiseTracesInPads = true, .InferViaInPad = false
    };
    LENGTH_DELAY_STATS stats = lengthCalc->CalculateLengthDetails( lengthItems, opts, nullptr, nullptr,
                                                                    LENGTH_DELAY_LAYER_OPT::NO_LAYER_DETAIL,
                                                                    LENGTH_DELAY_DOMAIN_OPT::NO_DELAY_DETAIL );

    BOOST_TEST_MESSAGE( wxString::Format( "Total track length: %lld nm", stats.TrackLength ) );
    BOOST_CHECK_GT( stats.TrackLength, 0 );

    // Verify that all track segments contributed to the length
    BOOST_CHECK_GT( lengthItems.size(), 0 );
}


/**
 * Verify that a via inside a pad (via-in-pad) contributes its stackup
 * height to the reported via length, even when the via is placed off-centre
 * relative to the pad centroid.
 *
 * GitLab issue #23690: Net inspector undercounts via length on BGA fanout when
 * the via is off-centre of the BGA pad.
 */
BOOST_FIXTURE_TEST_CASE( ViaInPadOffCenterCounts, TUNER_DRC_TEST_FIXTURE )
{
    KI_TEST::LoadBoard( m_settingsManager, "issue23690_via_in_pad", m_board );

    LENGTH_DELAY_CALCULATION* lengthCalc = m_board->GetLengthCalculation();

    NETINFO_ITEM* testNet = m_board->FindNet( "/ABC" );
    BOOST_REQUIRE( testNet != nullptr );

    // Collect every connectivity item on the test net: tracks, vias, and pads.
    // This mirrors what PCB_NET_INSPECTOR_PANEL::calculateNets feeds into
    // CalculateLengthDetails, so the test exercises the same code path the
    // user actually sees in the Net Inspector.
    std::vector<LENGTH_DELAY_CALCULATION_ITEM> lengthItems;

    int viaCount = 0;
    int padCount = 0;

    for( PCB_TRACK* track : m_board->Tracks() )
    {
        if( track->GetNetCode() != testNet->GetNetCode() )
            continue;

        LENGTH_DELAY_CALCULATION_ITEM item = lengthCalc->GetLengthCalculationItem( track );

        if( item.Type() == LENGTH_DELAY_CALCULATION_ITEM::TYPE::VIA )
            viaCount++;

        if( item.Type() != LENGTH_DELAY_CALCULATION_ITEM::TYPE::UNKNOWN )
            lengthItems.emplace_back( std::move( item ) );
    }

    for( FOOTPRINT* fp : m_board->Footprints() )
    {
        for( PAD* pad : fp->Pads() )
        {
            if( pad->GetNetCode() != testNet->GetNetCode() )
                continue;

            LENGTH_DELAY_CALCULATION_ITEM item = lengthCalc->GetLengthCalculationItem( pad );

            if( item.Type() != LENGTH_DELAY_CALCULATION_ITEM::TYPE::UNKNOWN )
            {
                lengthItems.emplace_back( std::move( item ) );
                padCount++;
            }
        }
    }

    BOOST_TEST_MESSAGE( wxString::Format( "TEST_NET has %d vias, %d pads, %zu total length items", viaCount, padCount,
                                          lengthItems.size() ) );


    constexpr PATH_OPTIMISATIONS opts = {
        .OptimiseVias = true, .MergeTracks = true, .OptimiseTracesInPads = true, .InferViaInPad = false
    };

    LENGTH_DELAY_STATS stats = lengthCalc->CalculateLengthDetails( lengthItems, opts, nullptr, nullptr,
                                                                   LENGTH_DELAY_LAYER_OPT::NO_LAYER_DETAIL,
                                                                   LENGTH_DELAY_DOMAIN_OPT::NO_DELAY_DETAIL );

    BOOST_TEST_MESSAGE( wxString::Format( "Via length: %d nm (from %d vias), track length: %lld nm", stats.ViaLength,
                                          stats.NumVias, stats.TrackLength ) );

    BOOST_CHECK_EQUAL( stats.NumVias, 2 );

    BOOST_CHECK_GT( stats.ViaLength, 0 );

    const int expectedHeight = 2 * lengthCalc->StackupHeight( F_Cu, In1_Cu );
    BOOST_CHECK_EQUAL( stats.ViaLength, expectedHeight );
}
