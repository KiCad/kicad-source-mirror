/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2014-2015 Mario Luzeiro <mrluzeiro@gmail.com>
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

#include <memory>
#include <boost/shared_ptr.hpp>
#include <vector>
#define GLM_FORCE_RADIANS
#include <glm/glm.hpp>
#include "3d_struct.h"
#include "3d_material.h"
#include "CBBox.h"

class S3D_MESH;

/** A smart pointer to an S3D_MESH object */
typedef boost::shared_ptr<S3D_MESH> S3D_MESH_PTR;

/** A container of smar S3D_MESH object pointers */
typedef std::vector<S3D_MESH_PTR> S3D_MESH_PTRS;

class S3D_MESH
{
public:

    S3D_MESH();
    ~S3D_MESH();

    void openGL_RenderAllChilds( bool aIsRenderingJustNonTransparentObjects,
                                 bool aIsRenderingJustTransparentObjects );

    S3D_MATERIAL                    *m_Materials;

    // Point and index list
    std::vector< S3D_VERTEX >       m_Point;
    std::vector< std::vector<int> > m_CoordIndex;
    std::vector< std::vector<int> > m_NormalIndex;
    std::vector< S3D_VERTEX >       m_PerFaceColor;
    std::vector< S3D_VERTEX >       m_PerFaceNormalsNormalized;
    std::vector< S3D_VERTEX >       m_PerVertexNormalsNormalized;
    std::vector< int >              m_MaterialIndexPerFace;
    std::vector< std::vector<int> > m_MaterialIndexPerVertex;
    S3D_MESH_PTRS                   childs;

    S3D_VERTEX  m_translation;
    glm::vec4   m_rotation;
    S3D_VERTEX  m_scale;

    CBBOX &getBBox();

private:
    std::vector< S3D_VERTEX >                 m_PerFaceNormalsRaw_X_PerFaceSquaredArea;
    std::vector< std::vector< S3D_VERTEX > >  m_PerFaceVertexNormals;
    std::vector< S3D_VERTEX >                 m_PointNormalized;

    std::vector< std::vector<int> >           m_InvalidCoordIndexes; //!TODO: check for invalid CoordIndex in file and remove the index and the same material index

    bool isPerFaceNormalsComputed;
    void calcPerFaceNormals ();

    bool isPointNormalizedComputed;
    void calcPointNormalized();

    bool isPerPointNormalsComputed;
    void calcPerPointNormals();

    bool isPerVertexNormalsVerified;
    void perVertexNormalsVerify_and_Repair();

    void calcBBox();
    void calcBBoxAllChilds();

    CBBOX   m_BBox;

    void openGL_Render( bool aIsRenderingJustNonTransparentObjects,
                        bool aIsRenderingJustTransparentObjects );
};

#endif
