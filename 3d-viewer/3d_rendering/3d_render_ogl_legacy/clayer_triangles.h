/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2015-2016 Mario Luzeiro <mrluzeiro@ua.pt>
 * Copyright (C) 1992-2016 KiCad Developers, see AUTHORS.txt for contributors.
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
 * @file  clayer_triangles.h
 * @brief
 */

#ifndef CLAYER_TRIANGLES_H_
#define CLAYER_TRIANGLES_H_

#include "../../common_ogl/openGL_includes.h"
#include <plugins/3dapi/xv3d_types.h>
#include <geometry/shape_line_chain.h>
#include <geometry/shape_poly_set.h>
#include <vector>


typedef std::vector< SFVEC3F > SFVEC3F_VECTOR;


/**
 * @brief The CLAYER_TRIANGLE_CONTAINER class stores an manage vector of triangles
 */
class CLAYER_TRIANGLE_CONTAINER
{

public:

    /**
     * @brief CLAYER_TRIANGLE_CONTAINER
     * @param aNrReservedTriangles: number of triangles expected to be used
     * @param aReserveNormals: if you will use normals, set it to bool to pre
     *        reserve space
     */
    CLAYER_TRIANGLE_CONTAINER( unsigned int aNrReservedTriangles, bool aReserveNormals );

    /**
     * @brief Reserve_More - reserve more triangles
     *
     */
    void Reserve_More( unsigned int aNrReservedTriangles, bool aReserveNormals );

    /**
     * @brief AddTriangle
     * @param aV1
     * @param aV2
     * @param aV3
     */
    void AddTriangle( const SFVEC3F &aV1, const SFVEC3F &aV2, const SFVEC3F &aV3 );

    /**
     * @brief AddQuad
     * @param aV1
     * @param aV2
     * @param aV3
     * @param aV4
     */
    void AddQuad( const SFVEC3F &aV1,
                  const SFVEC3F &aV2,
                  const SFVEC3F &aV3,
                  const SFVEC3F &aV4 );

    /**
     * @brief AddNormal
     * @param aN1
     * @param aN2
     * @param aN3
     */
    void AddNormal( const SFVEC3F &aN1, const SFVEC3F &aN2, const SFVEC3F &aN3 );

    /**
     * @brief AddNormal
     * @param aN1
     * @param aN2
     * @param aN3
     */
    void AddNormal( const SFVEC3F &aN1,
                    const SFVEC3F &aN2,
                    const SFVEC3F &aN3,
                    const SFVEC3F &aN4 );

    /**
     * @brief GetVertexPointer - Get the array of vertexes
     * @return The pointer to the start of array vertex
     */
    const float *GetVertexPointer() const { return (const float *)&m_vertexs[0].x; }

    /**
     * @brief GetNormalsPointer - Get the array of normals
     * @return The pointer to start of array of normals
     */
    const float *GetNormalsPointer() const { return (const float *)&m_normals[0].x; }

    /**
     * @brief GetVertexSize
     * @return
     */
    unsigned int GetVertexSize() const { return (unsigned int)m_vertexs.size(); }

    /**
     * @brief GetNormalsSize
     * @return
     */
    unsigned int GetNormalsSize() const { return (unsigned int)m_normals.size(); }

private:
    SFVEC3F_VECTOR m_vertexs;  ///< vertex array
    SFVEC3F_VECTOR m_normals;  ///< normals array
};


/**
 * @brief The CLAYER_TRIANGLES class stores arrays of triangles to be used to
 * create display lists
 */
class CLAYER_TRIANGLES
{
public:
    /**
     * @brief CLAYER_TRIANGLES - initialize arrays with reserved triangles
     * @param aNrReservedTriangles: number of pre alloc triangles to reserve
     */
    explicit CLAYER_TRIANGLES( unsigned int aNrReservedTriangles );

    /**
     * @brief ~CLAYER_TRIANGLES - Free containers
     */
    ~CLAYER_TRIANGLES();

    /**
     * @brief IsLayersSizeValid - check if the vertex arrays of the layers are
     * as expected
     * @return TRUE if layers are correctly setup
     */
    bool IsLayersSizeValid();


    void AddToMiddleContourns( const SHAPE_LINE_CHAIN &outlinePath,
                               float zBot,
                               float zTop,
                               double aBiuTo3Du,
                               bool aInvertFaceDirection );

    void AddToMiddleContourns( const SHAPE_POLY_SET &aPolySet,
                               float zBot,
                               float zTop,
                               double aBiuTo3Du,
                               bool aInvertFaceDirection );

    void AddToMiddleContourns( const std::vector< SFVEC2F > &aContournPoints,
                               float zBot,
                               float zTop,
                               bool aInvertFaceDirection );

    CLAYER_TRIANGLE_CONTAINER *m_layer_top_segment_ends;
    CLAYER_TRIANGLE_CONTAINER *m_layer_top_triangles;
    CLAYER_TRIANGLE_CONTAINER *m_layer_middle_contourns_quads;
    CLAYER_TRIANGLE_CONTAINER *m_layer_bot_triangles;
    CLAYER_TRIANGLE_CONTAINER *m_layer_bot_segment_ends;
};


/**
 * @brief The CLAYERS_OGL_DISP_LISTS class stores the openGL display lists to
 * related with a layer
 */
class CLAYERS_OGL_DISP_LISTS
{
public:
    /**
     * @brief CLAYERS_OGL_DISP_LISTS - Creates the display lists for a layer
     * @param aLayerTriangles: contains the layers array of vertex to render to
     *                         display lists
     * @param aTextureIndexForSegEnds: texture index to be used by segment ends.
     *                                 It is a black and white squared texture
     *                                 with a center circle diameter of the size
     *                                 of the texture.
     */
    CLAYERS_OGL_DISP_LISTS( const CLAYER_TRIANGLES &aLayerTriangles,
                            GLuint aTextureIndexForSegEnds,
                            float aZBot,
                            float aZTop );

    /**
     * @brief ~CLAYERS_OGL_DISP_LISTS - Destroy this class while free the display
     * lists from GPU mem
     */
    ~CLAYERS_OGL_DISP_LISTS();

    /**
     * @brief DrawTopAndMiddle - This function calls the display lists for the
     * top elements and middle contourns
     */
    void DrawTopAndMiddle() const;

    /**
     * @brief DrawBotAndMiddle - This function calls the display lists for the
     * botton elements and middle contourns
     */
    void DrawBotAndMiddle() const;

    /**
     * @brief DrawTop - This function calls the display lists for the top elements
     */
    void DrawTop() const;

    /**
     * @brief DrawBot - This function calls the display lists for the botton elements
     */
    void DrawBot() const;

    /**
     * @brief DrawMiddle - This function calls the display lists for the middle
     * elements
     */
    void DrawMiddle() const;

    /**
     * @brief DrawAll - This function calls all the display lists
     */
    void DrawAll( bool aDrawMiddle = true ) const;

    /**
     * @brief DrawAllCameraCulled - Draw all layers if they are visible by the camera.
     * i.e.: if camera position is above the layer. This only works because the
     * board is centered and the planes are always perpendicular to the Z axis.
     * @param zCameraPos: camera z position
     */
    void DrawAllCameraCulled( float zCameraPos, bool aDrawMiddle = true ) const;

    void DrawAllCameraCulledSubtractLayer( const CLAYERS_OGL_DISP_LISTS *aLayerToSubtractA,
                                           const CLAYERS_OGL_DISP_LISTS *aLayerToSubtractB,
                                           bool aDrawMiddle = true ) const;

    void ApplyScalePosition( float aZposition, float aZscale );

    void ClearScalePosition() { m_haveTransformation = false; }

    void SetItIsTransparent( bool aSetTransparent );

    float GetZBot() const { return m_zBot; }
    float GetZTop() const { return m_zTop; }

private:
    GLuint generate_top_or_bot_seg_ends( const CLAYER_TRIANGLE_CONTAINER * aTriangleContainer,
                                         bool aIsNormalUp,
                                         GLuint aTextureId ) const;

    GLuint generate_top_or_bot_triangles( const CLAYER_TRIANGLE_CONTAINER * aTriangleContainer,
                                          bool aIsNormalUp ) const;

    GLuint generate_middle_triangles( const CLAYER_TRIANGLE_CONTAINER * aTriangleContainer ) const;

    void beginTransformation() const;
    void endTransformation() const;

    void setBlendfunction() const;

private:
    float   m_zBot;
    float   m_zTop;
    GLuint  m_layer_top_segment_ends;
    GLuint  m_layer_top_triangles;
    GLuint  m_layer_middle_contourns_quads;
    GLuint  m_layer_bot_triangles;
    GLuint  m_layer_bot_segment_ends;

    bool    m_haveTransformation;
    float   m_zPositionTransformation;
    float   m_zScaleTransformation;

    bool    m_draw_it_transparent;
};

#endif // CLAYER_TRIANGLES_H_
