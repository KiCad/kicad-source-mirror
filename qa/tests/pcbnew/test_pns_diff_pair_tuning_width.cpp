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

#include <qa_utils/wx_utils/unit_test_utils.h>

#include <board.h>
#include <pad.h>
#include <pcb_track.h>
#include <pcbnew_utils/board_test_utils.h>
#include <settings/settings_manager.h>

#include <router/pns_diff_pair.h>
#include <router/pns_item.h>
#include <router/pns_kicad_iface.h>
#include <router/pns_line.h>
#include <router/pns_placement_algo.h>
#include <router/pns_router.h>
#include <router/pns_routing_settings.h>
#include <router/pns_sizes_settings.h>
#include <router/pns_topology.h>
#include <router/pns_via.h>

#include <memory>


struct DIFF_PAIR_TUNING_WIDTH_FIXTURE
{
    SETTINGS_MANAGER       m_settingsManager;
    std::unique_ptr<BOARD> m_board;
};


static void checkLineUsesOnlyTrackWidth( const PNS::LINE& aLine )
{
    BOOST_REQUIRE_GT( aLine.LinkCount(), 0 );

    const int lineWidth = aLine.Width();

    for( const PNS::LINKED_ITEM* link : aLine.Links() )
    {
        BOOST_CHECK_EQUAL( link->Width(), lineWidth );
    }
}


BOOST_FIXTURE_TEST_CASE( DiffPairTuningStopsAtTrackWidthChanges, DIFF_PAIR_TUNING_WIDTH_FIXTURE )
{
    KI_TEST::LoadBoard( m_settingsManager, "issue23550/issue23550", m_board );
    BOOST_REQUIRE( m_board );

    PCB_TRACK& startTrack = static_cast<PCB_TRACK&>( KI_TEST::RequireBoardItemWithTypeAndId(
            *m_board, PCB_TRACE_T, KIID( "5ca809fb-b7f4-4a3c-b239-272a2b12320f" ) ) );

    PNS::ROUTER          router;
    PNS_KICAD_IFACE_BASE iface;
    iface.SetBoard( m_board.get() );
    router.SetInterface( &iface );
    router.ClearWorld();
    router.SyncWorld();

    PNS::ITEM* startItem = router.GetWorld()->FindItemByParent( &startTrack );
    BOOST_REQUIRE( startItem );

    PNS::DIFF_PAIR pair;
    PNS::TOPOLOGY  topology( router.GetWorld() );

    BOOST_REQUIRE( topology.AssembleDiffPair( startItem, pair ) );

    BOOST_CHECK_EQUAL( pair.PLine().Width(), pcbIUScale.mmToIU( 0.085 ) );
    BOOST_CHECK_EQUAL( pair.NLine().Width(), pcbIUScale.mmToIU( 0.085 ) );

    checkLineUsesOnlyTrackWidth( pair.PLine() );
    checkLineUsesOnlyTrackWidth( pair.NLine() );
}


// Picking an inner target layer for a via sets a partial layer pair. A through via still has
// to span the whole board, or its annular ring stops being drawn below the target layer.
BOOST_FIXTURE_TEST_CASE( DiffPairThroughViaSpansFullStack, DIFF_PAIR_TUNING_WIDTH_FIXTURE )
{
    KI_TEST::LoadBoard( m_settingsManager, "issue23550/issue23550", m_board );
    BOOST_REQUIRE( m_board );

    PAD& startPad = static_cast<PAD&>( KI_TEST::RequireBoardItemWithTypeAndId(
            *m_board, PCB_PAD_T, KIID( "ac8b01c9-e917-4508-be81-bb61731d5b93" ) ) );

    PNS::ROUTING_SETTINGS settings( nullptr, "" );
    PNS::ROUTER           router;
    PNS_KICAD_IFACE_BASE  iface;

    iface.SetBoard( m_board.get() );
    router.SetInterface( &iface );
    router.ClearWorld();
    router.SyncWorld();
    router.LoadSettings( &settings );
    router.SetMode( PNS::PNS_MODE_ROUTE_DIFF_PAIR );

    PNS::ITEM* startItem = router.GetWorld()->FindItemByParent( &startPad );
    BOOST_REQUIRE( startItem );

    const int pnsFront = iface.GetPNSLayerFromBoardLayer( F_Cu );
    const int pnsBack = iface.GetPNSLayerFromBoardLayer( B_Cu );
    const int pnsTarget = iface.GetPNSLayerFromBoardLayer( In11_Cu );

    BOOST_REQUIRE_MESSAGE( router.StartRouting( startPad.GetPosition(), startItem, pnsFront ), router.FailureReason() );

    PNS::SIZES_SETTINGS sizes( router.Sizes() );
    sizes.SetViaType( VIATYPE::THROUGH );
    sizes.SetViaDiameter( pcbIUScale.mmToIU( 0.6 ) );
    sizes.SetViaDrill( pcbIUScale.mmToIU( 0.3 ) );
    sizes.ClearLayerPairs();
    sizes.AddLayerPair( pnsFront, pnsTarget );
    router.UpdateSizes( sizes );

    router.ToggleViaPlacement();
    BOOST_REQUIRE( router.IsPlacingVia() );

    router.Move( startPad.GetPosition() + VECTOR2I( pcbIUScale.mmToIU( 2 ), pcbIUScale.mmToIU( 2 ) ), nullptr );

    const PNS::ITEM_SET traces = router.Placer()->Traces();

    int viasChecked = 0;

    for( const PNS::ITEM* item : traces.CItems() )
    {
        if( !item->OfKind( PNS::ITEM::LINE_T ) )
            continue;

        const PNS::LINE* line = static_cast<const PNS::LINE*>( item );

        if( !line->EndsWithVia() )
            continue;

        const PNS::VIA& via = line->Via();
        viasChecked++;

        BOOST_CHECK_EQUAL( via.Layers().Start(), pnsFront );
        BOOST_CHECK_EQUAL( via.Layers().End(), pnsBack );
        BOOST_CHECK_EQUAL( via.HoleLayers().Start(), pnsFront );
        BOOST_CHECK_EQUAL( via.HoleLayers().End(), pnsBack );
    }

    router.StopRouting();

    BOOST_REQUIRE_EQUAL( viasChecked, 2 );
}
