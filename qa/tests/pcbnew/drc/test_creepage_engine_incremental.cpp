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
 * @file test_creepage_engine_incremental.cpp
 *
 * Asserts that the interactive engine's per-frame Update result equals a from-scratch whole-board
 * solve at the same geometry, for several drag positions.
 */

#include <qa_utils/wx_utils/unit_test_utils.h>
#include <pcbnew_utils/board_test_utils.h>

#include <board.h>
#include <board_connected_item.h>
#include <board_design_settings.h>
#include <netinfo.h>
#include <pcb_track.h>
#include <settings/settings_manager.h>

#include <drc/drc_creepage_engine.h>

#include <optional>
#include <set>


struct CREEPAGE_INCREMENTAL_FIXTURE
{
    CREEPAGE_INCREMENTAL_FIXTURE() = default;

    ~CREEPAGE_INCREMENTAL_FIXTURE()
    {
        if( m_board )
        {
            m_board->SetProject( nullptr );
            m_board = nullptr;
        }
    }

    SETTINGS_MANAGER       m_settingsManager;
    std::unique_ptr<BOARD> m_board;
};


// Large enough that every nearby creepage path is reported regardless of the board's actual rule
static constexpr int LARGE_CONSTRAINT = 50000000; // 50 mm in IU


BOOST_FIXTURE_TEST_CASE( CreepageIncrementalEqualsFull, CREEPAGE_INCREMENTAL_FIXTURE )
{
    KI_TEST::LoadBoard( m_settingsManager, "creepage/creepage", m_board );
    BOOST_REQUIRE_MESSAGE( m_board, "Failed to load board creepage" );

    PCB_LAYER_ID layer = F_Cu;

    // Find the net pair with the smallest finite creepage using the reference solver
    std::vector<int> netcodes;

    for( const auto& [code, net] : m_board->GetNetInfo().NetsByNetcode() )
    {
        if( net && code > 0 )
            netcodes.push_back( code );
    }

    CREEPAGE_ENGINE reference( *m_board );

    int    bestA = -1;
    int    bestB = -1;
    double bestDist = std::numeric_limits<double>::infinity();

    for( size_t i = 0; i < netcodes.size(); ++i )
    {
        for( size_t j = i + 1; j < netcodes.size(); ++j )
        {
            std::optional<CREEPAGE_RESULT> r =
                    reference.SolveNetPairWholeBoard( netcodes[i], netcodes[j], layer,
                                                      LARGE_CONSTRAINT );

            if( r && r->m_distance < bestDist )
            {
                bestDist = r->m_distance;
                bestA = netcodes[i];
                bestB = netcodes[j];
            }
        }
    }

    BOOST_REQUIRE_MESSAGE( bestA > 0 && bestB > 0,
                           "No finite creepage net pair found on F_Cu" );
    BOOST_TEST_MESSAGE( wxString::Format( "Tracking nets %d vs %d, base creepage %.0f nm", bestA,
                                          bestB, bestDist ) );

    std::set<const BOARD_ITEM*> movingItems;
    std::vector<BOARD_ITEM*>    movable;

    for( PCB_TRACK* track : m_board->Tracks() )
    {
        if( track && track->GetNetCode() == bestA && track->IsOnLayer( layer ) )
        {
            movingItems.insert( track );
            movable.push_back( track );
        }
    }

    BOOST_REQUIRE_MESSAGE( !movable.empty(), "Dragged net has no movable items" );

    auto constraintFn = []( int, int ) -> double { return LARGE_CONSTRAINT; };

    CREEPAGE_ENGINE engine( *m_board );
    engine.BeginInteractive( layer, { bestA }, movingItems, LARGE_CONSTRAINT, constraintFn );

    const std::vector<VECTOR2I> deltas = { { 0, 0 },
                                           { 500000, 0 },
                                           { 0, 400000 },
                                           { -300000, -200000 } };

    VECTOR2I applied( 0, 0 );

    for( const VECTOR2I& step : deltas )
    {
        // Move the items on the board itself so both solvers observe identical geometry
        for( BOARD_ITEM* item : movable )
            item->Move( step );

        applied += step;

        std::vector<CREEPAGE_RESULT> updated = engine.Update( LARGE_CONSTRAINT );

        std::optional<CREEPAGE_RESULT> full =
                reference.SolveNetPairWholeBoard( bestA, bestB, layer, LARGE_CONSTRAINT );

        double updateDist = std::numeric_limits<double>::infinity();

        for( const CREEPAGE_RESULT& r : updated )
        {
            if( ( r.m_netA == bestA && r.m_netB == bestB )
                || ( r.m_netA == bestB && r.m_netB == bestA ) )
            {
                updateDist = r.m_distance;
                break;
            }
        }

        BOOST_TEST_MESSAGE( wxString::Format( "  delta (%d,%d): update=%.0f full=%s", applied.x,
                                              applied.y, updateDist,
                                              full ? wxString::Format( "%.0f", full->m_distance )
                                                   : wxString( "none" ) ) );

        BOOST_REQUIRE_MESSAGE( full.has_value(), "Reference solve found no path" );
        BOOST_REQUIRE_MESSAGE( std::isfinite( updateDist ),
                               "Incremental Update found no path for the tracked pair" );

        // Same geometry through both code paths must agree to within rounding
        BOOST_CHECK_LE( std::abs( updateDist - full->m_distance ), 2000.0 );
    }

    for( BOARD_ITEM* item : movable )
        item->Move( -applied );

    engine.EndInteractive();
}
