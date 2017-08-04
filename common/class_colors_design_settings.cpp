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
    CYAN,      // LAYER_VIA_MICROVIA
    BROWN,     // LAYER_VIA_BBLIND
    LIGHTGRAY, // LAYER_VIA_THROUGH
    YELLOW,    // LAYER_NON_PLATED
    LIGHTGRAY, // LAYER_MOD_TEXT_FR
    BLUE,      // LAYER_MOD_TEXT_BK
    DARKGRAY,  // LAYER_MOD_TEXT_INVISIBLE
    BLUE,      // LAYER_ANCHOR
    RED,       // LAYER_PAD_FR
    GREEN,     // LAYER_PAD_BK
    LIGHTGRAY, // LAYER_RATSNEST
    DARKGRAY,  // LAYER_GRID
    LIGHTRED,  LIGHTGRAY, LIGHTGRAY, LIGHTGRAY,
    LIGHTGRAY, LIGHTGRAY, LIGHTGRAY, LIGHTGRAY,
    LIGHTGRAY, LIGHTGRAY, LIGHTGRAY, LIGHTGRAY,
    LIGHTGRAY, LIGHTGRAY, LIGHTGRAY, LIGHTGRAY,
    LIGHTGRAY, LIGHTGRAY, LIGHTGRAY
};


COLORS_DESIGN_SETTINGS::COLORS_DESIGN_SETTINGS()
{
    for( unsigned src = 0, dst = 0; dst < DIM( m_LayersColors ); ++dst )
    {
        m_LayersColors[dst] = COLOR4D( default_layer_color[src++] );

        if( src >= DIM( default_layer_color ) )
            src = 0;        // wrap the source.
    }

    for( unsigned src = 0, dst = LAYER_VIAS; dst < DIM( default_items_color ); ++dst )
    {
        m_LayersColors[dst] = COLOR4D( default_items_color[src++] );
    }

    m_LayersColors[ LAYER_PCB_BACKGROUND ] = BLACK;
    m_LayersColors[ LAYER_CURSOR ] = WHITE;
    m_LayersColors[ LAYER_AUX_ITEMS ] = WHITE;
    m_LayersColors[ LAYER_WORKSHEET ] = DARKRED;

    setupConfigParams();
}


COLOR4D COLORS_DESIGN_SETTINGS::GetLayerColor( LAYER_NUM aLayer ) const
{
    if( (unsigned) aLayer < DIM( m_LayersColors ) )
    {
        return m_LayersColors[aLayer];
    }
    return COLOR4D::UNSPECIFIED;
}


void COLORS_DESIGN_SETTINGS::SetLayerColor( LAYER_NUM aLayer, COLOR4D aColor )
{
    if( (unsigned) aLayer < DIM( m_LayersColors ) )
    {
        m_LayersColors[aLayer] = aColor;
    }
}


COLOR4D COLORS_DESIGN_SETTINGS::GetItemColor( int aItemIdx ) const
{
    if( (unsigned) aItemIdx < DIM( m_LayersColors ) )
    {
        return m_LayersColors[aItemIdx];
    }

    return COLOR4D::UNSPECIFIED;
}


void COLORS_DESIGN_SETTINGS::SetItemColor( int aItemIdx, COLOR4D aColor )
{
    if( (unsigned) aItemIdx < DIM( m_LayersColors ) )
    {
        m_LayersColors[aItemIdx] = aColor;
    }
}


void COLORS_DESIGN_SETTINGS::SetAllColorsAs( COLOR4D aColor )
{
    for( unsigned ii = 0; ii < DIM(m_LayersColors); ii++ )
        m_LayersColors[ii] = aColor;
}

#define LOC_COLOR(layer)            &m_LayersColors[layer]
#define ITEM_COLOR(item_visible)    &m_LayersColors[item_visible]

void COLORS_DESIGN_SETTINGS::setupConfigParams()
{
    wxASSERT( DIM( m_LayersColors ) >= PCB_LAYER_ID_COUNT );
    for( int i = 0;  i<PCB_LAYER_ID_COUNT;  ++i )
    {
        wxString vn = wxString::Format(
                        wxT( "ColorPCBLayer_%s" ),
                        LSET::Name( PCB_LAYER_ID( i ) ) );

        Add( vn, LOC_COLOR(i), m_LayersColors[i] );
    }

    Add( wxT( "ColorTxtFrontEx" ), ITEM_COLOR( LAYER_MOD_TEXT_FR ), LIGHTGRAY );
    Add( wxT( "ColorTxtBackEx" ), ITEM_COLOR( LAYER_MOD_TEXT_BK ), BLUE );
    Add( wxT( "ColorTxtInvisEx" ), ITEM_COLOR( LAYER_MOD_TEXT_INVISIBLE ), DARKGRAY );
    Add( wxT( "ColorPadBackEx" ), ITEM_COLOR( LAYER_PAD_BK ), GREEN );
    Add( wxT( "ColorAnchorEx" ), ITEM_COLOR( LAYER_ANCHOR ), BLUE );
    Add( wxT( "ColorPadFrontEx" ), ITEM_COLOR( LAYER_PAD_FR ), RED );
    Add( wxT( "ColorViaThruEx" ), ITEM_COLOR( LAYER_VIA_THROUGH ), LIGHTGRAY );
    Add( wxT( "ColorViaBBlindEx" ), ITEM_COLOR( LAYER_VIA_BBLIND ), BROWN );
    Add( wxT( "ColorViaMicroEx" ), ITEM_COLOR( LAYER_VIA_MICROVIA ), CYAN );
    Add( wxT( "ColorNonPlatedEx" ), ITEM_COLOR( LAYER_NON_PLATED ), YELLOW );
    Add( wxT( "ColorRatsEx" ), ITEM_COLOR( LAYER_RATSNEST ), WHITE );
    Add( wxT( "ColorPCBBackground" ), ITEM_COLOR( LAYER_PCB_BACKGROUND ), BLACK );
    Add( wxT( "ColorPCBCursor" ), ITEM_COLOR( LAYER_CURSOR ), WHITE );
    Add( wxT( "ColorAuxItems" ), ITEM_COLOR( LAYER_AUX_ITEMS ), WHITE );
    Add( wxT( "ColorWorksheet" ), ITEM_COLOR( LAYER_WORKSHEET ), DARKRED );

}

void COLORS_DESIGN_SETTINGS::Load( wxConfigBase *aConfig )
{
    SETTINGS::Load(aConfig);
}

void COLORS_DESIGN_SETTINGS::Save( wxConfigBase *aConfig )
{
    SETTINGS::Save(aConfig);
}
