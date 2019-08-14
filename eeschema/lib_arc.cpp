/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2017 Jean-Pierre Charras, jp.charras at wanadoo.fr
 * Copyright (C) 2004-2019 KiCad Developers, see AUTHORS.txt for contributors.
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

#include <fctsys.h>
#include <gr_basic.h>
#include <macros.h>
#include <sch_draw_panel.h>
#include <plotter.h>
#include <trigo.h>
#include <base_units.h>
#include <msgpanel.h>
#include <bitmaps.h>

#include <general.h>
#include <lib_arc.h>
#include <transform.h>
#include <status_popup.h>

// Helper function
static inline wxPoint twoPointVector( const wxPoint &startPoint, const wxPoint &endPoint )
{
    return endPoint - startPoint;
}


LIB_ARC::LIB_ARC( LIB_PART*      aParent ) : LIB_ITEM( LIB_ARC_T, aParent )
{
    m_Radius        = 0;
    m_t1            = 0;
    m_t2            = 0;
    m_Width         = 0;
    m_Fill          = NO_FILL;
    m_isFillable    = true;
    m_editState     = 0;
}


bool LIB_ARC::HitTest( const wxPoint& aRefPoint, int aAccuracy ) const
{
    int     mindist = std::max( aAccuracy + GetPenSize() / 2, MINIMUM_SELECTION_DISTANCE );
    wxPoint relativePosition = aRefPoint;

    relativePosition.y = -relativePosition.y; // reverse Y axis

    int distance = KiROUND( GetLineLength( m_Pos, relativePosition ) );

    if( abs( distance - m_Radius ) > mindist )
        return false;

    // We are on the circle, ensure we are only on the arc, i.e. between
    //  m_ArcStart and m_ArcEnd

    wxPoint startEndVector = twoPointVector( m_ArcStart, m_ArcEnd);
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
    if( m_Flags & ( STRUCT_DELETED | SKIP_STRUCT ) )
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


int LIB_ARC::compare( const LIB_ITEM& aOther ) const
{
    wxASSERT( aOther.Type() == LIB_ARC_T );

    const LIB_ARC* tmp = ( LIB_ARC* ) &aOther;

    if( m_Pos.x != tmp->m_Pos.x )
        return m_Pos.x - tmp->m_Pos.x;

    if( m_Pos.y != tmp->m_Pos.y )
        return m_Pos.y - tmp->m_Pos.y;

    if( m_t1 != tmp->m_t1 )
        return m_t1 - tmp->m_t1;

    if( m_t2 != tmp->m_t2 )
        return m_t2 - tmp->m_t2;

    return 0;
}


void LIB_ARC::Offset( const wxPoint& aOffset )
{
    m_Pos += aOffset;
    m_ArcStart += aOffset;
    m_ArcEnd += aOffset;
}


bool LIB_ARC::Inside( EDA_RECT& aRect ) const
{
    return aRect.Contains( m_ArcStart.x, -m_ArcStart.y )
        || aRect.Contains( m_ArcEnd.x, -m_ArcEnd.y );
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
    std::swap( m_t1, m_t2 );
    m_t1 = 1800 - m_t1;
    m_t2 = 1800 - m_t2;
    if( m_t1 > 3600 || m_t2 > 3600 )
    {
        m_t1 -= 3600;
        m_t2 -= 3600;
    }
    else if( m_t1 < -3600 || m_t2 < -3600 )
    {
        m_t1 += 3600;
        m_t2 += 3600;
    }
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
    std::swap( m_t1, m_t2 );
    m_t1 = - m_t1;
    m_t2 = - m_t2;
    if( m_t1 > 3600 || m_t2 > 3600 )
    {
        m_t1 -= 3600;
        m_t2 -= 3600;
    }
    else if( m_t1 < -3600 || m_t2 < -3600 )
    {
        m_t1 += 3600;
        m_t2 += 3600;
    }
}

void LIB_ARC::Rotate( const wxPoint& aCenter, bool aRotateCCW )
{
    int rot_angle = aRotateCCW ? -900 : 900;
    RotatePoint( &m_Pos, aCenter, rot_angle );
    RotatePoint( &m_ArcStart, aCenter, rot_angle );
    RotatePoint( &m_ArcEnd, aCenter, rot_angle );
    m_t1 -= rot_angle;
    m_t2 -= rot_angle;
    if( m_t1 > 3600 || m_t2 > 3600 )
    {
        m_t1 -= 3600;
        m_t2 -= 3600;
    }
    else if( m_t1 < -3600 || m_t2 < -3600 )
    {
        m_t1 += 3600;
        m_t2 += 3600;
    }
}



void LIB_ARC::Plot( PLOTTER* aPlotter, const wxPoint& aOffset, bool aFill,
                    const TRANSFORM& aTransform )
{
    wxASSERT( aPlotter != NULL );

    int t1 = m_t1;
    int t2 = m_t2;
    wxPoint pos = aTransform.TransformCoordinate( m_Pos ) + aOffset;

    aTransform.MapAngles( &t1, &t2 );

    if( aFill && m_Fill == FILLED_WITH_BG_BODYCOLOR )
    {
        aPlotter->SetColor( GetLayerColor( LAYER_DEVICE_BACKGROUND ) );
        aPlotter->Arc( pos, -t2, -t1, m_Radius, FILLED_WITH_BG_BODYCOLOR, 0 );
    }

    bool already_filled = m_Fill == FILLED_WITH_BG_BODYCOLOR;
    auto pen_size = GetPenSize();

    if( !already_filled || pen_size > 0 )
    {
        pen_size = std::max( 0, pen_size );
        aPlotter->SetColor( GetLayerColor( LAYER_DEVICE ) );
        aPlotter->Arc( pos, -t2, -t1, m_Radius, already_filled ? NO_FILL : m_Fill, pen_size );
    }
}


int LIB_ARC::GetPenSize() const
{
    if( m_Width > 0 )
        return m_Width;

    if( m_Width == 0 )
       return GetDefaultLineThickness();

    return -1;   // a value to use a minimal pen size
}


void LIB_ARC::print( wxDC* aDC, const wxPoint& aOffset, void* aData, const TRANSFORM& aTransform )
{
    wxPoint pos1, pos2, posc;
    COLOR4D color   = GetLayerColor( LAYER_DEVICE );
    COLOR4D bgColor = GetLayerColor( LAYER_DEVICE_BACKGROUND );

    pos1 = aTransform.TransformCoordinate( m_ArcEnd ) + aOffset;
    pos2 = aTransform.TransformCoordinate( m_ArcStart ) + aOffset;
    posc = aTransform.TransformCoordinate( m_Pos ) + aOffset;
    int  pt1  = m_t1;
    int  pt2  = m_t2;
    bool swap = aTransform.MapAngles( &pt1, &pt2 );

    if( swap )
    {
        std::swap( pos1.x, pos2.x );
        std::swap( pos1.y, pos2.y );
    }

    FILL_T fill = aData ? NO_FILL : m_Fill;

    int penSize = GetPenSize();

    if( fill == FILLED_WITH_BG_BODYCOLOR )
        GRFilledArc( nullptr, aDC, posc.x, posc.y, pt1, pt2, m_Radius, penSize, bgColor, bgColor );
    else if( fill == FILLED_SHAPE && !aData )
        GRFilledArc( nullptr, aDC, posc.x, posc.y, pt1, pt2, m_Radius, color, color );
    else
        GRArc1( nullptr, aDC, pos1.x, pos1.y, pos2.x, pos2.y, posc.x, posc.y, penSize, color );
}


const EDA_RECT LIB_ARC::GetBoundingBox() const
{
    int      minX, minY, maxX, maxY, angleStart, angleEnd;
    EDA_RECT rect;
    wxPoint  nullPoint, startPos, endPos, centerPos;
    wxPoint  normStart = m_ArcStart - m_Pos;
    wxPoint  normEnd   = m_ArcEnd - m_Pos;

    if( ( normStart == nullPoint ) || ( normEnd == nullPoint ) || ( m_Radius == 0 ) )
    {
        wxLogDebug( wxT("Invalid arc drawing definition, center(%d, %d), start(%d, %d), "
                        "end(%d, %d), radius %d" ),
                    m_Pos.x, m_Pos.y, m_ArcStart.x, m_ArcStart.y, m_ArcEnd.x,
                    m_ArcEnd.y, m_Radius );
        return rect;
    }

    endPos     = DefaultTransform.TransformCoordinate( m_ArcEnd );
    startPos   = DefaultTransform.TransformCoordinate( m_ArcStart );
    centerPos  = DefaultTransform.TransformCoordinate( m_Pos );
    angleStart = m_t1;
    angleEnd   = m_t2;

    if( DefaultTransform.MapAngles( &angleStart, &angleEnd ) )
    {
        std::swap( endPos.x, startPos.x );
        std::swap( endPos.y, startPos.y );
    }

    /* Start with the start and end point of the arc. */
    minX = std::min( startPos.x, endPos.x );
    minY = std::min( startPos.y, endPos.y );
    maxX = std::max( startPos.x, endPos.x );
    maxY = std::max( startPos.y, endPos.y );

    /* Zero degrees is a special case. */
    if( angleStart == 0 )
        maxX = centerPos.x + m_Radius;

    /* Arc end angle wrapped passed 360. */
    if( angleStart > angleEnd )
        angleEnd += 3600;

    if( angleStart <= 900 && angleEnd >= 900 )          /* 90 deg */
        maxY = centerPos.y + m_Radius;

    if( angleStart <= 1800 && angleEnd >= 1800 )        /* 180 deg */
        minX = centerPos.x - m_Radius;

    if( angleStart <= 2700 && angleEnd >= 2700 )        /* 270 deg */
        minY = centerPos.y - m_Radius;

    if( angleStart <= 3600 && angleEnd >= 3600 )        /* 0 deg   */
        maxX = centerPos.x + m_Radius;

    rect.SetOrigin( minX, minY );
    rect.SetEnd( maxX, maxY );
    rect.Inflate( ( GetPenSize()+1 ) / 2 );

    return rect;
}


void LIB_ARC::GetMsgPanelInfo( EDA_UNITS_T aUnits, std::vector< MSG_PANEL_ITEM >& aList )
{
    wxString msg;
    EDA_RECT bBox = GetBoundingBox();

    LIB_ITEM::GetMsgPanelInfo( aUnits, aList );

    msg = MessageTextFromValue( aUnits, m_Width, true );

    aList.push_back( MSG_PANEL_ITEM( _( "Line Width" ), msg, BLUE ) );

    msg.Printf( wxT( "(%d, %d, %d, %d)" ), bBox.GetOrigin().x,
                bBox.GetOrigin().y, bBox.GetEnd().x, bBox.GetEnd().y );

    aList.push_back( MSG_PANEL_ITEM( _( "Bounding Box" ), msg, BROWN ) );
}


wxString LIB_ARC::GetSelectMenuText( EDA_UNITS_T aUnits ) const
{
    return wxString::Format( _( "Arc center (%s, %s), radius %s" ),
                             MessageTextFromValue( aUnits, m_Pos.x ),
                             MessageTextFromValue( aUnits, m_Pos.y ),
                             MessageTextFromValue( aUnits, m_Radius ) );
}


BITMAP_DEF LIB_ARC::GetMenuImage() const
{
    return add_arc_xpm;
}


void LIB_ARC::BeginEdit( const wxPoint aPosition )
{
    m_ArcStart  = m_ArcEnd = aPosition;
    m_editState = 1;
}


void LIB_ARC::CalcEdit( const wxPoint& aPosition )
{
#define sq( x ) pow( x, 2 )

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
        m_Radius = 0;
        m_t1 = 0;
        m_t2 = 0;
        return;

    case 1:
        m_ArcEnd = aPosition;
        m_Radius = KiROUND( sqrt( pow( GetLineLength( m_ArcStart, m_ArcEnd ), 2 ) / 2.0 ) );
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

        if( ratio > 0 )
        {
            m_Radius = int( sqrt( m_Radius * m_Radius * ratio ) ) + 1;
            m_Radius = std::max( m_Radius, int( sqrt( chordAfter ) / 2 ) + 1 );
        }

        break;
    }

    case 4:
    {
        double chordA = GetLineLength( m_ArcStart, aPosition );
        double chordB = GetLineLength( m_ArcEnd, aPosition );
        m_Radius = int( ( chordA + chordB ) / 2.0 ) + 1;
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
    d.x = KiROUND( sqrt( sq( m_Radius ) - sq( l/2 ) ) * ( m_ArcStart.y - m_ArcEnd.y ) / l );
    d.y = KiROUND( sqrt( sq( m_Radius ) - sq( l/2 ) ) * ( m_ArcEnd.x - m_ArcStart.x ) / l );

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

    CalcRadiusAngles();
}


void LIB_ARC::CalcRadiusAngles()
{
    wxPoint centerStartVector = twoPointVector( m_Pos, m_ArcStart );
    wxPoint centerEndVector   = twoPointVector( m_Pos, m_ArcEnd );

    m_Radius = KiROUND( EuclideanNorm( centerStartVector ) );

    // Angles in eeschema are still integers
    m_t1 = KiROUND( ArcTangente( centerStartVector.y, centerStartVector.x ) );
    m_t2 = KiROUND( ArcTangente( centerEndVector.y, centerEndVector.x ) );

    NORMALIZE_ANGLE_POS( m_t1 );
    NORMALIZE_ANGLE_POS( m_t2 );  // angles = 0 .. 3600

    // Restrict angle to less than 180 to avoid PBS display mirror Trace because it is
    // assumed that the arc is less than 180 deg to find orientation after rotate or mirror.
    if( (m_t2 - m_t1) > 1800 )
        m_t2 -= 3600;
    else if( (m_t2 - m_t1) <= -1800 )
        m_t2 += 3600;

    while( (m_t2 - m_t1) >= 1800 )
    {
        m_t2--;
        m_t1++;
    }

    while( (m_t1 - m_t2) >= 1800 )
    {
        m_t2++;
        m_t1--;
    }

    NORMALIZE_ANGLE_POS( m_t1 );

    if( !IsMoving() )
        NORMALIZE_ANGLE_POS( m_t2 );
}
