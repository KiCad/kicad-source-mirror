/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2013 CERN
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
#include <class_colors_design_settings.h>
#include <class_marker_pcb.h>
#include <class_dimension.h>
#include <class_mire.h>
#include <class_marker_pcb.h>

#include <pcb_painter.h>
#include <gal/graphics_abstraction_layer.h>

using namespace KIGFX;

PCB_RENDER_SETTINGS::PCB_RENDER_SETTINGS()
{
    m_backgroundColor = COLOR4D( 0.0, 0.0, 0.0, 1.0 );
    m_padNumbers = true;
    m_netNamesOnPads = true;
    m_netNamesOnTracks = true;
    m_displayZoneMode = DZ_SHOW_FILLED;

    // By default everything should be displayed as filled
    for( unsigned int i = 0; i < TOTAL_LAYER_COUNT; ++i )
    {
        m_sketchMode[i] = false;
    }

    update();
}


void PCB_RENDER_SETTINGS::ImportLegacyColors( const COLORS_DESIGN_SETTINGS* aSettings )
{
    for( int i = 0; i < LAYER_ID_COUNT; i++ )
    {
        m_layerColors[i] = m_legacyColorMap[aSettings->GetLayerColor( i )];
    }

    for( int i = 0; i < END_PCB_VISIBLE_LIST; i++ )
    {
        m_layerColors[ITEM_GAL_LAYER( i )] = m_legacyColorMap[aSettings->GetItemColor( i )];
    }

    // Default colors for specific layers
    m_layerColors[ITEM_GAL_LAYER( VIAS_HOLES_VISIBLE )]             = COLOR4D( 0.5, 0.4, 0.0, 0.8 );
    m_layerColors[ITEM_GAL_LAYER( PADS_HOLES_VISIBLE )]             = COLOR4D( 0.0, 0.5, 0.5, 0.8 );
    m_layerColors[ITEM_GAL_LAYER( VIA_THROUGH_VISIBLE )]            = COLOR4D( 0.6, 0.6, 0.6, 0.8 );
    m_layerColors[ITEM_GAL_LAYER( VIA_BBLIND_VISIBLE )]             = COLOR4D( 0.6, 0.6, 0.6, 0.8 );
    m_layerColors[ITEM_GAL_LAYER( VIA_MICROVIA_VISIBLE )]           = COLOR4D( 0.4, 0.4, 0.8, 0.8 );
    m_layerColors[ITEM_GAL_LAYER( PADS_VISIBLE )]                   = COLOR4D( 0.6, 0.6, 0.6, 0.8 );
    m_layerColors[NETNAMES_GAL_LAYER( PADS_NETNAMES_VISIBLE )]      = COLOR4D( 1.0, 1.0, 1.0, 0.9 );
    m_layerColors[NETNAMES_GAL_LAYER( PAD_FR_NETNAMES_VISIBLE )]    = COLOR4D( 1.0, 1.0, 1.0, 0.9 );
    m_layerColors[NETNAMES_GAL_LAYER( PAD_BK_NETNAMES_VISIBLE )]    = COLOR4D( 1.0, 1.0, 1.0, 0.9 );
    m_layerColors[ITEM_GAL_LAYER( ANCHOR_VISIBLE )]                 = COLOR4D( 0.3, 0.3, 1.0, 0.9 );
    m_layerColors[ITEM_GAL_LAYER( RATSNEST_VISIBLE )]               = COLOR4D( 0.4, 0.4, 0.4, 0.8 );
    m_layerColors[ITEM_GAL_LAYER( WORKSHEET )]                      = COLOR4D( 0.5, 0.0, 0.0, 0.8 );
    m_layerColors[ITEM_GAL_LAYER( DRC_VISIBLE )]                    = COLOR4D( 1.0, 0.0, 0.0, 0.8 );

    // Netnames for copper layers
    for( LSEQ cu = LSET::AllCuMask().CuStack();  cu;  ++cu )
    {
        LAYER_ID layer = *cu;

        m_layerColors[GetNetnameLayer( layer )] = COLOR4D( 0.8, 0.8, 0.8, 0.7 );
    }

    update();
}


void PCB_RENDER_SETTINGS::LoadDisplayOptions( const DISPLAY_OPTIONS& aOptions )
{
    m_hiContrastEnabled = aOptions.ContrastModeDisplay;
    m_padNumbers        = aOptions.DisplayPadNum;

    // Whether to draw tracks, vias & pads filled or as outlines
    m_sketchMode[ITEM_GAL_LAYER( PADS_VISIBLE )]         = !aOptions.DisplayPadFill;
    m_sketchMode[ITEM_GAL_LAYER( VIA_THROUGH_VISIBLE )]  = !aOptions.DisplayViaFill;
    m_sketchMode[ITEM_GAL_LAYER( VIA_BBLIND_VISIBLE )]   = !aOptions.DisplayViaFill;
    m_sketchMode[ITEM_GAL_LAYER( VIA_MICROVIA_VISIBLE )] = !aOptions.DisplayViaFill;
    m_sketchMode[ITEM_GAL_LAYER( TRACKS_VISIBLE )]       = !aOptions.DisplayPcbTrackFill;

    switch( aOptions.DisplayNetNamesMode )
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

    switch( aOptions.DisplayZonesMode )
    {
    case 0:
        m_displayZoneMode = DZ_SHOW_FILLED;
        break;

    case 1:
        m_displayZoneMode = DZ_HIDE_FILLED;
        break;

    case 2:
        m_displayZoneMode = DZ_SHOW_OUTLINED;
        break;
    }
}


const COLOR4D& PCB_RENDER_SETTINGS::GetColor( const VIEW_ITEM* aItem, int aLayer ) const
{
    int netCode = -1;
    const EDA_ITEM* item = static_cast<const EDA_ITEM*>( aItem );

    if( item )
    {
        if( item->IsSelected() )
        {
            return m_layerColorsSel[aLayer];
        }

        // Try to obtain the netcode for the item
        if( const BOARD_CONNECTED_ITEM* conItem = dyn_cast<const BOARD_CONNECTED_ITEM*> ( item ) )
            netCode = conItem->GetNetCode();
    }

    // Return grayish color for non-highlighted layers in the high contrast mode
    if( m_hiContrastEnabled && m_activeLayers.count( aLayer ) == 0 )
        return m_hiContrastColor;

    // Single net highlight mode
    if( m_highlightEnabled )
    {
        if( netCode == m_highlightNetcode )
            return m_layerColorsHi[aLayer];
        else
            return m_layerColorsDark[aLayer];
    }

    // No special modificators enabled
    return m_layerColors[aLayer];
}


void PCB_RENDER_SETTINGS::update()
{
    RENDER_SETTINGS::update();

    // Calculate darkened/highlighted variants of layer colors
    for( int i = 0; i < TOTAL_LAYER_COUNT; i++ )
    {
        m_layerColorsHi[i]   = m_layerColors[i].Brightened( m_highlightFactor );
        m_layerColorsDark[i] = m_layerColors[i].Darkened( 1.0 - m_highlightFactor );
        m_layerColorsSel[i]  = m_layerColors[i].Brightened( m_selectFactor );
    }
}


PCB_PAINTER::PCB_PAINTER( GAL* aGal ) :
    PAINTER( aGal )
{
}


bool PCB_PAINTER::Draw( const VIEW_ITEM* aItem, int aLayer )
{
    const EDA_ITEM* item = static_cast<const EDA_ITEM*>( aItem );

    // the "cast" applied in here clarifies which overloaded draw() is called
    switch( item->Type() )
    {
    case PCB_ZONE_T:
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
        draw( static_cast<const ZONE_CONTAINER*>( item ) );
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

            const wxString& netName = aTrack->GetShortNetname();
            VECTOR2D textPosition = start + line / 2.0;     // center of the track
            double textOrientation = -atan( line.y / line.x );
            double textSize = std::min( static_cast<double>( width ), length / netName.length() );

            // Set a proper color for the label
            const COLOR4D& color = m_pcbSettings.GetColor( aTrack, aTrack->GetLayer() );
            const COLOR4D labelColor = m_pcbSettings.GetColor( NULL, aLayer );

            if( color.GetBrightness() > 0.5 )
                m_gal->SetStrokeColor( labelColor.Inverted() );
            else
                m_gal->SetStrokeColor( labelColor );

            m_gal->SetLineWidth( width / 10.0 );
            m_gal->SetBold( false );
            m_gal->SetItalic( false );
            m_gal->SetMirrored( false );
            m_gal->SetGlyphSize( VECTOR2D( textSize * 0.7, textSize * 0.7 ) );
            m_gal->SetHorizontalJustify( GR_TEXT_HJUSTIFY_CENTER );
            m_gal->SetVerticalJustify( GR_TEXT_VJUSTIFY_CENTER );
            m_gal->StrokeText( netName, textPosition, textOrientation );
        }
    }
    else if( IsCopperLayer( aLayer ) )
    {
        // Draw a regular track
        const COLOR4D& color = m_pcbSettings.GetColor( aTrack, aLayer );
        m_gal->SetStrokeColor( color );
        m_gal->SetIsStroke( true );

        if( m_pcbSettings.m_sketchMode[ITEM_GAL_LAYER( TRACKS_VISIBLE )] )
        {
            // Outline mode
            m_gal->SetLineWidth( m_pcbSettings.m_outlineWidth );
            m_gal->SetIsFill( false );
        }
        else
        {
            // Filled mode
            m_gal->SetFillColor( color );
            m_gal->SetIsFill( true );
        }

        m_gal->DrawSegment( start, end, width );
    }
}


void PCB_PAINTER::draw( const VIA* aVia, int aLayer )
{
    VECTOR2D center( aVia->GetStart() );
    double   radius = 0.0;

    // Only draw the via if at least one of the layers it crosses is being displayed
    BOARD*  brd =  aVia->GetBoard( );
    if( !( brd->GetVisibleLayers() & aVia->GetLayerSet() ).any() )
        return;

    // Choose drawing settings depending on if we are drawing via's pad or hole
    if( aLayer == ITEM_GAL_LAYER( VIAS_HOLES_VISIBLE ) )
        radius = aVia->GetDrillValue() / 2.0;
    else
        radius = aVia->GetWidth() / 2.0;

    bool sketchMode = false;
    const COLOR4D& color  = m_pcbSettings.GetColor( aVia, aLayer );

    switch( aVia->GetViaType() )
    {
    case VIA_THROUGH:
        sketchMode = m_pcbSettings.m_sketchMode[ITEM_GAL_LAYER( VIA_THROUGH_VISIBLE )];
        break;

    case VIA_BLIND_BURIED:
        sketchMode = m_pcbSettings.m_sketchMode[ITEM_GAL_LAYER( VIA_BBLIND_VISIBLE )];
        break;

    case VIA_MICROVIA:
        sketchMode = m_pcbSettings.m_sketchMode[ITEM_GAL_LAYER( VIA_MICROVIA_VISIBLE )];
        break;

    default:
        assert( false );
        break;
    }

    if( aVia->GetViaType() == VIA_BLIND_BURIED )
    {
        LAYER_ID layerTop, layerBottom;
        aVia->LayerPair( &layerTop, &layerBottom );

        if( aLayer == ITEM_GAL_LAYER( VIAS_HOLES_VISIBLE ) )
        {                                                               // TODO outline mode
            m_gal->SetIsFill( true );
            m_gal->SetIsStroke( false );
            m_gal->SetFillColor( color );
            m_gal->DrawCircle( center, radius );
        }
        else
        {
            double width = ( aVia->GetWidth() - aVia->GetDrillValue() ) / 2.0;
            radius -= width / 2.0;

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
            else if( aLayer == ITEM_GAL_LAYER( VIA_BBLIND_VISIBLE ) )
            {
                m_gal->DrawArc( center, radius, M_PI / 2.0, M_PI );
                m_gal->DrawArc( center, radius, 3.0 * M_PI / 2.0, 2.0 * M_PI );
            }
        }
    }
    else
    {
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
}


void PCB_PAINTER::draw( const D_PAD* aPad, int aLayer )
{
    VECTOR2D    size;
    VECTOR2D    position( aPad->GetPosition() );
    PAD_SHAPE_T shape;
    double      m, n;
    double      orientation = aPad->GetOrientation();
    wxString buffer;

    // Draw description layer
    if( IsNetnameLayer( aLayer ) )
    {
        // Is anything that we can display enabled?
        if( m_pcbSettings.m_netNamesOnPads || m_pcbSettings.m_padNumbers )
        {
            // Min char count to calculate string size
            const int MIN_CHAR_COUNT = 3;

            bool displayNetname = ( m_pcbSettings.m_netNamesOnPads &&
                                    !aPad->GetNetname().empty() );
            VECTOR2D padsize = VECTOR2D( aPad->GetSize() );
            double maxSize = PCB_RENDER_SETTINGS::MAX_FONT_SIZE;
            double size = padsize.y;

            // Keep the size ratio for the font, but make it smaller
            if( padsize.x < padsize.y )
            {
                orientation += 900.0;
                size = padsize.x;
                EXCHG( padsize.x, padsize.y );
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
            m_gal->Rotate( -orientation * M_PI / 1800.0 );

            // Default font settings
            m_gal->SetHorizontalJustify( GR_TEXT_HJUSTIFY_CENTER );
            m_gal->SetVerticalJustify( GR_TEXT_VJUSTIFY_CENTER );
            m_gal->SetBold( false );
            m_gal->SetItalic( false );
            m_gal->SetMirrored( false );

            // Set a proper color for the label
            const COLOR4D& color  = m_pcbSettings.GetColor( aPad, aPad->GetLayer() );
            const COLOR4D labelColor = m_pcbSettings.GetColor( NULL, aLayer );

            if( color.GetBrightness() > 0.5 )
                m_gal->SetStrokeColor( labelColor.Inverted() );
            else
                m_gal->SetStrokeColor( labelColor );

            VECTOR2D textpos( 0.0, 0.0);

            // Divide the space, to display both pad numbers and netnames
            // and set the Y text position to display 2 lines
            if( displayNetname && m_pcbSettings.m_padNumbers )
            {
                size = size / 2.0;
                textpos.y = size / 2.0;
            }

            if( displayNetname )
            {
                // calculate the size of net name text:
                double tsize = padsize.x / aPad->GetShortNetname().Length();
                tsize = std::min( tsize, size );
                // Use a smaller text size to handle interline, pen size..
                tsize *= 0.7;
                VECTOR2D namesize( tsize, tsize );
                m_gal->SetGlyphSize( namesize );
                m_gal->SetLineWidth( namesize.x / 12.0 );
                m_gal->StrokeText( aPad->GetShortNetname(), textpos, 0.0 );
            }

            if( m_pcbSettings.m_padNumbers )
            {
                textpos.y = -textpos.y;
                aPad->StringPadName( buffer );
                int len = buffer.Length();
                double tsize = padsize.x / std::max( len, MIN_CHAR_COUNT );
                tsize = std::min( tsize, size );
                // Use a smaller text size to handle interline, pen size..
                tsize *= 0.7;
                tsize = std::min( tsize, size );
                VECTOR2D numsize( tsize, tsize );

                m_gal->SetGlyphSize( numsize );
                m_gal->SetLineWidth( numsize.x / 12.0 );
                m_gal->StrokeText( aPad->GetPadName(), textpos, 0.0 );
            }

            m_gal->Restore();
        }
        return;
    }

    // Pad drawing
    const COLOR4D& color = m_pcbSettings.GetColor( aPad, aLayer );
    if( m_pcbSettings.m_sketchMode[ITEM_GAL_LAYER( PADS_VISIBLE )] )
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

    m_gal->Save();
    m_gal->Translate( VECTOR2D( aPad->GetPosition() ) );
    m_gal->Rotate( -aPad->GetOrientation() * M_PI / 1800.0 );

    // Choose drawing settings depending on if we are drawing a pad itself or a hole
    if( aLayer == ITEM_GAL_LAYER( PADS_HOLES_VISIBLE ) )
    {
        // Drawing hole: has same shape as PAD_CIRCLE or PAD_OVAL
        size  = VECTOR2D( aPad->GetDrillSize() ) / 2.0;
        shape = aPad->GetDrillShape() == PAD_DRILL_OBLONG ? PAD_OVAL : PAD_CIRCLE;
    }
    else if( aLayer == F_Mask || aLayer == B_Mask )
    {
        // Drawing soldermask
        int soldermaskMargin = aPad->GetSolderMaskMargin();

        m_gal->Translate( VECTOR2D( aPad->GetOffset() ) );
        size  = VECTOR2D( aPad->GetSize().x / 2.0 + soldermaskMargin,
                          aPad->GetSize().y / 2.0 + soldermaskMargin );
        shape = aPad->GetShape();
    }
    else if( aLayer == F_Paste || aLayer == B_Paste )
    {
        // Drawing solderpaste
        wxSize solderpasteMargin = aPad->GetSolderPasteMargin();

        m_gal->Translate( VECTOR2D( aPad->GetOffset() ) );
        size  = VECTOR2D( aPad->GetSize().x / 2.0 + solderpasteMargin.x,
                          aPad->GetSize().y / 2.0 + solderpasteMargin.y );
        shape = aPad->GetShape();
    }
    else
    {
        // Drawing every kind of pad
        m_gal->Translate( VECTOR2D( aPad->GetOffset() ) );
        size  = VECTOR2D( aPad->GetSize() ) / 2.0;
        shape = aPad->GetShape();
    }

    switch( shape )
    {
    case PAD_OVAL:
        if( size.y >= size.x )
        {
            m = ( size.y - size.x );
            n = size.x;

            if( m_pcbSettings.m_sketchMode[ITEM_GAL_LAYER( PADS_VISIBLE )] )
            {
                // Outline mode
                m_gal->DrawArc( VECTOR2D( 0, -m ), n, -M_PI, 0 );
                m_gal->DrawArc( VECTOR2D( 0, m ),  n, M_PI, 0 );
                m_gal->DrawLine( VECTOR2D( -n, -m ), VECTOR2D( -n, m ) );
                m_gal->DrawLine( VECTOR2D( n, -m ),  VECTOR2D( n, m ) );
            }
            else
            {
                // Filled mode
                m_gal->DrawCircle( VECTOR2D( 0, -m ), n );
                m_gal->DrawCircle( VECTOR2D( 0, m ),  n );
                m_gal->DrawRectangle( VECTOR2D( -n, -m ), VECTOR2D( n, m ) );
            }
        }
        else
        {
            m = ( size.x - size.y );
            n = size.y;

            if( m_pcbSettings.m_sketchMode[ITEM_GAL_LAYER( PADS_VISIBLE )] )
            {
                // Outline mode
                m_gal->DrawArc( VECTOR2D( -m, 0 ), n, M_PI / 2, 3 * M_PI / 2 );
                m_gal->DrawArc( VECTOR2D( m, 0 ),  n, M_PI / 2, -M_PI / 2 );
                m_gal->DrawLine( VECTOR2D( -m, -n ), VECTOR2D( m, -n ) );
                m_gal->DrawLine( VECTOR2D( -m, n ),  VECTOR2D( m, n ) );
            }
            else
            {
                // Filled mode
                m_gal->DrawCircle( VECTOR2D( -m, 0 ), n );
                m_gal->DrawCircle( VECTOR2D( m, 0 ),  n );
                m_gal->DrawRectangle( VECTOR2D( -m, -n ), VECTOR2D( m, n ) );
            }
        }
        break;

    case PAD_RECT:
        m_gal->DrawRectangle( VECTOR2D( -size.x, -size.y ), VECTOR2D( size.x, size.y ) );
        break;

    case PAD_TRAPEZOID:
    {
        std::deque<VECTOR2D> pointList;
        wxPoint corners[4];

        VECTOR2D padSize = VECTOR2D( aPad->GetSize().x, aPad->GetSize().y ) / 2;
        VECTOR2D deltaPadSize = size - padSize; // = solder[Paste/Mask]Margin or 0

        aPad->BuildPadPolygon( corners, wxSize( deltaPadSize.x, deltaPadSize.y ), 0.0 );
        pointList.push_back( VECTOR2D( corners[0] ) );
        pointList.push_back( VECTOR2D( corners[1] ) );
        pointList.push_back( VECTOR2D( corners[2] ) );
        pointList.push_back( VECTOR2D( corners[3] ) );

        if( m_pcbSettings.m_sketchMode[PADS_VISIBLE] )
        {
            // Add the beginning point to close the outline
            pointList.push_back( pointList.front() );
            m_gal->DrawPolyline( pointList );
        }
        else
        {
            m_gal->DrawPolygon( pointList );
        }
    }
    break;

    case PAD_CIRCLE:
        m_gal->DrawCircle( VECTOR2D( 0.0, 0.0 ), size.x );
        break;
    }

    m_gal->Restore();
}


void PCB_PAINTER::draw( const DRAWSEGMENT* aSegment, int aLayer )
{
    const COLOR4D& color = m_pcbSettings.GetColor( aSegment, aSegment->GetLayer() );

    m_gal->SetIsFill( false );
    m_gal->SetIsStroke( true );
    m_gal->SetStrokeColor( color );

    if( m_pcbSettings.m_sketchMode[aLayer] )
        m_gal->SetLineWidth( m_pcbSettings.m_outlineWidth );    // Outline mode
    else
        m_gal->SetLineWidth( aSegment->GetWidth() );            // Filled mode

    switch( aSegment->GetShape() )
    {
    case S_SEGMENT:
        m_gal->DrawLine( VECTOR2D( aSegment->GetStart() ), VECTOR2D( aSegment->GetEnd() ) );
        break;

    case S_RECT:
        wxASSERT_MSG( false, wxT( "Not tested yet" ) );
        m_gal->DrawRectangle( VECTOR2D( aSegment->GetStart() ), VECTOR2D( aSegment->GetEnd() ) );
        break;

    case S_ARC:
        m_gal->DrawArc( VECTOR2D( aSegment->GetCenter() ), aSegment->GetRadius(),
                        aSegment->GetArcAngleStart() * M_PI / 1800.0,
                        ( aSegment->GetArcAngleStart() + aSegment->GetAngle() ) * M_PI / 1800.0 );
        break;

    case S_CIRCLE:
        m_gal->DrawCircle( VECTOR2D( aSegment->GetCenter() ), aSegment->GetRadius() );
        break;

    case S_POLYGON:
    {
        std::deque<VECTOR2D> pointsList;

        m_gal->SetIsFill( true );
        m_gal->SetIsStroke( false );
        m_gal->SetFillColor( color );

        m_gal->Save();

        MODULE* module = aSegment->GetParentModule();
        if( module )
        {
            m_gal->Translate( module->GetPosition() );
            m_gal->Rotate( -module->GetOrientation() * M_PI / 1800.0 );
        }
        else
        {
            // not tested
            m_gal->Translate( aSegment->GetPosition() );
            m_gal->Rotate( -aSegment->GetAngle() * M_PI / 1800.0 );
        }

        std::copy( aSegment->GetPolyPoints().begin(), aSegment->GetPolyPoints().end(),
                   std::back_inserter( pointsList ) );

        m_gal->SetLineWidth( aSegment->GetWidth() );
        m_gal->DrawPolyline( pointsList );
        m_gal->DrawPolygon( pointsList );

        m_gal->Restore();
        break;
    }

    case S_CURVE:
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
    VECTOR2D position( aText->GetTextPosition().x, aText->GetTextPosition().y );
    double   orientation = aText->GetOrientation() * M_PI / 1800.0;

    if( m_pcbSettings.m_sketchMode[aLayer] )
    {
        // Outline mode
        m_gal->SetLineWidth( m_pcbSettings.m_outlineWidth );
    }
    else
    {
        // Filled mode
        m_gal->SetLineWidth( aText->GetThickness() );
    }

    m_gal->SetIsFill( false );
    m_gal->SetIsStroke( true );
    m_gal->SetStrokeColor( color );
    m_gal->SetTextAttributes( aText );
    m_gal->StrokeText( shownText, position, orientation );
}


void PCB_PAINTER::draw( const TEXTE_MODULE* aText, int aLayer )
{
    wxString shownText( aText->GetShownText() );
    if( shownText.Length() == 0 )
        return;

    const COLOR4D& color = m_pcbSettings.GetColor( aText, aLayer );
    VECTOR2D position( aText->GetTextPosition().x, aText->GetTextPosition().y );
    double   orientation = aText->GetDrawRotation() * M_PI / 1800.0;

    if( m_pcbSettings.m_sketchMode[aLayer] )
    {
        // Outline mode
        m_gal->SetLineWidth( m_pcbSettings.m_outlineWidth );
    }
    else
    {
        // Filled mode
        m_gal->SetLineWidth( aText->GetThickness() );
    }

    m_gal->SetIsFill( false );
    m_gal->SetIsStroke( true );
    m_gal->SetStrokeColor( color );
    m_gal->SetTextAttributes( aText );
    m_gal->StrokeText( shownText, position, orientation );
}


void PCB_PAINTER::draw( const MODULE* aModule, int aLayer )
{
    if( aLayer == ITEM_GAL_LAYER( ANCHOR_VISIBLE ) )
    {
        const COLOR4D color = m_pcbSettings.GetColor( aModule, ITEM_GAL_LAYER( ANCHOR_VISIBLE ) );

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


void PCB_PAINTER::draw( const ZONE_CONTAINER* aZone )
{
    const COLOR4D& color = m_pcbSettings.GetColor( aZone, aZone->GetLayer() );
    std::deque<VECTOR2D> corners;
    PCB_RENDER_SETTINGS::DisplayZonesMode displayMode = m_pcbSettings.m_displayZoneMode;

    // Draw the outline
    m_gal->SetStrokeColor( color );
    m_gal->SetIsFill( false );
    m_gal->SetIsStroke( true );
    m_gal->SetLineWidth( m_pcbSettings.m_outlineWidth );

    const CPolyLine* outline = aZone->Outline();
    for( int i = 0; i < outline->GetCornersCount(); ++i )
    {
        corners.push_back( VECTOR2D( outline->GetPos( i ) ) );

        if( outline->IsEndContour( i ) )
        {
            // The last point for closing the polyline
            corners.push_back( corners[0] );
            m_gal->DrawPolyline( corners );
            corners.clear();
        }
    }

    // Draw the filling
    if( displayMode != PCB_RENDER_SETTINGS::DZ_HIDE_FILLED )
    {
        const std::vector<CPolyPt> polyPoints = aZone->GetFilledPolysList().GetList();
        if( polyPoints.size() == 0 )  // Nothing to draw
            return;

        // Set up drawing options
        m_gal->SetFillColor( color );
        m_gal->SetLineWidth( aZone->GetMinThickness() );

        if( displayMode == PCB_RENDER_SETTINGS::DZ_SHOW_FILLED )
        {
            m_gal->SetIsFill( true );
            m_gal->SetIsStroke( true );
        }
        else if( displayMode == PCB_RENDER_SETTINGS::DZ_SHOW_OUTLINED )
        {
            m_gal->SetIsFill( false );
            m_gal->SetIsStroke( true );
        }

        std::vector<CPolyPt>::const_iterator polyIterator;
        for( polyIterator = polyPoints.begin(); polyIterator != polyPoints.end(); polyIterator++ )
        {
            // Find out all of polygons and then draw them
            corners.push_back( VECTOR2D( *polyIterator ) );

            if( polyIterator->end_contour )
            {
                if( displayMode == PCB_RENDER_SETTINGS::DZ_SHOW_FILLED )
                {
                    m_gal->DrawPolygon( corners );
                    m_gal->DrawPolyline( corners );
                }
                else if( displayMode == PCB_RENDER_SETTINGS::DZ_SHOW_OUTLINED )
                {
                    m_gal->DrawPolyline( corners );
                }

                corners.clear();
            }
        }
    }
}


void PCB_PAINTER::draw( const DIMENSION* aDimension, int aLayer )
{
    const COLOR4D& strokeColor = m_pcbSettings.GetColor( aDimension, aLayer );

    m_gal->SetStrokeColor( strokeColor );
    m_gal->SetIsFill( false );
    m_gal->SetIsStroke( true );
    m_gal->SetLineWidth( aDimension->GetWidth() );

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
    VECTOR2D position( text.GetTextPosition().x, text.GetTextPosition().y );
    double   orientation = text.GetOrientation() * M_PI / 1800.0;

    m_gal->SetLineWidth( text.GetThickness() );
    m_gal->SetTextAttributes( &text );
    m_gal->StrokeText( text.GetShownText(), position, orientation );
}


void PCB_PAINTER::draw( const PCB_TARGET* aTarget )
{
    const COLOR4D& strokeColor = m_pcbSettings.GetColor( aTarget, aTarget->GetLayer() );
    VECTOR2D position( aTarget->GetPosition() );
    double   size, radius;

    m_gal->SetLineWidth( aTarget->GetWidth() );
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
    const BOARD_ITEM* item = aMarker->GetItem();

    if( item )      // By default draw an item in a different color
    {
        Draw( item, ITEM_GAL_LAYER( DRC_VISIBLE ) );
    }
    else            // If there is no item associated - draw a circle marking the DRC error
    {
        m_gal->SetStrokeColor( COLOR4D( 1.0, 0.0, 0.0, 1.0 ) );
        m_gal->SetIsFill( false );
        m_gal->SetIsStroke( true );
        m_gal->SetLineWidth( 10000 );
        m_gal->DrawCircle( VECTOR2D( aMarker->GetPosition() ), 200000 );
    }
}


const double PCB_RENDER_SETTINGS::MAX_FONT_SIZE = Millimeter2iu( 10.0 );
