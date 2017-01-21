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
 * @file  c3d_render_createscene.cpp
 * @brief
 */


#include "c3d_render_raytracing.h"
#include "shapes3D/cplane.h"
#include "shapes3D/croundseg.h"
#include "shapes3D/clayeritem.h"
#include "shapes3D/ccylinder.h"
#include "shapes3D/ctriangle.h"
#include "shapes2D/citemlayercsg2d.h"
#include "shapes2D/cring2d.h"
#include "shapes2D/cpolygon2d.h"
#include "shapes2D/cfilledcircle2d.h"
#include "accelerators/cbvh_pbrt.h"
#include "3d_fastmath.h"
#include "3d_math.h"

#include <class_board.h>
#include <class_module.h>

#include <base_units.h>
#include <profile.h>        // To use GetRunningMicroSecs or an other profiling utility

/**
  * Scale convertion from 3d model units to pcb units
  */
#define UNITS3D_TO_UNITSPCB (IU_PER_MM)

void C3D_RENDER_RAYTRACING::setupMaterials()
{

    if( m_settings.GetFlag( FL_RENDER_RAYTRACING_PROCEDURAL_TEXTURES ) )
    {
        m_board_normal_perturbator  = CBOARDNORMAL( 0.5f * IU_PER_MM * m_settings.BiuTo3Dunits() );

        m_copper_normal_perturbator = CCOPPERNORMAL( 4.0f * IU_PER_MM * m_settings.BiuTo3Dunits(),
                                                     &m_board_normal_perturbator );

        m_solder_mask_normal_perturbator = CSOLDERMASKNORMAL( &m_board_normal_perturbator );

        m_plastic_normal_perturbator = CPLASTICNORMAL( 0.15f * IU_PER_MM * m_settings.BiuTo3Dunits() );

        m_plastic_shine_normal_perturbator = CPLASTICSHINENORMAL( 1.0f * IU_PER_MM * m_settings.BiuTo3Dunits() );

        m_brushed_metal_normal_perturbator = CMETALBRUSHEDNORMAL( 1.0f * IU_PER_MM * m_settings.BiuTo3Dunits() );
    }

    // http://devernay.free.fr/cours/opengl/materials.html

    // Copper
    m_materials.m_Copper = CBLINN_PHONG_MATERIAL(
                ConvertSRGBToLinear( (SFVEC3F)m_settings.m_CopperColor ) *
                                     (SFVEC3F)(0.18f),                  // ambient
                SFVEC3F( 0.0f, 0.0f, 0.0f ),                            // emissive
                glm::clamp( ((SFVEC3F)(1.0f) -
                             ConvertSRGBToLinear( (SFVEC3F)m_settings.m_CopperColor ) ),
                            SFVEC3F( 0.0f ),
                            SFVEC3F( 0.35f ) ),                         // specular
                0.4f * 128.0f,                                          // shiness
                0.0f,                                                   // transparency
                0.0f );

    if( m_settings.GetFlag( FL_RENDER_RAYTRACING_PROCEDURAL_TEXTURES ) )
        m_materials.m_Copper.SetNormalPerturbator( &m_copper_normal_perturbator );

    m_materials.m_Paste = CBLINN_PHONG_MATERIAL(
                ConvertSRGBToLinear( (SFVEC3F)m_settings.m_SolderPasteColor ) *
                ConvertSRGBToLinear( (SFVEC3F)m_settings.m_SolderPasteColor ), // ambient
                SFVEC3F( 0.0f, 0.0f, 0.0f ),            // emissive
                ConvertSRGBToLinear( (SFVEC3F)m_settings.m_SolderPasteColor ) *
                ConvertSRGBToLinear( (SFVEC3F)m_settings.m_SolderPasteColor ), // specular
                0.10f * 128.0f,                         // shiness
                0.0f,                                   // transparency
                0.0f );

    m_materials.m_SilkS = CBLINN_PHONG_MATERIAL(
                ConvertSRGBToLinear( SFVEC3F( 0.11f ) ),// ambient
                SFVEC3F( 0.0f, 0.0f, 0.0f ),            // emissive
                glm::clamp( ((SFVEC3F)(1.0f) -
                            ConvertSRGBToLinear( (SFVEC3F)m_settings.m_SilkScreenColor) ),
                            SFVEC3F( 0.0f ),
                            SFVEC3F( 0.10f ) ),         // specular
                0.078125f * 128.0f,                     // shiness
                0.0f,                                   // transparency
                0.0f );

    const float solderMask_gray = ( m_settings.m_SolderMaskColor.r +
                                    m_settings.m_SolderMaskColor.g +
                                    m_settings.m_SolderMaskColor.b ) / 3.0f;

    const float solderMask_transparency = solderMask_gray * 0.40f + 0.005f;

    m_materials.m_SolderMask = CBLINN_PHONG_MATERIAL(
                ConvertSRGBToLinear( (SFVEC3F)m_settings.m_SolderMaskColor ) *
                0.10f,                                  // ambient
                SFVEC3F( 0.0f, 0.0f, 0.0f ),            // emissive
                glm::clamp( ( (SFVEC3F)( 1.0f ) -
                            ConvertSRGBToLinear( (SFVEC3F)m_settings.m_SolderMaskColor ) ),
                            SFVEC3F( 0.0f ),
                            SFVEC3F( solderMask_gray * 2.0f ) ),         // specular
                0.85f * 128.0f,                         // shiness
                solderMask_transparency,                // transparency
                0.16f );                                // reflection

    m_materials.m_SolderMask.SetCastShadows( true );
    m_materials.m_SolderMask.SetNrRefractionsSamples( 1 );
    m_materials.m_SolderMask.SetNrReflectionsSamples( 2 );

    if( m_settings.GetFlag( FL_RENDER_RAYTRACING_PROCEDURAL_TEXTURES ) )
        m_materials.m_SolderMask.SetNormalPerturbator( &m_solder_mask_normal_perturbator );

    m_materials.m_EpoxyBoard = CBLINN_PHONG_MATERIAL(
                ConvertSRGBToLinear( SFVEC3F( 16.0f / 255.0f,
                                              14.0f / 255.0f,
                                              10.0f / 255.0f ) ), // ambient
                SFVEC3F( 0.0f, 0.0f, 0.0f ),                      // emissive
                ConvertSRGBToLinear( SFVEC3F( 10.0f / 255.0f,
                                              8.0f / 255.0f,
                                              10.0f / 255.0f ) ), // specular
                0.1f * 128.0f,                                    // shiness
                0.10f,                                            // transparency
                0.0f );                                           // reflection

    m_materials.m_EpoxyBoard.SetAbsorvance( 10.0f );
    m_materials.m_EpoxyBoard.SetNrRefractionsSamples( 3 );

    if( m_settings.GetFlag( FL_RENDER_RAYTRACING_PROCEDURAL_TEXTURES ) )
        m_materials.m_EpoxyBoard.SetNormalPerturbator( &m_board_normal_perturbator );

    SFVEC3F bgTop = ConvertSRGBToLinear( (SFVEC3F)m_settings.m_BgColorTop );
    //SFVEC3F bgBot = (SFVEC3F)m_settings.m_BgColorBot;

    m_materials.m_Floor = CBLINN_PHONG_MATERIAL(
                bgTop * 0.125f,                         // ambient
                SFVEC3F( 0.0f, 0.0f, 0.0f ),            // emissive
                (SFVEC3F(1.0f) - bgTop) / 3.0f,         // specular
                0.10f * 128.0f,                         // shiness
                0.0f,                                   // transparency
                0.50f );                                // reflection
}



/** Function create_3d_object_from
 * @brief Creates on or more 3D objects form a 2D object and Z positions. It try
 * optimize some types of objects that will be faster to trace than the
 * CLAYERITEM object.
 * @param aObject2D
 * @param aZMin
 * @param aZMax
 */
void C3D_RENDER_RAYTRACING::create_3d_object_from(  CCONTAINER &aDstContainer,
                                                    const COBJECT2D *aObject2D,
                                                    float aZMin, float aZMax,
                                                    const CMATERIAL *aMaterial,
                                                    const SFVEC3F &aObjColor )
{
    switch( aObject2D->GetObjectType() )
    {
        case OBJ2D_DUMMYBLOCK:
        {
            m_stats_converted_dummy_to_plane++;
#if 1
            CXYPLANE *objPtr;
            objPtr = new CXYPLANE( CBBOX ( SFVEC3F( aObject2D->GetBBox().Min().x,
                                                    aObject2D->GetBBox().Min().y,
                                                    aZMin ),
                                           SFVEC3F( aObject2D->GetBBox().Max().x,
                                                    aObject2D->GetBBox().Max().y,
                                                    aZMin ) ) );
            objPtr->SetMaterial( aMaterial );
            objPtr->SetColor( ConvertSRGBToLinear( aObjColor ) );
            aDstContainer.Add( objPtr );

            objPtr = new CXYPLANE( CBBOX ( SFVEC3F( aObject2D->GetBBox().Min().x,
                                                    aObject2D->GetBBox().Min().y,
                                                    aZMax ),
                                           SFVEC3F( aObject2D->GetBBox().Max().x,
                                                    aObject2D->GetBBox().Max().y,
                                                    aZMax ) ) );
            objPtr->SetMaterial( aMaterial );
            objPtr->SetColor( ConvertSRGBToLinear( aObjColor ) );
            aDstContainer.Add( objPtr );
#else
            objPtr = new CDUMMYBLOCK( CBBOX ( SFVEC3F( aObject2D->GetBBox().Min().x,
                                                       aObject2D->GetBBox().Min().y,
                                                       aZMin ),
                                              SFVEC3F( aObject2D->GetBBox().Max().x,
                                                       aObject2D->GetBBox().Max().y,
                                                       aZMax ) ) );
            objPtr->SetMaterial( aMaterial );
            aDstContainer.Add( objPtr );
#endif
        }
        break;

        case OBJ2D_ROUNDSEG:
        {
            m_stats_converted_roundsegment2d_to_roundsegment++;

            const CROUNDSEGMENT2D *aRoundSeg2D = static_cast<const CROUNDSEGMENT2D *>( aObject2D );
            CROUNDSEG *objPtr = new CROUNDSEG( *aRoundSeg2D, aZMin, aZMax );
            objPtr->SetMaterial( aMaterial );
            objPtr->SetColor( ConvertSRGBToLinear( aObjColor ) );
            aDstContainer.Add( objPtr );
        }
        break;


        default:
        {
            CLAYERITEM *objPtr = new CLAYERITEM( aObject2D, aZMin, aZMax );
            objPtr->SetMaterial( aMaterial );
            objPtr->SetColor( ConvertSRGBToLinear( aObjColor ) );
            aDstContainer.Add( objPtr );
        }
        break;
    }
}


void C3D_RENDER_RAYTRACING::reload( REPORTER *aStatusTextReporter )
{
    m_reloadRequested = false;

    m_model_materials.clear();

    COBJECT2D_STATS::Instance().ResetStats();
    COBJECT3D_STATS::Instance().ResetStats();

#ifdef PRINT_STATISTICS_3D_VIEWER
    printf("InitSettings...\n");
#endif

    unsigned stats_startReloadTime = GetRunningMicroSecs();

    m_settings.InitSettings( aStatusTextReporter );

#ifdef PRINT_STATISTICS_3D_VIEWER
    unsigned stats_endReloadTime = GetRunningMicroSecs();
    unsigned stats_startConvertTime = GetRunningMicroSecs();
 #endif

    SFVEC3F camera_pos = m_settings.GetBoardCenter3DU();
    m_settings.CameraGet().SetBoardLookAtPos( camera_pos );

    m_object_container.Clear();
    m_containerWithObjectsToDelete.Clear();


    // Create and add the outline board
    // /////////////////////////////////////////////////////////////////////////

#ifdef PRINT_STATISTICS_3D_VIEWER
    printf("Create outline board...\n");
#endif

    delete m_outlineBoard2dObjects;

    m_outlineBoard2dObjects = new CCONTAINER2D;

    if( ((const SHAPE_POLY_SET &)m_settings.GetBoardPoly()).OutlineCount() == 1 )
    {
        float divFactor = 0.0f;

        if( m_settings.GetStats_Nr_Vias() )
            divFactor = m_settings.GetStats_Med_Via_Hole_Diameter3DU() * 18.0f;
        else
            if( m_settings.GetStats_Nr_Holes() )
                divFactor = m_settings.GetStats_Med_Hole_Diameter3DU() * 8.0f;

        SHAPE_POLY_SET boardPolyCopy = m_settings.GetBoardPoly();
        boardPolyCopy.Fracture( SHAPE_POLY_SET::PM_FAST );

        Convert_path_polygon_to_polygon_blocks_and_dummy_blocks(
                    boardPolyCopy,
                    *m_outlineBoard2dObjects,
                    m_settings.BiuTo3Dunits(),
                    divFactor,
                    (const BOARD_ITEM &)*m_settings.GetBoard() );

        if( m_settings.GetFlag( FL_SHOW_BOARD_BODY ) )
        {
            const LIST_OBJECT2D &listObjects = m_outlineBoard2dObjects->GetList();

            for( LIST_OBJECT2D::const_iterator object2d_iterator = listObjects.begin();
                 object2d_iterator != listObjects.end();
                 ++object2d_iterator )
            {
                const COBJECT2D *object2d_A = static_cast<const COBJECT2D *>(*object2d_iterator);

                std::vector<const COBJECT2D *> *object2d_B = new std::vector<const COBJECT2D *>();

                // Check if there are any THT that intersects this outline object part
                if( !m_settings.GetThroughHole_Outer().GetList().empty() )
                {

                    CONST_LIST_OBJECT2D intersectionList;
                    m_settings.GetThroughHole_Outer().GetListObjectsIntersects(
                                object2d_A->GetBBox(),
                                intersectionList );

                    if( !intersectionList.empty() )
                    {
                        for( CONST_LIST_OBJECT2D::const_iterator hole = intersectionList.begin();
                             hole != intersectionList.end();
                             ++hole )
                        {
                            const COBJECT2D *hole2d = static_cast<const COBJECT2D *>(*hole);

                            if( object2d_A->Intersects( hole2d->GetBBox() ) )
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

                if( object2d_B == CSGITEM_EMPTY )
                {
        #if 0
                    create_3d_object_from( m_object_container, object2d_A,
                                           m_settings.GetLayerBottomZpos3DU( F_Cu ),
                                           m_settings.GetLayerBottomZpos3DU( B_Cu ),
                                           &m_materials.m_EpoxyBoard,
                                           g_epoxyColor );
        #else
                    CLAYERITEM *objPtr = new CLAYERITEM( object2d_A,
                                                         m_settings.GetLayerBottomZpos3DU( F_Cu ),
                                                         m_settings.GetLayerBottomZpos3DU( B_Cu ) );

                    objPtr->SetMaterial( &m_materials.m_EpoxyBoard );
                    objPtr->SetColor( ConvertSRGBToLinear( (SFVEC3F)m_settings.m_BoardBodyColor ) );
                    m_object_container.Add( objPtr );
        #endif
                }
                else
                {
                    CITEMLAYERCSG2D *itemCSG2d = new CITEMLAYERCSG2D(
                                object2d_A,
                                object2d_B,
                                CSGITEM_FULL,
                                (const BOARD_ITEM &)*m_settings.GetBoard() );

                    m_containerWithObjectsToDelete.Add( itemCSG2d );

                    CLAYERITEM *objPtr = new CLAYERITEM( itemCSG2d,
                                                         m_settings.GetLayerBottomZpos3DU( F_Cu ),
                                                         m_settings.GetLayerBottomZpos3DU( B_Cu ) );

                    objPtr->SetMaterial( &m_materials.m_EpoxyBoard );
                    objPtr->SetColor( ConvertSRGBToLinear( (SFVEC3F)m_settings.m_BoardBodyColor ) );
                    m_object_container.Add( objPtr );
                }
            }

            // Add cylinders of the board body to container
            // Note: This is actually a workarround for the holes in the board.
            // The issue is because if a hole is in a border of a divided polygon ( ex
            // a polygon or dummyblock) it will cut also the render of the hole.
            // So this will add a full hole.
            // In fact, that is not need if the hole have copper.
            // /////////////////////////////////////////////////////////////////////////
            if( !m_settings.GetThroughHole_Outer().GetList().empty() )
            {
                const LIST_OBJECT2D &holeList = m_settings.GetThroughHole_Outer().GetList();

                for( LIST_OBJECT2D::const_iterator hole = holeList.begin();
                     hole != holeList.end();
                     ++hole )
                {
                    const COBJECT2D *hole2d = static_cast<const COBJECT2D *>(*hole);

                    switch( hole2d->GetObjectType() )
                    {
                    case OBJ2D_FILLED_CIRCLE:
                    {
                        const float radius = hole2d->GetBBox().GetExtent().x * 0.5f * 0.999f;

                        CVCYLINDER *objPtr = new CVCYLINDER(
                                    hole2d->GetCentroid(),
                                    NextFloatDown( m_settings.GetLayerBottomZpos3DU( F_Cu ) ),
                                    NextFloatUp( m_settings.GetLayerBottomZpos3DU( B_Cu ) ),
                                    radius );

                        objPtr->SetMaterial( &m_materials.m_EpoxyBoard );
                        objPtr->SetColor( ConvertSRGBToLinear( (SFVEC3F)m_settings.m_BoardBodyColor ) );

                        m_object_container.Add( objPtr );
                    }
                    break;

                    default:
                        break;
                    }
                }
            }
        }
    }


    // Add layers maps (except B_Mask and F_Mask)
    // /////////////////////////////////////////////////////////////////////////

#ifdef PRINT_STATISTICS_3D_VIEWER
    printf("Add layers maps...\n");
#endif

    for( MAP_CONTAINER_2D::const_iterator ii = m_settings.GetMapLayers().begin();
         ii != m_settings.GetMapLayers().end();
         ++ii )
    {
        LAYER_ID layer_id = static_cast<LAYER_ID>(ii->first);

        // Mask kayers are not processed here because they are a special case
        if( (layer_id == B_Mask) || (layer_id == F_Mask) )
            continue;

        CMATERIAL *materialLayer = &m_materials.m_SilkS;
        SFVEC3F layerColor = SFVEC3F( 0.0f, 0.0f, 0.0f );

        switch( layer_id )
        {
            case B_Adhes:
            case F_Adhes:
            break;

            case B_Paste:
            case F_Paste:
                materialLayer = &m_materials.m_Paste;

                if( m_settings.GetFlag( FL_USE_REALISTIC_MODE ) )
                    layerColor = m_settings.m_SolderPasteColor;
                else
                    layerColor = m_settings.GetLayerColor( layer_id );
            break;

            case B_SilkS:
            case F_SilkS:
                materialLayer = &m_materials.m_SilkS;

                if( m_settings.GetFlag( FL_USE_REALISTIC_MODE ) )
                    layerColor = m_settings.m_SilkScreenColor;
                else
                    layerColor = m_settings.GetLayerColor( layer_id );
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
                materialLayer = &m_materials.m_Copper;

                if( m_settings.GetFlag( FL_USE_REALISTIC_MODE ) )
                    layerColor = m_settings.m_CopperColor;
                else
                    layerColor = m_settings.GetLayerColor( layer_id );
            break;
        }

        const CBVHCONTAINER2D *container2d = static_cast<const CBVHCONTAINER2D *>(ii->second);
        const LIST_OBJECT2D &listObject2d = container2d->GetList();

        for( LIST_OBJECT2D::const_iterator itemOnLayer = listObject2d.begin();
             itemOnLayer != listObject2d.end();
             ++itemOnLayer )
        {
            const COBJECT2D *object2d_A = static_cast<const COBJECT2D *>(*itemOnLayer);

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

                    const CBVHCONTAINER2D *containerLayerHoles2d =
                            static_cast<const CBVHCONTAINER2D *>(ii_hole->second);


                    CONST_LIST_OBJECT2D intersectionList;
                    containerLayerHoles2d->GetListObjectsIntersects( object2d_A->GetBBox(),
                                                                     intersectionList );

                    if( !intersectionList.empty() )
                    {
                        for( CONST_LIST_OBJECT2D::const_iterator holeOnLayer =
                             intersectionList.begin();
                             holeOnLayer != intersectionList.end();
                             ++holeOnLayer )
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
                if( !m_settings.GetThroughHole_Outer().GetList().empty() )
                {
                    CONST_LIST_OBJECT2D intersectionList;

                    m_settings.GetThroughHole_Outer().GetListObjectsIntersects(
                                object2d_A->GetBBox(),
                                intersectionList );

                    if( !intersectionList.empty() )
                    {
                        for( CONST_LIST_OBJECT2D::const_iterator hole = intersectionList.begin();
                             hole != intersectionList.end();
                             ++hole )
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
               create_3d_object_from( m_object_container,
                                      object2d_A,
                                      m_settings.GetLayerBottomZpos3DU( layer_id ),
                                      m_settings.GetLayerTopZpos3DU( layer_id ),
                                      materialLayer,
                                      layerColor );
#else
                CLAYERITEM *objPtr = new CLAYERITEM( object2d_A,
                                                     m_settings.GetLayerBottomZpos3DU( layer_id ),
                                                     m_settings.GetLayerTopZpos3DU( layer_id ) );
                objPtr->SetMaterial( materialLayer );
                objPtr->SetColor( ConvertSRGBToLinear( layerColor ) );
                m_object_container.Add( objPtr );
#endif
            }
            else
            {
#if 1
                CITEMLAYERCSG2D *itemCSG2d = new CITEMLAYERCSG2D( object2d_A,
                                                                  object2d_B,
                                                                  object2d_C,
                                                                  object2d_A->GetBoardItem() );
                m_containerWithObjectsToDelete.Add( itemCSG2d );

                CLAYERITEM *objPtr = new CLAYERITEM( itemCSG2d,
                                                     m_settings.GetLayerBottomZpos3DU( layer_id ),
                                                     m_settings.GetLayerTopZpos3DU( layer_id ) );

                objPtr->SetMaterial( materialLayer );
                objPtr->SetColor( ConvertSRGBToLinear( layerColor ) );

                m_object_container.Add( objPtr );
#endif
            }
        }
    }// for each layer on map



    // Add Mask layer
    // Solder mask layers are "negative" layers so the elements that we have
    // (in the container) should remove the board outline.
    // We will check for all objects in the outline if it intersects any object
    // in the layer container and also any hole.
    // /////////////////////////////////////////////////////////////////////////
    if( m_settings.GetFlag( FL_SOLDERMASK ) &&
        (m_outlineBoard2dObjects->GetList().size() >= 1) )
    {
        CMATERIAL *materialLayer = &m_materials.m_SolderMask;

        for( MAP_CONTAINER_2D::const_iterator ii = m_settings.GetMapLayers().begin();
             ii != m_settings.GetMapLayers().end();
             ++ii )
        {
            LAYER_ID layer_id = static_cast<LAYER_ID>(ii->first);

            const CBVHCONTAINER2D *containerLayer2d =
                    static_cast<const CBVHCONTAINER2D *>(ii->second);

            // Only get the Solder mask layers
            if( !((layer_id == B_Mask) || (layer_id == F_Mask)) )
                continue;

            SFVEC3F layerColor;
            if( m_settings.GetFlag( FL_USE_REALISTIC_MODE ) )
                layerColor = m_settings.m_SolderMaskColor;
            else
                layerColor = m_settings.GetLayerColor( layer_id );

            const float zLayerMin = m_settings.GetLayerBottomZpos3DU( layer_id );
            const float zLayerMax = m_settings.GetLayerTopZpos3DU( layer_id );

            // Get the outline board objects
            const LIST_OBJECT2D &listObjects = m_outlineBoard2dObjects->GetList();

            for( LIST_OBJECT2D::const_iterator object2d_iterator = listObjects.begin();
                 object2d_iterator != listObjects.end();
                 ++object2d_iterator )
            {
                const COBJECT2D *object2d_A = static_cast<const COBJECT2D *>(*object2d_iterator);

                std::vector<const COBJECT2D *> *object2d_B = new std::vector<const COBJECT2D *>();

                // Check if there are any THT that intersects this outline object part
                if( !m_settings.GetThroughHole_Outer().GetList().empty() )
                {

                    CONST_LIST_OBJECT2D intersectionList;

                    m_settings.GetThroughHole_Outer().GetListObjectsIntersects(
                                object2d_A->GetBBox(),
                                intersectionList );

                    if( !intersectionList.empty() )
                    {
                        for( CONST_LIST_OBJECT2D::const_iterator hole = intersectionList.begin();
                             hole != intersectionList.end();
                             ++hole )
                        {
                            const COBJECT2D *hole2d = static_cast<const COBJECT2D *>(*hole);

                            if( object2d_A->Intersects( hole2d->GetBBox() ) )
                            //if( object2d_A->GetBBox().Intersects( hole2d->GetBBox() ) )
                                object2d_B->push_back( hole2d );
                        }
                    }
                }

                // Check if there are any objects in the layer to subtract with the
                // corrent object
                if( !containerLayer2d->GetList().empty() )
                {
                    CONST_LIST_OBJECT2D intersectionList;

                    containerLayer2d->GetListObjectsIntersects( object2d_A->GetBBox(),
                                                                intersectionList );

                    if( !intersectionList.empty() )
                    {
                        for( CONST_LIST_OBJECT2D::const_iterator obj = intersectionList.begin();
                             obj != intersectionList.end();
                             ++obj )
                        {
                            const COBJECT2D *obj2d = static_cast<const COBJECT2D *>(*obj);

                            //if( object2d_A->Intersects( obj2d->GetBBox() ) )
                            //if( object2d_A->GetBBox().Intersects( obj2d->GetBBox() ) )
                                object2d_B->push_back( obj2d );
                        }
                    }
                }

                if( object2d_B->empty() )
                {
                    delete object2d_B;
                    object2d_B = CSGITEM_EMPTY;
                }

                if( object2d_B == CSGITEM_EMPTY )
                {
        #if 0
                   create_3d_object_from( m_object_container,
                                          object2d_A,
                                          zLayerMin,
                                          zLayerMax,
                                          materialLayer,
                                          layerColor );
        #else
                    CLAYERITEM *objPtr =  new CLAYERITEM( object2d_A,
                                                          zLayerMin,
                                                          zLayerMax );

                    objPtr->SetMaterial( materialLayer );
                    objPtr->SetColor( ConvertSRGBToLinear( layerColor ) );

                    m_object_container.Add( objPtr );
        #endif
                }
                else
                {
                    CITEMLAYERCSG2D *itemCSG2d = new CITEMLAYERCSG2D( object2d_A,
                                                                      object2d_B,
                                                                      CSGITEM_FULL,
                                                                      object2d_A->GetBoardItem() );

                    m_containerWithObjectsToDelete.Add( itemCSG2d );

                    CLAYERITEM *objPtr =  new CLAYERITEM( itemCSG2d,
                                                          zLayerMin,
                                                          zLayerMax );
                    objPtr->SetMaterial( materialLayer );
                    objPtr->SetColor( ConvertSRGBToLinear( layerColor ) );

                    m_object_container.Add( objPtr );
                }
            }
        }
    }

    add_3D_vias_and_pads_to_container();

#ifdef PRINT_STATISTICS_3D_VIEWER
    unsigned stats_endConvertTime = GetRunningMicroSecs();
    unsigned stats_startLoad3DmodelsTime = stats_endConvertTime;
#endif


    load_3D_models();


#ifdef PRINT_STATISTICS_3D_VIEWER
    unsigned stats_endLoad3DmodelsTime = GetRunningMicroSecs();
#endif

    // Add floor
    // /////////////////////////////////////////////////////////////////////////
    if( m_settings.GetFlag( FL_RENDER_RAYTRACING_BACKFLOOR ) )
    {
        CBBOX boardBBox = m_settings.GetBBox3DU();

        if( boardBBox.IsInitialized() )
        {
            boardBBox.Scale( 3.0f );

            if( m_object_container.GetList().size() > 0 )
            {
                CBBOX containerBBox = m_object_container.GetBBox();

                containerBBox.Scale( 1.3f );

                const SFVEC3F centerBBox = containerBBox.GetCenter();

                const float minZ = glm::min( containerBBox.Min().z,
                                             boardBBox.Min().z );

                const SFVEC3F v1 = SFVEC3F( -RANGE_SCALE_3D * 4.0f,
                                            -RANGE_SCALE_3D * 4.0f,
                                            minZ ) +
                                   SFVEC3F( centerBBox.x,
                                            centerBBox.y,
                                            0.0f );

                const SFVEC3F v3 = SFVEC3F( +RANGE_SCALE_3D * 4.0f,
                                            +RANGE_SCALE_3D * 4.0f,
                                            minZ ) +
                                   SFVEC3F( centerBBox.x,
                                            centerBBox.y,
                                            0.0f );

                const SFVEC3F v2 = SFVEC3F( v1.x, v3.y, v1.z );
                const SFVEC3F v4 = SFVEC3F( v3.x, v1.y, v1.z );

                CTRIANGLE *newTriangle1 = new CTRIANGLE( v1, v2, v3 );
                CTRIANGLE *newTriangle2 = new CTRIANGLE( v3, v4, v1 );

                m_object_container.Add( newTriangle1 );
                m_object_container.Add( newTriangle2 );

                newTriangle1->SetMaterial( (const CMATERIAL *)&m_materials.m_Floor );
                newTriangle2->SetMaterial( (const CMATERIAL *)&m_materials.m_Floor );

                newTriangle1->SetColor( ConvertSRGBToLinear( (SFVEC3F)m_settings.m_BgColorTop ) );
                newTriangle2->SetColor( ConvertSRGBToLinear( (SFVEC3F)m_settings.m_BgColorTop ) );
            }
        }
    }


    // Init initial lights
    // /////////////////////////////////////////////////////////////////////////
    m_lights.Clear();

    // This will work as the front camera light.
    const float light_camera_intensity = 0.20;
    const float light_top_bottom = 0.25;
    const float light_directional_intensity = ( 1.0f - ( light_camera_intensity +
                                                         light_top_bottom * 0.5f ) ) / 4.0f;

    m_camera_light = new CDIRECTIONALLIGHT( SFVEC3F( 0.0f, 0.0f, 0.0f ),
                                            SFVEC3F( light_camera_intensity ) );
    m_camera_light->SetCastShadows( false );
    m_lights.Add( m_camera_light );

    // Option 1 - using Point Lights

    const SFVEC3F &boarCenter = m_settings.GetBBox3DU().GetCenter();

    m_lights.Add( new CPOINTLIGHT( SFVEC3F( boarCenter.x, boarCenter.y, +RANGE_SCALE_3D * 2.0f ),
                                   SFVEC3F( light_top_bottom ) ) );

    m_lights.Add( new CPOINTLIGHT( SFVEC3F( boarCenter.x, boarCenter.y, -RANGE_SCALE_3D * 2.0f ),
                                   SFVEC3F( light_top_bottom ) ) );


    // http://www.flashandmath.com/mathlets/multicalc/coords/shilmay23fin.html

    // Option 2 - Top/Bottom direction lights
    /*
    m_lights.Add( new CDIRECTIONALLIGHT( SphericalToCartesian( glm::pi<float>() * 0.03f,
                                                               glm::pi<float>() * 0.25f ),
                                         SFVEC3F( light_top_bottom ) ) );

    m_lights.Add( new CDIRECTIONALLIGHT( SphericalToCartesian( glm::pi<float>() * 0.97f,
                                                               glm::pi<float>() * 1.25f ),
                                         SFVEC3F( light_top_bottom ) ) );
    */

    m_lights.Add( new CDIRECTIONALLIGHT( SphericalToCartesian( glm::pi<float>() * 1.0f / 8.0f,
                                                               glm::pi<float>() * 1 / 4.0f ),
                                         SFVEC3F( light_directional_intensity ) ) );
    m_lights.Add( new CDIRECTIONALLIGHT( SphericalToCartesian( glm::pi<float>() * 1.0f / 8.0f,
                                                               glm::pi<float>() * 3 / 4.0f ),
                                         SFVEC3F( light_directional_intensity ) ) );
    m_lights.Add( new CDIRECTIONALLIGHT( SphericalToCartesian( glm::pi<float>() * 1.0f / 8.0f,
                                                               glm::pi<float>() * 5 / 4.0f ),
                                         SFVEC3F( light_directional_intensity ) ) );
    m_lights.Add( new CDIRECTIONALLIGHT( SphericalToCartesian( glm::pi<float>() * 1.0f / 8.0f,
                                                               glm::pi<float>() * 7 / 4.0f ),
                                         SFVEC3F( light_directional_intensity ) ) );


    m_lights.Add( new CDIRECTIONALLIGHT( SphericalToCartesian( glm::pi<float>() * 7.0f / 8.0f,
                                                               glm::pi<float>() * 1 / 4.0f ),
                                         SFVEC3F( light_directional_intensity ) ) );
    m_lights.Add( new CDIRECTIONALLIGHT( SphericalToCartesian( glm::pi<float>() * 7.0f / 8.0f,
                                                               glm::pi<float>() * 3 / 4.0f ),
                                         SFVEC3F( light_directional_intensity ) ) );
    m_lights.Add( new CDIRECTIONALLIGHT( SphericalToCartesian( glm::pi<float>() * 7.0f / 8.0f,
                                                               glm::pi<float>() * 5 / 4.0f ),
                                         SFVEC3F( light_directional_intensity ) ) );
    m_lights.Add( new CDIRECTIONALLIGHT( SphericalToCartesian( glm::pi<float>() * 7.0f / 8.0f,
                                                               glm::pi<float>() * 7 / 4.0f ),
                                         SFVEC3F( light_directional_intensity ) ) );


    // Create an accelerator
    // /////////////////////////////////////////////////////////////////////////

#ifdef PRINT_STATISTICS_3D_VIEWER
    unsigned stats_startAcceleratorTime = GetRunningMicroSecs();
#endif

    if( m_accelerator )
    {
        delete m_accelerator;
    }
    m_accelerator = 0;

    //m_accelerator = new CGRID( m_object_container );
    m_accelerator = new CBVH_PBRT( m_object_container );

#ifdef PRINT_STATISTICS_3D_VIEWER
    unsigned stats_endAcceleratorTime = GetRunningMicroSecs();
#endif

    setupMaterials();

#ifdef PRINT_STATISTICS_3D_VIEWER
    printf( "C3D_RENDER_RAYTRACING::reload times:\n" );
    printf( "  Reload board:             %.3f ms\n", (float)( stats_endReloadTime -
                                                              stats_startReloadTime ) /

                                                     1000.0f );
    printf( "  Convert to 3D objects:    %.3f ms\n", (float)( stats_endConvertTime -
                                                              stats_startConvertTime ) /
                                                     1000.0f );
    printf( "  Accelerator construction: %.3f ms\n", (float)( stats_endAcceleratorTime -
                                                              stats_startAcceleratorTime ) /
                                                     1000.0f );
    printf( "  Load and add 3D models:   %.3f ms\n", (float)( stats_endLoad3DmodelsTime -
                                                              stats_startLoad3DmodelsTime ) /
                                                     1000.0f );
    printf( "Optimizations\n" );

    printf( "  m_stats_converted_dummy_to_plane: %u\n",
            m_stats_converted_dummy_to_plane );

    printf( "  m_stats_converted_roundsegment2d_to_roundsegment: %u\n",
            m_stats_converted_roundsegment2d_to_roundsegment );

    COBJECT2D_STATS::Instance().PrintStats();
    COBJECT3D_STATS::Instance().PrintStats();
#endif

    if( aStatusTextReporter )
    {
        // Calculation time in seconds
        const double calculation_time = (double)( GetRunningMicroSecs() -
                                                  stats_startReloadTime ) / 1e6;

        aStatusTextReporter->Report( wxString::Format( _( "Reload time %.3f s" ),
                                                       calculation_time ) );
    }
}



// Based on draw3DViaHole from
// 3d_draw_helper_functions.cpp
void C3D_RENDER_RAYTRACING::insert3DViaHole( const VIA* aVia )
{
    LAYER_ID    top_layer, bottom_layer;
    int radiusBUI = (aVia->GetDrillValue() / 2);

    aVia->LayerPair( &top_layer, &bottom_layer );

    float topZ = m_settings.GetLayerBottomZpos3DU( top_layer ) +
                 m_settings.GetCopperThickness3DU();

    float botZ = m_settings.GetLayerBottomZpos3DU( bottom_layer ) -
                 m_settings.GetCopperThickness3DU();

    const SFVEC2F center = SFVEC2F( aVia->GetStart().x * m_settings.BiuTo3Dunits(),
                                   -aVia->GetStart().y * m_settings.BiuTo3Dunits() );

    CRING2D *ring = new CRING2D( center,
                                 radiusBUI * m_settings.BiuTo3Dunits(),
                                 ( radiusBUI + m_settings.GetCopperThicknessBIU() ) *
                                 m_settings.BiuTo3Dunits(),
                                 *aVia );

    m_containerWithObjectsToDelete.Add( ring );


    CLAYERITEM *objPtr = new CLAYERITEM( ring, topZ, botZ );

    objPtr->SetMaterial( &m_materials.m_Copper );

    if( m_settings.GetFlag( FL_USE_REALISTIC_MODE ) )
        objPtr->SetColor( ConvertSRGBToLinear( (SFVEC3F)m_settings.m_CopperColor ) );
    else
        objPtr->SetColor( ConvertSRGBToLinear( m_settings.GetItemColor( VIAS_VISIBLE + aVia->GetViaType() ) ) );

    m_object_container.Add( objPtr );
}


// Based on draw3DPadHole from
// 3d_draw_helper_functions.cpp
void C3D_RENDER_RAYTRACING::insert3DPadHole( const D_PAD* aPad )
{
    const COBJECT2D *object2d_A = NULL;

    SFVEC3F objColor;

    if( m_settings.GetFlag( FL_USE_REALISTIC_MODE ) )
        objColor = (SFVEC3F)m_settings.m_CopperColor;
    else
        objColor = m_settings.GetItemColor( PADS_VISIBLE );

    const wxSize  drillsize   = aPad->GetDrillSize();
    const bool    hasHole     = drillsize.x && drillsize.y;

    if( !hasHole )
        return;

    const float topZ = m_settings.GetLayerBottomZpos3DU( F_Cu ) +
                       m_settings.GetCopperThickness3DU();

    const float botZ = m_settings.GetLayerBottomZpos3DU( B_Cu ) -
                       m_settings.GetCopperThickness3DU();

    if( drillsize.x == drillsize.y )    // usual round hole
    {
        SFVEC2F center = SFVEC2F( aPad->GetPosition().x * m_settings.BiuTo3Dunits(),
                                 -aPad->GetPosition().y * m_settings.BiuTo3Dunits() );

        CRING2D *ring = new CRING2D( center,
                                     ( drillsize.x / 2 ) * m_settings.BiuTo3Dunits(),
                                     ( ( drillsize.x / 2 ) +
                                       m_settings.GetCopperThicknessBIU() ) *
                                     m_settings.BiuTo3Dunits(),
                                     *aPad );

        m_containerWithObjectsToDelete.Add( ring );

        object2d_A = ring;
    }
    else    // Oblong hole
    {
        wxPoint ends_offset;
        int     width;

        if( drillsize.x > drillsize.y )    // Horizontal oval
        {
            ends_offset.x = ( drillsize.x - drillsize.y ) / 2;
            width = drillsize.y;
        }
        else    // Vertical oval
        {
            ends_offset.y = ( drillsize.y - drillsize.x ) / 2;
            width = drillsize.x;
        }

        RotatePoint( &ends_offset, aPad->GetOrientation() );

        wxPoint start   = aPad->GetPosition() + ends_offset;
        wxPoint end     = aPad->GetPosition() - ends_offset;

        CROUNDSEGMENT2D *innerSeg = new CROUNDSEGMENT2D(
                                    SFVEC2F( start.x * m_settings.BiuTo3Dunits(),
                                            -start.y * m_settings.BiuTo3Dunits() ),
                                    SFVEC2F( end.x   * m_settings.BiuTo3Dunits(),
                                            -end.y * m_settings.BiuTo3Dunits() ),
                                    width * m_settings.BiuTo3Dunits(),
                                    *aPad );

        CROUNDSEGMENT2D *outterSeg = new CROUNDSEGMENT2D(
                                    SFVEC2F( start.x * m_settings.BiuTo3Dunits(),
                                            -start.y * m_settings.BiuTo3Dunits() ),
                                    SFVEC2F( end.x   * m_settings.BiuTo3Dunits(),
                                            -end.y * m_settings.BiuTo3Dunits() ),
                                    ( width +  m_settings.GetCopperThicknessBIU() * 2 ) *
                                    m_settings.BiuTo3Dunits(),
                                    *aPad );

        // NOTE: the round segment width is the "diameter", so we double the thickness

        std::vector<const COBJECT2D *> *object2d_B = new std::vector<const COBJECT2D *>();
        object2d_B->push_back( innerSeg );

        CITEMLAYERCSG2D *itemCSG2d = new CITEMLAYERCSG2D( outterSeg,
                                                          object2d_B,
                                                          CSGITEM_FULL,
                                                          *aPad );

        m_containerWithObjectsToDelete.Add( itemCSG2d );
        m_containerWithObjectsToDelete.Add( innerSeg );
        m_containerWithObjectsToDelete.Add( outterSeg );

        object2d_A = itemCSG2d;
    }


    if( object2d_A )
    {
        std::vector<const COBJECT2D *> *object2d_B = new std::vector<const COBJECT2D *>();

        // Check if there are any other THT that intersects this hole
        // It will use the non inflated holes
        if( !m_settings.GetThroughHole_Inner().GetList().empty() )
        {

            CONST_LIST_OBJECT2D intersectionList;
            m_settings.GetThroughHole_Inner().GetListObjectsIntersects( object2d_A->GetBBox(),
                                                                        intersectionList );

            if( !intersectionList.empty() )
            {
                for( CONST_LIST_OBJECT2D::const_iterator hole = intersectionList.begin();
                     hole != intersectionList.end();
                     ++hole )
                {
                    const COBJECT2D *hole2d = static_cast<const COBJECT2D *>(*hole);

                    if( object2d_A->Intersects( hole2d->GetBBox() ) )
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

        if( object2d_B == CSGITEM_EMPTY )
        {
            CLAYERITEM *objPtr = new CLAYERITEM( object2d_A, topZ, botZ );

            objPtr->SetMaterial( &m_materials.m_Copper );
            objPtr->SetColor( ConvertSRGBToLinear( objColor ) );
            m_object_container.Add( objPtr );
        }
        else
        {
            CITEMLAYERCSG2D *itemCSG2d = new CITEMLAYERCSG2D( object2d_A,
                                                              object2d_B,
                                                              CSGITEM_FULL,
                                                              (const BOARD_ITEM &)*aPad );

            m_containerWithObjectsToDelete.Add( itemCSG2d );

            CLAYERITEM *objPtr = new CLAYERITEM( itemCSG2d, topZ, botZ );

            objPtr->SetMaterial( &m_materials.m_Copper );
            objPtr->SetColor( ConvertSRGBToLinear( objColor ) );

            m_object_container.Add( objPtr );
        }
    }
}


void C3D_RENDER_RAYTRACING::add_3D_vias_and_pads_to_container()
{
    // Insert plated vertical holes inside the board
    // /////////////////////////////////////////////////////////////////////////

    // Insert vias holes (vertical cylinders)
    for( const TRACK* track = m_settings.GetBoard()->m_Track;
         track;
         track = track->Next() )
    {
        if( track->Type() == PCB_VIA_T )
        {
            const VIA *via = static_cast<const VIA*>(track);
            insert3DViaHole( via );
        }
    }

    // Insert pads holes (vertical cylinders)
    for( const MODULE* module = m_settings.GetBoard()->m_Modules;
         module;
         module = module->Next() )
    {
        for( const D_PAD* pad = module->Pads(); pad; pad = pad->Next() )
            if( pad->GetAttribute () != PAD_ATTRIB_HOLE_NOT_PLATED )
            {
                insert3DPadHole( pad );
            }
    }
}


void C3D_RENDER_RAYTRACING::load_3D_models()
{
    // Go for all modules
    for( const MODULE* module = m_settings.GetBoard()->m_Modules;
         module;
         module = module->Next() )
    {
        if( (!module->Models().empty() ) &&
            m_settings.ShouldModuleBeDisplayed( (MODULE_ATTR_T)module->GetAttributes() ) )
        {
            double zpos = m_settings.GetModulesZcoord3DIU( module->IsFlipped() );

            wxPoint pos = module->GetPosition();

            glm::mat4 moduleMatrix = glm::mat4();

            moduleMatrix = glm::translate( moduleMatrix,
                                           SFVEC3F( pos.x * m_settings.BiuTo3Dunits(),
                                                   -pos.y * m_settings.BiuTo3Dunits(),
                                                    zpos ) );

            if( module->GetOrientation() )
            {
                moduleMatrix = glm::rotate( moduleMatrix,
                                            ( (float)(module->GetOrientation() / 10.0f) / 180.0f ) *
                                            glm::pi<float>(),
                                            SFVEC3F( 0.0f, 0.0f, 1.0f ) );
            }


            if( module->IsFlipped() )
            {
                moduleMatrix = glm::rotate( moduleMatrix,
                                            glm::pi<float>(),
                                            SFVEC3F( 0.0f, 1.0f, 0.0f ) );

                moduleMatrix = glm::rotate( moduleMatrix,
                                            glm::pi<float>(),
                                            SFVEC3F( 0.0f, 0.0f, 1.0f ) );
            }

            const double modelunit_to_3d_units_factor = m_settings.BiuTo3Dunits() *
                                                        UNITS3D_TO_UNITSPCB;

            moduleMatrix = glm::scale( moduleMatrix,
                                       SFVEC3F( modelunit_to_3d_units_factor,
                                                modelunit_to_3d_units_factor,
                                                modelunit_to_3d_units_factor ) );


            // Get the list of model files for this model
            std::list<S3D_INFO>::const_iterator sM = module->Models().begin();
            std::list<S3D_INFO>::const_iterator eM = module->Models().end();

            while( sM != eM )
            {
                // get it from cache
                const S3DMODEL *modelPtr =
                        m_settings.Get3DCacheManager()->GetModel( sM->m_Filename );

                // only add it if the return is not NULL
                if( modelPtr )
                {
                    glm::mat4 modelMatrix = moduleMatrix;

                    modelMatrix = glm::translate( modelMatrix,
                                                  SFVEC3F( sM->m_Offset.x * 25.4f,
                                                           sM->m_Offset.y * 25.4f,
                                                           sM->m_Offset.z * 25.4f ) );

                    modelMatrix = glm::rotate( modelMatrix,
                                               (float)-( sM->m_Rotation.z / 180.0f ) *
                                               glm::pi<float>(),
                                               SFVEC3F( 0.0f, 0.0f, 1.0f ) );

                    modelMatrix = glm::rotate( modelMatrix,
                                               (float)-( sM->m_Rotation.y / 180.0f ) *
                                               glm::pi<float>(),
                                               SFVEC3F( 0.0f, 1.0f, 0.0f ) );

                    modelMatrix = glm::rotate( modelMatrix,
                                               (float)-( sM->m_Rotation.x / 180.0f ) *
                                               glm::pi<float>(),
                                               SFVEC3F( 1.0f, 0.0f, 0.0f ) );

                    modelMatrix = glm::scale( modelMatrix,
                                              SFVEC3F( sM->m_Scale.x,
                                                       sM->m_Scale.y,
                                                       sM->m_Scale.z ) );

                    add_3D_models( modelPtr, modelMatrix );
                }

                ++sM;
            }
        }
    }
}


void C3D_RENDER_RAYTRACING::add_3D_models( const S3DMODEL *a3DModel,
                                           const glm::mat4 &aModelMatrix )
{

    // Validate a3DModel pointers
    wxASSERT( a3DModel != NULL );

    if( a3DModel == NULL )
        return;

    wxASSERT( a3DModel->m_Materials != NULL );
    wxASSERT( a3DModel->m_Meshes != NULL );
    wxASSERT( a3DModel->m_MaterialsSize > 0 );
    wxASSERT( a3DModel->m_MeshesSize > 0 );

    if( (a3DModel->m_Materials != NULL) && (a3DModel->m_Meshes != NULL) &&
        (a3DModel->m_MaterialsSize > 0) && (a3DModel->m_MeshesSize > 0) )
    {

        MODEL_MATERIALS *materialVector;

        // Try find if the materials already exists in the map list
        if( m_model_materials.find( a3DModel ) != m_model_materials.end() )
        {
            // Found it, so get the pointer
            materialVector = &m_model_materials[a3DModel];
        }
        else
        {
            // Materials was not found in the map, so it will create a new for
            // this model.

            m_model_materials[a3DModel] = MODEL_MATERIALS();
            materialVector = &m_model_materials[a3DModel];

            materialVector->resize( a3DModel->m_MaterialsSize );

            for( unsigned int imat = 0;
                 imat < a3DModel->m_MaterialsSize;
                 ++imat )
            {
                if( m_settings.MaterialModeGet() == MATERIAL_MODE_NORMAL )
                {
                    const SMATERIAL &material = a3DModel->m_Materials[imat];

                    // http://www.fooplot.com/#W3sidHlwZSI6MCwiZXEiOiJtaW4oc3FydCh4LTAuMzUpKjAuNDAtMC4wNSwxLjApIiwiY29sb3IiOiIjMDAwMDAwIn0seyJ0eXBlIjoxMDAwLCJ3aW5kb3ciOlsiMC4wNzA3NzM2NzMyMzY1OTAxMiIsIjEuNTY5NTcxNjI5MjI1NDY5OCIsIi0wLjI3NDYzNTMyMTc1OTkyOTMiLCIwLjY0NzcwMTg4MTkyNTUzNjIiXSwic2l6ZSI6WzY0NCwzOTRdfV0-

                    float reflectionFactor = 0.0f;

                    if( (material.m_Shininess - 0.35f) > FLT_EPSILON )
                    {
                        reflectionFactor = glm::clamp( glm::sqrt( (material.m_Shininess - 0.35f) ) *
                                                       0.40f - 0.05f,
                                                       0.0f,
                                                       0.5f );
                    }

                    CBLINN_PHONG_MATERIAL &blinnMaterial = (*materialVector)[imat];

                    SFVEC3F ambient;

                    if( m_settings.GetFlag( FL_RENDER_RAYTRACING_POST_PROCESSING ) )
                    {
                        // apply a gain to the (dark) ambient colors

                        // http://www.fooplot.com/#W3sidHlwZSI6MCwiZXEiOiIoKHgrMC4yMCleKDEvMi4wMCkpLTAuMzUiLCJjb2xvciI6IiMwMDAwMDAifSx7InR5cGUiOjAsImVxIjoieCIsImNvbG9yIjoiIzAwMDAwMCJ9LHsidHlwZSI6MTAwMCwid2luZG93IjpbIi0xLjI0OTUwNTMzOTIyMzYyIiwiMS42Nzc4MzQ0MTg1NjcxODQzIiwiLTAuNDM1NTA0NjQyODEwOTMwMjYiLCIxLjM2NTkzNTIwODEzNzI1OCJdLCJzaXplIjpbNjQ5LDM5OV19XQ--
                        // ambient = glm::max( (glm::pow((material.m_Ambient + 0.20f), SFVEC3F(1.0f / 2.00f)) - SFVEC3F(0.35f)), material.m_Ambient );

                        // http://www.fooplot.com/#W3sidHlwZSI6MCwiZXEiOiIoKHgrMC4yMCleKDEvMS41OCkpLTAuMzUiLCJjb2xvciI6IiMwMDAwMDAifSx7InR5cGUiOjAsImVxIjoieCIsImNvbG9yIjoiIzAwMDAwMCJ9LHsidHlwZSI6MTAwMCwid2luZG93IjpbIi0xLjI0OTUwNTMzOTIyMzYyIiwiMS42Nzc4MzQ0MTg1NjcxODQzIiwiLTAuNDM1NTA0NjQyODEwOTMwMjYiLCIxLjM2NTkzNTIwODEzNzI1OCJdLCJzaXplIjpbNjQ5LDM5OV19XQ--
                        //ambient = glm::max( (glm::pow((material.m_Ambient + 0.20f), SFVEC3F(1.0f / 1.58f)) - SFVEC3F(0.35f)), material.m_Ambient );

                        // http://www.fooplot.com/#W3sidHlwZSI6MCwiZXEiOiIoKHgrMC4yMCleKDEvMS41NCkpLTAuMzQiLCJjb2xvciI6IiMwMDAwMDAifSx7InR5cGUiOjAsImVxIjoieCIsImNvbG9yIjoiIzAwMDAwMCJ9LHsidHlwZSI6MTAwMCwid2luZG93IjpbIi0yLjcyMTA5NTg0MjA1MDYwNSIsIjEuODUyODcyNTI5NDk3NTIyMyIsIi0xLjQyMTM3NjAxOTkyOTA4MDYiLCIxLjM5MzM3Mzc0NzE3NzQ2MTIiXSwic2l6ZSI6WzY0OSwzOTldfV0-
                        ambient = ConvertSRGBToLinear(
                                glm::pow((material.m_Ambient + 0.30f), SFVEC3F(1.0f / 1.54f)) - SFVEC3F(0.34f) );
                    }
                    else
                    {
                        ambient = ConvertSRGBToLinear( material.m_Ambient );
                    }


                    blinnMaterial = CBLINN_PHONG_MATERIAL(
                                              ambient,
                                              ConvertSRGBToLinear( material.m_Emissive ),
                                              ConvertSRGBToLinear( material.m_Specular ),
                                              material.m_Shininess * 180.0f,
                                              material.m_Transparency,
                                              reflectionFactor );

                    if( m_settings.GetFlag( FL_RENDER_RAYTRACING_PROCEDURAL_TEXTURES ) )
                    {
                        // Guess material type and apply a normal perturbator

                        if( ( RGBtoGray(material.m_Diffuse) < 0.3f ) &&
                            ( material.m_Shininess < 0.36f ) &&
                            ( material.m_Transparency == 0.0f ) &&
                            ( (glm::abs( material.m_Diffuse.r - material.m_Diffuse.g ) < 0.15f) &&
                              (glm::abs( material.m_Diffuse.b - material.m_Diffuse.g ) < 0.15f) &&
                              (glm::abs( material.m_Diffuse.r - material.m_Diffuse.b ) < 0.15f) ) )
                        {
                            // This may be a black plastic..

                            if( material.m_Shininess < 0.26f )
                                blinnMaterial.SetNormalPerturbator( &m_plastic_normal_perturbator );
                            else
                                blinnMaterial.SetNormalPerturbator( &m_plastic_shine_normal_perturbator );
                        }
                        else
                        {
                            if( ( RGBtoGray(material.m_Diffuse) > 0.3f ) &&
                                ( material.m_Shininess < 0.30f ) &&
                                ( material.m_Transparency == 0.0f ) &&
                                ( (glm::abs( material.m_Diffuse.r - material.m_Diffuse.g ) > 0.25f) ||
                                  (glm::abs( material.m_Diffuse.b - material.m_Diffuse.g ) > 0.25f) ||
                                  (glm::abs( material.m_Diffuse.r - material.m_Diffuse.b ) > 0.25f) ) )
                            {
                                // This may be a color plastic ...
                                blinnMaterial.SetNormalPerturbator( &m_plastic_shine_normal_perturbator );
                            }
                            else
                            {
                                if( ( RGBtoGray(material.m_Diffuse) > 0.6f ) &&
                                    ( material.m_Shininess > 0.35f ) &&
                                    ( material.m_Transparency == 0.0f ) &&
                                    ( (glm::abs( material.m_Diffuse.r - material.m_Diffuse.g ) < 0.40f) &&
                                      (glm::abs( material.m_Diffuse.b - material.m_Diffuse.g ) < 0.40f) &&
                                      (glm::abs( material.m_Diffuse.r - material.m_Diffuse.b ) < 0.40f) ) )
                                {
                                    // This may be a brushed metal
                                    blinnMaterial.SetNormalPerturbator( &m_brushed_metal_normal_perturbator );
                                }
                            }
                        }
                    }
                }
                else
                {
                    (*materialVector)[imat] = CBLINN_PHONG_MATERIAL( SFVEC3F( 0.2f ),
                                                                     SFVEC3F( 0.0f ),
                                                                     SFVEC3F( 0.0f ),
                                                                     0.0f,
                                                                     0.0f,
                                                                     0.0f );
                }
            }
        }

        const glm::mat3 normalMatrix = glm::transpose( glm::inverse( glm::mat3( aModelMatrix ) ) );

        for( unsigned int mesh_i = 0;
             mesh_i < a3DModel->m_MeshesSize;
             ++mesh_i )
        {
            const SMESH &mesh = a3DModel->m_Meshes[mesh_i];

            // Validate the mesh pointers
            wxASSERT( mesh.m_Positions != NULL );
            wxASSERT( mesh.m_FaceIdx != NULL );
            wxASSERT( mesh.m_Normals != NULL );
            wxASSERT( mesh.m_FaceIdxSize > 0 );
            wxASSERT( (mesh.m_FaceIdxSize % 3) == 0 );


            if( (mesh.m_Positions != NULL) &&
                (mesh.m_Normals != NULL) &&
                (mesh.m_FaceIdx != NULL) &&
                (mesh.m_FaceIdxSize > 0) &&
                (mesh.m_VertexSize > 0) &&
                ((mesh.m_FaceIdxSize % 3) == 0) &&
                (mesh.m_MaterialIdx < a3DModel->m_MaterialsSize) )
            {
                const CBLINN_PHONG_MATERIAL &blinn_material = (*materialVector)[mesh.m_MaterialIdx];

                // Add all face triangles
                for( unsigned int faceIdx = 0;
                     faceIdx < mesh.m_FaceIdxSize;
                     faceIdx += 3 )
                {
                    const unsigned int idx0 = mesh.m_FaceIdx[faceIdx + 0];
                    const unsigned int idx1 = mesh.m_FaceIdx[faceIdx + 1];
                    const unsigned int idx2 = mesh.m_FaceIdx[faceIdx + 2];

                    wxASSERT( idx0 < mesh.m_VertexSize );
                    wxASSERT( idx1 < mesh.m_VertexSize );
                    wxASSERT( idx2 < mesh.m_VertexSize );

                    if( ( idx0 < mesh.m_VertexSize ) &&
                        ( idx1 < mesh.m_VertexSize ) &&
                        ( idx2 < mesh.m_VertexSize ) )
                    {
                        const SFVEC3F &v0 = mesh.m_Positions[idx0];
                        const SFVEC3F &v1 = mesh.m_Positions[idx1];
                        const SFVEC3F &v2 = mesh.m_Positions[idx2];

                        const SFVEC3F &n0 = mesh.m_Normals[idx0];
                        const SFVEC3F &n1 = mesh.m_Normals[idx1];
                        const SFVEC3F &n2 = mesh.m_Normals[idx2];

                        // Transform vertex with the model matrix
                        const SFVEC3F vt0 = SFVEC3F( aModelMatrix * glm::vec4( v0, 1.0f) );
                        const SFVEC3F vt1 = SFVEC3F( aModelMatrix * glm::vec4( v1, 1.0f) );
                        const SFVEC3F vt2 = SFVEC3F( aModelMatrix * glm::vec4( v2, 1.0f) );

                        const SFVEC3F nt0 = glm::normalize( SFVEC3F( normalMatrix * n0 ) );
                        const SFVEC3F nt1 = glm::normalize( SFVEC3F( normalMatrix * n1 ) );
                        const SFVEC3F nt2 = glm::normalize( SFVEC3F( normalMatrix * n2 ) );

                        CTRIANGLE *newTriangle = new  CTRIANGLE( vt0, vt2, vt1,
                                                                 nt0, nt2, nt1 );



                        m_object_container.Add( newTriangle );
                        newTriangle->SetMaterial( (const CMATERIAL *)&blinn_material );

                        if( mesh.m_Color == NULL )
                        {
                            const SFVEC3F diffuseColor =
                                a3DModel->m_Materials[mesh.m_MaterialIdx].m_Diffuse;

                            if( m_settings.MaterialModeGet() == MATERIAL_MODE_CAD_MODE )
                                newTriangle->SetColor( ConvertSRGBToLinear( MaterialDiffuseToColorCAD( diffuseColor ) ) );
                            else
                                newTriangle->SetColor( ConvertSRGBToLinear( diffuseColor ) );
                        }
                        else
                        {
                            if( m_settings.MaterialModeGet() == MATERIAL_MODE_CAD_MODE )
                                newTriangle->SetColor( ConvertSRGBToLinear( MaterialDiffuseToColorCAD( mesh.m_Color[idx0] ) ),
                                                       ConvertSRGBToLinear( MaterialDiffuseToColorCAD( mesh.m_Color[idx1] ) ),
                                                       ConvertSRGBToLinear( MaterialDiffuseToColorCAD( mesh.m_Color[idx2] ) ) );
                            else
                                newTriangle->SetColor( ConvertSRGBToLinear( mesh.m_Color[idx0] ),
                                                       ConvertSRGBToLinear( mesh.m_Color[idx1] ),
                                                       ConvertSRGBToLinear( mesh.m_Color[idx2] ) );
                        }
                    }
                }
            }
        }
    }
}
