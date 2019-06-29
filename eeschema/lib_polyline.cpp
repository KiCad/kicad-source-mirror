/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2017 Jean-Pierre Charras, jp.charras at wanadoo.fr
 * Copyright (C) 2004-2019 KiCad Developers, see AUTHORS.txt for contributors.
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
#include <lib_polyline.h>
#include <transform.h>


LIB_POLYLINE::LIB_POLYLINE( LIB_PART* aParent ) :
    LIB_ITEM( LIB_POLYLINE_T, aParent )
{
    m_Fill  = NO_FILL;
    m_Width = 0;
    m_isFillable = true;
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


void LIB_POLYLINE::Offset( const wxPoint& aOffset )
{
    for( wxPoint& point : m_PolyPoints )
        point += aOffset;
}


bool LIB_POLYLINE::Inside( EDA_RECT& aRect ) const
{
    for( const wxPoint& point : m_PolyPoints )
    {
        if( aRect.Contains( point.x, -point.y ) )
            return true;
    }

    return false;
}


void LIB_POLYLINE::MoveTo( const wxPoint& aPosition )
{
    Offset( aPosition - m_PolyPoints[ 0 ] );
}


void LIB_POLYLINE::MirrorHorizontal( const wxPoint& aCenter )
{
    for( wxPoint& point : m_PolyPoints )
    {
        point.x -= aCenter.x;
        point.x *= -1;
        point.x += aCenter.x;
    }
}

void LIB_POLYLINE::MirrorVertical( const wxPoint& aCenter )
{
    for( wxPoint& point : m_PolyPoints )
    {
        point.y -= aCenter.y;
        point.y *= -1;
        point.y += aCenter.y;
    }
}

void LIB_POLYLINE::Rotate( const wxPoint& aCenter, bool aRotateCCW )
{
    for( wxPoint& point : m_PolyPoints )
        RotatePoint( &point, aCenter, aRotateCCW ? -900 : 900 );
}


void LIB_POLYLINE::Plot( PLOTTER* aPlotter, const wxPoint& aOffset, bool aFill,
                         const TRANSFORM& aTransform )
{
    wxASSERT( aPlotter != NULL );

    static std::vector< wxPoint > cornerList;
    cornerList.clear();

    for( wxPoint pos : m_PolyPoints )
    {
        pos = aTransform.TransformCoordinate( pos ) + aOffset;
        cornerList.push_back( pos );
    }

    if( aFill && m_Fill == FILLED_WITH_BG_BODYCOLOR )
    {
        aPlotter->SetColor( GetLayerColor( LAYER_DEVICE_BACKGROUND ) );
        aPlotter->PlotPoly( cornerList, FILLED_WITH_BG_BODYCOLOR, 0 );
    }

    bool already_filled = m_Fill == FILLED_WITH_BG_BODYCOLOR;
    auto pen_size = GetPenSize();

    if( !already_filled || pen_size > 0 )
    {
        pen_size = std::max( 0, pen_size );
        aPlotter->SetColor( GetLayerColor( LAYER_DEVICE ) );
        aPlotter->PlotPoly( cornerList, already_filled ? NO_FILL : m_Fill, pen_size );
    }
}


void LIB_POLYLINE::AddPoint( const wxPoint& aPosition )
{
    m_PolyPoints.push_back( aPosition );
}


void LIB_POLYLINE::AddCorner( const wxPoint& aPosition )
{
    int currentMinDistance = INT_MAX;
    int closestLineStart = 0;

    for( unsigned i = 0; i < m_PolyPoints.size() - 1; ++i )
    {
        int distance = (int) DistanceLinePoint( m_PolyPoints[i], m_PolyPoints[i + 1], aPosition );

        if( distance < currentMinDistance )
        {
            currentMinDistance = distance;
            closestLineStart = i;
        }
    }

    m_PolyPoints.insert( m_PolyPoints.begin() + closestLineStart, aPosition );
}


void LIB_POLYLINE::RemoveCorner( int aIdx )
{
    m_PolyPoints.erase( m_PolyPoints.begin() + aIdx );
}


int LIB_POLYLINE::GetPenSize() const
{
    if( m_Width > 0 )
        return m_Width;

    if( m_Width == 0 )
       return GetDefaultLineThickness();

    return -1;   // the minimal pen value
}


void LIB_POLYLINE::print( wxDC* aDC, const wxPoint& aOffset, void* aData,
                          const TRANSFORM& aTransform )
{
    COLOR4D color   = GetLayerColor( LAYER_DEVICE );
    COLOR4D bgColor = GetLayerColor( LAYER_DEVICE_BACKGROUND );

    wxPoint* buffer = new wxPoint[ m_PolyPoints.size() ];

    for( unsigned ii = 0; ii < m_PolyPoints.size(); ii++ )
        buffer[ii] = aTransform.TransformCoordinate( m_PolyPoints[ii] ) + aOffset;

    FILL_T fill = aData ? NO_FILL : m_Fill;

    if( fill == FILLED_WITH_BG_BODYCOLOR )
        GRPoly( nullptr, aDC, m_PolyPoints.size(), buffer, 1, GetPenSize(), bgColor, bgColor );
    else if( fill == FILLED_SHAPE  )
        GRPoly( nullptr, aDC, m_PolyPoints.size(), buffer, 1, GetPenSize(), color, color );
    else
        GRPoly( nullptr, aDC, m_PolyPoints.size(), buffer, 0, GetPenSize(), color, color );

    delete[] buffer;
}


bool LIB_POLYLINE::HitTest( const wxPoint& aPosition, int aAccuracy ) const
{
    int              delta = std::max( aAccuracy + GetPenSize() / 2, MINIMUM_SELECTION_DISTANCE );
    SHAPE_LINE_CHAIN shape;

    for( wxPoint pt : m_PolyPoints )
        shape.Append( DefaultTransform.TransformCoordinate( pt ) );

    if( m_Fill != NO_FILL && m_PolyPoints.size() > 2 )
    {
        shape.SetClosed( true );
        return shape.PointInside( aPosition, delta );
    }
    else
        return shape.PointOnEdge( aPosition, delta );
}


bool LIB_POLYLINE::HitTest( const EDA_RECT& aRect, bool aContained, int aAccuracy ) const
{
    if( m_Flags & ( STRUCT_DELETED | SKIP_STRUCT ) )
        return false;

    EDA_RECT sel = aRect;

    if ( aAccuracy )
        sel.Inflate( aAccuracy );

    if( aContained )
        return sel.Contains( GetBoundingBox() );

    // Fast test: if rect is outside the polygon bounding box, then they cannot intersect
    if( !sel.Intersects( GetBoundingBox() ) )
        return false;

    // Account for the width of the line
    sel.Inflate( GetPenSize() / 2 );

    // Only test closing segment if the polyline is filled
    int count = m_Fill == NO_FILL ? m_PolyPoints.size() - 1 : m_PolyPoints.size();

    for( int ii = 0; ii < count; ii++ )
    {
        wxPoint pt = DefaultTransform.TransformCoordinate( m_PolyPoints[ ii ] );
        wxPoint ptNext = DefaultTransform.TransformCoordinate( m_PolyPoints[ (ii+1) % count ] );

        // Test if the point is within aRect
        if( sel.Contains( pt ) )
            return true;

        // Test if this edge intersects aRect
        if( sel.Intersects( pt, ptNext ) )
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

    rect.SetOrigin( xmin, ymin );
    rect.SetEnd( xmax, ymax );
    rect.Inflate( ( GetPenSize()+1 ) / 2 );

    rect.RevertYAxis();

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


void LIB_POLYLINE::GetMsgPanelInfo( EDA_UNITS_T aUnits, MSG_PANEL_ITEMS& aList )
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


wxString LIB_POLYLINE::GetSelectMenuText( EDA_UNITS_T aUnits ) const
{
    return wxString::Format( _( "Polyline at (%s, %s) with %d points" ),
                             MessageTextFromValue( aUnits, m_PolyPoints[0].x ),
                             MessageTextFromValue( aUnits, m_PolyPoints[0].y ),
                             int( m_PolyPoints.size() ) );
}


BITMAP_DEF LIB_POLYLINE::GetMenuImage() const
{
    return add_graphical_segments_xpm;
}


void LIB_POLYLINE::BeginEdit( const wxPoint aPosition )
{
    m_PolyPoints.push_back( aPosition );    // Start point of first segment.
    m_PolyPoints.push_back( aPosition );    // End point of first segment.
}


bool LIB_POLYLINE::ContinueEdit( const wxPoint aPosition )
{
    // do not add zero length segments
    if( m_PolyPoints[m_PolyPoints.size() - 2] != m_PolyPoints.back() )
        m_PolyPoints.push_back( aPosition );

    return true;
}


void LIB_POLYLINE::EndEdit()
{
    // do not include last point twice
    if( m_PolyPoints.size() > 2 )
    {
        if( m_PolyPoints[ m_PolyPoints.size() - 2 ] == m_PolyPoints.back() )
            m_PolyPoints.pop_back();
    }
}


void LIB_POLYLINE::CalcEdit( const wxPoint& aPosition )
{
    m_PolyPoints[ GetCornerCount() - 1 ] = aPosition;
}
