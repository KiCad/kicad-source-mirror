/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2016 Jean-Pierre Charras, jp.charras at wanadoo.fr
 * Copyright (C) 2016 KiCad Developers, see CHANGELOG.TXT for contributors.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, you may find one here:
 * http://www.gnu.org/licenses/old-licenses/gpl-2.0.html
 * or you may search the http://www.gnu.org website for the version 2 license,
 * or you may write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
 */

/**
 * @file class_colors_design_settings.h
 * @brief Handle colors used to draw all items or layers.
 */

#ifndef COLORS_DESIGN_SETTING_H
#define COLORS_DESIGN_SETTING_H
#include <gal/color4d.h>
#include <layers_id_colors_and_visibility.h>

using KIGFX::COLOR4D;

/**
 * Class COLORS_DESIGN_SETTINGS
 * is a list of color settings for designs in Eeschema, Pcbnew and GerbView
 */
class COLORS_DESIGN_SETTINGS
{
public:
    // Color options for screen display of the Printed Board and schematic:

    // Common to Eeschema, Pcbnew, GerbView
    COLOR4D m_LayersColors[LAYER_ID_COUNT];     ///< Layer colors (tracks and graphic items)

    // Common to Eeschema, Pcbnew
    COLOR4D m_ItemsColors[32];                  ///< All others items but layers

public:
    COLORS_DESIGN_SETTINGS();

    /**
     * Function GetLayerColor
     * @return the color for aLayer which is one of the layer indices given
     * in pcbstruct.h or in schematic
     */
    COLOR4D GetLayerColor( LAYER_NUM aLayer ) const;

    /**
     * Function SetLayerColor
     * sets the color for aLayer which is one of the layer indices given
     * in pcbstruct.h or in schematic
     */
    void SetLayerColor( LAYER_NUM aLayer, COLOR4D aColor );

    /**
     * Function GetItemColor
     * @return the color for an item which is one of the item indices given
     * in pcbstruct.h, enum PCB_VISIBLE or in schematic
     */
    COLOR4D GetItemColor( int aItemIdx ) const;

    /**
     * Function SetItemColor
     * sets the color for an item which is one of the item indices given
     * in pcbstruct.h, enum PCB_VISIBLE or in schematic
     */
    void SetItemColor( int aItemIdx, COLOR4D aColor );

    /**
     * Function SetAllColorsAs
     * sets alls colors to aColor
     * Usefull to create a monochrome color selection for printing purpose
     */
    void SetAllColorsAs( COLOR4D aColor );
};

#endif  //  COLORS_DESIGN_SETTING_H
