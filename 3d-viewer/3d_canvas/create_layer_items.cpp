/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2015-2016 Mario Luzeiro <mrluzeiro@ua.pt>
 * Copyright (C) 1992-2020 KiCad Developers, see AUTHORS.txt for contributors.
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

#include "board_adapter.h"
#include "../3d_rendering/3d_render_raytracing/shapes2D/cring2d.h"
#include "../3d_rendering/3d_render_raytracing/shapes2D/cfilledcircle2d.h"
#include "../3d_rendering/3d_render_raytracing/shapes3D/ccylinder.h"

#include <class_board.h>
#include <class_module.h>
#include <class_pad.h>
#include <pcb_text.h>
#include <fp_shape.h>
#include <zone.h>
#include <convert_basic_shapes_to_polygon.h>
#include <trigo.h>
#include <vector>
#include <thread>
#include <algorithm>
#include <atomic>

#ifdef PRINT_STATISTICS_3D_VIEWER
#include <profile.h>
#endif


void BOARD_ADAPTER::destroyLayers()
{
    if( !m_layers_poly.empty() )
    {
        for( auto& poly : m_layers_poly )
            delete poly.second;

        m_layers_poly.clear();
    }

    delete m_F_Cu_PlatedPads_poly;
    m_F_Cu_PlatedPads_poly = nullptr;

    delete m_B_Cu_PlatedPads_poly;
    m_B_Cu_PlatedPads_poly = nullptr;

    if( !m_layers_inner_holes_poly.empty() )
    {
        for( auto& poly : m_layers_inner_holes_poly )
            delete poly.second;

        m_layers_inner_holes_poly.clear();
    }

    if( !m_layers_outer_holes_poly.empty() )
    {
        for( auto& poly : m_layers_outer_holes_poly )
            delete poly.second;

        m_layers_outer_holes_poly.clear();
    }

    if( !m_layers_container2D.empty() )
    {
        for( auto& poly : m_layers_container2D )
            delete poly.second;

        m_layers_container2D.clear();
    }

    delete m_platedpads_container2D_F_Cu;
    m_platedpads_container2D_F_Cu = nullptr;

    delete m_platedpads_container2D_B_Cu;
    m_platedpads_container2D_B_Cu = nullptr;

    if( !m_layers_holes2D.empty() )
    {
        for( auto& poly : m_layers_holes2D )
            delete poly.second;

        m_layers_holes2D.clear();
    }

    m_through_holes_inner.Clear();
    m_through_holes_outer.Clear();
    m_through_holes_outer_ring.Clear();
    m_through_holes_vias_outer.Clear();
    m_through_holes_vias_inner.Clear();
    m_through_outer_holes_poly_NPTH.RemoveAllContours();
    m_through_outer_holes_poly.RemoveAllContours();

    m_through_outer_holes_vias_poly.RemoveAllContours();
    m_through_outer_ring_holes_poly.RemoveAllContours();
}


void BOARD_ADAPTER::createLayers( REPORTER* aStatusReporter )
{
    destroyLayers();

    // Build Copper layers
    // Based on: https://github.com/KiCad/kicad-source-mirror/blob/master/3d-viewer/3d_draw.cpp#L692
    // /////////////////////////////////////////////////////////////////////////

#ifdef PRINT_STATISTICS_3D_VIEWER
    unsigned stats_startCopperLayersTime = GetRunningMicroSecs();

    unsigned start_Time = stats_startCopperLayersTime;
#endif

    PCB_LAYER_ID cu_seq[MAX_CU_LAYERS];
    LSET         cu_set = LSET::AllCuMask( m_copperLayersCount );

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
    trackList.reserve( m_board->Tracks().size() );

    for( TRACK* track : m_board->Tracks() )
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

        if( GetFlag( FL_RENDER_OPENGL_COPPER_THICKNESS )
                && ( m_render_engine == RENDER_ENGINE::OPENGL_LEGACY ) )
        {
            SHAPE_POLY_SET* layerPoly    = new SHAPE_POLY_SET;
            m_layers_poly[curr_layer_id] = layerPoly;
        }
    }

    if( GetFlag( FL_RENDER_PLATED_PADS_AS_PLATED ) &&
        GetFlag( FL_USE_REALISTIC_MODE ) )
    {
        m_F_Cu_PlatedPads_poly = new SHAPE_POLY_SET;
        m_B_Cu_PlatedPads_poly = new SHAPE_POLY_SET;

        m_platedpads_container2D_F_Cu = new CBVHCONTAINER2D;
        m_platedpads_container2D_B_Cu = new CBVHCONTAINER2D;

    }

    if( aStatusReporter )
        aStatusReporter->Report( _( "Create tracks and vias" ) );

    // Create tracks as objects and add it to container
    // /////////////////////////////////////////////////////////////////////////
    for( PCB_LAYER_ID curr_layer_id : layer_id )
    {
        wxASSERT( m_layers_container2D.find( curr_layer_id ) != m_layers_container2D.end() );

        CBVHCONTAINER2D *layerContainer = m_layers_container2D[curr_layer_id];

        // Add track segments shapes and via annulus shapes
        unsigned int nTracks = trackList.size();

        for( unsigned int trackIdx = 0; trackIdx < nTracks; ++trackIdx )
        {
            const TRACK *track = trackList[trackIdx];

            // NOTE: Vias can be on multiple layers
            if( !track->IsOnLayer( curr_layer_id ) )
                continue;

            // Skip vias annulus when not connected on this layer (if removing is enabled)
            const VIA *via = dyn_cast< const VIA*>( track );

            if( via && !via->FlashLayer( curr_layer_id ) && IsCopperLayer( curr_layer_id ) )
                continue;

            // Add object item to layer container
            createNewTrack( track, layerContainer, 0.0f );
        }
    }

    // Create VIAS and THTs objects and add it to holes containers
    // /////////////////////////////////////////////////////////////////////////
    for( PCB_LAYER_ID curr_layer_id : layer_id )
    {
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
                const VIA*    via               = static_cast<const VIA*>( track );
                const VIATYPE viatype           = via->GetViaType();
                const float   holediameter      = via->GetDrillValue() * BiuTo3Dunits();
                const float   thickness         = GetCopperThickness3DU();
                const float   hole_inner_radius = ( holediameter / 2.0f );
                const float   ring_radius       = via->GetWidth() * BiuTo3Dunits() / 2.0f;

                const SFVEC2F via_center(
                        via->GetStart().x * m_biuTo3Dunits, -via->GetStart().y * m_biuTo3Dunits );

                if( viatype != VIATYPE::THROUGH )
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
                else if( curr_layer_id == layer_id[0] ) // it only adds once the THT holes
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

                    if( GetFlag( FL_CLIP_SILK_ON_VIA_ANNULUS ) &&
                        GetFlag( FL_USE_REALISTIC_MODE ) )
                    {
                        m_through_holes_outer_ring.Add( new CFILLEDCIRCLE2D( via_center,
                                                                             ring_radius,
                                                                             *track ) );
                    }

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

    // Create VIAS and THTs objects and add it to holes containers
    // /////////////////////////////////////////////////////////////////////////
    for( PCB_LAYER_ID curr_layer_id : layer_id )
    {
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
                const VIATYPE viatype = via->GetViaType();

                if( viatype != VIATYPE::THROUGH )
                {
                    // Add VIA hole contourns

                    // Add outer holes of VIAs
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
                    const int hole_outer_radius = (holediameter / 2) + GetHolePlatingThicknessBIU();

                    TransformCircleToPolygon( *layerOuterHolesPoly, via->GetStart(),
                                              hole_outer_radius, ARC_HIGH_DEF, ERROR_INSIDE );

                    TransformCircleToPolygon( *layerInnerHolesPoly, via->GetStart(),
                                              holediameter / 2, ARC_HIGH_DEF, ERROR_INSIDE );
                }
                else if( curr_layer_id == layer_id[0] ) // it only adds once the THT holes
                {
                    const int holediameter = via->GetDrillValue();
                    const int hole_outer_radius = (holediameter / 2) + GetHolePlatingThicknessBIU();
                    const int hole_outer_ring_radius = via->GetWidth() / 2.0f;

                    // Add through hole contourns
                    // /////////////////////////////////////////////////////////
                    TransformCircleToPolygon( m_through_outer_holes_poly, via->GetStart(),
                                              hole_outer_radius, ARC_HIGH_DEF, ERROR_INSIDE );

                    // Add same thing for vias only

                    TransformCircleToPolygon( m_through_outer_holes_vias_poly, via->GetStart(),
                                              hole_outer_radius, ARC_HIGH_DEF, ERROR_INSIDE );

                    if( GetFlag( FL_CLIP_SILK_ON_VIA_ANNULUS ) &&
                        GetFlag( FL_USE_REALISTIC_MODE ) )
                    {
                        TransformCircleToPolygon( m_through_outer_ring_holes_poly,
                                                  via->GetStart(), hole_outer_ring_radius,
                                                  ARC_HIGH_DEF, ERROR_INSIDE );
                    }
                }
            }
        }
    }

    // Creates vertical outline contours of the tracks and add it to the poly of the layer
    // /////////////////////////////////////////////////////////////////////////
    if( GetFlag( FL_RENDER_OPENGL_COPPER_THICKNESS )
            && ( m_render_engine == RENDER_ENGINE::OPENGL_LEGACY ) )
    {
        for( PCB_LAYER_ID curr_layer_id : layer_id )
        {
            wxASSERT( m_layers_poly.find( curr_layer_id ) != m_layers_poly.end() );

            SHAPE_POLY_SET *layerPoly = m_layers_poly[curr_layer_id];

            // ADD TRACKS
            unsigned int nTracks = trackList.size();

            for( unsigned int trackIdx = 0; trackIdx < nTracks; ++trackIdx )
            {
                const TRACK *track = trackList[trackIdx];

                if( !track->IsOnLayer( curr_layer_id ) )
                    continue;

                // Skip vias annulus when not connected on this layer (if removing is enabled)
                const VIA *via = dyn_cast< const VIA*>( track );

                if( via && !via->FlashLayer( curr_layer_id ) && IsCopperLayer( curr_layer_id ) )
                    continue;

                // Add the track/via contour
                track->TransformShapeWithClearanceToPolygon( *layerPoly, curr_layer_id, 0,
                                                             ARC_HIGH_DEF, ERROR_INSIDE );
            }
        }
    }

    // Add holes of footprints
    // /////////////////////////////////////////////////////////////////////////
    for( MODULE* module : m_board->Modules() )
    {
        for( D_PAD* pad : module->Pads() )
        {
            const wxSize padHole = pad->GetDrillSize();

            if( !padHole.x )    // Not drilled pad like SMD pad
                continue;

            // The hole in the body is inflated by copper thickness, if not plated, no copper
            const int inflate = ( pad->GetAttribute () != PAD_ATTRIB_NPTH ) ?
                                GetHolePlatingThicknessBIU() : 0;

            m_stats_nr_holes++;
            m_stats_hole_med_diameter += ( ( pad->GetDrillSize().x +
                                             pad->GetDrillSize().y ) / 2.0f ) * m_biuTo3Dunits;

            m_through_holes_outer.Add( createNewPadDrill( pad, inflate ) );

            if( GetFlag( FL_CLIP_SILK_ON_VIA_ANNULUS ) &&
                GetFlag( FL_USE_REALISTIC_MODE ) )
            {
                m_through_holes_outer_ring.Add( createNewPadDrill( pad, inflate ) );
            }

            m_through_holes_inner.Add( createNewPadDrill( pad, 0 ) );
        }
    }

    if( m_stats_nr_holes )
        m_stats_hole_med_diameter /= (float)m_stats_nr_holes;

    // Add contours of the pad holes (pads can be Circle or Segment holes)
    // /////////////////////////////////////////////////////////////////////////
    for( MODULE* module : m_board->Modules() )
    {
        for( D_PAD* pad : module->Pads() )
        {
            const wxSize padHole = pad->GetDrillSize();

            if( !padHole.x ) // Not drilled pad like SMD pad
                continue;

            // The hole in the body is inflated by copper thickness.
            const int inflate = GetHolePlatingThicknessBIU();

            if( pad->GetAttribute () != PAD_ATTRIB_NPTH )
            {
                if( GetFlag( FL_CLIP_SILK_ON_VIA_ANNULUS ) &&
                    GetFlag( FL_USE_REALISTIC_MODE ) )
                {
                    pad->TransformHoleWithClearanceToPolygon( m_through_outer_ring_holes_poly,
                                                              inflate,
                                                              ARC_HIGH_DEF, ERROR_INSIDE );
                }

                pad->TransformHoleWithClearanceToPolygon( m_through_outer_holes_poly, inflate,
                                                          ARC_HIGH_DEF, ERROR_INSIDE );
            }
            else
            {
                // If not plated, no copper.
                if( GetFlag( FL_CLIP_SILK_ON_VIA_ANNULUS ) &&
                    GetFlag( FL_USE_REALISTIC_MODE ) )
                {
                    pad->TransformHoleWithClearanceToPolygon( m_through_outer_ring_holes_poly, 0,
                                                              ARC_HIGH_DEF, ERROR_INSIDE );
                }

                pad->TransformHoleWithClearanceToPolygon( m_through_outer_holes_poly_NPTH, 0,
                                                          ARC_HIGH_DEF, ERROR_INSIDE );
            }
        }
    }

    const bool renderPlatedPadsAsPlated = GetFlag( FL_RENDER_PLATED_PADS_AS_PLATED ) &&
                                          GetFlag( FL_USE_REALISTIC_MODE );

    // Add footprints PADs objects to containers
    for( PCB_LAYER_ID curr_layer_id : layer_id )
    {
        wxASSERT( m_layers_container2D.find( curr_layer_id ) != m_layers_container2D.end() );

        CBVHCONTAINER2D *layerContainer = m_layers_container2D[curr_layer_id];

        // ADD PADS
        for( MODULE* module : m_board->Modules() )
        {
            // Note: NPTH pads are not drawn on copper layers when the pad
            // has same shape as its hole
            AddPadsShapesWithClearanceToContainer( module, layerContainer, curr_layer_id, 0,
                                                   true, renderPlatedPadsAsPlated, false );

            // Micro-wave footprints may have items on copper layers
            AddGraphicsShapesWithClearanceToContainer( module, layerContainer, curr_layer_id, 0 );
        }
    }

    if( renderPlatedPadsAsPlated )
    {
        // ADD PLATED PADS
        for( MODULE* module : m_board->Modules() )
        {
            AddPadsShapesWithClearanceToContainer( module, m_platedpads_container2D_F_Cu, F_Cu, 0,
                                                   true, false, true );

            AddPadsShapesWithClearanceToContainer( module, m_platedpads_container2D_B_Cu, B_Cu, 0,
                                                   true, false, true );
        }

        m_platedpads_container2D_F_Cu->BuildBVH();
        m_platedpads_container2D_B_Cu->BuildBVH();
    }

    // Add footprints PADs poly contourns (vertical outlines)
    if( GetFlag( FL_RENDER_OPENGL_COPPER_THICKNESS )
            && ( m_render_engine == RENDER_ENGINE::OPENGL_LEGACY ) )
    {
        for( PCB_LAYER_ID curr_layer_id : layer_id )
        {
            wxASSERT( m_layers_poly.find( curr_layer_id ) != m_layers_poly.end() );

            SHAPE_POLY_SET *layerPoly = m_layers_poly[curr_layer_id];

            // Add pads to polygon list
            for( auto module : m_board->Modules() )
            {
                // Note: NPTH pads are not drawn on copper layers when the pad
                // has same shape as its hole
                module->TransformPadsShapesWithClearanceToPolygon( *layerPoly, curr_layer_id,
                                                                   0, ARC_HIGH_DEF, ERROR_INSIDE,
                                                                   true, renderPlatedPadsAsPlated,
                                                                   false );

                transformGraphicModuleEdgeToPolygonSet( module, curr_layer_id, *layerPoly );
            }
        }

        if( renderPlatedPadsAsPlated )
        {
            // ADD PLATED PADS contourns
            for( auto module : m_board->Modules() )
            {
                module->TransformPadsShapesWithClearanceToPolygon( *m_F_Cu_PlatedPads_poly, F_Cu,
                                                                   0, ARC_HIGH_DEF, ERROR_INSIDE,
                                                                   true, false, true );

                //transformGraphicModuleEdgeToPolygonSet( module, F_Cu, *m_F_Cu_PlatedPads_poly );

                module->TransformPadsShapesWithClearanceToPolygon( *m_B_Cu_PlatedPads_poly, B_Cu,
                                                                   0, ARC_HIGH_DEF, ERROR_INSIDE,
                                                                   true, false, true );

                //transformGraphicModuleEdgeToPolygonSet( module, B_Cu, *m_B_Cu_PlatedPads_poly );
            }
        }
    }


    // Add graphic item on copper layers to object containers
    for( PCB_LAYER_ID curr_layer_id : layer_id )
    {
        wxASSERT( m_layers_container2D.find( curr_layer_id ) != m_layers_container2D.end() );

        CBVHCONTAINER2D *layerContainer = m_layers_container2D[curr_layer_id];

        // Add graphic items on copper layers (texts and other graphics)
        for( auto item : m_board->Drawings() )
        {
            if( !item->IsOnLayer( curr_layer_id ) )
                continue;

            switch( item->Type() )
            {
            case PCB_SHAPE_T:
            {
                AddShapeWithClearanceToContainer( (PCB_SHAPE*)item, layerContainer, curr_layer_id,
                                                  0 );
            }
            break;

            case PCB_TEXT_T:
                AddShapeWithClearanceToContainer( (PCB_TEXT*) item, layerContainer, curr_layer_id,
                                                  0 );
            break;

            case PCB_DIM_ALIGNED_T:
            case PCB_DIM_CENTER_T:
            case PCB_DIM_ORTHOGONAL_T:
            case PCB_DIM_LEADER_T:
                AddShapeWithClearanceToContainer( (DIMENSION_BASE*) item, layerContainer,
                                                  curr_layer_id, 0 );
            break;

            default:
                wxLogTrace( m_logTrace,
                            wxT( "createLayers: item type: %d not implemented" ),
                            item->Type() );
            break;
            }
        }
    }

    // Add graphic item on copper layers to poly contourns (vertical outlines)
    if( GetFlag( FL_RENDER_OPENGL_COPPER_THICKNESS )
            && ( m_render_engine == RENDER_ENGINE::OPENGL_LEGACY ) )
    {
        for( PCB_LAYER_ID cur_layer_id : layer_id )
        {
            wxASSERT( m_layers_poly.find( cur_layer_id ) != m_layers_poly.end() );

            SHAPE_POLY_SET *layerPoly = m_layers_poly[cur_layer_id];

            // Add graphic items on copper layers (texts and other )
            for( BOARD_ITEM* item : m_board->Drawings() )
            {
                if( !item->IsOnLayer( cur_layer_id ) )
                    continue;

                switch( item->Type() )
                {
                case PCB_SHAPE_T:
                    ( (PCB_SHAPE*) item )->TransformShapeWithClearanceToPolygon( *layerPoly,
                                                                                 cur_layer_id, 0,
                                                                                 ARC_HIGH_DEF,
                                                                                 ERROR_INSIDE );
                    break;

                case PCB_TEXT_T:
                    ( (PCB_TEXT*) item )->TransformShapeWithClearanceToPolygonSet( *layerPoly, 0,
                                                                                   ARC_HIGH_DEF,
                                                                                   ERROR_INSIDE );
                    break;

                default:
                    wxLogTrace( m_logTrace, wxT( "createLayers: item type: %d not implemented" ),
                                item->Type() );
                    break;
                }
            }
        }
    }

    if( GetFlag( FL_ZONE ) )
    {
        if( aStatusReporter )
            aStatusReporter->Report( _( "Create zones" ) );

        std::vector<std::pair<const ZONE*, PCB_LAYER_ID>> zones;

        for( ZONE* zone : m_board->Zones() )
        {
            for( PCB_LAYER_ID layer : zone->GetLayerSet().Seq() )
                zones.emplace_back( std::make_pair( zone, layer ) );
        }

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
                            areaId < zones.size();
                            areaId = nextZone.fetch_add( 1 ) )
                {
                    const ZONE* zone = zones[areaId].first;

                    if( zone == nullptr )
                        break;

                    PCB_LAYER_ID layer = zones[areaId].second;

                    auto layerContainer = m_layers_container2D.find( layer );

                    if( layerContainer != m_layers_container2D.end() )
                        AddSolidAreasShapesToContainer( zone, layerContainer->second, layer );
                }

                threadsFinished++;
            } );

            t.detach();
        }

        while( threadsFinished < parallelThreadCount )
            std::this_thread::sleep_for( std::chrono::milliseconds( 10 ) );

    }

    if( GetFlag( FL_ZONE ) && GetFlag( FL_RENDER_OPENGL_COPPER_THICKNESS )
            && ( m_render_engine == RENDER_ENGINE::OPENGL_LEGACY ) )
    {
        // Add copper zones
        for( ZONE* zone : m_board->Zones() )
        {
            if( zone == nullptr )
                break;

            for( PCB_LAYER_ID layer : zone->GetLayerSet().Seq() )
            {
                auto layerContainer = m_layers_poly.find( layer );

                if( layerContainer != m_layers_poly.end() )
                    zone->TransformSolidAreasShapesToPolygon( layer, *layerContainer->second );
            }
        }
    }

    // Simplify layer polygons

    if( aStatusReporter )
        aStatusReporter->Report( _( "Simplifying copper layers polygons" ) );

    if( GetFlag( FL_RENDER_OPENGL_COPPER_THICKNESS )
            && ( m_render_engine == RENDER_ENGINE::OPENGL_LEGACY ) )
    {
        if( GetFlag( FL_RENDER_PLATED_PADS_AS_PLATED ) &&
            GetFlag( FL_USE_REALISTIC_MODE ) )
        {
            if( m_F_Cu_PlatedPads_poly && ( m_layers_poly.find( F_Cu ) != m_layers_poly.end() ) )
            {
                SHAPE_POLY_SET *layerPoly_F_Cu = m_layers_poly[F_Cu];
                layerPoly_F_Cu->BooleanSubtract( *m_F_Cu_PlatedPads_poly, SHAPE_POLY_SET::POLYGON_MODE::PM_FAST );

                m_F_Cu_PlatedPads_poly->Simplify( SHAPE_POLY_SET::PM_FAST );
            }

            if( m_B_Cu_PlatedPads_poly && ( m_layers_poly.find( B_Cu ) != m_layers_poly.end() ) )
            {
                SHAPE_POLY_SET *layerPoly_B_Cu = m_layers_poly[B_Cu];
                layerPoly_B_Cu->BooleanSubtract( *m_B_Cu_PlatedPads_poly, SHAPE_POLY_SET::POLYGON_MODE::PM_FAST );

                m_B_Cu_PlatedPads_poly->Simplify( SHAPE_POLY_SET::PM_FAST );
            }
        }

        std::vector< PCB_LAYER_ID > &selected_layer_id = layer_id;
        std::vector< PCB_LAYER_ID > layer_id_without_F_and_B;

        if( GetFlag( FL_RENDER_PLATED_PADS_AS_PLATED ) &&
            GetFlag( FL_USE_REALISTIC_MODE ) )
        {
            layer_id_without_F_and_B.clear();
            layer_id_without_F_and_B.reserve( layer_id.size() );

            for( size_t i = 0; i < layer_id.size(); ++i )
            {
                if( ( layer_id[i] != F_Cu ) &&
                    ( layer_id[i] != B_Cu ) )
                    layer_id_without_F_and_B.push_back( layer_id[i] );
            }

            selected_layer_id = layer_id_without_F_and_B;
        }

        if( selected_layer_id.size() > 0 )
        {
            std::atomic<size_t> nextItem( 0 );
            std::atomic<size_t> threadsFinished( 0 );

            size_t parallelThreadCount = std::min<size_t>(
                    std::max<size_t>( std::thread::hardware_concurrency(), 2 ),
                    selected_layer_id.size() );
            for( size_t ii = 0; ii < parallelThreadCount; ++ii )
            {
                std::thread t = std::thread( [&nextItem, &threadsFinished, &selected_layer_id, this]()
                {
                    for( size_t i = nextItem.fetch_add( 1 );
                                i < selected_layer_id.size();
                                i = nextItem.fetch_add( 1 ) )
                    {
                        auto layerPoly = m_layers_poly.find( selected_layer_id[i] );

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
    }

    // Simplify holes polygon contours
    // /////////////////////////////////////////////////////////////////////////
    if( aStatusReporter )
        aStatusReporter->Report( _( "Simplify holes contours" ) );

    for( PCB_LAYER_ID layer : layer_id )
    {
        if( m_layers_outer_holes_poly.find( layer ) != m_layers_outer_holes_poly.end() )
        {
            // found
            SHAPE_POLY_SET *polyLayer = m_layers_outer_holes_poly[layer];
            polyLayer->Simplify( SHAPE_POLY_SET::PM_FAST );

            wxASSERT( m_layers_inner_holes_poly.find( layer ) != m_layers_inner_holes_poly.end() );

            polyLayer = m_layers_inner_holes_poly[layer];
            polyLayer->Simplify( SHAPE_POLY_SET::PM_FAST );
        }
    }

    // End Build Copper layers

    // This will make a union of all added contourns
    m_through_outer_holes_poly.Simplify( SHAPE_POLY_SET::PM_FAST );
    m_through_outer_holes_poly_NPTH.Simplify( SHAPE_POLY_SET::PM_FAST );
    m_through_outer_holes_vias_poly.Simplify( SHAPE_POLY_SET::PM_FAST );
    m_through_outer_ring_holes_poly.Simplify( SHAPE_POLY_SET::PM_FAST );

    // Build Tech layers
    // Based on: https://github.com/KiCad/kicad-source-mirror/blob/master/3d-viewer/3d_draw.cpp#L1059
    // /////////////////////////////////////////////////////////////////////////
    if( aStatusReporter )
        aStatusReporter->Report( _( "Build Tech layers" ) );

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
        for( BOARD_ITEM* item : m_board->Drawings() )
        {
            if( !item->IsOnLayer( curr_layer_id ) )
                continue;

            switch( item->Type() )
            {
            case PCB_SHAPE_T:
                AddShapeWithClearanceToContainer( (PCB_SHAPE*) item,
                                                  layerContainer,
                                                  curr_layer_id,
                                                  0 );
                break;

            case PCB_TEXT_T:
                AddShapeWithClearanceToContainer( (PCB_TEXT*) item,
                                                  layerContainer,
                                                  curr_layer_id,
                                                  0 );
                break;

            case PCB_DIM_ALIGNED_T:
            case PCB_DIM_CENTER_T:
            case PCB_DIM_ORTHOGONAL_T:
            case PCB_DIM_LEADER_T:
                AddShapeWithClearanceToContainer( (DIMENSION_BASE*) item,
                                                  layerContainer,
                                                  curr_layer_id,
                                                  0 );
                break;

            default:
                break;
            }
        }


        // Add drawing contours
        for( BOARD_ITEM* item : m_board->Drawings() )
        {
            if( !item->IsOnLayer( curr_layer_id ) )
                continue;

            switch( item->Type() )
            {
            case PCB_SHAPE_T:
                ( (PCB_SHAPE*) item )->TransformShapeWithClearanceToPolygon( *layerPoly,
                                                                             curr_layer_id, 0,
                                                                             ARC_HIGH_DEF,
                                                                             ERROR_INSIDE );
                break;

            case PCB_TEXT_T:
                ( (PCB_TEXT*) item )->TransformShapeWithClearanceToPolygonSet( *layerPoly, 0,
                                                                               ARC_HIGH_DEF,
                                                                               ERROR_INSIDE );
                break;

            default:
                break;
            }
        }


        // Add footprints tech layers - objects
        // /////////////////////////////////////////////////////////////////////
        for( MODULE* module : m_board->Modules() )
        {
            if( (curr_layer_id == F_SilkS) || (curr_layer_id == B_SilkS) )
            {
                int     linewidth = g_DrawDefaultLineThickness;

                for( D_PAD* pad : module->Pads() )
                {
                    if( !pad->IsOnLayer( curr_layer_id ) )
                        continue;

                    buildPadShapeThickOutlineAsSegments( pad, layerContainer, linewidth );
                }
            }
            else
            {
                AddPadsShapesWithClearanceToContainer( module, layerContainer, curr_layer_id, 0,
                                                       false,
                                                       false,
                                                       false );
            }

            AddGraphicsShapesWithClearanceToContainer( module, layerContainer, curr_layer_id, 0 );
        }


        // Add footprints tech layers - contours
        for( MODULE* module : m_board->Modules() )
        {
            if( (curr_layer_id == F_SilkS) || (curr_layer_id == B_SilkS) )
            {
                const int linewidth = g_DrawDefaultLineThickness;

                for( D_PAD* pad : module->Pads() )
                {
                    if( !pad->IsOnLayer( curr_layer_id ) )
                        continue;

                    buildPadShapeThickOutlineAsPolygon( pad, *layerPoly, linewidth );
                }
            }
            else
            {
                module->TransformPadsShapesWithClearanceToPolygon( *layerPoly, curr_layer_id, 0,
                                                                   ARC_HIGH_DEF, ERROR_INSIDE );
            }

            // On tech layers, use a poor circle approximation, only for texts (stroke font)
            module->TransformGraphicTextWithClearanceToPolygonSet( *layerPoly, curr_layer_id, 0,
                                                                   ARC_HIGH_DEF, ERROR_INSIDE );

            // Add the remaining things with dynamic seg count for circles
            transformGraphicModuleEdgeToPolygonSet( module, curr_layer_id, *layerPoly );
        }


        // Draw non copper zones
        if( GetFlag( FL_ZONE ) )
        {
            for( ZONE* zone : m_board->Zones() )
            {
                if( zone->IsOnLayer( curr_layer_id ) )
                    AddSolidAreasShapesToContainer( zone, layerContainer, curr_layer_id );
            }

            for( ZONE* zone : m_board->Zones() )
            {
                if( zone->IsOnLayer( curr_layer_id ) )
                    zone->TransformSolidAreasShapesToPolygon( curr_layer_id, *layerPoly );
            }
        }

        // This will make a union of all added contours
        layerPoly->Simplify( SHAPE_POLY_SET::PM_FAST );
    }
    // End Build Tech layers

    // Build BVH (Bounding volume hierarchy) for holes and vias

    if( aStatusReporter )
        aStatusReporter->Report( _( "Build BVH for holes and vias" ) );

    m_through_holes_inner.BuildBVH();
    m_through_holes_outer.BuildBVH();
    m_through_holes_outer_ring.BuildBVH();

    if( !m_layers_holes2D.empty() )
    {
        for( auto& hole : m_layers_holes2D )
            hole.second->BuildBVH();
    }

    // We only need the Solder mask to initialize the BVH
    // because..?
    if( m_layers_container2D[B_Mask] )
        m_layers_container2D[B_Mask]->BuildBVH();

    if( m_layers_container2D[F_Mask] )
        m_layers_container2D[F_Mask]->BuildBVH();
}
