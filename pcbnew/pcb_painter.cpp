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
#include <class_netinfo.h>
#include <pcbstruct.h>

#include <view/view.h>
#include <pcb_painter.h>
#include <gal/graphics_abstraction_layer.h>

using namespace KiGfx;

PCB_RENDER_SETTINGS::PCB_RENDER_SETTINGS()
{
    // By default everything should be displayed as filled
    for( unsigned int i = 0; i < END_PCB_VISIBLE_LIST; ++i )
    {
        m_sketchModeSelect[i] = false;
    }

    Update();
}


void PCB_RENDER_SETTINGS::ImportLegacyColors( COLORS_DESIGN_SETTINGS* aSettings )
{
    for( int i = 0; i < NB_LAYERS; i++ )
    {
        m_layerColors[i] = m_legacyColorMap[aSettings->GetLayerColor( i )];
    }

    for( int i = 0; i < END_PCB_VISIBLE_LIST; i++ )
    {
        m_itemColors[i] = m_legacyColorMap[aSettings->GetItemColor( i )];
    }

    // Default colors for specific layers
    m_itemColors[VIAS_HOLES_VISIBLE]        = COLOR4D( 0.5, 0.4, 0.0, 1.0 );
    m_itemColors[PADS_HOLES_VISIBLE]        = COLOR4D( 0.0, 0.5, 0.5, 1.0 );
    m_itemColors[VIAS_VISIBLE]              = COLOR4D( 0.7, 0.7, 0.7, 1.0 );
    m_itemColors[PADS_VISIBLE]              = COLOR4D( 0.7, 0.7, 0.7, 1.0 );
    m_itemColors[PADS_NETNAMES_VISIBLE]     = COLOR4D( 0.8, 0.8, 0.8, 0.7 );
    // Netnames for copper layers
    for( LAYER_NUM layer = FIRST_COPPER_LAYER; layer <= LAST_COPPER_LAYER; ++layer )
    {
        // Quick, dirty hack, netnames layers should be stored in usual layers
        m_itemColors[GetNetnameLayer( layer ) - NB_LAYERS] = COLOR4D( 0.8, 0.8, 0.8, 0.7 );
    }


    Update();
}


void PCB_RENDER_SETTINGS::LoadDisplayOptions( const DISPLAY_OPTIONS& aOptions )
{
    m_hiContrastEnabled = aOptions.ContrastModeDisplay;

    // Whether to draw tracks, vias & pads filled or as outlines
    m_sketchModeSelect[PADS_VISIBLE]   = !aOptions.DisplayPadFill;
    m_sketchModeSelect[VIAS_VISIBLE]   = !aOptions.DisplayViaFill;
    m_sketchModeSelect[TRACKS_VISIBLE] = !aOptions.DisplayPcbTrackFill;

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


void PCB_RENDER_SETTINGS::Update()
{
    // Calculate darkened/highlighted variants of layer colors
    for( int i = 0; i < NB_LAYERS; i++ )
    {
        m_layerColors[i].a   = m_layerOpacity;
        m_layerColorsHi[i]   = m_layerColors[i].Highlighted( m_highlightFactor );
        m_layerColorsDark[i] = m_layerColors[i].Darkened( 1.0 - m_highlightFactor );
        m_layerColorsSel[i]  = m_layerColors[i].Highlighted( m_selectFactor );
    }

    for( int i = 0; i < END_PCB_VISIBLE_LIST; i++ )
    {
        m_itemColors[i].a   = m_layerOpacity;
        m_itemColorsHi[i]   = m_itemColors[i].Highlighted( m_highlightFactor );
        m_itemColorsDark[i] = m_itemColors[i].Darkened( 1.0 - m_highlightFactor );
        m_itemColorsSel[i]  = m_itemColors[i].Highlighted( m_selectFactor );
    }

    m_hiContrastColor = COLOR4D( m_hiContrastFactor, m_hiContrastFactor, m_hiContrastFactor,
                                 m_layerOpacity );
}


PCB_PAINTER::PCB_PAINTER( GAL* aGal ) :
    PAINTER( aGal )
{
}


const COLOR4D& PCB_PAINTER::GetColor( const VIEW_ITEM* aItem, int aLayer )
{
    int netCode = 0;

    // Try to obtain the netcode for the item
    const BOARD_CONNECTED_ITEM* item = dynamic_cast<const BOARD_CONNECTED_ITEM*>( aItem );
    if( item )
        netCode = item->GetNet();

    return getLayerColor( aLayer, netCode );
}


const COLOR4D& PCB_PAINTER::getLayerColor( int aLayer, int aNetCode ) const
{
    if( m_pcbSettings->m_hiContrastEnabled && m_pcbSettings->m_activeLayer != aLayer )
    {
        return m_pcbSettings->m_hiContrastColor;
    }
    else if( m_pcbSettings->m_highlightEnabled )
    {
        if( aNetCode == m_pcbSettings->m_highlightNetcode )
        {
            return m_pcbSettings->m_layerColorsHi[aLayer];
        }
        else
        {
            return m_pcbSettings->m_layerColorsDark[aLayer];
        }
    }
    else
    {
        // For item layers (vias, texts, and so on)
        if( aLayer >= NB_LAYERS )
            return getItemColor( aLayer - NB_LAYERS, aNetCode );

        return m_pcbSettings->m_layerColors[aLayer];
    }
}


const COLOR4D& PCB_PAINTER::getItemColor( int aItemType, int aNetCode ) const
{
    // if(aNetCode == m_settings->m_hiContrastEnabled)
    //  return m_settings->m_itemColorsHi[ aItemType ];
    if( m_pcbSettings->m_highlightEnabled )
    {
        if( aNetCode == m_pcbSettings->m_highlightNetcode )
        {
            return m_pcbSettings->m_itemColorsHi[aItemType];
        }
        else
        {
            return m_pcbSettings->m_itemColorsDark[aItemType];
        }
    }
    else
    {
        return m_pcbSettings->m_itemColors[aItemType];
    }
}


bool PCB_PAINTER::Draw( const VIEW_ITEM* aItem, int aLayer )
{
    // the "cast" applied in here clarifies which overloaded draw() is called
    switch( aItem->Type() )
    {
    case PCB_ZONE_T:
    case PCB_TRACE_T:
        draw( (TRACK*) aItem, aLayer );
        break;

    case PCB_VIA_T:
        draw( (SEGVIA*) aItem, aLayer );
        break;

    case PCB_PAD_T:
        draw( (D_PAD*) aItem, aLayer );
        break;

    case PCB_LINE_T:
    case PCB_MODULE_EDGE_T:
        draw( (DRAWSEGMENT*) aItem );
        break;

    case PCB_TEXT_T:
        draw( (TEXTE_PCB*) aItem );
        break;

    case PCB_MODULE_TEXT_T:
        draw( (TEXTE_MODULE*) aItem, aLayer );
        break;

    case PCB_ZONE_AREA_T:
        draw( (ZONE_CONTAINER*) aItem );
        break;

    case PCB_DIMENSION_T:
        draw( (DIMENSION*) aItem );
        break;

    case PCB_TARGET_T:
        draw( (PCB_TARGET*) aItem );
        break;

    default:
        // Painter does not know how to draw the object
        return false;
        break;
    }

    return true;
}


void PCB_PAINTER::draw( const TRACK* aTrack, int aLayer )
{
    VECTOR2D start( aTrack->GetStart() );
    VECTOR2D end( aTrack->GetEnd() );
    int      width = aTrack->GetWidth();
    int      netNumber = aTrack->GetNet();
    COLOR4D  color = getLayerColor( aLayer, netNumber );

    m_gal->SetStrokeColor( color );

    if( IsNetnameLayer( aLayer ) )
    {
        // If there is a net name - display it on the track
        if( netNumber != 0 )
        {
            VECTOR2D line = ( end - start );
            double length = line.EuclideanNorm();

            // Check if the track is long enough to have a netname displayed
            if( length < 10 * width )
                return;

            NETINFO_ITEM* net = ( (BOARD*) aTrack->GetParent() )->FindNet( netNumber );
            std::string netName = std::string( net->GetShortNetname().mb_str() );
            VECTOR2D textPosition = start + line / 2.0;     // center of the track
            double textOrientation = -atan( line.y / line.x );
            double textSize = std::min( static_cast<double>( width ), length / netName.length() );

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
    else if( IsCopperLayer( aLayer ))
    {
        // Draw a regular track
        m_gal->SetIsStroke( true );

        if( m_pcbSettings->m_sketchModeSelect[TRACKS_VISIBLE] )
        {
            // Outline mode
            m_gal->SetLineWidth( m_pcbSettings->m_outlineWidth );
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


void PCB_PAINTER::draw( const SEGVIA* aVia, int aLayer )
{
    VECTOR2D center( aVia->GetStart() );
    double   radius;
    COLOR4D  color;

    // Choose drawing settings depending on if we are drawing via's pad or hole
    if( aLayer == ITEM_GAL_LAYER( VIAS_VISIBLE ) )
    {
        radius = aVia->GetWidth() / 2.0;
    }
    else if( aLayer == ITEM_GAL_LAYER( VIAS_HOLES_VISIBLE ) )
    {
        radius = aVia->GetDrillValue() / 2.0;
    }
    else
        return;

    color = getLayerColor( aLayer, aVia->GetNet() );

    if( m_pcbSettings->m_sketchModeSelect[VIAS_VISIBLE] )
    {
        // Outline mode
        m_gal->SetIsFill( false );
        m_gal->SetIsStroke( true );
        m_gal->SetLineWidth( m_pcbSettings->m_outlineWidth );
        m_gal->SetStrokeColor( color );
        m_gal->DrawCircle( center, radius );
    }
    else
    {
        // Filled mode
        m_gal->SetIsFill( true );
        m_gal->SetIsStroke( false );
        m_gal->SetFillColor( color );
        m_gal->DrawCircle( center, radius );
    }
}


void PCB_PAINTER::draw( const D_PAD* aPad, int aLayer )
{
    COLOR4D     color;
    VECTOR2D    size;
    VECTOR2D    position( aPad->GetPosition() );
    PAD_SHAPE_T shape;
    double      m, n;
    double      orientation = aPad->GetOrientation();
    NORMALIZE_ANGLE_90( orientation );  // do not display descriptions upside down
    orientation = orientation * M_PI / 1800.0;

    color = getLayerColor( aLayer, aPad->GetNet() );

    // Draw description layer
    if( aLayer == ITEM_GAL_LAYER( PADS_NETNAMES_VISIBLE ) )
    {
        size = VECTOR2D( aPad->GetSize() / 2 );
        double scale = m_gal->GetZoomFactor();
        double maxSize = PCB_RENDER_SETTINGS::MAX_FONT_SIZE / scale;

        // Font size limits
        if( size.x > maxSize )
            size.x = maxSize;
        if( size.y > maxSize )
            size.y = maxSize;

        // Keep the size ratio for the font, but make it smaller
        if( size.x < size.y )
        {
            orientation -= M_PI / 2;
            size.y = size.x * 4.0 / 3.0;
        }
        else if( size.x == size.y )
        {
            // If the text is displayed on a symmetrical pad, do not rotate it
            orientation = 0.0;
        }
        else
        {
            size.x = size.y * 3.0 / 4.0;
        }

        m_gal->Save();
        m_gal->Translate( position );
        m_gal->Rotate( -orientation );

        // Default font settings
        m_gal->SetHorizontalJustify( GR_TEXT_HJUSTIFY_CENTER );
        m_gal->SetVerticalJustify( GR_TEXT_VJUSTIFY_CENTER );
        m_gal->SetBold( false );
        m_gal->SetItalic( false );
        m_gal->SetMirrored( false );
        m_gal->SetStrokeColor( color );

        // Let's make some space for a netname too, if there's one to display
        if( !aPad->GetNetname().empty() )
        {
            size = size / 2.0;
            m_gal->SetGlyphSize( size );
            m_gal->SetLineWidth( size.y / 10.0 );

            m_gal->StrokeText( std::string( aPad->GetNetname().mb_str() ),
                                 VECTOR2D( 0, size.y ), 0.0 );
            m_gal->Translate( VECTOR2D( 0.0, -size.y / 2.0 ) );
        }
        else
        {
            // In case when there's no netname assigned
            m_gal->SetGlyphSize( size );
            m_gal->SetLineWidth( size.y / 10.0 );
        }

        m_gal->StrokeText( std::string( aPad->GetPadName().mb_str() ), VECTOR2D( 0, 0 ), 0.0 );

        m_gal->Restore();
        return;
    }

    if( m_pcbSettings->m_sketchModeSelect[PADS_VISIBLE] )
    {
        // Outline mode
        m_gal->SetIsFill( false );
        m_gal->SetIsStroke( true );
        m_gal->SetLineWidth( m_pcbSettings->m_outlineWidth );
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
    m_gal->Rotate( -aPad->GetOrientation() * M_PI / 1800.0 );    // orientation is in tenths of degree

    // Choose drawing settings depending on if we are drawing a pad itself or a hole
    if( aLayer == ITEM_GAL_LAYER( PADS_HOLES_VISIBLE ) )
    {
        // Drawing hole
        size  = VECTOR2D( aPad->GetDrillSize() ) / 2.0;
        shape = aPad->GetDrillShape();
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

            if( m_pcbSettings->m_sketchModeSelect[PADS_VISIBLE] )
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

            if( m_pcbSettings->m_sketchModeSelect[PADS_VISIBLE] )
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
    case PAD_TRAPEZOID:
        m_gal->DrawRectangle( VECTOR2D( -size.x, -size.y ), VECTOR2D( size.x, size.y ) );
        break;

    case PAD_CIRCLE:
        m_gal->DrawCircle( VECTOR2D( 0.0, 0.0 ), size.x );
        break;

    case PAD_OCTAGON:    // it is not used anywhere, neither you can set it using pcbnew..
    case PAD_NONE:
        break;
    }

    m_gal->Restore();
}


void PCB_PAINTER::draw( const DRAWSEGMENT* aSegment )
{
    COLOR4D              strokeColor = getLayerColor( aSegment->GetLayer(), 0 );
    std::deque<VECTOR2D> pointsList;

    m_gal->SetIsFill( false );
    m_gal->SetIsStroke( true );
    m_gal->SetStrokeColor( strokeColor );
    m_gal->SetLineWidth( aSegment->GetWidth() );

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
        std::copy( aSegment->GetPolyPoints().begin(), aSegment->GetPolyPoints().end(),
                   std::back_inserter( pointsList ) );
        m_gal->DrawPolygon( pointsList );
        break;

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


void PCB_PAINTER::draw( const TEXTE_PCB* aText )
{
    COLOR4D  strokeColor = getLayerColor( aText->GetLayer(), 0 );
    VECTOR2D position( aText->GetTextPosition().x, aText->GetTextPosition().y );
    double   orientation = aText->GetOrientation() * M_PI / 1800.0;

    m_gal->SetStrokeColor( strokeColor );
    m_gal->SetLineWidth( aText->GetThickness() );
    m_gal->SetTextAttributes( aText );
    m_gal->StrokeText( std::string( aText->GetText().mb_str() ), position, orientation );
}


void PCB_PAINTER::draw( const TEXTE_MODULE* aText, int aLayer )
{
    COLOR4D  strokeColor = getLayerColor( aLayer, 0 );
    VECTOR2D position( aText->GetTextPosition().x, aText->GetTextPosition().y);
    double   orientation = aText->GetDrawRotation() * M_PI / 1800.0;

    m_gal->SetStrokeColor( strokeColor );
    m_gal->SetLineWidth( aText->GetThickness() );
    m_gal->SetTextAttributes( aText );
    m_gal->StrokeText( std::string( aText->GetText().mb_str() ), position, orientation );
}


void PCB_PAINTER::draw( const ZONE_CONTAINER* aContainer )
{
    COLOR4D color = getLayerColor( aContainer->GetLayer(), aContainer->GetNet() );
    std::deque<VECTOR2D> corners;
    PCB_RENDER_SETTINGS::DisplayZonesMode displayMode = m_pcbSettings->m_displayZoneMode;

    // Draw the outline
    m_gal->SetStrokeColor( color );
    m_gal->SetIsFill( false );
    m_gal->SetIsStroke( true );
    m_gal->SetLineWidth( m_pcbSettings->m_outlineWidth );

    const CPolyLine* outline = aContainer->Outline();
    for( int i = 0; i < outline->GetCornersCount(); ++i )
    {
        corners.push_back( VECTOR2D( outline->GetPos( i ) ) );
    }
    // The last point for closing the polyline
    corners.push_back( VECTOR2D( outline->GetPos( 0 ) ) );
    m_gal->DrawPolyline( corners );
    corners.clear();

    // Draw the filling
    if( displayMode != PCB_RENDER_SETTINGS::DZ_HIDE_FILLED )
    {
        const std::vector<CPolyPt> polyPoints = aContainer->GetFilledPolysList().GetList();
        if( polyPoints.size() == 0 )  // Nothing to draw
            return;

        // Set up drawing options
        m_gal->SetFillColor( color );
        m_gal->SetLineWidth( aContainer->GetThermalReliefCopperBridge() / 2.0 );

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


void PCB_PAINTER::draw( const DIMENSION* aDimension )
{
    COLOR4D strokeColor = getLayerColor( aDimension->GetLayer(), 0 );

    m_gal->SetStrokeColor( strokeColor );
    m_gal->SetIsFill( false );
    m_gal->SetIsStroke( true );
    m_gal->SetLineWidth( aDimension->GetWidth() );

    // Draw an arrow
    m_gal->DrawLine( VECTOR2D( aDimension->m_crossBarO ), VECTOR2D( aDimension->m_crossBarF ) );
    m_gal->DrawLine( VECTOR2D( aDimension->m_featureLineGO ), VECTOR2D( aDimension->m_featureLineGF ) );
    m_gal->DrawLine( VECTOR2D( aDimension->m_featureLineDO ), VECTOR2D( aDimension->m_featureLineDF ) );
    m_gal->DrawLine( VECTOR2D( aDimension->m_arrowD1O ), VECTOR2D( aDimension->m_arrowD1F ) );
    m_gal->DrawLine( VECTOR2D( aDimension->m_arrowD2O ), VECTOR2D( aDimension->m_arrowD2F ) );
    m_gal->DrawLine( VECTOR2D( aDimension->m_arrowG1O ), VECTOR2D( aDimension->m_arrowG1F ) );
    m_gal->DrawLine( VECTOR2D( aDimension->m_arrowG2O ), VECTOR2D( aDimension->m_arrowG2F ) );

    // Draw text
    draw( &aDimension->Text() );
}


void PCB_PAINTER::draw( const PCB_TARGET* aTarget )
{
    COLOR4D  strokeColor = getLayerColor( aTarget->GetLayer(), 0 );
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

    m_gal->DrawLine( VECTOR2D( -size, 0.0 ),
                     VECTOR2D(  size, 0.0 ) );
    m_gal->DrawLine( VECTOR2D( 0.0, -size ),
                     VECTOR2D( 0.0,  size ) );
    m_gal->DrawCircle( VECTOR2D( 0.0, 0.0 ), radius );

    m_gal->Restore();
}
