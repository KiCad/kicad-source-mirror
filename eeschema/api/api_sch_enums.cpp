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
#include <api/schematic/schematic_rules.pb.h>
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


template <>
types::PinMapOverrideMode ToProtoEnum( PIN_MAP_OVERRIDE_MODE aValue )
{
    switch( aValue )
    {
    case PIN_MAP_OVERRIDE_MODE::USE_LIBRARY_DEFAULT: return types::PinMapOverrideMode::PMOM_USE_LIBRARY_DEFAULT;
    case PIN_MAP_OVERRIDE_MODE::USE_NAMED_MAP: return types::PinMapOverrideMode::PMOM_USE_NAMED_MAP;
    case PIN_MAP_OVERRIDE_MODE::FORCE_IDENTITY: return types::PinMapOverrideMode::PMOM_FORCE_IDENTITY;
    case PIN_MAP_OVERRIDE_MODE::DELEGATE_TO_UNIT_1: return types::PinMapOverrideMode::PMOM_DELEGATE_TO_UNIT_1;

    default:
        wxCHECK_MSG( false, types::PinMapOverrideMode::PMOM_USE_LIBRARY_DEFAULT,
                     "Unhandled case in ToProtoEnum<PIN_MAP_OVERRIDE_MODE>" );
    }
}


template <>
PIN_MAP_OVERRIDE_MODE FromProtoEnum( types::PinMapOverrideMode aValue )
{
    switch( aValue )
    {
    case types::PinMapOverrideMode::PMOM_UNKNOWN:
    case types::PinMapOverrideMode::PMOM_USE_LIBRARY_DEFAULT: return PIN_MAP_OVERRIDE_MODE::USE_LIBRARY_DEFAULT;
    case types::PinMapOverrideMode::PMOM_USE_NAMED_MAP: return PIN_MAP_OVERRIDE_MODE::USE_NAMED_MAP;
    case types::PinMapOverrideMode::PMOM_FORCE_IDENTITY: return PIN_MAP_OVERRIDE_MODE::FORCE_IDENTITY;
    case types::PinMapOverrideMode::PMOM_DELEGATE_TO_UNIT_1: return PIN_MAP_OVERRIDE_MODE::DELEGATE_TO_UNIT_1;

    default:
        wxCHECK_MSG( false, PIN_MAP_OVERRIDE_MODE::USE_LIBRARY_DEFAULT,
                     "Unhandled case in FromProtoEnum<types::PinMapOverrideMode>" );
    }
}


template<>
ErcErrorType ToProtoEnum( ERCE_T aValue )
{
    switch( aValue )
    {
    case ERCE_DUPLICATE_SHEET_NAME:         return ErcErrorType::ERCET_DUPLICATE_SHEET_NAME;
    case ERCE_ENDPOINT_OFF_GRID:            return ErcErrorType::ERCET_ENDPOINT_OFF_GRID;
    case ERCE_PIN_NOT_CONNECTED:            return ErcErrorType::ERCET_PIN_NOT_CONNECTED;
    case ERCE_PIN_NOT_DRIVEN:               return ErcErrorType::ERCET_PIN_NOT_DRIVEN;
    case ERCE_POWERPIN_NOT_DRIVEN:          return ErcErrorType::ERCET_POWERPIN_NOT_DRIVEN;
    case ERCE_HIERACHICAL_LABEL:            return ErcErrorType::ERCET_HIERARCHICAL_LABEL;
    case ERCE_NOCONNECT_CONNECTED:          return ErcErrorType::ERCET_NOCONNECT_CONNECTED;
    case ERCE_NOCONNECT_NOT_CONNECTED:      return ErcErrorType::ERCET_NOCONNECT_NOT_CONNECTED;
    case ERCE_LABEL_NOT_CONNECTED:          return ErcErrorType::ERCET_LABEL_NOT_CONNECTED;
    case ERCE_SIMILAR_LABELS:               return ErcErrorType::ERCET_SIMILAR_LABELS;
    case ERCE_SIMILAR_POWER:                return ErcErrorType::ERCET_SIMILAR_POWER;
    case ERCE_SIMILAR_LABEL_AND_POWER:      return ErcErrorType::ERCET_SIMILAR_LABEL_AND_POWER;
    case ERCE_SINGLE_GLOBAL_LABEL:          return ErcErrorType::ERCET_SINGLE_GLOBAL_LABEL;
    case ERCE_SAME_LOCAL_GLOBAL_LABEL:      return ErcErrorType::ERCET_SAME_LOCAL_GLOBAL_LABEL;
    case ERCE_SAME_LOCAL_GLOBAL_POWER:      return ErcErrorType::ERCET_SAME_LOCAL_GLOBAL_POWER;
    case ERCE_DIFFERENT_UNIT_FP:            return ErcErrorType::ERCET_DIFFERENT_UNIT_FP;
    case ERCE_MISSING_POWER_INPUT_PIN:      return ErcErrorType::ERCET_MISSING_POWER_INPUT_PIN;
    case ERCE_MISSING_INPUT_PIN:            return ErcErrorType::ERCET_MISSING_INPUT_PIN;
    case ERCE_MISSING_BIDI_PIN:             return ErcErrorType::ERCET_MISSING_BIDI_PIN;
    case ERCE_MISSING_UNIT:                 return ErcErrorType::ERCET_MISSING_UNIT;
    case ERCE_DIFFERENT_UNIT_NET:           return ErcErrorType::ERCET_DIFFERENT_UNIT_NET;
    case ERCE_BUS_ALIAS_CONFLICT:           return ErcErrorType::ERCET_BUS_ALIAS_CONFLICT;
    case ERCE_DRIVER_CONFLICT:              return ErcErrorType::ERCET_DRIVER_CONFLICT;
    case ERCE_BUS_ENTRY_CONFLICT:           return ErcErrorType::ERCET_BUS_ENTRY_CONFLICT;
    case ERCE_BUS_TO_BUS_CONFLICT:          return ErcErrorType::ERCET_BUS_TO_BUS_CONFLICT;
    case ERCE_BUS_TO_NET_CONFLICT:          return ErcErrorType::ERCET_BUS_TO_NET_CONFLICT;
    case ERCE_GROUND_PIN_NOT_GROUND:        return ErcErrorType::ERCET_GROUND_PIN_NOT_GROUND;
    case ERCE_LABEL_SINGLE_PIN:             return ErcErrorType::ERCET_LABEL_SINGLE_PIN;
    case ERCE_UNRESOLVED_VARIABLE:          return ErcErrorType::ERCET_UNRESOLVED_VARIABLE;
    case ERCE_UNDEFINED_NETCLASS:           return ErcErrorType::ERCET_UNDEFINED_NETCLASS;
    case ERCE_SIMULATION_MODEL:             return ErcErrorType::ERCET_SIMULATION_MODEL;
    case ERCE_WIRE_DANGLING:                return ErcErrorType::ERCET_WIRE_DANGLING;
    case ERCE_LIB_SYMBOL_ISSUES:            return ErcErrorType::ERCET_LIB_SYMBOL_ISSUES;
    case ERCE_LIB_SYMBOL_MISMATCH:          return ErcErrorType::ERCET_LIB_SYMBOL_MISMATCH;
    case ERCE_FOOTPRINT_LINK_ISSUES:        return ErcErrorType::ERCET_FOOTPRINT_LINK_ISSUES;
    case ERCE_FOOTPRINT_FILTERS:            return ErcErrorType::ERCET_FOOTPRINT_FILTERS;
    case ERCE_UNANNOTATED:                  return ErcErrorType::ERCET_UNANNOTATED;
    case ERCE_EXTRA_UNITS:                  return ErcErrorType::ERCET_EXTRA_UNITS;
    case ERCE_DIFFERENT_UNIT_VALUE:         return ErcErrorType::ERCET_DIFFERENT_UNIT_VALUE;
    case ERCE_DUPLICATE_REFERENCE:          return ErcErrorType::ERCET_DUPLICATE_REFERENCE;
    case ERCE_BUS_ENTRY_NEEDED:             return ErcErrorType::ERCET_BUS_ENTRY_NEEDED;
    case ERCE_FOUR_WAY_JUNCTION:            return ErcErrorType::ERCET_FOUR_WAY_JUNCTION;
    case ERCE_LABEL_MULTIPLE_WIRES:         return ErcErrorType::ERCET_LABEL_MULTIPLE_WIRES;
    case ERCE_UNCONNECTED_WIRE_ENDPOINT:    return ErcErrorType::ERCET_UNCONNECTED_WIRE_ENDPOINT;
    case ERCE_STACKED_PIN_SYNTAX:           return ErcErrorType::ERCET_STACKED_PIN_SYNTAX;
    case ERCE_PIN_MAP_BAD_PAD:              return ErcErrorType::ERCET_PIN_MAP_BAD_PAD;
    case ERCE_PIN_MAP_UNMAPPED_PIN:         return ErcErrorType::ERCET_PIN_MAP_UNMAPPED_PIN;
    case ERCE_PIN_MAP_DUPLICATE_PAD:        return ErcErrorType::ERCET_PIN_MAP_DUPLICATE_PAD;
    case ERCE_PIN_MAP_STALE_PIN:            return ErcErrorType::ERCET_PIN_MAP_STALE_PIN;
    case ERCE_EMPTY_LABEL_NAME:             return ErcErrorType::ERCET_EMPTY_LABEL_NAME;
    case ERCE_VARIANT_SYMBOL_INVALID:       return ErcErrorType::ERCET_VARIANT_SYMBOL_INVALID;
    case ERCE_VARIANT_SYMBOL_INCOMPATIBLE:  return ErcErrorType::ERCET_VARIANT_SYMBOL_INCOMPATIBLE;
    case ERCE_DUPLICATE_PIN_ERROR:          return ErcErrorType::ERCET_DUPLICATE_PIN_ERROR;
    case ERCE_PIN_TO_PIN_WARNING:           return ErcErrorType::ERCET_PIN_TO_PIN_WARNING;
    case ERCE_PIN_TO_PIN_ERROR:             return ErcErrorType::ERCET_PIN_TO_PIN_ERROR;
    case ERCE_ANNOTATION_ACTION:            return ErcErrorType::ERCET_ANNOTATION_ACTION;
    case ERCE_GENERIC_WARNING:              return ErcErrorType::ERCET_GENERIC_WARNING;
    case ERCE_GENERIC_ERROR:                return ErcErrorType::ERCET_GENERIC_ERROR;
    case ERCE_FIELD_NAME_WHITESPACE:        return ErcErrorType::ERCET_FIELD_NAME_WHITESPACE;

    case ERCE_UNSPECIFIED:
    default:
        wxCHECK_MSG( false, ErcErrorType::ERCET_UNKNOWN,
                     "Unhandled case in ToProtoEnum<ERCE_T>" );
    }
}


template<>
ERCE_T FromProtoEnum( ErcErrorType aValue )
{
    switch( aValue )
    {
    case ErcErrorType::ERCET_DUPLICATE_SHEET_NAME:          return ERCE_DUPLICATE_SHEET_NAME;
    case ErcErrorType::ERCET_ENDPOINT_OFF_GRID:             return ERCE_ENDPOINT_OFF_GRID;
    case ErcErrorType::ERCET_PIN_NOT_CONNECTED:             return ERCE_PIN_NOT_CONNECTED;
    case ErcErrorType::ERCET_PIN_NOT_DRIVEN:                return ERCE_PIN_NOT_DRIVEN;
    case ErcErrorType::ERCET_POWERPIN_NOT_DRIVEN:           return ERCE_POWERPIN_NOT_DRIVEN;
    case ErcErrorType::ERCET_HIERARCHICAL_LABEL:            return ERCE_HIERACHICAL_LABEL;
    case ErcErrorType::ERCET_NOCONNECT_CONNECTED:           return ERCE_NOCONNECT_CONNECTED;
    case ErcErrorType::ERCET_NOCONNECT_NOT_CONNECTED:       return ERCE_NOCONNECT_NOT_CONNECTED;
    case ErcErrorType::ERCET_LABEL_NOT_CONNECTED:           return ERCE_LABEL_NOT_CONNECTED;
    case ErcErrorType::ERCET_SIMILAR_LABELS:                return ERCE_SIMILAR_LABELS;
    case ErcErrorType::ERCET_SIMILAR_POWER:                 return ERCE_SIMILAR_POWER;
    case ErcErrorType::ERCET_SIMILAR_LABEL_AND_POWER:       return ERCE_SIMILAR_LABEL_AND_POWER;
    case ErcErrorType::ERCET_SINGLE_GLOBAL_LABEL:           return ERCE_SINGLE_GLOBAL_LABEL;
    case ErcErrorType::ERCET_SAME_LOCAL_GLOBAL_LABEL:       return ERCE_SAME_LOCAL_GLOBAL_LABEL;
    case ErcErrorType::ERCET_SAME_LOCAL_GLOBAL_POWER:       return ERCE_SAME_LOCAL_GLOBAL_POWER;
    case ErcErrorType::ERCET_DIFFERENT_UNIT_FP:             return ERCE_DIFFERENT_UNIT_FP;
    case ErcErrorType::ERCET_MISSING_POWER_INPUT_PIN:       return ERCE_MISSING_POWER_INPUT_PIN;
    case ErcErrorType::ERCET_MISSING_INPUT_PIN:             return ERCE_MISSING_INPUT_PIN;
    case ErcErrorType::ERCET_MISSING_BIDI_PIN:              return ERCE_MISSING_BIDI_PIN;
    case ErcErrorType::ERCET_MISSING_UNIT:                  return ERCE_MISSING_UNIT;
    case ErcErrorType::ERCET_DIFFERENT_UNIT_NET:            return ERCE_DIFFERENT_UNIT_NET;
    case ErcErrorType::ERCET_BUS_ALIAS_CONFLICT:            return ERCE_BUS_ALIAS_CONFLICT;
    case ErcErrorType::ERCET_DRIVER_CONFLICT:               return ERCE_DRIVER_CONFLICT;
    case ErcErrorType::ERCET_BUS_ENTRY_CONFLICT:            return ERCE_BUS_ENTRY_CONFLICT;
    case ErcErrorType::ERCET_BUS_TO_BUS_CONFLICT:           return ERCE_BUS_TO_BUS_CONFLICT;
    case ErcErrorType::ERCET_BUS_TO_NET_CONFLICT:           return ERCE_BUS_TO_NET_CONFLICT;
    case ErcErrorType::ERCET_GROUND_PIN_NOT_GROUND:         return ERCE_GROUND_PIN_NOT_GROUND;
    case ErcErrorType::ERCET_LABEL_SINGLE_PIN:              return ERCE_LABEL_SINGLE_PIN;
    case ErcErrorType::ERCET_UNRESOLVED_VARIABLE:           return ERCE_UNRESOLVED_VARIABLE;
    case ErcErrorType::ERCET_UNDEFINED_NETCLASS:            return ERCE_UNDEFINED_NETCLASS;
    case ErcErrorType::ERCET_SIMULATION_MODEL:              return ERCE_SIMULATION_MODEL;
    case ErcErrorType::ERCET_WIRE_DANGLING:                 return ERCE_WIRE_DANGLING;
    case ErcErrorType::ERCET_LIB_SYMBOL_ISSUES:             return ERCE_LIB_SYMBOL_ISSUES;
    case ErcErrorType::ERCET_LIB_SYMBOL_MISMATCH:           return ERCE_LIB_SYMBOL_MISMATCH;
    case ErcErrorType::ERCET_FOOTPRINT_LINK_ISSUES:         return ERCE_FOOTPRINT_LINK_ISSUES;
    case ErcErrorType::ERCET_FOOTPRINT_FILTERS:             return ERCE_FOOTPRINT_FILTERS;
    case ErcErrorType::ERCET_UNANNOTATED:                   return ERCE_UNANNOTATED;
    case ErcErrorType::ERCET_EXTRA_UNITS:                   return ERCE_EXTRA_UNITS;
    case ErcErrorType::ERCET_DIFFERENT_UNIT_VALUE:          return ERCE_DIFFERENT_UNIT_VALUE;
    case ErcErrorType::ERCET_DUPLICATE_REFERENCE:           return ERCE_DUPLICATE_REFERENCE;
    case ErcErrorType::ERCET_BUS_ENTRY_NEEDED:              return ERCE_BUS_ENTRY_NEEDED;
    case ErcErrorType::ERCET_FOUR_WAY_JUNCTION:             return ERCE_FOUR_WAY_JUNCTION;
    case ErcErrorType::ERCET_LABEL_MULTIPLE_WIRES:          return ERCE_LABEL_MULTIPLE_WIRES;
    case ErcErrorType::ERCET_UNCONNECTED_WIRE_ENDPOINT:     return ERCE_UNCONNECTED_WIRE_ENDPOINT;
    case ErcErrorType::ERCET_STACKED_PIN_SYNTAX:            return ERCE_STACKED_PIN_SYNTAX;
    case ErcErrorType::ERCET_PIN_MAP_BAD_PAD:               return ERCE_PIN_MAP_BAD_PAD;
    case ErcErrorType::ERCET_PIN_MAP_UNMAPPED_PIN:          return ERCE_PIN_MAP_UNMAPPED_PIN;
    case ErcErrorType::ERCET_PIN_MAP_DUPLICATE_PAD:         return ERCE_PIN_MAP_DUPLICATE_PAD;
    case ErcErrorType::ERCET_PIN_MAP_STALE_PIN:             return ERCE_PIN_MAP_STALE_PIN;
    case ErcErrorType::ERCET_EMPTY_LABEL_NAME:              return ERCE_EMPTY_LABEL_NAME;
    case ErcErrorType::ERCET_VARIANT_SYMBOL_INVALID:        return ERCE_VARIANT_SYMBOL_INVALID;
    case ErcErrorType::ERCET_VARIANT_SYMBOL_INCOMPATIBLE:   return ERCE_VARIANT_SYMBOL_INCOMPATIBLE;
    case ErcErrorType::ERCET_DUPLICATE_PIN_ERROR:           return ERCE_DUPLICATE_PIN_ERROR;
    case ErcErrorType::ERCET_PIN_TO_PIN_WARNING:            return ERCE_PIN_TO_PIN_WARNING;
    case ErcErrorType::ERCET_PIN_TO_PIN_ERROR:              return ERCE_PIN_TO_PIN_ERROR;
    case ErcErrorType::ERCET_ANNOTATION_ACTION:             return ERCE_ANNOTATION_ACTION;
    case ErcErrorType::ERCET_GENERIC_WARNING:               return ERCE_GENERIC_WARNING;
    case ErcErrorType::ERCET_GENERIC_ERROR:                 return ERCE_GENERIC_ERROR;
    case ErcErrorType::ERCET_FIELD_NAME_WHITESPACE:         return ERCE_FIELD_NAME_WHITESPACE;

    case ErcErrorType::ERCET_UNKNOWN:
    default:
        return ERCE_UNSPECIFIED;
    }
}
