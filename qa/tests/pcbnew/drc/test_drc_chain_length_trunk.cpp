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

#include <board.h>
#include <board_design_settings.h>
#include <drc/drc_chain_topology.h>
#include <drc/drc_engine.h>
#include <drc/drc_item.h>
#include <footprint.h>
#include <netinfo.h>
#include <pad.h>
#include <pcb_marker.h>
#include <pcb_track.h>
#include <pcbnew/pcb_io/kicad_sexpr/pcb_io_kicad_sexpr.h>


// Two-net daisy chain through a single bridge, terminals set.  The trunk
// length should equal the sum of both routed segments + the bridge span.
// A `(constraint net_chain_length (max 80mm))` rule passes when trunk == 50 mm.
static const char* DAISY_PCB_FILE = "net_chains/chain_length_daisy.kicad_pcb";


// Same trunk as the daisy fixture but adding three perpendicular branches off the trunk
// (T-junctions in the routed copper) all carrying the same chain so the trunk
// stays unaffected — provides a regression check that branches don't add to
// the trunk length when terminals are set.
static const char* BRANCHED_PCB_FILE = "net_chains/chain_length_branched.kicad_pcb";


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

// Tag every "/NET_*" net into the named chain.  Terminal pads come from the
// footprints whose anchor matches the given X positions (in mm) — the fixture
// boards don't set explicit Reference properties.
void tagAndSetTerminals( BOARD* aBoard, const wxString& aChain,
                         double aTermAxMm, double aTermBxMm )
{
    for( NETINFO_ITEM* n : aBoard->GetNetInfo() )
    {
        if( n && n->GetNetname().StartsWith( wxS( "/NET_" ) ) )
            n->SetNetChain( aChain );
    }

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

}  // namespace


BOOST_AUTO_TEST_SUITE( DRCChainLengthTrunk )


// Two-net daisy with terminals: trunk = (20 + 25 + 5 bridge) = 50 mm.
BOOST_AUTO_TEST_CASE( DaisyChainTrunkEqualsSumExplicit )
{
    auto board = loadBoard( DAISY_PCB_FILE );
    tagAndSetTerminals( board.get(), wxS( "DSY" ), 0.0, 50.0 );

    std::set<BOARD_CONNECTED_ITEM*> items;

    for( PCB_TRACK* t : board->Tracks() )
    {
        if( t->GetNet() && t->GetNet()->GetNetChain() == wxS( "DSY" ) )
            items.insert( t );
    }

    for( FOOTPRINT* fp : board->Footprints() )
    {
        for( PAD* p : fp->Pads() )
        {
            if( p->GetNet() && p->GetNet()->GetNetChain() == wxS( "DSY" ) )
                items.insert( p );
        }
    }

    CHAIN_TOPOLOGY topo( board.get(), wxS( "DSY" ), items );

    BOOST_REQUIRE( topo.IsValid() );
    BOOST_CHECK_CLOSE( topo.TrunkLength(), 50.0e6, 5.0 );
}


// Trunk + three perpendicular branches: trunk should be 50 mm, branches
// reported as stubs (3 of them).
BOOST_AUTO_TEST_CASE( BranchedChainTrunkExcludesStubs )
{
    auto board = loadBoard( BRANCHED_PCB_FILE );
    tagAndSetTerminals( board.get(), wxS( "BR" ), 0.0, 50.0 );

    std::set<BOARD_CONNECTED_ITEM*> items;

    for( PCB_TRACK* t : board->Tracks() )
    {
        if( t->GetNet() && t->GetNet()->GetNetChain() == wxS( "BR" ) )
            items.insert( t );
    }

    for( FOOTPRINT* fp : board->Footprints() )
    {
        for( PAD* p : fp->Pads() )
        {
            if( p->GetNet() && p->GetNet()->GetNetChain() == wxS( "BR" ) )
                items.insert( p );
        }
    }

    CHAIN_TOPOLOGY topo( board.get(), wxS( "BR" ), items );

    BOOST_REQUIRE( topo.IsValid() );
    BOOST_CHECK_CLOSE( topo.TrunkLength(), 50.0e6, 5.0 );
    BOOST_CHECK_EQUAL( topo.Stubs().size(), 3u );
}


BOOST_AUTO_TEST_SUITE_END()
