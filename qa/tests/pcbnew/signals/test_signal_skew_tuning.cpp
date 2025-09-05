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
        n->SetSignal( wxS("SIG_CLK") );

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

BOOST_AUTO_TEST_SUITE_END()
