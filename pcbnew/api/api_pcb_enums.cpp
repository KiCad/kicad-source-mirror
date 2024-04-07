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

#include <import_export.h>
#include <api/api_enums.h>
#include <api/board/board_types.pb.h>
#include <wx/wx.h>

#include <padstack.h>
#include <zones.h>

using namespace kiapi::board;

template<>
types::PadStackShape ToProtoEnum( PAD_SHAPE aValue )
{
    switch( aValue )
    {
    case PAD_SHAPE::CIRCLE:         return types::PadStackShape::PSS_CIRCLE;
    case PAD_SHAPE::RECTANGLE:      return types::PadStackShape::PSS_RECTANGLE;
    case PAD_SHAPE::OVAL:           return types::PadStackShape::PSS_OVAL;
    case PAD_SHAPE::TRAPEZOID:      return types::PadStackShape::PSS_TRAPEZOID;
    case PAD_SHAPE::ROUNDRECT:      return types::PadStackShape::PSS_ROUNDRECT;
    case PAD_SHAPE::CHAMFERED_RECT: return types::PadStackShape::PSS_CHAMFEREDRECT;
    case PAD_SHAPE::CUSTOM:         return types::PadStackShape::PSS_CUSTOM;

    default:
        wxCHECK_MSG( false, types::PadStackShape::PSS_UNKNOWN,
                     "Unhandled case in ToProtoEnum<PAD_SHAPE>");
    }
}


template<>
PAD_SHAPE FromProtoEnum( types::PadStackShape aValue )
{
    switch( aValue )
    {
    case types::PadStackShape::PSS_CIRCLE:         return PAD_SHAPE::CIRCLE;
    case types::PadStackShape::PSS_RECTANGLE:      return PAD_SHAPE::RECTANGLE;
    case types::PadStackShape::PSS_OVAL:           return PAD_SHAPE::OVAL;
    case types::PadStackShape::PSS_TRAPEZOID:      return PAD_SHAPE::TRAPEZOID;
    case types::PadStackShape::PSS_ROUNDRECT:      return PAD_SHAPE::ROUNDRECT;
    case types::PadStackShape::PSS_CHAMFEREDRECT:  return PAD_SHAPE::CHAMFERED_RECT;
    case types::PadStackShape::PSS_CUSTOM:         return PAD_SHAPE::CUSTOM;

    default:
        wxCHECK_MSG( false, PAD_SHAPE::CIRCLE,
                     "Unhandled case in FromProtoEnum<types::PadStackShape>" );
    }
}


template<>
types::ZoneConnectionStyle ToProtoEnum( ZONE_CONNECTION aValue )
{
    switch( aValue )
    {
    case ZONE_CONNECTION::INHERITED:    return types::ZoneConnectionStyle::ZCS_INHERITED;
    case ZONE_CONNECTION::NONE:         return types::ZoneConnectionStyle::ZCS_NONE;
    case ZONE_CONNECTION::THERMAL:      return types::ZoneConnectionStyle::ZCS_THERMAL;
    case ZONE_CONNECTION::FULL:         return types::ZoneConnectionStyle::ZCS_FULL;
    case ZONE_CONNECTION::THT_THERMAL:  return types::ZoneConnectionStyle::ZCS_PTH_THERMAL;

    default:
        wxCHECK_MSG( false, types::ZoneConnectionStyle::ZCS_UNKNOWN,
                     "Unhandled case in ToProtoEnum<ZONE_CONNECTION>");
    }
}


template<>
ZONE_CONNECTION FromProtoEnum( types::ZoneConnectionStyle aValue )
{
    switch( aValue )
    {
    case types::ZoneConnectionStyle::ZCS_UNKNOWN:      return ZONE_CONNECTION::INHERITED;
    case types::ZoneConnectionStyle::ZCS_INHERITED:    return ZONE_CONNECTION::INHERITED;
    case types::ZoneConnectionStyle::ZCS_NONE:         return ZONE_CONNECTION::NONE;
    case types::ZoneConnectionStyle::ZCS_THERMAL:      return ZONE_CONNECTION::THERMAL;
    case types::ZoneConnectionStyle::ZCS_FULL:         return ZONE_CONNECTION::FULL;
    case types::ZoneConnectionStyle::ZCS_PTH_THERMAL:  return ZONE_CONNECTION::THT_THERMAL;

    default:
        wxCHECK_MSG( false, ZONE_CONNECTION::INHERITED,
                     "Unhandled case in FromProtoEnum<types::ZoneConnectionStyle>" );
    }
}
