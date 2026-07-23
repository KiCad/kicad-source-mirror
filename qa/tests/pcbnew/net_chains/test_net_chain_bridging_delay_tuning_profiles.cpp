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


#include <boost/test/unit_test.hpp>

#include <pcbnew_utils/board_test_utils.h>
#include <board.h>
#include <board_connected_item.h>
#include <net_chain_bridging.h>
#include <settings/settings_manager.h>


BOOST_AUTO_TEST_SUITE( NetChainBridgingDelayTuningProfiles );

BOOST_AUTO_TEST_CASE( ZeroBridgingDelay )
{
    SETTINGS_MANAGER       settingsManager;
    std::unique_ptr<BOARD> board;
    KI_TEST::LoadBoard( settingsManager, "net_chain_bridging_tuning_profiles", board );

    auto [len_1, delay_1] = BoardChainBridging( board.get(), wxS( "CHAIN_1" ) );
    BOOST_CHECK_CLOSE( len_1, 3300000.0, 0.01 );
    BOOST_CHECK_CLOSE( delay_1, 0.0, 0.01 );

    auto [len_2, delay_2] = BoardChainBridging( board.get(), wxS( "CHAIN_2" ) );
    BOOST_CHECK_CLOSE( len_2, 3300000.0, 0.01 );
    BOOST_CHECK_CLOSE( delay_2, 3300000.0, 0.01 );
}

BOOST_AUTO_TEST_SUITE_END();
