/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2004 Jean-Pierre Charras, jaen-pierre.charras@gipsa-lab.inpg.com
 * Copyright (C) 2004-2015 KiCad Developers, see change_log.txt for contributors.
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

/**
 * @file lib_arc.cpp
 */

#include <fctsys.h>
#include <gr_basic.h>
#include <macros.h>
#include <class_drawpanel.h>
#include <plot_common.h>
#include <trigo.h>
#include <wxstruct.h>
#include <richio.h>
#include <base_units.h>
#include <msgpanel.h>

#include <general.h>
#include <lib_arc.h>
#include <transform.h>

// Helper function
static inline wxPoint twoPointVector( const wxPoint &startPoint, const wxPoint &endPoint )
{
    return endPoint - startPoint;
}


//! @brief Given three points A B C, compute the circumcenter of the resulting triangle
//! reference: http://en.wikipedia.org/wiki/Circumscribed_circle
//! Coordinates of circumcenter in Cartesian coordinates
static wxPoint calcCenter( const wxPoint& A, const wxPoint& B, const wxPoint& C )
{
    double  circumCenterX, circumCenterY;
    double  Ax = (double) A.x;
    double  Ay = (double) A.y;
    double  Bx = (double) B.x;
    double  By = (double) B.y;
    double  Cx = (double) C.x;
    double  Cy = (double) C.y;

    wxPoint circumCenter;

    double  D = 2.0 * ( Ax * ( By - Cy ) + Bx * ( Cy - Ay ) + Cx * ( Ay - By ) );

    // prevent division / 0
    if( fabs( D ) < 1e-7 )
        D = 1e-7;

    circumCenterX = ( (Ay * Ay + Ax * Ax) * (By - Cy) +
                      (By * By + Bx * Bx) * (Cy - Ay) +
                      (Cy * Cy + Cx * Cx) * (Ay - By) ) / D;

    circumCenterY = ( (Ay * Ay + Ax * Ax) * (Cx - Bx) +
                      (By * By + Bx * Bx) * (Ax - Cx) +
                      (Cy * Cy + Cx * Cx) * (Bx - Ax) ) / D;

    circumCenter.x = (int) circumCenterX;
    circumCenter.y = (int) circumCenterY;

    return circumCenter;
}


LIB_ARC::LIB_ARC( LIB_PART*      aParent ) : LIB_ITEM( LIB_ARC_T, aParent )
{
    m_Radius        = 0;
    m_t1            = 0;
    m_t2            = 0;
    m_Width         = 0;
    m_Fill          = NO_FILL;
    m_isFillable    = true;
    m_typeName      = _( "Arc" );
    m_editState     = 0;
    m_lastEditState = 0;
    m_editCenterDistance = 0.0;
    m_editSelectPoint = ARC_STATUS_START;
    m_editDirection = 0;
}


bool LIB_ARC::Save( OUTPUTFORMATTER& aFormatter )
{
    int x1 = m_t1;

    if( x1 > 1800 )
        x1 -= 3600;

    int x2 = m_t2;

    if( x2 > 1800 )
        x2 -= 3600;

    aFormatter.Print( 0, "A %d %d %d %d %d %d %d %d %c %d %d %d %d\n",
                      m_Pos.x, m_Pos.y, m_Radius, x1, x2, m_Unit, m_Convert, m_Width,
                      fill_tab[m_Fill], m_ArcStart.x, m_ArcStart.y, m_ArcEnd.x,
                      m_ArcEnd.y );

    return true;
}


bool LIB_ARC::Load( LINE_READER& aLineReader, wxString& aErrorMsg )
{
    int startx, starty, endx, endy, cnt;
    char tmp[256] = "";
    char* line = (char*) aLineReader;

    cnt = sscanf( line + 2, "%d %d %d %d %d %d %d %d %255s %d %d %d %d",
                  &m_Pos.x, &m_Pos.y, &m_Radius, &m_t1, &m_t2, &m_Unit,
                  &m_Convert, &m_Width, tmp, &startx, &starty, &endx, &endy );
    if( cnt < 8 )
    {
        aErrorMsg.Printf( _( "Arc only had %d parameters of the required 8" ), cnt );
        return false;
    }

    if( tmp[0] == 'F' )
        m_Fill = FILLED_SHAPE;

    if( tmp[0] == 'f' )
        m_Fill = FILLED_WITH_BG_BODYCOLOR;

    NORMALIZE_ANGLE_POS( m_t1 );
    NORMALIZE_ANGLE_POS( m_t2 );

    // Actual Coordinates of arc ends are read from file
    if( cnt >= 13 )
    {
        m_ArcStart.x = startx;
        m_ArcStart.y = starty;
        m_ArcEnd.x   = endx;
        m_ArcEnd.y   = endy;
    }
    else
    {
        // Actual Coordinates of arc ends are not read from file
        // (old library), calculate them
        m_ArcStart.x = m_Radius;
        m_ArcStart.y = 0;
        m_ArcEnd.x   = m_Radius;
        m_ArcEnd.y   = 0;
        RotatePoint( &m_ArcStart.x, &m_ArcStart.y, -m_t1 );
        m_ArcStart.x += m_Pos.x;
        m_ArcStart.y += m_Pos.y;
        RotatePoint( &m_ArcEnd.x, &m_ArcEnd.y, -m_t2 );
        m_ArcEnd.x += m_Pos.x;
        m_ArcEnd.y += m_Pos.y;
    }

    return true;
}


bool LIB_ARC::HitTest( const wxPoint& aRefPoint ) const
{
    int mindist = GetPenSize() / 2;

    // Have a minimal tolerance for hit test
    if( mindist < MINIMUM_SELECTION_DISTANCE )
        mindist = MINIMUM_SELECTION_DISTANCE;

    return HitTest( aRefPoint, mindist, DefaultTransform );
}


bool LIB_ARC::HitTest( const wxPoint &aPosition, int aThreshold, const TRANSFORM& aTransform ) const
{

    if( aThreshold < 0 )
        aThreshold = GetPenSize() / 2;

    // TODO: use aTransMat to calculates parameters
    wxPoint relativePosition = aPosition;

    NEGATE( relativePosition.y );       // reverse Y axis

    int distance = KiROUND( GetLineLength( m_Pos, relativePosition ) );

    if( abs( distance - m_Radius ) > aThreshold )
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
        EXCHG( crossProductStart, crossProductEnd );
    }

    // When the cross products have a different sign, the point lies in sector
    // also check, if the reference is near start or end point
    return 	HitTestPoints( m_ArcStart, relativePosition, MINIMUM_SELECTION_DISTANCE ) ||
            HitTestPoints( m_ArcEnd, relativePosition, MINIMUM_SELECTION_DISTANCE ) ||
            ( crossProductStart <= 0 && crossProductEnd >= 0 );
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


void LIB_ARC::SetOffset( const wxPoint& aOffset )
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


void LIB_ARC::Move( const wxPoint& aPosition )
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
    EXCHG( m_ArcStart, m_ArcEnd );
    EXCHG( m_t1, m_t2 );
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
    EXCHG( m_ArcStart, m_ArcEnd );
    EXCHG( m_t1, m_t2 );
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
        aPlotter->Arc( pos, -t2, -t1, m_Radius, FILLED_SHAPE, 0 );
    }

    bool already_filled = m_Fill == FILLED_WITH_BG_BODYCOLOR;
    aPlotter->SetColor( GetLayerColor( LAYER_DEVICE ) );
    aPlotter->Arc( pos, -t2, -t1, m_Radius, already_filled ? NO_FILL : m_Fill, GetPenSize() );
}


int LIB_ARC::GetPenSize() const
{
    return ( m_Width == 0 ) ? GetDefaultLineThickness() : m_Width;
}


void LIB_ARC::drawEditGraphics( EDA_RECT* aClipBox, wxDC* aDC, EDA_COLOR_T aColor )
{
    // The edit indicators only get drawn when a new arc is being drawn.
    if( !IsNew() )
        return;

    // Use the last edit state so when the drawing switches from the end mode to the center
    // point mode, the last line between the center points gets erased.
    if( m_lastEditState == 1 )
    {
        GRLine( aClipBox, aDC, m_ArcStart.x, -m_ArcStart.y, m_ArcEnd.x, -m_ArcEnd.y, 0, aColor );
    }
    else
    {
        GRDashedLine( aClipBox, aDC, m_ArcStart.x, -m_ArcStart.y, m_Pos.x, -m_Pos.y, 0, aColor );
        GRDashedLine( aClipBox, aDC, m_ArcEnd.x, -m_ArcEnd.y, m_Pos.x, -m_Pos.y, 0, aColor );
    }
}


void LIB_ARC::drawGraphic( EDA_DRAW_PANEL* aPanel, wxDC* aDC, const wxPoint& aOffset,
                           EDA_COLOR_T aColor, GR_DRAWMODE aDrawMode, void* aData,
                           const TRANSFORM& aTransform )
{
    // Don't draw the arc until the end point is selected.  Only the edit indicators
    // get drawn at this time.
    if( IsNew() && m_lastEditState == 1 )
        return;

    wxPoint pos1, pos2, posc;
    EDA_COLOR_T color = GetLayerColor( LAYER_DEVICE );

    if( aColor < 0 )       // Used normal color or selected color
    {
        if( IsSelected() )
            color = GetItemSelectedColor();
    }
    else
    {
        color = aColor;
    }

    pos1 = aTransform.TransformCoordinate( m_ArcEnd ) + aOffset;
    pos2 = aTransform.TransformCoordinate( m_ArcStart ) + aOffset;
    posc = aTransform.TransformCoordinate( m_Pos ) + aOffset;
    int  pt1  = m_t1;
    int  pt2  = m_t2;
    bool swap = aTransform.MapAngles( &pt1, &pt2 );

    if( swap )
    {
        EXCHG( pos1.x, pos2.x );
        EXCHG( pos1.y, pos2.y );
    }

    GRSetDrawMode( aDC, aDrawMode );

    FILL_T fill = aData ? NO_FILL : m_Fill;

    if( aColor >= 0 )
        fill = NO_FILL;

    EDA_RECT* const clipbox  = aPanel? aPanel->GetClipBox() : NULL;

    if( fill == FILLED_WITH_BG_BODYCOLOR )
    {
        GRFilledArc( clipbox, aDC, posc.x, posc.y, pt1, pt2,
                     m_Radius, GetPenSize( ),
                     (m_Flags & IS_MOVED) ? color : GetLayerColor( LAYER_DEVICE_BACKGROUND ),
                     GetLayerColor( LAYER_DEVICE_BACKGROUND ) );
    }
    else if( fill == FILLED_SHAPE && !aData )
    {
        GRFilledArc( clipbox, aDC, posc.x, posc.y, pt1, pt2, m_Radius,
                     color, color );
    }
    else
    {

#ifdef DRAW_ARC_WITH_ANGLE

        GRArc( clipbox, aDC, posc.x, posc.y, pt1, pt2, m_Radius,
               GetPenSize(), color );
#else

        GRArc1( clipbox, aDC, pos1.x, pos1.y, pos2.x, pos2.y,
                posc.x, posc.y, GetPenSize(), color );
#endif
    }

    /* Set to one (1) to draw bounding box around arc to validate bounding box
     * calculation. */
#if 0
    EDA_RECT bBox = GetBoundingBox();
    bBox.RevertYAxis();
    bBox = aTransform.TransformCoordinate( bBox );
    bBox.Move( aOffset );
    GRRect( clipbox, aDC, bBox, 0, LIGHTMAGENTA );
#endif
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
        wxLogDebug( wxT("Invalid arc drawing definition, center(%d, %d) \
start(%d, %d), end(%d, %d), radius %d" ),
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
        EXCHG( endPos.x, startPos.x );
        EXCHG( endPos.y, startPos.y );
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


void LIB_ARC::GetMsgPanelInfo( std::vector< MSG_PANEL_ITEM >& aList )
{
    wxString msg;
    EDA_RECT bBox = GetBoundingBox();

    LIB_ITEM::GetMsgPanelInfo( aList );

    msg = StringFromValue( g_UserUnit, m_Width, true );

    aList.push_back( MSG_PANEL_ITEM( _( "Line Width" ), msg, BLUE ) );

    msg.Printf( wxT( "(%d, %d, %d, %d)" ), bBox.GetOrigin().x,
                bBox.GetOrigin().y, bBox.GetEnd().x, bBox.GetEnd().y );

    aList.push_back( MSG_PANEL_ITEM( _( "Bounding Box" ), msg, BROWN ) );
}


wxString LIB_ARC::GetSelectMenuText() const
{
    return wxString::Format( _( "Arc center (%s, %s), radius %s" ),
                             GetChars( CoordinateToString( m_Pos.x ) ),
                             GetChars( CoordinateToString( m_Pos.y ) ),
                             GetChars( CoordinateToString( m_Radius ) ) );
}


void LIB_ARC::BeginEdit( STATUS_FLAGS aEditMode, const wxPoint aPosition )
{
    wxCHECK_RET( ( aEditMode & ( IS_NEW | IS_MOVED | IS_RESIZED ) ) != 0,
                 wxT( "Invalid edit mode for LIB_ARC object." ) );

    if( aEditMode == IS_NEW )
    {
        m_ArcStart  = m_ArcEnd = aPosition;
        m_editState = m_lastEditState = 1;
    }
    else if( aEditMode == IS_MOVED )
    {
        m_initialPos = m_Pos;
        m_initialCursorPos = aPosition;
        SetEraseLastDrawItem();
    }
    else
    {
        // The arc center point has to be rotated with while adjusting the
        // start or end point, determine the side of this point and the distance
        // from the start / end point
        wxPoint middlePoint = wxPoint( (m_ArcStart.x + m_ArcEnd.x) / 2,
                                       (m_ArcStart.y + m_ArcEnd.y) / 2 );
        wxPoint centerVector   = m_Pos - middlePoint;
        wxPoint startEndVector = twoPointVector( m_ArcStart, m_ArcEnd );
        m_editCenterDistance = EuclideanNorm( centerVector );

        // Determine on which side is the center point
        m_editDirection = CrossProduct( startEndVector, centerVector ) ? 1 : -1;

        // Drag either the start, end point or the outline
        if( HitTestPoints( m_ArcStart, aPosition, MINIMUM_SELECTION_DISTANCE ) )
        {
            m_editSelectPoint = ARC_STATUS_START;
        }
        else if( HitTestPoints( m_ArcEnd, aPosition, MINIMUM_SELECTION_DISTANCE ) )
        {
            m_editSelectPoint = ARC_STATUS_END;
        }
        else
        {
            m_editSelectPoint = ARC_STATUS_OUTLINE;
        }

        m_editState = 0;
        SetEraseLastDrawItem();
    }

    m_Flags = aEditMode;
}


bool LIB_ARC::ContinueEdit( const wxPoint aPosition )
{
    wxCHECK_MSG( ( m_Flags & ( IS_NEW | IS_MOVED | IS_RESIZED ) ) != 0, false,
                   wxT( "Bad call to ContinueEdit().  LIB_ARC is not being edited." ) );

    if( m_Flags == IS_NEW )
    {
        if( m_editState == 1 )        // Second position yields the arc segment length.
        {
            m_ArcEnd = aPosition;
            m_editState = 2;
            SetEraseLastDrawItem( false );
            return true;              // Need third position to calculate center point.
        }
    }

    return false;
}


void LIB_ARC::EndEdit( const wxPoint& aPosition, bool aAbort )
{
    wxCHECK_RET( ( m_Flags & ( IS_NEW | IS_MOVED | IS_RESIZED ) ) != 0,
                   wxT( "Bad call to EndEdit().  LIB_ARC is not being edited." ) );

    SetEraseLastDrawItem( false );
    m_lastEditState = 0;
    m_editState = 0;
    m_Flags = 0;
}


void LIB_ARC::calcEdit( const wxPoint& aPosition )
{
    if( m_Flags == IS_RESIZED )
    {
        wxPoint newCenterPoint, startPos, endPos;

        // Choose the point of the arc to be adjusted
        if( m_editSelectPoint == ARC_STATUS_START )
        {
            startPos = aPosition;
            endPos   = m_ArcEnd;
        }
        else if( m_editSelectPoint == ARC_STATUS_END )
        {
            endPos   = aPosition;
            startPos = m_ArcStart;
        }
        else
        {
            // Use the cursor for adjusting the arc curvature
            startPos = m_ArcStart;
            endPos   = m_ArcEnd;

            // If the distance is too small, use the old center point
            // else the new center point is calculated over the three points start/end/cursor
            if( DistanceLinePoint( startPos, endPos, aPosition ) > MINIMUM_SELECTION_DISTANCE )
            {
                newCenterPoint = calcCenter( startPos, aPosition, endPos );
            }
            else
            {
                newCenterPoint = m_Pos;
            }

            // Determine if the arc angle is larger than 180 degrees -> this happens if both
            // points (cursor position, center point) lie on the same side of the vector
            // start-end
            double  crossA = CrossProduct( twoPointVector( startPos, endPos ),
                                        twoPointVector( endPos, aPosition ) );
            double  crossB = CrossProduct( twoPointVector( startPos, endPos ),
                                        twoPointVector( endPos, newCenterPoint ) );

            if( ( crossA < 0 && crossB < 0 ) || ( crossA >= 0 && crossB >= 0 ) )
                newCenterPoint = m_Pos;
        }

        if( m_editSelectPoint == ARC_STATUS_START || m_editSelectPoint == ARC_STATUS_END )
        {
            // Compute the new center point when the start/end points are modified
            wxPoint middlePoint = wxPoint( (startPos.x + endPos.x) / 2,
                                           (startPos.y + endPos.y) / 2 );

            wxPoint startEndVector = twoPointVector( startPos, endPos );
            wxPoint perpendicularVector = wxPoint( -startEndVector.y, startEndVector.x );
            double  lengthPerpendicularVector = EuclideanNorm( perpendicularVector );

            // prevent too large values, division / 0
            if( lengthPerpendicularVector < 1e-1 )
                lengthPerpendicularVector = 1e-1;

            perpendicularVector.x = (int) ( (double) perpendicularVector.x *
                                            m_editCenterDistance /
                                            lengthPerpendicularVector ) * m_editDirection;
            perpendicularVector.y = (int) ( (double) perpendicularVector.y *
                                            m_editCenterDistance /
                                            lengthPerpendicularVector ) * m_editDirection;

            newCenterPoint = middlePoint + perpendicularVector;

            m_ArcStart = startPos;
            m_ArcEnd   = endPos;
        }

        m_Pos = newCenterPoint;
        calcRadiusAngles();
    }
    else if( m_Flags == IS_NEW )
    {
        if( m_editState == 1 )
        {
            m_ArcEnd = aPosition;
        }

        if( m_editState != m_lastEditState )
            m_lastEditState = m_editState;

        // Keep the arc center point up to date.  Otherwise, there will be edit graphic
        // artifacts left behind from the initial draw.
        int dx, dy;
        int cX, cY;
        double angle;

        cX = aPosition.x;
        cY = aPosition.y;

        dx = m_ArcEnd.x - m_ArcStart.x;
        dy = m_ArcEnd.y - m_ArcStart.y;
        cX -= m_ArcStart.x;
        cY -= m_ArcStart.y;
        angle = ArcTangente( dy, dx );
        RotatePoint( &dx, &dy, angle );     /* The segment dx, dy is horizontal
                                             * -> Length = dx, dy = 0 */
        RotatePoint( &cX, &cY, angle );
        cX = dx / 2;           /* cX, cY is on the median segment 0.0 a dx, 0 */

        RotatePoint( &cX, &cY, -angle );
        cX += m_ArcStart.x;
        cY += m_ArcStart.y;
        m_Pos.x = cX;
        m_Pos.y = cY;
        calcRadiusAngles();

        SetEraseLastDrawItem();
    }
    else if( m_Flags == IS_MOVED )
    {
        Move( m_initialPos + aPosition - m_initialCursorPos );
    }
}


void LIB_ARC::calcRadiusAngles()
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
