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

#include <set>

#include <board.h>
#include <board_connected_item.h>
#include <drc/drc_chain_topology.h>
#include <footprint.h>
#include <netinfo.h>
#include <pad.h>
#include <pcb_track.h>
#include <pcbnew/pcb_io/kicad_sexpr/pcb_io_kicad_sexpr.h>


namespace
{

constexpr int MM = 1000000;


std::unique_ptr<BOARD> loadBoard( const char* aBoardFile )
{
    PCB_IO_KICAD_SEXPR     plugin;
    std::unique_ptr<BOARD> board = std::make_unique<BOARD>();
    plugin.LoadBoard( KI_TEST::GetPcbnewTestDataDir() + aBoardFile, board.get() );
    board->BuildConnectivity();

    return board;
}


std::set<BOARD_CONNECTED_ITEM*> chainItems( BOARD* aBoard, const wxString& aChain )
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


// Tag every net whose name starts with "/NET_" into the named chain, and set
// terminal pads to the first/last footprint's first pad on that chain.  fp1
// pad-1 is terminal[0], lastFp pad-1 is terminal[1].
void tagChain( BOARD* aBoard, const wxString& aChain, const wxString& aFirstFpRef,
               const wxString& aLastFpRef )
{
    for( NETINFO_ITEM* n : aBoard->GetNetInfo() )
    {
        if( n && n->GetNetname().StartsWith( wxS( "/NET_" ) ) )
            n->SetNetChain( aChain );
    }

    PAD* termA = nullptr;
    PAD* termB = nullptr;

    for( FOOTPRINT* fp : aBoard->Footprints() )
    {
        if( fp->GetReference() == aFirstFpRef && !fp->Pads().empty() )
            termA = fp->Pads().front();

        if( fp->GetReference() == aLastFpRef && !fp->Pads().empty() )
            termB = fp->Pads().back();
    }

    if( termA )
    {
        for( NETINFO_ITEM* n : aBoard->GetNetInfo() )
            if( n && n->GetNetChain() == aChain )
                n->SetTerminalPad( 0, termA );
    }

    if( termB )
    {
        for( NETINFO_ITEM* n : aBoard->GetNetInfo() )
            if( n && n->GetNetChain() == aChain )
                n->SetTerminalPad( 1, termB );
    }
}


}  // namespace


BOOST_AUTO_TEST_SUITE( DRCChainTopology )


// Three-net trunk through two passives.
// pad@(0,0) — track 30mm — bridge 5mm — track 30mm — bridge 5mm — track 30mm — pad@(100,0).
// Trunk = 100 mm, zero stubs.
BOOST_AUTO_TEST_CASE( TopologyTreeOnSimpleTrunk )
{
    auto board = loadBoard( "net_chains/chain_topology_trunk.kicad_pcb" );
    tagChain( board.get(), wxS( "SIG" ), wxS( "FP_START" ), wxS( "FP_END" ) );

    auto items = chainItems( board.get(), wxS( "SIG" ) );
    CHAIN_TOPOLOGY topo( board.get(), wxS( "SIG" ), items );

    BOOST_CHECK_EQUAL( static_cast<int>( topo.GetStatus() ),
                       static_cast<int>( CHAIN_TOPOLOGY::STATUS::OK ) );
    BOOST_CHECK( topo.IsValid() );
    BOOST_CHECK_CLOSE( topo.TrunkLength(), 100.0 * MM, 5.0 );
    BOOST_CHECK( topo.Stubs().empty() );
}


// Single-net trunk plus a perpendicular T-stub of 5 mm at the midpoint.
BOOST_AUTO_TEST_CASE( TopologyDetectsTStub )
{
    auto board = loadBoard( "net_chains/chain_topology_t_stub.kicad_pcb" );
    tagChain( board.get(), wxS( "TSIG" ), wxS( "FP_START" ), wxS( "FP_END" ) );

    auto items = chainItems( board.get(), wxS( "TSIG" ) );
    CHAIN_TOPOLOGY topo( board.get(), wxS( "TSIG" ), items );

    BOOST_CHECK( topo.IsValid() );
    BOOST_REQUIRE_EQUAL( topo.Stubs().size(), 1u );

    const CHAIN_TOPOLOGY::STUB& stub = topo.Stubs().front();
    BOOST_CHECK_LE( std::abs( stub.branchPoint.x - 25 * MM ), 100 );
    BOOST_CHECK_LE( std::abs( stub.branchPoint.y ), 100 );
    BOOST_CHECK_CLOSE( stub.length, 5.0 * MM, 5.0 );

    // Trunk length is 50 mm and the stub does not contribute.
    BOOST_CHECK_CLOSE( topo.TrunkLength(), 50.0 * MM, 5.0 );
}


// Only one terminal pad set: NO_TERMINAL_PADS.
BOOST_AUTO_TEST_CASE( TopologyMissingTerminalPad )
{
    auto board = loadBoard( "net_chains/chain_topology_missing_terminal.kicad_pcb" );
    // Only set chain — no second terminal-pad anchor footprint.
    for( NETINFO_ITEM* n : board->GetNetInfo() )
    {
        if( n && n->GetNetname().StartsWith( wxS( "/NET_" ) ) )
            n->SetNetChain( wxS( "MISSING" ) );
    }

    PAD* termA = board->Footprints().empty()
                         ? nullptr
                         : board->Footprints().front()->Pads().empty()
                                   ? nullptr
                                   : board->Footprints().front()->Pads().front();

    if( termA )
    {
        for( NETINFO_ITEM* n : board->GetNetInfo() )
            if( n && n->GetNetChain() == wxS( "MISSING" ) )
                n->SetTerminalPad( 0, termA );
    }

    auto items = chainItems( board.get(), wxS( "MISSING" ) );
    CHAIN_TOPOLOGY topo( board.get(), wxS( "MISSING" ), items );

    BOOST_CHECK_EQUAL( static_cast<int>( topo.GetStatus() ),
                       static_cast<int>( CHAIN_TOPOLOGY::STATUS::NO_TERMINAL_PADS ) );
}


// Both terminals set, no track between them: DISCONNECTED.
BOOST_AUTO_TEST_CASE( TopologyDisconnected )
{
    auto board = loadBoard( "net_chains/chain_topology_disconnected.kicad_pcb" );
    tagChain( board.get(), wxS( "DISC" ), wxS( "FP_START" ), wxS( "FP_END" ) );

    auto items = chainItems( board.get(), wxS( "DISC" ) );
    CHAIN_TOPOLOGY topo( board.get(), wxS( "DISC" ), items );

    BOOST_CHECK_EQUAL( static_cast<int>( topo.GetStatus() ),
                       static_cast<int>( CHAIN_TOPOLOGY::STATUS::DISCONNECTED ) );
}


// Two parallel paths between terminals → CYCLE_DETECTED.
BOOST_AUTO_TEST_CASE( TopologyCycleDetected )
{
    auto board = loadBoard( "net_chains/chain_topology_cycle.kicad_pcb" );
    tagChain( board.get(), wxS( "LOOP" ), wxS( "FP_START" ), wxS( "FP_END" ) );

    auto items = chainItems( board.get(), wxS( "LOOP" ) );
    CHAIN_TOPOLOGY topo( board.get(), wxS( "LOOP" ), items );

    BOOST_CHECK_EQUAL( static_cast<int>( topo.GetStatus() ),
                       static_cast<int>( CHAIN_TOPOLOGY::STATUS::CYCLE_DETECTED ) );
}


BOOST_AUTO_TEST_SUITE_END()
