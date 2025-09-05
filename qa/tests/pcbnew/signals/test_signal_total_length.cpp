/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright The KiCad Developers.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation, either version 3 of the License, or (at your
 * option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program.  If not, see <http://www.gnu.org/licenses/>.
 */
#include <boost/test/unit_test.hpp>
#include <board.h>
#include <pcb_track.h>
#include <netinfo.h>
#include <router/pns_kicad_iface.h>

static const long long MM = 1000000LL; // internal nanometers per mm

BOOST_AUTO_TEST_SUITE( SignalTotalLength )

BOOST_AUTO_TEST_CASE( SignalAggregatePlusNetEqualsPadSpacing )
{
    std::unique_ptr<BOARD> board = std::make_unique<BOARD>();
    // Create 4 nets making a 30 mm chain (0-8, 8-18, 18-23, 23-30)
    NETINFO_ITEM* n1 = new NETINFO_ITEM( board.get(), wxS("N1"), 1 ); board->Add( n1 );
    NETINFO_ITEM* n2 = new NETINFO_ITEM( board.get(), wxS("N2"), 2 ); board->Add( n2 );
    NETINFO_ITEM* n3 = new NETINFO_ITEM( board.get(), wxS("N3"), 3 ); board->Add( n3 );
    NETINFO_ITEM* n4 = new NETINFO_ITEM( board.get(), wxS("N4"), 4 ); board->Add( n4 );
    for( NETINFO_ITEM* n : { n1, n2, n3, n4 } ) n->SetSignal( wxS("Signal1") );

    auto addSeg = [&]( NETINFO_ITEM* net, double x1mm, double x2mm )
    {
        PCB_TRACK* t = new PCB_TRACK( board.get() );
        t->SetNetCode( net->GetNetCode() );
        t->SetStart( VECTOR2I( (long long)( x1mm * MM ), 0 ) );
        t->SetEnd( VECTOR2I( (long long)( x2mm * MM ), 0 ) );
        t->SetWidth( 100000 );
        board->Add( t );
    };
    addSeg( n1, 0.0, 8.0 );      // 8 mm
    addSeg( n2, 8.0, 18.0 );     // 10 mm
    addSeg( n3, 18.0, 23.0 );    // 5 mm
    addSeg( n4, 23.0, 30.0 );    // 7 mm

    PNS_KICAD_IFACE_BASE ifaceBase; ifaceBase.SetBoard( board.get() );
    long long extraLen = 0, extraDelay = 0;
    bool ok = ifaceBase.GetSignalAggregate( n1, n1, extraLen, extraDelay ); // all other nets
    BOOST_CHECK( ok );

    // Enumerate tracks for diagnostics
    int trackCount = 0;
    for( BOARD_ITEM* bi : board->Tracks() ) if( dynamic_cast<PCB_TRACK*>( bi ) ) trackCount++;

    // Compute routed length for net1 summing track+pad contributions per segment
    long long net1Len = 0;
    for( BOARD_ITEM* bi : board->Tracks() )
        if( auto tr = dynamic_cast<PCB_TRACK*>( bi ) )
            if( tr->GetNetCode() == n1->GetNetCode() )
                net1Len += ( tr->GetStart() - tr->GetEnd() ).EuclideanNorm();

    // net1Len should be 8mm, aggregated should be 22mm
    long long expectedNet1 = 8 * MM;
    long long expectedExtra = 22 * MM;
    BOOST_CHECK_EQUAL( net1Len, expectedNet1 );
    BOOST_CHECK_EQUAL( extraLen, expectedExtra );
    BOOST_CHECK( net1Len > 0 );

    long long total = net1Len + extraLen;

    // Expected: 30 mm end-to-end spacing between TP2 (0,0) and TP1 (30,0)
    long long expected = 30 * MM;

    // Allow small tolerance (1 micron) for rounding
    long long tol = 1000; // 0.001 mm
    BOOST_CHECK_MESSAGE( llabs( total - expected ) <= tol,
        "Total signal length " << total << " differs from expected " << expected << " (extra=" << extraLen << " net1=" << net1Len << ")" );
}

BOOST_AUTO_TEST_SUITE_END()
