/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2015-2016 Mario Luzeiro <mrluzeiro@ua.pt>
 * Copyright (C) 2023 CERN
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

#include "render_3d_opengl.h"
#include <board.h>
#include <footprint.h>
#include "../../3d_math.h"
#include "convert_basic_shapes_to_polygon.h"
#include <lset.h>
#include <trigo.h>
#include <project.h>
#include <core/profile.h>        // To use GetRunningMicroSecs or another profiling utility
#include <fp_lib_table.h>
#include <eda_3d_viewer_frame.h>
#include <project_pcb.h>


void RENDER_3D_OPENGL::addObjectTriangles( const FILLED_CIRCLE_2D* aCircle,
                                           TRIANGLE_DISPLAY_LIST* aDstLayer, float aZtop,
                                           float aZbot )
{
    const SFVEC2F& center = aCircle->GetCenter();
    const float radius = aCircle->GetRadius() * 2.0f; // Double because the render triangle

    // This is a small adjustment to the circle texture
    const float texture_factor = ( 8.0f / (float) SIZE_OF_CIRCLE_TEXTURE ) + 1.0f;
    const float f = ( sqrtf( 2.0f ) / 2.0f ) * radius * texture_factor;

    // Top and Bot segments ends are just triangle semi-circles, so need to add it in duplicated.
    aDstLayer->m_layer_top_segment_ends->AddTriangle( SFVEC3F( center.x + f, center.y, aZtop ),
                                                      SFVEC3F( center.x - f, center.y, aZtop ),
                                                      SFVEC3F( center.x, center.y - f, aZtop ) );

    aDstLayer->m_layer_top_segment_ends->AddTriangle( SFVEC3F( center.x - f, center.y, aZtop ),
                                                      SFVEC3F( center.x + f, center.y, aZtop ),
                                                      SFVEC3F( center.x, center.y + f, aZtop ) );

    aDstLayer->m_layer_bot_segment_ends->AddTriangle( SFVEC3F( center.x - f, center.y, aZbot ),
                                                      SFVEC3F( center.x + f, center.y, aZbot ),
                                                      SFVEC3F( center.x, center.y - f, aZbot ) );

    aDstLayer->m_layer_bot_segment_ends->AddTriangle( SFVEC3F( center.x + f, center.y, aZbot ),
                                                      SFVEC3F( center.x - f, center.y, aZbot ),
                                                      SFVEC3F( center.x, center.y + f, aZbot ) );
}


void RENDER_3D_OPENGL::addObjectTriangles( const POLYGON_4PT_2D* aPoly,
                                           TRIANGLE_DISPLAY_LIST* aDstLayer,
                                           float aZtop, float aZbot )
{
    const SFVEC2F& v0 = aPoly->GetV0();
    const SFVEC2F& v1 = aPoly->GetV1();
    const SFVEC2F& v2 = aPoly->GetV2();
    const SFVEC2F& v3 = aPoly->GetV3();

    addTopAndBottomTriangles( aDstLayer, v0, v2, v1, aZtop, aZbot );
    addTopAndBottomTriangles( aDstLayer, v2, v0, v3, aZtop, aZbot );
}


void RENDER_3D_OPENGL::generateRing( const SFVEC2F& aCenter, float aInnerRadius,
                                     float aOuterRadius, unsigned int aNr_sides_per_circle,
                                     std::vector< SFVEC2F >& aInnerContourResult,
                                     std::vector< SFVEC2F >& aOuterContourResult,
                                     bool aInvertOrder )
{
    aInnerContourResult.clear();
    aInnerContourResult.reserve( aNr_sides_per_circle + 2 );

    aOuterContourResult.clear();
    aOuterContourResult.reserve( aNr_sides_per_circle + 2 );

    const int delta = 3600 / aNr_sides_per_circle;

    for( int ii = 0; ii < 3600; ii += delta )
    {
        float angle = (float)( aInvertOrder ? ( 3600 - ii ) : ii )
                               * 2.0f * glm::pi<float>() / 3600.0f;
        const SFVEC2F rotatedDir = SFVEC2F( cos( angle ), sin( angle ) );

        aInnerContourResult.emplace_back( aCenter.x + rotatedDir.x * aInnerRadius,
                                          aCenter.y + rotatedDir.y * aInnerRadius );

        aOuterContourResult.emplace_back( aCenter.x + rotatedDir.x * aOuterRadius,
                                          aCenter.y + rotatedDir.y * aOuterRadius );
    }

    aInnerContourResult.push_back( aInnerContourResult[0] );
    aOuterContourResult.push_back( aOuterContourResult[0] );

    wxASSERT( aInnerContourResult.size() == aOuterContourResult.size() );
}


void RENDER_3D_OPENGL::addObjectTriangles( const RING_2D* aRing, TRIANGLE_DISPLAY_LIST* aDstLayer,
                                           float aZtop, float aZbot )
{
    const SFVEC2F& center = aRing->GetCenter();
    const float inner = aRing->GetInnerRadius();
    const float outer = aRing->GetOuterRadius();

    std::vector< SFVEC2F > innerContour;
    std::vector< SFVEC2F > outerContour;

    generateRing( center, inner, outer, m_boardAdapter.GetCircleSegmentCount( outer * 2.0f ),
                  innerContour, outerContour, false );

    // This will add the top and bot quads that will form the approximated ring
    for( unsigned int i = 0; i < ( innerContour.size() - 1 ); ++i )
    {
        const SFVEC2F& vi0 = innerContour[i + 0];
        const SFVEC2F& vi1 = innerContour[i + 1];
        const SFVEC2F& vo0 = outerContour[i + 0];
        const SFVEC2F& vo1 = outerContour[i + 1];

        aDstLayer->m_layer_top_triangles->AddQuad( SFVEC3F( vi1.x, vi1.y, aZtop ),
                                                   SFVEC3F( vi0.x, vi0.y, aZtop ),
                                                   SFVEC3F( vo0.x, vo0.y, aZtop ),
                                                   SFVEC3F( vo1.x, vo1.y, aZtop ) );

        aDstLayer->m_layer_bot_triangles->AddQuad( SFVEC3F( vi1.x, vi1.y, aZbot ),
                                                   SFVEC3F( vo1.x, vo1.y, aZbot ),
                                                   SFVEC3F( vo0.x, vo0.y, aZbot ),
                                                   SFVEC3F( vi0.x, vi0.y, aZbot ) );
    }
}


void RENDER_3D_OPENGL::addObjectTriangles( const TRIANGLE_2D* aTri,
                                           TRIANGLE_DISPLAY_LIST* aDstLayer,
                                           float aZtop, float aZbot )
{
    const SFVEC2F& v1 = aTri->GetP1();
    const SFVEC2F& v2 = aTri->GetP2();
    const SFVEC2F& v3 = aTri->GetP3();

    addTopAndBottomTriangles( aDstLayer, v1, v2, v3, aZtop, aZbot );
}


void RENDER_3D_OPENGL::addObjectTriangles( const ROUND_SEGMENT_2D* aSeg,
                                           TRIANGLE_DISPLAY_LIST* aDstLayer,
                                           float aZtop, float aZbot )
{
    const SFVEC2F& leftStart   = aSeg->GetLeftStar();
    const SFVEC2F& leftEnd     = aSeg->GetLeftEnd();
    const SFVEC2F& leftDir     = aSeg->GetLeftDir();

    const SFVEC2F& rightStart  = aSeg->GetRightStar();
    const SFVEC2F& rightEnd    = aSeg->GetRightEnd();
    const SFVEC2F& rightDir    = aSeg->GetRightDir();
    const float    radius      = aSeg->GetRadius();

    const SFVEC2F& start       = aSeg->GetStart();
    const SFVEC2F& end         = aSeg->GetEnd();

    const float texture_factor  = ( 12.0f / (float) SIZE_OF_CIRCLE_TEXTURE ) + 1.0f;
    const float texture_factorF = ( 6.0f / (float) SIZE_OF_CIRCLE_TEXTURE ) + 1.0f;

    const float radius_of_the_square   = sqrtf( aSeg->GetRadiusSquared() * 2.0f );
    const float radius_triangle_factor = ( radius_of_the_square - radius ) / radius;

    const SFVEC2F factorS = SFVEC2F( -rightDir.y * radius * radius_triangle_factor,
                                      rightDir.x * radius * radius_triangle_factor );

    const SFVEC2F factorE = SFVEC2F( -leftDir.y  * radius * radius_triangle_factor,
                                      leftDir.x  * radius * radius_triangle_factor );

    // Top end segment triangles (semi-circles)
    aDstLayer->m_layer_top_segment_ends->AddTriangle(
                SFVEC3F( rightEnd.x  + texture_factor * factorS.x,
                         rightEnd.y  + texture_factor * factorS.y,
                         aZtop ),
                SFVEC3F( leftStart.x + texture_factor * factorE.x,
                         leftStart.y + texture_factor * factorE.y,
                         aZtop ),
                SFVEC3F( start.x - texture_factorF * leftDir.x * radius * sqrtf( 2.0f ),
                         start.y - texture_factorF * leftDir.y * radius * sqrtf( 2.0f ),
                         aZtop ) );

    aDstLayer->m_layer_top_segment_ends->AddTriangle(
                SFVEC3F( leftEnd.x    + texture_factor * factorE.x,
                         leftEnd.y    + texture_factor * factorE.y, aZtop ),
                SFVEC3F( rightStart.x + texture_factor * factorS.x,
                         rightStart.y + texture_factor * factorS.y, aZtop ),
                SFVEC3F( end.x - texture_factorF * rightDir.x * radius * sqrtf( 2.0f ),
                         end.y - texture_factorF * rightDir.y * radius * sqrtf( 2.0f ),
                         aZtop ) );

    // Bot end segment triangles (semi-circles)
    aDstLayer->m_layer_bot_segment_ends->AddTriangle(
                SFVEC3F( leftStart.x + texture_factor * factorE.x,
                         leftStart.y + texture_factor * factorE.y,
                         aZbot ),
                SFVEC3F( rightEnd.x  + texture_factor * factorS.x,
                         rightEnd.y  + texture_factor * factorS.y,
                         aZbot ),
                SFVEC3F( start.x - texture_factorF * leftDir.x * radius * sqrtf( 2.0f ),
                         start.y - texture_factorF * leftDir.y * radius * sqrtf( 2.0f ),
                         aZbot ) );

    aDstLayer->m_layer_bot_segment_ends->AddTriangle(
                SFVEC3F( rightStart.x + texture_factor * factorS.x,
                         rightStart.y + texture_factor * factorS.y, aZbot ),
                SFVEC3F( leftEnd.x    + texture_factor * factorE.x,
                         leftEnd.y    + texture_factor * factorE.y, aZbot ),
                SFVEC3F( end.x - texture_factorF * rightDir.x * radius * sqrtf( 2.0f ),
                         end.y - texture_factorF * rightDir.y * radius * sqrtf( 2.0f ),
                         aZbot ) );

    // Segment top and bot planes
    aDstLayer->m_layer_top_triangles->AddQuad(
                SFVEC3F( rightEnd.x,   rightEnd.y,   aZtop ),
                SFVEC3F( rightStart.x, rightStart.y, aZtop ),
                SFVEC3F( leftEnd.x,    leftEnd.y,    aZtop ),
                SFVEC3F( leftStart.x,  leftStart.y,  aZtop ) );

    aDstLayer->m_layer_bot_triangles->AddQuad(
                SFVEC3F( rightEnd.x,   rightEnd.y,   aZbot ),
                SFVEC3F( leftStart.x,  leftStart.y,  aZbot ),
                SFVEC3F( leftEnd.x,    leftEnd.y,    aZbot ),
                SFVEC3F( rightStart.x, rightStart.y, aZbot ) );
}


OPENGL_RENDER_LIST* RENDER_3D_OPENGL::generateHoles( const LIST_OBJECT2D& aListHolesObject2d,
                                                     const SHAPE_POLY_SET& aPoly, float aZtop,
                                                     float aZbot, bool aInvertFaces,
                                                     const BVH_CONTAINER_2D* aThroughHoles )
{
    OPENGL_RENDER_LIST* ret = nullptr;

    if( aListHolesObject2d.size() > 0 )
    {
        TRIANGLE_DISPLAY_LIST* layerTriangles =
                new TRIANGLE_DISPLAY_LIST( aListHolesObject2d.size() * 2 );

        // Convert the list of objects(filled circles) to triangle layer structure
        for( const OBJECT_2D* itemOnLayer : aListHolesObject2d )
        {
            const OBJECT_2D* object2d_A = itemOnLayer;

            wxASSERT( ( object2d_A->GetObjectType() == OBJECT_2D_TYPE::FILLED_CIRCLE )
                      || ( object2d_A->GetObjectType() == OBJECT_2D_TYPE::ROUNDSEG ) );

            switch( object2d_A->GetObjectType() )
            {
            case OBJECT_2D_TYPE::FILLED_CIRCLE:
                addObjectTriangles( static_cast<const FILLED_CIRCLE_2D*>( object2d_A ),
                                    layerTriangles, aZtop, aZbot );
                break;

            case OBJECT_2D_TYPE::ROUNDSEG:
                addObjectTriangles( static_cast<const ROUND_SEGMENT_2D*>( object2d_A ),
                                    layerTriangles, aZtop, aZbot );
                break;

            default:
                wxFAIL_MSG( wxT( "RENDER_3D_OPENGL::generateHoles: Object type not implemented" ) );
                break;
            }
        }

        // Note: he can have a aListHolesObject2d with holes but without contours
        // eg: when there are only NPTH on the list and the contours were not added
        if( aPoly.OutlineCount() > 0 )
        {
            layerTriangles->AddToMiddleContourns( aPoly, aZbot, aZtop,
                                                  m_boardAdapter.BiuTo3dUnits(),
                                                  aInvertFaces,  aThroughHoles );
        }

        ret = new OPENGL_RENDER_LIST( *layerTriangles, m_circleTexture, aZbot, aZtop );

        delete layerTriangles;
    }

    return ret;
}


OPENGL_RENDER_LIST* RENDER_3D_OPENGL::generateLayerList( const BVH_CONTAINER_2D* aContainer,
                                                         const SHAPE_POLY_SET* aPolyList,
                                                         PCB_LAYER_ID aLayer,
                                                         const BVH_CONTAINER_2D* aThroughHoles )
{
    if( aContainer == nullptr )
        return nullptr;

    const LIST_OBJECT2D& listObject2d = aContainer->GetList();

    if( listObject2d.size() == 0 )
        return nullptr;

    float layer_z_bot = 0.0f;
    float layer_z_top = 0.0f;

    getLayerZPos( aLayer, layer_z_top, layer_z_bot );

    // Calculate an estimation for the nr of triangles based on the nr of objects
    unsigned int nrTrianglesEstimation = listObject2d.size() * 8;

    TRIANGLE_DISPLAY_LIST* layerTriangles = new TRIANGLE_DISPLAY_LIST( nrTrianglesEstimation );

    // store in a list so it will be latter deleted
    m_triangles.push_back( layerTriangles );

    // Load the 2D (X,Y axis) component of shapes
    for( const OBJECT_2D* itemOnLayer : listObject2d )
    {
        const OBJECT_2D* object2d_A = itemOnLayer;

        switch( object2d_A->GetObjectType() )
        {
        case OBJECT_2D_TYPE::FILLED_CIRCLE:
            addObjectTriangles( static_cast<const FILLED_CIRCLE_2D*>( object2d_A ),
                                layerTriangles, layer_z_top, layer_z_bot );
            break;

        case OBJECT_2D_TYPE::POLYGON4PT:
            addObjectTriangles( static_cast<const POLYGON_4PT_2D*>( object2d_A ),
                                layerTriangles, layer_z_top, layer_z_bot );
            break;

        case OBJECT_2D_TYPE::RING:
            addObjectTriangles( static_cast<const RING_2D*>( object2d_A ),
                                layerTriangles, layer_z_top, layer_z_bot );
            break;

        case OBJECT_2D_TYPE::TRIANGLE:
            addObjectTriangles( static_cast<const TRIANGLE_2D*>( object2d_A ),
                                layerTriangles, layer_z_top, layer_z_bot );
            break;

        case OBJECT_2D_TYPE::ROUNDSEG:
            addObjectTriangles( static_cast<const ROUND_SEGMENT_2D*>( object2d_A ),
                                layerTriangles, layer_z_top, layer_z_bot );
            break;

        default:
            wxFAIL_MSG( wxT( "RENDER_3D_OPENGL: Object type is not implemented" ) );
            break;
        }
    }

    if( aPolyList && aPolyList->OutlineCount() > 0 )
    {
        layerTriangles->AddToMiddleContourns( *aPolyList, layer_z_bot, layer_z_top,
                                              m_boardAdapter.BiuTo3dUnits(), false, aThroughHoles );
    }

    // Create display list
    return new OPENGL_RENDER_LIST( *layerTriangles, m_circleTexture, layer_z_bot, layer_z_top );
}


OPENGL_RENDER_LIST* RENDER_3D_OPENGL::generateEmptyLayerList( PCB_LAYER_ID aLayer )
{
    float layer_z_bot = 0.0f;
    float layer_z_top = 0.0f;

    getLayerZPos( aLayer, layer_z_top, layer_z_bot );

    TRIANGLE_DISPLAY_LIST* layerTriangles = new TRIANGLE_DISPLAY_LIST( 1 );

    // store in a list so it will be latter deleted
    m_triangles.push_back( layerTriangles );

    return new OPENGL_RENDER_LIST( *layerTriangles, m_circleTexture, layer_z_bot, layer_z_top );
}


OPENGL_RENDER_LIST* RENDER_3D_OPENGL::createBoard( const SHAPE_POLY_SET& aBoardPoly,
                                                   const BVH_CONTAINER_2D* aThroughHoles )
{
    OPENGL_RENDER_LIST* dispLists = nullptr;
    CONTAINER_2D boardContainer;

    ConvertPolygonToTriangles( aBoardPoly, boardContainer, m_boardAdapter.BiuTo3dUnits(),
                               (const BOARD_ITEM &)*m_boardAdapter.GetBoard() );

    const LIST_OBJECT2D& listBoardObject2d = boardContainer.GetList();

    if( listBoardObject2d.size() > 0 )
    {
        // We will set a unitary Z so it will in future used with transformations since the
        // board poly will be used not only to draw itself but also the solder mask layers.
        const float layer_z_top = 1.0f;
        const float layer_z_bot = 0.0f;

        TRIANGLE_DISPLAY_LIST* layerTriangles =
                new TRIANGLE_DISPLAY_LIST( listBoardObject2d.size() );

        // Convert the list of objects(triangles) to triangle layer structure
        for( const OBJECT_2D* itemOnLayer : listBoardObject2d )
        {
            const OBJECT_2D* object2d_A = itemOnLayer;

            wxASSERT( object2d_A->GetObjectType() == OBJECT_2D_TYPE::TRIANGLE );

            const TRIANGLE_2D* tri = static_cast<const TRIANGLE_2D*>( object2d_A );

            const SFVEC2F& v1 = tri->GetP1();
            const SFVEC2F& v2 = tri->GetP2();
            const SFVEC2F& v3 = tri->GetP3();

            addTopAndBottomTriangles( layerTriangles, v1, v2, v3, layer_z_top, layer_z_bot );
        }

        if( aBoardPoly.OutlineCount() > 0 )
        {
            layerTriangles->AddToMiddleContourns( aBoardPoly, layer_z_bot, layer_z_top,
                                                  m_boardAdapter.BiuTo3dUnits(), false,
                                                  aThroughHoles );

            dispLists = new OPENGL_RENDER_LIST( *layerTriangles, m_circleTexture,
                                                layer_z_top, layer_z_top );
        }

        delete layerTriangles;
    }

    return dispLists;
}


void RENDER_3D_OPENGL::reload( REPORTER* aStatusReporter, REPORTER* aWarningReporter )
{
    m_reloadRequested = false;

    freeAllLists();

    OBJECT_2D_STATS::Instance().ResetStats();

    int64_t stats_startReloadTime = GetRunningMicroSecs();

    m_boardAdapter.InitSettings( aStatusReporter, aWarningReporter );

    SFVEC3F camera_pos = m_boardAdapter.GetBoardCenter();
    m_camera.SetBoardLookAtPos( camera_pos );

    if( aStatusReporter )
        aStatusReporter->Report( _( "Load OpenGL: board" ) );

    // Create Board
    m_board = createBoard( m_boardAdapter.GetBoardPoly(), &m_boardAdapter.GetTH_IDs() );

    m_antiBoardPolys.RemoveAllContours();
    m_antiBoardPolys.NewOutline();
    m_antiBoardPolys.Append( VECTOR2I( -INT_MAX/2, -INT_MAX/2 ) );
    m_antiBoardPolys.Append( VECTOR2I(  INT_MAX/2, -INT_MAX/2 ) );
    m_antiBoardPolys.Append( VECTOR2I(  INT_MAX/2,  INT_MAX/2 ) );
    m_antiBoardPolys.Append( VECTOR2I( -INT_MAX/2,  INT_MAX/2 ) );
    m_antiBoardPolys.Outline( 0 ).SetClosed( true );

    m_antiBoardPolys.BooleanSubtract( m_boardAdapter.GetBoardPoly() );
    m_antiBoard = createBoard( m_antiBoardPolys );

    SHAPE_POLY_SET board_poly_with_holes = m_boardAdapter.GetBoardPoly().CloneDropTriangulation();
    board_poly_with_holes.BooleanSubtract( m_boardAdapter.GetTH_ODPolys() );
    board_poly_with_holes.BooleanSubtract( m_boardAdapter.GetNPTH_ODPolys() );

    m_boardWithHoles = createBoard( board_poly_with_holes );

    if( m_antiBoard )
        m_antiBoard->SetItIsTransparent( true );

    // Create Through Holes and vias
    if( aStatusReporter )
        aStatusReporter->Report( _( "Load OpenGL: holes and vias" ) );

    SHAPE_POLY_SET outerPolyTHT = m_boardAdapter.GetTH_ODPolys().CloneDropTriangulation();

    outerPolyTHT.BooleanIntersection( m_boardAdapter.GetBoardPoly() );

    m_outerThroughHoles = generateHoles( m_boardAdapter.GetTH_ODs().GetList(), outerPolyTHT,
                                         1.0f, 0.0f, false, &m_boardAdapter.GetTH_IDs() );

    m_outerViaThroughHoles = generateHoles( m_boardAdapter.GetViaTH_ODs().GetList(),
                                            m_boardAdapter.GetViaTH_ODPolys(), 1.0f, 0.0f, false );

    if( m_boardAdapter.m_Cfg->m_Render.clip_silk_on_via_annuli )
    {
        m_outerThroughHoleRings = generateHoles( m_boardAdapter.GetViaAnnuli().GetList(),
                                                 m_boardAdapter.GetViaAnnuliPolys(),
                                                 1.0f, 0.0f, false );
    }

    const MAP_POLY& innerMapHoles = m_boardAdapter.GetHoleIdPolysMap();
    const MAP_POLY& outerMapHoles = m_boardAdapter.GetHoleOdPolysMap();

    wxASSERT( innerMapHoles.size() == outerMapHoles.size() );

    const MAP_CONTAINER_2D_BASE& map_holes = m_boardAdapter.GetLayerHoleMap();

    if( outerMapHoles.size() > 0 )
    {
        float layer_z_bot = 0.0f;
        float layer_z_top = 0.0f;

        for( const auto& [ layer, poly ] : outerMapHoles )
        {
            getLayerZPos( layer, layer_z_top, layer_z_bot );

            m_outerLayerHoles[layer] = generateHoles( map_holes.at( layer )->GetList(), *poly,
                                                      layer_z_top, layer_z_bot, false );
        }

        for( const auto& [ layer, poly ] : innerMapHoles )
        {
            getLayerZPos( layer, layer_z_top, layer_z_bot );

            m_innerLayerHoles[layer] = generateHoles( map_holes.at( layer )->GetList(), *poly,
                                                      layer_z_top, layer_z_bot, false );
        }
    }

    // Generate vertical cylinders of vias and pads (copper)
    generateViasAndPads();

    // Add layers maps
    if( aStatusReporter )
        aStatusReporter->Report( _( "Load OpenGL: layers" ) );

    std::bitset<LAYER_3D_END> visibilityFlags = m_boardAdapter.GetVisibleLayers();
    const MAP_POLY&           map_poly = m_boardAdapter.GetPolyMap();
    wxString                  msg;

    for( const auto& [ layer, container2d ] : m_boardAdapter.GetLayerMap() )
    {
        if( !m_boardAdapter.Is3dLayerEnabled( layer, visibilityFlags ) )
            continue;

        if( aStatusReporter )
        {
            msg = m_boardAdapter.GetBoard()->GetLayerName( layer );
            aStatusReporter->Report( wxString::Format( _( "Load OpenGL layer %s" ), msg ) );
        }

        SHAPE_POLY_SET polyListSubtracted;
        SHAPE_POLY_SET* polyList = nullptr;

        // Load the vertical (Z axis) component of shapes

        if( m_boardAdapter.m_Cfg->m_Render.opengl_copper_thickness )
        {
            if( map_poly.find( layer ) != map_poly.end() )
            {
                polyListSubtracted = *map_poly.at( layer );

                if( LSET::PhysicalLayersMask().test( layer ) )
                {
                    polyListSubtracted.BooleanIntersection( m_boardAdapter.GetBoardPoly() );
                }

                if( layer != B_Mask && layer != F_Mask )
                {
                    polyListSubtracted.BooleanSubtract( m_boardAdapter.GetTH_ODPolys() );
                    polyListSubtracted.BooleanSubtract( m_boardAdapter.GetNPTH_ODPolys() );
                }

                if( m_boardAdapter.m_Cfg->m_Render.subtract_mask_from_silk )
                {
                    if( layer == B_SilkS && map_poly.find( B_Mask ) != map_poly.end() )
                    {
                        polyListSubtracted.BooleanSubtract( *map_poly.at( B_Mask ) );
                    }
                    else if( layer == F_SilkS && map_poly.find( F_Mask ) != map_poly.end() )
                    {
                        polyListSubtracted.BooleanSubtract( *map_poly.at( F_Mask ) );
                    }
                }

                polyList = &polyListSubtracted;
            }
        }

        OPENGL_RENDER_LIST* oglList = generateLayerList( container2d, polyList, layer,
                                                         &m_boardAdapter.GetTH_IDs() );

        if( oglList != nullptr )
            m_layers[layer] = oglList;
    }

    if( m_boardAdapter.m_Cfg->m_Render.DifferentiatePlatedCopper() )
    {
        const SHAPE_POLY_SET* frontPlatedCopperPolys = m_boardAdapter.GetFrontPlatedCopperPolys();
        const SHAPE_POLY_SET* backPlatedCopperPolys = m_boardAdapter.GetBackPlatedCopperPolys();

        if( frontPlatedCopperPolys )
        {
            SHAPE_POLY_SET poly = frontPlatedCopperPolys->CloneDropTriangulation();
            poly.BooleanIntersection( m_boardAdapter.GetBoardPoly() );
            poly.BooleanSubtract( m_boardAdapter.GetTH_ODPolys() );
            poly.BooleanSubtract( m_boardAdapter.GetNPTH_ODPolys() );

            m_platedPadsFront = generateLayerList( m_boardAdapter.GetPlatedPadsFront(), &poly,
                                                   F_Cu );

            // An entry for F_Cu must exist in m_layers or we'll never look at m_platedPadsFront
            if( m_layers.count( F_Cu ) == 0 )
                m_layers[F_Cu] = generateEmptyLayerList( F_Cu );
        }

        if( backPlatedCopperPolys )
        {
            SHAPE_POLY_SET poly = backPlatedCopperPolys->CloneDropTriangulation();
            poly.BooleanIntersection( m_boardAdapter.GetBoardPoly() );
            poly.BooleanSubtract( m_boardAdapter.GetTH_ODPolys() );
            poly.BooleanSubtract( m_boardAdapter.GetNPTH_ODPolys() );

            m_platedPadsBack = generateLayerList( m_boardAdapter.GetPlatedPadsBack(), &poly, B_Cu );

            // An entry for B_Cu must exist in m_layers or we'll never look at m_platedPadsBack
            if( m_layers.count( B_Cu ) == 0 )
                m_layers[B_Cu] = generateEmptyLayerList( B_Cu );
        }
    }

    if( m_boardAdapter.m_Cfg->m_Render.show_off_board_silk )
    {
        if( const BVH_CONTAINER_2D* padsFront = m_boardAdapter.GetOffboardPadsFront() )
            m_offboardPadsFront = generateLayerList( padsFront, nullptr, F_Cu );

        if( const BVH_CONTAINER_2D* padsBack = m_boardAdapter.GetOffboardPadsBack() )
            m_offboardPadsBack = generateLayerList( padsBack, nullptr, B_Cu );
    }

    // Load 3D models
    if( aStatusReporter )
        aStatusReporter->Report( _( "Loading 3D models..." ) );

    load3dModels( aStatusReporter );

    if( aStatusReporter )
    {
        // Calculation time in seconds
        double calculation_time = (double)( GetRunningMicroSecs() - stats_startReloadTime) / 1e6;

        aStatusReporter->Report( wxString::Format( _( "Reload time %.3f s" ), calculation_time ) );
    }
}


void RENDER_3D_OPENGL::addTopAndBottomTriangles( TRIANGLE_DISPLAY_LIST* aDst, const SFVEC2F& v0,
                                                 const SFVEC2F& v1, const SFVEC2F& v2, float top,
                                                 float bot )
{
    aDst->m_layer_bot_triangles->AddTriangle( SFVEC3F( v0.x, v0.y, bot ),
                                              SFVEC3F( v1.x, v1.y, bot ),
                                              SFVEC3F( v2.x, v2.y, bot ) );

    aDst->m_layer_top_triangles->AddTriangle( SFVEC3F( v2.x, v2.y, top ),
                                              SFVEC3F( v1.x, v1.y, top ),
                                              SFVEC3F( v0.x, v0.y, top ) );
}


void RENDER_3D_OPENGL::getLayerZPos( PCB_LAYER_ID aLayer, float& aOutZtop, float& aOutZbot ) const
{
    aOutZbot = m_boardAdapter.GetLayerBottomZPos( aLayer );
    aOutZtop = m_boardAdapter.GetLayerTopZPos( aLayer );

    if( aOutZtop < aOutZbot )
    {
        float tmpFloat = aOutZbot;
        aOutZbot = aOutZtop;
        aOutZtop = tmpFloat;
    }
}


void RENDER_3D_OPENGL::generateCylinder( const SFVEC2F& aCenter, float aInnerRadius,
                                         float aOuterRadius, float aZtop, float aZbot,
                                         unsigned int aNr_sides_per_circle,
                                         TRIANGLE_DISPLAY_LIST* aDstLayer )
{
    std::vector< SFVEC2F > innerContour;
    std::vector< SFVEC2F > outerContour;

    generateRing( aCenter, aInnerRadius, aOuterRadius, aNr_sides_per_circle, innerContour,
                  outerContour, false );

    for( unsigned int i = 0; i < ( innerContour.size() - 1 ); ++i )
    {
        const SFVEC2F& vi0 = innerContour[i + 0];
        const SFVEC2F& vi1 = innerContour[i + 1];
        const SFVEC2F& vo0 = outerContour[i + 0];
        const SFVEC2F& vo1 = outerContour[i + 1];

        aDstLayer->m_layer_top_triangles->AddQuad( SFVEC3F( vi1.x, vi1.y, aZtop ),
                                                   SFVEC3F( vi0.x, vi0.y, aZtop ),
                                                   SFVEC3F( vo0.x, vo0.y, aZtop ),
                                                   SFVEC3F( vo1.x, vo1.y, aZtop ) );

        aDstLayer->m_layer_bot_triangles->AddQuad( SFVEC3F( vi1.x, vi1.y, aZbot ),
                                                   SFVEC3F( vo1.x, vo1.y, aZbot ),
                                                   SFVEC3F( vo0.x, vo0.y, aZbot ),
                                                   SFVEC3F( vi0.x, vi0.y, aZbot ) );
    }

    aDstLayer->AddToMiddleContourns( outerContour, aZbot, aZtop, true );
    aDstLayer->AddToMiddleContourns( innerContour, aZbot, aZtop, false );
}


void RENDER_3D_OPENGL::generateViasAndPads()
{
    if( !m_boardAdapter.GetBoard() )
        return;

    const int   platingThickness    = m_boardAdapter.GetHolePlatingThickness();
    const float platingThickness3d  = platingThickness * m_boardAdapter.BiuTo3dUnits();

    if( m_boardAdapter.GetViaCount() > 0 )
    {
        float        averageDiameter = m_boardAdapter.GetAverageViaHoleDiameter();
        unsigned int averageSegCount = m_boardAdapter.GetCircleSegmentCount( averageDiameter );
        unsigned int trianglesEstimate = averageSegCount * 8 * m_boardAdapter.GetViaCount();

        TRIANGLE_DISPLAY_LIST* layerTriangleVIA = new TRIANGLE_DISPLAY_LIST( trianglesEstimate );

        // Insert plated vertical holes inside the board

        // Insert vias holes (vertical cylinders)
        for( const PCB_TRACK* track : m_boardAdapter.GetBoard()->Tracks() )
        {
            if( track->Type() == PCB_VIA_T )
            {
                const PCB_VIA* via = static_cast<const PCB_VIA*>( track );

                if( via->GetViaType() == VIATYPE::THROUGH )
                    continue;   // handle with pad holes so castellation is taken into account

                const float holediameter = via->GetDrillValue() * m_boardAdapter.BiuTo3dUnits();
                const int nrSegments = m_boardAdapter.GetCircleSegmentCount( via->GetDrillValue() );
                const float hole_inner_radius = holediameter / 2.0f;

                const SFVEC2F via_center( via->GetStart().x * m_boardAdapter.BiuTo3dUnits(),
                                          -via->GetStart().y * m_boardAdapter.BiuTo3dUnits() );

                PCB_LAYER_ID top_layer, bottom_layer;
                via->LayerPair( &top_layer, &bottom_layer );

                float ztop, zbot, dummy;

                getLayerZPos( top_layer, ztop, dummy );
                getLayerZPos( bottom_layer, dummy, zbot );

                wxASSERT( zbot < ztop );

                generateCylinder( via_center, hole_inner_radius,
                                  hole_inner_radius + platingThickness3d,
                                  ztop, zbot, nrSegments, layerTriangleVIA );
            }
        }

        m_vias = new OPENGL_RENDER_LIST( *layerTriangleVIA, 0, 0.0f, 0.0f );

        delete layerTriangleVIA;
    }


    if( m_boardAdapter.GetHoleCount() > 0 || m_boardAdapter.GetViaCount() > 0 )
    {
        SHAPE_POLY_SET tht_outer_holes_poly; // Stores the outer poly of the copper holes
        SHAPE_POLY_SET tht_inner_holes_poly; // Stores the inner poly of the copper holes

        tht_outer_holes_poly.RemoveAllContours();
        tht_inner_holes_poly.RemoveAllContours();

        // Insert through-via holes (vertical cylinders)
        for( const PCB_TRACK* track : m_boardAdapter.GetBoard()->Tracks() )
        {
            if( track->Type() == PCB_VIA_T )
            {
                const PCB_VIA* via = static_cast<const PCB_VIA*>( track );

                if( via->GetViaType() == VIATYPE::THROUGH )
                {
                    TransformCircleToPolygon( tht_outer_holes_poly, via->GetPosition(),
                                              via->GetDrill() / 2 + platingThickness,
                                              ARC_HIGH_DEF, ERROR_INSIDE );

                    TransformCircleToPolygon( tht_inner_holes_poly, via->GetPosition(),
                                              via->GetDrill() / 2, ARC_HIGH_DEF, ERROR_INSIDE );
                }
            }
        }

        // Insert pads holes (vertical cylinders)
        for( const FOOTPRINT* footprint : m_boardAdapter.GetBoard()->Footprints() )
        {
            for( const PAD* pad : footprint->Pads() )
            {
                if( pad->GetAttribute() != PAD_ATTRIB::NPTH )
                {
                    const VECTOR2I drillsize = pad->GetDrillSize();
                    const bool     hasHole = drillsize.x && drillsize.y;

                    if( !hasHole )
                        continue;

                    pad->TransformHoleToPolygon( tht_outer_holes_poly, platingThickness,
                                                 ARC_HIGH_DEF, ERROR_INSIDE );
                    pad->TransformHoleToPolygon( tht_inner_holes_poly, 0, ARC_HIGH_DEF,
                                                 ERROR_INSIDE );
                }
            }
        }

        // Subtract the holes
        tht_outer_holes_poly.BooleanSubtract( tht_inner_holes_poly );

        tht_outer_holes_poly.BooleanSubtract( m_antiBoardPolys );

        CONTAINER_2D holesContainer;

        ConvertPolygonToTriangles( tht_outer_holes_poly, holesContainer,
                                   m_boardAdapter.BiuTo3dUnits(), *m_boardAdapter.GetBoard() );

        const LIST_OBJECT2D& holes2D = holesContainer.GetList();

        if( holes2D.size() > 0 )
        {
            float layer_z_top, layer_z_bot, dummy;

            getLayerZPos( F_Cu, layer_z_top, dummy );
            getLayerZPos( B_Cu, dummy, layer_z_bot );

            TRIANGLE_DISPLAY_LIST* layerTriangles = new TRIANGLE_DISPLAY_LIST( holes2D.size() );

            // Convert the list of objects(triangles) to triangle layer structure
            for( const OBJECT_2D* itemOnLayer : holes2D )
            {
                const OBJECT_2D* object2d_A = itemOnLayer;

                wxASSERT( object2d_A->GetObjectType() == OBJECT_2D_TYPE::TRIANGLE );

                const TRIANGLE_2D* tri = static_cast<const TRIANGLE_2D*>( object2d_A );

                const SFVEC2F& v1 = tri->GetP1();
                const SFVEC2F& v2 = tri->GetP2();
                const SFVEC2F& v3 = tri->GetP3();

                addTopAndBottomTriangles( layerTriangles, v1, v2, v3, layer_z_top, layer_z_bot );
            }

            wxASSERT( tht_outer_holes_poly.OutlineCount() > 0 );

            if( tht_outer_holes_poly.OutlineCount() > 0 )
            {
                layerTriangles->AddToMiddleContourns( tht_outer_holes_poly,
                                                      layer_z_bot, layer_z_top,
                                                      m_boardAdapter.BiuTo3dUnits(), false );

                m_padHoles = new OPENGL_RENDER_LIST( *layerTriangles, m_circleTexture,
                                                     layer_z_top, layer_z_top );
            }

            delete layerTriangles;
        }
    }
}


void RENDER_3D_OPENGL::Load3dModelsIfNeeded()
{
    if( m_3dModelMap.size() > 0 )
        return;

    if( wxFrame* frame = dynamic_cast<wxFrame*>( m_canvas->GetParent() ) )
    {
        STATUSBAR_REPORTER activityReporter( frame->GetStatusBar(),
                                             (int) EDA_3D_VIEWER_STATUSBAR::ACTIVITY );
        load3dModels( &activityReporter );
    }
    else
    {
        load3dModels( nullptr );
    }
}


void RENDER_3D_OPENGL::load3dModels( REPORTER* aStatusReporter )
{
    if( !m_boardAdapter.GetBoard() )
        return;

    // Building the 3D models late crashes on recent versions of macOS
    // Unclear the exact mechanism, but as a workaround, just build them
    // all the time.  See https://gitlab.com/kicad/code/kicad/-/issues/17198
#ifndef __WXMAC__
    if( !m_boardAdapter.m_IsPreviewer
          && !m_boardAdapter.m_Cfg->m_Render.show_footprints_normal
          && !m_boardAdapter.m_Cfg->m_Render.show_footprints_insert
          && !m_boardAdapter.m_Cfg->m_Render.show_footprints_virtual )
    {
        return;
    }
#endif

    // Go for all footprints
    for( const FOOTPRINT* footprint : m_boardAdapter.GetBoard()->Footprints() )
    {
        wxString libraryName = footprint->GetFPID().GetLibNickname();
        wxString footprintBasePath = wxEmptyString;

        if( m_boardAdapter.GetBoard()->GetProject() )
        {
            try
            {
                // FindRow() can throw an exception
                const FP_LIB_TABLE_ROW* fpRow =
                    PROJECT_PCB::PcbFootprintLibs( m_boardAdapter.GetBoard()->GetProject() )
                            ->FindRow( libraryName, false );

                if( fpRow )
                    footprintBasePath = fpRow->GetFullURI( true );
            }
            catch( ... )
            {
                // Do nothing if the libraryName is not found in lib table
            }
        }

        for( const FP_3DMODEL& fp_model : footprint->Models() )
        {
            if( fp_model.m_Show && !fp_model.m_Filename.empty() )
            {
                if( aStatusReporter )
                {
                    // Display the short filename of the 3D fp_model loaded:
                    // (the full name is usually too long to be displayed)
                    wxFileName fn( fp_model.m_Filename );
                    aStatusReporter->Report( wxString::Format( _( "Loading %s..." ),
                                                               fn.GetFullName() ) );
                }

                // Check if the fp_model is not present in our cache map
                // (Not already loaded in memory)
                if( m_3dModelMap.find( fp_model.m_Filename ) == m_3dModelMap.end() )
                {
                    // It is not present, try get it from cache
                    std::vector<const EMBEDDED_FILES*> embeddedFilesStack;
                    embeddedFilesStack.push_back( footprint->GetEmbeddedFiles() );
                    embeddedFilesStack.push_back( m_boardAdapter.GetBoard()->GetEmbeddedFiles() );

                    const S3DMODEL* modelPtr = m_boardAdapter.Get3dCacheManager()->GetModel( fp_model.m_Filename,
                                                                                             footprintBasePath,
                                                                                             embeddedFilesStack );

                    // only add it if the return is not NULL
                    if( modelPtr )
                    {
                        MATERIAL_MODE materialMode = m_boardAdapter.m_Cfg->m_Render.material_mode;
                        MODEL_3D*     model        = new MODEL_3D( *modelPtr, materialMode );

                        m_3dModelMap[ fp_model.m_Filename ] = model;
                    }
                }
            }
        }
    }
}
