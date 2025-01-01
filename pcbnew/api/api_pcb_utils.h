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

#ifndef KICAD_API_PCB_UTLIS_H
#define KICAD_API_PCB_UTLIS_H

#include <memory>
#include <core/typeinfo.h>
#include <import_export.h>
#include <layer_ids.h>
#include <lset.h>
#include <api/common/types/base_types.pb.h>
#include <api/board/board_types.pb.h>

class BOARD_ITEM;
class BOARD_ITEM_CONTAINER;

std::unique_ptr<BOARD_ITEM> CreateItemForType( KICAD_T aType, BOARD_ITEM_CONTAINER* aContainer );

namespace kiapi::board
{

void PackLayerSet( google::protobuf::RepeatedField<int>& aOutput, const LSET& aLayerSet );

LSET UnpackLayerSet( const google::protobuf::RepeatedField<int>& aInput );

}   // namespace kiapi::board

#endif //KICAD_API_PCB_UTLIS_H
