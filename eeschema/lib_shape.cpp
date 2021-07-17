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
#include <plotter.h>
#include <base_units.h>
#include <widgets/msgpanel.h>
#include <bitmaps.h>
#include <eda_draw_frame.h>
#include <general.h>
#include <lib_shape.h>


LIB_SHAPE::LIB_SHAPE( LIB_SYMBOL* aParent, SHAPE_T aShape, int aDefaultLineWidth,
                      FILL_T aFillType ) :
    LIB_ITEM( LIB_SHAPE_T, aParent ),
    EDA_SHAPE( aShape, aDefaultLineWidth, aFillType )
{
}


bool LIB_SHAPE::HitTest( const wxPoint& aPosRef, int aAccuracy ) const
{
    if( aAccuracy < Mils2iu( MINIMUM_SELECTION_DISTANCE ) )
        aAccuracy = Mils2iu( MINIMUM_SELECTION_DISTANCE );

    return hitTest( DefaultTransform.TransformCoordinate( aPosRef ), aAccuracy );
}


bool LIB_SHAPE::HitTest( const EDA_RECT& aRect, bool aContained, int aAccuracy ) const
{
    if( m_flags & (STRUCT_DELETED | SKIP_STRUCT ) )
        return false;

    return hitTest( aRect, aContained, aAccuracy );
}


EDA_ITEM* LIB_SHAPE::Clone() const
{
    return new LIB_SHAPE( *this );
}


int LIB_SHAPE::compare( const LIB_ITEM& aOther, LIB_ITEM::COMPARE_FLAGS aCompareFlags ) const
{
    int retv = LIB_ITEM::compare( aOther, aCompareFlags );

    if( retv )
        return retv;

    return EDA_SHAPE::Compare( &static_cast<const LIB_SHAPE&>( aOther ) );
}


void LIB_SHAPE::Offset( const wxPoint& aOffset )
{
    move( aOffset );
}


void LIB_SHAPE::MoveTo( const wxPoint& aPosition )
{
    setPosition( aPosition );
}


void LIB_SHAPE::MirrorHorizontal( const wxPoint& aCenter )
{
    flip( aCenter, true );
}


void LIB_SHAPE::MirrorVertical( const wxPoint& aCenter )
{
    flip( aCenter, false );
}


void LIB_SHAPE::Rotate( const wxPoint& aCenter, bool aRotateCCW )
{
    int rot_angle = aRotateCCW ? -900 : 900;

    rotate( aCenter, rot_angle );
}


void LIB_SHAPE::Plot( PLOTTER* aPlotter, const wxPoint& aOffset, bool aFill,
                      const TRANSFORM& aTransform ) const
{
    wxPoint start = aTransform.TransformCoordinate( m_start ) + aOffset;
    wxPoint end = aTransform.TransformCoordinate( m_end ) + aOffset;
    wxPoint center;
    int     startAngle;
    int     endAngle;
    int     pen_size = GetPenWidth();
    FILL_T fill = aFill ? m_fill : FILL_TYPE::NO_FILL;

    static std::vector<wxPoint> cornerList;

    if( GetShape() == SHAPE_T::POLYGON )
    {
        cornerList.clear();

        for( const VECTOR2I& pt : m_poly.Outline( 0 ).CPoints() )
            cornerList.push_back( aTransform.TransformCoordinate( (wxPoint) pt ) + aOffset );
    }
    else if( GetShape() == SHAPE_T::CURVE )
    {
        cornerList.clear();

        for( const wxPoint& pt : m_bezierPoints )
            cornerList.push_back( aTransform.TransformCoordinate( pt ) + aOffset );
    }
    else if( GetShape() == SHAPE_T::ARC )
    {
        center = aTransform.TransformCoordinate( getCenter() ) + aOffset;
        startAngle = GetArcAngleStart();
        endAngle = GetArcAngleEnd();
        aTransform.MapAngles( &startAngle, &endAngle );
    }

    if( fill == FILL_T::FILLED_WITH_BG_BODYCOLOR )
    {
        aPlotter->SetColor( aPlotter->RenderSettings()->GetLayerColor( LAYER_DEVICE_BACKGROUND ) );

        switch( GetShape() )
        {
        case SHAPE_T::ARC:
            aPlotter->Arc( center, -endAngle, -startAngle, GetRadius(), fill, 0 );
            break;

        case SHAPE_T::CIRCLE:
            aPlotter->Circle( start, GetRadius() * 2, fill, 0 );
            break;

        case SHAPE_T::RECT:
            aPlotter->Rect( start, end, fill, 0 );
            break;

        case SHAPE_T::POLYGON:
        case SHAPE_T::CURVE:
            aPlotter->PlotPoly( cornerList, fill, 0 );
            break;

        default:
            wxFAIL_MSG( "LIB_SHAPE::Plot not implemented for " + SHAPE_T_asString() );
        }

        if( pen_size <= 0 )
            return;
        else
            fill = FILL_T::NO_FILL;
    }

    pen_size = std::max( pen_size, aPlotter->RenderSettings()->GetMinPenWidth() );

    aPlotter->SetColor( aPlotter->RenderSettings()->GetLayerColor( LAYER_DEVICE ) );

    switch( GetShape() )
    {
    case SHAPE_T::ARC:
        aPlotter->Arc( center, -endAngle, -startAngle, GetRadius(), fill, pen_size );
        break;

    case SHAPE_T::CIRCLE:
        aPlotter->Circle( start, GetRadius() * 2, fill, pen_size );
        break;

    case SHAPE_T::RECT:
        aPlotter->Rect( start, end, fill, pen_size );
        break;

    case SHAPE_T::POLYGON:
    case SHAPE_T::CURVE:
        aPlotter->PlotPoly( cornerList, fill, pen_size );
        break;

    default:
        wxFAIL_MSG( "LIB_SHAPE::Plot not implemented for " + SHAPE_T_asString() );
    }
}


int LIB_SHAPE::GetPenWidth() const
{
    // Historically 0 meant "default width" and negative numbers meant "don't stroke".
    if( GetWidth() < 0 && GetFillType() != FILL_T::NO_FILL )
        return 0;
    else
        return std::max( GetWidth(), 1 );
}


void LIB_SHAPE::print( const RENDER_SETTINGS* aSettings, const wxPoint& aOffset,
                       void* aData, const TRANSFORM& aTransform )
{
    bool forceNoFill = static_cast<bool>( aData );
    int  penWidth = GetPenWidth();

    if( forceNoFill && IsFilled() && penWidth == 0 )
        return;

    wxDC*    DC = aSettings->GetPrintDC();
    wxPoint  pt1 = aTransform.TransformCoordinate( m_start ) + aOffset;
    wxPoint  pt2 = aTransform.TransformCoordinate( m_end ) + aOffset;
    wxPoint  c;
    int      t1;
    int      t2;
    COLOR4D  color = aSettings->GetLayerColor( LAYER_DEVICE );

    unsigned ptCount = 0;
    wxPoint* buffer = nullptr;

    if( GetShape() == SHAPE_T::POLYGON )
    {
        SHAPE_LINE_CHAIN poly = m_poly.Outline( 0 );

        ptCount = poly.GetPointCount();
        buffer = new wxPoint[ ptCount ];

        for( unsigned ii = 0; ii < ptCount; ++ii )
            buffer[ii] = aTransform.TransformCoordinate( (wxPoint) poly.CPoint( ii ) ) + aOffset;
    }
    else if( GetShape() == SHAPE_T::CURVE )
    {
        ptCount = m_bezierPoints.size();
        buffer = new wxPoint[ ptCount ];

        for( size_t ii = 0; ii < ptCount; ++ii )
            buffer[ii] = aTransform.TransformCoordinate( m_bezierPoints[ii] ) + aOffset;
    }
    else if( GetShape() == SHAPE_T::ARC )
    {
        c = aTransform.TransformCoordinate( getCenter() ) + aOffset;
        t1 = GetArcAngleStart();
        t2 = GetArcAngleEnd();
        aTransform.MapAngles( &t1, &t2 );
    }

    if( forceNoFill || GetFillType() == FILL_T::NO_FILL )
    {
        penWidth = std::max( penWidth, aSettings->GetDefaultPenWidth() );

        switch( GetShape() )
        {
        case SHAPE_T::ARC:
            GRArc1( nullptr, DC, pt2.x, pt2.y, pt1.x, pt1.y, c.x, c.y, penWidth, color );
            break;

        case SHAPE_T::CIRCLE:
            GRCircle( nullptr, DC, pt1.x, pt1.y, GetRadius(), penWidth, color );
            break;

        case SHAPE_T::RECT:
            GRRect( nullptr, DC, pt1.x, pt1.y, pt2.x, pt2.y, penWidth, color );
            break;

        case SHAPE_T::POLYGON:
            GRPoly( nullptr, DC, ptCount, buffer, false, penWidth, color, color );
            break;

        case SHAPE_T::CURVE:
            GRPoly( nullptr, DC, ptCount, buffer, false, penWidth, color, color );
            break;

        default:
            wxFAIL_MSG( "LIB_SHAPE::print not implemented for " + SHAPE_T_asString() );
        }
    }
    else
    {
        if( GetFillType() == FILL_T::FILLED_WITH_BG_BODYCOLOR )
            color = aSettings->GetLayerColor( LAYER_DEVICE_BACKGROUND );

        switch( GetShape() )
        {
        case SHAPE_T::ARC:
            GRFilledArc( nullptr, DC, c.x, c.y, t1, t2, GetRadius(), penWidth, color, color );
            break;

        case SHAPE_T::CIRCLE:
            GRFilledCircle( nullptr, DC, pt1.x, pt1.y, GetRadius(), 0, color, color );
            break;

        case SHAPE_T::RECT:
            GRFilledRect( nullptr, DC, pt1.x, pt1.y, pt2.x, pt2.y, penWidth, color, color );
            break;

        case SHAPE_T::POLYGON:
            GRPoly( nullptr, DC, ptCount, buffer, true, penWidth, color, color );
            break;

        case SHAPE_T::CURVE:
            GRPoly( nullptr, DC, ptCount, buffer, true, penWidth, color, color );
            break;

        default:
            wxFAIL_MSG( "LIB_SHAPE::print not implemented for " + SHAPE_T_asString() );
        }
    }

    delete[] buffer;
}


const EDA_RECT LIB_SHAPE::GetBoundingBox() const
{
    EDA_RECT rect = getBoundingBox();

    rect.RevertYAxis();

    return rect;
}


void LIB_SHAPE::GetMsgPanelInfo( EDA_DRAW_FRAME* aFrame, MSG_PANEL_ITEMS& aList )
{
    LIB_ITEM::GetMsgPanelInfo( aFrame, aList );

    ShapeGetMsgPanelInfo( aFrame, aList );
}


wxString LIB_SHAPE::GetSelectMenuText( EDA_UNITS aUnits ) const
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

    case SHAPE_T::POLYGON:
        return wxString::Format( _( "Polyline, %d points" ),
                                 int( m_poly.Outline( 0 ).GetPointCount() ) );

    case SHAPE_T::CURVE:
        return wxString::Format( _( "Bezier Curve, %d points" ),
                                 int( m_bezierPoints.size() ) );

    default:
        wxFAIL_MSG( "LIB_SHAPE::GetSelectMenuText unimplemented for " + SHAPE_T_asString() );
        return wxEmptyString;
    }
}


BITMAPS LIB_SHAPE::GetMenuImage() const
{
    switch( GetShape() )
    {
    case SHAPE_T::SEGMENT:  return BITMAPS::add_line;
    case SHAPE_T::ARC:      return BITMAPS::add_arc;
    case SHAPE_T::CIRCLE:   return BITMAPS::add_circle;
    case SHAPE_T::RECT:     return BITMAPS::add_rectangle;
    case SHAPE_T::POLYGON:  return BITMAPS::add_graphical_segments;

    default:
        wxFAIL_MSG( "LIB_SHAPE::GetMenuImage unimplemented for " + SHAPE_T_asString() );
        return BITMAPS::question_mark;
    }
}


void LIB_SHAPE::BeginEdit( const wxPoint& aPosition )
{
    switch( GetShape() )
    {
    case SHAPE_T::SEGMENT:
    case SHAPE_T::CIRCLE:
    case SHAPE_T::RECT:
        SetPosition( aPosition );
        SetEnd( aPosition );
        break;

    case SHAPE_T::ARC:
        SetStart( aPosition );
        SetEnd( aPosition );
        SetEditState( 1 );
        break;

    case SHAPE_T::POLYGON:
        // Start and end of the first segment (co-located for now)
        m_poly.Outline( 0 ).Append( aPosition );
        m_poly.Outline( 0 ).Append( aPosition, true );
        break;

    default:
        wxFAIL_MSG( "LIB_SHAPE::BeginEdit not implemented for " + SHAPE_T_asString() );
    }
}


bool LIB_SHAPE::ContinueEdit( const wxPoint& aPosition )
{
    switch( GetShape() )
    {
    case SHAPE_T::SEGMENT:
    case SHAPE_T::CIRCLE:
    case SHAPE_T::RECT:
        return false;

    case SHAPE_T::POLYGON:
    {
        SHAPE_LINE_CHAIN& poly = m_poly.Outline( 0 );

        // do not add zero-length segments
        if( poly.CPoint( poly.GetPointCount() - 2 ) != poly.CLastPoint() )
            poly.Append( aPosition );
    }
        return true;

    default:
        wxFAIL_MSG( "LIB_SHAPE::ContinueEdit not implemented for " + SHAPE_T_asString() );
        return false;
    }
}


void LIB_SHAPE::CalcEdit( const wxPoint& aPosition )
{
#define sq( x ) pow( x, 2 )

    switch( GetShape() )
    {
    case SHAPE_T::SEGMENT:
    case SHAPE_T::CIRCLE:
    case SHAPE_T::RECT:
        SetEnd( aPosition );
        break;

    case SHAPE_T::POLYGON:
        m_poly.Outline( 0 ).SetPoint( m_poly.Outline( 0 ).GetPointCount() - 1, aPosition );
        break;

    case SHAPE_T::ARC:
        // Edit state 0: drawing: place start
        // Edit state 1: drawing: place end (center calculated for 90-degree subtended angle)
        // Edit state 2: point edit: move start (center calculated for invariant subtended angle)
        // Edit state 3: point edit: move end (center calculated for invariant subtended angle)
        // Edit state 4: point edit: move center
        // Edit state 5: point edit: move arc-mid-point

        switch( m_editState )
        {
        case 0:
            SetArcGeometry( aPosition, aPosition, aPosition );
            return;

        case 1:
        {
            wxPoint start = GetStart();
            wxPoint end = aPosition;
            wxPoint center = CalcArcCenter( start, end, 90.0 );
            wxPoint mid = (wxPoint) CalcArcMid( start, end, center, true );

            SetArcGeometry( start, mid, aPosition );
        }
            break;

        case 2:
        {
            wxPoint delta = aPosition - GetStart();

            SetArcGeometry( aPosition, GetArcMid() + delta/2, GetEnd() );
        }
            break;

        case 3:
        {
            wxPoint delta = aPosition - GetEnd();

            SetArcGeometry( GetStart(), GetArcMid() + delta/2, aPosition );
        }
            break;

        case 4:
            MoveTo( aPosition );
            break;

        case 5:
            SetArcGeometry( GetStart(), aPosition, GetEnd() );
            break;

        }

        break;

    default:
        wxFAIL_MSG( "LIB_SHAPE::CalcEdit not implemented for " + SHAPE_T_asString() );
    }
}


void LIB_SHAPE::EndEdit()
{
    switch( GetShape() )
    {
    case SHAPE_T::ARC:
    case SHAPE_T::SEGMENT:
    case SHAPE_T::CIRCLE:
    case SHAPE_T::RECT:
        break;

    case SHAPE_T::POLYGON:
    {
        SHAPE_LINE_CHAIN& poly = m_poly.Outline( 0 );

        // do not include last point twice
        if( poly.GetPointCount() > 2 )
        {
            if( poly.CPoint( poly.GetPointCount() - 2 ) == poly.CLastPoint() )
                poly.Remove( poly.GetPointCount() - 1 );
        }
    }
        break;

    default:
        wxFAIL_MSG( "LIB_SHAPE::EndEdit not implemented for " + SHAPE_T_asString() );
    }
}


void LIB_SHAPE::AddPoint( const wxPoint& aPosition )
{
    if( GetShape() == SHAPE_T::POLYGON )
    {
        if( m_poly.IsEmpty() )
            m_poly.NewOutline();

        m_poly.Outline( 0 ).Append( aPosition, true );
    }
    else
    {
        wxFAIL_MSG( "LIB_SHAPE::AddPoint not implemented for " + SHAPE_T_asString() );
    }
}


