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


// Same trunk as DAISY_PCB but adding three perpendicular branches off the trunk
// (T-junctions in the routed copper) all carrying the same chain so the trunk
// stays unaffected — provides a regression check that branches don't add to
// the trunk length when terminals are set.
static const char* BRANCHED_PCB = R"(
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
    (gr_line (start -5 -25) (end 60 -25) (layer "Edge.Cuts") (width 0.05))
    (gr_line (start 60 -25) (end 60 25) (layer "Edge.Cuts") (width 0.05))
    (gr_line (start 60 25) (end -5 25) (layer "Edge.Cuts") (width 0.05))
    (gr_line (start -5 25) (end -5 -25) (layer "Edge.Cuts") (width 0.05))
    (footprint "Term1" (layer "F.Cu") (uuid "00000000-0000-0000-0000-000000000d01")
        (at 0 0)
        (pad "1" smd rect (at 0 0) (size 0.8 0.8) (layers "F.Cu") (net 1 "/NET_A") (uuid "00000000-0000-0000-0000-000000000d02"))
    )
    (footprint "Term2" (layer "F.Cu") (uuid "00000000-0000-0000-0000-000000000e01")
        (at 50 0)
        (pad "1" smd rect (at 0 0) (size 0.8 0.8) (layers "F.Cu") (net 1 "/NET_A") (uuid "00000000-0000-0000-0000-000000000e02"))
    )
    (segment (start 0 0) (end 50 0) (width 0.2) (layer "F.Cu") (net 1))
    (segment (start 12.5 0) (end 12.5 15) (width 0.2) (layer "F.Cu") (net 1))
    (segment (start 25 0) (end 25 -20) (width 0.2) (layer "F.Cu") (net 1))
    (segment (start 37.5 0) (end 37.5 18) (width 0.2) (layer "F.Cu") (net 1))
)
)";


namespace
{
std::unique_ptr<BOARD> loadBoard( const char* aText, const std::string& aSubdir )
{
    namespace fs = std::filesystem;
    fs::path tmpDir = fs::temp_directory_path() / aSubdir;
    fs::create_directories( tmpDir );
    fs::path pcbPath = tmpDir / "trunk.kicad_pcb";

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

// Tag every "/NET_*" net into the named chain.  Terminal pads come from the
// footprints whose anchor matches the given X positions (in mm) — the inline
// PCB strings here don't set explicit Reference properties.
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
    auto board = loadBoard( DAISY_PCB, "kicad_drc_trunk_daisy" );
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
    auto board = loadBoard( BRANCHED_PCB, "kicad_drc_trunk_branched" );
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
