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
 */

#include <boost/test/unit_test.hpp>

#include <filesystem>
#include <fstream>

#include <base_units.h>
#include <board.h>
#include <drc/drc_chain_topology.h>
#include <footprint.h>
#include <net_chain_bridging.h>
#include <netinfo.h>
#include <pad.h>
#include <pcb_track.h>
#include <pcbnew/pcb_io/kicad_sexpr/pcb_io_kicad_sexpr.h>


// Two-net daisy chain through a single bridge.  Trunk = 20 mm (net A track) +
// 5 mm (bridge pad-to-pad span) + 25 mm (net B track) = 50 mm.  The board has
// no stackup so the per-track length-delay calculator returns a known-type
// item with zero delay (trackDelay() does not fall back when Type != UNKNOWN);
// only the bridge edge carries delay, derived from the chain-wide fallback
// 5.9 ps/mm.  This pins the trunk-delay equal-to-bridge contract.
static const char* DAISY_PCB = R"(
(kicad_pcb
    (version 20250904)
    (generator "pcbnew")
    (generator_version "9.99")
    (layers
        (0 "F.Cu" signal)
        (2 "B.Cu" signal)
        (44 "Edge.Cuts" user)
    )
    (net 0 "")
    (net 1 "/NET_A")
    (net 2 "/NET_B")
    (gr_line (start -5 -5) (end 60 -5) (layer "Edge.Cuts") (width 0.05))
    (gr_line (start 60 -5) (end 60 5) (layer "Edge.Cuts") (width 0.05))
    (gr_line (start 60 5) (end -5 5) (layer "Edge.Cuts") (width 0.05))
    (gr_line (start -5 5) (end -5 -5) (layer "Edge.Cuts") (width 0.05))
    (footprint "Term1" (layer "F.Cu") (uuid "00000000-0000-0000-0000-000000000a01")
        (at 0 0)
        (pad "1" smd rect (at 0 0) (size 0.8 0.8) (layers "F.Cu") (net 1 "/NET_A") (uuid "00000000-0000-0000-0000-000000000a02"))
    )
    (footprint "Bridge" (layer "F.Cu") (uuid "00000000-0000-0000-0000-000000000b01")
        (at 22.5 0)
        (pad "1" smd rect (at -2.5 0) (size 0.8 0.8) (layers "F.Cu") (net 1 "/NET_A") (uuid "00000000-0000-0000-0000-000000000b02"))
        (pad "2" smd rect (at  2.5 0) (size 0.8 0.8) (layers "F.Cu") (net 2 "/NET_B") (uuid "00000000-0000-0000-0000-000000000b03"))
    )
    (footprint "Term2" (layer "F.Cu") (uuid "00000000-0000-0000-0000-000000000c01")
        (at 50 0)
        (pad "1" smd rect (at 0 0) (size 0.8 0.8) (layers "F.Cu") (net 2 "/NET_B") (uuid "00000000-0000-0000-0000-000000000c02"))
    )
    (segment (start 0 0) (end 20 0) (width 0.2) (layer "F.Cu") (net 1))
    (segment (start 25 0) (end 50 0) (width 0.2) (layer "F.Cu") (net 2))
)
)";


namespace
{
std::unique_ptr<BOARD> loadBoard( const char* aText, const std::string& aSubdir )
{
    namespace fs = std::filesystem;
    fs::path tmpDir = fs::temp_directory_path() / aSubdir;
    fs::create_directories( tmpDir );
    fs::path pcbPath = tmpDir / "trunk_delay.kicad_pcb";

    {
        std::ofstream out( pcbPath );
        out << aText;
    }

    PCB_IO_KICAD_SEXPR     plugin;
    std::unique_ptr<BOARD> board = std::make_unique<BOARD>();
    plugin.LoadBoard( pcbPath.string(), board.get() );
    board->BuildConnectivity();
    fs::remove( pcbPath );
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


void setTerminals( BOARD* aBoard, const wxString& aChain, double aTermAxMm, double aTermBxMm )
{
    PAD* termA = nullptr;
    PAD* termB = nullptr;
    constexpr int  EPS = 100;
    const VECTOR2I targetA( static_cast<int>( aTermAxMm * 1000000 ), 0 );
    const VECTOR2I targetB( static_cast<int>( aTermBxMm * 1000000 ), 0 );

    for( FOOTPRINT* fp : aBoard->Footprints() )
    {
        if( fp->Pads().empty() )
            continue;

        VECTOR2I pos = fp->GetPosition();

        if( std::abs( pos.x - targetA.x ) <= EPS && std::abs( pos.y - targetA.y ) <= EPS )
            termA = fp->Pads().front();

        if( std::abs( pos.x - targetB.x ) <= EPS && std::abs( pos.y - targetB.y ) <= EPS )
            termB = fp->Pads().front();
    }

    for( NETINFO_ITEM* n : aBoard->GetNetInfo() )
    {
        if( n && n->GetNetChain() == aChain )
        {
            if( termA )
                n->SetTerminalPad( 0, termA );
            if( termB )
                n->SetTerminalPad( 1, termB );
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
    auto board = loadBoard( DAISY_PCB, "kicad_chain_trunk_delay" );
    tagChainNets( board.get(), wxS( "DSY" ) );
    setTerminals( board.get(), wxS( "DSY" ), 0.0, 50.0 );

    CHAIN_TOPOLOGY topo( board.get(), wxS( "DSY" ),
                         collectChainItems( board.get(), wxS( "DSY" ) ) );

    BOOST_REQUIRE( topo.IsValid() );
    BOOST_CHECK_CLOSE( topo.TrunkLength(), 50.0e6, 5.0 );

    double expectedDelayIU = DEFAULT_PROPAGATION_DELAY_PS_PER_MM * pcbIUScale.IU_PER_PS * 5.0;
    BOOST_CHECK_CLOSE( topo.TrunkDelay(), expectedDelayIU, 5.0 );
    BOOST_CHECK_GT( topo.TrunkDelay(), 0.0 );

    // The bridging-only helper should agree with what the trunk picked up.
    auto [bridgingLen, bridgingDelay] = BoardChainBridging( board.get(), wxS( "DSY" ) );
    BOOST_CHECK_CLOSE( bridgingDelay, topo.TrunkDelay(), 0.001 );
    BOOST_CHECK_CLOSE( bridgingLen, 5.0e6, 0.001 );
}


// Without terminal pads the topology cannot reduce to a trunk; callers must
// fall back to BoardChainBridging.  Verify both: the topology is not valid,
// and the bridging-only delay covers the 5 mm cross-net pad span at the
// fallback per-mm rate.
BOOST_AUTO_TEST_CASE( NoTerminalsFallbackUsesBridgingDelay )
{
    auto board = loadBoard( DAISY_PCB, "kicad_chain_trunk_delay_noterms" );
    tagChainNets( board.get(), wxS( "DSY" ) );
    // Intentionally do NOT call setTerminals.

    CHAIN_TOPOLOGY topo( board.get(), wxS( "DSY" ),
                         collectChainItems( board.get(), wxS( "DSY" ) ) );

    BOOST_CHECK( !topo.IsValid() );
    BOOST_CHECK_EQUAL( static_cast<int>( topo.GetStatus() ),
                       static_cast<int>( CHAIN_TOPOLOGY::STATUS::NO_TERMINAL_PADS ) );

    auto [bridgingLen, bridgingDelay] = BoardChainBridging( board.get(), wxS( "DSY" ) );

    BOOST_CHECK_CLOSE( bridgingLen, 5.0e6, 0.001 );

    double expectedDelayIU = DEFAULT_PROPAGATION_DELAY_PS_PER_MM * pcbIUScale.IU_PER_PS * 5.0;
    BOOST_CHECK_CLOSE( bridgingDelay, expectedDelayIU, 0.001 );
}


BOOST_AUTO_TEST_SUITE_END()
