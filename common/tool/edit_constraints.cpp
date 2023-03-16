/*
 * This program source code file is part of KICAD, a free EDA CAD application.
 *
 * Copyright (C) 2014 CERN
 * Copyright (C) 2021-2022 KiCad Developers, see AUTHORS.txt for contributors.
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

#include "tool/edit_constraints.h"
#include "tool/edit_points.h"

#include <geometry/seg.h>
#include <trigo.h>

#include <geometry/geometry_utils.h>


void EC_VERTICAL::Apply( EDIT_POINT& aHandle, const GRID_HELPER& aGrid )
{
    VECTOR2I point = aHandle.GetPosition();

    if( aHandle.GetGridConstraint() == SNAP_TO_GRID )
        point = aGrid.AlignGrid( point );

    point.x = m_constrainer.GetPosition().x;
    aHandle.SetPosition( point );
}


void EC_HORIZONTAL::Apply( EDIT_POINT& aHandle, const GRID_HELPER& aGrid )
{
    VECTOR2I point = aHandle.GetPosition();

    if( aHandle.GetGridConstraint() == SNAP_TO_GRID )
        point = aGrid.AlignGrid( point );

    point.y = m_constrainer.GetPosition().y;
    aHandle.SetPosition( point );
}


void EC_45DEGREE::Apply( EDIT_POINT& aHandle, const GRID_HELPER& aGrid )
{
    VECTOR2I lineVector( aHandle.GetPosition() - m_constrainer.GetPosition() );
    VECTOR2I newLineVector = GetVectorSnapped45( lineVector );

    if( aHandle.GetGridConstraint() == SNAP_TO_GRID
            && ( newLineVector.x == 0 || newLineVector.y == 0 ) )
    {
        VECTOR2I snap = aGrid.AlignGrid( m_constrainer.GetPosition() + newLineVector );

        if( newLineVector.x == 0 )
            aHandle.SetPosition( VECTOR2I( m_constrainer.GetPosition().x, snap.y ) );
        else
            aHandle.SetPosition( VECTOR2I( snap.x, m_constrainer.GetPosition().y ) );
    }
    else
    {
        aHandle.SetPosition( m_constrainer.GetPosition() + newLineVector );
    }
}


EC_LINE::EC_LINE( EDIT_POINT& aConstrained, const EDIT_POINT& aConstrainer ) :
        EDIT_CONSTRAINT<EDIT_POINT>( aConstrained ),
        m_constrainer( aConstrainer )
{
    m_line = m_constrained.GetPosition() - m_constrainer.GetPosition();
}


void EC_LINE::Apply( EDIT_POINT& aHandle, const GRID_HELPER& aGrid )
{
    SEG main( m_constrainer.GetPosition(), m_constrainer.GetPosition() + m_line );

    if( aHandle.GetGridConstraint() == SNAP_TO_GRID
            && ( m_line.x == 0 || m_line.y == 0 ) )
    {
        VECTOR2I snappedHandle = aGrid.AlignGrid( aHandle.GetPosition() );

        if( m_line.x == 0 )
            aHandle.SetPosition( VECTOR2I( aHandle.GetPosition().x, snappedHandle.y ) );
        else
            aHandle.SetPosition( VECTOR2I( snappedHandle.x, aHandle.GetPosition().y ) );
    }

    SEG projection( aHandle.GetPosition(), aHandle.GetPosition() + m_line.Perpendicular() );

    if( OPT_VECTOR2I intersect = projection.IntersectLines( main ) )
        aHandle.SetPosition( *intersect );
}


void EC_CIRCLE::Apply( EDIT_POINT& aHandle, const GRID_HELPER& aGrid )
{
    VECTOR2I centerToEnd = m_end.GetPosition() - m_center.GetPosition();
    VECTOR2I centerToPoint = aHandle.GetPosition() - m_center.GetPosition();

    int       radius = centerToEnd.EuclideanNorm();
    EDA_ANGLE angle( centerToPoint );

    VECTOR2I newLine( radius, 0 );
    RotatePoint( newLine, -angle );

    aHandle.SetPosition( m_center.GetPosition() + newLine );
}


EC_CONVERGING::EC_CONVERGING( EDIT_LINE& aLine, EDIT_POINTS& aPoints ) :
    EDIT_CONSTRAINT<EDIT_LINE>( aLine ),
    m_colinearConstraint( nullptr ),
    m_editPoints( aPoints )
{
    // Dragged segment endings
    EDIT_POINT& origin = aLine.GetOrigin();
    EDIT_POINT& end = aLine.GetEnd();

    // Previous and next points, to make constraining lines (adjacent to the dragged line)
    EDIT_POINT& prevOrigin = *aPoints.Previous( origin, false );
    EDIT_POINT& nextEnd = *aPoints.Next( end, false );

    // Constraints for segments adjacent to the dragged one
    m_originSideConstraint = new EC_LINE( origin, prevOrigin );
    m_endSideConstraint = new EC_LINE( end, nextEnd );

    // Store the current vector of the line
    m_draggedVector = end.GetPosition() - origin.GetPosition();

    // Check for colinearity
    SEG originSide( origin.GetPosition(), prevOrigin.GetPosition() );
    SEG endSide( end.GetPosition(), nextEnd.GetPosition() );
    SEG dragged( origin.GetPosition(), end.GetPosition() );

    // Used to align lines that are almost collinear
    const int alignAngle = 10;

    m_originCollinear = dragged.Angle( originSide ).AsDegrees() < alignAngle;
    m_endCollinear = dragged.Angle( endSide ).AsDegrees() < alignAngle;

    if( m_originCollinear )
        m_colinearConstraint = m_originSideConstraint;
    else if( m_endCollinear )
        m_colinearConstraint = m_endSideConstraint;
}


EC_CONVERGING::~EC_CONVERGING()
{
    delete m_originSideConstraint;
    delete m_endSideConstraint;
    // m_colinearConstraint should not be freed, it is a pointer to one of the above
}


void EC_CONVERGING::Apply( EDIT_LINE& aHandle, const GRID_HELPER& aGrid )
{
    // The dragged segment endpoints
    EDIT_POINT& origin = aHandle.GetOrigin();
    EDIT_POINT& end = aHandle.GetEnd();

    if( m_colinearConstraint )
    {
        m_colinearConstraint->Apply( origin, aGrid );
        m_colinearConstraint->Apply( end, aGrid );
    }

    if( m_originCollinear && m_endCollinear )
        return;

    // The dragged segment
    SEG dragged( origin.GetPosition(), origin.GetPosition() + m_draggedVector );

    // Do not allow points on the adjacent segments move freely
    m_originSideConstraint->Apply( aGrid );
    m_endSideConstraint->Apply( aGrid );

    EDIT_POINT& prevOrigin = *m_editPoints.Previous( origin, false );
    EDIT_POINT& nextEnd = *m_editPoints.Next( end, false );

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
        // Triangle intersect by definition
        if( m_editPoints.LinesSize() > 3 )
        {
            origin.SetPosition( *originEndIntersect );
            end.SetPosition( *originEndIntersect );
        }
    }
}


EC_PERPLINE::EC_PERPLINE( EDIT_LINE& aLine ) :
    EDIT_CONSTRAINT<EDIT_LINE>( aLine )
{
    m_mid = aLine.GetPosition();
    m_line = ( aLine.GetEnd().GetPosition() - aLine.GetOrigin().GetPosition() ).Perpendicular();
}


void EC_PERPLINE::Apply( EDIT_LINE& aHandle, const GRID_HELPER& aGrid )
{
    SEG main( m_mid, m_mid + m_line );
    SEG projection( aHandle.GetPosition(), aHandle.GetPosition() + m_line.Perpendicular() );

    if( OPT_VECTOR2I intersect = projection.IntersectLines( main ) )
        aHandle.SetPosition( *intersect );

    VECTOR2D delta = aHandle.GetEnd().GetPosition() - aHandle.GetOrigin().GetPosition();

    aHandle.GetOrigin().SetPosition( aHandle.GetOrigin().GetPosition() );
    aHandle.GetEnd().SetPosition( aHandle.GetOrigin().GetPosition() + delta );
}
