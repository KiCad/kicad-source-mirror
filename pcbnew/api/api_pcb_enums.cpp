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
#include <pcb_track.h>
#include <zones.h>

using namespace kiapi::board;

template<>
types::PadType ToProtoEnum( PAD_ATTRIB aValue )
{
    switch( aValue )
    {
    case PAD_ATTRIB::PTH:   return types::PadType::PT_PTH;
    case PAD_ATTRIB::SMD:   return types::PadType::PT_SMD;
    case PAD_ATTRIB::CONN:  return types::PadType::PT_EDGE_CONNECTOR;
    case PAD_ATTRIB::NPTH:  return types::PadType::PT_NPTH;

    default:
        wxCHECK_MSG( false, types::PadType::PT_UNKNOWN,
                     "Unhandled case in ToProtoEnum<PAD_ATTRIB>");
    }
}


template<>
PAD_ATTRIB FromProtoEnum( types::PadType aValue )
{
    switch( aValue )
    {
    case types::PadType::PT_PTH:            return PAD_ATTRIB::PTH;
    case types::PadType::PT_SMD:            return PAD_ATTRIB::SMD;
    case types::PadType::PT_EDGE_CONNECTOR: return PAD_ATTRIB::CONN;
    case types::PadType::PT_NPTH:           return PAD_ATTRIB::NPTH;

    default:
        wxCHECK_MSG( false,  PAD_ATTRIB::PTH,
                     "Unhandled case in FromProtoEnum<types::PadType>" );
    }
}

template<>
types::DrillShape ToProtoEnum( PAD_DRILL_SHAPE aValue )
{
    switch( aValue )
    {
    case PAD_DRILL_SHAPE::CIRCLE:    return types::DrillShape::DS_CIRCLE;
    case PAD_DRILL_SHAPE::OBLONG:    return types::DrillShape::DS_OBLONG;
    case PAD_DRILL_SHAPE::UNDEFINED: return types::DrillShape::DS_UNDEFINED;
    default:
        wxCHECK_MSG( false, types::DrillShape::DS_UNKNOWN,
                     "Unhandled case in ToProtoEnum<PAD_DRILL_SHAPE>");
    }
}

template<>
PAD_DRILL_SHAPE FromProtoEnum( types::DrillShape aValue )
{
    switch( aValue )
    {
    case types::DrillShape::DS_CIRCLE:      return PAD_DRILL_SHAPE::CIRCLE;
    case types::DrillShape::DS_OBLONG:      return PAD_DRILL_SHAPE::OBLONG;
    case types::DrillShape::DS_UNDEFINED:   return PAD_DRILL_SHAPE::UNDEFINED;
    default:
        wxCHECK_MSG( false, PAD_DRILL_SHAPE::UNDEFINED,
                     "Unhandled case in FromProtoEnum<types::DrillShape>" );
    }
}

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
types::PadStackType ToProtoEnum( PADSTACK::MODE aValue )
{
    switch( aValue )
    {
    case PADSTACK::MODE::NORMAL:           return types::PadStackType::PST_NORMAL;
    case PADSTACK::MODE::FRONT_INNER_BACK: return types::PadStackType::PST_FRONT_INNER_BACK;
    case PADSTACK::MODE::CUSTOM:           return types::PadStackType::PST_CUSTOM;

    default:
        wxCHECK_MSG( false, types::PadStackType::PST_UNKNOWN,
                     "Unhandled case in ToProtoEnum<PADSTACK::MODE>");
    }
}


template<>
PADSTACK::MODE FromProtoEnum( types::PadStackType aValue )
{
    switch( aValue )
    {
    case types::PadStackType::PST_NORMAL:           return PADSTACK::MODE::NORMAL;
    case types::PadStackType::PST_FRONT_INNER_BACK: return PADSTACK::MODE::FRONT_INNER_BACK;
    case types::PadStackType::PST_CUSTOM:           return PADSTACK::MODE::CUSTOM;

    default:
        wxCHECK_MSG( false, PADSTACK::MODE::NORMAL,
                     "Unhandled case in FromProtoEnum<types::PadStackType>" );
    }
}


template<>
types::ViaType ToProtoEnum( VIATYPE aValue )
{
    switch( aValue )
    {
    case VIATYPE::THROUGH:      return types::ViaType::VT_THROUGH;
    case VIATYPE::BLIND_BURIED: return types::ViaType::VT_BLIND_BURIED;
    case VIATYPE::MICROVIA:     return types::ViaType::VT_MICRO;

    default:
        wxCHECK_MSG( false, types::ViaType::VT_UNKNOWN,
                     "Unhandled case in ToProtoEnum<VIATYPE>");
    }
}


template<>
VIATYPE FromProtoEnum( types::ViaType aValue )
{
    switch( aValue )
    {
    case types::ViaType::VT_THROUGH:      return VIATYPE::THROUGH;
    case types::ViaType::VT_BLIND_BURIED: return VIATYPE::BLIND_BURIED;
    case types::ViaType::VT_MICRO:        return VIATYPE::MICROVIA;

    default:
        wxCHECK_MSG( false, VIATYPE::THROUGH,
                     "Unhandled case in FromProtoEnum<types::ViaType>" );
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


template<>
types::UnconnectedLayerRemoval ToProtoEnum( PADSTACK::UNCONNECTED_LAYER_MODE aValue )
{
    switch( aValue )
    {
    case PADSTACK::UNCONNECTED_LAYER_MODE::KEEP_ALL:
        return types::UnconnectedLayerRemoval::ULR_KEEP;

    case PADSTACK::UNCONNECTED_LAYER_MODE::REMOVE_ALL:
        return types::UnconnectedLayerRemoval::ULR_REMOVE;

    case PADSTACK::UNCONNECTED_LAYER_MODE::REMOVE_EXCEPT_START_AND_END:
        return types::UnconnectedLayerRemoval::ULR_REMOVE_EXCEPT_START_AND_END;

    default:
        wxCHECK_MSG( false, types::UnconnectedLayerRemoval::ULR_UNKNOWN,
                     "Unhandled case in ToProtoEnum<PADSTACK::UNCONNECTED_LAYER_MODE>");
    }
}


template<>
PADSTACK::UNCONNECTED_LAYER_MODE FromProtoEnum( types::UnconnectedLayerRemoval aValue )
{
    switch( aValue )
    {
    case types::UnconnectedLayerRemoval::ULR_KEEP:
        return PADSTACK::UNCONNECTED_LAYER_MODE::KEEP_ALL;

    case types::UnconnectedLayerRemoval::ULR_REMOVE:
        return PADSTACK::UNCONNECTED_LAYER_MODE::REMOVE_ALL;

    case types::UnconnectedLayerRemoval::ULR_REMOVE_EXCEPT_START_AND_END:
        return PADSTACK::UNCONNECTED_LAYER_MODE::REMOVE_EXCEPT_START_AND_END;

    default:
        wxCHECK_MSG( false, PADSTACK::UNCONNECTED_LAYER_MODE::KEEP_ALL,
                     "Unhandled case in FromProtoEnum<types::UnconnectedLayerRemoval>");
    }
}
