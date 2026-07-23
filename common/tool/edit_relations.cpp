/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 3
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include <tool/edit_relations.h>

#include <geometry/circle.h>
#include <geometry/seg.h>
#include <geometry/geometry_utils.h>
#include <math/util.h>
#include <tool/edit_points.h>
#include <tool/grid_helper.h>


std::unique_ptr<EDIT_RELATION> EDIT_RELATION::make( EDIT_RELATION_KIND aKind, const EDIT_POINT& aReference )
{
    auto relation = std::unique_ptr<EDIT_RELATION>( new EDIT_RELATION( aKind ) );
    relation->m_reference = &aReference;
    return relation;
}


std::unique_ptr<EDIT_RELATION> EDIT_RELATION::SameX( const EDIT_POINT& aReference )
{
    return make( EDIT_RELATION_KIND::SAME_X, aReference );
}


std::unique_ptr<EDIT_RELATION> EDIT_RELATION::SameY( const EDIT_POINT& aReference )
{
    return make( EDIT_RELATION_KIND::SAME_Y, aReference );
}


std::unique_ptr<EDIT_RELATION> EDIT_RELATION::Angle45( const EDIT_POINT& aReference )
{
    return make( EDIT_RELATION_KIND::ANGLE_STEP_45, aReference );
}


std::unique_ptr<EDIT_RELATION> EDIT_RELATION::Angle90( const EDIT_POINT& aReference )
{
    return make( EDIT_RELATION_KIND::ANGLE_STEP_90, aReference );
}


std::unique_ptr<EDIT_RELATION> EDIT_RELATION::PointOnLine( const EDIT_POINT& aConstrained,
                                                           const EDIT_POINT& aReference )
{
    auto relation = make( EDIT_RELATION_KIND::POINT_ON_LINE, aReference );
    relation->m_direction = aConstrained.GetPosition() - aReference.GetPosition();
    return relation;
}


std::unique_ptr<EDIT_RELATION> EDIT_RELATION::PointOnCircle( const EDIT_POINT& aCenter, const EDIT_POINT& aEnd )
{
    auto relation = make( EDIT_RELATION_KIND::POINT_ON_CIRCLE, aCenter );
    relation->m_secondaryReference = &aEnd;
    return relation;
}


std::unique_ptr<EDIT_RELATION> EDIT_RELATION::PerpendicularTranslation( const EDIT_LINE& aLine )
{
    auto relation =
            std::unique_ptr<EDIT_RELATION>( new EDIT_RELATION( EDIT_RELATION_KIND::PERPENDICULAR_TRANSLATION ) );
    relation->m_origin = aLine.GetPosition();
    relation->m_direction = ( aLine.GetEnd().GetPosition() - aLine.GetOrigin().GetPosition() ).Perpendicular();
    return relation;
}


void EDIT_RELATION::Apply( EDIT_POINT& aHandle, const GRID_HELPER& aGrid ) const
{
    VECTOR2I point = aHandle.GetPosition();

    switch( m_kind )
    {
    case EDIT_RELATION_KIND::SAME_X:
        if( aHandle.GetGridConstraint() == SNAP_TO_GRID )
            point = aGrid.AlignGrid( point );

        point.x = m_reference->GetPosition().x;
        aHandle.SetPosition( point );
        break;

    case EDIT_RELATION_KIND::SAME_Y:
        if( aHandle.GetGridConstraint() == SNAP_TO_GRID )
            point = aGrid.AlignGrid( point );

        point.y = m_reference->GetPosition().y;
        aHandle.SetPosition( point );
        break;

    case EDIT_RELATION_KIND::ANGLE_STEP_45:
    case EDIT_RELATION_KIND::ANGLE_STEP_90:
    {
        VECTOR2I vector = point - m_reference->GetPosition();
        VECTOR2I snapped = m_kind == EDIT_RELATION_KIND::ANGLE_STEP_45 ? GetVectorSnapped45( vector )
                                                                       : GetVectorSnapped90( vector );

        if( aHandle.GetGridConstraint() == SNAP_TO_GRID
            && ( m_kind == EDIT_RELATION_KIND::ANGLE_STEP_90 || snapped.x == 0 || snapped.y == 0 ) )
        {
            VECTOR2I gridPoint = aGrid.AlignGrid( m_reference->GetPosition() + snapped );

            if( snapped.x == 0 )
                point = VECTOR2I( m_reference->GetPosition().x, gridPoint.y );
            else
                point = VECTOR2I( gridPoint.x, m_reference->GetPosition().y );
        }
        else
        {
            point = m_reference->GetPosition() + snapped;
        }

        aHandle.SetPosition( point );
        break;
    }

    case EDIT_RELATION_KIND::POINT_ON_LINE:
    {
        if( aHandle.GetGridConstraint() == SNAP_TO_GRID && ( m_direction.x == 0 || m_direction.y == 0 ) )
        {
            VECTOR2I gridPoint = aGrid.AlignGrid( point );

            if( m_direction.x == 0 )
                point.y = gridPoint.y;
            else
                point.x = gridPoint.x;
        }

        const VECTOR2I& reference = m_reference->GetPosition();
        aHandle.SetPosition( SEG( reference, reference + m_direction ).LineProject( point ) );
        break;
    }

    case EDIT_RELATION_KIND::POINT_ON_CIRCLE:
    {
        const VECTOR2I& center = m_reference->GetPosition();
        const int       radius = ( m_secondaryReference->GetPosition() - center ).EuclideanNorm();
        aHandle.SetPosition( CIRCLE( center, radius ).NearestPoint( point ) );
        break;
    }

    case EDIT_RELATION_KIND::PERPENDICULAR_TRANSLATION: break;
    }
}


void EDIT_RELATION::Apply( EDIT_LINE& aHandle, const GRID_HELPER& aGrid ) const
{
    if( m_kind != EDIT_RELATION_KIND::PERPENDICULAR_TRANSLATION )
        return;

    aHandle.SetPosition( SEG( m_origin, m_origin + m_direction ).LineProject( aHandle.GetPosition() ) );
}


POLYGON_EDGE_DRAG_POLICY::POLYGON_EDGE_DRAG_POLICY( EDIT_LINE& aLine, EDIT_POINTS& aPoints, POLYGON_LINE_MODE aMode ) :
        m_mode( aMode ),
        m_editPoints( aPoints ),
        m_prevOrigin( aPoints.Previous( aLine.GetOrigin(), false ) ),
        m_nextEnd( aPoints.Next( aLine.GetEnd(), false ) )
{
    EDIT_POINT& origin = aLine.GetOrigin();
    EDIT_POINT& end = aLine.GetEnd();

    m_originSideDirection = origin.GetPosition() - m_prevOrigin->GetPosition();
    m_endSideDirection = end.GetPosition() - m_nextEnd->GetPosition();
    m_draggedVector = end.GetPosition() - origin.GetPosition();
    m_originalCenter = aLine.GetPosition();
    m_perpVector = m_draggedVector.Perpendicular();
    m_halfLength = m_draggedVector.EuclideanNorm() / 2.0;

    SEG originSide( origin.GetPosition(), m_prevOrigin->GetPosition() );
    SEG endSide( end.GetPosition(), m_nextEnd->GetPosition() );
    SEG dragged( origin.GetPosition(), end.GetPosition() );

    constexpr int alignAngle = 10;
    m_originCollinear = dragged.Angle( originSide ).AsDegrees() < alignAngle;
    m_endCollinear = dragged.Angle( endSide ).AsDegrees() < alignAngle;

    if( OPT_VECTOR2I intersection = originSide.IntersectLines( endSide ) )
        m_convergencePoint = *intersection;
    else
        m_convergencePoint = aLine.GetPosition();
}


void POLYGON_EDGE_DRAG_POLICY::Apply( EDIT_LINE& aHandle, const GRID_HELPER& aGrid )
{
    SEG      perpendicular( m_originalCenter, m_originalCenter + m_perpVector );
    SEG      throughHandle( aHandle.GetPosition(), aHandle.GetPosition() + m_draggedVector );
    VECTOR2I newCenter = aHandle.GetPosition();

    if( OPT_VECTOR2I intersection = perpendicular.IntersectLines( throughHandle ) )
        newCenter = *intersection;

    if( m_mode == POLYGON_LINE_MODE::CONVERGING )
    {
        VECTOR2I centerToConvergence = m_convergencePoint - m_originalCenter;
        VECTOR2I centerToNew = newCenter - m_originalCenter;
        auto     denominator = centerToConvergence.Dot( m_perpVector );

        if( denominator != 0 && double( centerToNew.Dot( m_perpVector ) ) / double( denominator ) > 1.0 )
        {
            newCenter = m_convergencePoint;
        }
    }

    aHandle.SetPosition( newCenter );

    if( m_mode == POLYGON_LINE_MODE::FIXED_LENGTH )
        applyFixedLength( aHandle );
    else
        applyConverging( aHandle );
}


void POLYGON_EDGE_DRAG_POLICY::applyConverging( EDIT_LINE& aHandle )
{
    EDIT_POINT& origin = aHandle.GetOrigin();
    EDIT_POINT& end = aHandle.GetEnd();

    if( m_originCollinear && m_endCollinear )
    {
        const VECTOR2I& reference = m_prevOrigin->GetPosition();
        const SEG       line( reference, reference + m_originSideDirection );

        for( EDIT_POINT* point : { &origin, &end } )
            point->SetPosition( line.LineProject( point->GetPosition() ) );

        return;
    }

    VECTOR2I center = aHandle.GetPosition();
    SEG      dragged( center - m_draggedVector / 2, center + m_draggedVector / 2 );
    SEG      originSide( m_prevOrigin->GetPosition(), m_prevOrigin->GetPosition() + m_originSideDirection );
    SEG      endSide( m_nextEnd->GetPosition(), m_nextEnd->GetPosition() + m_endSideDirection );

    if( OPT_VECTOR2I intersection = dragged.IntersectLines( originSide ) )
        origin.SetPosition( *intersection );

    if( OPT_VECTOR2I intersection = dragged.IntersectLines( endSide ) )
        end.SetPosition( *intersection );

    originSide = SEG( origin.GetPosition(), m_prevOrigin->GetPosition() );
    endSide = SEG( end.GetPosition(), m_nextEnd->GetPosition() );

    if( OPT_VECTOR2I intersection = endSide.Intersect( originSide ) )
    {
        if( m_editPoints.LinesSize() > 3 )
        {
            origin.SetPosition( *intersection );
            end.SetPosition( *intersection );
        }
    }
}


void POLYGON_EDGE_DRAG_POLICY::applyFixedLength( EDIT_LINE& aHandle )
{
    VECTOR2D direction( m_draggedVector );

    if( direction.EuclideanNorm() > 0 )
        direction = direction / direction.EuclideanNorm();

    VECTOR2I halfVector = KiROUND( direction * m_halfLength );
    aHandle.GetOrigin().SetPosition( aHandle.GetPosition() - halfVector );
    aHandle.GetEnd().SetPosition( aHandle.GetPosition() + halfVector );
}
