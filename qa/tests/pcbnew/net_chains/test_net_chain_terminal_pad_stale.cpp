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
 */

#include <boost/test/unit_test.hpp>
#include <board.h>
#include <footprint.h>
#include <pad.h>
#include <netinfo.h>
#include <netlist_reader/pcb_netlist.h>
#include <netlist_reader/board_netlist_updater.h>
#include <qa_utils/wx_utils/unit_test_utils.h>


namespace
{

PAD* MakePad( FOOTPRINT* aFp, NETINFO_ITEM* aNet, const wxString& aNumber, const VECTOR2I& aPos )
{
    PAD* pad = new PAD( aFp );
    pad->SetFrontShape( PAD_SHAPE::CIRCLE );
    pad->SetSize( F_Cu, VECTOR2I( 1000000, 1000000 ) );
    pad->SetPosition( aPos );
    pad->SetNumber( aNumber );
    pad->SetNet( aNet );
    aFp->Add( pad );
    return pad;
}

}  // namespace


BOOST_AUTO_TEST_SUITE( NetChainTerminalPadStaleness )


// H-2: Removing a chain assignment via the netlist update must drop the prior-session
// terminal pads on the affected net. Otherwise the s-expr writer will synthesize a chain
// from the netname when it serialises the orphaned pad, and DRC matched-length will keep
// treating the net as on-trunk.
BOOST_AUTO_TEST_CASE( ChainRemovalClearsTerminalPads )
{
    std::unique_ptr<BOARD> board = std::make_unique<BOARD>();

    NETINFO_ITEM* n1 = new NETINFO_ITEM( board.get(), wxS( "Net1" ), 1 );
    NETINFO_ITEM* n2 = new NETINFO_ITEM( board.get(), wxS( "Net2" ), 2 );
    board->Add( n1 );
    board->Add( n2 );

    n1->SetNetChain( wxS( "BUS_X" ) );
    n2->SetNetChain( wxS( "BUS_X" ) );

    FOOTPRINT* fp1 = new FOOTPRINT( board.get() );
    fp1->SetReference( wxS( "U1" ) );
    board->Add( fp1 );

    FOOTPRINT* fp2 = new FOOTPRINT( board.get() );
    fp2->SetReference( wxS( "U2" ) );
    board->Add( fp2 );

    PAD* pA = MakePad( fp1, n1, wxS( "1" ), VECTOR2I( 0,        0 ) );
    PAD* pB = MakePad( fp2, n2, wxS( "1" ), VECTOR2I( 10000000, 0 ) );

    n1->SetTerminalPad( 0, pA );
    n1->SetTerminalPadUuid( 0, pA->m_Uuid );
    n2->SetTerminalPad( 1, pB );
    n2->SetTerminalPadUuid( 1, pB->m_Uuid );

    NETLIST netlist;

    BOARD_NETLIST_UPDATER::ApplyChainAssignments( board.get(), netlist, nullptr, false );

    BOOST_CHECK( n1->GetNetChain().IsEmpty() );
    BOOST_CHECK( n2->GetNetChain().IsEmpty() );

    BOOST_CHECK( n1->GetTerminalPad( 0 ) == nullptr );
    BOOST_CHECK( n1->GetTerminalPad( 1 ) == nullptr );
    BOOST_CHECK( n2->GetTerminalPad( 0 ) == nullptr );
    BOOST_CHECK( n2->GetTerminalPad( 1 ) == nullptr );

    BOOST_CHECK( n1->GetTerminalPadUuid( 0 ) == niluuid );
    BOOST_CHECK( n2->GetTerminalPadUuid( 1 ) == niluuid );
}


// Renaming a chain (BUS_OLD -> BUS_NEW) must drop prior-session pads on every member net so
// the netlist's terminal-pin map starts from a clean slate. A net dropped from the chain
// during the rename (Net2 here) must end up with both slots null.
BOOST_AUTO_TEST_CASE( ChainRenameClearsStaleTerminalPads )
{
    std::unique_ptr<BOARD> board = std::make_unique<BOARD>();

    NETINFO_ITEM* n1 = new NETINFO_ITEM( board.get(), wxS( "Net1" ), 1 );
    NETINFO_ITEM* n2 = new NETINFO_ITEM( board.get(), wxS( "Net2" ), 2 );
    board->Add( n1 );
    board->Add( n2 );

    n1->SetNetChain( wxS( "BUS_OLD" ) );
    n2->SetNetChain( wxS( "BUS_OLD" ) );

    FOOTPRINT* fp1 = new FOOTPRINT( board.get() );
    fp1->SetReference( wxS( "U1" ) );
    board->Add( fp1 );

    FOOTPRINT* fp2 = new FOOTPRINT( board.get() );
    fp2->SetReference( wxS( "U2" ) );
    board->Add( fp2 );

    PAD* pA = MakePad( fp1, n1, wxS( "1" ), VECTOR2I( 0,        0 ) );
    PAD* pB = MakePad( fp2, n2, wxS( "1" ), VECTOR2I( 10000000, 0 ) );

    n1->SetTerminalPad( 0, pA );
    n1->SetTerminalPadUuid( 0, pA->m_Uuid );
    n2->SetTerminalPad( 1, pB );
    n2->SetTerminalPadUuid( 1, pB->m_Uuid );

    NETLIST netlist;
    netlist.SetNetChainFor( wxS( "Net1" ), wxS( "BUS_NEW" ) );

    BOARD_NETLIST_UPDATER::ApplyChainAssignments( board.get(), netlist, nullptr, false );

    BOOST_CHECK_EQUAL( n1->GetNetChain(), wxS( "BUS_NEW" ) );
    BOOST_CHECK( n2->GetNetChain().IsEmpty() );

    BOOST_CHECK( n1->GetTerminalPad( 0 ) == nullptr );
    BOOST_CHECK( n1->GetTerminalPad( 1 ) == nullptr );
    BOOST_CHECK( n2->GetTerminalPad( 0 ) == nullptr );
    BOOST_CHECK( n2->GetTerminalPad( 1 ) == nullptr );

    BOOST_CHECK( n1->GetTerminalPadUuid( 0 ) == niluuid );
    BOOST_CHECK( n2->GetTerminalPadUuid( 1 ) == niluuid );
}


// Sanity: a net whose chain assignment is unchanged must keep its prior terminal pads.
// The clearing rule must NOT fire on stable chains, otherwise a benign netlist refresh
// would wipe the s-expr-restored bindings before the terminal-pin reapply pass runs.
BOOST_AUTO_TEST_CASE( UnchangedChainPreservesTerminalPads )
{
    std::unique_ptr<BOARD> board = std::make_unique<BOARD>();

    NETINFO_ITEM* n1 = new NETINFO_ITEM( board.get(), wxS( "Net1" ), 1 );
    board->Add( n1 );
    n1->SetNetChain( wxS( "BUS_K" ) );

    FOOTPRINT* fp = new FOOTPRINT( board.get() );
    fp->SetReference( wxS( "U1" ) );
    board->Add( fp );

    PAD* pA = MakePad( fp, n1, wxS( "1" ), VECTOR2I( 0, 0 ) );

    n1->SetTerminalPad( 0, pA );
    n1->SetTerminalPadUuid( 0, pA->m_Uuid );

    NETLIST netlist;
    netlist.SetNetChainFor( wxS( "Net1" ), wxS( "BUS_K" ) );

    BOARD_NETLIST_UPDATER::ApplyChainAssignments( board.get(), netlist, nullptr, false );

    BOOST_CHECK_EQUAL( n1->GetNetChain(), wxS( "BUS_K" ) );
    BOOST_CHECK_EQUAL( n1->GetTerminalPad( 0 ), pA );
    BOOST_CHECK( n1->GetTerminalPadUuid( 0 ) == pA->m_Uuid );
}


BOOST_AUTO_TEST_SUITE_END()
