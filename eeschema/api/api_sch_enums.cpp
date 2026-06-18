/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
 * @author Jon Evans <jon@craftyjon.com>
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

#include <api/api_enums.h>
#include <wx/wx.h>

#include <core/typeinfo.h>
#include <layer_ids.h>
#include <lib_symbol.h>
#include <pin_type.h>
#include <sch_label.h>
#include <sch_sheet_pin.h>
#include <symbol.h>
#include <api/schematic/schematic_types.pb.h>

using namespace kiapi::schematic;


template<>
types::SchematicLabelShape ToProtoEnum( LABEL_FLAG_SHAPE aValue )
{
    switch( aValue )
    {
    case LABEL_FLAG_SHAPE::L_INPUT:        return types::SchematicLabelShape::SLSH_INPUT;
    case LABEL_FLAG_SHAPE::L_OUTPUT:       return types::SchematicLabelShape::SLSH_OUTPUT;
    case LABEL_FLAG_SHAPE::L_BIDI:         return types::SchematicLabelShape::SLSH_BIDI;
    case LABEL_FLAG_SHAPE::L_TRISTATE:     return types::SchematicLabelShape::SLSH_TRISTATE;
    case LABEL_FLAG_SHAPE::L_UNSPECIFIED:  return types::SchematicLabelShape::SLSH_PASSIVE;
    case LABEL_FLAG_SHAPE::F_DOT:          return types::SchematicLabelShape::SLSH_DOT;
    case LABEL_FLAG_SHAPE::F_ROUND:        return types::SchematicLabelShape::SLSH_CIRCLE;
    case LABEL_FLAG_SHAPE::F_DIAMOND:      return types::SchematicLabelShape::SLSH_DIAMOND;
    case LABEL_FLAG_SHAPE::F_RECTANGLE:    return types::SchematicLabelShape::SLSH_RECTANGLE;

    default:
        wxCHECK_MSG( false, types::SchematicLabelShape::SLSH_UNKNOWN,
                     "Unhandled case in ToProtoEnum<LABEL_FLAG_SHAPE>" );
    }
}


template<>
LABEL_FLAG_SHAPE FromProtoEnum( types::SchematicLabelShape aValue )
{
    switch( aValue )
    {
    case types::SchematicLabelShape::SLSH_UNKNOWN:
    case types::SchematicLabelShape::SLSH_PASSIVE:    return LABEL_FLAG_SHAPE::L_UNSPECIFIED;
    case types::SchematicLabelShape::SLSH_INPUT:      return LABEL_FLAG_SHAPE::L_INPUT;
    case types::SchematicLabelShape::SLSH_OUTPUT:     return LABEL_FLAG_SHAPE::L_OUTPUT;
    case types::SchematicLabelShape::SLSH_BIDI:       return LABEL_FLAG_SHAPE::L_BIDI;
    case types::SchematicLabelShape::SLSH_TRISTATE:   return LABEL_FLAG_SHAPE::L_TRISTATE;
    case types::SchematicLabelShape::SLSH_DOT:        return LABEL_FLAG_SHAPE::F_DOT;
    case types::SchematicLabelShape::SLSH_CIRCLE:     return LABEL_FLAG_SHAPE::F_ROUND;
    case types::SchematicLabelShape::SLSH_DIAMOND:    return LABEL_FLAG_SHAPE::F_DIAMOND;
    case types::SchematicLabelShape::SLSH_RECTANGLE:  return LABEL_FLAG_SHAPE::F_RECTANGLE;

    default:
        wxCHECK_MSG( false, LABEL_FLAG_SHAPE::L_UNSPECIFIED,
                     "Unhandled case in FromProtoEnum<types::SchematicLabelShape>" );
    }
}


template<>
types::SchematicLabelSpinStyle ToProtoEnum( SPIN_STYLE::SPIN aValue )
{
    switch( aValue )
    {
    case SPIN_STYLE::SPIN::LEFT:    return types::SchematicLabelSpinStyle::SLSS_LEFT;
    case SPIN_STYLE::SPIN::UP:      return types::SchematicLabelSpinStyle::SLSS_UP;
    case SPIN_STYLE::SPIN::RIGHT:   return types::SchematicLabelSpinStyle::SLSS_RIGHT;
    case SPIN_STYLE::SPIN::BOTTOM:  return types::SchematicLabelSpinStyle::SLSS_BOTTOM;

    default:
        wxCHECK_MSG( false, types::SchematicLabelSpinStyle::SLSS_UNKNOWN,
                     "Unhandled case in ToProtoEnum<SPIN_STYLE::SPIN>" );
    }
}


template<>
SPIN_STYLE::SPIN FromProtoEnum( types::SchematicLabelSpinStyle aValue )
{
    switch( aValue )
    {
    case types::SchematicLabelSpinStyle::SLSS_UNKNOWN:
    case types::SchematicLabelSpinStyle::SLSS_LEFT:    return SPIN_STYLE::SPIN::LEFT;
    case types::SchematicLabelSpinStyle::SLSS_UP:      return SPIN_STYLE::SPIN::UP;
    case types::SchematicLabelSpinStyle::SLSS_RIGHT:   return SPIN_STYLE::SPIN::RIGHT;
    case types::SchematicLabelSpinStyle::SLSS_BOTTOM:  return SPIN_STYLE::SPIN::BOTTOM;

    default:
        wxCHECK_MSG( false, SPIN_STYLE::SPIN::LEFT,
                     "Unhandled case in FromProtoEnum<types::SchematicLabelSpinStyle>" );
    }
}


template<>
types::SheetSide ToProtoEnum( SHEET_SIDE aValue )
{
    switch( aValue )
    {
    case SHEET_SIDE::LEFT:       return types::SheetSide::SHS_LEFT;
    case SHEET_SIDE::RIGHT:      return types::SheetSide::SHS_RIGHT;
    case SHEET_SIDE::TOP:        return types::SheetSide::SHS_TOP;
    case SHEET_SIDE::BOTTOM:     return types::SheetSide::SHS_BOTTOM;

    default:
        wxCHECK_MSG( false, types::SheetSide::SHS_UNKNOWN,
                     "Unhandled case in ToProtoEnum<SHEET_SIDE>" );
    }
}


template<>
SHEET_SIDE FromProtoEnum( types::SheetSide aValue )
{
    switch( aValue )
    {
    case types::SheetSide::SHS_UNKNOWN:  return SHEET_SIDE::UNDEFINED;
    case types::SheetSide::SHS_LEFT:     return SHEET_SIDE::LEFT;
    case types::SheetSide::SHS_RIGHT:    return SHEET_SIDE::RIGHT;
    case types::SheetSide::SHS_TOP:      return SHEET_SIDE::TOP;
    case types::SheetSide::SHS_BOTTOM:   return SHEET_SIDE::BOTTOM;

    default:
        wxCHECK_MSG( false, SHEET_SIDE::UNDEFINED,
                     "Unhandled case in FromProtoEnum<types::SheetSide>" );
    }
}


template<>
types::SchematicSymbolType ToProtoEnum( LIBRENTRYOPTIONS aValue )
{
    switch( aValue )
    {
    case LIBRENTRYOPTIONS::ENTRY_NORMAL:        return types::SchematicSymbolType::SST_NORMAL;
    case LIBRENTRYOPTIONS::ENTRY_GLOBAL_POWER:  return types::SchematicSymbolType::SST_GLOBAL_POWER;
    case LIBRENTRYOPTIONS::ENTRY_LOCAL_POWER:   return types::SchematicSymbolType::SST_LOCAL_POWER;

    default:
        wxCHECK_MSG( false, types::SchematicSymbolType::SST_UNKNOWN,
                     "Unhandled case in ToProtoEnum<SHEET_SIDE>" );
    }
}


template<>
LIBRENTRYOPTIONS FromProtoEnum( types::SchematicSymbolType aValue )
{
    switch( aValue )
    {
    case types::SchematicSymbolType::SST_NORMAL:       return LIBRENTRYOPTIONS::ENTRY_NORMAL;
    case types::SchematicSymbolType::SST_GLOBAL_POWER: return LIBRENTRYOPTIONS::ENTRY_GLOBAL_POWER;
    case types::SchematicSymbolType::SST_LOCAL_POWER:  return LIBRENTRYOPTIONS::ENTRY_LOCAL_POWER;

    default:
        wxCHECK_MSG( false, LIBRENTRYOPTIONS::ENTRY_NORMAL,
                     "Unhandled case in FromProtoEnum<types::SchematicSymbolType>" );
    }
}


template<>
types::SchematicSymbolOrientation ToProtoEnum( SYMBOL_ORIENTATION_PROP aValue )
{
    switch( aValue )
    {
    case SYMBOL_ORIENTATION_PROP::SYMBOL_ANGLE_0:   return types::SchematicSymbolOrientation::SSO_0;
    case SYMBOL_ORIENTATION_PROP::SYMBOL_ANGLE_90:  return types::SchematicSymbolOrientation::SSO_90;
    case SYMBOL_ORIENTATION_PROP::SYMBOL_ANGLE_180: return types::SchematicSymbolOrientation::SSO_180;
    case SYMBOL_ORIENTATION_PROP::SYMBOL_ANGLE_270: return types::SchematicSymbolOrientation::SSO_270;

    default:
        wxCHECK_MSG( false, types::SchematicSymbolOrientation::SSO_UNKNOWN,
                     "Unhandled case in ToProtoEnum<SYMBOL_ORIENTATION_PROP>" );
    }
}


template<>
SYMBOL_ORIENTATION_PROP FromProtoEnum( types::SchematicSymbolOrientation aValue )
{
    switch( aValue )
    {
    case types::SchematicSymbolOrientation::SSO_0:   return SYMBOL_ORIENTATION_PROP::SYMBOL_ANGLE_0;
    case types::SchematicSymbolOrientation::SSO_90:  return SYMBOL_ORIENTATION_PROP::SYMBOL_ANGLE_90;
    case types::SchematicSymbolOrientation::SSO_180: return SYMBOL_ORIENTATION_PROP::SYMBOL_ANGLE_180;
    case types::SchematicSymbolOrientation::SSO_270: return SYMBOL_ORIENTATION_PROP::SYMBOL_ANGLE_270;

    default:
        wxCHECK_MSG( false, SYMBOL_ORIENTATION_PROP::SYMBOL_ANGLE_0,
                     "Unhandled case in FromProtoEnum<types::SchematicSymbolOrientation>" );
    }
}


template<>
types::SchematicPinOrientation ToProtoEnum( PIN_ORIENTATION aValue )
{
    switch( aValue )
    {
    case PIN_ORIENTATION::PIN_RIGHT:    return types::SchematicPinOrientation::SPO_RIGHT;
    case PIN_ORIENTATION::PIN_LEFT:     return types::SchematicPinOrientation::SPO_LEFT;
    case PIN_ORIENTATION::PIN_UP:       return types::SchematicPinOrientation::SPO_UP;
    case PIN_ORIENTATION::PIN_DOWN:     return types::SchematicPinOrientation::SPO_DOWN;

    default:
        wxCHECK_MSG( false, types::SchematicPinOrientation::SPO_UNKNOWN,
                     "Unhandled case in ToProtoEnum<PIN_ORIENTATION>" );
    }
}


template<>
PIN_ORIENTATION FromProtoEnum( types::SchematicPinOrientation aValue )
{
    switch( aValue )
    {
    case types::SchematicPinOrientation::SPO_RIGHT:     return PIN_ORIENTATION::PIN_RIGHT;
    case types::SchematicPinOrientation::SPO_LEFT:      return PIN_ORIENTATION::PIN_LEFT;
    case types::SchematicPinOrientation::SPO_UP:        return PIN_ORIENTATION::PIN_UP;
    case types::SchematicPinOrientation::SPO_DOWN:      return PIN_ORIENTATION::PIN_DOWN;

    default:
        wxCHECK_MSG( false, PIN_ORIENTATION::PIN_LEFT,
                     "Unhandled case in FromProtoEnum<types::SheetSide>" );
    }
}


template<>
types::SchematicPinShape ToProtoEnum( GRAPHIC_PINSHAPE aValue )
{
    switch( aValue )
    {
    case GRAPHIC_PINSHAPE::LINE:                return types::SchematicPinShape::SPS_LINE;
    case GRAPHIC_PINSHAPE::INVERTED:            return types::SchematicPinShape::SPS_INVERTED;
    case GRAPHIC_PINSHAPE::CLOCK:               return types::SchematicPinShape::SPS_CLOCK;
    case GRAPHIC_PINSHAPE::INVERTED_CLOCK:      return types::SchematicPinShape::SPS_INVERTED_CLOCK;
    case GRAPHIC_PINSHAPE::INPUT_LOW:           return types::SchematicPinShape::SPS_INPUT_LOW;
    case GRAPHIC_PINSHAPE::CLOCK_LOW:           return types::SchematicPinShape::SPS_CLOCK_LOW;
    case GRAPHIC_PINSHAPE::OUTPUT_LOW:          return types::SchematicPinShape::SPS_OUTPUT_LOW;
    case GRAPHIC_PINSHAPE::FALLING_EDGE_CLOCK:  return types::SchematicPinShape::SPS_FALLING_EDGE_CLOCK;
    case GRAPHIC_PINSHAPE::NONLOGIC:            return types::SchematicPinShape::SPS_NONLOGIC;

    default:
        wxCHECK_MSG( false, types::SchematicPinShape::SPS_UNKNOWN,
                     "Unhandled case in ToProtoEnum<GRAPHIC_PINSHAPE>" );
    }
}


template<>
GRAPHIC_PINSHAPE FromProtoEnum( types::SchematicPinShape aValue )
{
    switch( aValue )
    {
    case types::SchematicPinShape::SPS_LINE:                return GRAPHIC_PINSHAPE::LINE;
    case types::SchematicPinShape::SPS_INVERTED:            return GRAPHIC_PINSHAPE::INVERTED;
    case types::SchematicPinShape::SPS_CLOCK:               return GRAPHIC_PINSHAPE::CLOCK;
    case types::SchematicPinShape::SPS_INVERTED_CLOCK:      return GRAPHIC_PINSHAPE::INVERTED_CLOCK;
    case types::SchematicPinShape::SPS_INPUT_LOW:           return GRAPHIC_PINSHAPE::INPUT_LOW;
    case types::SchematicPinShape::SPS_CLOCK_LOW:           return GRAPHIC_PINSHAPE::CLOCK_LOW;
    case types::SchematicPinShape::SPS_OUTPUT_LOW:          return GRAPHIC_PINSHAPE::OUTPUT_LOW;
    case types::SchematicPinShape::SPS_FALLING_EDGE_CLOCK:  return GRAPHIC_PINSHAPE::FALLING_EDGE_CLOCK;
    case types::SchematicPinShape::SPS_NONLOGIC:            return GRAPHIC_PINSHAPE::NONLOGIC;

    default:
        wxCHECK_MSG( false, GRAPHIC_PINSHAPE::LINE,
                     "Unhandled case in FromProtoEnum<types::SchematicPinShape>" );
    }
}


template<>
types::PinMapOverrideMode ToProtoEnum( PIN_MAP_OVERRIDE_MODE aValue )
{
    switch( aValue )
    {
    case PIN_MAP_OVERRIDE_MODE::USE_LIBRARY_DEFAULT: return types::PinMapOverrideMode::PMOM_USE_LIBRARY_DEFAULT;
    case PIN_MAP_OVERRIDE_MODE::USE_NAMED_MAP:       return types::PinMapOverrideMode::PMOM_USE_NAMED_MAP;
    case PIN_MAP_OVERRIDE_MODE::FORCE_IDENTITY:      return types::PinMapOverrideMode::PMOM_FORCE_IDENTITY;
    case PIN_MAP_OVERRIDE_MODE::DELEGATE_TO_UNIT_1:  return types::PinMapOverrideMode::PMOM_DELEGATE_TO_UNIT_1;

    default:
        wxCHECK_MSG( false, types::PinMapOverrideMode::PMOM_USE_LIBRARY_DEFAULT,
                     "Unhandled case in ToProtoEnum<PIN_MAP_OVERRIDE_MODE>" );
    }
}


template<>
PIN_MAP_OVERRIDE_MODE FromProtoEnum( types::PinMapOverrideMode aValue )
{
    switch( aValue )
    {
    case types::PinMapOverrideMode::PMOM_UNKNOWN:
    case types::PinMapOverrideMode::PMOM_USE_LIBRARY_DEFAULT: return PIN_MAP_OVERRIDE_MODE::USE_LIBRARY_DEFAULT;
    case types::PinMapOverrideMode::PMOM_USE_NAMED_MAP:       return PIN_MAP_OVERRIDE_MODE::USE_NAMED_MAP;
    case types::PinMapOverrideMode::PMOM_FORCE_IDENTITY:      return PIN_MAP_OVERRIDE_MODE::FORCE_IDENTITY;
    case types::PinMapOverrideMode::PMOM_DELEGATE_TO_UNIT_1:  return PIN_MAP_OVERRIDE_MODE::DELEGATE_TO_UNIT_1;

    default:
        wxCHECK_MSG( false, PIN_MAP_OVERRIDE_MODE::USE_LIBRARY_DEFAULT,
                     "Unhandled case in FromProtoEnum<types::PinMapOverrideMode>" );
    }
}
