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
#include <magic_enum.hpp>
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
    case UNDEFINED_LAYER:   return _( "undefined" );

    // Copper
    case PCB_LAYER_ID::F_Cu:              return wxT( "F.Cu" );
    case PCB_LAYER_ID::In1_Cu:            return wxT( "In1.Cu" );
    case PCB_LAYER_ID::In2_Cu:            return wxT( "In2.Cu" );
    case PCB_LAYER_ID::In3_Cu:            return wxT( "In3.Cu" );
    case PCB_LAYER_ID::In4_Cu:            return wxT( "In4.Cu" );
    case PCB_LAYER_ID::In5_Cu:            return wxT( "In5.Cu" );
    case PCB_LAYER_ID::In6_Cu:            return wxT( "In6.Cu" );
    case PCB_LAYER_ID::In7_Cu:            return wxT( "In7.Cu" );
    case PCB_LAYER_ID::In8_Cu:            return wxT( "In8.Cu" );
    case PCB_LAYER_ID::In9_Cu:            return wxT( "In9.Cu" );
    case PCB_LAYER_ID::In10_Cu:           return wxT( "In10.Cu" );
    case PCB_LAYER_ID::In11_Cu:           return wxT( "In11.Cu" );
    case PCB_LAYER_ID::In12_Cu:           return wxT( "In12.Cu" );
    case PCB_LAYER_ID::In13_Cu:           return wxT( "In13.Cu" );
    case PCB_LAYER_ID::In14_Cu:           return wxT( "In14.Cu" );
    case PCB_LAYER_ID::In15_Cu:           return wxT( "In15.Cu" );
    case PCB_LAYER_ID::In16_Cu:           return wxT( "In16.Cu" );
    case PCB_LAYER_ID::In17_Cu:           return wxT( "In17.Cu" );
    case PCB_LAYER_ID::In18_Cu:           return wxT( "In18.Cu" );
    case PCB_LAYER_ID::In19_Cu:           return wxT( "In19.Cu" );
    case PCB_LAYER_ID::In20_Cu:           return wxT( "In20.Cu" );
    case PCB_LAYER_ID::In21_Cu:           return wxT( "In21.Cu" );
    case PCB_LAYER_ID::In22_Cu:           return wxT( "In22.Cu" );
    case PCB_LAYER_ID::In23_Cu:           return wxT( "In23.Cu" );
    case PCB_LAYER_ID::In24_Cu:           return wxT( "In24.Cu" );
    case PCB_LAYER_ID::In25_Cu:           return wxT( "In25.Cu" );
    case PCB_LAYER_ID::In26_Cu:           return wxT( "In26.Cu" );
    case PCB_LAYER_ID::In27_Cu:           return wxT( "In27.Cu" );
    case PCB_LAYER_ID::In28_Cu:           return wxT( "In28.Cu" );
    case PCB_LAYER_ID::In29_Cu:           return wxT( "In29.Cu" );
    case PCB_LAYER_ID::In30_Cu:           return wxT( "In30.Cu" );
    case PCB_LAYER_ID::B_Cu:              return wxT( "B.Cu" );

    // Technicals
    case PCB_LAYER_ID::B_Adhes:           return wxT( "B.Adhesive" );
    case PCB_LAYER_ID::F_Adhes:           return wxT( "F.Adhesive" );
    case PCB_LAYER_ID::B_Paste:           return wxT( "B.Paste" );
    case PCB_LAYER_ID::F_Paste:           return wxT( "F.Paste" );
    case PCB_LAYER_ID::B_SilkS:           return wxT( "B.Silkscreen" );
    case PCB_LAYER_ID::F_SilkS:           return wxT( "F.Silkscreen" );
    case PCB_LAYER_ID::B_Mask:            return wxT( "B.Mask" );
    case PCB_LAYER_ID::F_Mask:            return wxT( "F.Mask" );

    // Users
    case PCB_LAYER_ID::Dwgs_User:         return wxT( "User.Drawings" );
    case PCB_LAYER_ID::Cmts_User:         return wxT( "User.Comments" );
    case PCB_LAYER_ID::Eco1_User:         return wxT( "User.Eco1" );
    case PCB_LAYER_ID::Eco2_User:         return wxT( "User.Eco2" );
    case PCB_LAYER_ID::Edge_Cuts:         return wxT( "Edge.Cuts" );
    case PCB_LAYER_ID::Margin:            return wxT( "Margin" );

    // Footprint
    case PCB_LAYER_ID::F_CrtYd:           return wxT( "F.Courtyard" );
    case PCB_LAYER_ID::B_CrtYd:           return wxT( "B.Courtyard" );
    case PCB_LAYER_ID::F_Fab:             return wxT( "F.Fab" );
    case PCB_LAYER_ID::B_Fab:             return wxT( "B.Fab" );

    // User definable layers.
    case PCB_LAYER_ID::User_1:            return wxT( "User.1" );
    case PCB_LAYER_ID::User_2:            return wxT( "User.2" );
    case PCB_LAYER_ID::User_3:            return wxT( "User.3" );
    case PCB_LAYER_ID::User_4:            return wxT( "User.4" );
    case PCB_LAYER_ID::User_5:            return wxT( "User.5" );
    case PCB_LAYER_ID::User_6:            return wxT( "User.6" );
    case PCB_LAYER_ID::User_7:            return wxT( "User.7" );
    case PCB_LAYER_ID::User_8:            return wxT( "User.8" );
    case PCB_LAYER_ID::User_9:            return wxT( "User.9" );
    case 57:            return wxT( "User.10" );
    case 59:            return wxT( "User.11" );
    case 61:            return wxT( "User.12" );
    case 63:            return wxT( "User.13" );

    // Rescue
    case PCB_LAYER_ID::Rescue:            return _( "Rescue" );

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
    case LAYER_RULE_AREAS:              return _( "Rule areas" );
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
    case LAYER_EXCLUDED_FROM_SIM:       return _( "Excluded-from-simulation markers" );
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
    case LAYER_NET_COLOR_HIGHLIGHT:     return _( "Net color highlight" );
    case LAYER_SCHEMATIC_DRAWINGSHEET:  return _( "Drawing sheet" );
    case LAYER_SCHEMATIC_PAGE_LIMITS:   return _( "Page limits" );
    case LAYER_OP_VOLTAGES:             return _( "Operating point voltages" );
    case LAYER_OP_CURRENTS:             return _( "Operating point currents" );

    // GAL_LAYER_ID

    case LAYER_FOOTPRINTS_FR:           return _( "Footprints front" );
    case LAYER_FOOTPRINTS_BK:           return _( "Footprints back" );
    case LAYER_FP_VALUES:               return _( "Values" );
    case LAYER_FP_REFERENCES:           return _( "Reference designators" );
    case LAYER_FP_TEXT:                 return _( "Footprint text" );
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
    case LAYER_DRC_SHAPE1:              return _( "DRC shape 1" );
    case LAYER_DRC_SHAPE2:              return _( "DRC shape 2" );
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
    case NETNAMES_LAYER_ID_START:       return _( "Track net names" );
    case LAYER_PAD_NETNAMES:            return _( "Pad net names" );

    default:
        wxCHECK_MSG( false, wxEmptyString, wxString::Format( "Unknown layer ID %d", aLayer ) );
    }
}


PCB_LAYER_ID FlipLayer( PCB_LAYER_ID aLayerId, int aCopperLayersCount )
{
    switch( aLayerId )
    {
    case B_Cu:              return F_Cu;
    case F_Cu:              return B_Cu;

    case B_SilkS:           return F_SilkS;
    case F_SilkS:           return B_SilkS;

    case B_Adhes:           return F_Adhes;
    case F_Adhes:           return B_Adhes;

    case B_Mask:            return F_Mask;
    case F_Mask:            return B_Mask;

    case B_Paste:           return F_Paste;
    case F_Paste:           return B_Paste;

    case B_CrtYd:           return F_CrtYd;
    case F_CrtYd:           return B_CrtYd;

    case B_Fab:             return F_Fab;
    case F_Fab:             return B_Fab;

    default:    // change internal layer if aCopperLayersCount is >= 4
        if( IsCopperLayer( aLayerId ) && aCopperLayersCount >= 4 )
        {
            // internal copper layers count is aCopperLayersCount-2
            PCB_LAYER_ID fliplayer = PCB_LAYER_ID(aCopperLayersCount - 2 - ( aLayerId - In1_Cu ) );
            // Ensure fliplayer has a value which does not crash Pcbnew:
            if( fliplayer < F_Cu )
                fliplayer = F_Cu;

            if( fliplayer > B_Cu )
                fliplayer = B_Cu;

            return fliplayer;
        }

        // No change for the other layers
        return aLayerId;
    }
}


PCB_LAYER_ID BoardLayerFromLegacyId( int aLegacyId )
{
    switch( aLegacyId )
    {
    case 0:  return F_Cu;
    case 31: return B_Cu;

    default:
        if( aLegacyId < 0 )
            return magic_enum::enum_cast<PCB_LAYER_ID>( aLegacyId ).value_or( UNDEFINED_LAYER );

        if( aLegacyId < 31 )
            return static_cast<PCB_LAYER_ID>( In1_Cu + ( aLegacyId - 1 ) * 2 );

        switch( aLegacyId )
        {
        case 32: return B_Adhes;
        case 33: return F_Adhes;
        case 34: return B_Paste;
        case 35: return F_Paste;
        case 36: return B_SilkS;
        case 37: return F_SilkS;
        case 38: return B_Mask;
        case 39: return F_Mask;
        case 40: return Dwgs_User;
        case 41: return Cmts_User;
        case 42: return Eco1_User;
        case 43: return Eco2_User;
        case 44: return Edge_Cuts;
        case 45: return Margin;
        case 46: return B_CrtYd;
        case 47: return F_CrtYd;
        case 48: return B_Fab;
        case 49: return F_Fab;
        case 50: return User_1;
        case 51: return User_2;
        case 52: return User_3;
        case 53: return User_4;
        case 54: return User_5;
        case 55: return User_6;
        case 56: return User_7;
        case 57: return User_8;
        case 58: return User_9;
        case 59: return Rescue;
        default: return UNDEFINED_LAYER;
        }
    }
}
