/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2012 Jean-Pierre Charras, jp.charras at wanadoo.fr
 * Copyright (C) 2004-2011 KiCad Developers, see change_log.txt for contributors.
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
 * @file lib_polyline.cpp
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
#include <lib_polyline.h>
#include <transform.h>

#include <boost/foreach.hpp>


LIB_POLYLINE::LIB_POLYLINE( LIB_PART*      aParent ) :
    LIB_ITEM( LIB_POLYLINE_T, aParent )
{
    m_Fill  = NO_FILL;
    m_Width = 0;
    m_isFillable = true;
    m_typeName   = _( "PolyLine" );
}


bool LIB_POLYLINE::Save( OUTPUTFORMATTER& aFormatter )
{
    int ccount = GetCornerCount();

    aFormatter.Print( 0, "P %d %d %d %d", ccount, m_Unit, m_Convert, m_Width );

    for( unsigned i = 0; i < GetCornerCount(); i++ )
    {
        aFormatter.Print( 0, "  %d %d", m_PolyPoints[i].x, m_PolyPoints[i].y );
    }

    aFormatter.Print( 0, " %c\n", fill_tab[m_Fill] );

    return true;
}


bool LIB_POLYLINE::Load( LINE_READER& aLineReader, wxString& aErrorMsg )
{
    char*   p;
    int     i, ccount = 0;
    wxPoint pt;
    char*   line = (char*) aLineReader;

    i = sscanf( line + 2, "%d %d %d %d", &ccount, &m_Unit, &m_Convert, &m_Width );

    m_Fill = NO_FILL;

    if( i < 4 )
    {
        aErrorMsg.Printf( _( "Polyline only had %d parameters of the required 4" ), i );
        return false;
    }

    if( ccount <= 0 )
    {
        aErrorMsg.Printf( _( "Polyline count parameter %d is invalid" ), ccount );
        return false;
    }

    p = strtok( line + 2, " \t\n" );
    p = strtok( NULL, " \t\n" );
    p = strtok( NULL, " \t\n" );
    p = strtok( NULL, " \t\n" );

    for( i = 0; i < ccount; i++ )
    {
        wxPoint point;
        p = strtok( NULL, " \t\n" );

        if( p == NULL || sscanf( p, "%d", &pt.x ) != 1 )
        {
            aErrorMsg.Printf( _( "Polyline point %d X position not defined" ), i );
            return false;
        }

        p = strtok( NULL, " \t\n" );

        if( p == NULL || sscanf( p, "%d", &pt.y ) != 1 )
        {
            aErrorMsg.Printf( _( "Polyline point %d Y position not defined" ), i );
            return false;
        }

        AddPoint( pt );
    }

    if( ( p = strtok( NULL, " \t\n" ) ) != NULL )
    {
        if( p[0] == 'F' )
            m_Fill = FILLED_SHAPE;

        if( p[0] == 'f' )
            m_Fill = FILLED_WITH_BG_BODYCOLOR;
    }

    return true;
}


EDA_ITEM* LIB_POLYLINE::Clone() const
{
    return new LIB_POLYLINE( *this );
}


int LIB_POLYLINE::compare( const LIB_ITEM& aOther ) const
{
    wxASSERT( aOther.Type() == LIB_POLYLINE_T );

    const LIB_POLYLINE* tmp = (LIB_POLYLINE*) &aOther;

    if( m_PolyPoints.size() != tmp->m_PolyPoints.size() )
        return m_PolyPoints.size() - tmp->m_PolyPoints.size();

    for( size_t i = 0; i < m_PolyPoints.size(); i++ )
    {
        if( m_PolyPoints[i].x != tmp->m_PolyPoints[i].x )
            return m_PolyPoints[i].x - tmp->m_PolyPoints[i].x;

        if( m_PolyPoints[i].y != tmp->m_PolyPoints[i].y )
            return m_PolyPoints[i].y - tmp->m_PolyPoints[i].y;
    }

    return 0;
}


void LIB_POLYLINE::SetOffset( const wxPoint& aOffset )
{
    for( size_t i = 0; i < m_PolyPoints.size(); i++ )
        m_PolyPoints[i] += aOffset;
}


bool LIB_POLYLINE::Inside( EDA_RECT& aRect ) const
{
    for( size_t i = 0; i < m_PolyPoints.size(); i++ )
    {
        if( aRect.Contains( m_PolyPoints[i].x, -m_PolyPoints[i].y ) )
            return true;
    }

    return false;
}


void LIB_POLYLINE::Move( const wxPoint& aPosition )
{
    SetOffset( aPosition - m_PolyPoints[0] );
}


void LIB_POLYLINE::MirrorHorizontal( const wxPoint& aCenter )
{
    size_t i, imax = m_PolyPoints.size();

    for( i = 0; i < imax; i++ )
    {
        m_PolyPoints[i].x -= aCenter.x;
        m_PolyPoints[i].x *= -1;
        m_PolyPoints[i].x += aCenter.x;
    }
}

void LIB_POLYLINE::MirrorVertical( const wxPoint& aCenter )
{
    size_t i, imax = m_PolyPoints.size();

    for( i = 0; i < imax; i++ )
    {
        m_PolyPoints[i].y -= aCenter.y;
        m_PolyPoints[i].y *= -1;
        m_PolyPoints[i].y += aCenter.y;
    }
}

void LIB_POLYLINE::Rotate( const wxPoint& aCenter, bool aRotateCCW )
{
    int rot_angle = aRotateCCW ? -900 : 900;

    size_t i, imax = m_PolyPoints.size();

    for( i = 0; i < imax; i++ )
    {
        RotatePoint( &m_PolyPoints[i], aCenter, rot_angle );
   }
}


void LIB_POLYLINE::Plot( PLOTTER* aPlotter, const wxPoint& aOffset, bool aFill,
                         const TRANSFORM& aTransform )
{
    wxASSERT( aPlotter != NULL );

    static std::vector< wxPoint > cornerList;
    cornerList.clear();

    for( unsigned ii = 0; ii < m_PolyPoints.size(); ii++ )
    {
        wxPoint pos = m_PolyPoints[ii];
        pos = aTransform.TransformCoordinate( pos ) + aOffset;
        cornerList.push_back( pos );
    }

    if( aFill && m_Fill == FILLED_WITH_BG_BODYCOLOR )
    {
        aPlotter->SetColor( GetLayerColor( LAYER_DEVICE_BACKGROUND ) );
        aPlotter->PlotPoly( cornerList, FILLED_WITH_BG_BODYCOLOR, 0 );
        aFill = false;  // body is now filled, do not fill it later.
    }

    bool already_filled = m_Fill == FILLED_WITH_BG_BODYCOLOR;
    aPlotter->SetColor( GetLayerColor( LAYER_DEVICE ) );
    aPlotter->PlotPoly( cornerList, already_filled ? NO_FILL : m_Fill, GetPenSize() );
}


void LIB_POLYLINE::AddPoint( const wxPoint& point )
{
    m_PolyPoints.push_back( point );
}


int LIB_POLYLINE::GetPenSize() const
{
    return ( m_Width == 0 ) ? GetDefaultLineThickness() : m_Width;
}


void LIB_POLYLINE::drawGraphic( EDA_DRAW_PANEL* aPanel, wxDC* aDC, const wxPoint& aOffset,
                                EDA_COLOR_T aColor, GR_DRAWMODE aDrawMode, void* aData,
                                const TRANSFORM& aTransform )
{
    wxPoint  pos1;
    EDA_COLOR_T color = GetLayerColor( LAYER_DEVICE );
    wxPoint* buffer = NULL;

    if( aColor < 0 )                // Used normal color or selected color
    {
        if( IsSelected() )
            color = GetItemSelectedColor();
    }
    else
    {
        color = aColor;
    }

    buffer = new wxPoint[ m_PolyPoints.size() ];

    for( unsigned ii = 0; ii < m_PolyPoints.size(); ii++ )
    {
        buffer[ii] = aTransform.TransformCoordinate( m_PolyPoints[ii] ) + aOffset;
    }

    FILL_T fill = aData ? NO_FILL : m_Fill;

    if( aColor >= 0 )
        fill = NO_FILL;

    GRSetDrawMode( aDC, aDrawMode );

    EDA_RECT* const clipbox  = aPanel? aPanel->GetClipBox() : NULL;
    if( fill == FILLED_WITH_BG_BODYCOLOR )
        GRPoly( clipbox, aDC, m_PolyPoints.size(), buffer, 1, GetPenSize(),
                (m_Flags & IS_MOVED) ? color : GetLayerColor( LAYER_DEVICE_BACKGROUND ),
                GetLayerColor( LAYER_DEVICE_BACKGROUND ) );
    else if( fill == FILLED_SHAPE  )
        GRPoly( clipbox, aDC, m_PolyPoints.size(), buffer, 1, GetPenSize(),
                color, color );
    else
        GRPoly( clipbox, aDC, m_PolyPoints.size(), buffer, 0, GetPenSize(),
                color, color );

    delete[] buffer;

    /* Set to one (1) to draw bounding box around polyline to validate
     * bounding box calculation. */
#if 0
    EDA_RECT bBox = GetBoundingBox();
    bBox.Inflate( m_Thickness + 1, m_Thickness + 1 );
    GRRect( clipbox, aDC, bBox.GetOrigin().x, bBox.GetOrigin().y,
            bBox.GetEnd().x, bBox.GetEnd().y, 0, LIGHTMAGENTA );
#endif
}


bool LIB_POLYLINE::HitTest( const wxPoint& aPosition ) const
{
    int mindist = GetPenSize() / 2;

    // Have a minimal tolerance for hit test
    if( mindist < MINIMUM_SELECTION_DISTANCE )
        mindist = MINIMUM_SELECTION_DISTANCE;

    return HitTest( aPosition, mindist, DefaultTransform );
}


bool LIB_POLYLINE::HitTest( const wxPoint &aPosition, int aThreshold, const TRANSFORM& aTransform ) const
{
    wxPoint ref, start, end;

    if( aThreshold < 0 )
        aThreshold = GetPenSize() / 2;

    for( unsigned ii = 1; ii < GetCornerCount(); ii++ )
    {
        start = aTransform.TransformCoordinate( m_PolyPoints[ii - 1] );
        end   = aTransform.TransformCoordinate( m_PolyPoints[ii] );

        if( TestSegmentHit( aPosition, start, end, aThreshold ) )
            return true;
    }

    return false;
}


const EDA_RECT LIB_POLYLINE::GetBoundingBox() const
{
    EDA_RECT rect;
    int      xmin, xmax, ymin, ymax;

    xmin = xmax = m_PolyPoints[0].x;
    ymin = ymax = m_PolyPoints[0].y;

    for( unsigned ii = 1; ii < GetCornerCount(); ii++ )
    {
        xmin = std::min( xmin, m_PolyPoints[ii].x );
        xmax = std::max( xmax, m_PolyPoints[ii].x );
        ymin = std::min( ymin, m_PolyPoints[ii].y );
        ymax = std::max( ymax, m_PolyPoints[ii].y );
    }

    rect.SetOrigin( xmin, ymin * -1 );
    rect.SetEnd( xmax, ymax * -1 );
    rect.Inflate( m_Width / 2, m_Width / 2 );

    return rect;
}


void LIB_POLYLINE::DeleteSegment( const wxPoint aPosition )
{
    // First segment is kept, only its end point is changed
    while( GetCornerCount() > 2 )
    {
        m_PolyPoints.pop_back();

        if( m_PolyPoints[ GetCornerCount() - 1 ] != aPosition )
        {
            m_PolyPoints[ GetCornerCount() - 1 ] = aPosition;
            break;
        }
    }
}


void LIB_POLYLINE::GetMsgPanelInfo( MSG_PANEL_ITEMS& aList )
{
    wxString msg;
    EDA_RECT bBox = GetBoundingBox();

    LIB_ITEM::GetMsgPanelInfo( aList );

    msg = StringFromValue( g_UserUnit, m_Width, true );

    aList.push_back( MSG_PANEL_ITEM( _( "Line width" ), msg, BLUE ) );

    msg.Printf( wxT( "(%d, %d, %d, %d)" ), bBox.GetOrigin().x,
                bBox.GetOrigin().y, bBox.GetEnd().x, bBox.GetEnd().y );

    aList.push_back( MSG_PANEL_ITEM( _( "Bounding box" ), msg, BROWN ) );
}


wxString LIB_POLYLINE::GetSelectMenuText() const
{
    return wxString::Format( _( "Polyline at (%s, %s) with %zu points" ),
                             GetChars( CoordinateToString( m_PolyPoints[0].x ) ),
                             GetChars( CoordinateToString( m_PolyPoints[0].y ) ),
                             m_PolyPoints.size() );
}


void LIB_POLYLINE::BeginEdit( STATUS_FLAGS aEditMode, const wxPoint aPosition )
{
    wxCHECK_RET( ( aEditMode & ( IS_NEW | IS_MOVED | IS_RESIZED ) ) != 0,
                 wxT( "Invalid edit mode for LIB_POLYLINE object." ) );

    if( aEditMode == IS_NEW )
    {
        m_PolyPoints.push_back( aPosition );    // Start point of first segment.
        m_PolyPoints.push_back( aPosition );    // End point of first segment.
    }
    else if( aEditMode == IS_RESIZED )
    {
        // Drag one edge point of the polyline
        // Find the nearest edge point to be dragged
        wxPoint startPoint = m_PolyPoints[0];

        // Begin with the first list point as nearest point
        int     index = 0;
        m_ModifyIndex = 0;
        m_initialPos  = startPoint;

        // First distance is the current minimum distance
        int     distanceMin = (aPosition - startPoint).x * (aPosition - startPoint).x
                              + (aPosition - startPoint).y * (aPosition - startPoint).y;

        wxPoint prevPoint = startPoint;

        // Find the right index of the point to be dragged
        BOOST_FOREACH( wxPoint point, m_PolyPoints )
        {
            int distancePoint = (aPosition - point).x * (aPosition - point).x +
                                (aPosition - point).y * (aPosition - point).y;

            if( distancePoint < distanceMin )
            {
                // Save point.
                m_initialPos  = point;
                m_ModifyIndex = index;
                distanceMin   = distancePoint;
            }

            // check middle of an edge
            wxPoint offset = ( aPosition + aPosition - point - prevPoint );
            distancePoint = ( offset.x * offset.x + offset.y * offset.y ) / 4 + 1;

            if( distancePoint < distanceMin )
            {
                // Save point.
                m_initialPos  = point;
                m_ModifyIndex = -index;  // negative indicates new vertex is to be inserted
                distanceMin   = distancePoint;
            }

            prevPoint = point;
            index++;
        }

        SetEraseLastDrawItem();
    }
    else if( aEditMode == IS_MOVED )
    {
        m_initialCursorPos = aPosition;
        m_initialPos = m_PolyPoints[0];
        SetEraseLastDrawItem();
    }

    m_Flags = aEditMode;
}


bool LIB_POLYLINE::ContinueEdit( const wxPoint aPosition )
{
    wxCHECK_MSG( ( m_Flags & ( IS_NEW | IS_MOVED | IS_RESIZED ) ) != 0, false,
                wxT( "Bad call to ContinueEdit().  LIB_POLYLINE is not being edited." ) );

    if( m_Flags == IS_NEW )
    {
        // do not add zero length segments
        if( m_PolyPoints[m_PolyPoints.size() - 2] != m_PolyPoints.back() )
            m_PolyPoints.push_back( aPosition );

        return true;
    }

    return false;
}


void LIB_POLYLINE::EndEdit( const wxPoint& aPosition, bool aAbort )
{
    wxCHECK_RET( ( m_Flags & ( IS_NEW | IS_MOVED | IS_RESIZED ) ) != 0,
                 wxT( "Bad call to EndEdit().  LIB_POLYLINE is not being edited." ) );

    // do not include last point twice
    if( m_Flags == IS_NEW && 2 < m_PolyPoints.size() )
    {
        if( m_PolyPoints[ m_PolyPoints.size() - 2 ] == m_PolyPoints.back() )
            m_PolyPoints.pop_back();
    }

    if( (m_Flags == IS_RESIZED) && (m_PolyPoints.size() > 2) ) // do not delete last two points... keep it alive
    {
        if( ( m_ModifyIndex > 0 && m_PolyPoints[ m_ModifyIndex ] ==
              m_PolyPoints[ m_ModifyIndex - 1 ] )
          || ( m_ModifyIndex < (int) m_PolyPoints.size() - 1
             && m_PolyPoints[ m_ModifyIndex ] == m_PolyPoints[ m_ModifyIndex + 1 ] ) )
        {
            m_PolyPoints.erase( m_PolyPoints.begin() + m_ModifyIndex ); // delete a point on this
        }
    }

    m_Flags = 0;
    SetEraseLastDrawItem( false );
}


void LIB_POLYLINE::calcEdit( const wxPoint& aPosition )
{
    if( m_Flags == IS_NEW )
    {
        m_PolyPoints[ GetCornerCount() - 1 ] = aPosition;
        SetEraseLastDrawItem();
    }
    else if( m_Flags == IS_RESIZED )
    {
        if( m_ModifyIndex < 0 ) // negative indicates new vertex is to be inserted
        {
            m_ModifyIndex = -m_ModifyIndex;
            m_PolyPoints.insert( m_PolyPoints.begin() + m_ModifyIndex, aPosition );
        }

        m_PolyPoints[ m_ModifyIndex ] = aPosition;
    }
    else if( m_Flags == IS_MOVED )
    {
        Move( m_initialPos + aPosition - m_initialCursorPos );
    }
}
