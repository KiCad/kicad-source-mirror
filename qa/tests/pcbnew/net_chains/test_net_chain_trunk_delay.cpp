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

#include <pcbnew_utils/board_file_utils.h>

#include <base_units.h>
#include <board.h>
#include <drc/drc_chain_topology.h>
#include <footprint.h>
#include <net_chain_bridging.h>
#include <netinfo.h>
#include <pad.h>
#include <pcb_track.h>
#include <pcbnew/pcb_io/kicad_sexpr/pcb_io_kicad_sexpr.h>


constexpr double DEFAULT_PROPAGATION_DELAY_PS_PER_MM = 5.9 * pcbIUScale.IU_PER_PS;

// Two-net daisy chain through a single bridge.  Trunk = 20 mm (net A track) +
// 5 mm (bridge pad-to-pad span) + 25 mm (net B track) = 50 mm.  The board has
// no stackup so the per-track length-delay calculator returns a known-type
// item with zero delay (trackDelay() does not fall back when Type != UNKNOWN);
// only the bridge edge carries delay, derived from the chain-wide fallback
// 5.9 ps/mm.  This pins the trunk-delay equal-to-bridge contract.
static const char* DAISY_PCB_FILE = "net_chains/net_chain_trunk_delay.kicad_pcb";


namespace
{
std::unique_ptr<BOARD> loadBoard( const char* aBoardFile )
{
    PCB_IO_KICAD_SEXPR     plugin;
    std::unique_ptr<BOARD> board = std::make_unique<BOARD>();
    plugin.LoadBoard( KI_TEST::GetPcbnewTestDataDir() + aBoardFile, board.get() );
    board->BuildConnectivity();

    return board;
}


void tagChainNets( BOARD* aBoard, const wxString& aChain )
{
    for( NETINFO_ITEM* n : aBoard->GetNetInfo() )
    {
        if( n && n->GetNetname().StartsWith( wxS( "/NET_" ) ) )
            n->SetNetChain( aChain );
    }
}


// Anchor terminals at the extreme-X footprints; the fixture routes in a single
// row and carries no Reference strings to match on
void setTerminals( BOARD* aBoard, const wxString& aChain )
{
    FOOTPRINT* first = nullptr;
    FOOTPRINT* last = nullptr;

    for( FOOTPRINT* fp : aBoard->Footprints() )
    {
        if( fp->Pads().empty() )
            continue;

        if( !first || fp->GetPosition().x < first->GetPosition().x )
            first = fp;

        if( !last || fp->GetPosition().x > last->GetPosition().x )
            last = fp;
    }

    BOOST_REQUIRE( first );
    BOOST_REQUIRE( last );
    BOOST_REQUIRE( first != last );

    for( NETINFO_ITEM* n : aBoard->GetNetInfo() )
    {
        if( n && n->GetNetChain() == aChain )
        {
            n->SetTerminalPad( 0, first->Pads().front() );
            n->SetTerminalPad( 1, last->Pads().front() );
        }
    }
}


std::set<BOARD_CONNECTED_ITEM*> collectChainItems( BOARD* aBoard, const wxString& aChain )
{
    std::set<BOARD_CONNECTED_ITEM*> items;

    for( PCB_TRACK* t : aBoard->Tracks() )
    {
        if( t->GetNet() && t->GetNet()->GetNetChain() == aChain )
            items.insert( t );
    }

    for( FOOTPRINT* fp : aBoard->Footprints() )
    {
        for( PAD* p : fp->Pads() )
        {
            if( p->GetNet() && p->GetNet()->GetNetChain() == aChain )
                items.insert( p );
        }
    }

    return items;
}

}  // namespace


BOOST_AUTO_TEST_SUITE( NetChainTrunkDelay )


// 50 mm trunk, no stackup => tracks contribute 0 delay, bridge contributes
// 5 mm at the fallback 5.9 ps/mm.  Verifies that the bridge edge's delay is
// derived consistently with BoardChainBridging() and folded into TrunkDelay().
BOOST_AUTO_TEST_CASE( DaisyTrunkDelayEqualsBridgeWithoutStackup )
{
    auto board = loadBoard( DAISY_PCB_FILE );
    tagChainNets( board.get(), wxS( "DSY" ) );
    setTerminals( board.get(), wxS( "DSY" ) );

    CHAIN_TOPOLOGY topo( board.get(), wxS( "DSY" ), collectChainItems( board.get(), wxS( "DSY" ) ),
                         DEFAULT_PROPAGATION_DELAY_PS_PER_MM );

    BOOST_REQUIRE( topo.IsValid() );
    BOOST_CHECK_CLOSE( topo.TrunkLength(), 50.0e6, 5.0 );

    double expectedDelayIU = DEFAULT_PROPAGATION_DELAY_PS_PER_MM * 5.0;
    BOOST_CHECK_CLOSE( topo.TrunkDelay(), expectedDelayIU, 5.0 );
    BOOST_CHECK_GT( topo.TrunkDelay(), 0.0 );

    // The bridging-only helper should agree with what the trunk picked up.
    auto [bridgingLen, bridgingDelay] =
            BoardChainBridging( board.get(), wxS( "DSY" ), DEFAULT_PROPAGATION_DELAY_PS_PER_MM );
    BOOST_CHECK_CLOSE( bridgingDelay, topo.TrunkDelay(), 0.001 );
    BOOST_CHECK_CLOSE( bridgingLen, 5.0e6, 0.001 );
}


// Without terminal pads the topology cannot reduce to a trunk; callers must
// fall back to BoardChainBridging.  Verify both: the topology is not valid,
// and the bridging-only delay covers the 5 mm cross-net pad span at the
// fallback per-mm rate.
BOOST_AUTO_TEST_CASE( NoTerminalsFallbackUsesBridgingDelay )
{
    auto board = loadBoard( DAISY_PCB_FILE );
    tagChainNets( board.get(), wxS( "DSY" ) );
    // Intentionally do NOT call setTerminals.

    CHAIN_TOPOLOGY topo( board.get(), wxS( "DSY" ), collectChainItems( board.get(), wxS( "DSY" ) ),
                         DEFAULT_PROPAGATION_DELAY_PS_PER_MM );

    BOOST_CHECK( !topo.IsValid() );
    BOOST_CHECK_EQUAL( static_cast<int>( topo.GetStatus() ),
                       static_cast<int>( CHAIN_TOPOLOGY::STATUS::NO_TERMINAL_PADS ) );

    auto [bridgingLen, bridgingDelay] =
            BoardChainBridging( board.get(), wxS( "DSY" ), DEFAULT_PROPAGATION_DELAY_PS_PER_MM );

    BOOST_CHECK_CLOSE( bridgingLen, 5.0e6, 0.001 );

    double expectedDelayIU = DEFAULT_PROPAGATION_DELAY_PS_PER_MM * 5.0;
    BOOST_CHECK_CLOSE( bridgingDelay, expectedDelayIU, 0.001 );
}


BOOST_AUTO_TEST_SUITE_END()
