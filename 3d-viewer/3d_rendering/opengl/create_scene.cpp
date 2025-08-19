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
#include <layer_range.h>
#include <pcb_track.h>
#include "../../3d_math.h"
#include "convert_basic_shapes_to_polygon.h"
#include <lset.h>
#include <trigo.h>
#include <project.h>
#include <core/profile.h>        // To use GetRunningMicroSecs or another profiling utility
#include <footprint_library_adapter.h>
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


void RENDER_3D_OPENGL::addObjectTriangles( const TRIANGLE_2D* aTri, TRIANGLE_DISPLAY_LIST* aDstLayer,
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
                                                     const SHAPE_POLY_SET& aPoly, float aZtop, float aZbot,
                                                     bool aInvertFaces, const BVH_CONTAINER_2D* aThroughHoles )
{
    if( aListHolesObject2d.size() == 0 )
        return nullptr;

    OPENGL_RENDER_LIST*    ret = nullptr;
    TRIANGLE_DISPLAY_LIST* layerTriangles = new TRIANGLE_DISPLAY_LIST( aListHolesObject2d.size() * 2 );

    // Convert the list of objects(filled circles) to triangle layer structure
    for( const OBJECT_2D* object2d : aListHolesObject2d )
    {
        switch( object2d->GetObjectType() )
        {
        case OBJECT_2D_TYPE::FILLED_CIRCLE:
            addObjectTriangles( static_cast<const FILLED_CIRCLE_2D*>( object2d ), layerTriangles, aZtop, aZbot );
            break;

        case OBJECT_2D_TYPE::ROUNDSEG:
            addObjectTriangles( static_cast<const ROUND_SEGMENT_2D*>( object2d ), layerTriangles, aZtop, aZbot );
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
        layerTriangles->AddToMiddleContours( aPoly, aZbot, aZtop, m_boardAdapter.BiuTo3dUnits(), aInvertFaces,
                                             aThroughHoles );
    }

    ret = new OPENGL_RENDER_LIST( *layerTriangles, m_circleTexture, aZbot, aZtop );

    delete layerTriangles;

    return ret;
}


OPENGL_RENDER_LIST* RENDER_3D_OPENGL::generateLayerList( const BVH_CONTAINER_2D* aContainer,
                                                         const SHAPE_POLY_SET* aPolyList, PCB_LAYER_ID aLayer,
                                                         const BVH_CONTAINER_2D* aThroughHoles )
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

    TRIANGLE_DISPLAY_LIST* layerTriangles = new TRIANGLE_DISPLAY_LIST( nrTrianglesEstimation );

    // store in a list so it will be latter deleted
    m_triangles.push_back( layerTriangles );

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
    return new OPENGL_RENDER_LIST( *layerTriangles, m_circleTexture, zBot, zTop );
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
            layerTriangles->AddToMiddleContours( aBoardPoly, layer_z_bot, layer_z_top,
                                                 m_boardAdapter.BiuTo3dUnits(), false,
                                                 aThroughHoles );

            dispLists = new OPENGL_RENDER_LIST( *layerTriangles, m_circleTexture,
                                                layer_z_top, layer_z_top );
        }

        delete layerTriangles;
    }

    return dispLists;
}


void RENDER_3D_OPENGL::backfillPostMachine()
{
    if( !m_boardAdapter.GetBoard() )
        return;

    const int copperLayerCount = m_boardAdapter.GetBoard()->GetCopperLayerCount();
    const float unitScale = m_boardAdapter.BiuTo3dUnits();
    const int platingThickness = m_boardAdapter.GetHolePlatingThickness();
    const float boardBodyThickness = m_boardAdapter.GetBoardBodyThickness();

    // We use the same unit z range as the board (0 to 1) and apply scaling when rendering
    const float boardZTop = 1.0f;  // Top of board body
    const float boardZBot = 0.0f;  // Bottom of board body

    // Helper to convert layer Z position to normalized 0-1 range
    auto normalizeZ = [&]( float absZ ) -> float
    {
        float boardTop = m_boardAdapter.GetLayerBottomZPos( F_Cu );
        float boardBot = m_boardAdapter.GetLayerBottomZPos( B_Cu );
        float boardThick = boardTop - boardBot;

        if( boardThick <= 0 )
            return 0.5f;

        // Map absolute Z to 0-1 range where 0 = B_Cu and 1 = F_Cu
        return ( absZ - boardBot ) / boardThick;
    };

    // We'll accumulate all plug geometry into a single triangle list
    TRIANGLE_DISPLAY_LIST* plugTriangles = new TRIANGLE_DISPLAY_LIST( 1024 );

    // Process vias for backdrill and post-machining plugs
    for( const PCB_TRACK* track : m_boardAdapter.GetBoard()->Tracks() )
    {
        if( track->Type() != PCB_VIA_T )
            continue;

        const PCB_VIA* via = static_cast<const PCB_VIA*>( track );

        const float holeDiameter = via->GetDrillValue() * unitScale;
        const float holeInnerRadius = holeDiameter / 2.0f;
        const float holeOuterRadius = holeInnerRadius + platingThickness * unitScale;
        const SFVEC2F center( via->GetStart().x * unitScale, -via->GetStart().y * unitScale );
        const int nrSegments = m_boardAdapter.GetCircleSegmentCount( via->GetDrillValue() );

        PCB_LAYER_ID topLayer, bottomLayer;
        via->LayerPair( &topLayer, &bottomLayer );

        float viaZTop, viaZBot, dummy;
        getLayerZPos( topLayer, viaZTop, dummy );
        getLayerZPos( bottomLayer, dummy, viaZBot );

        // Handle backdrill plugs
        const float secondaryDrillRadius = via->GetSecondaryDrillSize().value_or( 0 ) * 0.5f * unitScale;
        const float tertiaryDrillRadius = via->GetTertiaryDrillSize().value_or( 0 ) * 0.5f * unitScale;

        if( secondaryDrillRadius > holeOuterRadius || tertiaryDrillRadius > holeOuterRadius )
        {
            PCB_LAYER_ID plug_start_layer = F_Cu;
            PCB_LAYER_ID plug_end_layer = B_Cu;

            // Case 1: secondary drill exists, so we need to adjust the plug_end_layer
            if( secondaryDrillRadius > holeOuterRadius )
            {
                plug_end_layer = via->GetSecondaryDrillEndLayer();
            }
            // Case 2: tertiary drill exists, so we need to adjust the plug_start_layer
            if( tertiaryDrillRadius > holeOuterRadius )
            {
                plug_start_layer = via->GetTertiaryDrillStartLayer();
            }

            // Calculate where the backdrill ends and plug should start
            float plugZTop, plugZBot, temp;
            getLayerZPos( plug_end_layer, temp, plugZBot );
            getLayerZPos( plug_start_layer, plugZTop, temp );

            // Create a ring from holeOuterRadius to backdrillRadius
            generateCylinder( center, holeOuterRadius, std::max( secondaryDrillRadius, tertiaryDrillRadius ),
                                plugZTop, plugZBot, nrSegments, plugTriangles );
        }

        // Handle front post-machining plugs
        const auto frontMode = via->GetFrontPostMachining();

        if( frontMode.has_value()
            && frontMode.value() != PAD_DRILL_POST_MACHINING_MODE::NOT_POST_MACHINED
            && frontMode.value() != PAD_DRILL_POST_MACHINING_MODE::UNKNOWN )
        {
            const float frontRadius = via->GetFrontPostMachiningSize() * 0.5f * unitScale;
            const float frontDepth = via->GetFrontPostMachiningDepth() * unitScale;

            if( frontRadius > holeOuterRadius && frontDepth > 0 )
            {
                // Plug goes from bottom of post-machining to bottom of via
                float pmBottomZ = normalizeZ( viaZTop - frontDepth );
                float plugZBot = normalizeZ( viaZBot );

                if( pmBottomZ > plugZBot )
                {
                    if( frontMode.value() == PAD_DRILL_POST_MACHINING_MODE::COUNTERSINK )
                    {
                        EDA_ANGLE angle( via->GetFrontPostMachiningAngle(), TENTHS_OF_A_DEGREE_T );
                        generateInvCone( center, holeOuterRadius, frontRadius,
                                         pmBottomZ, plugZBot, nrSegments, plugTriangles, angle );
                    }
                    else
                    {
                        generateCylinder( center, holeOuterRadius, frontRadius,
                                          pmBottomZ, plugZBot, nrSegments, plugTriangles );
                    }
                }
            }
        }

        // Handle back post-machining plugs
        const auto backMode = via->GetBackPostMachining();

        if( backMode.has_value()
            && backMode.value() != PAD_DRILL_POST_MACHINING_MODE::NOT_POST_MACHINED
            && backMode.value() != PAD_DRILL_POST_MACHINING_MODE::UNKNOWN )
        {
            const float backRadius = via->GetBackPostMachiningSize() * 0.5f * unitScale;
            const float backDepth = via->GetBackPostMachiningDepth() * unitScale;

            if( backRadius > holeOuterRadius && backDepth > 0 )
            {
                // Plug goes from top of via to top of post-machining
                float plugZTop = normalizeZ( viaZTop );
                float pmTopZ = normalizeZ( viaZBot + backDepth );

                if( plugZTop > pmTopZ )
                {
                    if( backMode.value() == PAD_DRILL_POST_MACHINING_MODE::COUNTERSINK )
                    {
                        EDA_ANGLE angle( via->GetBackPostMachiningAngle(), TENTHS_OF_A_DEGREE_T );
                        generateInvCone( center, holeOuterRadius, backRadius,
                                         plugZTop, pmTopZ, nrSegments, plugTriangles, angle );
                    }
                    else
                    {
                        generateCylinder( center, holeOuterRadius, backRadius,
                                          plugZTop, pmTopZ, nrSegments, plugTriangles );
                    }
                }
            }
        }
    }

    // Process pads for post-machining plugs
    for( const FOOTPRINT* footprint : m_boardAdapter.GetBoard()->Footprints() )
    {
        for( const PAD* pad : footprint->Pads() )
        {
            if( !pad->HasHole() )
                continue;

            if( pad->GetDrillShape() != PAD_DRILL_SHAPE::CIRCLE )
                continue;

            const SFVEC2F padCenter( pad->GetPosition().x * unitScale,
                                     -pad->GetPosition().y * unitScale );
            const float holeInnerRadius = pad->GetDrillSize().x * 0.5f * unitScale;
            const float holeOuterRadius = holeInnerRadius + platingThickness * unitScale;
            const int nrSegments = m_boardAdapter.GetCircleSegmentCount( pad->GetDrillSize().x );

            float padZTop, padZBot, padDummy;
            getLayerZPos( F_Cu, padZTop, padDummy );
            getLayerZPos( B_Cu, padDummy, padZBot );

            // Handle front post-machining plugs for pads
            const auto frontMode = pad->GetFrontPostMachining();

            if( frontMode.has_value()
                && frontMode.value() != PAD_DRILL_POST_MACHINING_MODE::NOT_POST_MACHINED
                && frontMode.value() != PAD_DRILL_POST_MACHINING_MODE::UNKNOWN )
            {
                const float frontRadius = pad->GetFrontPostMachiningSize() * 0.5f * unitScale;
                const float frontDepth = pad->GetFrontPostMachiningDepth() * unitScale;

                if( frontRadius > holeOuterRadius && frontDepth > 0 )
                {
                    float pmBottomZ = normalizeZ( padZTop - frontDepth );
                    float plugZBot = normalizeZ( padZBot );

                    if( pmBottomZ > plugZBot )
                    {
                        if( frontMode.value() == PAD_DRILL_POST_MACHINING_MODE::COUNTERSINK )
                        {
                            EDA_ANGLE angle( pad->GetFrontPostMachiningAngle(), TENTHS_OF_A_DEGREE_T );
                            generateInvCone( padCenter, holeOuterRadius, frontRadius,
                                             pmBottomZ, plugZBot, nrSegments, plugTriangles, angle );
                        }
                        else
                        {
                            generateCylinder( padCenter, holeOuterRadius, frontRadius,
                                              pmBottomZ, plugZBot, nrSegments, plugTriangles );
                        }
                    }
                }
            }

            // Handle back post-machining plugs for pads
            const auto backMode = pad->GetBackPostMachining();

            if( backMode.has_value()
                && backMode.value() != PAD_DRILL_POST_MACHINING_MODE::NOT_POST_MACHINED
                && backMode.value() != PAD_DRILL_POST_MACHINING_MODE::UNKNOWN )
            {
                const float backRadius = pad->GetBackPostMachiningSize() * 0.5f * unitScale;
                const float backDepth = pad->GetBackPostMachiningDepth() * unitScale;

                if( backRadius > holeOuterRadius && backDepth > 0 )
                {
                    float plugZTop = normalizeZ( padZTop );
                    float pmTopZ = normalizeZ( padZBot + backDepth );

                    if( plugZTop > pmTopZ )
                    {
                        if( backMode.value() == PAD_DRILL_POST_MACHINING_MODE::COUNTERSINK )
                        {
                            EDA_ANGLE angle( pad->GetBackPostMachiningAngle(), TENTHS_OF_A_DEGREE_T );
                            generateInvCone( padCenter, holeOuterRadius, backRadius,
                                             plugZTop, pmTopZ, nrSegments, plugTriangles, angle );
                        }
                        else
                        {
                            generateCylinder( padCenter, holeOuterRadius, backRadius,
                                              plugZTop, pmTopZ, nrSegments, plugTriangles );
                        }
                    }
                }
            }
        }
    }

    // If we have any plug geometry, create a render list for it
    if( plugTriangles->m_layer_top_triangles->GetVertexSize() > 0
        || plugTriangles->m_layer_bot_triangles->GetVertexSize() > 0
        || plugTriangles->m_layer_middle_contours_quads->GetVertexSize() > 0 )
    {
        // Store the triangles for later cleanup
        m_triangles.push_back( plugTriangles );

        // Create a render list for the plugs using the same Z range as the board
        // This will be scaled and drawn alongside m_boardWithHoles in renderBoardBody()
        m_postMachinePlugs = new OPENGL_RENDER_LIST( *plugTriangles, m_circleTexture,
                                                     boardZTop, boardZTop );
    }
    else
    {
        delete plugTriangles;
    }
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

    // Also subtract counterbore, countersink, and backdrill polygons from the board
    if( m_boardAdapter.GetFrontCounterborePolys().OutlineCount() > 0 )
        board_poly_with_holes.BooleanSubtract( m_boardAdapter.GetFrontCounterborePolys() );

    if( m_boardAdapter.GetBackCounterborePolys().OutlineCount() > 0 )
        board_poly_with_holes.BooleanSubtract( m_boardAdapter.GetBackCounterborePolys() );

    if( m_boardAdapter.GetFrontCountersinkPolys().OutlineCount() > 0 )
        board_poly_with_holes.BooleanSubtract( m_boardAdapter.GetFrontCountersinkPolys() );

    if( m_boardAdapter.GetBackCountersinkPolys().OutlineCount() > 0 )
        board_poly_with_holes.BooleanSubtract( m_boardAdapter.GetBackCountersinkPolys() );

    if( m_boardAdapter.GetBackdrillPolys().OutlineCount() > 0 )
        board_poly_with_holes.BooleanSubtract( m_boardAdapter.GetBackdrillPolys() );

    if( m_boardAdapter.GetTertiarydrillPolys().OutlineCount() > 0 )
        board_poly_with_holes.BooleanSubtract( m_boardAdapter.GetTertiarydrillPolys() );


    m_boardWithHoles = createBoard( board_poly_with_holes, &m_boardAdapter.GetTH_IDs() );

    // Create plugs for backdrilled and post-machined areas
    backfillPostMachine();

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
            poly.BooleanSubtract( m_boardAdapter.GetFrontCounterborePolys() );
            poly.BooleanSubtract( m_boardAdapter.GetFrontCountersinkPolys() );
            poly.BooleanSubtract( m_boardAdapter.GetTertiarydrillPolys() );

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
            poly.BooleanSubtract( m_boardAdapter.GetBackCounterborePolys() );
            poly.BooleanSubtract( m_boardAdapter.GetBackCountersinkPolys() );
            poly.BooleanSubtract( m_boardAdapter.GetBackdrillPolys() );

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

    aDstLayer->AddToMiddleContours( outerContour, aZbot, aZtop, true );
    aDstLayer->AddToMiddleContours( innerContour, aZbot, aZtop, false );
}


void RENDER_3D_OPENGL::generateInvCone( const SFVEC2F& aCenter, float aInnerRadius,
                                        float aOuterRadius, float aZtop, float aZbot,
                                        unsigned int aNr_sides_per_circle,
                                        TRIANGLE_DISPLAY_LIST* aDstLayer, EDA_ANGLE aAngle )
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

    float zInnerTop = aZbot + innerHeight;

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

    // Outer contour wall goes full height
    aDstLayer->AddToMiddleContours( outerContour, aZbot, aZtop, true );
    // Inner contour wall only goes up to zInnerTop
    aDstLayer->AddToMiddleContours( innerContour, aZbot, zInnerTop, false );
}


void RENDER_3D_OPENGL::generateDisk( const SFVEC2F& aCenter, float aRadius, float aZ,
                                     unsigned int aNr_sides_per_circle, TRIANGLE_DISPLAY_LIST* aDstLayer,
                                     bool aTop )
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


void RENDER_3D_OPENGL::generateDimple( const SFVEC2F& aCenter, float aRadius, float aZ,
                                       float aDepth, unsigned int aNr_sides_per_circle,
                                       TRIANGLE_DISPLAY_LIST* aDstLayer, bool aTop )
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


bool RENDER_3D_OPENGL::appendPostMachiningGeometry( TRIANGLE_DISPLAY_LIST* aDstLayer,
                                                    const SFVEC2F& aHoleCenter,
                                                    PAD_DRILL_POST_MACHINING_MODE aMode,
                                                    int aSizeIU,
                                                    int aDepthIU,
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

    const int diameterBIU = std::max( aSizeIU,
                                      std::max( 1,
                                                (int) ( ( aHoleInnerRadius * 2.0f )
                                                        / aUnitScale ) ) );
    const unsigned int nrSegments =
            std::max( 12u, m_boardAdapter.GetCircleSegmentCount( diameterBIU ) );

    if( aMode == PAD_DRILL_POST_MACHINING_MODE::COUNTERBORE )
    {
        generateCylinder( aHoleCenter, radius, radius + aPlatingThickness3d, zTop, zBot,
                          nrSegments, aDstLayer );
        return true;
    }

    float csTopRadius = radius;
    float csBotRadius = aHoleInnerRadius;

    std::vector< SFVEC2F > innerContourTop, outerContourTop;
    std::vector< SFVEC2F > innerContourBot, outerContourBot;

    generateRing( aHoleCenter, csTopRadius, csTopRadius + aPlatingThickness3d, nrSegments,
                  innerContourTop, outerContourTop, false );
    generateRing( aHoleCenter, csBotRadius, csBotRadius + aPlatingThickness3d, nrSegments,
                  innerContourBot, outerContourBot, false );

    for( unsigned int i = 0; i < ( innerContourTop.size() - 1 ); ++i )
    {
        const SFVEC2F& vi0_top = innerContourTop[i + 0];
        const SFVEC2F& vi1_top = innerContourTop[i + 1];
        const SFVEC2F& vo0_top = outerContourTop[i + 0];
        const SFVEC2F& vo1_top = outerContourTop[i + 1];

        const SFVEC2F& vi0_bot = innerContourBot[i + 0];
        const SFVEC2F& vi1_bot = innerContourBot[i + 1];
        const SFVEC2F& vo0_bot = outerContourBot[i + 0];
        const SFVEC2F& vo1_bot = outerContourBot[i + 1];

        aDstLayer->m_layer_middle_contours_quads->AddQuad(
                SFVEC3F( vi1_top.x, vi1_top.y, zTop ),
                SFVEC3F( vi0_top.x, vi0_top.y, zTop ),
                SFVEC3F( vi0_bot.x, vi0_bot.y, zBot ),
                SFVEC3F( vi1_bot.x, vi1_bot.y, zBot ) );

        aDstLayer->m_layer_middle_contours_quads->AddQuad(
                SFVEC3F( vo1_top.x, vo1_top.y, zTop ),
                SFVEC3F( vo0_top.x, vo0_top.y, zTop ),
                SFVEC3F( vo0_bot.x, vo0_bot.y, zBot ),
                SFVEC3F( vo1_bot.x, vo1_bot.y, zBot ) );
    }

    return true;
}


void RENDER_3D_OPENGL::generateViaBarrels( float aPlatingThickness3d, float aUnitScale )
{
    if( !m_boardAdapter.GetBoard() || m_boardAdapter.GetViaCount() <= 0 )
        return;

    if( !m_boardAdapter.m_Cfg->m_Render.show_plated_barrels )
        return;

    float        averageDiameter = m_boardAdapter.GetAverageViaHoleDiameter();
    unsigned int averageSegCount = m_boardAdapter.GetCircleSegmentCount( averageDiameter );
    unsigned int trianglesEstimate = averageSegCount * 8 * m_boardAdapter.GetViaCount();

    TRIANGLE_DISPLAY_LIST* layerTriangleVIA = new TRIANGLE_DISPLAY_LIST( trianglesEstimate );

    for( const PCB_TRACK* track : m_boardAdapter.GetBoard()->Tracks() )
    {
        if( track->Type() != PCB_VIA_T )
            continue;

        const PCB_VIA* via = static_cast<const PCB_VIA*>( track );
        bool isBackdrilled = via->GetSecondaryDrillSize().has_value();
        bool isTertiarydrilled = via->GetTertiaryDrillSize().has_value();
        bool hasFrontPostMachining = via->GetFrontPostMachining().value_or( PAD_DRILL_POST_MACHINING_MODE::UNKNOWN )
                                     != PAD_DRILL_POST_MACHINING_MODE::UNKNOWN
                                     && via->GetFrontPostMachining().value_or( PAD_DRILL_POST_MACHINING_MODE::UNKNOWN )
                                                != PAD_DRILL_POST_MACHINING_MODE::NOT_POST_MACHINED;
        bool hasBackPostMachining = via->GetBackPostMachining().value_or( PAD_DRILL_POST_MACHINING_MODE::UNKNOWN )
                                    != PAD_DRILL_POST_MACHINING_MODE::UNKNOWN
                                    && via->GetBackPostMachining().value_or( PAD_DRILL_POST_MACHINING_MODE::UNKNOWN )
                                               != PAD_DRILL_POST_MACHINING_MODE::NOT_POST_MACHINED;

        if( via->GetViaType() == VIATYPE::THROUGH && !isBackdrilled
                && !hasFrontPostMachining && !hasBackPostMachining )
        {
            continue;
        }

        const float holediameter = via->GetDrillValue() * aUnitScale;
        const int nrSegments = m_boardAdapter.GetCircleSegmentCount( via->GetDrillValue() );
        const float hole_inner_radius = holediameter / 2.0f;

        const SFVEC2F via_center( via->GetStart().x * aUnitScale,
                                  -via->GetStart().y * aUnitScale );

        PCB_LAYER_ID top_layer, bottom_layer;
        via->LayerPair( &top_layer, &bottom_layer );

        float ztop, zbot, dummy;

        getLayerZPos( top_layer, ztop, dummy );
        getLayerZPos( bottom_layer, dummy, zbot );

        wxASSERT( zbot < ztop );

        float ztop_plated = ztop;
        float zbot_plated = zbot;

        if( isBackdrilled )
        {
            PCB_LAYER_ID secEnd = via->GetSecondaryDrillEndLayer();
            float secZEnd;

            // Backdrill goes from the back surface up to secEnd, so get the top of that layer
            // as the bottom of the plated barrel
            getLayerZPos( secEnd, secZEnd, dummy );
            zbot_plated = std::max( zbot_plated, secZEnd );
        }

        if( isTertiarydrilled )
        {
            PCB_LAYER_ID terEnd = via->GetTertiaryDrillEndLayer();
            float terZEnd;

            // Tertiary drill goes from the front surface down to terEnd, so get the bottom of that layer
            // as the top of the plated barrel
            getLayerZPos( terEnd, dummy, terZEnd );
            ztop_plated = std::min( ztop_plated, terZEnd );
        }

        auto applyViaPostMachining = [&]( bool isFront )
        {
            auto modeOpt = isFront ? via->GetFrontPostMachining()
                                   : via->GetBackPostMachining();

            if( !modeOpt
                    || modeOpt.value() == PAD_DRILL_POST_MACHINING_MODE::NOT_POST_MACHINED )
            {
                return;
            }

            int sizeIU = isFront ? via->GetFrontPostMachiningSize()
                                  : via->GetBackPostMachiningSize();
            int depthIU = isFront ? via->GetFrontPostMachiningDepth()
                                   : via->GetBackPostMachiningDepth();
            float zSurface = isFront ? ztop : zbot;
            float zEnd = 0.0f;

            if( appendPostMachiningGeometry( layerTriangleVIA, via_center, modeOpt.value(),
                                             sizeIU, depthIU, hole_inner_radius, zSurface,
                                             isFront, aPlatingThickness3d, aUnitScale, &zEnd ) )
            {
                if( isFront )
                    ztop_plated = std::min( ztop_plated, zEnd );
                else
                    zbot_plated = std::max( zbot_plated, zEnd );
            }
        };

        if( hasFrontPostMachining )
            applyViaPostMachining( true );
        if( hasBackPostMachining )
            applyViaPostMachining( false );

        // generateCylinder( via_center, hole_inner_radius,
        //                     hole_inner_radius + aPlatingThickness3d, ztop_plated,
        //                     zbot_plated, nrSegments, layerTriangleVIA );
    }

    const float padFrontSurface =
            m_boardAdapter.GetLayerBottomZPos( F_Cu )
            + m_boardAdapter.GetFrontCopperThickness() * 0.99f;
    const float padBackSurface =
            m_boardAdapter.GetLayerBottomZPos( B_Cu )
            - m_boardAdapter.GetBackCopperThickness() * 0.99f;

    for( const FOOTPRINT* footprint : m_boardAdapter.GetBoard()->Footprints() )
    {
        for( const PAD* pad : footprint->Pads() )
        {
            if( pad->GetAttribute() == PAD_ATTRIB::NPTH )
                continue;

            if( !pad->HasHole() )
                continue;

            if( pad->GetDrillShape() != PAD_DRILL_SHAPE::CIRCLE )
                continue;

            const SFVEC2F padCenter( pad->GetPosition().x * aUnitScale,
                                        -pad->GetPosition().y * aUnitScale );
            const float holeInnerRadius =
                    pad->GetDrillSize().x * 0.5f * aUnitScale;

            auto emitPadPostMachining = [&]( bool isFront )
            {
                auto modeOpt = isFront ? pad->GetFrontPostMachining()
                                        : pad->GetBackPostMachining();

                if( !modeOpt
                        || modeOpt.value() == PAD_DRILL_POST_MACHINING_MODE::NOT_POST_MACHINED )
                {
                    return;
                }

                int sizeIU = isFront ? pad->GetFrontPostMachiningSize()
                                        : pad->GetBackPostMachiningSize();
                int depthIU = isFront ? pad->GetFrontPostMachiningDepth()
                                        : pad->GetBackPostMachiningDepth();
                float zSurface = isFront ? padFrontSurface : padBackSurface;

                appendPostMachiningGeometry( layerTriangleVIA, padCenter, modeOpt.value(),
                                                sizeIU, depthIU, holeInnerRadius, zSurface,
                                                isFront, aPlatingThickness3d, aUnitScale,
                                                nullptr );
            };

            emitPadPostMachining( true );
            emitPadPostMachining( false );
        }
    }

    m_microviaHoles = new OPENGL_RENDER_LIST( *layerTriangleVIA, 0, 0.0f, 0.0f );

    delete layerTriangleVIA;
}


void RENDER_3D_OPENGL::generatePlatedHoleShells( int aPlatingThickness, float aUnitScale )
{
    if( !m_boardAdapter.GetBoard() )
        return;

    if( m_boardAdapter.GetHoleCount() <= 0 && m_boardAdapter.GetViaCount() <= 0 )
        return;

    SHAPE_POLY_SET tht_outer_holes_poly; // Stores the outer poly of the copper holes
    SHAPE_POLY_SET tht_inner_holes_poly; // Stores the inner poly of the copper holes

    tht_outer_holes_poly.RemoveAllContours();
    tht_inner_holes_poly.RemoveAllContours();

    for( const PCB_TRACK* track : m_boardAdapter.GetBoard()->Tracks() )
    {
        if( track->Type() != PCB_VIA_T )
            continue;

        const PCB_VIA* via = static_cast<const PCB_VIA*>( track );

        if( via->GetViaType() == VIATYPE::THROUGH )
        {
            TransformCircleToPolygon( tht_outer_holes_poly, via->GetPosition(),
                                      via->GetDrill() / 2 + aPlatingThickness,
                                      via->GetMaxError(), ERROR_INSIDE );

            TransformCircleToPolygon( tht_inner_holes_poly, via->GetPosition(), via->GetDrill() / 2,
                                      via->GetMaxError(), ERROR_INSIDE );
        }
    }

    for( const FOOTPRINT* footprint : m_boardAdapter.GetBoard()->Footprints() )
    {
        for( const PAD* pad : footprint->Pads() )
        {
            if( pad->GetAttribute() != PAD_ATTRIB::NPTH )
            {
                if( !pad->HasHole() )
                    continue;

                pad->TransformHoleToPolygon( tht_outer_holes_poly, aPlatingThickness,
                                             pad->GetMaxError(), ERROR_INSIDE );
                pad->TransformHoleToPolygon( tht_inner_holes_poly, 0,
                                             pad->GetMaxError(), ERROR_INSIDE );
            }
        }
    }

    tht_outer_holes_poly.BooleanSubtract( tht_inner_holes_poly );

    tht_outer_holes_poly.BooleanSubtract( m_antiBoardPolys );

    CONTAINER_2D holesContainer;

    ConvertPolygonToTriangles( tht_outer_holes_poly, holesContainer,
                               aUnitScale, *m_boardAdapter.GetBoard() );

    const LIST_OBJECT2D& holes2D = holesContainer.GetList();

    if( holes2D.size() > 0 && m_boardAdapter.m_Cfg->m_Render.show_plated_barrels )
    {
        float layer_z_top, layer_z_bot, dummy;

        getLayerZPos( F_Cu, layer_z_top, dummy );
        getLayerZPos( B_Cu, dummy, layer_z_bot );

        TRIANGLE_DISPLAY_LIST* layerTriangles = new TRIANGLE_DISPLAY_LIST( holes2D.size() );

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
            layerTriangles->AddToMiddleContours( tht_outer_holes_poly,
                                                 layer_z_bot, layer_z_top,
                                                 aUnitScale, false );

            m_padHoles = new OPENGL_RENDER_LIST( *layerTriangles, m_circleTexture,
                                                 layer_z_top, layer_z_top );
        }

        delete layerTriangles;
    }
}


void RENDER_3D_OPENGL::generateViaCovers( float aPlatingThickness3d, float aUnitScale )
{
    if( !m_boardAdapter.GetBoard() || m_boardAdapter.GetViaCount() <= 0 )
        return;

    TRIANGLE_DISPLAY_LIST* frontCover = new TRIANGLE_DISPLAY_LIST( m_boardAdapter.GetViaCount() );
    TRIANGLE_DISPLAY_LIST* backCover = new TRIANGLE_DISPLAY_LIST( m_boardAdapter.GetViaCount() );

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

        bool frontCovering = via->GetFrontCoveringMode() == COVERING_MODE::COVERED
                             || via->IsTented( F_Mask );
        bool backCovering = via->GetBackCoveringMode() == COVERING_MODE::COVERED
                            || via->IsTented( B_Mask );
        bool frontPlugged = via->GetFrontPluggingMode() == PLUGGING_MODE::PLUGGED;
        bool backPlugged  = via->GetBackPluggingMode() == PLUGGING_MODE::PLUGGED;
        bool filled = via->GetFillingMode() == FILLING_MODE::FILLED
                      || via->GetCappingMode() == CAPPING_MODE::CAPPED;

        const auto frontPostMachining =
                via->GetFrontPostMachining().value_or( PAD_DRILL_POST_MACHINING_MODE::NOT_POST_MACHINED );
        const auto backPostMachining =
                via->GetBackPostMachining().value_or( PAD_DRILL_POST_MACHINING_MODE::NOT_POST_MACHINED );

        bool hasFrontPostMachining = frontPostMachining != PAD_DRILL_POST_MACHINING_MODE::NOT_POST_MACHINED
                                     && frontPostMachining != PAD_DRILL_POST_MACHINING_MODE::UNKNOWN;
        bool hasBackPostMachining = backPostMachining != PAD_DRILL_POST_MACHINING_MODE::NOT_POST_MACHINED
                                    && backPostMachining != PAD_DRILL_POST_MACHINING_MODE::UNKNOWN;

        bool hasFrontBackdrill = via->GetSecondaryDrillStartLayer() == F_Cu;
        bool hasBackBackdrill = via->GetSecondaryDrillStartLayer() == B_Cu;

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
        m_viaFrontCover = new OPENGL_RENDER_LIST( *frontCover, 0, 0.0f, 0.0f );

    if( backCover->m_layer_bot_triangles->GetVertexSize() > 0 )
        m_viaBackCover = new OPENGL_RENDER_LIST( *backCover, 0, 0.0f, 0.0f );

    delete frontCover;
    delete backCover;
}


void RENDER_3D_OPENGL::generateViasAndPads()
{
    if( !m_boardAdapter.GetBoard() )
        return;

    const int   platingThickness    = m_boardAdapter.GetHolePlatingThickness();
    const float unitScale           = m_boardAdapter.BiuTo3dUnits();
    const float platingThickness3d  = platingThickness * unitScale;

    // generateViaBarrels( platingThickness3d, unitScale );
    // generatePlatedHoleShells( platingThickness, unitScale );
    // generateViaCovers( platingThickness3d, unitScale );
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

    S3D_CACHE* cacheMgr = m_boardAdapter.Get3dCacheManager();

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
                std::optional<LIBRARY_TABLE_ROW*> fpRow =
                    PROJECT_PCB::FootprintLibAdapter( m_boardAdapter.GetBoard()->GetProject() )
                            ->GetRow( libraryName );

                if( fpRow )
                    footprintBasePath = LIBRARY_MANAGER::GetFullURI( *fpRow, true );
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
                        MODEL_3D*     model        = new MODEL_3D( *modelPtr, materialMode );

                        m_3dModelMap[ fp_model.m_Filename ] = model;
                    }
                }
            }
        }
    }
}
