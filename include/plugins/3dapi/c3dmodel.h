/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2015 Mario Luzeiro <mrluzeiro@ua.pt>
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
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
 * @file  c3dmodel.h
 * @brief define an internal structure to be used by the 3D renders
 */


#ifndef C3DMODEL_H
#define C3DMODEL_H

#include "plugins/3dapi/xv3d_types.h"


struct SMATERIAL
{
    SFVEC3F m_Ambient;          //
    SFVEC3F m_Diffuse;          ///< Default diffuse color if m_Color is NULL
    SFVEC3F m_Emissive;         //
    SFVEC3F m_Specular;         //
    float   m_Shininess;        //
    float   m_Transparency;     ///< 1.0 is completely transparent, 0.0 completely opaque

    // !TODO: to be implemented
    /*struct textures
    {
        wxString m_Ambient;            // map_Ka
        wxString m_Diffuse;            // map_Kd
        wxString m_Specular;           // map_Ks
        wxString m_Specular_highlight; // map_Ns
        wxString m_Bump;               // map_bump, bump
        wxString m_Displacement;       // disp
        wxString m_Alpha;              // map_d
    };*/
};


/**
 * Per-vertex normal/color/texcoors structure.
 *
 * CONDITIONS:
 *     m_Positions size == m_Normals size == m_Texcoords size == m_Color size
 *     m_Texcoords can be NULL, textures will not be applied in that case
 *     m_Color can be NULL, it will use the m_Diffuse color for every triangle
 *     any m_FaceIdx must be an index of a the element lists
 *     m_MaterialIdx must be an existent material index stored in the parent model
 *
 * SCALES:
 *     m_Positions units are in mm, example:
 *          0.1 unit ==  0.1 mm
 *          1.0 unit ==  1.0 mm
 *          10.0 unit == 10.0 mm
 *
 * To convert this units to pcbunits, use the conversion factor #UNITS3D_TO_UNITSPCB.
 *
 * m_Normals, m_Color and m_Texcoords are between 0.0f and 1.0f.
 */
struct SMESH
{
    unsigned int    m_VertexSize;   ///< Number of vertex in the arrays
    SFVEC3F        *m_Positions;    ///< Vertex position array
    SFVEC3F        *m_Normals;      ///< Vertex normals array
    SFVEC2F        *m_Texcoords;    ///< Vertex texture coordinates array, can be NULL
    SFVEC3F        *m_Color;        ///< Vertex color array, can be NULL
    unsigned int    m_FaceIdxSize;  ///< Number of elements of the m_FaceIdx array
    unsigned int   *m_FaceIdx;      ///< Triangle Face Indexes
    unsigned int    m_MaterialIdx;  ///< Material Index to be used in this mesh (must be < m_MaterialsSize )
};


/// Store the a model based on meshes and materials
struct S3DMODEL
{
    unsigned int    m_MeshesSize;       ///< Number of meshes in the array
    SMESH          *m_Meshes;           ///< The meshes list of this model

    unsigned int    m_MaterialsSize;    ///< Number of materials in the material array
    SMATERIAL      *m_Materials;        ///< The materials list of this model
};

#endif // C3DMODEL_H
