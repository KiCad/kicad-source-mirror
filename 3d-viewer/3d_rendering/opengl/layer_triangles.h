/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2015-2016 Mario Luzeiro <mrluzeiro@ua.pt>
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

#ifndef TRIANGLE_DISPLAY_LIST_H
#define TRIANGLE_DISPLAY_LIST_H

#include "../../common_ogl/openGL_includes.h"
#include <plugins/3dapi/xv3d_types.h>
#include <geometry/shape_line_chain.h>
#include <geometry/shape_poly_set.h>
#include <raytracing/accelerators/container_2d.h>
#include <vector>
#include <mutex>


typedef std::vector< SFVEC3F > SFVEC3F_VECTOR;


/**
 * Container to manage a vector of triangles.
 */
class TRIANGLE_LIST
{
public:

    /**
     * @param aNrReservedTriangles is number of triangles expected to be used.
     * @param aReserveNormals if you use normals, set it to bool to  reserve space.
     */
    TRIANGLE_LIST( unsigned int aNrReservedTriangles, bool aReserveNormals );

    /**
     * Reserve more triangles.
     */
    void Reserve_More( unsigned int aNrReservedTriangles, bool aReserveNormals );

    void AddTriangle( const SFVEC3F& aV1, const SFVEC3F& aV2, const SFVEC3F& aV3 );

    void AddQuad( const SFVEC3F& aV1, const SFVEC3F& aV2, const SFVEC3F& aV3, const SFVEC3F& aV4 );

    void AddNormal( const SFVEC3F& aN1, const SFVEC3F& aN2, const SFVEC3F& aN3 );

    void AddNormal( const SFVEC3F& aN1, const SFVEC3F& aN2, const SFVEC3F& aN3,
                    const SFVEC3F& aN4 );

    /**
     * Get the array of vertices.
     *
     * @return a pointer to the start of array vertex.
     */
    const float* GetVertexPointer() const { return (const float *)&m_vertexs[0].x; }

    /**
     * Get the array of normals.
     *
     * @return a pointer to start of array of normals.
     */
    const float* GetNormalsPointer() const { return (const float*) &m_normals[0].x; }

    unsigned int GetVertexSize() const { return (unsigned int) m_vertexs.size(); }

    unsigned int GetNormalsSize() const { return (unsigned int) m_normals.size(); }

private:
    SFVEC3F_VECTOR m_vertexs;  ///< vertex array
    SFVEC3F_VECTOR m_normals;  ///< normals array
};


/**
 * Store arrays of triangles to be used to create display lists.
 */
class TRIANGLE_DISPLAY_LIST
{
public:
    /**
     * Initialize arrays with reserved triangles.
     *
     * @param aNrReservedTriangles is the number of triangles to reserve.
     */
    explicit TRIANGLE_DISPLAY_LIST( unsigned int aNrReservedTriangles );

    ~TRIANGLE_DISPLAY_LIST();

    /**
     * Check if the vertex arrays of the layers are as expected.
     *
     * @return true if layers are correctly setup.
     */
    bool IsLayersSizeValid();


    void AddToMiddleContours( const SHAPE_LINE_CHAIN& outlinePath, float zBot, float zTop,
                              double aBiuTo3Du, bool aInvertFaceDirection,
                              const BVH_CONTAINER_2D* aThroughHoles = nullptr );

    void AddToMiddleContours( const SHAPE_POLY_SET& aPolySet, float zBot, float zTop,
                              double aBiuTo3Du, bool aInvertFaceDirection,
                              const BVH_CONTAINER_2D* aThroughHoles = nullptr );

    void AddToMiddleContours( const std::vector< SFVEC2F >& aContourPoints, float zBot,
                              float zTop, bool aInvertFaceDirection,
                              const BVH_CONTAINER_2D* aThroughHoles = nullptr );

    std::mutex m_middle_layer_lock;

    TRIANGLE_LIST* m_layer_top_segment_ends;
    TRIANGLE_LIST* m_layer_top_triangles;
    TRIANGLE_LIST* m_layer_middle_contours_quads;
    TRIANGLE_LIST* m_layer_bot_triangles;
    TRIANGLE_LIST* m_layer_bot_segment_ends;
};


/**
 * Store the OpenGL display lists to related with a layer.
 */
class OPENGL_RENDER_LIST
{
public:
    /**
     * Create the display lists for a layer.
     *
     * @param aLayerTriangles contains the layers array of vertex to render to display lists.
     * @param aTextureIndexForSegEnds is the texture index to be used by segment ends.
     *                                It is a black and white squared texture
     *                                with a center circle diameter of the size
     *                                of the texture.
     */
    OPENGL_RENDER_LIST( const TRIANGLE_DISPLAY_LIST& aLayerTriangles,
                        GLuint aTextureIndexForSegEnds, float aZBot, float aZTop );

    /**
     * Destroy this class while free the display lists from GPU memory.
     */
    ~OPENGL_RENDER_LIST();

    /**
     * Call the display lists for the top elements and middle contours.
     */
    void DrawTopAndMiddle() const;

    /**
     * Call the display lists for the bottom elements and middle contours.
     */
    void DrawBotAndMiddle() const;

    /**
     * Call the display lists for the top elements.
     */
    void DrawTop() const;

    /**
     * Call the display lists for the bottom elements.
     */
    void DrawBot() const;

    /**
     * Call the display lists for the middle elements.
     */
    void DrawMiddle() const;

    /**
     * Call to draw all the display lists.
     */
    void DrawAll( bool aDrawMiddle = true ) const;

    /**
     * Draw all layers if they are visible by the camera if camera position is above the layer.
     *
     * This only works because the board is centered and the planes are always perpendicular to
     * the Z axis.
     *
     * @param zCameraPos is the camera z axis position.
     */
    void DrawCulled( bool aDrawMiddle,
                        const OPENGL_RENDER_LIST* aSubtractList = nullptr,
                        const OPENGL_RENDER_LIST* bSubtractList = nullptr,
                        const OPENGL_RENDER_LIST* cSubtractList = nullptr,
                        const OPENGL_RENDER_LIST* dSubtractList = nullptr ) const;

    void ApplyScalePosition( float aZposition, float aZscale );
    void ApplyScalePosition( OPENGL_RENDER_LIST* aOtherList );

    void ClearScalePosition() { m_haveTransformation = false; }

    void SetItIsTransparent( bool aSetTransparent );

    float GetZBot() const { return m_zBot; }
    float GetZTop() const { return m_zTop; }

private:
    GLuint generate_top_or_bot_seg_ends( const TRIANGLE_LIST* aTriangleContainer,
                                         bool aIsNormalUp, GLuint aTextureId ) const;

    GLuint generate_top_or_bot_triangles( const TRIANGLE_LIST* aTriangleContainer,
                                          bool aIsNormalUp ) const;

    GLuint generate_middle_triangles( const TRIANGLE_LIST* aTriangleContainer ) const;

    void beginTransformation() const;
    void endTransformation() const;

    void setBlendfunction() const;

private:
    float   m_zBot;
    float   m_zTop;
    GLuint  m_layer_top_segment_ends;
    GLuint  m_layer_top_triangles;
    GLuint  m_layer_middle_contours_quads;
    GLuint  m_layer_bot_triangles;
    GLuint  m_layer_bot_segment_ends;

    bool    m_haveTransformation;
    float   m_zPositionTransformation;
    float   m_zScaleTransformation;

    bool    m_draw_it_transparent;
};

#endif // TRIANGLE_DISPLAY_LIST_H
