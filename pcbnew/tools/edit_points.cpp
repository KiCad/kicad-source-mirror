/*
 * This program source code file is part of KICAD, a free EDA CAD application.
 *
 * Copyright (C) 2014 CERN
 * @author Maciej Suminski <maciej.suminski@cern.ch>
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

#include <boost/foreach.hpp>

#include "edit_points.h"
#include <gal/graphics_abstraction_layer.h>
#include <geometry/seg.h>

bool EDIT_POINT::WithinPoint( const VECTOR2I& aPoint, unsigned int aSize ) const
{
    // Corners of the EDIT_POINT square
    VECTOR2I topLeft = GetPosition() - aSize;
    VECTOR2I bottomRight = GetPosition() + aSize;

    return ( aPoint.x > topLeft.x && aPoint.y > topLeft.y &&
             aPoint.x < bottomRight.x && aPoint.y < bottomRight.y );
}


EDIT_POINTS::EDIT_POINTS( EDA_ITEM* aParent ) :
    EDA_ITEM( NOT_USED ), m_parent( aParent )
{
}


EDIT_POINT* EDIT_POINTS::FindPoint( const VECTOR2I& aLocation )
{
    float size = m_view->ToWorld( EDIT_POINT::POINT_SIZE );

    std::deque<EDIT_POINT>::iterator pit, pitEnd;
    for( pit = m_points.begin(), pitEnd = m_points.end(); pit != pitEnd; ++pit )
    {
        EDIT_POINT& point = *pit;

        if( point.WithinPoint( aLocation, size ) )
            return &point;
    }

    std::deque<EDIT_LINE>::iterator lit, litEnd;
    for( lit = m_lines.begin(), litEnd = m_lines.end(); lit != litEnd; ++lit )
    {
        EDIT_LINE& point = *lit;

        if( point.WithinPoint( aLocation, size ) )
            return &point;
    }

    return NULL;
}


EDIT_POINT* EDIT_POINTS::Previous( const EDIT_POINT& aPoint )
{
    for( unsigned int i = 0; i < m_points.size(); ++i )
    {
        if( m_points[i] == aPoint )
        {
            if( i == 0 )
                return &m_points[m_points.size() - 1];
            else
                return &m_points[i - 1];
        }
    }

    for( unsigned int i = 0; i < m_lines.size(); ++i )
    {
        if( m_lines[i] == aPoint )
        {
            if( i == 0 )
                return &m_lines[m_lines.size() - 1];
            else
                return &m_lines[i - 1];
        }
    }

    return NULL;
}


EDIT_POINT* EDIT_POINTS::Next( const EDIT_POINT& aPoint )
{
    for( unsigned int i = 0; i < m_points.size(); ++i )
    {
        if( m_points[i] == aPoint )
        {
            if( i == m_points.size() - 1 )
                return &m_points[0];
            else
                return &m_points[i + 1];
        }
    }

    for( unsigned int i = 0; i < m_lines.size(); ++i )
    {
        if( m_lines[i] == aPoint )
        {
            if( i == m_lines.size() - 1 )
                return &m_lines[0];
            else
                return &m_lines[i + 1];
        }
    }

    return NULL;
}


void EDIT_POINTS::ViewDraw( int aLayer, KIGFX::GAL* aGal ) const
{
    aGal->SetFillColor( KIGFX::COLOR4D( 1.0, 1.0, 1.0, 1.0 ) );
    aGal->SetIsFill( true );
    aGal->SetIsStroke( false );
    aGal->PushDepth();
    aGal->SetLayerDepth( -512.0 );      // TODO no hardcoded depths?

    float size = m_view->ToWorld( EDIT_POINT::POINT_SIZE );

    BOOST_FOREACH( const EDIT_POINT& point, m_points )
        aGal->DrawRectangle( point.GetPosition() - size / 2, point.GetPosition() + size / 2 );

    BOOST_FOREACH( const EDIT_LINE& line, m_lines )
        aGal->DrawCircle( line.GetPosition(), size / 2 );

    aGal->PopDepth();
}


void EC_45DEGREE::Apply( EDIT_POINT& aHandle )
{
    // Current line vector
    VECTOR2I lineVector( aHandle.GetPosition() - m_constrainer.GetPosition() );
    double angle = lineVector.Angle();

    // Find the closest angle, which is a multiple of 45 degrees
    double newAngle = round( angle / ( M_PI / 4.0 ) ) * M_PI / 4.0;
    VECTOR2I newLineVector = lineVector.Rotate( newAngle - angle );

    aHandle.SetPosition( m_constrainer.GetPosition() + newLineVector );
}


EC_LINE::EC_LINE( EDIT_POINT& aConstrained, const EDIT_POINT& aConstrainer ) :
    EDIT_CONSTRAINT<EDIT_POINT>( aConstrained ), m_constrainer( aConstrainer )
{
    m_line = m_constrained.GetPosition() - m_constrainer.GetPosition();
}


void EC_LINE::Apply( EDIT_POINT& aHandle )
{
    SEG main( m_constrainer.GetPosition(), m_constrainer.GetPosition() + m_line );
    SEG projection( aHandle.GetPosition(), aHandle.GetPosition() + m_line.Perpendicular() );

    if( OPT_VECTOR2I intersect = projection.IntersectLines( main ) )
        aHandle.SetPosition( *intersect );
}


void EC_CIRCLE::Apply( EDIT_POINT& aHandle )
{
    VECTOR2I centerToEnd = m_end.GetPosition() - m_center.GetPosition();
    VECTOR2I centerToPoint = aHandle.GetPosition() - m_center.GetPosition();

    int radius = centerToEnd.EuclideanNorm();
    double angle = centerToPoint.Angle();

    VECTOR2I newLine( radius, 0 );
    newLine = newLine.Rotate( angle );

    aHandle.SetPosition( m_center.GetPosition() + newLine );
}


EC_CONVERGING::EC_CONVERGING( EDIT_LINE& aLine, EDIT_POINTS& aPoints ) :
    EDIT_CONSTRAINT<EDIT_POINT>( aLine.GetOrigin() ), m_line( aLine ), m_editPoints( aPoints )
{
    // Dragged segment endings
    EDIT_POINT& origin = aLine.GetOrigin();
    EDIT_POINT& end = aLine.GetEnd();

    // Previous and next points, to make constraining lines (adjacent to the dragged line)
    EDIT_POINT& prevOrigin = *aPoints.Previous( origin );
    EDIT_POINT& nextEnd = *aPoints.Next( end );

    // Constraints for segments adjacent to the dragged one
    m_originSideConstraint = new EC_LINE( origin, prevOrigin );
    m_endSideConstraint = new EC_LINE( end, nextEnd );

    // Store the current vector of the line
    m_draggedVector = end.GetPosition() - origin.GetPosition() ;
}


EC_CONVERGING::~EC_CONVERGING()
{
    delete m_originSideConstraint;
    delete m_endSideConstraint;
}


void EC_CONVERGING::Apply( EDIT_POINT& aHandle )
{
    // The dragged segment
    SEG dragged( m_line.GetPosition(), m_line.GetPosition() + m_draggedVector );

    // The dragged segment endpoints
    EDIT_POINT& origin = m_line.GetOrigin();
    EDIT_POINT& end = m_line.GetEnd();

    // Do not allow points on the adjacent segments move freely
    m_originSideConstraint->Apply();
    m_endSideConstraint->Apply();

    EDIT_POINT& prevOrigin = *m_editPoints.Previous( origin );
    EDIT_POINT& nextEnd = *m_editPoints.Next( end );

    // Two segments adjacent to the dragged segment
    SEG originSide = SEG( origin.GetPosition(), prevOrigin.GetPosition() );
    SEG endSide = SEG( end.GetPosition(), nextEnd.GetPosition() );

    // First intersection point (dragged segment against origin side)
    if( OPT_VECTOR2I originIntersect = dragged.IntersectLines( originSide ) )
        origin.SetPosition( *originIntersect );

    // Second intersection point (dragged segment against end side)
    if( OPT_VECTOR2I endIntersect = dragged.IntersectLines( endSide ) )
        end.SetPosition( *endIntersect );

    // Check if adjacent segments intersect (did we dragged the line to the point that it may
    // create a selfintersecting polygon?)
    originSide = SEG( origin.GetPosition(), prevOrigin.GetPosition() );
    endSide = SEG( end.GetPosition(), nextEnd.GetPosition() );

    if( OPT_VECTOR2I originEndIntersect = endSide.Intersect( originSide ) )
    {
        origin.SetPosition( *originEndIntersect );
        end.SetPosition( *originEndIntersect );
    }
}
