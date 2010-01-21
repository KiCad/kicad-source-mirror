/***************************************************************************/
/* class_board_design_settings.cpp - EDA_BoardDesignSettings class functions */
/***************************************************************************/
#include "fctsys.h"
#include "common.h"

#include "pcbnew.h"
#include "class_board_design_settings.h"


/*****************************************************/
EDA_BoardDesignSettings::EDA_BoardDesignSettings()
/*****************************************************/

// Default values for designing boards
{
    int ii;

    static const int default_layer_color[32] =
    {
        GREEN,     LIGHTGRAY, LIGHTGRAY, LIGHTGRAY,
        LIGHTGRAY, LIGHTGRAY, LIGHTGRAY, LIGHTGRAY,
        LIGHTGRAY, LIGHTGRAY, LIGHTGRAY, LIGHTGRAY,
        LIGHTGRAY, LIGHTGRAY, LIGHTGRAY, RED,
        LIGHTGRAY, LIGHTGRAY,
        LIGHTGRAY, LIGHTGRAY,
        MAGENTA,   CYAN,
        LIGHTGRAY, LIGHTGRAY,
        LIGHTGRAY,
        LIGHTGRAY,
        LIGHTGRAY, LIGHTGRAY,
        LIGHTGRAY,
        LIGHTGRAY,
        LIGHTGRAY,
        LIGHTGRAY
    };

    m_EnabledLayers = ALL_LAYERS;                       // All layers enabled at first.
                                                        // SetCopperLayerCount() will adjust this.
    m_VisibleLayers   = 0xffffffff;                     // All layers visible at first.
    m_VisibleElements = 0x00000fff;                     // All elements visible at first. TODO: Use a macro for the initial value.

    SetCopperLayerCount( 2 );                           // Default design is a double sided board

    m_CurrentViaType = VIA_THROUGH;                     // via type (VIA_BLIND_BURIED, VIA_THROUGH VIA_MICROVIA)
    m_UseConnectedTrackWidth = false;                   // if true, when creating a new track starting on an existing track, use this track width
    m_MicroViasAllowed = false;                         // true to allow micro vias
    m_DrawSegmentWidth = 100;                           // current graphic line width (not EDGE layer)
    m_EdgeSegmentWidth = 100;                           // current graphic line width (EDGE layer only)
    m_PcbTextWidth     = 100;                           // current Pcb (not module) Text width
    m_PcbTextSize       = wxSize( 500, 500 );           // current Pcb (not module) Text size
    m_TrackMinWidth     = 80;                           // track min value for width ((min copper size value
    m_ViasMinSize       = 350;                          // vias (not micro vias) min diameter
    m_ViasMinDrill      = 200;                          // vias (not micro vias) min drill diameter
    m_MicroViasMinSize  = 200;                          // micro vias (not vias) min diameter
    m_MicroViasMinDrill = 50;                           // micro vias (not vias) min drill diameter

    // Global mask margins:
    m_SolderMaskMargin  = 150;                          // Solder mask margin
    m_SolderPasteMargin = 0;                            // Solder paste margin absolute value
    m_SolderPasteMarginRatio = 0.0;                     // Solder pask margin ratio value of pad size
                                                        // The final margin is the sum of these 2 values
                                                        // Usually < 0 because the mask is smaller than pad

    /* Color options for screen display of the Printed Board: */
    for( ii = 0; ii < 32; ii++ )
        m_LayerColor[ii] = default_layer_color[ii];

    // Layer colors (tracks and graphic items)
    m_ViaColor[VIA_NOT_DEFINED]  = DARKGRAY;
    m_ViaColor[VIA_MICROVIA]     = CYAN;
    m_ViaColor[VIA_BLIND_BURIED] = BROWN;
    m_ViaColor[VIA_THROUGH] = WHITE;

    m_RatsnestColor = WHITE;                // Ratsnest color
}


// see pcbstruct.h
int EDA_BoardDesignSettings::GetVisibleLayers() const
{
    return m_VisibleLayers;
}


void EDA_BoardDesignSettings::SetVisibleLayers( int aMask )
{
    // Although Pcbnew uses only 29, Gerbview uses all 32 layers
    m_VisibleLayers = aMask & m_EnabledLayers & FULL_LAYERS;
}


void EDA_BoardDesignSettings::SetLayerVisibility( int aLayerIndex, bool aNewState )
{
    // Altough Pcbnew uses only 29, Gerbview uses all 32 layers
    if( aLayerIndex < 0 || aLayerIndex >= 32 )
        return;
    if( aNewState && IsLayerEnabled( aLayerIndex ) )
        m_VisibleLayers |= 1 << aLayerIndex;
    else
        m_VisibleLayers &= ~( 1 << aLayerIndex );
}


void EDA_BoardDesignSettings::SetElementVisibility( int aElementCategory, bool aNewState )
{
    if( aElementCategory < 0 || aElementCategory >= END_VISIBLE )
        return;
    if( aNewState )
        m_VisibleElements |= 1 << aElementCategory;
    else
        m_VisibleElements &= ~( 1 << aElementCategory );
}


void EDA_BoardDesignSettings::SetCopperLayerCount( int aNewLayerCount )
{
    // if( aNewLayerCount < 2 ) aNewLayerCount = 2;

    m_CopperLayerCount = aNewLayerCount;

    // ensure consistency with the m_EnabledLayers member
    m_EnabledLayers &= ~ALL_CU_LAYERS;
    m_EnabledLayers |= LAYER_BACK;

    if( m_CopperLayerCount > 1 )
        m_EnabledLayers |= LAYER_FRONT;

    for( int ii = 1; ii < aNewLayerCount - 1; ii++ )
        m_EnabledLayers |= 1 << ii;
}

/**
 * Function SetEnabledLayers
 * changes the bit-mask of enabled layers
 * @param aMask = The new bit-mask of enabled layers
 */
void EDA_BoardDesignSettings::SetEnabledLayers( int aMask )
{
    // Back and front layers are always enabled.
    aMask |= LAYER_BACK | LAYER_FRONT;

    m_EnabledLayers = aMask;

    // A disabled layer cannot be visible
    m_VisibleLayers &= aMask;

    // update m_CopperLayerCount to ensure its consistency with m_EnabledLayers
    m_CopperLayerCount = 0;
    for( int ii = 0;  aMask && ii < NB_COPPER_LAYERS;  ii++, aMask >>= 1 )
    {
        if( aMask & 1 )
            m_CopperLayerCount++;
    }
}

