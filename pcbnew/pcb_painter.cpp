/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2013-2019 CERN
 * Copyright (C) 2021-2023 KiCad Developers, see AUTHORS.txt for contributors.
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

#include <board.h>
#include <board_design_settings.h>
#include <pcb_track.h>
#include <pcb_group.h>
#include <footprint.h>
#include <pad.h>
#include <pcb_shape.h>
#include <string_utils.h>
#include <zone.h>
#include <pcb_bitmap.h>
#include <pcb_text.h>
#include <pcb_textbox.h>
#include <pcb_marker.h>
#include <pcb_dimension.h>
#include <pcb_target.h>

#include <layer_ids.h>
#include <pcb_painter.h>
#include <pcb_display_options.h>
#include <project/net_settings.h>
#include <settings/color_settings.h>
#include <settings/common_settings.h>
#include <settings/settings_manager.h>
#include <settings/cvpcb_settings.h>
#include <pcbnew_settings.h>

#include <convert_basic_shapes_to_polygon.h>
#include <gal/graphics_abstraction_layer.h>
#include <callback_gal.h>
#include <geometry/geometry_utils.h>
#include <geometry/shape_line_chain.h>
#include <geometry/shape_rect.h>
#include <geometry/shape_segment.h>
#include <geometry/shape_simple.h>
#include <geometry/shape_circle.h>
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
    case FRAME_FOOTPRINT_EDITOR:
    case FRAME_FOOTPRINT_WIZARD:
    case FRAME_PCB_DISPLAY3D:
    default:
        return Pgm().GetSettingsManager().GetAppSettings<PCBNEW_SETTINGS>();

    case FRAME_FOOTPRINT_VIEWER:
    case FRAME_FOOTPRINT_VIEWER_MODAL:
    case FRAME_FOOTPRINT_PREVIEW:
    case FRAME_CVPCB:
    case FRAME_CVPCB_DISPLAY:
        return Pgm().GetSettingsManager().GetAppSettings<CVPCB_SETTINGS>();
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

    m_ForcePadSketchModeOn = false;

    m_PadEditModePad = nullptr;

    SetDashLengthRatio( 12 );       // From ISO 128-2
    SetGapLengthRatio( 3 );         // From ISO 128-2

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
    m_layerColors[LAYER_VIA_NETNAMES]    = COLOR4D( 0.2, 0.2, 0.2, 0.9 );
    m_layerColors[LAYER_PAD_NETNAMES]    = COLOR4D( 1.0, 1.0, 1.0, 0.9 );
    m_layerColors[LAYER_PAD_FR]          = aSettings->GetColor( F_Cu );
    m_layerColors[LAYER_PAD_BK]          = aSettings->GetColor( B_Cu );
    m_layerColors[LAYER_PAD_FR_NETNAMES] = COLOR4D( 1.0, 1.0, 1.0, 0.9 );
    m_layerColors[LAYER_PAD_BK_NETNAMES] = COLOR4D( 1.0, 1.0, 1.0, 0.9 );

    // Netnames for copper layers
    for( LSEQ cu = LSET::AllCuMask().CuStack();  cu;  ++cu )
    {
        const COLOR4D lightLabel( 1.0, 1.0, 1.0, 0.7 );
        const COLOR4D darkLabel = lightLabel.Inverted();
        PCB_LAYER_ID  layer = *cu;

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
}


COLOR4D PCB_RENDER_SETTINGS::GetColor( const VIEW_ITEM* aItem, int aLayer ) const
{
    const EDA_ITEM*             item = dynamic_cast<const EDA_ITEM*>( aItem );
    const BOARD_CONNECTED_ITEM* conItem = dynamic_cast<const BOARD_CONNECTED_ITEM*> ( aItem );
    int                         netCode = -1;
    int                         originalLayer = aLayer;

    // Marker shadows
    if( aLayer == LAYER_MARKER_SHADOWS )
        return m_backgroundColor.WithAlpha( 0.6 );

    if( IsHoleLayer( aLayer ) && m_isPrinting )
    {
        // Careful that we don't end up with the same colour for the annular ring and the hole
        // when printing in B&W.
        const PAD*     pad = dynamic_cast<const PAD*>( item );
        const PCB_VIA* via = dynamic_cast<const PCB_VIA*>( item );
        int            holeLayer = aLayer;
        int            annularRingLayer = UNDEFINED_LAYER;

        if( pad && pad->GetAttribute() == PAD_ATTRIB::PTH )
            annularRingLayer = LAYER_PADS_TH;
        else if( via && via->GetViaType() == VIATYPE::MICROVIA )
            annularRingLayer = LAYER_VIA_MICROVIA;
        else if( via && via->GetViaType() == VIATYPE::BLIND_BURIED )
            annularRingLayer = LAYER_VIA_BBLIND;
        else if( via && via->GetViaType() == VIATYPE::THROUGH )
            annularRingLayer = LAYER_VIA_THROUGH;

        if( annularRingLayer != UNDEFINED_LAYER
                && m_layerColors[ holeLayer ] == m_layerColors[ annularRingLayer ] )
        {
            aLayer = LAYER_PCB_BACKGROUND;
        }
    }

    // Zones should pull from the copper layer
    if( item && item->Type() == PCB_ZONE_T )
    {
        if( IsZoneFillLayer( aLayer ) )
            aLayer = aLayer - LAYER_ZONE_START;
    }

    // Hole walls should pull from the copper layer
    if( aLayer == LAYER_PAD_HOLEWALLS )
        aLayer = LAYER_PADS_TH;
    else if( aLayer == LAYER_VIA_HOLEWALLS )
        aLayer = LAYER_VIA_THROUGH;

    // Normal path: get the layer base color
    COLOR4D color = m_layerColors[aLayer];

    if( !item )
        return m_layerColors[aLayer];

    // Selection disambiguation
    if( item->IsBrightened() )
        return color.Brightened( m_selectFactor ).WithAlpha( 0.8 );

    // Normal selection
    if( item->IsSelected() )
        color = m_layerColorsSel[aLayer];

    // Try to obtain the netcode for the item
    if( conItem )
        netCode = conItem->GetNetCode();

    bool highlighted = m_highlightEnabled && m_highlightNetcodes.count( netCode );
    bool selected    = item->IsSelected();

    // Apply net color overrides
    if( conItem && m_netColorMode == NET_COLOR_MODE::ALL && IsNetCopperLayer( aLayer ) )
    {
        COLOR4D netColor = COLOR4D::UNSPECIFIED;

        auto ii = m_netColors.find( netCode );

        if( ii != m_netColors.end() )
            netColor = ii->second;

        if( netColor == COLOR4D::UNSPECIFIED )
        {
            auto jj = m_netclassColors.find( conItem->GetNetClassName() );

            if( jj != m_netclassColors.end() )
                netColor = jj->second;
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
        color = m_highlightNetcodes.count( netCode ) ? m_layerColorsHi[aLayer]
                                                     : m_layerColorsDark[aLayer];
    }

    // Apply high-contrast dimming
    if( m_hiContrastEnabled && m_highContrastLayers.size() && !highlighted && !selected )
    {
        PCB_LAYER_ID primary = GetPrimaryHighContrastLayer();
        bool         isActive = m_highContrastLayers.count( aLayer );
        bool         hide = false;

        switch( originalLayer )
        {
        case LAYER_PADS_TH:
        {
            const PAD* pad = static_cast<const PAD*>( item );

            if( !pad->FlashLayer( primary ) )
            {
                isActive = false;

                if( IsCopperLayer( primary ) )
                    hide = true;
            }

            if( m_PadEditModePad && pad != m_PadEditModePad )
                isActive = false;

            break;
        }

        case LAYER_VIA_BBLIND:
        case LAYER_VIA_MICROVIA:
        {
            const PCB_VIA* via = static_cast<const PCB_VIA*>( item );

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
            const PCB_VIA* via = static_cast<const PCB_VIA*>( item );

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
            const PCB_VIA* via = static_cast<const PCB_VIA*>( item );

            if( via->GetViaType() == VIATYPE::BLIND_BURIED
                    || via->GetViaType() == VIATYPE::MICROVIA )
            {
                // A blind or micro via's hole is active if it crosses the primary layer
                if( via->GetLayerSet().test( primary ) == 0 )
                    isActive = false;
            }
            else
            {
                // A through via's hole is active if any physical layer is active
                if( LSET::PhysicalLayersMask().test( primary ) == 0 )
                    isActive = false;
            }

            break;
        }

        default:
            break;
        }

        if( !isActive )
        {
            if( m_ContrastModeDisplay == HIGH_CONTRAST_MODE::HIDDEN
                || IsNetnameLayer( aLayer )
                || hide )
            {
                color = COLOR4D::CLEAR;
            }
            else
            {
                color = color.Mix( m_layerColors[LAYER_PCB_BACKGROUND], m_hiContrastFactor );

                // Bitmaps can't have their color mixed so just reduce the opacity a bit so they
                // show through less
                if( item->Type() == PCB_BITMAP_T )
                    color.a *= m_hiContrastFactor;
            }
        }
    }
    else if( originalLayer == LAYER_VIA_BBLIND || originalLayer == LAYER_VIA_MICROVIA )
    {
        const PCB_VIA* via = static_cast<const PCB_VIA*>( item );
        const BOARD*   board = via->GetBoard();
        LSET           visibleLayers = board->GetVisibleLayers() & board->GetEnabledLayers();

        // Target graphic is visible if the via crosses a visible layer
        if( ( via->GetLayerSet() & visibleLayers ).none() )
            color = COLOR4D::CLEAR;
    }

    // Apply per-type opacity overrides
    if( item->Type() == PCB_TRACE_T || item->Type() == PCB_ARC_T )
        color.a *= m_trackOpacity;
    else if( item->Type() == PCB_VIA_T )
        color.a *= m_viaOpacity;
    else if( item->Type() == PCB_PAD_T )
        color.a *= m_padOpacity;
    else if( item->Type() == PCB_ZONE_T && static_cast<const ZONE*>( item )->IsTeardropArea() )
        color.a *= m_trackOpacity;
    else if( item->Type() == PCB_ZONE_T )
        color.a *= m_zoneOpacity;
    else if( item->Type() == PCB_BITMAP_T )
        color.a *= m_imageOpacity;

    if( item->GetForcedTransparency() > 0.0 )
        color = color.WithAlpha( color.a * ( 1.0 - item->GetForcedTransparency() ) );

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


int PCB_PAINTER::getDrillShape( const PAD* aPad ) const
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
    const BOARD_ITEM* item = dynamic_cast<const BOARD_ITEM*>( aItem );

    if( !item )
        return false;

    if( const BOARD* board = item->GetBoard() )
    {
        BOARD_DESIGN_SETTINGS& bds = board->GetDesignSettings();
        m_maxError = bds.m_MaxError;
        m_holePlatingThickness = bds.GetHolePlatingThickness();
        m_lockedShadowMargin = bds.GetLineThickness( F_SilkS ) * 4;

        if( item->GetParentFootprint() && !board->IsFootprintHolder() )
        {
            FOOTPRINT* parentFP = item->GetParentFootprint();

            // Never draw footprint bitmaps on board
            if( item->Type() == PCB_BITMAP_T )
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
                PCB_LAYER_ID singleLayer = item->GetLayerSet().Seq()[0];

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

    case PCB_BITMAP_T:
        draw( static_cast<const PCB_BITMAP*>( item ), aLayer );
        break;

    case PCB_TEXT_T:
        draw( static_cast<const PCB_TEXT*>( item ), aLayer );
        break;

    case PCB_TEXTBOX_T:
        draw( static_cast<const PCB_TEXTBOX*>( item ), aLayer );
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

    case PCB_TARGET_T:
        draw( static_cast<const PCB_TARGET*>( item ) );
        break;

    case PCB_MARKER_T:
        draw( static_cast<const PCB_MARKER*>( item ), aLayer );
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

        // When drawing netnames, clip the track to the viewport
        BOX2D             viewport;
        VECTOR2D          screenSize = m_gal->GetScreenPixelSize();
        const MATRIX3x3D& matrix = m_gal->GetScreenWorldMatrix();

        viewport.SetOrigin( VECTOR2D( matrix * VECTOR2D( 0, 0 ) ) );
        viewport.SetEnd( VECTOR2D( matrix * screenSize ) );
        viewport.Normalize();

        BOX2I clipBox( viewport.GetOrigin(), viewport.GetSize() );
        SEG   visibleSeg( start, end );

        ClipLine( &clipBox, visibleSeg.A.x, visibleSeg.A.y, visibleSeg.B.x, visibleSeg.B.y );

        wxString netName = aTrack->GetUnescapedShortNetname();
        size_t  num_char = netName.size();

        // Check if the track is long enough to have a netname displayed
        int seg_minlength = track_width * num_char;

        if( visibleSeg.Length() < seg_minlength )
            return;

        double    textSize = track_width;
        double    penWidth = textSize / 12.0;
        EDA_ANGLE textOrientation;
        int num_names = 1;

        if( end.y == start.y ) // horizontal
        {
            textOrientation = ANGLE_HORIZONTAL;
            num_names = std::max( num_names,
                    static_cast<int>( aTrack->GetLength() / viewport.GetWidth() ) );
        }
        else if( end.x == start.x ) // vertical
        {
            textOrientation = ANGLE_VERTICAL;
            num_names = std::max( num_names,
                    static_cast<int>( aTrack->GetLength() / viewport.GetHeight() ) );
        }
        else
        {
            textOrientation = -EDA_ANGLE( visibleSeg.B - visibleSeg.A );
            textOrientation.Normalize90();

            double min_size = std::min( viewport.GetWidth(), viewport.GetHeight() );
            num_names = std::max( num_names,
                    static_cast<int>( aTrack->GetLength() / ( M_SQRT2 * min_size ) ) );
        }

        m_gal->SetIsStroke( true );
        m_gal->SetIsFill( false );
        m_gal->SetStrokeColor( color );
        m_gal->SetLineWidth( penWidth );
        m_gal->SetFontBold( false );
        m_gal->SetFontItalic( false );
        m_gal->SetFontUnderlined( false );
        m_gal->SetTextMirrored( false );
        m_gal->SetGlyphSize( VECTOR2D( textSize * 0.55, textSize * 0.55 ) );
        m_gal->SetHorizontalJustify( GR_TEXT_H_ALIGN_CENTER );
        m_gal->SetVerticalJustify( GR_TEXT_V_ALIGN_CENTER );

        for( int ii = 0; ii < num_names; ++ii )
        {
            VECTOR2I textPosition =
                      VECTOR2D( start ) * static_cast<double>( num_names - ii ) / ( num_names + 1 )
                    + VECTOR2D( end ) * static_cast<double>( ii + 1 ) / ( num_names + 1 );

            if( clipBox.Contains( textPosition ) )
                m_gal->BitmapText( netName, textPosition, textOrientation );
        }

        return;
    }
    else if( IsCopperLayer( aLayer ) || aLayer == LAYER_LOCKED_ITEM_SHADOW )
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

        if( aLayer == LAYER_LOCKED_ITEM_SHADOW )
            track_width = track_width + m_lockedShadowMargin;

        m_gal->DrawSegment( start, end, track_width );
    }

    // Clearance lines
    if( pcbconfig() && pcbconfig()->m_Display.m_TrackClearance == SHOW_WITH_VIA_ALWAYS
            && !m_pcbSettings.m_isPrinting && aLayer != LAYER_LOCKED_ITEM_SHADOW )
    {
        int clearance = aTrack->GetOwnClearance( m_pcbSettings.GetActiveLayer() );

        m_gal->SetLineWidth( m_pcbSettings.m_outlineWidth );
        m_gal->SetIsFill( false );
        m_gal->SetIsStroke( true );
        m_gal->SetStrokeColor( color );
        m_gal->DrawSegment( start, end, track_width + clearance * 2 );
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
    else if( IsCopperLayer( aLayer ) || aLayer == LAYER_LOCKED_ITEM_SHADOW )
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

        if( aLayer == LAYER_LOCKED_ITEM_SHADOW )
            width = width + m_lockedShadowMargin;

        m_gal->DrawArcSegment( center, radius, start_angle, start_angle + angle, width,
                               m_maxError );
    }

    // Clearance lines
    if( pcbconfig() && pcbconfig()->m_Display.m_TrackClearance == SHOW_WITH_VIA_ALWAYS
            && !m_pcbSettings.m_isPrinting && aLayer != LAYER_LOCKED_ITEM_SHADOW )
    {
        int clearance = aArc->GetOwnClearance( m_pcbSettings.GetActiveLayer() );

        m_gal->SetLineWidth( m_pcbSettings.m_outlineWidth );
        m_gal->SetIsFill( false );
        m_gal->SetIsStroke( true );
        m_gal->SetStrokeColor( color );

        m_gal->DrawArcSegment( center, radius, start_angle, start_angle + angle,
                               width + clearance * 2, m_maxError );
    }

// Debug only: enable this code only to test the TransformArcToPolygon function
// and display the polygon outline created by it.
// arcs on F_Cu are approximated with ERROR_INSIDE, others with ERROR_OUTSIDE
#if 0
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

// Debug only: enable this code only to test the SHAPE_ARC::ConvertToPolyline function
// and display the polyline created by it.
#if 0
    SHAPE_ARC arc( aArc->GetCenter(), aArc->GetStart(), aArc->GetAngle() / 10.0, aArc->GetWidth() );
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
        double size = aVia->GetWidth();

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
        m_gal->SetStrokeColor( m_pcbSettings.GetColor( nullptr, aLayer ) );
        m_gal->SetIsStroke( true );
        m_gal->SetIsFill( false );

        // Set the text position via position. if only one text, it is on the via position
        // For 2 lines, the netname is slightly below the center, and the layer IDs above
        // the netname
        VECTOR2D textpos( 0.0, 0.0 );

        wxString netname = aVia->GetUnescapedShortNetname();

        int topLayer = aVia->TopLayer() + 1;
        int bottomLayer = std::min( aVia->BottomLayer() + 1, board->GetCopperLayerCount() );

        wxString layerIds;
        layerIds.Printf( wxT( "%d-%d" ), topLayer, bottomLayer );

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
        if( showLayers )
            textpos.y += tsize/5;

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
        double radius = ( getViaDrillSize( aVia ) / 2.0 ) + m_holePlatingThickness;

        if( !outline_mode )
        {
            m_gal->SetLineWidth( m_holePlatingThickness );
            radius -= m_holePlatingThickness / 2.0;
        }

        m_gal->DrawCircle( center, radius );
    }
    else if( aLayer == LAYER_VIA_HOLES )
    {
        m_gal->SetIsStroke( false );
        m_gal->SetIsFill( true );
        m_gal->DrawCircle( center, getViaDrillSize( aVia ) / 2.0 );
    }
    else if( aLayer == LAYER_VIA_THROUGH || m_pcbSettings.IsPrinting() )
    {
        int    annular_width = ( aVia->GetWidth() - getViaDrillSize( aVia ) ) / 2.0;
        double radius = aVia->GetWidth() / 2.0;
        bool   draw = false;

        if( m_pcbSettings.IsPrinting() )
        {
            draw = aVia->FlashLayer( m_pcbSettings.GetPrintLayers() );
        }
        else if( aVia->FlashLayer( board->GetVisibleLayers() & board->GetEnabledLayers() ) )
        {
            draw = true;
        }
        else if( aVia->IsSelected() )
        {
            draw = true;
            outline_mode = true;
            m_gal->SetLineWidth( m_pcbSettings.m_outlineWidth );
        }

        if( !outline_mode )
        {
            m_gal->SetLineWidth( annular_width );
            radius -= annular_width / 2.0;
        }

        if( draw )
            m_gal->DrawCircle( center, radius );
    }
    else if( aLayer == LAYER_VIA_BBLIND || aLayer == LAYER_VIA_MICROVIA )
    {
        int    annular_width = ( aVia->GetWidth() - getViaDrillSize( aVia ) ) / 2.0;
        double radius = aVia->GetWidth() / 2.0;

        // Outer circles of blind/buried and micro-vias are drawn in a special way to indicate the
        // top and bottom layers
        PCB_LAYER_ID layerTop, layerBottom;
        aVia->LayerPair( &layerTop, &layerBottom );

        if( !outline_mode )
        {
            m_gal->SetIsStroke( false );
            m_gal->SetIsFill( true );
        }

        m_gal->SetStrokeColor( m_pcbSettings.GetColor( aVia, layerTop ) );
        m_gal->SetFillColor( m_pcbSettings.GetColor( aVia, layerTop ) );
        m_gal->DrawArc( center, radius, EDA_ANGLE( 240, DEGREES_T ), EDA_ANGLE( 300, DEGREES_T ) );

        m_gal->SetStrokeColor( m_pcbSettings.GetColor( aVia, layerBottom ) );
        m_gal->SetFillColor( m_pcbSettings.GetColor( aVia, layerBottom ) );
        m_gal->DrawArc( center, radius, EDA_ANGLE( 60, DEGREES_T ), EDA_ANGLE( 120, DEGREES_T ) );

        m_gal->SetStrokeColor( color );
        m_gal->SetFillColor( color );
        m_gal->SetIsStroke( true );
        m_gal->SetIsFill( false );

        if( !outline_mode )
        {
            m_gal->SetLineWidth( annular_width );
            radius -= annular_width / 2.0;
        }

        m_gal->DrawCircle( center, radius );
    }
    else if( aLayer == LAYER_LOCKED_ITEM_SHADOW )    // draw a ring around the via
    {
        m_gal->SetLineWidth( m_lockedShadowMargin );

        m_gal->DrawCircle( center, ( aVia->GetWidth() + m_lockedShadowMargin ) / 2.0 );
    }

    // Clearance lines
    if( pcbconfig() && pcbconfig()->m_Display.m_TrackClearance == SHOW_WITH_VIA_ALWAYS
            && aLayer != LAYER_VIA_HOLES
            && !m_pcbSettings.m_isPrinting )
    {
        PCB_LAYER_ID activeLayer = m_pcbSettings.GetActiveLayer();
        double       radius;

        if( aVia->FlashLayer( activeLayer ) )
            radius = aVia->GetWidth() / 2.0;
        else
            radius = getViaDrillSize( aVia ) / 2.0 + m_holePlatingThickness;

        m_gal->SetLineWidth( m_pcbSettings.m_outlineWidth );
        m_gal->SetIsFill( false );
        m_gal->SetIsStroke( true );
        m_gal->SetStrokeColor( color );
        m_gal->DrawCircle( center, radius + aVia->GetOwnClearance( activeLayer ) );
    }
}


void PCB_PAINTER::draw( const PAD* aPad, int aLayer )
{
    const BOARD* board = aPad->GetBoard();
    COLOR4D      color = m_pcbSettings.GetColor( aPad, aLayer );

    if( IsNetnameLayer( aLayer ) )
    {
        PCBNEW_SETTINGS::DISPLAY_OPTIONS* displayOpts = pcbconfig() ? &pcbconfig()->m_Display : nullptr;
        wxString                          netname;
        wxString                          padNumber;

        if( viewer_settings()->m_ViewersDisplay.m_DisplayPadNumbers )
        {
            padNumber = UnescapeString( aPad->GetNumber() );

            if( dynamic_cast<CVPCB_SETTINGS*>( viewer_settings() ) )
                netname = aPad->GetUnescapedShortNetname();
        }

        if( displayOpts )
        {
            if( displayOpts->m_NetNames == 1 || displayOpts->m_NetNames == 3 )
                netname = aPad->GetUnescapedShortNetname();

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

        if( aPad->GetFlags() & ENTERED )
        {
            FOOTPRINT* fp = aPad->GetParentFootprint();

            // Find the number box
            for( const BOARD_ITEM* aItem : fp->GraphicalItems() )
            {
                if( aItem->Type() == PCB_SHAPE_T )
                {
                    const PCB_SHAPE* shape = static_cast<const PCB_SHAPE*>( aItem );

                    if( shape->IsAnnotationProxy() )
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
        else if( aPad->GetShape() == PAD_SHAPE::CUSTOM )
        {
            // See if we have a number box
            for( const std::shared_ptr<PCB_SHAPE>& primitive : aPad->GetPrimitives() )
            {
                if( primitive->IsAnnotationProxy() )
                {
                    position = aPad->GetPosition() + primitive->GetCenter();
                    padsize.x = abs( primitive->GetBotRight().x - primitive->GetTopLeft().x );
                    padsize.y = abs( primitive->GetBotRight().y - primitive->GetTopLeft().y );

                    // We normally draw a bit outside the pad, but this will be somewhat
                    // unexpected when the user has drawn a box.
                    padsize *= 0.9;

                    break;
                }
            }
        }

        if( aPad->GetShape() != PAD_SHAPE::CUSTOM )
        {
            // Don't allow a 45° rotation to bloat a pad's bounding box unnecessarily
            double limit = std::min( aPad->GetSize().x, aPad->GetSize().y ) * 1.1;

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
            if( aPad->GetShape() == PAD_SHAPE::CIRCLE || aPad->GetShape() == PAD_SHAPE::OVAL )
                tsize *= 0.9;

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
        m_gal->SetIsFill( false );
        m_gal->SetIsStroke( true );
        m_gal->SetLineWidth( m_holePlatingThickness );
        m_gal->SetStrokeColor( color );

        std::shared_ptr<SHAPE_SEGMENT> slot = aPad->GetEffectiveHoleShape();
        int                            holeSize = slot->GetWidth() + m_holePlatingThickness;

        if( slot->GetSeg().A == slot->GetSeg().B )    // Circular hole
            m_gal->DrawCircle( slot->GetSeg().A, KiROUND( holeSize / 2.0 ) );
        else
            m_gal->DrawSegment( slot->GetSeg().A, slot->GetSeg().B, holeSize );

        return;
    }

    bool outline_mode = !viewer_settings()->m_ViewersDisplay.m_DisplayPadFill;

    if( m_pcbSettings.m_ForcePadSketchModeOn )
        outline_mode = true;

    if( outline_mode )
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

    bool drawShape = false;

    if( aLayer == LAYER_PAD_PLATEDHOLES || aLayer == LAYER_NON_PLATEDHOLES )
    {
        SHAPE_SEGMENT slot = getPadHoleShape( aPad );

        if( slot.GetSeg().A == slot.GetSeg().B )    // Circular hole
            m_gal->DrawCircle( slot.GetSeg().A, slot.GetWidth() / 2.0 );
        else
            m_gal->DrawSegment( slot.GetSeg().A, slot.GetSeg().B, slot.GetWidth() );
    }
    else if( m_pcbSettings.IsPrinting() )
    {
        drawShape = aPad->FlashLayer( m_pcbSettings.GetPrintLayers() );
    }
    else if( aPad->FlashLayer( board->GetVisibleLayers() & board->GetEnabledLayers() ) )
    {
        drawShape = true;
    }
    else if( aPad->IsSelected() )
    {
        drawShape = true;
        outline_mode = true;
    }

    if( outline_mode )
    {
        // Outline mode
        m_gal->SetIsFill( false );
        m_gal->SetIsStroke( true );
        m_gal->SetLineWidth( m_pcbSettings.m_outlineWidth );
        m_gal->SetStrokeColor( color );
    }

    if( drawShape )
    {
        VECTOR2I pad_size = aPad->GetSize();
        VECTOR2I margin;

        switch( aLayer )
        {
        case F_Mask:
        case B_Mask:
            margin.x = margin.y = aPad->GetSolderMaskExpansion();
            break;

        case F_Paste:
        case B_Paste:
            margin = aPad->GetSolderPasteMargin();
            break;

        default:
            margin.x = margin.y = 0;
            break;
        }

        std::unique_ptr<PAD>            dummyPad;
        std::shared_ptr<SHAPE_COMPOUND> shapes;

        // Drawing components of compound shapes in outline mode produces a mess.
        bool simpleShapes = !outline_mode;

        if( simpleShapes )
        {
            if( ( margin.x != margin.y && aPad->GetShape() != PAD_SHAPE::CUSTOM )
                || ( aPad->GetShape() == PAD_SHAPE::ROUNDRECT && ( margin.x < 0 || margin.y < 0 ) ) )
            {
                // Our algorithms below (polygon inflation in particular) can't handle differential
                // inflation along separate axes.  So for those cases we build a dummy pad instead,
                // and inflate it.

                // Margin is added to both sides.  If the total margin is larger than the pad
                // then don't display this layer
                if( pad_size.x + 2 * margin.x <= 0 || pad_size.y + 2 * margin.y <= 0 )
                    return;

                dummyPad.reset( static_cast<PAD*>( aPad->Duplicate() ) );

                if( dummyPad->GetParentGroup() )
                    dummyPad->GetParentGroup()->RemoveItem( dummyPad.get() );

                int initial_radius = dummyPad->GetRoundRectCornerRadius();

                dummyPad->SetSize( pad_size + margin + margin );

                if( dummyPad->GetShape() == PAD_SHAPE::ROUNDRECT )
                {
                    // To keep the right margin around the corners, we need to modify the corner radius.
                    // We must have only one radius correction, so use the smallest absolute margin.
                    int radius_margin = std::max( margin.x, margin.y );     // radius_margin is < 0
                    dummyPad->SetRoundRectCornerRadius( std::max( initial_radius + radius_margin, 0 ) );
                }

                shapes = std::dynamic_pointer_cast<SHAPE_COMPOUND>( dummyPad->GetEffectiveShape() );
                margin.x = margin.y = 0;
            }
            else
            {
                shapes = std::dynamic_pointer_cast<SHAPE_COMPOUND>( aPad->GetEffectiveShape() );
            }

            if( aPad->GetShape() == PAD_SHAPE::CUSTOM && ( margin.x || margin.y ) )
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

        if( simpleShapes )
        {
            for( const SHAPE* shape : shapes->Shapes() )
            {
                switch( shape->Type() )
                {
                case SH_SEGMENT:
                {
                    const SHAPE_SEGMENT* seg = (const SHAPE_SEGMENT*) shape;
                    int                  effectiveWidth = seg->GetWidth() + 2 * margin.x;

                    if( effectiveWidth > 0 )
                        m_gal->DrawSegment( seg->GetSeg().A, seg->GetSeg().B, effectiveWidth );

                    break;
                }

                case SH_CIRCLE:
                {
                    const SHAPE_CIRCLE* circle = (const SHAPE_CIRCLE*) shape;
                    int                 effectiveRadius = circle->GetRadius() + margin.x;

                    if( effectiveRadius > 0 )
                        m_gal->DrawCircle( circle->GetCenter(), effectiveRadius );

                    break;
                }

                case SH_RECT:
                {
                    const SHAPE_RECT* r = (const SHAPE_RECT*) shape;
                    VECTOR2I          pos = r->GetPosition();
                    VECTOR2I          effectiveMargin = margin;

                    if( effectiveMargin.x < 0 )
                    {
                        // A negative margin just produces a smaller rect.
                        VECTOR2I effectiveSize = r->GetSize() + effectiveMargin;

                        if( effectiveSize.x > 0 && effectiveSize.y > 0 )
                            m_gal->DrawRectangle( pos - effectiveMargin, pos + effectiveSize );
                    }
                    else if( effectiveMargin.x > 0 )
                    {
                        // A positive margin produces a larger rect, but with rounded corners
                        m_gal->DrawRectangle( r->GetPosition(), r->GetPosition() + r->GetSize() );

                        // Use segments to produce the margin with rounded corners
                        m_gal->DrawSegment( pos,
                                            pos + VECTOR2I( r->GetWidth(), 0 ),
                                            effectiveMargin.x * 2 );
                        m_gal->DrawSegment( pos + VECTOR2I( r->GetWidth(), 0 ),
                                            pos + r->GetSize(),
                                            effectiveMargin.x * 2 );
                        m_gal->DrawSegment( pos + r->GetSize(),
                                            pos + VECTOR2I( 0, r->GetHeight() ),
                                            effectiveMargin.x * 2 );
                        m_gal->DrawSegment( pos + VECTOR2I( 0, r->GetHeight() ),
                                            pos,
                                            effectiveMargin.x * 2 );
                    }
                    else
                    {
                        m_gal->DrawRectangle( r->GetPosition(), r->GetPosition() + r->GetSize() );
                    }

                    break;
                }

                case SH_SIMPLE:
                {
                    const SHAPE_SIMPLE* poly = static_cast<const SHAPE_SIMPLE*>( shape );

                    if( poly->PointCount() < 2 )     // Careful of empty pads
                        break;

                    if( margin.x < 0 )  // The poly shape must be deflated
                    {
                        int numSegs = GetArcToSegmentCount( -margin.x, m_maxError, FULL_CIRCLE );
                        SHAPE_POLY_SET outline;
                        outline.NewOutline();

                        for( int ii = 0; ii < poly->PointCount(); ++ii )
                            outline.Append( poly->CPoint( ii ) );

                        outline.Deflate( -margin.x, numSegs );

                        m_gal->DrawPolygon( outline );
                    }
                    else
                    {
                        m_gal->DrawPolygon( poly->Vertices() );
                    }

                    // Now add on a rounded margin (using segments) if the margin > 0
                    if( margin.x > 0 )
                    {
                        for( size_t ii = 0; ii < poly->GetSegmentCount(); ++ii )
                        {
                            SEG seg = poly->GetSegment( ii );
                            m_gal->DrawSegment( seg.A, seg.B, margin.x * 2 );
                        }
                    }

                    break;
                }

                default:
                    // Better not get here; we already pre-flighted the shapes...
                    break;
                }
            }
        }
        else
        {
            // This is expensive.  Avoid if possible.
            SHAPE_POLY_SET polySet;
            aPad->TransformShapeToPolygon( polySet, ToLAYER_ID( aLayer ), margin.x, m_maxError,
                                           ERROR_INSIDE );
            m_gal->DrawPolygon( polySet );
        }
    }

    if( pcbconfig() && pcbconfig()->m_Display.m_PadClearance
            && ( aLayer == LAYER_PAD_FR || aLayer == LAYER_PAD_BK || aLayer == LAYER_PADS_TH )
            && !m_pcbSettings.m_isPrinting )
    {
        /* Showing the clearance area is not obvious.
         * - A pad can be removed from some copper layers.
         * - For non copper layers, what is the clearance area?
         * So for copper layers, the clearance area is the shape if the pad is flashed on this
         * layer and the hole clearance area for other copper layers.
         * For other layers, use the pad shape, although one can use an other criteria,
         * depending on the non copper layer.
         */
        int  activeLayer = m_pcbSettings.GetActiveLayer();
        bool flashActiveLayer = true;

        if( IsCopperLayer( activeLayer ) )
            flashActiveLayer = aPad->FlashLayer( activeLayer );

        if( !board->GetVisibleLayers().test( activeLayer ) )
            flashActiveLayer = false;

        if( flashActiveLayer || aPad->GetDrillSize().x )
        {
            if( aPad->GetAttribute() == PAD_ATTRIB::NPTH )
                color = m_pcbSettings.GetLayerColor( LAYER_NON_PLATEDHOLES );

            m_gal->SetLineWidth( m_pcbSettings.m_outlineWidth );
            m_gal->SetIsStroke( true );
            m_gal->SetIsFill( false );
            m_gal->SetStrokeColor( color );

            int clearance = aPad->GetOwnClearance( m_pcbSettings.GetActiveLayer() );

            if( flashActiveLayer && clearance > 0 )
            {
                auto shape = std::dynamic_pointer_cast<SHAPE_COMPOUND>( aPad->GetEffectiveShape() );

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
                    aPad->TransformShapeToPolygon( polySet, ToLAYER_ID( aLayer ), clearance,
                                                   m_maxError, ERROR_INSIDE );

                    if( polySet.Outline( 0 ).PointCount() > 2 )     // Careful of empty pads
                        m_gal->DrawPolygon( polySet );
                }
            }
            else if( aPad->GetEffectiveHoleShape() && clearance > 0 )
            {
                clearance += m_holePlatingThickness;

                std::shared_ptr<SHAPE_SEGMENT> slot = aPad->GetEffectiveHoleShape();
                m_gal->DrawSegment( slot->GetSeg().A, slot->GetSeg().B,
                                    slot->GetWidth() + 2 * clearance );
            }
        }
    }
}


void PCB_PAINTER::draw( const PCB_SHAPE* aShape, int aLayer )
{
    COLOR4D        color = m_pcbSettings.GetColor( aShape, aShape->GetLayer() );
    bool           outline_mode = !viewer_settings()->m_ViewersDisplay.m_DisplayGraphicsFill;
    int            thickness = getLineThickness( aShape->GetWidth() );
    PLOT_DASH_TYPE lineStyle = aShape->GetStroke().GetPlotStyle();

    if( aLayer == LAYER_LOCKED_ITEM_SHADOW )
    {
        color = m_pcbSettings.GetColor( aShape, aLayer );
        thickness = thickness + m_lockedShadowMargin;
    }

    if( outline_mode )
    {
        m_gal->SetIsFill( false );
        m_gal->SetIsStroke( true );
        m_gal->SetLineWidth( m_pcbSettings.m_outlineWidth );
    }

    m_gal->SetFillColor( color );
    m_gal->SetStrokeColor( color );

    if( lineStyle <= PLOT_DASH_TYPE::FIRST_TYPE )
    {
        switch( aShape->GetShape() )
        {
        case SHAPE_T::SEGMENT:
            if( outline_mode )
            {
                m_gal->DrawSegment( aShape->GetStart(), aShape->GetEnd(), thickness );
            }
            else
            {
                m_gal->SetIsFill( true );
                m_gal->SetIsStroke( false );

                m_gal->DrawSegment( aShape->GetStart(), aShape->GetEnd(), thickness );
            }

            break;

        case SHAPE_T::RECT:
        {
            std::vector<VECTOR2I> pts = aShape->GetRectCorners();

            if( aShape->IsAnnotationProxy() )
            {
                m_gal->DrawSegment( pts[0], pts[1], thickness );
                m_gal->DrawSegment( pts[1], pts[2], thickness );
                m_gal->DrawSegment( pts[2], pts[3], thickness );
                m_gal->DrawSegment( pts[3], pts[0], thickness );
                m_gal->DrawSegment( pts[0], pts[2], thickness );
                m_gal->DrawSegment( pts[1], pts[3], thickness );
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

                if( thickness > 0 )
                {
                    m_gal->DrawSegment( pts[0], pts[1], thickness );
                    m_gal->DrawSegment( pts[1], pts[2], thickness );
                    m_gal->DrawSegment( pts[2], pts[3], thickness );
                    m_gal->DrawSegment( pts[3], pts[0], thickness );
                }

                if( aShape->IsFilled() )
                {
                    SHAPE_POLY_SET poly;
                    poly.NewOutline();

                    for( const VECTOR2I& pt : pts )
                        poly.Append( pt );

                    m_gal->DrawPolygon( poly );
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
                                       endAngle, thickness, m_maxError );
            }
            else
            {
                m_gal->SetIsFill( true );
                m_gal->SetIsStroke( false );

                m_gal->DrawArcSegment( aShape->GetCenter(), aShape->GetRadius(), startAngle,
                                       endAngle, thickness, m_maxError );
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
                m_gal->SetIsFill( aShape->IsFilled() );
                m_gal->SetIsStroke( thickness > 0 );
                m_gal->SetLineWidth( thickness );

                m_gal->DrawCircle( aShape->GetStart(), aShape->GetRadius() );
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

                if( thickness > 0 )
                {
                    for( int ii = 0; ii < shape.OutlineCount(); ++ii )
                        m_gal->DrawSegmentChain( shape.Outline( ii ), thickness );
                }

                if( aShape->IsFilled() )
                {
                    // On Opengl, a not convex filled polygon is usually drawn by using triangles
                    // as primitives. CacheTriangulation() can create basic triangle primitives to
                    // draw the polygon solid shape on Opengl.  GLU tessellation is much slower,
                    // so currently we are using our tessellation.
                    if( m_gal->IsOpenGlEngine() && !shape.IsTriangulationUpToDate() )
                        shape.CacheTriangulation( true, true );

                    m_gal->DrawPolygon( shape );
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
                converter.GetPoly( output, thickness );

                m_gal->DrawSegmentChain( output, thickness );
            }
            else
            {
            m_gal->SetIsFill( aShape->IsFilled() );
            m_gal->SetIsStroke( thickness > 0 );
            m_gal->SetLineWidth( thickness );

                // Use thickness as filter value to convert the curve to polyline when the curve
                // is not supported
                m_gal->DrawCurve( VECTOR2D( aShape->GetStart() ),
                                  VECTOR2D( aShape->GetBezierC1() ),
                                  VECTOR2D( aShape->GetBezierC2() ),
                                  VECTOR2D( aShape->GetEnd() ), thickness );
            }

            break;

        case SHAPE_T::LAST:
            break;
        }
    }
    else
    {
        if( !outline_mode )
        {
            m_gal->SetIsFill( true );
            m_gal->SetIsStroke( false );
        }

        std::vector<SHAPE*> shapes = aShape->MakeEffectiveShapes( true );

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


void PCB_PAINTER::strokeText( const wxString& aText, const VECTOR2I& aPosition,
                              const TEXT_ATTRIBUTES& aAttrs )
{
    KIFONT::FONT* font = aAttrs.m_Font;

    if( !font )
        font = KIFONT::FONT::GetFont( wxEmptyString, aAttrs.m_Bold, aAttrs.m_Italic );

    m_gal->SetIsFill( font->IsOutline() );
    m_gal->SetIsStroke( font->IsStroke() );

    font->Draw( m_gal, aText, aPosition, aAttrs );
}


void PCB_PAINTER::draw( const PCB_BITMAP* aBitmap, int aLayer )
{
    m_gal->Save();
    m_gal->Translate( aBitmap->GetPosition() );

    // When the image scale factor is not 1.0, we need to modify the actual as the image scale
    // factor is similar to a local zoom
    double img_scale = aBitmap->GetImageScale();

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
        VECTOR2D bm_size( aBitmap->GetSize() );
        // bm_size is the actual image size in UI.
        // but m_gal scale was previously set to img_scale
        // so recalculate size relative to this image size.
        bm_size.x /= img_scale;
        bm_size.y /= img_scale;
        VECTOR2D origin( -bm_size.x / 2.0, -bm_size.y / 2.0 );
        VECTOR2D end = origin + bm_size;

        m_gal->DrawRectangle( origin, end );

        // Hard code bitmaps as opaque when selected. Otherwise cached layers
        // will not be rendered under the selected bitmap because cached layers
        // are rendered after non-cached layers (e.g. bitmaps), which will have
        // a closer Z order.
        m_gal->DrawBitmap( *aBitmap->GetImage(), 1.0 );
    }
    else
        m_gal->DrawBitmap( *aBitmap->GetImage(),
                           m_pcbSettings.GetColor( aBitmap, aBitmap->GetLayer() ).a );


    m_gal->Restore();
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

    TEXT_ATTRIBUTES attrs = aText->GetAttributes();
    const COLOR4D& color = m_pcbSettings.GetColor( aText, aText->GetLayer() );
    bool           outline_mode = !viewer_settings()->m_ViewersDisplay.m_DisplayTextFill;

    KIFONT::FONT* font = aText->GetFont();

    if( !font )
    {
        font = KIFONT::FONT::GetFont( m_pcbSettings.GetDefaultFont(), aText->IsBold(),
                                      aText->IsItalic() );
    }

    m_gal->SetStrokeColor( color );
    m_gal->SetFillColor( color );
    attrs.m_Angle = aText->GetDrawRotation();

    if( aText->IsKnockout() )
    {
        KIGFX::GAL_DISPLAY_OPTIONS empty_opts;
        SHAPE_POLY_SET             knockouts;

        CALLBACK_GAL callback_gal( empty_opts,
                // Polygon callback
                [&]( const SHAPE_LINE_CHAIN& aPoly )
                {
                    knockouts.AddOutline( aPoly );
                } );

        attrs.m_StrokeWidth = getLineThickness( aText->GetEffectiveTextPenWidth() );

        callback_gal.SetIsFill( font->IsOutline() );
        callback_gal.SetIsStroke( font->IsStroke() );
        callback_gal.SetLineWidth( attrs.m_StrokeWidth );
        font->Draw( &callback_gal, resolvedText, aText->GetDrawPos(), attrs );

        SHAPE_POLY_SET finalPoly;
        int            margin = attrs.m_StrokeWidth * 1.5
                                    + GetKnockoutTextMargin( attrs.m_Size, attrs.m_StrokeWidth );

        aText->TransformBoundingBoxToPolygon( &finalPoly, margin );
        finalPoly.BooleanSubtract( knockouts, SHAPE_POLY_SET::PM_FAST );
        finalPoly.Fracture( SHAPE_POLY_SET::PM_FAST );

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

        if( m_gal->IsFlippedX() && !( aText->GetLayerSet() & LSET::SideSpecificMask() ).any() )
        {
            attrs.m_Mirrored = !attrs.m_Mirrored;
            attrs.m_Halign = static_cast<GR_TEXT_H_ALIGN_T>( -attrs.m_Halign );
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
            strokeText( resolvedText, aText->GetTextPos(), attrs );
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
    const COLOR4D& color = m_pcbSettings.GetColor( aTextBox, aLayer );
    int            thickness = getLineThickness( aTextBox->GetWidth() );
    PLOT_DASH_TYPE lineStyle = aTextBox->GetStroke().GetPlotStyle();
    wxString       resolvedText( aTextBox->GetShownText( true ) );

    KIFONT::FONT* font = aTextBox->GetFont();

    if( !font )
    {
        font = KIFONT::FONT::GetFont( m_pcbSettings.GetDefaultFont(), aTextBox->IsBold(),
                                      aTextBox->IsItalic() );
    }

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

        for( size_t ii = 0; ii < pts.size(); ++ii )
            m_gal->DrawSegment( pts[ ii ], pts[ (ii + 1) % pts.size() ], line_thickness );
    }

    m_gal->SetFillColor( color );
    m_gal->SetStrokeColor( color );
    m_gal->SetIsFill( true );
    m_gal->SetIsStroke( false );

    if( lineStyle <= PLOT_DASH_TYPE::FIRST_TYPE )
    {
        if( thickness > 0 )
        {
            std::vector<VECTOR2I> pts = aTextBox->GetCorners();

            for( size_t ii = 0; ii < pts.size(); ++ii )
                m_gal->DrawSegment( pts[ ii ], pts[ (ii + 1) % pts.size() ], thickness );
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

    if( resolvedText.Length() == 0 )
        return;

    TEXT_ATTRIBUTES attrs = aTextBox->GetAttributes();
    attrs.m_StrokeWidth = getLineThickness( aTextBox->GetEffectiveTextPenWidth() );

    if( m_gal->IsFlippedX() && !( aTextBox->GetLayerSet() & LSET::SideSpecificMask() ).any() )
    {
        attrs.m_Mirrored = !attrs.m_Mirrored;
        attrs.m_Halign = static_cast<GR_TEXT_H_ALIGN_T>( -attrs.m_Halign );
    }

    if( aLayer == LAYER_LOCKED_ITEM_SHADOW )
    {
        const COLOR4D sh_color = m_pcbSettings.GetColor( aTextBox, aLayer );
        m_gal->SetFillColor( sh_color );
        m_gal->SetStrokeColor( sh_color );
        attrs.m_StrokeWidth += m_lockedShadowMargin;
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
        strokeText( resolvedText, aTextBox->GetDrawPos(), attrs );
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
        m_gal->DrawPolygon( poly );
#else
        BOX2I    bbox = aFootprint->GetBoundingBox( false, false );
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
        VECTOR2I textOffset = VECTOR2I( width.x / 2, -KiROUND( textSize * 0.5 ) );
        VECTOR2I titleHeight = VECTOR2I( 0, KiROUND( textSize * 2.0 ) );

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

            KIFONT::FONT::GetFont()->Draw( m_gal, aGroup->GetName(), topLeft + textOffset, attrs );
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

    // Draw the outline
    if( !IsZoneFillLayer( aLayer ) )
    {
        const SHAPE_POLY_SET* outline = aZone->Outline();

        if( !m_pcbSettings.m_isPrinting && outline && outline->OutlineCount() > 0 )
        {
            m_gal->SetStrokeColor( color.a > 0.0 ? color.WithAlpha( 1.0 ) : color );
            m_gal->SetIsFill( false );
            m_gal->SetIsStroke( true );
            m_gal->SetLineWidth( m_pcbSettings.m_outlineWidth );

            // Draw each contour (main contour and holes)

            /*
             * m_gal->DrawPolygon( *outline );
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

        m_gal->DrawPolygon( *polySet, displayMode == ZONE_DISPLAY_MODE::SHOW_TRIANGULATION );
    }
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

    if( m_gal->IsFlippedX() && !( aDimension->GetLayerSet() & LSET::SideSpecificMask() ).any() )
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
        strokeText( resolvedText, aDimension->GetTextPos(), attrs );
    }
}


void PCB_PAINTER::draw( const PCB_TARGET* aTarget )
{
    const COLOR4D& strokeColor = m_pcbSettings.GetColor( aTarget, aTarget->GetLayer() );
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


void PCB_PAINTER::draw( const PCB_MARKER* aMarker, int aLayer )
{
    bool isShadow = aLayer == LAYER_MARKER_SHADOWS;

    // Don't paint shadows for invisible markers.
    // It would be nice to do this through layer dependencies but we can't do an "or" there today
    if( isShadow && aMarker->GetBoard()
            && !aMarker->GetBoard()->IsElementVisible( aMarker->GetColorLayer() ) )
    {
        return;
    }

    const_cast<PCB_MARKER*>( aMarker )->SetZoom( 1.0 / sqrt( m_gal->GetZoomFactor() ) );

    SHAPE_LINE_CHAIN polygon;
    aMarker->ShapeToPolygon( polygon );

    COLOR4D color = m_pcbSettings.GetColor( aMarker, isShadow ? LAYER_MARKER_SHADOWS
                                                              : aMarker->GetColorLayer() );

    m_gal->Save();
    m_gal->Translate( aMarker->GetPosition() );

    if( isShadow )
    {
        m_gal->SetStrokeColor( color );
        m_gal->SetIsStroke( true );
        m_gal->SetLineWidth( aMarker->MarkerScale() );
    }
    else
    {
        m_gal->SetFillColor( color );
        m_gal->SetIsFill( true );
    }

    m_gal->DrawPolygon( polygon );
    m_gal->Restore();
}


const double PCB_RENDER_SETTINGS::MAX_FONT_SIZE = pcbIUScale.mmToIU( 10.0 );
