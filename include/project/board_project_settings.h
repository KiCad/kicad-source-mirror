/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2020 Jon Evans <jon@craftyjon.com>
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

#ifndef KICAD_BOARD_PROJECT_SETTINGS_H
#define KICAD_BOARD_PROJECT_SETTINGS_H

#include <layers_id_colors_and_visibility.h>

/**
 * This file contains data structures that are saved in the project file or project local settings
 * file that are specific to PcbNew.  This is done so that these structures are available in common.
 */


/**
 * Selection filtering that applies all the time (not the "filter selection" dialog that modifies
 * the current selection)
 */
struct SELECTION_FILTER_OPTIONS
{
    bool lockedItems;   ///< Allow selecting locked items
    bool footprints;    ///< Allow selecting entire footprints
    bool text;          ///< Text (free or attached to a footprint)
    bool tracks;        ///< Copper tracks
    bool vias;          ///< Vias (all types>
    bool pads;          ///< Footprint pads
    bool graphics;      ///< Graphic lines, shapes, polygons
    bool zones;         ///< Copper zones
    bool keepouts;      ///< Keepout zones
    bool dimensions;    ///< Dimension items
    bool otherItems;    ///< Anything not fitting one of the above categories

    SELECTION_FILTER_OPTIONS()
    {
        lockedItems = true;
        footprints  = true;
        text        = true;
        tracks      = true;
        vias        = true;
        pads        = true;
        graphics    = true;
        zones       = true;
        keepouts    = true;
        dimensions  = true;
        otherItems  = true;
    }

    bool Any()
    {
        return ( lockedItems || footprints || text || tracks || vias || pads || graphics || zones
                 || keepouts || dimensions || otherItems );
    }

    bool All()
    {
        return ( lockedItems && footprints && text && tracks && vias && pads && graphics && zones
                 && keepouts && dimensions && otherItems );
    }
};

/**
     * Determines how inactive layers should be displayed
     */
enum class HIGH_CONTRAST_MODE
{
    NORMAL = 0,     ///> Non-active layers are shown normally (no high-contrast mode)
    DIMMED,         ///> Non-active layers are dimmed (old high-contrast mode)
    HIDDEN          ///> Non-active layers are hidden
};

/**
 * A saved set of layers that are visible
 */
struct LAYER_PRESET
{
    wxString     name;          ///< A name for this layer set
    LSET         layers;        ///< Board layers that are visible
    GAL_SET      renderLayers;  ///< Render layers (e.g. object types) that are visible
    PCB_LAYER_ID activeLayer;   ///< Optional layer to set active when this preset is loaded

    LAYER_PRESET( const wxString& aName ) :
            name( aName ),
            activeLayer( UNSELECTED_LAYER )
    {
    }

    LAYER_PRESET( const wxString& aName, const LSET& aSet ) :
            name( aName ),
            layers( aSet ),
            activeLayer( UNSELECTED_LAYER )
    {
    }

    LAYER_PRESET( const wxString& aName, const LSET& aSet, PCB_LAYER_ID aActive ) :
            name( aName ),
            layers( aSet ),
            activeLayer( aActive )
    {
    }

    bool LayersMatch( const LAYER_PRESET& aOther )
    {
        return aOther.layers == layers && aOther.renderLayers == renderLayers;
    }
};

#endif // KICAD_BOARD_PROJECT_SETTINGS_H
