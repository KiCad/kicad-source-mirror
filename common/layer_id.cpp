/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2022 KiCad Developers, see AUTHORS.txt for contributors.
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

#include <layer_ids.h>
#include <wx/translation.h>

/**
 * Returns the default display name for a given layer.  These are not the same as the canonical
 * name in LSET::Name(), which is used in board files and cannot be translated or changed.
 * WARNING: do not translate board physical layers names (F.Cu to User.9): because canonical names
 * are used in files (boards and fab files), using translated names in UI create mistakes for users.
 * Board physical layers names must be seen as proper nouns.
 */
wxString LayerName( int aLayer )
{
    switch( aLayer )
    {
    // PCB_LAYER_ID
    case F_Cu:              return wxT( "F.Cu" );
    case In1_Cu:            return wxT( "In1.Cu" );
    case In2_Cu:            return wxT( "In2.Cu" );
    case In3_Cu:            return wxT( "In3.Cu" );
    case In4_Cu:            return wxT( "In4.Cu" );
    case In5_Cu:            return wxT( "In5.Cu" );
    case In6_Cu:            return wxT( "In6.Cu" );
    case In7_Cu:            return wxT( "In7.Cu" );
    case In8_Cu:            return wxT( "In8.Cu" );
    case In9_Cu:            return wxT( "In9.Cu" );
    case In10_Cu:           return wxT( "In10.Cu" );
    case In11_Cu:           return wxT( "In11.Cu" );
    case In12_Cu:           return wxT( "In12.Cu" );
    case In13_Cu:           return wxT( "In13.Cu" );
    case In14_Cu:           return wxT( "In14.Cu" );
    case In15_Cu:           return wxT( "In15.Cu" );
    case In16_Cu:           return wxT( "In16.Cu" );
    case In17_Cu:           return wxT( "In17.Cu" );
    case In18_Cu:           return wxT( "In18.Cu" );
    case In19_Cu:           return wxT( "In19.Cu" );
    case In20_Cu:           return wxT( "In20.Cu" );
    case In21_Cu:           return wxT( "In21.Cu" );
    case In22_Cu:           return wxT( "In22.Cu" );
    case In23_Cu:           return wxT( "In23.Cu" );
    case In24_Cu:           return wxT( "In24.Cu" );
    case In25_Cu:           return wxT( "In25.Cu" );
    case In26_Cu:           return wxT( "In26.Cu" );
    case In27_Cu:           return wxT( "In27.Cu" );
    case In28_Cu:           return wxT( "In28.Cu" );
    case In29_Cu:           return wxT( "In29.Cu" );
    case In30_Cu:           return wxT( "In30.Cu" );
    case B_Cu:              return wxT( "B.Cu" );

    // Technicals
    case B_Adhes:           return wxT( "B.Adhesive" );
    case F_Adhes:           return wxT( "F.Adhesive" );
    case B_Paste:           return wxT( "B.Paste" );
    case F_Paste:           return wxT( "F.Paste" );
    case B_SilkS:           return wxT( "B.Silkscreen" );
    case F_SilkS:           return wxT( "F.Silkscreen" );
    case B_Mask:            return wxT( "B.Mask" );
    case F_Mask:            return wxT( "F.Mask" );

    // Users
    case Dwgs_User:         return wxT( "User.Drawings" );
    case Cmts_User:         return wxT( "User.Comments" );
    case Eco1_User:         return wxT( "User.Eco1" );
    case Eco2_User:         return wxT( "User.Eco2" );
    case Edge_Cuts:         return wxT( "Edge.Cuts" );
    case Margin:            return wxT( "Margin" );

    // Footprint
    case F_CrtYd:           return wxT( "F.Courtyard" );
    case B_CrtYd:           return wxT( "B.Courtyard" );
    case F_Fab:             return wxT( "F.Fab" );
    case B_Fab:             return wxT( "B.Fab" );

    // User definable layers.
    case User_1:            return wxT( "User.1" );
    case User_2:            return wxT( "User.2" );
    case User_3:            return wxT( "User.3" );
    case User_4:            return wxT( "User.4" );
    case User_5:            return wxT( "User.5" );
    case User_6:            return wxT( "User.6" );
    case User_7:            return wxT( "User.7" );
    case User_8:            return wxT( "User.8" );
    case User_9:            return wxT( "User.9" );

    // Rescue
    case Rescue:            return _( "Rescue" );

    // SCH_LAYER_ID

    case LAYER_WIRE:                    return _( "Wires" );
    case LAYER_BUS:                     return _( "Buses" );
    case LAYER_BUS_JUNCTION:            return _( "Bus junctions" );
    case LAYER_JUNCTION:                return _( "Junctions" );
    case LAYER_LOCLABEL:                return _( "Labels" );
    case LAYER_GLOBLABEL:               return _( "Global labels" );
    case LAYER_HIERLABEL:               return _( "Hierarchical labels" );
    case LAYER_PINNUM:                  return _( "Pin numbers" );
    case LAYER_PINNAM:                  return _( "Pin names" );
    case LAYER_REFERENCEPART:           return _( "Symbol references" );
    case LAYER_VALUEPART:               return _( "Symbol values" );
    case LAYER_FIELDS:                  return _( "Symbol fields" );
    case LAYER_INTERSHEET_REFS:         return _( "Sheet references" );
    case LAYER_NETCLASS_REFS:           return _( "Net class references" );
    case LAYER_DEVICE:                  return _( "Symbol body outlines" );
    case LAYER_DEVICE_BACKGROUND:       return _( "Symbol body fills" );
    case LAYER_NOTES:                   return _( "Schematic text && graphics" );
    case LAYER_PRIVATE_NOTES:           return _( "Symbol private text && graphics" );
    case LAYER_NOTES_BACKGROUND:        return _( "Schematic text && graphics backgrounds" );
    case LAYER_PIN:                     return _( "Pins" );
    case LAYER_SHEET:                   return _( "Sheet borders" );
    case LAYER_SHEET_BACKGROUND:        return _( "Sheet backgrounds" );
    case LAYER_SHEETNAME:               return _( "Sheet names" );
    case LAYER_SHEETFIELDS:             return _( "Sheet fields" );
    case LAYER_SHEETFILENAME:           return _( "Sheet file names" );
    case LAYER_SHEETLABEL:              return _( "Sheet pins" );
    case LAYER_NOCONNECT:               return _( "No-connect symbols" );
    case LAYER_DNP_MARKER:              return _( "DNP markers" );
    case LAYER_ERC_WARN:                return _( "ERC warnings" );
    case LAYER_ERC_ERR:                 return _( "ERC errors" );
    case LAYER_ERC_EXCLUSION:           return _( "ERC exclusions" );
    case LAYER_SCHEMATIC_ANCHOR:        return _( "Anchors" );
    case LAYER_SCHEMATIC_AUX_ITEMS:     return _( "Helper items" );
    case LAYER_SCHEMATIC_GRID:          return _( "Grid" );
    case LAYER_SCHEMATIC_GRID_AXES:     return _( "Axes" );
    case LAYER_SCHEMATIC_BACKGROUND:    return _( "Background" );
    case LAYER_SCHEMATIC_CURSOR:        return _( "Cursor" );
    case LAYER_HOVERED:                 return _( "Hovered items" );
    case LAYER_BRIGHTENED:              return _( "Highlighted items" );
    case LAYER_HIDDEN:                  return _( "Hidden items" );
    case LAYER_SELECTION_SHADOWS:       return _( "Selection highlight" );
    case LAYER_SCHEMATIC_DRAWINGSHEET:  return _( "Drawing sheet" );
    case LAYER_SCHEMATIC_PAGE_LIMITS:   return _( "Page limits" );
    case LAYER_OP_VOLTAGES:             return _( "Operating point voltages" );
    case LAYER_OP_CURRENTS:             return _( "Operating point currents" );

    // GAL_LAYER_ID

    case LAYER_MOD_FR:                  return _( "Footprints front" );
    case LAYER_MOD_BK:                  return _( "Footprints back" );
    case LAYER_MOD_VALUES:              return _( "Values" );
    case LAYER_MOD_REFERENCES:          return _( "Reference designators" );
    case LAYER_MOD_TEXT:                return _( "Footprint text" );
    case LAYER_MOD_TEXT_INVISIBLE:      return _( "Hidden text" );
    case LAYER_PAD_FR:                  return _( "SMD pads front" );
    case LAYER_PAD_BK:                  return _( "SMD pads back" );
    case LAYER_PADS_TH:                 return _( "Through-hole pads" );
    case LAYER_TRACKS:                  return _( "Tracks" );
    case LAYER_VIA_THROUGH:             return _( "Through vias" );
    case LAYER_VIA_BBLIND:              return _( "Blind/Buried vias" );
    case LAYER_VIA_MICROVIA:            return _( "Micro-vias" );
    case LAYER_VIA_HOLES:               return _( "Via holes" );
    case LAYER_VIA_HOLEWALLS:           return _( "Via hole walls" );
    case LAYER_PAD_PLATEDHOLES:         return _( "Plated holes" );
    case LAYER_PAD_HOLEWALLS:           return _( "Plated hole walls" );
    case LAYER_NON_PLATEDHOLES:         return _( "Non-plated holes" );
    case LAYER_RATSNEST:                return _( "Ratsnest" );
    case LAYER_DRC_WARNING:             return _( "DRC warnings" );
    case LAYER_DRC_ERROR:               return _( "DRC errors" );
    case LAYER_DRC_EXCLUSION:           return _( "DRC exclusions" );
    case LAYER_MARKER_SHADOWS:          return _( "DRC marker shadows" );
    case LAYER_ANCHOR:                  return _( "Anchors" );
    case LAYER_DRAWINGSHEET:            return _( "Drawing sheet" );
    case LAYER_PAGE_LIMITS:             return _( "Page limits" );
    case LAYER_CURSOR:                  return _( "Cursor" );
    case LAYER_AUX_ITEMS:               return _( "Helper items" );
    case LAYER_GRID:                    return _( "Grid" );
    case LAYER_GRID_AXES:               return _( "Grid axes" );
    case LAYER_PCB_BACKGROUND:          return _( "Background" );
    case LAYER_SELECT_OVERLAY:          return _( "Selection highlight" );
    case LAYER_LOCKED_ITEM_SHADOW:      return _( "Locked item shadow" );
    case LAYER_CONFLICTS_SHADOW:        return _( "Courtyard collision shadow" );

    default:
        wxCHECK_MSG( false, wxEmptyString, wxString::Format( "Unknown layer ID %d", aLayer ) );
    }
}
