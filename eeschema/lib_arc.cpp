/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2017 Jean-Pierre Charras, jp.charras at wanadoo.fr
 * Copyright (C) 2004-2021 KiCad Developers, see AUTHORS.txt for contributors.
 * Copyright (C) 2019 CERN
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
#include <trigo.h>
#include <base_units.h>
#include <widgets/msgpanel.h>
#include <bitmaps.h>
#include <math/util.h>      // for KiROUND
#include <eda_draw_frame.h>
#include <general.h>
#include <lib_arc.h>
#include <transform.h>
#include <settings/color_settings.h>
#include <status_popup.h>

// Helper function
static inline wxPoint twoPointVector( const wxPoint &startPoint, const wxPoint &endPoint )
{
    return endPoint - startPoint;
}


LIB_ARC::LIB_ARC( LIB_SYMBOL* aParent ) : LIB_ITEM( LIB_ARC_T, aParent )
{
    m_Width         = 0;
    m_fill          = FILL_TYPE::NO_FILL;
    m_isFillable    = true;
    m_editState     = 0;
}


int LIB_ARC::GetRadius() const
{
    return KiROUND( GetLineLength( GetCenter(), GetStart() ) );
}


bool LIB_ARC::HitTest( const wxPoint& aRefPoint, int aAccuracy ) const
{
    int     mindist = std::max( aAccuracy + GetPenWidth() / 2,
                                Mils2iu( MINIMUM_SELECTION_DISTANCE ) );
    wxPoint relativePosition = aRefPoint;

    relativePosition.y = -relativePosition.y; // reverse Y axis

    int distance = KiROUND( GetLineLength( m_Pos, relativePosition ) );

    if( abs( distance - GetRadius() ) > mindist )
        return false;

    // We are on the circle, ensure we are only on the arc, i.e. between
    //  m_ArcStart and m_ArcEnd

    wxPoint startEndVector = twoPointVector( m_ArcStart, m_ArcEnd );
    wxPoint startRelativePositionVector = twoPointVector( m_ArcStart, relativePosition );

    wxPoint centerStartVector = twoPointVector( m_Pos, m_ArcStart );
    wxPoint centerEndVector = twoPointVector( m_Pos, m_ArcEnd );
    wxPoint centerRelativePositionVector = twoPointVector( m_Pos, relativePosition );

    // Compute the cross product to check if the point is in the sector
    double crossProductStart = CrossProduct( centerStartVector, centerRelativePositionVector );
    double crossProductEnd = CrossProduct( centerEndVector, centerRelativePositionVector );

    // The cross products need to be exchanged, depending on which side the center point
    // relative to the start point to end point vector lies
    if( CrossProduct( startEndVector, startRelativePositionVector ) < 0 )
    {
        std::swap( crossProductStart, crossProductEnd );
    }

    // When the cross products have a different sign, the point lies in sector
    // also check, if the reference is near start or end point
    return 	HitTestPoints( m_ArcStart, relativePosition, MINIMUM_SELECTION_DISTANCE ) ||
              HitTestPoints( m_ArcEnd, relativePosition, MINIMUM_SELECTION_DISTANCE ) ||
              ( crossProductStart <= 0 && crossProductEnd >= 0 );
}


bool LIB_ARC::HitTest( const EDA_RECT& aRect, bool aContained, int aAccuracy ) const
{
    if( m_flags & (STRUCT_DELETED | SKIP_STRUCT ) )
        return false;

    wxPoint  center = DefaultTransform.TransformCoordinate( GetPosition() );
    int      radius = GetRadius();
    int      lineWidth = GetWidth();
    EDA_RECT sel = aRect ;

    if ( aAccuracy )
        sel.Inflate( aAccuracy );

    if( aContained )
        return sel.Contains( GetBoundingBox() );

    EDA_RECT arcRect = GetBoundingBox().Common( sel );

    /* All following tests must pass:
     * 1. Rectangle must intersect arc BoundingBox
     * 2. Rectangle must cross the outside of the arc
     */
    return arcRect.Intersects( sel ) && arcRect.IntersectsCircleEdge( center, radius, lineWidth );
}


EDA_ITEM* LIB_ARC::Clone() const
{
    return new LIB_ARC( *this );
}


int LIB_ARC::compare( const LIB_ITEM& aOther, LIB_ITEM::COMPARE_FLAGS aCompareFlags ) const
{
    wxASSERT( aOther.Type() == LIB_ARC_T );

    int retv = LIB_ITEM::compare( aOther );

    if( retv )
        return retv;

    const LIB_ARC* tmp = ( LIB_ARC* ) &aOther;

    if( m_Pos.x != tmp->m_Pos.x )
        return m_Pos.x - tmp->m_Pos.x;

    if( m_Pos.y != tmp->m_Pos.y )
        return m_Pos.y - tmp->m_Pos.y;

    return 0;
}


void LIB_ARC::Offset( const wxPoint& aOffset )
{
    m_Pos += aOffset;
    m_ArcStart += aOffset;
    m_ArcEnd += aOffset;
}


void LIB_ARC::MoveTo( const wxPoint& aPosition )
{
    wxPoint offset = aPosition - m_Pos;
    m_Pos = aPosition;
    m_ArcStart += offset;
    m_ArcEnd   += offset;
}


void LIB_ARC::MirrorHorizontal( const wxPoint& aCenter )
{
    m_Pos.x -= aCenter.x;
    m_Pos.x *= -1;
    m_Pos.x += aCenter.x;
    m_ArcStart.x -= aCenter.x;
    m_ArcStart.x *= -1;
    m_ArcStart.x += aCenter.x;
    m_ArcEnd.x -= aCenter.x;
    m_ArcEnd.x *= -1;
    m_ArcEnd.x += aCenter.x;
    std::swap( m_ArcStart, m_ArcEnd );
}


void LIB_ARC::MirrorVertical( const wxPoint& aCenter )
{
    m_Pos.y -= aCenter.y;
    m_Pos.y *= -1;
    m_Pos.y += aCenter.y;
    m_ArcStart.y -= aCenter.y;
    m_ArcStart.y *= -1;
    m_ArcStart.y += aCenter.y;
    m_ArcEnd.y -= aCenter.y;
    m_ArcEnd.y *= -1;
    m_ArcEnd.y += aCenter.y;
    std::swap( m_ArcStart, m_ArcEnd );
}


void LIB_ARC::Rotate( const wxPoint& aCenter, bool aRotateCCW )
{
    int rot_angle = aRotateCCW ? -900 : 900;
    RotatePoint( &m_Pos, aCenter, rot_angle );
    RotatePoint( &m_ArcStart, aCenter, rot_angle );
    RotatePoint( &m_ArcEnd, aCenter, rot_angle );
}


void LIB_ARC::Plot( PLOTTER* aPlotter, const wxPoint& aOffset, bool aFill,
                    const TRANSFORM& aTransform ) const
{
    wxASSERT( aPlotter != nullptr );

    wxPoint   center = aTransform.TransformCoordinate( GetCenter() ) + aOffset;
    int       startAngle;
    int       endAngle;
    int       pen_size = GetEffectivePenWidth( aPlotter->RenderSettings() );
    FILL_TYPE fill = aFill ? m_fill : FILL_TYPE::NO_FILL;

    CalcAngles( startAngle, endAngle );
    aTransform.MapAngles( &startAngle, &endAngle );

    if( fill == FILL_TYPE::FILLED_WITH_BG_BODYCOLOR )
    {
        aPlotter->SetColor( aPlotter->RenderSettings()->GetLayerColor( LAYER_DEVICE_BACKGROUND ) );
        aPlotter->Arc( center, -endAngle, -startAngle, GetRadius(), fill, 0 );

        if( pen_size <= 0 )
            return;
        else
            fill = FILL_TYPE::NO_FILL;
    }

    pen_size = std::max( pen_size, aPlotter->RenderSettings()->GetMinPenWidth() );

    aPlotter->SetColor( aPlotter->RenderSettings()->GetLayerColor( LAYER_DEVICE ) );
    aPlotter->Arc( center, -endAngle, -startAngle, GetRadius(), fill, pen_size );
}


int LIB_ARC::GetPenWidth() const
{
    return m_Width;
}


void LIB_ARC::print( const RENDER_SETTINGS* aSettings, const wxPoint& aOffset, void* aData,
                     const TRANSFORM& aTransform )
{
    bool forceNoFill = static_cast<bool>( aData );
    int  penWidth = GetEffectivePenWidth( aSettings );

    if( forceNoFill && m_fill != FILL_TYPE::NO_FILL && penWidth == 0 )
        return;

    wxDC*   DC = aSettings->GetPrintDC();
    wxPoint pos1, pos2, posc;
    COLOR4D color = aSettings->GetLayerColor( LAYER_DEVICE );

    pos1 = aTransform.TransformCoordinate( m_ArcEnd ) + aOffset;
    pos2 = aTransform.TransformCoordinate( m_ArcStart ) + aOffset;
    posc = aTransform.TransformCoordinate( m_Pos ) + aOffset;

    int t1;
    int t2;

    CalcAngles( t1, t2 );
    aTransform.MapAngles( &t1, &t2 );

    if( forceNoFill || m_fill == FILL_TYPE::NO_FILL )
    {
        GRArc1( nullptr, DC, pos1.x, pos1.y, pos2.x, pos2.y, posc.x, posc.y, penWidth, color );
    }
    else
    {
        if( m_fill == FILL_TYPE::FILLED_WITH_BG_BODYCOLOR )
            color = aSettings->GetLayerColor( LAYER_DEVICE_BACKGROUND );

        GRFilledArc( nullptr, DC, posc.x, posc.y, t1, t2, GetRadius(), penWidth, color, color );
    }
}


const EDA_RECT LIB_ARC::GetBoundingBox() const
{
    int      radius = GetRadius();
    int      minX, minY, maxX, maxY, angleStart, angleEnd;
    EDA_RECT rect;
    wxPoint  nullPoint, startPos, endPos, centerPos;
    wxPoint  normStart = m_ArcStart - m_Pos;
    wxPoint  normEnd   = m_ArcEnd - m_Pos;

    if( normStart == nullPoint || normEnd == nullPoint || radius == 0 )
        return rect;

    endPos     = DefaultTransform.TransformCoordinate( m_ArcEnd );
    startPos   = DefaultTransform.TransformCoordinate( m_ArcStart );
    centerPos  = DefaultTransform.TransformCoordinate( m_Pos );

    CalcAngles( angleStart, angleEnd );
    DefaultTransform.MapAngles( &angleStart, &angleEnd );

    /* Start with the start and end point of the arc. */
    minX = std::min( startPos.x, endPos.x );
    minY = std::min( startPos.y, endPos.y );
    maxX = std::max( startPos.x, endPos.x );
    maxY = std::max( startPos.y, endPos.y );

    /* Zero degrees is a special case. */
    if( angleStart == 0 )
        maxX = centerPos.x + radius;

    /* Arc end angle wrapped passed 360. */
    if( angleStart > angleEnd )
        angleEnd += 3600;

    if( angleStart <= 900 && angleEnd >= 900 )          /* 90 deg */
        maxY = centerPos.y + radius;

    if( angleStart <= 1800 && angleEnd >= 1800 )        /* 180 deg */
        minX = centerPos.x - radius;

    if( angleStart <= 2700 && angleEnd >= 2700 )        /* 270 deg */
        minY = centerPos.y - radius;

    if( angleStart <= 3600 && angleEnd >= 3600 )        /* 0 deg   */
        maxX = centerPos.x + radius;

    rect.SetOrigin( minX, minY );
    rect.SetEnd( maxX, maxY );
    rect.Inflate( ( GetPenWidth() / 2 ) + 1 );

    return rect;
}


void LIB_ARC::GetMsgPanelInfo( EDA_DRAW_FRAME* aFrame, std::vector<MSG_PANEL_ITEM>& aList )
{
    wxString msg;
    EDA_RECT bBox = GetBoundingBox();

    LIB_ITEM::GetMsgPanelInfo( aFrame, aList );

    msg = MessageTextFromValue( aFrame->GetUserUnits(), m_Width );

    aList.emplace_back( _( "Line Width" ), msg );

    msg.Printf( wxT( "(%d, %d, %d, %d)" ), bBox.GetOrigin().x,
                bBox.GetOrigin().y, bBox.GetEnd().x, bBox.GetEnd().y );

    aList.emplace_back( _( "Bounding Box" ), msg );
}


wxString LIB_ARC::GetSelectMenuText( EDA_UNITS aUnits ) const
{
    return wxString::Format( _( "Arc, radius %s" ),
                             MessageTextFromValue( aUnits, GetRadius() ) );
}


BITMAPS LIB_ARC::GetMenuImage() const
{
    return BITMAPS::add_arc;
}


void LIB_ARC::BeginEdit( const wxPoint& aPosition )
{
    m_ArcStart  = m_ArcEnd = aPosition;
    m_editState = 1;
}


void LIB_ARC::CalcEdit( const wxPoint& aPosition )
{
#define sq( x ) pow( x, 2 )

    int radius = GetRadius();

    // Edit state 0: drawing: place ArcStart
    // Edit state 1: drawing: place ArcEnd (center calculated for 90-degree subtended angle)
    // Edit state 2: point editing: move ArcStart (center calculated for invariant subtended angle)
    // Edit state 3: point editing: move ArcEnd (center calculated for invariant subtended angle)
    // Edit state 4: point editing: move center

    switch( m_editState )
    {
    case 0:
        m_ArcStart = aPosition;
        m_ArcEnd = aPosition;
        m_Pos = aPosition;
        radius = 0;
        return;

    case 1:
        m_ArcEnd = aPosition;
        radius = KiROUND( sqrt( sq( GetLineLength( m_ArcStart, m_ArcEnd ) ) / 2.0 ) );
        break;

    case 2:
    case 3:
    {
        wxPoint v = m_ArcStart - m_ArcEnd;
        double chordBefore = sq( v.x ) + sq( v.y );

        if( m_editState == 2 )
            m_ArcStart = aPosition;
        else
            m_ArcEnd = aPosition;

        v = m_ArcStart - m_ArcEnd;
        double chordAfter = sq( v.x ) + sq( v.y );
        double ratio = chordAfter / chordBefore;

        if( ratio != 0 )
        {
            radius = std::max( int( sqrt( sq( radius ) * ratio ) ) + 1,
                               int( sqrt( chordAfter ) / 2 ) + 1 );
        }

        break;
    }

    case 4:
    {
        double chordA = GetLineLength( m_ArcStart, aPosition );
        double chordB = GetLineLength( m_ArcEnd, aPosition );
        radius = int( ( chordA + chordB ) / 2.0 ) + 1;
        break;
    }
    }

    // Calculate center based on start, end, and radius
    //
    // Let 'l' be the length of the chord and 'm' the middle point of the chord
    double  l = GetLineLength( m_ArcStart, m_ArcEnd );
    wxPoint m = ( m_ArcStart + m_ArcEnd ) / 2;

    // Calculate 'd', the vector from the chord midpoint to the center
    wxPoint d;
    d.x = KiROUND( sqrt( sq( radius ) - sq( l/2 ) ) * ( m_ArcStart.y - m_ArcEnd.y ) / l );
    d.y = KiROUND( sqrt( sq( radius ) - sq( l/2 ) ) * ( m_ArcEnd.x - m_ArcStart.x ) / l );

    wxPoint c1 = m + d;
    wxPoint c2 = m - d;

    // Solution gives us 2 centers; we need to pick one:
    switch( m_editState )
    {
    case 1:
    {
        // Keep center clockwise from chord while drawing
        wxPoint chordVector = twoPointVector( m_ArcStart, m_ArcEnd );
        double  chordAngle = ArcTangente( chordVector.y, chordVector.x );
        NORMALIZE_ANGLE_POS( chordAngle );

        wxPoint c1Test = c1;
        RotatePoint( &c1Test, m_ArcStart, -chordAngle );

        m_Pos = c1Test.x > 0 ? c2 : c1;
    }
        break;

    case 2:
    case 3:
        // Pick the one closer to the old center
        m_Pos = ( GetLineLength( c1, m_Pos ) < GetLineLength( c2, m_Pos ) ) ? c1 : c2;
        break;

    case 4:
        // Pick the one closer to the mouse position
        m_Pos = ( GetLineLength( c1, aPosition ) < GetLineLength( c2, aPosition ) ) ? c1 : c2;
        break;
    }
}


void LIB_ARC::CalcAngles( int& aStartAngle, int& aEndAngle ) const
{
    wxPoint centerStartVector = twoPointVector( GetCenter(), GetStart() );
    wxPoint centerEndVector   = twoPointVector( GetCenter(), GetEnd() );

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


VECTOR2I LIB_ARC::CalcMidPoint() const
{
    VECTOR2D midPoint;
    int      radius = GetRadius();
    int      t1;
    int      t2;

    CalcAngles( t1, t2 );

    double startAngle = static_cast<double>( t1 ) / 10.0;
    double endAngle = static_cast<double>( t2 ) / 10.0;

    if( endAngle < startAngle )
        endAngle -= 360.0;

    double midPointAngle = ( ( endAngle - startAngle ) / 2.0 ) + startAngle;
    double x = cos( DEG2RAD( midPointAngle ) ) * radius;
    double y = sin( DEG2RAD( midPointAngle ) ) * radius;

    midPoint.x = KiROUND( x ) + m_Pos.x;
    midPoint.y = KiROUND( y ) + m_Pos.y;

    return midPoint;
}

