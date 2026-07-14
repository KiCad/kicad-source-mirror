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
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include <import_export.h>
#include <api/api_enums.h>
#include <api/board/board.pb.h>
#include <api/board/board_types.pb.h>
#include <api/board/board_commands.pb.h>
#include <api/board/board_jobs.pb.h>
#include <api/common/types/enums.pb.h>
#include <wx/wx.h>
#include <widgets/report_severity.h>

#include <board_stackup_manager/board_stackup.h>
#include <constraints/pcb_constraint.h>
#include <drc/drc_item.h>
#include <padstack.h>
#include <pcb_dimension.h>
#include <pcb_track.h>
#include <jobs/job_export_pcb_3d.h>
#include <jobs/job_export_pcb_dxf.h>
#include <jobs/job_export_pcb_drill.h>
#include <jobs/job_export_pcb_ipc2581.h>
#include <jobs/job_export_pcb_odb.h>
#include <jobs/job_export_pcb_pdf.h>
#include <jobs/job_export_pcb_pos.h>
#include <jobs/job_export_pcb_ps.h>
#include <jobs/job_export_pcb_stats.h>
#include <jobs/job_export_pcb_svg.h>
#include <jobs/job_pcb_render.h>
#include <drc/drc_rule.h>
#include <plotprint_opts.h>
#include <zones.h>
#include <zone_settings.h>
#include <project/board_project_settings.h>

// Adding something new here?  Add it to test_api_enums.cpp!

using namespace kiapi::board;
using namespace kiapi::board::commands;
using namespace kiapi::board::jobs;

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
    case types::PadType::PT_UNKNOWN:
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
    case types::DrillShape::DS_UNKNOWN:
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
    case types::PadStackShape::PSS_UNKNOWN:
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
    case types::PadStackType::PST_UNKNOWN:
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
    case VIATYPE::BLIND:        return types::ViaType::VT_BLIND; // Since V10
    case VIATYPE::BURIED:       return types::ViaType::VT_BURIED;
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
    case types::ViaType::VT_UNKNOWN:
    case types::ViaType::VT_THROUGH:      return VIATYPE::THROUGH;
    case types::ViaType::VT_BLIND_BURIED: return VIATYPE::BLIND;
    case types::ViaType::VT_BLIND:        return VIATYPE::BLIND;    // Since V10
    case types::ViaType::VT_BURIED:       return VIATYPE::BURIED;
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
CustomRuleConstraintType ToProtoEnum( DRC_CONSTRAINT_T aValue )
{
    switch( aValue )
    {
    case NULL_CONSTRAINT:                 return CustomRuleConstraintType::CRCT_UNKNOWN;
    case CLEARANCE_CONSTRAINT:            return CustomRuleConstraintType::CRCT_CLEARANCE;
    case CREEPAGE_CONSTRAINT:             return CustomRuleConstraintType::CRCT_CREEPAGE;
    case HOLE_CLEARANCE_CONSTRAINT:       return CustomRuleConstraintType::CRCT_HOLE_CLEARANCE;
    case HOLE_TO_HOLE_CONSTRAINT:         return CustomRuleConstraintType::CRCT_HOLE_TO_HOLE;
    case EDGE_CLEARANCE_CONSTRAINT:       return CustomRuleConstraintType::CRCT_EDGE_CLEARANCE;
    case HOLE_SIZE_CONSTRAINT:            return CustomRuleConstraintType::CRCT_HOLE_SIZE;
    case COURTYARD_CLEARANCE_CONSTRAINT:  return CustomRuleConstraintType::CRCT_COURTYARD_CLEARANCE;
    case SILK_CLEARANCE_CONSTRAINT:       return CustomRuleConstraintType::CRCT_SILK_CLEARANCE;
    case TEXT_HEIGHT_CONSTRAINT:          return CustomRuleConstraintType::CRCT_TEXT_HEIGHT;
    case TEXT_THICKNESS_CONSTRAINT:       return CustomRuleConstraintType::CRCT_TEXT_THICKNESS;
    case TRACK_WIDTH_CONSTRAINT:          return CustomRuleConstraintType::CRCT_TRACK_WIDTH;
    case TRACK_SEGMENT_LENGTH_CONSTRAINT: return CustomRuleConstraintType::CRCT_TRACK_SEGMENT_LENGTH;
    case ANNULAR_WIDTH_CONSTRAINT:        return CustomRuleConstraintType::CRCT_ANNULAR_WIDTH;
    case ZONE_CONNECTION_CONSTRAINT:      return CustomRuleConstraintType::CRCT_ZONE_CONNECTION;
    case THERMAL_RELIEF_GAP_CONSTRAINT:   return CustomRuleConstraintType::CRCT_THERMAL_RELIEF_GAP;
    case THERMAL_SPOKE_WIDTH_CONSTRAINT:  return CustomRuleConstraintType::CRCT_THERMAL_SPOKE_WIDTH;
    case MIN_RESOLVED_SPOKES_CONSTRAINT:  return CustomRuleConstraintType::CRCT_MIN_RESOLVED_SPOKES;
    case SOLDER_MASK_EXPANSION_CONSTRAINT:return CustomRuleConstraintType::CRCT_SOLDER_MASK_EXPANSION;
    case SOLDER_PASTE_ABS_MARGIN_CONSTRAINT:return CustomRuleConstraintType::CRCT_SOLDER_PASTE_ABS_MARGIN;
    case SOLDER_PASTE_REL_MARGIN_CONSTRAINT:return CustomRuleConstraintType::CRCT_SOLDER_PASTE_REL_MARGIN;
    case DISALLOW_CONSTRAINT:             return CustomRuleConstraintType::CRCT_DISALLOW;
    case VIA_DIAMETER_CONSTRAINT:         return CustomRuleConstraintType::CRCT_VIA_DIAMETER;
    case LENGTH_CONSTRAINT:               return CustomRuleConstraintType::CRCT_LENGTH;
    case SKEW_CONSTRAINT:                 return CustomRuleConstraintType::CRCT_SKEW;
    case DIFF_PAIR_GAP_CONSTRAINT:        return CustomRuleConstraintType::CRCT_DIFF_PAIR_GAP;
    case MAX_UNCOUPLED_CONSTRAINT:        return CustomRuleConstraintType::CRCT_MAX_UNCOUPLED;
    case DIFF_PAIR_INTRA_SKEW_CONSTRAINT: return CustomRuleConstraintType::CRCT_DIFF_PAIR_INTRA_SKEW;
    case VIA_COUNT_CONSTRAINT:            return CustomRuleConstraintType::CRCT_VIA_COUNT;
    case PHYSICAL_CLEARANCE_CONSTRAINT:   return CustomRuleConstraintType::CRCT_PHYSICAL_CLEARANCE;
    case PHYSICAL_HOLE_CLEARANCE_CONSTRAINT:return CustomRuleConstraintType::CRCT_PHYSICAL_HOLE_CLEARANCE;
    case ASSERTION_CONSTRAINT:            return CustomRuleConstraintType::CRCT_ASSERTION;
    case CONNECTION_WIDTH_CONSTRAINT:     return CustomRuleConstraintType::CRCT_CONNECTION_WIDTH;
    case TRACK_ANGLE_CONSTRAINT:          return CustomRuleConstraintType::CRCT_TRACK_ANGLE;
    case VIA_DANGLING_CONSTRAINT:         return CustomRuleConstraintType::CRCT_VIA_DANGLING;
    case BRIDGED_MASK_CONSTRAINT:         return CustomRuleConstraintType::CRCT_BRIDGED_MASK;
    case SOLDER_MASK_SLIVER_CONSTRAINT:   return CustomRuleConstraintType::CRCT_SOLDER_MASK_SLIVER;
    case NET_CHAIN_LENGTH_CONSTRAINT:        return CustomRuleConstraintType::CRCT_NET_CHAIN_LENGTH;
    case NET_CHAIN_STUB_LENGTH_CONSTRAINT:
        return CustomRuleConstraintType::CRCT_NET_CHAIN_STUB_LENGTH;
    case NET_CHAIN_RETURN_PATH_CONSTRAINT:
        return CustomRuleConstraintType::CRCT_NET_CHAIN_RETURN_PATH;

    default:
        wxCHECK_MSG( false, CustomRuleConstraintType::CRCT_UNKNOWN,
                     "Unhandled case in ToProtoEnum<DRC_CONSTRAINT_T>" );
    }
}


template<>
DRC_CONSTRAINT_T FromProtoEnum( CustomRuleConstraintType aValue )
{
    switch( aValue )
    {
    case CustomRuleConstraintType::CRCT_UNKNOWN:               return NULL_CONSTRAINT;
    case CustomRuleConstraintType::CRCT_CLEARANCE:             return CLEARANCE_CONSTRAINT;
    case CustomRuleConstraintType::CRCT_CREEPAGE:              return CREEPAGE_CONSTRAINT;
    case CustomRuleConstraintType::CRCT_HOLE_CLEARANCE:        return HOLE_CLEARANCE_CONSTRAINT;
    case CustomRuleConstraintType::CRCT_HOLE_TO_HOLE:          return HOLE_TO_HOLE_CONSTRAINT;
    case CustomRuleConstraintType::CRCT_EDGE_CLEARANCE:        return EDGE_CLEARANCE_CONSTRAINT;
    case CustomRuleConstraintType::CRCT_HOLE_SIZE:             return HOLE_SIZE_CONSTRAINT;
    case CustomRuleConstraintType::CRCT_COURTYARD_CLEARANCE:   return COURTYARD_CLEARANCE_CONSTRAINT;
    case CustomRuleConstraintType::CRCT_SILK_CLEARANCE:        return SILK_CLEARANCE_CONSTRAINT;
    case CustomRuleConstraintType::CRCT_TEXT_HEIGHT:           return TEXT_HEIGHT_CONSTRAINT;
    case CustomRuleConstraintType::CRCT_TEXT_THICKNESS:        return TEXT_THICKNESS_CONSTRAINT;
    case CustomRuleConstraintType::CRCT_TRACK_WIDTH:           return TRACK_WIDTH_CONSTRAINT;
    case CustomRuleConstraintType::CRCT_TRACK_SEGMENT_LENGTH:  return TRACK_SEGMENT_LENGTH_CONSTRAINT;
    case CustomRuleConstraintType::CRCT_ANNULAR_WIDTH:         return ANNULAR_WIDTH_CONSTRAINT;
    case CustomRuleConstraintType::CRCT_ZONE_CONNECTION:       return ZONE_CONNECTION_CONSTRAINT;
    case CustomRuleConstraintType::CRCT_THERMAL_RELIEF_GAP:    return THERMAL_RELIEF_GAP_CONSTRAINT;
    case CustomRuleConstraintType::CRCT_THERMAL_SPOKE_WIDTH:   return THERMAL_SPOKE_WIDTH_CONSTRAINT;
    case CustomRuleConstraintType::CRCT_MIN_RESOLVED_SPOKES:   return MIN_RESOLVED_SPOKES_CONSTRAINT;
    case CustomRuleConstraintType::CRCT_SOLDER_MASK_EXPANSION: return SOLDER_MASK_EXPANSION_CONSTRAINT;
    case CustomRuleConstraintType::CRCT_SOLDER_PASTE_ABS_MARGIN:return SOLDER_PASTE_ABS_MARGIN_CONSTRAINT;
    case CustomRuleConstraintType::CRCT_SOLDER_PASTE_REL_MARGIN:return SOLDER_PASTE_REL_MARGIN_CONSTRAINT;
    case CustomRuleConstraintType::CRCT_DISALLOW:              return DISALLOW_CONSTRAINT;
    case CustomRuleConstraintType::CRCT_VIA_DIAMETER:          return VIA_DIAMETER_CONSTRAINT;
    case CustomRuleConstraintType::CRCT_LENGTH:                return LENGTH_CONSTRAINT;
    case CustomRuleConstraintType::CRCT_SKEW:                  return SKEW_CONSTRAINT;
    case CustomRuleConstraintType::CRCT_DIFF_PAIR_GAP:         return DIFF_PAIR_GAP_CONSTRAINT;
    case CustomRuleConstraintType::CRCT_MAX_UNCOUPLED:         return MAX_UNCOUPLED_CONSTRAINT;
    case CustomRuleConstraintType::CRCT_DIFF_PAIR_INTRA_SKEW:  return DIFF_PAIR_INTRA_SKEW_CONSTRAINT;
    case CustomRuleConstraintType::CRCT_VIA_COUNT:             return VIA_COUNT_CONSTRAINT;
    case CustomRuleConstraintType::CRCT_PHYSICAL_CLEARANCE:    return PHYSICAL_CLEARANCE_CONSTRAINT;
    case CustomRuleConstraintType::CRCT_PHYSICAL_HOLE_CLEARANCE:return PHYSICAL_HOLE_CLEARANCE_CONSTRAINT;
    case CustomRuleConstraintType::CRCT_ASSERTION:             return ASSERTION_CONSTRAINT;
    case CustomRuleConstraintType::CRCT_CONNECTION_WIDTH:      return CONNECTION_WIDTH_CONSTRAINT;
    case CustomRuleConstraintType::CRCT_TRACK_ANGLE:           return TRACK_ANGLE_CONSTRAINT;
    case CustomRuleConstraintType::CRCT_VIA_DANGLING:          return VIA_DANGLING_CONSTRAINT;
    case CustomRuleConstraintType::CRCT_BRIDGED_MASK:          return BRIDGED_MASK_CONSTRAINT;
    case CustomRuleConstraintType::CRCT_SOLDER_MASK_SLIVER:    return SOLDER_MASK_SLIVER_CONSTRAINT;
    case CustomRuleConstraintType::CRCT_NET_CHAIN_LENGTH:         return NET_CHAIN_LENGTH_CONSTRAINT;
    case CustomRuleConstraintType::CRCT_NET_CHAIN_STUB_LENGTH: return NET_CHAIN_STUB_LENGTH_CONSTRAINT;
    case CustomRuleConstraintType::CRCT_NET_CHAIN_RETURN_PATH: return NET_CHAIN_RETURN_PATH_CONSTRAINT;

    default:
        wxCHECK_MSG( false, NULL_CONSTRAINT,
                     "Unhandled case in FromProtoEnum<CustomRuleConstraintType>" );
    }
}


template<>
CustomRuleConstraintOption ToProtoEnum( DRC_CONSTRAINT::OPTIONS aValue )
{
    switch( aValue )
    {
    case DRC_CONSTRAINT::OPTIONS::SKEW_WITHIN_DIFF_PAIRS: return CustomRuleConstraintOption::CRCO_SKEW_WITHIN_DIFF_PAIRS;
    case DRC_CONSTRAINT::OPTIONS::SPACE_DOMAIN:           return CustomRuleConstraintOption::CRCO_SPACE_DOMAIN;
    case DRC_CONSTRAINT::OPTIONS::TIME_DOMAIN:            return CustomRuleConstraintOption::CRCO_TIME_DOMAIN;

    default:
        wxCHECK_MSG( false, CustomRuleConstraintOption::CRCO_UNKNOWN,
                     "Unhandled case in ToProtoEnum<DRC_CONSTRAINT::OPTIONS>" );
    }
}


template<>
DRC_CONSTRAINT::OPTIONS FromProtoEnum( CustomRuleConstraintOption aValue )
{
    switch( aValue )
    {
    case CustomRuleConstraintOption::CRCO_SKEW_WITHIN_DIFF_PAIRS:
        return DRC_CONSTRAINT::OPTIONS::SKEW_WITHIN_DIFF_PAIRS;

    case CustomRuleConstraintOption::CRCO_SPACE_DOMAIN:
        return DRC_CONSTRAINT::OPTIONS::SPACE_DOMAIN;

    case CustomRuleConstraintOption::CRCO_TIME_DOMAIN:
        return DRC_CONSTRAINT::OPTIONS::TIME_DOMAIN;

    case CustomRuleConstraintOption::CRCO_UNKNOWN:
    default:
        wxCHECK_MSG( false, DRC_CONSTRAINT::OPTIONS::SKEW_WITHIN_DIFF_PAIRS,
                     "Unhandled case in FromProtoEnum<CustomRuleConstraintOption>" );
    }
}


template<>
CustomRuleDisallowType ToProtoEnum( DRC_DISALLOW_T aValue )
{
    switch( aValue )
    {
    case DRC_DISALLOW_THROUGH_VIAS: return CustomRuleDisallowType::CRDT_THROUGH_VIAS;
    case DRC_DISALLOW_MICRO_VIAS:   return CustomRuleDisallowType::CRDT_MICRO_VIAS;
    case DRC_DISALLOW_BLIND_VIAS:   return CustomRuleDisallowType::CRDT_BLIND_VIAS;
    case DRC_DISALLOW_BURIED_VIAS:  return CustomRuleDisallowType::CRDT_BURIED_VIAS;
    case DRC_DISALLOW_TRACKS:       return CustomRuleDisallowType::CRDT_TRACKS;
    case DRC_DISALLOW_PADS:         return CustomRuleDisallowType::CRDT_PADS;
    case DRC_DISALLOW_ZONES:        return CustomRuleDisallowType::CRDT_ZONES;
    case DRC_DISALLOW_TEXTS:        return CustomRuleDisallowType::CRDT_TEXTS;
    case DRC_DISALLOW_GRAPHICS:     return CustomRuleDisallowType::CRDT_GRAPHICS;
    case DRC_DISALLOW_HOLES:        return CustomRuleDisallowType::CRDT_HOLES;
    case DRC_DISALLOW_FOOTPRINTS:   return CustomRuleDisallowType::CRDT_FOOTPRINTS;

    default:
        wxCHECK_MSG( false, CustomRuleDisallowType::CRDT_UNKNOWN,
                     "Unhandled case in ToProtoEnum<DRC_DISALLOW_T>" );
    }
}


template<>
DRC_DISALLOW_T FromProtoEnum( CustomRuleDisallowType aValue )
{
    switch( aValue )
    {
    case CustomRuleDisallowType::CRDT_THROUGH_VIAS: return DRC_DISALLOW_THROUGH_VIAS;
    case CustomRuleDisallowType::CRDT_MICRO_VIAS:   return DRC_DISALLOW_MICRO_VIAS;
    case CustomRuleDisallowType::CRDT_BLIND_VIAS:   return DRC_DISALLOW_BLIND_VIAS;
    case CustomRuleDisallowType::CRDT_BURIED_VIAS:  return DRC_DISALLOW_BURIED_VIAS;
    case CustomRuleDisallowType::CRDT_TRACKS:       return DRC_DISALLOW_TRACKS;
    case CustomRuleDisallowType::CRDT_PADS:         return DRC_DISALLOW_PADS;
    case CustomRuleDisallowType::CRDT_ZONES:        return DRC_DISALLOW_ZONES;
    case CustomRuleDisallowType::CRDT_TEXTS:        return DRC_DISALLOW_TEXTS;
    case CustomRuleDisallowType::CRDT_GRAPHICS:     return DRC_DISALLOW_GRAPHICS;
    case CustomRuleDisallowType::CRDT_HOLES:        return DRC_DISALLOW_HOLES;
    case CustomRuleDisallowType::CRDT_FOOTPRINTS:   return DRC_DISALLOW_FOOTPRINTS;

    case CustomRuleDisallowType::CRDT_UNKNOWN:
    default:
        wxCHECK_MSG( false, DRC_DISALLOW_THROUGH_VIAS,
                     "Unhandled case in FromProtoEnum<CustomRuleDisallowType>" );
    }
}


template<>
types::UnconnectedLayerRemoval ToProtoEnum( UNCONNECTED_LAYER_MODE aValue )
{
    switch( aValue )
    {
    case UNCONNECTED_LAYER_MODE::KEEP_ALL:
        return types::UnconnectedLayerRemoval::ULR_KEEP;

    case UNCONNECTED_LAYER_MODE::REMOVE_ALL:
        return types::UnconnectedLayerRemoval::ULR_REMOVE;

    case UNCONNECTED_LAYER_MODE::REMOVE_EXCEPT_START_AND_END:
        return types::UnconnectedLayerRemoval::ULR_REMOVE_EXCEPT_START_AND_END;

    case UNCONNECTED_LAYER_MODE::START_END_ONLY:
        return types::UnconnectedLayerRemoval::ULR_START_END_ONLY;

    default:
        wxCHECK_MSG( false, types::UnconnectedLayerRemoval::ULR_UNKNOWN,
                     "Unhandled case in ToProtoEnum<PADSTACK::UNCONNECTED_LAYER_MODE>");
    }
}


template<>
UNCONNECTED_LAYER_MODE FromProtoEnum( types::UnconnectedLayerRemoval aValue )
{
    switch( aValue )
    {
    case types::UnconnectedLayerRemoval::ULR_UNKNOWN:
    case types::UnconnectedLayerRemoval::ULR_KEEP:
        return UNCONNECTED_LAYER_MODE::KEEP_ALL;

    case types::UnconnectedLayerRemoval::ULR_REMOVE:
        return UNCONNECTED_LAYER_MODE::REMOVE_ALL;

    case types::UnconnectedLayerRemoval::ULR_REMOVE_EXCEPT_START_AND_END:
        return UNCONNECTED_LAYER_MODE::REMOVE_EXCEPT_START_AND_END;

    case types::UnconnectedLayerRemoval::ULR_START_END_ONLY:
        return UNCONNECTED_LAYER_MODE::START_END_ONLY;

    default:
        wxCHECK_MSG( false, UNCONNECTED_LAYER_MODE::KEEP_ALL,
                     "Unhandled case in FromProtoEnum<types::UnconnectedLayerRemoval>");
    }
}


template<>
types::IslandRemovalMode ToProtoEnum( ISLAND_REMOVAL_MODE aValue )
{
    switch( aValue )
    {
    case ISLAND_REMOVAL_MODE::ALWAYS:   return types::IslandRemovalMode::IRM_ALWAYS;
    case ISLAND_REMOVAL_MODE::NEVER:    return types::IslandRemovalMode::IRM_NEVER;
    case ISLAND_REMOVAL_MODE::AREA:     return types::IslandRemovalMode::IRM_AREA;

    default:
        wxCHECK_MSG( false, types::IslandRemovalMode::IRM_UNKNOWN,
                     "Unhandled case in ToProtoEnum<ISLAND_REMOVAL_MODE>");
    }
}


template<>
ISLAND_REMOVAL_MODE FromProtoEnum( types::IslandRemovalMode aValue )
{
    switch( aValue )
    {
    case types::IslandRemovalMode::IRM_UNKNOWN:
    case types::IslandRemovalMode::IRM_ALWAYS:  return ISLAND_REMOVAL_MODE::ALWAYS;
    case types::IslandRemovalMode::IRM_NEVER:   return ISLAND_REMOVAL_MODE::NEVER;
    case types::IslandRemovalMode::IRM_AREA:    return ISLAND_REMOVAL_MODE::AREA;

    default:
        wxCHECK_MSG( false, ISLAND_REMOVAL_MODE::ALWAYS,
                     "Unhandled case in FromProtoEnum<types::IslandRemovalMode>" );
    }
}


template<>
types::ZoneFillMode ToProtoEnum( ZONE_FILL_MODE aValue )
{
    switch( aValue )
    {
    case ZONE_FILL_MODE::POLYGONS:        return types::ZoneFillMode::ZFM_SOLID;
    case ZONE_FILL_MODE::HATCH_PATTERN:   return types::ZoneFillMode::ZFM_HATCHED;
    case ZONE_FILL_MODE::COPPER_THIEVING: return types::ZoneFillMode::ZFM_COPPER_THIEVING;

    default:
        wxCHECK_MSG( false, types::ZoneFillMode::ZFM_UNKNOWN,
                     "Unhandled case in ToProtoEnum<ZONE_FILL_MODE>");
    }
}


template<>
ZONE_FILL_MODE FromProtoEnum( types::ZoneFillMode aValue )
{
    switch( aValue )
    {
    case types::ZoneFillMode::ZFM_UNKNOWN:
    case types::ZoneFillMode::ZFM_SOLID:           return ZONE_FILL_MODE::POLYGONS;
    case types::ZoneFillMode::ZFM_HATCHED:         return ZONE_FILL_MODE::HATCH_PATTERN;
    case types::ZoneFillMode::ZFM_COPPER_THIEVING: return ZONE_FILL_MODE::COPPER_THIEVING;

    default:
        wxCHECK_MSG( false, ZONE_FILL_MODE::POLYGONS,
                     "Unhandled case in FromProtoEnum<types::ZoneFillMode>" );
    }
}


template<>
types::ThievingPattern ToProtoEnum( THIEVING_PATTERN aValue )
{
    switch( aValue )
    {
    case THIEVING_PATTERN::DOTS:       return types::ThievingPattern::TP_DOTS;
    case THIEVING_PATTERN::SQUARES:    return types::ThievingPattern::TP_SQUARES;
    case THIEVING_PATTERN::HATCH: return types::ThievingPattern::TP_CROSSHATCH;

    default:
        wxCHECK_MSG( false, types::ThievingPattern::TP_UNKNOWN,
                     "Unhandled case in ToProtoEnum<THIEVING_PATTERN>" );
    }
}


template<>
THIEVING_PATTERN FromProtoEnum( types::ThievingPattern aValue )
{
    switch( aValue )
    {
    case types::ThievingPattern::TP_UNKNOWN:
    case types::ThievingPattern::TP_DOTS:       return THIEVING_PATTERN::DOTS;
    case types::ThievingPattern::TP_SQUARES:    return THIEVING_PATTERN::SQUARES;
    case types::ThievingPattern::TP_CROSSHATCH: return THIEVING_PATTERN::HATCH;

    default:
        wxCHECK_MSG( false, THIEVING_PATTERN::DOTS,
                     "Unhandled case in FromProtoEnum<types::ThievingPattern>" );
    }
}


template<>
types::ZoneBorderStyle ToProtoEnum( ZONE_BORDER_DISPLAY_STYLE aValue )
{
    switch( aValue )
    {
    case ZONE_BORDER_DISPLAY_STYLE::NO_HATCH:         return types::ZoneBorderStyle::ZBS_SOLID;
    case ZONE_BORDER_DISPLAY_STYLE::DIAGONAL_FULL:    return types::ZoneBorderStyle::ZBS_DIAGONAL_FULL;
    case ZONE_BORDER_DISPLAY_STYLE::DIAGONAL_EDGE:    return types::ZoneBorderStyle::ZBS_DIAGONAL_EDGE;
    case ZONE_BORDER_DISPLAY_STYLE::INVISIBLE_BORDER: return types::ZoneBorderStyle::ZBS_INVISIBLE;

    default:
        wxCHECK_MSG( false, types::ZoneBorderStyle::ZBS_UNKNOWN,
                     "Unhandled case in ToProtoEnum<ZONE_BORDER_DISPLAY_STYLE>");
    }
}


template<>
ZONE_BORDER_DISPLAY_STYLE FromProtoEnum( types::ZoneBorderStyle aValue )
{
    switch( aValue )
    {
    case types::ZoneBorderStyle::ZBS_SOLID:         return ZONE_BORDER_DISPLAY_STYLE::NO_HATCH;
    case types::ZoneBorderStyle::ZBS_DIAGONAL_FULL: return ZONE_BORDER_DISPLAY_STYLE::DIAGONAL_FULL;
    case types::ZoneBorderStyle::ZBS_UNKNOWN:
    case types::ZoneBorderStyle::ZBS_DIAGONAL_EDGE: return ZONE_BORDER_DISPLAY_STYLE::DIAGONAL_EDGE;
    case types::ZoneBorderStyle::ZBS_INVISIBLE:     return ZONE_BORDER_DISPLAY_STYLE::INVISIBLE_BORDER;

    default:
        wxCHECK_MSG( false, ZONE_BORDER_DISPLAY_STYLE::DIAGONAL_EDGE,
                     "Unhandled case in FromProtoEnum<types::ZoneHatchBorderMode>" );
    }
}


template<>
types::PlacementRuleSourceType ToProtoEnum( PLACEMENT_SOURCE_T aValue )
{
    switch( aValue )
    {
    case PLACEMENT_SOURCE_T::SHEETNAME:
        return types::PlacementRuleSourceType::PRST_SHEET_NAME;

    case PLACEMENT_SOURCE_T::COMPONENT_CLASS:
        return types::PlacementRuleSourceType::PRST_COMPONENT_CLASS;

    case PLACEMENT_SOURCE_T::GROUP_PLACEMENT:
        return types::PlacementRuleSourceType::PRST_GROUP;

    case PLACEMENT_SOURCE_T::DESIGN_BLOCK:
        return types::PlacementRuleSourceType::PRST_DESIGN_BLOCK;

    default:
        wxCHECK_MSG( false, types::PlacementRuleSourceType::PRST_UNKNOWN,
                     "Unhandled case in ToProtoEnum<PLACEMENT_SOURCE_T>");
    }
}


template<>
PLACEMENT_SOURCE_T FromProtoEnum( types::PlacementRuleSourceType aValue )
{
    switch( aValue )
    {
    case types::PlacementRuleSourceType::PRST_UNKNOWN:
    case types::PlacementRuleSourceType::PRST_SHEET_NAME:
        return PLACEMENT_SOURCE_T::SHEETNAME;

    case types::PlacementRuleSourceType::PRST_COMPONENT_CLASS:
        return PLACEMENT_SOURCE_T::COMPONENT_CLASS;

    case types::PlacementRuleSourceType::PRST_GROUP:
        return PLACEMENT_SOURCE_T::GROUP_PLACEMENT;

    case types::PlacementRuleSourceType::PRST_DESIGN_BLOCK:
        return PLACEMENT_SOURCE_T::DESIGN_BLOCK;

    default:
        wxCHECK_MSG( false, PLACEMENT_SOURCE_T::SHEETNAME,
                     "Unhandled case in FromProtoEnum<types::PlacementRuleSourceType>" );
    }
}


template<>
types::TeardropType ToProtoEnum( TEARDROP_TYPE aValue )
{
    switch( aValue )
    {
    case TEARDROP_TYPE::TD_NONE:        return types::TeardropType::TDT_NONE;
    case TEARDROP_TYPE::TD_UNSPECIFIED: return types::TeardropType::TDT_UNSPECIFIED;
    case TEARDROP_TYPE::TD_VIAPAD:      return types::TeardropType::TDT_VIA_PAD;
    case TEARDROP_TYPE::TD_TRACKEND:    return types::TeardropType::TDT_TRACK_END;

    default:
        wxCHECK_MSG( false, types::TeardropType::TDT_UNKNOWN,
                     "Unhandled case in ToProtoEnum<TEARDROP_TYPE>");
    }
}


template<>
TEARDROP_TYPE FromProtoEnum( types::TeardropType aValue )
{
    switch( aValue )
    {
    case types::TeardropType::TDT_UNKNOWN:
    case types::TeardropType::TDT_NONE:         return TEARDROP_TYPE::TD_NONE;
    case types::TeardropType::TDT_UNSPECIFIED:  return TEARDROP_TYPE::TD_UNSPECIFIED;
    case types::TeardropType::TDT_VIA_PAD:      return TEARDROP_TYPE::TD_VIAPAD;
    case types::TeardropType::TDT_TRACK_END:    return TEARDROP_TYPE::TD_TRACKEND;

    default:
        wxCHECK_MSG( false, TEARDROP_TYPE::TD_NONE,
                     "Unhandled case in FromProtoEnum<types::ZoneHatchBorderMode>" );
    }
}


template<>
kiapi::board::TeardropTarget ToProtoEnum( TARGET_TD aValue )
{
    switch( aValue )
    {
    case TARGET_ROUND: return kiapi::board::TeardropTarget::TDT_ROUND;
    case TARGET_RECT:  return kiapi::board::TeardropTarget::TDT_RECT;
    case TARGET_TRACK: return kiapi::board::TeardropTarget::TDT_TRACK;

    default:
        wxCHECK_MSG( false, kiapi::board::TeardropTarget::TDT_UNKNOWN,
                     "Unhandled case in ToProtoEnum<TARGET_TD>" );
    }
}


template<>
TARGET_TD FromProtoEnum( kiapi::board::TeardropTarget aValue )
{
    switch( aValue )
    {
    case kiapi::board::TeardropTarget::TDT_ROUND: return TARGET_ROUND;
    case kiapi::board::TeardropTarget::TDT_RECT:  return TARGET_RECT;
    case kiapi::board::TeardropTarget::TDT_TRACK: return TARGET_TRACK;

    case kiapi::board::TeardropTarget::TDT_UNKNOWN:
    default:
        return TARGET_UNKNOWN;
    }
}


template<>
types::DimensionTextBorderStyle ToProtoEnum( DIM_TEXT_BORDER aValue )
{
    switch( aValue )
    {
    case DIM_TEXT_BORDER::NONE:         return types::DimensionTextBorderStyle::DTBS_NONE;
    case DIM_TEXT_BORDER::RECTANGLE:    return types::DimensionTextBorderStyle::DTBS_RECTANGLE;
    case DIM_TEXT_BORDER::CIRCLE:       return types::DimensionTextBorderStyle::DTBS_CIRCLE;
    case DIM_TEXT_BORDER::ROUNDRECT:    return types::DimensionTextBorderStyle::DTBS_ROUNDRECT;

    default:
        wxCHECK_MSG( false, types::DimensionTextBorderStyle::DTBS_UNKNOWN,
                     "Unhandled case in ToProtoEnum<DIM_TEXT_BORDER>");
    }
}


template<>
DIM_TEXT_BORDER FromProtoEnum( types::DimensionTextBorderStyle aValue )
{
    switch( aValue )
    {
    case types::DimensionTextBorderStyle::DTBS_UNKNOWN:
    case types::DimensionTextBorderStyle::DTBS_NONE:        return DIM_TEXT_BORDER::NONE;
    case types::DimensionTextBorderStyle::DTBS_RECTANGLE:   return DIM_TEXT_BORDER::RECTANGLE;
    case types::DimensionTextBorderStyle::DTBS_CIRCLE:      return DIM_TEXT_BORDER::CIRCLE;
    case types::DimensionTextBorderStyle::DTBS_ROUNDRECT:   return DIM_TEXT_BORDER::ROUNDRECT;

    default:
        wxCHECK_MSG( false,  DIM_TEXT_BORDER::NONE,
                     "Unhandled case in FromProtoEnum<types::DimensionTextBorderStyle>" );
    }
}


template<>
types::DimensionUnitFormat ToProtoEnum( DIM_UNITS_FORMAT aValue )
{
    switch( aValue )
    {
    case DIM_UNITS_FORMAT::NO_SUFFIX:       return types::DimensionUnitFormat::DUF_NO_SUFFIX;
    case DIM_UNITS_FORMAT::BARE_SUFFIX:     return types::DimensionUnitFormat::DUF_BARE_SUFFIX;
    case DIM_UNITS_FORMAT::PAREN_SUFFIX:    return types::DimensionUnitFormat::DUF_PAREN_SUFFIX;

    default:
        wxCHECK_MSG( false, types::DimensionUnitFormat::DUF_UNKNOWN,
                     "Unhandled case in ToProtoEnum<DIM_UNITS_FORMAT>");
    }
}


template<>
DIM_UNITS_FORMAT FromProtoEnum( types::DimensionUnitFormat aValue )
{
    switch( aValue )
    {
    case types::DimensionUnitFormat::DUF_UNKNOWN:
    case types::DimensionUnitFormat::DUF_NO_SUFFIX:     return DIM_UNITS_FORMAT::NO_SUFFIX;
    case types::DimensionUnitFormat::DUF_BARE_SUFFIX:   return DIM_UNITS_FORMAT::BARE_SUFFIX;
    case types::DimensionUnitFormat::DUF_PAREN_SUFFIX:  return DIM_UNITS_FORMAT::PAREN_SUFFIX;

    default:
        wxCHECK_MSG( false,  DIM_UNITS_FORMAT::NO_SUFFIX,
                     "Unhandled case in FromProtoEnum<types::DimensionUnitFormat>" );
    }
}


template<>
types::DimensionArrowDirection ToProtoEnum( DIM_ARROW_DIRECTION aValue )
{
    switch( aValue )
    {
    case DIM_ARROW_DIRECTION::INWARD:   return types::DimensionArrowDirection::DAD_INWARD;
    case DIM_ARROW_DIRECTION::OUTWARD:  return types::DimensionArrowDirection::DAD_OUTWARD;

    default:
        wxCHECK_MSG( false, types::DimensionArrowDirection::DAD_UNKNOWN,
                     "Unhandled case in ToProtoEnum<DIM_ARROW_DIRECTION>");
    }
}


template<>
DIM_ARROW_DIRECTION FromProtoEnum( types::DimensionArrowDirection aValue )
{
    switch( aValue )
    {
    case types::DimensionArrowDirection::DAD_INWARD:    return DIM_ARROW_DIRECTION::INWARD;
    case types::DimensionArrowDirection::DAD_UNKNOWN:
    case types::DimensionArrowDirection::DAD_OUTWARD:   return DIM_ARROW_DIRECTION::OUTWARD;

    default:
        wxCHECK_MSG( false,  DIM_ARROW_DIRECTION::OUTWARD,
                     "Unhandled case in FromProtoEnum<types::DimensionArrowDirection>" );
    }
}


template<>
types::DimensionPrecision ToProtoEnum( DIM_PRECISION aValue )
{
    switch( aValue )
    {
    case DIM_PRECISION::X:          return types::DimensionPrecision::DP_FIXED_0;
    case DIM_PRECISION::X_X:        return types::DimensionPrecision::DP_FIXED_1;
    case DIM_PRECISION::X_XX:       return types::DimensionPrecision::DP_FIXED_2;
    case DIM_PRECISION::X_XXX:      return types::DimensionPrecision::DP_FIXED_3;
    case DIM_PRECISION::X_XXXX:     return types::DimensionPrecision::DP_FIXED_4;
    case DIM_PRECISION::X_XXXXX:    return types::DimensionPrecision::DP_FIXED_5;
    case DIM_PRECISION::V_VV:       return types::DimensionPrecision::DP_SCALED_IN_2;
    case DIM_PRECISION::V_VVV:      return types::DimensionPrecision::DP_SCALED_IN_3;
    case DIM_PRECISION::V_VVVV:     return types::DimensionPrecision::DP_SCALED_IN_4;
    case DIM_PRECISION::V_VVVVV:    return types::DimensionPrecision::DP_SCALED_IN_5;

    default:
        wxCHECK_MSG( false, types::DimensionPrecision::DP_UNKNOWN,
                     "Unhandled case in ToProtoEnum<DIM_PRECISION>");
    }
}


template<>
DIM_PRECISION FromProtoEnum( types::DimensionPrecision aValue )
{
    switch( aValue )
    {
    case types::DimensionPrecision::DP_FIXED_0:     return DIM_PRECISION::X;
    case types::DimensionPrecision::DP_FIXED_1:     return DIM_PRECISION::X_X;
    case types::DimensionPrecision::DP_FIXED_2:     return DIM_PRECISION::X_XX;
    case types::DimensionPrecision::DP_FIXED_3:     return DIM_PRECISION::X_XXX;
    case types::DimensionPrecision::DP_FIXED_4:     return DIM_PRECISION::X_XXXX;
    case types::DimensionPrecision::DP_FIXED_5:     return DIM_PRECISION::X_XXXXX;
    case types::DimensionPrecision::DP_UNKNOWN:
    case types::DimensionPrecision::DP_SCALED_IN_2: return DIM_PRECISION::V_VV;
    case types::DimensionPrecision::DP_SCALED_IN_3: return DIM_PRECISION::V_VVV;
    case types::DimensionPrecision::DP_SCALED_IN_4: return DIM_PRECISION::V_VVVV;
    case types::DimensionPrecision::DP_SCALED_IN_5: return DIM_PRECISION::V_VVVVV;

    default:
        wxCHECK_MSG( false,  DIM_PRECISION::V_VV,
                     "Unhandled case in FromProtoEnum<types::DimensionPrecision>" );
    }
}


template<>
types::DimensionTextPosition ToProtoEnum( DIM_TEXT_POSITION aValue )
{
    switch( aValue )
    {
    case DIM_TEXT_POSITION::OUTSIDE:    return types::DimensionTextPosition::DTP_OUTSIDE;
    case DIM_TEXT_POSITION::INLINE:     return types::DimensionTextPosition::DTP_INLINE;
    case DIM_TEXT_POSITION::MANUAL:     return types::DimensionTextPosition::DTP_MANUAL;

    default:
        wxCHECK_MSG( false, types::DimensionTextPosition::DTP_UNKNOWN,
                     "Unhandled case in ToProtoEnum<DIM_TEXT_POSITION>");
    }
}


template<>
DIM_TEXT_POSITION FromProtoEnum( types::DimensionTextPosition aValue )
{
    switch( aValue )
    {
    case types::DimensionTextPosition::DTP_UNKNOWN:
    case types::DimensionTextPosition::DTP_OUTSIDE: return DIM_TEXT_POSITION::OUTSIDE;
    case types::DimensionTextPosition::DTP_INLINE:  return DIM_TEXT_POSITION::INLINE;
    case types::DimensionTextPosition::DTP_MANUAL:  return DIM_TEXT_POSITION::MANUAL;

    default:
        wxCHECK_MSG( false,  DIM_TEXT_POSITION::OUTSIDE,
                     "Unhandled case in FromProtoEnum<types::DimensionTextPosition>" );
    }
}


template<>
types::DimensionUnit ToProtoEnum( DIM_UNITS_MODE aValue )
{
    switch( aValue )
    {
    case DIM_UNITS_MODE::INCH:      return types::DimensionUnit::DU_INCHES;
    case DIM_UNITS_MODE::MILS:      return types::DimensionUnit::DU_MILS;
    case DIM_UNITS_MODE::MM:        return types::DimensionUnit::DU_MILLIMETERS;
    case DIM_UNITS_MODE::AUTOMATIC: return types::DimensionUnit::DU_AUTOMATIC;

    default:
        wxCHECK_MSG( false, types::DimensionUnit::DU_UNKNOWN,
                     "Unhandled case in ToProtoEnum<DIM_UNITS_MODE>");
    }
}


template<>
DIM_UNITS_MODE FromProtoEnum( types::DimensionUnit aValue )
{
    switch( aValue )
    {
    case types::DimensionUnit::DU_INCHES:       return DIM_UNITS_MODE::INCH;
    case types::DimensionUnit::DU_MILS:         return DIM_UNITS_MODE::MILS;
    case types::DimensionUnit::DU_MILLIMETERS:  return DIM_UNITS_MODE::MM;
    case types::DimensionUnit::DU_UNKNOWN:
    case types::DimensionUnit::DU_AUTOMATIC:    return DIM_UNITS_MODE::AUTOMATIC;

    default:
        wxCHECK_MSG( false,  DIM_UNITS_MODE::AUTOMATIC,
                     "Unhandled case in FromProtoEnum<types::DimensionUnit>" );
    }
}


template<>
commands::InactiveLayerDisplayMode ToProtoEnum( HIGH_CONTRAST_MODE aValue )
{
    switch( aValue )
    {
    case HIGH_CONTRAST_MODE::NORMAL:    return commands::InactiveLayerDisplayMode::ILDM_NORMAL;
    case HIGH_CONTRAST_MODE::DIMMED:    return commands::InactiveLayerDisplayMode::ILDM_DIMMED;
    case HIGH_CONTRAST_MODE::HIDDEN:    return commands::InactiveLayerDisplayMode::ILDM_HIDDEN;

    default:
        wxCHECK_MSG( false, commands::InactiveLayerDisplayMode::ILDM_NORMAL,
                     "Unhandled case in ToProtoEnum<HIGH_CONTRAST_MODE>");
    }
}


template<>
HIGH_CONTRAST_MODE FromProtoEnum( commands::InactiveLayerDisplayMode aValue )
{
    switch( aValue )
    {
    case commands::InactiveLayerDisplayMode::ILDM_DIMMED:   return HIGH_CONTRAST_MODE::DIMMED;
    case commands::InactiveLayerDisplayMode::ILDM_HIDDEN:   return HIGH_CONTRAST_MODE::HIDDEN;
    case commands::InactiveLayerDisplayMode::ILDM_UNKNOWN:
    case commands::InactiveLayerDisplayMode::ILDM_NORMAL:   return HIGH_CONTRAST_MODE::NORMAL;

    default:
        wxCHECK_MSG( false, HIGH_CONTRAST_MODE::NORMAL,
                     "Unhandled case in FromProtoEnum<commands::InactiveLayerDisplayMode>" );
    }
}


template<>
commands::NetColorDisplayMode ToProtoEnum( NET_COLOR_MODE aValue )
{
    switch( aValue )
    {
    case NET_COLOR_MODE::ALL:       return commands::NetColorDisplayMode::NCDM_ALL;
    case NET_COLOR_MODE::RATSNEST:  return commands::NetColorDisplayMode::NCDM_RATSNEST;
    case NET_COLOR_MODE::OFF:       return commands::NetColorDisplayMode::NCDM_OFF;

    default:
        wxCHECK_MSG( false, commands::NetColorDisplayMode::NCDM_UNKNOWN,
                     "Unhandled case in ToProtoEnum<NET_COLOR_MODE>");
    }
}


template<>
NET_COLOR_MODE FromProtoEnum( commands::NetColorDisplayMode aValue )
{
    switch( aValue )
    {
    case commands::NetColorDisplayMode::NCDM_ALL:       return NET_COLOR_MODE::ALL;
    case commands::NetColorDisplayMode::NCDM_OFF:       return NET_COLOR_MODE::OFF;
    case commands::NetColorDisplayMode::NCDM_UNKNOWN:
    case commands::NetColorDisplayMode::NCDM_RATSNEST:  return NET_COLOR_MODE::RATSNEST;

    default:
        wxCHECK_MSG( false, NET_COLOR_MODE::RATSNEST,
                     "Unhandled case in FromProtoEnum<commands::NetColorDisplayMode>" );
    }
}


template<>
commands::RatsnestDisplayMode ToProtoEnum( RATSNEST_MODE aValue )
{
    switch( aValue )
    {
    case RATSNEST_MODE::ALL:        return commands::RatsnestDisplayMode::RDM_ALL_LAYERS;
    case RATSNEST_MODE::VISIBLE:    return commands::RatsnestDisplayMode::RDM_VISIBLE_LAYERS;

    default:
        wxCHECK_MSG( false, commands::RatsnestDisplayMode::RDM_UNKNOWN,
                     "Unhandled case in ToProtoEnum<RATSNEST_MODE>");
    }
}


template<>
RATSNEST_MODE FromProtoEnum( commands::RatsnestDisplayMode aValue )
{
    switch( aValue )
    {
    case commands::RatsnestDisplayMode::RDM_VISIBLE_LAYERS: return RATSNEST_MODE::VISIBLE;
    case commands::RatsnestDisplayMode::RDM_UNKNOWN:
    case commands::RatsnestDisplayMode::RDM_ALL_LAYERS:     return RATSNEST_MODE::ALL;

    default:
        wxCHECK_MSG( false, RATSNEST_MODE::ALL,
                     "Unhandled case in FromProtoEnum<commands::RatsnestDisplayMode>" );
    }
}


template<>
BoardStackupLayerType ToProtoEnum( BOARD_STACKUP_ITEM_TYPE aValue )
{
    switch( aValue )
    {
    case BS_ITEM_TYPE_UNDEFINED:    return BoardStackupLayerType::BSLT_UNDEFINED;
    case BS_ITEM_TYPE_COPPER:       return BoardStackupLayerType::BSLT_COPPER;
    case BS_ITEM_TYPE_DIELECTRIC:   return BoardStackupLayerType::BSLT_DIELECTRIC;
    case BS_ITEM_TYPE_SOLDERPASTE:  return BoardStackupLayerType::BSLT_SOLDERPASTE;
    case BS_ITEM_TYPE_SOLDERMASK:   return BoardStackupLayerType::BSLT_SOLDERMASK;
    case BS_ITEM_TYPE_SILKSCREEN:   return BoardStackupLayerType::BSLT_SILKSCREEN;

    default:
        wxCHECK_MSG( false, BoardStackupLayerType::BSLT_UNKNOWN,
                     "Unhandled case in ToProtoEnum<BOARD_STACKUP_ITEM_TYPE>");
    }
}


template<>
BOARD_STACKUP_ITEM_TYPE FromProtoEnum( BoardStackupLayerType aValue )
{
    switch( aValue )
    {
    case BoardStackupLayerType::BSLT_UNDEFINED:    return BS_ITEM_TYPE_UNDEFINED;
    case BoardStackupLayerType::BSLT_COPPER:       return BS_ITEM_TYPE_COPPER;
    case BoardStackupLayerType::BSLT_DIELECTRIC:   return BS_ITEM_TYPE_DIELECTRIC;
    case BoardStackupLayerType::BSLT_SOLDERPASTE:  return BS_ITEM_TYPE_SOLDERPASTE;
    case BoardStackupLayerType::BSLT_SOLDERMASK:   return BS_ITEM_TYPE_SOLDERMASK;
    case BoardStackupLayerType::BSLT_SILKSCREEN:   return BS_ITEM_TYPE_SILKSCREEN;

    default:
        wxCHECK_MSG( false, BS_ITEM_TYPE_UNDEFINED,
                     "Unhandled case in FromProtoEnum<BoardStackupLayerType>" );
    }
}


template<>
DrcSeverity ToProtoEnum( SEVERITY aValue )
{
    switch( aValue )
    {
    case RPT_SEVERITY_WARNING:   return DrcSeverity::DRS_WARNING;
    case RPT_SEVERITY_ERROR:     return DrcSeverity::DRS_ERROR;
    case RPT_SEVERITY_EXCLUSION: return DrcSeverity::DRS_EXCLUSION;
    case RPT_SEVERITY_IGNORE:    return DrcSeverity::DRS_IGNORE;
    case RPT_SEVERITY_INFO:      return DrcSeverity::DRS_INFO;
    case RPT_SEVERITY_ACTION:    return DrcSeverity::DRS_ACTION;
    case RPT_SEVERITY_DEBUG:     return DrcSeverity::DRS_DEBUG;
    case RPT_SEVERITY_UNDEFINED: return DrcSeverity::DRS_UNDEFINED;
    default:
        wxCHECK_MSG( false, DrcSeverity::DRS_UNDEFINED,
                     "Unhandled case in ToProtoEnum<SEVERITY>");
    }
}


template<>
SEVERITY FromProtoEnum( DrcSeverity aValue )
{
    switch( aValue )
    {
    case DrcSeverity::DRS_WARNING:   return RPT_SEVERITY_WARNING;
    case DrcSeverity::DRS_ERROR:     return RPT_SEVERITY_ERROR;
    case DrcSeverity::DRS_EXCLUSION: return RPT_SEVERITY_EXCLUSION;
    case DrcSeverity::DRS_IGNORE:    return RPT_SEVERITY_IGNORE;
    case DrcSeverity::DRS_INFO:      return RPT_SEVERITY_INFO;
    case DrcSeverity::DRS_ACTION:    return RPT_SEVERITY_ACTION;
    case DrcSeverity::DRS_DEBUG:     return RPT_SEVERITY_DEBUG;
    case DrcSeverity::DRS_UNKNOWN:
    default:
        return RPT_SEVERITY_UNDEFINED;
    }
}


template<>
PlotDrillMarks ToProtoEnum( DRILL_MARKS aValue )
{
    switch( aValue )
    {
    case DRILL_MARKS::NO_DRILL_SHAPE:    return PlotDrillMarks::PDM_NONE;
    case DRILL_MARKS::SMALL_DRILL_SHAPE: return PlotDrillMarks::PDM_SMALL;
    case DRILL_MARKS::FULL_DRILL_SHAPE:  return PlotDrillMarks::PDM_FULL;
    default:
        wxCHECK_MSG( false, PlotDrillMarks::PDM_UNKNOWN,
                     "Unhandled case in ToProtoEnum<DRILL_MARKS>" );
    }
}


template<>
DRILL_MARKS FromProtoEnum( PlotDrillMarks aValue )
{
    switch( aValue )
    {
    case PlotDrillMarks::PDM_NONE:    return DRILL_MARKS::NO_DRILL_SHAPE;
    case PlotDrillMarks::PDM_SMALL:   return DRILL_MARKS::SMALL_DRILL_SHAPE;
    case PlotDrillMarks::PDM_FULL:    return DRILL_MARKS::FULL_DRILL_SHAPE;
    case PlotDrillMarks::PDM_UNKNOWN:
    default:
        return DRILL_MARKS::NO_DRILL_SHAPE;
    }
}


template<>
Board3DFormat ToProtoEnum( JOB_EXPORT_PCB_3D::FORMAT aValue )
{
    switch( aValue )
    {
    case JOB_EXPORT_PCB_3D::FORMAT::STEP:    return Board3DFormat::B3D_STEP;
    case JOB_EXPORT_PCB_3D::FORMAT::STEPZ:   return Board3DFormat::B3D_STEPZ;
    case JOB_EXPORT_PCB_3D::FORMAT::BREP:    return Board3DFormat::B3D_BREP;
    case JOB_EXPORT_PCB_3D::FORMAT::XAO:     return Board3DFormat::B3D_XAO;
    case JOB_EXPORT_PCB_3D::FORMAT::GLB:     return Board3DFormat::B3D_GLB;
    case JOB_EXPORT_PCB_3D::FORMAT::VRML:    return Board3DFormat::B3D_VRML;
    case JOB_EXPORT_PCB_3D::FORMAT::PLY:     return Board3DFormat::B3D_PLY;
    case JOB_EXPORT_PCB_3D::FORMAT::STL:     return Board3DFormat::B3D_STL;
    case JOB_EXPORT_PCB_3D::FORMAT::U3D:     return Board3DFormat::B3D_U3D;
    case JOB_EXPORT_PCB_3D::FORMAT::PDF:     return Board3DFormat::B3D_PDF;
    default:
        wxCHECK_MSG( false, Board3DFormat::B3D_UNKNOWN,
                     "Unhandled case in ToProtoEnum<JOB_EXPORT_PCB_3D::FORMAT>" );
    }
}


template<>
JOB_EXPORT_PCB_3D::FORMAT FromProtoEnum( Board3DFormat aValue )
{
    switch( aValue )
    {
    case Board3DFormat::B3D_STEP:  return JOB_EXPORT_PCB_3D::FORMAT::STEP;
    case Board3DFormat::B3D_STEPZ: return JOB_EXPORT_PCB_3D::FORMAT::STEPZ;
    case Board3DFormat::B3D_BREP:  return JOB_EXPORT_PCB_3D::FORMAT::BREP;
    case Board3DFormat::B3D_XAO:   return JOB_EXPORT_PCB_3D::FORMAT::XAO;
    case Board3DFormat::B3D_GLB:   return JOB_EXPORT_PCB_3D::FORMAT::GLB;
    case Board3DFormat::B3D_VRML:  return JOB_EXPORT_PCB_3D::FORMAT::VRML;
    case Board3DFormat::B3D_PLY:   return JOB_EXPORT_PCB_3D::FORMAT::PLY;
    case Board3DFormat::B3D_STL:   return JOB_EXPORT_PCB_3D::FORMAT::STL;
    case Board3DFormat::B3D_U3D:   return JOB_EXPORT_PCB_3D::FORMAT::U3D;
    case Board3DFormat::B3D_PDF:   return JOB_EXPORT_PCB_3D::FORMAT::PDF;
    case Board3DFormat::B3D_UNKNOWN:
    default:
        return JOB_EXPORT_PCB_3D::FORMAT::UNKNOWN;
    }
}


template<>
kiapi::common::types::Units ToProtoEnum( JOB_EXPORT_PCB_3D::VRML_UNITS aValue )
{
    switch( aValue )
    {
    case JOB_EXPORT_PCB_3D::VRML_UNITS::INCH:   return kiapi::common::types::Units::U_INCH;
    case JOB_EXPORT_PCB_3D::VRML_UNITS::MM:     return kiapi::common::types::Units::U_MM;
    case JOB_EXPORT_PCB_3D::VRML_UNITS::METERS: return kiapi::common::types::Units::U_METERS;
    case JOB_EXPORT_PCB_3D::VRML_UNITS::TENTHS: return kiapi::common::types::Units::U_TENTHS;
    default:
        wxCHECK_MSG( false, kiapi::common::types::Units::U_UNKNOWN,
                     "Unhandled case in ToProtoEnum<JOB_EXPORT_PCB_3D::VRML_UNITS>" );
    }
}


template<>
JOB_EXPORT_PCB_3D::VRML_UNITS FromProtoEnum( kiapi::common::types::Units aValue )
{
    switch( aValue )
    {
    case kiapi::common::types::Units::U_INCH:   return JOB_EXPORT_PCB_3D::VRML_UNITS::INCH;
    case kiapi::common::types::Units::U_MM:     return JOB_EXPORT_PCB_3D::VRML_UNITS::MM;
    case kiapi::common::types::Units::U_METERS: return JOB_EXPORT_PCB_3D::VRML_UNITS::METERS;
    case kiapi::common::types::Units::U_TENTHS: return JOB_EXPORT_PCB_3D::VRML_UNITS::TENTHS;
    case kiapi::common::types::Units::U_UNKNOWN:
    default:
        return JOB_EXPORT_PCB_3D::VRML_UNITS::METERS;
    }
}


template<>
RenderFormat ToProtoEnum( JOB_PCB_RENDER::FORMAT aValue )
{
    switch( aValue )
    {
    case JOB_PCB_RENDER::FORMAT::PNG:  return RenderFormat::RF_PNG;
    case JOB_PCB_RENDER::FORMAT::JPEG: return RenderFormat::RF_JPEG;
    default:
        wxCHECK_MSG( false, RenderFormat::RF_UNKNOWN,
                     "Unhandled case in ToProtoEnum<JOB_PCB_RENDER::FORMAT>" );
    }
}


template<>
JOB_PCB_RENDER::FORMAT FromProtoEnum( RenderFormat aValue )
{
    switch( aValue )
    {
    case RenderFormat::RF_PNG: return JOB_PCB_RENDER::FORMAT::PNG;
    case RenderFormat::RF_JPEG:return JOB_PCB_RENDER::FORMAT::JPEG;
    case RenderFormat::RF_UNKNOWN:
    default:
        return JOB_PCB_RENDER::FORMAT::PNG;
    }
}


template<>
RenderQuality ToProtoEnum( JOB_PCB_RENDER::QUALITY aValue )
{
    switch( aValue )
    {
    case JOB_PCB_RENDER::QUALITY::BASIC:        return RenderQuality::RQ_BASIC;
    case JOB_PCB_RENDER::QUALITY::HIGH:         return RenderQuality::RQ_HIGH;
    case JOB_PCB_RENDER::QUALITY::USER:         return RenderQuality::RQ_USER;
    case JOB_PCB_RENDER::QUALITY::JOB_SETTINGS: return RenderQuality::RQ_JOB_SETTINGS;
    default:
        wxCHECK_MSG( false, RenderQuality::RQ_UNKNOWN,
                     "Unhandled case in ToProtoEnum<JOB_PCB_RENDER::QUALITY>" );
    }
}


template<>
JOB_PCB_RENDER::QUALITY FromProtoEnum( RenderQuality aValue )
{
    switch( aValue )
    {
    case RenderQuality::RQ_BASIC:        return JOB_PCB_RENDER::QUALITY::BASIC;
    case RenderQuality::RQ_HIGH:         return JOB_PCB_RENDER::QUALITY::HIGH;
    case RenderQuality::RQ_USER:         return JOB_PCB_RENDER::QUALITY::USER;
    case RenderQuality::RQ_JOB_SETTINGS: return JOB_PCB_RENDER::QUALITY::JOB_SETTINGS;
    case RenderQuality::RQ_UNKNOWN:
    default:
        return JOB_PCB_RENDER::QUALITY::BASIC;
    }
}


template<>
RenderBackgroundStyle ToProtoEnum( JOB_PCB_RENDER::BG_STYLE aValue )
{
    switch( aValue )
    {
    case JOB_PCB_RENDER::BG_STYLE::DEFAULT:     return RenderBackgroundStyle::RBS_DEFAULT;
    case JOB_PCB_RENDER::BG_STYLE::TRANSPARENT: return RenderBackgroundStyle::RBS_TRANSPARENT;
    case JOB_PCB_RENDER::BG_STYLE::OPAQUE:      return RenderBackgroundStyle::RBS_OPAQUE;
    default:
        wxCHECK_MSG( false, RenderBackgroundStyle::RBS_UNKNOWN,
                     "Unhandled case in ToProtoEnum<JOB_PCB_RENDER::BG_STYLE>" );
    }
}


template<>
JOB_PCB_RENDER::BG_STYLE FromProtoEnum( RenderBackgroundStyle aValue )
{
    switch( aValue )
    {
    case RenderBackgroundStyle::RBS_DEFAULT:     return JOB_PCB_RENDER::BG_STYLE::DEFAULT;
    case RenderBackgroundStyle::RBS_TRANSPARENT: return JOB_PCB_RENDER::BG_STYLE::TRANSPARENT;
    case RenderBackgroundStyle::RBS_OPAQUE:      return JOB_PCB_RENDER::BG_STYLE::OPAQUE;
    case RenderBackgroundStyle::RBS_UNKNOWN:
    default:
        return JOB_PCB_RENDER::BG_STYLE::DEFAULT;
    }
}


template<>
RenderSide ToProtoEnum( JOB_PCB_RENDER::SIDE aValue )
{
    switch( aValue )
    {
    case JOB_PCB_RENDER::SIDE::TOP:    return RenderSide::RS_TOP;
    case JOB_PCB_RENDER::SIDE::BOTTOM: return RenderSide::RS_BOTTOM;
    case JOB_PCB_RENDER::SIDE::LEFT:   return RenderSide::RS_LEFT;
    case JOB_PCB_RENDER::SIDE::RIGHT:  return RenderSide::RS_RIGHT;
    case JOB_PCB_RENDER::SIDE::FRONT:  return RenderSide::RS_FRONT;
    case JOB_PCB_RENDER::SIDE::BACK:   return RenderSide::RS_BACK;
    default:
        wxCHECK_MSG( false, RenderSide::RS_UNKNOWN,
                     "Unhandled case in ToProtoEnum<JOB_PCB_RENDER::SIDE>" );
    }
}


template<>
JOB_PCB_RENDER::SIDE FromProtoEnum( RenderSide aValue )
{
    switch( aValue )
    {
    case RenderSide::RS_TOP:    return JOB_PCB_RENDER::SIDE::TOP;
    case RenderSide::RS_BOTTOM: return JOB_PCB_RENDER::SIDE::BOTTOM;
    case RenderSide::RS_LEFT:   return JOB_PCB_RENDER::SIDE::LEFT;
    case RenderSide::RS_RIGHT:  return JOB_PCB_RENDER::SIDE::RIGHT;
    case RenderSide::RS_FRONT:  return JOB_PCB_RENDER::SIDE::FRONT;
    case RenderSide::RS_BACK:   return JOB_PCB_RENDER::SIDE::BACK;
    case RenderSide::RS_UNKNOWN:
    default:
        return JOB_PCB_RENDER::SIDE::TOP;
    }
}


template<>
BoardJobPaginationMode ToProtoEnum( JOB_EXPORT_PCB_SVG::GEN_MODE aValue )
{
    switch( aValue )
    {
    case JOB_EXPORT_PCB_SVG::GEN_MODE::SINGLE: return BoardJobPaginationMode::BJPM_ALL_LAYERS_ONE_PAGE;
    case JOB_EXPORT_PCB_SVG::GEN_MODE::MULTI:  return BoardJobPaginationMode::BJPM_EACH_LAYER_OWN_FILE;
    default:
        wxCHECK_MSG( false, BoardJobPaginationMode::BJPM_UNKNOWN,
                     "Unhandled case in ToProtoEnum<JOB_EXPORT_PCB_SVG::GEN_MODE>" );
    }
}


template<>
JOB_EXPORT_PCB_SVG::GEN_MODE FromProtoEnum( BoardJobPaginationMode aValue )
{
    switch( aValue )
    {
    case BoardJobPaginationMode::BJPM_ALL_LAYERS_ONE_PAGE:
        return JOB_EXPORT_PCB_SVG::GEN_MODE::SINGLE;
    case BoardJobPaginationMode::BJPM_EACH_LAYER_OWN_FILE:
        return JOB_EXPORT_PCB_SVG::GEN_MODE::MULTI;
    case BoardJobPaginationMode::BJPM_UNKNOWN:
    case BoardJobPaginationMode::BJPM_EACH_LAYER_OWN_PAGE:
    default:
        return JOB_EXPORT_PCB_SVG::GEN_MODE::SINGLE;
    }
}


template<>
BoardJobPaginationMode ToProtoEnum( JOB_EXPORT_PCB_DXF::GEN_MODE aValue )
{
    switch( aValue )
    {
    case JOB_EXPORT_PCB_DXF::GEN_MODE::SINGLE: return BoardJobPaginationMode::BJPM_ALL_LAYERS_ONE_PAGE;
    case JOB_EXPORT_PCB_DXF::GEN_MODE::MULTI:  return BoardJobPaginationMode::BJPM_EACH_LAYER_OWN_FILE;
    default:
        wxCHECK_MSG( false, BoardJobPaginationMode::BJPM_UNKNOWN,
                     "Unhandled case in ToProtoEnum<JOB_EXPORT_PCB_DXF::GEN_MODE>" );
    }
}


template<>
JOB_EXPORT_PCB_DXF::GEN_MODE FromProtoEnum( BoardJobPaginationMode aValue )
{
    switch( aValue )
    {
    case BoardJobPaginationMode::BJPM_ALL_LAYERS_ONE_PAGE:
        return JOB_EXPORT_PCB_DXF::GEN_MODE::SINGLE;
    case BoardJobPaginationMode::BJPM_EACH_LAYER_OWN_FILE:
        return JOB_EXPORT_PCB_DXF::GEN_MODE::MULTI;
    case BoardJobPaginationMode::BJPM_UNKNOWN:
    case BoardJobPaginationMode::BJPM_EACH_LAYER_OWN_PAGE:
    default:
        return JOB_EXPORT_PCB_DXF::GEN_MODE::SINGLE;
    }
}


template<>
BoardJobPaginationMode ToProtoEnum( JOB_EXPORT_PCB_PDF::GEN_MODE aValue )
{
    switch( aValue )
    {
    case JOB_EXPORT_PCB_PDF::GEN_MODE::ALL_LAYERS_ONE_FILE:
        return BoardJobPaginationMode::BJPM_ALL_LAYERS_ONE_PAGE;
    case JOB_EXPORT_PCB_PDF::GEN_MODE::ONE_PAGE_PER_LAYER_ONE_FILE:
        return BoardJobPaginationMode::BJPM_EACH_LAYER_OWN_PAGE;
    case JOB_EXPORT_PCB_PDF::GEN_MODE::ALL_LAYERS_SEPARATE_FILE:
        return BoardJobPaginationMode::BJPM_EACH_LAYER_OWN_FILE;
    default:
        wxCHECK_MSG( false, BoardJobPaginationMode::BJPM_UNKNOWN,
                     "Unhandled case in ToProtoEnum<JOB_EXPORT_PCB_PDF::GEN_MODE>" );
    }
}


template<>
JOB_EXPORT_PCB_PDF::GEN_MODE FromProtoEnum( BoardJobPaginationMode aValue )
{
    switch( aValue )
    {
    case BoardJobPaginationMode::BJPM_ALL_LAYERS_ONE_PAGE:
        return JOB_EXPORT_PCB_PDF::GEN_MODE::ALL_LAYERS_ONE_FILE;
    case BoardJobPaginationMode::BJPM_EACH_LAYER_OWN_PAGE:
        return JOB_EXPORT_PCB_PDF::GEN_MODE::ONE_PAGE_PER_LAYER_ONE_FILE;
    case BoardJobPaginationMode::BJPM_EACH_LAYER_OWN_FILE:
        return JOB_EXPORT_PCB_PDF::GEN_MODE::ALL_LAYERS_SEPARATE_FILE;
    case BoardJobPaginationMode::BJPM_UNKNOWN:
    default:
        return JOB_EXPORT_PCB_PDF::GEN_MODE::ALL_LAYERS_ONE_FILE;
    }
}


template<>
BoardJobPaginationMode ToProtoEnum( JOB_EXPORT_PCB_PS::GEN_MODE aValue )
{
    switch( aValue )
    {
    case JOB_EXPORT_PCB_PS::GEN_MODE::SINGLE: return BoardJobPaginationMode::BJPM_ALL_LAYERS_ONE_PAGE;
    case JOB_EXPORT_PCB_PS::GEN_MODE::MULTI:  return BoardJobPaginationMode::BJPM_EACH_LAYER_OWN_FILE;
    default:
        wxCHECK_MSG( false, BoardJobPaginationMode::BJPM_UNKNOWN,
                     "Unhandled case in ToProtoEnum<JOB_EXPORT_PCB_PS::GEN_MODE>" );
    }
}


template<>
JOB_EXPORT_PCB_PS::GEN_MODE FromProtoEnum( BoardJobPaginationMode aValue )
{
    switch( aValue )
    {
    case BoardJobPaginationMode::BJPM_ALL_LAYERS_ONE_PAGE:
        return JOB_EXPORT_PCB_PS::GEN_MODE::SINGLE;
    case BoardJobPaginationMode::BJPM_EACH_LAYER_OWN_FILE:
        return JOB_EXPORT_PCB_PS::GEN_MODE::MULTI;
    case BoardJobPaginationMode::BJPM_UNKNOWN:
    case BoardJobPaginationMode::BJPM_EACH_LAYER_OWN_PAGE:
    default:
        return JOB_EXPORT_PCB_PS::GEN_MODE::SINGLE;
    }
}


template<>
DrillFormat ToProtoEnum( JOB_EXPORT_PCB_DRILL::DRILL_FORMAT aValue )
{
    switch( aValue )
    {
    case JOB_EXPORT_PCB_DRILL::DRILL_FORMAT::EXCELLON: return DrillFormat::DF_EXCELLON;
    case JOB_EXPORT_PCB_DRILL::DRILL_FORMAT::GERBER:   return DrillFormat::DF_GERBER;
    default:
        wxCHECK_MSG( false, DrillFormat::DF_UNKNOWN,
                     "Unhandled case in ToProtoEnum<JOB_EXPORT_PCB_DRILL::DRILL_FORMAT>" );
    }
}


template<>
JOB_EXPORT_PCB_DRILL::DRILL_FORMAT FromProtoEnum( DrillFormat aValue )
{
    switch( aValue )
    {
    case DrillFormat::DF_EXCELLON: return JOB_EXPORT_PCB_DRILL::DRILL_FORMAT::EXCELLON;
    case DrillFormat::DF_GERBER:   return JOB_EXPORT_PCB_DRILL::DRILL_FORMAT::GERBER;
    case DrillFormat::DF_UNKNOWN:
    default:
        return JOB_EXPORT_PCB_DRILL::DRILL_FORMAT::EXCELLON;
    }
}


template<>
DrillOrigin ToProtoEnum( JOB_EXPORT_PCB_DRILL::DRILL_ORIGIN aValue )
{
    switch( aValue )
    {
    case JOB_EXPORT_PCB_DRILL::DRILL_ORIGIN::ABS:  return DrillOrigin::DO_ABSOLUTE;
    case JOB_EXPORT_PCB_DRILL::DRILL_ORIGIN::PLOT: return DrillOrigin::DO_PLOT;
    default:
        wxCHECK_MSG( false, DrillOrigin::DO_UNKNOWN,
                     "Unhandled case in ToProtoEnum<JOB_EXPORT_PCB_DRILL::DRILL_ORIGIN>" );
    }
}


template<>
JOB_EXPORT_PCB_DRILL::DRILL_ORIGIN FromProtoEnum( DrillOrigin aValue )
{
    switch( aValue )
    {
    case DrillOrigin::DO_ABSOLUTE: return JOB_EXPORT_PCB_DRILL::DRILL_ORIGIN::ABS;
    case DrillOrigin::DO_PLOT:     return JOB_EXPORT_PCB_DRILL::DRILL_ORIGIN::PLOT;
    case DrillOrigin::DO_UNKNOWN:
    default:
        return JOB_EXPORT_PCB_DRILL::DRILL_ORIGIN::ABS;
    }
}


template<>
DrillZerosFormat ToProtoEnum( JOB_EXPORT_PCB_DRILL::ZEROS_FORMAT aValue )
{
    switch( aValue )
    {
    case JOB_EXPORT_PCB_DRILL::ZEROS_FORMAT::DECIMAL:
        return DrillZerosFormat::DZF_DECIMAL;
    case JOB_EXPORT_PCB_DRILL::ZEROS_FORMAT::SUPPRESS_LEADING:
        return DrillZerosFormat::DZF_SUPPRESS_LEADING;
    case JOB_EXPORT_PCB_DRILL::ZEROS_FORMAT::SUPPRESS_TRAILING:
        return DrillZerosFormat::DZF_SUPPRESS_TRAILING;
    case JOB_EXPORT_PCB_DRILL::ZEROS_FORMAT::KEEP_ZEROS:
        return DrillZerosFormat::DZF_KEEP_ZEROS;
    default:
        wxCHECK_MSG( false, DrillZerosFormat::DZF_UNKNOWN,
                     "Unhandled case in ToProtoEnum<JOB_EXPORT_PCB_DRILL::ZEROS_FORMAT>" );
    }
}


template<>
JOB_EXPORT_PCB_DRILL::ZEROS_FORMAT FromProtoEnum( DrillZerosFormat aValue )
{
    switch( aValue )
    {
    case DrillZerosFormat::DZF_DECIMAL:
        return JOB_EXPORT_PCB_DRILL::ZEROS_FORMAT::DECIMAL;
    case DrillZerosFormat::DZF_SUPPRESS_LEADING:
        return JOB_EXPORT_PCB_DRILL::ZEROS_FORMAT::SUPPRESS_LEADING;
    case DrillZerosFormat::DZF_SUPPRESS_TRAILING:
        return JOB_EXPORT_PCB_DRILL::ZEROS_FORMAT::SUPPRESS_TRAILING;
    case DrillZerosFormat::DZF_KEEP_ZEROS:
        return JOB_EXPORT_PCB_DRILL::ZEROS_FORMAT::KEEP_ZEROS;
    case DrillZerosFormat::DZF_UNKNOWN:
    default:
        return JOB_EXPORT_PCB_DRILL::ZEROS_FORMAT::DECIMAL;
    }
}


template<>
DrillMapFormat ToProtoEnum( JOB_EXPORT_PCB_DRILL::MAP_FORMAT aValue )
{
    switch( aValue )
    {
    case JOB_EXPORT_PCB_DRILL::MAP_FORMAT::POSTSCRIPT:
        return DrillMapFormat::DMF_POSTSCRIPT;
    case JOB_EXPORT_PCB_DRILL::MAP_FORMAT::GERBER_X2:
        return DrillMapFormat::DMF_GERBER_X2;
    case JOB_EXPORT_PCB_DRILL::MAP_FORMAT::DXF:
        return DrillMapFormat::DMF_DXF;
    case JOB_EXPORT_PCB_DRILL::MAP_FORMAT::SVG:
        return DrillMapFormat::DMF_SVG;
    case JOB_EXPORT_PCB_DRILL::MAP_FORMAT::PDF:
        return DrillMapFormat::DMF_PDF;
    default:
        wxCHECK_MSG( false, DrillMapFormat::DMF_UNKNOWN,
                     "Unhandled case in ToProtoEnum<JOB_EXPORT_PCB_DRILL::MAP_FORMAT>" );
    }
}


template<>
JOB_EXPORT_PCB_DRILL::MAP_FORMAT FromProtoEnum( DrillMapFormat aValue )
{
    switch( aValue )
    {
    case DrillMapFormat::DMF_POSTSCRIPT:
        return JOB_EXPORT_PCB_DRILL::MAP_FORMAT::POSTSCRIPT;
    case DrillMapFormat::DMF_GERBER_X2:
        return JOB_EXPORT_PCB_DRILL::MAP_FORMAT::GERBER_X2;
    case DrillMapFormat::DMF_DXF:
        return JOB_EXPORT_PCB_DRILL::MAP_FORMAT::DXF;
    case DrillMapFormat::DMF_SVG:
        return JOB_EXPORT_PCB_DRILL::MAP_FORMAT::SVG;
    case DrillMapFormat::DMF_PDF:
        return JOB_EXPORT_PCB_DRILL::MAP_FORMAT::PDF;
    case DrillMapFormat::DMF_UNKNOWN:
    default:
        return JOB_EXPORT_PCB_DRILL::MAP_FORMAT::PDF;
    }
}


template<>
kiapi::common::types::Units ToProtoEnum( JOB_EXPORT_PCB_DRILL::DRILL_UNITS aValue )
{
    switch( aValue )
    {
    case JOB_EXPORT_PCB_DRILL::DRILL_UNITS::INCH: return kiapi::common::types::Units::U_INCH;
    case JOB_EXPORT_PCB_DRILL::DRILL_UNITS::MM:   return kiapi::common::types::Units::U_MM;
    default:
        wxCHECK_MSG( false, kiapi::common::types::Units::U_UNKNOWN,
                     "Unhandled case in ToProtoEnum<JOB_EXPORT_PCB_DRILL::DRILL_UNITS>" );
    }
}


template<>
JOB_EXPORT_PCB_DRILL::DRILL_UNITS FromProtoEnum( kiapi::common::types::Units aValue )
{
    switch( aValue )
    {
    case kiapi::common::types::Units::U_INCH: return JOB_EXPORT_PCB_DRILL::DRILL_UNITS::INCH;
    case kiapi::common::types::Units::U_MM:   return JOB_EXPORT_PCB_DRILL::DRILL_UNITS::MM;
    case kiapi::common::types::Units::U_UNKNOWN:
    case kiapi::common::types::Units::U_METERS:
    case kiapi::common::types::Units::U_TENTHS:
    default:
        return JOB_EXPORT_PCB_DRILL::DRILL_UNITS::INCH;
    }
}


template<>
PositionSide ToProtoEnum( JOB_EXPORT_PCB_POS::SIDE aValue )
{
    switch( aValue )
    {
    case JOB_EXPORT_PCB_POS::SIDE::FRONT: return PositionSide::PS_FRONT;
    case JOB_EXPORT_PCB_POS::SIDE::BACK:  return PositionSide::PS_BACK;
    case JOB_EXPORT_PCB_POS::SIDE::BOTH:  return PositionSide::PS_BOTH;
    default:
        wxCHECK_MSG( false, PositionSide::PS_UNKNOWN,
                     "Unhandled case in ToProtoEnum<JOB_EXPORT_PCB_POS::SIDE>" );
    }
}


template<>
JOB_EXPORT_PCB_POS::SIDE FromProtoEnum( PositionSide aValue )
{
    switch( aValue )
    {
    case PositionSide::PS_FRONT: return JOB_EXPORT_PCB_POS::SIDE::FRONT;
    case PositionSide::PS_BACK:  return JOB_EXPORT_PCB_POS::SIDE::BACK;
    case PositionSide::PS_BOTH:  return JOB_EXPORT_PCB_POS::SIDE::BOTH;
    case PositionSide::PS_UNKNOWN:
    default:
        return JOB_EXPORT_PCB_POS::SIDE::BOTH;
    }
}


template<>
PositionFormat ToProtoEnum( JOB_EXPORT_PCB_POS::FORMAT aValue )
{
    switch( aValue )
    {
    case JOB_EXPORT_PCB_POS::FORMAT::ASCII:  return PositionFormat::PF_ASCII;
    case JOB_EXPORT_PCB_POS::FORMAT::CSV:    return PositionFormat::PF_CSV;
    case JOB_EXPORT_PCB_POS::FORMAT::GERBER: return PositionFormat::PF_GERBER;
    default:
        wxCHECK_MSG( false, PositionFormat::PF_UNKNOWN,
                     "Unhandled case in ToProtoEnum<JOB_EXPORT_PCB_POS::FORMAT>" );
    }
}


template<>
JOB_EXPORT_PCB_POS::FORMAT FromProtoEnum( PositionFormat aValue )
{
    switch( aValue )
    {
    case PositionFormat::PF_ASCII:  return JOB_EXPORT_PCB_POS::FORMAT::ASCII;
    case PositionFormat::PF_CSV:    return JOB_EXPORT_PCB_POS::FORMAT::CSV;
    case PositionFormat::PF_GERBER: return JOB_EXPORT_PCB_POS::FORMAT::GERBER;
    case PositionFormat::PF_UNKNOWN:
    default:
        return JOB_EXPORT_PCB_POS::FORMAT::ASCII;
    }
}


template<>
Ipc2581Version ToProtoEnum( JOB_EXPORT_PCB_IPC2581::IPC2581_VERSION aValue )
{
    switch( aValue )
    {
    case JOB_EXPORT_PCB_IPC2581::IPC2581_VERSION::B: return Ipc2581Version::IPC2581V_B;
    case JOB_EXPORT_PCB_IPC2581::IPC2581_VERSION::C: return Ipc2581Version::IPC2581V_C;
    default:
        wxCHECK_MSG( false, Ipc2581Version::IPC2581V_UNKNOWN,
                     "Unhandled case in ToProtoEnum<JOB_EXPORT_PCB_IPC2581::IPC2581_VERSION>" );
    }
}


template<>
JOB_EXPORT_PCB_IPC2581::IPC2581_VERSION FromProtoEnum( Ipc2581Version aValue )
{
    switch( aValue )
    {
    case Ipc2581Version::IPC2581V_B: return JOB_EXPORT_PCB_IPC2581::IPC2581_VERSION::B;
    case Ipc2581Version::IPC2581V_C: return JOB_EXPORT_PCB_IPC2581::IPC2581_VERSION::C;
    case Ipc2581Version::IPC2581V_UNKNOWN:
    default:
        return JOB_EXPORT_PCB_IPC2581::IPC2581_VERSION::C;
    }
}


template<>
OdbCompression ToProtoEnum( JOB_EXPORT_PCB_ODB::ODB_COMPRESSION aValue )
{
    switch( aValue )
    {
    case JOB_EXPORT_PCB_ODB::ODB_COMPRESSION::NONE: return OdbCompression::ODBC_NONE;
    case JOB_EXPORT_PCB_ODB::ODB_COMPRESSION::ZIP:  return OdbCompression::ODBC_ZIP;
    case JOB_EXPORT_PCB_ODB::ODB_COMPRESSION::TGZ:  return OdbCompression::ODBC_TGZ;
    default:
        wxCHECK_MSG( false, OdbCompression::ODBC_UNKNOWN,
                     "Unhandled case in ToProtoEnum<JOB_EXPORT_PCB_ODB::ODB_COMPRESSION>" );
    }
}


template<>
JOB_EXPORT_PCB_ODB::ODB_COMPRESSION FromProtoEnum( OdbCompression aValue )
{
    switch( aValue )
    {
    case OdbCompression::ODBC_NONE: return JOB_EXPORT_PCB_ODB::ODB_COMPRESSION::NONE;
    case OdbCompression::ODBC_ZIP:  return JOB_EXPORT_PCB_ODB::ODB_COMPRESSION::ZIP;
    case OdbCompression::ODBC_TGZ:  return JOB_EXPORT_PCB_ODB::ODB_COMPRESSION::TGZ;
    case OdbCompression::ODBC_UNKNOWN:
    default:
        return JOB_EXPORT_PCB_ODB::ODB_COMPRESSION::ZIP;
    }
}


template<>
StatsOutputFormat ToProtoEnum( JOB_EXPORT_PCB_STATS::OUTPUT_FORMAT aValue )
{
    switch( aValue )
    {
    case JOB_EXPORT_PCB_STATS::OUTPUT_FORMAT::REPORT: return StatsOutputFormat::SOF_REPORT;
    case JOB_EXPORT_PCB_STATS::OUTPUT_FORMAT::JSON:   return StatsOutputFormat::SOF_JSON;
    default:
        wxCHECK_MSG( false, StatsOutputFormat::SOF_UNKNOWN,
                     "Unhandled case in ToProtoEnum<JOB_EXPORT_PCB_STATS::OUTPUT_FORMAT>" );
    }
}


template<>
JOB_EXPORT_PCB_STATS::OUTPUT_FORMAT FromProtoEnum( StatsOutputFormat aValue )
{
    switch( aValue )
    {
    case StatsOutputFormat::SOF_REPORT: return JOB_EXPORT_PCB_STATS::OUTPUT_FORMAT::REPORT;
    case StatsOutputFormat::SOF_JSON:   return JOB_EXPORT_PCB_STATS::OUTPUT_FORMAT::JSON;
    case StatsOutputFormat::SOF_UNKNOWN:
    default:
        return JOB_EXPORT_PCB_STATS::OUTPUT_FORMAT::REPORT;
    }
}


template<>
kiapi::common::types::Units ToProtoEnum( JOB_EXPORT_PCB_DXF::DXF_UNITS aValue )
{
    switch( aValue )
    {
    case JOB_EXPORT_PCB_DXF::DXF_UNITS::INCH: return kiapi::common::types::Units::U_INCH;
    case JOB_EXPORT_PCB_DXF::DXF_UNITS::MM:   return kiapi::common::types::Units::U_MM;
    default:
        wxCHECK_MSG( false, kiapi::common::types::Units::U_UNKNOWN,
                     "Unhandled case in ToProtoEnum<JOB_EXPORT_PCB_DXF::DXF_UNITS>" );
    }
}


template<>
JOB_EXPORT_PCB_DXF::DXF_UNITS FromProtoEnum( kiapi::common::types::Units aValue )
{
    switch( aValue )
    {
    case kiapi::common::types::Units::U_INCH: return JOB_EXPORT_PCB_DXF::DXF_UNITS::INCH;
    case kiapi::common::types::Units::U_MM:   return JOB_EXPORT_PCB_DXF::DXF_UNITS::MM;
    case kiapi::common::types::Units::U_UNKNOWN:
    case kiapi::common::types::Units::U_METERS:
    case kiapi::common::types::Units::U_TENTHS:
    default:
        return JOB_EXPORT_PCB_DXF::DXF_UNITS::INCH;
    }
}


template<>
kiapi::common::types::Units ToProtoEnum( JOB_EXPORT_PCB_POS::UNITS aValue )
{
    switch( aValue )
    {
    case JOB_EXPORT_PCB_POS::UNITS::INCH: return kiapi::common::types::Units::U_INCH;
    case JOB_EXPORT_PCB_POS::UNITS::MM:   return kiapi::common::types::Units::U_MM;
    default:
        wxCHECK_MSG( false, kiapi::common::types::Units::U_UNKNOWN,
                     "Unhandled case in ToProtoEnum<JOB_EXPORT_PCB_POS::UNITS>" );
    }
}


template<>
JOB_EXPORT_PCB_POS::UNITS FromProtoEnum( kiapi::common::types::Units aValue )
{
    switch( aValue )
    {
    case kiapi::common::types::Units::U_INCH: return JOB_EXPORT_PCB_POS::UNITS::INCH;
    case kiapi::common::types::Units::U_MM:   return JOB_EXPORT_PCB_POS::UNITS::MM;
    case kiapi::common::types::Units::U_UNKNOWN:
    case kiapi::common::types::Units::U_METERS:
    case kiapi::common::types::Units::U_TENTHS:
    default:
        return JOB_EXPORT_PCB_POS::UNITS::MM;
    }
}


template<>
kiapi::common::types::Units ToProtoEnum( JOB_EXPORT_PCB_IPC2581::IPC2581_UNITS aValue )
{
    switch( aValue )
    {
    case JOB_EXPORT_PCB_IPC2581::IPC2581_UNITS::INCH: return kiapi::common::types::Units::U_INCH;
    case JOB_EXPORT_PCB_IPC2581::IPC2581_UNITS::MM:   return kiapi::common::types::Units::U_MM;
    default:
        wxCHECK_MSG( false, kiapi::common::types::Units::U_UNKNOWN,
                     "Unhandled case in ToProtoEnum<JOB_EXPORT_PCB_IPC2581::IPC2581_UNITS>" );
    }
}


template<>
JOB_EXPORT_PCB_IPC2581::IPC2581_UNITS FromProtoEnum( kiapi::common::types::Units aValue )
{
    switch( aValue )
    {
    case kiapi::common::types::Units::U_INCH: return JOB_EXPORT_PCB_IPC2581::IPC2581_UNITS::INCH;
    case kiapi::common::types::Units::U_MM:   return JOB_EXPORT_PCB_IPC2581::IPC2581_UNITS::MM;
    case kiapi::common::types::Units::U_UNKNOWN:
    case kiapi::common::types::Units::U_METERS:
    case kiapi::common::types::Units::U_TENTHS:
    default:
        return JOB_EXPORT_PCB_IPC2581::IPC2581_UNITS::MM;
    }
}


template<>
kiapi::common::types::Units ToProtoEnum( JOB_EXPORT_PCB_ODB::ODB_UNITS aValue )
{
    switch( aValue )
    {
    case JOB_EXPORT_PCB_ODB::ODB_UNITS::INCH: return kiapi::common::types::Units::U_INCH;
    case JOB_EXPORT_PCB_ODB::ODB_UNITS::MM:   return kiapi::common::types::Units::U_MM;
    default:
        wxCHECK_MSG( false, kiapi::common::types::Units::U_UNKNOWN,
                     "Unhandled case in ToProtoEnum<JOB_EXPORT_PCB_ODB::ODB_UNITS>" );
    }
}


template<>
JOB_EXPORT_PCB_ODB::ODB_UNITS FromProtoEnum( kiapi::common::types::Units aValue )
{
    switch( aValue )
    {
    case kiapi::common::types::Units::U_INCH: return JOB_EXPORT_PCB_ODB::ODB_UNITS::INCH;
    case kiapi::common::types::Units::U_MM:   return JOB_EXPORT_PCB_ODB::ODB_UNITS::MM;
    case kiapi::common::types::Units::U_UNKNOWN:
    case kiapi::common::types::Units::U_METERS:
    case kiapi::common::types::Units::U_TENTHS:
    default:
        return JOB_EXPORT_PCB_ODB::ODB_UNITS::MM;
    }
}


template<>
kiapi::common::types::Units ToProtoEnum( JOB_EXPORT_PCB_STATS::UNITS aValue )
{
    switch( aValue )
    {
    case JOB_EXPORT_PCB_STATS::UNITS::INCH: return kiapi::common::types::Units::U_INCH;
    case JOB_EXPORT_PCB_STATS::UNITS::MM:   return kiapi::common::types::Units::U_MM;
    default:
        wxCHECK_MSG( false, kiapi::common::types::Units::U_UNKNOWN,
                     "Unhandled case in ToProtoEnum<JOB_EXPORT_PCB_STATS::UNITS>" );
    }
}


template<>
JOB_EXPORT_PCB_STATS::UNITS FromProtoEnum( kiapi::common::types::Units aValue )
{
    switch( aValue )
    {
    case kiapi::common::types::Units::U_INCH: return JOB_EXPORT_PCB_STATS::UNITS::INCH;
    case kiapi::common::types::Units::U_MM:   return JOB_EXPORT_PCB_STATS::UNITS::MM;
    case kiapi::common::types::Units::U_UNKNOWN:
    case kiapi::common::types::Units::U_METERS:
    case kiapi::common::types::Units::U_TENTHS:
    default:
        return JOB_EXPORT_PCB_STATS::UNITS::MM;
    }
}


template<>
DrcErrorType ToProtoEnum( PCB_DRC_CODE aValue )
{
    switch( aValue )
    {
    case DRCE_UNCONNECTED_ITEMS:                return DrcErrorType::DRCET_UNCONNECTED_ITEMS;
    case DRCE_SHORTING_ITEMS:                   return DrcErrorType::DRCET_SHORTING_ITEMS;
    case DRCE_ALLOWED_ITEMS:                    return DrcErrorType::DRCET_ALLOWED_ITEMS;
    case DRCE_TEXT_ON_EDGECUTS:                 return DrcErrorType::DRCET_TEXT_ON_EDGECUTS;
    case DRCE_CLEARANCE:                        return DrcErrorType::DRCET_CLEARANCE;
    case DRCE_CREEPAGE:                         return DrcErrorType::DRCET_CREEPAGE;
    case DRCE_TRACKS_CROSSING:                  return DrcErrorType::DRCET_TRACKS_CROSSING;
    case DRCE_EDGE_CLEARANCE:                   return DrcErrorType::DRCET_EDGE_CLEARANCE;
    case DRCE_ZONES_INTERSECT:                  return DrcErrorType::DRCET_ZONES_INTERSECT;
    case DRCE_ISOLATED_COPPER:                  return DrcErrorType::DRCET_ISOLATED_COPPER;
    case DRCE_STARVED_THERMAL:                  return DrcErrorType::DRCET_STARVED_THERMAL;
    case DRCE_DANGLING_VIA:                     return DrcErrorType::DRCET_DANGLING_VIA;
    case DRCE_DANGLING_TRACK:                   return DrcErrorType::DRCET_DANGLING_TRACK;
    case DRCE_DRILLED_HOLES_TOO_CLOSE:          return DrcErrorType::DRCET_DRILLED_HOLES_TOO_CLOSE;
    case DRCE_DRILLED_HOLES_COLOCATED:          return DrcErrorType::DRCET_DRILLED_HOLES_COLOCATED;
    case DRCE_HOLE_CLEARANCE:                   return DrcErrorType::DRCET_HOLE_CLEARANCE;
    case DRCE_CONNECTION_WIDTH:                 return DrcErrorType::DRCET_CONNECTION_WIDTH;
    case DRCE_TRACK_WIDTH:                      return DrcErrorType::DRCET_TRACK_WIDTH;
    case DRCE_TRACK_ANGLE:                      return DrcErrorType::DRCET_TRACK_ANGLE;
    case DRCE_TRACK_SEGMENT_LENGTH:             return DrcErrorType::DRCET_TRACK_SEGMENT_LENGTH;
    case DRCE_ANNULAR_WIDTH:                    return DrcErrorType::DRCET_ANNULAR_WIDTH;
    case DRCE_DRILL_OUT_OF_RANGE:               return DrcErrorType::DRCET_DRILL_OUT_OF_RANGE;
    case DRCE_VIA_DIAMETER:                     return DrcErrorType::DRCET_VIA_DIAMETER;
    case DRCE_PADSTACK:                         return DrcErrorType::DRCET_PADSTACK;
    case DRCE_PADSTACK_INVALID:                 return DrcErrorType::DRCET_PADSTACK_INVALID;
    case DRCE_MICROVIA_DRILL_OUT_OF_RANGE:      return DrcErrorType::DRCET_MICROVIA_DRILL_OUT_OF_RANGE;
    case DRCE_OVERLAPPING_FOOTPRINTS:           return DrcErrorType::DRCET_OVERLAPPING_FOOTPRINTS;
    case DRCE_MISSING_COURTYARD:                return DrcErrorType::DRCET_MISSING_COURTYARD;
    case DRCE_MALFORMED_COURTYARD:              return DrcErrorType::DRCET_MALFORMED_COURTYARD;
    case DRCE_PTH_IN_COURTYARD:                 return DrcErrorType::DRCET_PTH_IN_COURTYARD;
    case DRCE_NPTH_IN_COURTYARD:                return DrcErrorType::DRCET_NPTH_IN_COURTYARD;
    case DRCE_DISABLED_LAYER_ITEM:              return DrcErrorType::DRCET_DISABLED_LAYER_ITEM;
    case DRCE_INVALID_OUTLINE:                  return DrcErrorType::DRCET_INVALID_OUTLINE;
    case DRCE_MISSING_FOOTPRINT:                return DrcErrorType::DRCET_MISSING_FOOTPRINT;
    case DRCE_DUPLICATE_FOOTPRINT:              return DrcErrorType::DRCET_DUPLICATE_FOOTPRINT;
    case DRCE_NET_CONFLICT:                     return DrcErrorType::DRCET_NET_CONFLICT;
    case DRCE_EXTRA_FOOTPRINT:                  return DrcErrorType::DRCET_EXTRA_FOOTPRINT;
    case DRCE_SCHEMATIC_PARITY:                 return DrcErrorType::DRCET_SCHEMATIC_PARITY;
    case DRCE_SCHEMATIC_FIELDS_PARITY:          return DrcErrorType::DRCET_SCHEMATIC_FIELDS_PARITY;
    case DRCE_FOOTPRINT_FILTERS:                return DrcErrorType::DRCET_FOOTPRINT_FILTERS;
    case DRCE_LIB_FOOTPRINT_ISSUES:             return DrcErrorType::DRCET_LIB_FOOTPRINT_ISSUES;
    case DRCE_LIB_FOOTPRINT_MISMATCH:           return DrcErrorType::DRCET_LIB_FOOTPRINT_MISMATCH;
    case DRCE_UNRESOLVED_VARIABLE:              return DrcErrorType::DRCET_UNRESOLVED_VARIABLE;
    case DRCE_ASSERTION_FAILURE:                return DrcErrorType::DRCET_ASSERTION_FAILURE;
    case DRCE_GENERIC_WARNING:                  return DrcErrorType::DRCET_GENERIC_WARNING;
    case DRCE_GENERIC_ERROR:                    return DrcErrorType::DRCET_GENERIC_ERROR;
    case DRCE_COPPER_SLIVER:                    return DrcErrorType::DRCET_COPPER_SLIVER;
    case DRCE_SILK_CLEARANCE:                   return DrcErrorType::DRCET_SILK_CLEARANCE;
    case DRCE_SILK_MASK_CLEARANCE:              return DrcErrorType::DRCET_SILK_MASK_CLEARANCE;
    case DRCE_SILK_EDGE_CLEARANCE:              return DrcErrorType::DRCET_SILK_EDGE_CLEARANCE;
    case DRCE_SOLDERMASK_BRIDGE:                return DrcErrorType::DRCET_SOLDERMASK_BRIDGE;
    case DRCE_TEXT_HEIGHT:                      return DrcErrorType::DRCET_TEXT_HEIGHT;
    case DRCE_TEXT_THICKNESS:                   return DrcErrorType::DRCET_TEXT_THICKNESS;
    case DRCE_LENGTH_OUT_OF_RANGE:              return DrcErrorType::DRCET_LENGTH_OUT_OF_RANGE;
    case DRCE_SKEW_OUT_OF_RANGE:                return DrcErrorType::DRCET_SKEW_OUT_OF_RANGE;
    case DRCE_VIA_COUNT_OUT_OF_RANGE:           return DrcErrorType::DRCET_VIA_COUNT_OUT_OF_RANGE;
    case DRCE_DIFF_PAIR_GAP_OUT_OF_RANGE:       return DrcErrorType::DRCET_DIFF_PAIR_GAP_OUT_OF_RANGE;
    case DRCE_DIFF_PAIR_UNCOUPLED_LENGTH_TOO_LONG: return DrcErrorType::DRCET_DIFF_PAIR_UNCOUPLED_LENGTH_TOO_LONG;
    case DRCE_FOOTPRINT:                        return DrcErrorType::DRCET_FOOTPRINT;
    case DRCE_FOOTPRINT_TYPE_MISMATCH:          return DrcErrorType::DRCET_FOOTPRINT_TYPE_MISMATCH;
    case DRCE_PAD_TH_WITH_NO_HOLE:              return DrcErrorType::DRCET_PAD_TH_WITH_NO_HOLE;
    case DRCE_MIRRORED_TEXT_ON_FRONT_LAYER:     return DrcErrorType::DRCET_MIRRORED_TEXT_ON_FRONT_LAYER;
    case DRCE_NONMIRRORED_TEXT_ON_BACK_LAYER:   return DrcErrorType::DRCET_NONMIRRORED_TEXT_ON_BACK_LAYER;
    case DRCE_MISSING_TUNING_PROFILE:           return DrcErrorType::DRCET_MISSING_TUNING_PROFILE;
    case DRCE_TUNING_PROFILE_IMPLICIT_RULES:    return DrcErrorType::DRCET_TUNING_PROFILE_IMPLICIT_RULES;
    case DRCE_TRACK_ON_POST_MACHINED_LAYER:     return DrcErrorType::DRCET_TRACK_ON_POST_MACHINED_LAYER;
    case DRCE_TRACK_NOT_CENTERED_ON_VIA:        return DrcErrorType::DRCET_TRACK_NOT_CENTERED_ON_VIA;
    default:
        wxCHECK_MSG( false, DrcErrorType::DRCET_UNKNOWN,
                     "Unhandled case in ToProtoEnum<PCB_DRC_CODE>" );
    }
}


template<>
PCB_DRC_CODE FromProtoEnum( DrcErrorType aValue )
{
    switch( aValue )
    {
    case DrcErrorType::DRCET_UNCONNECTED_ITEMS:           return DRCE_UNCONNECTED_ITEMS;
    case DrcErrorType::DRCET_SHORTING_ITEMS:              return DRCE_SHORTING_ITEMS;
    case DrcErrorType::DRCET_ALLOWED_ITEMS:               return DRCE_ALLOWED_ITEMS;
    case DrcErrorType::DRCET_TEXT_ON_EDGECUTS:            return DRCE_TEXT_ON_EDGECUTS;
    case DrcErrorType::DRCET_CLEARANCE:                   return DRCE_CLEARANCE;
    case DrcErrorType::DRCET_CREEPAGE:                    return DRCE_CREEPAGE;
    case DrcErrorType::DRCET_TRACKS_CROSSING:             return DRCE_TRACKS_CROSSING;
    case DrcErrorType::DRCET_EDGE_CLEARANCE:              return DRCE_EDGE_CLEARANCE;
    case DrcErrorType::DRCET_ZONES_INTERSECT:             return DRCE_ZONES_INTERSECT;
    case DrcErrorType::DRCET_ISOLATED_COPPER:             return DRCE_ISOLATED_COPPER;
    case DrcErrorType::DRCET_STARVED_THERMAL:             return DRCE_STARVED_THERMAL;
    case DrcErrorType::DRCET_DANGLING_VIA:                return DRCE_DANGLING_VIA;
    case DrcErrorType::DRCET_DANGLING_TRACK:              return DRCE_DANGLING_TRACK;
    case DrcErrorType::DRCET_DRILLED_HOLES_TOO_CLOSE:     return DRCE_DRILLED_HOLES_TOO_CLOSE;
    case DrcErrorType::DRCET_DRILLED_HOLES_COLOCATED:     return DRCE_DRILLED_HOLES_COLOCATED;
    case DrcErrorType::DRCET_HOLE_CLEARANCE:              return DRCE_HOLE_CLEARANCE;
    case DrcErrorType::DRCET_CONNECTION_WIDTH:            return DRCE_CONNECTION_WIDTH;
    case DrcErrorType::DRCET_TRACK_WIDTH:                 return DRCE_TRACK_WIDTH;
    case DrcErrorType::DRCET_TRACK_ANGLE:                 return DRCE_TRACK_ANGLE;
    case DrcErrorType::DRCET_TRACK_SEGMENT_LENGTH:        return DRCE_TRACK_SEGMENT_LENGTH;
    case DrcErrorType::DRCET_ANNULAR_WIDTH:               return DRCE_ANNULAR_WIDTH;
    case DrcErrorType::DRCET_DRILL_OUT_OF_RANGE:          return DRCE_DRILL_OUT_OF_RANGE;
    case DrcErrorType::DRCET_VIA_DIAMETER:                return DRCE_VIA_DIAMETER;
    case DrcErrorType::DRCET_PADSTACK:                    return DRCE_PADSTACK;
    case DrcErrorType::DRCET_PADSTACK_INVALID:            return DRCE_PADSTACK_INVALID;
    case DrcErrorType::DRCET_MICROVIA_DRILL_OUT_OF_RANGE: return DRCE_MICROVIA_DRILL_OUT_OF_RANGE;
    case DrcErrorType::DRCET_OVERLAPPING_FOOTPRINTS:      return DRCE_OVERLAPPING_FOOTPRINTS;
    case DrcErrorType::DRCET_MISSING_COURTYARD:           return DRCE_MISSING_COURTYARD;
    case DrcErrorType::DRCET_MALFORMED_COURTYARD:         return DRCE_MALFORMED_COURTYARD;
    case DrcErrorType::DRCET_PTH_IN_COURTYARD:            return DRCE_PTH_IN_COURTYARD;
    case DrcErrorType::DRCET_NPTH_IN_COURTYARD:           return DRCE_NPTH_IN_COURTYARD;
    case DrcErrorType::DRCET_DISABLED_LAYER_ITEM:         return DRCE_DISABLED_LAYER_ITEM;
    case DrcErrorType::DRCET_INVALID_OUTLINE:             return DRCE_INVALID_OUTLINE;
    case DrcErrorType::DRCET_MISSING_FOOTPRINT:           return DRCE_MISSING_FOOTPRINT;
    case DrcErrorType::DRCET_DUPLICATE_FOOTPRINT:         return DRCE_DUPLICATE_FOOTPRINT;
    case DrcErrorType::DRCET_NET_CONFLICT:                return DRCE_NET_CONFLICT;
    case DrcErrorType::DRCET_EXTRA_FOOTPRINT:             return DRCE_EXTRA_FOOTPRINT;
    case DrcErrorType::DRCET_SCHEMATIC_PARITY:            return DRCE_SCHEMATIC_PARITY;
    case DrcErrorType::DRCET_SCHEMATIC_FIELDS_PARITY:     return DRCE_SCHEMATIC_FIELDS_PARITY;
    case DrcErrorType::DRCET_FOOTPRINT_FILTERS:           return DRCE_FOOTPRINT_FILTERS;
    case DrcErrorType::DRCET_LIB_FOOTPRINT_ISSUES:        return DRCE_LIB_FOOTPRINT_ISSUES;
    case DrcErrorType::DRCET_LIB_FOOTPRINT_MISMATCH:      return DRCE_LIB_FOOTPRINT_MISMATCH;
    case DrcErrorType::DRCET_UNRESOLVED_VARIABLE:         return DRCE_UNRESOLVED_VARIABLE;
    case DrcErrorType::DRCET_ASSERTION_FAILURE:           return DRCE_ASSERTION_FAILURE;
    case DrcErrorType::DRCET_GENERIC_WARNING:             return DRCE_GENERIC_WARNING;
    case DrcErrorType::DRCET_GENERIC_ERROR:               return DRCE_GENERIC_ERROR;
    case DrcErrorType::DRCET_COPPER_SLIVER:               return DRCE_COPPER_SLIVER;
    case DrcErrorType::DRCET_SILK_CLEARANCE:              return DRCE_SILK_CLEARANCE;
    case DrcErrorType::DRCET_SILK_MASK_CLEARANCE:         return DRCE_SILK_MASK_CLEARANCE;
    case DrcErrorType::DRCET_SILK_EDGE_CLEARANCE:         return DRCE_SILK_EDGE_CLEARANCE;
    case DrcErrorType::DRCET_SOLDERMASK_BRIDGE:           return DRCE_SOLDERMASK_BRIDGE;
    case DrcErrorType::DRCET_TEXT_HEIGHT:                 return DRCE_TEXT_HEIGHT;
    case DrcErrorType::DRCET_TEXT_THICKNESS:              return DRCE_TEXT_THICKNESS;
    case DrcErrorType::DRCET_LENGTH_OUT_OF_RANGE:         return DRCE_LENGTH_OUT_OF_RANGE;
    case DrcErrorType::DRCET_SKEW_OUT_OF_RANGE:           return DRCE_SKEW_OUT_OF_RANGE;
    case DrcErrorType::DRCET_VIA_COUNT_OUT_OF_RANGE:      return DRCE_VIA_COUNT_OUT_OF_RANGE;
    case DrcErrorType::DRCET_DIFF_PAIR_GAP_OUT_OF_RANGE:  return DRCE_DIFF_PAIR_GAP_OUT_OF_RANGE;
    case DrcErrorType::DRCET_DIFF_PAIR_UNCOUPLED_LENGTH_TOO_LONG: return DRCE_DIFF_PAIR_UNCOUPLED_LENGTH_TOO_LONG;
    case DrcErrorType::DRCET_FOOTPRINT:                      return DRCE_FOOTPRINT;
    case DrcErrorType::DRCET_FOOTPRINT_TYPE_MISMATCH:        return DRCE_FOOTPRINT_TYPE_MISMATCH;
    case DrcErrorType::DRCET_PAD_TH_WITH_NO_HOLE:            return DRCE_PAD_TH_WITH_NO_HOLE;
    case DrcErrorType::DRCET_MIRRORED_TEXT_ON_FRONT_LAYER:   return DRCE_MIRRORED_TEXT_ON_FRONT_LAYER;
    case DrcErrorType::DRCET_NONMIRRORED_TEXT_ON_BACK_LAYER: return DRCE_NONMIRRORED_TEXT_ON_BACK_LAYER;
    case DrcErrorType::DRCET_MISSING_TUNING_PROFILE:         return DRCE_MISSING_TUNING_PROFILE;
    case DrcErrorType::DRCET_TUNING_PROFILE_IMPLICIT_RULES:  return DRCE_TUNING_PROFILE_IMPLICIT_RULES;
    case DrcErrorType::DRCET_TRACK_ON_POST_MACHINED_LAYER:   return DRCE_TRACK_ON_POST_MACHINED_LAYER;
    case DrcErrorType::DRCET_TRACK_NOT_CENTERED_ON_VIA:      return DRCE_TRACK_NOT_CENTERED_ON_VIA;

    case DrcErrorType::DRCET_UNKNOWN:
    default:
        return static_cast<PCB_DRC_CODE>( 0 );
    }
}


template<>
types::ConstraintType ToProtoEnum( PCB_CONSTRAINT_TYPE aValue )
{
    switch( aValue )
    {
    case PCB_CONSTRAINT_TYPE::COINCIDENT:        return types::ConstraintType::CT_COINCIDENT;
    case PCB_CONSTRAINT_TYPE::HORIZONTAL:        return types::ConstraintType::CT_HORIZONTAL;
    case PCB_CONSTRAINT_TYPE::VERTICAL:          return types::ConstraintType::CT_VERTICAL;
    case PCB_CONSTRAINT_TYPE::PARALLEL:          return types::ConstraintType::CT_PARALLEL;
    case PCB_CONSTRAINT_TYPE::PERPENDICULAR:     return types::ConstraintType::CT_PERPENDICULAR;
    case PCB_CONSTRAINT_TYPE::COLLINEAR:         return types::ConstraintType::CT_COLLINEAR;
    case PCB_CONSTRAINT_TYPE::SYMMETRIC:         return types::ConstraintType::CT_SYMMETRIC;
    case PCB_CONSTRAINT_TYPE::EQUAL_LENGTH:      return types::ConstraintType::CT_EQUAL_LENGTH;
    case PCB_CONSTRAINT_TYPE::EQUAL_RADIUS:      return types::ConstraintType::CT_EQUAL_RADIUS;
    case PCB_CONSTRAINT_TYPE::POINT_ON_LINE:     return types::ConstraintType::CT_POINT_ON_LINE;
    case PCB_CONSTRAINT_TYPE::MIDPOINT:          return types::ConstraintType::CT_MIDPOINT;
    case PCB_CONSTRAINT_TYPE::FIXED_POSITION:    return types::ConstraintType::CT_FIXED_POSITION;
    case PCB_CONSTRAINT_TYPE::FIXED_LENGTH:      return types::ConstraintType::CT_FIXED_LENGTH;
    case PCB_CONSTRAINT_TYPE::CONCENTRIC:        return types::ConstraintType::CT_CONCENTRIC;
    case PCB_CONSTRAINT_TYPE::FIXED_RADIUS:      return types::ConstraintType::CT_FIXED_RADIUS;
    case PCB_CONSTRAINT_TYPE::ANGULAR_DIMENSION: return types::ConstraintType::CT_ANGULAR_DIMENSION;
    case PCB_CONSTRAINT_TYPE::TANGENT:           return types::ConstraintType::CT_TANGENT;
    case PCB_CONSTRAINT_TYPE::ARC_ANGLE:         return types::ConstraintType::CT_ARC_ANGLE;

    case PCB_CONSTRAINT_TYPE::UNDEFINED:
    default:
        wxCHECK_MSG( false, types::ConstraintType::CT_UNKNOWN,
                     "Unhandled case in ToProtoEnum<PCB_CONSTRAINT_TYPE>" );
    }
}


template<>
PCB_CONSTRAINT_TYPE FromProtoEnum( types::ConstraintType aValue )
{
    switch( aValue )
    {
    case types::ConstraintType::CT_UNKNOWN:           return PCB_CONSTRAINT_TYPE::UNDEFINED;
    case types::ConstraintType::CT_COINCIDENT:        return PCB_CONSTRAINT_TYPE::COINCIDENT;
    case types::ConstraintType::CT_HORIZONTAL:        return PCB_CONSTRAINT_TYPE::HORIZONTAL;
    case types::ConstraintType::CT_VERTICAL:          return PCB_CONSTRAINT_TYPE::VERTICAL;
    case types::ConstraintType::CT_PARALLEL:          return PCB_CONSTRAINT_TYPE::PARALLEL;
    case types::ConstraintType::CT_PERPENDICULAR:     return PCB_CONSTRAINT_TYPE::PERPENDICULAR;
    case types::ConstraintType::CT_COLLINEAR:         return PCB_CONSTRAINT_TYPE::COLLINEAR;
    case types::ConstraintType::CT_SYMMETRIC:         return PCB_CONSTRAINT_TYPE::SYMMETRIC;
    case types::ConstraintType::CT_EQUAL_LENGTH:      return PCB_CONSTRAINT_TYPE::EQUAL_LENGTH;
    case types::ConstraintType::CT_EQUAL_RADIUS:      return PCB_CONSTRAINT_TYPE::EQUAL_RADIUS;
    case types::ConstraintType::CT_POINT_ON_LINE:     return PCB_CONSTRAINT_TYPE::POINT_ON_LINE;
    case types::ConstraintType::CT_MIDPOINT:          return PCB_CONSTRAINT_TYPE::MIDPOINT;
    case types::ConstraintType::CT_FIXED_POSITION:    return PCB_CONSTRAINT_TYPE::FIXED_POSITION;
    case types::ConstraintType::CT_FIXED_LENGTH:      return PCB_CONSTRAINT_TYPE::FIXED_LENGTH;
    case types::ConstraintType::CT_CONCENTRIC:        return PCB_CONSTRAINT_TYPE::CONCENTRIC;
    case types::ConstraintType::CT_FIXED_RADIUS:      return PCB_CONSTRAINT_TYPE::FIXED_RADIUS;
    case types::ConstraintType::CT_ANGULAR_DIMENSION: return PCB_CONSTRAINT_TYPE::ANGULAR_DIMENSION;
    case types::ConstraintType::CT_TANGENT:           return PCB_CONSTRAINT_TYPE::TANGENT;
    case types::ConstraintType::CT_ARC_ANGLE:         return PCB_CONSTRAINT_TYPE::ARC_ANGLE;

    default:
        wxCHECK_MSG( false, PCB_CONSTRAINT_TYPE::UNDEFINED,
                     "Unhandled case in FromProtoEnum<types::ConstraintType>" );
    }
}


template<>
types::ConstraintAnchor ToProtoEnum( CONSTRAINT_ANCHOR aValue )
{
    switch( aValue )
    {
    case CONSTRAINT_ANCHOR::WHOLE:  return types::ConstraintAnchor::CA_WHOLE;
    case CONSTRAINT_ANCHOR::START:  return types::ConstraintAnchor::CA_START;
    case CONSTRAINT_ANCHOR::END:    return types::ConstraintAnchor::CA_END;
    case CONSTRAINT_ANCHOR::MID:    return types::ConstraintAnchor::CA_MID;
    case CONSTRAINT_ANCHOR::CENTER: return types::ConstraintAnchor::CA_CENTER;
    case CONSTRAINT_ANCHOR::RADIUS: return types::ConstraintAnchor::CA_RADIUS;

    default:
        wxCHECK_MSG( false, types::ConstraintAnchor::CA_UNKNOWN,
                     "Unhandled case in ToProtoEnum<CONSTRAINT_ANCHOR>" );
    }
}


template<>
CONSTRAINT_ANCHOR FromProtoEnum( types::ConstraintAnchor aValue )
{
    switch( aValue )
    {
    case types::ConstraintAnchor::CA_UNKNOWN:
    case types::ConstraintAnchor::CA_WHOLE:  return CONSTRAINT_ANCHOR::WHOLE;
    case types::ConstraintAnchor::CA_START:  return CONSTRAINT_ANCHOR::START;
    case types::ConstraintAnchor::CA_END:    return CONSTRAINT_ANCHOR::END;
    case types::ConstraintAnchor::CA_MID:    return CONSTRAINT_ANCHOR::MID;
    case types::ConstraintAnchor::CA_CENTER: return CONSTRAINT_ANCHOR::CENTER;
    case types::ConstraintAnchor::CA_RADIUS: return CONSTRAINT_ANCHOR::RADIUS;

    default:
        wxCHECK_MSG( false, CONSTRAINT_ANCHOR::WHOLE,
                     "Unhandled case in FromProtoEnum<types::ConstraintAnchor>" );
    }
}


// Adding something new here?  Add it to test_api_enums.cpp!
