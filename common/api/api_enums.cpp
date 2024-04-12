/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
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

#include <api/api_enums.h>
#include <import_export.h>
#include <api/common/types/enums.pb.h>
#include <api/board/board_types.pb.h>
#include <api/schematic/schematic_types.pb.h>

#include <core/typeinfo.h>
#include <font/text_attributes.h>
#include <layer_ids.h>
#include <stroke_params.h>

using namespace kiapi;
using namespace kiapi::common;

template<>
KICAD_T FromProtoEnum( types::KiCadObjectType aValue )
{
    switch( aValue )
    {
    case types::KiCadObjectType::KOT_PCB_FOOTPRINT:         return PCB_FOOTPRINT_T;
    case types::KiCadObjectType::KOT_PCB_PAD:               return PCB_PAD_T;
    case types::KiCadObjectType::KOT_PCB_SHAPE:             return PCB_SHAPE_T;
    case types::KiCadObjectType::KOT_PCB_REFERENCE_IMAGE:   return PCB_REFERENCE_IMAGE_T;
    case types::KiCadObjectType::KOT_PCB_FIELD:             return PCB_FIELD_T;
    case types::KiCadObjectType::KOT_PCB_GENERATOR:         return PCB_GENERATOR_T;
    case types::KiCadObjectType::KOT_PCB_TEXT:              return PCB_TEXT_T;
    case types::KiCadObjectType::KOT_PCB_TEXTBOX:           return PCB_TEXTBOX_T;
    case types::KiCadObjectType::KOT_PCB_TABLE:             return PCB_TABLE_T;
    case types::KiCadObjectType::KOT_PCB_TABLECELL:         return PCB_TABLECELL_T;
    case types::KiCadObjectType::KOT_PCB_TRACE:             return PCB_TRACE_T;
    case types::KiCadObjectType::KOT_PCB_VIA:               return PCB_VIA_T;
    case types::KiCadObjectType::KOT_PCB_ARC:               return PCB_ARC_T;
    case types::KiCadObjectType::KOT_PCB_MARKER:            return PCB_MARKER_T;
    case types::KiCadObjectType::KOT_PCB_DIMENSION:         return PCB_DIMENSION_T;
    case types::KiCadObjectType::KOT_PCB_ZONE:              return PCB_ZONE_T;
    case types::KiCadObjectType::KOT_PCB_GROUP:             return PCB_GROUP_T;
    case types::KiCadObjectType::KOT_SCH_MARKER:            return SCH_MARKER_T;
    case types::KiCadObjectType::KOT_SCH_JUNCTION:          return SCH_JUNCTION_T;
    case types::KiCadObjectType::KOT_SCH_NO_CONNECT:        return SCH_NO_CONNECT_T;
    case types::KiCadObjectType::KOT_SCH_BUS_WIRE_ENTRY:    return SCH_BUS_WIRE_ENTRY_T;
    case types::KiCadObjectType::KOT_SCH_BUS_BUS_ENTRY:     return SCH_BUS_BUS_ENTRY_T;
    case types::KiCadObjectType::KOT_SCH_LINE:              return SCH_LINE_T;
    case types::KiCadObjectType::KOT_SCH_SHAPE:             return SCH_SHAPE_T;
    case types::KiCadObjectType::KOT_SCH_BITMAP:            return SCH_BITMAP_T;
    case types::KiCadObjectType::KOT_SCH_TEXTBOX:           return SCH_TEXTBOX_T;
    case types::KiCadObjectType::KOT_SCH_TEXT:              return SCH_TEXT_T;
    case types::KiCadObjectType::KOT_SCH_TABLE:             return SCH_TABLE_T;
    case types::KiCadObjectType::KOT_SCH_TABLECELL:         return SCH_TABLECELL_T;
    case types::KiCadObjectType::KOT_SCH_LABEL:             return SCH_LABEL_T;
    case types::KiCadObjectType::KOT_SCH_GLOBAL_LABEL:      return SCH_GLOBAL_LABEL_T;
    case types::KiCadObjectType::KOT_SCH_HIER_LABEL:        return SCH_HIER_LABEL_T;
    case types::KiCadObjectType::KOT_SCH_DIRECTIVE_LABEL:   return SCH_DIRECTIVE_LABEL_T;
    case types::KiCadObjectType::KOT_SCH_FIELD:             return SCH_FIELD_T;
    case types::KiCadObjectType::KOT_SCH_SYMBOL:            return SCH_SYMBOL_T;
    case types::KiCadObjectType::KOT_SCH_SHEET_PIN:         return SCH_SHEET_PIN_T;
    case types::KiCadObjectType::KOT_SCH_SHEET:             return SCH_SHEET_T;
    case types::KiCadObjectType::KOT_SCH_PIN:               return SCH_PIN_T;
    case types::KiCadObjectType::KOT_LIB_SYMBOL:            return LIB_SYMBOL_T;
    case types::KiCadObjectType::KOT_LIB_SHAPE:             return LIB_SHAPE_T;
    case types::KiCadObjectType::KOT_LIB_TEXT:              return LIB_TEXT_T;
    case types::KiCadObjectType::KOT_LIB_TEXTBOX:           return LIB_TEXTBOX_T;
    case types::KiCadObjectType::KOT_LIB_PIN:               return LIB_PIN_T;
    case types::KiCadObjectType::KOT_WSG_LINE:              return WSG_LINE_T;
    case types::KiCadObjectType::KOT_WSG_RECT:              return WSG_RECT_T;
    case types::KiCadObjectType::KOT_WSG_POLY:              return WSG_POLY_T;
    case types::KiCadObjectType::KOT_WSG_TEXT:              return WSG_TEXT_T;
    case types::KiCadObjectType::KOT_WSG_BITMAP:            return WSG_BITMAP_T;
    case types::KiCadObjectType::KOT_WSG_PAGE:              return WSG_PAGE_T;

    case types::KiCadObjectType::KOT_UNKNOWN:               return TYPE_NOT_INIT;
    default:
        wxCHECK_MSG( false, TYPE_NOT_INIT,
                     "Unhandled case in FromProtoEnum<types::KiCadObjectType>" );
    }
}


template<>
types::KiCadObjectType ToProtoEnum( KICAD_T aValue )
{
    switch( aValue )
    {
    case PCB_FOOTPRINT_T:        return types::KiCadObjectType::KOT_PCB_FOOTPRINT;
    case PCB_PAD_T:              return types::KiCadObjectType::KOT_PCB_PAD;
    case PCB_SHAPE_T:            return types::KiCadObjectType::KOT_PCB_SHAPE;
    case PCB_REFERENCE_IMAGE_T:  return types::KiCadObjectType::KOT_PCB_REFERENCE_IMAGE;
    case PCB_FIELD_T:            return types::KiCadObjectType::KOT_PCB_FIELD;
    case PCB_GENERATOR_T:        return types::KiCadObjectType::KOT_PCB_GENERATOR;
    case PCB_TEXT_T:             return types::KiCadObjectType::KOT_PCB_TEXT;
    case PCB_TEXTBOX_T:          return types::KiCadObjectType::KOT_PCB_TEXTBOX;
    case PCB_TABLE_T:            return types::KiCadObjectType::KOT_PCB_TABLE;
    case PCB_TABLECELL_T:        return types::KiCadObjectType::KOT_PCB_TABLECELL;
    case PCB_TRACE_T:            return types::KiCadObjectType::KOT_PCB_TRACE;
    case PCB_VIA_T:              return types::KiCadObjectType::KOT_PCB_VIA;
    case PCB_ARC_T:              return types::KiCadObjectType::KOT_PCB_ARC;
    case PCB_MARKER_T:           return types::KiCadObjectType::KOT_PCB_MARKER;
    case PCB_DIMENSION_T:        return types::KiCadObjectType::KOT_PCB_DIMENSION;
    case PCB_ZONE_T:             return types::KiCadObjectType::KOT_PCB_ZONE;
    case PCB_GROUP_T:            return types::KiCadObjectType::KOT_PCB_GROUP;
    case SCH_MARKER_T:           return types::KiCadObjectType::KOT_SCH_MARKER;
    case SCH_JUNCTION_T:         return types::KiCadObjectType::KOT_SCH_JUNCTION;
    case SCH_NO_CONNECT_T:       return types::KiCadObjectType::KOT_SCH_NO_CONNECT;
    case SCH_BUS_WIRE_ENTRY_T:   return types::KiCadObjectType::KOT_SCH_BUS_WIRE_ENTRY;
    case SCH_BUS_BUS_ENTRY_T:    return types::KiCadObjectType::KOT_SCH_BUS_BUS_ENTRY;
    case SCH_LINE_T:             return types::KiCadObjectType::KOT_SCH_LINE;
    case SCH_SHAPE_T:            return types::KiCadObjectType::KOT_SCH_SHAPE;
    case SCH_BITMAP_T:           return types::KiCadObjectType::KOT_SCH_BITMAP;
    case SCH_TEXTBOX_T:          return types::KiCadObjectType::KOT_SCH_TEXTBOX;
    case SCH_TEXT_T:             return types::KiCadObjectType::KOT_SCH_TEXT;
    case SCH_TABLE_T:            return types::KiCadObjectType::KOT_SCH_TABLE;
    case SCH_TABLECELL_T:        return types::KiCadObjectType::KOT_SCH_TABLECELL;
    case SCH_LABEL_T:            return types::KiCadObjectType::KOT_SCH_LABEL;
    case SCH_GLOBAL_LABEL_T:     return types::KiCadObjectType::KOT_SCH_GLOBAL_LABEL;
    case SCH_HIER_LABEL_T:       return types::KiCadObjectType::KOT_SCH_HIER_LABEL;
    case SCH_DIRECTIVE_LABEL_T:  return types::KiCadObjectType::KOT_SCH_DIRECTIVE_LABEL;
    case SCH_FIELD_T:            return types::KiCadObjectType::KOT_SCH_FIELD;
    case SCH_SYMBOL_T:           return types::KiCadObjectType::KOT_SCH_SYMBOL;
    case SCH_SHEET_PIN_T:        return types::KiCadObjectType::KOT_SCH_SHEET_PIN;
    case SCH_SHEET_T:            return types::KiCadObjectType::KOT_SCH_SHEET;
    case SCH_PIN_T:              return types::KiCadObjectType::KOT_SCH_PIN;
    case LIB_SYMBOL_T:           return types::KiCadObjectType::KOT_LIB_SYMBOL;
    case LIB_SHAPE_T:            return types::KiCadObjectType::KOT_LIB_SHAPE;
    case LIB_TEXT_T:             return types::KiCadObjectType::KOT_LIB_TEXT;
    case LIB_TEXTBOX_T:          return types::KiCadObjectType::KOT_LIB_TEXTBOX;
    case LIB_PIN_T:              return types::KiCadObjectType::KOT_LIB_PIN;
    case WSG_LINE_T:             return types::KiCadObjectType::KOT_WSG_LINE;
    case WSG_RECT_T:             return types::KiCadObjectType::KOT_WSG_RECT;
    case WSG_POLY_T:             return types::KiCadObjectType::KOT_WSG_POLY;
    case WSG_TEXT_T:             return types::KiCadObjectType::KOT_WSG_TEXT;
    case WSG_BITMAP_T:           return types::KiCadObjectType::KOT_WSG_BITMAP;
    case WSG_PAGE_T:             return types::KiCadObjectType::KOT_WSG_PAGE;
    default:
        wxCHECK_MSG( false, types::KiCadObjectType::KOT_UNKNOWN,
                     "Unhandled case in ToProtoEnum<KICAD_T>");
    }
}


template<>
PCB_LAYER_ID FromProtoEnum( board::types::BoardLayer aValue )
{
    switch( aValue )
    {
    case board::types::BoardLayer::BL_UNDEFINED:   return UNDEFINED_LAYER;
    case board::types::BoardLayer::BL_UNSELECTED:  return UNSELECTED_LAYER;
    case board::types::BoardLayer::BL_F_Cu:        return F_Cu;
    case board::types::BoardLayer::BL_In1_Cu:      return In1_Cu;
    case board::types::BoardLayer::BL_In2_Cu:      return In2_Cu;
    case board::types::BoardLayer::BL_In3_Cu:      return In3_Cu;
    case board::types::BoardLayer::BL_In4_Cu:      return In4_Cu;
    case board::types::BoardLayer::BL_In5_Cu:      return In5_Cu;
    case board::types::BoardLayer::BL_In6_Cu:      return In6_Cu;
    case board::types::BoardLayer::BL_In7_Cu:      return In7_Cu;
    case board::types::BoardLayer::BL_In8_Cu:      return In8_Cu;
    case board::types::BoardLayer::BL_In9_Cu:      return In9_Cu;
    case board::types::BoardLayer::BL_In10_Cu:     return In10_Cu;
    case board::types::BoardLayer::BL_In11_Cu:     return In11_Cu;
    case board::types::BoardLayer::BL_In12_Cu:     return In12_Cu;
    case board::types::BoardLayer::BL_In13_Cu:     return In13_Cu;
    case board::types::BoardLayer::BL_In14_Cu:     return In14_Cu;
    case board::types::BoardLayer::BL_In15_Cu:     return In15_Cu;
    case board::types::BoardLayer::BL_In16_Cu:     return In16_Cu;
    case board::types::BoardLayer::BL_In17_Cu:     return In17_Cu;
    case board::types::BoardLayer::BL_In18_Cu:     return In18_Cu;
    case board::types::BoardLayer::BL_In19_Cu:     return In19_Cu;
    case board::types::BoardLayer::BL_In20_Cu:     return In20_Cu;
    case board::types::BoardLayer::BL_In21_Cu:     return In21_Cu;
    case board::types::BoardLayer::BL_In22_Cu:     return In22_Cu;
    case board::types::BoardLayer::BL_In23_Cu:     return In23_Cu;
    case board::types::BoardLayer::BL_In24_Cu:     return In24_Cu;
    case board::types::BoardLayer::BL_In25_Cu:     return In25_Cu;
    case board::types::BoardLayer::BL_In26_Cu:     return In26_Cu;
    case board::types::BoardLayer::BL_In27_Cu:     return In27_Cu;
    case board::types::BoardLayer::BL_In28_Cu:     return In28_Cu;
    case board::types::BoardLayer::BL_In29_Cu:     return In29_Cu;
    case board::types::BoardLayer::BL_In30_Cu:     return In30_Cu;
    case board::types::BoardLayer::BL_B_Cu:        return B_Cu;
    case board::types::BoardLayer::BL_B_Adhes:     return B_Adhes;
    case board::types::BoardLayer::BL_F_Adhes:     return F_Adhes;
    case board::types::BoardLayer::BL_B_Paste:     return B_Paste;
    case board::types::BoardLayer::BL_F_Paste:     return F_Paste;
    case board::types::BoardLayer::BL_B_SilkS:     return B_SilkS;
    case board::types::BoardLayer::BL_F_SilkS:     return F_SilkS;
    case board::types::BoardLayer::BL_B_Mask:      return B_Mask;
    case board::types::BoardLayer::BL_F_Mask:      return F_Mask;
    case board::types::BoardLayer::BL_Dwgs_User:   return Dwgs_User;
    case board::types::BoardLayer::BL_Cmts_User:   return Cmts_User;
    case board::types::BoardLayer::BL_Eco1_User:   return Eco1_User;
    case board::types::BoardLayer::BL_Eco2_User:   return Eco2_User;
    case board::types::BoardLayer::BL_Edge_Cuts:   return Edge_Cuts;
    case board::types::BoardLayer::BL_Margin:      return Margin;
    case board::types::BoardLayer::BL_B_CrtYd:     return B_CrtYd;
    case board::types::BoardLayer::BL_F_CrtYd:     return F_CrtYd;
    case board::types::BoardLayer::BL_B_Fab:       return B_Fab;
    case board::types::BoardLayer::BL_F_Fab:       return F_Fab;
    case board::types::BoardLayer::BL_User_1:      return User_1;
    case board::types::BoardLayer::BL_User_2:      return User_2;
    case board::types::BoardLayer::BL_User_3:      return User_3;
    case board::types::BoardLayer::BL_User_4:      return User_4;
    case board::types::BoardLayer::BL_User_5:      return User_5;
    case board::types::BoardLayer::BL_User_6:      return User_6;
    case board::types::BoardLayer::BL_User_7:      return User_7;
    case board::types::BoardLayer::BL_User_8:      return User_8;
    case board::types::BoardLayer::BL_User_9:      return User_9;

    case board::types::BoardLayer::BL_UNKNOWN:     return UNDEFINED_LAYER;
    default:
        wxCHECK_MSG( false, UNDEFINED_LAYER,
                     "Unhandled case in FromProtoEnum<board::types::BoardLayer>" );
    }
}


template<>
board::types::BoardLayer ToProtoEnum( PCB_LAYER_ID aValue )
{
    switch( aValue )
    {
    case UNDEFINED_LAYER:   return board::types::BoardLayer::BL_UNDEFINED;
    case UNSELECTED_LAYER:  return board::types::BoardLayer::BL_UNSELECTED;
    case F_Cu:              return board::types::BoardLayer::BL_F_Cu;
    case In1_Cu:            return board::types::BoardLayer::BL_In1_Cu;
    case In2_Cu:            return board::types::BoardLayer::BL_In2_Cu;
    case In3_Cu:            return board::types::BoardLayer::BL_In3_Cu;
    case In4_Cu:            return board::types::BoardLayer::BL_In4_Cu;
    case In5_Cu:            return board::types::BoardLayer::BL_In5_Cu;
    case In6_Cu:            return board::types::BoardLayer::BL_In6_Cu;
    case In7_Cu:            return board::types::BoardLayer::BL_In7_Cu;
    case In8_Cu:            return board::types::BoardLayer::BL_In8_Cu;
    case In9_Cu:            return board::types::BoardLayer::BL_In9_Cu;
    case In10_Cu:           return board::types::BoardLayer::BL_In10_Cu;
    case In11_Cu:           return board::types::BoardLayer::BL_In11_Cu;
    case In12_Cu:           return board::types::BoardLayer::BL_In12_Cu;
    case In13_Cu:           return board::types::BoardLayer::BL_In13_Cu;
    case In14_Cu:           return board::types::BoardLayer::BL_In14_Cu;
    case In15_Cu:           return board::types::BoardLayer::BL_In15_Cu;
    case In16_Cu:           return board::types::BoardLayer::BL_In16_Cu;
    case In17_Cu:           return board::types::BoardLayer::BL_In17_Cu;
    case In18_Cu:           return board::types::BoardLayer::BL_In18_Cu;
    case In19_Cu:           return board::types::BoardLayer::BL_In19_Cu;
    case In20_Cu:           return board::types::BoardLayer::BL_In20_Cu;
    case In21_Cu:           return board::types::BoardLayer::BL_In21_Cu;
    case In22_Cu:           return board::types::BoardLayer::BL_In22_Cu;
    case In23_Cu:           return board::types::BoardLayer::BL_In23_Cu;
    case In24_Cu:           return board::types::BoardLayer::BL_In24_Cu;
    case In25_Cu:           return board::types::BoardLayer::BL_In25_Cu;
    case In26_Cu:           return board::types::BoardLayer::BL_In26_Cu;
    case In27_Cu:           return board::types::BoardLayer::BL_In27_Cu;
    case In28_Cu:           return board::types::BoardLayer::BL_In28_Cu;
    case In29_Cu:           return board::types::BoardLayer::BL_In29_Cu;
    case In30_Cu:           return board::types::BoardLayer::BL_In30_Cu;
    case B_Cu:              return board::types::BoardLayer::BL_B_Cu;
    case B_Adhes:           return board::types::BoardLayer::BL_B_Adhes;
    case F_Adhes:           return board::types::BoardLayer::BL_F_Adhes;
    case B_Paste:           return board::types::BoardLayer::BL_B_Paste;
    case F_Paste:           return board::types::BoardLayer::BL_F_Paste;
    case B_SilkS:           return board::types::BoardLayer::BL_B_SilkS;
    case F_SilkS:           return board::types::BoardLayer::BL_F_SilkS;
    case B_Mask:            return board::types::BoardLayer::BL_B_Mask;
    case F_Mask:            return board::types::BoardLayer::BL_F_Mask;
    case Dwgs_User:         return board::types::BoardLayer::BL_Dwgs_User;
    case Cmts_User:         return board::types::BoardLayer::BL_Cmts_User;
    case Eco1_User:         return board::types::BoardLayer::BL_Eco1_User;
    case Eco2_User:         return board::types::BoardLayer::BL_Eco2_User;
    case Edge_Cuts:         return board::types::BoardLayer::BL_Edge_Cuts;
    case Margin:            return board::types::BoardLayer::BL_Margin;
    case B_CrtYd:           return board::types::BoardLayer::BL_B_CrtYd;
    case F_CrtYd:           return board::types::BoardLayer::BL_F_CrtYd;
    case B_Fab:             return board::types::BoardLayer::BL_B_Fab;
    case F_Fab:             return board::types::BoardLayer::BL_F_Fab;
    case User_1:            return board::types::BoardLayer::BL_User_1;
    case User_2:            return board::types::BoardLayer::BL_User_2;
    case User_3:            return board::types::BoardLayer::BL_User_3;
    case User_4:            return board::types::BoardLayer::BL_User_4;
    case User_5:            return board::types::BoardLayer::BL_User_5;
    case User_6:            return board::types::BoardLayer::BL_User_6;
    case User_7:            return board::types::BoardLayer::BL_User_7;
    case User_8:            return board::types::BoardLayer::BL_User_8;
    case User_9:            return board::types::BoardLayer::BL_User_9;
    default:
        wxCHECK_MSG( false, board::types::BoardLayer::BL_UNKNOWN,
                     "Unhandled case in ToProtoEnum<PCB_LAYER_ID>");
    }
}


template<>
SCH_LAYER_ID FromProtoEnum( schematic::types::SchematicLayer aValue )
{
    switch( aValue )
    {

    default:
        wxCHECK_MSG( false, SCH_LAYER_ID_START,
                     "Unhandled case in FromProtoEnum<schematic::types::SchematicLayer>" );
    }
}


template<>
schematic::types::SchematicLayer ToProtoEnum( SCH_LAYER_ID aValue )
{
    switch( aValue )
    {

    default:
        wxCHECK_MSG( false, schematic::types::SchematicLayer::SL_UNKNOWN,
                     "Unhandled case in ToProtoEnum<SCH_LAYER_ID>");
    }
}


template<>
GR_TEXT_H_ALIGN_T FromProtoEnum( types::HorizontalAlignment aValue )
{
    switch( aValue )
    {
    case types::HorizontalAlignment::HA_LEFT:          return GR_TEXT_H_ALIGN_LEFT;
    case types::HorizontalAlignment::HA_CENTER:        return GR_TEXT_H_ALIGN_CENTER;
    case types::HorizontalAlignment::HA_RIGHT:         return GR_TEXT_H_ALIGN_RIGHT;
    case types::HorizontalAlignment::HA_INDETERMINATE: return GR_TEXT_H_ALIGN_INDETERMINATE;

    case types::HorizontalAlignment::HA_UNKNOWN:       return GR_TEXT_H_ALIGN_CENTER;
    default:
        wxCHECK_MSG( false, GR_TEXT_H_ALIGN_CENTER,
                     "Unhandled case in FromProtoEnum<types::HorizontalAlignment>" );
    }
}


template<>
types::HorizontalAlignment ToProtoEnum( GR_TEXT_H_ALIGN_T aValue )
{
    switch( aValue )
    {
    case GR_TEXT_H_ALIGN_LEFT:          return types::HorizontalAlignment::HA_LEFT;
    case GR_TEXT_H_ALIGN_CENTER:        return types::HorizontalAlignment::HA_CENTER;
    case GR_TEXT_H_ALIGN_RIGHT:         return types::HorizontalAlignment::HA_RIGHT;
    case GR_TEXT_H_ALIGN_INDETERMINATE: return types::HorizontalAlignment::HA_INDETERMINATE;
    default:
        wxCHECK_MSG( false, types::HorizontalAlignment::HA_UNKNOWN,
                     "Unhandled case in ToProtoEnum<GR_TEXT_H_ALIGN_T>");
    }
}


template<>
GR_TEXT_V_ALIGN_T FromProtoEnum( types::VerticalAlignment aValue )
{
    switch( aValue )
    {
    case types::VerticalAlignment::VA_TOP:           return GR_TEXT_V_ALIGN_TOP;
    case types::VerticalAlignment::VA_CENTER:        return GR_TEXT_V_ALIGN_CENTER;
    case types::VerticalAlignment::VA_BOTTOM:        return GR_TEXT_V_ALIGN_BOTTOM;
    case types::VerticalAlignment::VA_INDETERMINATE: return GR_TEXT_V_ALIGN_INDETERMINATE;

    case types::VerticalAlignment::VA_UNKNOWN:       return GR_TEXT_V_ALIGN_CENTER;
    default:
        wxCHECK_MSG( false, GR_TEXT_V_ALIGN_CENTER,
                     "Unhandled case in FromProtoEnum<types::VerticalAlignment>" );
    }
}


template<>
types::VerticalAlignment ToProtoEnum( GR_TEXT_V_ALIGN_T aValue )
{
    switch( aValue )
    {
    case GR_TEXT_V_ALIGN_TOP:           return types::VerticalAlignment::VA_TOP;
    case GR_TEXT_V_ALIGN_CENTER:        return types::VerticalAlignment::VA_CENTER;
    case GR_TEXT_V_ALIGN_BOTTOM:        return types::VerticalAlignment::VA_BOTTOM;
    case GR_TEXT_V_ALIGN_INDETERMINATE: return types::VerticalAlignment::VA_INDETERMINATE;
    default:
        wxCHECK_MSG( false, types::VerticalAlignment::VA_UNKNOWN,
                     "Unhandled case in ToProtoEnum<GR_TEXT_V_ALIGN_T>");
    }
}


template<>
LINE_STYLE FromProtoEnum( types::StrokeLineStyle aValue )
{
    switch( aValue )
    {
    case types::StrokeLineStyle::SLS_DEFAULT:    return LINE_STYLE::DEFAULT;
    case types::StrokeLineStyle::SLS_SOLID:      return LINE_STYLE::SOLID;
    case types::StrokeLineStyle::SLS_DASH:       return LINE_STYLE::DASH;
    case types::StrokeLineStyle::SLS_DOT:        return LINE_STYLE::DOT;
    case types::StrokeLineStyle::SLS_DASHDOT:    return LINE_STYLE::DASHDOT;
    case types::StrokeLineStyle::SLS_DASHDOTDOT: return LINE_STYLE::DASHDOTDOT;
    case types::StrokeLineStyle::SLS_UNKNOWN:
    default:
        wxCHECK_MSG( false, LINE_STYLE::DEFAULT,
                     "Unhandled case in FromProtoEnum<types::StrokeLineStyle>" );
    }
}


template<>
types::StrokeLineStyle ToProtoEnum( LINE_STYLE aValue )
{
    switch( aValue )
    {
    case LINE_STYLE::DEFAULT:    return types::StrokeLineStyle::SLS_DEFAULT;
    case LINE_STYLE::SOLID:      return types::StrokeLineStyle::SLS_SOLID;
    case LINE_STYLE::DASH:       return types::StrokeLineStyle::SLS_DASH;
    case LINE_STYLE::DOT:        return types::StrokeLineStyle::SLS_DOT;
    case LINE_STYLE::DASHDOT:    return types::StrokeLineStyle::SLS_DASHDOT;
    case LINE_STYLE::DASHDOTDOT: return types::StrokeLineStyle::SLS_DASHDOTDOT;
    default:
        wxCHECK_MSG( false, types::StrokeLineStyle::SLS_UNKNOWN,
                     "Unhandled case in ToProtoEnum<LINE_STYLE>");
    }
}
