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
#include <board.h>
#include <footprint.h>
#include <pad.h>
#include <netinfo.h>
#include <netlist_reader/board_netlist_updater.h>
#include <netlist_reader/pcb_netlist.h>
#include <qa_utils/wx_utils/unit_test_utils.h>


BOOST_AUTO_TEST_SUITE( ApplyChainAssignmentsDryRun )


// A dry-run pass through ApplyChainAssignments must not mutate the board. The function
// writes net->SetNetChain() and net->ClearTerminalPad() unconditionally if the dry-run
// flag is not honored, so seed each net with a distinct chain label and a sentinel
// terminal pad and check both survive the call.
BOOST_AUTO_TEST_CASE( DryRunLeavesNetChainAndTerminalPadsUntouched )
{
    std::unique_ptr<BOARD> board = std::make_unique<BOARD>();

    NETINFO_ITEM* n1 = new NETINFO_ITEM( board.get(), wxS( "Net1" ), 1 );
    NETINFO_ITEM* n2 = new NETINFO_ITEM( board.get(), wxS( "Net2" ), 2 );
    board->Add( n1 );
    board->Add( n2 );

    n1->SetNetChain( wxS( "BUS_OLD" ) );
    n2->SetNetChain( wxS( "BUS_OLD" ) );

    FOOTPRINT* fp = new FOOTPRINT( board.get() );
    fp->SetReference( wxS( "U1" ) );
    board->Add( fp );

    PAD* pA = new PAD( fp );
    pA->SetFrontShape( PAD_SHAPE::CIRCLE );
    pA->SetSize( F_Cu, VECTOR2I( 1000000, 1000000 ) );
    pA->SetNumber( wxS( "1" ) );
    pA->SetNet( n1 );
    fp->Add( pA );

    PAD* pB = new PAD( fp );
    pB->SetFrontShape( PAD_SHAPE::CIRCLE );
    pB->SetSize( F_Cu, VECTOR2I( 1000000, 1000000 ) );
    pB->SetNumber( wxS( "2" ) );
    pB->SetNet( n2 );
    fp->Add( pB );

    n1->SetTerminalPad( 0, pA );
    n2->SetTerminalPad( 1, pB );

    // Netlist proposes a new chain assignment that differs from what is on the board.
    // A live run would write "BUS_NEW" and clear both terminal slots; a dry run must
    // not.
    NETLIST netlist;
    netlist.SetNetChainFor( wxS( "Net1" ), wxS( "BUS_NEW" ) );
    netlist.SetNetChainFor( wxS( "Net2" ), wxString() );

    BOARD_NETLIST_UPDATER::ApplyChainAssignments( board.get(), netlist, nullptr, true );

    BOOST_CHECK_EQUAL( n1->GetNetChain(), wxS( "BUS_OLD" ) );
    BOOST_CHECK_EQUAL( n2->GetNetChain(), wxS( "BUS_OLD" ) );

    BOOST_CHECK_EQUAL( n1->GetTerminalPad( 0 ), pA );
    BOOST_CHECK_EQUAL( n2->GetTerminalPad( 1 ), pB );
}


// The live path (aDryRun=false) is exercised here as a positive control so a future
// regression that no-ops the function entirely cannot pass the dry-run test in
// isolation.
BOOST_AUTO_TEST_CASE( LiveRunAppliesChainAndClearsTerminals )
{
    std::unique_ptr<BOARD> board = std::make_unique<BOARD>();

    NETINFO_ITEM* n1 = new NETINFO_ITEM( board.get(), wxS( "Net1" ), 1 );
    board->Add( n1 );

    n1->SetNetChain( wxS( "BUS_OLD" ) );

    FOOTPRINT* fp = new FOOTPRINT( board.get() );
    fp->SetReference( wxS( "U1" ) );
    board->Add( fp );

    PAD* pA = new PAD( fp );
    pA->SetFrontShape( PAD_SHAPE::CIRCLE );
    pA->SetSize( F_Cu, VECTOR2I( 1000000, 1000000 ) );
    pA->SetNumber( wxS( "1" ) );
    pA->SetNet( n1 );
    fp->Add( pA );

    n1->SetTerminalPad( 0, pA );

    NETLIST netlist;
    netlist.SetNetChainFor( wxS( "Net1" ), wxS( "BUS_NEW" ) );

    BOARD_NETLIST_UPDATER::ApplyChainAssignments( board.get(), netlist, nullptr, false );

    BOOST_CHECK_EQUAL( n1->GetNetChain(), wxS( "BUS_NEW" ) );
    BOOST_CHECK( n1->GetTerminalPad( 0 ) == nullptr );
}


BOOST_AUTO_TEST_SUITE_END()
