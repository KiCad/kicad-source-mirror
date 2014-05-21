/**
 * @file class_colors_design_settings.h
 * @brief Handle colors used to draw all items or layers.
 */

#ifndef _COLORS_DESIGN_SETTING_H
#define _COLORS_DESIGN_SETTING_H
#include <layers_id_colors_and_visibility.h>

#define LAYERSCOLORSBUFFERSIZE NB_LAYERS
#define ITEMSCOLORSBUFFERSIZE 32

/**
 * Class COLORS_DESIGN_SETTINGS
 * is a list of color settings for designs in Eeschema, Pcbnew and GerbView
 */
class COLORS_DESIGN_SETTINGS
{
public:
    // Color options for screen display of the Printed Board and schematic:

    // Common to Eeschema, Pcbnew, GerbView
    EDA_COLOR_T m_LayersColors[LAYERSCOLORSBUFFERSIZE]; ///< Layer colors (tracks and graphic items)

    // Common to Eeschema, Pcbnew
    EDA_COLOR_T m_ItemsColors[ITEMSCOLORSBUFFERSIZE];   ///< All others items but layers

public:
    COLORS_DESIGN_SETTINGS();

    /**
     * Function GetLayerColor
     * @return the color for aLayer which is one of the layer indices given
     * in pcbstruct.h or in schematic
     */
    EDA_COLOR_T GetLayerColor( LAYER_NUM aLayer ) const;

    /**
     * Function SetLayerColor
     * sets the color for aLayer which is one of the layer indices given
     * in pcbstruct.h or in schematic
     */
    void SetLayerColor( LAYER_NUM aLayer, EDA_COLOR_T aColor );

    /**
     * Function GetItemColor
     * @return the color for an item which is one of the item indices given
     * in pcbstruct.h, enum PCB_VISIBLE or in schematic
     */
    EDA_COLOR_T GetItemColor( int aItemIdx ) const;

    /**
     * Function SetItemColor
     * sets the color for an item which is one of the item indices given
     * in pcbstruct.h, enum PCB_VISIBLE or in schematic
     */
    void SetItemColor( int aItemIdx, EDA_COLOR_T aColor );

    /**
     * Function SetAllColorsAs
     * sets alls colors to aColor
     * Usefull to create a monochrome color selection for printing purpose
     */
    void SetAllColorsAs( EDA_COLOR_T aColor);
};

#endif  //  _COLORS_DESIGN_SETTING_H
