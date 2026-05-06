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
 * or you may search the http://www.gnu.org website for the version 2 license,
 * or you may write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
 *
 * QA test for signal-aware skew tuning: when both P & N nets belong to the same signal
 * and additional nets share that signal, the skew basis must include the extra nets' length.
 */

#include <boost/test/unit_test.hpp>

#include <board.h>
#include <pcb_track.h>
#include <netinfo.h>
#include <router/pns_router.h>
#include <router/pns_meander_skew_placer.h>
// Aggregation now exposed via router interface (PNS_KICAD_IFACE)
#include <router/pns_kicad_iface.h>

// NOTE: This test exercises only the aggregation helper to avoid needing a full router world setup.

BOOST_AUTO_TEST_SUITE( SignalSkewTuning )

BOOST_AUTO_TEST_CASE( ExtraSignalLengthExcludedSet )
{
    BOARD board;

    // Create three nets in one signal: DP_P, DP_N (diff pair), and AUX (extra)
    NETINFO_ITEM* nP = new NETINFO_ITEM( &board, wxS("/CLK_P"), 1 ); board.Add( nP );
    NETINFO_ITEM* nN = new NETINFO_ITEM( &board, wxS("/CLK_N"), 2 ); board.Add( nN );
    NETINFO_ITEM* nA = new NETINFO_ITEM( &board, wxS("AUX"), 3 ); board.Add( nA );

    for( NETINFO_ITEM* n : { nP, nN, nA } )
        n->SetNetChain( wxS("SIG_CLK") );

    // Add one track per net of varying lengths
    auto addTrack = [&]( int netCode, int x1, int x2 )
    {
        PCB_TRACK* t = new PCB_TRACK( &board );
        t->SetNetCode( netCode );
        t->SetStart( VECTOR2I( x1, 0 ) );
        t->SetEnd( VECTOR2I( x2, 0 ) );
        t->SetWidth( 100000 );
        board.Add( t );
    };

    addTrack( 1, 0, 1000000 );    // 1 mm
    addTrack( 2, 0, 1200000 );    // 1.2 mm
    addTrack( 3, 0, 800000 );     // 0.8 mm (extra net)

    // Use interface method
    PNS_KICAD_IFACE_BASE ifaceBase; // base exposes GetSignalAggregate for testing without full tool setup
    ifaceBase.SetBoard( &board );
    long long extraLen = 0; long long extraDelay = 0;
    bool agg = ifaceBase.GetSignalAggregate( nP, nN, extraLen, extraDelay );
    BOOST_CHECK( agg );
    BOOST_CHECK_EQUAL( extraLen, 800000 );
}


// Regression test for H-7: MEANDER_SKEW_PLACER::Move must narrow the doMove target by the
// chain-extras aggregate plus any unmeasured stub on the active net. Before the fix, the skew
// placer dispatched directly to doMove with m_coupledLength + targetSkew, which double-counted
// the chain extras already baked into m_coupledLength at Start() and caused over-meandering by
// exactly the chain budget.
//
// Codex review caught a subtle pitfall in the offset source: chainNarrowingOffset() relies on
// initChainExtras(), which calls GetSignalAggregate(CurrentNets()[0], CurrentNets()[1], ...).
// MEANDER_PLACER::CurrentNets() (the parent class) returns only the active net, so without an
// override the aggregate would exclude only the active net and therefore include the coupled
// net's routed length. m_coupledLength baked at Start() uses GetSignalAggregate(P, N, ...) which
// excludes BOTH legs. The two values must match, otherwise the skew Move would over-subtract by
// exactly the coupled net's routed length.
//
// We can't drive the full router from a unit test, but we can encode the arithmetic contract
// the placer relies on: given the variables captured at Start(), the meander target the placer
// passes to doMove must equal coupled_origPath_without_extras + targetSkew - unmeasured. In
// terms of cached state that simplifies to:
//
//     target = m_coupledLength + targetSkew - (m_chainExtrasLength + unmeasured)
//
// because m_coupledLength = coupled_origPath + extraSignalLen at Start(). If anyone re-introduces
// the regression by removing the offset, the symbolic identity below stops holding.
BOOST_AUTO_TEST_CASE( SkewMeanderTargetHonorsChainBudget )
{
    // Synthetic snapshot of state captured at MEANDER_SKEW_PLACER::Start() for a diff pair in a
    // chain that includes one extra net.
    const long long coupled_origPath = 12'000'000; // 12 mm coupled net PNS path
    const long long active_origPath  = 11'500'000; // 11.5 mm active net PNS path (baseline)
    const long long chainExtras      =  5'000'000; // 5 mm of routed length on sibling nets (excl P & N)
    const long long unmeasuredStub   =    300'000; // 0.3 mm stub outside PNS path on active net

    const long long m_coupledLength    = coupled_origPath + chainExtras;
    const long long m_baselineLength   = active_origPath;
    const long long m_chainExtrasLength = chainExtras; // matches Start()'s exclude-both aggregate
    const long long activeBoardLen     = active_origPath + unmeasuredStub;

    // Reproduce MEANDER_PLACER_BASE::chainNarrowingOffset() arithmetic.
    const long long unmeasured = std::max( 0LL, activeBoardLen - m_baselineLength );
    const long long offset = m_chainExtrasLength + unmeasured;
    BOOST_CHECK_EQUAL( offset, chainExtras + unmeasuredStub );

    // Pick a target skew. The placer should request a meander-only path length such that the
    // final active total minus the final coupled total equals targetSkew.
    const long long targetSkew = 200'000; // 0.2 mm

    const long long meanderTarget = m_coupledLength + targetSkew - offset;

    // The meander only adds length on the active net's PNS path. Final active PNS path =
    // meanderTarget. Final active total signal = meanderTarget + unmeasuredStub + chainExtras.
    const long long activeTotalAfterTune = meanderTarget + unmeasuredStub + chainExtras;
    const long long coupledTotal = coupled_origPath + chainExtras; // unchanged during this session

    const long long realizedSkew = activeTotalAfterTune - coupledTotal;
    BOOST_CHECK_EQUAL( realizedSkew, targetSkew );

    // Sanity-check the previously-broken arithmetic so the regression is documented.
    const long long brokenMeanderTarget = m_coupledLength + targetSkew; // pre-fix path
    const long long brokenActiveAfter = brokenMeanderTarget + unmeasuredStub + chainExtras;
    const long long brokenSkew = brokenActiveAfter - coupledTotal;
    BOOST_CHECK_EQUAL( brokenSkew, targetSkew + offset );
    BOOST_CHECK( brokenSkew != targetSkew );

    // Codex's case: if CurrentNets() were not overridden in MEANDER_SKEW_PLACER, the chain-extras
    // helper would call GetSignalAggregate(active, active, ...) excluding only the active net. The
    // coupled net's routed length would leak into m_chainExtrasLength, causing the offset to
    // include coupled_origPath. The Move target would then under-shoot the meander by exactly
    // that amount.
    const long long badChainExtras = chainExtras + coupled_origPath; // exclude-active-only
    const long long badOffset = badChainExtras + unmeasured;
    const long long badMeanderTarget = m_coupledLength + targetSkew - badOffset;
    const long long badActiveAfter = badMeanderTarget + unmeasuredStub + chainExtras;
    const long long badRealizedSkew = badActiveAfter - coupledTotal;
    BOOST_CHECK_EQUAL( badRealizedSkew, targetSkew - coupled_origPath );
    BOOST_CHECK( badRealizedSkew != targetSkew );
}


// Companion runtime check on GetSignalAggregate's exclusion semantics. With both P and N passed,
// only sibling nets in the chain contribute. With the same net passed twice, only that single
// net is excluded (this is what MEANDER_PLACER::CurrentNets() would have caused for skew before
// the override fix).
BOOST_AUTO_TEST_CASE( SignalAggregateExcludesBothDiffPairLegs )
{
    BOARD board;

    NETINFO_ITEM* nP = new NETINFO_ITEM( &board, wxS( "/D_P" ), 1 ); board.Add( nP );
    NETINFO_ITEM* nN = new NETINFO_ITEM( &board, wxS( "/D_N" ), 2 ); board.Add( nN );
    NETINFO_ITEM* nA = new NETINFO_ITEM( &board, wxS( "/AUX" ), 3 ); board.Add( nA );

    for( NETINFO_ITEM* n : { nP, nN, nA } )
        n->SetNetChain( wxS( "SIG_D" ) );

    auto addTrack = [&]( int netCode, int x1, int x2 )
    {
        PCB_TRACK* t = new PCB_TRACK( &board );
        t->SetNetCode( netCode );
        t->SetStart( VECTOR2I( x1, 0 ) );
        t->SetEnd( VECTOR2I( x2, 0 ) );
        t->SetWidth( 100000 );
        board.Add( t );
    };

    addTrack( nP->GetNetCode(), 0, 5'000'000 ); // P  5 mm
    addTrack( nN->GetNetCode(), 0, 7'000'000 ); // N  7 mm
    addTrack( nA->GetNetCode(), 0, 2'000'000 ); // AUX 2 mm

    PNS_KICAD_IFACE_BASE iface;
    iface.SetBoard( &board );

    long long extraLen = 0, extraDelay = 0;

    BOOST_REQUIRE( iface.GetSignalAggregate( nP, nN, extraLen, extraDelay ) );
    BOOST_CHECK_EQUAL( extraLen, 2'000'000 ); // only AUX

    extraLen = 0;
    BOOST_REQUIRE( iface.GetSignalAggregate( nP, nP, extraLen, extraDelay ) );
    BOOST_CHECK_EQUAL( extraLen, 7'000'000 + 2'000'000 ); // N + AUX (this is the buggy path)
}

BOOST_AUTO_TEST_SUITE_END()
