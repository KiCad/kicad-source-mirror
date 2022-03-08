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
#include <plotters/plotter.h>
#include <macros.h>
#include <base_units.h>
#include <widgets/msgpanel.h>
#include <bitmaps.h>
#include <eda_draw_frame.h>
#include <general.h>
#include <lib_shape.h>


LIB_SHAPE::LIB_SHAPE( LIB_SYMBOL* aParent, SHAPE_T aShape, int aLineWidth, FILL_T aFillType,
                      KICAD_T aType ) :
    LIB_ITEM( aType, aParent ),
    EDA_SHAPE( aShape, aLineWidth, aFillType, true )
{
    m_editState = 0;
}


bool LIB_SHAPE::HitTest( const VECTOR2I& aPosRef, int aAccuracy ) const
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


int LIB_SHAPE::compare( const LIB_ITEM& aOther, int aCompareFlags ) const
{
    int retv = LIB_ITEM::compare( aOther, aCompareFlags );

    if( retv )
        return retv;

    return EDA_SHAPE::Compare( &static_cast<const LIB_SHAPE&>( aOther ) );
}


void LIB_SHAPE::Offset( const VECTOR2I& aOffset )
{
    move( aOffset );
}


void LIB_SHAPE::MoveTo( const VECTOR2I& aPosition )
{
    setPosition( aPosition );
}


void LIB_SHAPE::MirrorHorizontal( const VECTOR2I& aCenter )
{
    flip( aCenter, true );
}


void LIB_SHAPE::MirrorVertical( const VECTOR2I& aCenter )
{
    flip( aCenter, false );
}


void LIB_SHAPE::Rotate( const VECTOR2I& aCenter, bool aRotateCCW )
{
    EDA_ANGLE rot_angle = aRotateCCW ? -ANGLE_90 : ANGLE_90;

    rotate( aCenter, rot_angle );
}


void LIB_SHAPE::Plot( PLOTTER* aPlotter, bool aBackground, const VECTOR2I& aOffset,
                      const TRANSFORM& aTransform ) const
{
    if( IsPrivate() )
        return;

    VECTOR2I  start = aTransform.TransformCoordinate( m_start ) + aOffset;
    VECTOR2I  end = aTransform.TransformCoordinate( m_end ) + aOffset;
    VECTOR2I  center = aTransform.TransformCoordinate( getCenter() ) + aOffset;

    static std::vector<VECTOR2I> cornerList;

    if( GetShape() == SHAPE_T::POLY )
    {
        const SHAPE_LINE_CHAIN& poly = m_poly.Outline( 0 );
        cornerList.clear();

        for( const VECTOR2I& pt : poly.CPoints() )
            cornerList.push_back( aTransform.TransformCoordinate( pt ) + aOffset );
    }
    else if( GetShape() == SHAPE_T::BEZIER )
    {
        cornerList.clear();

        for( const VECTOR2I& pt : m_bezierPoints )
            cornerList.push_back( aTransform.TransformCoordinate( pt ) + aOffset );
    }
    else if( GetShape() == SHAPE_T::ARC )
    {
        EDA_ANGLE t1, t2;

        CalcArcAngles( t1, t2 );

        if( aTransform.MapAngles( &t1, &t2 ) != ( ( t1 - t2 ).Normalize180() > ANGLE_0 ) )
            std::swap( start, end );
    }

    int     penWidth;
    COLOR4D color;
    FILL_T  fill = m_fill;

    if( aBackground )
    {
        if( !aPlotter->GetColorMode() )
            return;

        switch( m_fill )
        {
        case FILL_T::FILLED_SHAPE:
            return;

        case FILL_T::FILLED_WITH_COLOR:
            color = GetFillColor();
            break;

        case FILL_T::FILLED_WITH_BG_BODYCOLOR:
            color = aPlotter->RenderSettings()->GetLayerColor( LAYER_DEVICE_BACKGROUND );
            break;

        default:
            return;
        }

        penWidth = 0;
    }
    else
    {
        if( m_fill == FILL_T::FILLED_SHAPE )
            fill = m_fill;
        else
            fill = FILL_T::NO_FILL;

        penWidth = GetEffectivePenWidth( aPlotter->RenderSettings() );
        color = aPlotter->RenderSettings()->GetLayerColor( LAYER_DEVICE );
    }

    aPlotter->SetColor( color );

    switch( GetShape() )
    {
    case SHAPE_T::ARC:
        aPlotter->Arc( center, start, end, fill, penWidth, ARC_HIGH_DEF );
        break;

    case SHAPE_T::CIRCLE:
        aPlotter->Circle( center, GetRadius() * 2, fill, penWidth );
        break;

    case SHAPE_T::RECT:
        aPlotter->Rect( start, end, fill, penWidth );
        break;

    case SHAPE_T::POLY:
    case SHAPE_T::BEZIER:
        aPlotter->PlotPoly( cornerList, fill, penWidth );
        break;

    default:
        UNIMPLEMENTED_FOR( SHAPE_T_asString() );
    }
}


int LIB_SHAPE::GetPenWidth() const
{
    return GetWidth();
}


void LIB_SHAPE::print( const RENDER_SETTINGS* aSettings, const VECTOR2I& aOffset, void* aData,
                       const TRANSFORM& aTransform )
{
    if( IsPrivate() )
        return;

    bool forceNoFill = static_cast<bool>( aData );
    int  penWidth = GetEffectivePenWidth( aSettings );

    if( forceNoFill && IsFilled() && penWidth == 0 )
        return;

    wxDC*    DC = aSettings->GetPrintDC();
    VECTOR2I pt1 = aTransform.TransformCoordinate( m_start ) + aOffset;
    VECTOR2I pt2 = aTransform.TransformCoordinate( m_end ) + aOffset;
    VECTOR2I c;
    COLOR4D  color = aSettings->GetLayerColor( LAYER_DEVICE );

    unsigned ptCount = 0;
    VECTOR2I* buffer = nullptr;

    if( GetShape() == SHAPE_T::POLY )
    {
        const SHAPE_LINE_CHAIN& poly = m_poly.Outline( 0 );

        ptCount = poly.GetPointCount();
        buffer = new VECTOR2I[ptCount];

        for( unsigned ii = 0; ii < ptCount; ++ii )
            buffer[ii] = aTransform.TransformCoordinate( poly.CPoint( ii ) ) + aOffset;
    }
    else if( GetShape() == SHAPE_T::BEZIER )
    {
        ptCount = m_bezierPoints.size();
        buffer = new VECTOR2I[ptCount];

        for( size_t ii = 0; ii < ptCount; ++ii )
            buffer[ii] = aTransform.TransformCoordinate( m_bezierPoints[ii] ) + aOffset;
    }
    else if( GetShape() == SHAPE_T::ARC )
    {
        c = aTransform.TransformCoordinate( getCenter() ) + aOffset;

        EDA_ANGLE t1, t2;

        CalcArcAngles( t1, t2 );

        // N.B. The order of evaluation is critical here as MapAngles will modify t1, t2
        // and the Normalize routine depends on these modifications for the correct output
        bool transformed = aTransform.MapAngles( &t1, &t2 );
        bool transformed2 = ( ( t1 - t2 ).Normalize180() > ANGLE_0 );

        if( transformed  == transformed2 )
            std::swap( pt1, pt2 );
    }

    if( forceNoFill || GetFillMode() == FILL_T::NO_FILL )
    {
        penWidth = std::max( penWidth, aSettings->GetDefaultPenWidth() );

        switch( GetShape() )
        {
        case SHAPE_T::ARC:
            GRArc( DC, pt1, pt2, c, penWidth, color );
            break;

        case SHAPE_T::CIRCLE:
            GRCircle( DC, pt1, GetRadius(), penWidth, color );
            break;

        case SHAPE_T::RECT:
            GRRect( DC, pt1, pt2, penWidth, color );
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
        COLOR4D  fillColor = color;

        if( GetFillMode() == FILL_T::FILLED_WITH_BG_BODYCOLOR )
            fillColor = aSettings->GetLayerColor( LAYER_DEVICE_BACKGROUND );
        else if( GetFillMode() == FILL_T::FILLED_WITH_COLOR )
            fillColor = GetFillColor();

        switch( GetShape() )
        {
        case SHAPE_T::ARC:
            // If we stroke in GRFilledArc it will stroke the two radials too, so we have to
            // fill and stroke separately

            GRFilledArc( DC, pt1, pt2, c, 0, fillColor, fillColor );

            GRArc( DC, pt1, pt2, c, penWidth, color );
            break;

        case SHAPE_T::CIRCLE:
            GRFilledCircle( DC, pt1, GetRadius(), 0, color, fillColor );
            break;

        case SHAPE_T::RECT:
            GRFilledRect( DC, pt1, pt2, penWidth, color, fillColor );
            break;

        case SHAPE_T::POLY:

            GRPoly( DC, ptCount, buffer, true, 0, fillColor, fillColor );

            if( penWidth > 0 )
                GRPoly( DC, ptCount, buffer, false, penWidth, color, fillColor );

            break;

        case SHAPE_T::BEZIER:
            if( penWidth > 0 )
                GRPoly( DC, ptCount, buffer, true, penWidth, color, fillColor );
            else
                GRPoly( DC, ptCount, buffer, true, 0, fillColor, fillColor );
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


void LIB_SHAPE::AddPoint( const VECTOR2I& aPosition )
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




void LIB_SHAPE::ViewGetLayers( int aLayers[], int& aCount ) const
{
    aCount     = 3;
    aLayers[0] = IsPrivate() ? LAYER_NOTES            : LAYER_DEVICE;
    aLayers[1] = IsPrivate() ? LAYER_NOTES_BACKGROUND : LAYER_DEVICE_BACKGROUND;
    aLayers[2] = LAYER_SELECTION_SHADOWS;
}
