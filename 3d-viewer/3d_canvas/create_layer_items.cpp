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

#include "board_adapter.h"
#include "../3d_rendering/raytracing/shapes2D/filled_circle_2d.h"
#include "raytracing/shapes2D/triangle_2d.h"
#include <board_design_settings.h>
#include <board.h>
#include <layer_range.h>
#include <lset.h>
#include <convert_basic_shapes_to_polygon.h>
#include <trigo.h>
#include <vector>
#include <thread>
#include <algorithm>
#include <atomic>
#include <wx/log.h>
#include <pcb_barcode.h>

#ifdef PRINT_STATISTICS_3D_VIEWER
#include <core/profile.h>
#endif

/*
 * This is used to draw pad outlines on silk layers.
 */
void buildPadOutlineAsPolygon( const PAD* aPad, PCB_LAYER_ID aLayer, SHAPE_POLY_SET& aBuffer,
                               int aWidth, int aMaxError, ERROR_LOC aErrorLoc )
{
    if( aPad->GetShape( aLayer ) == PAD_SHAPE::CIRCLE )    // Draw a ring
    {
        TransformRingToPolygon( aBuffer, aPad->ShapePos( aLayer ), aPad->GetSize( aLayer ).x / 2,
                                aWidth, aMaxError, aErrorLoc );
    }
    else
    {
        // For other shapes, add outlines as thick segments in polygon buffer
        const SHAPE_LINE_CHAIN& path = aPad->GetEffectivePolygon( aLayer, ERROR_INSIDE )->COutline( 0 );

        for( int ii = 0; ii < path.PointCount(); ++ii )
        {
            const VECTOR2I& a = path.CPoint( ii );
            const VECTOR2I& b = path.CPoint( ii + 1 );

            TransformOvalToPolygon( aBuffer, a, b, aWidth, aMaxError, aErrorLoc );
        }
    }
}


void transformFPShapesToPolySet( const FOOTPRINT* aFootprint, PCB_LAYER_ID aLayer,
                                 SHAPE_POLY_SET& aBuffer, int aMaxError, ERROR_LOC aErrorLoc )
{
    for( BOARD_ITEM* item : aFootprint->GraphicalItems() )
    {
        if( item->Type() == PCB_SHAPE_T
                || item->Type() == PCB_BARCODE_T
                || BaseType( item->Type() ) == PCB_DIMENSION_T )
        {
            if( item->GetLayer() == aLayer )
                item->TransformShapeToPolySet( aBuffer, aLayer, 0, aMaxError, aErrorLoc );
        }
    }
}


void transformFPTextToPolySet( const FOOTPRINT* aFootprint, PCB_LAYER_ID aLayer,
                               const std::bitset<LAYER_3D_END>& aFlags, SHAPE_POLY_SET& aBuffer,
                               int aMaxError, ERROR_LOC aErrorLoc )
{
    for( BOARD_ITEM* item : aFootprint->GraphicalItems() )
    {
        if( item->GetLayer() != aLayer )
            continue;

        if( item->Type() == PCB_TEXT_T )
        {
            PCB_TEXT* text = static_cast<PCB_TEXT*>( item );

            if( !aFlags.test( LAYER_FP_TEXT ) )
                continue;

            if( text->GetText() == wxT( "${REFERENCE}" ) && !aFlags.test( LAYER_FP_REFERENCES ) )
                continue;

            if( text->GetText() == wxT( "${VALUE}" ) && !aFlags.test( LAYER_FP_VALUES ) )
                continue;

            if( aLayer != UNDEFINED_LAYER && text->GetLayer() == aLayer )
                text->TransformTextToPolySet( aBuffer, 0, aMaxError, aErrorLoc );
        }

        if( item->Type() == PCB_TEXTBOX_T )
        {
            PCB_TEXTBOX* textbox = static_cast<PCB_TEXTBOX*>( item );

            if( aLayer != UNDEFINED_LAYER && textbox->GetLayer() == aLayer )
            {
                // border
                if( textbox->IsBorderEnabled() )
                    textbox->PCB_SHAPE::TransformShapeToPolygon( aBuffer, aLayer, 0, aMaxError, aErrorLoc );

                // text
                textbox->TransformTextToPolySet( aBuffer, 0, aMaxError, aErrorLoc );
            }
        }
    }

    for( const PCB_FIELD* field : aFootprint->GetFields() )
    {
        if( !aFlags.test( LAYER_FP_TEXT ) )
            continue;

        wxCHECK2( field, continue );

        if( field->IsReference() && !aFlags.test( LAYER_FP_REFERENCES ) )
            continue;

        if( field->IsValue() && !aFlags.test( LAYER_FP_VALUES ) )
            continue;

        if(  field->GetLayer() == aLayer && field->IsVisible() )
            field->TransformTextToPolySet( aBuffer, 0, aMaxError, aErrorLoc );
    }
}


void BOARD_ADAPTER::destroyLayers()
{
#define DELETE_AND_FREE( ptr ) \
    {                          \
        delete ptr;            \
        ptr = nullptr;         \
    }                          \

#define DELETE_AND_FREE_MAP( map )         \
    {                                      \
        for( auto& [ layer, poly ] : map ) \
            delete poly;                   \
                                           \
        map.clear();                       \
    }

    DELETE_AND_FREE_MAP( m_layers_poly );

    DELETE_AND_FREE( m_frontPlatedCopperPolys )
    DELETE_AND_FREE( m_backPlatedCopperPolys )

    DELETE_AND_FREE_MAP( m_layerHoleOdPolys )
    DELETE_AND_FREE_MAP( m_layerHoleIdPolys )

    m_NPTH_ODPolys.RemoveAllContours();
    m_TH_ODPolys.RemoveAllContours();
    m_viaTH_ODPolys.RemoveAllContours();
    m_viaAnnuliPolys.RemoveAllContours();

    m_frontCounterborePolys.RemoveAllContours();
    m_backCounterborePolys.RemoveAllContours();
    m_frontCountersinkPolys.RemoveAllContours();
    m_backCountersinkPolys.RemoveAllContours();

    DELETE_AND_FREE_MAP( m_layerMap )
    DELETE_AND_FREE_MAP( m_layerHoleMap )

    DELETE_AND_FREE( m_platedPadsFront )
    DELETE_AND_FREE( m_platedPadsBack )
    DELETE_AND_FREE( m_offboardPadsFront )
    DELETE_AND_FREE( m_offboardPadsBack )

    m_TH_ODs.Clear();
    m_TH_IDs.Clear();
    m_viaAnnuli.Clear();
    m_viaTH_ODs.Clear();

    m_frontCounterboreCutouts.Clear();
    m_backCounterboreCutouts.Clear();
    m_frontCountersinkCutouts.Clear();
    m_backCountersinkCutouts.Clear();
    m_backdrillCutouts.Clear();
    m_tertiarydrillCutouts.Clear();
}


void BOARD_ADAPTER::createLayers( REPORTER* aStatusReporter )
{
    destroyLayers();

    // Build Copper layers
    // Based on:
    //    https://github.com/KiCad/kicad-source-mirror/blob/master/3d-viewer/3d_draw.cpp#L692

#ifdef PRINT_STATISTICS_3D_VIEWER
    int64_t stats_startCopperLayersTime = GetRunningMicroSecs();

    int64_t start_Time = stats_startCopperLayersTime;
#endif

    EDA_3D_VIEWER_SETTINGS::RENDER_SETTINGS& cfg = m_Cfg->m_Render;

    std::bitset<LAYER_3D_END> visibilityFlags = GetVisibleLayers();

    m_trackCount               = 0;
    m_averageTrackWidth        = 0;
    m_viaCount                 = 0;
    m_averageViaHoleDiameter   = 0;
    m_holeCount                = 0;
    m_averageHoleDiameter      = 0;

    if( !m_board )
        return;

    // Prepare track list, convert in a vector. Calc statistic for the holes
    std::vector<const PCB_TRACK*> trackList;
    trackList.clear();
    trackList.reserve( m_board->Tracks().size() );

    for( PCB_TRACK* track : m_board->Tracks() )
    {
         // Skip tracks (not vias theyt are on more than one layer ) on disabled layers
        if( track->Type() != PCB_VIA_T && !Is3dLayerEnabled( track->GetLayer(), visibilityFlags ) )
        {
            continue;
        }

        // Note: a PCB_TRACK holds normal segment tracks and also vias circles (that have also
        // drill values)
        trackList.push_back( track );

        if( track->Type() == PCB_VIA_T )
        {
            const PCB_VIA *via = static_cast< const PCB_VIA*>( track );
            m_viaCount++;
            m_averageViaHoleDiameter += static_cast<float>( via->GetDrillValue() * m_biuTo3Dunits );
        }
        else
        {
            m_trackCount++;
            m_averageTrackWidth += static_cast<float>( track->GetWidth() * m_biuTo3Dunits );
        }
    }

    if( m_trackCount )
        m_averageTrackWidth /= (float)m_trackCount;

    if( m_viaCount )
        m_averageViaHoleDiameter /= (float)m_viaCount;

    // Prepare copper layers index and containers
    std::vector<PCB_LAYER_ID> layer_ids;
    layer_ids.clear();
    layer_ids.reserve( m_copperLayersCount );

    for( PCB_LAYER_ID layer : LAYER_RANGE( F_Cu,B_Cu, m_copperLayersCount) )
    {
        if( !Is3dLayerEnabled( layer, visibilityFlags ) ) // Skip non enabled layers
            continue;

        layer_ids.push_back( layer );

        BVH_CONTAINER_2D *layerContainer = new BVH_CONTAINER_2D;
        m_layerMap[layer] = layerContainer;

        if( cfg.opengl_copper_thickness && cfg.engine == RENDER_ENGINE::OPENGL )
        {
            SHAPE_POLY_SET* layerPoly = new SHAPE_POLY_SET;
            m_layers_poly[layer] = layerPoly;
        }
    }

    if( cfg.DifferentiatePlatedCopper() )
    {
        m_frontPlatedCopperPolys = new SHAPE_POLY_SET;
        m_backPlatedCopperPolys = new SHAPE_POLY_SET;

        m_platedPadsFront = new BVH_CONTAINER_2D;
        m_platedPadsBack = new BVH_CONTAINER_2D;
    }

    if( cfg.show_off_board_silk )
    {
        m_offboardPadsFront = new BVH_CONTAINER_2D;
        m_offboardPadsBack = new BVH_CONTAINER_2D;
    }

    if( aStatusReporter )
        aStatusReporter->Report( _( "Create tracks and vias" ) );

    // Create tracks as objects and add it to container
    for( PCB_LAYER_ID layer : layer_ids )
    {
        wxASSERT( m_layerMap.contains( layer ) );

        BVH_CONTAINER_2D *layerContainer = m_layerMap[layer];

        for( const PCB_TRACK* track : trackList )
        {
            // NOTE: Vias can be on multiple layers
            if( !track->IsOnLayer( layer ) )
                continue;

            // Skip vias annulus when not flashed on this layer
            if( track->Type() == PCB_VIA_T && !static_cast<const PCB_VIA*>( track )->FlashLayer( layer ) )
                continue;

            // Add object item to layer container
            createTrackWithMargin( track, layerContainer, layer );
        }
    }

    // Create VIAS and THTs objects and add it to holes containers
    for( PCB_LAYER_ID layer : layer_ids )
    {
        // Check if the layer is already created
        if( !m_layerHoleMap.contains( layer ) )
        {
            m_layerHoleMap[layer] = new BVH_CONTAINER_2D;
        }

        // ADD TRACKS
        unsigned int nTracks = trackList.size();

        for( unsigned int trackIdx = 0; trackIdx < nTracks; ++trackIdx )
        {
            const PCB_TRACK *track = trackList[trackIdx];

            if( !track->IsOnLayer( layer ) )
                continue;

            // ADD VIAS and THT
            if( track->Type() == PCB_VIA_T )
            {
                const PCB_VIA* via               = static_cast<const PCB_VIA*>( track );
                const VIATYPE  viatype           = via->GetViaType();
                const double   holediameter      = via->GetDrillValue() * BiuTo3dUnits();
                const double   viasize           = via->GetWidth( layer ) * BiuTo3dUnits();
                const double   plating           = GetHolePlatingThickness() * BiuTo3dUnits();

                // holes and layer copper extend half info cylinder wall to hide transition
                const float    thickness         = static_cast<float>( plating / 2.0f );
                const float    hole_inner_radius = static_cast<float>( holediameter / 2.0f );
                const float    ring_radius       = static_cast<float>( viasize / 2.0f );

                const SFVEC2F via_center( via->GetStart().x * m_biuTo3Dunits,
                                          -via->GetStart().y * m_biuTo3Dunits );

                if( viatype != VIATYPE::THROUGH )
                {
                    // Add hole objects
                    BVH_CONTAINER_2D *layerHoleContainer = nullptr;
                    layerHoleContainer = m_layerHoleMap[layer];

                    // Add a hole for this layer
                    layerHoleContainer->Add( new FILLED_CIRCLE_2D( via_center, hole_inner_radius + thickness,
                                                                   *track ) );
                }
                else if( layer == layer_ids[0] ) // it only adds once the THT holes
                {
                    // Add through hole object
                    m_TH_ODs.Add( new FILLED_CIRCLE_2D( via_center, hole_inner_radius + thickness, *track ) );
                    m_viaTH_ODs.Add( new FILLED_CIRCLE_2D( via_center, hole_inner_radius + thickness, *track ) );

                    if( cfg.clip_silk_on_via_annuli && ring_radius > 0.0 )
                        m_viaAnnuli.Add( new FILLED_CIRCLE_2D( via_center, ring_radius, *track ) );

                    if( hole_inner_radius > 0.0 )
                        m_TH_IDs.Add( new FILLED_CIRCLE_2D( via_center, hole_inner_radius, *track ) );
                }

                // Add counterbore/countersink cutouts for vias
                const auto frontMode = via->GetFrontPostMachining().value_or( PAD_DRILL_POST_MACHINING_MODE::UNKNOWN );
                const double frontRadius = via->GetFrontPostMachiningSize() * m_biuTo3Dunits / 2.0;

                if( frontMode != PAD_DRILL_POST_MACHINING_MODE::NOT_POST_MACHINED
                    && frontMode != PAD_DRILL_POST_MACHINING_MODE::UNKNOWN
                    && frontRadius > hole_inner_radius )
                {
                    if( layer == layer_ids[0] ) // only add once for front layer
                    {
                        if( frontMode == PAD_DRILL_POST_MACHINING_MODE::COUNTERBORE )
                            m_frontCounterboreCutouts.Add( new FILLED_CIRCLE_2D( via_center, frontRadius, *track ) );
                        else if( frontMode == PAD_DRILL_POST_MACHINING_MODE::COUNTERSINK )
                            m_frontCountersinkCutouts.Add( new FILLED_CIRCLE_2D( via_center, frontRadius, *track ) );
                    }

                    BVH_CONTAINER_2D *layerHoleContainer = m_layerHoleMap[layer];
                    layerHoleContainer->Add( new FILLED_CIRCLE_2D( via_center, frontRadius + thickness, *track ) );
                }

                const auto backMode = via->GetBackPostMachining().value_or( PAD_DRILL_POST_MACHINING_MODE::UNKNOWN );
                const double backRadius = via->GetBackPostMachiningSize() * m_biuTo3Dunits / 2.0;

                if( backMode != PAD_DRILL_POST_MACHINING_MODE::NOT_POST_MACHINED
                    && backMode != PAD_DRILL_POST_MACHINING_MODE::UNKNOWN
                    && backRadius > hole_inner_radius )
                {

                    if( layer == layer_ids.back() ) // only add once for back layer
                    {
                        if( backMode == PAD_DRILL_POST_MACHINING_MODE::COUNTERBORE )
                            m_backCounterboreCutouts.Add( new FILLED_CIRCLE_2D( via_center, backRadius, *track ) );
                        else if( backMode == PAD_DRILL_POST_MACHINING_MODE::COUNTERSINK )
                            m_backCountersinkCutouts.Add( new FILLED_CIRCLE_2D( via_center, backRadius, *track ) );
                    }

                    BVH_CONTAINER_2D *layerHoleContainer = m_layerHoleMap[layer];
                    layerHoleContainer->Add( new FILLED_CIRCLE_2D( via_center, backRadius + thickness, *track ) );
                }

                // Add backdrill cutouts for vias - affects specific layers only
                float backdrillRadius = via->GetSecondaryDrillSize().value_or( 0 ) * m_biuTo3Dunits / 2.0f ;

                // Only add if backdrill is larger than original hole
                if( backdrillRadius > hole_inner_radius + thickness )
                {
                    PCB_LAYER_ID secStart = via->GetSecondaryDrillStartLayer();
                    PCB_LAYER_ID secEnd = via->GetSecondaryDrillEndLayer();

                    if( LAYER_RANGE( secStart, secEnd, m_copperLayersCount ).Contains( layer ) )
                    {
                        // Add to layer hole map for this layer
                        BVH_CONTAINER_2D* layerHoleContainer = m_layerHoleMap[layer];

                        layerHoleContainer->Add( new FILLED_CIRCLE_2D( via_center, backdrillRadius, *track ) );
                    }

                    if( layer == secStart )
                        m_backdrillCutouts.Add( new FILLED_CIRCLE_2D( via_center, backdrillRadius, *track ) );
                }

                // Add tertiary drill cutouts for vias - affects specific layers only
                float tertiaryDrillRadius = via->GetTertiaryDrillSize().value_or( 0 ) * m_biuTo3Dunits / 2.0f ;

                // Only add if tertiary drill is larger than original hole
                if( tertiaryDrillRadius > hole_inner_radius + thickness )
                {
                    PCB_LAYER_ID terStart = via->GetTertiaryDrillStartLayer();
                    PCB_LAYER_ID terEnd = via->GetTertiaryDrillEndLayer();

                    if( LAYER_RANGE( terStart, terEnd, m_copperLayersCount ).Contains( layer ) )
                    {
                        // Add to layer hole map for this layer
                        BVH_CONTAINER_2D* layerHoleContainer = m_layerHoleMap[layer];

                        layerHoleContainer->Add( new FILLED_CIRCLE_2D( via_center, tertiaryDrillRadius, *track ) );
                    }

                    if( layer == terStart )
                        m_tertiarydrillCutouts.Add( new FILLED_CIRCLE_2D( via_center, tertiaryDrillRadius, *track ) );
                }
            }

            if( cfg.DifferentiatePlatedCopper() && layer == F_Cu )
            {
                track->TransformShapeToPolygon( *m_frontPlatedCopperPolys, F_Cu, 0, track->GetMaxError(),
                                                ERROR_INSIDE );
            }
            else if( cfg.DifferentiatePlatedCopper() && layer == B_Cu )
            {
                track->TransformShapeToPolygon( *m_backPlatedCopperPolys, B_Cu, 0, track->GetMaxError(),
                                                ERROR_INSIDE );
            }
        }
    }

    // Create VIAS and THTs objects and add it to holes containers
    for( PCB_LAYER_ID layer : layer_ids )
    {
        if( !m_layerHoleOdPolys.contains( layer ) )
        {
            m_layerHoleOdPolys[layer] = new SHAPE_POLY_SET;
        }

        if( !m_layerHoleIdPolys.contains( layer ) )
        {
            m_layerHoleIdPolys[layer] = new SHAPE_POLY_SET;
        }

        // ADD TRACKS
        const unsigned int nTracks = trackList.size();

        for( unsigned int trackIdx = 0; trackIdx < nTracks; ++trackIdx )
        {
            const PCB_TRACK *track = trackList[trackIdx];

            if( !track->IsOnLayer( layer ) )
                continue;

            // ADD VIAS and THT
            if( track->Type() == PCB_VIA_T )
            {
                const PCB_VIA* via = static_cast<const PCB_VIA*>( track );
                const VIATYPE  viatype = via->GetViaType();

                // Add outer holes of VIAs
                SHAPE_POLY_SET *layerOuterHolesPoly = nullptr;
                SHAPE_POLY_SET *layerInnerHolesPoly = nullptr;

                // found
                wxASSERT( m_layerHoleOdPolys.contains( layer ) );
                wxASSERT( m_layerHoleIdPolys.contains( layer ) );

                layerOuterHolesPoly = m_layerHoleOdPolys[layer];
                layerInnerHolesPoly = m_layerHoleIdPolys[layer];

                const int holediameter = via->GetDrillValue();
                const int hole_outer_radius = (holediameter / 2) + GetHolePlatingThickness();

                if( viatype != VIATYPE::THROUGH )
                {
                    // Add PCB_VIA hole contours
                    TransformCircleToPolygon( *layerOuterHolesPoly, via->GetStart(), hole_outer_radius,
                                              via->GetMaxError(), ERROR_INSIDE );

                    TransformCircleToPolygon( *layerInnerHolesPoly, via->GetStart(), holediameter / 2,
                                              via->GetMaxError(), ERROR_INSIDE );
                }
                else if( layer == layer_ids[0] ) // it only adds once the THT holes
                {
                    const int hole_outer_ring_radius = KiROUND( via->GetWidth( layer ) / 2.0 );

                    // Add through hole contours
                    TransformCircleToPolygon( m_TH_ODPolys, via->GetStart(), hole_outer_radius,
                                              via->GetMaxError(), ERROR_INSIDE );

                    // Add same thing for vias only
                    TransformCircleToPolygon( m_viaTH_ODPolys, via->GetStart(), hole_outer_radius,
                                              via->GetMaxError(), ERROR_INSIDE );

                    if( cfg.clip_silk_on_via_annuli )
                    {
                        TransformCircleToPolygon( m_viaAnnuliPolys, via->GetStart(), hole_outer_ring_radius,
                                                  via->GetMaxError(), ERROR_INSIDE );
                    }
                }

                // Add counterbore/countersink polygons for vias
                const auto frontMode = via->GetFrontPostMachining();

                if( frontMode.has_value()
                    && frontMode.value() != PAD_DRILL_POST_MACHINING_MODE::NOT_POST_MACHINED
                    && frontMode.value() != PAD_DRILL_POST_MACHINING_MODE::UNKNOWN )
                {
                    const int frontRadiusBIU = via->GetFrontPostMachiningSize() / 2;

                    if( frontRadiusBIU > holediameter / 2 )
                    {
                        int frontRadiusOuterBIU = frontRadiusBIU + GetHolePlatingThickness();

                        if( frontMode.value() == PAD_DRILL_POST_MACHINING_MODE::COUNTERBORE )
                        {
                            TransformCircleToPolygon( m_frontCounterborePolys, via->GetStart(),
                                                        frontRadiusBIU, via->GetMaxError(),
                                                        ERROR_INSIDE );
                        }
                        else if( frontMode.value() == PAD_DRILL_POST_MACHINING_MODE::COUNTERSINK )
                        {
                            TransformCircleToPolygon( m_frontCountersinkPolys, via->GetStart(),
                                                        frontRadiusBIU, via->GetMaxError(),
                                                        ERROR_INSIDE );
                        }

                        TransformCircleToPolygon( *layerOuterHolesPoly, via->GetStart(),
                                                    frontRadiusOuterBIU, via->GetMaxError(),
                                                    ERROR_INSIDE );
                        TransformCircleToPolygon( *layerInnerHolesPoly, via->GetStart(),
                                                    frontRadiusBIU, via->GetMaxError(),
                                                    ERROR_INSIDE );
                    }
                }

                const auto backMode = via->GetBackPostMachining();

                if( backMode.has_value()
                    && backMode.value() != PAD_DRILL_POST_MACHINING_MODE::NOT_POST_MACHINED
                    && backMode.value() != PAD_DRILL_POST_MACHINING_MODE::UNKNOWN )
                {
                    const int backRadiusBIU = via->GetBackPostMachiningSize() / 2;

                    if( backRadiusBIU > holediameter / 2 )
                    {
                        int backRadiusOuterBIU = backRadiusBIU + GetHolePlatingThickness();

                        if( backMode.value() == PAD_DRILL_POST_MACHINING_MODE::COUNTERBORE )
                        {
                            TransformCircleToPolygon( m_backCounterborePolys, via->GetStart(),
                                                        backRadiusBIU, via->GetMaxError(),
                                                        ERROR_INSIDE );
                        }
                        else if( backMode.value() == PAD_DRILL_POST_MACHINING_MODE::COUNTERSINK )
                        {
                            TransformCircleToPolygon( m_backCountersinkPolys, via->GetStart(),
                                                        backRadiusBIU, via->GetMaxError(),
                                                        ERROR_INSIDE );
                        }

                        TransformCircleToPolygon( *layerOuterHolesPoly, via->GetStart(),
                                                    backRadiusOuterBIU, via->GetMaxError(),
                                                    ERROR_INSIDE );
                        TransformCircleToPolygon( *layerInnerHolesPoly, via->GetStart(),
                                                    backRadiusBIU, via->GetMaxError(),
                                                    ERROR_INSIDE );
                    }
                }

                // Add backdrill polygons for vias - affects specific layers only
                const auto secondaryDrillSize = via->GetSecondaryDrillSize();

                if( secondaryDrillSize.has_value() && secondaryDrillSize.value() > 0 )
                {
                    const int backdrillRadiusBIU = secondaryDrillSize.value() / 2;
                    const int backdrillOuterRadiusBIU = backdrillRadiusBIU + GetHolePlatingThickness();
                    const int holeOuterRadiusBIU = hole_outer_radius;

                    // Only add if backdrill is larger than original hole outer diameter
                    if( backdrillOuterRadiusBIU > holeOuterRadiusBIU )
                    {
                        PCB_LAYER_ID secStart = via->GetSecondaryDrillStartLayer();
                        PCB_LAYER_ID secEnd = via->GetSecondaryDrillEndLayer();

                        // Iterate through layers affected by backdrill
                        if( LAYER_RANGE( secStart, secEnd, m_copperLayersCount ).Contains( layer ) )
                        {
                            TransformCircleToPolygon( *m_layerHoleOdPolys[layer], via->GetStart(),
                                                        backdrillOuterRadiusBIU, via->GetMaxError(),
                                                        ERROR_INSIDE );

                            TransformCircleToPolygon( *m_layerHoleIdPolys[layer], via->GetStart(),
                                                        backdrillRadiusBIU, via->GetMaxError(),
                                                        ERROR_INSIDE );

                            TransformCircleToPolygon( m_BackdrillPolys, via->GetStart(),
                                                        backdrillOuterRadiusBIU, via->GetMaxError(),
                                                        ERROR_INSIDE );
                        }
                    }
                }

                // Add backdrill polygons for vias - affects specific layers only
                const auto tertiaryDrillSize = via->GetTertiaryDrillSize();

                if( tertiaryDrillSize.has_value() && tertiaryDrillSize.value() > 0 )
                {
                    const int backdrillRadiusBIU = tertiaryDrillSize.value() / 2;
                    const int backdrillOuterRadiusBIU = backdrillRadiusBIU + GetHolePlatingThickness();
                    const int holeOuterRadiusBIU = hole_outer_radius;

                    // Only add if backdrill is larger than original hole outer diameter
                    if( backdrillOuterRadiusBIU > holeOuterRadiusBIU )
                    {
                        PCB_LAYER_ID terStart = via->GetTertiaryDrillStartLayer();
                        PCB_LAYER_ID terEnd = via->GetTertiaryDrillEndLayer();

                        if( LAYER_RANGE( terStart, terEnd, m_copperLayersCount ).Contains( layer ) )
                        {
                            PCB_LAYER_ID backdrillLayer = layer;

                            TransformCircleToPolygon( *m_layerHoleOdPolys[backdrillLayer], via->GetStart(),
                                                        backdrillOuterRadiusBIU, via->GetMaxError(),
                                                        ERROR_INSIDE );

                            TransformCircleToPolygon( *m_layerHoleIdPolys[backdrillLayer], via->GetStart(),
                                                        backdrillRadiusBIU, via->GetMaxError(),
                                                        ERROR_INSIDE );

                            TransformCircleToPolygon( m_TertiarydrillPolys, via->GetStart(),
                                                        backdrillOuterRadiusBIU, via->GetMaxError(),
                                                        ERROR_INSIDE );
                        }
                    }
                }
            }
        }
    }

    // Creates vertical outline contours of the tracks and add it to the poly of the layer
    if( cfg.opengl_copper_thickness && cfg.engine == RENDER_ENGINE::OPENGL )
    {
        for( PCB_LAYER_ID layer : layer_ids )
        {
            wxASSERT( m_layers_poly.contains( layer ) );

            SHAPE_POLY_SET *layerPoly = m_layers_poly[layer];

            // ADD TRACKS
            unsigned int nTracks = trackList.size();

            for( unsigned int trackIdx = 0; trackIdx < nTracks; ++trackIdx )
            {
                const PCB_TRACK *track = trackList[trackIdx];

                if( !track->IsOnLayer( layer ) )
                    continue;

                // Skip vias annulus when not flashed on this layer
                if( track->Type() == PCB_VIA_T && !static_cast<const PCB_VIA*>( track )->FlashLayer( layer ) )
                    continue;

                // Add the track/via contour
                track->TransformShapeToPolygon( *layerPoly, layer, 0, track->GetMaxError(), ERROR_INSIDE );
            }
        }
    }

    // Add holes of footprints
    for( FOOTPRINT* footprint : m_board->Footprints() )
    {
        for( PAD* pad : footprint->Pads() )
        {
            // Note: holes of NPTH are already built by GetBoardPolygonOutlines
            if( !pad->HasHole() )
                continue;

            m_holeCount++;
            double holeDiameter = ( pad->GetDrillSize().x + pad->GetDrillSize().y ) / 2.0;
            m_averageHoleDiameter += static_cast<float>( holeDiameter * m_biuTo3Dunits );

            if( pad->GetAttribute() == PAD_ATTRIB::NPTH )
            {
                // Ensure the silk drawings are clipped to the NPTH hole, like other pad/via holes
                // even if the clip to board body is not activated (remember NPTH holes are part of
                // the board body)
                createPadHoleShape( pad, &m_TH_ODs, 0 );
                continue;
            }

            // The hole in the body is inflated by copper thickness
            int inflate = KiROUND( GetHolePlatingThickness() / 2.0 );

            createPadHoleShape( pad, &m_TH_ODs, inflate );

            if( cfg.clip_silk_on_via_annuli )
                createPadHoleShape( pad, &m_viaAnnuli, inflate );

            createPadHoleShape( pad, &m_TH_IDs, 0 );

            // Add counterbore/countersink cutouts for pads
            const float    holeDiameterUnits = static_cast<float>(
                    ( pad->GetDrillSize().x + pad->GetDrillSize().y ) / 2.0 * m_biuTo3Dunits );
            const float    holeInnerRadius = holeDiameterUnits / 2.0f;
            const SFVEC2F  padCenter( pad->GetPosition().x * static_cast<float>( m_biuTo3Dunits ),
                                      -pad->GetPosition().y * static_cast<float>( m_biuTo3Dunits ) );

            const auto frontMode = pad->GetFrontPostMachining();

            if( frontMode.has_value()
                && frontMode.value() != PAD_DRILL_POST_MACHINING_MODE::NOT_POST_MACHINED
                && frontMode.value() != PAD_DRILL_POST_MACHINING_MODE::UNKNOWN )
            {
                const float frontRadius = pad->GetFrontPostMachiningSize() * 0.5f
                                          * static_cast<float>( m_biuTo3Dunits );

                if( frontRadius > holeInnerRadius )
                {
                    if( frontMode.value() == PAD_DRILL_POST_MACHINING_MODE::COUNTERBORE )
                    {
                        m_frontCounterboreCutouts.Add(
                                new FILLED_CIRCLE_2D( padCenter, frontRadius, *pad ) );
                    }
                    else if( frontMode.value() == PAD_DRILL_POST_MACHINING_MODE::COUNTERSINK )
                    {
                        m_frontCountersinkCutouts.Add(
                                new FILLED_CIRCLE_2D( padCenter, frontRadius, *pad ) );
                    }
                }
            }

            const auto backMode = pad->GetBackPostMachining();

            if( backMode.has_value()
                && backMode.value() != PAD_DRILL_POST_MACHINING_MODE::NOT_POST_MACHINED
                && backMode.value() != PAD_DRILL_POST_MACHINING_MODE::UNKNOWN )
            {
                const float backRadius = pad->GetBackPostMachiningSize() * 0.5f
                                         * static_cast<float>( m_biuTo3Dunits );

                if( backRadius > holeInnerRadius )
                {
                    if( backMode.value() == PAD_DRILL_POST_MACHINING_MODE::COUNTERBORE )
                    {
                        m_backCounterboreCutouts.Add(
                                new FILLED_CIRCLE_2D( padCenter, backRadius, *pad ) );
                    }
                    else if( backMode.value() == PAD_DRILL_POST_MACHINING_MODE::COUNTERSINK )
                    {
                        m_backCountersinkCutouts.Add(
                                new FILLED_CIRCLE_2D( padCenter, backRadius, *pad ) );
                    }
                }
            }

            // Add backdrill cutouts for pads - affects specific layers only
            const VECTOR2I& secDrillSize = pad->GetSecondaryDrillSize();

            if( secDrillSize.x > 0 || secDrillSize.y > 0 )
            {
                const float backdrillRadius = ( secDrillSize.x + secDrillSize.y ) * 0.25f
                                              * static_cast<float>( m_biuTo3Dunits );

                // The hole outer diameter with plating
                const float holeOuterRadius = holeInnerRadius
                                              + static_cast<float>( GetHolePlatingThickness()
                                                                    * m_biuTo3Dunits );

                // Only add if backdrill is larger than original hole outer diameter
                if( backdrillRadius > holeOuterRadius )
                {
                    PCB_LAYER_ID secStart = pad->GetSecondaryDrillStartLayer();
                    PCB_LAYER_ID secEnd = pad->GetSecondaryDrillEndLayer();

                    // Iterate through layers affected by backdrill
                    for( PCB_LAYER_ID backdrillLayer : LAYER_RANGE( secStart, secEnd,
                                                                     m_copperLayersCount ) )
                    {
                        // Add to layer hole map for this layer
                        BVH_CONTAINER_2D* layerHoleContainer = nullptr;

                        if( !m_layerHoleMap.contains( backdrillLayer ) )
                        {
                            layerHoleContainer = new BVH_CONTAINER_2D;
                            m_layerHoleMap[backdrillLayer] = layerHoleContainer;
                        }
                        else
                        {
                            layerHoleContainer = m_layerHoleMap[backdrillLayer];
                        }

                        layerHoleContainer->Add(
                                new FILLED_CIRCLE_2D( padCenter, backdrillRadius, *pad ) );
                    }
                }
            }
        }
    }

    if( m_holeCount )
        m_averageHoleDiameter /= (float)m_holeCount;

    // Add contours of the pad holes (pads can be Circle or Segment holes)
    for( FOOTPRINT* footprint : m_board->Footprints() )
    {
        for( PAD* pad : footprint->Pads() )
        {
            if( !pad->HasHole() )
                continue;

            // The hole in the body is inflated by copper thickness.
            const int inflate = GetHolePlatingThickness();

            if( pad->GetAttribute() != PAD_ATTRIB::NPTH )
            {
                if( cfg.clip_silk_on_via_annuli )
                    pad->TransformHoleToPolygon( m_viaAnnuliPolys, inflate, pad->GetMaxError(), ERROR_INSIDE );

                pad->TransformHoleToPolygon( m_TH_ODPolys, inflate, pad->GetMaxError(), ERROR_INSIDE );
            }
            else
            {
                // If not plated, no copper.
                if( cfg.clip_silk_on_via_annuli )
                    pad->TransformHoleToPolygon( m_viaAnnuliPolys, 0, pad->GetMaxError(), ERROR_INSIDE );

                pad->TransformHoleToPolygon( m_NPTH_ODPolys, 0, pad->GetMaxError(), ERROR_INSIDE );
            }


            // Add counterbore/countersink polygons for pads
            const double holeDiameter = ( pad->GetDrillSize().x + pad->GetDrillSize().y ) / 2.0;
            const int holeRadius = KiROUND( holeDiameter / 2.0 );

            const auto frontMode = pad->GetFrontPostMachining();

            if( frontMode.has_value()
                && frontMode.value() != PAD_DRILL_POST_MACHINING_MODE::NOT_POST_MACHINED
                && frontMode.value() != PAD_DRILL_POST_MACHINING_MODE::UNKNOWN )
            {
                const int frontRadiusBIU = pad->GetFrontPostMachiningSize() / 2;

                if( frontRadiusBIU > holeRadius )
                {
                    if( frontMode.value() == PAD_DRILL_POST_MACHINING_MODE::COUNTERBORE )
                    {
                        TransformCircleToPolygon( m_frontCounterborePolys, pad->GetPosition(),
                                                    frontRadiusBIU, pad->GetMaxError(),
                                                    ERROR_INSIDE );
                    }
                    else if( frontMode.value() == PAD_DRILL_POST_MACHINING_MODE::COUNTERSINK )
                    {
                        TransformCircleToPolygon( m_frontCountersinkPolys, pad->GetPosition(),
                                                    frontRadiusBIU, pad->GetMaxError(),
                                                    ERROR_INSIDE );
                    }
                }
            }

            const auto backMode = pad->GetBackPostMachining();

            if( backMode.has_value()
                && backMode.value() != PAD_DRILL_POST_MACHINING_MODE::NOT_POST_MACHINED
                && backMode.value() != PAD_DRILL_POST_MACHINING_MODE::UNKNOWN )
            {
                const int backRadiusBIU = pad->GetBackPostMachiningSize() / 2;

                if( backRadiusBIU > holeRadius )
                {
                    if( backMode.value() == PAD_DRILL_POST_MACHINING_MODE::COUNTERBORE )
                    {
                        TransformCircleToPolygon( m_backCounterborePolys, pad->GetPosition(),
                                                    backRadiusBIU, pad->GetMaxError(),
                                                    ERROR_INSIDE );
                    }
                    else if( backMode.value() == PAD_DRILL_POST_MACHINING_MODE::COUNTERSINK )
                    {
                        TransformCircleToPolygon( m_backCountersinkPolys, pad->GetPosition(),
                                                    backRadiusBIU, pad->GetMaxError(),
                                                    ERROR_INSIDE );
                    }
                }
            }

            // Add backdrill polygons for pads - affects specific layers only
            const VECTOR2I& secDrillSize = pad->GetSecondaryDrillSize();

            if( secDrillSize.x > 0 || secDrillSize.y > 0 )
            {
                const int backdrillRadiusBIU = ( secDrillSize.x + secDrillSize.y ) / 4;
                const int holeOuterRadiusBIU = holeRadius + inflate;

                // Only add if backdrill is larger than original hole outer diameter
                if( backdrillRadiusBIU > holeOuterRadiusBIU )
                {
                    PCB_LAYER_ID secStart = pad->GetSecondaryDrillStartLayer();
                    PCB_LAYER_ID secEnd = pad->GetSecondaryDrillEndLayer();

                    TransformCircleToPolygon( m_BackdrillPolys, pad->GetPosition(),
                                                backdrillRadiusBIU, pad->GetMaxError(),
                                                ERROR_INSIDE );

                    // Iterate through layers affected by backdrill
                    for( PCB_LAYER_ID backdrillLayer : LAYER_RANGE( secStart, secEnd,
                                                                        m_copperLayersCount ) )
                    {
                        // Add polygon to per-layer hole polys
                        SHAPE_POLY_SET* layerHolePoly = nullptr;

                        if( !m_layerHoleOdPolys.contains( backdrillLayer ) )
                        {
                            layerHolePoly = new SHAPE_POLY_SET;
                            m_layerHoleOdPolys[backdrillLayer] = layerHolePoly;
                        }
                        else
                        {
                            layerHolePoly = m_layerHoleOdPolys[backdrillLayer];
                        }

                        TransformCircleToPolygon( *layerHolePoly, pad->GetPosition(),
                                                    backdrillRadiusBIU, pad->GetMaxError(),
                                                    ERROR_INSIDE );
                    }
                }
            }
        }
    }

    // Add footprints copper items (pads, shapes and text) to containers
    for( PCB_LAYER_ID layer : layer_ids )
    {
        wxASSERT( m_layerMap.contains( layer ) );

        BVH_CONTAINER_2D *layerContainer = m_layerMap[layer];

        for( FOOTPRINT* fp : m_board->Footprints() )
        {
            addPads( fp, layerContainer, layer );
            addFootprintShapes( fp, layerContainer, layer, visibilityFlags );

            // Add copper item to the plated copper polygon list if required
            if( cfg.DifferentiatePlatedCopper() && IsExternalCopperLayer( layer ) )
            {
                SHAPE_POLY_SET* layerPoly = layer == F_Cu ? m_frontPlatedCopperPolys : m_backPlatedCopperPolys;

                fp->TransformPadsToPolySet( *layerPoly, layer, 0, fp->GetMaxError(), ERROR_INSIDE );
                transformFPTextToPolySet( fp, layer, visibilityFlags, *layerPoly, fp->GetMaxError(), ERROR_INSIDE );
                transformFPShapesToPolySet( fp, layer, *layerPoly, fp->GetMaxError(), ERROR_INSIDE );
            }

            // Add copper item to poly contours (vertical outlines) if required
            if( cfg.opengl_copper_thickness && cfg.engine == RENDER_ENGINE::OPENGL )
            {
                wxASSERT( m_layers_poly.contains( layer ) );

                SHAPE_POLY_SET* layerPoly = m_layers_poly[layer];

                fp->TransformPadsToPolySet( *layerPoly, layer, 0, fp->GetMaxError(), ERROR_INSIDE );
                transformFPTextToPolySet( fp, layer, visibilityFlags, *layerPoly, fp->GetMaxError(), ERROR_INSIDE );
                transformFPShapesToPolySet( fp, layer, *layerPoly, fp->GetMaxError(), ERROR_INSIDE );
            }
        }
    }

    // Add graphic item on copper layers to object containers
    for( PCB_LAYER_ID layer : layer_ids )
    {
        wxASSERT( m_layerMap.contains( layer ) );

        BVH_CONTAINER_2D *layerContainer = m_layerMap[layer];

        // Add graphic items on copper layers (texts and other graphics)
        for( BOARD_ITEM* item : m_board->Drawings() )
        {
            if( !item->IsOnLayer( layer ) )
                continue;

            switch( item->Type() )
            {
            case PCB_SHAPE_T:
                addShape( static_cast<PCB_SHAPE*>( item ), layerContainer, item, layer );
                break;

            case PCB_TEXT_T:
                addText( static_cast<PCB_TEXT*>( item ), layerContainer, item );
                break;

            case PCB_TEXTBOX_T:
                addShape( static_cast<PCB_TEXTBOX*>( item ), layerContainer, item );
                break;

            case PCB_TABLE_T:
                addTable( static_cast<PCB_TABLE*>( item ), layerContainer, item );
                break;

            case PCB_BARCODE_T:
                addBarCode( static_cast<PCB_BARCODE*>( item ), layerContainer, item );
                break;

            case PCB_DIM_ALIGNED_T:
            case PCB_DIM_CENTER_T:
            case PCB_DIM_RADIAL_T:
            case PCB_DIM_ORTHOGONAL_T:
            case PCB_DIM_LEADER_T:
                addShape( static_cast<PCB_DIMENSION_BASE*>( item ), layerContainer, item );
                break;

            case PCB_REFERENCE_IMAGE_T:     // ignore
                break;

            default:
                wxLogTrace( m_logTrace, wxT( "createLayers: item type: %d not implemented" ), item->Type() );
                break;
            }

            // Add copper item to the plated copper polygon list if required
            if( cfg.DifferentiatePlatedCopper() && IsExternalCopperLayer( layer ) )
            {
                SHAPE_POLY_SET* copperPolys = layer == F_Cu ? m_frontPlatedCopperPolys : m_backPlatedCopperPolys;

                // Note: for TEXT and TEXTBOX, TransformShapeToPolygon returns the bounding
                // box shape, not the exact text shape. So it is not used for these items
                if( item->Type() == PCB_TEXTBOX_T )
                {
                    PCB_TEXTBOX* text_box = static_cast<PCB_TEXTBOX*>( item );
                    text_box->TransformTextToPolySet( *copperPolys, 0, text_box->GetMaxError(), ERROR_INSIDE );

                    // Add box outlines
                    text_box->PCB_SHAPE::TransformShapeToPolygon( *copperPolys, layer, 0, text_box->GetMaxError(),
                                                                  ERROR_INSIDE );
                }
                else if( item->Type() == PCB_TEXT_T )
                {
                    PCB_TEXT* text = static_cast<PCB_TEXT*>( item );
                    text->TransformTextToPolySet( *copperPolys, 0, text->GetMaxError(), ERROR_INSIDE );
                }
                else if( item->Type() != PCB_REFERENCE_IMAGE_T )
                {
                    item->TransformShapeToPolySet( *copperPolys, layer, 0, item->GetMaxError(), ERROR_INSIDE );
                }
            }

            // Add copper item to poly contours (vertical outlines) if required
            if( cfg.opengl_copper_thickness && cfg.engine == RENDER_ENGINE::OPENGL )
            {
                wxASSERT( m_layers_poly.contains( layer ) );

                SHAPE_POLY_SET *layerPoly = m_layers_poly[layer];

                switch( item->Type() )
                {
                case PCB_SHAPE_T:
                    item->TransformShapeToPolySet( *layerPoly, layer, 0, item->GetMaxError(), ERROR_INSIDE );
                    break;

                case PCB_TEXT_T:
                {
                    PCB_TEXT* text = static_cast<PCB_TEXT*>( item );

                    text->TransformTextToPolySet( *layerPoly, 0, text->GetMaxError(), ERROR_INSIDE );
                    break;
                }

                case PCB_TEXTBOX_T:
                {
                    PCB_TEXTBOX* textbox = static_cast<PCB_TEXTBOX*>( item );

                    if( textbox->IsBorderEnabled() )
                    {
                        textbox->PCB_SHAPE::TransformShapeToPolygon( *layerPoly, layer, 0, textbox->GetMaxError(),
                                                                     ERROR_INSIDE );
                    }

                    textbox->TransformTextToPolySet( *layerPoly, 0, textbox->GetMaxError(), ERROR_INSIDE );
                    break;
                }

                case PCB_TABLE_T:
                {
                    PCB_TABLE* table = static_cast<PCB_TABLE*>( item );

                    for( PCB_TABLECELL* cell : table->GetCells() )
                        cell->TransformTextToPolySet( *layerPoly, 0, cell->GetMaxError(), ERROR_INSIDE );

                    table->DrawBorders(
                            [&]( const VECTOR2I& ptA, const VECTOR2I& ptB,
                                 const STROKE_PARAMS& stroke )
                            {
                                SHAPE_SEGMENT seg( ptA, ptB, stroke.GetWidth()  );
                                seg.TransformToPolygon( *layerPoly, table->GetMaxError(), ERROR_INSIDE );
                            } );
                    break;
                }

                case PCB_BARCODE_T:
                {
                    PCB_BARCODE* bar_code = static_cast<PCB_BARCODE*>( item );

                    bar_code->TransformShapeToPolySet( *layerPoly, layer, 0, 0, ERROR_INSIDE );
                    break;
                }

                case PCB_DIM_ALIGNED_T:
                case PCB_DIM_CENTER_T:
                case PCB_DIM_RADIAL_T:
                case PCB_DIM_ORTHOGONAL_T:
                case PCB_DIM_LEADER_T:
                {
                    PCB_DIMENSION_BASE* dimension = static_cast<PCB_DIMENSION_BASE*>( item );

                    dimension->TransformTextToPolySet( *layerPoly, 0, dimension->GetMaxError(), ERROR_INSIDE );

                    for( const std::shared_ptr<SHAPE>& shape : dimension->GetShapes() )
                        shape->TransformToPolygon( *layerPoly, dimension->GetMaxError(), ERROR_INSIDE );

                    break;
                }

                case PCB_REFERENCE_IMAGE_T:     // ignore
                    break;

                default:
                    wxLogTrace( m_logTrace, wxT( "createLayers: item type: %d not implemented" ), item->Type() );
                    break;
                }
            }
        }
    }

    if( cfg.show_zones )
    {
        if( aStatusReporter )
            aStatusReporter->Report( _( "Create zones" ) );

        std::vector<std::pair<ZONE*, PCB_LAYER_ID>> zones;
        std::unordered_map<PCB_LAYER_ID, std::unique_ptr<std::mutex>> layer_lock;

        for( ZONE* zone : m_board->Zones() )
        {
            for( PCB_LAYER_ID layer : zone->GetLayerSet().Seq() )
            {
                zones.emplace_back( std::make_pair( zone, layer ) );
                layer_lock.emplace( layer, std::make_unique<std::mutex>() );

                if( cfg.DifferentiatePlatedCopper() && IsExternalCopperLayer( layer ) )
                {
                    SHAPE_POLY_SET* copperPolys = layer == F_Cu ? m_frontPlatedCopperPolys : m_backPlatedCopperPolys;

                    zone->TransformShapeToPolygon( *copperPolys, layer, 0, zone->GetMaxError(), ERROR_INSIDE );
                }
            }
        }

        // Add zones objects
        std::atomic<size_t> nextZone( 0 );
        std::atomic<size_t> threadsFinished( 0 );

        size_t parallelThreadCount = std::min<size_t>( zones.size(),
                std::max<size_t>( std::thread::hardware_concurrency(), 2 ) );

        for( size_t ii = 0; ii < parallelThreadCount; ++ii )
        {
            std::thread t = std::thread( [&]()
            {
                for( size_t areaId = nextZone.fetch_add( 1 );
                            areaId < zones.size();
                            areaId = nextZone.fetch_add( 1 ) )
                {
                    ZONE* zone = zones[areaId].first;

                    if( zone == nullptr )
                        break;

                    PCB_LAYER_ID layer = zones[areaId].second;

                    if( m_layerMap.contains( layer ) )
                        addSolidAreasShapes( zone, m_layerMap[layer], layer );

                    if( cfg.opengl_copper_thickness && cfg.engine == RENDER_ENGINE::OPENGL
                            && m_layers_poly.contains( layer ) )
                    {
                        auto mut_it = layer_lock.find( layer );

                        if( mut_it != layer_lock.end() )
                        {
                            std::lock_guard< std::mutex > lock( *( mut_it->second ) );
                            zone->TransformSolidAreasShapesToPolygon( layer, *m_layers_poly[layer] );
                        }
                    }
                }

                threadsFinished++;
            } );

            t.detach();
        }

        while( threadsFinished < parallelThreadCount )
            std::this_thread::sleep_for( std::chrono::milliseconds( 10 ) );
    }
    // End Build Copper layers

    // This will make a union of all added contours
    m_TH_ODPolys.Simplify();
    m_NPTH_ODPolys.Simplify();
    m_viaTH_ODPolys.Simplify();
    m_viaAnnuliPolys.Simplify();

    m_frontCounterborePolys.Simplify();
    m_backCounterborePolys.Simplify();
    m_frontCountersinkPolys.Simplify();
    m_backCountersinkPolys.Simplify();

    // Remove counterbore/countersink/backdrill cutouts from via annuli (both front and back)
    // The via annuli are used for silkscreen clipping and shouldn't include areas removed
    // by counterbore, countersink, or backdrill operations
    if( m_viaAnnuliPolys.OutlineCount() > 0 )
    {
        if( m_frontCounterborePolys.OutlineCount() > 0 )
            m_viaAnnuliPolys.BooleanSubtract( m_frontCounterborePolys );

        if( m_backCounterborePolys.OutlineCount() > 0 )
            m_viaAnnuliPolys.BooleanSubtract( m_backCounterborePolys );

        if( m_frontCountersinkPolys.OutlineCount() > 0 )
            m_viaAnnuliPolys.BooleanSubtract( m_frontCountersinkPolys );

        if( m_backCountersinkPolys.OutlineCount() > 0 )
            m_viaAnnuliPolys.BooleanSubtract( m_backCountersinkPolys );

        if( m_BackdrillPolys.OutlineCount() > 0 )
            m_viaAnnuliPolys.BooleanSubtract( m_BackdrillPolys );

        if( m_TertiarydrillPolys.OutlineCount() > 0 )
            m_viaAnnuliPolys.BooleanSubtract( m_TertiarydrillPolys );
    }

    // Build Tech layers
    // Based on:
    //    https://github.com/KiCad/kicad-source-mirror/blob/master/3d-viewer/3d_draw.cpp#L1059
    if( aStatusReporter )
        aStatusReporter->Report( _( "Build Tech layers" ) );

    // draw graphic items, on technical layers

    LSEQ techLayerList = LSET::AllNonCuMask().Seq( {
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
            User_1,
            User_2,
            User_3,
            User_4,
            User_5,
            User_6,
            User_7,
            User_8,
            User_9,
            User_10,
            User_11,
            User_12,
            User_13,
            User_14,
            User_15,
            User_16,
            User_17,
            User_18,
            User_19,
            User_20,
            User_21,
            User_22,
            User_23,
            User_24,
            User_25,
            User_26,
            User_27,
            User_28,
            User_29,
            User_30,
            User_31,
            User_32,
            User_33,
            User_34,
            User_35,
            User_36,
            User_37,
            User_38,
            User_39,
            User_40,
            User_41,
            User_42,
            User_43,
            User_44,
            User_45,
        } );

    std::bitset<LAYER_3D_END> enabledFlags = visibilityFlags;

    if( cfg.subtract_mask_from_silk || cfg.DifferentiatePlatedCopper() )
    {
        enabledFlags.set( LAYER_3D_SOLDERMASK_TOP );
        enabledFlags.set( LAYER_3D_SOLDERMASK_BOTTOM );
    }

    for( PCB_LAYER_ID layer : techLayerList )
    {
        if( aStatusReporter )
            aStatusReporter->Report( wxString::Format( _( "Build Tech layer %d" ), (int) layer ) );

        if( !Is3dLayerEnabled( layer, enabledFlags ) )
            continue;

        BVH_CONTAINER_2D *layerContainer = new BVH_CONTAINER_2D;
        m_layerMap[layer] = layerContainer;

        SHAPE_POLY_SET *layerPoly = new SHAPE_POLY_SET;
        m_layers_poly[layer] = layerPoly;

        if( Is3dLayerEnabled( layer, visibilityFlags ) )
        {
            // Add drawing objects
            for( BOARD_ITEM* item : m_board->Drawings() )
            {
                if( !item->IsOnLayer( layer ) )
                    continue;

                switch( item->Type() )
                {
                case PCB_SHAPE_T:
                    addShape( static_cast<PCB_SHAPE*>( item ), layerContainer, item, layer );
                    break;

                case PCB_TEXT_T:
                    addText( static_cast<PCB_TEXT*>( item ), layerContainer, item );
                    break;

                case PCB_TEXTBOX_T:
                    addShape( static_cast<PCB_TEXTBOX*>( item ), layerContainer, item );
                    break;

                case PCB_TABLE_T:
                    addTable( static_cast<PCB_TABLE*>( item ), layerContainer, item );
                    break;

                case PCB_BARCODE_T:
                    addBarCode( static_cast<PCB_BARCODE*>( item ), layerContainer, item );
                    break;

                case PCB_DIM_ALIGNED_T:
                case PCB_DIM_CENTER_T:
                case PCB_DIM_RADIAL_T:
                case PCB_DIM_ORTHOGONAL_T:
                case PCB_DIM_LEADER_T:
                    addShape( static_cast<PCB_DIMENSION_BASE*>( item ), layerContainer, item );
                    break;

                default:
                    break;
                }
            }

            // Add track, via and arc tech layers
            if( IsSolderMaskLayer( layer ) )
            {
                for( PCB_TRACK* track : m_board->Tracks() )
                {
                    if( !track->IsOnLayer( layer ) )
                        continue;

                    // Only vias on a external copper layer can have a solder mask
                    PCB_LAYER_ID copper_layer = ( layer == F_Mask ) ? F_Cu : B_Cu;

                    if( track->Type() == PCB_VIA_T )
                    {
                        const PCB_VIA* via = static_cast<const PCB_VIA*>( track );

                        if( !via->FlashLayer( copper_layer ) )
                            continue;
                    }

                    int maskExpansion = track->GetSolderMaskExpansion();
                    createTrackWithMargin( track, layerContainer, layer, maskExpansion );
                }
            }

            // Add footprints tech layers - objects
            const bool     isPasteLayer = ( layer == F_Paste || layer == B_Paste );
            const wxString currentVariant = m_board->GetCurrentVariant();

            for( FOOTPRINT* footprint : m_board->Footprints() )
            {
                // Skip paste layers for footprints that are DNP in the current variant
                bool skipForVariantDNP = isPasteLayer && footprint->GetDNPForVariant( currentVariant );

                if( layer == F_SilkS || layer == B_SilkS )
                {
                    int linewidth = m_board->GetDesignSettings().m_LineThickness[ LAYER_CLASS_SILK ];

                    for( PAD* pad : footprint->Pads() )
                    {
                        if( !pad->IsOnLayer( layer ) )
                            continue;

                        buildPadOutlineAsSegments( pad, layer, layerContainer, linewidth );
                    }
                }
                else if( !skipForVariantDNP )
                {
                    addPads( footprint, layerContainer, layer );
                }

                if( !skipForVariantDNP )
                    addFootprintShapes( footprint, layerContainer, layer, visibilityFlags );
            }

            // Draw non copper zones
            if( cfg.show_zones )
            {
                for( ZONE* zone : m_board->Zones() )
                {
                    if( zone->IsOnLayer( layer ) )
                        addSolidAreasShapes( zone, layerContainer, layer );
                }
            }
        }

        // Add item contours.  We need these if we're building vertical walls or if this is a
        // mask layer and we're differentiating copper from plated copper.
        if( ( cfg.engine == RENDER_ENGINE::OPENGL && cfg.opengl_copper_thickness )
                || ( cfg.DifferentiatePlatedCopper() && ( layer == F_Mask || layer == B_Mask ) ) )
        {
            // DRAWINGS
            for( BOARD_ITEM* item : m_board->Drawings() )
            {
                if( !item->IsOnLayer( layer ) )
                    continue;

                switch( item->Type() )
                {
                case PCB_SHAPE_T:
                    item->TransformShapeToPolySet( *layerPoly, layer, 0, item->GetMaxError(), ERROR_INSIDE );
                    break;

                case PCB_TEXT_T:
                {
                    PCB_TEXT* text = static_cast<PCB_TEXT*>( item );

                    text->TransformTextToPolySet( *layerPoly, 0, text->GetMaxError(), ERROR_INSIDE );
                    break;
                }

                case PCB_TEXTBOX_T:
                {
                    PCB_TEXTBOX* textbox = static_cast<PCB_TEXTBOX*>( item );

                    if( textbox->IsBorderEnabled() )
                    {
                        textbox->PCB_SHAPE::TransformShapeToPolygon( *layerPoly, layer, 0, textbox->GetMaxError(),
                                                                     ERROR_INSIDE );
                    }

                    textbox->TransformTextToPolySet( *layerPoly, 0, textbox->GetMaxError(), ERROR_INSIDE );
                    break;
                }

                case PCB_TABLE_T:
                {
                    PCB_TABLE* table = static_cast<PCB_TABLE*>( item );

                    for( PCB_TABLECELL* cell : table->GetCells() )
                        cell->TransformTextToPolySet( *layerPoly, 0, cell->GetMaxError(), ERROR_INSIDE );

                    table->DrawBorders(
                            [&]( const VECTOR2I& ptA, const VECTOR2I& ptB,
                                 const STROKE_PARAMS& stroke )
                            {
                                SHAPE_SEGMENT seg( ptA, ptB, stroke.GetWidth()  );
                                seg.TransformToPolygon( *layerPoly, table->GetMaxError(), ERROR_INSIDE );
                            } );

                    break;
                }

                case PCB_BARCODE_T:
                {
                    PCB_BARCODE* bar_code = static_cast<PCB_BARCODE*>( item );

                    bar_code->TransformShapeToPolySet( *layerPoly, layer, 0, 0, ERROR_INSIDE );
                    break;
                }

                default:
                    break;
                }
            }

            // NON-TENTED VIAS
            if( ( layer == F_Mask || layer == B_Mask ) )
            {
                int maskExpansion = GetBoard()->GetDesignSettings().m_SolderMaskExpansion;

                for( PCB_TRACK* track : m_board->Tracks() )
                {
                    if( track->Type() == PCB_VIA_T )
                    {
                        const PCB_VIA* via = static_cast<const PCB_VIA*>( track );

                        if( via->FlashLayer( layer ) && !via->IsTented( layer ) )
                        {
                            track->TransformShapeToPolygon( *layerPoly, layer, maskExpansion, track->GetMaxError(),
                                                            ERROR_INSIDE );
                        }
                    }
                    else
                    {
                        if( track->HasSolderMask() )
                        {
                            track->TransformShapeToPolySet( *layerPoly, layer, maskExpansion, track->GetMaxError(),
                                                            ERROR_INSIDE );
                        }
                    }
                }
            }

            // FOOTPRINT CHILDREN
            for( FOOTPRINT* footprint : m_board->Footprints() )
            {
                if( layer == F_SilkS || layer == B_SilkS )
                {
                    int linewidth = m_board->GetDesignSettings().m_LineThickness[ LAYER_CLASS_SILK ];

                    for( PAD* pad : footprint->Pads() )
                    {
                        if( pad->IsOnLayer( layer ) )
                        {
                            buildPadOutlineAsPolygon( pad, layer, *layerPoly, linewidth, pad->GetMaxError(),
                                                      ERROR_INSIDE );
                        }
                    }
                }
                else
                {
                    footprint->TransformPadsToPolySet( *layerPoly, layer, 0, footprint->GetMaxError(), ERROR_INSIDE );
                }

                transformFPTextToPolySet( footprint, layer, visibilityFlags, *layerPoly, footprint->GetMaxError(),
                                          ERROR_INSIDE );
                transformFPShapesToPolySet( footprint, layer, *layerPoly, footprint->GetMaxError(), ERROR_INSIDE );
            }

            if( cfg.show_zones || layer == F_Mask || layer == B_Mask )
            {
                for( ZONE* zone : m_board->Zones() )
                {
                    if( zone->IsOnLayer( layer ) )
                        zone->TransformSolidAreasShapesToPolygon( layer, *layerPoly );
                }
            }

            // This will make a union of all added contours
            layerPoly->Simplify();
        }
    }
    // End Build Tech layers

    // If we're rendering off-board silk, also render pads of footprints which are entirely
    // outside the board outline.  This makes off-board footprints more visually recognizable.
    if( cfg.show_off_board_silk )
    {
        BOX2I boardBBox = m_board_poly.BBox();

        for( FOOTPRINT* footprint : m_board->Footprints() )
        {
            if( !footprint->GetBoundingBox().Intersects( boardBBox ) )
            {
                if( footprint->IsFlipped() )
                    addPads( footprint, m_offboardPadsBack, B_Cu );
                else
                    addPads( footprint, m_offboardPadsFront, F_Cu );
            }
        }

        m_offboardPadsFront->BuildBVH();
        m_offboardPadsBack->BuildBVH();
    }

    // Simplify layer polygons

    if( aStatusReporter )
        aStatusReporter->Report( _( "Simplifying copper layer polygons" ) );

    if( cfg.DifferentiatePlatedCopper() )
    {
        if( aStatusReporter )
            aStatusReporter->Report( _( "Calculating plated copper" ) );

        // TRIM PLATED COPPER TO SOLDERMASK
        if( m_layers_poly.contains( F_Mask ) )
            m_frontPlatedCopperPolys->BooleanIntersection( *m_layers_poly.at( F_Mask ) );

        if( m_layers_poly.contains( B_Mask ))
            m_backPlatedCopperPolys->BooleanIntersection( *m_layers_poly.at( B_Mask ) );

        // Subtract plated copper from unplated copper
        if( m_layers_poly.contains( F_Cu ) )
            m_layers_poly[F_Cu]->BooleanSubtract( *m_frontPlatedCopperPolys );

        if( m_layers_poly.contains( B_Cu ) )
            m_layers_poly[B_Cu]->BooleanSubtract( *m_backPlatedCopperPolys );

        // ADD PLATED COPPER
        ConvertPolygonToTriangles( *m_frontPlatedCopperPolys, *m_platedPadsFront, m_biuTo3Dunits,
                                   *DELETED_BOARD_ITEM::GetInstance() );

        ConvertPolygonToTriangles( *m_backPlatedCopperPolys, *m_platedPadsBack, m_biuTo3Dunits,
                                   *DELETED_BOARD_ITEM::GetInstance() );

        m_platedPadsFront->BuildBVH();
        m_platedPadsBack->BuildBVH();
    }

    if( cfg.opengl_copper_thickness && cfg.engine == RENDER_ENGINE::OPENGL )
    {
        std::vector<PCB_LAYER_ID> &selected_layer_id = layer_ids;
        std::vector<PCB_LAYER_ID> layer_id_without_F_and_B;

        if( cfg.DifferentiatePlatedCopper() )
        {
            layer_id_without_F_and_B.clear();
            layer_id_without_F_and_B.reserve( layer_ids.size() );

            for( PCB_LAYER_ID layer: layer_ids )
            {
                if( layer != F_Cu && layer != B_Cu )
                    layer_id_without_F_and_B.push_back( layer );
            }

            selected_layer_id = layer_id_without_F_and_B;
        }

        if( selected_layer_id.size() > 0 )
        {
            if( aStatusReporter )
            {
                aStatusReporter->Report( wxString::Format( _( "Simplifying %d copper layers" ),
                                                           (int) selected_layer_id.size() ) );
            }

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
                                if( m_layers_poly.contains( selected_layer_id[i] ) )
                                {
                                    // This will make a union of all added contours
                                    m_layers_poly[ selected_layer_id[i] ]->ClearArcs();
                                    m_layers_poly[ selected_layer_id[i] ]->Simplify();
                                }
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

    for( PCB_LAYER_ID layer : layer_ids )
    {
        if( m_layerHoleOdPolys.contains( layer ) )
        {
            // found
            SHAPE_POLY_SET *polyLayer = m_layerHoleOdPolys[layer];
            polyLayer->Simplify();

            wxASSERT( m_layerHoleIdPolys.contains( layer ) );

            polyLayer = m_layerHoleIdPolys[layer];
            polyLayer->Simplify();
        }
    }

    // Build BVH (Bounding volume hierarchy) for holes and vias

    if( aStatusReporter )
        aStatusReporter->Report( _( "Build BVH for holes and vias" ) );

    m_TH_IDs.BuildBVH();
    m_TH_ODs.BuildBVH();
    m_viaAnnuli.BuildBVH();

    m_frontCounterboreCutouts.BuildBVH();
    m_backCounterboreCutouts.BuildBVH();
    m_frontCountersinkCutouts.BuildBVH();
    m_backCountersinkCutouts.BuildBVH();
    m_backdrillCutouts.BuildBVH();
    m_tertiarydrillCutouts.BuildBVH();

    if( !m_layerHoleMap.empty() )
    {
        for( std::pair<const PCB_LAYER_ID, BVH_CONTAINER_2D*>& hole : m_layerHoleMap )
            hole.second->BuildBVH();
    }

    // We only need the Solder mask to initialize the BVH
    // because..?
    if( m_layerMap[B_Mask] )
        m_layerMap[B_Mask]->BuildBVH();

    if( m_layerMap[F_Mask] )
        m_layerMap[F_Mask]->BuildBVH();
}
