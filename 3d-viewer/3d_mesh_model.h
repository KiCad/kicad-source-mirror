/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2014 Mario Luzeiro <mrluzeiro@gmail.com>
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
 * @file 3d_mesh_model.h
 * @brief 
 */

#ifndef __3D_MESH_MODEL_H__
#define __3D_MESH_MODEL_H__

#include <fctsys.h>
#include <common.h>
#include <macros.h>
#include <base_struct.h>
#include <gal/opengl/glm/glm.hpp>
#include <vector>
#include <kicad_string.h>
#include <info3d_visu.h>
#ifdef __WXMAC__
#  ifdef __DARWIN__
#    include <OpenGL/glu.h>
#  else
#    include <glu.h>
#  endif
#else
#  include <GL/glu.h>
#endif
#include <wx/glcanvas.h>

class S3D_MESH;

class S3D_MESH
{
public:

    S3D_MESH();
    ~S3D_MESH();

    void openGL_Render();
    void openGL_RenderAllChilds();

    S3D_MATERIAL    *m_Materials;

    // Point and index list
    std::vector< glm::vec3 > m_Point;
    std::vector< std::vector<int> > m_CoordIndex;
    std::vector< glm::vec3 > m_PerFaceNormalsNormalized;
    std::vector< glm::vec3 > m_PerVertexNormalsNormalized;

    std::vector< int > m_MaterialIndex;

    glm::vec3 m_translation;
    glm::vec4 m_rotation;
    glm::vec3 m_scale;
    glm::vec4 m_scaleOrientation; // not used
    glm::vec3 m_center; // not used

    std::vector<S3D_MESH *> childs;

private:
    std::vector< glm::vec3 > m_PerFaceNormalsRaw;
    std::vector< std::vector< glm::vec3 > > m_PerFaceVertexNormals;
    std::vector< glm::vec3 > m_PointNormalized;
    std::vector< float > m_PerFaceSquaredArea;
    std::vector< std::vector<int> > m_InvalidCoordIndexes; //!TODO: check for invalid CoordIndex in file and remove the index and the same material index

    bool isPerFaceNormalsComputed;
    void calcPerFaceNormals ();

    bool isPointNormalizedComputed;
    void calcPointNormalized();

    bool isPerPointNormalsComputed;
    void calcPerPointNormals();
};


#endif
