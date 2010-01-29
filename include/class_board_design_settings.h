/**********************************************************/
/*  class_board_design_settings.h :  handle board options */
/**********************************************************/

#ifndef _BOARD_DESIGN_SETTING_H
#define _BOARD_DESIGN_SETTING_H

#include "pcbstruct.h"      // NB_COLORS

// Class for handle current printed board design settings
class EDA_BoardDesignSettings
{
protected:
    int    m_CopperLayerCount;                          // Number of copper layers for this design
public:
    bool   m_MicroViasAllowed;                          // true to allow micro vias
    int    m_CurrentViaType;                            // via type (VIA_BLIND_BURIED, VIA_TROUGHT VIA_MICROVIA)
    bool   m_UseConnectedTrackWidth;                    // if true, when creating a new track starting on an existing track, use this track width
    int    m_DrawSegmentWidth;                          // current graphic line width (not EDGE layer)
    int    m_EdgeSegmentWidth;                          // current graphic line width (EDGE layer only)
    int    m_PcbTextWidth;                              // current Pcb (not module) Text width
    wxSize m_PcbTextSize;                               // current Pcb (not module) Text size
    int    m_TrackMinWidth;                             // track min value for width ((min copper size value
    int    m_ViasMinSize;                               // vias (not micro vias) min diameter
    int    m_ViasMinDrill;                              // vias (not micro vias) min drill diameter
    int    m_MicroViasMinSize;                          // micro vias (not vias) min diameter
    int    m_MicroViasMinDrill;                         // micro vias (not vias) min drill diameter
    // Global mask margins:
    int    m_SolderMaskMargin;                          // Solder mask margin
    int    m_SolderPasteMargin;                         // Solder paste margin absolute value
    double m_SolderPasteMarginRatio;                    // Solder pask margin ratio value of pad size
                                                        // The final margin is the sum of these 2 values
    int    m_LayerThickness;                            // Layer Thickness for 3D viewer

protected:
    int    m_EnabledLayers;                         // Bit-mask for layer enabling
    int    m_VisibleLayers;                         // Bit-mask for layer visibility
    int    m_VisibleElements;                       // Bit-mask for element category visibility

public:
    EDA_BoardDesignSettings();

    /**
     * Function GetVisibleLayers
     * returns a bit-mask of all the layers that are visible
     * @return int - the visible layers in bit-mapped form.
     */
    int  GetVisibleLayers() const;

    /**
     * Function SetVisibleAlls
     * Set the bit-mask of all visible elements categories, including layers
     */
    void SetVisibleAlls( );

    /**
     * Function SetVisibleLayers
     * changes the bit-mask of visible layers
     * @param aMask = The new bit-mask of visible layers
     */
    void SetVisibleLayers( int aMask );

    /**
     * Function IsLayerVisible
     * tests whether a given layer is visible
     * @param aLayerIndex = The index of the layer to be tested
     * @return bool - true if the layer is visible.
     */
    bool IsLayerVisible( int aLayerIndex ) const
    {
        if( aLayerIndex < 0 || aLayerIndex >= 32 ) //@@IMB: Altough Pcbnew uses only 29, Gerbview uses all 32 layers
            return false;

        // If a layer is disabled, it is automatically invisible
        return (bool) ( m_VisibleLayers & m_EnabledLayers & (1 << aLayerIndex) );
    }


    /**
     * Function SetLayerVisibility
     * changes the visibility of a given layer
     * @param aLayerIndex = The index of the layer to be changed
     * @param aNewState = The new visibility state of the layer
     */
    void SetLayerVisibility( int aLayerIndex, bool aNewState );

    /**
     * Function GetVisibleElements
     * returns a bit-mask of all the element categories that are visible
     * @return int - the visible element categories in bit-mapped form.
     */
    int GetVisibleElements() const
    {
        return m_VisibleElements;
    }


    /**
     * Function SetVisibleElements
     * changes the bit-mask of visible element categories
     * @param aMask = The new bit-mask of visible element categories
     */
    void SetVisibleElements( int aMask )
    {
        m_VisibleElements = aMask;
    }

    /**
     * Function IsElementVisible
     * tests whether a given element category is visible. Keep this as an
     * inline function.
     * @param aPCB_VISIBLE is from the enum by the same name
     * @return bool - true if the element is visible.
     * @see enum PCB_VISIBLE
     */
    bool IsElementVisible( int aPCB_VISIBLE ) const
    {
        return bool( m_VisibleElements & (1 << aPCB_VISIBLE) );
    }

    /**
     * Function SetElementVisibility
     * changes the visibility of an element category
     * @param aPCB_VISIBLE is from the enum by the same name
     * @param aNewState = The new visibility state of the element category
     * @see enum PCB_VISIBLE
     */
    void SetElementVisibility( int aPCB_VISIBLE, bool aNewState );

    /**
     * Function GetEnabledLayers
     * returns a bit-mask of all the layers that are enabled
     * @return int - the enabled layers in bit-mapped form.
     */
    inline int GetEnabledLayers() const
    {
        return m_EnabledLayers;
    }

    /**
     * Function SetEnabledLayers
     * changes the bit-mask of enabled layers
     * @param aMask = The new bit-mask of enabled layers
     */
    void SetEnabledLayers( int aMask );

    /**
     * Function IsLayerEnabled
     * tests whether a given layer is enabled
     * @param aLayerIndex = The index of the layer to be tested
     * @return bool - true if the layer is enabled
     */
    bool IsLayerEnabled( int aLayerIndex )
    {
        return bool( m_EnabledLayers & (1 << aLayerIndex) );
    }


    /**
     * Function GetCopperLayerCount
     * @return int - the number of neabled copper layers
     */
    int GetCopperLayerCount() const
    {
        return m_CopperLayerCount;
    }


    /**
     * Function SetCopperLayerCount
     * do what its name says...
     * @param aNewLayerCount = The new number of enabled copper layers
     */
    void SetCopperLayerCount( int aNewLayerCount );
};


#endif

//  _BOARD_DESIGN_SETTING_H
