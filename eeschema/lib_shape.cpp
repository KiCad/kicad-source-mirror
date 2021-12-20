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
#include <plotters/plotter.h>
#include <macros.h>
#include <base_units.h>
#include <widgets/msgpanel.h>
#include <bitmaps.h>
#include <eda_draw_frame.h>
#include <general.h>
#include <lib_shape.h>


LIB_SHAPE::LIB_SHAPE( LIB_SYMBOL* aParent, SHAPE_T aShape, int aDefaultLineWidth,
                      FILL_T aFillType ) :
    LIB_ITEM( LIB_SHAPE_T, aParent ),
    EDA_SHAPE( aShape, aDefaultLineWidth, aFillType, true )
{
    m_editState = 0;
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

    return hitTest( DefaultTransform.TransformCoordinate( aRect ), aContained, aAccuracy );
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
    int     startAngle = 0;
    int     endAngle = 0;
    int     pen_size = GetEffectivePenWidth( aPlotter->RenderSettings() );
    FILL_T fill = aFill ? m_fill : FILL_T::NO_FILL;

    static std::vector<wxPoint> cornerList;

    if( GetShape() == SHAPE_T::POLY )
    {
        const SHAPE_LINE_CHAIN& poly = m_poly.Outline( 0 );
        cornerList.clear();

        for( const VECTOR2I& pt : poly.CPoints() )
            cornerList.push_back( aTransform.TransformCoordinate( (wxPoint) pt ) + aOffset );
    }
    else if( GetShape() == SHAPE_T::BEZIER )
    {
        cornerList.clear();

        for( const wxPoint& pt : m_bezierPoints )
            cornerList.push_back( aTransform.TransformCoordinate( pt ) + aOffset );
    }
    else if( GetShape() == SHAPE_T::ARC )
    {
        center = aTransform.TransformCoordinate( getCenter() ) + aOffset;

        CalcArcAngles( startAngle, endAngle );
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

        case SHAPE_T::POLY:
        case SHAPE_T::BEZIER:
            aPlotter->PlotPoly( cornerList, fill, 0 );
            break;

        default:
            UNIMPLEMENTED_FOR( SHAPE_T_asString() );
        }

        if( pen_size <= 0 )
            return;
        else
            fill = FILL_T::NO_FILL;
    }

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

    case SHAPE_T::POLY:
    case SHAPE_T::BEZIER:
        aPlotter->PlotPoly( cornerList, fill, pen_size );
        break;

    default:
        UNIMPLEMENTED_FOR( SHAPE_T_asString() );
    }
}


int LIB_SHAPE::GetPenWidth() const
{
    return GetWidth();
}


void LIB_SHAPE::print( const RENDER_SETTINGS* aSettings, const wxPoint& aOffset,
                       void* aData, const TRANSFORM& aTransform )
{
    bool forceNoFill = static_cast<bool>( aData );
    int  penWidth = GetEffectivePenWidth( aSettings );

    if( forceNoFill && IsFilled() && penWidth == 0 )
        return;

    wxDC*    DC = aSettings->GetPrintDC();
    wxPoint  pt1 = aTransform.TransformCoordinate( m_start ) + aOffset;
    wxPoint  pt2 = aTransform.TransformCoordinate( m_end ) + aOffset;
    wxPoint  c;
    COLOR4D  color = aSettings->GetLayerColor( LAYER_DEVICE );
    COLOR4D  fillColor = color;

    unsigned ptCount = 0;
    wxPoint* buffer = nullptr;

    if( GetShape() == SHAPE_T::POLY )
    {
        const SHAPE_LINE_CHAIN& poly = m_poly.Outline( 0 );

        ptCount = poly.GetPointCount();
        buffer = new wxPoint[ ptCount ];

        for( unsigned ii = 0; ii < ptCount; ++ii )
            buffer[ii] = aTransform.TransformCoordinate( (wxPoint) poly.CPoint( ii ) ) + aOffset;
    }
    else if( GetShape() == SHAPE_T::BEZIER )
    {
        ptCount = m_bezierPoints.size();
        buffer = new wxPoint[ ptCount ];

        for( size_t ii = 0; ii < ptCount; ++ii )
            buffer[ii] = aTransform.TransformCoordinate( m_bezierPoints[ii] ) + aOffset;
    }
    else if( GetShape() == SHAPE_T::ARC )
    {
        c = aTransform.TransformCoordinate( getCenter() ) + aOffset;

        int t1, t2;

        CalcArcAngles( t1, t2 );

        if( aTransform.MapAngles( &t1, &t2 ) == ( NormalizeAngle180( t1 - t2 ) > 0 ) )
            std::swap( pt1, pt2 );
    }

    if( forceNoFill || GetFillType() == FILL_T::NO_FILL )
    {
        penWidth = std::max( penWidth, aSettings->GetDefaultPenWidth() );

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
        if( GetFillType() == FILL_T::FILLED_WITH_BG_BODYCOLOR )
            fillColor = aSettings->GetLayerColor( LAYER_DEVICE_BACKGROUND );

        switch( GetShape() )
        {
        case SHAPE_T::ARC:
            // If we stroke in GRFilledArc it will stroke the two radials too, so we have to
            // fill and stroke separately

            GRFilledArc1( nullptr, DC, pt1, pt2, c, 0, fillColor, fillColor );

            GRArc1( nullptr, DC, pt1, pt2, c, penWidth, color );
            break;

        case SHAPE_T::CIRCLE:
            GRFilledCircle( nullptr, DC, pt1.x, pt1.y, GetRadius(), 0, color, fillColor );
            break;

        case SHAPE_T::RECT:
            GRFilledRect( nullptr, DC, pt1.x, pt1.y, pt2.x, pt2.y, penWidth, color, fillColor );
            break;

        case SHAPE_T::POLY:
            if( penWidth > 0 )
                GRPoly( nullptr, DC, ptCount, buffer, true, penWidth, color, fillColor );
            else
                GRPoly( nullptr, DC, ptCount, buffer, true, 0, fillColor, fillColor );
            break;

        case SHAPE_T::BEZIER:
            if( penWidth > 0 )
                GRPoly( nullptr, DC, ptCount, buffer, true, penWidth, color, fillColor );
            else
                GRPoly( nullptr, DC, ptCount, buffer, true, 0, fillColor, fillColor );
            break;

        default:
            UNIMPLEMENTED_FOR( SHAPE_T_asString() );
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


void LIB_SHAPE::GetMsgPanelInfo( EDA_DRAW_FRAME* aFrame, std::vector<MSG_PANEL_ITEM>& aList )
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


BITMAPS LIB_SHAPE::GetMenuImage() const
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


void LIB_SHAPE::AddPoint( const wxPoint& aPosition )
{
    if( GetShape() == SHAPE_T::POLY )
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


void LIB_SHAPE::CalcArcAngles( int& aStartAngle, int& aEndAngle ) const
{
    wxPoint centerStartVector = GetStart() - GetCenter();
    wxPoint centerEndVector   = GetEnd() - GetCenter();

    // Angles in Eeschema are still integers
    aStartAngle = KiROUND( ArcTangente( centerStartVector.y, centerStartVector.x ) );
    aEndAngle = KiROUND( ArcTangente( centerEndVector.y, centerEndVector.x ) );

    NORMALIZE_ANGLE_POS( aStartAngle );
    NORMALIZE_ANGLE_POS( aEndAngle );  // angles = 0 .. 3600

    // Restrict angle to less than 180 to avoid PBS display mirror Trace because it is
    // assumed that the arc is less than 180 deg to find orientation after rotate or mirror.
    if( ( aEndAngle - aStartAngle ) > 1800 )
        aEndAngle -= 3600;
    else if( ( aEndAngle - aStartAngle ) <= -1800 )
        aEndAngle += 3600;

    while( ( aEndAngle - aStartAngle ) >= 1800 )
    {
        aEndAngle--;
        aStartAngle++;
    }

    while( ( aStartAngle - aEndAngle ) >= 1800 )
    {
        aEndAngle++;
        aStartAngle--;
    }

    NORMALIZE_ANGLE_POS( aStartAngle );

    if( !IsMoving() )
        NORMALIZE_ANGLE_POS( aEndAngle );
}


