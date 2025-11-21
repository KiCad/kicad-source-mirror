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
#include <gal/color4d.h>

class SHAPE_LINE_CHAIN;
class KIID_PATH;

/**
 * Flag to enable debug output related to the IPC API and its plugin system
 *
 * Use "KICAD_API" to enable.
 *
 * @ingroup trace_env_vars
 */
extern const KICOMMON_API wxChar* const traceApi;

namespace kiapi::common
{

KICOMMON_API std::optional<KICAD_T> TypeNameFromAny( const google::protobuf::Any& aMessage );

KICOMMON_API LIB_ID LibIdFromProto( const types::LibraryIdentifier& aId );

KICOMMON_API types::LibraryIdentifier LibIdToProto( const LIB_ID& aId );

KICOMMON_API void PackVector2( types::Vector2& aOutput, const VECTOR2I& aInput );

KICOMMON_API VECTOR2I UnpackVector2( const types::Vector2& aInput );

KICOMMON_API void PackVector3D( types::Vector3D& aOutput, const VECTOR3D& aInput );

KICOMMON_API VECTOR3D UnpackVector3D( const types::Vector3D& aInput );

KICOMMON_API void PackBox2( types::Box2& aOutput, const BOX2I& aInput );

KICOMMON_API BOX2I UnpackBox2( const types::Box2& aInput );

KICOMMON_API void PackPolyLine( types::PolyLine& aOutput, const SHAPE_LINE_CHAIN& aSlc );

KICOMMON_API SHAPE_LINE_CHAIN UnpackPolyLine( const types::PolyLine& aInput );

KICOMMON_API void PackPolySet( types::PolySet& aOutput, const SHAPE_POLY_SET& aInput );

KICOMMON_API SHAPE_POLY_SET UnpackPolySet( const types::PolySet& aInput );

KICOMMON_API void PackColor( types::Color& aOutput, const KIGFX::COLOR4D& aInput );

KICOMMON_API KIGFX::COLOR4D UnpackColor( const types::Color& aInput );

KICOMMON_API void PackSheetPath( types::SheetPath& aOutput, const KIID_PATH& aInput );

KICOMMON_API KIID_PATH UnpackSheetPath( const types::SheetPath& aInput );

} // namespace kiapi::common

#endif //KICAD_API_UTILS_H
