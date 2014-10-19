/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2014 Jean-Pierre Charras, jp.charras at wanadoo.fr
 * Copyright (C) 2014 KiCad Developers, see CHANGELOG.TXT for contributors.
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
 * @file class_colors_design_settings.cpp
 * @brief Handle colors used to draw all items or layers.
 */
#include <fctsys.h>
#include <colors.h>
#include <macros.h>

#include <class_colors_design_settings.h>

/* Class for handle list of color settings for designs
 * in Eeschema, Pcbnew and GerbView
 */

/* Initial colors values: optimized for Pcbnew 64 layers.
 * The table is not actually used by Eeschema.
 * these values are superseded by config reading
 */
static const EDA_COLOR_T default_layer_color[] = {
    RED,            YELLOW,         LIGHTMAGENTA,   LIGHTRED,
    CYAN,           GREEN,          BLUE,           DARKGRAY,
    MAGENTA,        LIGHTGRAY,      MAGENTA,        RED,
    BROWN,          LIGHTGRAY,      BLUE,           GREEN,

    RED,            YELLOW,         LIGHTMAGENTA,   LIGHTRED,
    CYAN,           GREEN,          BLUE,           DARKGRAY,
    MAGENTA,        LIGHTGRAY,      MAGENTA,        RED,
    BROWN,          LIGHTGRAY,      BLUE,           GREEN,

    BLUE,         MAGENTA,
    LIGHTCYAN,    RED,
    MAGENTA,      CYAN,
    BROWN,        MAGENTA,
    LIGHTGRAY,
    BLUE,
    GREEN,        YELLOW,
    YELLOW,
    LIGHTMAGENTA,
    YELLOW,
    DARKGRAY
};


static const EDA_COLOR_T default_items_color[] = {
    LIGHTGRAY, // unused
    CYAN,      // VIA_MICROVIA_VISIBLE
    BROWN,     // VIA_BBLIND_VISIBLE
    LIGHTGRAY, // VIA_THROUGH_VISIBLE
    YELLOW,    // NON_PLATED_VISIBLE
    LIGHTGRAY, // MOD_TEXT_FR_VISIBLE
    BLUE,      // MOD_TEXT_BK_VISIBLE
    DARKGRAY,  // MOD_TEXT_INVISIBLE
    BLUE,      // ANCHOR_VISIBLE
    RED,       // PAD_FR_VISIBLE
    GREEN,     // PAD_BK_VISIBLE
    LIGHTGRAY, // RATSNEST_VISIBLE
    DARKGRAY,  // GRID_VISIBLE
    LIGHTRED,  LIGHTGRAY, LIGHTGRAY, LIGHTGRAY,
    LIGHTGRAY, LIGHTGRAY, LIGHTGRAY, LIGHTGRAY,
    LIGHTGRAY, LIGHTGRAY, LIGHTGRAY, LIGHTGRAY,
    LIGHTGRAY, LIGHTGRAY, LIGHTGRAY, LIGHTGRAY,
    LIGHTGRAY, LIGHTGRAY, LIGHTGRAY
};


COLORS_DESIGN_SETTINGS::COLORS_DESIGN_SETTINGS()
{
    for( unsigned src = 0, dst = 0; dst < DIM(m_LayersColors); ++dst )
    {
        m_LayersColors[dst] = default_layer_color[src++];

        if( src >= DIM( default_layer_color ) )
            src = 0;        // wrap the source.
    }

    for( unsigned src = 0, dst = 0; dst < DIM(m_ItemsColors);  ++dst )
    {
        m_ItemsColors[dst] = default_items_color[src++];

        if( src >= DIM( default_items_color ) )
            src = 0;
    }
}


EDA_COLOR_T COLORS_DESIGN_SETTINGS::GetLayerColor( LAYER_NUM aLayer ) const
{
    if( (unsigned) aLayer < DIM(m_LayersColors) )
    {
        return m_LayersColors[aLayer];
    }
    return UNSPECIFIED_COLOR;
}


void COLORS_DESIGN_SETTINGS::SetLayerColor( LAYER_NUM aLayer, EDA_COLOR_T aColor )
{
    if( (unsigned) aLayer < DIM(m_LayersColors) )
    {
        m_LayersColors[aLayer] = aColor;
    }
}


EDA_COLOR_T COLORS_DESIGN_SETTINGS::GetItemColor( int aItemIdx ) const
{
    if( (unsigned) aItemIdx < DIM( m_ItemsColors ) )
    {
        return m_ItemsColors[aItemIdx];
    }

    return UNSPECIFIED_COLOR;
}


void COLORS_DESIGN_SETTINGS::SetItemColor( int aItemIdx, EDA_COLOR_T aColor )
{
    if( (unsigned) aItemIdx < DIM(m_ItemsColors) )
    {
        m_ItemsColors[aItemIdx] = aColor;
    }
}


void COLORS_DESIGN_SETTINGS::SetAllColorsAs( EDA_COLOR_T aColor )
{
    for( unsigned ii = 0; ii < DIM(m_LayersColors); ii++ )
        m_LayersColors[ii] = aColor;

    for( unsigned ii = 0; ii < DIM(m_ItemsColors); ii++ )
        m_ItemsColors[ii] = aColor;
}
