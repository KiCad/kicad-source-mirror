/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2023 Jon Evans <jon@craftyjon.com>
 * Copyright (C) 2023 KiCad Developers, see AUTHORS.txt for contributors.
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

#include <magic_enum.hpp>
#include <api/api_utils.h>

namespace kiapi::common
{

std::optional<KICAD_T> TypeNameFromAny( const google::protobuf::Any& aMessage )
{
    static const std::map<std::string, KICAD_T> s_types = {
        { "type.googleapis.com/kiapi.board.types.Track", PCB_TRACE_T },
        { "type.googleapis.com/kiapi.board.types.Arc", PCB_ARC_T },
        { "type.googleapis.com/kiapi.board.types.Via", PCB_VIA_T },
        { "type.googleapis.com/kiapi.board.types.Text", PCB_TEXT_T },
        { "type.googleapis.com/kiapi.board.types.TextBox", PCB_TEXTBOX_T },
        { "type.googleapis.com/kiapi.board.types.GraphicShape", PCB_SHAPE_T },
        { "type.googleapis.com/kiapi.board.types.Pad", PCB_PAD_T },
        { "type.googleapis.com/kiapi.board.types.Zone", PCB_ZONE_T },
        { "type.googleapis.com/kiapi.board.types.Dimension", PCB_DIMENSION_T },
        { "type.googleapis.com/kiapi.board.types.ReferenceImage", PCB_REFERENCE_IMAGE_T },
        { "type.googleapis.com/kiapi.board.types.Group", PCB_GROUP_T },
        { "type.googleapis.com/kiapi.board.types.Field", PCB_FIELD_T },
        { "type.googleapis.com/kiapi.board.types.FootprintInstance", PCB_FOOTPRINT_T },
    };

    auto it = s_types.find( aMessage.type_url() );

    if( it != s_types.end() )
        return it->second;

    return std::nullopt;
}


LIB_ID LibIdFromProto( const types::LibraryIdentifier& aId )
{
    return LIB_ID( aId.library_nickname(), aId.entry_name() );
}


types::LibraryIdentifier LibIdToProto( const LIB_ID& aId )
{
    types::LibraryIdentifier msg;
    msg.set_library_nickname( aId.GetLibNickname() );
    msg.set_entry_name( aId.GetLibItemName() );
    return msg;
}


void PackVector2( kiapi::common::types::Vector2& aOutput, const VECTOR2I aInput )
{
    aOutput.set_x_nm( aInput.x );
    aOutput.set_y_nm( aInput.y );
}

VECTOR2I UnpackVector2( const types::Vector2& aInput )
{
    return VECTOR2I( aInput.x_nm(), aInput.y_nm() );
}

} // namespace kiapi::common
