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
 *
 * It is based on the function found in the files:
 *  board_items_to_polygon_shape_transform.cpp
 *  board_items_to_polygon_shape_transform.cpp
 */

#include "board_adapter.h"
#include "../3d_rendering/3d_render_raytracing/shapes2D/ring_2d.h"
#include "../3d_rendering/3d_render_raytracing/shapes2D/filled_circle_2d.h"
#include "../3d_rendering/3d_render_raytracing/shapes3D/cylinder_3d.h"

#include <board.h>
#include <board_design_settings.h>
#include <footprint.h>
#include <pad.h>
#include <pcb_text.h>
#include <fp_shape.h>
#include <zone.h>
#include <convert_basic_shapes_to_polygon.h>
#include <trigo.h>
#include <vector>
#include <thread>
#include <core/arraydim.h>
#include <algorithm>
#include <atomic>
#include <wx/log.h>

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

    delete m_frontPlatedPadPolys;
    m_frontPlatedPadPolys = nullptr;

    delete m_backPlatedPadPolys;
    m_backPlatedPadPolys = nullptr;

    if( !m_layerHoleIdPolys.empty() )
    {
        for( auto& poly : m_layerHoleIdPolys )
            delete poly.second;

        m_layerHoleIdPolys.clear();
    }

    if( !m_layerHoleOdPolys.empty() )
    {
        for( auto& poly : m_layerHoleOdPolys )
            delete poly.second;

        m_layerHoleOdPolys.clear();
    }

    if( !m_layerMap.empty() )
    {
        for( auto& poly : m_layerMap )
            delete poly.second;

        m_layerMap.clear();
    }

    delete m_platedPadsFront;
    m_platedPadsFront = nullptr;

    delete m_platedPadsBack;
    m_platedPadsBack = nullptr;

    if( !m_layerHoleMap.empty() )
    {
        for( auto& poly : m_layerHoleMap )
            delete poly.second;

        m_layerHoleMap.clear();
    }

    m_throughHoleIds.Clear();
    m_throughHoleOds.Clear();
    m_throughHoleAnnularRings.Clear();
    m_throughHoleViaOds.Clear();
    m_throughHoleViaIds.Clear();
    m_nonPlatedThroughHoleOdPolys.RemoveAllContours();
    m_throughHoleOdPolys.RemoveAllContours();

    m_throughHoleViaOdPolys.RemoveAllContours();
    m_throughHoleAnnularRingPolys.RemoveAllContours();
}


void BOARD_ADAPTER::createLayers( REPORTER* aStatusReporter )
{
    destroyLayers();

    // Build Copper layers
    // Based on:
    //    https://github.com/KiCad/kicad-source-mirror/blob/master/3d-viewer/3d_draw.cpp#L692

#ifdef PRINT_STATISTICS_3D_VIEWER
    unsigned stats_startCopperLayersTime = GetRunningMicroSecs();

    unsigned start_Time = stats_startCopperLayersTime;
#endif

    PCB_LAYER_ID cu_seq[MAX_CU_LAYERS];
    LSET         cu_set = LSET::AllCuMask( m_copperLayersCount );

    m_trackCount               = 0;
    m_averageTrackWidth        = 0;
    m_viaCount                 = 0;
    m_averageViaHoleDiameter   = 0;
    m_holeCount                = 0;
    m_averageHoleDiameter      = 0;

    // Prepare track list, convert in a vector. Calc statistic for the holes
    std::vector< const TRACK *> trackList;
    trackList.clear();
    trackList.reserve( m_board->Tracks().size() );

    for( TRACK* track : m_board->Tracks() )
    {
        if( !Is3dLayerEnabled( track->GetLayer() ) ) // Skip non enabled layers
            continue;

        // Note: a TRACK holds normal segment tracks and
        // also vias circles (that have also drill values)
        trackList.push_back( track );

        if( track->Type() == PCB_VIA_T )
        {
            const VIA *via = static_cast< const VIA*>( track );
            m_viaCount++;
            m_averageViaHoleDiameter += via->GetDrillValue() * m_biuTo3Dunits;
        }
        else
        {
            m_trackCount++;
        }

        m_averageTrackWidth += track->GetWidth() * m_biuTo3Dunits;
    }

    if( m_trackCount )
        m_averageTrackWidth /= (float)m_trackCount;

    if( m_viaCount )
        m_averageViaHoleDiameter /= (float)m_viaCount;

    // Prepare copper layers index and containers
    std::vector< PCB_LAYER_ID > layer_id;
    layer_id.clear();
    layer_id.reserve( m_copperLayersCount );

    for( unsigned i = 0; i < arrayDim( cu_seq ); ++i )
        cu_seq[i] = ToLAYER_ID( B_Cu - i );

    for( LSEQ cu = cu_set.Seq( cu_seq, arrayDim( cu_seq ) ); cu; ++cu )
    {
        const PCB_LAYER_ID curr_layer_id = *cu;

        if( !Is3dLayerEnabled( curr_layer_id ) ) // Skip non enabled layers
            continue;

        layer_id.push_back( curr_layer_id );

        BVH_CONTAINER_2D *layerContainer = new BVH_CONTAINER_2D;
        m_layerMap[curr_layer_id] = layerContainer;

        if( GetFlag( FL_RENDER_OPENGL_COPPER_THICKNESS )
          && ( m_renderEngine == RENDER_ENGINE::OPENGL_LEGACY ) )
        {
            SHAPE_POLY_SET* layerPoly    = new SHAPE_POLY_SET;
            m_layers_poly[curr_layer_id] = layerPoly;
        }
    }

    if( GetFlag( FL_RENDER_PLATED_PADS_AS_PLATED ) && GetFlag( FL_USE_REALISTIC_MODE ) )
    {
        m_frontPlatedPadPolys = new SHAPE_POLY_SET;
        m_backPlatedPadPolys = new SHAPE_POLY_SET;

        m_platedPadsFront = new BVH_CONTAINER_2D;
        m_platedPadsBack = new BVH_CONTAINER_2D;

    }

    if( aStatusReporter )
        aStatusReporter->Report( _( "Create tracks and vias" ) );

    // Create tracks as objects and add it to container
    for( PCB_LAYER_ID curr_layer_id : layer_id )
    {
        wxASSERT( m_layerMap.find( curr_layer_id ) != m_layerMap.end() );

        BVH_CONTAINER_2D *layerContainer = m_layerMap[curr_layer_id];

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
            createTrack( track, layerContainer, 0.0f );
        }
    }

    // Create VIAS and THTs objects and add it to holes containers
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
                const float   holediameter      = via->GetDrillValue() * BiuTo3dUnits();

                // holes and layer copper extend half info cylinder wall to hide transition
                const float   thickness         = GetHolePlatingThickness() * BiuTo3dUnits() / 2.0f;
                const float   hole_inner_radius = ( holediameter / 2.0f );
                const float   ring_radius       = via->GetWidth() * BiuTo3dUnits() / 2.0f;

                const SFVEC2F via_center( via->GetStart().x * m_biuTo3Dunits,
                                          -via->GetStart().y * m_biuTo3Dunits );

                if( viatype != VIATYPE::THROUGH )
                {

                    // Add hole objects
                    BVH_CONTAINER_2D *layerHoleContainer = nullptr;

                    // Check if the layer is already created
                    if( m_layerHoleMap.find( curr_layer_id ) == m_layerHoleMap.end() )
                    {
                        // not found, create a new container
                        layerHoleContainer = new BVH_CONTAINER_2D;
                        m_layerHoleMap[curr_layer_id] = layerHoleContainer;
                    }
                    else
                    {
                        // found
                        layerHoleContainer = m_layerHoleMap[curr_layer_id];
                    }

                    // Add a hole for this layer
                    layerHoleContainer->Add( new FILLED_CIRCLE_2D( via_center,
                                                                   hole_inner_radius + thickness,
                                                                   *track ) );
                }
                else if( curr_layer_id == layer_id[0] ) // it only adds once the THT holes
                {
                    // Add through hole object
                    m_throughHoleOds.Add( new FILLED_CIRCLE_2D( via_center,
                                                                     hole_inner_radius + thickness,
                                                                     *track ) );
                    m_throughHoleViaOds.Add( new FILLED_CIRCLE_2D( via_center,
                                                                   hole_inner_radius + thickness,
                                                                   *track ) );

                    if( GetFlag( FL_CLIP_SILK_ON_VIA_ANNULUS ) &&
                        GetFlag( FL_USE_REALISTIC_MODE ) )
                    {
                        m_throughHoleAnnularRings.Add( new FILLED_CIRCLE_2D( via_center,
                                                                              ring_radius,
                                                                              *track ) );
                    }

                    m_throughHoleIds.Add( new FILLED_CIRCLE_2D( via_center, hole_inner_radius,
                                                                     *track ) );
                }
            }
        }
    }

    // Create VIAS and THTs objects and add it to holes containers
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
                    // Add VIA hole contours

                    // Add outer holes of VIAs
                    SHAPE_POLY_SET *layerOuterHolesPoly = nullptr;
                    SHAPE_POLY_SET *layerInnerHolesPoly = nullptr;

                    // Check if the layer is already created
                    if( m_layerHoleOdPolys.find( curr_layer_id ) ==
                        m_layerHoleOdPolys.end() )
                    {
                        // not found, create a new container
                        layerOuterHolesPoly = new SHAPE_POLY_SET;
                        m_layerHoleOdPolys[curr_layer_id] = layerOuterHolesPoly;

                        wxASSERT( m_layerHoleIdPolys.find( curr_layer_id ) ==
                                  m_layerHoleIdPolys.end() );

                        layerInnerHolesPoly = new SHAPE_POLY_SET;
                        m_layerHoleIdPolys[curr_layer_id] = layerInnerHolesPoly;
                    }
                    else
                    {
                        // found
                        layerOuterHolesPoly = m_layerHoleOdPolys[curr_layer_id];

                        wxASSERT( m_layerHoleIdPolys.find( curr_layer_id ) !=
                                  m_layerHoleIdPolys.end() );

                        layerInnerHolesPoly = m_layerHoleIdPolys[curr_layer_id];
                    }

                    const int holediameter = via->GetDrillValue();
                    const int hole_outer_radius = (holediameter / 2) + GetHolePlatingThickness();

                    TransformCircleToPolygon( *layerOuterHolesPoly, via->GetStart(),
                                              hole_outer_radius, ARC_HIGH_DEF, ERROR_INSIDE );

                    TransformCircleToPolygon( *layerInnerHolesPoly, via->GetStart(),
                                              holediameter / 2, ARC_HIGH_DEF, ERROR_INSIDE );
                }
                else if( curr_layer_id == layer_id[0] ) // it only adds once the THT holes
                {
                    const int holediameter = via->GetDrillValue();
                    const int hole_outer_radius = (holediameter / 2) + GetHolePlatingThickness();
                    const int hole_outer_ring_radius = via->GetWidth() / 2.0f;

                    // Add through hole contours
                    TransformCircleToPolygon( m_throughHoleOdPolys, via->GetStart(),
                                              hole_outer_radius, ARC_HIGH_DEF, ERROR_INSIDE );

                    // Add same thing for vias only
                    TransformCircleToPolygon( m_throughHoleViaOdPolys, via->GetStart(),
                                              hole_outer_radius, ARC_HIGH_DEF, ERROR_INSIDE );

                    if( GetFlag( FL_CLIP_SILK_ON_VIA_ANNULUS ) && GetFlag( FL_USE_REALISTIC_MODE ) )
                    {
                        TransformCircleToPolygon( m_throughHoleAnnularRingPolys,
                                                  via->GetStart(), hole_outer_ring_radius,
                                                  ARC_HIGH_DEF, ERROR_INSIDE );
                    }
                }
            }
        }
    }

    // Creates vertical outline contours of the tracks and add it to the poly of the layer
    if( GetFlag( FL_RENDER_OPENGL_COPPER_THICKNESS )
      && ( m_renderEngine == RENDER_ENGINE::OPENGL_LEGACY ) )
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
    for( FOOTPRINT* footprint : m_board->Footprints() )
    {
        for( PAD* pad : footprint->Pads() )
        {
            const wxSize padHole = pad->GetDrillSize();

            if( !padHole.x )    // Not drilled pad like SMD pad
                continue;

            // The hole in the body is inflated by copper thickness, if not plated, no copper
            const int inflate = ( pad->GetAttribute () != PAD_ATTRIB::NPTH ) ?
                                GetHolePlatingThickness() / 2 : 0;

            m_holeCount++;
            m_averageHoleDiameter += ( ( pad->GetDrillSize().x +
                                             pad->GetDrillSize().y ) / 2.0f ) * m_biuTo3Dunits;

            m_throughHoleOds.Add( createPadWithDrill( pad, inflate ) );

            if( GetFlag( FL_CLIP_SILK_ON_VIA_ANNULUS ) && GetFlag( FL_USE_REALISTIC_MODE ) )
            {
                m_throughHoleAnnularRings.Add( createPadWithDrill( pad, inflate ) );
            }

            m_throughHoleIds.Add( createPadWithDrill( pad, 0 ) );
        }
    }

    if( m_holeCount )
        m_averageHoleDiameter /= (float)m_holeCount;

    // Add contours of the pad holes (pads can be Circle or Segment holes)
    for( FOOTPRINT* footprint : m_board->Footprints() )
    {
        for( PAD* pad : footprint->Pads() )
        {
            const wxSize padHole = pad->GetDrillSize();

            if( !padHole.x ) // Not drilled pad like SMD pad
                continue;

            // The hole in the body is inflated by copper thickness.
            const int inflate = GetHolePlatingThickness();

            if( pad->GetAttribute () != PAD_ATTRIB::NPTH )
            {
                if( GetFlag( FL_CLIP_SILK_ON_VIA_ANNULUS ) && GetFlag( FL_USE_REALISTIC_MODE ) )
                {
                    pad->TransformHoleWithClearanceToPolygon( m_throughHoleAnnularRingPolys,
                                                              inflate, ARC_HIGH_DEF, ERROR_INSIDE );
                }

                pad->TransformHoleWithClearanceToPolygon( m_throughHoleOdPolys, inflate,
                                                          ARC_HIGH_DEF, ERROR_INSIDE );
            }
            else
            {
                // If not plated, no copper.
                if( GetFlag( FL_CLIP_SILK_ON_VIA_ANNULUS ) && GetFlag( FL_USE_REALISTIC_MODE ) )
                {
                    pad->TransformHoleWithClearanceToPolygon( m_throughHoleAnnularRingPolys, 0,
                                                              ARC_HIGH_DEF, ERROR_INSIDE );
                }

                pad->TransformHoleWithClearanceToPolygon( m_nonPlatedThroughHoleOdPolys, 0,
                                                          ARC_HIGH_DEF, ERROR_INSIDE );
            }
        }
    }

    const bool renderPlatedPadsAsPlated = GetFlag( FL_RENDER_PLATED_PADS_AS_PLATED ) &&
                                          GetFlag( FL_USE_REALISTIC_MODE );

    // Add footprints PADs objects to containers
    for( PCB_LAYER_ID curr_layer_id : layer_id )
    {
        wxASSERT( m_layerMap.find( curr_layer_id ) != m_layerMap.end() );

        BVH_CONTAINER_2D *layerContainer = m_layerMap[curr_layer_id];

        // ADD PADS
        for( FOOTPRINT* footprint : m_board->Footprints() )
        {
            // Note: NPTH pads are not drawn on copper layers when the pad
            // has same shape as its hole
            addPadsWithClearance( footprint, layerContainer, curr_layer_id, 0,
                                  true, renderPlatedPadsAsPlated, false );

            // Micro-wave footprints may have items on copper layers
            addFootprintShapesWithClearance( footprint, layerContainer, curr_layer_id, 0 );
        }
    }

    if( renderPlatedPadsAsPlated )
    {
        // ADD PLATED PADS
        for( FOOTPRINT* footprint : m_board->Footprints() )
        {
            addPadsWithClearance( footprint, m_platedPadsFront, F_Cu, 0, true, false, true );

            addPadsWithClearance( footprint, m_platedPadsBack, B_Cu, 0, true, false, true );
        }

        m_platedPadsFront->BuildBVH();
        m_platedPadsBack->BuildBVH();
    }

    // Add footprints PADs poly contours (vertical outlines)
    if( GetFlag( FL_RENDER_OPENGL_COPPER_THICKNESS )
      && ( m_renderEngine == RENDER_ENGINE::OPENGL_LEGACY ) )
    {
        for( PCB_LAYER_ID curr_layer_id : layer_id )
        {
            wxASSERT( m_layers_poly.find( curr_layer_id ) != m_layers_poly.end() );

            SHAPE_POLY_SET *layerPoly = m_layers_poly[curr_layer_id];

            // Add pads to polygon list
            for( FOOTPRINT* footprint : m_board->Footprints() )
            {
                // Note: NPTH pads are not drawn on copper layers when the pad
                // has same shape as its hole
                footprint->TransformPadsWithClearanceToPolygon( *layerPoly, curr_layer_id,
                                                                0, ARC_HIGH_DEF, ERROR_INSIDE,
                                                                true, renderPlatedPadsAsPlated,
                                                                false );

                transformFPShapesToPolygon( footprint, curr_layer_id, *layerPoly );
            }
        }

        if( renderPlatedPadsAsPlated )
        {
            // ADD PLATED PADS contours
            for( FOOTPRINT* footprint : m_board->Footprints() )
            {
                footprint->TransformPadsWithClearanceToPolygon( *m_frontPlatedPadPolys, F_Cu,
                                                                0, ARC_HIGH_DEF, ERROR_INSIDE,
                                                                true, false, true );

                footprint->TransformPadsWithClearanceToPolygon( *m_backPlatedPadPolys, B_Cu,
                                                                0, ARC_HIGH_DEF, ERROR_INSIDE,
                                                                true, false, true );
            }
        }
    }

    // Add graphic item on copper layers to object containers
    for( PCB_LAYER_ID curr_layer_id : layer_id )
    {
        wxASSERT( m_layerMap.find( curr_layer_id ) != m_layerMap.end() );

        BVH_CONTAINER_2D *layerContainer = m_layerMap[curr_layer_id];

        // Add graphic items on copper layers (texts and other graphics)
        for( BOARD_ITEM* item : m_board->Drawings() )
        {
            if( !item->IsOnLayer( curr_layer_id ) )
                continue;

            switch( item->Type() )
            {
            case PCB_SHAPE_T:
                addShapeWithClearance( static_cast<PCB_SHAPE*>( item ), layerContainer,
                                       curr_layer_id, 0 );
            break;

            case PCB_TEXT_T:
                addShapeWithClearance( static_cast<PCB_TEXT*>( item ), layerContainer,
                                       curr_layer_id, 0 );
            break;

            case PCB_DIM_ALIGNED_T:
            case PCB_DIM_CENTER_T:
            case PCB_DIM_ORTHOGONAL_T:
            case PCB_DIM_LEADER_T:
                addShapeWithClearance( static_cast<DIMENSION_BASE*>( item ),
                                       layerContainer, curr_layer_id, 0 );
            break;

            default:
                wxLogTrace( m_logTrace, wxT( "createLayers: item type: %d not implemented" ),
                            item->Type() );
            break;
            }
        }
    }

    // Add graphic item on copper layers to poly contours (vertical outlines)
    if( GetFlag( FL_RENDER_OPENGL_COPPER_THICKNESS )
      && ( m_renderEngine == RENDER_ENGINE::OPENGL_LEGACY ) )
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
                    item->TransformShapeWithClearanceToPolygon( *layerPoly, cur_layer_id, 0,
                                                                ARC_HIGH_DEF, ERROR_INSIDE );
                    break;

                case PCB_TEXT_T:
                {
                    PCB_TEXT* text = static_cast<PCB_TEXT*>( item );

                    text->TransformTextShapeWithClearanceToPolygon( *layerPoly, cur_layer_id, 0,
                                                                    ARC_HIGH_DEF, ERROR_INSIDE );
                }
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

                    auto layerContainer = m_layerMap.find( layer );

                    if( layerContainer != m_layerMap.end() )
                        addSolidAreasShapes( zone, layerContainer->second, layer );
                }

                threadsFinished++;
            } );

            t.detach();
        }

        while( threadsFinished < parallelThreadCount )
            std::this_thread::sleep_for( std::chrono::milliseconds( 10 ) );

    }

    if( GetFlag( FL_ZONE ) && GetFlag( FL_RENDER_OPENGL_COPPER_THICKNESS )
      && ( m_renderEngine == RENDER_ENGINE::OPENGL_LEGACY ) )
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
      && ( m_renderEngine == RENDER_ENGINE::OPENGL_LEGACY ) )
    {
        if( GetFlag( FL_RENDER_PLATED_PADS_AS_PLATED ) && GetFlag( FL_USE_REALISTIC_MODE ) )
        {
            if( m_frontPlatedPadPolys && ( m_layers_poly.find( F_Cu ) != m_layers_poly.end() ) )
            {
                SHAPE_POLY_SET *layerPoly_F_Cu = m_layers_poly[F_Cu];
                layerPoly_F_Cu->BooleanSubtract( *m_frontPlatedPadPolys, SHAPE_POLY_SET::PM_FAST );

                m_frontPlatedPadPolys->Simplify( SHAPE_POLY_SET::PM_FAST );
            }

            if( m_backPlatedPadPolys && ( m_layers_poly.find( B_Cu ) != m_layers_poly.end() ) )
            {
                SHAPE_POLY_SET *layerPoly_B_Cu = m_layers_poly[B_Cu];
                layerPoly_B_Cu->BooleanSubtract( *m_backPlatedPadPolys, SHAPE_POLY_SET::PM_FAST );

                m_backPlatedPadPolys->Simplify( SHAPE_POLY_SET::PM_FAST );
            }
        }

        std::vector< PCB_LAYER_ID > &selected_layer_id = layer_id;
        std::vector< PCB_LAYER_ID > layer_id_without_F_and_B;

        if( GetFlag( FL_RENDER_PLATED_PADS_AS_PLATED ) && GetFlag( FL_USE_REALISTIC_MODE ) )
        {
            layer_id_without_F_and_B.clear();
            layer_id_without_F_and_B.reserve( layer_id.size() );

            for( size_t i = 0; i < layer_id.size(); ++i )
            {
                if( ( layer_id[i] != F_Cu ) && ( layer_id[i] != B_Cu ) )
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
                std::thread t = std::thread(
                        [&nextItem, &threadsFinished, &selected_layer_id, this]()
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
    if( aStatusReporter )
        aStatusReporter->Report( _( "Simplify holes contours" ) );

    for( PCB_LAYER_ID layer : layer_id )
    {
        if( m_layerHoleOdPolys.find( layer ) != m_layerHoleOdPolys.end() )
        {
            // found
            SHAPE_POLY_SET *polyLayer = m_layerHoleOdPolys[layer];
            polyLayer->Simplify( SHAPE_POLY_SET::PM_FAST );

            wxASSERT( m_layerHoleIdPolys.find( layer ) != m_layerHoleIdPolys.end() );

            polyLayer = m_layerHoleIdPolys[layer];
            polyLayer->Simplify( SHAPE_POLY_SET::PM_FAST );
        }
    }

    // End Build Copper layers

    // This will make a union of all added contours
    m_throughHoleOdPolys.Simplify( SHAPE_POLY_SET::PM_FAST );
    m_nonPlatedThroughHoleOdPolys.Simplify( SHAPE_POLY_SET::PM_FAST );
    m_throughHoleViaOdPolys.Simplify( SHAPE_POLY_SET::PM_FAST );
    m_throughHoleAnnularRingPolys.Simplify( SHAPE_POLY_SET::PM_FAST );

    // Build Tech layers
    // Based on:
    //    https://github.com/KiCad/kicad-source-mirror/blob/master/3d-viewer/3d_draw.cpp#L1059
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

        if( !Is3dLayerEnabled( curr_layer_id ) )
            continue;

        BVH_CONTAINER_2D *layerContainer = new BVH_CONTAINER_2D;
        m_layerMap[curr_layer_id] = layerContainer;

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
                addShapeWithClearance( static_cast<PCB_SHAPE*>( item ), layerContainer,
                                       curr_layer_id, 0 );
                break;

            case PCB_TEXT_T:
                addShapeWithClearance( static_cast<PCB_TEXT*>( item ), layerContainer,
                                       curr_layer_id, 0 );
                break;

            case PCB_DIM_ALIGNED_T:
            case PCB_DIM_CENTER_T:
            case PCB_DIM_ORTHOGONAL_T:
            case PCB_DIM_LEADER_T:
                addShapeWithClearance( static_cast<DIMENSION_BASE*>( item ), layerContainer,
                                       curr_layer_id, 0 );
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
                item->TransformShapeWithClearanceToPolygon( *layerPoly, curr_layer_id, 0,
                                                            ARC_HIGH_DEF, ERROR_INSIDE );
                break;

            case PCB_TEXT_T:
            {
                PCB_TEXT* text = static_cast<PCB_TEXT*>( item );

                text->TransformTextShapeWithClearanceToPolygon( *layerPoly, curr_layer_id, 0,
                                                                ARC_HIGH_DEF, ERROR_INSIDE );
            }
                break;

            default:
                break;
            }
        }

        // Add footprints tech layers - objects
        for( FOOTPRINT* footprint : m_board->Footprints() )
        {
            if( ( curr_layer_id == F_SilkS ) || ( curr_layer_id == B_SilkS ) )
            {
                int linewidth = m_board->GetDesignSettings().m_LineThickness[ LAYER_CLASS_SILK ];

                for( PAD* pad : footprint->Pads() )
                {
                    if( !pad->IsOnLayer( curr_layer_id ) )
                        continue;

                    buildPadOutlineAsSegments( pad, layerContainer, linewidth );
                }
            }
            else
            {
                addPadsWithClearance( footprint, layerContainer, curr_layer_id, 0,
                                      false, false, false );
            }

            addFootprintShapesWithClearance( footprint, layerContainer, curr_layer_id, 0 );
        }


        // Add footprints tech layers - contours
        for( FOOTPRINT* footprint : m_board->Footprints() )
        {
            if( ( curr_layer_id == F_SilkS ) || ( curr_layer_id == B_SilkS ) )
            {
                int linewidth = m_board->GetDesignSettings().m_LineThickness[ LAYER_CLASS_SILK ];

                for( PAD* pad : footprint->Pads() )
                {
                    if( !pad->IsOnLayer( curr_layer_id ) )
                        continue;

                    buildPadOutlineAsPolygon( pad, *layerPoly, linewidth );
                }
            }
            else
            {
                footprint->TransformPadsWithClearanceToPolygon( *layerPoly, curr_layer_id, 0,
                                                                ARC_HIGH_DEF, ERROR_INSIDE );
            }

            // On tech layers, use a poor circle approximation, only for texts (stroke font)
            footprint->TransformFPTextWithClearanceToPolygonSet( *layerPoly, curr_layer_id, 0,
                                                                 ARC_HIGH_DEF, ERROR_INSIDE );

            // Add the remaining things with dynamic seg count for circles
            transformFPShapesToPolygon( footprint, curr_layer_id, *layerPoly );
        }


        // Draw non copper zones
        if( GetFlag( FL_ZONE ) )
        {
            for( ZONE* zone : m_board->Zones() )
            {
                if( zone->IsOnLayer( curr_layer_id ) )
                    addSolidAreasShapes( zone, layerContainer, curr_layer_id );
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

    m_throughHoleIds.BuildBVH();
    m_throughHoleOds.BuildBVH();
    m_throughHoleAnnularRings.BuildBVH();

    if( !m_layerHoleMap.empty() )
    {
        for( auto& hole : m_layerHoleMap )
            hole.second->BuildBVH();
    }

    // We only need the Solder mask to initialize the BVH
    // because..?
    if( m_layerMap[B_Mask] )
        m_layerMap[B_Mask]->BuildBVH();

    if( m_layerMap[F_Mask] )
        m_layerMap[F_Mask]->BuildBVH();
}
