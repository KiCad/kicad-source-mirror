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
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#ifndef KICAD_API_PCB_UTLIS_H
#define KICAD_API_PCB_UTLIS_H

#include <map>
#include <memory>
#include <core/typeinfo.h>
#include <import_export.h>
#include <board_item.h>
#include <layer_ids.h>
#include <lset.h>
#include <api/common/types/base_types.pb.h>
#include <api/common/types/embedded_files.pb.h>
#include <api/board/board_types.pb.h>

class BOARD;
class BOARD_ITEM;
class BOARD_ITEM_CONTAINER;
class TEARDROP_PARAMETERS;

std::unique_ptr<BOARD_ITEM> CreateItemForType( KICAD_T aType, BOARD_ITEM_CONTAINER* aContainer );

namespace kiapi::board
{

class BoardStackup;

void PackLayerSet( google::protobuf::RepeatedField<int>& aOutput, const LSET& aLayerSet );

LSET UnpackLayerSet( const google::protobuf::RepeatedField<int>& aInput );

void PackBoardStackup( const BOARD& aBoard, BoardStackup& aOut );

void PackTeardropSettings( types::PadTeardropSettings& aOutput, const TEARDROP_PARAMETERS& aParams );

void UnpackTeardropSettings( TEARDROP_PARAMETERS& aOutput, const types::PadTeardropSettings& aProto );

void PackZoneLayerOverrides( google::protobuf::RepeatedPtrField<types::ZoneLayerOverrideEntry>* aOutput,
                             const std::map<PCB_LAYER_ID, ZONE_LAYER_OVERRIDE>& aInput );

void UnpackZoneLayerOverrides( std::map<PCB_LAYER_ID, ZONE_LAYER_OVERRIDE>& aOutput,
                               const google::protobuf::RepeatedPtrField<types::ZoneLayerOverrideEntry>& aInput );

void PackEmbeddedFiles( kiapi::common::types::EmbeddedFiles& aOutput, const EMBEDDED_FILES& aFiles );

bool UnpackEmbeddedFiles( EMBEDDED_FILES& aOutput, const kiapi::common::types::EmbeddedFiles& aProto );

}   // namespace kiapi::board

#endif //KICAD_API_PCB_UTLIS_H
