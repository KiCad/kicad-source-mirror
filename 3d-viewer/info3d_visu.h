/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2012 Jean-Pierre Charras, jp.charras at wanadoo.fr
 * Copyright (C) 2011 Wayne Stambaugh <stambaughw@verizon.net>
 * Copyright (C) 1992-2011 KiCad Developers, see AUTHORS.txt for contributors.
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
 * @file info3d_visu.h
 */

#ifndef __INFO3D_VISU_H__
#define __INFO3D_VISU_H__

#include <layers_id_colors_and_visibility.h>    // Layers id definitions

#include <wx/glcanvas.h>

#ifdef __WXMAC__
#  ifdef __DARWIN__
#    include <OpenGL/glu.h>
#  else
#    include <glu.h>
#  endif
#else
#  include <GL/glu.h>
#endif

#include <3d_struct.h>

#define m_ROTX  m_Rot[0]
#define m_ROTY  m_Rot[1]
#define m_ROTZ  m_Rot[2]

class BOARD_DESIGN_SETTINGS;

/** Minor class to store a 3D color (R, G, B) 3 floats range 0 to 1.0
 */
class S3D_COLOR
{
public:
    double m_Red, m_Green, m_Blue;
public: S3D_COLOR()
    {
        m_Red = m_Green = m_Blue = 0;
    }
};

enum DISPLAY3D_FLG {
    FL_AXIS=0, FL_MODULE, FL_ZONE,
    FL_ADHESIVE, FL_SILKSCREEN, FL_SOLDERMASK, FL_SOLDERPASTE,
    FL_COMMENTS, FL_ECO,
    FL_GRID,
    FL_USE_COPPER_THICKNESS,
    FL_SHOW_BOARD_BODY,
    FL_USE_REALISTIC_MODE,
    FL_RENDER_SHADOWS,
    FL_RENDER_SHOW_HOLES_IN_ZONES,
    FL_RENDER_TEXTURES,
    FL_RENDER_SMOOTH_NORMALS,
    FL_RENDER_USE_MODEL_NORMALS,
    FL_RENDER_MATERIAL,
    FL_LAST
};

/** Helper class to handle information needed to display 3D board
 */
class INFO3D_VISU
{
public:
    double      m_Beginx, m_Beginy;                 // position of mouse (used in drag commands)
    double      m_Quat[4];                          // orientation of 3D view
    double      m_Rot[4];                           // rotation parameters of 3D view
    double      m_Zoom;                             // 3D zoom value
    double      m_3D_Grid;                          // 3D grid value, in mm
    S3D_COLOR   m_BgColor;
    S3D_COLOR   m_BgColor_Top;
    S3D_COLOR   m_BoardBodyColor;                   // in realistic mode: FR4 board color
    S3D_COLOR   m_SolderMaskColor;                  // in realistic mode: solder mask color
    S3D_COLOR   m_SilkScreenColor;                  // in realistic mode: SilkScreen color
    S3D_COLOR   m_CopperColor;                      // in realistic mode: copper color
    wxPoint     m_BoardPos;                         // center board actual position in board units
    wxSize      m_BoardSize;                        // board actual size in board units
    int         m_CopperLayersCount;                // Number of copper layers actually used by the board

    const BOARD_DESIGN_SETTINGS* m_BoardSettings;   // Link to current board design settings

    double  m_BiuTo3Dunits;                         // Normalization scale to convert board
                                                    // internal units to 3D units
                                                    // to normalize 3D units between -1.0 and +1.0

    double zpos_offset;

private:
    double  m_layerZcoord[LAYER_ID_COUNT];          // Z position of each layer (normalized)
    double  m_copperThickness;                      // Copper thickness (normalized)
    double  m_epoxyThickness;                       // Epoxy thickness (normalized)
    double  m_nonCopperLayerThickness;              // Non copper layers thickness
    std::bitset<FL_LAST> m_drawFlags;               // Enable/disable flags (see DISPLAY3D_FLG list)

public: INFO3D_VISU();
    ~INFO3D_VISU();

    // Accessors
    bool GetFlag( DISPLAY3D_FLG aFlag ) const { return m_drawFlags[aFlag]; }
    void SetFlag( DISPLAY3D_FLG aFlag, bool aState )
    {
        m_drawFlags[aFlag] = aState;
    }

    /**
     * Initialize 3D Parameters depending on aBoard
     * @param aBoard: the board to display
     */
    void InitSettings( BOARD* aBoard );

    /**
     * @return the Z position of 3D shapes, in 3D Units
     * @param aIsFlipped: true for modules on Front (top) layer, false
     * if on back (bottom) layer
     */
    double GetModulesZcoord3DIU( bool aIsFlipped );

    /**
     * @return the Z coordinate of the layer aLayer, in Board Internal Units
     * @param aLayerId: the layer number
     */
    int GetLayerZcoordBIU( int aLayerId )
    {
        return KiROUND( m_layerZcoord[aLayerId] / m_BiuTo3Dunits );
    }

    /**
     * @return the thickness (Z size) of the copper, in Board Internal Units
     * note: the thickness (Z size) of the copper is not the thickness
     * of the layer (the thickness of the layer is the epoxy thickness / layer count)
     *
     * Note: if m_drawFlags[FL_USE_COPPER_THICKNESS] is not set,
     * and normal mode, returns 0
     */
    int GetCopperThicknessBIU() const
    {
        bool use_thickness = GetFlag( FL_USE_COPPER_THICKNESS );

        return use_thickness ?
            KiROUND( m_copperThickness / m_BiuTo3Dunits )
            : 0;
    }

    /**
     * function GetEpoxyThicknessBIU
     * @return the thickness (Z size) of the epoxy board, in Board Internal Units
     */
    int GetEpoxyThicknessBIU() const
    {
        return KiROUND( m_epoxyThickness / m_BiuTo3Dunits );
    }

    /**
     * function GetNonCopperLayerThicknessBIU
     * @return the thickness (Z size) of a technical layer,
     *  in Board Internal Units
     *
     * Note: if m_drawFlags[FL_USE_COPPER_THICKNESS] is not set, returns 0
     */
    int GetNonCopperLayerThicknessBIU() const
    {
        bool use_thickness = GetFlag( FL_USE_COPPER_THICKNESS )
//                                    || GetFlag( FL_USE_REALISTIC_MODE )
                                    ;
        return  use_thickness ?
            KiROUND( m_nonCopperLayerThickness / m_BiuTo3Dunits )
            : 0;
    }

    /**
     * function GetNonCopperLayerThicknessBIU
     * @return the thickness (Z size) of the copper or a technical layer,
     *  in Board Internal Units, depending on the layer id
     *
     * Note: if m_drawFlags[FL_USE_COPPER_THICKNESS] is not set, returns 0
     */
    int GetLayerObjectThicknessBIU( int aLayerId ) const
    {
        return IsCopperLayer( aLayerId ) ?
                        GetCopperThicknessBIU() :
                        GetNonCopperLayerThicknessBIU()
                        ;
    }

    bool IsRealisticMode() { return GetFlag( FL_USE_REALISTIC_MODE ); }
};

extern INFO3D_VISU g_Parm_3D_Visu;

#endif /*  __INFO3D_VISU_H__ */
