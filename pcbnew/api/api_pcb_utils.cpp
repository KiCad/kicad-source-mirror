/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2023 Jon Evans <jon@craftyjon.com>
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

#include <api/api_pcb_utils.h>
#include <api/api_enums.h>
#include <api/api_utils.h>
#include <board.h>
#include <teardrop/teardrop_parameters.h>
#include <board_item_container.h>
#include <footprint.h>
#include <lset.h>
#include <pad.h>
#include <pcb_group.h>
#include <pcb_barcode.h>
#include <pcb_reference_image.h>
#include <pcb_shape.h>
#include <pcb_point.h>
#include <pcb_track.h>
#include <pcb_field.h>
#include <pcb_text.h>
#include <pcb_textbox.h>
#include <pcb_table.h>
#include <pcb_dimension.h>
#include <zone.h>


std::unique_ptr<BOARD_ITEM> CreateItemForType( KICAD_T aType, BOARD_ITEM_CONTAINER* aContainer )
{
    switch( aType )
    {
    case PCB_TRACE_T:   return std::make_unique<PCB_TRACK>( aContainer );
    case PCB_ARC_T:     return std::make_unique<PCB_ARC>( aContainer );
    case PCB_VIA_T:     return std::make_unique<PCB_VIA>( aContainer );
    case PCB_TEXT_T:    return std::make_unique<PCB_TEXT>( aContainer );
    case PCB_TEXTBOX_T: return std::make_unique<PCB_TEXTBOX>( aContainer );
    case PCB_TABLE_T:   return std::make_unique<PCB_TABLE>( aContainer );
    case PCB_TABLECELL_T:
    {
        PCB_TABLE* table = dynamic_cast<PCB_TABLE*>( aContainer );

        if( !table )
            return nullptr;

        return std::make_unique<PCB_TABLECELL>( aContainer );
    }
    case PCB_SHAPE_T:   return std::make_unique<PCB_SHAPE>( aContainer );
    case PCB_POINT_T:   return std::make_unique<PCB_POINT>( aContainer );
    case PCB_BARCODE_T: return std::make_unique<PCB_BARCODE>( aContainer );
    case PCB_ZONE_T:    return std::make_unique<ZONE>( aContainer );
    case PCB_GROUP_T:   return std::make_unique<PCB_GROUP>( aContainer );
    case PCB_REFERENCE_IMAGE_T: return std::make_unique<PCB_REFERENCE_IMAGE>( aContainer );

    case PCB_PAD_T:
    {
        FOOTPRINT* footprint = dynamic_cast<FOOTPRINT*>( aContainer );

        if( !footprint )
            return nullptr;

        return std::make_unique<PAD>( footprint );
    }

    case PCB_FIELD_T:
    {
        FOOTPRINT* footprint = dynamic_cast<FOOTPRINT*>( aContainer );

        if( !footprint )
            return nullptr;

        return std::make_unique<PCB_FIELD>( footprint, FIELD_T::USER );
    }

    case PCB_FOOTPRINT_T:
    {
        BOARD* board = dynamic_cast<BOARD*>( aContainer );

        if( !board )
            return nullptr;

        return std::make_unique<FOOTPRINT>( board );
    }

    case PCB_DIM_ALIGNED_T: return std::make_unique<PCB_DIM_ALIGNED>( aContainer );
    case PCB_DIM_ORTHOGONAL_T: return std::make_unique<PCB_DIM_ORTHOGONAL>( aContainer );
    case PCB_DIM_RADIAL_T: return std::make_unique<PCB_DIM_RADIAL>( aContainer );
    case PCB_DIM_LEADER_T: return std::make_unique<PCB_DIM_LEADER>( aContainer );
    case PCB_DIM_CENTER_T: return std::make_unique<PCB_DIM_CENTER>( aContainer );

    default:
        return nullptr;
    }
}

namespace kiapi::board
{

void PackLayerSet( google::protobuf::RepeatedField<int>& aOutput, const LSET& aLayerSet )
{
    for( const PCB_LAYER_ID& layer : aLayerSet.Seq() )
        aOutput.Add( ToProtoEnum<PCB_LAYER_ID, types::BoardLayer>( layer ) );
}


LSET UnpackLayerSet( const google::protobuf::RepeatedField<int>& aProtoLayerSet )
{
    LSET set;

    for( int layer : aProtoLayerSet )
    {
        wxCHECK2( layer >= F_Cu && layer < PCB_LAYER_ID_COUNT, continue );
        PCB_LAYER_ID boardLayer =
                FromProtoEnum<PCB_LAYER_ID>( static_cast<types::BoardLayer>( layer ) );

        if( boardLayer >= 0 && IsValidLayer( boardLayer ) )
            set.set( boardLayer );
    }

    return set;
}


void PackTeardropSettings( types::PadTeardropSettings& aOutput, const TEARDROP_PARAMETERS& aParams )
{
    aOutput.set_mode( aParams.m_Enabled ? types::PadTeardropMode::PTM_ENABLED : types::PadTeardropMode::PTM_DISABLED );
    aOutput.set_curved_edges( aParams.m_CurvedEdges );
    aOutput.set_allow_multiple_track_segments( aParams.m_AllowUseTwoTracks );
    aOutput.set_prefer_zone_connection( !aParams.m_TdOnPadsInZones );
    aOutput.mutable_max_length()->set_value_nm( aParams.m_TdMaxLen );
    aOutput.mutable_max_width()->set_value_nm( aParams.m_TdMaxWidth );
    aOutput.set_best_length_ratio( aParams.m_BestLengthRatio );
    aOutput.set_best_width_ratio( aParams.m_BestWidthRatio );
    aOutput.set_max_track_width_ratio( aParams.m_WidthtoSizeFilterRatio );
}


void UnpackTeardropSettings( TEARDROP_PARAMETERS& aOutput, const types::PadTeardropSettings& aProto )
{
    aOutput.m_Enabled = ( aProto.mode() == types::PadTeardropMode::PTM_ENABLED );
    aOutput.m_CurvedEdges = aProto.curved_edges();
    aOutput.m_AllowUseTwoTracks = aProto.allow_multiple_track_segments();
    aOutput.m_TdOnPadsInZones = !aProto.prefer_zone_connection();
    aOutput.m_TdMaxLen = aProto.max_length().value_nm();
    aOutput.m_TdMaxWidth = aProto.max_width().value_nm();
    aOutput.m_BestLengthRatio = aProto.best_length_ratio();
    aOutput.m_BestWidthRatio = aProto.best_width_ratio();
    aOutput.m_WidthtoSizeFilterRatio = aProto.max_track_width_ratio();
}

}   // namespace kiapi::board
