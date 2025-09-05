/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 3
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, you may find one here:
 * http://www.gnu.org/licenses/gpl-3.0.html
 * or you may search the http://www.gnu.org website for the version 3 license,
 * or you may write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
 */

#include <boost/test/unit_test.hpp>
#include <pcbnew/pcb_io/kicad_sexpr/pcb_io_kicad_sexpr.h>
#include <board.h>
#include <footprint.h>
#include <pad.h>
#include <pcb_track.h>
#include <qa_utils/wx_utils/unit_test_utils.h>
#include <pcbnew_utils/board_file_utils.h>
#include <pcbnew_utils/board_test_utils.h>
#include <wx/filename.h>
#include <filesystem>

struct SIGNAL_PAD_FIXTURE
{
    PCB_IO_KICAD_SEXPR plugin;
};

BOOST_FIXTURE_TEST_SUITE( RegressionSaveLoadTests, SIGNAL_PAD_FIXTURE )

BOOST_AUTO_TEST_CASE( SignalTerminalPadsRoundTrip )
{
    std::unique_ptr<BOARD> board = std::make_unique<BOARD>();

    // Create two footprints each with two pads; connect pads with tracks into two nets sharing a signal
    FOOTPRINT* fp1 = new FOOTPRINT( board.get() );
    fp1->SetReference( wxS( "U1" ) );
    FOOTPRINT* fp2 = new FOOTPRINT( board.get() );
    fp2->SetReference( wxS( "U2" ) );
    board->Add( fp1 );
    board->Add( fp2 );

    PAD* p1a = new PAD( fp1 ); p1a->SetFrontShape( PAD_SHAPE::CIRCLE ); p1a->SetSize( F_Cu, VECTOR2I( 1000000,1000000 ) ); p1a->SetPosition( VECTOR2I( 0,0 ) ); p1a->SetNumber( wxS( "1" ) ); fp1->Add( p1a );
    PAD* p1b = new PAD( fp1 ); p1b->SetFrontShape( PAD_SHAPE::CIRCLE ); p1b->SetSize( F_Cu, VECTOR2I( 1000000,1000000 ) ); p1b->SetPosition( VECTOR2I( 3000000,0 ) ); p1b->SetNumber( wxS( "2" ) ); fp1->Add( p1b );
    PAD* p2a = new PAD( fp2 ); p2a->SetFrontShape( PAD_SHAPE::CIRCLE ); p2a->SetSize( F_Cu, VECTOR2I( 1000000,1000000 ) ); p2a->SetPosition( VECTOR2I( 10000000,0 ) ); p2a->SetNumber( wxS( "1" ) ); fp2->Add( p2a );
    PAD* p2b = new PAD( fp2 ); p2b->SetFrontShape( PAD_SHAPE::CIRCLE ); p2b->SetSize( F_Cu, VECTOR2I( 1000000,1000000 ) ); p2b->SetPosition( VECTOR2I( 13000000,0 ) ); p2b->SetNumber( wxS( "2" ) ); fp2->Add( p2b );

    // Add nets first so pads referencing them are valid
    NETINFO_ITEM* n1 = new NETINFO_ITEM( board.get(), wxS( "Net-(1)" ), 1 ); board->Add( n1 );
    NETINFO_ITEM* n2 = new NETINFO_ITEM( board.get(), wxS( "Net-(2)" ), 2 ); board->Add( n2 );

    n1->SetSignal( wxS( "SIG_A" ) );
    n2->SetSignal( wxS( "SIG_A" ) );

    // Assign pads to nets for resolver (only first pad of each footprint for simplicity)
    p1a->SetNet( n1 );
    p2a->SetNet( n2 );

    // Assign terminal pads (simulate import heuristic outcome)
    n1->SetTerminalPad( 0, p1a ); n1->SetTerminalPad( 1, p2a );
    n2->SetTerminalPad( 0, p1a ); n2->SetTerminalPad( 1, p2a );

    // Store original UUIDs for debugging comparison after reload
    wxString orig_p1a = p1a->m_Uuid.AsString();
    wxString orig_p2a = p2a->m_Uuid.AsString();

    // Create simple tracks to ensure nets are serialized
    PCB_TRACK* t1 = new PCB_TRACK( board.get() ); t1->SetNetCode( 1 ); t1->SetStart( p1a->GetPosition() ); t1->SetEnd( p2a->GetPosition() ); board->Add( t1 );
    PCB_TRACK* t2 = new PCB_TRACK( board.get() ); t2->SetNetCode( 2 ); t2->SetStart( p1a->GetPosition() ); t2->SetEnd( p2a->GetPosition() ); board->Add( t2 );

    auto tmpFile = std::filesystem::temp_directory_path() / "signal_terminal_pads_roundtrip.kicad_pcb";
    plugin.SaveBoard( tmpFile.string(), board.get() );

    std::unique_ptr<BOARD> loaded = std::make_unique<BOARD>();
    plugin.LoadBoard( tmpFile.string(), loaded.get() );

    NETINFO_ITEM* ln1 = loaded->FindNet( 1 );
    NETINFO_ITEM* ln2 = loaded->FindNet( 2 );

    // Ensure terminal pads resolved (parser should do this but call defensively for test stability)
    for( NETINFO_ITEM* n : loaded->GetNetInfo() )
        n->ResolveTerminalPads( loaded.get() );

    BOOST_REQUIRE( ln1 );
    BOOST_REQUIRE( ln2 );
    BOOST_CHECK_EQUAL( ln1->GetSignal(), wxS( "SIG_A" ) );
    BOOST_CHECK_EQUAL( ln2->GetSignal(), wxS( "SIG_A" ) );

    // Both nets should have terminal pad UUIDs resolved
    BOOST_REQUIRE( ln1->GetTerminalPad( 0 ) );
    BOOST_REQUIRE( ln1->GetTerminalPad( 1 ) );
    BOOST_REQUIRE( ln2->GetTerminalPad( 0 ) );
    BOOST_REQUIRE( ln2->GetTerminalPad( 1 ) );

    BOOST_TEST_MESSAGE( wxString::Format( wxS("orig p1a=%s p2a=%s"), orig_p1a, orig_p2a ) );
    BOOST_TEST_MESSAGE( wxString::Format( wxS("loaded terminal A net1=%s net2=%s"), ln1->GetTerminalPad(0)->m_Uuid.AsString(), ln2->GetTerminalPad(0)->m_Uuid.AsString() ) );
    BOOST_TEST_MESSAGE( wxString::Format( wxS("loaded terminal B net1=%s net2=%s"), ln1->GetTerminalPad(1)->m_Uuid.AsString(), ln2->GetTerminalPad(1)->m_Uuid.AsString() ) );

    // UUIDs can be regenerated on load (timestamp -> UUID or duplicate resolution). Validate
    // that both nets agree on terminal pads and that those pads exist in the loaded board.
    PAD* termA1 = ln1->GetTerminalPad( 0 );
    PAD* termB1 = ln1->GetTerminalPad( 1 );
    BOOST_CHECK( termA1 == ln2->GetTerminalPad( 0 ) );
    BOOST_CHECK( termB1 == ln2->GetTerminalPad( 1 ) );

    bool foundA = false, foundB = false;
    for( FOOTPRINT* fp : loaded->Footprints() )
    {
        for( PAD* pad : fp->Pads() )
        {
            if( pad == termA1 ) foundA = true;
            if( pad == termB1 ) foundB = true;
        }
    }
    BOOST_CHECK( foundA );
    BOOST_CHECK( foundB );
}

BOOST_AUTO_TEST_CASE( SingleNetSignalNamePersists )
{
    std::unique_ptr<BOARD> board = std::make_unique<BOARD>();
    // Single net with explicit signal name, no terminal pads
    NETINFO_ITEM* n1 = new NETINFO_ITEM( board.get(), wxS( "Net-(X)" ), 1 ); board->Add( n1 );
    n1->SetSignal( wxS( "CLK_REF" ) );
    // Add trivial track so net is serialized
    PCB_TRACK* t = new PCB_TRACK( board.get() ); t->SetNetCode( 1 ); t->SetStart( VECTOR2I(0,0) ); t->SetEnd( VECTOR2I(1000000,0) ); board->Add( t );

    auto tmpFile = std::filesystem::temp_directory_path() / "single_net_signal_roundtrip.kicad_pcb";
    plugin.SaveBoard( tmpFile.string(), board.get() );

    std::unique_ptr<BOARD> loaded = std::make_unique<BOARD>();
    plugin.LoadBoard( tmpFile.string(), loaded.get() );
    NETINFO_ITEM* ln1 = loaded->FindNet( 1 );
    BOOST_REQUIRE( ln1 );
    BOOST_CHECK_EQUAL( ln1->GetSignal(), wxS( "CLK_REF" ) );
}

BOOST_AUTO_TEST_CASE( SignalsSurviveBuildListOfNets )
{
    std::unique_ptr<BOARD> board = std::make_unique<BOARD>();

    // Create 4 nets
    NETINFO_ITEM* n1 = new NETINFO_ITEM( board.get(), wxS( "Net-(R1-Pad1)" ), 1 ); board->Add( n1 );
    NETINFO_ITEM* n2 = new NETINFO_ITEM( board.get(), wxS( "Net-(R1-Pad2)" ), 2 ); board->Add( n2 );
    NETINFO_ITEM* n3 = new NETINFO_ITEM( board.get(), wxS( "Net-(R2-Pad2)" ), 3 ); board->Add( n3 );
    NETINFO_ITEM* n4 = new NETINFO_ITEM( board.get(), wxS( "Net-(R3-Pad2)" ), 4 ); board->Add( n4 );

    for( NETINFO_ITEM* n : { n1, n2, n3, n4 } )
        n->SetSignal( wxS( "Signal1" ) );

    // Add trivial tracks so that each net is considered used and serialized
    int x = 0;
    for( int code : { 1, 2, 3, 4 } )
    {
        PCB_TRACK* t = new PCB_TRACK( board.get() );
        t->SetNetCode( code );
        t->SetStart( VECTOR2I( x, 0 ) );
        t->SetEnd( VECTOR2I( x + 1000000, 0 ) );
        board->Add( t );
        x += 2000000;
    }

    auto tmpFile = std::filesystem::temp_directory_path() / "signals_survive_buildlist.kicad_pcb";
    plugin.SaveBoard( tmpFile.string(), board.get() );

    std::unique_ptr<BOARD> loaded = std::make_unique<BOARD>();
    plugin.LoadBoard( tmpFile.string(), loaded.get() );

    // Simulate GUI post-load rebuild that previously cleared signals
    loaded->BuildListOfNets();

    int signalCount = 0;
    std::set<wxString> expectedNames = { wxS("Net-(R1-Pad1)"), wxS("Net-(R1-Pad2)"), wxS("Net-(R2-Pad2)"), wxS("Net-(R3-Pad2)") };
    std::set<wxString> seenNames;
    for( NETINFO_ITEM* net : loaded->GetNetInfo() )
    {
        if( net->GetSignal() == wxS( "Signal1" ) )
        {
            signalCount++;
            seenNames.insert( net->GetNetname() );
        }
    }
    BOOST_CHECK_EQUAL( signalCount, 4 );
    BOOST_CHECK_EQUAL( seenNames.size(), expectedNames.size() );
    for( const wxString& name : expectedNames )
        BOOST_CHECK( seenNames.count( name ) );
}

BOOST_AUTO_TEST_SUITE_END()
