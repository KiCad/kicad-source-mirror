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

#include <base_units.h>
#include <board.h>
#include <board_connected_item.h>
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


BOOST_AUTO_TEST_SUITE( NetChainBridgingDelay )


BOOST_AUTO_TEST_CASE( EmptyInputsReturnZero )
{
    BOARD board;

    auto [len, delay] = BoardChainBridging( &board, wxS( "SIG" ) );

    BOOST_CHECK_EQUAL( len, 0.0 );
    BOOST_CHECK_EQUAL( delay, 0.0 );

    auto [len2, delay2] = BoardChainBridging( nullptr, wxS( "SIG" ) );

    BOOST_CHECK_EQUAL( len2, 0.0 );
    BOOST_CHECK_EQUAL( delay2, 0.0 );

    auto [len3, delay3] = BoardChainBridging( &board, wxString() );

    BOOST_CHECK_EQUAL( len3, 0.0 );
    BOOST_CHECK_EQUAL( delay3, 0.0 );
}


BOOST_AUTO_TEST_CASE( BridgingLengthAndDelayMatchFallback )
{
    BOARD board;

    NETINFO_ITEM* netA = addNet( &board, wxS( "/A" ), 1, wxS( "SIG" ) );
    NETINFO_ITEM* netB = addNet( &board, wxS( "/B" ), 2, wxS( "SIG" ) );

    FOOTPRINT* fp = addFootprint( &board );
    addPad( fp, netA, VECTOR2I( -1'000'000, 0 ) );
    addPad( fp, netB, VECTOR2I(  1'000'000, 0 ) );

    auto [len, delay] = BoardChainBridging( &board, wxS( "SIG" ) );

    // 2 mm bridging span.
    BOOST_CHECK_CLOSE( len, 2'000'000.0, 0.001 );

    // 2 mm at fallback ps/mm, expressed in delay IU (attoseconds).
    double expectedDelayIU = DEFAULT_PROPAGATION_DELAY_PS_PER_MM * pcbIUScale.IU_PER_PS * 2.0;
    BOOST_CHECK_CLOSE( delay, expectedDelayIU, 0.001 );
    BOOST_CHECK_GT( delay, 0.0 );
}


BOOST_AUTO_TEST_CASE( SingleNetFootprintContributesNothing )
{
    BOARD board;

    NETINFO_ITEM* netA = addNet( &board, wxS( "/A" ), 1, wxS( "SIG" ) );

    FOOTPRINT* fp = addFootprint( &board );
    addPad( fp, netA, VECTOR2I( -1'000'000, 0 ) );
    addPad( fp, netA, VECTOR2I(  1'000'000, 0 ) );

    auto [len, delay] = BoardChainBridging( &board, wxS( "SIG" ) );

    BOOST_CHECK_EQUAL( len, 0.0 );
    BOOST_CHECK_EQUAL( delay, 0.0 );
}


BOOST_AUTO_TEST_CASE( MultipleFootprintsSum )
{
    BOARD board;

    NETINFO_ITEM* netA = addNet( &board, wxS( "/A" ), 1, wxS( "SIG" ) );
    NETINFO_ITEM* netB = addNet( &board, wxS( "/B" ), 2, wxS( "SIG" ) );
    NETINFO_ITEM* netC = addNet( &board, wxS( "/C" ), 3, wxS( "SIG" ) );

    FOOTPRINT* fp1 = addFootprint( &board );
    addPad( fp1, netA, VECTOR2I( -1'500'000, 0 ) );
    addPad( fp1, netB, VECTOR2I(  1'500'000, 0 ) );

    FOOTPRINT* fp2 = addFootprint( &board );
    addPad( fp2, netB, VECTOR2I( 10'000'000, 0 ) );
    addPad( fp2, netC, VECTOR2I( 13'000'000, 0 ) );

    auto [len, delay] = BoardChainBridging( &board, wxS( "SIG" ) );

    BOOST_CHECK_CLOSE( len, 6'000'000.0, 0.001 );

    double expectedDelayIU = DEFAULT_PROPAGATION_DELAY_PS_PER_MM * pcbIUScale.IU_PER_PS * 6.0;
    BOOST_CHECK_CLOSE( delay, expectedDelayIU, 0.001 );
}


BOOST_AUTO_TEST_CASE( ChainsArePartitionedByName )
{
    BOARD board;

    NETINFO_ITEM* netA = addNet( &board, wxS( "/A" ), 1, wxS( "SIG_X" ) );
    NETINFO_ITEM* netB = addNet( &board, wxS( "/B" ), 2, wxS( "SIG_X" ) );
    NETINFO_ITEM* netC = addNet( &board, wxS( "/C" ), 3, wxS( "SIG_Y" ) );
    NETINFO_ITEM* netD = addNet( &board, wxS( "/D" ), 4, wxS( "SIG_Y" ) );

    FOOTPRINT* fp1 = addFootprint( &board );
    addPad( fp1, netA, VECTOR2I( -1'000'000, 0 ) );
    addPad( fp1, netB, VECTOR2I(  1'000'000, 0 ) );

    FOOTPRINT* fp2 = addFootprint( &board );
    addPad( fp2, netC, VECTOR2I( 10'000'000, 0 ) );
    addPad( fp2, netD, VECTOR2I( 14'000'000, 0 ) );

    auto [xLen, xDelay] = BoardChainBridging( &board, wxS( "SIG_X" ) );
    auto [yLen, yDelay] = BoardChainBridging( &board, wxS( "SIG_Y" ) );

    BOOST_CHECK_CLOSE( xLen, 2'000'000.0, 0.001 );
    BOOST_CHECK_CLOSE( yLen, 4'000'000.0, 0.001 );

    double expectedXDelayIU = DEFAULT_PROPAGATION_DELAY_PS_PER_MM * pcbIUScale.IU_PER_PS * 2.0;
    double expectedYDelayIU = DEFAULT_PROPAGATION_DELAY_PS_PER_MM * pcbIUScale.IU_PER_PS * 4.0;
    BOOST_CHECK_CLOSE( xDelay, expectedXDelayIU, 0.001 );
    BOOST_CHECK_CLOSE( yDelay, expectedYDelayIU, 0.001 );
    BOOST_CHECK_GT( yDelay, xDelay );
}


BOOST_AUTO_TEST_CASE( LargeBridgingLengthDoesNotOverflowIntCast )
{
    BOARD board;

    NETINFO_ITEM* netA = addNet( &board, wxS( "/A" ), 1, wxS( "SIG" ) );
    NETINFO_ITEM* netB = addNet( &board, wxS( "/B" ), 2, wxS( "SIG" ) );

    // 3 m span (3,000,000,000 nm) exceeds INT_MAX (~2.147e9).  The previous
    // (int) cast on lengthIU silently truncated to a negative value here,
    // producing a negative delay; the direct-divide form must remain accurate.
    FOOTPRINT* fp = addFootprint( &board );
    addPad( fp, netA, VECTOR2I( -1'500'000'000, 0 ) );
    addPad( fp, netB, VECTOR2I(  1'500'000'000, 0 ) );

    auto [len, delay] = BoardChainBridging( &board, wxS( "SIG" ) );

    BOOST_CHECK_CLOSE( len, 3'000'000'000.0, 0.001 );

    double expectedDelayIU = DEFAULT_PROPAGATION_DELAY_PS_PER_MM * pcbIUScale.IU_PER_PS * 3000.0;
    BOOST_CHECK_CLOSE( delay, expectedDelayIU, 0.001 );
    BOOST_CHECK_GT( delay, 0.0 );
}


BOOST_AUTO_TEST_SUITE_END()
