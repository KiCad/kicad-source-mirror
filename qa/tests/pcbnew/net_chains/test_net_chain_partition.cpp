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

#include <board.h>
#include <footprint.h>
#include <net_chain_bridging.h>
#include <netinfo.h>
#include <pad.h>
#include <padstack.h>


namespace
{

constexpr int PAD_SIZE_NM = 500'000;


PAD* addPad( FOOTPRINT* aFp, NETINFO_ITEM* aNet, const VECTOR2I& aPos )
{
    PAD* pad = new PAD( aFp );
    pad->SetShape( PADSTACK::ALL_LAYERS, PAD_SHAPE::CIRCLE );
    pad->SetSize( PADSTACK::ALL_LAYERS, VECTOR2I( PAD_SIZE_NM, PAD_SIZE_NM ) );
    pad->SetPosition( aPos );
    pad->SetNet( aNet );
    aFp->Add( pad );
    return pad;
}


NETINFO_ITEM* addNet( BOARD* aBoard, const wxString& aName, int aCode, const wxString& aChain )
{
    NETINFO_ITEM* n = new NETINFO_ITEM( aBoard, aName, aCode );
    n->SetNetChain( aChain );
    aBoard->Add( n );
    return n;
}


FOOTPRINT* addFootprint( BOARD* aBoard )
{
    FOOTPRINT* fp = new FOOTPRINT( aBoard );
    aBoard->Add( fp );
    return fp;
}

}  // namespace


BOOST_AUTO_TEST_SUITE( NetChainPartition )


// Linear chain  NA -- R1 -- NB -- R2 -- NC  with query net NB.  Expect before={NA}, after={NC}.
BOOST_AUTO_TEST_CASE( LinearChainSplitsCleanly )
{
    BOARD board;

    NETINFO_ITEM* nA = addNet( &board, wxS( "/A" ), 1, wxS( "SIG" ) );
    NETINFO_ITEM* nB = addNet( &board, wxS( "/B" ), 2, wxS( "SIG" ) );
    NETINFO_ITEM* nC = addNet( &board, wxS( "/C" ), 3, wxS( "SIG" ) );

    FOOTPRINT* r1 = addFootprint( &board );
    addPad( r1, nA, VECTOR2I( -2'000'000, 0 ) );
    PAD* startPad = addPad( r1, nB, VECTOR2I( -1'000'000, 0 ) );

    FOOTPRINT* r2 = addFootprint( &board );
    PAD* endPad = addPad( r2, nB, VECTOR2I( 1'000'000, 0 ) );
    addPad( r2, nC, VECTOR2I( 2'000'000, 0 ) );

    NET_CHAIN_PARTITION p = PartitionNetChainAroundNet( &board, nB->GetNetCode(),
                                                       startPad, endPad );

    BOOST_REQUIRE( p.status == NET_CHAIN_PARTITION_STATUS::OK );
    BOOST_CHECK_EQUAL( p.beforeStart.size(), 1u );
    BOOST_CHECK_EQUAL( p.afterEnd.size(), 1u );
    BOOST_CHECK( p.beforeStart.count( nA->GetNetCode() ) == 1 );
    BOOST_CHECK( p.afterEnd.count( nC->GetNetCode() ) == 1 );
}


// Query net is the chain terminal: end pad has no bridge neighbor.
BOOST_AUTO_TEST_CASE( TerminalQueryNetLeavesOneSideEmpty )
{
    BOARD board;

    NETINFO_ITEM* nA = addNet( &board, wxS( "/A" ), 1, wxS( "SIG" ) );
    NETINFO_ITEM* nB = addNet( &board, wxS( "/B" ), 2, wxS( "SIG" ) );

    FOOTPRINT* r1 = addFootprint( &board );
    PAD* startPad = addPad( r1, nA, VECTOR2I( -1'000'000, 0 ) );
    addPad( r1, nB, VECTOR2I( 1'000'000, 0 ) );

    // Synthetic second pad on nA with no bridge partner so we have two distinct pads.
    FOOTPRINT* loose = addFootprint( &board );
    PAD* endPad = addPad( loose, nA, VECTOR2I( -10'000'000, 0 ) );

    NET_CHAIN_PARTITION p = PartitionNetChainAroundNet( &board, nA->GetNetCode(),
                                                       startPad, endPad );

    BOOST_REQUIRE( p.status == NET_CHAIN_PARTITION_STATUS::OK );
    BOOST_CHECK_EQUAL( p.beforeStart.size(), 1u );
    BOOST_CHECK( p.beforeStart.count( nB->GetNetCode() ) == 1 );
    BOOST_CHECK( p.afterEnd.empty() );
}


// 3-pin part: start pad shares its footprint with two distinct non-query nets.
// Both must seed the BFS.
BOOST_AUTO_TEST_CASE( MultiBridgePadSeedsAllNeighbors )
{
    BOARD board;

    NETINFO_ITEM* nQ  = addNet( &board, wxS( "/Q" ),  1, wxS( "SIG" ) );
    NETINFO_ITEM* nN1 = addNet( &board, wxS( "/N1" ), 2, wxS( "SIG" ) );
    NETINFO_ITEM* nN2 = addNet( &board, wxS( "/N2" ), 3, wxS( "SIG" ) );
    NETINFO_ITEM* nE  = addNet( &board, wxS( "/E" ),  4, wxS( "SIG" ) );

    // 3-pin part: query pad + two distinct cross-net pads.
    FOOTPRINT* tri = addFootprint( &board );
    PAD* startPad = addPad( tri, nQ,  VECTOR2I( 0, 0 ) );
    addPad( tri, nN1, VECTOR2I( 1'000'000, 0 ) );
    addPad( tri, nN2, VECTOR2I( 0, 1'000'000 ) );

    // Endpoint pad on a regular 2-pin part with its own neighbor.
    FOOTPRINT* r = addFootprint( &board );
    PAD* endPad = addPad( r, nQ, VECTOR2I( 5'000'000, 0 ) );
    addPad( r, nE, VECTOR2I( 6'000'000, 0 ) );

    NET_CHAIN_PARTITION p = PartitionNetChainAroundNet( &board, nQ->GetNetCode(),
                                                       startPad, endPad );

    BOOST_REQUIRE( p.status == NET_CHAIN_PARTITION_STATUS::OK );
    BOOST_CHECK_EQUAL( p.beforeStart.size(), 2u );
    BOOST_CHECK( p.beforeStart.count( nN1->GetNetCode() ) == 1 );
    BOOST_CHECK( p.beforeStart.count( nN2->GetNetCode() ) == 1 );
    BOOST_CHECK_EQUAL( p.afterEnd.size(), 1u );
    BOOST_CHECK( p.afterEnd.count( nE->GetNetCode() ) == 1 );
}


// Parallel passives between NB and NC form a cycle that bypasses the query net.
// Cutting at NB still leaves the two sides reachable from each other → ambiguous.
BOOST_AUTO_TEST_CASE( CycleNotThroughQueryIsAmbiguous )
{
    BOARD board;

    NETINFO_ITEM* nA = addNet( &board, wxS( "/A" ), 1, wxS( "SIG" ) );
    NETINFO_ITEM* nB = addNet( &board, wxS( "/B" ), 2, wxS( "SIG" ) );
    NETINFO_ITEM* nC = addNet( &board, wxS( "/C" ), 3, wxS( "SIG" ) );

    // R1: A -- B  (start side)
    FOOTPRINT* r1 = addFootprint( &board );
    addPad( r1, nA, VECTOR2I( -2'000'000, 0 ) );
    PAD* startPad = addPad( r1, nB, VECTOR2I( -1'000'000, 0 ) );

    // R2: B -- C  (end side)
    FOOTPRINT* r2 = addFootprint( &board );
    PAD* endPad = addPad( r2, nB, VECTOR2I( 1'000'000, 0 ) );
    addPad( r2, nC, VECTOR2I( 2'000'000, 0 ) );

    // R3: A -- C  (parallel cycle, bypassing nB entirely)
    FOOTPRINT* r3 = addFootprint( &board );
    addPad( r3, nA, VECTOR2I( -2'000'000, 5'000'000 ) );
    addPad( r3, nC, VECTOR2I( 2'000'000, 5'000'000 ) );

    NET_CHAIN_PARTITION p = PartitionNetChainAroundNet( &board, nB->GetNetCode(),
                                                       startPad, endPad );

    BOOST_REQUIRE( p.status == NET_CHAIN_PARTITION_STATUS::AMBIGUOUS_OVERLAP );

    // Both sides still populated for caller inspection.
    BOOST_CHECK( !p.beforeStart.empty() );
    BOOST_CHECK( !p.afterEnd.empty() );
}


// Start and end pads supplied on a net other than the query net → error.
BOOST_AUTO_TEST_CASE( StartPadOnDifferentNetReportsError )
{
    BOARD board;

    NETINFO_ITEM* nA = addNet( &board, wxS( "/A" ), 1, wxS( "SIG" ) );
    NETINFO_ITEM* nB = addNet( &board, wxS( "/B" ), 2, wxS( "SIG" ) );

    FOOTPRINT* r1 = addFootprint( &board );
    PAD* aPad = addPad( r1, nA, VECTOR2I( -1'000'000, 0 ) );
    PAD* bPad = addPad( r1, nB, VECTOR2I(  1'000'000, 0 ) );

    NET_CHAIN_PARTITION p = PartitionNetChainAroundNet( &board, nA->GetNetCode(),
                                                       bPad, aPad );

    BOOST_CHECK( p.status == NET_CHAIN_PARTITION_STATUS::START_PAD_NOT_ON_QUERY );
    BOOST_CHECK( p.beforeStart.empty() );
    BOOST_CHECK( p.afterEnd.empty() );
}


// Same pad for start and end is an invalid query.
BOOST_AUTO_TEST_CASE( EqualStartEndPadsAreInvalid )
{
    BOARD board;

    NETINFO_ITEM* nA = addNet( &board, wxS( "/A" ), 1, wxS( "SIG" ) );
    NETINFO_ITEM* nB = addNet( &board, wxS( "/B" ), 2, wxS( "SIG" ) );

    FOOTPRINT* r1 = addFootprint( &board );
    PAD* startPad = addPad( r1, nA, VECTOR2I( -1'000'000, 0 ) );
    addPad( r1, nB, VECTOR2I( 1'000'000, 0 ) );

    NET_CHAIN_PARTITION p = PartitionNetChainAroundNet( &board, nA->GetNetCode(),
                                                       startPad, startPad );

    BOOST_CHECK( p.status == NET_CHAIN_PARTITION_STATUS::INVALID_INPUT );
}


// Net not assigned to any chain → distinct status.
BOOST_AUTO_TEST_CASE( QueryNetWithoutChainReportsMissing )
{
    BOARD board;

    NETINFO_ITEM* nA = addNet( &board, wxS( "/A" ), 1, wxString() );
    NETINFO_ITEM* nB = addNet( &board, wxS( "/B" ), 2, wxString() );

    FOOTPRINT* fp = addFootprint( &board );
    PAD* a1 = addPad( fp, nA, VECTOR2I( 0, 0 ) );
    addPad( fp, nB, VECTOR2I( 1'000'000, 0 ) );

    FOOTPRINT* fp2 = addFootprint( &board );
    PAD* a2 = addPad( fp2, nA, VECTOR2I( 10'000'000, 0 ) );

    NET_CHAIN_PARTITION p = PartitionNetChainAroundNet( &board, nA->GetNetCode(), a1, a2 );

    BOOST_CHECK( p.status == NET_CHAIN_PARTITION_STATUS::QUERY_NET_NOT_IN_CHAIN );
}


// A chain with no bridges at all (only one net) cannot be partitioned.
BOOST_AUTO_TEST_CASE( ChainWithNoBridgesReportsNoBridges )
{
    BOARD board;

    NETINFO_ITEM* nA = addNet( &board, wxS( "/A" ), 1, wxS( "SIG" ) );

    FOOTPRINT* fp = addFootprint( &board );
    PAD* p1 = addPad( fp, nA, VECTOR2I( 0, 0 ) );
    PAD* p2 = addPad( fp, nA, VECTOR2I( 1'000'000, 0 ) );

    NET_CHAIN_PARTITION p = PartitionNetChainAroundNet( &board, nA->GetNetCode(), p1, p2 );

    BOOST_CHECK( p.status == NET_CHAIN_PARTITION_STATUS::NO_CHAIN_BRIDGES );
}


// A chain has bridges between other nets, but no bridge is incident on the query net.
// The query net was tagged as a chain member but is unreachable through the bridge graph.
BOOST_AUTO_TEST_CASE( QueryNetWithoutOwnBridgeReportsNoBridges )
{
    BOARD board;

    NETINFO_ITEM* nQ = addNet( &board, wxS( "/Q" ), 1, wxS( "SIG" ) );
    NETINFO_ITEM* nA = addNet( &board, wxS( "/A" ), 2, wxS( "SIG" ) );
    NETINFO_ITEM* nB = addNet( &board, wxS( "/B" ), 3, wxS( "SIG" ) );

    // Bridge between nA and nB; nothing touches nQ.
    FOOTPRINT* r = addFootprint( &board );
    addPad( r, nA, VECTOR2I( -1'000'000, 0 ) );
    addPad( r, nB, VECTOR2I( 1'000'000, 0 ) );

    // Two distinct pads on nQ, neither on a bridge.
    FOOTPRINT* fp1 = addFootprint( &board );
    PAD* startPad = addPad( fp1, nQ, VECTOR2I( 10'000'000, 0 ) );

    FOOTPRINT* fp2 = addFootprint( &board );
    PAD* endPad = addPad( fp2, nQ, VECTOR2I( 20'000'000, 0 ) );

    NET_CHAIN_PARTITION p = PartitionNetChainAroundNet( &board, nQ->GetNetCode(),
                                                       startPad, endPad );

    BOOST_CHECK( p.status == NET_CHAIN_PARTITION_STATUS::NO_CHAIN_BRIDGES );
    BOOST_CHECK( p.beforeStart.empty() );
    BOOST_CHECK( p.afterEnd.empty() );
}


// Cross-board pad reference must be rejected, since EnumerateChainBridges walks aBoard.
BOOST_AUTO_TEST_CASE( CrossBoardPadIsInvalidInput )
{
    BOARD boardA;
    BOARD boardB;

    NETINFO_ITEM* nA = addNet( &boardA, wxS( "/A" ), 1, wxS( "SIG" ) );
    NETINFO_ITEM* nB = addNet( &boardA, wxS( "/B" ), 2, wxS( "SIG" ) );

    FOOTPRINT* r = addFootprint( &boardA );
    PAD* startPad = addPad( r, nA, VECTOR2I( 0, 0 ) );
    addPad( r, nB, VECTOR2I( 1'000'000, 0 ) );

    // A pad on a different board.
    NETINFO_ITEM* nA_b = addNet( &boardB, wxS( "/A" ), 1, wxS( "SIG" ) );
    FOOTPRINT* r_b = addFootprint( &boardB );
    PAD* foreignPad = addPad( r_b, nA_b, VECTOR2I( 0, 0 ) );

    NET_CHAIN_PARTITION p = PartitionNetChainAroundNet( &boardA, nA->GetNetCode(),
                                                       startPad, foreignPad );

    BOOST_CHECK( p.status == NET_CHAIN_PARTITION_STATUS::INVALID_INPUT );
}


// Caller-owned result struct starts empty; we don't depend on the caller pre-clearing.
BOOST_AUTO_TEST_CASE( PrePopulatedCallerStateIsIgnored )
{
    BOARD board;

    NETINFO_ITEM* nA = addNet( &board, wxS( "/A" ), 1, wxS( "SIG" ) );
    NETINFO_ITEM* nB = addNet( &board, wxS( "/B" ), 2, wxS( "SIG" ) );
    NETINFO_ITEM* nC = addNet( &board, wxS( "/C" ), 3, wxS( "SIG" ) );

    FOOTPRINT* r1 = addFootprint( &board );
    addPad( r1, nA, VECTOR2I( -2'000'000, 0 ) );
    PAD* startPad = addPad( r1, nB, VECTOR2I( -1'000'000, 0 ) );

    FOOTPRINT* r2 = addFootprint( &board );
    PAD* endPad = addPad( r2, nB, VECTOR2I( 1'000'000, 0 ) );
    addPad( r2, nC, VECTOR2I( 2'000'000, 0 ) );

    NET_CHAIN_PARTITION p = PartitionNetChainAroundNet( &board, nB->GetNetCode(),
                                                       startPad, endPad );

    BOOST_REQUIRE( p.status == NET_CHAIN_PARTITION_STATUS::OK );

    // Reassignment from another call must not leak prior state.
    p = PartitionNetChainAroundNet( &board, nB->GetNetCode(), startPad, endPad );

    BOOST_CHECK_EQUAL( p.beforeStart.size(), 1u );
    BOOST_CHECK_EQUAL( p.afterEnd.size(), 1u );
}


BOOST_AUTO_TEST_SUITE_END()
