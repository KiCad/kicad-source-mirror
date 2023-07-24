/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2017 Jean-Pierre Charras, jp.charras at wanadoo.fr
 * Copyright (C) 2004-2022 KiCad Developers, see AUTHORS.txt for contributors.
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

#include <sch_draw_panel.h>
#include <macros.h>
#include <plotters/plotter.h>
#include <base_units.h>
#include <widgets/msgpanel.h>
#include <bitmaps.h>
#include <eda_draw_frame.h>
#include <general.h>
#include <schematic.h>
#include <sch_shape.h>
#include "plotters/plotter.h"


SCH_SHAPE::SCH_SHAPE( SHAPE_T aShape, int aLineWidth, FILL_T aFillType, KICAD_T aType ) :
    SCH_ITEM( nullptr, aType ),
    EDA_SHAPE( aShape, aLineWidth, aFillType )
{
    SetLayer( LAYER_NOTES );
}


EDA_ITEM* SCH_SHAPE::Clone() const
{
    return new SCH_SHAPE( *this );
}


void SCH_SHAPE::SwapData( SCH_ITEM* aItem )
{
    SCH_ITEM::SwapFlags( aItem );

    SCH_SHAPE* shape = static_cast<SCH_SHAPE*>( aItem );

    EDA_SHAPE::SwapShape( shape );
    std::swap( m_layer, shape->m_layer );
}


void SCH_SHAPE::SetStroke( const STROKE_PARAMS& aStroke )
{
    m_stroke = aStroke;
}


void SCH_SHAPE::Move( const VECTOR2I& aOffset )
{
    move( aOffset );
}


void SCH_SHAPE::Normalize()
{
    if( GetShape() == SHAPE_T::RECTANGLE )
    {
        VECTOR2I size = GetEnd() - GetPosition();

        if( size.y < 0 )
        {
            SetStartY( GetStartY() + size.y );
            SetEndY( GetStartY() - size.y );
        }

        if( size.x < 0 )
        {
            SetStartX( GetStartX() + size.x );
            SetEndX( GetStartX() - size.x );
        }
    }
}


void SCH_SHAPE::MirrorHorizontally( int aCenter )
{
    flip( VECTOR2I( aCenter, 0 ), true );
}


void SCH_SHAPE::MirrorVertically( int aCenter )
{
    flip( VECTOR2I( 0, aCenter ), false );
}


void SCH_SHAPE::Rotate( const VECTOR2I& aCenter )
{
    rotate( aCenter, -ANGLE_90 );
}


void SCH_SHAPE::Plot( PLOTTER* aPlotter, bool aBackground ) const
{
    int pen_size = std::max( GetPenWidth(), aPlotter->RenderSettings()->GetMinPenWidth() );

    static std::vector<VECTOR2I> cornerList;

    if( GetShape() == SHAPE_T::POLY )
    {
        cornerList.clear();

        for( const VECTOR2I& pt : m_poly.Outline( 0 ).CPoints() )
            cornerList.push_back( pt );
    }

    if( aBackground )
    {
        if( !aPlotter->GetColorMode() )
            return;

        if( m_fill == FILL_T::FILLED_WITH_COLOR && GetFillColor() != COLOR4D::UNSPECIFIED )
        {
            if( GetFillColor() != COLOR4D::UNSPECIFIED )
                aPlotter->SetColor( GetFillColor() );
            else
                aPlotter->SetColor( aPlotter->RenderSettings()->GetLayerColor( LAYER_NOTES ) );

            switch( GetShape() )
            {
            case SHAPE_T::ARC:
            {
                // In some plotters (not all) the arc is approximated by segments, and
                // a error max is needed. We try to approximate by 360/5 segments by 360 deg
                int arc2segment_error = CircleToEndSegmentDeltaRadius( GetRadius(), 360/5 );
                aPlotter->Arc( getCenter(), GetStart(), GetEnd(), m_fill, 0, arc2segment_error );
            }

                break;

            case SHAPE_T::CIRCLE:
                aPlotter->Circle( getCenter(), GetRadius() * 2, m_fill, 0 );
                break;

            case SHAPE_T::RECTANGLE:
                aPlotter->Rect( GetStart(), GetEnd(), m_fill, 0 );
                break;

            case SHAPE_T::POLY:
                aPlotter->PlotPoly( cornerList, m_fill, 0 );
                break;

            case SHAPE_T::BEZIER:
                aPlotter->PlotPoly( m_bezierPoints, m_fill, 0 );
                break;

            default:
                UNIMPLEMENTED_FOR( SHAPE_T_asString() );
            }
        }
    }
    else /* if( aForeground ) */
    {
        if( aPlotter->GetColorMode() && GetStroke().GetColor() != COLOR4D::UNSPECIFIED )
            aPlotter->SetColor( GetStroke().GetColor() );
        else
            aPlotter->SetColor( aPlotter->RenderSettings()->GetLayerColor( LAYER_NOTES ) );

        aPlotter->SetCurrentLineWidth( pen_size );
        aPlotter->SetDash( pen_size, GetEffectiveLineStyle() );

        switch( GetShape() )
        {
        case SHAPE_T::ARC:
        {
            // In some plotters (not all) the arc is approximated by segments, and
            // a error max is needed. We try to approximate by 360/5 segments by 360 deg
            int arc2segment_error = CircleToEndSegmentDeltaRadius( GetRadius(), 360/5 );
            aPlotter->Arc( getCenter(), GetStart(), GetEnd(), FILL_T::NO_FILL, pen_size, arc2segment_error );
        }

            break;

        case SHAPE_T::CIRCLE:
            aPlotter->Circle( getCenter(), GetRadius() * 2, FILL_T::NO_FILL, pen_size );
            break;

        case SHAPE_T::RECTANGLE:
        {
            std::vector<VECTOR2I> pts = GetRectCorners();

            aPlotter->MoveTo( pts[0] );
            aPlotter->LineTo( pts[1] );
            aPlotter->LineTo( pts[2] );
            aPlotter->LineTo( pts[3] );
            aPlotter->FinishTo( pts[0] );
        }
            break;

        case SHAPE_T::POLY:
            aPlotter->PlotPoly( cornerList, FILL_T::NO_FILL, pen_size );
            break;

        case SHAPE_T::BEZIER:
            aPlotter->PlotPoly( m_bezierPoints, FILL_T::NO_FILL, pen_size );
            break;

        default:
            UNIMPLEMENTED_FOR( SHAPE_T_asString() );
        }

        aPlotter->SetDash( pen_size, PLOT_DASH_TYPE::SOLID );
    }
}


int SCH_SHAPE::GetPenWidth() const
{
    if( m_stroke.GetWidth() > 0 )
        return m_stroke.GetWidth();

    // Historically 0 meant "default width" and negative numbers meant "don't stroke".
    if( GetWidth() < 0 )
        return 0;

    SCHEMATIC* schematic = Schematic();

    if( schematic )
        return schematic->Settings().m_DefaultLineWidth;

    return schIUScale.MilsToIU( DEFAULT_LINE_WIDTH_MILS );
}


void SCH_SHAPE::PrintBackground( const RENDER_SETTINGS* aSettings, const VECTOR2I& aOffset )
{
    wxDC*    DC = aSettings->GetPrintDC();
    COLOR4D  color;

    unsigned ptCount = 0;
    VECTOR2I* buffer = nullptr;

    if( GetShape() == SHAPE_T::POLY )
    {
        SHAPE_LINE_CHAIN poly = m_poly.Outline( 0 );

        ptCount = poly.GetPointCount();
        buffer = new VECTOR2I[ptCount];

        for( unsigned ii = 0; ii < ptCount; ++ii )
            buffer[ii] = poly.CPoint( ii );
    }
    else if( GetShape() == SHAPE_T::BEZIER )
    {
        ptCount = m_bezierPoints.size();
        buffer = new VECTOR2I[ptCount];

        for( size_t ii = 0; ii < ptCount; ++ii )
            buffer[ii] = m_bezierPoints[ii];
    }

    if( GetFillMode() == FILL_T::FILLED_WITH_COLOR )
    {
        if( GetFillColor() == COLOR4D::UNSPECIFIED )
            color = aSettings->GetLayerColor( LAYER_NOTES );
        else
            color = GetFillColor();

        switch( GetShape() )
        {
        case SHAPE_T::ARC:
            GRFilledArc( DC, GetEnd(), GetStart(), getCenter(), 0, color, color );
            break;

        case SHAPE_T::CIRCLE:
            GRFilledCircle( DC, GetStart(), GetRadius(), 0, color, color );
            break;

        case SHAPE_T::RECTANGLE:
            GRFilledRect( DC, GetStart(), GetEnd(), 0, color, color );
            break;

        case SHAPE_T::POLY:
            GRPoly( DC, ptCount, buffer, true, 0, color, color );
            break;

        case SHAPE_T::BEZIER:
            GRPoly( DC, ptCount, buffer, true, 0, color, color );
            break;

        default:
            UNIMPLEMENTED_FOR( SHAPE_T_asString() );
        }
    }

    delete[] buffer;

}


void SCH_SHAPE::Print( const RENDER_SETTINGS* aSettings, const VECTOR2I& aOffset )
{
    int      penWidth = GetPenWidth();
    wxDC*    DC = aSettings->GetPrintDC();
    COLOR4D  color = GetStroke().GetColor();

    if( color == COLOR4D::UNSPECIFIED )
        color = aSettings->GetLayerColor( LAYER_NOTES );

    COLOR4D bg = aSettings->GetBackgroundColor();

    if( bg == COLOR4D::UNSPECIFIED || GetGRForceBlackPenState() )
        bg = COLOR4D::WHITE;

    unsigned ptCount = 0;
    VECTOR2I* buffer = nullptr;

    if( GetShape() == SHAPE_T::POLY )
    {
        SHAPE_LINE_CHAIN poly = m_poly.Outline( 0 );

        ptCount = poly.GetPointCount();
        buffer = new VECTOR2I[ptCount];

        for( unsigned ii = 0; ii < ptCount; ++ii )
            buffer[ii] = poly.CPoint( ii );
    }
    else if( GetShape() == SHAPE_T::BEZIER )
    {
        ptCount = m_bezierPoints.size();
        buffer = new VECTOR2I[ptCount];

        for( size_t ii = 0; ii < ptCount; ++ii )
            buffer[ii] = m_bezierPoints[ii];
    }

    COLOR4D fillColor = COLOR4D::UNSPECIFIED;

    if( GetFillMode() == FILL_T::FILLED_SHAPE )
        fillColor = color;
    else if( GetFillMode() == FILL_T::FILLED_WITH_COLOR )
        fillColor = GetFillColor();

    if( fillColor != COLOR4D::UNSPECIFIED )
    {
        switch( GetShape() )
        {
        case SHAPE_T::ARC:
            GRFilledArc( DC, GetEnd(), GetStart(), getCenter(), 0, fillColor, fillColor );
            break;

        case SHAPE_T::CIRCLE:
            GRFilledCircle( DC, GetStart(), GetRadius(), 0, fillColor, fillColor );
            break;

        case SHAPE_T::RECTANGLE:
            GRFilledRect( DC, GetStart(), GetEnd(), 0, fillColor, fillColor );
            break;

        case SHAPE_T::POLY:
            GRPoly( DC, ptCount, buffer, true, 0, fillColor, fillColor );
            break;

        case SHAPE_T::BEZIER:
            GRPoly( DC, ptCount, buffer, true, 0, fillColor, fillColor );
            break;

        default:
            UNIMPLEMENTED_FOR( SHAPE_T_asString() );
        }
    }
    else
    {
        penWidth = std::max( penWidth, aSettings->GetMinPenWidth() );
    }

    if( penWidth > 0 )
    {
        if( GetEffectiveLineStyle() == PLOT_DASH_TYPE::SOLID )
        {
            switch( GetShape() )
            {
            case SHAPE_T::ARC:
                GRArc( DC, GetEnd(), GetStart(), getCenter(), penWidth, color );
                break;

            case SHAPE_T::CIRCLE:
                GRCircle( DC, GetStart(), GetRadius(), penWidth, color );
                break;

            case SHAPE_T::RECTANGLE:
                GRRect( DC, GetStart(), GetEnd(), penWidth, color );
                break;

            case SHAPE_T::POLY:
                GRPoly( DC, ptCount, buffer, false, penWidth, color, color );
                break;

            case SHAPE_T::BEZIER:
                GRPoly( DC, ptCount, buffer, false, penWidth, color, color );
                break;

            default:
                UNIMPLEMENTED_FOR( SHAPE_T_asString() );
            }
        }
        else
        {
            std::vector<SHAPE*> shapes = MakeEffectiveShapes( true );

            for( SHAPE* shape : shapes )
            {
                STROKE_PARAMS::Stroke( shape, GetEffectiveLineStyle(), penWidth, aSettings,
                                       [&]( const VECTOR2I& a, const VECTOR2I& b )
                                       {
                                           GRLine( DC, a.x, a.y, b.x, b.y, penWidth, color );
                                       } );
            }

            for( SHAPE* shape : shapes )
                delete shape;
        }
    }

    delete[] buffer;
}


void SCH_SHAPE::GetMsgPanelInfo( EDA_DRAW_FRAME* aFrame, std::vector<MSG_PANEL_ITEM>& aList )
{
    SCH_ITEM::GetMsgPanelInfo( aFrame, aList );

    ShapeGetMsgPanelInfo( aFrame, aList );
}


wxString SCH_SHAPE::GetItemDescription( UNITS_PROVIDER* aUnitsProvider ) const
{
    switch( GetShape() )
    {
    case SHAPE_T::ARC:
        return wxString::Format( _( "Arc, radius %s" ),
                                 aUnitsProvider->MessageTextFromValue( GetRadius() ) );

    case SHAPE_T::CIRCLE:
        return wxString::Format( _( "Circle, radius %s" ),
                                 aUnitsProvider->MessageTextFromValue( GetRadius() ) );

    case SHAPE_T::RECTANGLE:
        return wxString::Format( _( "Rectangle, width %s height %s" ),
                                 aUnitsProvider->MessageTextFromValue( std::abs( m_start.x - m_end.x ) ),
                                 aUnitsProvider->MessageTextFromValue( std::abs( m_start.y - m_end.y ) ) );

    case SHAPE_T::POLY:
        return wxString::Format( _( "Polyline, %d points" ),
                                 int( m_poly.Outline( 0 ).GetPointCount() ) );

    case SHAPE_T::BEZIER:
        return wxString::Format( _( "Bezier Curve, %d points" ),
                                 int( m_bezierPoints.size() ) );

    default:
        UNIMPLEMENTED_FOR( SHAPE_T_asString() );
        return wxEmptyString;
    }
}


BITMAPS SCH_SHAPE::GetMenuImage() const
{
    switch( GetShape() )
    {
    case SHAPE_T::SEGMENT: return BITMAPS::add_line;
    case SHAPE_T::ARC:     return BITMAPS::add_arc;
    case SHAPE_T::CIRCLE:  return BITMAPS::add_circle;
    case SHAPE_T::RECTANGLE:    return BITMAPS::add_rectangle;
    case SHAPE_T::POLY:    return BITMAPS::add_graphical_segments;

    default:
        UNIMPLEMENTED_FOR( SHAPE_T_asString() );
        return BITMAPS::question_mark;
    }
}


void SCH_SHAPE::ViewGetLayers( int aLayers[], int& aCount ) const
{
    aCount     = 3;
    aLayers[0] = LAYER_NOTES;
    aLayers[1] = LAYER_NOTES_BACKGROUND;
    aLayers[2] = LAYER_SELECTION_SHADOWS;
}


void SCH_SHAPE::AddPoint( const VECTOR2I& aPosition )
{
    if( GetShape() == SHAPE_T::POLY )
    {
        if( m_poly.IsEmpty() )
            m_poly.NewOutline();

        m_poly.Outline( 0 ).Append( aPosition, true );
    }
    else
    {
        UNIMPLEMENTED_FOR( SHAPE_T_asString() );
    }
}


static struct SCH_SHAPE_DESC
{
    SCH_SHAPE_DESC()
    {
        PROPERTY_MANAGER& propMgr = PROPERTY_MANAGER::Instance();
        REGISTER_TYPE( SCH_SHAPE );
        propMgr.AddTypeCast( new TYPE_CAST<SCH_SHAPE, SCH_ITEM> );
        propMgr.AddTypeCast( new TYPE_CAST<SCH_SHAPE, EDA_SHAPE> );
        propMgr.InheritsAfter( TYPE_HASH( SCH_SHAPE ), TYPE_HASH( SCH_ITEM ) );
        propMgr.InheritsAfter( TYPE_HASH( SCH_SHAPE ), TYPE_HASH( EDA_SHAPE ) );

        // Only polygons have meaningful Position properties.
        // On other shapes, these are duplicates of the Start properties.
        auto isPolygon =
                []( INSPECTABLE* aItem ) -> bool
        {
            if( SCH_SHAPE* shape = dynamic_cast<SCH_SHAPE*>( aItem ) )
                return shape->GetShape() == SHAPE_T::POLY;

            return false;
        };

        propMgr.OverrideAvailability( TYPE_HASH( SCH_SHAPE ), TYPE_HASH( SCH_ITEM ),
                                      _HKI( "Position X" ), isPolygon );
        propMgr.OverrideAvailability( TYPE_HASH( SCH_SHAPE ), TYPE_HASH( SCH_ITEM ),
                                      _HKI( "Position Y" ), isPolygon );
    }
} _SCH_SHAPE_DESC;
