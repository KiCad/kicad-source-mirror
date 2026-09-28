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
#include <board_design_settings.h>
#include <connectivity/connectivity_data.h>
#include <footprint.h>
#include <pad.h>
#include <pcb_track.h>

#include <memory>


BOOST_AUTO_TEST_SUITE( ViaFlashingContactLayer )


static void setupFourLayerBoard( BOARD& aBoard )
{
    aBoard.SetCopperLayerCount( 4 );
    aBoard.SetEnabledLayers( aBoard.GetEnabledLayers() | LSET::AllCuMask( 4 ) );
    aBoard.GetDesignSettings().SetCopperLayerCount( 4 );
}


static PCB_VIA* addThroughVia( BOARD& aBoard, NETINFO_ITEM* aNet, const VECTOR2I& aPos )
{
    PCB_VIA* via = new PCB_VIA( &aBoard );

    via->SetViaType( VIATYPE::THROUGH );
    via->SetPadstackMode( PADSTACK::MODE::NORMAL );
    via->SetPosition( aPos );
    via->SetLayerPair( F_Cu, B_Cu );
    via->SetWidth( PADSTACK::ALL_LAYERS, pcbIUScale.mmToIU( 0.6 ) );
    via->SetDrill( pcbIUScale.mmToIU( 0.3 ) );
    via->Padstack().SetUnconnectedLayerMode( UNCONNECTED_LAYER_MODE::REMOVE_EXCEPT_START_AND_END );
    via->SetNet( aNet );

    aBoard.Add( via );
    return via;
}


BOOST_AUTO_TEST_CASE( ViaTouchingPadOnFrontOnlyHasNoInnerRings )
{
    BOARD board;
    setupFourLayerBoard( board );

    NETINFO_ITEM* net = new NETINFO_ITEM( &board, "GND", 1 );
    board.Add( net );

    std::unique_ptr<FOOTPRINT> footprint = std::make_unique<FOOTPRINT>( &board );
    PAD*                       pad = new PAD( footprint.get() );

    const int outerDiameter = pcbIUScale.mmToIU( 5.0 );
    const int innerDiameter = pcbIUScale.mmToIU( 3.0 );
    const int holeDiameter = pcbIUScale.mmToIU( 2.5 );

    pad->SetAttribute( PAD_ATTRIB::PTH );
    pad->SetLayerSet( LSET::AllCuMask() );
    pad->SetPadstackMode( PADSTACK::MODE::FRONT_INNER_BACK );
    pad->SetSize( F_Cu, VECTOR2I( outerDiameter, outerDiameter ) );
    pad->SetSize( PADSTACK::INNER_LAYERS, VECTOR2I( innerDiameter, innerDiameter ) );
    pad->SetSize( B_Cu, VECTOR2I( outerDiameter, outerDiameter ) );
    pad->SetDrillSize( VECTOR2I( holeDiameter, holeDiameter ) );
    pad->SetPosition( VECTOR2I( 0, 0 ) );
    pad->SetUnconnectedLayerMode( UNCONNECTED_LAYER_MODE::REMOVE_EXCEPT_START_AND_END );
    pad->SetNet( net );

    footprint->Add( pad );
    board.Add( footprint.release() );

    // 2.2 mm puts the via inside the 5 mm front copper and 0.4 mm clear of the 3 mm inner copper
    PCB_VIA* via = addThroughVia( board, net, VECTOR2I( pcbIUScale.mmToIU( 2.2 ), 0 ) );

    board.BuildConnectivity();

    BOOST_REQUIRE_MESSAGE( board.GetConnectivity()->IsConnectedOnLayer( via, F_Cu, { PCB_PAD_T } ),
                           "the via and the pad do not touch on F.Cu, so the rest of this test proves nothing" );

    BOOST_CHECK_MESSAGE( via->FlashLayer( F_Cu ), "the via lost its ring on its start layer" );
    BOOST_CHECK_MESSAGE( via->FlashLayer( B_Cu ), "the via lost its ring on its end layer" );
    BOOST_CHECK_MESSAGE( pad->FlashLayer( F_Cu ), "the pad lost its front copper" );

    BOOST_CHECK_MESSAGE( !via->FlashLayer( In1_Cu ), "the via has a ring on In1.Cu where it touches nothing" );
    BOOST_CHECK_MESSAGE( !via->FlashLayer( In2_Cu ), "the via has a ring on In2.Cu where it touches nothing" );
    BOOST_CHECK_MESSAGE( !pad->FlashLayer( In1_Cu ), "the pad has copper on In1.Cu where it touches nothing" );
    BOOST_CHECK_MESSAGE( !pad->FlashLayer( In2_Cu ), "the pad has copper on In2.Cu where it touches nothing" );
}


BOOST_AUTO_TEST_CASE( ViaKeepsTheRingOnTheLayerItsTrackIsOn )
{
    BOARD board;
    setupFourLayerBoard( board );

    NETINFO_ITEM* net = new NETINFO_ITEM( &board, "GND", 1 );
    board.Add( net );

    PCB_VIA* via = addThroughVia( board, net, VECTOR2I( 0, 0 ) );

    PCB_TRACK* track = new PCB_TRACK( &board );

    track->SetLayer( In1_Cu );
    track->SetStart( VECTOR2I( 0, 0 ) );
    track->SetEnd( VECTOR2I( pcbIUScale.mmToIU( 5.0 ), 0 ) );
    track->SetWidth( pcbIUScale.mmToIU( 0.25 ) );
    track->SetNet( net );
    board.Add( track );

    board.BuildConnectivity();

    BOOST_REQUIRE_MESSAGE( board.GetConnectivity()->IsConnectedOnLayer( via, In1_Cu, { PCB_TRACE_T } ),
                           "the via and the track do not touch on In1.Cu, so this test proves nothing" );

    BOOST_CHECK_MESSAGE( via->FlashLayer( In1_Cu ), "the via lost the ring on the layer its track is on" );
    BOOST_CHECK_MESSAGE( !via->FlashLayer( In2_Cu ), "the via has a ring on In2.Cu where it touches nothing" );
}


BOOST_AUTO_TEST_SUITE_END()
