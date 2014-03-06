/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 1992-2012 KiCad Developers, see AUTHORS.txt for contributors.
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
 * @file class_board_design_settings.cpp
 * BOARD_DESIGN_SETTINGS class functions.
 */

#include <fctsys.h>
#include <common.h>
#include <layers_id_colors_and_visibility.h>

#include <pcbnew.h>
#include <class_board_design_settings.h>

#include <class_track.h>
#include <convert_from_iu.h>

// Board thickness, mainly for 3D view:
#define DEFAULT_BOARD_THICKNESS_MM   1.6

// Default values for some board items
#define DEFAULT_TEXT_PCB_SIZE  Millimeter2iu( 1.5 )
#define DEFAULT_TEXT_PCB_THICKNESS  Millimeter2iu( 0.3 )
#define DEFAULT_PCB_EDGE_THICKNESS  Millimeter2iu( 0.15 )
#define DEFAULT_GRAPHIC_THICKNESS   Millimeter2iu( 0.2 )
#define DEFAULT_TEXT_MODULE_SIZE    Millimeter2iu( 1.5 )
#define DEFAULT_GR_MODULE_THICKNESS Millimeter2iu( 0.15 )

#define DEFAULT_SOLDERMASK_CLEARANCE Millimeter2iu( 0.2 )
#define DEFAULT_SOLDERMASK_MIN_WIDTH Millimeter2iu( 0.0 )


BOARD_DESIGN_SETTINGS::BOARD_DESIGN_SETTINGS() :
    m_Pad_Master( NULL )
{
    m_EnabledLayers = ALL_LAYERS;               // All layers enabled at first.
                                                // SetCopperLayerCount() will adjust this.

    SetVisibleLayers( FULL_LAYERS );

    // set all but hidden text as visible.
    m_VisibleElements = ~( 1 << MOD_TEXT_INVISIBLE );

    SetCopperLayerCount( 2 );                   // Default design is a double sided board

    // via type (VIA_BLIND_BURIED, VIA_THROUGH VIA_MICROVIA).
    m_CurrentViaType = VIA_THROUGH;

    // if true, when creating a new track starting on an existing track, use this track width
    m_UseConnectedTrackWidth = false;

    m_BlindBuriedViaAllowed = false;            // true to allow blind/buried vias
    m_MicroViasAllowed = false;                 // true to allow micro vias

    m_DrawSegmentWidth = DEFAULT_GRAPHIC_THICKNESS;     // current graphic line width (not EDGE layer)

    m_EdgeSegmentWidth = DEFAULT_PCB_EDGE_THICKNESS;    // current graphic line width (EDGE layer only)
    m_PcbTextWidth     = DEFAULT_TEXT_PCB_THICKNESS;    // current Pcb (not module) Text width

    m_PcbTextSize       = wxSize( DEFAULT_TEXT_PCB_SIZE,
                                  DEFAULT_TEXT_PCB_SIZE );  // current Pcb (not module) Text size

    m_TrackMinWidth     = DMils2iu( 100 );      // track min value for width ((min copper size value
    m_ViasMinSize       = DMils2iu( 350 );      // vias (not micro vias) min diameter
    m_ViasMinDrill      = DMils2iu( 200 );      // vias (not micro vias) min drill diameter
    m_MicroViasMinSize  = DMils2iu( 200 );      // micro vias (not vias) min diameter
    m_MicroViasMinDrill = DMils2iu( 50 );       // micro vias (not vias) min drill diameter

    // Global mask margins:
    m_SolderMaskMargin  = DEFAULT_SOLDERMASK_CLEARANCE; // Solder mask margin
    m_SolderMaskMinWidth = DEFAULT_SOLDERMASK_MIN_WIDTH;   // Solder mask min width
    m_SolderPasteMargin = 0;                    // Solder paste margin absolute value
    m_SolderPasteMarginRatio = 0.0;             // Solder pask margin ratio value of pad size
                                                // The final margin is the sum of these 2 values
                                                // Usually < 0 because the mask is smaller than pad

    m_ModuleTextSize = wxSize( DEFAULT_TEXT_MODULE_SIZE,
                               DEFAULT_TEXT_MODULE_SIZE );
    m_ModuleTextWidth = DEFAULT_GR_MODULE_THICKNESS;
    m_ModuleSegmentWidth = DEFAULT_GR_MODULE_THICKNESS;

    // Layer thickness for 3D viewer
    m_boardThickness = Millimeter2iu( DEFAULT_BOARD_THICKNESS_MM );
}

// Add parameters to save in project config.
// values are saved in mm
void BOARD_DESIGN_SETTINGS::AppendConfigs( PARAM_CFG_ARRAY* aResult )
{
    m_Pad_Master.AppendConfigs( aResult );

    aResult->push_back( new PARAM_CFG_INT_WITH_SCALE( wxT( "PcbTextSizeV" ),
                    &m_PcbTextSize.y,
                    DEFAULT_TEXT_PCB_SIZE, TEXTS_MIN_SIZE, TEXTS_MAX_SIZE,
                    NULL, MM_PER_IU ) );

    aResult->push_back( new PARAM_CFG_INT_WITH_SCALE( wxT( "PcbTextSizeH" ),
                    &m_PcbTextSize.x,
                    DEFAULT_TEXT_PCB_SIZE, TEXTS_MIN_SIZE, TEXTS_MAX_SIZE,
                    NULL, MM_PER_IU ) );

    aResult->push_back( new PARAM_CFG_INT_WITH_SCALE( wxT( "PcbTextThickness" ),
                    &m_PcbTextWidth,
                    DEFAULT_TEXT_PCB_THICKNESS,
                    Millimeter2iu( 0.01 ), Millimeter2iu( 5.0 ),
                    NULL, MM_PER_IU ) );

    aResult->push_back( new PARAM_CFG_INT_WITH_SCALE( wxT( "ModuleTextSizeV" ),
                    &m_ModuleTextSize.y,
                    DEFAULT_TEXT_MODULE_SIZE, TEXTS_MIN_SIZE, TEXTS_MAX_SIZE,
                    NULL, MM_PER_IU ) );

    aResult->push_back( new PARAM_CFG_INT_WITH_SCALE( wxT( "ModuleTextSizeH" ),
                    &m_ModuleTextSize.x,
                    DEFAULT_TEXT_MODULE_SIZE, TEXTS_MIN_SIZE, TEXTS_MAX_SIZE,
                    NULL, MM_PER_IU ) );

    aResult->push_back( new PARAM_CFG_INT_WITH_SCALE( wxT( "ModuleTextSizeThickness" ),
                    &m_ModuleTextWidth,
                    DEFAULT_GR_MODULE_THICKNESS, 1, TEXTS_MAX_WIDTH,
                    NULL, MM_PER_IU ) );

    aResult->push_back( new PARAM_CFG_INT_WITH_SCALE( wxT( "SolderMaskClearance" ),
                    &m_SolderMaskMargin,
                    DEFAULT_SOLDERMASK_CLEARANCE, 0, Millimeter2iu( 1.0 ),
                    NULL, MM_PER_IU ) );

    aResult->push_back( new PARAM_CFG_INT_WITH_SCALE( wxT( "SolderMaskMinWidth" ),
                    &m_SolderMaskMinWidth,
                    DEFAULT_SOLDERMASK_MIN_WIDTH, 0, Millimeter2iu( 0.5 ),
                    NULL, MM_PER_IU ) );

    aResult->push_back( new PARAM_CFG_INT_WITH_SCALE( wxT( "DrawSegmentWidth" ),
                    &m_DrawSegmentWidth,
                    DEFAULT_GRAPHIC_THICKNESS,
                    Millimeter2iu( 0.01 ), Millimeter2iu( 5.0 ),
                    NULL, MM_PER_IU ) );

    aResult->push_back( new PARAM_CFG_INT_WITH_SCALE( wxT( "BoardOutlineThickness" ),
                    &m_EdgeSegmentWidth,
                    DEFAULT_PCB_EDGE_THICKNESS,
                    Millimeter2iu( 0.01 ), Millimeter2iu( 5.0 ),
                    NULL, MM_PER_IU ) );

    aResult->push_back( new PARAM_CFG_INT_WITH_SCALE( wxT( "ModuleOutlineThickness" ),
                    &m_ModuleSegmentWidth,
                    DEFAULT_GR_MODULE_THICKNESS,
                    Millimeter2iu( 0.01 ), Millimeter2iu( 5.0 ),
                    NULL, MM_PER_IU ) );
}


// see pcbstruct.h
LAYER_MSK BOARD_DESIGN_SETTINGS::GetVisibleLayers() const
{
    return m_VisibleLayers;
}


void BOARD_DESIGN_SETTINGS::SetVisibleAlls()
{
    SetVisibleLayers( FULL_LAYERS );
    m_VisibleElements = -1;
}


void BOARD_DESIGN_SETTINGS::SetVisibleLayers( LAYER_MSK aMask )
{
    m_VisibleLayers = aMask & m_EnabledLayers & FULL_LAYERS;
}


void BOARD_DESIGN_SETTINGS::SetLayerVisibility( LAYER_NUM aLayer, bool aNewState )
{
    if( aNewState && IsLayerEnabled( aLayer ) )
        m_VisibleLayers |= GetLayerMask( aLayer );
    else
        m_VisibleLayers &= ~GetLayerMask( aLayer );
}


void BOARD_DESIGN_SETTINGS::SetElementVisibility( int aElementCategory, bool aNewState )
{
    if( aElementCategory < 0 || aElementCategory >= END_PCB_VISIBLE_LIST )
        return;

    if( aNewState )
        m_VisibleElements |= 1 << aElementCategory;
    else
        m_VisibleElements &= ~( 1 << aElementCategory );
}


void BOARD_DESIGN_SETTINGS::SetCopperLayerCount( int aNewLayerCount )
{
    // if( aNewLayerCount < 2 ) aNewLayerCount = 2;

    m_CopperLayerCount = aNewLayerCount;

    // ensure consistency with the m_EnabledLayers member
    m_EnabledLayers &= ~ALL_CU_LAYERS;
    m_EnabledLayers |= LAYER_BACK;

    if( m_CopperLayerCount > 1 )
        m_EnabledLayers |= LAYER_FRONT;

    for( LAYER_NUM ii = LAYER_N_2; ii < aNewLayerCount - 1; ++ii )
        m_EnabledLayers |= GetLayerMask( ii );
}


void BOARD_DESIGN_SETTINGS::SetEnabledLayers( LAYER_MSK aMask )
{
    // Back and front layers are always enabled.
    aMask |= LAYER_BACK | LAYER_FRONT;

    m_EnabledLayers = aMask;

    // A disabled layer cannot be visible
    m_VisibleLayers &= aMask;

    // update m_CopperLayerCount to ensure its consistency with m_EnabledLayers
    m_CopperLayerCount = LayerMaskCountSet( aMask & ALL_CU_LAYERS);
}


#ifndef NDEBUG
struct list_size_check {
   list_size_check()
   {
       // Int (the type used for saving visibility settings) is only 32 bits guaranteed,
       // be sure that we do not cross the limit
       assert( END_PCB_VISIBLE_LIST <= 32 );
   };
};
static list_size_check check;
#endif
