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

#include <tools/graphic_trim.h>

#include <board_item.h>
#include <geometry/circle.h>
#include <geometry/intersection.h>
#include <geometry/seg.h>
#include <geometry/shape_utils.h>
#include <pcb_shape.h>
#include <tools/board_item_geometry.h>

#include <algorithm>
#include <cmath>
#include <numbers>

/// A fraction of the carrier's own length.  Same meaning at every scale.
static constexpr double PARAM_EPSILON = 1e-7;

struct TRIM_CUT
{
    double            m_Parameter;
    VECTOR2I          m_Point;
    const BOARD_ITEM* m_Boundary;
};


static double arcParameter( const SHAPE_ARC& aSource, const VECTOR2I& aPoint )
{
    // ConstructFromStartEndCenter cannot measure a zero or full sweep.  Take the ends direct.
    if( GraphicEditCoincident( aPoint, aSource.GetP0() ) )
        return 0.0;

    if( GraphicEditCoincident( aPoint, aSource.GetP1() ) )
        return 1.0;

    SHAPE_ARC portion;
    portion.ConstructFromStartEndCenter( aSource.GetP0(), aPoint, aSource.GetCenter(), aSource.IsClockwise() );

    return std::abs( portion.GetCentralAngle().AsDegrees() / aSource.GetCentralAngle().AsDegrees() );
}


/// The one curve the pointer is trimming.  A circle has no ends, so its cuts wrap round; a
/// rectangle offers only the side the pointer is nearest, and the other three come through.
struct TRIM_CARRIER
{
    SHAPE_T            m_Kind = SHAPE_T::SEGMENT;
    SEG                m_Seg;
    SHAPE_ARC          m_Arc;
    CIRCLE             m_Circle;
    INTERSECTABLE_GEOM m_Geometry = SEG();
    std::vector<SEG>   m_Untouched;

    bool IsClosed() const { return m_Kind == SHAPE_T::CIRCLE; }

    VECTOR2I Start() const { return m_Kind == SHAPE_T::ARC ? m_Arc.GetP0() : m_Seg.A; }
    VECTOR2I End() const { return m_Kind == SHAPE_T::ARC ? m_Arc.GetP1() : m_Seg.B; }

    double Parameter( const VECTOR2I& aPoint ) const
    {
        if( m_Kind == SHAPE_T::ARC )
            return arcParameter( m_Arc, aPoint );

        if( m_Kind == SHAPE_T::CIRCLE )
        {
            double turn = std::atan2( (double) aPoint.y - m_Circle.Center.y, (double) aPoint.x - m_Circle.Center.x );

            if( turn < 0.0 )
                turn += 2.0 * std::numbers::pi;

            return turn / ( 2.0 * std::numbers::pi );
        }

        return KIGEOM::ParameterAlong( m_Seg, aPoint );
    }

    /// Where the pointer sits, brought onto the carrier.
    double PointerParameter( const VECTOR2I& aPointer ) const
    {
        // NearestPoint() is clamped to the arc.  A line projection is not.
        if( m_Kind == SHAPE_T::ARC )
            return Parameter( m_Arc.NearestPoint( aPointer ) );

        if( m_Kind == SHAPE_T::CIRCLE )
            return Parameter( aPointer );

        return std::clamp( Parameter( aPointer ), 0.0, 1.0 );
    }

    /// The piece of the carrier running forward from aFrom to aTo.
    GRAPHIC_EDIT_GEOMETRY Span( const VECTOR2I& aFrom, const VECTOR2I& aTo, double aFromParameter,
                                double aToParameter ) const
    {
        GRAPHIC_EDIT_GEOMETRY span;

        span.m_Start = aFrom;
        span.m_End = aTo;

        if( m_Kind == SHAPE_T::SEGMENT )
        {
            span.m_Shape = SHAPE_T::SEGMENT;
            return span;
        }

        span.m_Shape = SHAPE_T::ARC;

        if( m_Kind == SHAPE_T::ARC )
        {
            SHAPE_ARC retained;
            retained.ConstructFromStartEndCenter( aFrom, aTo, m_Arc.GetCenter(), m_Arc.IsClockwise() );
            span.m_Mid = retained.GetArcMid();
            return span;
        }

        // Round the circle from aFrom, so the halfway point is half of however far that is.
        double sweep = aToParameter - aFromParameter;

        if( sweep <= 0.0 )
            sweep += 1.0;

        double middle = ( aFromParameter + sweep / 2.0 ) * 2.0 * std::numbers::pi;

        span.m_Mid = VECTOR2I( m_Circle.Center.x + KiROUND( m_Circle.Radius * std::cos( middle ) ),
                               m_Circle.Center.y + KiROUND( m_Circle.Radius * std::sin( middle ) ) );
        return span;
    }
};


/// Fills aCarrier from aSource, or returns the reason it cannot.
static GRAPHIC_EDIT_REFUSAL buildCarrier( const PCB_SHAPE& aSource, const VECTOR2I& aPointer,
                                          TRIM_CARRIER& aCarrier )
{
    switch( aSource.GetShape() )
    {
    case SHAPE_T::SEGMENT:
        aCarrier.m_Kind = SHAPE_T::SEGMENT;
        aCarrier.m_Seg = SEG( aSource.GetStart(), aSource.GetEnd() );

        if( aCarrier.m_Seg.A == aCarrier.m_Seg.B )
            return GRAPHIC_EDIT_REFUSAL::DEGENERATE;

        aCarrier.m_Geometry = aCarrier.m_Seg;
        break;

    case SHAPE_T::RECTANGLE:
    {
        std::vector<VECTOR2I> corners = aSource.GetRectCorners();

        if( corners.size() != 4 )
            return GRAPHIC_EDIT_REFUSAL::DEGENERATE;

        // GetRectCorners() answers for a flat rectangle too, with two of its sides zero length
        // and the other two the same run in opposite directions.  There is no area to cut.
        if( aSource.GetStart().x == aSource.GetEnd().x || aSource.GetStart().y == aSource.GetEnd().y )
            return GRAPHIC_EDIT_REFUSAL::DEGENERATE;

        std::vector<SEG> sides;

        for( size_t i = 0; i < corners.size(); i++ )
            sides.emplace_back( corners[i], corners[( i + 1 ) % corners.size()] );

        auto nearest = std::ranges::min_element( sides,
                                                 [&]( const SEG& aA, const SEG& aB )
                                                 {
                                                     return aA.SquaredDistance( aPointer )
                                                            < aB.SquaredDistance( aPointer );
                                                 } );

        aCarrier.m_Kind = SHAPE_T::SEGMENT;
        aCarrier.m_Seg = *nearest;

        if( aCarrier.m_Seg.A == aCarrier.m_Seg.B )
            return GRAPHIC_EDIT_REFUSAL::DEGENERATE;

        for( const SEG& side : sides )
        {
            if( &side != &*nearest )
                aCarrier.m_Untouched.push_back( side );
        }

        aCarrier.m_Geometry = aCarrier.m_Seg;
        break;
    }

    case SHAPE_T::ARC:
        aCarrier.m_Kind = SHAPE_T::ARC;
        aCarrier.m_Arc = GraphicEditArc( aSource );

        if( !IsGraphicEditArcUsable( aCarrier.m_Arc ) )
            return GRAPHIC_EDIT_REFUSAL::DEGENERATE;

        aCarrier.m_Geometry = aCarrier.m_Arc;
        break;

    case SHAPE_T::CIRCLE:
        aCarrier.m_Kind = SHAPE_T::CIRCLE;
        aCarrier.m_Circle = CIRCLE( aSource.GetCenter(), aSource.GetRadius() );

        if( aCarrier.m_Circle.Radius <= 0 || aCarrier.m_Circle.Radius > MAX_GRAPHIC_EDIT_ARC_RADIUS )
            return GRAPHIC_EDIT_REFUSAL::DEGENERATE;

        aCarrier.m_Geometry = aCarrier.m_Circle;
        break;

    default:
        return GRAPHIC_EDIT_REFUSAL::UNSUPPORTED_SOURCE;
    }

    return GRAPHIC_EDIT_REFUSAL::NONE;
}


GRAPHIC_EDIT_RESULT GRAPHIC_TRIM_PLANNER::Plan( const BOARD_ITEM& aSource, const VECTOR2I& aPointer,
                                                const std::vector<const BOARD_ITEM*>& aBoundaries )
{
    GRAPHIC_EDIT_RESULT result;
    const PCB_SHAPE*    source = GraphicEditSource( aSource, IsGraphicTrimSource, result );

    if( !source )
        return result;

    TRIM_CARRIER               carrier;
    const GRAPHIC_EDIT_REFUSAL refusal = buildCarrier( *source, aPointer, carrier );

    if( refusal != GRAPHIC_EDIT_REFUSAL::NONE )
    {
        result.m_Refusal = refusal;
        return result;
    }

    std::vector<TRIM_CUT> cuts;
    bool                  touched = false;
    bool                  overlapped = false;

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

        std::visit( INTERSECTION_VISITOR( *boundaryGeometry, intersections, contact ), carrier.m_Geometry );

        // A shared run has no one point to cut at.  A graze has exactly one, so it counts.
        if( contact.m_Overlapping )
        {
            overlapped = true;
            continue;
        }

        for( const VECTOR2I& point : intersections )
        {
            touched = true;

            double parameter = carrier.Parameter( point );

            // An end is already a partition point.  It still says the shape is bounded here.
            if( !carrier.IsClosed() && ( parameter <= PARAM_EPSILON || parameter >= 1.0 - PARAM_EPSILON ) )
                continue;

            auto duplicate = std::find_if( cuts.begin(), cuts.end(),
                                           [&]( const TRIM_CUT& aCut )
                                           {
                                               return GraphicEditCoincident( aCut.m_Point, point );
                                           } );

            if( duplicate == cuts.end() )
                cuts.push_back( { parameter, point, item } );
        }
    }

    if( !touched )
    {
        // An overlap is the only thing that met the source.  There is no single place to cut.
        result.m_Refusal = overlapped ? GRAPHIC_EDIT_REFUSAL::AMBIGUOUS : GRAPHIC_EDIT_REFUSAL::NO_INTERSECTION;
        return result;
    }

    // A closed carrier has no ends of its own to fall back on.  It needs a cut either side.
    if( carrier.IsClosed() && cuts.size() < 2 )
    {
        result.m_Refusal = GRAPHIC_EDIT_REFUSAL::NO_INTERSECTION;
        return result;
    }

    std::ranges::sort( cuts, {}, &TRIM_CUT::m_Parameter );

    const double pointerParameter = carrier.PointerParameter( aPointer );

    for( const TRIM_CUT& cut : cuts )
    {
        if( std::abs( cut.m_Parameter - pointerParameter ) <= PARAM_EPSILON )
        {
            result.m_Refusal = GRAPHIC_EDIT_REFUSAL::AMBIGUOUS;
            return result;
        }
    }

    const TRIM_CUT* before = nullptr;
    const TRIM_CUT* after = nullptr;

    if( carrier.IsClosed() )
    {
        // Cuts wrap, so the pointer always falls between two of them.  Past the last one it is
        // back before the first.
        auto next = std::ranges::find_if( cuts,
                                          [&]( const TRIM_CUT& aCut )
                                          {
                                              return aCut.m_Parameter > pointerParameter;
                                          } );

        after = next == cuts.end() ? &cuts.front() : &*next;
        before = next == cuts.begin() || next == cuts.end() ? &cuts.back() : &*( next - 1 );
    }
    else
    {
        for( const TRIM_CUT& cut : cuts )
        {
            if( cut.m_Parameter < pointerParameter )
                before = &cut;
            else if( !after )
                after = &cut;
        }
    }

    // The span the pointer is in.  An open carrier's own ends close it off.
    const VECTOR2I removedFrom = before ? before->m_Point : carrier.Start();
    const VECTOR2I removedTo = after ? after->m_Point : carrier.End();
    const double   removedFromParameter = before ? before->m_Parameter : 0.0;
    const double   removedToParameter = after ? after->m_Parameter : 1.0;

    result.m_Preview.push_back(
            carrier.Span( removedFrom, removedTo, removedFromParameter, removedToParameter ) );

    if( carrier.IsClosed() )
    {
        // Everything the other way round survives, as one arc.
        result.m_Geometry.push_back(
                carrier.Span( removedTo, removedFrom, removedToParameter, removedFromParameter ) );
    }
    else
    {
        if( before )
            result.m_Geometry.push_back( carrier.Span( carrier.Start(), before->m_Point, 0.0, before->m_Parameter ) );

        if( after )
            result.m_Geometry.push_back( carrier.Span( after->m_Point, carrier.End(), after->m_Parameter, 1.0 ) );
    }

    // A rectangle keeps its other three sides whatever happens to the one under the pointer.
    for( const SEG& side : carrier.m_Untouched )
    {
        GRAPHIC_EDIT_GEOMETRY kept;

        kept.m_Shape = SHAPE_T::SEGMENT;
        kept.m_Start = side.A;
        kept.m_End = side.B;
        result.m_Geometry.push_back( kept );
    }

    if( before )
        result.m_Boundaries.push_back( before->m_Boundary );

    if( after && ( !before || after->m_Boundary != before->m_Boundary ) )
        result.m_Boundaries.push_back( after->m_Boundary );

    result.m_Refusal = GRAPHIC_EDIT_REFUSAL::NONE;
    return result;
}
