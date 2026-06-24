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
 * @file test_drc_skew_arc.cpp
 *
 * Regression for GitLab #24726. A diff pair tuned to zero skew was wrongly
 * flagged when a track curves into a via. The arc was not merged, so the segment
 * chain ended at a bend inside the via. The via clip then stretched that bend to
 * the via centre and added phantom length. DRC must match the tuner length.
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


struct DRC_SKEW_ARC_FIXTURE
{
    SETTINGS_MANAGER       m_settingsManager;
    std::unique_ptr<BOARD> m_board;

    int64_t netLength( const wxString& aNet, const PATH_OPTIMISATIONS& aOpts )
    {
        LENGTH_DELAY_CALCULATION* calc = m_board->GetLengthCalculation();
        NETINFO_ITEM*             net = m_board->FindNet( aNet );
        BOOST_REQUIRE_MESSAGE( net != nullptr, "net not found: " << aNet.ToStdString() );

        std::vector<LENGTH_DELAY_CALCULATION_ITEM> items;

        for( PCB_TRACK* track : m_board->Tracks() )
        {
            if( track->GetNetCode() != net->GetNetCode() )
                continue;

            LENGTH_DELAY_CALCULATION_ITEM item = calc->GetLengthCalculationItem( track );

            if( item.Type() != LENGTH_DELAY_CALCULATION_ITEM::TYPE::UNKNOWN )
                items.emplace_back( std::move( item ) );
        }

        for( FOOTPRINT* fp : m_board->Footprints() )
        {
            for( PAD* pad : fp->Pads() )
            {
                if( pad->GetNetCode() != net->GetNetCode() )
                    continue;

                LENGTH_DELAY_CALCULATION_ITEM item = calc->GetLengthCalculationItem( pad );

                if( item.Type() != LENGTH_DELAY_CALCULATION_ITEM::TYPE::UNKNOWN )
                    items.emplace_back( std::move( item ) );
            }
        }

        return calc->CalculateLength( items, aOpts, nullptr, nullptr );
    }
};


// DRC (clip and merge on) must agree with the tuner (clip and merge off) for
// every diff pair. Before the fix net2_P was inflated by about 223 um, over the
// 0.1 mm rule.
BOOST_FIXTURE_TEST_CASE( DrcSkewArc_TunerVsDrc, DRC_SKEW_ARC_FIXTURE )
{
    KI_TEST::LoadBoard( m_settingsManager, "drc_skew/drc_skew", m_board );

    constexpr PATH_OPTIMISATIONS drcOpts = {
        .OptimiseVias = true, .MergeTracks = true, .OptimiseTracesInPads = true, .InferViaInPad = false
    };
    constexpr PATH_OPTIMISATIONS tunerOpts = {
        .OptimiseVias = false, .MergeTracks = false, .OptimiseTracesInPads = false, .InferViaInPad = true
    };

    const int64_t skewLimit = 100000; // 0.1 mm rule in the board

    for( const wxString& pair : { wxString( "net1" ), wxString( "net2" ), wxString( "net3" ) } )
    {
        const wxString p = "/" + pair + "_P";
        const wxString n = "/" + pair + "_N";

        const int64_t drcSkew = std::llabs( netLength( p, drcOpts ) - netLength( n, drcOpts ) );
        const int64_t tunSkew = std::llabs( netLength( p, tunerOpts ) - netLength( n, tunerOpts ) );

        BOOST_TEST_MESSAGE( pair.ToStdString() << ": DRC skew = " << drcSkew / 1000.0
                                               << " um, tuner skew = " << tunSkew / 1000.0 << " um" );

        BOOST_CHECK_MESSAGE( std::llabs( drcSkew - tunSkew ) < skewLimit,
                             pair.ToStdString() << ": DRC and tuner skew disagree by "
                                                << std::llabs( drcSkew - tunSkew ) / 1000.0 << " um" );
    }
}
