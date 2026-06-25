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

// Re-caching every remove-unconnected pad on each active-layer change stalled the start of a track
// drag on dense boards. activeLayerUpdateFlags() narrows that, but only with high contrast off.

#include <qa_utils/wx_utils/unit_test_utils.h>

#include <board.h>
#include <footprint.h>
#include <pad.h>
#include <pcb_shape.h>
#include <pcb_edit_frame.h>
#include <netinfo.h>
#include <view/view_item.h>
#include <project/board_project_settings.h>


BOOST_AUTO_TEST_SUITE( ActiveLayerUpdate )


BOOST_AUTO_TEST_CASE( OnlyFlashChangedPadsUpdateWithoutHighContrast )
{
    BOARD board;
    board.SetBoardUse( BOARD_USE::FPHOLDER );

    auto footprint = std::make_unique<FOOTPRINT>( &board );
    auto net = new NETINFO_ITEM( &board, "P1", 1 );
    board.Add( net );

    // PTH pad that flashes on the outer layers but not on an unconnected inner layer.
    PAD* pad = new PAD( footprint.get() );
    pad->SetAttribute( PAD_ATTRIB::PTH );
    pad->SetLayerSet( LSET::AllCuMask() );
    pad->SetSize( PADSTACK::ALL_LAYERS, VECTOR2I( pcbIUScale.mmToIU( 1.0 ), pcbIUScale.mmToIU( 1.0 ) ) );
    pad->SetDrillSize( VECTOR2I( pcbIUScale.mmToIU( 0.5 ), pcbIUScale.mmToIU( 0.5 ) ) );
    pad->SetUnconnectedLayerMode( UNCONNECTED_LAYER_MODE::REMOVE_EXCEPT_START_AND_END );
    pad->SetNet( net );
    footprint->Add( pad );
    board.Add( footprint.release() );
    board.BuildConnectivity();

    BOOST_REQUIRE( pad->FlashLayer( F_Cu ) );
    BOOST_REQUIRE( pad->FlashLayer( B_Cu ) );
    BOOST_REQUIRE( !pad->FlashLayer( In1_Cu ) );

    // Same flashing on both layers: nothing to re-cache.
    BOOST_CHECK_EQUAL(
            PCB_EDIT_FRAME::activeLayerUpdateFlags( pad, F_Cu, B_Cu, HIGH_CONTRAST_MODE::NORMAL ),
            KIGFX::NONE );

    BOOST_CHECK_EQUAL(
            PCB_EDIT_FRAME::activeLayerUpdateFlags( pad, F_Cu, In1_Cu, HIGH_CONTRAST_MODE::NORMAL ),
            KIGFX::ALL );

    // High contrast also dims by active layer, so the redraw is kept even when flashing is unchanged.
    BOOST_CHECK_EQUAL(
            PCB_EDIT_FRAME::activeLayerUpdateFlags( pad, F_Cu, B_Cu, HIGH_CONTRAST_MODE::DIMMED ),
            KIGFX::ALL );

    PCB_SHAPE shape( &board, SHAPE_T::RECTANGLE );
    BOOST_CHECK_EQUAL(
            PCB_EDIT_FRAME::activeLayerUpdateFlags( &shape, F_Cu, In1_Cu, HIGH_CONTRAST_MODE::NORMAL ),
            KIGFX::NONE );
}


BOOST_AUTO_TEST_SUITE_END()
