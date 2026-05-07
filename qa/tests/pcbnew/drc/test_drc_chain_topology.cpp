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

// Header that produces a minimal layer set sufficient for the test fixtures.
constexpr const char* PCB_HEADER = R"(
(kicad_pcb
    (version 20250904)
    (generator "pcbnew")
    (generator_version "9.99")
    (layers
        (0 "F.Cu" signal)
        (2 "B.Cu" signal)
        (44 "Edge.Cuts" user)
    )
)";

constexpr int MM = 1000000;


std::unique_ptr<BOARD> loadFromString( const std::string& aPcbText,
                                       const std::string& aSubdir )
{
    namespace fs = std::filesystem;
    fs::path tmpDir = fs::temp_directory_path() / aSubdir;
    fs::create_directories( tmpDir );
    fs::path pcbPath = tmpDir / "topo.kicad_pcb";

    {
        std::ofstream out( pcbPath );
        out << aPcbText;
    }

    PCB_IO_KICAD_SEXPR     plugin;
    std::unique_ptr<BOARD> board = std::make_unique<BOARD>();
    plugin.LoadBoard( pcbPath.string(), board.get() );
    board->BuildConnectivity();

    fs::remove( pcbPath );
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


// Footprint stamper for SMD passives.  Each pad on its own net.  Pad-net
// references use the (net N "name") format the modern parser expects.
std::string passive( const wxString& aRef, double aXmm, double aYmm,
                     double aPadSpanMm,
                     int aNetA, const wxString& aNetAName,
                     int aNetB, const wxString& aNetBName )
{
    return wxString::Format(
            R"(
    (footprint "Passive" (layer "F.Cu") (uuid "00000000-0000-0000-0000-000000%06d")
        (at %f %f)
        (property "Reference" "%s" (at 0 -1) (layer "F.Cu") (hide yes) (uuid "00000000-0000-0000-0000-000000%06da") (effects (font (size 1 1) (thickness 0.15))))
        (pad "1" smd rect (at %f 0) (size 0.8 0.8) (layers "F.Cu") (net %d "%s") (uuid "00000000-0000-0000-0000-000000%06d1"))
        (pad "2" smd rect (at %f 0) (size 0.8 0.8) (layers "F.Cu") (net %d "%s") (uuid "00000000-0000-0000-0000-000000%06d2"))
    )
)", static_cast<int>( aXmm ), aXmm, aYmm, aRef, static_cast<int>( aXmm ),
   -aPadSpanMm / 2, aNetA, aNetAName, static_cast<int>( aXmm ),
   aPadSpanMm / 2, aNetB, aNetBName, static_cast<int>( aXmm ) ).ToStdString();
}


std::string segment( double aX1mm, double aY1mm, double aX2mm, double aY2mm, int aNet )
{
    return wxString::Format(
            "    (segment (start %f %f) (end %f %f) (width 0.2) (layer \"F.Cu\") (net %d))\n",
            aX1mm, aY1mm, aX2mm, aY2mm, aNet ).ToStdString();
}

}  // namespace


BOOST_AUTO_TEST_SUITE( DRCChainTopology )


// Three-net trunk through two passives.
// pad@(0,0) — track 30mm — bridge 5mm — track 30mm — bridge 5mm — track 30mm — pad@(100,0).
// Trunk = 100 mm, zero stubs.
BOOST_AUTO_TEST_CASE( TopologyTreeOnSimpleTrunk )
{
    std::string pcb = std::string( PCB_HEADER ) +
            R"(
    (net 0 "")
    (net 1 "/NET_A")
    (net 2 "/NET_B")
    (net 3 "/NET_C")
    (gr_line (start -5 -5) (end 110 -5) (layer "Edge.Cuts") (width 0.05))
    (gr_line (start 110 -5) (end 110 5) (layer "Edge.Cuts") (width 0.05))
    (gr_line (start 110 5) (end -5 5) (layer "Edge.Cuts") (width 0.05))
    (gr_line (start -5 5) (end -5 -5) (layer "Edge.Cuts") (width 0.05))
)" +
            passive( wxS( "FP_START" ), 0.0, 0.0, 0.0,
                     1, wxS( "/NET_A" ), 1, wxS( "/NET_A" ) ) +
            passive( wxS( "R1" ), 32.5, 0.0, 5.0,
                     1, wxS( "/NET_A" ), 2, wxS( "/NET_B" ) ) +
            passive( wxS( "R2" ), 67.5, 0.0, 5.0,
                     2, wxS( "/NET_B" ), 3, wxS( "/NET_C" ) ) +
            passive( wxS( "FP_END" ), 100.0, 0.0, 0.0,
                     3, wxS( "/NET_C" ), 3, wxS( "/NET_C" ) ) +
            segment( 0.0, 0.0, 30.0, 0.0, 1 ) +
            segment( 35.0, 0.0, 65.0, 0.0, 2 ) +
            segment( 70.0, 0.0, 100.0, 0.0, 3 ) +
            "\n)";

    auto board = loadFromString( pcb, "kicad_drc_topo_simple_trunk" );
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
    std::string pcb = std::string( PCB_HEADER ) +
            R"(
    (net 0 "")
    (net 1 "/NET_A")
    (gr_line (start -5 -10) (end 60 -10) (layer "Edge.Cuts") (width 0.05))
    (gr_line (start 60 -10) (end 60 15) (layer "Edge.Cuts") (width 0.05))
    (gr_line (start 60 15) (end -5 15) (layer "Edge.Cuts") (width 0.05))
    (gr_line (start -5 15) (end -5 -10) (layer "Edge.Cuts") (width 0.05))
)" +
            passive( wxS( "FP_START" ), 0.0, 0.0, 0.0,
                     1, wxS( "/NET_A" ), 1, wxS( "/NET_A" ) ) +
            passive( wxS( "FP_END" ), 50.0, 0.0, 0.0,
                     1, wxS( "/NET_A" ), 1, wxS( "/NET_A" ) ) +
            // Trunk 0..50 along y=0; stub branches off at x=25 going to (25,5).
            segment( 0.0, 0.0, 50.0, 0.0, 1 ) +
            segment( 25.0, 0.0, 25.0, 5.0, 1 ) +
            "\n)";

    auto board = loadFromString( pcb, "kicad_drc_topo_t_stub" );
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
    std::string pcb = std::string( PCB_HEADER ) +
            R"(
    (net 0 "")
    (net 1 "/NET_A")
    (gr_line (start -5 -5) (end 40 -5) (layer "Edge.Cuts") (width 0.05))
    (gr_line (start 40 -5) (end 40 5) (layer "Edge.Cuts") (width 0.05))
    (gr_line (start 40 5) (end -5 5) (layer "Edge.Cuts") (width 0.05))
    (gr_line (start -5 5) (end -5 -5) (layer "Edge.Cuts") (width 0.05))
)" +
            passive( wxS( "FP_START" ), 0.0, 0.0, 0.0,
                     1, wxS( "/NET_A" ), 1, wxS( "/NET_A" ) ) +
            segment( 0.0, 0.0, 30.0, 0.0, 1 ) +
            "\n)";

    auto board = loadFromString( pcb, "kicad_drc_topo_missing_term" );
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
    std::string pcb = std::string( PCB_HEADER ) +
            R"(
    (net 0 "")
    (net 1 "/NET_A")
    (gr_line (start -5 -5) (end 110 -5) (layer "Edge.Cuts") (width 0.05))
    (gr_line (start 110 -5) (end 110 5) (layer "Edge.Cuts") (width 0.05))
    (gr_line (start 110 5) (end -5 5) (layer "Edge.Cuts") (width 0.05))
    (gr_line (start -5 5) (end -5 -5) (layer "Edge.Cuts") (width 0.05))
)" +
            passive( wxS( "FP_START" ), 0.0, 0.0, 0.0,
                     1, wxS( "/NET_A" ), 1, wxS( "/NET_A" ) ) +
            passive( wxS( "FP_END" ), 100.0, 0.0, 0.0,
                     1, wxS( "/NET_A" ), 1, wxS( "/NET_A" ) ) +
            "\n)";

    auto board = loadFromString( pcb, "kicad_drc_topo_disconnected" );
    tagChain( board.get(), wxS( "DISC" ), wxS( "FP_START" ), wxS( "FP_END" ) );

    auto items = chainItems( board.get(), wxS( "DISC" ) );
    CHAIN_TOPOLOGY topo( board.get(), wxS( "DISC" ), items );

    BOOST_CHECK_EQUAL( static_cast<int>( topo.GetStatus() ),
                       static_cast<int>( CHAIN_TOPOLOGY::STATUS::DISCONNECTED ) );
}


// Two parallel paths between terminals → CYCLE_DETECTED.
BOOST_AUTO_TEST_CASE( TopologyCycleDetected )
{
    std::string pcb = std::string( PCB_HEADER ) +
            R"(
    (net 0 "")
    (net 1 "/NET_A")
    (gr_line (start -5 -10) (end 60 -10) (layer "Edge.Cuts") (width 0.05))
    (gr_line (start 60 -10) (end 60 10) (layer "Edge.Cuts") (width 0.05))
    (gr_line (start 60 10) (end -5 10) (layer "Edge.Cuts") (width 0.05))
    (gr_line (start -5 10) (end -5 -10) (layer "Edge.Cuts") (width 0.05))
)" +
            passive( wxS( "FP_START" ), 0.0, 0.0, 0.0,
                     1, wxS( "/NET_A" ), 1, wxS( "/NET_A" ) ) +
            passive( wxS( "FP_END" ), 50.0, 0.0, 0.0,
                     1, wxS( "/NET_A" ), 1, wxS( "/NET_A" ) ) +
            // Two routes start→end forming a loop.
            segment( 0.0, 0.0, 25.0, 5.0, 1 ) +
            segment( 25.0, 5.0, 50.0, 0.0, 1 ) +
            segment( 0.0, 0.0, 25.0, -5.0, 1 ) +
            segment( 25.0, -5.0, 50.0, 0.0, 1 ) +
            "\n)";

    auto board = loadFromString( pcb, "kicad_drc_topo_cycle" );
    tagChain( board.get(), wxS( "LOOP" ), wxS( "FP_START" ), wxS( "FP_END" ) );

    auto items = chainItems( board.get(), wxS( "LOOP" ) );
    CHAIN_TOPOLOGY topo( board.get(), wxS( "LOOP" ), items );

    BOOST_CHECK_EQUAL( static_cast<int>( topo.GetStatus() ),
                       static_cast<int>( CHAIN_TOPOLOGY::STATUS::CYCLE_DETECTED ) );
}


BOOST_AUTO_TEST_SUITE_END()
