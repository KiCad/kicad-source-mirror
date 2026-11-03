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
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include "render_3d_opengl.h"
#include "../../3d_canvas/eda_3d_canvas.h"
#include "../3d_placeholder_utils.h"
#include <3d_viewer_id.h>
#include <board.h>
#include <footprint.h>
#include <pcb_3d_body.h>
#include <pcb_track.h>
#include "convert_basic_shapes_to_polygon.h"
#include <lset.h>
#include <core/profile.h>        // To use GetRunningMicroSecs or another profiling utility
#include <geometry/shape_poly_set.h>
#include <thread_pool.h>
#include <exporters/fab_model/fab_drill.h>


void RENDER_3D_OPENGL::addObjectTriangles( const FILLED_CIRCLE_2D*                aCircle,
                                           std::shared_ptr<TRIANGLE_DISPLAY_LIST> aDstLayer, float aZtop, float aZbot )
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


void RENDER_3D_OPENGL::addObjectTriangles( const POLYGON_4PT_2D*                  aPoly,
                                           std::shared_ptr<TRIANGLE_DISPLAY_LIST> aDstLayer, float aZtop, float aZbot )
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


void RENDER_3D_OPENGL::addObjectTriangles( const RING_2D* aRing, std::shared_ptr<TRIANGLE_DISPLAY_LIST> aDstLayer,
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


void RENDER_3D_OPENGL::addObjectTriangles( const TRIANGLE_2D* aTri, std::shared_ptr<TRIANGLE_DISPLAY_LIST> aDstLayer,
                                           float aZtop, float aZbot )
{
    const SFVEC2F& v1 = aTri->GetP1();
    const SFVEC2F& v2 = aTri->GetP2();
    const SFVEC2F& v3 = aTri->GetP3();

    addTopAndBottomTriangles( aDstLayer, v1, v2, v3, aZtop, aZbot );
}


void RENDER_3D_OPENGL::addObjectTriangles( const ROUND_SEGMENT_2D*                aSeg,
                                           std::shared_ptr<TRIANGLE_DISPLAY_LIST> aDstLayer, float aZtop, float aZbot )
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


std::shared_ptr<OPENGL_RENDER_LIST_DEFERRED> RENDER_3D_OPENGL::generateHoles( const LIST_OBJECT2D&  aListHolesObject2d,
                                                                              const SHAPE_POLY_SET& aPoly, float aZtop,
                                                                              float aZbot, bool aInvertFaces,
                                                                              const BVH_CONTAINER_2D* aThroughHoles )
{
    if( aListHolesObject2d.size() == 0 )
        return nullptr;

    auto triangles = std::make_shared<TRIANGLE_DISPLAY_LIST>( aListHolesObject2d.size() * 2 );

    // Convert the list of objects(filled circles) to triangle layer structure
    for( const OBJECT_2D* object2d : aListHolesObject2d )
    {
        switch( object2d->GetObjectType() )
        {
        case OBJECT_2D_TYPE::FILLED_CIRCLE:
            addObjectTriangles( static_cast<const FILLED_CIRCLE_2D*>( object2d ), triangles, aZtop, aZbot );
            break;

        case OBJECT_2D_TYPE::ROUNDSEG:
            addObjectTriangles( static_cast<const ROUND_SEGMENT_2D*>( object2d ), triangles, aZtop, aZbot );
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
        triangles->AddToMiddleContours( aPoly, aZbot, aZtop, m_boardAdapter.BiuTo3dUnits(), aInvertFaces,
                                        aThroughHoles );
    }

    return std::make_shared<OPENGL_RENDER_LIST_DEFERRED>( triangles, m_circleTexture, aZbot, aZtop );
}


std::shared_ptr<OPENGL_RENDER_LIST_DEFERRED>
RENDER_3D_OPENGL::generateLayerList( const BVH_CONTAINER_2D* aContainer, const SHAPE_POLY_SET* aPolyList,
                                     PCB_LAYER_ID aLayer, const BVH_CONTAINER_2D* aThroughHoles )
{
    if( aContainer == nullptr )
        return nullptr;

    const LIST_OBJECT2D& listObject2d = aContainer->GetList();

    if( listObject2d.size() == 0 )
        return nullptr;

    float zBot = 0.0f;
    float zTop = 0.0f;

    getLayerZPos( aLayer, zTop, zBot );

    // Calculate an estimation for the nr of triangles based on the nr of objects
    unsigned int nrTrianglesEstimation = listObject2d.size() * 8;

    auto layerTriangles = std::make_shared<TRIANGLE_DISPLAY_LIST>( nrTrianglesEstimation );

    // store in a list so it will be latter deleted
    appendRenderTriangleList( layerTriangles );

    // Load the 2D (X,Y axis) component of shapes
    for( const OBJECT_2D* object2d : listObject2d )
    {
        switch( object2d->GetObjectType() )
        {
        case OBJECT_2D_TYPE::FILLED_CIRCLE:
            addObjectTriangles( static_cast<const FILLED_CIRCLE_2D*>( object2d ), layerTriangles, zTop, zBot );
            break;

        case OBJECT_2D_TYPE::POLYGON4PT:
            addObjectTriangles( static_cast<const POLYGON_4PT_2D*>( object2d ), layerTriangles, zTop, zBot );
            break;

        case OBJECT_2D_TYPE::RING:
            addObjectTriangles( static_cast<const RING_2D*>( object2d ), layerTriangles, zTop, zBot );
            break;

        case OBJECT_2D_TYPE::TRIANGLE:
            addObjectTriangles( static_cast<const TRIANGLE_2D*>( object2d ), layerTriangles, zTop, zBot );
            break;

        case OBJECT_2D_TYPE::ROUNDSEG:
            addObjectTriangles( static_cast<const ROUND_SEGMENT_2D*>( object2d ), layerTriangles, zTop, zBot );
            break;

        default:
            wxFAIL_MSG( wxT( "RENDER_3D_OPENGL: Object type is not implemented" ) );
            break;
        }
    }

    if( aPolyList && aPolyList->OutlineCount() > 0 )
    {
        layerTriangles->AddToMiddleContours( *aPolyList, zBot, zTop, m_boardAdapter.BiuTo3dUnits(), false,
                                             aThroughHoles );
    }

    // Create display list
    return std::make_shared<OPENGL_RENDER_LIST_DEFERRED>( layerTriangles, m_circleTexture, zBot, zTop );
}


std::shared_ptr<OPENGL_RENDER_LIST_DEFERRED> RENDER_3D_OPENGL::generateEmptyLayerList( PCB_LAYER_ID aLayer )
{
    float layer_z_bot = 0.0f;
    float layer_z_top = 0.0f;

    getLayerZPos( aLayer, layer_z_top, layer_z_bot );

    auto layerTriangles = std::make_shared<TRIANGLE_DISPLAY_LIST>( 1 );

    // store in a list so it will be latter deleted
    appendRenderTriangleList( layerTriangles );

    return std::make_shared<OPENGL_RENDER_LIST_DEFERRED>( layerTriangles, m_circleTexture, layer_z_bot, layer_z_top );
}


std::shared_ptr<OPENGL_RENDER_LIST_DEFERRED> RENDER_3D_OPENGL::createBoard( const SHAPE_POLY_SET&   aBoardPoly,
                                                                            const BVH_CONTAINER_2D* aThroughHoles,
                                                                            bool                    aTransparent )
{
    std::shared_ptr<OPENGL_RENDER_LIST_DEFERRED> dispLists;
    CONTAINER_2D                                 boardContainer;

    ConvertPolygonToTriangles( aBoardPoly, boardContainer, m_boardAdapter.BiuTo3dUnits(),
                               (const BOARD_ITEM &)*m_boardAdapter.GetBoard() );

    const LIST_OBJECT2D& listBoardObject2d = boardContainer.GetList();

    if( listBoardObject2d.size() > 0 )
    {
        // We will set a unitary Z so it will in future used with transformations since the
        // board poly will be used not only to draw itself but also the solder mask layers.
        const float layer_z_top = 1.0f;
        const float layer_z_bot = 0.0f;

        auto triangles = std::make_shared<TRIANGLE_DISPLAY_LIST>( listBoardObject2d.size() );

        // Convert the list of objects(triangles) to triangle layer structure
        for( const OBJECT_2D* itemOnLayer : listBoardObject2d )
        {
            const OBJECT_2D* object2d_A = itemOnLayer;

            wxASSERT( object2d_A->GetObjectType() == OBJECT_2D_TYPE::TRIANGLE );

            const TRIANGLE_2D* tri = static_cast<const TRIANGLE_2D*>( object2d_A );

            const SFVEC2F& v1 = tri->GetP1();
            const SFVEC2F& v2 = tri->GetP2();
            const SFVEC2F& v3 = tri->GetP3();

            addTopAndBottomTriangles( triangles, v1, v2, v3, layer_z_top, layer_z_bot );
        }

        if( aBoardPoly.OutlineCount() > 0 )
        {
            triangles->AddToMiddleContours( aBoardPoly, layer_z_bot, layer_z_top, m_boardAdapter.BiuTo3dUnits(),
                                            false, aThroughHoles );

            dispLists = std::make_shared<OPENGL_RENDER_LIST_DEFERRED>( triangles, m_circleTexture, 0.0f, 0.0f,
                                                                       aTransparent );
        }
    }

    return dispLists;
}


/*
 * Pretend we had created a hole straight through the board with a radius equal to the largest
 * backdrill/counterbore.  We then fill the parts of that hole that _weren't_ backdrilled/counterbored
 * with a thick cylindrical plug to step the non-backdrilled/counterbored sections back to the smaller
 * via/pad hole size.
 *
 * Now, we don't really create the full-height max-radius hole, but the plugs do still provide top and
 * bottom rings to their thick cylinders, and we need the one that faces the drill/counterbore.
 */
void RENDER_3D_OPENGL::backfillPostMachine()
{
    if( !m_boardAdapter.GetBoard() )
        return;

    const BOARD_STACKUP& stackup = m_boardAdapter.GetBoard()->GetStackupOrDefault();
    LSEQ                 cuStack = m_boardAdapter.GetBoard()->GetEnabledLayers().CuStack();

    const float          unitScale = (float) m_boardAdapter.BiuTo3dUnits();
    const float          platingThickness = (float) m_boardAdapter.GetHolePlatingThickness();

    // We use the same unit z range as the board (0 to 1) and apply scaling when rendering
    const float boardZTop = 1.0f;  // Top of board body
    const float boardZBot = 0.0f;  // Bottom of board body

    // Helper to convert layer Z position to normalized 0-1 range
    auto normalizeZ =
            [&]( float absZ ) -> float
            {
                float boardTop = m_boardAdapter.GetLayerTopZPos( F_Cu );
                float boardBot = m_boardAdapter.GetLayerBottomZPos( B_Cu );
                float boardThick = boardTop - boardBot;

                if( boardThick <= 0 )
                    return 0.5f;

                // Map absolute Z to 0-1 range where 0 = B_Cu and 1 = F_Cu
                return ( absZ - boardBot ) / boardThick;
            };

    // We'll accumulate all plug geometry into a single triangle list
    std::shared_ptr<TRIANGLE_DISPLAY_LIST> plugTriangles = std::make_shared<TRIANGLE_DISPLAY_LIST>( 1024 );

    auto generatePlug =
            [&]( const SFVEC2F center, const float holeOuterR, const float plugR, unsigned int nrSegments,
                const float plugZTop, const float plugZBot, const EDA_ANGLE topAngle, const EDA_ANGLE botAngle )
            {
                // If we have an angle on both ends we need two plugs.
                if( topAngle != ANGLE_0 && botAngle != ANGLE_0 )
                {
                    float middle = ( plugZTop + plugZBot ) / 2.0f;
                    generateInvCone( center, holeOuterR, plugR, normalizeZ( plugZTop ), normalizeZ( middle ),
                                     nrSegments, plugTriangles, topAngle, true );
                    generateInvCone( center, holeOuterR, plugR, normalizeZ( middle ), normalizeZ( plugZBot ),
                                     nrSegments, plugTriangles, botAngle, false );
                }
                else if( topAngle != ANGLE_0 )
                {
                    generateInvCone( center, holeOuterR, plugR, normalizeZ( plugZTop ), normalizeZ( plugZBot ),
                                     nrSegments, plugTriangles, topAngle, true );
                }
                else if( botAngle != ANGLE_0 )
                {
                    generateInvCone( center, holeOuterR, plugR, normalizeZ( plugZTop ), normalizeZ( plugZBot ),
                                     nrSegments, plugTriangles, botAngle, false );
                }
                else
                {
                    generateCylinder( center, holeOuterR, plugR, normalizeZ( plugZTop ), normalizeZ( plugZBot ),
                                      nrSegments, plugTriangles );
                }
            };

    // Process vias for backdrill / post-machining plugs
    for( const PCB_TRACK* track : m_boardAdapter.GetBoard()->Tracks() )
    {
        if( track->Type() != PCB_VIA_T )
            continue;

        const PCB_VIA* via = static_cast<const PCB_VIA*>( track );

        const float        holeInnerR = (float) via->GetDrillValue() * 0.5f * unitScale;
        const float        holeOuterR = holeInnerR + platingThickness * unitScale;
        const SFVEC2F      center( (float) via->GetStart().x * unitScale, - (float) via->GetStart().y * unitScale );
        const unsigned int nrSegments = m_boardAdapter.GetCircleSegmentCount( via->GetDrillValue() );

        PCB_LAYER_ID topLayer, bottomLayer;
        via->LayerPair( &topLayer, &bottomLayer );

        float viaZTop, viaZBot, plugZTop, plugZBot;
        getLayerZPos( topLayer, viaZTop, plugZTop );
        getLayerZPos( bottomLayer, plugZBot, viaZBot );

        EDA_ANGLE topAngle = ANGLE_0;
        EDA_ANGLE botAngle = ANGLE_0;
        float     plugR = holeOuterR;   // max backdrill/post-machine radius

        // Handle backdrills
        for( const PADSTACK::DRILL_PROPS& backdrill : { via->Padstack().SecondaryDrill(),
                                                        via->Padstack().TertiaryDrill() } )
        {
            if( backdrill.start == F_Cu && backdrill.size.x > 0 )
            {
                int          depthIU = stackup.GetLayerDistance( backdrill.start, backdrill.end );
                PCB_LAYER_ID mustNotCut = FabBackdrillMustNotCut( cuStack, backdrill.start, backdrill.end );

                if( mustNotCut != UNDEFINED_LAYER )
                    depthIU += stackup.GetLayerDistance( backdrill.end, mustNotCut ) / 2;

                float depth = (float) depthIU * unitScale;
                float drillRadius = (float) backdrill.size.x * 0.5f * unitScale;

                // Plug is "re-filling" the non-drilled part, so it goes from the bottom of the drill
                // *into* the board.
                plugZTop = std::min( plugZTop, viaZTop - depth );
                plugR = std::max( plugR, drillRadius );
            }
            else if( backdrill.end == B_Cu && backdrill.size.x > 0 )
            {
                int          depthIU = stackup.GetLayerDistance( backdrill.end, backdrill.start );
                PCB_LAYER_ID mustNotCut = FabBackdrillMustNotCut( cuStack, backdrill.start, backdrill.end );

                if( mustNotCut != UNDEFINED_LAYER )
                    depthIU += stackup.GetLayerDistance( mustNotCut, backdrill.end ) / 2;

                float depth = (float) depthIU * unitScale;
                float drillRadius = (float) backdrill.size.x * 0.5f * unitScale;

                // Plug is "re-filling" the non-drilled part, so it goes from the top of the drill
                // *into* the board.
                plugZBot = std::max( plugZBot, viaZBot + depth );
                plugR = std::max( plugR, drillRadius );
            }
        }

        // Handle front post-machining
        if( via->GetFrontPostMachiningMode() == PAD_DRILL_POST_MACHINING_MODE::COUNTERBORE
                || via->GetFrontPostMachiningMode() == PAD_DRILL_POST_MACHINING_MODE::COUNTERSINK )
        {
            const float postMachiningR = (float) via->GetFrontPostMachiningSize() * 0.5f * unitScale;
            const float postMachiningDepth = (float) via->GetFrontPostMachiningDepth() * unitScale;

            if( postMachiningR > holeOuterR && postMachiningDepth > 0 )
            {
                // Plug is "re-filling" the non-machined part, so it goes from bottom of the
                // post-machining *into* the board.
                plugZTop = std::min( plugZTop, viaZTop - postMachiningDepth );
                plugR = std::max( plugR, postMachiningR );

                if( via->GetFrontPostMachiningMode() == PAD_DRILL_POST_MACHINING_MODE::COUNTERSINK )
                    topAngle = EDA_ANGLE( via->GetFrontPostMachiningAngle(), TENTHS_OF_A_DEGREE_T );
            }
        }

        // Handle back post-machining
        if( via->GetBackPostMachiningMode() == PAD_DRILL_POST_MACHINING_MODE::COUNTERBORE
                || via->GetBackPostMachiningMode() == PAD_DRILL_POST_MACHINING_MODE::COUNTERSINK )
        {
            const float postMachiningR = (float) via->GetBackPostMachiningSize() * 0.5f * unitScale;
            const float postMachiningDepth = (float) via->GetBackPostMachiningDepth() * unitScale;

            if( postMachiningR > holeOuterR && postMachiningDepth > 0 )
            {
                // Plug is "re-filling" the non-machined part, so it goes from the top of the
                // post-machining *into* the board.
                plugZBot = std::max( plugZBot, viaZBot + postMachiningDepth );
                plugR = std::max( plugR, postMachiningR );

                if( via->GetBackPostMachiningMode() == PAD_DRILL_POST_MACHINING_MODE::COUNTERSINK )
                    botAngle = EDA_ANGLE( via->GetBackPostMachiningAngle(), TENTHS_OF_A_DEGREE_T );
            }
        }

        generatePlug( center, holeOuterR, plugR, nrSegments, plugZTop, plugZBot, topAngle, botAngle );
    }

    // Process pads for backdrill / post-machining plugs
    for( const FOOTPRINT* footprint : m_boardAdapter.GetBoard()->Footprints() )
    {
        for( const PAD* pad : footprint->Pads() )
        {
            if( !pad->HasHole() )
                continue;

            if( pad->GetDrillShape() != PAD_DRILL_SHAPE::CIRCLE )
                continue;

            const SFVEC2F      padCenter(   (float) pad->GetPosition().x * unitScale,
                                          - (float) pad->GetPosition().y * unitScale );
            const float        holeInnerR = (float) pad->GetDrillSize().x * 0.5f * unitScale;
            const float        holeOuterR = holeInnerR + platingThickness * unitScale;
            const unsigned int nrSegments = m_boardAdapter.GetCircleSegmentCount( pad->GetDrillSize().x );

            float padZTop, padZBot, plugZTop, plugZBot;
            getLayerZPos( F_Cu, padZTop, plugZTop );
            getLayerZPos( B_Cu, plugZBot, padZBot );

            EDA_ANGLE topAngle = ANGLE_0;
            EDA_ANGLE botAngle = ANGLE_0;
            float     plugR = holeOuterR;   // max backdrill/post-machine radius

            // Handle backdrills
            for( const PADSTACK::DRILL_PROPS& backdrill : { pad->Padstack().SecondaryDrill(),
                                                            pad->Padstack().TertiaryDrill() } )
            {
                if( backdrill.start == F_Cu && backdrill.size.x > 0 )
                {
                    int          depthIU = stackup.GetLayerDistance( backdrill.start, backdrill.end );
                    PCB_LAYER_ID mustNotCut = FabBackdrillMustNotCut( cuStack, backdrill.start, backdrill.end );

                    if( mustNotCut != UNDEFINED_LAYER )
                        depthIU += stackup.GetLayerDistance( backdrill.end, mustNotCut ) / 2;

                    float depth = (float) depthIU * unitScale;
                    float drillRadius = (float) backdrill.size.x * 0.5f * unitScale;

                    // Plug is "re-filling" the non-drilled part, so it goes from the bottom of the drill
                    // *into* the board.
                    plugZTop = std::min( plugZTop, padZTop - depth );
                    plugR = std::max( plugR, drillRadius );
                }
                else if( backdrill.end == B_Cu && backdrill.size.x > 0 )
                {
                    int          depthIU = stackup.GetLayerDistance( backdrill.end, backdrill.start );
                    PCB_LAYER_ID mustNotCut = FabBackdrillMustNotCut( cuStack, backdrill.start, backdrill.end );

                    if( mustNotCut != UNDEFINED_LAYER )
                        depthIU += stackup.GetLayerDistance( mustNotCut, backdrill.end ) / 2;

                    float depth = (float) depthIU * unitScale;
                    float drillRadius = (float) backdrill.size.x * 0.5f * unitScale;

                    // Plug is "re-filling" the non-drilled part, so it goes from the top of the drill
                    // *into* the board.
                    plugZBot = std::max( plugZBot, padZBot + depth );
                    plugR = std::max( plugR, drillRadius );
                }
            }

            // Handle front post-machining
            if( pad->GetFrontPostMachiningMode() == PAD_DRILL_POST_MACHINING_MODE::COUNTERBORE
                    || pad->GetFrontPostMachiningMode() == PAD_DRILL_POST_MACHINING_MODE::COUNTERSINK )
            {
                const float postMachiningR = (float) pad->GetFrontPostMachiningSize() * 0.5f * unitScale;
                const float postMachiningDepth = (float) pad->GetFrontPostMachiningDepth() * unitScale;

                if( postMachiningR > holeOuterR && postMachiningDepth > 0 )
                {
                    // Plug is "re-filling" the non-machined part, so it goes from bottom of the post-machining
                    // into the board.
                    plugZTop = std::min( plugZTop, padZTop - postMachiningDepth );
                    plugR = std::max( plugR, postMachiningR );

                    if( pad->GetFrontPostMachiningMode() == PAD_DRILL_POST_MACHINING_MODE::COUNTERSINK )
                        topAngle = EDA_ANGLE( pad->GetFrontPostMachiningAngle(), TENTHS_OF_A_DEGREE_T );
                }
            }

            // Handle back post-machining
            if( pad->GetBackPostMachiningMode() == PAD_DRILL_POST_MACHINING_MODE::COUNTERBORE
                    || pad->GetBackPostMachiningMode() == PAD_DRILL_POST_MACHINING_MODE::COUNTERSINK )
            {
                const float postMachiningR = (float) pad->GetBackPostMachiningSize() * 0.5f * unitScale;
                const float postMachiningDepth = (float) pad->GetBackPostMachiningDepth() * unitScale;

                if( postMachiningR > holeOuterR && postMachiningDepth > 0 )
                {
                    // Plug is "re-filling" the non-machined part, so it goes from the top of the
                    // post-machining *into* the board.
                    plugZBot = std::max( plugZBot, padZBot + postMachiningDepth );
                    plugR = std::max( plugR, postMachiningR );

                    if( pad->GetBackPostMachiningMode() == PAD_DRILL_POST_MACHINING_MODE::COUNTERSINK )
                        botAngle = EDA_ANGLE( pad->GetBackPostMachiningAngle(), TENTHS_OF_A_DEGREE_T );
                }
            }

            generatePlug( padCenter, holeOuterR, plugR, nrSegments, plugZTop, plugZBot, topAngle, botAngle );
        }
    }

    // If we have any plug geometry, create a render list for it
    if( plugTriangles->m_layer_top_triangles->GetVertexSize() > 0
        || plugTriangles->m_layer_bot_triangles->GetVertexSize() > 0
        || plugTriangles->m_layer_middle_contours_quads->GetVertexSize() > 0 )
    {
        // Store the triangles for later cleanup
        appendRenderTriangleList( plugTriangles );

        // Create a render list for the plugs using the same Z range as the board
        // This will be scaled and drawn alongside m_boardWithHoles in renderBoardBody()
        assignRenderPtr( m_postMachinePlugs, std::make_shared<OPENGL_RENDER_LIST_DEFERRED>( plugTriangles,
                                                                                            m_circleTexture,
                                                                                            boardZBot, boardZTop ) );
    }
}


void RENDER_3D_OPENGL::renderExtrudedBodies()
{
    if( !m_boardAdapter.GetBoard() )
        return;

    const float biuTo3d = m_boardAdapter.BiuTo3dUnits();

    for( FOOTPRINT* fp : m_boardAdapter.GetBoard()->Footprints() )
    {
        if( !fp->HasBody() )
            continue;

        const EXTRUDED_3D_BODY* body = fp->GetBody()->GetExtruded();

        if( !body->m_show || !m_boardAdapter.IsFootprintShown( fp ) )
            continue;

        SHAPE_POLY_SET outline;

        if( !GetExtrusionOutline( fp, outline ) )
            continue;

        if( outline.OutlineCount() == 0 )
            continue;

        outline.Simplify();

        VECTOR2I fpPos = fp->GetPosition();
        ApplyExtrusionTransform( outline, body, fpPos );

        bool  isBack = fp->IsFlipped();
        float boardSurfaceZ = m_boardAdapter.GetFootprintZPos( isBack );
        float standoff3d = body->m_standoff * biuTo3d;
        float bodyThickness = ( body->m_height - body->m_standoff ) * biuTo3d * body->m_scale.z;
        float zOffset3d = pcbIUScale.mmToIU( body->m_offset.z ) * biuTo3d;

        float zBot, zTop;

        if( !isBack )
        {
            zBot = boardSurfaceZ + standoff3d + zOffset3d;
            zTop = zBot + bodyThickness;
        }
        else
        {
            zTop = boardSurfaceZ - standoff3d - zOffset3d;
            zBot = zTop - bodyThickness;
        }

        CONTAINER_2D triangleContainer;
        ConvertPolygonToTriangles( outline, triangleContainer, biuTo3d, *fp );

        if( triangleContainer.GetList().empty() )
            continue;

        auto triangles = std::make_shared<TRIANGLE_DISPLAY_LIST>( triangleContainer.GetList().size() );

        for( const OBJECT_2D* obj : triangleContainer.GetList() )
        {
            const TRIANGLE_2D* tri = static_cast<const TRIANGLE_2D*>( obj );
            addTopAndBottomTriangles( triangles, tri->GetP1(), tri->GetP2(), tri->GetP3(), zTop, zBot );
        }

        triangles->AddToMiddleContours( outline, zBot, zTop, biuTo3d, false );

        auto renderList = std::make_shared<OPENGL_RENDER_LIST_DEFERRED>( triangles, m_circleTexture, zTop, zBot );

        appendRenderTriangleList( triangles );
        assignRenderMap( m_extrudedBodyLists, fp, renderList );

        // Create pin extrusions for pad holes
        // The pins move with the body, so the Z offset shifts them whole
        if( standoff3d > 0.0f )
        {
            SHAPE_POLY_SET pinPoly;
            SHAPE_POLY_SET pegPoly;

            if( GetExtrusionPinOutlines( fp, pinPoly, pegPoly ) )
            {
                float oppositeSurfaceZ = m_boardAdapter.GetFootprintZPos( !isBack );
                float protrusion = 1.0f * pcbIUScale.IU_PER_MM * biuTo3d;
                float pinZBot, pinZTop;

                if( !isBack )
                {
                    pinZBot = oppositeSurfaceZ - protrusion + zOffset3d;
                    pinZTop = boardSurfaceZ + standoff3d + zOffset3d;
                }
                else
                {
                    pinZTop = oppositeSurfaceZ + protrusion - zOffset3d;
                    pinZBot = boardSurfaceZ - standoff3d - zOffset3d;
                }

                auto buildPinList =
                        [&]( SHAPE_POLY_SET& aPoly, auto& aDstMap )
                        {
                            if( aPoly.OutlineCount() == 0 )
                                return;

                            ApplyExtrusionTransform( aPoly, body, fpPos );

                            CONTAINER_2D pinTriangleContainer;
                            ConvertPolygonToTriangles( aPoly, pinTriangleContainer, biuTo3d, *fp );

                            if( pinTriangleContainer.GetList().empty() )
                                return;

                            auto pinTriangles = std::make_shared<TRIANGLE_DISPLAY_LIST>( pinTriangleContainer.GetList().size() );

                            for( const OBJECT_2D* obj : pinTriangleContainer.GetList() )
                            {
                                const TRIANGLE_2D* tri = static_cast<const TRIANGLE_2D*>( obj );
                                addTopAndBottomTriangles( pinTriangles, tri->GetP1(), tri->GetP2(), tri->GetP3(),
                                                          pinZTop, pinZBot );
                            }

                            pinTriangles->AddToMiddleContours( aPoly, pinZBot, pinZTop, biuTo3d, false );

                            auto pinRenderList = std::make_shared<OPENGL_RENDER_LIST_DEFERRED>( pinTriangles,
                                                                                                m_circleTexture,
                                                                                                pinZTop, pinZBot );

                            appendRenderTriangleList( pinTriangles );
                            assignRenderMap( aDstMap, fp, pinRenderList );
                        };

                buildPinList( pinPoly, m_extrudedPadLists );
                buildPinList( pegPoly, m_extrudedPegLists );
            }
        }
    }
}


void RENDER_3D_OPENGL::reload()
{
    m_reloadRequested = false;

    // Ensure previous reload worker is finished before freeing/rebuilding render data.
    StopBgWorker();

    // Drop hover BVH before layers are destroyed; LAYER_ITEM points into those OBJECT_2Ds.
    if( m_canvas )
        m_canvas->InvalidateRaytracingHitTesting();

    freeAllLists();

    if( !m_boardAdapter.m_IsPreviewer )
        createPlaceholderModel();

    OBJECT_2D_STATS::Instance().ResetStats();

    m_boardAdapter.InitSettings( m_activityReporter, m_warningReporter );

    SFVEC3F camera_pos = m_boardAdapter.GetBoardCenter();
    m_camera.SetBoardLookAtPos( camera_pos );

    // Create basic Board without holes
    assignRenderPtr( m_board, createBoard( m_boardAdapter.GetBoardPoly(), nullptr ) );

    startBgWorker();
}


void RENDER_3D_OPENGL::sendRefreshView()
{
    if( m_canvas )
        wxQueueEvent( m_canvas, new wxCommandEvent( wxEVT_REFRESH_CUSTOM_COMMAND, ID_CUSTOM_EVENT_1 ) );
}


void RENDER_3D_OPENGL::bgWorker( std::stop_token aStop )
{
    int64_t stats_startReloadTime = GetRunningMicroSecs();

    if( aStop.stop_requested() )
        return;

    // Ensure hover BVH is gone before destroyLayers() inside CreateLayers.
    if( m_canvas )
        m_canvas->InvalidateRaytracingHitTesting();

    m_boardAdapter.CreateLayers( m_activityReporter, aStop );

    if( aStop.stop_requested() )
        return;

    sendRefreshView();

    if( m_activityReporter )
        m_activityReporter->Report( _( "Load OpenGL: board" ) );

    // Create Board with TH
    assignRenderPtr( m_board, createBoard( m_boardAdapter.GetBoardPoly(), &m_boardAdapter.GetTH_IDs() ) );

    if( aStop.stop_requested() )
        return;

    sendRefreshView();

    m_antiBoardPolys.RemoveAllContours();
    m_antiBoardPolys.NewOutline();
    m_antiBoardPolys.Append( VECTOR2I( -INT_MAX/2, -INT_MAX/2 ) );
    m_antiBoardPolys.Append( VECTOR2I(  INT_MAX/2, -INT_MAX/2 ) );
    m_antiBoardPolys.Append( VECTOR2I(  INT_MAX/2,  INT_MAX/2 ) );
    m_antiBoardPolys.Append( VECTOR2I( -INT_MAX/2,  INT_MAX/2 ) );
    m_antiBoardPolys.Outline( 0 ).SetClosed( true );

    m_antiBoardPolys.BooleanSubtract( m_boardAdapter.GetBoardPoly() );

    assignRenderPtr( m_antiBoard, createBoard( m_antiBoardPolys, nullptr, true ) );

    if( aStop.stop_requested() )
        return;

    SHAPE_POLY_SET board_poly_with_holes = m_boardAdapter.GetBoardPoly().CloneDropTriangulation();
    board_poly_with_holes.BooleanSubtract( m_boardAdapter.GetTH_ODPolys() );
    board_poly_with_holes.BooleanSubtract( m_boardAdapter.GetNPTH_ODPolys() );

    // Also subtract counterbore, countersink, and backdrill polygons from the board
    if( m_boardAdapter.GetFrontCounterborePolys().OutlineCount() > 0 )
        board_poly_with_holes.BooleanSubtract( m_boardAdapter.GetFrontCounterborePolys() );

    if( m_boardAdapter.GetBackCounterborePolys().OutlineCount() > 0 )
        board_poly_with_holes.BooleanSubtract( m_boardAdapter.GetBackCounterborePolys() );

    if( m_boardAdapter.GetFrontCountersinkPolys().OutlineCount() > 0 )
        board_poly_with_holes.BooleanSubtract( m_boardAdapter.GetFrontCountersinkPolys() );

    if( m_boardAdapter.GetBackCountersinkPolys().OutlineCount() > 0 )
        board_poly_with_holes.BooleanSubtract( m_boardAdapter.GetBackCountersinkPolys() );

    if( m_boardAdapter.GetFrontBackdrillPolys().OutlineCount() > 0 )
        board_poly_with_holes.BooleanSubtract( m_boardAdapter.GetFrontBackdrillPolys() );

    if( m_boardAdapter.GetBackBackdrillPolys().OutlineCount() > 0 )
        board_poly_with_holes.BooleanSubtract( m_boardAdapter.GetBackBackdrillPolys() );

    if( aStop.stop_requested() )
        return;

    assignRenderPtr( m_boardWithHoles, createBoard( board_poly_with_holes, &m_boardAdapter.GetTH_IDs() ) );

    // Create plugs for backdrilled and post-machined areas
    backfillPostMachine();

    if( aStop.stop_requested() )
        return;

    sendRefreshView();

    // Create Through Holes and vias
    if( m_activityReporter )
        m_activityReporter->Report( _( "Load OpenGL: holes and vias" ) );

    SHAPE_POLY_SET outerPolyTHT = m_boardAdapter.GetTH_ODPolys().CloneDropTriangulation();

    // Include NPTH polygons so their barrel walls are also generated
    if( m_boardAdapter.GetNPTH_ODPolys().OutlineCount() > 0 )
        outerPolyTHT.BooleanAdd( m_boardAdapter.GetNPTH_ODPolys() );

    outerPolyTHT.BooleanIntersection( m_boardAdapter.GetBoardPoly() );

    LIST_OBJECT2D frontODs = m_boardAdapter.GetTH_ODs().GetList();

    for( OBJECT_2D* obj : m_boardAdapter.GetFrontCounterboreCutouts().GetList() )
        frontODs.emplace_back( obj );

    for( OBJECT_2D* obj : m_boardAdapter.GetFrontCountersinkCutouts().GetList() )
        frontODs.emplace_back( obj );

    assignRenderPtr( m_frontOuterThroughHoles, generateHoles( frontODs, outerPolyTHT, 1.0f, 0.0f, false,
                                                              &m_boardAdapter.GetTH_IDs() ) );

    LIST_OBJECT2D backODs = m_boardAdapter.GetTH_ODs().GetList();

    for( OBJECT_2D* obj : m_boardAdapter.GetBackCounterboreCutouts().GetList() )
        backODs.emplace_back( obj );

    for( OBJECT_2D* obj : m_boardAdapter.GetBackCountersinkCutouts().GetList() )
        backODs.emplace_back( obj );

    assignRenderPtr( m_backOuterThroughHoles, generateHoles( backODs, outerPolyTHT, 1.0f, 0.0f, false,
                                                             &m_boardAdapter.GetTH_IDs() ) );

    if( m_boardAdapter.m_Cfg->m_Render.clip_silk_on_via_annuli )
    {
        assignRenderPtr( m_outerThroughHoleRings, generateHoles( m_boardAdapter.GetViaAnnuli().GetList(),
                                                                 m_boardAdapter.GetViaAnnuliPolys(), 1.0f, 0.0f,
                                                                 false ) );
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

            assignRenderMap( m_outerLayerHoles, layer, generateHoles( map_holes.at( layer )->GetList(), *poly,
                             layer_z_top, layer_z_bot, false ) );
        }

        for( const auto& [ layer, poly ] : innerMapHoles )
        {
            getLayerZPos( layer, layer_z_top, layer_z_bot );

            assignRenderMap( m_innerLayerHoles, layer, generateHoles( map_holes.at( layer )->GetList(), *poly,
                             layer_z_top, layer_z_bot, false ) );
        }
    }

    // Generate vertical cylinders of vias and pads (copper)
    generateViasAndPads();

    if( aStop.stop_requested() )
        return;

    sendRefreshView();

    // Add layers maps
    if( m_activityReporter )
        m_activityReporter->Report( _( "Load OpenGL: layers" ) );

    std::bitset<LAYER_3D_END> visibilityFlags = m_boardAdapter.GetVisibleLayers();
    const MAP_POLY&           map_poly = m_boardAdapter.GetPolyMap();
    wxString                  msg;

    for( const auto& [ layer, container2d ] : m_boardAdapter.GetLayerMap() )
    {
        if( !m_boardAdapter.Is3dLayerEnabled( layer, visibilityFlags ) )
            continue;

        if( m_activityReporter )
        {
            msg = m_boardAdapter.GetBoard()->GetLayerName( layer );
            m_activityReporter->Report( wxString::Format( _( "Load OpenGL layer %s" ), msg ) );
        }

        SHAPE_POLY_SET polyListSubtracted;
        SHAPE_POLY_SET* polyList = nullptr;

        // Load the vertical (Z axis) component of shapes

        if( m_boardAdapter.m_Cfg->m_Render.opengl_copper_thickness )
        {
            if( map_poly.contains( layer ) )
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

                    // Subtract counterbore/countersink cutouts from copper layers
                    if( layer == F_Cu )
                    {
                        polyListSubtracted.BooleanSubtract( m_boardAdapter.GetFrontCounterborePolys() );
                        polyListSubtracted.BooleanSubtract( m_boardAdapter.GetFrontCountersinkPolys() );
                    }
                    else if( layer == B_Cu )
                    {
                        polyListSubtracted.BooleanSubtract( m_boardAdapter.GetBackCounterborePolys() );
                        polyListSubtracted.BooleanSubtract( m_boardAdapter.GetBackCountersinkPolys() );
                    }
                }

                if( m_boardAdapter.m_Cfg->m_Render.subtract_mask_from_silk )
                {
                    if( layer == B_SilkS && map_poly.contains( B_Mask ) )
                    {
                        polyListSubtracted.BooleanSubtract( *map_poly.at( B_Mask ) );
                    }
                    else if( layer == F_SilkS && map_poly.contains( F_Mask ) )
                    {
                        polyListSubtracted.BooleanSubtract( *map_poly.at( F_Mask ) );
                    }
                }

                polyList = &polyListSubtracted;
            }
        }

        if( aStop.stop_requested() )
            return;

        std::shared_ptr<OPENGL_RENDER_LIST_DEFERRED> oglList =
                generateLayerList( container2d, polyList, layer, &m_boardAdapter.GetTH_IDs() );

        if( oglList != nullptr )
            assignRenderMap( m_layers, layer, oglList );

        if( aStop.stop_requested() )
            return;

        sendRefreshView();
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
            poly.BooleanSubtract( m_boardAdapter.GetFrontCounterborePolys() );
            poly.BooleanSubtract( m_boardAdapter.GetFrontCountersinkPolys() );
            poly.BooleanSubtract( m_boardAdapter.GetFrontBackdrillPolys() );

            assignRenderPtr( m_platedPadsFront, generateLayerList( m_boardAdapter.GetPlatedPadsFront(), &poly, F_Cu ) );

            // An entry for F_Cu must exist in m_layers or we'll never look at m_platedPadsFront
            if( m_layers.count( F_Cu ) == 0 )
                assignRenderMap( m_layers, F_Cu, generateEmptyLayerList( F_Cu ) );
        }

        if( backPlatedCopperPolys )
        {
            SHAPE_POLY_SET poly = backPlatedCopperPolys->CloneDropTriangulation();
            poly.BooleanIntersection( m_boardAdapter.GetBoardPoly() );
            poly.BooleanSubtract( m_boardAdapter.GetTH_ODPolys() );
            poly.BooleanSubtract( m_boardAdapter.GetNPTH_ODPolys() );
            poly.BooleanSubtract( m_boardAdapter.GetBackCounterborePolys() );
            poly.BooleanSubtract( m_boardAdapter.GetBackCountersinkPolys() );
            poly.BooleanSubtract( m_boardAdapter.GetBackBackdrillPolys() );

            assignRenderPtr( m_platedPadsBack, generateLayerList( m_boardAdapter.GetPlatedPadsBack(), &poly, B_Cu ) );

            // An entry for B_Cu must exist in m_layers or we'll never look at m_platedPadsBack
            if( m_layers.count( B_Cu ) == 0 )
                assignRenderMap( m_layers, B_Cu, generateEmptyLayerList( B_Cu ) );
        }
    }

    if( m_boardAdapter.m_Cfg->m_Render.show_off_board_silk )
    {
        if( const BVH_CONTAINER_2D* padsFront = m_boardAdapter.GetOffboardPadsFront() )
            assignRenderPtr( m_offboardPadsFront, generateLayerList( padsFront, nullptr, F_Cu ) );

        if( const BVH_CONTAINER_2D* padsBack = m_boardAdapter.GetOffboardPadsBack() )
            assignRenderPtr( m_offboardPadsBack, generateLayerList( padsBack, nullptr, B_Cu ) );
    }

    if( aStop.stop_requested() )
        return;

    sendRefreshView();

    // Load 3D models
    if( m_activityReporter )
        m_activityReporter->Report( _( "Loading 3D models..." ) );

    load3dModels( aStop );

    renderExtrudedBodies();

    if( aStop.stop_requested() )
        return;

    sendRefreshView();

    // OpenGL draws the board on the GPU, but hover/picking still uses the auxiliary
    // raytracing renderer (IntersectBoardItem). That path keeps its own BVH
    // (m_accelerator), which is not updated by the OpenGL scene build above.
    // Rebuild it here on the same worker thread once board layers are ready.
    if( !aStop.stop_requested() && m_canvas )
    {
        m_hitTestDirty = true;
        runHitTestRebuild( aStop );
    }

    if( aStop.stop_requested() )
        return;

    if( m_activityReporter )
    {
        // Calculation time in seconds
        double calculation_time = (double)( GetRunningMicroSecs() - stats_startReloadTime) / 1e6;

        m_activityReporter->Report( wxString::Format( _( "Load completed in %.3f s" ), calculation_time ) );
    }
}


void RENDER_3D_OPENGL::startBgWorker()
{
    wxLogTrace( m_logTrace, wxT( "RENDER_3D_OPENGL::startBgWorker" ) );

    // Claim the flag before the thread runs so a rebuild cannot displace the load.
    m_bgWorkerBusy = true;

    m_bgWorkerThread = std::jthread(
            [this]( std::stop_token aStopToken )
            {
                // Avoid lag in main (UI) thread
                BS::this_thread::set_os_thread_priority( BS::os_thread_priority::below_normal );

                bgWorker( aStopToken );

                m_bgWorkerBusy = false;
            } );
}


void RENDER_3D_OPENGL::RebuildHitTestAsync()
{
    m_hitTestDirty = true;

    // A running worker picks the request up before it releases the flag.
    if( m_bgWorkerBusy.exchange( true ) )
        return;

    m_bgWorkerThread = std::jthread(
            [this]( std::stop_token aStop )
            {
                BS::this_thread::set_os_thread_priority( BS::os_thread_priority::below_normal );

                runHitTestRebuild( aStop );

                m_bgWorkerBusy = false;
            } );
}


void RENDER_3D_OPENGL::runHitTestRebuild( std::stop_token aStop )
{
    for( ;; )
    {
        while( m_hitTestDirty.exchange( false ) )
        {
            if( aStop.stop_requested() || !m_canvas )
                return;

            m_canvas->ReloadRaytracingForHitTesting( aStop );
        }

        m_bgWorkerBusy = false;

        // Pick up a request that raced the release above.
        if( !m_hitTestDirty || m_bgWorkerBusy.exchange( true ) )
            return;
    }
}


void RENDER_3D_OPENGL::StopBgWorker()
{
    wxLogTrace( m_logTrace, wxT( "RENDER_3D_OPENGL::StopBgWorker" ) );

    if( m_bgWorkerThread.joinable() )
    {
        if( m_activityReporter )
            m_activityReporter->Report( _( "Stopping background load..." ) );

        m_bgWorkerThread.request_stop();
        m_bgWorkerThread.join();
    }

    m_bgWorkerBusy = false;
}


void RENDER_3D_OPENGL::JoinBgWorker()
{
    wxLogTrace( m_logTrace, wxT( "RENDER_3D_OPENGL::JoinBgWorker" ) );

    if( m_bgWorkerThread.joinable() )
        m_bgWorkerThread.join();
}


void RENDER_3D_OPENGL::addTopAndBottomTriangles( std::shared_ptr<TRIANGLE_DISPLAY_LIST> aDst, const SFVEC2F& v0,
                                                 const SFVEC2F& v1, const SFVEC2F& v2, float top, float bot )
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
        std::swap( aOutZtop, aOutZbot );
}


void RENDER_3D_OPENGL::generateCylinder( const SFVEC2F& aCenter, float aInnerRadius, float aOuterRadius, float aZtop,
                                         float aZbot, unsigned int aNr_sides_per_circle,
                                         std::shared_ptr<TRIANGLE_DISPLAY_LIST> aDstLayer )
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

    aDstLayer->AddToMiddleContours( outerContour, aZbot, aZtop, true );
    aDstLayer->AddToMiddleContours( innerContour, aZbot, aZtop, false );
}


void RENDER_3D_OPENGL::generateInvCone( const SFVEC2F& aCenter, float aInnerRadius, float aOuterRadius, float aZtop,
                                        float aZbot, unsigned int aNr_sides_per_circle,
                                        std::shared_ptr<TRIANGLE_DISPLAY_LIST> aDstLayer, EDA_ANGLE aAngle,
                                        bool aIsFront )
{
    // For a countersink cone:
    // - The outer contour goes from aZbot to aZtop (full height)
    // - The inner contour goes from aZbot to aZbot + innerHeight
    // - The top surface is conical, sloping from inner top to outer top
    // - aAngle is the half-angle of the cone (in decidegrees from the vertical)

    // Calculate the inner contour height based on the cone angle
    // tan(angle) = (outerRadius - innerRadius) / innerHeight
    // innerHeight = (outerRadius - innerRadius) / tan(angle)
    float radialDiff = aOuterRadius - aInnerRadius;
    float angleRad = aAngle.AsRadians();

    // Clamp angle to avoid division by zero or negative heights
    if( angleRad < 0.01f )
        angleRad = 0.01f;

    float innerHeight = radialDiff / tanf( angleRad );
    float totalHeight = aZtop - aZbot;

    // Clamp inner height to not exceed total height
    if( innerHeight > totalHeight )
        innerHeight = totalHeight;

    float zInnerTop = aIsFront ? aZbot + innerHeight : aZtop;
    float zInnerBot = aIsFront ? aZbot : aZtop - innerHeight;

    std::vector< SFVEC2F > innerContour;
    std::vector< SFVEC2F > outerContour;

    generateRing( aCenter, aInnerRadius, aOuterRadius, aNr_sides_per_circle, innerContour, outerContour, false );

    for( unsigned int i = 0; i < ( innerContour.size() - 1 ); ++i )
    {
        const SFVEC2F& vi0 = innerContour[i + 0];
        const SFVEC2F& vi1 = innerContour[i + 1];
        const SFVEC2F& vo0 = outerContour[i + 0];
        const SFVEC2F& vo1 = outerContour[i + 1];

        if( aIsFront )
        {
            // Conical top surface: from inner contour at zInnerTop to outer contour at aZtop
            aDstLayer->m_layer_top_triangles->AddQuad( SFVEC3F( vi1.x, vi1.y, zInnerTop ),
                                                       SFVEC3F( vi0.x, vi0.y, zInnerTop ),
                                                       SFVEC3F( vo0.x, vo0.y, aZtop ),
                                                       SFVEC3F( vo1.x, vo1.y, aZtop ) );

            // Flat bottom surface
            aDstLayer->m_layer_bot_triangles->AddQuad( SFVEC3F( vi1.x, vi1.y, aZbot ),
                                                       SFVEC3F( vo1.x, vo1.y, aZbot ),
                                                       SFVEC3F( vo0.x, vo0.y, aZbot ),
                                                       SFVEC3F( vi0.x, vi0.y, aZbot ) );
        }
        else
        {
            // Flat top surface
            aDstLayer->m_layer_bot_triangles->AddQuad( SFVEC3F( vi1.x, vi1.y, aZtop ),
                                                       SFVEC3F( vo1.x, vo1.y, aZtop ),
                                                       SFVEC3F( vo0.x, vo0.y, aZtop ),
                                                       SFVEC3F( vi0.x, vi0.y, aZtop ) );

            // Conical bottom surface: from inner contour at zInnerBot to outer contour at aZbot
            aDstLayer->m_layer_top_triangles->AddQuad( SFVEC3F( vi1.x, vi1.y, zInnerBot ),
                                                       SFVEC3F( vi0.x, vi0.y, zInnerBot ),
                                                       SFVEC3F( vo0.x, vo0.y, aZbot ),
                                                       SFVEC3F( vo1.x, vo1.y, aZbot ) );
        }
    }

    // Outer contour wall goes full height
    aDstLayer->AddToMiddleContours( outerContour, aZbot, aZtop, true );
    // Inner contour wall only goes [zInnerBot,zInnerTop]
    aDstLayer->AddToMiddleContours( innerContour, zInnerBot, zInnerTop, false );
}


void RENDER_3D_OPENGL::generateDisk( const SFVEC2F& aCenter, float aRadius, float aZ, unsigned int aNr_sides_per_circle,
                                     std::shared_ptr<TRIANGLE_DISPLAY_LIST> aDstLayer, bool aTop )
{
    const float delta = 2.0f * glm::pi<float>() / (float) aNr_sides_per_circle;

    for( unsigned int i = 0; i < aNr_sides_per_circle; ++i )
    {
        float a0 = delta * i;
        float a1 = delta * ( i + 1 );
        const SFVEC3F p0( aCenter.x + cosf( a0 ) * aRadius,
                          aCenter.y + sinf( a0 ) * aRadius, aZ );
        const SFVEC3F p1( aCenter.x + cosf( a1 ) * aRadius,
                          aCenter.y + sinf( a1 ) * aRadius, aZ );
        const SFVEC3F c( aCenter.x, aCenter.y, aZ );

        if( aTop )
            aDstLayer->m_layer_top_triangles->AddTriangle( p1, p0, c );
        else
            aDstLayer->m_layer_bot_triangles->AddTriangle( p0, p1, c );
    }
}


void RENDER_3D_OPENGL::generateDimple( const SFVEC2F& aCenter, float aRadius, float aZ, float aDepth,
                                       unsigned int                           aNr_sides_per_circle,
                                       std::shared_ptr<TRIANGLE_DISPLAY_LIST> aDstLayer, bool aTop )
{
    const float delta = 2.0f * glm::pi<float>() / (float) aNr_sides_per_circle;
    const SFVEC3F c( aCenter.x, aCenter.y, aTop ? aZ - aDepth : aZ + aDepth );

    for( unsigned int i = 0; i < aNr_sides_per_circle; ++i )
    {
        float a0 = delta * i;
        float a1 = delta * ( i + 1 );
        const SFVEC3F p0( aCenter.x + cosf( a0 ) * aRadius,
                          aCenter.y + sinf( a0 ) * aRadius, aZ );
        const SFVEC3F p1( aCenter.x + cosf( a1 ) * aRadius,
                          aCenter.y + sinf( a1 ) * aRadius, aZ );

        if( aTop )
            aDstLayer->m_layer_top_triangles->AddTriangle( p0, p1, c );
        else
            aDstLayer->m_layer_bot_triangles->AddTriangle( p1, p0, c );
    }
}


bool RENDER_3D_OPENGL::appendPostMachiningGeometry( std::shared_ptr<TRIANGLE_DISPLAY_LIST> aDstLayer,
                                                    const SFVEC2F& aHoleCenter,
                                                    PAD_DRILL_POST_MACHINING_MODE aMode,
                                                    int aSizeIU,
                                                    int aDepthIU,
                                                    const EDA_ANGLE& aAngle,
                                                    float aHoleInnerRadius,
                                                    float aZSurface,
                                                    bool aIsFront,
                                                    float aPlatingThickness3d,
                                                    float aUnitScale,
                                                    float* aZEnd )
{
    if( !m_boardAdapter.m_Cfg->m_Render.show_plated_barrels )
        return false;

    if( !aDstLayer || aPlatingThickness3d <= 0.0f || aHoleInnerRadius <= 0.0f )
        return false;

    if( aMode != PAD_DRILL_POST_MACHINING_MODE::COUNTERBORE
            && aMode != PAD_DRILL_POST_MACHINING_MODE::COUNTERSINK )
    {
        return false;
    }

    if( aSizeIU <= 0 || aDepthIU <= 0 )
        return false;

    const float radius = aSizeIU * 0.5f * aUnitScale;
    const float depth = aDepthIU * aUnitScale;

    if( radius <= aHoleInnerRadius || depth <= 0.0f )
        return false;

    float zEnd = aIsFront ? ( aZSurface - depth ) : ( aZSurface + depth );

    if( aZEnd )
        *aZEnd = zEnd;

    const float zTop = std::max( aZSurface, zEnd );
    const float zBot = std::min( aZSurface, zEnd );

    const int diameterBIU = std::max( aSizeIU, std::max( 1, (int) ( ( aHoleInnerRadius * 2.0f ) / aUnitScale ) ) );
    const unsigned int nrSegments = std::max( 12u, m_boardAdapter.GetCircleSegmentCount( diameterBIU ) );

    if( aMode == PAD_DRILL_POST_MACHINING_MODE::COUNTERSINK )
    {
        generateInvCone( aHoleCenter, radius, radius + aPlatingThickness3d, zTop, zBot, nrSegments, aDstLayer,
                         aAngle, aIsFront );
    }
    else
    {
        generateCylinder( aHoleCenter, radius, radius + aPlatingThickness3d, zTop, zBot, nrSegments, aDstLayer );
    }

    return true;
}


bool isSimpleThroughHole( const BOARD_ITEM* aItem )
{
    if( aItem->Type() == PCB_VIA_T )
    {
        const PCB_VIA* via = static_cast<const PCB_VIA*>( aItem );
        bool isBackdrilled = via->GetSecondaryDrillSize().has_value()
                                && via->GetSecondaryDrillStartLayer() != UNDEFINED_LAYER;
        bool isTertiarydrilled = via->GetTertiaryDrillSize().has_value()
                                    && via->GetTertiaryDrillStartLayer() != UNDEFINED_LAYER;
        bool hasFrontPostMachining = via->GetFrontPostMachiningMode() == PAD_DRILL_POST_MACHINING_MODE::COUNTERBORE
                                  || via->GetFrontPostMachiningMode() == PAD_DRILL_POST_MACHINING_MODE::COUNTERSINK;
        bool hasBackPostMachining = via->GetBackPostMachiningMode() == PAD_DRILL_POST_MACHINING_MODE::COUNTERBORE
                                 || via->GetBackPostMachiningMode() == PAD_DRILL_POST_MACHINING_MODE::COUNTERSINK;

        return via->GetViaType() == VIATYPE::THROUGH
                && !isBackdrilled
                && !isTertiarydrilled
                && !hasFrontPostMachining
                && !hasBackPostMachining;
    }
    else if( aItem->Type() == PCB_PAD_T )
    {
        const PAD* pad = static_cast<const PAD*>( aItem );

        bool isBackdrilled = pad->GetSecondaryDrillSize().x > 0
                                && pad->GetSecondaryDrillSize().y > 0
                                && pad->GetSecondaryDrillStartLayer() != UNDEFINED_LAYER;
        bool isTertiarydrilled = pad->GetTertiaryDrillSize().x > 0
                                    && pad->GetTertiaryDrillSize().y > 0
                                    && pad->GetTertiaryDrillStartLayer() != UNDEFINED_LAYER;
        bool hasFrontPostMachining = pad->GetFrontPostMachiningMode() == PAD_DRILL_POST_MACHINING_MODE::COUNTERBORE
                                  || pad->GetFrontPostMachiningMode() == PAD_DRILL_POST_MACHINING_MODE::COUNTERSINK;
        bool hasBackPostMachining = pad->GetBackPostMachiningMode() == PAD_DRILL_POST_MACHINING_MODE::COUNTERBORE
                                 || pad->GetBackPostMachiningMode() == PAD_DRILL_POST_MACHINING_MODE::COUNTERSINK;

        return pad->HasHole()
                && pad->GetAttribute() != PAD_ATTRIB::NPTH
                && !isBackdrilled
                && !isTertiarydrilled
                && !hasFrontPostMachining
                && !hasBackPostMachining;
    }

    return false;
}


void RENDER_3D_OPENGL::generateComplexHoles( float aPlatingThickness3d, float aUnitScale )
{
    if( !m_boardAdapter.GetBoard() )
        return;

    if( m_boardAdapter.GetHoleCount() <= 0 && m_boardAdapter.GetViaCount() <= 0 )
        return;

    if( !m_boardAdapter.m_Cfg->m_Render.show_plated_barrels )
        return;

    float        averageDiameter = m_boardAdapter.GetAverageViaHoleDiameter();
    unsigned int averageSegCount = m_boardAdapter.GetCircleSegmentCount( averageDiameter );
    unsigned int trianglesEstimate = averageSegCount * 8 * m_boardAdapter.GetViaCount();
    auto         triangles = std::make_shared<TRIANGLE_DISPLAY_LIST>( trianglesEstimate );

    const BOARD_STACKUP& stackup = m_boardAdapter.GetBoard()->GetStackupOrDefault();
    LSEQ                 cuStack = m_boardAdapter.GetBoard()->GetEnabledLayers().CuStack();

    for( const PCB_TRACK* track : m_boardAdapter.GetBoard()->Tracks() )
    {
        if( track->Type() != PCB_VIA_T )
            continue;

        const PCB_VIA* via = static_cast<const PCB_VIA*>( track );

        if( isSimpleThroughHole( via ) )
        {
            // Simple through-hole vias have their barrels generated by generateSimplePlatedHoleShells()
            continue;
        }

        SFVEC2F      via_center( (float) via->GetStart().x * aUnitScale, - (float) via->GetStart().y * aUnitScale );
        unsigned int nrSegments = m_boardAdapter.GetCircleSegmentCount( via->GetDrillValue() );
        float        holeInnerR = (float) via->GetDrillValue() * 0.5f * aUnitScale;

        float ztop = m_boardAdapter.GetLayerBottomZPos( F_Cu ) + m_boardAdapter.GetFrontCopperThickness() * 0.99f;
        float zbot = m_boardAdapter.GetLayerBottomZPos( B_Cu ) - m_boardAdapter.GetBackCopperThickness() * 0.99f;

        float ztop_plated = ztop;
        float zbot_plated = zbot;

        for( const PADSTACK::DRILL_PROPS& backdrill : { via->Padstack().SecondaryDrill(),
                                                        via->Padstack().TertiaryDrill() } )
        {
            if( backdrill.start == F_Cu && backdrill.size.x > 0 )
            {
                int          depthIU = stackup.GetLayerDistance( backdrill.start, backdrill.end );
                PCB_LAYER_ID mustNotCut = FabBackdrillMustNotCut( cuStack, backdrill.start, backdrill.end );

                if( mustNotCut != UNDEFINED_LAYER )
                    depthIU += stackup.GetLayerDistance( backdrill.end, mustNotCut ) / 2;

                float zEnd = 0.0f;

                if( appendPostMachiningGeometry( triangles, via_center, PAD_DRILL_POST_MACHINING_MODE::COUNTERSINK,
                                                 backdrill.size.x, depthIU, EDA_ANGLE( 118.0, DEGREES_T ),
                                                 holeInnerR, ztop, true, aPlatingThickness3d, aUnitScale,
                                                 &zEnd ) )
                {
                    ztop_plated = std::min( ztop_plated, zEnd );
                }
            }
            else if( backdrill.end == B_Cu && backdrill.size.x > 0 )
            {
                int          depthIU = stackup.GetLayerDistance( backdrill.end, backdrill.start );
                PCB_LAYER_ID mustNotCut = FabBackdrillMustNotCut( cuStack, backdrill.start, backdrill.end );

                if( mustNotCut != UNDEFINED_LAYER )
                    depthIU += stackup.GetLayerDistance( mustNotCut, backdrill.end ) / 2;

                float zEnd = 0.0f;

                if( appendPostMachiningGeometry( triangles, via_center, PAD_DRILL_POST_MACHINING_MODE::COUNTERSINK,
                                                 backdrill.size.x, depthIU, EDA_ANGLE( 118.0, DEGREES_T ),
                                                 holeInnerR, zbot, false, aPlatingThickness3d, aUnitScale,
                                                 &zEnd ) )
                {
                    zbot_plated = std::max( zbot_plated, zEnd );
                }
            }
        }

        if( via->GetFrontPostMachiningMode() == PAD_DRILL_POST_MACHINING_MODE::COUNTERBORE
                || via->GetFrontPostMachiningMode() == PAD_DRILL_POST_MACHINING_MODE::COUNTERSINK )
        {
            float zEnd = 0.0f;

            if( appendPostMachiningGeometry( triangles, via_center, via->GetFrontPostMachiningMode(),
                                             via->GetFrontPostMachiningSize(), via->GetFrontPostMachiningDepth(),
                                             EDA_ANGLE( via->GetFrontPostMachiningAngle(), TENTHS_OF_A_DEGREE_T ),
                                             holeInnerR, ztop, true, aPlatingThickness3d, aUnitScale, &zEnd ) )
            {
                ztop_plated = std::min( ztop_plated, zEnd );
            }
        }

        if( via->GetBackPostMachiningMode() == PAD_DRILL_POST_MACHINING_MODE::COUNTERBORE
                || via->GetBackPostMachiningMode() == PAD_DRILL_POST_MACHINING_MODE::COUNTERSINK )
        {
            float zEnd = 0.0f;

            if( appendPostMachiningGeometry( triangles, via_center,via->GetBackPostMachiningMode(),
                                             via->GetBackPostMachiningSize(), via->GetBackPostMachiningDepth(),
                                             EDA_ANGLE( via->GetBackPostMachiningAngle(), TENTHS_OF_A_DEGREE_T ),
                                             holeInnerR, zbot, false, aPlatingThickness3d, aUnitScale, &zEnd ) )
            {
                zbot_plated = std::max( zbot_plated, zEnd );
            }
        }

        generateCylinder( via_center, holeInnerR, holeInnerR + aPlatingThickness3d, ztop_plated, zbot_plated,
                          nrSegments, triangles );
    }

    float ztop = m_boardAdapter.GetLayerBottomZPos( F_Cu ) + m_boardAdapter.GetFrontCopperThickness() * 0.99f;
    float zbot = m_boardAdapter.GetLayerBottomZPos( B_Cu ) - m_boardAdapter.GetBackCopperThickness() * 0.99f;

    float ztop_plated = ztop;
    float zbot_plated = zbot;

    for( const FOOTPRINT* footprint : m_boardAdapter.GetBoard()->Footprints() )
    {
        for( const PAD* pad : footprint->Pads() )
        {
            if( pad->GetAttribute() == PAD_ATTRIB::NPTH )
                continue;

            if( !pad->HasHole() || isSimpleThroughHole( pad ) )
            {
                // Simple through-hole pads have their barrels generated by generateSimplePlatedHoleShells()
                continue;
            }
            else if( pad->GetDrillShape() != PAD_DRILL_SHAPE::CIRCLE )
            {
                // Slot-holed pads have their slot plating generated by ???
                continue;
            }

            SFVEC2F      padCenter(   (float) pad->GetPosition().x * aUnitScale,
                                    - (float) pad->GetPosition().y * aUnitScale );
            unsigned int nrSegments = m_boardAdapter.GetCircleSegmentCount( pad->GetDrillSize().x );
            float        holeInnerR = (float) pad->GetDrillSize().x * 0.5f * aUnitScale;

            if( pad->GetFrontPostMachiningMode() == PAD_DRILL_POST_MACHINING_MODE::COUNTERBORE
                    || pad->GetFrontPostMachiningMode() == PAD_DRILL_POST_MACHINING_MODE::COUNTERSINK )
            {
                float zEnd = 0.0f;

                if( appendPostMachiningGeometry( triangles, padCenter, pad->GetFrontPostMachiningMode(),
                                                 pad->GetFrontPostMachiningSize(), pad->GetFrontPostMachiningDepth(),
                                                 EDA_ANGLE( pad->GetFrontPostMachiningAngle(), TENTHS_OF_A_DEGREE_T ),
                                                 holeInnerR, ztop, true, aPlatingThickness3d, aUnitScale, &zEnd ) )
                {
                    ztop_plated = std::min( ztop_plated, zEnd );
                }
            }

            if( pad->GetBackPostMachiningMode() == PAD_DRILL_POST_MACHINING_MODE::COUNTERBORE
                    || pad->GetBackPostMachiningMode() == PAD_DRILL_POST_MACHINING_MODE::COUNTERSINK )
            {
                float zEnd = 0.0f;

                if( appendPostMachiningGeometry( triangles, padCenter, pad->GetBackPostMachiningMode(),
                                                 pad->GetBackPostMachiningSize(), pad->GetBackPostMachiningDepth(),
                                                 EDA_ANGLE( pad->GetBackPostMachiningAngle(), TENTHS_OF_A_DEGREE_T ),
                                                 holeInnerR, zbot, false, aPlatingThickness3d, aUnitScale, &zEnd ) )
                {
                    zbot_plated = std::max( zbot_plated, zEnd );
                }
            }

            generateCylinder( padCenter, holeInnerR, holeInnerR + aPlatingThickness3d, ztop_plated, zbot_plated,
                              nrSegments, triangles );
        }
    }

    assignRenderPtr( m_complexPlatedHoles, std::make_shared<OPENGL_RENDER_LIST_DEFERRED>( triangles, 0, 0.0f, 0.0f ) );
}


/*
 * For simple through holes only.  Anything that needs the hole plating shortened (backdrilling, post-machining,
 * micro-vias, etc.) must be handled by generateComplexHoles().
 */
void RENDER_3D_OPENGL::generateSimplePlatedHoleShells( int aPlatingThickness, float aUnitScale )
{
    if( !m_boardAdapter.GetBoard() )
        return;

    if( m_boardAdapter.GetHoleCount() <= 0 && m_boardAdapter.GetViaCount() <= 0 )
        return;

    SHAPE_POLY_SET tht_outer_holes_poly; // Stores the outer poly of the copper holes
    SHAPE_POLY_SET tht_inner_holes_poly; // Stores the inner poly of the copper holes
    // Same as ***_holes_poly but for vias that are capped
    SHAPE_POLY_SET capped_outer_vias_poly;
    SHAPE_POLY_SET capped_inner_vias_poly;

    tht_outer_holes_poly.RemoveAllContours();
    tht_inner_holes_poly.RemoveAllContours();
    capped_outer_vias_poly.RemoveAllContours();
    capped_inner_vias_poly.RemoveAllContours();

    for( const PCB_TRACK* track : m_boardAdapter.GetBoard()->Tracks() )
    {
        if( track->Type() != PCB_VIA_T )
            continue;

        const PCB_VIA* via = static_cast<const PCB_VIA*>( track );

        if( !isSimpleThroughHole( via ) )
            continue;

        const bool capped = via->ResolveCappingMode() == CAPPING_MODE::CAPPED;

        TransformCircleToPolygon( capped ? capped_outer_vias_poly : tht_outer_holes_poly, via->GetPosition(),
                                  via->GetDrill() / 2 + aPlatingThickness, via->GetMaxError(), ERROR_INSIDE );

        TransformCircleToPolygon( capped ? capped_inner_vias_poly : tht_inner_holes_poly, via->GetPosition(),
                                  via->GetDrill() / 2, via->GetMaxError(), ERROR_INSIDE );
    }

    for( const FOOTPRINT* footprint : m_boardAdapter.GetBoard()->Footprints() )
    {
        for( const PAD* pad : footprint->Pads() )
        {
            if( !isSimpleThroughHole( pad ) )
                continue;

            pad->TransformHoleToPolygon( tht_outer_holes_poly, aPlatingThickness, pad->GetMaxError(), ERROR_INSIDE );
            pad->TransformHoleToPolygon( tht_inner_holes_poly, 0, pad->GetMaxError(), ERROR_INSIDE );
        }
    }

    tht_outer_holes_poly.BooleanSubtract( tht_inner_holes_poly );
    tht_outer_holes_poly.BooleanSubtract( m_antiBoardPolys );

    tht_outer_holes_poly.BooleanAdd( capped_outer_vias_poly );
    tht_inner_holes_poly.BooleanAdd( capped_inner_vias_poly );

    CONTAINER_2D holesContainer;

    ConvertPolygonToTriangles( tht_outer_holes_poly, holesContainer, aUnitScale, *m_boardAdapter.GetBoard() );

    const LIST_OBJECT2D& holes2D = holesContainer.GetList();

    if( holes2D.size() > 0 && m_boardAdapter.m_Cfg->m_Render.show_plated_barrels )
    {
        float layer_z_top, layer_z_bot, dummy;

        getLayerZPos( F_Cu, layer_z_top, dummy );
        getLayerZPos( B_Cu, dummy, layer_z_bot );

        auto triangles = std::make_shared<TRIANGLE_DISPLAY_LIST>( holes2D.size() );

        for( const OBJECT_2D* itemOnLayer : holes2D )
        {
            const OBJECT_2D* object2d_A = itemOnLayer;

            wxASSERT( object2d_A->GetObjectType() == OBJECT_2D_TYPE::TRIANGLE );

            const TRIANGLE_2D* tri = static_cast<const TRIANGLE_2D*>( object2d_A );

            const SFVEC2F& v1 = tri->GetP1();
            const SFVEC2F& v2 = tri->GetP2();
            const SFVEC2F& v3 = tri->GetP3();

            addTopAndBottomTriangles( triangles, v1, v2, v3, layer_z_top, layer_z_bot );
        }

        wxASSERT( tht_outer_holes_poly.OutlineCount() > 0 );

        if( tht_outer_holes_poly.OutlineCount() > 0 )
        {
            triangles->AddToMiddleContours( tht_outer_holes_poly, layer_z_bot, layer_z_top, aUnitScale, false );

            assignRenderPtr( m_padHoles, std::make_shared<OPENGL_RENDER_LIST_DEFERRED>( triangles, m_circleTexture,
                                                                                        0.0f, 0.0f ) );
        }
    }
}


void RENDER_3D_OPENGL::generateViaCovers( float aPlatingThickness3d, float aUnitScale )
{
    if( !m_boardAdapter.GetBoard() || m_boardAdapter.GetViaCount() <= 0 )
        return;

    auto frontCover = std::make_shared<TRIANGLE_DISPLAY_LIST>( m_boardAdapter.GetViaCount() );
    auto backCover = std::make_shared<TRIANGLE_DISPLAY_LIST>( m_boardAdapter.GetViaCount() );

    for( const PCB_TRACK* track : m_boardAdapter.GetBoard()->Tracks() )
    {
        if( track->Type() != PCB_VIA_T )
            continue;

        const PCB_VIA* via = static_cast<const PCB_VIA*>( track );

        const float holediameter = via->GetDrillValue() * aUnitScale;
        const float hole_radius = holediameter / 2.0f + 2.0f * aPlatingThickness3d;
        const SFVEC2F center( via->GetStart().x * aUnitScale,
                              -via->GetStart().y * aUnitScale );
        unsigned int seg = m_boardAdapter.GetCircleSegmentCount( via->GetDrillValue() );

        PCB_LAYER_ID top_layer, bottom_layer;
        via->LayerPair( &top_layer, &bottom_layer );
        float ztop, zbot, dummy;
        getLayerZPos( top_layer, ztop, dummy );
        getLayerZPos( bottom_layer, dummy, zbot );

        bool frontCovering = via->ResolveFrontCoveringMode() == COVERING_MODE::COVERED
                             || via->IsTented( F_Mask );
        bool backCovering = via->ResolveBackCoveringMode() == COVERING_MODE::COVERED
                            || via->IsTented( B_Mask );
        bool frontPlugged = via->ResolveFrontPluggingMode() == PLUGGING_MODE::PLUGGED;
        bool backPlugged  = via->ResolveBackPluggingMode() == PLUGGING_MODE::PLUGGED;
        bool filled = via->ResolveFillingMode() == FILLING_MODE::FILLED
                      || via->ResolveCappingMode() == CAPPING_MODE::CAPPED;

        bool hasFrontPostMachining = via->GetFrontPostMachiningMode() == PAD_DRILL_POST_MACHINING_MODE::COUNTERBORE
                                  || via->GetFrontPostMachiningMode() == PAD_DRILL_POST_MACHINING_MODE::COUNTERSINK;
        bool hasBackPostMachining = via->GetBackPostMachiningMode() == PAD_DRILL_POST_MACHINING_MODE::COUNTERBORE
                                 || via->GetBackPostMachiningMode() == PAD_DRILL_POST_MACHINING_MODE::COUNTERSINK;

        bool hasFrontBackdrill = via->GetSecondaryDrillStartLayer() == F_Cu
                                    || via->GetTertiaryDrillStartLayer() == F_Cu;
        bool hasBackBackdrill = via->GetSecondaryDrillStartLayer() == B_Cu
                                    || via->GetTertiaryDrillStartLayer() == B_Cu;

        const float depth = hole_radius * 0.3f;

        if( frontCovering && !hasFrontPostMachining && !hasFrontBackdrill )
        {
            if( filled || !frontPlugged )
                generateDisk( center, hole_radius, ztop, seg, frontCover, true );
            else
                generateDimple( center, hole_radius, ztop, depth, seg, frontCover, true );
        }

        if( backCovering && !hasBackPostMachining && !hasBackBackdrill )
        {
            if( filled || !backPlugged )
                generateDisk( center, hole_radius, zbot, seg, backCover, false );
            else
                generateDimple( center, hole_radius, zbot, depth, seg, backCover, false );
        }
    }

    if( frontCover->m_layer_top_triangles->GetVertexSize() > 0 )
        assignRenderPtr( m_viaFrontCover, std::make_shared<OPENGL_RENDER_LIST_DEFERRED>( frontCover, 0, 0.0f, 0.0f ) );

    if( backCover->m_layer_bot_triangles->GetVertexSize() > 0 )
        assignRenderPtr( m_viaBackCover, std::make_shared<OPENGL_RENDER_LIST_DEFERRED>( backCover, 0, 0.0f, 0.0f ) );
}


void RENDER_3D_OPENGL::generateViasAndPads()
{
    if( !m_boardAdapter.GetBoard() )
        return;

    const int   platingThickness    = m_boardAdapter.GetHolePlatingThickness();
    const float unitScale           = m_boardAdapter.BiuTo3dUnits();
    const float platingThickness3d  = platingThickness * unitScale;

    generateComplexHoles( platingThickness3d, unitScale );
    generateSimplePlatedHoleShells( platingThickness, unitScale );
    generateViaCovers( platingThickness3d, unitScale );
}


void RENDER_3D_OPENGL::Load3dModelsIfNeeded()
{
    if( m_3dModelMap.size() > 0 )
        return;

    load3dModels();
}


void RENDER_3D_OPENGL::load3dModels( std::stop_token aStop )
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

    S3D_CACHE* cacheMgr = m_boardAdapter.Get3dCacheManager();

    // Go for all footprints
    for( const FOOTPRINT* footprint : m_boardAdapter.GetBoard()->Footprints() )
    {
        const wxString footprintBasePath = m_boardAdapter.GetFootprintLibraryPath( footprint );

        for( const FP_3DMODEL& fp_model : footprint->Models() )
        {
            if( fp_model.m_Show && !fp_model.m_Filename.empty() )
            {
                if( m_activityReporter )
                {
                    // Display the short filename of the 3D fp_model loaded:
                    // (the full name is usually too long to be displayed)
                    wxFileName fn( fp_model.m_Filename );
                    m_activityReporter->Report( wxString::Format( _( "Loading %s..." ), fn.GetFullName() ) );
                }

                // Check if the fp_model is not present in our cache map
                // (Not already loaded in memory)
                if( !m_3dModelMap.contains( fp_model.m_Filename ) )
                {
                    // It is not present, try get it from cache
                    std::vector<const EMBEDDED_FILES*> embeddedFilesStack;
                    embeddedFilesStack.push_back( footprint->GetEmbeddedFiles() );
                    embeddedFilesStack.push_back( m_boardAdapter.GetBoard()->GetEmbeddedFiles() );

                    const S3DMODEL* modelPtr = cacheMgr->GetModel( fp_model.m_Filename, footprintBasePath,
                                                                   std::move( embeddedFilesStack ) );

                    // only add it if the return is not NULL
                    if( modelPtr )
                    {
                        MATERIAL_MODE materialMode = m_boardAdapter.m_Cfg->m_Render.material_mode;
                        auto          model = std::make_shared<MODEL_3D_DEFERRED>( *modelPtr, materialMode );

                        assignRenderMap( m_3dModelMap, fp_model.m_Filename, model );
                    }
                }
            }

            if( aStop.stop_requested() )
                return;
        }

        sendRefreshView();
    }
}
