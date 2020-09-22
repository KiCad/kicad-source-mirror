/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2020 KiCad Developers, see AUTHORS.txt for contributors.
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

#include <layers_id_colors_and_visibility.h>
#include <wx/wx.h>


wxString LayerName( int aLayer )
{
    switch( aLayer )
    {
    // PCB_LAYER_ID
    case F_Cu:              return _( "F.Cu" );
    case In1_Cu:            return _( "In1.Cu" );
    case In2_Cu:            return _( "In2.Cu" );
    case In3_Cu:            return _( "In3.Cu" );
    case In4_Cu:            return _( "In4.Cu" );
    case In5_Cu:            return _( "In5.Cu" );
    case In6_Cu:            return _( "In6.Cu" );
    case In7_Cu:            return _( "In7.Cu" );
    case In8_Cu:            return _( "In8.Cu" );
    case In9_Cu:            return _( "In9.Cu" );
    case In10_Cu:           return _( "In10.Cu" );
    case In11_Cu:           return _( "In11.Cu" );
    case In12_Cu:           return _( "In12.Cu" );
    case In13_Cu:           return _( "In13.Cu" );
    case In14_Cu:           return _( "In14.Cu" );
    case In15_Cu:           return _( "In15.Cu" );
    case In16_Cu:           return _( "In16.Cu" );
    case In17_Cu:           return _( "In17.Cu" );
    case In18_Cu:           return _( "In18.Cu" );
    case In19_Cu:           return _( "In19.Cu" );
    case In20_Cu:           return _( "In20.Cu" );
    case In21_Cu:           return _( "In21.Cu" );
    case In22_Cu:           return _( "In22.Cu" );
    case In23_Cu:           return _( "In23.Cu" );
    case In24_Cu:           return _( "In24.Cu" );
    case In25_Cu:           return _( "In25.Cu" );
    case In26_Cu:           return _( "In26.Cu" );
    case In27_Cu:           return _( "In27.Cu" );
    case In28_Cu:           return _( "In28.Cu" );
    case In29_Cu:           return _( "In29.Cu" );
    case In30_Cu:           return _( "In30.Cu" );
    case B_Cu:              return _( "B.Cu" );

    // Technicals
    case B_Adhes:           return _( "B.Adhes" );
    case F_Adhes:           return _( "F.Adhes" );
    case B_Paste:           return _( "B.Paste" );
    case F_Paste:           return _( "F.Paste" );
    case B_SilkS:           return _( "B.SilkS" );
    case F_SilkS:           return _( "F.SilkS" );
    case B_Mask:            return _( "B.Mask" );
    case F_Mask:            return _( "F.Mask" );

    // Users
    case Dwgs_User:         return _( "Dwgs.User" );
    case Cmts_User:         return _( "Cmts.User" );
    case Eco1_User:         return _( "Eco1.User" );
    case Eco2_User:         return _( "Eco2.User" );
    case Edge_Cuts:         return _( "Edge.Cuts" );
    case Margin:            return _( "Margin" );

    // Footprint
    case F_CrtYd:           return _( "F.CrtYd" );
    case B_CrtYd:           return _( "B.CrtYd" );
    case F_Fab:             return _( "F.Fab" );
    case B_Fab:             return _( "B.Fab" );

    // User definable layers.
    case User_1:            return _( "User.1" );
    case User_2:            return _( "User.2" );
    case User_3:            return _( "User.3" );
    case User_4:            return _( "User.4" );
    case User_5:            return _( "User.5" );
    case User_6:            return _( "User.6" );
    case User_7:            return _( "User.7" );
    case User_8:            return _( "User.8" );
    case User_9:            return _( "User.9" );

    // Rescue
    case Rescue:            return _( "Rescue" );

    // SCH_LAYER_ID

    case LAYER_WIRE:                    return _( "Wire" );
    case LAYER_BUS:                     return _( "Bus" );
    case LAYER_BUS_JUNCTION:            return _( "Bus Junction" );
    case LAYER_JUNCTION:                return _( "Junction" );
    case LAYER_LOCLABEL:                return _( "Label" );
    case LAYER_GLOBLABEL:               return _( "Global label" );
    case LAYER_HIERLABEL:               return _( "Hierarchical label" );
    case LAYER_PINNUM:                  return _( "Pin number" );
    case LAYER_PINNAM:                  return _( "Pin name" );
    case LAYER_REFERENCEPART:           return _( "Symbol reference" );
    case LAYER_VALUEPART:               return _( "Symbol value" );
    case LAYER_FIELDS:                  return _( "Symbol fields" );
    case LAYER_DEVICE:                  return _( "Symbol body outline" );
    case LAYER_DEVICE_BACKGROUND:       return _( "Symbol body fill" );
    case LAYER_NOTES:                   return _( "Notes" );
    case LAYER_NETNAM:                  return _( "Net name" );
    case LAYER_PIN:                     return _( "Pin" );
    case LAYER_SHEET:                   return _( "Sheet border" );
    case LAYER_SHEET_BACKGROUND:        return _( "Sheet background" );
    case LAYER_SHEETNAME:               return _( "Sheet name" );
    case LAYER_SHEETFIELDS:             return _( "Sheet fields" );
    case LAYER_SHEETFILENAME:           return _( "Sheet file name" );
    case LAYER_SHEETLABEL:              return _( "Sheet label" );
    case LAYER_NOCONNECT:               return _( "No connect symbol" );
    case LAYER_ERC_WARN:                return _( "ERC warning" );
    case LAYER_ERC_ERR:                 return _( "ERC error" );
    case LAYER_SCHEMATIC_AUX_ITEMS:     return _( "Helper items" );
    case LAYER_SCHEMATIC_GRID:          return _( "Grid" );
    case LAYER_SCHEMATIC_GRID_AXES:     return _( "Axes" );
    case LAYER_SCHEMATIC_BACKGROUND:    return _( "Background" );
    case LAYER_SCHEMATIC_CURSOR:        return _( "Cursor" );
    case LAYER_BRIGHTENED:              return _( "Highlighted items" );
    case LAYER_HIDDEN:                  return _( "Hidden item" );
    case LAYER_SELECTION_SHADOWS:       return _( "Selection highlight" );
    case LAYER_SCHEMATIC_WORKSHEET:     return _( "Worksheet" );

    // GAL_LAYER_ID

    case LAYER_MOD_FR:                  return _( "Footprints Front" );
    case LAYER_MOD_BK:                  return _( "Footprints Back" );
    case LAYER_MOD_VALUES:              return _( "Values" );
    case LAYER_MOD_REFERENCES:          return _( "Reference Designators" );
    case LAYER_MOD_TEXT_FR:             return _( "Footprint Text Front" );
    case LAYER_MOD_TEXT_BK:             return _( "Footprint Text Back" );
    case LAYER_MOD_TEXT_INVISIBLE:      return _( "Hidden Text" );
    case LAYER_PAD_FR:                  return _( "Pads Front" );
    case LAYER_PAD_BK:                  return _( "Pads Back" );
    case LAYER_PADS_TH:                 return _( "Through Hole Pads" );
    case LAYER_TRACKS:                  return _( "Tracks" );
    case LAYER_VIA_THROUGH:             return _( "Through Via" );
    case LAYER_VIA_BBLIND:              return _( "Bl/Buried Via" );
    case LAYER_VIA_MICROVIA:            return _( "Micro Via" );
    case LAYER_NON_PLATEDHOLES:         return _( "Non Plated Holes" );
    case LAYER_RATSNEST:                return _( "Ratsnest" );
    case LAYER_NO_CONNECTS:             return _( "No-Connects" );
    case LAYER_DRC_WARNING:             return _( "DRC Warnings" );
    case LAYER_DRC_ERROR:               return _( "DRC Errors" );
    case LAYER_DRC_EXCLUSION:           return _( "DRC Exclusions" );
    case LAYER_ANCHOR:                  return _( "Anchors" );
    case LAYER_WORKSHEET:               return _( "Worksheet" );
    case LAYER_CURSOR:                  return _( "Cursor" );
    case LAYER_AUX_ITEMS:               return _( "Helper items" );
    case LAYER_GRID:                    return _( "Grid" );
    case LAYER_GRID_AXES:               return _( "Grid Axes" );
    case LAYER_PCB_BACKGROUND:          return _( "Background" );
    case LAYER_SELECT_OVERLAY:          return _( "Selection highlight" );

    default:
        wxCHECK_MSG( false, wxEmptyString, wxString::Format( "Unknown layer ID %d", aLayer ) );
    }
}
