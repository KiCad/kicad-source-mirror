/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2024 Jon Evans <jon@craftyjon.com>
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
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
#include <import_export.h>
#include <qa_utils/api_test_utils.h>
#include <qa_utils/wx_utils/wx_assert.h>
#include <pcbnew_utils/board_test_utils.h>
#include <settings/settings_manager.h>

#include <api/board/board_types.pb.h>

#include <board.h>
#include <footprint.h>
#include <pcb_barcode.h>
#include <pcb_dimension.h>
#include <pcb_reference_image.h>
#include <pcb_track.h>
#include <zone.h>


BOOST_AUTO_TEST_SUITE( ApiProto )

struct PROTO_TEST_FIXTURE
{
    PROTO_TEST_FIXTURE()
    { }

    SETTINGS_MANAGER       m_settingsManager;
    std::unique_ptr<BOARD> m_board;
};


BOOST_FIXTURE_TEST_CASE( BoardTypes, PROTO_TEST_FIXTURE )
{
    KI_TEST::LoadBoard( m_settingsManager, "api_kitchen_sink", m_board );

    int barcodeCount = 0;
    int referenceImageCount = 0;

    for( PCB_TRACK* track : m_board->Tracks() )
    {
        switch( track->Type() )
        {
        case PCB_TRACE_T:
            testProtoFromKiCadObject<kiapi::board::types::Track>( track, m_board.get() );
            break;

        case PCB_ARC_T:
            testProtoFromKiCadObject<kiapi::board::types::Arc>( static_cast<PCB_ARC*>( track ),
                                                                m_board.get() );
            break;

        case PCB_VIA_T:
            // Vias are not strict-checked at the moment because m_zoneLayerOverrides is not
            // currently exposed to the API
            // TODO(JE) enable strict when fixed
            testProtoFromKiCadObject<kiapi::board::types::Via>( static_cast<PCB_VIA*>( track ),
                                                                m_board.get(), false );
            break;

        default:
            wxFAIL;
        }
    }

    for( FOOTPRINT* footprint : m_board->Footprints() )
        testProtoFromKiCadObject<kiapi::board::types::FootprintInstance>( footprint, m_board.get() );

    for( ZONE* zone : m_board->Zones() )
        testProtoFromKiCadObject<kiapi::board::types::Zone>( zone, m_board.get() );

    for( BOARD_ITEM* item : m_board->Drawings() )
    {
        switch( item->Type() )
        {
        case PCB_DIM_ALIGNED_T:
            testProtoFromKiCadObject<kiapi::board::types::Dimension>(
                    static_cast<PCB_DIM_ALIGNED*>( item ), m_board.get() );
            break;

        case PCB_DIM_ORTHOGONAL_T:
            testProtoFromKiCadObject<kiapi::board::types::Dimension>(
                    static_cast<PCB_DIM_ORTHOGONAL*>( item ), m_board.get() );
            break;

        case PCB_DIM_CENTER_T:
            testProtoFromKiCadObject<kiapi::board::types::Dimension>(
                    static_cast<PCB_DIM_CENTER*>( item ), m_board.get() );
            break;

        case PCB_DIM_LEADER_T:
            testProtoFromKiCadObject<kiapi::board::types::Dimension>(
                    static_cast<PCB_DIM_LEADER*>( item ), m_board.get() );
            break;

        case PCB_DIM_RADIAL_T:
            testProtoFromKiCadObject<kiapi::board::types::Dimension>(
                    static_cast<PCB_DIM_RADIAL*>( item ), m_board.get() );
            break;

        case PCB_BARCODE_T:
            testProtoFromKiCadObject<kiapi::board::types::Barcode>(
                    static_cast<PCB_BARCODE*>( item ), m_board.get() );
            ++barcodeCount;
            break;

        case PCB_REFERENCE_IMAGE_T:
            testProtoFromKiCadObject<kiapi::board::types::ReferenceImage>(
                    static_cast<PCB_REFERENCE_IMAGE*>( item ), m_board.get() );
            ++referenceImageCount;
            break;

        default: break;
        }
        // TODO(JE) Shapes

        // TODO(JE) Text
    }

    BOOST_CHECK_GT( barcodeCount, 0 );
    BOOST_CHECK_GT( referenceImageCount, 0 );
}


BOOST_FIXTURE_TEST_CASE( Padstacks, PROTO_TEST_FIXTURE )
{
    KI_TEST::LoadBoard( m_settingsManager, "padstacks", m_board );

    for( PCB_TRACK* track : m_board->Tracks() )
    {
        switch( track->Type() )
        {
        case PCB_VIA_T:
            // Vias are not strict-checked at the moment because m_zoneLayerOverrides is not
            // currently exposed to the API
            // TODO(JE) enable strict when fixed
            testProtoFromKiCadObject<kiapi::board::types::Via>( static_cast<PCB_VIA*>( track ),
                                                                m_board.get(), false );
            break;

        default:
            wxFAIL;
        }
    }

    for( FOOTPRINT* footprint : m_board->Footprints() )
        testProtoFromKiCadObject<kiapi::board::types::FootprintInstance>( footprint, m_board.get() );
}

BOOST_AUTO_TEST_SUITE_END()
