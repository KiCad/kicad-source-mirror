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
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

/**
 * @file test_pns_tuning_path_through_pad.cpp
 *
 * Length tuning must follow a single net all the way between its terminal pads, even when the
 * trace routes through an in-line pad (e.g. a series / strapping resistor) mid-net.  The fixture
 * is one net (Net-(R1-Pad2)) routed TP1 -- TP2.pad -- R1.pad2, where TP2's pad sits on the path.
 * AssembleTuningPath used to treat any pad as a hard terminal, so it stopped at TP2 and reported
 * only part of the net.
 */

#include <qa_utils/wx_utils/unit_test_utils.h>
#include <pcbnew_utils/board_test_utils.h>
#include <board.h>
#include <footprint.h>
#include <pad.h>
#include <pcb_track.h>
#include <settings/settings_manager.h>

#include <router/pns_router.h>
#include <router/pns_kicad_iface.h>
#include <router/pns_topology.h>
#include <router/pns_item.h>
#include <router/pns_itemset.h>
#include <router/pns_line.h>
#include <router/pns_solid.h>

#include <set>


struct TUNING_PATH_FIXTURE
{
    SETTINGS_MANAGER       m_settingsManager;
    std::unique_ptr<BOARD> m_board;
};


static wxString refOfSolid( PNS::SOLID* aSolid )
{
    if( !aSolid || !aSolid->Parent() || aSolid->Parent()->Type() != PCB_PAD_T )
        return wxEmptyString;

    return static_cast<PAD*>( aSolid->Parent() )->GetParentFootprint()->GetReference();
}


BOOST_FIXTURE_TEST_CASE( TuningPathWalksThroughInlinePad, TUNING_PATH_FIXTURE )
{
    KI_TEST::LoadBoard( m_settingsManager, "tuning_path_through_pad", m_board );

    NETINFO_ITEM* net = m_board->FindNet( "Net-(R1-Pad2)" );
    BOOST_REQUIRE( net );

    // The in-line resistor pad that the trace routes through.
    PAD* inlinePad = m_board->FindFootprintByReference( "R1" )->FindPadByNumber( "2" );
    BOOST_REQUIRE( inlinePad );

    // Reference: the whole-net copper length, as the Net Inspector reports it, plus a start
    // segment anchored on the resistor pad so the walk must cross TP2 to reach TP1.
    int64_t    fullNetLength = 0;
    PCB_TRACK* startTrack = nullptr;

    for( PCB_TRACK* t : m_board->Tracks() )
    {
        if( t->GetNetCode() != net->GetNetCode() )
            continue;

        fullNetLength += t->GetLength();

        if( !startTrack && ( t->GetStart() == inlinePad->GetPosition() || t->GetEnd() == inlinePad->GetPosition() ) )
            startTrack = t;
    }

    BOOST_REQUIRE( startTrack );
    BOOST_REQUIRE_GT( fullNetLength, 0 );

    // Build a PNS world from the board the same way the router does.
    PNS::ROUTER          router;
    PNS_KICAD_IFACE_BASE iface;
    iface.SetBoard( m_board.get() );
    router.SetInterface( &iface );
    router.ClearWorld();
    router.SyncWorld();

    PNS::ITEM* startItem = router.GetWorld()->FindItemByParent( startTrack );
    BOOST_REQUIRE( startItem );

    PNS::TOPOLOGY topo( router.GetWorld() );
    PNS::SOLID*   startPad = nullptr;
    PNS::SOLID*   endPad = nullptr;

    const PNS::ITEM_SET path = topo.AssembleTuningPath( &iface, startItem, &startPad, &endPad );

    std::set<wxString> terminals = { refOfSolid( startPad ), refOfSolid( endPad ) };

    BOOST_TEST_MESSAGE( "terminal pads: " + refOfSolid( startPad ) + " / " + refOfSolid( endPad ) );

    // The path must terminate on the real net ends (TP1 and R1), never on the in-line pad TP2.
    BOOST_CHECK( terminals.count( "TP1" ) );
    BOOST_CHECK( terminals.count( "R1" ) );
    BOOST_CHECK( !terminals.count( "TP2" ) );

    // ... and it must cover (nearly) the whole net, not just one side of TP2.
    int64_t pathLength = 0;

    for( int i = 0; i < path.Size(); i++ )
    {
        if( path[i]->Kind() == PNS::ITEM::LINE_T )
            pathLength += static_cast<const PNS::LINE*>( path[i] )->CLine().Length();
    }

    BOOST_TEST_MESSAGE( wxString::Format( "tuned path = %lld nm, full net = %lld nm", pathLength, fullNetLength ) );

    BOOST_CHECK_GT( pathLength, fullNetLength * 9 / 10 );
}
