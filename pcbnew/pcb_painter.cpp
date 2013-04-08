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
#include <pcbstruct.h>

#include <view/view.h>
#include <pcb_painter.h>
#include <gal/graphics_abstraction_layer.h>
#include <gal/stroke_font.h>
#include <newstroke_font.h>

using namespace KiGfx;

PCB_RENDER_SETTINGS::PCB_RENDER_SETTINGS()
{
    Update();
}


void PCB_RENDER_SETTINGS::ImportLegacyColors( COLORS_DESIGN_SETTINGS* aSettings )
{
    for( int i = 0; i < LAYER_COUNT; i++ )
    {
        m_layerColors[i] = m_legacyColorMap[aSettings->GetLayerColor( i )];
    }

    for( int i = 0; i < END_PCB_VISIBLE_LIST; i++ )
    {
        m_itemColors[i] = m_legacyColorMap[aSettings->GetItemColor( i )];
    }

    m_itemColors[VIA_HOLES_VISIBLE] = COLOR4D( 0.5f, 0.4f, 0.0f, 1.0f );
    m_itemColors[PAD_HOLES_VISIBLE] = COLOR4D( 0.0f, 0.5f, 0.5f, 1.0f );
    m_itemColors[VIAS_VISIBLE]      = COLOR4D( 0.7f, 0.7f, 0.7f, 1.0f );
    m_itemColors[PADS_VISIBLE]      = COLOR4D( 0.7f, 0.7f, 0.7f, 1.0f );

    Update();
}


void PCB_RENDER_SETTINGS::LoadDisplayOptions( const DISPLAY_OPTIONS& aOptions )
{
    m_hiContrastEnabled = aOptions.ContrastModeDisplay;
}


void PCB_RENDER_SETTINGS::Update()
{
    // Calculate darkened/highlighted variants of layer colors
    for( int i = 0; i < LAYER_COUNT; i++ )
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
        if( aLayer >= LAYER_COUNT )
            return getItemColor( aLayer - LAYER_COUNT, aNetCode );

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


bool PCB_PAINTER::Draw( const EDA_ITEM* aItem, int aLayer )
{
    // the "cast" applied in here clarifies which overloaded draw() is called
    switch( aItem->Type() )
    {
    case PCB_ZONE_T:
    case PCB_TRACE_T:
        draw( (TRACK*) aItem );
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


void PCB_PAINTER::draw( const TRACK* aTrack )
{
    VECTOR2D start( aTrack->GetStart() );
    VECTOR2D end( aTrack->GetEnd() );
    COLOR4D  strokeColor = getLayerColor( aTrack->GetLayer(), aTrack->GetNet() );

    m_gal->SetLineCap( LINE_CAP_ROUND );
    m_gal->SetLineJoin( LINE_JOIN_ROUND );
    m_gal->SetLineWidth( aTrack->GetWidth() );
    m_gal->SetStrokeColor( strokeColor );
    m_gal->DrawLine( start, end );
}


void PCB_PAINTER::draw( const SEGVIA* aVia, int aLayer )
{
    VECTOR2D center( aVia->GetStart() );
    double   radius;
    COLOR4D  fillColor;

    // Choose drawing settings depending on if we are drawing via's pad or hole
    if( aLayer == ITEM_GAL_LAYER( VIAS_VISIBLE ) )
    {
        radius = aVia->GetWidth() / 2.0f;
    }
    else if( aLayer == ITEM_GAL_LAYER( VIA_HOLES_VISIBLE ) )
    {
        radius = aVia->GetDrillValue() / 2.0f;
    }
    else
        return;

    fillColor = getLayerColor( aLayer, aVia->GetNet() );

    m_gal->SetIsStroke( false );
    m_gal->SetIsFill( true );
    m_gal->SetFillColor( fillColor );
    m_gal->DrawCircle( center, radius );
}


void PCB_PAINTER::draw( const D_PAD* aPad, int aLayer )
{
    COLOR4D     fillColor;
    VECTOR2D    size;
    PAD_SHAPE_T shape;
    double      m, n;

    m_gal->Save();
    m_gal->Translate( VECTOR2D( aPad->GetPosition() ) );
    m_gal->Rotate( -aPad->GetOrientation() * M_PI / 1800.0 );    // orientation is in tenths of degree

    // Choose drawing settings depending on if we are drawing a pad itself or a hole
    if( aLayer == ITEM_GAL_LAYER( PAD_HOLES_VISIBLE ) )
    {
        // Drawing hole
        size  = VECTOR2D( aPad->GetDrillSize() ) / 2.0f;
        shape = aPad->GetDrillShape();
    }
    else
    {
        // Drawing every kind of pad
        m_gal->Translate( VECTOR2D( aPad->GetOffset() ) );
        size  = VECTOR2D( aPad->GetSize() ) / 2.0f;
        shape = aPad->GetShape();
    }

    fillColor = getLayerColor( aLayer, aPad->GetNet() );

    m_gal->SetIsFill( true );
    m_gal->SetIsStroke( false );
    m_gal->SetFillColor( fillColor );

    switch( shape )
    {
    case PAD_OVAL:
        if( size.y >= size.x )
        {
            m = ( size.y - size.x );
            n = size.x;

            m_gal->DrawCircle( VECTOR2D( 0, -m ), n );
            m_gal->DrawCircle( VECTOR2D( 0, m ), n );
            m_gal->DrawRectangle( VECTOR2D( -n, -m ), VECTOR2D( n, m ) );
        }
        else
        {
            m = ( size.x - size.y );
            n = size.y;

            m_gal->DrawCircle( VECTOR2D( -m, 0 ), n );
            m_gal->DrawCircle( VECTOR2D( m, 0 ), n );
            m_gal->DrawRectangle( VECTOR2D( -m, -n ), VECTOR2D( m, n ) );
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
    m_gal->SetLineCap( LINE_CAP_ROUND );
    m_gal->SetLineJoin( LINE_JOIN_ROUND );

    switch( aSegment->GetShape() )
    {
    case S_SEGMENT:
        m_gal->DrawLine( VECTOR2D( aSegment->GetStart() ), VECTOR2D( aSegment->GetEnd() ) );
        break;

    case S_RECT:
        m_gal->SetLineCap( LINE_CAP_SQUARED );
        m_gal->SetLineJoin( LINE_JOIN_BEVEL );
        m_gal->DrawLine( VECTOR2D( aSegment->GetStart() ), VECTOR2D( aSegment->GetEnd() ) );
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
    m_stroke_font->LoadAttributes( aText );
    m_stroke_font->Draw( std::string( aText->GetText().mb_str() ), position, orientation );
}


void PCB_PAINTER::draw( const TEXTE_MODULE* aText, int aLayer )
{
    COLOR4D  strokeColor = getLayerColor( aLayer, 0 );
    VECTOR2D position( aText->GetTextPosition().x, aText->GetTextPosition().y);
    double   orientation = aText->GetDrawRotation() * M_PI / 1800.0;

    m_gal->SetStrokeColor( strokeColor );
    m_gal->SetLineWidth( aText->GetThickness() );
    m_stroke_font->LoadAttributes( aText );
    m_stroke_font->Draw( std::string( aText->GetText().mb_str() ), position, orientation );
}


void PCB_PAINTER::draw( const ZONE_CONTAINER* aContainer )
{
    COLOR4D fillColor = getLayerColor( aContainer->GetLayer(), aContainer->GetNet() );
    std::vector<CPolyPt>::iterator polyIterator;
    std::vector<CPolyPt> polyPoints = aContainer->GetFilledPolysList();
    std::deque<VECTOR2D> corners;

    m_gal->SetLineCap( LINE_CAP_ROUND );
    m_gal->SetLineJoin( LINE_JOIN_ROUND );
    m_gal->SetFillColor( fillColor );
    m_gal->SetStrokeColor( fillColor );
    m_gal->SetIsFill( aContainer->IsFilled() );
    m_gal->SetIsStroke( true );
    m_gal->SetLineWidth( aContainer->GetThermalReliefCopperBridge() / 2.0 );

    // FIXME implement hatch mode

    for( polyIterator = polyPoints.begin(); polyIterator != polyPoints.end(); polyIterator++ )
    {
        // Find out all of polygons and then draw them
        if( !polyIterator->end_contour )
        {
            corners.push_back( VECTOR2D( *polyIterator ) );
        }
        else
        {
            m_gal->DrawPolygon( corners );
            m_gal->DrawPolyline( corners );
            corners.clear();
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
    COLOR4D strokeColor = getLayerColor( aTarget->GetLayer(), 0 );
    double  radius;

    // according to PCB_TARGET::Draw() (class_mire.cpp)
    if( aTarget->GetShape() )    // shape X
    {
        radius = aTarget->GetSize() / 2;
    }
    else
    {
        radius = aTarget->GetSize() / 3;
    }

    m_gal->SetStrokeColor( strokeColor );
    m_gal->SetIsFill( true );
    m_gal->SetIsStroke( true );
    m_gal->DrawCircle( VECTOR2D( aTarget->GetPosition() ), radius );
}
