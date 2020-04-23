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


wxString LayerName( SCH_LAYER_ID aLayer )
{
    switch( aLayer )
    {
    case LAYER_WIRE:
        return _( "Wire" );

    case LAYER_BUS:
        return _( "Bus" );

    case LAYER_BUS_JUNCTION:
        return _( "Bus Junction" );

    case LAYER_JUNCTION:
        return _( "Junction" );

    case LAYER_LOCLABEL:
        return _( "Label" );

    case LAYER_GLOBLABEL:
        return _( "Global label" );

    case LAYER_HIERLABEL:
        return _( "Hierarchical label" );

    case LAYER_PINNUM:
        return _( "Pin number" );

    case LAYER_PINNAM:
        return _( "Pin name" );

    case LAYER_REFERENCEPART:
        return _( "Symbol reference" );

    case LAYER_VALUEPART:
        return _( "Symbol value" );

    case LAYER_FIELDS:
        return _( "Symbol fields" );

    case LAYER_DEVICE:
        return _( "Symbol body outline" );

    case LAYER_DEVICE_BACKGROUND:
        return _( "Symbol body fill" );

    case LAYER_NOTES:
        return _( "Notes" );

    case LAYER_NETNAM:
        return _( "Net name" );

    case LAYER_PIN:
        return _( "Pin" );

    case LAYER_SHEET:
        return _( "Sheet border" );

    case LAYER_SHEET_BACKGROUND:
        return _( "Sheet background" );

    case LAYER_SHEETNAME:
        return _( "Sheet name" );

    case LAYER_SHEETFIELDS:
        return _( "Sheet fields" );

    case LAYER_SHEETFILENAME:
        return _( "Sheet file name" );

    case LAYER_SHEETLABEL:
        return _( "Sheet label" );

    case LAYER_NOCONNECT:
        return _( "No connect symbol" );

    case LAYER_ERC_WARN:
        return _( "ERC warning" );

    case LAYER_ERC_ERR:
        return _( "ERC error" );

    case LAYER_SCHEMATIC_GRID:
        return _( "Grid" );

    case LAYER_SCHEMATIC_GRID_AXES:
        return _( "Axes" );

    case LAYER_SCHEMATIC_BACKGROUND:
        return _( "Background" );

    case LAYER_SCHEMATIC_CURSOR:
        return _( "Cursor" );

    case LAYER_BRIGHTENED:
        return _( "Highlighted items" );

    case LAYER_HIDDEN:
        return _( "Hidden item" );

    case LAYER_SELECTION_SHADOWS:
        return _( "Selection highlight" );

    case LAYER_SCHEMATIC_WORKSHEET:
        return _( "Worksheet" );

    default:
        return wxEmptyString;
    }
}