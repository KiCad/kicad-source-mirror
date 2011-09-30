/**
 * @file class_colors_design_settings.h
 * @brief Handle colors used to draw all items or layers.
 */

#ifndef _COLORS_DESIGN_SETTING_H
#define _COLORS_DESIGN_SETTING_H

#define LAYERSCOLORSBUFFERSIZE 32
#define ITEMSCOLORSBUFFERSIZE 32

/* Class for handle list of color settings for designs in Eeschema, Pcbnew and GerbView
 */
class COLORS_DESIGN_SETTINGS
{
public:
    // Color options for screen display of the Printed Board and schematic:

    // Common to Eeschema, Pcbnew, GerbView
    int m_LayersColors[LAYERSCOLORSBUFFERSIZE];          ///< Layer colors (tracks and graphic items)

    // Common to Eeschema, Pcbnew
    int m_ItemsColors[ITEMSCOLORSBUFFERSIZE];          ///< All others items but layers

public:
    COLORS_DESIGN_SETTINGS( );

    /**
     * Function GetLayerColor
     * @return the color for aLayer which is one of the layer indices given
     * in pcbstruct.h or in schematic
     */
    int GetLayerColor( int aLayer );

    /**
     * Function SetLayerColor
     * sets the color for aLayer which is one of the layer indices given
     * in pcbstruct.h or in schematic
     */
    void SetLayerColor( int aLayer, int aColor );

    /**
     * Function GetItemColor
     * @return the color for an item which is one of the item indices given
     * in pcbstruct.h, enum PCB_VISIBLE or in schematic
     */
    int GetItemColor( int aItemIdx );

    /**
     * Function SetItemColor
     * sets the color for an item which is one of the item indices given
     * in pcbstruct.h, enum PCB_VISIBLE or in schematic
     */
    void SetItemColor(  int aItemIdx, int aColor );

    /**
     * Function SetAllColorsAs
     * sets alls colors to aColor
     * Usefull to create a monochrome color selection for printing purpose
     */
    void SetAllColorsAs( int aColor);
};

#endif  //  _COLORS_DESIGN_SETTING_H
