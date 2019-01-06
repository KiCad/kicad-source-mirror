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
 * @file  create_layer_items.cpp
 * @brief This file implements the creation of the pcb board.
 * It is based on the function found in the files:
 *  board_items_to_polygon_shape_transform.cpp
 *  board_items_to_polygon_shape_transform.cpp
 */

#include "cinfo3d_visu.h"
#include "../3d_rendering/3d_render_raytracing/shapes2D/cring2d.h"
#include "../3d_rendering/3d_render_raytracing/shapes2D/cfilledcircle2d.h"
#include "../3d_rendering/3d_render_raytracing/shapes2D/croundsegment2d.h"
#include "../3d_rendering/3d_render_raytracing/shapes2D/cpolygon4pts2d.h"
#include "../3d_rendering/3d_render_raytracing/shapes2D/cpolygon2d.h"
#include "../3d_rendering/3d_render_raytracing/shapes2D/ctriangle2d.h"
#include "../3d_rendering/3d_render_raytracing/accelerators/ccontainer2d.h"
#include "../3d_rendering/3d_render_raytracing/shapes3D/ccylinder.h"
#include "../3d_rendering/3d_render_raytracing/shapes3D/clayeritem.h"

#include <class_board.h>
#include <class_module.h>
#include <class_pad.h>
#include <class_pcb_text.h>
#include <class_edge_mod.h>
#include <class_zone.h>
#include <class_text_mod.h>
#include <convert_basic_shapes_to_polygon.h>
#include <trigo.h>
#include <utility>
#include <vector>
#include <thread>
#include <algorithm>
#include <atomic>

#include <profile.h>

void CINFO3D_VISU::destroyLayers()
{
    if( !m_layers_poly.empty() )
    {
        for( MAP_POLY::iterator ii = m_layers_poly.begin();
             ii != m_layers_poly.end();
             ++ii )
        {
            delete ii->second;
            ii->second = NULL;
        }

        m_layers_poly.clear();
    }

    if( !m_layers_inner_holes_poly.empty() )
    {
        for( MAP_POLY::iterator ii = m_layers_inner_holes_poly.begin();
             ii != m_layers_inner_holes_poly.end();
             ++ii )
        {
            delete ii->second;
            ii->second = NULL;
        }

        m_layers_inner_holes_poly.clear();
    }

    if( !m_layers_outer_holes_poly.empty() )
    {
        for( MAP_POLY::iterator ii = m_layers_outer_holes_poly.begin();
             ii != m_layers_outer_holes_poly.end();
             ++ii )
        {
            delete ii->second;
            ii->second = NULL;
        }

        m_layers_outer_holes_poly.clear();
    }

    if( !m_layers_container2D.empty() )
    {
        for( MAP_CONTAINER_2D::iterator ii = m_layers_container2D.begin();
             ii != m_layers_container2D.end();
             ++ii )
        {
            delete ii->second;
            ii->second = NULL;
        }

        m_layers_container2D.clear();
    }

    if( !m_layers_holes2D.empty() )
    {
        for( MAP_CONTAINER_2D::iterator ii = m_layers_holes2D.begin();
             ii != m_layers_holes2D.end();
             ++ii )
        {
            delete ii->second;
            ii->second = NULL;
        }

        m_layers_holes2D.clear();
    }

    m_through_holes_inner.Clear();
    m_through_holes_outer.Clear();
    m_through_holes_vias_outer.Clear();
    m_through_holes_vias_inner.Clear();
    m_through_outer_holes_poly_NPTH.RemoveAllContours();
    m_through_outer_holes_poly.RemoveAllContours();
    //m_through_inner_holes_poly.RemoveAllContours();

    m_through_outer_holes_vias_poly.RemoveAllContours();
    m_through_inner_holes_vias_poly.RemoveAllContours();
}


void CINFO3D_VISU::createLayers( REPORTER *aStatusTextReporter )
{
    // Number of segments to draw a circle using segments (used on countour zones
    // and text copper elements )
    const int    segcountforcircle = 12;
    const double correctionFactor  = GetCircleCorrectionFactor( segcountforcircle );

    // segments to draw a circle to build texts. Is is used only to build
    // the shape of each segment of the stroke font, therefore no need to have
    // many segments per circle.
    const int segcountInStrokeFont  = 12;
    const double correctionFactorStroke = GetCircleCorrectionFactor( segcountInStrokeFont );

    destroyLayers();

    // Build Copper layers
    // Based on: https://github.com/KiCad/kicad-source-mirror/blob/master/3d-viewer/3d_draw.cpp#L692
    // /////////////////////////////////////////////////////////////////////////

    #ifdef PRINT_STATISTICS_3D_VIEWER
    unsigned stats_startCopperLayersTime = GetRunningMicroSecs();

    unsigned start_Time = stats_startCopperLayersTime;
#endif

    PCB_LAYER_ID cu_seq[MAX_CU_LAYERS];
    LSET     cu_set = LSET::AllCuMask( m_copperLayersCount );

    m_stats_nr_tracks               = 0;
    m_stats_track_med_width         = 0;
    m_stats_nr_vias                 = 0;
    m_stats_via_med_hole_diameter   = 0;
    m_stats_nr_holes                = 0;
    m_stats_hole_med_diameter       = 0;

    // Prepare track list, convert in a vector. Calc statistic for the holes
    // /////////////////////////////////////////////////////////////////////////
    std::vector< const TRACK *> trackList;
    trackList.clear();
    trackList.reserve( m_board->m_Track.GetCount() );

    for( const TRACK* track = m_board->m_Track; track; track = track->Next() )
    {
        if( !Is3DLayerEnabled( track->GetLayer() ) ) // Skip non enabled layers
            continue;

        // Note: a TRACK holds normal segment tracks and
        // also vias circles (that have also drill values)
        trackList.push_back( track );

        if( track->Type() == PCB_VIA_T )
        {
            const VIA *via = static_cast< const VIA*>( track );
            m_stats_nr_vias++;
            m_stats_via_med_hole_diameter += via->GetDrillValue() * m_biuTo3Dunits;
        }
        else
        {
            m_stats_nr_tracks++;
        }

        m_stats_track_med_width += track->GetWidth() * m_biuTo3Dunits;
    }

    if( m_stats_nr_tracks )
        m_stats_track_med_width /= (float)m_stats_nr_tracks;

    if( m_stats_nr_vias )
        m_stats_via_med_hole_diameter /= (float)m_stats_nr_vias;

#ifdef PRINT_STATISTICS_3D_VIEWER
    printf( "T01: %.3f ms\n", (float)( GetRunningMicroSecs()  - start_Time  ) / 1e3 );
    start_Time = GetRunningMicroSecs();
#endif

    // Prepare copper layers index and containers
    // /////////////////////////////////////////////////////////////////////////
    std::vector< PCB_LAYER_ID > layer_id;
    layer_id.clear();
    layer_id.reserve( m_copperLayersCount );

    for( unsigned i = 0; i < arrayDim( cu_seq ); ++i )
        cu_seq[i] = ToLAYER_ID( B_Cu - i );

    for( LSEQ cu = cu_set.Seq( cu_seq, arrayDim( cu_seq ) ); cu; ++cu )
    {
        const PCB_LAYER_ID curr_layer_id = *cu;

        if( !Is3DLayerEnabled( curr_layer_id ) ) // Skip non enabled layers
            continue;

        layer_id.push_back( curr_layer_id );

        CBVHCONTAINER2D *layerContainer = new CBVHCONTAINER2D;
        m_layers_container2D[curr_layer_id] = layerContainer;

        if( GetFlag( FL_RENDER_OPENGL_COPPER_THICKNESS ) &&
            (m_render_engine == RENDER_ENGINE_OPENGL_LEGACY) )
        {
            SHAPE_POLY_SET *layerPoly = new SHAPE_POLY_SET;
            m_layers_poly[curr_layer_id] = layerPoly;
        }
    }

#ifdef PRINT_STATISTICS_3D_VIEWER
    printf( "T02: %.3f ms\n", (float)( GetRunningMicroSecs() - start_Time ) / 1e3 );
    start_Time = GetRunningMicroSecs();
#endif

    if( aStatusTextReporter )
        aStatusTextReporter->Report( _( "Create tracks and vias" ) );

    // Create tracks as objects and add it to container
    // /////////////////////////////////////////////////////////////////////////
    for( unsigned int lIdx = 0; lIdx < layer_id.size(); ++lIdx )
    {
        const PCB_LAYER_ID curr_layer_id = layer_id[lIdx];

        wxASSERT( m_layers_container2D.find( curr_layer_id ) != m_layers_container2D.end() );

        CBVHCONTAINER2D *layerContainer = m_layers_container2D[curr_layer_id];

        // ADD TRACKS
        unsigned int nTracks = trackList.size();

        for( unsigned int trackIdx = 0; trackIdx < nTracks; ++trackIdx )
        {
            const TRACK *track = trackList[trackIdx];

            // NOTE: Vias can be on multiple layers
            if( !track->IsOnLayer( curr_layer_id ) )
                continue;

            // Add object item to layer container
            layerContainer->Add( createNewTrack( track, 0.0f ) );
        }
    }

#ifdef PRINT_STATISTICS_3D_VIEWER
    printf( "T03: %.3f ms\n", (float)( GetRunningMicroSecs() - start_Time  ) / 1e3 );
    start_Time = GetRunningMicroSecs();
#endif

    // Create VIAS and THTs objects and add it to holes containers
    // /////////////////////////////////////////////////////////////////////////
    for( unsigned int lIdx = 0; lIdx < layer_id.size(); ++lIdx )
    {
        const PCB_LAYER_ID curr_layer_id = layer_id[lIdx];

        // ADD TRACKS
        unsigned int nTracks = trackList.size();

        for( unsigned int trackIdx = 0; trackIdx < nTracks; ++trackIdx )
        {
            const TRACK *track = trackList[trackIdx];

            if( !track->IsOnLayer( curr_layer_id ) )
                continue;

            // ADD VIAS and THT
            if( track->Type() == PCB_VIA_T )
            {
                const VIA *via = static_cast< const VIA*>( track );
                const VIATYPE_T viatype = via->GetViaType();
                const float holediameter = via->GetDrillValue() * BiuTo3Dunits();
                const float thickness = GetCopperThickness3DU();
                const float hole_inner_radius = ( holediameter / 2.0f );

                const SFVEC2F via_center(  via->GetStart().x * m_biuTo3Dunits,
                                          -via->GetStart().y * m_biuTo3Dunits );

                if( viatype != VIA_THROUGH )
                {

                    // Add hole objects
                    // /////////////////////////////////////////////////////////

                    CBVHCONTAINER2D *layerHoleContainer = NULL;

                    // Check if the layer is already created
                    if( m_layers_holes2D.find( curr_layer_id ) == m_layers_holes2D.end() )
                    {
                        // not found, create a new container
                        layerHoleContainer = new CBVHCONTAINER2D;
                        m_layers_holes2D[curr_layer_id] = layerHoleContainer;
                    }
                    else
                    {
                        // found
                        layerHoleContainer = m_layers_holes2D[curr_layer_id];
                    }

                    // Add a hole for this layer
                    layerHoleContainer->Add( new CFILLEDCIRCLE2D( via_center,
                                                                  hole_inner_radius + thickness,
                                                                  *track ) );
                }
                else if( lIdx == 0 ) // it only adds once the THT holes
                {
                    // Add through hole object
                    // /////////////////////////////////////////////////////////
                    m_through_holes_outer.Add( new CFILLEDCIRCLE2D( via_center,
                                                                    hole_inner_radius + thickness,
                                                                    *track ) );

                    m_through_holes_vias_outer.Add(
                                new CFILLEDCIRCLE2D( via_center,
                                                     hole_inner_radius + thickness,
                                                     *track ) );

                    m_through_holes_inner.Add( new CFILLEDCIRCLE2D( via_center,
                                                                    hole_inner_radius,
                                                                    *track ) );

                    //m_through_holes_vias_inner.Add( new CFILLEDCIRCLE2D( via_center,
                    //                                                     hole_inner_radius,
                    //                                                     *track ) );
                }
            }
        }
    }

#ifdef PRINT_STATISTICS_3D_VIEWER
    printf( "T04: %.3f ms\n", (float)( GetRunningMicroSecs() - start_Time  ) / 1e3 );
    start_Time = GetRunningMicroSecs();
#endif

    // Create VIAS and THTs objects and add it to holes containers
    // /////////////////////////////////////////////////////////////////////////
    for( unsigned int lIdx = 0; lIdx < layer_id.size(); ++lIdx )
    {
        const PCB_LAYER_ID curr_layer_id = layer_id[lIdx];

        // ADD TRACKS
        const unsigned int nTracks = trackList.size();

        for( unsigned int trackIdx = 0; trackIdx < nTracks; ++trackIdx )
        {
            const TRACK *track = trackList[trackIdx];

            if( !track->IsOnLayer( curr_layer_id ) )
                continue;

            // ADD VIAS and THT
            if( track->Type() == PCB_VIA_T )
            {
                const VIA *via = static_cast< const VIA*>( track );
                const VIATYPE_T viatype = via->GetViaType();

                if( viatype != VIA_THROUGH )
                {

                    // Add VIA hole contourns
                    // /////////////////////////////////////////////////////////

                    // Add outter holes of VIAs
                    SHAPE_POLY_SET *layerOuterHolesPoly = NULL;
                    SHAPE_POLY_SET *layerInnerHolesPoly = NULL;

                    // Check if the layer is already created
                    if( m_layers_outer_holes_poly.find( curr_layer_id ) ==
                        m_layers_outer_holes_poly.end() )
                    {
                        // not found, create a new container
                        layerOuterHolesPoly = new SHAPE_POLY_SET;
                        m_layers_outer_holes_poly[curr_layer_id] = layerOuterHolesPoly;

                        wxASSERT( m_layers_inner_holes_poly.find( curr_layer_id ) ==
                                  m_layers_inner_holes_poly.end() );

                        layerInnerHolesPoly = new SHAPE_POLY_SET;
                        m_layers_inner_holes_poly[curr_layer_id] = layerInnerHolesPoly;
                    }
                    else
                    {
                        // found
                        layerOuterHolesPoly = m_layers_outer_holes_poly[curr_layer_id];

                        wxASSERT( m_layers_inner_holes_poly.find( curr_layer_id ) !=
                                  m_layers_inner_holes_poly.end() );

                        layerInnerHolesPoly = m_layers_inner_holes_poly[curr_layer_id];
                    }

                    const int holediameter = via->GetDrillValue();
                    const int hole_outer_radius = (holediameter / 2) + GetCopperThicknessBIU();

                    TransformCircleToPolygon( *layerOuterHolesPoly,
                                              via->GetStart(),
                                              hole_outer_radius,
                                              GetNrSegmentsCircle( hole_outer_radius * 2 ) );

                    TransformCircleToPolygon( *layerInnerHolesPoly,
                                              via->GetStart(),
                                              holediameter / 2,
                                              GetNrSegmentsCircle( holediameter ) );
                }
                else if( lIdx == 0 ) // it only adds once the THT holes
                {
                    const int holediameter = via->GetDrillValue();
                    const int hole_outer_radius = (holediameter / 2)+ GetCopperThicknessBIU();

                    // Add through hole contourns
                    // /////////////////////////////////////////////////////////
                    TransformCircleToPolygon( m_through_outer_holes_poly,
                                              via->GetStart(),
                                              hole_outer_radius,
                                              GetNrSegmentsCircle( hole_outer_radius * 2 ) );

                    TransformCircleToPolygon( m_through_inner_holes_poly,
                                              via->GetStart(),
                                              holediameter / 2,
                                              GetNrSegmentsCircle( holediameter ) );

                    // Add samething for vias only

                    TransformCircleToPolygon( m_through_outer_holes_vias_poly,
                                              via->GetStart(),
                                              hole_outer_radius,
                                              GetNrSegmentsCircle( hole_outer_radius * 2 ) );

                    //TransformCircleToPolygon( m_through_inner_holes_vias_poly,
                    //                          via->GetStart(),
                    //                          holediameter / 2,
                    //                          GetNrSegmentsCircle( holediameter ) );
                }
            }
        }
    }

#ifdef PRINT_STATISTICS_3D_VIEWER
    printf( "T05: %.3f ms\n", (float)( GetRunningMicroSecs() - start_Time  ) / 1e3 );
    start_Time = GetRunningMicroSecs();
#endif

    // Creates outline contours of the tracks and add it to the poly of the layer
    // /////////////////////////////////////////////////////////////////////////
    if( GetFlag( FL_RENDER_OPENGL_COPPER_THICKNESS ) &&
        (m_render_engine == RENDER_ENGINE_OPENGL_LEGACY) )
    {
        for( unsigned int lIdx = 0; lIdx < layer_id.size(); ++lIdx )
        {
            const PCB_LAYER_ID curr_layer_id = layer_id[lIdx];

            wxASSERT( m_layers_poly.find( curr_layer_id ) != m_layers_poly.end() );

            SHAPE_POLY_SET *layerPoly = m_layers_poly[curr_layer_id];

            // ADD TRACKS
            unsigned int nTracks = trackList.size();

            for( unsigned int trackIdx = 0; trackIdx < nTracks; ++trackIdx )
            {
                const TRACK *track = trackList[trackIdx];

                if( !track->IsOnLayer( curr_layer_id ) )
                    continue;

                // Add the track contour
                int nrSegments = GetNrSegmentsCircle( track->GetWidth() );

                track->TransformShapeWithClearanceToPolygon(
                            *layerPoly,
                            0,
                            nrSegments,
                            GetCircleCorrectionFactor( nrSegments ) );
            }
        }
    }

#ifdef PRINT_STATISTICS_3D_VIEWER
    printf( "T06: %.3f ms\n", (float)( GetRunningMicroSecs()  - start_Time  ) / 1e3 );
    start_Time = GetRunningMicroSecs();
#endif

    // Add holes of modules
    // /////////////////////////////////////////////////////////////////////////
    for( const MODULE* module = m_board->m_Modules; module; module = module->Next() )
    {
        const D_PAD* pad = module->PadsList();

        for( ; pad; pad = pad->Next() )
        {
            const wxSize padHole = pad->GetDrillSize();

            if( !padHole.x )    // Not drilled pad like SMD pad
                continue;

            // The hole in the body is inflated by copper thickness,
            // if not plated, no copper
            const int inflate = (pad->GetAttribute () != PAD_ATTRIB_HOLE_NOT_PLATED) ?
                                GetCopperThicknessBIU() : 0;

            m_stats_nr_holes++;
            m_stats_hole_med_diameter += ( ( pad->GetDrillSize().x +
                                             pad->GetDrillSize().y ) / 2.0f ) * m_biuTo3Dunits;

            m_through_holes_outer.Add( createNewPadDrill( pad, inflate ) );
            m_through_holes_inner.Add( createNewPadDrill( pad,       0 ) );
        }
    }
    if( m_stats_nr_holes )
        m_stats_hole_med_diameter /= (float)m_stats_nr_holes;

#ifdef PRINT_STATISTICS_3D_VIEWER
    printf( "T07: %.3f ms\n", (float)( GetRunningMicroSecs() - start_Time  ) / 1e3 );
    start_Time = GetRunningMicroSecs();
#endif

    // Add contours of the pad holes (pads can be Circle or Segment holes)
    // /////////////////////////////////////////////////////////////////////////
    for( const MODULE* module = m_board->m_Modules; module; module = module->Next() )
    {
        const D_PAD* pad = module->PadsList();

        for( ; pad; pad = pad->Next() )
        {
            const wxSize padHole = pad->GetDrillSize();

            if( !padHole.x ) // Not drilled pad like SMD pad
                continue;

            // The hole in the body is inflated by copper thickness.
            const int inflate = GetCopperThicknessBIU();

            // we use the hole diameter to calculate the seg count.
            // for round holes, padHole.x == padHole.y
            // for oblong holes, the diameter is the smaller of (padHole.x, padHole.y)
            const int diam = std::min( padHole.x, padHole.y );


            if( pad->GetAttribute () != PAD_ATTRIB_HOLE_NOT_PLATED )
            {
                pad->BuildPadDrillShapePolygon( m_through_outer_holes_poly,
                                                inflate,
                                                GetNrSegmentsCircle( diam ) );

                pad->BuildPadDrillShapePolygon( m_through_inner_holes_poly,
                                                0,
                                                GetNrSegmentsCircle( diam ) );
            }
            else
            {
                // If not plated, no copper.
                pad->BuildPadDrillShapePolygon( m_through_outer_holes_poly_NPTH,
                                                inflate,
                                                GetNrSegmentsCircle( diam ) );
            }
        }
    }

#ifdef PRINT_STATISTICS_3D_VIEWER
    printf( "T08: %.3f ms\n", (float)( GetRunningMicroSecs()  - start_Time  ) / 1e3 );
    start_Time = GetRunningMicroSecs();
#endif

    // Add modules PADs objects to containers
    // /////////////////////////////////////////////////////////////////////////
    for( unsigned int lIdx = 0; lIdx < layer_id.size(); ++lIdx )
    {
        const PCB_LAYER_ID curr_layer_id = layer_id[lIdx];

        wxASSERT( m_layers_container2D.find( curr_layer_id ) != m_layers_container2D.end() );

        CBVHCONTAINER2D *layerContainer = m_layers_container2D[curr_layer_id];

        // ADD PADS
        for( const MODULE* module = m_board->m_Modules; module; module = module->Next() )
        {
            // Note: NPTH pads are not drawn on copper layers when the pad
            // has same shape as its hole
            AddPadsShapesWithClearanceToContainer( module,
                                                   layerContainer,
                                                   curr_layer_id,
                                                   0,
                                                   true );

            // Micro-wave modules may have items on copper layers
            AddGraphicsShapesWithClearanceToContainer( module,
                                                       layerContainer,
                                                       curr_layer_id,
                                                       0 );
        }
    }

#ifdef PRINT_STATISTICS_3D_VIEWER
    printf( "T09: %.3f ms\n", (float)( GetRunningMicroSecs()  - start_Time  ) / 1e3 );
    start_Time = GetRunningMicroSecs();
#endif

    // Add modules PADs poly contourns
    // /////////////////////////////////////////////////////////////////////////
    if( GetFlag( FL_RENDER_OPENGL_COPPER_THICKNESS ) &&
        (m_render_engine == RENDER_ENGINE_OPENGL_LEGACY) )
    {
        for( unsigned int lIdx = 0; lIdx < layer_id.size(); ++lIdx )
        {
            const PCB_LAYER_ID curr_layer_id = layer_id[lIdx];

            wxASSERT( m_layers_poly.find( curr_layer_id ) != m_layers_poly.end() );

            SHAPE_POLY_SET *layerPoly = m_layers_poly[curr_layer_id];

            // ADD PADS
            for( const MODULE* module = m_board->m_Modules; module; module = module->Next() )
            {
                // Construct polys
                // /////////////////////////////////////////////////////////////

                // Note: NPTH pads are not drawn on copper layers when the pad
                // has same shape as its hole
                transformPadsShapesWithClearanceToPolygon( module->PadsList(),
                                                           curr_layer_id,
                                                           *layerPoly,
                                                           0,
                                                           true );

                // Micro-wave modules may have items on copper layers
                module->TransformGraphicTextWithClearanceToPolygonSet( curr_layer_id,
                                                                        *layerPoly,
                                                                        0,
                                                                        segcountforcircle,
                                                                        correctionFactor );

                transformGraphicModuleEdgeToPolygonSet( module, curr_layer_id, *layerPoly );
            }
        }
    }

#ifdef PRINT_STATISTICS_3D_VIEWER
    printf( "T10: %.3f ms\n", (float)( GetRunningMicroSecs() - start_Time  ) / 1e3 );
    start_Time = GetRunningMicroSecs();
#endif

    // Add graphic item on copper layers to object containers
    // /////////////////////////////////////////////////////////////////////////
    for( unsigned int lIdx = 0; lIdx < layer_id.size(); ++lIdx )
    {
        const PCB_LAYER_ID curr_layer_id = layer_id[lIdx];

        wxASSERT( m_layers_container2D.find( curr_layer_id ) != m_layers_container2D.end() );

        CBVHCONTAINER2D *layerContainer = m_layers_container2D[curr_layer_id];

        // ADD GRAPHIC ITEMS ON COPPER LAYERS (texts)
        for( auto item : m_board->Drawings() )
        {
            if( !item->IsOnLayer( curr_layer_id ) )
                continue;

            switch( item->Type() )
            {
            case PCB_LINE_T:  // should not exist on copper layers
            {
                AddShapeWithClearanceToContainer( (DRAWSEGMENT*)item,
                                                  layerContainer,
                                                  curr_layer_id,
                                                  0 );
            }
            break;

            case PCB_TEXT_T:
                AddShapeWithClearanceToContainer( (TEXTE_PCB*) item,
                                                  layerContainer,
                                                  curr_layer_id,
                                                  0 );
            break;

            case PCB_DIMENSION_T:
                AddShapeWithClearanceToContainer( (DIMENSION*) item,
                                                  layerContainer,
                                                  curr_layer_id,
                                                  0 );
            break;

            default:
                wxLogTrace( m_logTrace,
                            wxT( "createLayers: item type: %d not implemented" ),
                            item->Type() );
            break;
            }
        }
    }

#ifdef PRINT_STATISTICS_3D_VIEWER
    printf( "T11: %.3f ms\n", (float)( GetRunningMicroSecs() - start_Time  ) / 1e3 );
    start_Time = GetRunningMicroSecs();
#endif

    // Add graphic item on copper layers to poly contourns
    // /////////////////////////////////////////////////////////////////////////
    if( GetFlag( FL_RENDER_OPENGL_COPPER_THICKNESS ) &&
        (m_render_engine == RENDER_ENGINE_OPENGL_LEGACY) )
    {
        for( unsigned int lIdx = 0; lIdx < layer_id.size(); ++lIdx )
        {
            const PCB_LAYER_ID curr_layer_id = layer_id[lIdx];

            wxASSERT( m_layers_poly.find( curr_layer_id ) != m_layers_poly.end() );

            SHAPE_POLY_SET *layerPoly = m_layers_poly[curr_layer_id];

            // ADD GRAPHIC ITEMS ON COPPER LAYERS (texts)
            for( auto item : m_board->Drawings() )
            {
                if( !item->IsOnLayer( curr_layer_id ) )
                    continue;

                switch( item->Type() )
                {
                case PCB_LINE_T:
                {
                    const int nrSegments =
                            GetNrSegmentsCircle( item->GetBoundingBox().GetSizeMax() );

                    ( (DRAWSEGMENT*) item )->TransformShapeWithClearanceToPolygon(
                                *layerPoly,
                                0,
                                nrSegments,
                                GetCircleCorrectionFactor( nrSegments ) );
                }
                break;

                case PCB_TEXT_T:
                    ( (TEXTE_PCB*) item )->TransformShapeWithClearanceToPolygonSet(
                                *layerPoly,
                                0,
                                segcountforcircle,
                                correctionFactor );
                break;

                default:
                    wxLogTrace( m_logTrace,
                                wxT( "createLayers: item type: %d not implemented" ),
                                item->Type() );
                break;
                }
            }
        }
    }

#ifdef PRINT_STATISTICS_3D_VIEWER
    printf( "T12: %.3f ms\n", (float)( GetRunningMicroSecs() - start_Time  ) / 1e3 );
    start_Time = GetRunningMicroSecs();
#endif

    if( GetFlag( FL_ZONE ) )
    {
        if( aStatusTextReporter )
            aStatusTextReporter->Report( _( "Create zones" ) );

        // Add zones objects
        // /////////////////////////////////////////////////////////////////////
        std::atomic<size_t> nextZone( 0 );
        std::atomic<size_t> threadsFinished( 0 );

        size_t parallelThreadCount = std::max<size_t>( std::thread::hardware_concurrency(), 2 );
        for( size_t ii = 0; ii < parallelThreadCount; ++ii )
        {
            std::thread t = std::thread( [&]()
            {
                for( size_t areaId = nextZone.fetch_add( 1 );
                            areaId < static_cast<size_t>( m_board->GetAreaCount() );
                            areaId = nextZone.fetch_add( 1 ) )
                {
                    const ZONE_CONTAINER* zone = m_board->GetArea( areaId );

                    if( zone == nullptr )
                        break;

                    auto layerContainer = m_layers_container2D.find( zone->GetLayer() );

                    if( layerContainer != m_layers_container2D.end() )
                        AddSolidAreasShapesToContainer( zone, layerContainer->second,
                                                        zone->GetLayer() );
                }

                threadsFinished++;
            } );

            t.detach();
        }

        while( threadsFinished < parallelThreadCount )
            std::this_thread::sleep_for( std::chrono::milliseconds( 10 ) );

    }

#ifdef PRINT_STATISTICS_3D_VIEWER
    printf( "fill zones T13: %.3f ms\n", (float)( GetRunningMicroSecs()  - start_Time  ) / 1e3 );
    start_Time = GetRunningMicroSecs();
#endif

    if( GetFlag( FL_ZONE ) &&
        GetFlag( FL_RENDER_OPENGL_COPPER_THICKNESS ) &&
        (m_render_engine == RENDER_ENGINE_OPENGL_LEGACY) )
    {
        // ADD COPPER ZONES
        for( int ii = 0; ii < m_board->GetAreaCount(); ++ii )
        {
            const ZONE_CONTAINER* zone = m_board->GetArea( ii );

            if( zone == nullptr )
                break;

            auto layerContainer = m_layers_poly.find( zone->GetLayer() );

            if( layerContainer != m_layers_poly.end() )
                zone->TransformSolidAreasShapesToPolygonSet( *layerContainer->second, segcountforcircle, correctionFactor );
        }
    }

#ifdef PRINT_STATISTICS_3D_VIEWER
    printf( "T14: %.3f ms\n", (float)( GetRunningMicroSecs() - start_Time  ) / 1e3 );
    start_Time = GetRunningMicroSecs();
#endif

    // Simplify layer polygons
    // /////////////////////////////////////////////////////////////////////////

    if( aStatusTextReporter )
        aStatusTextReporter->Report( _( "Simplifying copper layers polygons" ) );

    if( GetFlag( FL_RENDER_OPENGL_COPPER_THICKNESS ) &&
        (m_render_engine == RENDER_ENGINE_OPENGL_LEGACY) )
    {
        std::atomic<size_t> nextItem( 0 );
        std::atomic<size_t> threadsFinished( 0 );

        size_t parallelThreadCount = std::min<size_t>(
                std::max<size_t>( std::thread::hardware_concurrency(), 2 ),
                layer_id.size() );
        for( size_t ii = 0; ii < parallelThreadCount; ++ii )
        {
            std::thread t = std::thread( [&nextItem, &threadsFinished, &layer_id, this]()
            {
                for( size_t i = nextItem.fetch_add( 1 );
                            i < layer_id.size();
                            i = nextItem.fetch_add( 1 ) )
                {
                    auto layerPoly = m_layers_poly.find( layer_id[i] );

                    if( layerPoly != m_layers_poly.end() )
                        // This will make a union of all added contours
                        layerPoly->second->Simplify( SHAPE_POLY_SET::PM_FAST );
                }

                threadsFinished++;
            } );

            t.detach();
        }

        while( threadsFinished < parallelThreadCount )
            std::this_thread::sleep_for( std::chrono::milliseconds( 10 ) );
    }

#ifdef PRINT_STATISTICS_3D_VIEWER
    printf( "T15: %.3f ms\n", (float)( GetRunningMicroSecs() - start_Time ) / 1e3 );
    start_Time = GetRunningMicroSecs();
#endif

    // Simplify holes polygon contours
    // /////////////////////////////////////////////////////////////////////////
    if( aStatusTextReporter )
        aStatusTextReporter->Report( _( "Simplify holes contours" ) );

    for( unsigned int lIdx = 0; lIdx < layer_id.size(); ++lIdx )
    {
        const PCB_LAYER_ID curr_layer_id = layer_id[lIdx];

        if( m_layers_outer_holes_poly.find( curr_layer_id ) !=
            m_layers_outer_holes_poly.end() )
        {
            // found
            SHAPE_POLY_SET *polyLayer = m_layers_outer_holes_poly[curr_layer_id];
            polyLayer->Simplify( SHAPE_POLY_SET::PM_FAST );

            wxASSERT( m_layers_inner_holes_poly.find( curr_layer_id ) !=
                      m_layers_inner_holes_poly.end() );

            polyLayer = m_layers_inner_holes_poly[curr_layer_id];
            polyLayer->Simplify( SHAPE_POLY_SET::PM_FAST );
        }
    }

#ifdef PRINT_STATISTICS_3D_VIEWER
    printf( "T16: %.3f ms\n", (float)( GetRunningMicroSecs() - start_Time ) / 1e3 );
#endif
    // End Build Copper layers


    // This will make a union of all added contourns
    m_through_inner_holes_poly.Simplify( SHAPE_POLY_SET::PM_FAST );
    m_through_outer_holes_poly.Simplify( SHAPE_POLY_SET::PM_FAST );
    m_through_outer_holes_poly_NPTH.Simplify( SHAPE_POLY_SET::PM_FAST );
    m_through_outer_holes_vias_poly.Simplify( SHAPE_POLY_SET::PM_FAST );
    //m_through_inner_holes_vias_poly.Simplify( SHAPE_POLY_SET::PM_FAST ); // Not in use

#ifdef PRINT_STATISTICS_3D_VIEWER
    unsigned stats_endCopperLayersTime = GetRunningMicroSecs();
#endif

    // Build Tech layers
    // Based on: https://github.com/KiCad/kicad-source-mirror/blob/master/3d-viewer/3d_draw.cpp#L1059
    // /////////////////////////////////////////////////////////////////////////
#ifdef PRINT_STATISTICS_3D_VIEWER
    unsigned stats_startTechLayersTime = GetRunningMicroSecs();
#endif

    if( aStatusTextReporter )
        aStatusTextReporter->Report( _( "Build Tech layers" ) );

    // draw graphic items, on technical layers
    static const PCB_LAYER_ID teckLayerList[] = {
            B_Adhes,
            F_Adhes,
            B_Paste,
            F_Paste,
            B_SilkS,
            F_SilkS,
            B_Mask,
            F_Mask,

            // Aux Layers
            Dwgs_User,
            Cmts_User,
            Eco1_User,
            Eco2_User,
            Edge_Cuts,
            Margin
        };

    // User layers are not drawn here, only technical layers

    for( LSEQ seq = LSET::AllNonCuMask().Seq( teckLayerList, arrayDim( teckLayerList ) );
         seq;
         ++seq )
    {
        const PCB_LAYER_ID curr_layer_id = *seq;

        if( !Is3DLayerEnabled( curr_layer_id ) )
                    continue;

        CBVHCONTAINER2D *layerContainer = new CBVHCONTAINER2D;
        m_layers_container2D[curr_layer_id] = layerContainer;

        SHAPE_POLY_SET *layerPoly = new SHAPE_POLY_SET;
        m_layers_poly[curr_layer_id] = layerPoly;

        // Add drawing objects
        // /////////////////////////////////////////////////////////////////////
        for( auto item : m_board->Drawings() )
        {
            if( !item->IsOnLayer( curr_layer_id ) )
                continue;

            switch( item->Type() )
            {
            case PCB_LINE_T:
                AddShapeWithClearanceToContainer( (DRAWSEGMENT*)item,
                                                  layerContainer,
                                                  curr_layer_id,
                                                  0 );
                break;

            case PCB_TEXT_T:
                AddShapeWithClearanceToContainer( (TEXTE_PCB*) item,
                                                  layerContainer,
                                                  curr_layer_id,
                                                  0 );
                break;

            case PCB_DIMENSION_T:
                AddShapeWithClearanceToContainer( (DIMENSION*) item,
                                                  layerContainer,
                                                  curr_layer_id,
                                                  0 );
                break;

            default:
                break;
            }
        }


        // Add drawing contours
        // /////////////////////////////////////////////////////////////////////
        for( auto item : m_board->Drawings() )
        {
            if( !item->IsOnLayer( curr_layer_id ) )
                continue;

            switch( item->Type() )
            {
            case PCB_LINE_T:
            {
                const unsigned int nr_segments =
                        GetNrSegmentsCircle( item->GetBoundingBox().GetSizeMax() );

                ((DRAWSEGMENT*) item)->TransformShapeWithClearanceToPolygon( *layerPoly,
                                                                             0,
                                                                             nr_segments,
                                                                             0.0 );
            }
                break;

            case PCB_TEXT_T:
                ((TEXTE_PCB*) item)->TransformShapeWithClearanceToPolygonSet( *layerPoly,
                                                                              0,
                                                                              segcountInStrokeFont,
                                                                              1.0 );
                break;

            default:
                break;
            }
        }


        // Add modules tech layers - objects
        // /////////////////////////////////////////////////////////////////////
        for( MODULE* module = m_board->m_Modules; module; module = module->Next() )
        {
            if( (curr_layer_id == F_SilkS) || (curr_layer_id == B_SilkS) )
            {
                D_PAD*  pad = module->PadsList();
                int     linewidth = g_DrawDefaultLineThickness;

                for( ; pad; pad = pad->Next() )
                {
                    if( !pad->IsOnLayer( curr_layer_id ) )
                        continue;

                    buildPadShapeThickOutlineAsSegments( pad,
                                                         layerContainer,
                                                         linewidth );
                }
            }
            else
            {
                AddPadsShapesWithClearanceToContainer( module,
                                                       layerContainer,
                                                       curr_layer_id,
                                                       0,
                                                       false );
            }

            AddGraphicsShapesWithClearanceToContainer( module,
                                                       layerContainer,
                                                       curr_layer_id,
                                                       0 );
        }


        // Add modules tech layers - contours
        // /////////////////////////////////////////////////////////////////////
        for( MODULE* module = m_board->m_Modules; module; module = module->Next() )
        {
            if( (curr_layer_id == F_SilkS) || (curr_layer_id == B_SilkS) )
            {
                D_PAD*  pad = module->PadsList();
                const int linewidth = g_DrawDefaultLineThickness;

                for( ; pad; pad = pad->Next() )
                {
                    if( !pad->IsOnLayer( curr_layer_id ) )
                        continue;

                    buildPadShapeThickOutlineAsPolygon( pad, *layerPoly, linewidth );
                }
            }
            else
            {
                transformPadsShapesWithClearanceToPolygon( module->PadsList(),
                                                           curr_layer_id,
                                                           *layerPoly,
                                                           0,
                                                           false );
            }

            // On tech layers, use a poor circle approximation, only for texts (stroke font)
            module->TransformGraphicTextWithClearanceToPolygonSet( curr_layer_id,
                                                                   *layerPoly,
                                                                   0,
                                                                   segcountInStrokeFont,
                                                                   correctionFactorStroke,
                                                                   segcountInStrokeFont );

            // Add the remaining things with dynamic seg count for circles
            transformGraphicModuleEdgeToPolygonSet( module, curr_layer_id, *layerPoly );
        }


        // Draw non copper zones
        // /////////////////////////////////////////////////////////////////////
        if( GetFlag( FL_ZONE ) )
        {
            for( int ii = 0; ii < m_board->GetAreaCount(); ++ii )
            {
                ZONE_CONTAINER* zone = m_board->GetArea( ii );

                if( !zone->IsOnLayer( curr_layer_id ) )
                    continue;

                AddSolidAreasShapesToContainer( zone,
                                                layerContainer,
                                                curr_layer_id );
            }

            for( int ii = 0; ii < m_board->GetAreaCount(); ++ii )
            {
                ZONE_CONTAINER* zone = m_board->GetArea( ii );

                if( !zone->IsOnLayer( curr_layer_id ) )
                    continue;

                zone->TransformSolidAreasShapesToPolygonSet( *layerPoly,
                                                             // Use the same segcount as stroke font
                                                             segcountInStrokeFont,
                                                             correctionFactorStroke );
            }
        }

        // This will make a union of all added contours
        layerPoly->Simplify( SHAPE_POLY_SET::PM_FAST );
    }
    // End Build Tech layers

#ifdef PRINT_STATISTICS_3D_VIEWER
    unsigned stats_endTechLayersTime = GetRunningMicroSecs();
#endif


    // Build BVH for holes and vias
    // /////////////////////////////////////////////////////////////////////////

#ifdef PRINT_STATISTICS_3D_VIEWER
    unsigned stats_startHolesBVHTime = GetRunningMicroSecs();
#endif
    if( aStatusTextReporter )
        aStatusTextReporter->Report( _( "Build BVH for holes and vias" ) );

    m_through_holes_inner.BuildBVH();
    m_through_holes_outer.BuildBVH();

    if( !m_layers_holes2D.empty() )
    {
        for( MAP_CONTAINER_2D::iterator ii = m_layers_holes2D.begin();
             ii != m_layers_holes2D.end();
             ++ii )
        {
            ((CBVHCONTAINER2D *)(ii->second))->BuildBVH();
        }
    }

    // We only need the Solder mask to initialize the BVH
    // because..?
    if( (CBVHCONTAINER2D *)m_layers_container2D[B_Mask] )
        ((CBVHCONTAINER2D *)m_layers_container2D[B_Mask])->BuildBVH();

    if( (CBVHCONTAINER2D *)m_layers_container2D[F_Mask] )
        ((CBVHCONTAINER2D *)m_layers_container2D[F_Mask])->BuildBVH();

#ifdef PRINT_STATISTICS_3D_VIEWER
    unsigned stats_endHolesBVHTime = GetRunningMicroSecs();

    printf( "CINFO3D_VISU::createLayers times\n" );
    printf( "  Copper Layers:          %.3f ms\n",
            (float)( stats_endCopperLayersTime  - stats_startCopperLayersTime  ) / 1e3 );
    printf( "  Holes BVH creation:     %.3f ms\n",
            (float)( stats_endHolesBVHTime      - stats_startHolesBVHTime      ) / 1e3 );
    printf( "  Tech Layers:            %.3f ms\n",
            (float)( stats_endTechLayersTime    - stats_startTechLayersTime    ) / 1e3 );
    printf( "Statistics:\n" );
    printf( "  m_stats_nr_tracks                   %u\n", m_stats_nr_tracks );
    printf( "  m_stats_nr_vias                     %u\n", m_stats_nr_vias );
    printf( "  m_stats_nr_holes                    %u\n", m_stats_nr_holes );
    printf( "  m_stats_via_med_hole_diameter (3DU) %f\n", m_stats_via_med_hole_diameter );
    printf( "  m_stats_hole_med_diameter     (3DU) %f\n", m_stats_hole_med_diameter );
    printf( "  m_calc_seg_min_factor3DU      (3DU) %f\n", m_calc_seg_min_factor3DU );
    printf( "  m_calc_seg_max_factor3DU      (3DU) %f\n", m_calc_seg_max_factor3DU );
#endif
}
