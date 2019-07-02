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

#include <class_board.h>
#include <class_track.h>
#include <class_module.h>
#include <class_pad.h>
#include <class_drawsegment.h>
#include <class_zone.h>
#include <class_pcb_text.h>
#include <colors_design_settings.h>
#include <class_marker_pcb.h>
#include <class_dimension.h>
#include <class_pcb_target.h>
#include <class_marker_pcb.h>

#include <layers_id_colors_and_visibility.h>
#include <pcb_painter.h>
#include <pcb_display_options.h>

#include <convert_basic_shapes_to_polygon.h>
#include <gal/graphics_abstraction_layer.h>
#include <geometry/geometry_utils.h>
#include <geometry/shape_line_chain.h>


using namespace KIGFX;

PCB_RENDER_SETTINGS::PCB_RENDER_SETTINGS()
{
    m_backgroundColor = COLOR4D( 0.0, 0.0, 0.0, 1.0 );
    m_padNumbers = true;
    m_netNamesOnPads = true;
    m_netNamesOnTracks = true;
    m_netNamesOnVias = true;
    m_zoneOutlines = true;
    m_displayZone = DZ_SHOW_FILLED;
    m_clearance = CL_NONE;
    m_sketchBoardGfx = false;
    m_sketchFpGfx = false;
    m_sketchFpTxtfx = false;
    m_selectionCandidateColor = COLOR4D( 0.0, 1.0, 0.0, 0.75 );

    // By default everything should be displayed as filled
    for( unsigned int i = 0; i < arrayDim( m_sketchMode ); ++i )
    {
        m_sketchMode[i] = false;
    }

    COLORS_DESIGN_SETTINGS dummyCds( FRAME_PCB );
    ImportLegacyColors( &dummyCds );

    update();
}


void PCB_RENDER_SETTINGS::ImportLegacyColors( const COLORS_DESIGN_SETTINGS* aSettings )
{
    // Init board layers colors:
    for( int i = 0; i < PCB_LAYER_ID_COUNT; i++ )
    {
        m_layerColors[i] = aSettings->GetLayerColor( i );

        // Guard: if the alpah channel is too small, the layer is not visible.
        // clamp it to 0.2
        if( m_layerColors[i].a < 0.2 )
            m_layerColors[i].a = 0.2;
    }

    // Init specific graphic layers colors:
    for( int i = GAL_LAYER_ID_START; i < GAL_LAYER_ID_END; i++ )
        m_layerColors[i] = aSettings->GetItemColor( i );

    // Default colors for specific layers (not really board layers).
    m_layerColors[LAYER_VIAS_HOLES]         = COLOR4D( 0.5, 0.4, 0.0, 0.8 );
    m_layerColors[LAYER_PADS_PLATEDHOLES]   = aSettings->GetItemColor( LAYER_PCB_BACKGROUND );
    m_layerColors[LAYER_VIAS_NETNAMES]      = COLOR4D( 0.2, 0.2, 0.2, 0.9 );
    m_layerColors[LAYER_PADS_NETNAMES]      = COLOR4D( 1.0, 1.0, 1.0, 0.9 );
    m_layerColors[LAYER_PAD_FR_NETNAMES]    = COLOR4D( 1.0, 1.0, 1.0, 0.9 );
    m_layerColors[LAYER_PAD_BK_NETNAMES]    = COLOR4D( 1.0, 1.0, 1.0, 0.9 );
    m_layerColors[LAYER_DRC]                = COLOR4D( 1.0, 0.0, 0.0, 0.8 );

    // LAYER_PADS_TH, LAYER_NON_PLATEDHOLES, LAYER_ANCHOR ,LAYER_RATSNEST,
    // LAYER_VIA_THROUGH, LAYER_VIA_BBLIND, LAYER_VIA_MICROVIA
    // are initialized from aSettings

    // These colors are not actually used. Set just in case...
    m_layerColors[LAYER_MOD_TEXT_FR] = m_layerColors[F_SilkS];
    m_layerColors[LAYER_MOD_TEXT_BK] = m_layerColors[B_SilkS];

    // Netnames for copper layers
    for( LSEQ cu = LSET::AllCuMask().CuStack();  cu;  ++cu )
    {
        const COLOR4D lightLabel( 0.8, 0.8, 0.8, 0.7 );
        const COLOR4D darkLabel = lightLabel.Inverted();
        PCB_LAYER_ID layer = *cu;

        if( m_layerColors[layer].GetBrightness() > 0.5 )
            m_layerColors[GetNetnameLayer( layer )] = darkLabel;
        else
            m_layerColors[GetNetnameLayer( layer )] = lightLabel;
    }

    update();
}


void PCB_RENDER_SETTINGS::LoadDisplayOptions( const PCB_DISPLAY_OPTIONS* aOptions,
                                              bool aShowPageLimits )
{
    if( aOptions == NULL )
        return;

    m_hiContrastEnabled = aOptions->m_ContrastModeDisplay;
    m_padNumbers        = aOptions->m_DisplayPadNum;
    m_sketchBoardGfx    = !aOptions->m_DisplayDrawItemsFill;
    m_sketchFpGfx       = !aOptions->m_DisplayModEdgeFill;
    m_sketchFpTxtfx     = !aOptions->m_DisplayModTextFill;
    m_curvedRatsnestlines = aOptions->m_DisplayRatsnestLinesCurved;
    m_globalRatsnestlines = aOptions->m_ShowGlobalRatsnest;

    // Whether to draw tracks, vias & pads filled or as outlines
    m_sketchMode[LAYER_PADS_TH]      = !aOptions->m_DisplayPadFill;
    m_sketchMode[LAYER_VIA_THROUGH]  = !aOptions->m_DisplayViaFill;
    m_sketchMode[LAYER_VIA_BBLIND]   = !aOptions->m_DisplayViaFill;
    m_sketchMode[LAYER_VIA_MICROVIA] = !aOptions->m_DisplayViaFill;
    m_sketchMode[LAYER_TRACKS]       = !aOptions->m_DisplayPcbTrackFill;

    // Net names display settings
    switch( aOptions->m_DisplayNetNamesMode )
    {
    case 0:
        m_netNamesOnPads = false;
        m_netNamesOnTracks = false;
        break;

    case 1:
        m_netNamesOnPads = true;
        m_netNamesOnTracks = false;
        break;

    case 2:
        m_netNamesOnPads = false;
        m_netNamesOnTracks = true;
        break;

    case 3:
        m_netNamesOnPads = true;
        m_netNamesOnTracks = true;
        break;
    }

    // Zone display settings
    switch( aOptions->m_DisplayZonesMode )
    {
    case 0:
        m_displayZone = DZ_SHOW_FILLED;
        break;

    case 1:
        m_displayZone = DZ_HIDE_FILLED;
        break;

    case 2:
        m_displayZone = DZ_SHOW_OUTLINED;
        break;
    }

    // Clearance settings
    switch( aOptions->m_ShowTrackClearanceMode )
    {
        case PCB_DISPLAY_OPTIONS::DO_NOT_SHOW_CLEARANCE:
            m_clearance = CL_NONE;
            break;

        case PCB_DISPLAY_OPTIONS::SHOW_CLEARANCE_NEW_TRACKS:
            m_clearance = CL_NEW | CL_TRACKS;
            break;

        case PCB_DISPLAY_OPTIONS::SHOW_CLEARANCE_NEW_TRACKS_AND_VIA_AREAS:
            m_clearance = CL_NEW | CL_TRACKS | CL_VIAS;
            break;

        case PCB_DISPLAY_OPTIONS::SHOW_CLEARANCE_NEW_AND_EDITED_TRACKS_AND_VIA_AREAS:
            m_clearance = CL_NEW | CL_EDITED | CL_TRACKS | CL_VIAS;
            break;

        case PCB_DISPLAY_OPTIONS::SHOW_CLEARANCE_ALWAYS:
            m_clearance = CL_NEW | CL_EDITED | CL_EXISTING | CL_TRACKS | CL_VIAS;
            break;
    }

    if( aOptions->m_DisplayPadIsol )
        m_clearance |= CL_PADS;

    m_showPageLimits = aShowPageLimits;
}


const COLOR4D& PCB_RENDER_SETTINGS::GetColor( const VIEW_ITEM* aItem, int aLayer ) const
{
    int netCode = -1;
    const EDA_ITEM* item = dynamic_cast<const EDA_ITEM*>( aItem );

    if( item )
    {
        // Selection disambiguation
        if( item->IsBrightened() )
        {
            return m_selectionCandidateColor;
        }

        // Don't let pads that *should* be NPTHs get lost
        if( item->Type() == PCB_PAD_T && dyn_cast<const D_PAD*>( item )->PadShouldBeNPTH() )
            aLayer = LAYER_MOD_TEXT_INVISIBLE;

        if( item->IsSelected() )
        {
            return m_layerColorsSel[aLayer];
        }

        if( m_highlightEnabled && m_highlightItems )
        {
            if( item->IsHighlighted() )
                return m_layerColorsHi[aLayer];
            else
                return m_layerColorsDark[aLayer];
        }

        // Try to obtain the netcode for the item
        if( const BOARD_CONNECTED_ITEM* conItem = dyn_cast<const BOARD_CONNECTED_ITEM*> ( item ) )
            netCode = conItem->GetNetCode();

        if( item->Type() == PCB_MARKER_T )
            return m_layerColors[aLayer];
    }

    // Single net highlight mode
    if( m_highlightEnabled && netCode == m_highlightNetcode )
        return m_layerColorsHi[aLayer];

    // Return grayish color for non-highlighted layers in the high contrast mode
    if( m_hiContrastEnabled && m_activeLayers.count( aLayer ) == 0 )
        return m_hiContrastColor[aLayer];

    // Catch the case when highlight and high-contraste modes are enabled
    // and we are drawing a not highlighted track
    if( m_highlightEnabled )
        return m_layerColorsDark[aLayer];

    // No special modificators enabled
    return m_layerColors[aLayer];
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


int PCB_PAINTER::getDrillShape( const D_PAD* aPad ) const
{
    return aPad->GetDrillShape();
}


VECTOR2D PCB_PAINTER::getDrillSize( const D_PAD* aPad ) const
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

    // the "cast" applied in here clarifies which overloaded draw() is called
    switch( item->Type() )
    {
    case PCB_TRACE_T:
        draw( static_cast<const TRACK*>( item ), aLayer );
        break;

    case PCB_VIA_T:
        draw( static_cast<const VIA*>( item ), aLayer );
        break;

    case PCB_PAD_T:
        draw( static_cast<const D_PAD*>( item ), aLayer );
        break;

    case PCB_LINE_T:
    case PCB_MODULE_EDGE_T:
        draw( static_cast<const DRAWSEGMENT*>( item ), aLayer );
        break;

    case PCB_TEXT_T:
        draw( static_cast<const TEXTE_PCB*>( item ), aLayer );
        break;

    case PCB_MODULE_TEXT_T:
        draw( static_cast<const TEXTE_MODULE*>( item ), aLayer );
        break;

    case PCB_MODULE_T:
        draw( static_cast<const MODULE*>( item ), aLayer );
        break;

    case PCB_ZONE_AREA_T:
        draw( static_cast<const ZONE_CONTAINER*>( item ), aLayer );
        break;

    case PCB_DIMENSION_T:
        draw( static_cast<const DIMENSION*>( item ), aLayer );
        break;

    case PCB_TARGET_T:
        draw( static_cast<const PCB_TARGET*>( item ) );
        break;

    case PCB_MARKER_T:
        draw( static_cast<const MARKER_PCB*>( item ) );
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

    if( m_pcbSettings.m_netNamesOnTracks && IsNetnameLayer( aLayer ) )
    {
        // If there is a net name - display it on the track
        if( aTrack->GetNetCode() > NETINFO_LIST::UNCONNECTED )
        {
            VECTOR2D line = ( end - start );
            double length = line.EuclideanNorm();

            // Check if the track is long enough to have a netname displayed
            if( length < 10 * width )
                return;

            const wxString& netName = UnescapeString( aTrack->GetShortNetname() );
            VECTOR2D textPosition = start + line / 2.0;     // center of the track

            double textOrientation;

            if( end.y == start.y ) // horizontal
                textOrientation = 0;
            else if( end.x == start.x ) // vertical
                textOrientation = M_PI / 2;
            else
                textOrientation = -atan( line.y / line.x );

            double textSize = width;

            m_gal->SetIsStroke( true );
            m_gal->SetIsFill( false );
            m_gal->SetStrokeColor( m_pcbSettings.GetColor( NULL, aLayer ) );
            m_gal->SetLineWidth( width / 10.0 );
            m_gal->SetFontBold( false );
            m_gal->SetFontItalic( false );
            m_gal->SetTextMirrored( false );
            m_gal->SetGlyphSize( VECTOR2D( textSize * 0.7, textSize * 0.7 ) );
            m_gal->SetHorizontalJustify( GR_TEXT_HJUSTIFY_CENTER );
            m_gal->SetVerticalJustify( GR_TEXT_VJUSTIFY_CENTER );
            m_gal->BitmapText( netName, textPosition, textOrientation );
        }
    }
    else if( IsCopperLayer( aLayer ) )
    {
        // Draw a regular track
        const COLOR4D& color = m_pcbSettings.GetColor( aTrack, aLayer );
        bool outline_mode = m_pcbSettings.m_sketchMode[LAYER_TRACKS];
        m_gal->SetStrokeColor( color );
        m_gal->SetFillColor( color );
        m_gal->SetIsStroke( outline_mode );
        m_gal->SetIsFill( not outline_mode );
        m_gal->SetLineWidth( m_pcbSettings.m_outlineWidth );

        m_gal->DrawSegment( start, end, width );

        // Clearance lines
        constexpr int clearanceFlags = PCB_RENDER_SETTINGS::CL_EXISTING | PCB_RENDER_SETTINGS::CL_TRACKS;

        if( ( m_pcbSettings.m_clearance & clearanceFlags ) == clearanceFlags )
        {
            m_gal->SetLineWidth( m_pcbSettings.m_outlineWidth );
            m_gal->SetIsFill( false );
            m_gal->SetIsStroke( true );
            m_gal->SetStrokeColor( color );
            m_gal->DrawSegment( start, end, width + aTrack->GetClearance() * 2 );
        }
    }
}


void PCB_PAINTER::draw( const VIA* aVia, int aLayer )
{
    VECTOR2D center( aVia->GetStart() );
    double   radius = 0.0;

    // Draw description layer
    if( IsNetnameLayer( aLayer ) )
    {
        VECTOR2D position( center );

        // Is anything that we can display enabled?
        if( m_pcbSettings.m_netNamesOnVias )
        {
            bool displayNetname = ( !aVia->GetNetname().empty() );
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

            if( displayNetname )
            {
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
            }


            m_gal->Restore();
        }
        return;
    }

    // Choose drawing settings depending on if we are drawing via's pad or hole
    if( aLayer == LAYER_VIAS_HOLES )
        radius = getDrillSize( aVia ) / 2.0;
    else
        radius = aVia->GetWidth() / 2.0;

    bool sketchMode = false;
    const COLOR4D& color  = m_pcbSettings.GetColor( aVia, aLayer );

    switch( aVia->GetViaType() )
    {
    case VIA_THROUGH:
        sketchMode = m_pcbSettings.m_sketchMode[LAYER_VIA_THROUGH];
        break;

    case VIA_BLIND_BURIED:
        sketchMode = m_pcbSettings.m_sketchMode[LAYER_VIA_BBLIND];
        break;

    case VIA_MICROVIA:
        sketchMode = m_pcbSettings.m_sketchMode[LAYER_VIA_MICROVIA];
        break;

    default:
        wxASSERT( false );
        break;
    }

    if( aVia->GetViaType() == VIA_BLIND_BURIED )
    {
        // Buried vias are drawn in a special way to indicate the top and bottom layers
        PCB_LAYER_ID layerTop, layerBottom;
        aVia->LayerPair( &layerTop, &layerBottom );

        if( aLayer == LAYER_VIAS_HOLES )
        {                                                               // TODO outline mode
            m_gal->SetIsFill( true );
            m_gal->SetIsStroke( false );
            m_gal->SetFillColor( color );
            m_gal->DrawCircle( center, radius );
        }
        else
        {
            double width = ( aVia->GetWidth() - aVia->GetDrillValue() ) / 2.0;

            m_gal->SetLineWidth( width );
            m_gal->SetIsFill( true );
            m_gal->SetIsStroke( false );
            m_gal->SetFillColor( color );

            if( aLayer == layerTop )
            {
                m_gal->DrawArc( center, radius, 0.0, M_PI / 2.0 );
            }
            else if( aLayer == layerBottom )
            {
                m_gal->DrawArc( center, radius, M_PI, 3.0 * M_PI / 2.0 );
            }
            else if( aLayer == LAYER_VIA_BBLIND )
            {
                m_gal->DrawArc( center, radius, M_PI / 2.0, M_PI );
                m_gal->DrawArc( center, radius, 3.0 * M_PI / 2.0, 2.0 * M_PI );
            }
        }
    }
    else
    {
        // Regular vias
        m_gal->SetIsFill( !sketchMode );
        m_gal->SetIsStroke( sketchMode );

        if( sketchMode )
        {
            // Outline mode
            m_gal->SetLineWidth( m_pcbSettings.m_outlineWidth );
            m_gal->SetStrokeColor( color );
        }
        else
        {
            // Filled mode
            m_gal->SetFillColor( color );
        }

        m_gal->DrawCircle( center, radius );
    }

    // Clearance lines
    constexpr int clearanceFlags = PCB_RENDER_SETTINGS::CL_EXISTING | PCB_RENDER_SETTINGS::CL_VIAS;

    if( ( m_pcbSettings.m_clearance & clearanceFlags ) == clearanceFlags
            && aLayer != LAYER_VIAS_HOLES )
    {
        m_gal->SetLineWidth( m_pcbSettings.m_outlineWidth );
        m_gal->SetIsFill( false );
        m_gal->SetIsStroke( true );
        m_gal->SetStrokeColor( color );
        m_gal->DrawCircle( center, radius + aVia->GetClearance() );
    }
}


void PCB_PAINTER::draw( const D_PAD* aPad, int aLayer )
{
    double m, n;
    double orientation = aPad->GetOrientation();

    // Draw description layer
    if( IsNetnameLayer( aLayer ) )
    {
        VECTOR2D position( aPad->ShapePos() );

        // Is anything that we can display enabled?
        if( m_pcbSettings.m_netNamesOnPads || m_pcbSettings.m_padNumbers )
        {
            bool displayNetname = ( m_pcbSettings.m_netNamesOnPads && !aPad->GetNetname().empty() );
            VECTOR2D padsize = VECTOR2D( aPad->GetSize() );
            double maxSize = PCB_RENDER_SETTINGS::MAX_FONT_SIZE;
            double size = padsize.y;

            // Keep the size ratio for the font, but make it smaller
            if( padsize.x < padsize.y )
            {
                orientation += 900.0;
                size = padsize.x;
                std::swap( padsize.x, padsize.y );
            }
            else if( padsize.x == padsize.y )
            {
                // If the text is displayed on a symmetrical pad, do not rotate it
                orientation = 0.0;
            }

            // Font size limits
            if( size > maxSize )
                size = maxSize;

            m_gal->Save();
            m_gal->Translate( position );

            // do not display descriptions upside down
            NORMALIZE_ANGLE_90( orientation );
            m_gal->Rotate( DECIDEG2RAD( -orientation ) );

            // Default font settings
            m_gal->SetHorizontalJustify( GR_TEXT_HJUSTIFY_CENTER );
            m_gal->SetVerticalJustify( GR_TEXT_VJUSTIFY_CENTER );
            m_gal->SetFontBold( false );
            m_gal->SetFontItalic( false );
            m_gal->SetTextMirrored( false );
            m_gal->SetStrokeColor( m_pcbSettings.GetColor( NULL, aLayer ) );
            m_gal->SetIsStroke( true );
            m_gal->SetIsFill( false );

            // Set the text position to the pad shape position (the pad position is not the best place)
            VECTOR2D textpos( 0.0, 0.0 );

            // Divide the space, to display both pad numbers and netnames
            // and set the Y text position to display 2 lines
            if( displayNetname && m_pcbSettings.m_padNumbers )
            {
                size = size / 2.0;
                textpos.y = size / 2.0;
            }

            if( displayNetname )
            {
                wxString netname = UnescapeString( aPad->GetShortNetname() );
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

    // Pad drawing
    COLOR4D color;

    // Pad holes color is type specific
    if( aLayer == LAYER_PADS_PLATEDHOLES || aLayer == LAYER_NON_PLATEDHOLES )
    {
        // Hole color is the background color for plated holes, but a specific color
        // for not plated holes (LAYER_NON_PLATEDHOLES color layer )
        if( aPad->GetAttribute() == PAD_ATTRIB_HOLE_NOT_PLATED )
            color = m_pcbSettings.GetColor( nullptr, LAYER_NON_PLATEDHOLES );
        // Don't let pads that *should* be NPTH get lost
        else if( aPad->PadShouldBeNPTH() )
            color = m_pcbSettings.GetColor( aPad, aLayer );
        else
            color = m_pcbSettings.GetBackgroundColor();
    }
    else
    {
        color = m_pcbSettings.GetColor( aPad, aLayer );
    }

    VECTOR2D size;

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

    // Choose drawing settings depending on if we are drawing a pad itself or a hole
    if( aLayer == LAYER_PADS_PLATEDHOLES || aLayer == LAYER_NON_PLATEDHOLES )
    {
        m_gal->Save();
        m_gal->Translate( VECTOR2D( aPad->GetPosition() ) );
        m_gal->Rotate( -aPad->GetOrientationRadians() );

        // Drawing hole: has same shape as PAD_CIRCLE or PAD_OVAL
        size  = getDrillSize( aPad ) / 2.0;

        if( getDrillShape( aPad ) == PAD_DRILL_SHAPE_OBLONG )
        {
            if( size.y >= size.x )
            {
                m = ( size.y - size.x );
                n = size.x;

                m_gal->DrawArc( VECTOR2D( 0, -m ), n, -M_PI, 0 );
                m_gal->DrawArc( VECTOR2D( 0, m ),  n, M_PI, 0 );

                if( m_pcbSettings.m_sketchMode[LAYER_PADS_TH] )
                {
                    m_gal->DrawLine( VECTOR2D( -n, -m ), VECTOR2D( -n, m ) );
                    m_gal->DrawLine( VECTOR2D( n, -m ),  VECTOR2D( n, m ) );
                }
                else
                {
                    m_gal->DrawRectangle( VECTOR2D( -n, -m ), VECTOR2D( n, m ) );
                }
            }
            else
            {
                m = ( size.x - size.y );
                n = size.y;
                m_gal->DrawArc( VECTOR2D( -m, 0 ), n, M_PI / 2, 3 * M_PI / 2 );
                m_gal->DrawArc( VECTOR2D( m, 0 ),  n, M_PI / 2, -M_PI / 2 );

                if( m_pcbSettings.m_sketchMode[LAYER_PADS_TH] )
                {
                    m_gal->DrawLine( VECTOR2D( -m, -n ), VECTOR2D( m, -n ) );
                    m_gal->DrawLine( VECTOR2D( -m, n ),  VECTOR2D( m, n ) );
                }
                else
                {
                    m_gal->DrawRectangle( VECTOR2D( -m, -n ), VECTOR2D( m, n ) );
                }
            }
        }
        else
        {
            m_gal->DrawCircle( VECTOR2D( 0.0, 0.0 ), size.x );
        }

        m_gal->Restore();
    }
    else
    {
        SHAPE_POLY_SET polySet;
        wxSize margin;
        int clearance = 0;

        switch( aLayer )
        {
        case F_Mask:
        case B_Mask:
            clearance += aPad->GetSolderMaskMargin();
            break;

        case F_Paste:
        case B_Paste:
            margin = aPad->GetSolderPasteMargin();
            clearance += ( margin.x + margin.y ) / 2;
            break;

        default:
            break;
        }

        aPad->TransformShapeWithClearanceToPolygon( polySet, clearance );
        m_gal->DrawPolygon( polySet );
    }

    // Clearance lines
    constexpr int clearanceFlags = PCB_RENDER_SETTINGS::CL_PADS;

    if( ( m_pcbSettings.m_clearance & clearanceFlags ) == clearanceFlags
            && ( aLayer == LAYER_PAD_FR
                || aLayer == LAYER_PAD_BK
                || aLayer == LAYER_PADS_TH ) )
    {
        SHAPE_POLY_SET polySet;
        aPad->TransformShapeWithClearanceToPolygon( polySet, aPad->GetClearance() );
        m_gal->SetLineWidth( m_pcbSettings.m_outlineWidth );
        m_gal->SetIsStroke( true );
        m_gal->SetIsFill( false );
        m_gal->SetStrokeColor( color );
        m_gal->DrawPolygon( polySet );
    }
}


void PCB_PAINTER::draw( const DRAWSEGMENT* aSegment, int aLayer )
{
    const COLOR4D& color = m_pcbSettings.GetColor( aSegment, aSegment->GetLayer() );
    bool sketch = ( aSegment->Type() == PCB_LINE_T && m_pcbSettings.m_sketchBoardGfx )
        || ( aSegment->Type() == PCB_MODULE_EDGE_T && m_pcbSettings.m_sketchFpGfx );

    int thickness = getLineThickness( aSegment->GetWidth() );
    VECTOR2D start( aSegment->GetStart() );
    VECTOR2D end( aSegment->GetEnd() );

    m_gal->SetIsFill( !sketch );
    m_gal->SetIsStroke( sketch );
    m_gal->SetFillColor( color );
    m_gal->SetStrokeColor( color );
    m_gal->SetLineWidth( m_pcbSettings.m_outlineWidth );

    switch( aSegment->GetShape() )
    {
    case S_SEGMENT:
        m_gal->DrawSegment( start, end, thickness );
        break;

    case S_RECT:
        wxASSERT_MSG( false, "Not tested yet" );
        m_gal->DrawRectangle( start, end );
        break;

    case S_ARC:
        m_gal->DrawArcSegment( start, aSegment->GetRadius(),
                        DECIDEG2RAD( aSegment->GetArcAngleStart() ),
                        DECIDEG2RAD( aSegment->GetArcAngleStart() + aSegment->GetAngle() ),
                        thickness );
        break;

    case S_CIRCLE:
        if( sketch )
        {
            m_gal->DrawCircle( start, aSegment->GetRadius() - thickness / 2 );
            m_gal->DrawCircle( start, aSegment->GetRadius() + thickness / 2 );
        }
        else
        {
            m_gal->SetLineWidth( thickness );
            m_gal->SetIsFill( false );
            m_gal->SetIsStroke( true );
            m_gal->DrawCircle( start, aSegment->GetRadius() );
        }
        break;

    case S_POLYGON:
    {
        SHAPE_POLY_SET& shape = ((DRAWSEGMENT*)aSegment)->GetPolyShape();

        if( shape.OutlineCount() == 0 )
            break;

        // On Opengl, a not convex filled polygon is usually drawn by using triangles as primitives.
        // CacheTriangulation() can create basic triangle primitives to draw the polygon solid shape
        // on Opengl.
        // GLU tesselation is much slower, so currently we are using our tesselation.
        if( m_gal->IsOpenGlEngine() && !shape.IsTriangulationUpToDate() )
        {
            shape.CacheTriangulation();
        }

        m_gal->Save();

        if( MODULE* module = aSegment->GetParentModule() )
        {
            m_gal->Translate( module->GetPosition() );
            m_gal->Rotate( -module->GetOrientationRadians() );
        }

        m_gal->SetLineWidth( thickness );

        if( sketch )
            m_gal->SetIsFill( false );
        else
            m_gal->SetIsFill( aSegment->IsPolygonFilled() );

        m_gal->SetIsStroke( true );
        m_gal->DrawPolygon( shape );

        m_gal->Restore();
        break;
    }

    case S_CURVE:
        m_gal->SetIsFill( false );
        m_gal->SetIsStroke( true );
        m_gal->SetLineWidth( thickness );
        m_gal->DrawCurve( VECTOR2D( aSegment->GetStart() ),
                          VECTOR2D( aSegment->GetBezControl1() ),
                          VECTOR2D( aSegment->GetBezControl2() ),
                          VECTOR2D( aSegment->GetEnd() ) );
        break;

    case S_LAST:
        break;
    }
}


void PCB_PAINTER::draw( const TEXTE_PCB* aText, int aLayer )
{
    wxString shownText( aText->GetShownText() );
    if( shownText.Length() == 0 )
        return;

    const COLOR4D& color = m_pcbSettings.GetColor( aText, aText->GetLayer() );
    VECTOR2D position( aText->GetTextPos().x, aText->GetTextPos().y );

    if( m_pcbSettings.m_sketchMode[aLayer] )
    {
        // Outline mode
        m_gal->SetLineWidth( m_pcbSettings.m_outlineWidth );
    }
    else
    {
        // Filled mode
        m_gal->SetLineWidth( getLineThickness( aText->GetThickness() ) );
    }

    m_gal->SetStrokeColor( color );
    m_gal->SetIsFill( false );
    m_gal->SetIsStroke( true );
    m_gal->SetTextAttributes( aText );
    m_gal->StrokeText( shownText, position, aText->GetTextAngleRadians() );
}


void PCB_PAINTER::draw( const TEXTE_MODULE* aText, int aLayer )
{
    wxString shownText( aText->GetShownText() );
    if( shownText.Length() == 0 )
        return;

    bool sketch = m_pcbSettings.m_sketchFpTxtfx;

    const COLOR4D& color = m_pcbSettings.GetColor( aText, aLayer );
    VECTOR2D position( aText->GetTextPos().x, aText->GetTextPos().y );

    // Currently, draw text routines do not know the true outline mode.
    // so draw the text in "line" mode (no thickness)
    if( sketch )
    {
        // Outline mode
        m_gal->SetLineWidth( m_pcbSettings.m_outlineWidth );
    }
    else
    {
        // Filled mode
        m_gal->SetLineWidth( getLineThickness( aText->GetThickness() ) );
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
        m_gal->SetStrokeColor( COLOR4D( 0.0, 0.0, 1.0, 1.0 ) );
        m_gal->DrawLine( position, aText->GetParent()->GetPosition() );
    }
}


void PCB_PAINTER::draw( const MODULE* aModule, int aLayer )
{
    if( aLayer == LAYER_ANCHOR )
    {
        const COLOR4D color = m_pcbSettings.GetColor( aModule, LAYER_ANCHOR );

        // Draw anchor
        m_gal->SetStrokeColor( color );
        m_gal->SetLineWidth( m_pcbSettings.m_outlineWidth );

        // Keep the size constant, not related to the scale
        double anchorSize = 5.0 / m_gal->GetWorldScale();

        VECTOR2D center = aModule->GetPosition();
        m_gal->DrawLine( center - VECTOR2D( anchorSize, 0 ), center + VECTOR2D( anchorSize, 0 ) );
        m_gal->DrawLine( center - VECTOR2D( 0, anchorSize ), center + VECTOR2D( 0, anchorSize ) );
    }
}


void PCB_PAINTER::draw( const ZONE_CONTAINER* aZone, int aLayer )
{
    if( !aZone->IsOnLayer( (PCB_LAYER_ID) aLayer ) )
        return;

    const COLOR4D& color = m_pcbSettings.GetColor( aZone, aLayer );
    std::deque<VECTOR2D> corners;
    PCB_RENDER_SETTINGS::DISPLAY_ZONE_MODE displayMode = m_pcbSettings.m_displayZone;

    // Draw the outline
    const SHAPE_POLY_SET* outline = aZone->Outline();

    if( m_pcbSettings.m_zoneOutlines && outline )
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
    if( displayMode != PCB_RENDER_SETTINGS::DZ_HIDE_FILLED )
    {
        const SHAPE_POLY_SET& polySet = aZone->GetFilledPolysList();

        if( polySet.OutlineCount() == 0 )  // Nothing to draw
            return;

        // Set up drawing options
        int outline_thickness = aZone->GetFilledPolysUseThickness() ? aZone->GetMinThickness() : 0;
        m_gal->SetStrokeColor( color );
        m_gal->SetFillColor( color );
        m_gal->SetLineWidth( outline_thickness );

        if( displayMode == PCB_RENDER_SETTINGS::DZ_SHOW_FILLED )
        {
            m_gal->SetIsFill( true );
            m_gal->SetIsStroke( outline_thickness > 0 );
        }
        else if( displayMode == PCB_RENDER_SETTINGS::DZ_SHOW_OUTLINED )
        {
            m_gal->SetIsFill( false );
            m_gal->SetIsStroke( true );
        }

        m_gal->DrawPolygon( polySet );
    }

}


void PCB_PAINTER::draw( const DIMENSION* aDimension, int aLayer )
{
    const COLOR4D& strokeColor = m_pcbSettings.GetColor( aDimension, aLayer );

    m_gal->SetStrokeColor( strokeColor );
    m_gal->SetIsFill( false );
    m_gal->SetIsStroke( true );
    m_gal->SetLineWidth( getLineThickness( aDimension->GetWidth() ) );

    // Draw an arrow
    m_gal->DrawLine( VECTOR2D( aDimension->m_crossBarO ), VECTOR2D( aDimension->m_crossBarF ) );
    m_gal->DrawLine( VECTOR2D( aDimension->m_featureLineGO ),
                     VECTOR2D( aDimension->m_featureLineGF ) );
    m_gal->DrawLine( VECTOR2D( aDimension->m_featureLineDO ),
                     VECTOR2D( aDimension->m_featureLineDF ) );
    m_gal->DrawLine( VECTOR2D( aDimension->m_crossBarF ), VECTOR2D( aDimension->m_arrowD1F ) );
    m_gal->DrawLine( VECTOR2D( aDimension->m_crossBarF ), VECTOR2D( aDimension->m_arrowD2F ) );
    m_gal->DrawLine( VECTOR2D( aDimension->m_crossBarO ), VECTOR2D( aDimension->m_arrowG1F ) );
    m_gal->DrawLine( VECTOR2D( aDimension->m_crossBarO ), VECTOR2D( aDimension->m_arrowG2F ) );

    // Draw text
    TEXTE_PCB& text = aDimension->Text();
    VECTOR2D position( text.GetTextPos().x, text.GetTextPos().y );

    m_gal->SetLineWidth( text.GetThickness() );
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


void PCB_PAINTER::draw( const MARKER_PCB* aMarker )
{
    SHAPE_LINE_CHAIN polygon;
    aMarker->ShapeToPolygon( polygon );

    auto strokeColor = m_pcbSettings.GetColor( aMarker, LAYER_DRC );

    m_gal->Save();
    m_gal->Translate( aMarker->GetPosition() );
    m_gal->SetFillColor( strokeColor );
    m_gal->SetIsFill( true );
    m_gal->SetIsStroke( false );
    m_gal->DrawPolygon( polygon );
    m_gal->Restore();
}


const double PCB_RENDER_SETTINGS::MAX_FONT_SIZE = Millimeter2iu( 10.0 );
