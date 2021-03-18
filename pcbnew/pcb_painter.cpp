/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2013-2019 CERN
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
#include <track.h>
#include <pcb_group.h>
#include <footprint.h>
#include <pad.h>
#include <pcb_shape.h>
#include <kicad_string.h>
#include <zone.h>
#include <pcb_text.h>
#include <pcb_marker.h>
#include <dimension.h>
#include <pcb_target.h>
#include <advanced_config.h>
#include <core/arraydim.h>

#include <layers_id_colors_and_visibility.h>
#include <pcb_painter.h>
#include <pcb_display_options.h>
#include <project/net_settings.h>
#include <settings/color_settings.h>

#include <convert_basic_shapes_to_polygon.h>
#include <gal/graphics_abstraction_layer.h>
#include <geometry/geometry_utils.h>
#include <geometry/shape_line_chain.h>
#include <geometry/shape_rect.h>
#include <geometry/shape_segment.h>
#include <geometry/shape_simple.h>
#include <geometry/shape_circle.h>

using namespace KIGFX;

PCB_RENDER_SETTINGS::PCB_RENDER_SETTINGS()
{
    m_backgroundColor = COLOR4D( 0.0, 0.0, 0.0, 1.0 );
    m_padNumbers = true;
    m_netNamesOnPads = true;
    m_netNamesOnTracks = true;
    m_netNamesOnVias = true;
    m_zoneOutlines = true;
    m_zoneDisplayMode = ZONE_DISPLAY_MODE::SHOW_FILLED;
    m_clearanceDisplayFlags = CL_NONE;
    m_sketchGraphics = false;
    m_sketchText = false;
    m_netColorMode = NET_COLOR_MODE::RATSNEST;
    m_contrastModeDisplay = HIGH_CONTRAST_MODE::NORMAL;
    m_ratsnestDisplayMode = RATSNEST_MODE::ALL;

    m_trackOpacity = 1.0;
    m_viaOpacity   = 1.0;
    m_padOpacity   = 1.0;
    m_zoneOpacity  = 1.0;

    // By default everything should be displayed as filled
    for( unsigned int i = 0; i < arrayDim( m_sketchMode ); ++i )
        m_sketchMode[i] = false;

    update();
}


void PCB_RENDER_SETTINGS::LoadColors( const COLOR_SETTINGS* aSettings )
{
    SetBackgroundColor( aSettings->GetColor( LAYER_PCB_BACKGROUND ) );

    // Init board layers colors:
    for( int i = 0; i < PCB_LAYER_ID_COUNT; i++ )
    {
        m_layerColors[i] = aSettings->GetColor( i );

        // Guard: if the alpah channel is too small, the layer is not visible.
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
    m_layerColors[LAYER_PAD_FR_NETNAMES] = COLOR4D( 1.0, 1.0, 1.0, 0.9 );
    m_layerColors[LAYER_PAD_BK_NETNAMES] = COLOR4D( 1.0, 1.0, 1.0, 0.9 );

    // Netnames for copper layers
    for( LSEQ cu = LSET::AllCuMask().CuStack();  cu;  ++cu )
    {
        const COLOR4D lightLabel( 0.8, 0.8, 0.8, 0.7 );
        const COLOR4D darkLabel = lightLabel.Inverted();
        PCB_LAYER_ID  layer = *cu;

        if( m_layerColors[layer].GetBrightness() > 0.5 )
            m_layerColors[GetNetnameLayer( layer )] = darkLabel;
        else
            m_layerColors[GetNetnameLayer( layer )] = lightLabel;
    }

    update();
}


void PCB_RENDER_SETTINGS::LoadDisplayOptions( const PCB_DISPLAY_OPTIONS& aOptions,
                                              bool aShowPageLimits )
{
    m_hiContrastEnabled = ( aOptions.m_ContrastModeDisplay !=
                            HIGH_CONTRAST_MODE::NORMAL );
    m_padNumbers        = aOptions.m_DisplayPadNum;
    m_sketchGraphics    = !aOptions.m_DisplayGraphicsFill;
    m_sketchText        = !aOptions.m_DisplayTextFill;
    m_curvedRatsnestlines = aOptions.m_DisplayRatsnestLinesCurved;
    m_globalRatsnestlines = aOptions.m_ShowGlobalRatsnest;

    // Whether to draw tracks, vias & pads filled or as outlines
    m_sketchMode[LAYER_PADS_TH]      = !aOptions.m_DisplayPadFill;
    m_sketchMode[LAYER_VIA_THROUGH]  = !aOptions.m_DisplayViaFill;
    m_sketchMode[LAYER_VIA_BBLIND]   = !aOptions.m_DisplayViaFill;
    m_sketchMode[LAYER_VIA_MICROVIA] = !aOptions.m_DisplayViaFill;
    m_sketchMode[LAYER_TRACKS]       = !aOptions.m_DisplayPcbTrackFill;

    // Net names display settings
    switch( aOptions.m_DisplayNetNamesMode )
    {
    case 0:
        m_netNamesOnPads   = false;
        m_netNamesOnTracks = false;
        m_netNamesOnVias   = false;
        break;

    case 1:
        m_netNamesOnPads   = true;
        m_netNamesOnTracks = false;
        m_netNamesOnVias   = true;        // Follow pads or tracks?  For now we chose pads....
        break;

    case 2:
        m_netNamesOnPads   = false;
        m_netNamesOnTracks = true;
        m_netNamesOnVias   = false;       // Follow pads or tracks?  For now we chose pads....
        break;

    case 3:
        m_netNamesOnPads   = true;
        m_netNamesOnTracks = true;
        m_netNamesOnVias   = true;
        break;
    }

    // Zone display settings
    m_zoneDisplayMode = aOptions.m_ZoneDisplayMode;

    // Clearance settings
    switch( aOptions.m_ShowTrackClearanceMode )
    {
        case PCB_DISPLAY_OPTIONS::DO_NOT_SHOW_CLEARANCE:
            m_clearanceDisplayFlags = CL_NONE;
            break;

        case PCB_DISPLAY_OPTIONS::SHOW_TRACK_CLEARANCE_WHILE_ROUTING:
            m_clearanceDisplayFlags = CL_NEW | CL_TRACKS;
            break;

        case PCB_DISPLAY_OPTIONS::SHOW_TRACK_CLEARANCE_WITH_VIA_WHILE_ROUTING:
            m_clearanceDisplayFlags = CL_NEW | CL_TRACKS | CL_VIAS;
            break;

        case PCB_DISPLAY_OPTIONS::SHOW_WHILE_ROUTING_OR_DRAGGING:
            m_clearanceDisplayFlags = CL_NEW | CL_EDITED | CL_TRACKS | CL_VIAS;
            break;

        case PCB_DISPLAY_OPTIONS::SHOW_TRACK_CLEARANCE_WITH_VIA_ALWAYS:
            m_clearanceDisplayFlags = CL_NEW | CL_EDITED | CL_EXISTING | CL_TRACKS | CL_VIAS;
            break;
    }

    if( aOptions.m_DisplayPadClearance )
        m_clearanceDisplayFlags |= CL_PADS;

    m_contrastModeDisplay = aOptions.m_ContrastModeDisplay;

    m_netColorMode = aOptions.m_NetColorMode;

    m_ratsnestDisplayMode = aOptions.m_RatsnestMode;

    m_trackOpacity = aOptions.m_TrackOpacity;
    m_viaOpacity   = aOptions.m_ViaOpacity;
    m_padOpacity   = aOptions.m_PadOpacity;
    m_zoneOpacity  = aOptions.m_ZoneOpacity;

    m_showPageLimits = aShowPageLimits;
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
        const PAD* pad = dynamic_cast<const PAD*>( item );
        const VIA* via = dynamic_cast<const VIA*>( item );
        int        holeLayer = aLayer;
        int        annularRingLayer = UNDEFINED_LAYER;

        if( pad && pad->GetAttribute() == PAD_ATTRIB_PTH )
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
    if( item && ( item->Type() == PCB_ZONE_T || item->Type() == PCB_FP_ZONE_T ) )
    {
        if( IsZoneLayer( aLayer ) )
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
    if( m_hiContrastEnabled && !highlighted && !selected )
    {
        PCB_LAYER_ID primary = GetPrimaryHighContrastLayer();
        bool         isActive = m_highContrastLayers.count( aLayer );

        if( originalLayer == LAYER_PADS_TH )
        {
            if( !static_cast<const PAD*>( item )->FlashLayer( primary ) )
                isActive = false;
        }
        else if( originalLayer == LAYER_VIA_THROUGH )
        {
            if( !static_cast<const VIA*>( item )->FlashLayer( primary ) )
                isActive = false;
        }

        if( !isActive )
        {
            if( m_contrastModeDisplay == HIGH_CONTRAST_MODE::HIDDEN || IsNetnameLayer( aLayer ) )
                color = COLOR4D::CLEAR;
            else
                color = color.Mix( m_layerColors[LAYER_PCB_BACKGROUND], m_hiContrastFactor );
        }
    }

    // Apply per-type opacity overrides
    if( item->Type() == PCB_TRACE_T || item->Type() == PCB_ARC_T )
        color.a *= m_trackOpacity;
    else if( item->Type() == PCB_VIA_T )
        color.a *= m_viaOpacity;
    else if( item->Type() == PCB_PAD_T )
        color.a *= m_padOpacity;
    else if( item->Type() == PCB_ZONE_T || item->Type() == PCB_FP_ZONE_T )
        color.a *= m_zoneOpacity;

    // No special modificators enabled
    return color;
}


PCB_PAINTER::PCB_PAINTER( GAL* aGal ) :
    PAINTER( aGal )
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


VECTOR2D PCB_PAINTER::getDrillSize( const PAD* aPad ) const
{
    return VECTOR2D( aPad->GetDrillSize() );
}


int PCB_PAINTER::getDrillSize( const VIA* aVia ) const
{
    return aVia->GetDrillValue();
}


bool PCB_PAINTER::Draw( const VIEW_ITEM* aItem, int aLayer )
{
    const EDA_ITEM* item = dynamic_cast<const EDA_ITEM*>( aItem );

    if( !item )
        return false;

    if( ADVANCED_CFG::GetCfg().m_DrawBoundingBoxes )
    {
        // Show bounding boxes of painted objects for debugging.
        EDA_RECT box = item->GetBoundingBox();
        m_gal->SetIsFill( false );
        m_gal->SetIsStroke( true );
        m_gal->SetStrokeColor( item->IsSelected() ? COLOR4D( 1.0, 0.2, 0.2, 1 ) :
                               COLOR4D( 0.2, 0.2, 0.2, 1 ) );
        m_gal->SetLineWidth( Mils2iu( 3 ) );
        m_gal->DrawRectangle( box.GetOrigin(), box.GetEnd() );
    }

    // the "cast" applied in here clarifies which overloaded draw() is called
    switch( item->Type() )
    {
    case PCB_TRACE_T:
        draw( static_cast<const TRACK*>( item ), aLayer );
        break;

    case PCB_ARC_T:
        draw( static_cast<const ARC*>( item ), aLayer );
        break;

    case PCB_VIA_T:
        draw( static_cast<const VIA*>( item ), aLayer );
        break;

    case PCB_PAD_T:
        draw( static_cast<const PAD*>( item ), aLayer );
        break;

    case PCB_SHAPE_T:
    case PCB_FP_SHAPE_T:
        draw( static_cast<const PCB_SHAPE*>( item ), aLayer );
        break;

    case PCB_TEXT_T:
        draw( static_cast<const PCB_TEXT*>( item ), aLayer );
        break;

    case PCB_FP_TEXT_T:
        draw( static_cast<const FP_TEXT*>( item ), aLayer );
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

    case PCB_FP_ZONE_T:
        draw( static_cast<const ZONE*>( item ), aLayer );
        break;

    case PCB_DIM_ALIGNED_T:
    case PCB_DIM_CENTER_T:
    case PCB_DIM_ORTHOGONAL_T:
    case PCB_DIM_LEADER_T:
        draw( static_cast<const DIMENSION_BASE*>( item ), aLayer );
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

    return true;
}


void PCB_PAINTER::draw( const TRACK* aTrack, int aLayer )
{
    VECTOR2D start( aTrack->GetStart() );
    VECTOR2D end( aTrack->GetEnd() );
    int      width = aTrack->GetWidth();
    COLOR4D  color = m_pcbSettings.GetColor( aTrack, aLayer );

    if( IsNetnameLayer( aLayer ) )
    {
        if( !m_pcbSettings.m_netNamesOnTracks )
            return;

        if( aTrack->GetNetCode() <= NETINFO_LIST::UNCONNECTED )
            return;

        VECTOR2D line = ( end - start );
        double length = line.EuclideanNorm();

        // Check if the track is long enough to have a netname displayed
        if( length < 10 * width )
            return;

        const wxString& netName = UnescapeString( aTrack->GetShortNetname() );
        double   textSize = width;
        double   penWidth = width / 12.0;
        VECTOR2D textPosition = start + line / 2.0;     // center of the track
        double   textOrientation;

        if( end.y == start.y ) // horizontal
        {
            textOrientation = 0;
            textPosition.y += penWidth;
        }
        else if( end.x == start.x ) // vertical
        {
            textOrientation = M_PI / 2;
            textPosition.x += penWidth;
        }
        else
        {
            textOrientation = -atan( line.y / line.x );
            textPosition.x += penWidth / 1.4;
            textPosition.y += penWidth / 1.4;
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
        m_gal->SetHorizontalJustify( GR_TEXT_HJUSTIFY_CENTER );
        m_gal->SetVerticalJustify( GR_TEXT_VJUSTIFY_CENTER );
        m_gal->BitmapText( netName, textPosition, textOrientation );

        return;
    }
    else if( IsCopperLayer( aLayer ) )
    {
        // Draw a regular track
        bool outline_mode = m_pcbSettings.m_sketchMode[LAYER_TRACKS];
        m_gal->SetStrokeColor( color );
        m_gal->SetFillColor( color );
        m_gal->SetIsStroke( outline_mode );
        m_gal->SetIsFill( not outline_mode );
        m_gal->SetLineWidth( m_pcbSettings.m_outlineWidth );

        m_gal->DrawSegment( start, end, width );
    }

    // Clearance lines
    constexpr int clearanceFlags = PCB_RENDER_SETTINGS::CL_EXISTING
                                 | PCB_RENDER_SETTINGS::CL_TRACKS;

    if( ( m_pcbSettings.m_clearanceDisplayFlags & clearanceFlags ) == clearanceFlags )
    {
        int clearance = aTrack->GetOwnClearance( m_pcbSettings.GetActiveLayer() );

        m_gal->SetLineWidth( m_pcbSettings.m_outlineWidth );
        m_gal->SetIsFill( false );
        m_gal->SetIsStroke( true );
        m_gal->SetStrokeColor( color );
        m_gal->DrawSegment( start, end, width + clearance * 2 );
    }
}


void PCB_PAINTER::draw( const ARC* aArc, int aLayer )
{
    VECTOR2D center( aArc->GetCenter() );
    int      width = aArc->GetWidth();
    COLOR4D  color = m_pcbSettings.GetColor( aArc, aLayer );
    double   radius = aArc->GetRadius();
    double   start_angle = DECIDEG2RAD( aArc->GetArcAngleStart() );
    double   angle = DECIDEG2RAD( aArc->GetAngle() );

    if( IsNetnameLayer( aLayer ) )
    {
        // Ummm, yeah.  Anyone fancy implementing text on a path?
        return;
    }
    else if( IsCopperLayer( aLayer ) )
    {
        // Draw a regular track
        bool outline_mode = m_pcbSettings.m_sketchMode[LAYER_TRACKS];
        m_gal->SetStrokeColor( color );
        m_gal->SetFillColor( color );
        m_gal->SetIsStroke( outline_mode );
        m_gal->SetIsFill( not outline_mode );
        m_gal->SetLineWidth( m_pcbSettings.m_outlineWidth );

        m_gal->DrawArcSegment( center, radius, start_angle, start_angle + angle, width );
    }

    // Clearance lines
    constexpr int clearanceFlags = PCB_RENDER_SETTINGS::CL_EXISTING
                                 | PCB_RENDER_SETTINGS::CL_TRACKS;

    if( ( m_pcbSettings.m_clearanceDisplayFlags & clearanceFlags ) == clearanceFlags )
    {
        int clearance = aArc->GetOwnClearance( m_pcbSettings.GetActiveLayer() );

        m_gal->SetLineWidth( m_pcbSettings.m_outlineWidth );
        m_gal->SetIsFill( false );
        m_gal->SetIsStroke( true );
        m_gal->SetStrokeColor( color );

        m_gal->DrawArcSegment( center, radius, start_angle, start_angle + angle,
                               width + clearance * 2 );
    }
}


void PCB_PAINTER::draw( const VIA* aVia, int aLayer )
{
    BOARD*                 board = aVia->GetBoard();
    BOARD_DESIGN_SETTINGS& bds = board->GetDesignSettings();
    COLOR4D                color = m_pcbSettings.GetColor( aVia, aLayer );
    VECTOR2D               center( aVia->GetStart() );

    if( color == COLOR4D::CLEAR )
        return;

    // Draw description layer
    if( IsNetnameLayer( aLayer ) )
    {
        VECTOR2D position( center );

        // Is anything that we can display enabled?
        if( !m_pcbSettings.m_netNamesOnVias || aVia->GetNetname().empty() )
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
        m_gal->SetStrokeColor( m_pcbSettings.GetColor( NULL, aLayer ) );

        // Set the text position to the pad shape position (the pad position is not the best place)
        VECTOR2D textpos( 0.0, 0.0 );

        wxString netname = UnescapeString( aVia->GetShortNetname() );
        // calculate the size of net name text:
        double tsize = 1.5 * size / netname.Length();
        tsize = std::min( tsize, size );
        // Use a smaller text size to handle interline, pen size..
        tsize *= 0.7;
        VECTOR2D namesize( tsize, tsize );

        m_gal->SetGlyphSize( namesize );
        m_gal->SetLineWidth( namesize.x / 12.0 );
        m_gal->BitmapText( netname, textpos, 0.0 );

        m_gal->Restore();

        return;
    }
    else if( aLayer == LAYER_VIA_HOLEWALLS )
    {
        int platingThickness = bds.GetHolePlatingThickness();

        m_gal->SetIsFill( false );
        m_gal->SetIsStroke( true );
        m_gal->SetStrokeColor( color );
        m_gal->SetLineWidth( platingThickness );

        m_gal->DrawCircle( center, ( getDrillSize( aVia ) + platingThickness ) / 2.0 );

        return;
    }

    bool sketchMode = false;

    switch( aVia->GetViaType() )
    {
    case VIATYPE::THROUGH:      sketchMode = m_pcbSettings.m_sketchMode[LAYER_VIA_THROUGH];  break;
    case VIATYPE::BLIND_BURIED: sketchMode = m_pcbSettings.m_sketchMode[LAYER_VIA_BBLIND];   break;
    case VIATYPE::MICROVIA:     sketchMode = m_pcbSettings.m_sketchMode[LAYER_VIA_MICROVIA]; break;
    default:                    wxASSERT( false );                                           break;
    }

    if( sketchMode )
    {
        // Outline mode
        m_gal->SetIsStroke( true );
        m_gal->SetIsFill( false );
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

    if( aLayer == LAYER_VIA_HOLES )
    {
        m_gal->DrawCircle( center, getDrillSize( aVia ) / 2.0 );
    }
    else if( aLayer == LAYER_VIA_THROUGH || m_pcbSettings.GetDrawIndividualViaLayers() )
    {
        m_gal->DrawCircle( center, aVia->GetWidth() / 2.0 );
    }
    else if( aLayer == LAYER_VIA_BBLIND || aLayer == LAYER_VIA_MICROVIA )
    {
        // Outer circles of blind/buried and micro-vias are drawn in a special way to indicate the
        // top and bottom layers
        PCB_LAYER_ID layerTop, layerBottom;
        aVia->LayerPair( &layerTop, &layerBottom );

        double radius =  aVia->GetWidth() / 2.0;

        if( !sketchMode )
            m_gal->SetLineWidth( ( aVia->GetWidth() - aVia->GetDrillValue() ) / 2.0 );

        m_gal->DrawArc( center, radius, M_PI / 2.0, M_PI );
        m_gal->DrawArc( center, radius, 3.0 * M_PI / 2.0, 2.0 * M_PI );

        if( sketchMode )
            m_gal->SetStrokeColor( m_pcbSettings.GetColor( aVia, layerTop ) );
        else
            m_gal->SetFillColor( m_pcbSettings.GetColor( aVia, layerTop ) );

        m_gal->DrawArc( center, radius, 0.0, M_PI / 2.0 );

        if( sketchMode )
            m_gal->SetStrokeColor( m_pcbSettings.GetColor( aVia, layerBottom ) );
        else
            m_gal->SetFillColor( m_pcbSettings.GetColor( aVia, layerBottom ) );

        m_gal->DrawArc( center, radius, M_PI, 3.0 * M_PI / 2.0 );
    }

    // Clearance lines
    constexpr int clearanceFlags = PCB_RENDER_SETTINGS::CL_EXISTING | PCB_RENDER_SETTINGS::CL_VIAS;

    if( ( m_pcbSettings.m_clearanceDisplayFlags & clearanceFlags ) == clearanceFlags
            && aLayer != LAYER_VIA_HOLES )
    {
        PCB_LAYER_ID activeLayer = m_pcbSettings.GetActiveLayer();
        double       radius;

        if( aVia->FlashLayer( activeLayer ) )
            radius = aVia->GetWidth() / 2.0;
        else
            radius = getDrillSize( aVia ) / 2.0 + bds.GetHolePlatingThickness();

        m_gal->SetLineWidth( m_pcbSettings.m_outlineWidth );
        m_gal->SetIsFill( false );
        m_gal->SetIsStroke( true );
        m_gal->SetStrokeColor( color );
        m_gal->DrawCircle( center, radius + aVia->GetOwnClearance( activeLayer ) );
    }
}


bool isImplicitNet( const wxString& aNetName )
{
    return aNetName.StartsWith( wxT( "Net-(" ) ) || aNetName.StartsWith( wxT( "unconnected-(" ) );
}


void PCB_PAINTER::draw( const PAD* aPad, int aLayer )
{
    BOARD*                 board = aPad->GetBoard();
    BOARD_DESIGN_SETTINGS& bds = board->GetDesignSettings();
    COLOR4D                color = m_pcbSettings.GetColor( aPad, aLayer );

    if( IsNetnameLayer( aLayer ) )
    {
        // Is anything that we can display enabled?
        if( m_pcbSettings.m_netNamesOnPads || m_pcbSettings.m_padNumbers )
        {
            bool displayNetname = ( m_pcbSettings.m_netNamesOnPads && !aPad->GetNetname().empty() );
            EDA_RECT padBBox = aPad->GetBoundingBox();
            VECTOR2D position = padBBox.Centre();
            VECTOR2D padsize = VECTOR2D( padBBox.GetSize() );

            if( aPad->GetShape() != PAD_SHAPE_CUSTOM )
            {
                // Don't allow a 45º rotation to bloat a pad's bounding box unnecessarily
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
            if( padsize.x < padsize.y )
            {
                m_gal->Rotate( DECIDEG2RAD( -900.0 ) );
                size = padsize.x;
                std::swap( padsize.x, padsize.y );
            }

            // Font size limits
            if( size > maxSize )
                size = maxSize;

            // Default font settings
            m_gal->SetHorizontalJustify( GR_TEXT_HJUSTIFY_CENTER );
            m_gal->SetVerticalJustify( GR_TEXT_VJUSTIFY_CENTER );
            m_gal->SetFontBold( false );
            m_gal->SetFontItalic( false );
            m_gal->SetFontUnderlined( false );
            m_gal->SetTextMirrored( false );
            m_gal->SetStrokeColor( m_pcbSettings.GetColor( aPad, aLayer ) );
            m_gal->SetIsStroke( true );
            m_gal->SetIsFill( false );

            // We have already translated the GAL to be centered at the center of the pad's
            // bounding box
            VECTOR2D textpos( 0.0, 0.0 );

            // Divide the space, to display both pad numbers and netnames and set the Y text
            // position to display 2 lines
            if( displayNetname && m_pcbSettings.m_padNumbers )
            {
                size = size / 2.0;
                textpos.y = size / 2.0;
            }

            if( displayNetname )
            {
                wxString netname = UnescapeString( aPad->GetShortNetname() );
                wxString pinType = aPad->GetPinType();

                if( pinType == wxT( "no_connect" ) || pinType.EndsWith( wxT( "+no_connect" ) ) )
                    netname = "x";
                else if( pinType == wxT( "free" ) && isImplicitNet( netname ) )
                    netname = "*";

                // calculate the size of net name text:
                double tsize = 1.5 * padsize.x / netname.Length();
                tsize = std::min( tsize, size );
                // Use a smaller text size to handle interline, pen size..
                tsize *= 0.7;
                VECTOR2D namesize( tsize, tsize );

                m_gal->SetGlyphSize( namesize );
                m_gal->SetLineWidth( namesize.x / 12.0 );
                m_gal->BitmapText( netname, textpos, 0.0 );
            }

            if( m_pcbSettings.m_padNumbers )
            {
                const wxString& padName = aPad->GetName();
                textpos.y = -textpos.y;
                double tsize = 1.5 * padsize.x / padName.Length();
                tsize = std::min( tsize, size );
                // Use a smaller text size to handle interline, pen size..
                tsize *= 0.7;
                tsize = std::min( tsize, size );
                VECTOR2D numsize( tsize, tsize );

                m_gal->SetGlyphSize( numsize );
                m_gal->SetLineWidth( numsize.x / 12.0 );
                m_gal->BitmapText( padName, textpos, 0.0 );
            }

            m_gal->Restore();
        }
        return;
    }
    else if( aLayer == LAYER_PAD_HOLEWALLS )
    {
        int platingThickness = bds.GetHolePlatingThickness();

        m_gal->SetIsFill( false );
        m_gal->SetIsStroke( true );
        m_gal->SetLineWidth( platingThickness );
        m_gal->SetStrokeColor( color );

        const SHAPE_SEGMENT* seg = aPad->GetEffectiveHoleShape();
        int                  holeSize = seg->GetWidth() + platingThickness;

        if( seg->GetSeg().A == seg->GetSeg().B )    // Circular hole
            m_gal->DrawCircle( seg->GetSeg().A, holeSize / 2 );
        else
            m_gal->DrawSegment( seg->GetSeg().A, seg->GetSeg().B, holeSize );

        return;
    }

    if( m_pcbSettings.m_sketchMode[LAYER_PADS_TH] )
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
        const SHAPE_SEGMENT* seg = aPad->GetEffectiveHoleShape();

        if( seg->GetSeg().A == seg->GetSeg().B )    // Circular hole
            m_gal->DrawCircle( seg->GetSeg().A, getDrillSize( aPad ).x / 2 );
        else
            m_gal->DrawSegment( seg->GetSeg().A, seg->GetSeg().B, seg->GetWidth() );
    }
    else
    {
        wxSize pad_size = aPad->GetSize();
        wxSize margin;

        switch( aLayer )
        {
        case F_Mask:
        case B_Mask:
            margin.x = margin.y = aPad->GetSolderMaskMargin();
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
        bool                            simpleShapes = true;

        if( margin.x != margin.y && aPad->GetShape() != PAD_SHAPE_CUSTOM )
        {
            // Our algorithms below (polygon inflation in particular) can't handle differential
            // inflation along separate axes.  So for those cases we build a dummy pad instead,
            // and inflate it.

            // Margin is added to both sides.  If the total margin is larger than the pad
            // then don't display this layer
            if( pad_size.x + 2 * margin.x <= 0 || pad_size.y + 2 * margin.y <= 0 )
                return;

            dummyPad.reset( static_cast<PAD*>( aPad->Duplicate() ) );
            dummyPad->SetSize( pad_size + margin + margin );
            shapes = std::dynamic_pointer_cast<SHAPE_COMPOUND>( dummyPad->GetEffectiveShape() );
            margin.x = margin.y = 0;
        }
        else
        {
            shapes = std::dynamic_pointer_cast<SHAPE_COMPOUND>( aPad->GetEffectiveShape() );
        }

        for( const SHAPE* shape : shapes->Shapes() )
        {
            // Drawing components of compound shapes in outline mode produces a mess.
            if( m_pcbSettings.m_sketchMode[LAYER_PADS_TH] )
                simpleShapes = false;

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
                }
                    break;

                case SH_CIRCLE:
                {
                    const SHAPE_CIRCLE* circle = (const SHAPE_CIRCLE*) shape;
                    int                 effectiveRadius = circle->GetRadius() + margin.x;

                    if( effectiveRadius > 0 )
                        m_gal->DrawCircle( circle->GetCenter(), effectiveRadius );
                }
                    break;

                case SH_RECT:
                {
                    const SHAPE_RECT* r = (const SHAPE_RECT*) shape;
                    VECTOR2I          position = r->GetPosition();
                    VECTOR2I          effectiveSize = r->GetSize() + margin;

                    // At this point, if margin.x < 0 the actual rectangle size is
                    // smaller than SHAPE_RECT r (the pad size was not modifed)
                    if( margin.x < 0 )
                    {
                        if( effectiveSize.x > 0 && effectiveSize.y > 0 )
                            m_gal->DrawRectangle( position - margin, position + effectiveSize );
                    }
                    else
                    {
                        m_gal->DrawRectangle( r->GetPosition(), r->GetPosition() + r->GetSize() );
                    }

                    // Now add on a rounded margin (using segments) if the margin > 0
                    if( margin.x > 0 )
                    {
                        m_gal->DrawSegment( position,
                                            position + VECTOR2I( r->GetWidth(), 0 ),
                                            margin.x * 2 );
                        m_gal->DrawSegment( position + VECTOR2I( r->GetWidth(), 0 ),
                                            position + r->GetSize(),
                                            margin.x * 2 );
                        m_gal->DrawSegment( position + r->GetSize(),
                                            position + VECTOR2I( 0, r->GetHeight() ),
                                            margin.x * 2 );
                        m_gal->DrawSegment( position + VECTOR2I( 0, r->GetHeight() ),
                                            position,
                                            margin.x * 2 );
                    }
                }
                    break;

                case SH_SIMPLE:
                {
                    const SHAPE_SIMPLE* poly = static_cast<const SHAPE_SIMPLE*>( shape );
                    m_gal->DrawPolygon( poly->Vertices() );

                    // Now add on a rounded margin (using segments) if the margin > 0
                    if( margin.x > 0 )
                    {
                        for( size_t ii = 0; ii < poly->GetSegmentCount(); ++ii )
                        {
                            SEG seg = poly->GetSegment( ii );
                            m_gal->DrawSegment( seg.A, seg.B, margin.x * 2 );
                        }
                    }
                }
                    break;

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
            aPad->TransformShapeWithClearanceToPolygon( polySet, ToLAYER_ID( aLayer ), margin.x,
                                                        bds.m_MaxError, ERROR_INSIDE );
            m_gal->DrawPolygon( polySet );
        }
    }

    constexpr int clearanceFlags = PCB_RENDER_SETTINGS::CL_PADS;

    if( ( m_pcbSettings.m_clearanceDisplayFlags & clearanceFlags ) == clearanceFlags
            && ( aLayer == LAYER_PAD_FR || aLayer == LAYER_PAD_BK || aLayer == LAYER_PADS_TH ) )
    {
        /* Showing the clearance area is not obvious.
         * - A pad can be removed from some copper layers.
         * - For non copper layers, what is the clearance area?
         * So for copper layers, the clearance area is the shape if the pad is flashed on this
         * layer and the hole clearance area for other copper layers.
         * For other layers, use the pad shape, although one can use an other criteria,
         * depending on the non copper layer.
         */
        int activeLayer = m_pcbSettings.GetActiveLayer();
        bool flashActiveLayer = IsCopperLayer( activeLayer ) ?
                                    aPad->FlashLayer( activeLayer ) : true;

        if( flashActiveLayer || aPad->GetDrillSize().x )
        {
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
                    aPad->TransformShapeWithClearanceToPolygon( polySet, ToLAYER_ID( aLayer ),
                                                                clearance,
                                                                bds.m_MaxError, ERROR_OUTSIDE );
                    m_gal->DrawPolygon( polySet );
                }
            }
            else if( aPad->GetEffectiveHoleShape() && clearance > 0 )
            {
                clearance += bds.GetHolePlatingThickness();

                const SHAPE_SEGMENT* seg = aPad->GetEffectiveHoleShape();
                m_gal->DrawSegment( seg->GetSeg().A, seg->GetSeg().B,
                                    seg->GetWidth() + 2 * clearance );
            }
        }
    }
}


void PCB_PAINTER::draw( const PCB_SHAPE* aShape, int aLayer )
{
    const COLOR4D& color = m_pcbSettings.GetColor( aShape, aShape->GetLayer() );
    bool           sketch = m_pcbSettings.m_sketchGraphics;
    int            thickness = getLineThickness( aShape->GetWidth() );
    VECTOR2D       start( aShape->GetStart() );
    VECTOR2D       end( aShape->GetEnd() );

    if( sketch )
    {
        m_gal->SetIsFill( false );
        m_gal->SetIsStroke( true );
        m_gal->SetLineWidth( m_pcbSettings.m_outlineWidth );
    }

    m_gal->SetFillColor( color );
    m_gal->SetStrokeColor( color );

    switch( aShape->GetShape() )
    {
    case S_SEGMENT:
        if( sketch )
        {
            m_gal->DrawSegment( start, end, thickness );
        }
        else
        {
            m_gal->SetIsFill( true );
            m_gal->SetIsStroke( false );

            m_gal->DrawSegment( start, end, thickness );
        }
        break;

    case S_RECT:
    {
        std::vector<wxPoint> pts = aShape->GetRectCorners();

        if( sketch )
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

                for( const wxPoint& pt : pts )
                    poly.Append( pt );

                m_gal->DrawPolygon( poly );
            }
        }
    }
        break;

    case S_ARC:
        if( sketch )
        {
            m_gal->DrawArcSegment( start, aShape->GetRadius(),
                    DECIDEG2RAD( aShape->GetArcAngleStart() ),
                    DECIDEG2RAD( aShape->GetArcAngleStart() + aShape->GetAngle() ), // Change this
                    thickness );
        }
        else
        {
            m_gal->SetIsFill( true );
            m_gal->SetIsStroke( false );

            m_gal->DrawArcSegment( start, aShape->GetRadius(),
                    DECIDEG2RAD( aShape->GetArcAngleStart() ),
                    DECIDEG2RAD( aShape->GetArcAngleStart() + aShape->GetAngle() ), // Change this
                    thickness );
        }
        break;

    case S_CIRCLE:
        if( sketch )
        {
            m_gal->DrawCircle( start, aShape->GetRadius() - thickness / 2 );
            m_gal->DrawCircle( start, aShape->GetRadius() + thickness / 2 );
        }
        else
        {
            m_gal->SetIsFill( aShape->IsFilled() );
            m_gal->SetIsStroke( thickness > 0 );
            m_gal->SetLineWidth( thickness );

            m_gal->DrawCircle( start, aShape->GetRadius() );
        }
        break;

    case S_POLYGON:
    {
        SHAPE_POLY_SET&  shape = const_cast<PCB_SHAPE*>( aShape )->GetPolyShape();
        const FOOTPRINT* parentFootprint = aShape->GetParentFootprint();

        if( shape.OutlineCount() == 0 )
            break;

        if( parentFootprint )
        {
            m_gal->Save();
            m_gal->Translate( parentFootprint->GetPosition() );
            m_gal->Rotate( -parentFootprint->GetOrientationRadians() );
        }

        if( sketch )
        {
            for( int ii = 0; ii < shape.Outline( 0 ).SegmentCount(); ++ii )
            {
                SEG seg = shape.Outline( 0 ).Segment( ii );
                m_gal->DrawSegment( seg.A, seg.B, thickness );
            }
        }
        else
        {
            m_gal->SetIsFill( true );
            m_gal->SetIsStroke( false );

            if( thickness > 0 )
            {
                for( int ii = 0; ii < shape.Outline( 0 ).SegmentCount(); ++ii )
                {
                    SEG seg = shape.Outline( 0 ).Segment( ii );
                    m_gal->DrawSegment( seg.A, seg.B, thickness );
                }
            }

            if( aShape->IsFilled() )
            {
                // On Opengl, a not convex filled polygon is usually drawn by using triangles
                // as primitives. CacheTriangulation() can create basic triangle primitives to
                // draw the polygon solid shape on Opengl.  GLU tesselation is much slower, so
                // currently we are using our tesselation.
                if( m_gal->IsOpenGlEngine() && !shape.IsTriangulationUpToDate() )
                    shape.CacheTriangulation();

                m_gal->DrawPolygon( shape );
            }
        }

        if( parentFootprint )
            m_gal->Restore();
    }
        break;

    case S_CURVE:
        if( sketch )
        {
            // Use thickness as filter value to convert the curve to polyline when the curve
            // is not supported
            m_gal->DrawCurve( VECTOR2D( aShape->GetStart() ),
                              VECTOR2D( aShape->GetBezControl1() ),
                              VECTOR2D( aShape->GetBezControl2() ),
                              VECTOR2D( aShape->GetEnd() ), thickness );
        }
        else
        {
            m_gal->SetIsFill( aShape->IsFilled() );
            m_gal->SetIsStroke( thickness > 0 );
            m_gal->SetLineWidth( thickness );

            // Use thickness as filter value to convert the curve to polyline when the curve
            // is not supported
            m_gal->DrawCurve( VECTOR2D( aShape->GetStart() ),
                              VECTOR2D( aShape->GetBezControl1() ),
                              VECTOR2D( aShape->GetBezControl2() ),
                              VECTOR2D( aShape->GetEnd() ), thickness );
        }
        break;

    case S_LAST:
        break;
    }
}


void PCB_PAINTER::draw( const PCB_TEXT* aText, int aLayer )
{
    wxString shownText( aText->GetShownText() );

    if( shownText.Length() == 0 )
        return;

    const COLOR4D& color = m_pcbSettings.GetColor( aText, aText->GetLayer() );
    VECTOR2D position( aText->GetTextPos().x, aText->GetTextPos().y );

    if( m_pcbSettings.m_sketchText || m_pcbSettings.m_sketchMode[aLayer] )
    {
        // Outline mode
        m_gal->SetLineWidth( m_pcbSettings.m_outlineWidth );
    }
    else
    {
        // Filled mode
        m_gal->SetLineWidth( getLineThickness( aText->GetEffectiveTextPenWidth() ) );
    }

    m_gal->SetStrokeColor( color );
    m_gal->SetIsFill( false );
    m_gal->SetIsStroke( true );
    m_gal->SetTextAttributes( aText );
    m_gal->StrokeText( shownText, position, aText->GetTextAngleRadians() );
}


void PCB_PAINTER::draw( const FP_TEXT* aText, int aLayer )
{
    wxString shownText( aText->GetShownText() );

    if( shownText.Length() == 0 )
        return;

    const COLOR4D& color = m_pcbSettings.GetColor( aText, aLayer );
    VECTOR2D position( aText->GetTextPos().x, aText->GetTextPos().y );

    if( m_pcbSettings.m_sketchText )
    {
        // Outline mode
        m_gal->SetLineWidth( m_pcbSettings.m_outlineWidth );
    }
    else
    {
        // Filled mode
        m_gal->SetLineWidth( getLineThickness( aText->GetEffectiveTextPenWidth() ) );
    }

    m_gal->SetStrokeColor( color );
    m_gal->SetIsFill( false );
    m_gal->SetIsStroke( true );
    m_gal->SetTextAttributes( aText );
    m_gal->StrokeText( shownText, position, aText->GetDrawRotationRadians() );

    // Draw the umbilical line
    if( aText->IsSelected() )
    {
        m_gal->SetLineWidth( m_pcbSettings.m_outlineWidth );
        m_gal->SetStrokeColor( m_pcbSettings.GetColor( nullptr, LAYER_ANCHOR ) );
        m_gal->DrawLine( position, aText->GetParent()->GetPosition() );
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

#if 0   // For debug purpose only: draw the footing bounding box
        double bboxThickness = 1.0 / m_gal->GetWorldScale();
        m_gal->SetLineWidth( bboxThickness );
        EDA_RECT rect = aFootprint->GetBoundingBox();
        m_gal->DrawRectangle( VECTOR2D( rect.GetOrigin() ), VECTOR2D( rect.GetEnd() ) );

        double bboxThickness = 3.0 / m_gal->GetWorldScale();
        m_gal->SetLineWidth( bboxThickness );
        SHAPE_POLY_SET convex = aFootprint->GetBoundingHull();

        m_gal->DrawPolyline( convex.COutline( 0 ) );
#endif
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
            return;
        }

        const COLOR4D color = m_pcbSettings.GetColor( aGroup, LAYER_ANCHOR );

        EDA_RECT bbox = aGroup->GetBoundingBox();
        m_gal->SetStrokeColor( color );
        m_gal->SetLineWidth( m_pcbSettings.m_outlineWidth * 2.0f );
        wxPoint topLeft = bbox.GetPosition();
        wxPoint width = wxPoint( bbox.GetWidth(), 0 );
        wxPoint height = wxPoint( 0, bbox.GetHeight() );

        m_gal->DrawLine( topLeft, topLeft + width );
        m_gal->DrawLine( topLeft + width, topLeft + width + height );
        m_gal->DrawLine( topLeft + width + height, topLeft + height );
        m_gal->DrawLine( topLeft + height, topLeft );

        wxString name = aGroup->GetName();

        int ptSize = 12;
        int scaledSize = abs( KiROUND( m_gal->GetScreenWorldMatrix().GetScale().x * ptSize ) );
        int unscaledSize = Mils2iu( ptSize );

        // Scale by zoom a bit, but not too much
        int     textSize = ( scaledSize + ( unscaledSize * 2 ) ) / 3;
        int     penWidth = textSize / 10;
        wxPoint textOffset = wxPoint( width.x / 2, - KiROUND( textSize * 0.5 ) );
        wxPoint titleHeight = wxPoint( 0, KiROUND( textSize * 2.0 ) );

        if( !name.IsEmpty() && (int) aGroup->GetName().Length() * textSize < bbox.GetWidth() )
        {
            m_gal->DrawLine( topLeft, topLeft - titleHeight );
            m_gal->DrawLine( topLeft - titleHeight, topLeft + width - titleHeight );
            m_gal->DrawLine( topLeft + width - titleHeight, topLeft + width );

            m_gal->SetFontBold( false );
            m_gal->SetFontItalic( true );
            m_gal->SetFontUnderlined( false );
            m_gal->SetTextMirrored( m_gal->IsFlippedX() );
            m_gal->SetHorizontalJustify( GR_TEXT_HJUSTIFY_CENTER );
            m_gal->SetVerticalJustify( GR_TEXT_VJUSTIFY_BOTTOM );
            m_gal->SetIsFill( false );
            m_gal->SetGlyphSize( VECTOR2D( textSize, textSize ) );
            m_gal->SetLineWidth( penWidth );
            m_gal->StrokeText( aGroup->GetName(), topLeft + textOffset, 0.0 );
        }
    }
}


void PCB_PAINTER::draw( const ZONE* aZone, int aLayer )
{
    /**
     * aLayer will be the virtual zone layer (LAYER_ZONE_START, ... in GAL_LAYER_ID)
     * This is used for draw ordering in the GAL.
     * The color for the zone comes from the associated copper layer ( aLayer - LAYER_ZONE_START )
     * and the visibility comes from the combination of that copper layer and LAYER_ZONES
     */
    wxASSERT( IsZoneLayer( aLayer ) );
    PCB_LAYER_ID layer = static_cast<PCB_LAYER_ID>( aLayer - LAYER_ZONE_START );

    if( !aZone->IsOnLayer( layer ) )
        return;

    COLOR4D color = m_pcbSettings.GetColor( aZone, layer );
    std::deque<VECTOR2D> corners;
    ZONE_DISPLAY_MODE displayMode = m_pcbSettings.m_zoneDisplayMode;

    // Draw the outline
    const SHAPE_POLY_SET* outline = aZone->Outline();

    if( m_pcbSettings.m_zoneOutlines && outline && outline->OutlineCount() > 0 )
    {
        m_gal->SetStrokeColor( color );
        m_gal->SetIsFill( false );
        m_gal->SetIsStroke( true );
        m_gal->SetLineWidth( m_pcbSettings.m_outlineWidth );

        // Draw each contour (main contour and holes)

        /* This line:
         * m_gal->DrawPolygon( *outline );
         * should be enough, but currently does not work to draw holes contours in a complex polygon
         * so each contour is draw as a simple polygon
         */

        // Draw the main contour
        m_gal->DrawPolyline( outline->COutline( 0 ) );

        // Draw holes
        int holes_count = outline->HoleCount( 0 );

        for( int ii = 0; ii < holes_count; ++ii )
            m_gal->DrawPolyline( outline->CHole( 0, ii ) );

        // Draw hatch lines
        for( const SEG& hatchLine : aZone->GetHatchLines() )
            m_gal->DrawLine( hatchLine.A, hatchLine.B );
    }

    // Draw the filling
    if( displayMode == ZONE_DISPLAY_MODE::SHOW_FILLED
            || displayMode == ZONE_DISPLAY_MODE::SHOW_FILLED_OUTLINE )
    {
        const SHAPE_POLY_SET& polySet = aZone->GetFilledPolysList( layer );

        if( polySet.OutlineCount() == 0 )  // Nothing to draw
            return;

        // Set up drawing options
        int outline_thickness = 0;

        if( aZone->GetFilledPolysUseThickness( layer ) )
            outline_thickness = aZone->GetMinThickness();

        m_gal->SetStrokeColor( color );
        m_gal->SetFillColor( color );
        m_gal->SetLineWidth( outline_thickness );

        if( displayMode == ZONE_DISPLAY_MODE::SHOW_FILLED )
        {
            m_gal->SetIsFill( true );
            m_gal->SetIsStroke( outline_thickness > 0 );
        }
        else if( displayMode == ZONE_DISPLAY_MODE::SHOW_FILLED_OUTLINE )
        {
            m_gal->SetIsFill( false );
            m_gal->SetIsStroke( true );
        }

        m_gal->DrawPolygon( polySet );
    }
}


void PCB_PAINTER::draw( const DIMENSION_BASE* aDimension, int aLayer )
{
    const COLOR4D& strokeColor = m_pcbSettings.GetColor( aDimension, aLayer );

    m_gal->SetStrokeColor( strokeColor );
    m_gal->SetIsFill( false );
    m_gal->SetIsStroke( true );

    if( m_pcbSettings.m_sketchGraphics )
    {
        // Outline mode
        m_gal->SetLineWidth( m_pcbSettings.m_outlineWidth );
    }
    else
    {
        // Filled mode
        m_gal->SetLineWidth( getLineThickness( aDimension->GetLineThickness() ) );
    }

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
    const PCB_TEXT& text = aDimension->Text();
    VECTOR2D position( text.GetTextPos().x, text.GetTextPos().y );

    if( m_pcbSettings.m_sketchText )
    {
        // Outline mode
        m_gal->SetLineWidth( m_pcbSettings.m_outlineWidth );
    }
    else
    {
        // Filled mode
        m_gal->SetLineWidth( getLineThickness( text.GetEffectiveTextPenWidth() ) );
    }

    m_gal->SetTextAttributes( &text );
    m_gal->StrokeText( text.GetShownText(), position, text.GetTextAngleRadians() );
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


const double PCB_RENDER_SETTINGS::MAX_FONT_SIZE = Millimeter2iu( 10.0 );
