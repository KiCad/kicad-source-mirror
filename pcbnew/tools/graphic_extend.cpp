/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation, either version 3 of the License, or (at your
 * option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include <tools/graphic_extend.h>

#include <board_item.h>
#include <geometry/half_line.h>
#include <geometry/intersection.h>
#include <geometry/seg.h>
#include <geometry/shape_circle.h>
#include <geometry/shape_utils.h>
#include <pcb_shape.h>
#include <tools/board_item_geometry.h>

#include <cmath>
#include <limits>

/**
 * The ray the shape grows along.  Runs from the fixed end through the extended one.
 *
 * Anchoring on the two existing endpoints keeps the direction exact.  Projecting a third point
 * would overflow at full range.  The ray also covers the shape, so drop candidates behind it.
 */
static HALF_LINE extensionRay( const PCB_SHAPE& aSource, GRAPHIC_ENDPOINT aEndpoint )
{
    if( aEndpoint == GRAPHIC_ENDPOINT::START )
        return HALF_LINE( aSource.GetEnd(), aSource.GetStart() );

    return HALF_LINE( aSource.GetStart(), aSource.GetEnd() );
}


GRAPHIC_ENDPOINT GRAPHIC_EXTEND_PLANNER::NearestEndpoint( const BOARD_ITEM& aSource, const VECTOR2I& aPointer )
{
    const PCB_SHAPE* source = GraphicEditShape( &aSource );

    if( !source )
        return GRAPHIC_ENDPOINT::END;

    // Distance() widens before subtracting.  A far-corner pointer cannot overflow.
    return source->GetStart().Distance( aPointer ) < source->GetEnd().Distance( aPointer ) ? GRAPHIC_ENDPOINT::START
                                                                                           : GRAPHIC_ENDPOINT::END;
}


BOX2I GRAPHIC_EXTEND_PLANNER::QueryBounds( const BOARD_ITEM& aSource, GRAPHIC_ENDPOINT aEndpoint,
                                           const BOX2I& aWorldBounds )
{
    const PCB_SHAPE* source = GraphicEditShape( &aSource );

    if( !source )
        return {};

    if( source->GetShape() == SHAPE_T::ARC )
    {
        SHAPE_ARC arc = GraphicEditArc( *source );

        if( !IsGraphicEditArcUsable( arc ) )
            return {};

        BOX2I circle = SHAPE_CIRCLE( arc.GetCenter(), KiROUND( arc.GetRadius() ) ).BBox();

        // Nothing outside the world is a boundary.  Keeps a large arc local.
        return circle.Intersect( aWorldBounds );
    }

    const HALF_LINE    ray = extensionRay( *source, aEndpoint );
    std::optional<SEG> swept = KIGEOM::ClipHalfLineToBox( ray, aWorldBounds );

    if( !swept )
        return {};

    // ClipHalfLineToBox reports crossings in box-edge order.  It substitutes the ray origin if
    // the ray starts inside.  The exit is whichever is farther along.
    const VECTOR2I exit =
            ray.GetStart().Distance( swept->A ) > ray.GetStart().Distance( swept->B ) ? swept->A : swept->B;
    BOX2I bounds;

    bounds.SetOrigin( aEndpoint == GRAPHIC_ENDPOINT::START ? source->GetStart() : source->GetEnd() );
    bounds.Merge( exit );

    return bounds;
}


GRAPHIC_EDIT_RESULT GRAPHIC_EXTEND_PLANNER::Plan( const BOARD_ITEM& aSource, GRAPHIC_ENDPOINT aEndpoint,
                                                  const std::vector<const BOARD_ITEM*>& aBoundaries )
{
    GRAPHIC_EDIT_RESULT result;
    const PCB_SHAPE*    source = GraphicEditSource( aSource, result );

    if( !source )
        return result;

    const bool     segment = source->GetShape() == SHAPE_T::SEGMENT;
    const VECTOR2I selected = aEndpoint == GRAPHIC_ENDPOINT::START ? source->GetStart() : source->GetEnd();
    const VECTOR2I fixed = aEndpoint == GRAPHIC_ENDPOINT::START ? source->GetEnd() : source->GetStart();

    double             bestDistance = std::numeric_limits<double>::infinity();
    VECTOR2I           bestPoint;
    const BOARD_ITEM*  bestBoundary = nullptr;
    double             sourceSweep = 0.0;
    double             sourceRadius = 0.0;
    bool               sourceClockwise = false;
    VECTOR2I           sourceCenter;
    INTERSECTABLE_GEOM sourceGeometry = SEG();

    if( segment )
    {
        if( fixed == selected )
        {
            result.m_Refusal = GRAPHIC_EDIT_REFUSAL::DEGENERATE;
            return result;
        }

        sourceGeometry = extensionRay( *source, aEndpoint );
    }
    else
    {
        SHAPE_ARC arc = GraphicEditArc( *source );

        if( !IsGraphicEditArcUsable( arc ) )
        {
            result.m_Refusal = GRAPHIC_EDIT_REFUSAL::DEGENERATE;
            return result;
        }

        sourceSweep = arc.GetCentralAngle().AsDegrees();
        sourceClockwise = arc.IsClockwise();
        sourceCenter = arc.GetCenter();
        sourceRadius = arc.GetRadius();

        // An arc grows either way.  The whole circle is in play.
        sourceGeometry = CIRCLE( sourceCenter, KiROUND( arc.GetRadius() ) );
    }

    for( const BOARD_ITEM* item : aBoundaries )
    {
        const PCB_SHAPE* boundary = GraphicEditBoundary( item, *source );

        if( !boundary )
            continue;

        std::optional<INTERSECTABLE_GEOM> boundaryGeometry = BoardItemIntersectable( *boundary );

        if( !boundaryGeometry )
            continue;

        std::vector<VECTOR2I> intersections;
        INTERSECTION_CONTACT  contact;

        std::visit( INTERSECTION_VISITOR( *boundaryGeometry, intersections, contact ), sourceGeometry );

        // A shared run gives nothing to stop at.  A graze gives exactly one point, so it counts.
        if( contact.m_Overlapping )
            continue;

        for( const VECTOR2I& point : intersections )
        {
            if( GraphicEditCoincident( point, selected ) )
                continue;

            double distance;

            if( segment )
            {
                // Candidates all lie on the ray from the fixed end.  Anything short of the
                // moving end is nearer the fixed one.  Distance() widens, so no overflow.
                if( fixed.Distance( point ) <= fixed.Distance( selected ) )
                    continue;

                distance = selected.Distance( point );
            }
            else
            {
                SHAPE_ARC extension;

                if( aEndpoint == GRAPHIC_ENDPOINT::END )
                    extension.ConstructFromStartEndCenter( selected, point, sourceCenter, sourceClockwise );
                else
                    extension.ConstructFromStartEndCenter( point, selected, sourceCenter, sourceClockwise );

                distance = std::abs( extension.GetCentralAngle().AsDegrees() );

                // Growing back over the source closes the circle.
                if( distance <= GRAPHIC_EDIT_ANGLE_EPSILON
                    || std::abs( sourceSweep ) + distance >= 360.0 - GRAPHIC_EDIT_ANGLE_EPSILON )
                {
                    continue;
                }
            }

            // One ray, or one circle traversed one way.  Equal distance means the same point.
            if( distance < bestDistance )
            {
                bestDistance = distance;
                bestPoint = point;
                bestBoundary = item;
            }
        }
    }

    if( !bestBoundary )
    {
        // An arc that reaches nothing keeps going, all the way round.  A line has no such end.
        if( segment )
            return result;

        GRAPHIC_EDIT_GEOMETRY closed;

        closed.m_Shape = SHAPE_T::CIRCLE;
        closed.m_Start = sourceCenter;
        closed.m_End = sourceCenter + VECTOR2I( KiROUND( sourceRadius ), 0 );

        result.m_Refusal = GRAPHIC_EDIT_REFUSAL::NONE;
        result.m_Geometry.push_back( closed );
        return result;
    }

    GRAPHIC_EDIT_GEOMETRY extended;
    const VECTOR2I        start = aEndpoint == GRAPHIC_ENDPOINT::START ? bestPoint : source->GetStart();
    const VECTOR2I        end = aEndpoint == GRAPHIC_ENDPOINT::END ? bestPoint : source->GetEnd();

    extended.m_Shape = segment ? SHAPE_T::SEGMENT : SHAPE_T::ARC;

    if( segment )
    {
        extended.m_Start = start;
        extended.m_End = end;
    }
    else
    {
        // Build from the two endpoints, not a sweep.  The intersection is rounded, so it sits a
        // few IU off the source radius.  Sweeping that radius misses a large arc by tens of IU.
        SHAPE_ARC arc;
        arc.ConstructFromStartEndCenter( start, end, sourceCenter, sourceClockwise );

        extended.m_Start = arc.GetP0();
        extended.m_Mid = arc.GetArcMid();
        extended.m_End = arc.GetP1();
    }

    result.m_Refusal = GRAPHIC_EDIT_REFUSAL::NONE;
    result.m_Geometry.push_back( extended );
    result.m_Boundaries.push_back( bestBoundary );
    return result;
}
