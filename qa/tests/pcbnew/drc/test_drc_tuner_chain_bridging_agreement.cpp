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

#include <memory>

#include <board.h>
#include <footprint.h>
#include <pad.h>
#include <padstack.h>
#include <pcb_generator.h>
#include <pcb_track.h>
#include <generators/pcb_tuning_pattern.h>
#include <net_chain_bridging.h>
#include <netinfo.h>


// Regression coverage for the H-8 review finding: DRC's matched-length provider and the
// tuning-pattern generator must agree on which footprints bridge a chain and by how much.
//
// Pre-fix the two engines diverged.
//   DRC counted chain pads and skipped only when fewer than two existed, then summed the
//   max pairwise cross-net pad span.
//   The tuner counted unique netcodes via std::map<int, PAD*>, skipped when more than two
//   distinct codes were seen, and otherwise added the distance between two arbitrary
//   "first seen" pads.
//
// A 3-pad / 2-net device (centre-tap ferrite bead) bridged in the tuner with the wrong
// pad pair, and a 3-pad / 3-net device (transformer) was silently dropped by the tuner
// while DRC still scored it.  The shared helper FootprintChainBridgingLength now drives
// both engines, so this test pins down their identical answers across the cases that
// previously diverged.


namespace
{

constexpr double EPS_IU = 1.0;
constexpr int    PAD_SIZE_NM = 500'000;  // 0.5 mm in nm


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


// Drive the tuner cache through one full miss-then-hit cycle and return its bridging.
// PCB_TUNING_PATTERN holds private cache state but exposes GetCachedBridgingLength as
// the read path actually used by the generator.
long long tunerBridging( BOARD* aBoard, const wxString& aChain )
{
    PCB_TUNING_PATTERN tuner;
    return tuner.GetCachedBridgingLength( aBoard, aChain, nullptr );
}

}  // namespace


BOOST_AUTO_TEST_SUITE( DRCTunerChainBridgingAgreement )


// Two-pad two-net footprint.  Both engines have always scored this case; included as a
// sanity baseline for the cases that follow.
BOOST_AUTO_TEST_CASE( TwoPadTwoNetFootprintAgrees )
{
    BOARD board;

    NETINFO_ITEM* netA = addNet( &board, wxS( "/A" ), 1, wxS( "SIG" ) );
    NETINFO_ITEM* netB = addNet( &board, wxS( "/B" ), 2, wxS( "SIG" ) );

    FOOTPRINT* fp = addFootprint( &board );
    addPad( fp, netA, VECTOR2I( -1'000'000, 0 ) );  // -1 mm
    addPad( fp, netB, VECTOR2I(  1'000'000, 0 ) );  // +1 mm

    double drcLen = BoardChainBridgingLength( &board, wxS( "SIG" ) );
    long long tunerLen = tunerBridging( &board, wxS( "SIG" ) );

    BOOST_CHECK_CLOSE( drcLen, 2'000'000.0, 0.001 );
    BOOST_CHECK_LE( std::abs( drcLen - static_cast<double>( tunerLen ) ), EPS_IU );
}


// Three-pad two-net (centre-tap bead / split inductor): pre-fix the tuner picked the
// first-seen pad on each net, so the reported span depended on pad iteration order
// rather than the worst-case bridge.  DRC always took the max pairwise span.
BOOST_AUTO_TEST_CASE( ThreePadTwoNetFootprintAgrees )
{
    BOARD board;

    NETINFO_ITEM* netA = addNet( &board, wxS( "/A" ), 1, wxS( "SIG" ) );
    NETINFO_ITEM* netB = addNet( &board, wxS( "/B" ), 2, wxS( "SIG" ) );

    FOOTPRINT* fp = addFootprint( &board );
    addPad( fp, netA, VECTOR2I( -2'500'000, 0 ) );  // -2.5 mm  net A
    addPad( fp, netA, VECTOR2I(          0, 0 ) );  //    0 mm  net A
    addPad( fp, netB, VECTOR2I(  2'500'000, 0 ) );  // +2.5 mm  net B

    double drcLen = BoardChainBridgingLength( &board, wxS( "SIG" ) );
    long long tunerLen = tunerBridging( &board, wxS( "SIG" ) );

    // Max pairwise cross-net span is from -2.5 mm (A) to +2.5 mm (B) = 5 mm.
    BOOST_CHECK_CLOSE( drcLen, 5'000'000.0, 0.001 );
    BOOST_CHECK_LE( std::abs( drcLen - static_cast<double>( tunerLen ) ), EPS_IU );
}


// Three-pad three-net (transformer-style): pre-fix the tuner saw size > 2 and silently
// dropped the footprint (returning 0), while DRC still added the worst-case cross-net
// span.  The fix makes both report the max pairwise span.
BOOST_AUTO_TEST_CASE( ThreePadThreeNetFootprintAgrees )
{
    BOARD board;

    NETINFO_ITEM* netA = addNet( &board, wxS( "/A" ), 1, wxS( "SIG" ) );
    NETINFO_ITEM* netB = addNet( &board, wxS( "/B" ), 2, wxS( "SIG" ) );
    NETINFO_ITEM* netC = addNet( &board, wxS( "/C" ), 3, wxS( "SIG" ) );

    FOOTPRINT* fp = addFootprint( &board );
    addPad( fp, netA, VECTOR2I( -2'500'000, 0 ) );  // -2.5 mm  net A
    addPad( fp, netB, VECTOR2I(          0, 0 ) );  //    0 mm  net B
    addPad( fp, netC, VECTOR2I(  2'500'000, 0 ) );  // +2.5 mm  net C

    double drcLen = BoardChainBridgingLength( &board, wxS( "SIG" ) );
    long long tunerLen = tunerBridging( &board, wxS( "SIG" ) );

    // Max pairwise cross-net span is -2.5 mm (A) to +2.5 mm (C) = 5 mm.  Pre-fix the
    // tuner returned 0 here; agreement is the regression we are pinning down.
    BOOST_CHECK_CLOSE( drcLen, 5'000'000.0, 0.001 );
    BOOST_CHECK_GT( tunerLen, 0 );
    BOOST_CHECK_LE( std::abs( drcLen - static_cast<double>( tunerLen ) ), EPS_IU );
}


// Four-pad two-net: max cross-net pairwise span exceeds the "first seen on each net"
// pair the tuner used to pick.  Pads at -3, -1, +1, +3 mm with nets A, A, B, B; max
// span is from -3 mm (A) to +3 mm (B) = 6 mm.
BOOST_AUTO_TEST_CASE( FourPadTwoNetFootprintAgrees )
{
    BOARD board;

    NETINFO_ITEM* netA = addNet( &board, wxS( "/A" ), 1, wxS( "SIG" ) );
    NETINFO_ITEM* netB = addNet( &board, wxS( "/B" ), 2, wxS( "SIG" ) );

    FOOTPRINT* fp = addFootprint( &board );
    addPad( fp, netA, VECTOR2I( -3'000'000, 0 ) );
    addPad( fp, netA, VECTOR2I( -1'000'000, 0 ) );
    addPad( fp, netB, VECTOR2I(  1'000'000, 0 ) );
    addPad( fp, netB, VECTOR2I(  3'000'000, 0 ) );

    double drcLen = BoardChainBridgingLength( &board, wxS( "SIG" ) );
    long long tunerLen = tunerBridging( &board, wxS( "SIG" ) );

    BOOST_CHECK_CLOSE( drcLen, 6'000'000.0, 0.001 );
    BOOST_CHECK_LE( std::abs( drcLen - static_cast<double>( tunerLen ) ), EPS_IU );
}


// Three-pad single-net footprint: no cross-net bridging is possible.  Both engines
// must contribute zero so a star-tied device cannot inflate chain length.
BOOST_AUTO_TEST_CASE( ThreePadSingleNetFootprintAgreesAtZero )
{
    BOARD board;

    NETINFO_ITEM* netA = addNet( &board, wxS( "/A" ), 1, wxS( "SIG" ) );

    FOOTPRINT* fp = addFootprint( &board );
    addPad( fp, netA, VECTOR2I( -2'000'000, 0 ) );
    addPad( fp, netA, VECTOR2I(          0, 0 ) );
    addPad( fp, netA, VECTOR2I(  2'000'000, 0 ) );

    double drcLen = BoardChainBridgingLength( &board, wxS( "SIG" ) );
    long long tunerLen = tunerBridging( &board, wxS( "SIG" ) );

    BOOST_CHECK_EQUAL( drcLen, 0.0 );
    BOOST_CHECK_EQUAL( tunerLen, 0 );
}


BOOST_AUTO_TEST_SUITE_END()
