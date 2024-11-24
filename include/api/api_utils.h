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
#include <google/protobuf/any.pb.h>

#include <core/typeinfo.h>
#include <lib_id.h>
#include <api/common/types/base_types.pb.h>
#include <layer_ids.h>
#include <geometry/shape_line_chain.h>
#include <math/vector2d.h>
#include <math/vector3.h>

class SHAPE_LINE_CHAIN;

namespace kiapi::common
{

std::optional<KICAD_T> TypeNameFromAny( const google::protobuf::Any& aMessage );

LIB_ID LibIdFromProto( const types::LibraryIdentifier& aId );

types::LibraryIdentifier LibIdToProto( const LIB_ID& aId );

void PackVector2( types::Vector2& aOutput, const VECTOR2I& aInput );

VECTOR2I UnpackVector2( const types::Vector2& aInput );

void PackVector3D( types::Vector3D& aOutput, const VECTOR3D& aInput );

VECTOR3D UnpackVector3D( const types::Vector3D& aInput );

void PackBox2( types::Box2& aOutput, const BOX2I& aInput );

BOX2I UnpackBox2( const types::Box2& aInput );

void PackPolyLine( kiapi::common::types::PolyLine& aOutput, const SHAPE_LINE_CHAIN& aSlc );

SHAPE_LINE_CHAIN UnpackPolyLine( const kiapi::common::types::PolyLine& aInput );

void PackPolySet( kiapi::common::types::PolySet& aOutput, const SHAPE_POLY_SET& aInput );

SHAPE_POLY_SET UnpackPolySet( const kiapi::common::types::PolySet& aInput );

} // namespace kiapi::common

#endif //KICAD_API_UTILS_H
