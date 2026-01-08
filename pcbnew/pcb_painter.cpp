/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2013-2019 CERN
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
 *
 * @author Tomasz Wlostowski <tomasz.wlostowski@cern.ch>
 * @author Maciej Suminski <maciej.suminski@cern.ch>
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

#include <advanced_config.h>
#include <board.h>
#include <board_design_settings.h>
#include <pcb_track.h>
#include <pcb_group.h>
#include <footprint.h>
#include <pad.h>
#include <pcb_shape.h>
#include <string_utils.h>
#include <zone.h>
#include <pcb_reference_image.h>
#include <pcb_text.h>
#include <pcb_textbox.h>
#include <pcb_table.h>
#include <pcb_tablecell.h>
#include <pcb_marker.h>
#include <pcb_dimension.h>
#include <pcb_point.h>
#include <pcb_barcode.h>
#include <pcb_target.h>
#include <pcb_board_outline.h>

#include <layer_ids.h>
#include <lset.h>
#include <pcb_painter.h>
#include <pcb_display_options.h>
#include <project/net_settings.h>
#include <settings/color_settings.h>
#include <settings/common_settings.h>
#include <settings/settings_manager.h>
#include <settings/cvpcb_settings.h>
#include <pcbnew_settings.h>
#include <footprint_editor_settings.h>

#include <convert_basic_shapes_to_polygon.h>
#include <gal/graphics_abstraction_layer.h>
#include <callback_gal.h>
#include <geometry/geometry_utils.h>
#include <geometry/shape_line_chain.h>
#include <geometry/shape_rect.h>
#include <geometry/shape_poly_set.h>
#include <geometry/roundrect.h>
#include <geometry/shape_segment.h>
#include <geometry/shape_simple.h>
#include <geometry/shape_circle.h>
#include <geometry/shape_arc.h>
#include <stroke_params.h>
#include <bezier_curves.h>
#include <kiface_base.h>
#include <gr_text.h>
#include <pgm_base.h>

using namespace KIGFX;


PCBNEW_SETTINGS* pcbconfig()
{
    return dynamic_cast<PCBNEW_SETTINGS*>( Kiface().KifaceSettings() );
}

// Helpers for display options existing in Cvpcb and Pcbnew
// Note, when running Cvpcb, pcbconfig() returns nullptr and viewer_settings()
// returns the viewer options existing to Cvpcb and Pcbnew
PCB_VIEWERS_SETTINGS_BASE* PCB_PAINTER::viewer_settings()
{
    switch( m_frameType )
    {
    case FRAME_PCB_EDITOR:
    case FRAME_PCB_DISPLAY3D:
    default:
        return Pgm().GetSettingsManager().GetAppSettings<PCBNEW_SETTINGS>( "pcbnew" );

    case FRAME_FOOTPRINT_EDITOR:
    case FRAME_FOOTPRINT_WIZARD:
        return Pgm().GetSettingsManager().GetAppSettings<FOOTPRINT_EDITOR_SETTINGS>( "fpedit" );

    case FRAME_FOOTPRINT_VIEWER:
    case FRAME_FOOTPRINT_CHOOSER:
    case FRAME_FOOTPRINT_PREVIEW:
    case FRAME_CVPCB:
    case FRAME_CVPCB_DISPLAY:
        return Pgm().GetSettingsManager().GetAppSettings<CVPCB_SETTINGS>( "cvpcb" );
    }
}


PCB_RENDER_SETTINGS::PCB_RENDER_SETTINGS()
{
    m_backgroundColor = COLOR4D( 0.0, 0.0, 0.0, 1.0 );
    m_ZoneDisplayMode = ZONE_DISPLAY_MODE::SHOW_FILLED;
    m_netColorMode = NET_COLOR_MODE::RATSNEST;
    m_ContrastModeDisplay = HIGH_CONTRAST_MODE::NORMAL;

    m_trackOpacity    = 1.0;
    m_viaOpacity      = 1.0;
    m_padOpacity      = 1.0;
    m_zoneOpacity     = 1.0;
    m_imageOpacity    = 1.0;
    m_filledShapeOpacity    = 1.0;

    m_ForcePadSketchModeOn = false;

    m_PadEditModePad = nullptr;

    SetDashLengthRatio( 12 );       // From ISO 128-2
    SetGapLengthRatio( 3 );         // From ISO 128-2

    m_ForceShowFieldsWhenFPSelected = true;

    update();
}


void PCB_RENDER_SETTINGS::LoadColors( const COLOR_SETTINGS* aSettings )
{
    SetBackgroundColor( aSettings->GetColor( LAYER_PCB_BACKGROUND ) );

    // Init board layers colors:
    for( int i = 0; i < PCB_LAYER_ID_COUNT; i++ )
    {
        m_layerColors[i] = aSettings->GetColor( i );

        // Guard: if the alpha channel is too small, the layer is not visible.
        if( m_layerColors[i].a < 0.2 )
            m_layerColors[i].a = 0.2;
    }

    // Init specific graphic layers colors:
    for( int i = GAL_LAYER_ID_START; i < GAL_LAYER_ID_END; i++ )
        m_layerColors[i] = aSettings->GetColor( i );

    // Colors for layers that aren't theme-able
    m_layerColors[LAYER_PAD_PLATEDHOLES] = aSettings->GetColor( LAYER_PCB_BACKGROUND );
    m_layerColors[LAYER_PAD_NETNAMES]    = aSettings->GetColor( NETNAMES_LAYER_ID_START );

    // Netnames for copper layers
    const COLOR4D lightLabel = aSettings->GetColor( NETNAMES_LAYER_ID_START );
    const COLOR4D darkLabel = lightLabel.Inverted();

    for( PCB_LAYER_ID layer : LSET::AllCuMask().CuStack() )
    {
        if( m_layerColors[layer].GetBrightness() > 0.5 )
            m_layerColors[GetNetnameLayer( layer )] = darkLabel;
        else
            m_layerColors[GetNetnameLayer( layer )] = lightLabel;
    }

    if( PgmOrNull() )   // can be null if used without project (i.e. from python script)
        m_hiContrastFactor = 1.0f - Pgm().GetCommonSettings()->m_Appearance.hicontrast_dimming_factor;
    else
        m_hiContrastFactor = 1.0f - 0.8f;   // default value

    update();
}


void PCB_RENDER_SETTINGS::LoadDisplayOptions( const PCB_DISPLAY_OPTIONS& aOptions )
{
    m_hiContrastEnabled   = aOptions.m_ContrastModeDisplay != HIGH_CONTRAST_MODE::NORMAL;
    m_ZoneDisplayMode     = aOptions.m_ZoneDisplayMode;
    m_ContrastModeDisplay = aOptions.m_ContrastModeDisplay;
    m_netColorMode        = aOptions.m_NetColorMode;

    m_trackOpacity    = aOptions.m_TrackOpacity;
    m_viaOpacity      = aOptions.m_ViaOpacity;
    m_padOpacity      = aOptions.m_PadOpacity;
    m_zoneOpacity     = aOptions.m_ZoneOpacity;
    m_imageOpacity    = aOptions.m_ImageOpacity;
    m_filledShapeOpacity = aOptions.m_FilledShapeOpacity;
}


COLOR4D PCB_RENDER_SETTINGS::GetColor( const VIEW_ITEM* aItem, int aLayer ) const
{
    return GetColor( dynamic_cast<const BOARD_ITEM*>( aItem ), aLayer );
}


COLOR4D PCB_RENDER_SETTINGS::GetColor( const BOARD_ITEM* aItem, int aLayer ) const
{
    int netCode = -1;
    int originalLayer = aLayer;

    if( aLayer == LAYER_MARKER_SHADOWS )
        return m_backgroundColor.WithAlpha( 0.6 );

    if( aLayer == LAYER_LOCKED_ITEM_SHADOW )
        return m_layerColors.at( aLayer );

    // SMD pads use the copper netname layer
    if( aLayer == LAYER_PAD_FR_NETNAMES )
        aLayer = GetNetnameLayer( F_Cu );
    else if( aLayer == LAYER_PAD_BK_NETNAMES )
        aLayer = GetNetnameLayer( B_Cu );

    if( IsHoleLayer( aLayer ) && m_isPrinting )
    {
        // Careful that we don't end up with the same colour for the annular ring and the hole
        // when printing in B&W.
        const PAD*     pad = dynamic_cast<const PAD*>( aItem );
        const PCB_VIA* via = dynamic_cast<const PCB_VIA*>( aItem );
        int            holeLayer = aLayer;
        int            annularRingLayer = UNDEFINED_LAYER;

        // TODO(JE) padstacks -- this won't work, we don't know what the annular ring layer is
        // Inserting F_Cu here for now.
        if( pad && pad->GetAttribute() == PAD_ATTRIB::PTH )
            annularRingLayer = F_Cu;
        else if( via )
            annularRingLayer = F_Cu;

        if( annularRingLayer != UNDEFINED_LAYER )
        {
            auto it = m_layerColors.find( holeLayer );
            auto it2 = m_layerColors.find( annularRingLayer );

            if( it != m_layerColors.end() && it2 != m_layerColors.end() && it->second == it2->second )
                aLayer = LAYER_PCB_BACKGROUND;
        }
    }

    // Zones should pull from the copper layer
    if( aItem && aItem->Type() == PCB_ZONE_T )
    {
        if( IsZoneFillLayer( aLayer ) )
            aLayer = aLayer - LAYER_ZONE_START;
    }

    // Points use the LAYER_POINTS color for their virtual per-layer layers
    if( IsPointsLayer( aLayer ) )
        aLayer = LAYER_POINTS;

    // Pad and via copper and clearance outlines take their color from the copper layer
    if( IsPadCopperLayer( aLayer ) )
    {
        if( pcbconfig() && aItem && aItem->Type() == PCB_PAD_T )
        {
            const PAD* pad = static_cast<const PAD*>( aItem );

            // Old-skool display for people who struggle with change
            if( pcbconfig()->m_Display.m_UseViaColorForNormalTHPadstacks
                    && pad->GetAttribute() == PAD_ATTRIB::PTH
                    && pad->Padstack().Mode() == PADSTACK::MODE::NORMAL )
            {
                aLayer = LAYER_VIA_HOLES;
            }
            else
            {
                aLayer = aLayer - LAYER_PAD_COPPER_START;
            }
        }
        else
        {
            aLayer = aLayer - LAYER_PAD_COPPER_START;
        }
    }
    else if( IsViaCopperLayer( aLayer ) )
        aLayer = aLayer - LAYER_VIA_COPPER_START;
    else if( IsClearanceLayer( aLayer ) )
        aLayer = aLayer - LAYER_CLEARANCE_START;

    // Use via "golden copper" hole color for pad hole walls for contrast
    else if( aLayer == LAYER_PAD_HOLEWALLS )
        aLayer = LAYER_VIA_HOLES;

    // Show via mask layers if appropriate
    if( aLayer == LAYER_VIA_THROUGH && !m_isPrinting )
    {
        if( aItem && aItem->GetBoard() )
        {
            LSET visibleLayers = aItem->GetBoard()->GetVisibleLayers()
                                 & aItem->GetBoard()->GetEnabledLayers()
                                 & aItem->GetLayerSet();

            if( GetActiveLayer() == F_Mask && visibleLayers.test( F_Mask ) )
            {
                aLayer = F_Mask;
            }
            else if( GetActiveLayer() == B_Mask && visibleLayers.test( B_Mask ) )
            {
                aLayer = B_Mask;
            }
            else if( ( visibleLayers & LSET::AllCuMask() ).none() )
            {
                if( visibleLayers.any() )
                    aLayer = visibleLayers.Seq().back();
            }
        }
    }

    // Normal path: get the layer base color
    auto it = m_layerColors.find( aLayer );
    COLOR4D color = it == m_layerColors.end() ? COLOR4D::WHITE : it->second;

    if( !aItem )
        return color;

    // Selection disambiguation
    if( aItem->IsBrightened() )
        return color.Brightened( m_selectFactor ).WithAlpha( 0.8 );

    // Normal selection
    if( aItem->IsSelected() )
    {
        // Selection for tables is done with a background wash, so pass in nullptr to GetColor()
        // so we just get the "normal" (un-selected/un-brightened) color for the borders.
        if( aItem->Type() != PCB_TABLE_T && aItem->Type() != PCB_TABLECELL_T )
        {
            auto it_selected = m_layerColorsSel.find( aLayer );
            color = it_selected == m_layerColorsSel.end() ? color.Brightened( 0.8 ) : it_selected->second;
        }
    }

    // Some graphic objects are BOARD_CONNECTED_ITEM, but they are seen here as
    // actually board connected objects only if on a copper layer
    const BOARD_CONNECTED_ITEM* conItem = nullptr;

    if( aItem->IsConnected() && aItem->IsOnCopperLayer() )
        conItem = static_cast<const BOARD_CONNECTED_ITEM*>( aItem );

    // Try to obtain the netcode for the aItem
    if( conItem )
        netCode = conItem->GetNetCode();

    bool highlighted = m_highlightEnabled && m_highlightNetcodes.count( netCode );
    bool selected    = aItem->IsSelected();

    // Apply net color overrides
    if( conItem && m_netColorMode == NET_COLOR_MODE::ALL && IsCopperLayer( aLayer ) )
    {
        COLOR4D netColor = COLOR4D::UNSPECIFIED;

        auto ii = m_netColors.find( netCode );

        if( ii != m_netColors.end() )
            netColor = ii->second;

        if( netColor == COLOR4D::UNSPECIFIED )
        {
            const NETCLASS* nc = conItem->GetEffectiveNetClass();

            if( nc->HasPcbColor() )
                netColor = nc->GetPcbColor();
        }

        if( netColor == COLOR4D::UNSPECIFIED )
            netColor = color;

        if( selected )
        {
            // Selection brightening overrides highlighting
            netColor.Brighten( m_selectFactor );
        }
        else if( m_highlightEnabled )
        {
            // Highlight brightens objects on all layers and darkens everything else for contrast
            if( highlighted )
                netColor.Brighten( m_highlightFactor );
            else
                netColor.Darken( 1.0 - m_highlightFactor );
        }

        color = netColor;
    }
    else if( !selected && m_highlightEnabled )
    {
        // Single net highlight mode
        if( m_highlightNetcodes.contains( netCode ) )
        {
            auto it_hi = m_layerColorsHi.find( aLayer );
            color = it_hi == m_layerColorsHi.end() ? color.Brightened( m_highlightFactor ) : it_hi->second;
        }
        else
        {
            auto it_dark = m_layerColorsDark.find( aLayer );
            color = it_dark == m_layerColorsDark.end() ? color.Darkened( 1.0 - m_highlightFactor ) : it_dark->second;
        }
    }

    // Apply high-contrast dimming
    if( m_hiContrastEnabled && m_highContrastLayers.size() && !highlighted && !selected )
    {
        PCB_LAYER_ID primary = GetPrimaryHighContrastLayer();
        bool         isActive = m_highContrastLayers.count( aLayer );
        bool         hide = false;

        switch( originalLayer )
        {
        // TODO(JE) not sure if this is needed
        case LAYER_PADS:
        {
            const PAD* pad = static_cast<const PAD*>( aItem );

            if( pad->IsOnLayer( primary ) && !pad->FlashLayer( primary ) )
            {
                isActive = false;

                if( IsCopperLayer( primary ) )
                    hide = true;
            }

            if( m_PadEditModePad && pad != m_PadEditModePad )
                isActive = false;

            break;
        }

        case LAYER_VIA_BLIND:
        case LAYER_VIA_BURIED:
        case LAYER_VIA_MICROVIA:
        {
            const PCB_VIA* via = static_cast<const PCB_VIA*>( aItem );

            // Target graphic is active if the via crosses the primary layer
            if( via->GetLayerSet().test( primary ) == 0 )
            {
                isActive = false;
                hide = true;
            }

            break;
        }

        case LAYER_VIA_THROUGH:
        {
            const PCB_VIA* via = static_cast<const PCB_VIA*>( aItem );

            if( !via->FlashLayer( primary ) )
            {
                isActive = false;

                if( IsCopperLayer( primary ) )
                    hide = true;
            }

            break;
        }

        case LAYER_PAD_PLATEDHOLES:
        case LAYER_PAD_HOLEWALLS:
        case LAYER_NON_PLATEDHOLES:
            // Pad holes are active is any physical layer is active
            if( LSET::PhysicalLayersMask().test( primary ) == 0 )
                isActive = false;

            break;

        case LAYER_VIA_HOLES:
        case LAYER_VIA_HOLEWALLS:
        {
            const PCB_VIA* via = static_cast<const PCB_VIA*>( aItem );

            if( via->GetViaType() == VIATYPE::THROUGH )
            {
                // A through via's hole is active if any physical layer is active
                if( LSET::PhysicalLayersMask().test( primary ) == 0 )
                    isActive = false;
            }
            else
            {
                // A blind/buried or micro via's hole is active if it crosses the primary layer
                if( via->GetLayerSet().test( primary ) == 0 )
                    isActive = false;
            }

            break;
        }

        case LAYER_DRC_ERROR:
        case LAYER_DRC_WARNING:
        case LAYER_DRC_EXCLUSION:
        case LAYER_DRC_SHAPES:
            isActive = true;
            break;

        default:
            break;
        }

        if( !isActive )
        {
            // Graphics on Edge_Cuts layer are not fully dimmed or hidden because they are
            // useful when working on another layer
            // We could use a dim factor = m_hiContrastFactor, but to have a sufficient
            // contrast whenever m_hiContrastFactor value, we clamp the factor to 0.3f
            // (arbitray choice after tests)
            float dim_factor_Edge_Cuts = std::max( m_hiContrastFactor, 0.3f );

            if( m_ContrastModeDisplay == HIGH_CONTRAST_MODE::HIDDEN
                || IsNetnameLayer( aLayer )
                || hide )
            {
                if( originalLayer == Edge_Cuts )
                {
                    it = m_layerColors.find( LAYER_PCB_BACKGROUND );

                    if( it != m_layerColors.end() )
                        color = color.Mix( it->second, dim_factor_Edge_Cuts );
                    else
                        color = color.Mix( COLOR4D::BLACK, dim_factor_Edge_Cuts );
                }
                else
                    color = COLOR4D::CLEAR;
            }
            else
            {
                it = m_layerColors.find( LAYER_PCB_BACKGROUND );
                COLOR4D backgroundColor = it == m_layerColors.end() ? COLOR4D::BLACK : it->second;

                if( originalLayer == Edge_Cuts )
                    color = color.Mix( backgroundColor, dim_factor_Edge_Cuts );
                else
                    color = color.Mix( backgroundColor, m_hiContrastFactor );

                // Reference images can't have their color mixed so just reduce the opacity a bit
                // so they show through less
                if( aItem->Type() == PCB_REFERENCE_IMAGE_T )
                    color.a *= m_hiContrastFactor;
            }
        }
    }
    else if( originalLayer == LAYER_VIA_BLIND
          || originalLayer == LAYER_VIA_BURIED
          || originalLayer == LAYER_VIA_MICROVIA )
    {
        const PCB_VIA* via = static_cast<const PCB_VIA*>( aItem );
        const BOARD*   board = via->GetBoard();
        LSET           visibleLayers = board->GetVisibleLayers() & board->GetEnabledLayers();

        // Target graphic is visible if the via crosses a visible layer
        if( ( via->GetLayerSet() & visibleLayers ).none() )
            color = COLOR4D::CLEAR;
    }

    // Apply per-type opacity overrides
    if( aItem->Type() == PCB_TRACE_T || aItem->Type() == PCB_ARC_T )
        color.a *= m_trackOpacity;
    else if( aItem->Type() == PCB_VIA_T )
        color.a *= m_viaOpacity;
    else if( aItem->Type() == PCB_PAD_T )
        color.a *= m_padOpacity;
    else if( aItem->Type() == PCB_ZONE_T && static_cast<const ZONE*>( aItem )->IsTeardropArea() )
        color.a *= m_trackOpacity;
    else if( aItem->Type() == PCB_ZONE_T )
        color.a *= m_zoneOpacity;
    else if( aItem->Type() == PCB_REFERENCE_IMAGE_T )
        color.a *= m_imageOpacity;
    else if( aItem->Type() == PCB_SHAPE_T && static_cast<const PCB_SHAPE*>( aItem )->IsAnyFill() )
        color.a *= m_filledShapeOpacity;
    else if( aItem->Type() == PCB_SHAPE_T && aItem->IsOnCopperLayer() )
        color.a *= m_trackOpacity;

    if( aItem->GetForcedTransparency() > 0.0 )
        color = color.WithAlpha( color.a * ( 1.0 - aItem->GetForcedTransparency() ) );

    // No special modifiers enabled
    return color;
}


bool PCB_RENDER_SETTINGS::GetShowPageLimits() const
{
    return pcbconfig() && pcbconfig()->m_ShowPageLimits;
}


PCB_PAINTER::PCB_PAINTER( GAL* aGal, FRAME_T aFrameType ) :
    PAINTER( aGal ),
    m_frameType( aFrameType ),
    m_maxError( ARC_HIGH_DEF ),
    m_holePlatingThickness( 0 ),
    m_lockedShadowMargin( 0 )
{
}


int PCB_PAINTER::getLineThickness( int aActualThickness ) const
{
    // if items have 0 thickness, draw them with the outline
    // width, otherwise respect the set value (which, no matter
    // how small will produce something)
    if( aActualThickness == 0 )
        return m_pcbSettings.m_outlineWidth;

    return aActualThickness;
}


PAD_DRILL_SHAPE PCB_PAINTER::getDrillShape( const PAD* aPad ) const
{
    return aPad->GetDrillShape();
}


SHAPE_SEGMENT PCB_PAINTER::getPadHoleShape( const PAD* aPad ) const
{
    SHAPE_SEGMENT segm = *aPad->GetEffectiveHoleShape().get();
    return segm;
}


int PCB_PAINTER::getViaDrillSize( const PCB_VIA* aVia ) const
{
    return aVia->GetDrillValue();
}


bool PCB_PAINTER::Draw( const VIEW_ITEM* aItem, int aLayer )
{
    if( !aItem->IsBOARD_ITEM() )
        return false;

    const BOARD_ITEM* item = static_cast<const BOARD_ITEM*>( aItem );

    if( const BOARD* board = item->GetBoard() )
    {
        BOARD_DESIGN_SETTINGS& bds = board->GetDesignSettings();
        m_maxError = bds.m_MaxError;
        m_holePlatingThickness = bds.GetHolePlatingThickness();
        m_lockedShadowMargin = bds.GetLineThickness( F_SilkS ) * 4;

        if( item->GetParentFootprint() && !board->IsFootprintHolder() )
        {
            FOOTPRINT* parentFP = item->GetParentFootprint();

            // Never draw footprint reference images on board
            if( item->Type() == PCB_REFERENCE_IMAGE_T )
            {
                return false;
            }
            else if( item->GetLayerSet().count() > 1 )
            {
                // For multi-layer objects, exclude only those layers that are private
                if( IsPcbLayer( aLayer ) && parentFP->GetPrivateLayers().test( aLayer ) )
                    return false;
            }
            else if( item->GetLayerSet().count() == 1 )
            {
                // For single-layer objects, exclude all layers including ancillary layers
                // such as holes, netnames, etc.
                PCB_LAYER_ID singleLayer = item->GetLayerSet().ExtractLayer();

                if( parentFP->GetPrivateLayers().test( singleLayer ) )
                    return false;
            }
        }
    }
    else
    {
        m_maxError = ARC_HIGH_DEF;
        m_holePlatingThickness = 0;
    }

    // the "cast" applied in here clarifies which overloaded draw() is called
    switch( item->Type() )
    {
    case PCB_TRACE_T:
        draw( static_cast<const PCB_TRACK*>( item ), aLayer );
        break;

    case PCB_ARC_T:
        draw( static_cast<const PCB_ARC*>( item ), aLayer );
        break;

    case PCB_VIA_T:
        draw( static_cast<const PCB_VIA*>( item ), aLayer );
        break;

    case PCB_PAD_T:
        draw( static_cast<const PAD*>( item ), aLayer );
        break;

    case PCB_SHAPE_T:
        draw( static_cast<const PCB_SHAPE*>( item ), aLayer );
        break;

    case PCB_REFERENCE_IMAGE_T:
        draw( static_cast<const PCB_REFERENCE_IMAGE*>( item ), aLayer );
        break;

    case PCB_FIELD_T:
        draw( static_cast<const PCB_FIELD*>( item ), aLayer );
        break;

    case PCB_TEXT_T:
        draw( static_cast<const PCB_TEXT*>( item ), aLayer );
        break;

    case PCB_TEXTBOX_T:
        draw( static_cast<const PCB_TEXTBOX*>( item ), aLayer );
        break;

    case PCB_TABLE_T:
        draw( static_cast<const PCB_TABLE*>( item ), aLayer );
        break;

    case PCB_FOOTPRINT_T:
        draw( static_cast<const FOOTPRINT*>( item ), aLayer );
        break;

    case PCB_GROUP_T:
        draw( static_cast<const PCB_GROUP*>( item ), aLayer );
        break;

    case PCB_ZONE_T:
        draw( static_cast<const ZONE*>( item ), aLayer );
        break;

    case PCB_DIM_ALIGNED_T:
    case PCB_DIM_CENTER_T:
    case PCB_DIM_RADIAL_T:
    case PCB_DIM_ORTHOGONAL_T:
    case PCB_DIM_LEADER_T:
        draw( static_cast<const PCB_DIMENSION_BASE*>( item ), aLayer );
        break;

    case PCB_BARCODE_T:
        draw( static_cast<const PCB_BARCODE*>( item ), aLayer );
        break;

    case PCB_TARGET_T:
        draw( static_cast<const PCB_TARGET*>( item ) );
        break;

    case PCB_POINT_T:
        draw( static_cast<const PCB_POINT*>( item ), aLayer );
        break;

    case PCB_MARKER_T:
        draw( static_cast<const PCB_MARKER*>( item ), aLayer );
        break;

    case PCB_BOARD_OUTLINE_T:
        draw( static_cast<const PCB_BOARD_OUTLINE*>( item ), aLayer );
        break;

    default:
        // Painter does not know how to draw the object
        return false;
    }

    // Draw bounding boxes after drawing objects so they can be seen.
    if( m_pcbSettings.GetDrawBoundingBoxes() )
    {
        // Show bounding boxes of painted objects for debugging.
        BOX2I box = item->GetBoundingBox();

        m_gal->SetIsFill( false );
        m_gal->SetIsStroke( true );

        if( item->Type() == PCB_FOOTPRINT_T )
        {
            m_gal->SetStrokeColor( item->IsSelected() ? COLOR4D( 1.0, 0.2, 0.2, 1 ) :
                                   COLOR4D( MAGENTA ) );
        }
        else
        {
            m_gal->SetStrokeColor( item->IsSelected() ? COLOR4D( 1.0, 0.2, 0.2, 1 ) :
                                   COLOR4D( 0.4, 0.4, 0.4, 1 ) );
        }

        m_gal->SetLineWidth( 1 );
        m_gal->DrawRectangle( box.GetOrigin(), box.GetEnd() );

        if( item->Type() == PCB_FOOTPRINT_T )
        {
            m_gal->SetStrokeColor( item->IsSelected() ? COLOR4D( 1.0, 0.2, 0.2, 1 ) :
                                   COLOR4D( CYAN ) );

            const FOOTPRINT* fp = static_cast<const FOOTPRINT*>( item );

            if( fp )
            {
                const SHAPE_POLY_SET& convex = fp->GetBoundingHull();

                m_gal->DrawPolyline( convex.COutline( 0 ) );
            }
        }
    }

    return true;
}


void PCB_PAINTER::draw( const PCB_TRACK* aTrack, int aLayer )
{
    VECTOR2I start( aTrack->GetStart() );
    VECTOR2I end( aTrack->GetEnd() );
    int      track_width = aTrack->GetWidth();
    COLOR4D  color       = m_pcbSettings.GetColor( aTrack, aLayer );

    if( IsNetnameLayer( aLayer ) )
    {
        if( !pcbconfig() || pcbconfig()->m_Display.m_NetNames < 2 )
            return;

        if( aTrack->GetNetCode() <= NETINFO_LIST::UNCONNECTED )
            return;

        SHAPE_SEGMENT trackShape( { aTrack->GetStart(), aTrack->GetEnd() }, aTrack->GetWidth() );
        renderNetNameForSegment( trackShape, color, aTrack->GetDisplayNetname() );
        return;
    }
    else if( IsCopperLayer( aLayer ) || IsSolderMaskLayer( aLayer )
                 || aLayer == LAYER_LOCKED_ITEM_SHADOW )
    {
        // Draw a regular track
        bool outline_mode = pcbconfig()
                            && !pcbconfig()->m_Display.m_DisplayPcbTrackFill
                            && aLayer != LAYER_LOCKED_ITEM_SHADOW;
        m_gal->SetStrokeColor( color );
        m_gal->SetFillColor( color );
        m_gal->SetIsStroke( outline_mode );
        m_gal->SetIsFill( not outline_mode );
        m_gal->SetLineWidth( m_pcbSettings.m_outlineWidth );

        if( IsSolderMaskLayer( aLayer ) )
            track_width = track_width + aTrack->GetSolderMaskExpansion() * 2;

        if( aLayer == LAYER_LOCKED_ITEM_SHADOW )
            track_width = track_width + m_lockedShadowMargin;

        m_gal->DrawSegment( start, end, track_width );
    }

    // Clearance lines
    if( IsClearanceLayer( aLayer ) && pcbconfig()
        && pcbconfig()->m_Display.m_TrackClearance == SHOW_WITH_VIA_ALWAYS
        && !m_pcbSettings.m_isPrinting )
    {
        const PCB_LAYER_ID copperLayerForClearance = ToLAYER_ID( aLayer - LAYER_CLEARANCE_START );

        int clearance = aTrack->GetOwnClearance( copperLayerForClearance );

        m_gal->SetLineWidth( m_pcbSettings.m_outlineWidth );
        m_gal->SetIsFill( false );
        m_gal->SetIsStroke( true );
        m_gal->SetStrokeColor( color );
        m_gal->DrawSegment( start, end, track_width + clearance * 2 );
    }
}


void PCB_PAINTER::renderNetNameForSegment( const SHAPE_SEGMENT& aSeg, const COLOR4D& aColor,
                                           const wxString& aNetName ) const
{
    // When drawing netnames, clip the track to the viewport
    BOX2D             viewport;
    VECTOR2D          screenSize = m_gal->GetScreenPixelSize();
    const MATRIX3x3D& matrix = m_gal->GetScreenWorldMatrix();

    viewport.SetOrigin( VECTOR2D( matrix * VECTOR2D( 0, 0 ) ) );
    viewport.SetEnd( VECTOR2D( matrix * screenSize ) );
    viewport.Normalize();

    int num_char = aNetName.size();

    // Check if the track is long enough to have a netname displayed
    int         seg_minlength = aSeg.GetWidth() * num_char;
    SEG::ecoord seg_minlength_sq = (SEG::ecoord)seg_minlength * seg_minlength;

    if( aSeg.GetSeg().SquaredLength() < seg_minlength_sq )
        return;

    double    textSize = aSeg.GetWidth();
    double    penWidth = textSize / 12.0;
    EDA_ANGLE textOrientation;
    int num_names = 1;

    VECTOR2I start = aSeg.GetSeg().A;
    VECTOR2I end   = aSeg.GetSeg().B;
    VECTOR2D segV  = end - start;

    if( end.y == start.y ) // horizontal
    {
        textOrientation = ANGLE_HORIZONTAL;
        num_names = std::max( num_names, KiROUND( aSeg.GetSeg().Length() / viewport.GetWidth() ) );
    }
    else if( end.x == start.x ) // vertical
    {
        textOrientation = ANGLE_VERTICAL;
        num_names = std::max( num_names, KiROUND( aSeg.GetSeg().Length() / viewport.GetHeight() ) );
    }
    else
    {
        textOrientation = -EDA_ANGLE( segV );
        textOrientation.Normalize90();

        double min_size = std::min( viewport.GetWidth(), viewport.GetHeight() );
        num_names = std::max( num_names, KiROUND( aSeg.GetSeg().Length() / ( M_SQRT2 * min_size ) ) );
    }

    m_gal->SetIsStroke( true );
    m_gal->SetIsFill( false );
    m_gal->SetStrokeColor( aColor );
    m_gal->SetLineWidth( penWidth );
    m_gal->SetFontBold( false );
    m_gal->SetFontItalic( false );
    m_gal->SetFontUnderlined( false );
    m_gal->SetTextMirrored( false );
    m_gal->SetGlyphSize( VECTOR2D( textSize * 0.55, textSize * 0.55 ) );
    m_gal->SetHorizontalJustify( GR_TEXT_H_ALIGN_CENTER );
    m_gal->SetVerticalJustify( GR_TEXT_V_ALIGN_CENTER );

    int divisions = num_names + 1;

    for( int ii = 1; ii < divisions; ++ii )
    {
        VECTOR2I textPosition = start + segV * ( (double) ii / divisions );

        if( viewport.Contains( textPosition ) )
            m_gal->BitmapText( aNetName, textPosition, textOrientation );
    }
}


void PCB_PAINTER::draw( const PCB_ARC* aArc, int aLayer )
{
    VECTOR2D  center( aArc->GetCenter() );
    int       width = aArc->GetWidth();
    COLOR4D   color = m_pcbSettings.GetColor( aArc, aLayer );
    double    radius = aArc->GetRadius();
    EDA_ANGLE start_angle = aArc->GetArcAngleStart();
    EDA_ANGLE angle = aArc->GetAngle();

    if( IsNetnameLayer( aLayer ) )
    {
        // Ummm, yeah.  Anyone fancy implementing text on a path?
        return;
    }
    else if( IsCopperLayer( aLayer ) || IsSolderMaskLayer( aLayer ) || aLayer == LAYER_LOCKED_ITEM_SHADOW )
    {
        // Draw a regular track
        bool outline_mode = pcbconfig()
                            && !pcbconfig()->m_Display.m_DisplayPcbTrackFill
                            && aLayer != LAYER_LOCKED_ITEM_SHADOW;
        m_gal->SetStrokeColor( color );
        m_gal->SetFillColor( color );
        m_gal->SetIsStroke( outline_mode );
        m_gal->SetIsFill( not outline_mode );
        m_gal->SetLineWidth( m_pcbSettings.m_outlineWidth );

        if( IsSolderMaskLayer( aLayer ) )
            width = width + aArc->GetSolderMaskExpansion() * 2;

        if( aLayer == LAYER_LOCKED_ITEM_SHADOW )
            width = width + m_lockedShadowMargin;

        m_gal->DrawArcSegment( center, radius, start_angle, angle, width, m_maxError );
    }

    // Clearance lines
    if( IsClearanceLayer( aLayer )
            && pcbconfig() && pcbconfig()->m_Display.m_TrackClearance == SHOW_WITH_VIA_ALWAYS
            && !m_pcbSettings.m_isPrinting )
    {
        /*
         * Showing the clearance area is not obvious for optionally-flashed pads and vias, so we
         * choose to not display clearance lines at all on non-copper active layers.  We follow
         * the same rule for tracks to be consistent (even though they don't have the same issue).
         */
        const PCB_LAYER_ID activeLayer = m_pcbSettings.GetActiveLayer();
        const BOARD&       board = *aArc->GetBoard();

        if( IsCopperLayer( activeLayer ) && board.GetVisibleLayers().test( activeLayer ) )
        {
            int clearance = aArc->GetOwnClearance( activeLayer );

            m_gal->SetLineWidth( m_pcbSettings.m_outlineWidth );
            m_gal->SetIsFill( false );
            m_gal->SetIsStroke( true );
            m_gal->SetStrokeColor( color );

            m_gal->DrawArcSegment( center, radius, start_angle, angle, width + clearance * 2, m_maxError );
        }
    }

#if 0
    // Debug only: enable this code only to test the TransformArcToPolygon function and display the polygon
    // outline created by it.
    // arcs on F_Cu are approximated with ERROR_INSIDE, others with ERROR_OUTSIDE
    SHAPE_POLY_SET cornerBuffer;
    ERROR_LOC errorloc = aLayer == F_Cu ? ERROR_LOC::ERROR_INSIDE : ERROR_LOC::ERROR_OUTSIDE;
    TransformArcToPolygon( cornerBuffer, aArc->GetStart(), aArc->GetMid(), aArc->GetEnd(), width,
                           m_maxError, errorloc );
    m_gal->SetLineWidth( m_pcbSettings.m_outlineWidth );
    m_gal->SetIsFill( false );
    m_gal->SetIsStroke( true );
    m_gal->SetStrokeColor( COLOR4D( 0, 0, 1.0, 1.0 ) );
    m_gal->DrawPolygon( cornerBuffer );
#endif

#if 0
    // Debug only: enable this code only to test the arc geometry.
    // Draw 3 lines from arc center to arc start, arc middle, arc end to show how the arc is defined
    SHAPE_ARC arc( aArc->GetStart(), aArc->GetMid(), aArc->GetEnd(), m_pcbSettings.m_outlineWidth );
    m_gal->SetIsFill( false );
    m_gal->SetIsStroke( true );
    m_gal->SetStrokeColor( COLOR4D( 0, 0, 1.0, 1.0 ) );
    m_gal->DrawSegment( arc.GetStart(), arc.GetCenter(), m_pcbSettings.m_outlineWidth );
    m_gal->DrawSegment( aArc->GetFocusPosition(), arc.GetCenter(), m_pcbSettings.m_outlineWidth );
    m_gal->DrawSegment( arc.GetEnd(), arc.GetCenter(), m_pcbSettings.m_outlineWidth );
#endif

#if 0
    // Debug only: enable this code only to test the SHAPE_ARC::ConvertToPolyline function and display the
    // polyline created by it.
    SHAPE_ARC arc( aArc->GetCenter(), aArc->GetStart(), aArc->GetAngle(), aArc->GetWidth() );
    SHAPE_LINE_CHAIN arcSpine = arc.ConvertToPolyline( m_maxError );
    m_gal->SetLineWidth( m_pcbSettings.m_outlineWidth );
    m_gal->SetIsFill( false );
    m_gal->SetIsStroke( true );
    m_gal->SetStrokeColor( COLOR4D( 0.3, 0.2, 0.5, 1.0 ) );

    for( int idx = 1; idx < arcSpine.PointCount(); idx++ )
        m_gal->DrawSegment( arcSpine.CPoint( idx-1 ), arcSpine.CPoint( idx ), aArc->GetWidth() );
#endif
}


void PCB_PAINTER::draw( const PCB_VIA* aVia, int aLayer )
{
    const BOARD* board = aVia->GetBoard();
    COLOR4D      color = m_pcbSettings.GetColor( aVia, aLayer );
    VECTOR2D     center( aVia->GetStart() );

    if( color == COLOR4D::CLEAR )
        return;

    const int copperLayer = IsViaCopperLayer( aLayer ) ? aLayer - LAYER_VIA_COPPER_START : aLayer;

    PCB_LAYER_ID currentLayer = ToLAYER_ID( copperLayer );
    PCB_LAYER_ID layerTop, layerBottom;
    aVia->LayerPair( &layerTop, &layerBottom );

    // Blind/buried vias (and microvias) will use different hole and label rendering
    bool isBlindBuried = aVia->GetViaType() == VIATYPE::BLIND
            || aVia->GetViaType() == VIATYPE::BURIED
            || ( aVia->GetViaType() == VIATYPE::MICROVIA
                 && ( layerTop != F_Cu || layerBottom != B_Cu ) );

    // Draw description layer
    if( IsNetnameLayer( aLayer ) )
    {
        VECTOR2D position( center );

        // Is anything that we can display enabled (netname and/or layers ids)?
        bool showNets =  pcbconfig() && pcbconfig()->m_Display.m_NetNames != 0
                         && !aVia->GetNetname().empty();
        bool showLayers = aVia->GetViaType() != VIATYPE::THROUGH;

        if( !showNets && !showLayers )
            return;

        double maxSize = PCB_RENDER_SETTINGS::MAX_FONT_SIZE;
        double size = aVia->GetWidth( currentLayer );

        // Font size limits
        if( size > maxSize )
            size = maxSize;

        m_gal->Save();
        m_gal->Translate( position );

        // Default font settings
        m_gal->ResetTextAttributes();
        m_gal->SetHorizontalJustify( GR_TEXT_H_ALIGN_CENTER );
        m_gal->SetVerticalJustify( GR_TEXT_V_ALIGN_CENTER );
        m_gal->SetFontBold( false );
        m_gal->SetFontItalic( false );
        m_gal->SetFontUnderlined( false );
        m_gal->SetTextMirrored( false );
        m_gal->SetStrokeColor( m_pcbSettings.GetColor( aVia, aLayer ) );
        m_gal->SetIsStroke( true );
        m_gal->SetIsFill( false );

        // Set the text position via position. if only one text, it is on the via position
        // For 2 lines, the netname is slightly below the center, and the layer IDs above
        // the netname
        VECTOR2D textpos( 0.0, 0.0 );

        wxString netname = aVia->GetDisplayNetname();

        PCB_LAYER_ID topLayerId = aVia->TopLayer();
        PCB_LAYER_ID bottomLayerId = aVia->BottomLayer();
        int topLayer;       // The via top layer number (from 1 to copper layer count)
        int bottomLayer;    // The via bottom layer number (from 1 to copper layer count)

        switch( topLayerId )
        {
        case F_Cu: topLayer = 1; break;
        case B_Cu: topLayer = board->GetCopperLayerCount(); break;
        default: topLayer = (topLayerId - B_Cu)/2 + 1; break;
        }

        switch( bottomLayerId )
        {
        case F_Cu: bottomLayer = 1; break;
        case B_Cu: bottomLayer = board->GetCopperLayerCount(); break;
        default: bottomLayer = (bottomLayerId - B_Cu)/2 + 1; break;
        }

        wxString layerIds;
#if wxUSE_UNICODE_WCHAR
        layerIds << std::to_wstring( topLayer ) << L'-' << std::to_wstring( bottomLayer );
#else
        layerIds << std::to_string( topLayer ) << '-' << std::to_string( bottomLayer );
#endif

        // a good size is set room for at least 6 chars, to be able to print 2 lines of text,
        // or at least 3 chars for only the netname
        // (The layerIds string has 5 chars max)
        int minCharCnt = showLayers ? 6 : 3;

        // approximate the size of netname and layerIds text:
        double tsize = 1.5 * size / std::max( PrintableCharCount( netname ), minCharCnt );
        tsize = std::min( tsize, size );

        // Use a smaller text size to handle interline, pen size..
        tsize *= 0.75;
        VECTOR2D namesize( tsize, tsize );

        // For 2 lines, adjust the text pos (move it a small amount to the bottom)
        if( showLayers && showNets )
            textpos.y += ( tsize * 1.3 )/ 2;

        m_gal->SetGlyphSize( namesize );
        m_gal->SetLineWidth( namesize.x / 10.0 );

        if( showNets )
            m_gal->BitmapText( netname, textpos, ANGLE_HORIZONTAL );

        if( showLayers )
        {
            if( showNets )
                textpos.y -= tsize * 1.3;

            m_gal->BitmapText( layerIds, textpos, ANGLE_HORIZONTAL );
        }

        m_gal->Restore();

        return;
    }

    bool outline_mode = pcbconfig() && !pcbconfig()->m_Display.m_DisplayViaFill;

    m_gal->SetStrokeColor( color );
    m_gal->SetFillColor( color );
    m_gal->SetIsStroke( true );
    m_gal->SetIsFill( false );

    if( outline_mode )
        m_gal->SetLineWidth( m_pcbSettings.m_outlineWidth );

    if( aLayer == LAYER_VIA_HOLEWALLS )
    {
        double thickness =
            m_holePlatingThickness * ADVANCED_CFG::GetCfg().m_HoleWallPaintingMultiplier;
        double radius = ( getViaDrillSize( aVia ) / 2.0 ) + thickness;

        if( !outline_mode )
        {
            m_gal->SetLineWidth( thickness );
            radius -= thickness / 2.0;
        }

        // Underpaint the hole so that there aren't artifacts at its edge
        m_gal->SetIsFill( true );

        m_gal->DrawCircle( center, radius );
    }
    else if( aLayer == LAYER_VIA_HOLES )
    {
        double radius = getViaDrillSize( aVia ) / 2.0;

        m_gal->SetIsStroke( false );
        m_gal->SetIsFill( true );

        if( isBlindBuried && !m_pcbSettings.IsPrinting() )
        {
            m_gal->SetIsStroke( false );
            m_gal->SetIsFill( true );

            m_gal->SetFillColor( m_pcbSettings.GetColor( aVia, layerTop ) );
            m_gal->DrawArc( center, radius, EDA_ANGLE( 180, DEGREES_T ),
                            EDA_ANGLE( 180, DEGREES_T ) );

            m_gal->SetFillColor( m_pcbSettings.GetColor( aVia, layerBottom ) );
            m_gal->DrawArc( center, radius, EDA_ANGLE( 0, DEGREES_T ),
                            EDA_ANGLE( 180, DEGREES_T ) );
        }
        else
        {
            m_gal->DrawCircle( center, radius );
        }

    }
    else if( ( aLayer == F_Mask && aVia->IsOnLayer( F_Mask ) )
             || ( aLayer == B_Mask && aVia->IsOnLayer( B_Mask ) ) )
    {
        int margin = board->GetDesignSettings().m_SolderMaskExpansion;

        m_gal->SetIsFill( true );
        m_gal->SetIsStroke( false );

        m_gal->SetLineWidth( margin );
        m_gal->DrawCircle( center, aVia->GetWidth( currentLayer ) / 2.0 + margin );
    }
    else if( m_pcbSettings.IsPrinting() || IsCopperLayer( currentLayer ) )
    {
        int    annular_width = ( aVia->GetWidth( currentLayer ) - getViaDrillSize( aVia ) ) / 2.0;
        double radius = aVia->GetWidth( currentLayer ) / 2.0;
        bool   draw = false;

        if( m_pcbSettings.IsPrinting() )
        {
            draw = aVia->FlashLayer( m_pcbSettings.GetPrintLayers() );
        }
        else if( aVia->IsSelected() )
        {
            draw = true;
        }
        else if( aVia->FlashLayer( board->GetVisibleLayers() & board->GetEnabledLayers() ) )
        {
            draw = true;
        }

        if( !aVia->FlashLayer( currentLayer ) )
            draw = false;

        if( !outline_mode )
        {
            m_gal->SetLineWidth( annular_width );
            radius -= annular_width / 2.0;
        }

        if( draw )
            m_gal->DrawCircle( center, radius );

        // Draw backdrill indicators (semi-circles extending into the hole)
        // Drawn on copper layer so they appear above the annular ring
        if( !m_pcbSettings.IsPrinting() && draw )
        {
            std::optional<int> secDrill = aVia->GetSecondaryDrillSize();
            std::optional<int> terDrill = aVia->GetTertiaryDrillSize();

            if( secDrill.value_or( 0 ) > 0 )
            {
                drawBackdrillIndicator( aVia, center, *secDrill,
                                        aVia->GetSecondaryDrillStartLayer(),
                                        aVia->GetSecondaryDrillEndLayer() );
            }

            if( terDrill.value_or( 0 ) > 0 )
            {
                drawBackdrillIndicator( aVia, center, *terDrill,
                                        aVia->GetTertiaryDrillStartLayer(),
                                        aVia->GetTertiaryDrillEndLayer() );
            }
        }

        // Draw post-machining indicator if this layer is post-machined
        if( !m_pcbSettings.IsPrinting() && draw )
        {
            drawPostMachiningIndicator( aVia, center, currentLayer );
        }
    }
    else if( aLayer == LAYER_LOCKED_ITEM_SHADOW )    // draw a ring around the via
    {
        m_gal->SetLineWidth( m_lockedShadowMargin );

        m_gal->DrawCircle( center, ( aVia->GetWidth( currentLayer ) + m_lockedShadowMargin ) / 2.0 );
    }

    // Clearance lines
    if( IsClearanceLayer( aLayer ) && pcbconfig()
        && pcbconfig()->m_Display.m_TrackClearance == SHOW_WITH_VIA_ALWAYS
        && !m_pcbSettings.m_isPrinting )
    {
        const PCB_LAYER_ID copperLayerForClearance = ToLAYER_ID( aLayer - LAYER_CLEARANCE_START );

        double radius;

        if( aVia->FlashLayer( copperLayerForClearance ) )
            radius = aVia->GetWidth( copperLayerForClearance ) / 2.0;
        else
            radius = getViaDrillSize( aVia ) / 2.0 + m_holePlatingThickness;

        m_gal->SetLineWidth( m_pcbSettings.m_outlineWidth );
        m_gal->SetIsFill( false );
        m_gal->SetIsStroke( true );
        m_gal->SetStrokeColor( color );
        m_gal->DrawCircle( center, radius + aVia->GetOwnClearance( copperLayerForClearance ) );
    }
}


void PCB_PAINTER::draw( const PAD* aPad, int aLayer )
{
    COLOR4D      color = m_pcbSettings.GetColor( aPad, aLayer );
    const int    copperLayer = IsPadCopperLayer( aLayer ) ? aLayer - LAYER_PAD_COPPER_START : aLayer;
    PCB_LAYER_ID pcbLayer = static_cast<PCB_LAYER_ID>( copperLayer );

    if( IsNetnameLayer( aLayer ) )
    {
        PCBNEW_SETTINGS::DISPLAY_OPTIONS* displayOpts = pcbconfig() ? &pcbconfig()->m_Display : nullptr;
        wxString                          netname;
        wxString                          padNumber;

        if( viewer_settings()->m_ViewersDisplay.m_DisplayPadNumbers )
        {
            padNumber = UnescapeString( aPad->GetNumber() );

            if( dynamic_cast<CVPCB_SETTINGS*>( viewer_settings() ) )
                netname = aPad->GetPinFunction();
        }

        if( displayOpts && !dynamic_cast<CVPCB_SETTINGS*>( viewer_settings() ) )
        {
            if( displayOpts->m_NetNames == 1 || displayOpts->m_NetNames == 3 )
                netname = aPad->GetDisplayNetname();

            if( aPad->IsNoConnectPad() )
                netname = wxT( "x" );
            else if( aPad->IsFreePad() )
                netname = wxT( "*" );
        }

        if( netname.IsEmpty() && padNumber.IsEmpty() )
            return;

        BOX2I    padBBox = aPad->GetBoundingBox();
        VECTOR2D position = padBBox.Centre();
        VECTOR2D padsize = VECTOR2D( padBBox.GetSize() );

        if( aPad->IsEntered() )
        {
            FOOTPRINT* fp = aPad->GetParentFootprint();

            // Find the number box
            for( const BOARD_ITEM* aItem : fp->GraphicalItems() )
            {
                if( aItem->Type() == PCB_SHAPE_T )
                {
                    const PCB_SHAPE* shape = static_cast<const PCB_SHAPE*>( aItem );

                    if( shape->IsProxyItem() && shape->GetShape() == SHAPE_T::RECTANGLE )
                    {
                        position = shape->GetCenter();
                        padsize = shape->GetBotRight() - shape->GetTopLeft();

                        // We normally draw a bit outside the pad, but this will be somewhat
                        // unexpected when the user has drawn a box.
                        padsize *= 0.9;

                        break;
                    }
                }
            }
        }
        else if( aPad->GetShape( pcbLayer ) == PAD_SHAPE::CUSTOM )
        {
            // See if we have a number box
            for( const std::shared_ptr<PCB_SHAPE>& primitive : aPad->GetPrimitives( pcbLayer ) )
            {
                if( primitive->IsProxyItem() && primitive->GetShape() == SHAPE_T::RECTANGLE )
                {
                    position = primitive->GetCenter();
                    RotatePoint( position, aPad->GetOrientation() );
                    position += aPad->ShapePos( pcbLayer );

                    padsize.x = abs( primitive->GetBotRight().x - primitive->GetTopLeft().x );
                    padsize.y = abs( primitive->GetBotRight().y - primitive->GetTopLeft().y );

                    // We normally draw a bit outside the pad, but this will be somewhat
                    // unexpected when the user has drawn a box.
                    padsize *= 0.9;

                    break;
                }
            }
        }

        if( aPad->GetShape( pcbLayer ) != PAD_SHAPE::CUSTOM )
        {
            // Don't allow a 45° rotation to bloat a pad's bounding box unnecessarily
            double limit = std::min( aPad->GetSize( pcbLayer ).x,
                                     aPad->GetSize( pcbLayer ).y ) * 1.1;

            if( padsize.x > limit && padsize.y > limit )
            {
                padsize.x = limit;
                padsize.y = limit;
            }
        }

        double maxSize = PCB_RENDER_SETTINGS::MAX_FONT_SIZE;
        double size = padsize.y;

        m_gal->Save();
        m_gal->Translate( position );

        // Keep the size ratio for the font, but make it smaller
        if( padsize.x < ( padsize.y * 0.95 ) )
        {
            m_gal->Rotate( -ANGLE_90.AsRadians() );
            size = padsize.x;
            std::swap( padsize.x, padsize.y );
        }

        // Font size limits
        if( size > maxSize )
            size = maxSize;

        // Default font settings
        m_gal->ResetTextAttributes();
        m_gal->SetHorizontalJustify( GR_TEXT_H_ALIGN_CENTER );
        m_gal->SetVerticalJustify( GR_TEXT_V_ALIGN_CENTER );
        m_gal->SetFontBold( false );
        m_gal->SetFontItalic( false );
        m_gal->SetFontUnderlined( false );
        m_gal->SetTextMirrored( false );
        m_gal->SetStrokeColor( m_pcbSettings.GetColor( aPad, aLayer ) );
        m_gal->SetIsStroke( true );
        m_gal->SetIsFill( false );

        // We have already translated the GAL to be centered at the center of the pad's
        // bounding box
        VECTOR2I textpos( 0, 0 );

        // Divide the space, to display both pad numbers and netnames and set the Y text
        // offset position to display 2 lines
        int Y_offset_numpad = 0;
        int Y_offset_netname = 0;

        if( !netname.IsEmpty() && !padNumber.IsEmpty() )
        {
            // The magic numbers are defined experimentally for a better look.
            size = size / 2.5;
            Y_offset_netname = size / 1.4;  // netname size is usually smaller than num pad
                                            // so the offset can be smaller
            Y_offset_numpad = size / 1.7;
        }

        // We are using different fonts to display names, depending on the graphic
        // engine (OpenGL or Cairo).
        // Xscale_for_stroked_font adjust the text X size for cairo (stroke fonts) engine
        const double Xscale_for_stroked_font = 0.9;

        if( !netname.IsEmpty() )
        {
            // approximate the size of net name text:
            // We use a size for at least 5 chars, to give a good look even for short names
            // (like VCC, GND...)
            double tsize = 1.5 * padsize.x / std::max( PrintableCharCount( netname )+1, 5 );
            tsize = std::min( tsize, size );

            // Use a smaller text size to handle interline, pen size...
            tsize *= 0.85;

            // Round and oval pads have less room to display the net name than other
            // (i.e RECT) shapes, so reduce the text size for these shapes
            if( aPad->GetShape( pcbLayer ) == PAD_SHAPE::CIRCLE
                || aPad->GetShape( pcbLayer ) == PAD_SHAPE::OVAL )
            {
                tsize *= 0.9;
            }

            VECTOR2D namesize( tsize*Xscale_for_stroked_font, tsize );
            textpos.y = std::min( tsize * 1.4, double( Y_offset_netname ) );

            m_gal->SetGlyphSize( namesize );
            m_gal->SetLineWidth( namesize.x / 6.0 );
            m_gal->SetFontBold( true );
            m_gal->BitmapText( netname, textpos, ANGLE_HORIZONTAL );
        }

        if( !padNumber.IsEmpty() )
        {
            // approximate the size of the pad number text:
            // We use a size for at least 3 chars, to give a good look even for short numbers
            double tsize = 1.5 * padsize.x / std::max( PrintableCharCount( padNumber ), 3 );
            tsize = std::min( tsize, size );

            // Use a smaller text size to handle interline, pen size...
            tsize *= 0.85;
            tsize = std::min( tsize, size );
            VECTOR2D numsize( tsize*Xscale_for_stroked_font, tsize );
            textpos.y = -Y_offset_numpad;

            m_gal->SetGlyphSize( numsize );
            m_gal->SetLineWidth( numsize.x / 6.0 );
            m_gal->SetFontBold( true );
            m_gal->BitmapText( padNumber, textpos, ANGLE_HORIZONTAL );
        }

        m_gal->Restore();

        return;
    }
    else if( aLayer == LAYER_PAD_HOLEWALLS )
    {
        m_gal->SetIsFill( true );
        m_gal->SetIsStroke( false );
        double widthFactor = ADVANCED_CFG::GetCfg().m_HoleWallPaintingMultiplier;
        double lineWidth = widthFactor * m_holePlatingThickness;
        lineWidth = std::min( lineWidth, aPad->GetSizeX() / 2.0 );
        lineWidth = std::min( lineWidth, aPad->GetSizeY() / 2.0 );

        m_gal->SetFillColor( color );
        m_gal->SetMinLineWidth( lineWidth );

        std::shared_ptr<SHAPE_SEGMENT> slot = aPad->GetEffectiveHoleShape();

        if( slot->GetSeg().A == slot->GetSeg().B )    // Circular hole
        {
            double holeRadius = slot->GetWidth() / 2.0;
            m_gal->DrawHoleWall( slot->GetSeg().A, holeRadius, lineWidth );
        }
        else
        {
            int holeSize = slot->GetWidth() + ( 2 * lineWidth );
            m_gal->DrawSegment( slot->GetSeg().A, slot->GetSeg().B, holeSize );
        }

        m_gal->SetMinLineWidth( 1.0 );

        return;
    }

    bool outline_mode = !viewer_settings()->m_ViewersDisplay.m_DisplayPadFill;

    if( m_pcbSettings.m_ForcePadSketchModeOn )
        outline_mode = true;

    bool drawShape = false;

    if( m_pcbSettings.IsPrinting() )
    {
        drawShape = aPad->FlashLayer( m_pcbSettings.GetPrintLayers() );
    }
    else if( ( aLayer < PCB_LAYER_ID_COUNT || IsPadCopperLayer( aLayer ) )
             && aPad->FlashLayer( pcbLayer ) )
    {
        drawShape = true;
    }
    else if( aPad->IsSelected() )
    {
        drawShape = true;
        outline_mode = true;
    }
    else if( aLayer == LAYER_LOCKED_ITEM_SHADOW )
    {
        drawShape = true;
        outline_mode = false;
    }

    // Plated holes are always filled as they use a solid BG fill to
    // draw the "hole" over the hole-wall segment/circle.
    if( outline_mode && aLayer != LAYER_PAD_PLATEDHOLES )
    {
        // Outline mode
        m_gal->SetIsFill( false );
        m_gal->SetIsStroke( true );
        m_gal->SetLineWidth( m_pcbSettings.m_outlineWidth );
        m_gal->SetStrokeColor( color );
    }
    else
    {
        // Filled mode
        m_gal->SetIsFill( true );
        m_gal->SetIsStroke( false );
        m_gal->SetFillColor( color );
    }

    if( aLayer == LAYER_PAD_PLATEDHOLES || aLayer == LAYER_NON_PLATEDHOLES )
    {
        SHAPE_SEGMENT slot = getPadHoleShape( aPad );
        VECTOR2I center = slot.GetSeg().A;

        if( slot.GetSeg().A == slot.GetSeg().B )    // Circular hole
            m_gal->DrawCircle( center, slot.GetWidth() / 2.0 );
        else
            m_gal->DrawSegment( slot.GetSeg().A, slot.GetSeg().B, slot.GetWidth() );
    }
    else if( drawShape )
    {
        VECTOR2I pad_size = aPad->GetSize( pcbLayer );
        VECTOR2I margin;

        auto getExpansion =
                [&]( PCB_LAYER_ID layer )
                {
                    VECTOR2I expansion;

                    switch( aLayer )
                    {
                    case F_Mask:
                    case B_Mask:
                        expansion.x = expansion.y = aPad->GetSolderMaskExpansion( layer );
                        break;

                    case F_Paste:
                    case B_Paste:
                        expansion = aPad->GetSolderPasteMargin( layer );
                        break;

                    default:
                        expansion.x = expansion.y = 0;
                        break;
                    }

                    return expansion;
                };

        if( aLayer == LAYER_LOCKED_ITEM_SHADOW )
        {
            LSET visibleLayers = aPad->GetBoard()->GetVisibleLayers()
                                        & aPad->GetBoard()->GetEnabledLayers()
                                        & aPad->GetLayerSet();

            for( PCB_LAYER_ID layer : visibleLayers )
                margin = std::max( margin, getExpansion( layer ) );

            margin.x += m_lockedShadowMargin / 2;
            margin.y += m_lockedShadowMargin / 2;
        }
        else
        {
            margin = getExpansion( pcbLayer );
        }

        std::unique_ptr<PAD>            dummyPad;
        std::shared_ptr<SHAPE_COMPOUND> shapes;

        // Drawing components of compound shapes in outline mode produces a mess.
        bool simpleShapes = !outline_mode;

        if( simpleShapes )
        {
            if( ( margin.x != margin.y && aPad->GetShape( pcbLayer ) != PAD_SHAPE::CUSTOM )
                || ( aPad->GetShape( pcbLayer ) == PAD_SHAPE::ROUNDRECT
                     && ( margin.x < 0 || margin.y < 0 ) ) )
            {
                // Our algorithms below (polygon inflation in particular) can't handle differential
                // inflation along separate axes.  So for those cases we build a dummy pad instead,
                // and inflate it.

                // Margin is added to both sides.  If the total margin is larger than the pad
                // then don't display this layer
                if( pad_size.x + 2 * margin.x <= 0 || pad_size.y + 2 * margin.y <= 0 )
                    return;

                dummyPad.reset( static_cast<PAD*>( aPad->Duplicate( IGNORE_PARENT_GROUP ) ) );

                int initial_radius = dummyPad->GetRoundRectCornerRadius( pcbLayer );

                dummyPad->SetSize( pcbLayer, pad_size + margin + margin );

                if( dummyPad->GetShape( pcbLayer ) == PAD_SHAPE::ROUNDRECT )
                {
                    // To keep the right margin around the corners, we need to modify the corner radius.
                    // We must have only one radius correction, so use the smallest absolute margin.
                    int radius_margin = std::max( margin.x, margin.y );     // radius_margin is < 0
                    dummyPad->SetRoundRectCornerRadius(
                            pcbLayer, std::max( initial_radius + radius_margin, 0 ) );
                }

                shapes = std::dynamic_pointer_cast<SHAPE_COMPOUND>(
                        dummyPad->GetEffectiveShape( pcbLayer ) );
                margin.x = margin.y = 0;
            }
            else
            {
                shapes = std::dynamic_pointer_cast<SHAPE_COMPOUND>(
                        aPad->GetEffectiveShape( pcbLayer ) );
            }

            // The dynamic cast above will fail if the pad returned the hole shape or a null shape
            // instead of a SHAPE_COMPOUND, which happens if we're on a copper layer and the pad has
            // no shape on that layer.
            if( !shapes )
                return;

            if( aPad->GetShape( pcbLayer ) == PAD_SHAPE::CUSTOM && ( margin.x || margin.y ) )
            {
                // We can't draw as shapes because we don't know which edges are internal and which
                // are external (so we don't know when to apply the margin and when not to).
                simpleShapes = false;
            }

            for( const SHAPE* shape : shapes->Shapes() )
            {
                if( !simpleShapes )
                    break;

                switch( shape->Type() )
                {
                case SH_SEGMENT:
                case SH_CIRCLE:
                case SH_RECT:
                case SH_SIMPLE:
                    // OK so far
                    break;

                default:
                    // Not OK
                    simpleShapes = false;
                    break;
                }
            }
        }

        const auto drawOneSimpleShape =
                [&]( const SHAPE& aShape )
                {
                    switch( aShape.Type() )
                    {
                    case SH_SEGMENT:
                    {
                        const SHAPE_SEGMENT& seg = (const SHAPE_SEGMENT&) aShape;
                        int                  effectiveWidth = seg.GetWidth() + 2 * margin.x;

                        if( effectiveWidth > 0 )
                            m_gal->DrawSegment( seg.GetSeg().A, seg.GetSeg().B, effectiveWidth );

                        break;
                    }

                    case SH_CIRCLE:
                    {
                        const SHAPE_CIRCLE& circle = (const SHAPE_CIRCLE&) aShape;
                        int                 effectiveRadius = circle.GetRadius() + margin.x;

                        if( effectiveRadius > 0 )
                            m_gal->DrawCircle( circle.GetCenter(), effectiveRadius );

                        break;
                    }

                    case SH_RECT:
                    {
                        const SHAPE_RECT& r = (const SHAPE_RECT&) aShape;
                        VECTOR2I          pos = r.GetPosition();
                        VECTOR2I          effectiveMargin = margin;

                        if( effectiveMargin.x < 0 )
                        {
                            // A negative margin just produces a smaller rect.
                            VECTOR2I effectiveSize = r.GetSize() + effectiveMargin;

                            if( effectiveSize.x > 0 && effectiveSize.y > 0 )
                                m_gal->DrawRectangle( pos - effectiveMargin, pos + effectiveSize );
                        }
                        else if( effectiveMargin.x > 0 )
                        {
                            // A positive margin produces a larger rect, but with rounded corners
                            m_gal->DrawRectangle( r.GetPosition(), r.GetPosition() + r.GetSize() );

                            // Use segments to produce the margin with rounded corners
                            m_gal->DrawSegment( pos,
                                                pos + VECTOR2I( r.GetWidth(), 0 ),
                                                effectiveMargin.x * 2 );
                            m_gal->DrawSegment( pos + VECTOR2I( r.GetWidth(), 0 ),
                                                pos + r.GetSize(),
                                                effectiveMargin.x * 2 );
                            m_gal->DrawSegment( pos + r.GetSize(),
                                                pos + VECTOR2I( 0, r.GetHeight() ),
                                                effectiveMargin.x * 2 );
                            m_gal->DrawSegment( pos + VECTOR2I( 0, r.GetHeight() ),
                                                pos,
                                                effectiveMargin.x * 2 );
                        }
                        else
                        {
                            m_gal->DrawRectangle( r.GetPosition(), r.GetPosition() + r.GetSize() );
                        }

                        break;
                    }

                    case SH_SIMPLE:
                    {
                        const SHAPE_SIMPLE& poly = static_cast<const SHAPE_SIMPLE&>( aShape );

                        if( poly.PointCount() < 2 ) // Careful of empty pads
                            break;

                        if( margin.x < 0 ) // The poly shape must be deflated
                        {
                            SHAPE_POLY_SET outline;
                            outline.NewOutline();

                            for( int ii = 0; ii < poly.PointCount(); ++ii )
                                outline.Append( poly.CPoint( ii ) );

                            outline.Deflate( -margin.x, CORNER_STRATEGY::CHAMFER_ALL_CORNERS, m_maxError );

                            m_gal->DrawPolygon( outline );
                        }
                        else
                        {
                            m_gal->DrawPolygon( poly.Vertices() );
                        }

                        // Now add on a rounded margin (using segments) if the margin > 0
                        if( margin.x > 0 )
                        {
                            for( size_t ii = 0; ii < poly.GetSegmentCount(); ++ii )
                            {
                                SEG seg = poly.GetSegment( ii );
                                m_gal->DrawSegment( seg.A, seg.B, margin.x * 2 );
                            }
                        }

                        break;
                    }

                    default:
                        // Better not get here; we already pre-flighted the shapes...
                        break;
                    }
                };

        if( simpleShapes )
        {
            for( const SHAPE* shape : shapes->Shapes() )
                drawOneSimpleShape( *shape );
        }
        else
        {
            // This is expensive.  Avoid if possible.
            SHAPE_POLY_SET polySet;
            aPad->TransformShapeToPolygon( polySet, ToLAYER_ID( aLayer ), margin.x, m_maxError, ERROR_INSIDE );
            m_gal->DrawPolygon( polySet );
        }

        // Draw post-machining indicator if this layer is post-machined
        if( !m_pcbSettings.IsPrinting() && aPad->GetDrillSizeX() > 0 )
        {
            VECTOR2D holePos = aPad->GetPosition() + aPad->GetOffset( pcbLayer );

            // Draw backdrill indicators (semi-circles extending into the hole)
            // Drawn on copper layer so they appear above the annular ring
            VECTOR2I secDrill = aPad->GetSecondaryDrillSize();
            VECTOR2I terDrill = aPad->GetTertiaryDrillSize();

            if( secDrill.x > 0 )
            {
                drawBackdrillIndicator( aPad, holePos, secDrill.x,
                                        aPad->GetSecondaryDrillStartLayer(),
                                        aPad->GetSecondaryDrillEndLayer() );
            }

            if( terDrill.x > 0 )
            {
                drawBackdrillIndicator( aPad, holePos, terDrill.x,
                                        aPad->GetTertiaryDrillStartLayer(),
                                        aPad->GetTertiaryDrillEndLayer() );
            }

            drawPostMachiningIndicator( aPad, holePos, pcbLayer );
        }
    }

    if( IsClearanceLayer( aLayer )
        && ( ( pcbconfig() && pcbconfig()->m_Display.m_PadClearance ) || !pcbconfig() )
        && !m_pcbSettings.m_isPrinting )
    {
        const PCB_LAYER_ID copperLayerForClearance = ToLAYER_ID( aLayer - LAYER_CLEARANCE_START );

        if( aPad->GetAttribute() == PAD_ATTRIB::NPTH )
            color = m_pcbSettings.GetLayerColor( LAYER_NON_PLATEDHOLES );

        m_gal->SetLineWidth( m_pcbSettings.m_outlineWidth );
        m_gal->SetIsStroke( true );
        m_gal->SetIsFill( false );
        m_gal->SetStrokeColor( color );

        const int clearance = aPad->GetOwnClearance( copperLayerForClearance );

        if( aPad->FlashLayer( copperLayerForClearance ) && clearance > 0 )
        {
            auto shape = std::dynamic_pointer_cast<SHAPE_COMPOUND>( aPad->GetEffectiveShape( pcbLayer ) );

            if( shape && shape->Size() == 1 && shape->Shapes()[0]->Type() == SH_SEGMENT )
            {
                const SHAPE_SEGMENT* seg = (SHAPE_SEGMENT*) shape->Shapes()[0];
                m_gal->DrawSegment( seg->GetSeg().A, seg->GetSeg().B,
                                    seg->GetWidth() + 2 * clearance );
            }
            else if( shape && shape->Size() == 1 && shape->Shapes()[0]->Type() == SH_CIRCLE )
            {
                const SHAPE_CIRCLE* circle = (SHAPE_CIRCLE*) shape->Shapes()[0];
                m_gal->DrawCircle( circle->GetCenter(), circle->GetRadius() + clearance );
            }
            else
            {
                SHAPE_POLY_SET polySet;

                // Use ERROR_INSIDE because it avoids Clipper and is therefore much faster.
                aPad->TransformShapeToPolygon( polySet, copperLayerForClearance, clearance,
                                               m_maxError, ERROR_INSIDE );

                if( polySet.Outline( 0 ).PointCount() > 2 ) // Careful of empty pads
                    m_gal->DrawPolygon( polySet );
            }
        }
        else if( aPad->GetEffectiveHoleShape() && clearance > 0 )
        {
            std::shared_ptr<SHAPE_SEGMENT> slot = aPad->GetEffectiveHoleShape();
            m_gal->DrawSegment( slot->GetSeg().A, slot->GetSeg().B,
                                slot->GetWidth() + 2 * clearance );
        }
    }
}


void PCB_PAINTER::draw( const PCB_SHAPE* aShape, int aLayer )
{
    COLOR4D    color = m_pcbSettings.GetColor( aShape, aLayer );
    bool       outline_mode = !viewer_settings()->m_ViewersDisplay.m_DisplayGraphicsFill;
    int        thickness = getLineThickness( aShape->GetWidth() );
    LINE_STYLE lineStyle = aShape->GetStroke().GetLineStyle();
    bool       isSolidFill = aShape->IsSolidFill();
    bool       isHatchedFill = aShape->IsHatchedFill();

    if( lineStyle == LINE_STYLE::DEFAULT )
        lineStyle = LINE_STYLE::SOLID;

    if( IsSolderMaskLayer( aLayer )
            && aShape->HasSolderMask()
            && IsExternalCopperLayer( aShape->GetLayer() ) )
    {
        lineStyle = LINE_STYLE::SOLID;
        thickness += aShape->GetSolderMaskExpansion() * 2;

        if( isHatchedFill )
        {
            isSolidFill = true;
            isHatchedFill = false;
        }
    }

    if( IsNetnameLayer( aLayer ) )
    {
        // Net names are shown only in board editor:
        if( m_frameType != FRAME_T::FRAME_PCB_EDITOR )
            return;

        if( !pcbconfig() || pcbconfig()->m_Display.m_NetNames < 2 )
            return;

        if( aShape->GetNetCode() <= NETINFO_LIST::UNCONNECTED )
            return;

        const wxString& netname = aShape->GetDisplayNetname();

        if( netname.IsEmpty() )
            return;

        if( aShape->GetShape() == SHAPE_T::SEGMENT )
        {
            SHAPE_SEGMENT seg( { aShape->GetStart(), aShape->GetEnd() }, aShape->GetWidth() );
            renderNetNameForSegment( seg, color, netname );
            return;
        }

        // TODO: Maybe use some of the pad code?

        return;
    }

    if( aLayer == LAYER_LOCKED_ITEM_SHADOW )
    {
        color = m_pcbSettings.GetColor( aShape, aLayer );
        thickness = thickness + m_lockedShadowMargin;

        // Note: on LAYER_LOCKED_ITEM_SHADOW always draw shadow shapes as continuous lines
        // otherwise the look is very strange and ugly
        lineStyle = LINE_STYLE::SOLID;
    }

    if( outline_mode )
    {
        m_gal->SetIsFill( false );
        m_gal->SetIsStroke( true );
        m_gal->SetLineWidth( m_pcbSettings.m_outlineWidth );
    }

    m_gal->SetFillColor( color );
    m_gal->SetStrokeColor( color );

    if( lineStyle == LINE_STYLE::SOLID || aShape->IsSolidFill() )
    {
        switch( aShape->GetShape() )
        {
        case SHAPE_T::SEGMENT:
            if( aShape->IsProxyItem() )
            {
                std::vector<VECTOR2I> pts;
                VECTOR2I offset = ( aShape->GetEnd() - aShape->GetStart() ).Perpendicular();
                offset = offset.Resize( thickness / 2 );

                pts.push_back( aShape->GetStart() + offset );
                pts.push_back( aShape->GetStart() - offset );
                pts.push_back( aShape->GetEnd() - offset );
                pts.push_back( aShape->GetEnd() + offset );

                m_gal->SetLineWidth( m_pcbSettings.m_outlineWidth );
                m_gal->DrawLine( pts[0], pts[1] );
                m_gal->DrawLine( pts[1], pts[2] );
                m_gal->DrawLine( pts[2], pts[3] );
                m_gal->DrawLine( pts[3], pts[0] );
                m_gal->DrawLine( ( pts[0] + pts[1] ) / 2, ( pts[1] + pts[2] ) / 2 );
                m_gal->DrawLine( ( pts[1] + pts[2] ) / 2, ( pts[2] + pts[3] ) / 2 );
                m_gal->DrawLine( ( pts[2] + pts[3] ) / 2, ( pts[3] + pts[0] ) / 2 );
                m_gal->DrawLine( ( pts[3] + pts[0] ) / 2, ( pts[0] + pts[1] ) / 2 );
            }
            else if( outline_mode )
            {
                m_gal->DrawSegment( aShape->GetStart(), aShape->GetEnd(), thickness );
            }
            else if( lineStyle == LINE_STYLE::SOLID )
            {
                m_gal->SetIsFill( true );
                m_gal->SetIsStroke( false );

                m_gal->DrawSegment( aShape->GetStart(), aShape->GetEnd(), thickness );
            }

            break;

        case SHAPE_T::RECTANGLE:
        {
            if( aShape->GetCornerRadius() > 0 )
            {
                // Creates a normalized ROUNDRECT item
                // (GetRectangleWidth() and GetRectangleHeight() can be < 0 with transforms
                ROUNDRECT rr( SHAPE_RECT( aShape->GetStart(), aShape->GetRectangleWidth(),
                                          aShape->GetRectangleHeight() ),
                              aShape->GetCornerRadius(), true /* normalize */  );
                SHAPE_POLY_SET poly;
                rr.TransformToPolygon( poly, aShape->GetMaxError() );
                SHAPE_LINE_CHAIN outline = poly.Outline( 0 );

                if( aShape->IsProxyItem() )
                {
                    m_gal->SetLineWidth( m_pcbSettings.m_outlineWidth );
                    m_gal->DrawPolygon( outline );
                }
                else if( outline_mode )
                {
                    m_gal->DrawSegmentChain( outline, thickness );
                }
                else
                {
                    m_gal->SetIsFill( true );
                    m_gal->SetIsStroke( false );

                    if( lineStyle == LINE_STYLE::SOLID && thickness > 0 )
                    {
                        m_gal->DrawSegmentChain( outline, thickness );
                    }

                    if( isSolidFill )
                    {
                        if( thickness < 0 )
                        {
                            SHAPE_POLY_SET deflated_shape = outline;
                            deflated_shape.Inflate( thickness / 2, CORNER_STRATEGY::ROUND_ALL_CORNERS, m_maxError );
                            m_gal->DrawPolygon( deflated_shape );
                        }
                        else
                        {
                            m_gal->DrawPolygon( outline );
                        }
                    }
                }
            }
            else
            {
                std::vector<VECTOR2I> pts = aShape->GetRectCorners();

                if( aShape->IsProxyItem() )
                {
                    m_gal->SetLineWidth( m_pcbSettings.m_outlineWidth );
                    m_gal->DrawLine( pts[0], pts[1] );
                    m_gal->DrawLine( pts[1], pts[2] );
                    m_gal->DrawLine( pts[2], pts[3] );
                    m_gal->DrawLine( pts[3], pts[0] );
                    m_gal->DrawLine( pts[0], pts[2] );
                    m_gal->DrawLine( pts[1], pts[3] );
                }
                else if( outline_mode )
                {
                    m_gal->DrawSegment( pts[0], pts[1], thickness );
                    m_gal->DrawSegment( pts[1], pts[2], thickness );
                    m_gal->DrawSegment( pts[2], pts[3], thickness );
                    m_gal->DrawSegment( pts[3], pts[0], thickness );
                }
                else
                {
                    m_gal->SetIsFill( true );
                    m_gal->SetIsStroke( false );

                    if( lineStyle == LINE_STYLE::SOLID && thickness > 0 )
                    {
                        m_gal->DrawSegment( pts[0], pts[1], thickness );
                        m_gal->DrawSegment( pts[1], pts[2], thickness );
                        m_gal->DrawSegment( pts[2], pts[3], thickness );
                        m_gal->DrawSegment( pts[3], pts[0], thickness );
                    }

                    if( isSolidFill )
                    {
                        SHAPE_POLY_SET poly;
                        poly.NewOutline();

                        for( const VECTOR2I& pt : pts )
                            poly.Append( pt );

                        if( thickness < 0 )
                            poly.Inflate( thickness / 2, CORNER_STRATEGY::ROUND_ALL_CORNERS,
                                          m_maxError );

                        m_gal->DrawPolygon( poly );
                    }
                }
            }

            break;
        }

        case SHAPE_T::ARC:
        {
            EDA_ANGLE startAngle;
            EDA_ANGLE endAngle;
            aShape->CalcArcAngles( startAngle, endAngle );

            if( outline_mode )
            {
                m_gal->DrawArcSegment( aShape->GetCenter(), aShape->GetRadius(), startAngle,
                                       endAngle - startAngle, thickness, m_maxError );
            }
            else if( lineStyle == LINE_STYLE::SOLID )
            {
                m_gal->SetIsFill( true );
                m_gal->SetIsStroke( false );

                m_gal->DrawArcSegment( aShape->GetCenter(), aShape->GetRadius(), startAngle,
                                       endAngle - startAngle, thickness, m_maxError );
            }
            break;
        }

        case SHAPE_T::CIRCLE:
            if( outline_mode )
            {
                m_gal->DrawCircle( aShape->GetStart(), aShape->GetRadius() - thickness / 2 );
                m_gal->DrawCircle( aShape->GetStart(), aShape->GetRadius() + thickness / 2 );
            }
            else
            {
                m_gal->SetIsFill( aShape->IsSolidFill() );
                m_gal->SetIsStroke( lineStyle == LINE_STYLE::SOLID && thickness > 0 );
                m_gal->SetLineWidth( thickness );

                int radius = aShape->GetRadius();

                if( lineStyle == LINE_STYLE::SOLID && thickness > 0 )
                {
                    m_gal->DrawCircle( aShape->GetStart(), radius );
                }
                else if( isSolidFill )
                {
                    if( thickness < 0 )
                    {
                        radius += thickness / 2;
                        radius = std::max( radius, 0 );
                    }

                    m_gal->DrawCircle( aShape->GetStart(), radius );
                }
            }
            break;

        case SHAPE_T::POLY:
        {
            SHAPE_POLY_SET&  shape = const_cast<PCB_SHAPE*>( aShape )->GetPolyShape();

            if( shape.OutlineCount() == 0 )
                break;

            if( outline_mode )
            {
                for( int ii = 0; ii < shape.OutlineCount(); ++ii )
                    m_gal->DrawSegmentChain( shape.Outline( ii ), thickness );
            }
            else
            {
                m_gal->SetIsFill( true );
                m_gal->SetIsStroke( false );

                if( lineStyle == LINE_STYLE::SOLID && thickness > 0 )
                {
                    for( int ii = 0; ii < shape.OutlineCount(); ++ii )
                        m_gal->DrawSegmentChain( shape.Outline( ii ), thickness );
                }

                if( isSolidFill )
                {
                    if( thickness < 0 )
                    {
                        SHAPE_POLY_SET deflated_shape = shape;
                        deflated_shape.Inflate( thickness / 2, CORNER_STRATEGY::ROUND_ALL_CORNERS,
                                                m_maxError );
                        m_gal->DrawPolygon( deflated_shape );
                    }
                    else
                    {
                        // On Opengl, a not convex filled polygon is usually drawn by using
                        // triangles as primitives. CacheTriangulation() can create basic triangle
                        // primitives to draw the polygon solid shape on Opengl.  GLU tessellation
                        // is much slower, so currently we are using our tessellation.
                        if( m_gal->IsOpenGlEngine() && !shape.IsTriangulationUpToDate() )
                            shape.CacheTriangulation( true, true );

                        m_gal->DrawPolygon( shape );
                    }
                }
            }

            break;
        }

        case SHAPE_T::BEZIER:
            if( outline_mode )
            {
                std::vector<VECTOR2D> output;
                std::vector<VECTOR2D> pointCtrl;

                pointCtrl.push_back( aShape->GetStart() );
                pointCtrl.push_back( aShape->GetBezierC1() );
                pointCtrl.push_back( aShape->GetBezierC2() );
                pointCtrl.push_back( aShape->GetEnd() );

                BEZIER_POLY converter( pointCtrl );
                converter.GetPoly( output, m_maxError );

                m_gal->DrawSegmentChain( aShape->GetBezierPoints(), thickness );
            }
            else
            {
                m_gal->SetIsFill( aShape->IsSolidFill() );
                m_gal->SetIsStroke( lineStyle == LINE_STYLE::SOLID && thickness > 0 );
                m_gal->SetLineWidth( thickness );

                if( aShape->GetBezierPoints().size() > 2 )
                {
                    m_gal->DrawPolygon( aShape->GetBezierPoints() );
                }
                else
                {
                    m_gal->DrawCurve( VECTOR2D( aShape->GetStart() ),
                                      VECTOR2D( aShape->GetBezierC1() ),
                                      VECTOR2D( aShape->GetBezierC2() ),
                                      VECTOR2D( aShape->GetEnd() ), m_maxError );
                }
            }

            break;

        case SHAPE_T::UNDEFINED:
            break;
        }
    }

    if( lineStyle != LINE_STYLE::SOLID )
    {
        if( !outline_mode )
        {
            m_gal->SetIsFill( true );
            m_gal->SetIsStroke( false );
        }

        std::vector<SHAPE*> shapes = aShape->MakeEffectiveShapes( true );

        for( SHAPE* shape : shapes )
        {
            STROKE_PARAMS::Stroke( shape, lineStyle, getLineThickness( aShape->GetWidth() ),
                                   &m_pcbSettings,
                                   [&]( const VECTOR2I& a, const VECTOR2I& b )
                                   {
                                       m_gal->DrawSegment( a, b, thickness );
                                   } );
        }

        for( SHAPE* shape : shapes )
            delete shape;
    }

    if( isHatchedFill )
    {
        aShape->UpdateHatching();
        m_gal->SetIsFill( false );
        m_gal->SetIsStroke( true );
        m_gal->SetLineWidth( aShape->GetHatchLineWidth() );

        for( const SEG& seg : aShape->GetHatchLines() )
            m_gal->DrawLine( seg.A, seg.B );
    }
}


void PCB_PAINTER::strokeText( const wxString& aText, const VECTOR2I& aPosition,
                              const TEXT_ATTRIBUTES& aAttrs, const KIFONT::METRICS& aFontMetrics )
{
    KIFONT::FONT* font = aAttrs.m_Font;

    if( !font )
        font = KIFONT::FONT::GetFont( wxEmptyString, aAttrs.m_Bold, aAttrs.m_Italic );

    m_gal->SetIsFill( font->IsOutline() );
    m_gal->SetIsStroke( font->IsStroke() );

    VECTOR2I pos( aPosition );
    VECTOR2I fudge( KiROUND( 0.16 * aAttrs.m_StrokeWidth ), 0 );

    RotatePoint( fudge, aAttrs.m_Angle );

    if( ( aAttrs.m_Halign == GR_TEXT_H_ALIGN_LEFT && !aAttrs.m_Mirrored )
            || ( aAttrs.m_Halign == GR_TEXT_H_ALIGN_RIGHT && aAttrs.m_Mirrored ) )
    {
        pos -= fudge;
    }
    else if( ( aAttrs.m_Halign == GR_TEXT_H_ALIGN_RIGHT && !aAttrs.m_Mirrored )
            || ( aAttrs.m_Halign == GR_TEXT_H_ALIGN_LEFT && aAttrs.m_Mirrored ) )
    {
        pos += fudge;
    }

    font->Draw( m_gal, aText, pos, aAttrs, aFontMetrics );
}


void PCB_PAINTER::draw( const PCB_REFERENCE_IMAGE* aBitmap, int aLayer )
{
    m_gal->Save();

    const REFERENCE_IMAGE& refImg = aBitmap->GetReferenceImage();
    m_gal->Translate( refImg.GetPosition() );

    // When the image scale factor is not 1.0, we need to modify the actual as the image scale
    // factor is similar to a local zoom
    const double img_scale = refImg.GetImageScale();

    if( img_scale != 1.0 )
        m_gal->Scale( VECTOR2D( img_scale, img_scale ) );

    if( aBitmap->IsSelected() || aBitmap->IsBrightened() )
    {
        COLOR4D color = m_pcbSettings.GetColor( aBitmap, LAYER_ANCHOR );
        m_gal->SetIsStroke( true );
        m_gal->SetStrokeColor( color );
        m_gal->SetLineWidth( m_pcbSettings.m_outlineWidth * 2.0f );
        m_gal->SetIsFill( false );

        // Draws a bounding box.
        VECTOR2D bm_size( refImg.GetSize() );
        // bm_size is the actual image size in UI.
        // but m_canvas scale was previously set to img_scale
        // so recalculate size relative to this image size.
        bm_size.x /= img_scale;
        bm_size.y /= img_scale;
        VECTOR2D origin( -bm_size.x / 2.0, -bm_size.y / 2.0 );
        VECTOR2D end = origin + bm_size;

        m_gal->DrawRectangle( origin, end );

        // Hard code reference images as opaque when selected. Otherwise cached layers will
        // not be rendered under the selected image because cached layers are rendered after
        // non-cached layers (e.g. bitmaps), which will have a closer Z order.
        m_gal->DrawBitmap( refImg.GetImage(), 1.0 );
    }
    else
        m_gal->DrawBitmap( refImg.GetImage(),
                           m_pcbSettings.GetColor( aBitmap, aBitmap->GetLayer() ).a );

    m_gal->Restore();
}


void PCB_PAINTER::draw( const PCB_FIELD* aField, int aLayer )
{
    if( aField->IsVisible() )
        draw( static_cast<const PCB_TEXT*>( aField ), aLayer );
}


void PCB_PAINTER::draw( const PCB_TEXT* aText, int aLayer )
{
    wxString resolvedText( aText->GetShownText( true ) );

    if( resolvedText.Length() == 0 )
        return;

    if( aLayer == LAYER_LOCKED_ITEM_SHADOW )    // happens only if locked
    {
        const COLOR4D color = m_pcbSettings.GetColor( aText, aLayer );

        m_gal->SetIsFill( true );
        m_gal->SetIsStroke( true );
        m_gal->SetFillColor( color );
        m_gal->SetStrokeColor( color );
        m_gal->SetLineWidth( m_lockedShadowMargin );

        SHAPE_POLY_SET poly;
        aText->TransformShapeToPolygon( poly, aText->GetLayer(), 0, m_maxError, ERROR_OUTSIDE );
        m_gal->DrawPolygon( poly );

        return;
    }

    const KIFONT::METRICS& metrics = aText->GetFontMetrics();
    TEXT_ATTRIBUTES        attrs = aText->GetAttributes();
    const COLOR4D&         color = m_pcbSettings.GetColor( aText, aLayer );
    bool                   outline_mode = !viewer_settings()->m_ViewersDisplay.m_DisplayTextFill;

    KIFONT::FONT* font = aText->GetDrawFont( &m_pcbSettings );

    m_gal->SetStrokeColor( color );
    m_gal->SetFillColor( color );
    attrs.m_Angle = aText->GetDrawRotation();

    if( aText->IsKnockout() )
    {
        SHAPE_POLY_SET finalPoly = aText->GetKnockoutCache( font, resolvedText, m_maxError );

        m_gal->SetIsStroke( false );
        m_gal->SetIsFill( true );
        m_gal->DrawPolygon( finalPoly );
    }
    else
    {
        if( outline_mode )
            attrs.m_StrokeWidth = m_pcbSettings.m_outlineWidth;
        else
            attrs.m_StrokeWidth = getLineThickness( aText->GetEffectiveTextPenWidth() );

        if( m_gal->IsFlippedX() && !aText->IsSideSpecific() )
        {
            // We do not want to change the mirroring for this kind of text
            // on the mirrored canvas
            // (not mirrored is draw not mirrored and mirrored is draw mirrored)
            // So we need to recalculate the text position to keep it at the same position
            // on the canvas
            VECTOR2I textPos = aText->GetTextPos();
            VECTOR2I textWidth = VECTOR2I( aText->GetTextBox( &m_pcbSettings ).GetWidth(), 0 );

            if( aText->GetHorizJustify() == GR_TEXT_H_ALIGN_RIGHT )
                textWidth.x = -textWidth.x;
            else if( aText->GetHorizJustify() == GR_TEXT_H_ALIGN_CENTER )
                textWidth.x = 0;

            RotatePoint( textWidth, VECTOR2I( 0, 0 ), aText->GetDrawRotation() );

            if( attrs.m_Mirrored )
                textPos -= textWidth;
            else
                textPos += textWidth;

            attrs.m_Mirrored = !attrs.m_Mirrored;
            strokeText( resolvedText, textPos, attrs, metrics );
            return;
        }

        std::vector<std::unique_ptr<KIFONT::GLYPH>>* cache = nullptr;

        if( font->IsOutline() )
            cache = aText->GetRenderCache( font, resolvedText );

        if( cache )
        {
            m_gal->SetLineWidth( attrs.m_StrokeWidth );
            m_gal->DrawGlyphs( *cache );
        }
        else
        {
            strokeText( resolvedText, aText->GetTextPos(), attrs, metrics );
        }
    }

    // Draw the umbilical line for texts in footprints
    FOOTPRINT* fp_parent = aText->GetParentFootprint();

    if( fp_parent && aText->IsSelected() )
    {
        m_gal->SetLineWidth( m_pcbSettings.m_outlineWidth );
        m_gal->SetStrokeColor( m_pcbSettings.GetColor( nullptr, LAYER_ANCHOR ) );
        m_gal->DrawLine( aText->GetTextPos(), fp_parent->GetPosition() );
    }
}


void PCB_PAINTER::draw( const PCB_TEXTBOX* aTextBox, int aLayer )
{
    if( aTextBox->Type() == PCB_TABLECELL_T )
    {
        const PCB_TABLECELL* cell = static_cast<const PCB_TABLECELL*>( aTextBox );

        if( cell->GetColSpan() == 0 || cell->GetRowSpan() == 0 )
            return;
    }

    COLOR4D       color = m_pcbSettings.GetColor( aTextBox, aLayer );
    int           thickness = getLineThickness( aTextBox->GetWidth() );
    LINE_STYLE    lineStyle = aTextBox->GetStroke().GetLineStyle();
    wxString      resolvedText( aTextBox->GetShownText( true ) );
    KIFONT::FONT* font = aTextBox->GetDrawFont( &m_pcbSettings );

    if( aLayer == LAYER_LOCKED_ITEM_SHADOW )    // happens only if locked
    {
        const COLOR4D sh_color = m_pcbSettings.GetColor( aTextBox, aLayer );

        m_gal->SetIsFill( true );
        m_gal->SetIsStroke( false );
        m_gal->SetFillColor( sh_color );
        m_gal->SetStrokeColor( sh_color );

        // Draw the box with a larger thickness than box thickness to show
        // the shadow mask
        std::vector<VECTOR2I> pts = aTextBox->GetCorners();
        int line_thickness = std::max( thickness*3, pcbIUScale.mmToIU( 0.2 ) );

        std::deque<VECTOR2D> dpts;

        for( const VECTOR2I& pt : pts )
            dpts.push_back( VECTOR2D( pt ) );

        dpts.push_back( VECTOR2D( pts[0] ) );

        m_gal->SetIsStroke( true );
        m_gal->SetLineWidth( line_thickness );
        m_gal->DrawPolygon( dpts );
    }

    m_gal->SetFillColor( color );
    m_gal->SetStrokeColor( color );
    m_gal->SetIsFill( true );
    m_gal->SetIsStroke( false );

    if( aTextBox->Type() != PCB_TABLECELL_T && aTextBox->IsBorderEnabled() )
    {
        if( lineStyle <= LINE_STYLE::FIRST_TYPE )
        {
            if( thickness > 0 )
            {
                std::vector<VECTOR2I> pts = aTextBox->GetCorners();

                for( size_t ii = 0; ii < pts.size(); ++ii )
                    m_gal->DrawSegment( pts[ii], pts[( ii + 1 ) % pts.size()], thickness );
            }
        }
        else
        {
            std::vector<SHAPE*> shapes = aTextBox->MakeEffectiveShapes( true );

            for( SHAPE* shape : shapes )
            {
                STROKE_PARAMS::Stroke( shape, lineStyle, thickness, &m_pcbSettings,
                                       [&]( const VECTOR2I& a, const VECTOR2I& b )
                                       {
                                           m_gal->DrawSegment( a, b, thickness );
                                       } );
            }

            for( SHAPE* shape : shapes )
                delete shape;
        }
    }

    if( aLayer == LAYER_LOCKED_ITEM_SHADOW )
    {
        // For now, the textbox is a filled shape.
        // so the text drawn on LAYER_LOCKED_ITEM_SHADOW with a thick width is disabled
        // If enabled, the thick text position must be offsetted to be exactly on the
        // initial text, which is not easy, depending on its rotation and justification.
#if 0
        const COLOR4D sh_color = m_pcbSettings.GetColor( aTextBox, aLayer );
        m_canvas->SetFillColor( sh_color );
        m_canvas->SetStrokeColor( sh_color );
        attrs.m_StrokeWidth += m_lockedShadowMargin;
#else
        return;
#endif
    }

    if( aTextBox->IsKnockout() )
    {
        SHAPE_POLY_SET finalPoly;
        aTextBox->TransformTextToPolySet( finalPoly, 0, m_maxError, ERROR_INSIDE );
        finalPoly.Fracture();

        m_gal->SetIsStroke( false );
        m_gal->SetIsFill( true );
        m_gal->DrawPolygon( finalPoly );
    }
    else
    {
        if( resolvedText.Length() == 0 )
            return;

        const KIFONT::METRICS& metrics = aTextBox->GetFontMetrics();
        TEXT_ATTRIBUTES        attrs = aTextBox->GetAttributes();
        attrs.m_StrokeWidth = getLineThickness( aTextBox->GetEffectiveTextPenWidth() );

        if( m_gal->IsFlippedX() && !aTextBox->IsSideSpecific() )
        {
            attrs.m_Mirrored = !attrs.m_Mirrored;
            strokeText( resolvedText, aTextBox->GetDrawPos( true ), attrs, metrics );
            return;
        }

        std::vector<std::unique_ptr<KIFONT::GLYPH>>* cache = nullptr;

        if( font->IsOutline() )
            cache = aTextBox->GetRenderCache( font, resolvedText );

        if( cache )
        {
            m_gal->SetLineWidth( attrs.m_StrokeWidth );
            m_gal->DrawGlyphs( *cache );
        }
        else
        {
            strokeText( resolvedText, aTextBox->GetDrawPos(), attrs, metrics );
        }
    }
}

void PCB_PAINTER::draw( const PCB_TABLE* aTable, int aLayer )
{
    if( aTable->GetCells().empty() )
        return;

    for( PCB_TABLECELL* cell : aTable->GetCells() )
    {
        if( cell->GetColSpan() > 0 || cell->GetRowSpan() > 0 )
            draw( static_cast<PCB_TEXTBOX*>( cell ), aLayer );
    }

    COLOR4D color = m_pcbSettings.GetColor( aTable, aLayer );

    aTable->DrawBorders(
            [&]( const VECTOR2I& ptA, const VECTOR2I& ptB, const STROKE_PARAMS& stroke )
            {
                int        lineWidth = getLineThickness( stroke.GetWidth() );
                LINE_STYLE lineStyle = stroke.GetLineStyle();

                m_gal->SetIsFill( false );
                m_gal->SetIsStroke( true );
                m_gal->SetStrokeColor( color );
                m_gal->SetLineWidth( lineWidth );

                if( lineStyle <= LINE_STYLE::FIRST_TYPE )
                {
                    m_gal->DrawLine( ptA, ptB );
                }
                else
                {
                    SHAPE_SEGMENT seg( ptA, ptB );

                    STROKE_PARAMS::Stroke( &seg, lineStyle, lineWidth, &m_pcbSettings,
                            [&]( VECTOR2I a, VECTOR2I b )
                            {
                                // DrawLine has problem with 0 length lines so enforce minimum
                                if( a == b )
                                    m_gal->DrawLine( a+1, b );
                                else
                                    m_gal->DrawLine( a, b );
                            } );
                }
            } );

    // Highlight selected tablecells with a background wash.
    for( PCB_TABLECELL* cell : aTable->GetCells() )
    {
        if( aTable->IsSelected() || cell->IsSelected() )
        {
            std::vector<VECTOR2I> corners = cell->GetCorners();
            std::deque<VECTOR2D>  pts;

            pts.insert( pts.end(), corners.begin(), corners.end() );

            m_gal->SetFillColor( color.WithAlpha( 0.5 ) );
            m_gal->SetIsFill( true );
            m_gal->SetIsStroke( false );
            m_gal->DrawPolygon( pts );
        }
    }
}


void PCB_PAINTER::draw( const FOOTPRINT* aFootprint, int aLayer )
{
    if( aLayer == LAYER_ANCHOR )
    {
        const COLOR4D color = m_pcbSettings.GetColor( aFootprint, aLayer );

        // Keep the size and width constant, not related to the scale because the anchor
        // is just a marker on screen
        double anchorSize = 5.0 / m_gal->GetWorldScale();           // 5 pixels size
        double anchorThickness = 1.0 / m_gal->GetWorldScale();      // 1 pixels width

        // Draw anchor
        m_gal->SetIsFill( false );
        m_gal->SetIsStroke( true );
        m_gal->SetStrokeColor( color );
        m_gal->SetLineWidth( anchorThickness );

        VECTOR2D center = aFootprint->GetPosition();
        m_gal->DrawLine( center - VECTOR2D( anchorSize, 0 ), center + VECTOR2D( anchorSize, 0 ) );
        m_gal->DrawLine( center - VECTOR2D( 0, anchorSize ), center + VECTOR2D( 0, anchorSize ) );
    }

    if( aLayer == LAYER_LOCKED_ITEM_SHADOW && m_frameType == FRAME_PCB_EDITOR )    // happens only if locked
    {
        const COLOR4D color = m_pcbSettings.GetColor( aFootprint, aLayer );

        m_gal->SetIsFill( true );
        m_gal->SetIsStroke( false );
        m_gal->SetFillColor( color );

#if 0 // GetBoundingHull() can be very slow, especially for logos imported from graphics
        const SHAPE_POLY_SET& poly = aFootprint->GetBoundingHull();
        m_canvas->DrawPolygon( poly );
#else
        BOX2I    bbox = aFootprint->GetBoundingBox( false );
        VECTOR2I topLeft = bbox.GetPosition();
        VECTOR2I botRight = bbox.GetPosition() + bbox.GetSize();

        m_gal->DrawRectangle( topLeft, botRight );

        // Use segments to produce a margin with rounded corners
        m_gal->DrawSegment( topLeft, VECTOR2I( botRight.x, topLeft.y ), m_lockedShadowMargin );
        m_gal->DrawSegment( VECTOR2I( botRight.x, topLeft.y ), botRight, m_lockedShadowMargin );
        m_gal->DrawSegment( botRight, VECTOR2I( topLeft.x, botRight.y ), m_lockedShadowMargin );
        m_gal->DrawSegment( VECTOR2I( topLeft.x, botRight.y ), topLeft, m_lockedShadowMargin );
#endif
    }

    if( aLayer == LAYER_CONFLICTS_SHADOW )
    {
        const SHAPE_POLY_SET& frontpoly = aFootprint->GetCourtyard( F_CrtYd );
        const SHAPE_POLY_SET& backpoly = aFootprint->GetCourtyard( B_CrtYd );

        const COLOR4D color = m_pcbSettings.GetColor( aFootprint, aLayer );

        m_gal->SetIsFill( true );
        m_gal->SetIsStroke( false );
        m_gal->SetFillColor( color );

        if( frontpoly.OutlineCount() > 0 )
            m_gal->DrawPolygon( frontpoly );

        if( backpoly.OutlineCount() > 0 )
            m_gal->DrawPolygon( backpoly );
    }
}


void PCB_PAINTER::draw( const PCB_GROUP* aGroup, int aLayer )
{
    if( aLayer == LAYER_ANCHOR )
    {
        if( aGroup->IsSelected() && !( aGroup->GetParent() && aGroup->GetParent()->IsSelected() ) )
        {
            // Selected on our own; draw enclosing box
        }
        else if( aGroup->IsEntered() )
        {
            // Entered group; draw enclosing box
        }
        else
        {
            // Neither selected nor entered; draw nothing at the group level (ie: only draw
            // its members)
            return;
        }

        const COLOR4D color = m_pcbSettings.GetColor( aGroup, LAYER_ANCHOR );

        m_gal->SetStrokeColor( color );
        m_gal->SetLineWidth( m_pcbSettings.m_outlineWidth * 2.0f );

        BOX2I    bbox = aGroup->GetBoundingBox();
        VECTOR2I topLeft = bbox.GetPosition();
        VECTOR2I width = VECTOR2I( bbox.GetWidth(), 0 );
        VECTOR2I height = VECTOR2I( 0, bbox.GetHeight() );

        m_gal->DrawLine( topLeft, topLeft + width );
        m_gal->DrawLine( topLeft + width, topLeft + width + height );
        m_gal->DrawLine( topLeft + width + height, topLeft + height );
        m_gal->DrawLine( topLeft + height, topLeft );

        wxString name = aGroup->GetName();

        if( name.IsEmpty() )
            return;

        int ptSize = 12;
        int scaledSize = abs( KiROUND( m_gal->GetScreenWorldMatrix().GetScale().x * ptSize ) );
        int unscaledSize = pcbIUScale.MilsToIU( ptSize );

        // Scale by zoom a bit, but not too much
        int      textSize = ( scaledSize + ( unscaledSize * 2 ) ) / 3;
        VECTOR2I textOffset = KiROUND( width.x / 2.0, -textSize * 0.5 );
        VECTOR2I titleHeight = KiROUND( 0.0, textSize * 2.0 );

        if( PrintableCharCount( name ) * textSize < bbox.GetWidth() )
        {
            m_gal->DrawLine( topLeft, topLeft - titleHeight );
            m_gal->DrawLine( topLeft - titleHeight, topLeft + width - titleHeight );
            m_gal->DrawLine( topLeft + width - titleHeight, topLeft + width );

            TEXT_ATTRIBUTES attrs;
            attrs.m_Italic = true;
            attrs.m_Halign = GR_TEXT_H_ALIGN_CENTER;
            attrs.m_Valign = GR_TEXT_V_ALIGN_BOTTOM;
            attrs.m_Size = VECTOR2I( textSize, textSize );
            attrs.m_StrokeWidth = GetPenSizeForNormal( textSize );

            KIFONT::FONT::GetFont()->Draw( m_gal, aGroup->GetName(), topLeft + textOffset, attrs,
                                           aGroup->GetFontMetrics() );
        }
    }
}


void PCB_PAINTER::draw( const ZONE* aZone, int aLayer )
{
    if( aLayer == LAYER_CONFLICTS_SHADOW )
    {
        COLOR4D color = m_pcbSettings.GetColor( aZone, aLayer );

        m_gal->SetIsFill( true );
        m_gal->SetIsStroke( false );
        m_gal->SetFillColor( color );

        m_gal->DrawPolygon( aZone->Outline()->Outline( 0 ) );
        return;
    }

    /*
     * aLayer will be the virtual zone layer (LAYER_ZONE_START, ... in GAL_LAYER_ID)
     * This is used for draw ordering in the GAL.
     * The color for the zone comes from the associated copper layer ( aLayer - LAYER_ZONE_START )
     * and the visibility comes from the combination of that copper layer and LAYER_ZONES
     */
    PCB_LAYER_ID layer;

    if( IsZoneFillLayer( aLayer ) )
        layer = ToLAYER_ID( aLayer - LAYER_ZONE_START );
    else
        layer = ToLAYER_ID( aLayer );

    if( !aZone->IsOnLayer( layer ) )
        return;

    COLOR4D              color = m_pcbSettings.GetColor( aZone, layer );
    std::deque<VECTOR2D> corners;
    ZONE_DISPLAY_MODE    displayMode = m_pcbSettings.m_ZoneDisplayMode;

    if( aZone->IsTeardropArea() )
        displayMode = ZONE_DISPLAY_MODE::SHOW_FILLED;

    // Draw the outline
    if( !IsZoneFillLayer( aLayer ) )
    {
        const SHAPE_POLY_SET* outline = aZone->Outline();
        bool allowDrawOutline = aZone->GetHatchStyle() != ZONE_BORDER_DISPLAY_STYLE::INVISIBLE_BORDER;

        if( allowDrawOutline && !m_pcbSettings.m_isPrinting && outline && outline->OutlineCount() > 0 )
        {
            m_gal->SetStrokeColor( color.a > 0.0 ? color.WithAlpha( 1.0 ) : color );
            m_gal->SetIsFill( false );
            m_gal->SetIsStroke( true );
            m_gal->SetLineWidth( m_pcbSettings.m_outlineWidth );

            // Draw each contour (main contour and holes)

            /*
             * m_canvas->DrawPolygon( *outline );
             * should be enough, but currently does not work to draw holes contours in a complex
             * polygon so each contour is draw as a simple polygon
             */

            // Draw the main contour(s?)
            for( int ii = 0; ii < outline->OutlineCount(); ++ii )
            {
                m_gal->DrawPolyline( outline->COutline( ii ) );

                // Draw holes
                int holes_count = outline->HoleCount( ii );

                for( int jj = 0; jj < holes_count; ++jj )
                    m_gal->DrawPolyline( outline->CHole( ii, jj ) );
            }

            // Draw hatch lines
            for( const SEG& hatchLine : aZone->GetHatchLines() )
                m_gal->DrawLine( hatchLine.A, hatchLine.B );
        }
    }

    // Draw the filling
    if( IsZoneFillLayer( aLayer )
            && ( displayMode == ZONE_DISPLAY_MODE::SHOW_FILLED
                || displayMode == ZONE_DISPLAY_MODE::SHOW_FRACTURE_BORDERS
                || displayMode == ZONE_DISPLAY_MODE::SHOW_TRIANGULATION ) )
    {
        const std::shared_ptr<SHAPE_POLY_SET>& polySet = aZone->GetFilledPolysList( layer );

        if( polySet->OutlineCount() == 0 )  // Nothing to draw
            return;

        m_gal->SetStrokeColor( color );
        m_gal->SetFillColor( color );
        m_gal->SetLineWidth( 0 );

        if( displayMode == ZONE_DISPLAY_MODE::SHOW_FILLED )
        {
            m_gal->SetIsFill( true );
            m_gal->SetIsStroke( false );
        }
        else
        {
            m_gal->SetIsFill( false );
            m_gal->SetIsStroke( true );
        }

        // On Opengl, a not convex filled polygon is usually drawn by using triangles
        // as primitives. CacheTriangulation() can create basic triangle primitives to
        // draw the polygon solid shape on Opengl.  GLU tessellation is much slower,
        // so currently we are using our tessellation.
        if( m_gal->IsOpenGlEngine() && !polySet->IsTriangulationUpToDate() )
            polySet->CacheTriangulation( true, true );

        m_gal->DrawPolygon( *polySet, displayMode == ZONE_DISPLAY_MODE::SHOW_TRIANGULATION );
    }
}


void PCB_PAINTER::draw( const PCB_BARCODE* aBarcode, int aLayer )
{
    const COLOR4D& color = m_pcbSettings.GetColor( aBarcode, aLayer );

    m_gal->SetIsFill( true );
    m_gal->SetIsStroke( false );
    m_gal->SetFillColor( color );

    // Draw the barcode
    SHAPE_POLY_SET shape;

    if( aLayer == LAYER_LOCKED_ITEM_SHADOW )
        aBarcode->GetBoundingHull( shape, aBarcode->GetLayer(), m_lockedShadowMargin, m_maxError, ERROR_INSIDE );
    else
        aBarcode->TransformShapeToPolySet( shape, aBarcode->GetLayer(), 0, m_maxError, ERROR_INSIDE );

    if( shape.OutlineCount() != 0 )
        m_gal->DrawPolygon( shape );
}


void PCB_PAINTER::draw( const PCB_DIMENSION_BASE* aDimension, int aLayer )
{
    const COLOR4D& color = m_pcbSettings.GetColor( aDimension, aLayer );

    m_gal->SetStrokeColor( color );
    m_gal->SetFillColor( color );
    m_gal->SetIsFill( false );
    m_gal->SetIsStroke( true );

    bool outline_mode = !viewer_settings()->m_ViewersDisplay.m_DisplayGraphicsFill;

    if( outline_mode )
        m_gal->SetLineWidth( m_pcbSettings.m_outlineWidth );
    else
        m_gal->SetLineWidth( getLineThickness( aDimension->GetLineThickness() ) );

    // Draw dimension shapes
    // TODO(JE) lift this out
    for( const std::shared_ptr<SHAPE>& shape : aDimension->GetShapes() )
    {
        switch( shape->Type() )
        {
        case SH_SEGMENT:
        {
            const SEG& seg = static_cast<const SHAPE_SEGMENT*>( shape.get() )->GetSeg();
            m_gal->DrawLine( seg.A, seg.B );
            break;
        }

        case SH_CIRCLE:
        {
            int radius = static_cast<const SHAPE_CIRCLE*>( shape.get() )->GetRadius();
            m_gal->DrawCircle( shape->Centre(), radius );
            break;
        }

        default:
            break;
        }
    }

    // Draw text
    wxString        resolvedText = aDimension->GetShownText( true );
    TEXT_ATTRIBUTES attrs = aDimension->GetAttributes();

    if( m_gal->IsFlippedX() && !aDimension->IsSideSpecific() )
        attrs.m_Mirrored = !attrs.m_Mirrored;

    if( outline_mode )
        attrs.m_StrokeWidth = m_pcbSettings.m_outlineWidth;
    else
        attrs.m_StrokeWidth = getLineThickness( aDimension->GetEffectiveTextPenWidth() );

    std::vector<std::unique_ptr<KIFONT::GLYPH>>* cache = nullptr;

    if( aDimension->GetFont() && aDimension->GetFont()->IsOutline() )
        cache = aDimension->GetRenderCache( aDimension->GetFont(), resolvedText );

    if( cache )
    {
        for( const std::unique_ptr<KIFONT::GLYPH>& glyph : *cache )
            m_gal->DrawGlyph( *glyph.get() );
    }
    else
    {
        strokeText( resolvedText, aDimension->GetTextPos(), attrs, aDimension->GetFontMetrics() );
    }
}


void PCB_PAINTER::draw( const PCB_TARGET* aTarget )
{
    const COLOR4D strokeColor = m_pcbSettings.GetColor( aTarget, aTarget->GetLayer() );
    VECTOR2D position( aTarget->GetPosition() );
    double   size, radius;

    m_gal->SetLineWidth( getLineThickness( aTarget->GetWidth() ) );
    m_gal->SetStrokeColor( strokeColor );
    m_gal->SetIsFill( false );
    m_gal->SetIsStroke( true );

    m_gal->Save();
    m_gal->Translate( position );

    if( aTarget->GetShape() )
    {
        // shape x
        m_gal->Rotate( M_PI / 4.0 );
        size   = 2.0 * aTarget->GetSize() / 3.0;
        radius = aTarget->GetSize() / 2.0;
    }
    else
    {
        // shape +
        size   = aTarget->GetSize() / 2.0;
        radius = aTarget->GetSize() / 3.0;
    }

    m_gal->DrawLine( VECTOR2D( -size, 0.0 ), VECTOR2D( size, 0.0 ) );
    m_gal->DrawLine( VECTOR2D( 0.0, -size ), VECTOR2D( 0.0,  size ) );
    m_gal->DrawCircle( VECTOR2D( 0.0, 0.0 ), radius );

    m_gal->Restore();
}


void PCB_PAINTER::draw( const PCB_POINT* aPoint, int aLayer )
{
    // aLayer will be the virtual point layer (LAYER_POINT_START, ... in GAL_LAYER_ID).
    // This is used for draw ordering in the GAL.
    // The cross color comes from LAYER_POINTS and the ring color follows the point's board layer.
    // Visibility comes from the combination of that board layer and LAYER_POINTS.

    double size = (double)aPoint->GetSize() / 2;

    // Keep the width constant, not related to the scale because the anchor
    // is just a marker on screen, just draw in pixels
    double thickness = m_pcbSettings.m_outlineWidth;

    // The general "points" colour
    COLOR4D crossColor = m_pcbSettings.GetColor( aPoint, LAYER_POINTS );
    // The colour for the ring around the point follows the "real" layer of the point
    COLOR4D ringColor = m_pcbSettings.GetColor( aPoint, aPoint->GetLayer() );

    if( aLayer == LAYER_LOCKED_ITEM_SHADOW )
    {
        thickness += m_lockedShadowMargin;
        crossColor = m_pcbSettings.GetColor( aPoint, aLayer );
        ringColor = m_pcbSettings.GetColor( aPoint, aLayer );
    }

    VECTOR2D position( aPoint->GetPosition() );

    m_gal->SetLineWidth( thickness );
    m_gal->SetStrokeColor( crossColor );
    m_gal->SetIsFill( false );
    m_gal->SetIsStroke( true );

    m_gal->Save();
    m_gal->Translate( position );

    // Draw as X to make it clearer when overlaid on cursor or axes
    m_gal->DrawLine( VECTOR2D( -size, -size ), VECTOR2D( size, size ) );
    m_gal->DrawLine( VECTOR2D( size, -size ), VECTOR2D( -size, size ) );

    // Draw the circle in the layer colour
    m_gal->SetStrokeColor( ringColor );
    m_gal->DrawCircle( VECTOR2D( 0.0, 0.0 ), size / 2 );

    m_gal->Restore();
}


void PCB_PAINTER::draw( const PCB_MARKER* aMarker, int aLayer )
{
    // Don't paint invisible markers.
    // It would be nice to do this through layer dependencies but we can't do an "or" there today
    if( aMarker->GetBoard() && !aMarker->GetBoard()->IsElementVisible( aMarker->GetColorLayer() ) )
        return;

    COLOR4D color = m_pcbSettings.GetColor( aMarker, aMarker->GetColorLayer() );

    aMarker->SetZoom( 1.0 / sqrt( m_gal->GetZoomFactor() ) );

    switch( aLayer )
    {
    case LAYER_MARKER_SHADOWS:
    case LAYER_DRC_ERROR:
    case LAYER_DRC_WARNING:
    {
        bool isShadow = aLayer == LAYER_MARKER_SHADOWS;

        SHAPE_LINE_CHAIN polygon;
        aMarker->ShapeToPolygon( polygon );

        m_gal->Save();
        m_gal->Translate( aMarker->GetPosition() );

        if( isShadow )
        {
            m_gal->SetStrokeColor( m_pcbSettings.GetColor( aMarker, LAYER_MARKER_SHADOWS ) );
            m_gal->SetIsStroke( true );
            m_gal->SetLineWidth( (float) aMarker->MarkerScale() );
        }
        else
        {
            m_gal->SetFillColor( color );
            m_gal->SetIsFill( true );
        }

        m_gal->DrawPolygon( polygon );
        m_gal->Restore();
        break;
    }

    case LAYER_DRC_SHAPES:
        if( !aMarker->IsBrightened() )
            return;

        for( const PCB_SHAPE& shape : aMarker->GetShapes() )
        {
            if( shape.GetStroke().GetWidth() == 1.0 )
            {
                m_gal->SetIsFill( false );
                m_gal->SetIsStroke( true );
                m_gal->SetStrokeColor( WHITE );
                m_gal->SetLineWidth( KiROUND( aMarker->MarkerScale() / 2.0 ) );

                if( shape.GetShape() == SHAPE_T::SEGMENT )
                {
                    m_gal->DrawLine( shape.GetStart(), shape.GetEnd() );
                }
                else if( shape.GetShape() == SHAPE_T::ARC )
                {
                    EDA_ANGLE startAngle, endAngle;
                    shape.CalcArcAngles( startAngle, endAngle );

                    m_gal->DrawArc( shape.GetCenter(), shape.GetRadius(), startAngle, shape.GetArcAngle() );
                }
            }
            else
            {
                m_gal->SetIsFill( true );
                m_gal->SetIsStroke( false );
                m_gal->SetFillColor( color.WithAlpha( 0.5 ) );

                if( shape.GetShape() == SHAPE_T::SEGMENT )
                {
                    m_gal->DrawSegment( shape.GetStart(), shape.GetEnd(), shape.GetWidth() );
                }
                else if( shape.GetShape() == SHAPE_T::ARC )
                {
                    EDA_ANGLE startAngle, endAngle;
                    shape.CalcArcAngles( startAngle, endAngle );

                    m_gal->DrawArcSegment( shape.GetCenter(), shape.GetRadius(), startAngle, shape.GetArcAngle(),
                                           shape.GetWidth(), ARC_HIGH_DEF );
                }
            }
        }

        break;
    }
}


void PCB_PAINTER::draw( const PCB_BOARD_OUTLINE* aBoardOutline, int aLayer )
{
    if( !aBoardOutline->HasOutline() )
        return;

    // aBoardOutline makes sense only for the board editor. for fp holder boards
    // there are no board outlines area.
    const BOARD* brd = aBoardOutline->GetBoard();

    if( !brd || brd->GetBoardUse() == BOARD_USE::FPHOLDER )
        return;

    GAL_SCOPED_ATTRS scopedAttrs( *m_gal, GAL_SCOPED_ATTRS::ALL_ATTRS );
    m_gal->Save();

    const COLOR4D& outlineColor = m_pcbSettings.GetColor( aBoardOutline, aLayer );
    m_gal->SetFillColor( outlineColor );
    m_gal->AdvanceDepth();
    m_gal->SetLineWidth( 0 );
    m_gal->SetIsFill( true );
    m_gal->SetIsStroke( false );
    m_gal->DrawPolygon( aBoardOutline->GetOutline() );

    m_gal->Restore();
}


void PCB_PAINTER::drawBackdrillIndicator( const BOARD_ITEM* aItem, const VECTOR2D& aCenter,
                                          int aDrillSize, PCB_LAYER_ID aStartLayer,
                                          PCB_LAYER_ID aEndLayer )
{
    double backdrillRadius = aDrillSize / 2.0;
    double lineWidth = std::max( backdrillRadius / 4.0, m_pcbSettings.m_outlineWidth * 2.0 );

    GAL_SCOPED_ATTRS scopedAttrs( *m_gal, GAL_SCOPED_ATTRS::ALL_ATTRS );
    m_gal->AdvanceDepth();
    m_gal->SetIsFill( false );
    m_gal->SetIsStroke( true );
    m_gal->SetLineWidth( lineWidth );

    // Draw semi-circle in start layer color (top half, from 90° to 270°)
    m_gal->SetStrokeColor( m_pcbSettings.GetColor( aItem, aStartLayer ) );
    m_gal->DrawArc( aCenter, backdrillRadius, EDA_ANGLE( 90, DEGREES_T ),
                    EDA_ANGLE( 180, DEGREES_T ) );

    // Draw semi-circle in end layer color (bottom half, from 270° to 90°)
    m_gal->SetStrokeColor( m_pcbSettings.GetColor( aItem, aEndLayer ) );
    m_gal->DrawArc( aCenter, backdrillRadius, EDA_ANGLE( 270, DEGREES_T ),
                    EDA_ANGLE( 180, DEGREES_T ) );
}


void PCB_PAINTER::drawPostMachiningIndicator( const BOARD_ITEM* aItem, const VECTOR2D& aCenter, PCB_LAYER_ID aLayer )
{
    int size = 0;

    // Check to see if the pad or via has a post-machining operation on this layer
    if( const PAD* pad = dynamic_cast<const PAD*>( aItem ) )
    {
        size = pad->GetPostMachiningKnockout( aLayer );
    }
    else if( const PCB_VIA* via = dynamic_cast<const PCB_VIA*>( aItem ) )
    {
        size = via->GetPostMachiningKnockout( aLayer );
    }

    if( size <= 0 )
        return;

    GAL_SCOPED_ATTRS scopedAttrs( *m_gal, GAL_SCOPED_ATTRS::ALL_ATTRS );
    m_gal->AdvanceDepth();

    double pmRadius = size / 2.0;
    // Use a line width proportional to the radius for visibility
    double lineWidth = std::max( pmRadius / 8.0, m_pcbSettings.m_outlineWidth * 2.0 );

    COLOR4D layerColor = m_pcbSettings.GetColor( aItem, aLayer );

    m_gal->SetIsFill( false );
    m_gal->SetIsStroke( true );
    m_gal->SetStrokeColor( layerColor );
    m_gal->SetLineWidth( lineWidth );

    // Draw dashed circle manually with fixed number of segments for consistent appearance
    constexpr int NUM_DASHES = 12;  // Number of dashes around the circle
    EDA_ANGLE dashAngle = ANGLE_360 / ( NUM_DASHES * 2 );  // Dash and gap are equal size

    for( int i = 0; i < NUM_DASHES; ++i )
    {
        EDA_ANGLE startAngle = dashAngle * ( i * 2 );
        m_gal->DrawArc( aCenter, pmRadius, startAngle, dashAngle );
    }
}


const double PCB_RENDER_SETTINGS::MAX_FONT_SIZE = pcbIUScale.mmToIU( 10.0 );
