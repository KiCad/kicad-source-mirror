/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2014 Jean-Pierre Charras, jp.charras at wanadoo.fr
 * Copyright (C) 2014 KiCad Developers, see AUTHORS.txt for contributors.
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
 * @file colors_design_settings.cpp
 * @brief Handle colors used to draw all items or layers.
 */
#include <fctsys.h>
#include <macros.h>

#include <colors_design_settings.h>

/* Class for handle list of color settings for designs
 * in Eeschema, Pcbnew and GerbView
 */

/* Initial colors values: optimized for Pcbnew up to 64 layers.
 * The table is not actually used by Eeschema.
 * these values are superseded by config reading
 */
static const EDA_COLOR_T default_layer_color[] = {
    // Copper layers
    RED,            YELLOW,         LIGHTMAGENTA,   LIGHTRED,
    CYAN,           GREEN,          BLUE,           DARKGRAY,
    MAGENTA,        LIGHTGRAY,      MAGENTA,        RED,
    BROWN,          LIGHTGRAY,      BLUE,           GREEN,

    RED,            YELLOW,         LIGHTMAGENTA,   LIGHTRED,
    CYAN,           GREEN,          BLUE,           DARKGRAY,
    MAGENTA,        LIGHTGRAY,      MAGENTA,        RED,
    BROWN,          LIGHTGRAY,      BLUE,           GREEN,

    // tech layers
    BLUE,         MAGENTA,  // B_Adhes, F_Adhes
    LIGHTCYAN,    RED,      // B_Paste, F_Paste
    MAGENTA,      CYAN,     // B_SilkS, F_SilkS
    BROWN,        MAGENTA,  // B_Mask, F_Mask

    // user layers
    LIGHTGRAY,    BLUE, GREEN, YELLOW,  // Dwgs_User, Cmts_User, Eco1_User, Eco2_User

    // Special layers
    YELLOW,                     // Edge_Cuts
    LIGHTMAGENTA,               // Margin
    DARKGRAY,     LIGHTGRAY,    // B_CrtYd, F_CrtYd,
    BLUE,         DARKGRAY      // B_Fab, F_Fab
};


// for color order, see enum GAL_LAYER_ID
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
    LIGHTRED,  // LAYER_GRID_AXES
    BLUE,      // LAYER_NO_CONNECTS
    LIGHTGRAY, LIGHTGRAY,   // LAYER_MOD_FR, LAYER_MOD_BK
    LIGHTGRAY, LIGHTGRAY,   // LAYER_MOD_VALUES, LAYER_MOD_REFERENCES
    LIGHTGRAY, // LAYER_TRACKS
    YELLOW,    // LAYER_PADS
    LIGHTGRAY, LIGHTGRAY, LIGHTGRAY, LIGHTGRAY,
    LIGHTGRAY, LIGHTGRAY, LIGHTGRAY, LIGHTGRAY,
    LIGHTGRAY, LIGHTGRAY, LIGHTGRAY
};


COLORS_DESIGN_SETTINGS::COLORS_DESIGN_SETTINGS( FRAME_T aFrameType )
{
    m_frameType = aFrameType;
    m_legacyMode = false;

    for( unsigned src = 0, dst = 0; dst < arrayDim( m_LayersColors ); ++dst )
    {
        m_LayersColors[dst] = COLOR4D( default_layer_color[src++] );

        if( src >= arrayDim( default_layer_color ) )
            src = 0;        // wrap the source.
    }

    for( unsigned src = 0, dst = LAYER_VIAS; src < arrayDim( default_items_color ); ++dst, ++src )
    {
        m_LayersColors[dst] = COLOR4D( default_items_color[src] );
    }

    m_LayersColors[ LAYER_PCB_BACKGROUND ] = BLACK;
    m_LayersColors[ LAYER_CURSOR ] = WHITE;
    m_LayersColors[ LAYER_AUX_ITEMS ] = WHITE;
    m_LayersColors[ LAYER_WORKSHEET ] = DARKRED;
    m_LayersColors[ LAYER_GRID ] = DARKGRAY;

    setupConfigParams();
}


COLOR4D COLORS_DESIGN_SETTINGS::GetLayerColor( LAYER_NUM aLayer ) const
{
    if( (unsigned) aLayer < arrayDim( m_LayersColors ) )
    {
        return m_legacyMode ? m_LayersColors[aLayer].AsLegacyColor()
                            : m_LayersColors[aLayer];
    }
    return COLOR4D::UNSPECIFIED;
}


void COLORS_DESIGN_SETTINGS::SetLayerColor( LAYER_NUM aLayer, COLOR4D aColor )
{
    if( (unsigned) aLayer < arrayDim( m_LayersColors ) )
    {
        m_LayersColors[aLayer] = aColor;
    }
}


COLOR4D COLORS_DESIGN_SETTINGS::GetItemColor( int aItemIdx ) const
{
    if( (unsigned) aItemIdx < arrayDim( m_LayersColors ) )
    {
        return m_legacyMode ? m_LayersColors[aItemIdx].AsLegacyColor()
                            : m_LayersColors[aItemIdx];
    }

    return COLOR4D::UNSPECIFIED;
}


void COLORS_DESIGN_SETTINGS::SetItemColor( int aItemIdx, COLOR4D aColor )
{
    if( (unsigned) aItemIdx < arrayDim( m_LayersColors ) )
    {
        m_LayersColors[aItemIdx] = aColor;
    }
}


void COLORS_DESIGN_SETTINGS::SetAllColorsAs( COLOR4D aColor )
{
    for( unsigned ii = 0; ii < arrayDim(m_LayersColors); ii++ )
        m_LayersColors[ii] = aColor;
}

#define LOC_COLOR(layer)            &m_LayersColors[layer]
#define ITEM_COLOR(item_visible)    &m_LayersColors[item_visible]

void COLORS_DESIGN_SETTINGS::setupConfigParams()
{
    wxASSERT( arrayDim( m_LayersColors ) >= PCB_LAYER_ID_COUNT );

    wxString currprefix = GetConfigPrefix();

    switch( m_frameType )
    {
    case FRAME_GERBER:
    case FRAME_PCB:                      /* no prefix */               break;

    case FRAME_CVPCB_DISPLAY:
    case FRAME_PCB_MODULE_VIEWER:
    case FRAME_PCB_MODULE_VIEWER_MODAL:
    case FRAME_PCB_FOOTPRINT_WIZARD:
    case FRAME_PCB_FOOTPRINT_PREVIEW:
    case FRAME_PCB_MODULE_EDITOR:        SetConfigPrefix( "ModEdit" ); break;

    case FRAME_PCB_DISPLAY3D:            SetConfigPrefix( "fp3d_" );   break;

    default:                                                           break;
    }

    wxString fmt( "Color4DPCBLayer_%s" );

    for( int i = 0; i < PCB_LAYER_ID_COUNT;  ++i )
    {
        wxString cfgkey = wxString::Format( fmt, LSET::Name( PCB_LAYER_ID( i ) ) );
        Add( cfgkey, LOC_COLOR(i), m_LayersColors[i] );
    }

    Add( "Color4DTxtFrontEx", ITEM_COLOR( LAYER_MOD_TEXT_FR ), LIGHTGRAY );
    Add( "Color4DTxtBackEx", ITEM_COLOR( LAYER_MOD_TEXT_BK ), BLUE );
    Add( "Color4DTxtInvisEx", ITEM_COLOR( LAYER_MOD_TEXT_INVISIBLE ), DARKGRAY );
    Add( "Color4DPadBackEx", ITEM_COLOR( LAYER_PAD_BK ), GREEN );
    Add( "Color4DAnchorEx", ITEM_COLOR( LAYER_ANCHOR ), BLUE );
    Add( "Color4DPadFrontEx", ITEM_COLOR( LAYER_PAD_FR ), RED );
    Add( "Color4DPadThruHoleEx", ITEM_COLOR( LAYER_PADS_TH ), YELLOW );
    Add( "Color4DNonPlatedEx", ITEM_COLOR( LAYER_NON_PLATEDHOLES ), YELLOW );
    Add( "Color4DPCBBackground", ITEM_COLOR( LAYER_PCB_BACKGROUND ), BLACK );
    Add( "Color4DPCBCursor", ITEM_COLOR( LAYER_CURSOR ), WHITE );
    Add( "Color4DAuxItems", ITEM_COLOR( LAYER_AUX_ITEMS ), WHITE );
    Add( "Color4DWorksheet", ITEM_COLOR( LAYER_WORKSHEET ), DARKRED );
    Add( "Color4DGrid", ITEM_COLOR( LAYER_GRID ), DARKGRAY );


    // Add prms only relevant in board editor
    if( m_frameType == FRAME_PCB )
    {
        Add( "Color4DViaThruEx", ITEM_COLOR( LAYER_VIA_THROUGH ), LIGHTGRAY );
        Add( "Color4DViaBBlindEx", ITEM_COLOR( LAYER_VIA_BBLIND ), BROWN );
        Add( "Color4DViaMicroEx", ITEM_COLOR( LAYER_VIA_MICROVIA ), CYAN );
        Add( "Color4DRatsEx", ITEM_COLOR( LAYER_RATSNEST ), WHITE );
        Add( "Color4DNoNetPadMarker", ITEM_COLOR( LAYER_NO_CONNECTS ), BLUE );
    }

    SetConfigPrefix( currprefix );
}

void COLORS_DESIGN_SETTINGS::Load( wxConfigBase *aConfig )
{
    SETTINGS::Load(aConfig);
}

void COLORS_DESIGN_SETTINGS::Save( wxConfigBase *aConfig )
{
    SETTINGS::Save(aConfig);
}
