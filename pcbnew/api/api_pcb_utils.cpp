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
#include <board.h>
#include <board_item_container.h>
#include <footprint.h>
#include <lset.h>
#include <pad.h>
#include <pcb_group.h>
#include <pcb_barcode.h>
#include <pcb_reference_image.h>
#include <pcb_shape.h>
#include <pcb_track.h>
#include <pcb_field.h>
#include <pcb_text.h>
#include <pcb_textbox.h>
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
    case PCB_SHAPE_T:   return std::make_unique<PCB_SHAPE>( aContainer );
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

}   // namespace kiapi::board
