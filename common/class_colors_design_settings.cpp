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

/* Initial colors values: optimized for Pcbnew, but are also Ok for Eeschema
 * these values are superseded by config reading
 */
static const EDA_COLOR_T default_layer_color[LAYERSCOLORSBUFFERSIZE] =
{
    GREEN,        BLUE,         LIGHTGRAY, BROWN,
    RED,          MAGENTA,      LIGHTGRAY, MAGENTA,
    DARKGRAY,     BLUE,         GREEN,     CYAN,
    LIGHTRED,     LIGHTMAGENTA, YELLOW,    RED,
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

static const EDA_COLOR_T default_items_color[LAYERSCOLORSBUFFERSIZE] =
{
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
    for( unsigned ii = 0; ii < DIM(m_LayersColors); ii++ )
        m_LayersColors[ii] = default_layer_color[ii];

    for( unsigned ii = 0; ii < DIM(m_ItemsColors); ii++ )
        m_ItemsColors[ii] = default_items_color[ii];
}


/**
 * Function GetLayerColor
 * @return the color for aLayer which is one of the layer indices given
 * in pcbstruct.h or in schematic
 */
EDA_COLOR_T COLORS_DESIGN_SETTINGS::GetLayerColor( LAYER_NUM aLayer ) const
{
    if( (unsigned) aLayer < DIM(m_LayersColors) )
    {
        return m_LayersColors[aLayer];
    }
    return UNSPECIFIED_COLOR;
}


/**
 * Function SetLayerColor
 * sets the color for aLayer which is one of the layer indices given
 * in pcbstruct.h or in schematic
 */
void COLORS_DESIGN_SETTINGS::SetLayerColor( LAYER_NUM aLayer, EDA_COLOR_T aColor )
{
    if( (unsigned) aLayer < DIM(m_LayersColors) )
    {
        m_LayersColors[aLayer] = aColor;
    }
}


/**
 * Function GetItemColor
 * @return the color for an item which is one of the item indices given
 * in pcbstruct.h, enum PCB_VISIBLE or in schematic
 */
EDA_COLOR_T COLORS_DESIGN_SETTINGS::GetItemColor( int aItemIdx ) const
{
    if( (unsigned) aItemIdx < DIM( m_ItemsColors ) )
    {
        return m_ItemsColors[aItemIdx];
    }

    return UNSPECIFIED_COLOR;
}


/**
 * Function SetItemColor
 * sets the color for an item which is one of the item indices given
 * in pcbstruct.h, enum PCB_VISIBLE or in schematic
 */
void COLORS_DESIGN_SETTINGS::SetItemColor( int aItemIdx, EDA_COLOR_T aColor )
{
    if( (unsigned) aItemIdx < DIM(m_ItemsColors) )
    {
        m_ItemsColors[aItemIdx] = aColor;
    }
}


/**
 * Function SetAllColorsAs
 * sets alls colors to aColor
 * Usefull to create a monochrome color selection for printing purpose
 */
void COLORS_DESIGN_SETTINGS::SetAllColorsAs( EDA_COLOR_T aColor )
{
    for( unsigned ii = 0; ii < DIM(m_LayersColors); ii++ )
        m_LayersColors[ii] = aColor;

    for( unsigned ii = 0; ii < DIM(m_ItemsColors); ii++ )
        m_ItemsColors[ii] = aColor;
}
