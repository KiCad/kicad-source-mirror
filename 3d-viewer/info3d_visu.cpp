/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2014 Jean-Pierre Charras, jp.charras at wanadoo.fr
 * Copyright (C) 1992-2014 KiCad Developers, see AUTHORS.txt for contributors.
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
 * @file info3d_visu.cpp
 */

#include <fctsys.h>

#include <common.h>

#include <class_board_design_settings.h>
#include <class_board.h>

#include <3d_viewer.h>
#include <info3d_visu.h>
#include <trackball.h>

// Thickness of copper
// TODO: define the actual copper thickness by user
#define COPPER_THICKNESS KiROUND( 0.035 * IU_PER_MM )   // for 35 um
#define TECH_LAYER_THICKNESS KiROUND( 0.04 * IU_PER_MM )
#define EPOXY_THICKNESS KiROUND( 1.6 * IU_PER_MM )   // for 1.6 mm


/* INFO3D_VISU in an helper class to store parameters like scaling factors,
 * board size, Z coordinates of layers ...
 * to create the 3D GLList
 */
INFO3D_VISU::INFO3D_VISU()
{
    int ii;

    m_Beginx    = m_Beginy = 0.0;   // position of mouse
    m_Zoom      = 1.0;
    m_3D_Grid   = 10.0;             // Grid value in mm
    trackball( m_Quat, 0.0, 0.0, 0.0, 0.0 );

    for( ii = 0; ii < 4; ii++ )
        m_Rot[ii] = 0.0;

    m_CopperLayersCount = 2;
    m_BoardSettings     = NULL;
    m_copperThickness   = 0;
    m_epoxyThickness    = 0;
    m_nonCopperLayerThickness = 0;
    m_BiuTo3Dunits = 1.0;
    zpos_offset = 0.0;

    // default all special item layers Visible
    m_drawFlags.set();

    SetFlag( FL_GRID, false );
    SetFlag( FL_USE_COPPER_THICKNESS, false );
    SetFlag( FL_RENDER_SHADOWS, false );
    SetFlag( FL_RENDER_SHOW_HOLES_IN_ZONES, false );
}


INFO3D_VISU::~INFO3D_VISU()
{
}


/* Initialize info 3D Parameters from aBoard
 */
void INFO3D_VISU::InitSettings( BOARD* aBoard )
{
    // Calculates the board bounding box
    // First, use only the board outlines
    EDA_RECT bbbox = aBoard->ComputeBoundingBox( true );

    // If no outlines, use the board with items
    if( bbbox.GetWidth() == 0 && bbbox.GetHeight() == 0 )
       bbbox = aBoard->ComputeBoundingBox( false );

    // Gives a non null size to avoid issues in zoom / scale calculations
    if( bbbox.GetWidth() == 0 && bbbox.GetHeight() == 0 )
        bbbox.Inflate( Millimeter2iu( 10 ) );

    m_BoardSettings = &aBoard->GetDesignSettings();

    m_BoardSize = bbbox.GetSize();
    m_BoardPos  = bbbox.Centre();

    m_BoardPos.y = -m_BoardPos.y;
    m_CopperLayersCount = aBoard->GetCopperLayerCount();

    // Ensure the board has 2 sides for 3D views, because it is hard to find
    // a *really* single side board in the true life...
    if( m_CopperLayersCount < 2 )
        m_CopperLayersCount = 2;

    m_BiuTo3Dunits = 2.0 / std::max( m_BoardSize.x, m_BoardSize.y );

    m_epoxyThickness = aBoard->GetDesignSettings().GetBoardThickness() * m_BiuTo3Dunits;

    // TODO use value defined by user (currently use default values by ctor
    m_copperThickness   = COPPER_THICKNESS * m_BiuTo3Dunits;
    m_nonCopperLayerThickness = TECH_LAYER_THICKNESS * m_BiuTo3Dunits;

    // Init  Z position of each layer
    // calculate z position for each copper layer
    // Z = 0 is the z position of the back (bottom) layer (layer id = 31)
    // Z = m_epoxyThickness is the z position of the front (top) layer (layer id = 0)
    // all unused copper layer z position are set to 0
    int layer;
    int copper_layers_cnt = m_CopperLayersCount;

    for( layer = 0; layer < copper_layers_cnt; layer++ )
    {
        m_layerZcoord[layer] =
            m_epoxyThickness - (m_epoxyThickness * layer / (copper_layers_cnt - 1));
    }

    #define layerThicknessMargin 1.1
    double zpos_offset = m_nonCopperLayerThickness * layerThicknessMargin;
    double  zpos_copper_back    = - layerThicknessMargin*m_copperThickness/2;
    double  zpos_copper_front   = m_epoxyThickness + layerThicknessMargin*m_copperThickness/2;

    // Fill remaining unused copper layers and back layer zpos
    // with 0
    for( ; layer < MAX_CU_LAYERS; layer++ )
    {
        m_layerZcoord[layer] = 0;
    }

    // calculate z position for each non copper layer
    // Solder mask and Solder paste have the same Z position
    for( int layer_id = MAX_CU_LAYERS; layer_id < LAYER_ID_COUNT; layer_id++ )
    {
        double zpos;

        switch( layer_id )
        {
        case B_Adhes:
            zpos = zpos_copper_back - 3 * zpos_offset;
            break;

        case F_Adhes:
            zpos = zpos_copper_front + 3 * zpos_offset;
            break;

        case B_Paste:
            zpos = zpos_copper_back - 1 * zpos_offset;
            break;

        case F_Paste:
            zpos = zpos_copper_front + 1 * zpos_offset;
            break;

        case B_Mask:
            zpos = zpos_copper_back - 1 * zpos_offset;
            break;

        case F_Mask:
            zpos = zpos_copper_front + 1 * zpos_offset;
            break;

        case B_SilkS:
            zpos = zpos_copper_back - 2 * zpos_offset;
            break;

        case F_SilkS:
            zpos = zpos_copper_front + 2 * zpos_offset;
            break;

        default:
            zpos = zpos_copper_front + (layer_id - MAX_CU_LAYERS + 4) * zpos_offset;
            break;
        }

        m_layerZcoord[layer_id] = zpos;
    }
}

/* return the Z position of 3D shapes, in 3D Units
 * aIsFlipped: true for modules on Front (top) layer, false
 * if on back (bottom) layer
 * Note: in draw functions, the copper has a thickness = m_copperThickness
 * Vias and tracks are draw with the top side position = m_copperThickness/2
 * and the bottom side position = -m_copperThickness/2 from the Z layer position
 */
double INFO3D_VISU::GetModulesZcoord3DIU( bool aIsFlipped )
{
    if( aIsFlipped )
    {
        if( g_Parm_3D_Visu.GetFlag( FL_SOLDERPASTE ) )
            return m_layerZcoord[B_SilkS] - ( m_copperThickness / 2.0 );
        else
            return m_layerZcoord[B_Paste] - ( m_copperThickness / 2.0 );
    }
    else
    {
        if( g_Parm_3D_Visu.GetFlag( FL_SOLDERPASTE ) )
            return m_layerZcoord[F_SilkS] + ( m_copperThickness / 2.0 );
        else
            return m_layerZcoord[F_Paste] + ( m_copperThickness / 2.0 );
    }
}

