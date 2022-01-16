/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2017 Jean-Pierre Charras, jp.charras at wanadoo.fr
 * Copyright (C) 2004-2021 KiCad Developers, see AUTHORS.txt for contributors.
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


SCH_SHAPE::SCH_SHAPE( SHAPE_T aShape, int aLineWidth, FILL_T aFillType ) :
    SCH_ITEM( nullptr, SCH_SHAPE_T ),
    EDA_SHAPE( aShape, aLineWidth, aFillType, true )
{
    SetLayer( LAYER_NOTES );
}


EDA_ITEM* SCH_SHAPE::Clone() const
{
    return new SCH_SHAPE( *this );
}


void SCH_SHAPE::SwapData( SCH_ITEM* aItem )
{
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
    rotate( aCenter, ANGLE_90 );
}


void SCH_SHAPE::Plot( PLOTTER* aPlotter ) const
{
    int       pen_size = std::max( GetPenWidth(), aPlotter->RenderSettings()->GetMinPenWidth() );
    VECTOR2I  center;
    int       radius = 0;
    EDA_ANGLE startAngle;
    EDA_ANGLE endAngle;

    static std::vector<VECTOR2I> cornerList;

    if( GetShape() == SHAPE_T::POLY )
    {
        cornerList.clear();

        for( const VECTOR2I& pt : m_poly.Outline( 0 ).CPoints() )
            cornerList.push_back( pt );
    }
    else if( GetShape() == SHAPE_T::ARC )
    {
        center = getCenter();
        radius = GetRadius();
        CalcArcAngles( startAngle, endAngle );
    }

    if( GetStroke().GetColor() == COLOR4D::UNSPECIFIED )
        aPlotter->SetColor( aPlotter->RenderSettings()->GetLayerColor( LAYER_NOTES ) );
    else
        aPlotter->SetColor( GetStroke().GetColor() );

    aPlotter->SetCurrentLineWidth( pen_size );
    aPlotter->SetDash( GetEffectiveLineStyle() );

    switch( GetShape() )
    {
    case SHAPE_T::ARC:
        aPlotter->Arc( center, -endAngle, -startAngle, radius, FILL_T::NO_FILL, pen_size );
        break;

    case SHAPE_T::CIRCLE:
        aPlotter->Circle( GetStart(), GetRadius() * 2, FILL_T::NO_FILL, pen_size );
        break;

    case SHAPE_T::RECT:
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
    {
        aPlotter->MoveTo( cornerList[0] );

        for( size_t ii = 1; ii < cornerList.size(); ++ii )
            aPlotter->LineTo( cornerList[ii] );

        aPlotter->FinishTo( cornerList[0] );
    }
        break;

    case SHAPE_T::BEZIER:
        aPlotter->PlotPoly( m_bezierPoints, FILL_T::NO_FILL, pen_size );
        break;

    default:
        UNIMPLEMENTED_FOR( SHAPE_T_asString() );
    }

    aPlotter->SetDash( PLOT_DASH_TYPE::SOLID );

    if( m_fill == FILL_T::FILLED_WITH_COLOR && GetFillColor() != COLOR4D::UNSPECIFIED )
    {
        aPlotter->SetColor( GetFillColor() );

        switch( GetShape() )
        {
        case SHAPE_T::ARC:
            aPlotter->Arc( center, -endAngle, -startAngle, radius, m_fill, 0 );
            break;

        case SHAPE_T::CIRCLE:
            aPlotter->Circle( GetStart(), GetRadius() * 2, m_fill, 0 );
            break;

        case SHAPE_T::RECT:
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


int SCH_SHAPE::GetPenWidth() const
{
    if( m_stroke.GetWidth() > 0 )
        return m_stroke.GetWidth();

    // Historically 0 meant "default width" and negative numbers meant "don't stroke".
    if( GetWidth() < 0 && GetFillMode() != FILL_T::NO_FILL )
        return 0;

    SCHEMATIC* schematic = Schematic();

    if( schematic )
        return schematic->Settings().m_DefaultLineWidth;

    return Mils2iu( DEFAULT_LINE_WIDTH_MILS );
}


void SCH_SHAPE::Print( const RENDER_SETTINGS* aSettings, const VECTOR2I& aOffset )
{
    int      penWidth = GetPenWidth();
    wxDC*    DC = aSettings->GetPrintDC();
    VECTOR2I pt1 = GetStart();
    VECTOR2I pt2 = GetEnd();
    VECTOR2I c;
    COLOR4D  color;

    penWidth = std::max( penWidth, aSettings->GetDefaultPenWidth() );

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
    else if( GetShape() == SHAPE_T::ARC )
    {
        c = getCenter();
        EDA_ANGLE t1, t2;

        CalcArcAngles( t1, t2 );

        if( ( t1 - t2 ).Normalize180() > ANGLE_0 )
            std::swap( pt1, pt2 );
    }

    if( GetFillMode() == FILL_T::FILLED_WITH_COLOR )
    {
        color = GetFillColor();

        switch( GetShape() )
        {
        case SHAPE_T::ARC:
            GRFilledArc1( nullptr, DC, pt1, pt2, c, 0, color, color );
            break;

        case SHAPE_T::CIRCLE:
            GRFilledCircle( nullptr, DC, pt1.x, pt1.y, GetRadius(), 0, color, color );
            break;

        case SHAPE_T::RECT:
            GRFilledRect( nullptr, DC, pt1.x, pt1.y, pt2.x, pt2.y, 0, color, color );
            break;

        case SHAPE_T::POLY:
            GRPoly( nullptr, DC, ptCount, buffer, true, 0, color, color );
            break;

        case SHAPE_T::BEZIER:
            GRPoly( nullptr, DC, ptCount, buffer, true, 0, color, color );
            break;

        default:
            UNIMPLEMENTED_FOR( SHAPE_T_asString() );
        }
    }

    if( GetStroke().GetColor() == COLOR4D::UNSPECIFIED )
        color = aSettings->GetLayerColor( LAYER_NOTES );
    else
        color = GetStroke().GetColor();

    if( GetStroke().GetPlotStyle() <= PLOT_DASH_TYPE::FIRST_TYPE )
    {
        switch( GetShape() )
        {
        case SHAPE_T::ARC:
            GRArc1( nullptr, DC, pt1, pt2, c, penWidth, color );
            break;

        case SHAPE_T::CIRCLE:
            GRCircle( nullptr, DC, pt1.x, pt1.y, GetRadius(), penWidth, color );
            break;

        case SHAPE_T::RECT:
            GRRect( nullptr, DC, pt1.x, pt1.y, pt2.x, pt2.y, penWidth, color );
            break;

        case SHAPE_T::POLY:
            GRPoly( nullptr, DC, ptCount, buffer, false, penWidth, color, color );
            break;

        case SHAPE_T::BEZIER:
            GRPoly( nullptr, DC, ptCount, buffer, false, penWidth, color, color );
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
            STROKE_PARAMS::Stroke( shape, GetStroke().GetPlotStyle(), penWidth, aSettings,
                                   [&]( const VECTOR2I& a, const VECTOR2I& b )
                                   {
                                       GRLine( nullptr, DC, a.x, a.y, b.x, b.y, penWidth, color );
                                   } );
        }

        for( SHAPE* shape : shapes )
            delete shape;
    }

    delete[] buffer;
}


void SCH_SHAPE::GetMsgPanelInfo( EDA_DRAW_FRAME* aFrame, std::vector<MSG_PANEL_ITEM>& aList )
{
    SCH_ITEM::GetMsgPanelInfo( aFrame, aList );

    ShapeGetMsgPanelInfo( aFrame, aList );
}


wxString SCH_SHAPE::GetSelectMenuText( EDA_UNITS aUnits ) const
{
    switch( GetShape() )
    {
    case SHAPE_T::ARC:
        return wxString::Format( _( "Arc, radius %s" ),
                                 MessageTextFromValue( aUnits, GetRadius() ) );

    case SHAPE_T::CIRCLE:
        return wxString::Format( _( "Circle, radius %s" ),
                                 MessageTextFromValue( aUnits, GetRadius() ) );

    case SHAPE_T::RECT:
        return wxString::Format( _( "Rectangle, width %s height %s" ),
                                 MessageTextFromValue( aUnits, std::abs( m_start.x - m_end.x ) ),
                                 MessageTextFromValue( aUnits, std::abs( m_start.y - m_end.y ) ) );

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
    case SHAPE_T::RECT:    return BITMAPS::add_rectangle;
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


