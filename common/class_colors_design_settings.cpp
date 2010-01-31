/****************************************************/
/*  class_colors_design_settings.cpp                  */
/*  handle colors used to draw  all items or layers */
/****************************************************/
#include "fctsys.h"
#include "colors.h"
#include "macros.h"

#include "class_colors_design_settings.h"

/* Class for handle list of color settings for designs
 * in Eeschema, Pcbnew and gerbview
 */

/* Initial colors values: optimized for pcbnew, but are also Ok for eeschema
 * these values are superseded by config reading
 */
static const int default_layer_color[LAYERSCOLORSBUFFERSIZE] =
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

static const int default_items_color[LAYERSCOLORSBUFFERSIZE] =
{
    LIGHTGRAY,  // unused
    CYAN,       // VIA_MICROVIA_VISIBLE
    BROWN,      // VIA_BBLIND_VISIBLE
    LIGHTGRAY,  // VIA_THROUGH_VISIBLE
    LIGHTGRAY,  // MOD_TEXT_FR_VISIBLE
    BLUE,      // MOD_TEXT_BK_VISIBLE
    DARKGRAY,  // MOD_TEXT_INVISIBLE
    BLUE,      // ANCHOR_VISIBLE
    RED,       // PAD_FR_VISIBLE
    GREEN,     // PAD_BK_VISIBLE
    LIGHTGRAY, // RATSNEST_VISIBLE
    DARKGRAY,  //GRID_VISIBLE
    LIGHTRED,  LIGHTGRAY, LIGHTGRAY, LIGHTGRAY,
    LIGHTGRAY, LIGHTGRAY,
    LIGHTGRAY, LIGHTGRAY,
    LIGHTGRAY, LIGHTGRAY,
    LIGHTGRAY, LIGHTGRAY,
    LIGHTGRAY,
    LIGHTGRAY,
    LIGHTGRAY,
    LIGHTGRAY,
    LIGHTGRAY,
    LIGHTGRAY,
    LIGHTGRAY,
    LIGHTGRAY
};

COLORS_DESIGN_SETTINGS:: COLORS_DESIGN_SETTINGS()
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
int COLORS_DESIGN_SETTINGS::GetLayerColor( int aLayer )
{
    if( (unsigned) aLayer < DIM(m_LayersColors) )
    {
        return m_LayersColors[aLayer];
    }
    return -1;
}


/**
 * Function SetLayerColor
 * sets the color for aLayer which is one of the layer indices given
 * in pcbstruct.h or in schematic
 */
void COLORS_DESIGN_SETTINGS::SetLayerColor( int aLayer, int aColor )
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
int COLORS_DESIGN_SETTINGS::GetItemColor( int aItemIdx )
{
    if( (unsigned) aItemIdx < DIM(m_ItemsColors) )
    {
        return m_ItemsColors[aItemIdx];
    }
    return -1;
}


/**
 * Function SetItemColor
 * sets the color for an item which is one of the item indices given
 * in pcbstruct.h, enum PCB_VISIBLE or in schematic
 */
void COLORS_DESIGN_SETTINGS::SetItemColor( int aItemIdx, int aColor )
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
void COLORS_DESIGN_SETTINGS::SetAllColorsAs( int aColor)
{
    for( unsigned ii = 0; ii < DIM(m_LayersColors); ii++ )
        m_LayersColors[ii] = aColor;

    for( unsigned ii = 0; ii < DIM(m_ItemsColors); ii++ )
        m_ItemsColors[ii] = aColor;
}
