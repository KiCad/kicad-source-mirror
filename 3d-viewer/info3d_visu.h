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

#include <wxBasePcbFrame.h>                     // m_auimanager member.
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


class S3D_COLOR     /* 3D color (R, G, B) 3 floats range 0 to 1.0*/
{
public:
    double m_Red, m_Green, m_Blue;
public: S3D_COLOR()
    {
        m_Red = m_Green = m_Blue = 0;
    }
};

/* information needed to display 3D board */
class INFO3D_VISU
{
public:
    enum DISPLAY3D_FLG {
        FL_AXIS=0, FL_MODULE, FL_ZONE,
        FL_COMMENTS, FL_DRAWINGS, FL_ECO1, FL_ECO2,
        FL_GRID,
        FL_USE_COPPER_THICKNESS,
        FL_LAST
    };

    double      m_Beginx, m_Beginy;                 // position of mouse (used in drag commands)
    double      m_Quat[4];                          // orientation of 3D view
    double      m_Rot[4];                           // rotation parameters of 3D view
    double      m_Zoom;                             // 3D zoom value
    double      m_3D_Grid;                          // 3D grid valmue, in mm
    S3D_COLOR   m_BgColor;
    bool        m_DrawFlags[FL_LAST];               // Enable/disable flags (see DISPLAY3D_FLG list)
    wxPoint     m_BoardPos;                         // center board actual position in board units
    wxSize      m_BoardSize;                        // board actual size in board units
    int         m_CopperLayersCount;                // Number of copper layers actually used by the board

    const BOARD_DESIGN_SETTINGS* m_BoardSettings;   // Link to current board design settings

    double  m_BiuTo3Dunits;                         // Normalization scale to convert board
                                                    // internal units to 3D units
                                                    // to scale 3D units between -1.0 and +1.0
    double  m_LayerZcoord[LAYER_COUNT];             // Z position of each layer (normalized)
    double  m_CurrentZpos;                          // temporary storage of current value of Z position,
                                                    // used in some calculation
private:
    double  m_CopperThickness;                      // Copper thickness (normalized)
    double  m_EpoxyThickness;                       // Epoxy thickness (normalized)
    double  m_NonCopperLayerThickness;              // Non copper layers thickness

public: INFO3D_VISU();
    ~INFO3D_VISU();

    /**
     * Function InitSettings
     * Initialize info 3D Parameters from aBoard
     * @param aBoard: the board to display
     */
    void InitSettings( BOARD* aBoard );

    /**
     * function m_BiuTo3Dunits
     * @return the Z coordinate of the layer aLayer, in Board Internal Units
     * @param aLayer: the layer number
     */
    int GetLayerZcoordBIU( int aLayer )
    {
        return (int) (m_LayerZcoord[aLayer] / m_BiuTo3Dunits );
    }

    int GetCopperThicknessBIU() const
    {
        return m_DrawFlags[FL_USE_COPPER_THICKNESS] ?
            (int) (m_CopperThickness / m_BiuTo3Dunits )
            : 0;
    }

    int GetEpoxyThicknessBIU() const
    {
        return (int) (m_EpoxyThickness / m_BiuTo3Dunits );
    }

    int GetNonCopperLayerThicknessBIU() const
    {
        return  m_DrawFlags[FL_USE_COPPER_THICKNESS] ?
            (int) (m_NonCopperLayerThickness / m_BiuTo3Dunits )
            : 0;
    }
};

extern INFO3D_VISU g_Parm_3D_Visu;

#endif /*  __INFO3D_VISU_H__ */
