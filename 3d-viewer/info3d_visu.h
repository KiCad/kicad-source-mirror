/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2004 Jean-Pierre Charras, jaen-pierre.charras@gipsa-lab.inpg.com
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
 * @file 3d_viewer.h
 */

#ifndef __INFO3D_VISU_H__
#define __INFO3D_VISU_H__

#include <wxBasePcbFrame.h>        // m_auimanager member.
#include <layers_id_colors_and_visibility.h>    // Layers id definitions

#if !wxUSE_GLCANVAS
#error Please set wxUSE_GLCANVAS to 1 in setup.h.
#endif

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

#define m_ROTX m_Rot[0]
#define m_ROTY m_Rot[1]
#define m_ROTZ m_Rot[2]

/* information needed to display 3D board */
class INFO3D_VISU
{

public:
    enum DISPLAY3D_FLG
    {
        FL_AXIS=0,        FL_MODULE,        FL_ZONE,
        FL_COMMENTS,      FL_DRAWINGS,      FL_ECO1,          FL_ECO2,
        FL_GRID,
        FL_LAST
    };

    double    m_Beginx, m_Beginy;   // position of mouse
    double    m_Quat[4];            // orientation of object
    double    m_Rot[4];             // man rotation of object
    double    m_Zoom;               // field of view in degrees
    double    m_3D_Grid;            // 3D grid valmue, in mm
    S3D_Color m_BgColor;
    bool      m_DrawFlags[FL_LAST]; // show these special items
    wxPoint   m_BoardPos;
    wxSize    m_BoardSize;
    int       m_CopperLayersCount;  // Number of copper layers actually used by the board

    const BOARD_DESIGN_SETTINGS* m_BoardSettings;   // Link to current board design settings

    double    m_EpoxyThickness;             // Epoxy thickness (normalized)
    double    m_NonCopperLayerThickness;   // Non copper layers thickness

    double    m_BoardScale;                 /* Normalization scale for coordinates:
                                            * when scaled between -1.0 and +1.0 */
    double    m_LayerZcoord[LAYER_COUNT];    // Z position of each layer (normalized)
    double    m_ActZpos;

public: INFO3D_VISU();
    ~INFO3D_VISU();
};

extern INFO3D_VISU  g_Parm_3D_Visu;
extern double       DataScale3D; // 3D scale units.

#endif  /*  __INFO3D_VISU_H__ */
