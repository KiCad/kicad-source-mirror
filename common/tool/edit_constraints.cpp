/*
 * This program source code file is part of KICAD, a free EDA CAD application.
 *
 * Copyright (C) 2014 CERN
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
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
#include <math/vector2d.h>
#include <math/util.h>


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


void EC_90DEGREE::Apply( EDIT_POINT& aHandle, const GRID_HELPER& aGrid )
{
    VECTOR2I lineVector( aHandle.GetPosition() - m_constrainer.GetPosition() );
    VECTOR2I newLineVector = GetVectorSnapped90( lineVector );

    if( aHandle.GetGridConstraint() == SNAP_TO_GRID )
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


EC_CONVERGING::EC_CONVERGING( EDIT_LINE& aLine, EDIT_POINTS& aPoints,
                              POLYGON_LINE_MODE aMode ) :
        EDIT_CONSTRAINT<EDIT_LINE>( aLine ),
        m_mode( aMode ),
        m_colinearConstraint( nullptr ),
        m_editPoints( aPoints ),
        m_prevOrigin( aPoints.Previous( aLine.GetOrigin(), false ) ),
        m_nextEnd( aPoints.Next( aLine.GetEnd(), false ) )
{
    EDIT_POINT& origin = aLine.GetOrigin();
    EDIT_POINT& end = aLine.GetEnd();

    // Constraints for segments adjacent to the dragged one
    m_originSideConstraint = std::make_unique<EC_LINE>( origin, *m_prevOrigin );
    m_endSideConstraint = std::make_unique<EC_LINE>( end, *m_nextEnd );

    // Store the current vector and center of the line
    m_draggedVector = end.GetPosition() - origin.GetPosition();
    m_originalCenter = aLine.GetPosition();

    // Perpendicular direction for constraining movement
    m_perpVector = m_draggedVector.Perpendicular();

    // Half-length for fixed-length mode
    m_halfLength = m_draggedVector.EuclideanNorm() / 2.0;

    // Check for colinearity
    SEG originSide( origin.GetPosition(), m_prevOrigin->GetPosition() );
    SEG endSide( end.GetPosition(), m_nextEnd->GetPosition() );
    SEG dragged( origin.GetPosition(), end.GetPosition() );

    const int alignAngle = 10;

    m_originCollinear = dragged.Angle( originSide ).AsDegrees() < alignAngle;
    m_endCollinear = dragged.Angle( endSide ).AsDegrees() < alignAngle;

    if( m_originCollinear )
        m_colinearConstraint = m_originSideConstraint.get();
    else if( m_endCollinear )
        m_colinearConstraint = m_endSideConstraint.get();

    if( OPT_VECTOR2I intersect = originSide.IntersectLines( endSide ) )
        m_convergencePoint = *intersect;
    else
        m_convergencePoint = aLine.GetPosition();

    m_midVector = aLine.GetPosition() - m_convergencePoint;
}


EC_CONVERGING::~EC_CONVERGING()
{
    // m_colinearConstraint should not be freed, it is a pointer to one of the above
}


void EC_CONVERGING::Apply( EDIT_LINE& aHandle, const GRID_HELPER& aGrid )
{
    VECTOR2I handlePos = aHandle.GetPosition();

    // Project the handle position onto the perpendicular line through the original center.
    // This ensures the line always moves perpendicular to itself.
    SEG perpLine( m_originalCenter, m_originalCenter + m_perpVector );
    SEG toHandle( handlePos, handlePos + m_draggedVector );
    VECTOR2I newCenter;

    if( OPT_VECTOR2I intersect = perpLine.IntersectLines( toHandle ) )
        newCenter = *intersect;
    else
        newCenter = handlePos;

    // In converging mode, don't allow movement past the convergence point
    if( m_mode == POLYGON_LINE_MODE::CONVERGING )
    {
        VECTOR2I centerToConv = m_convergencePoint - m_originalCenter;
        VECTOR2I centerToNew = newCenter - m_originalCenter;

        // Check if we've crossed past the convergence point
        if( centerToConv.Dot( m_perpVector ) != 0 )
        {
            double t = double( centerToNew.Dot( m_perpVector ) )
                       / double( centerToConv.Dot( m_perpVector ) );

            if( t > 1.0 )
                newCenter = m_convergencePoint;
        }
    }

    aHandle.SetPosition( newCenter );

    if( m_mode == POLYGON_LINE_MODE::FIXED_LENGTH )
        applyFixedLength( aHandle );
    else
        applyConverging( aHandle );
}


void EC_CONVERGING::applyConverging( EDIT_LINE& aHandle )
{
    EDIT_POINT& origin = aHandle.GetOrigin();
    EDIT_POINT& end = aHandle.GetEnd();

    if( m_originCollinear && m_endCollinear )
    {
        if( m_colinearConstraint )
        {
            GRID_HELPER dummyGrid;
            m_colinearConstraint->Apply( origin, dummyGrid );
            m_colinearConstraint->Apply( end, dummyGrid );
        }

        return;
    }

    // The dragged segment at new position (parallel to original)
    VECTOR2I newCenter = aHandle.GetPosition();
    SEG dragged( newCenter - m_draggedVector / 2, newCenter + m_draggedVector / 2 );

    // Get the fixed directions of adjacent segments from the stored EC_LINE constraints.
    // These directions were captured at drag start and don't change.
    EC_LINE* originLine = static_cast<EC_LINE*>( m_originSideConstraint.get() );
    EC_LINE* endLine = static_cast<EC_LINE*>( m_endSideConstraint.get() );

    VECTOR2I originDir = originLine->GetLineVector();
    VECTOR2I endDir = endLine->GetLineVector();

    // Adjacent segments use fixed directions from the anchor points
    SEG originSide( m_prevOrigin->GetPosition(),
                    m_prevOrigin->GetPosition() + originDir );
    SEG endSide( m_nextEnd->GetPosition(),
                 m_nextEnd->GetPosition() + endDir );

    // Find intersection of dragged line with origin side
    if( OPT_VECTOR2I originIntersect = dragged.IntersectLines( originSide ) )
        origin.SetPosition( *originIntersect );

    // Find intersection of dragged line with end side
    if( OPT_VECTOR2I endIntersect = dragged.IntersectLines( endSide ) )
        end.SetPosition( *endIntersect );

    // Check if adjacent segments would intersect (self-intersecting polygon)
    originSide = SEG( origin.GetPosition(), m_prevOrigin->GetPosition() );
    endSide = SEG( end.GetPosition(), m_nextEnd->GetPosition() );

    if( OPT_VECTOR2I originEndIntersect = endSide.Intersect( originSide ) )
    {
        if( m_editPoints.LinesSize() > 3 )
        {
            origin.SetPosition( *originEndIntersect );
            end.SetPosition( *originEndIntersect );
        }
    }
}


void EC_CONVERGING::applyFixedLength( EDIT_LINE& aHandle )
{
    EDIT_POINT& origin = aHandle.GetOrigin();
    EDIT_POINT& end = aHandle.GetEnd();

    VECTOR2I newCenter = aHandle.GetPosition();

    // Keep the line at its original length, centered on the new position
    VECTOR2D unitDir = VECTOR2D( m_draggedVector );

    if( unitDir.EuclideanNorm() > 0 )
        unitDir = unitDir / unitDir.EuclideanNorm();

    VECTOR2I newOrigin = newCenter - KiROUND( unitDir * m_halfLength );
    VECTOR2I newEnd = newCenter + KiROUND( unitDir * m_halfLength );

    origin.SetPosition( newOrigin );
    end.SetPosition( newEnd );
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
