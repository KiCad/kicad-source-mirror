/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2024 Jon Evans <jon@craftyjon.com>
 * Copyright (C) 2024 KiCad Developers, see AUTHORS.txt for contributors.
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

#ifndef KICAD_API_UTILS_H
#define KICAD_API_UTILS_H

#include <optional>

#include <core/typeinfo.h>
#include <lib_id.h>
#include <api/common/types/base_types.pb.h>
#include <google/protobuf/any.pb.h>
#include <layer_ids.h>
#include <math/vector2d.h>

namespace kiapi::common
{

std::optional<KICAD_T> TypeNameFromAny( const google::protobuf::Any& aMessage );

LIB_ID LibIdFromProto( const types::LibraryIdentifier& aId );

types::LibraryIdentifier LibIdToProto( const LIB_ID& aId );

void PackVector2( kiapi::common::types::Vector2& aOutput, const VECTOR2I aInput );

VECTOR2I UnpackVector2( const types::Vector2& aInput );

} // namespace kiapi::common

#endif //KICAD_API_UTILS_H
