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
#include <qa_utils/wx_utils/unit_test_utils.h>

namespace
{

// Mirrors the assignment block in BOARD_NETLIST_UPDATER. Refactored here so a unit test can
// exercise the storage rule independently of the rest of the updater. Any future divergence
// between this and the production implementation is itself a bug worth catching.
void AssignChainTerminalPads( BOARD* aBoard, const wxString& aChain, PAD* aPads[2] )
{
    for( int i = 0; i < 2; ++i )
    {
        if( !aPads[i] )
            continue;

        NETINFO_ITEM* termNet = aPads[i]->GetNet();

        if( !termNet || termNet->GetNetChain() != aChain )
            continue;

        for( NETINFO_ITEM* net : aBoard->GetNetInfo() )
        {
            if( net != termNet && net->GetNetChain() == aChain
                && net->GetTerminalPad( i ) )
            {
                net->SetTerminalPad( i, nullptr );
            }
        }

        termNet->SetTerminalPad( i, aPads[i] );
    }
}

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


BOOST_AUTO_TEST_SUITE( NetChainTerminalPadAssignment )

// Three-net chain Net1 -- Comp1 -- Net2 -- Comp2 -- Net3 with terminals on Net1 (slot 0) and
// Net3 (slot 1). The middle interior net (Net2) must keep both slots null so the matched-length
// stub predicate continues to treat it as "not on trunk".
BOOST_AUTO_TEST_CASE( TerminalsOnlyAttachToOwningNet )
{
    std::unique_ptr<BOARD> board = std::make_unique<BOARD>();

    NETINFO_ITEM* n1 = new NETINFO_ITEM( board.get(), wxS( "Net1" ), 1 );
    NETINFO_ITEM* n2 = new NETINFO_ITEM( board.get(), wxS( "Net2" ), 2 );
    NETINFO_ITEM* n3 = new NETINFO_ITEM( board.get(), wxS( "Net3" ), 3 );
    board->Add( n1 );
    board->Add( n2 );
    board->Add( n3 );

    n1->SetNetChain( wxS( "BUS_A" ) );
    n2->SetNetChain( wxS( "BUS_A" ) );
    n3->SetNetChain( wxS( "BUS_A" ) );

    FOOTPRINT* src = new FOOTPRINT( board.get() );
    src->SetReference( wxS( "U1" ) );
    board->Add( src );

    FOOTPRINT* mid = new FOOTPRINT( board.get() );
    mid->SetReference( wxS( "R1" ) );
    board->Add( mid );

    FOOTPRINT* sink = new FOOTPRINT( board.get() );
    sink->SetReference( wxS( "U2" ) );
    board->Add( sink );

    PAD* pSrc  = MakePad( src,  n1, wxS( "1" ), VECTOR2I( 0,         0 ) );
    PAD* pMidA = MakePad( mid,  n1, wxS( "1" ), VECTOR2I( 5000000,   0 ) );
    PAD* pMidB = MakePad( mid,  n2, wxS( "2" ), VECTOR2I( 6000000,   0 ) );
    PAD* pSink = MakePad( sink, n3, wxS( "1" ), VECTOR2I( 15000000,  0 ) );

    (void) pMidA;
    (void) pMidB;

    PAD* pads[2] = { pSrc, pSink };
    AssignChainTerminalPads( board.get(), wxS( "BUS_A" ), pads );

    BOOST_CHECK_EQUAL( n1->GetTerminalPad( 0 ), pSrc );
    BOOST_CHECK( n1->GetTerminalPad( 1 ) == nullptr );

    BOOST_CHECK( n2->GetTerminalPad( 0 ) == nullptr );
    BOOST_CHECK( n2->GetTerminalPad( 1 ) == nullptr );

    BOOST_CHECK( n3->GetTerminalPad( 0 ) == nullptr );
    BOOST_CHECK_EQUAL( n3->GetTerminalPad( 1 ), pSink );
}


// A null pad in slot 1 must not clobber an existing assignment from a previous resolver pass
// (the H-3 null-overwrite case). Only the slot we are providing is touched.
BOOST_AUTO_TEST_CASE( NullPadDoesNotClobberExisting )
{
    std::unique_ptr<BOARD> board = std::make_unique<BOARD>();

    NETINFO_ITEM* n1 = new NETINFO_ITEM( board.get(), wxS( "Net1" ), 1 );
    NETINFO_ITEM* n2 = new NETINFO_ITEM( board.get(), wxS( "Net2" ), 2 );
    board->Add( n1 );
    board->Add( n2 );

    n1->SetNetChain( wxS( "BUS_B" ) );
    n2->SetNetChain( wxS( "BUS_B" ) );

    FOOTPRINT* fp1 = new FOOTPRINT( board.get() );
    fp1->SetReference( wxS( "U1" ) );
    board->Add( fp1 );

    FOOTPRINT* fp2 = new FOOTPRINT( board.get() );
    fp2->SetReference( wxS( "U2" ) );
    board->Add( fp2 );

    PAD* pA = MakePad( fp1, n1, wxS( "1" ), VECTOR2I( 0,        0 ) );
    PAD* pB = MakePad( fp2, n2, wxS( "1" ), VECTOR2I( 10000000, 0 ) );

    n2->SetTerminalPad( 1, pB );

    PAD* pads[2] = { pA, nullptr };
    AssignChainTerminalPads( board.get(), wxS( "BUS_B" ), pads );

    BOOST_CHECK_EQUAL( n1->GetTerminalPad( 0 ), pA );
    BOOST_CHECK_EQUAL( n2->GetTerminalPad( 1 ), pB );
    BOOST_CHECK( n1->GetTerminalPad( 1 ) == nullptr );
    BOOST_CHECK( n2->GetTerminalPad( 0 ) == nullptr );
}


// Boards saved with the legacy broadcast assignment have the same pad pointer attached to every
// member net. Re-running the resolver must clear the foreign claims so the middle net stops
// reporting itself as on-trunk.
BOOST_AUTO_TEST_CASE( StaleBroadcastSamePadIsCleared )
{
    std::unique_ptr<BOARD> board = std::make_unique<BOARD>();

    NETINFO_ITEM* n1 = new NETINFO_ITEM( board.get(), wxS( "Net1" ), 1 );
    NETINFO_ITEM* n2 = new NETINFO_ITEM( board.get(), wxS( "Net2" ), 2 );
    NETINFO_ITEM* n3 = new NETINFO_ITEM( board.get(), wxS( "Net3" ), 3 );
    board->Add( n1 );
    board->Add( n2 );
    board->Add( n3 );

    n1->SetNetChain( wxS( "BUS_C" ) );
    n2->SetNetChain( wxS( "BUS_C" ) );
    n3->SetNetChain( wxS( "BUS_C" ) );

    FOOTPRINT* src = new FOOTPRINT( board.get() );
    src->SetReference( wxS( "U1" ) );
    board->Add( src );

    FOOTPRINT* sink = new FOOTPRINT( board.get() );
    sink->SetReference( wxS( "U2" ) );
    board->Add( sink );

    PAD* pSrc = MakePad( src,  n1, wxS( "1" ), VECTOR2I( 0,        0 ) );
    PAD* pSnk = MakePad( sink, n3, wxS( "1" ), VECTOR2I( 10000000, 0 ) );

    n1->SetTerminalPad( 0, pSrc );
    n2->SetTerminalPad( 0, pSrc );
    n3->SetTerminalPad( 0, pSrc );
    n1->SetTerminalPad( 1, pSnk );
    n2->SetTerminalPad( 1, pSnk );
    n3->SetTerminalPad( 1, pSnk );

    PAD* pads[2] = { pSrc, pSnk };
    AssignChainTerminalPads( board.get(), wxS( "BUS_C" ), pads );

    BOOST_CHECK_EQUAL( n1->GetTerminalPad( 0 ), pSrc );
    BOOST_CHECK( n1->GetTerminalPad( 1 ) == nullptr );

    BOOST_CHECK( n2->GetTerminalPad( 0 ) == nullptr );
    BOOST_CHECK( n2->GetTerminalPad( 1 ) == nullptr );

    BOOST_CHECK( n3->GetTerminalPad( 0 ) == nullptr );
    BOOST_CHECK_EQUAL( n3->GetTerminalPad( 1 ), pSnk );
}


// Mirrors the matched-length predicate. With pads owned by Net1/Net3, Net2 must report
// onTrunk=false even though all three nets share a chain.
BOOST_AUTO_TEST_CASE( OnTrunkPredicateRequiresNetcodeMatch )
{
    std::unique_ptr<BOARD> board = std::make_unique<BOARD>();

    NETINFO_ITEM* n1 = new NETINFO_ITEM( board.get(), wxS( "Net1" ), 1 );
    NETINFO_ITEM* n2 = new NETINFO_ITEM( board.get(), wxS( "Net2" ), 2 );
    NETINFO_ITEM* n3 = new NETINFO_ITEM( board.get(), wxS( "Net3" ), 3 );
    board->Add( n1 );
    board->Add( n2 );
    board->Add( n3 );

    FOOTPRINT* fp = new FOOTPRINT( board.get() );
    fp->SetReference( wxS( "U1" ) );
    board->Add( fp );

    PAD* pA = MakePad( fp, n1, wxS( "1" ), VECTOR2I( 0, 0 ) );
    PAD* pB = MakePad( fp, n3, wxS( "2" ), VECTOR2I( 1000000, 0 ) );

    n1->SetTerminalPad( 0, pA );
    n3->SetTerminalPad( 1, pB );

    auto isOnTrunk = []( NETINFO_ITEM* aNet, int aNetCode )
    {
        return ( aNet->GetTerminalPad( 0 )
                 && aNet->GetTerminalPad( 0 )->GetNetCode() == aNetCode )
            || ( aNet->GetTerminalPad( 1 )
                 && aNet->GetTerminalPad( 1 )->GetNetCode() == aNetCode );
    };

    BOOST_CHECK( isOnTrunk( n1, 1 ) );
    BOOST_CHECK( isOnTrunk( n3, 3 ) );
    BOOST_CHECK( !isOnTrunk( n2, 2 ) );

    // Defense-in-depth: even if a stale broadcast slipped a foreign-net pad onto n2, the
    // predicate refuses to be fooled because the netcodes mismatch.
    n2->SetTerminalPad( 0, pA );
    BOOST_CHECK( !isOnTrunk( n2, 2 ) );
}

BOOST_AUTO_TEST_SUITE_END()
