/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2015 Mario Luzeiro <mrluzeiro@ua.pt>
 * Copyright (C) 1992-2015 KiCad Developers, see AUTHORS.txt for contributors.
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
 * @file  c3d_render_createscene_ogl_legacy.cpp
 * @brief
 */

#include "c3d_render_ogl_legacy.h"
#include "ogl_legacy_utils.h"
#include "geometry/shape_poly_set.h"

#include "../3d_render_raytracing/shapes2D/cpolygon2d.h"
#include "../3d_render_raytracing/shapes2D/ctriangle2d.h"
#include "../3d_render_raytracing/shapes2D/cpolygon4pts2d.h"
#include "../3d_render_raytracing/shapes2D/cfilledcircle2d.h"
#include "../3d_render_raytracing/shapes2D/cring2d.h"
#include "3d_math/3d_math.h"
#include "3d_math/3d_fastmath.h"
#include <trigo.h>

void C3D_RENDER_OGL_LEGACY::reload()
{
    m_reloadRequested = false;

    ogl_free_all_display_lists();

    COBJECT2D_STATS::Instance().ResetStats();

    m_settings.InitSettings();

    SFVEC3F camera_pos = m_settings.GetBoardCenter3DU();
    m_settings.CameraGet().SetBoardLookAtPos( camera_pos );

    // Create Board
    // /////////////////////////////////////////////////////////////////////////
    printf("Create board...\n");
    CCONTAINER2D boardContainer;
    Convert_shape_line_polygon_to_triangles( m_settings.GetBoardPoly(),
                                             boardContainer,
                                             m_settings.BiuTo3Dunits(),
                                             (const BOARD_ITEM &)*m_settings.GetBoard() );

    const LIST_OBJECT2D listBoardObject2d = boardContainer.GetList();

    if( listBoardObject2d.size() > 0 )
    {
        /*
        float layer_z_top = m_settings.GetLayerBottomZpos3DU( F_Cu );
        float layer_z_bot = m_settings.GetLayerBottomZpos3DU( B_Cu );
*/
        float layer_z_top = m_settings.GetLayerBottomZpos3DU( B_Mask );
        float layer_z_bot = m_settings.GetLayerTopZpos3DU( B_Mask );
        CLAYER_TRIANGLES *layerTriangles = new CLAYER_TRIANGLES( listBoardObject2d.size() );

        for( LIST_OBJECT2D::const_iterator itemOnLayer = listBoardObject2d.begin();
             itemOnLayer != listBoardObject2d.end();
             itemOnLayer++ )
        {
            const COBJECT2D *object2d_A = static_cast<const COBJECT2D *>(*itemOnLayer);

            wxASSERT( object2d_A->GetObjectType() == OBJ2D_TRIANGLE );

            const CTRIANGLE2D *tri = (const CTRIANGLE2D *)object2d_A;
            const SFVEC2F &v1 = tri->GetP1();
            const SFVEC2F &v2 = tri->GetP2();
            const SFVEC2F &v3 = tri->GetP3();

            add_triangle_top_bot( layerTriangles, v1, v2, v3, layer_z_top, layer_z_bot );
        }

        const SHAPE_POLY_SET boardPoly = m_settings.GetBoardPoly();
        SHAPE_POLY_SET boardPolyCopy = boardPoly;

        boardPolyCopy.Simplify( SHAPE_POLY_SET::PM_FAST );

        if( boardPolyCopy.OutlineCount() == 1 )
        {
            const SHAPE_LINE_CHAIN& outlinePath = boardPolyCopy.COutline( 0 );

            std::vector< SFVEC2F > contournPoints;
            contournPoints.clear();
            contournPoints.reserve( outlinePath.PointCount() + 2 );

            for( unsigned int i = 0; i < (unsigned int)outlinePath.PointCount(); ++i )
            {
                const VECTOR2I& v = outlinePath.CPoint( i );
                contournPoints.push_back( SFVEC2F( v.x * m_settings.BiuTo3Dunits(),
                                                  -v.y * m_settings.BiuTo3Dunits() ) );
            }
            contournPoints.push_back( contournPoints[0] );

            if( contournPoints.size() > 4 )
            {
                // Calculate normals of each segment of the contourn
                std::vector< SFVEC2F > contournNormals;
                contournNormals.clear();
                contournNormals.reserve( contournPoints.size() );
                for( unsigned int i = 0; i < ( contournPoints.size() - 1 ); ++i )
                {
                    const SFVEC2F &v0 = contournPoints[i + 0];
                    const SFVEC2F &v1 = contournPoints[i + 1];

                    SFVEC2F n = glm::normalize( v1 - v0 );
                    contournNormals.push_back( SFVEC2F( -n.y, n.x ) );
                }

                SFVEC2F lastNormal = contournNormals[contournPoints.size() - 2];
                for( unsigned int i = 0; i < ( contournPoints.size() - 1 ); ++i )
                {
                    const SFVEC2F &v0 = contournPoints[i + 0];
                    const SFVEC2F &v1 = contournPoints[i + 1];

                    layerTriangles->m_layer_middle_contourns_quads->AddQuad( SFVEC3F( v0.x, v0.y, NextFloatUp( layer_z_top ) ),
                                                                             SFVEC3F( v1.x, v1.y, NextFloatUp( layer_z_top ) ),
                                                                             SFVEC3F( v1.x, v1.y, NextFloatDown( layer_z_bot ) ),
                                                                             SFVEC3F( v0.x, v0.y, NextFloatDown( layer_z_bot ) ) );

                    SFVEC2F n0 = contournNormals[i];

                    if( glm::dot( n0, lastNormal ) > 0.5f )
                        n0 += lastNormal;
                    else
                        n0 += contournNormals[i];

                    const SFVEC2F &nextNormal = contournNormals[ (i + 1) % (contournPoints.size() - 1) ];
                    SFVEC2F n1 = contournNormals[i];

                    if( glm::dot( n1, nextNormal ) > 0.5f )
                        n1 += nextNormal;
                    else
                        n1 += contournNormals[i];

                    n0 = glm::normalize( n0 );
                    n1 = glm::normalize( n1 );

                    const SFVEC3F n3d0 = SFVEC3F( n0.x, n0.y, 0.0f );
                    const SFVEC3F n3d1 = SFVEC3F( n1.x, n1.y, 0.0f );

                    layerTriangles->m_layer_middle_contourns_quads->AddNormal( n3d0, n3d1, n3d1, n3d0 );

                    lastNormal = contournNormals[i];
/*
                    const SFVEC2F n0 = glm::normalize( v0 - center );
                    const SFVEC2F n1 = glm::normalize( v1 - center );
                    const SFVEC3F n0z = SFVEC3F( n0.x, n0.y, 0.0f );
                    const SFVEC3F n1z = SFVEC3F( n1.x, n1.y, 0.0f );
                    layerTriangles->m_layer_middle_contourns_quads->AddNormal( n0z, n1z, n1z, n0z );*/
                }
            }
            contournPoints.clear();
        }

        m_ogl_disp_list_board = new CLAYERS_OGL_DISP_LISTS( *layerTriangles, m_ogl_circle_texture, SFVEC3F(0.65f,0.55f,0.05f) );

        delete layerTriangles;
    }


    float calc_sides_min_factor = (float)(   10.0 * IU_PER_MILS * m_settings.BiuTo3Dunits() );
    float calc_sides_max_factor = (float)( 1000.0 * IU_PER_MILS * m_settings.BiuTo3Dunits() );


    // Add layers maps (except B_Mask and F_Mask)
    // /////////////////////////////////////////////////////////////////////////
    printf("Add layers maps...\n");
    for( MAP_CONTAINER_2D::const_iterator ii = m_settings.GetMapLayers().begin();
         ii != m_settings.GetMapLayers().end();
         ii++ )
    {
        LAYER_ID layer_id = static_cast<LAYER_ID>(ii->first);

        if( !m_settings.Is3DLayerEnabled( layer_id ) )
            continue;

        const CBVHCONTAINER2D *container2d = static_cast<const CBVHCONTAINER2D *>(ii->second);
        const LIST_OBJECT2D listObject2d = container2d->GetList();

        if( listObject2d.size() == 0 )
            continue;

        //CMATERIAL *materialLayer = &m_materials.m_SilkS;
        SFVEC3F layerColor = SFVEC3F( 0.3f, 0.4f, 0.5f );

        float layer_z_bot = m_settings.GetLayerBottomZpos3DU( layer_id );
        float layer_z_top = m_settings.GetLayerTopZpos3DU( layer_id );

        if( layer_z_top < layer_z_bot )
        {
            float tmpFloat = layer_z_bot;
            layer_z_bot = layer_z_top;
            layer_z_top = tmpFloat;
        }

        layer_z_bot -= m_settings.GetNonCopperLayerThickness3DU();
        layer_z_top += m_settings.GetNonCopperLayerThickness3DU();

        if( m_settings.GetFlag( FL_USE_REALISTIC_MODE ) )
        {
            switch( layer_id )
            {
                case B_Adhes:
                case F_Adhes:

                break;

                case B_Paste:
                case F_Paste:
    //                materialLayer = &m_materials.m_Paste;
                break;

                case B_SilkS:
                case F_SilkS:

    //                materialLayer = &m_materials.m_SilkS;
    //                layerColor = g_silkscreenColor;
                break;

                case Dwgs_User:
                case Cmts_User:
                case Eco1_User:
                case Eco2_User:
                case Edge_Cuts:
                case Margin:
                break;

                case B_CrtYd:
                case F_CrtYd:
                break;

                case B_Fab:
                case F_Fab:
                break;

                default:
                    //materialLayer = &m_materials.m_Copper;

                        //layerColor = g_copperColor;


                break;
            }
        }
        else
        {
            layerColor = m_settings.GetLayerColor( layer_id );

        }


        // Calculate an estiation for then nr of triangles based on the nr of objects
        unsigned int nrTrianglesEstimation = listObject2d.size() * 8;

        CLAYER_TRIANGLES *layerTriangles = new CLAYER_TRIANGLES( nrTrianglesEstimation );

        m_triangles[layer_id] = layerTriangles;

        for( LIST_OBJECT2D::const_iterator itemOnLayer = listObject2d.begin();
             itemOnLayer != listObject2d.end();
             itemOnLayer++ )
        {
            const COBJECT2D *object2d_A = static_cast<const COBJECT2D *>(*itemOnLayer);

            switch( object2d_A->GetObjectType() )
            {

                case OBJ2D_FILLED_CIRCLE:
                {
                    const CFILLEDCIRCLE2D *filledCircle = (const CFILLEDCIRCLE2D *)object2d_A;
                    const SFVEC2F &center = filledCircle->GetCenter();
                    float radius = filledCircle->GetRadius() * 2.0f;            // Double because the render triangle
                    float radiusSquared = radius * radius;

                    const float f = (sqrtf(2.0f) / 2.0f) * radius * 0.9;// * texture_factor;

                    layerTriangles->m_layer_top_segment_ends->AddTriangle( SFVEC3F( center.x + f, center.y, layer_z_top ),
                                                                           SFVEC3F( center.x - f, center.y, layer_z_top ),
                                                                           SFVEC3F( center.x,
                                                                                    center.y - f, layer_z_top ) );

                    layerTriangles->m_layer_top_segment_ends->AddTriangle( SFVEC3F( center.x - f, center.y, layer_z_top ),
                                                                           SFVEC3F( center.x + f, center.y, layer_z_top ),
                                                                           SFVEC3F( center.x,
                                                                                    center.y + f, layer_z_top ) );

                    layerTriangles->m_layer_bot_segment_ends->AddTriangle( SFVEC3F( center.x - f, center.y, layer_z_bot ),
                                                                           SFVEC3F( center.x + f, center.y, layer_z_bot ),
                                                                           SFVEC3F( center.x,
                                                                                    center.y - f, layer_z_bot ) );

                    layerTriangles->m_layer_bot_segment_ends->AddTriangle( SFVEC3F( center.x + f, center.y, layer_z_bot ),
                                                                           SFVEC3F( center.x - f, center.y, layer_z_bot ),
                                                                           SFVEC3F( center.x,
                                                                                    center.y + f, layer_z_bot ) );

                    unsigned int nr_sides_per_circle = (unsigned int)mapf( radiusSquared,
                                                                           calc_sides_min_factor, calc_sides_max_factor,
                                                                           24.0f, 256.0f );

                    wxASSERT( nr_sides_per_circle >= 24 );

                    // Normal radius for the circle
                    radius = filledCircle->GetRadius();

                    std::vector< SFVEC2F > contournPoints;

                    contournPoints.clear();
                    contournPoints.reserve( nr_sides_per_circle + 2 );
                    int    delta       = 3600 / nr_sides_per_circle;
                    int ii;
                    for( ii = 0; ii < 3600; ii += delta )
                    {
                        const SFVEC2F rotatedDir = glm::rotate( SFVEC2F( 0.0f, 1.0f ),  (float)ii  * 2.0f * 3.14f / 3600.0f );
                        contournPoints.push_back(   SFVEC2F( center.x - rotatedDir.y * radius, center.y + rotatedDir.x * radius ) );
                    }
                    contournPoints.push_back( contournPoints[0] );

                    if( contournPoints.size() > 1 )
                    {
                        for( unsigned int i = 0; i < ( contournPoints.size() - 1 ); ++i )
                        {
                            const SFVEC2F &v0 = contournPoints[i + 0];
                            const SFVEC2F &v1 = contournPoints[i + 1];

                            layerTriangles->m_layer_middle_contourns_quads->AddQuad( SFVEC3F( v0.x, v0.y, NextFloatUp( layer_z_bot ) ),
                                                                                     SFVEC3F( v1.x, v1.y, NextFloatUp( layer_z_bot ) ),
                                                                                     SFVEC3F( v1.x, v1.y, NextFloatDown( layer_z_top ) ),
                                                                                     SFVEC3F( v0.x, v0.y, NextFloatDown( layer_z_top ) ) );


                            const SFVEC2F n0 = glm::normalize( v0 - center );
                            const SFVEC2F n1 = glm::normalize( v1 - center );
                            const SFVEC3F n0z = SFVEC3F( n0.x, n0.y, 0.0f );
                            const SFVEC3F n1z = SFVEC3F( n1.x, n1.y, 0.0f );
                            layerTriangles->m_layer_middle_contourns_quads->AddNormal( n0z, n1z, n1z, n0z );
                        }
                    }
                    contournPoints.clear();

                }
                break;

                case OBJ2D_DUMMYBLOCK:
                {
                }
                break;

                case OBJ2D_POLYGON4PT:
                {
                    const CPOLYGON4PTS2D *poly = (const CPOLYGON4PTS2D *)object2d_A;
                    const SFVEC2F &v0 = poly->GetV0();
                    const SFVEC2F &v1 = poly->GetV1();
                    const SFVEC2F &v2 = poly->GetV2();
                    const SFVEC2F &v3 = poly->GetV3();

                    add_triangle_top_bot( layerTriangles, v0, v2, v1, layer_z_top, layer_z_bot );
                    add_triangle_top_bot( layerTriangles, v2, v0, v3, layer_z_top, layer_z_bot );

                    const SFVEC2F &n0 = poly->GetN0();
                    const SFVEC2F &n1 = poly->GetN1();
                    const SFVEC2F &n2 = poly->GetN2();
                    const SFVEC2F &n3 = poly->GetN3();

                    const SFVEC3F n3d0 = SFVEC3F(-n0.y, n0.x, 0.0f );
                    const SFVEC3F n3d1 = SFVEC3F(-n1.y, n1.x, 0.0f );
                    const SFVEC3F n3d2 = SFVEC3F(-n2.y, n2.x, 0.0f );
                    const SFVEC3F n3d3 = SFVEC3F(-n3.y, n3.x, 0.0f );

                    layerTriangles->m_layer_middle_contourns_quads->AddQuad( SFVEC3F( v0.x, v0.y, layer_z_bot ),
                                                                             SFVEC3F( v1.x, v1.y, layer_z_bot ),
                                                                             SFVEC3F( v1.x, v1.y, layer_z_top ),
                                                                             SFVEC3F( v0.x, v0.y, layer_z_top ) );
                    layerTriangles->m_layer_middle_contourns_quads->AddNormal( n3d0, n3d0, n3d0, n3d0 );


                    layerTriangles->m_layer_middle_contourns_quads->AddQuad( SFVEC3F( v2.x, v2.y, layer_z_top ),
                                                                             SFVEC3F( v1.x, v1.y, layer_z_top ),
                                                                             SFVEC3F( v1.x, v1.y, layer_z_bot ),
                                                                             SFVEC3F( v2.x, v2.y, layer_z_bot ) );
                    layerTriangles->m_layer_middle_contourns_quads->AddNormal( n3d1, n3d1, n3d1, n3d1 );


                    layerTriangles->m_layer_middle_contourns_quads->AddQuad( SFVEC3F( v3.x, v3.y, layer_z_top ),
                                                                             SFVEC3F( v2.x, v2.y, layer_z_top ),
                                                                             SFVEC3F( v2.x, v2.y, layer_z_bot ),
                                                                             SFVEC3F( v3.x, v3.y, layer_z_bot ) );
                    layerTriangles->m_layer_middle_contourns_quads->AddNormal( n3d2, n3d2, n3d2, n3d2 );


                    layerTriangles->m_layer_middle_contourns_quads->AddQuad( SFVEC3F( v0.x, v0.y, layer_z_top ),
                                                                             SFVEC3F( v3.x, v3.y, layer_z_top ),
                                                                             SFVEC3F( v3.x, v3.y, layer_z_bot ),
                                                                             SFVEC3F( v0.x, v0.y, layer_z_bot ) );
                    layerTriangles->m_layer_middle_contourns_quads->AddNormal( n3d3, n3d3, n3d3, n3d3 );
                }
                break;


                case OBJ2D_RING:
                {
                    const CRING2D *ring = (const CRING2D *)object2d_A;
                    const SFVEC2F &center = ring->GetCenter();
                    float inner = ring->GetInnerRadius();
                    float outer = ring->GetOuterRadius();

                    unsigned int nr_sides_per_circle = (unsigned int)mapf( outer,
                                                                           calc_sides_min_factor, calc_sides_max_factor,
                                                                           24.0f, 256.0f );

                    wxASSERT( nr_sides_per_circle >= 24 );


                    std::vector< SFVEC2F > innerContour;
                    std::vector< SFVEC2F > outerContour;
                    innerContour.clear();
                    innerContour.reserve( nr_sides_per_circle + 2 );

                    outerContour.clear();
                    outerContour.reserve( nr_sides_per_circle + 2 );

                    int    delta       = 3600 / nr_sides_per_circle;
                    for( int ii = 0; ii < 3600; ii += delta )
                    {
                        const SFVEC2F rotatedDir = glm::rotate( SFVEC2F( 0.0f, 1.0f),  (float)        ii  * 2.0f * 3.14f / 3600.0f );

                        innerContour.push_back( SFVEC2F( center.x - rotatedDir.y * inner, center.y + rotatedDir.x * inner ) );
                        outerContour.push_back( SFVEC2F( center.x - rotatedDir.y * outer, center.y + rotatedDir.x * outer ) );
                    }

                    innerContour.push_back( innerContour[0] );
                    outerContour.push_back( outerContour[0] );

                    wxASSERT( innerContour.size() == outerContour.size() );

                    for( unsigned int i = 0; i < ( innerContour.size() - 1 ); ++i )
                    {
                        const SFVEC2F &vi0 = innerContour[i + 0];
                        const SFVEC2F &vi1 = innerContour[i + 1];
                        const SFVEC2F &vo0 = outerContour[i + 0];
                        const SFVEC2F &vo1 = outerContour[i + 1];

                        layerTriangles->m_layer_top_triangles->AddQuad( SFVEC3F( vi1.x, vi1.y, layer_z_top ),
                                                                        SFVEC3F( vi0.x, vi0.y, layer_z_top ),
                                                                        SFVEC3F( vo0.x, vo0.y, layer_z_top ),
                                                                        SFVEC3F( vo1.x, vo1.y, layer_z_top ) );

                        layerTriangles->m_layer_bot_triangles->AddQuad( SFVEC3F( vi1.x, vi1.y, layer_z_bot ),
                                                                        SFVEC3F( vo1.x, vo1.y, layer_z_bot ),
                                                                        SFVEC3F( vo0.x, vo0.y, layer_z_bot ),
                                                                        SFVEC3F( vi0.x, vi0.y, layer_z_bot ) );
                    }

                    for( unsigned int i = 0; i < ( innerContour.size() - 1 ); ++i )
                    {
                        const SFVEC2F &v0 = innerContour[i + 0];
                        const SFVEC2F &v1 = innerContour[i + 1];

                        layerTriangles->m_layer_middle_contourns_quads->AddQuad( SFVEC3F( v1.x, v1.y, NextFloatUp( layer_z_bot ) ),
                                                                                 SFVEC3F( v0.x, v0.y, NextFloatUp( layer_z_bot ) ),
                                                                                 SFVEC3F( v0.x, v0.y, NextFloatDown( layer_z_top ) ),
                                                                                 SFVEC3F( v1.x, v1.y, NextFloatDown( layer_z_top ) ) );


                        const SFVEC2F n0 = glm::normalize( v0 - center );
                        const SFVEC2F n1 = glm::normalize( v1 - center );
                        const SFVEC3F n0z = SFVEC3F( n0.x, n0.y, 0.0f );
                        const SFVEC3F n1z = SFVEC3F( n1.x, n1.y, 0.0f );
                        layerTriangles->m_layer_middle_contourns_quads->AddNormal( n0z, n1z, n1z, n0z );
                    }


                    for( unsigned int i = 0; i < ( outerContour.size() - 1 ); ++i )
                    {
                        const SFVEC2F &v0 = outerContour[i + 0];
                        const SFVEC2F &v1 = outerContour[i + 1];

                        layerTriangles->m_layer_middle_contourns_quads->AddQuad( SFVEC3F( v0.x, v0.y, NextFloatUp( layer_z_bot ) ),
                                                                                 SFVEC3F( v1.x, v1.y, NextFloatUp( layer_z_bot ) ),
                                                                                 SFVEC3F( v1.x, v1.y, NextFloatDown( layer_z_top ) ),
                                                                                 SFVEC3F( v0.x, v0.y, NextFloatDown( layer_z_top ) ) );


                        const SFVEC2F n0 = glm::normalize( v0 - center );
                        const SFVEC2F n1 = glm::normalize( v1 - center );
                        const SFVEC3F n0z = SFVEC3F( n0.x, n0.y, 0.0f );
                        const SFVEC3F n1z = SFVEC3F( n1.x, n1.y, 0.0f );
                        layerTriangles->m_layer_middle_contourns_quads->AddNormal( n0z, n1z, n1z, n0z );
                    }

                }
                break;


                case OBJ2D_TRIANGLE:
                {
                    const CTRIANGLE2D *tri = (const CTRIANGLE2D *)object2d_A;
                    const SFVEC2F &v1 = tri->GetP1();
                    const SFVEC2F &v2 = tri->GetP2();
                    const SFVEC2F &v3 = tri->GetP3();

                    add_triangle_top_bot( layerTriangles, v1, v2, v3, layer_z_top, layer_z_bot );
                }
                break;

                case OBJ2D_ROUNDSEG:
                {
                    const CROUNDSEGMENT2D &roundSeg = (const CROUNDSEGMENT2D &) *object2d_A;
                    unsigned int nr_sides_per_circle = (unsigned int)mapf( roundSeg.GetWidth(),
                                                                           calc_sides_min_factor, calc_sides_max_factor,
                                                                           24.0f, 256.0f );

                    wxASSERT( nr_sides_per_circle >= 24 );

                    SFVEC2F leftStart   = roundSeg.GetLeftStar();
                    SFVEC2F leftEnd     = roundSeg.GetLeftEnd();
                    SFVEC2F leftDir     = roundSeg.GetLeftDir();

                    SFVEC2F rightStart  = roundSeg.GetRightStar();
                    SFVEC2F rightEnd    = roundSeg.GetRightEnd();
                    SFVEC2F rightDir    = roundSeg.GetRightDir();
                    float   radius      = roundSeg.GetRadius();

                    SFVEC2F start       = roundSeg.GetStart();
                    SFVEC2F end         = roundSeg.GetEnd();

                    float texture_factor = (12.0f/(float)SIZE_OF_CIRCLE_TEXTURE) + 1.0f;
                    float texture_factorF= ( 4.0f/(float)SIZE_OF_CIRCLE_TEXTURE) + 1.0f;

                    const float radius_of_the_square = sqrtf( roundSeg.GetRadiusSquared() * 2.0f );
                    const float radius_triangle_factor = (radius_of_the_square - radius) / radius;

                    const SFVEC2F factorS = SFVEC2F( -rightDir.y * radius * radius_triangle_factor, rightDir.x * radius * radius_triangle_factor );
                    const SFVEC2F factorE = SFVEC2F( -leftDir.y  * radius * radius_triangle_factor, leftDir.x  * radius * radius_triangle_factor );

                    // Top end segment triangles
                    layerTriangles->m_layer_top_segment_ends->AddTriangle( SFVEC3F( rightEnd.x   + texture_factor * factorS.x, rightEnd.y  + texture_factor * factorS.y, layer_z_top ),
                                                                           SFVEC3F( leftStart.x  + texture_factor * factorE.x, leftStart.y + texture_factor * factorE.y, layer_z_top ),
                                                                           SFVEC3F( start.x - texture_factorF * leftDir.x * radius * sqrtf(2.0f),
                                                                                    start.y - texture_factorF * leftDir.y * radius * sqrtf(2.0f), layer_z_top ) );

                    layerTriangles->m_layer_top_segment_ends->AddTriangle( SFVEC3F( leftEnd.x    + texture_factor * factorE.x, leftEnd.y    + texture_factor * factorE.y, layer_z_top ),
                                                                           SFVEC3F( rightStart.x + texture_factor * factorS.x, rightStart.y + texture_factor * factorS.y, layer_z_top ),
                                                                           SFVEC3F( end.x - texture_factorF * rightDir.x * radius * sqrtf(2.0f),
                                                                                    end.y - texture_factorF * rightDir.y * radius * sqrtf(2.0f), layer_z_top ) );

                    // Bot end segment triangles
                    layerTriangles->m_layer_bot_segment_ends->AddTriangle( SFVEC3F( leftStart.x  + texture_factor * factorE.x, leftStart.y + texture_factor * factorE.y, layer_z_bot ),
                                                                           SFVEC3F( rightEnd.x   + texture_factor * factorS.x, rightEnd.y  + texture_factor * factorS.y, layer_z_bot ),
                                                                           SFVEC3F( start.x - texture_factorF * leftDir.x * radius * sqrtf(2.0f),
                                                                                    start.y - texture_factorF * leftDir.y * radius * sqrtf(2.0f), layer_z_bot ) );

                    layerTriangles->m_layer_bot_segment_ends->AddTriangle( SFVEC3F( rightStart.x + texture_factor * factorS.x, rightStart.y + texture_factor * factorS.y, layer_z_bot ),
                                                                           SFVEC3F( leftEnd.x    + texture_factor * factorE.x, leftEnd.y    + texture_factor * factorE.y, layer_z_bot ),
                                                                           SFVEC3F( end.x - texture_factorF * rightDir.x * radius * sqrtf(2.0f),
                                                                                    end.y - texture_factorF * rightDir.y * radius * sqrtf(2.0f), layer_z_bot ) );

                    // Segment top and bot planes
                    layerTriangles->m_layer_top_triangles->AddQuad( SFVEC3F( rightEnd.x,   rightEnd.y,   layer_z_top ),
                                                                    SFVEC3F( rightStart.x, rightStart.y, layer_z_top ),
                                                                    SFVEC3F( leftEnd.x,    leftEnd.y,    layer_z_top ),
                                                                    SFVEC3F( leftStart.x,  leftStart.y,  layer_z_top ) );

                    layerTriangles->m_layer_bot_triangles->AddQuad( SFVEC3F( rightEnd.x,   rightEnd.y,   layer_z_bot ),
                                                                    SFVEC3F( leftStart.x,  leftStart.y,  layer_z_bot ),
                                                                    SFVEC3F( leftEnd.x,    leftEnd.y,    layer_z_bot ),
                                                                    SFVEC3F( rightStart.x, rightStart.y, layer_z_bot ) );

                    // Middle contourns (two sides of the segment)
                    layerTriangles->m_layer_middle_contourns_quads->AddQuad( SFVEC3F( leftStart.x, leftStart.y, layer_z_top ),
                                                                             SFVEC3F( leftEnd.x,   leftEnd.y,   layer_z_top ),
                                                                             SFVEC3F( leftEnd.x,   leftEnd.y,   layer_z_bot ),
                                                                             SFVEC3F( leftStart.x, leftStart.y, layer_z_bot ) );
                    const SFVEC3F leftNormal = SFVEC3F( -leftDir.y, leftDir.x, 0.0f );
                    layerTriangles->m_layer_middle_contourns_quads->AddNormal( leftNormal, leftNormal, leftNormal, leftNormal );


                    layerTriangles->m_layer_middle_contourns_quads->AddQuad( SFVEC3F( rightStart.x, rightStart.y, layer_z_top ),
                                                                             SFVEC3F( rightEnd.x,   rightEnd.y,   layer_z_top ),
                                                                             SFVEC3F( rightEnd.x,   rightEnd.y,   layer_z_bot ),
                                                                             SFVEC3F( rightStart.x, rightStart.y, layer_z_bot ) );
                    const SFVEC3F rightNormal = SFVEC3F( -rightDir.y, rightDir.x, 0.0f );
                    layerTriangles->m_layer_middle_contourns_quads->AddNormal( rightNormal, rightNormal, rightNormal, rightNormal );


                    // Compute the outlines of the segment, and creates a polygon
                    // add right rounded end:

                    std::vector< SFVEC2F > roundedEndPointsStart;
                    std::vector< SFVEC2F > roundedEndPointsEnd;
                    roundedEndPointsStart.clear();
                    roundedEndPointsStart.reserve( nr_sides_per_circle + 2 );

                    roundedEndPointsEnd.clear();
                    roundedEndPointsEnd.reserve( nr_sides_per_circle + 2 );

                    roundedEndPointsStart.push_back( SFVEC2F( leftStart.x, leftStart.y ) );
                    roundedEndPointsEnd.push_back(   SFVEC2F( leftEnd.x,   leftEnd.y ) );

                    int    delta       = 3600 / nr_sides_per_circle;
                    for( int ii = delta; ii < 1800; ii += delta )
                    {
                        const SFVEC2F rotatedDirL = glm::rotate( leftDir,  (float)        ii  * 2.0f * 3.14f / 3600.0f );
                        const SFVEC2F rotatedDirR = glm::rotate( rightDir, (float)(1800 - ii) * 2.0f * 3.14f / 3600.0f );
                        roundedEndPointsStart.push_back( SFVEC2F( start.x - rotatedDirL.y * radius, start.y + rotatedDirL.x * radius ) );
                        roundedEndPointsEnd.push_back(   SFVEC2F( end.x   - rotatedDirR.y * radius, end.y   + rotatedDirR.x * radius ) );
                    }
                    roundedEndPointsStart.push_back( SFVEC2F( rightEnd.x, rightEnd.y ) );
                    roundedEndPointsEnd.push_back(   SFVEC2F( rightStart.x, rightStart.y ) );

                    if( roundedEndPointsStart.size() > 1 )
                    {
                        for( unsigned int i = 0; i < ( roundedEndPointsStart.size() - 1 ); ++i )
                        {
                            const SFVEC2F &v0 = roundedEndPointsStart[i + 0];
                            const SFVEC2F &v1 = roundedEndPointsStart[i + 1];

                            layerTriangles->m_layer_middle_contourns_quads->AddQuad( SFVEC3F( v0.x, v0.y, layer_z_bot ),
                                                                                     SFVEC3F( v1.x, v1.y, layer_z_bot ),
                                                                                     SFVEC3F( v1.x, v1.y, layer_z_top ),
                                                                                     SFVEC3F( v0.x, v0.y, layer_z_top ) );


                            const SFVEC2F n0 = glm::normalize( v0 - start );
                            const SFVEC2F n1 = glm::normalize( v1 - start );
                            const SFVEC3F n0z = SFVEC3F( n0.x, n0.y, 0.0f );
                            const SFVEC3F n1z = SFVEC3F( n1.x, n1.y, 0.0f );
                            layerTriangles->m_layer_middle_contourns_quads->AddNormal( n0z, n1z, n1z, n0z );
                        }
                    }
                    roundedEndPointsStart.clear();

                    if( roundedEndPointsEnd.size() > 1 )
                    {
                        for( unsigned int i = 0; i < ( roundedEndPointsEnd.size() - 1 ); ++i )
                        {
                            const SFVEC2F &v0 = roundedEndPointsEnd[i + 0];
                            const SFVEC2F &v1 = roundedEndPointsEnd[i + 1];

                            layerTriangles->m_layer_middle_contourns_quads->AddQuad( SFVEC3F( v0.x, v0.y, layer_z_top ),
                                                                                     SFVEC3F( v1.x, v1.y, layer_z_top ),
                                                                                     SFVEC3F( v1.x, v1.y, layer_z_bot ),
                                                                                     SFVEC3F( v0.x, v0.y, layer_z_bot ) );


                            const SFVEC2F n0 = glm::normalize( v0 - end );
                            const SFVEC2F n1 = glm::normalize( v1 - end );
                            const SFVEC3F n0z = SFVEC3F( n0.x, n0.y, 0.0f );
                            const SFVEC3F n1z = SFVEC3F( n1.x, n1.y, 0.0f );
                            layerTriangles->m_layer_middle_contourns_quads->AddNormal( n0z, n1z, n1z, n0z );
                        }
                    }
                    roundedEndPointsEnd.clear();
                }
                break;

                default:
                {
                }
                break;
            }
#if 0
            // not yet used / implemented (can be used in future to clip the objects in the board borders
            COBJECT2D *object2d_C = CSGITEM_FULL;

            std::vector<const COBJECT2D *> *object2d_B = CSGITEM_EMPTY;

            if( m_settings.GetFlag( FL_RENDER_SHOW_HOLES_IN_ZONES ) )
            {
                object2d_B = new std::vector<const COBJECT2D *>();

                // Check if there are any layerhole that intersects this object
                // Eg: a segment is cutted by a via hole or THT hole.
                // /////////////////////////////////////////////////////////////
                const MAP_CONTAINER_2D &layerHolesMap = m_settings.GetMapLayersHoles();
                if( layerHolesMap.find(layer_id) != layerHolesMap.end() )
                {
                    MAP_CONTAINER_2D::const_iterator ii_hole = layerHolesMap.find(layer_id);
                    const CBVHCONTAINER2D *containerLayerHoles2d = static_cast<const CBVHCONTAINER2D *>(ii_hole->second);


                    CONST_LIST_OBJECT2D intersectionList;
                    containerLayerHoles2d->GetListObjectsIntersects( object2d_A->GetBBox(), intersectionList );

                    if( !intersectionList.empty() )
                    {
                        for( CONST_LIST_OBJECT2D::const_iterator holeOnLayer = intersectionList.begin();
                             holeOnLayer != intersectionList.end();
                             holeOnLayer++ )
                        {
                            const COBJECT2D *hole2d = static_cast<const COBJECT2D *>(*holeOnLayer);

                            //if( object2d_A->Intersects( hole2d->GetBBox() ) )
                                //if( object2d_A->GetBBox().Intersects( hole2d->GetBBox() ) )
                                    object2d_B->push_back( hole2d );
                        }
                    }
                }

                // Check if there are any THT that intersects this object
                // /////////////////////////////////////////////////////////////
                if( !m_settings.GetThroughHole_Inflated().GetList().empty() )
                {
                    CONST_LIST_OBJECT2D intersectionList;
                    m_settings.GetThroughHole_Inflated().GetListObjectsIntersects( object2d_A->GetBBox(), intersectionList );

                    if( !intersectionList.empty() )
                    {
                        for( CONST_LIST_OBJECT2D::const_iterator hole = intersectionList.begin();
                             hole != intersectionList.end();
                             hole++ )
                        {
                            const COBJECT2D *hole2d = static_cast<const COBJECT2D *>(*hole);

                            //if( object2d_A->Intersects( hole2d->GetBBox() ) )
                                //if( object2d_A->GetBBox().Intersects( hole2d->GetBBox() ) )
                                    object2d_B->push_back( hole2d );
                        }
                    }
                }

                if( object2d_B->empty() )
                {
                    delete object2d_B;
                    object2d_B = CSGITEM_EMPTY;
                }
            }

            if( (object2d_B == CSGITEM_EMPTY) &&
                (object2d_C == CSGITEM_FULL) )
            {
#if 0
               create_3d_object_from( m_object_container, object2d_A, m_settings.GetLayerBottomZpos3DU( layer_id ),
                                                                       m_settings.GetLayerTopZpos3DU( layer_id ),
                                       materialLayer,
                                       layerColor );
#else
                CLAYERITEM *objPtr = new CLAYERITEM( object2d_A, m_settings.GetLayerBottomZpos3DU( layer_id ),
                                                                 m_settings.GetLayerTopZpos3DU( layer_id ) );
                objPtr->SetMaterial( materialLayer );
                objPtr->SetColor( layerColor );
                m_object_container.Add( objPtr );
#endif
            }
            else
            {
#if 1
                CITEMLAYERCSG2D *itemCSG2d = new CITEMLAYERCSG2D( object2d_A, object2d_B, object2d_C,
                                                                  object2d_A->GetBoardItem() );
                m_containerWithObjectsToDelete.Add( itemCSG2d );

                CLAYERITEM *objPtr = new CLAYERITEM( itemCSG2d, m_settings.GetLayerBottomZpos3DU( layer_id ),
                                                     m_settings.GetLayerTopZpos3DU( layer_id ) );

                objPtr->SetMaterial( materialLayer );
                objPtr->SetColor( layerColor );
                m_object_container.Add( objPtr );
#endif
            }
#endif
        }

        // Create display list
        // /////////////////////////////////////////////////////////////////////
        m_ogl_disp_lists_layers[layer_id] = new CLAYERS_OGL_DISP_LISTS( *layerTriangles,
                                                                        m_ogl_circle_texture,
                                                                        layerColor );
    }// for each layer on map

}


void C3D_RENDER_OGL_LEGACY::add_triangle_top_bot( CLAYER_TRIANGLES *aDst, const SFVEC2F &v0, const SFVEC2F &v1, const SFVEC2F &v2, float top, float bot )
{
    aDst->m_layer_bot_triangles->AddTriangle( SFVEC3F( v0.x, v0.y, bot ),
                                              SFVEC3F( v1.x, v1.y, bot ),
                                              SFVEC3F( v2.x, v2.y, bot ) );

    aDst->m_layer_top_triangles->AddTriangle( SFVEC3F( v2.x, v2.y, top ),
                                              SFVEC3F( v1.x, v1.y, top ),
                                              SFVEC3F( v0.x, v0.y, top ) );
}
