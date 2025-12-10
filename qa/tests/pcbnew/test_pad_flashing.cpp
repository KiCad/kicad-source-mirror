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
 */

#include <qa_utils/wx_utils/unit_test_utils.h>

#include <board.h>
#include <footprint.h>
#include <pad.h>
#include <memory>

BOOST_AUTO_TEST_SUITE( PadFlashing )

BOOST_AUTO_TEST_CASE( PadsInSameFootprintDoNotForceInnerLayerFlashing )
{
    BOARD board;
    board.SetBoardUse( BOARD_USE::FPHOLDER );

    auto footprint = std::make_unique<FOOTPRINT>( &board );

    auto net = new NETINFO_ITEM( &board, "P1", 1 );
    board.Add( net );

    auto pad1 = new PAD( footprint.get() );
    auto pad2 = new PAD( footprint.get() );

    const int diameter = pcbIUScale.mmToIU( 1.0 );
    const int drill    = pcbIUScale.mmToIU( 0.5 );

    pad1->SetAttribute( PAD_ATTRIB::PTH );
    pad2->SetAttribute( PAD_ATTRIB::PTH );
    pad1->SetLayerSet( LSET::AllCuMask() );
    pad2->SetLayerSet( LSET::AllCuMask() );
    pad1->SetSize( PADSTACK::ALL_LAYERS, VECTOR2I( diameter, diameter ) );
    pad2->SetSize( PADSTACK::ALL_LAYERS, VECTOR2I( diameter, diameter ) );
    pad1->SetDrillSize( VECTOR2I( drill, drill ) );
    pad2->SetDrillSize( VECTOR2I( drill, drill ) );
    pad1->SetPosition( VECTOR2I( 0, 0 ) );
    pad2->SetPosition( VECTOR2I( 0, 0 ) );
    pad1->SetUnconnectedLayerMode( UNCONNECTED_LAYER_MODE::REMOVE_EXCEPT_START_AND_END );
    pad2->SetUnconnectedLayerMode( UNCONNECTED_LAYER_MODE::REMOVE_EXCEPT_START_AND_END );
    pad1->SetNet( net );
    pad2->SetNet( net );

    footprint->Add( pad1 );
    footprint->Add( pad2 );
    board.Add( footprint.release() );

    board.BuildConnectivity();

    BOOST_CHECK( pad1->FlashLayer( F_Cu ) );
    BOOST_CHECK( pad2->FlashLayer( B_Cu ) );
    BOOST_CHECK( !pad1->FlashLayer( In1_Cu ) );
    BOOST_CHECK( !pad2->FlashLayer( In1_Cu ) );
}

BOOST_AUTO_TEST_CASE( PadsInDifferentFootprintsDoNotForceInnerLayerFlashing )
{
    BOARD board;
    board.SetBoardUse( BOARD_USE::FPHOLDER );

    auto footprint1 = std::make_unique<FOOTPRINT>( &board );
    auto footprint2 = std::make_unique<FOOTPRINT>( &board );

    auto net = new NETINFO_ITEM( &board, "P1", 1 );
    board.Add( net );

    auto pad1 = new PAD( footprint1.get() );
    auto pad2 = new PAD( footprint2.get() );

    const int diameter = pcbIUScale.mmToIU( 1.0 );
    const int drill    = pcbIUScale.mmToIU( 0.5 );

    pad1->SetAttribute( PAD_ATTRIB::PTH );
    pad2->SetAttribute( PAD_ATTRIB::PTH );
    pad1->SetLayerSet( LSET::AllCuMask() );
    pad2->SetLayerSet( LSET::AllCuMask() );
    pad1->SetSize( PADSTACK::ALL_LAYERS, VECTOR2I( diameter, diameter ) );
    pad2->SetSize( PADSTACK::ALL_LAYERS, VECTOR2I( diameter, diameter ) );
    pad1->SetDrillSize( VECTOR2I( drill, drill ) );
    pad2->SetDrillSize( VECTOR2I( drill, drill ) );
    pad1->SetPosition( VECTOR2I( 0, 0 ) );
    pad2->SetPosition( VECTOR2I( 0, 0 ) );
    pad1->SetUnconnectedLayerMode( UNCONNECTED_LAYER_MODE::REMOVE_EXCEPT_START_AND_END );
    pad2->SetUnconnectedLayerMode( UNCONNECTED_LAYER_MODE::REMOVE_EXCEPT_START_AND_END );
    pad1->SetNet( net );
    pad2->SetNet( net );

    footprint1->Add( pad1 );
    footprint2->Add( pad2 );
    board.Add( footprint1.release() );
    board.Add( footprint2.release() );

    board.BuildConnectivity();

    BOOST_CHECK( pad1->FlashLayer( F_Cu ) );
    BOOST_CHECK( pad2->FlashLayer( B_Cu ) );
    BOOST_CHECK( !pad1->FlashLayer( In1_Cu ) );
    BOOST_CHECK( !pad2->FlashLayer( In1_Cu ) );
}

BOOST_AUTO_TEST_SUITE_END()
