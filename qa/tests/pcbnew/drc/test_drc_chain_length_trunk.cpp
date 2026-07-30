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
#include <drc/drc_chain_topology.h>
#include <footprint.h>
#include <netinfo.h>
#include <pad.h>
#include <pcb_track.h>
#include <pcbnew/pcb_io/kicad_sexpr/pcb_io_kicad_sexpr.h>


// Two-net daisy through a single bridge; trunk is both routed runs plus the
// cross-net pad span: 21.55 + 1.9 + 26.55 = 50 mm
static const char* DAISY_PCB_FILE = "net_chains/chain_length_daisy.kicad_pcb";


// Same 50 mm trunk as the daisy fixture with three perpendicular branches
// tapped off it, all on the same chain, so stubs must not reach the trunk sum
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

// Anchor terminals at the extreme-X footprints; the fixtures route in a single
// row and carry no Reference strings to match on
void tagAndSetTerminals( BOARD* aBoard, const wxString& aChain )
{
    for( NETINFO_ITEM* n : aBoard->GetNetInfo() )
    {
        if( n && n->GetNetname().StartsWith( wxS( "/NET_" ) ) )
            n->SetNetChain( aChain );
    }

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

}  // namespace


BOOST_AUTO_TEST_SUITE( DRCChainLengthTrunk )


BOOST_AUTO_TEST_CASE( DaisyChainTrunkEqualsSumExplicit )
{
    auto board = loadBoard( DAISY_PCB_FILE );
    tagAndSetTerminals( board.get(), wxS( "DSY" ) );

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


BOOST_AUTO_TEST_CASE( BranchedChainTrunkExcludesStubs )
{
    auto board = loadBoard( BRANCHED_PCB_FILE );
    tagAndSetTerminals( board.get(), wxS( "BR" ) );

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
