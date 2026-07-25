/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
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
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include "geometry/intersection.h"

#include <core/type_helpers.h>

#include <geometry/shape_utils.h>

#include <algorithm>
#include <concepts>
#include <limits>

/*
 * Helper functions that dispatch to the correct intersection function
 * in one of the geometry classes.
 */
namespace
{

constexpr double OVERLAP_EPSILON = 1e-7;

/// Carries a seg.  SEG is its own.
template <typename T>
concept LINE_LIKE = std::same_as<T, SEG> || std::same_as<T, LINE> || std::same_as<T, HALF_LINE>;

/// Carries a circle.  CIRCLE is its own.
template <typename T>
concept CIRCULAR = std::same_as<T, CIRCLE> || std::same_as<T, SHAPE_ARC>;

/*
 * A contact counts only where it lies on both geometries, so each shape answers for its own
 * extent.  LINE and CIRCLE are unbounded carriers.  Any point off them is on them.
 */
template <typename GEOM>
    requires LINE_LIKE<GEOM> || CIRCULAR<GEOM>
bool extentContains( const GEOM& aGeom, const VECTOR2I& aPoint )
{
    if constexpr( std::same_as<GEOM, LINE> || std::same_as<GEOM, CIRCLE> )
        return true;
    else if constexpr( std::same_as<GEOM, SHAPE_ARC> )
        return aGeom.Collide( aPoint );
    else
        return aGeom.Contains( aPoint );
}

template <LINE_LIKE GEOM>
const SEG& carrierSeg( const GEOM& aGeom )
{
    if constexpr( std::same_as<GEOM, SEG> )
        return aGeom;
    else
        return aGeom.GetContainedSeg();
}

template <CIRCULAR GEOM>
CIRCLE carrierCircle( const GEOM& aGeom )
{
    if constexpr( std::same_as<GEOM, CIRCLE> )
        return aGeom;
    else
        return CIRCLE( aGeom.GetCenter(), KiROUND( aGeom.GetRadius() ) );
}

/// Span along aRef.  Unbounded ends included.
std::pair<double, double> extentAlong( const SEG& aSeg, const SEG& aRef )
{
    return std::minmax( KIGEOM::ParameterAlong( aRef, aSeg.A ), KIGEOM::ParameterAlong( aRef, aSeg.B ) );
}

std::pair<double, double> extentAlong( const LINE&, const SEG& )
{
    return { -std::numeric_limits<double>::infinity(), std::numeric_limits<double>::infinity() };
}

std::pair<double, double> extentAlong( const HALF_LINE& aHalfLine, const SEG& aRef )
{
    const SEG& ray = aHalfLine.GetContainedSeg();
    double     start = KIGEOM::ParameterAlong( aRef, ray.A );

    if( KIGEOM::ParameterAlong( aRef, ray.B ) > start )
        return { start, std::numeric_limits<double>::infinity() };

    return { -std::numeric_limits<double>::infinity(), start };
}

/// Collinear and sharing more than a point.  No single intersection.
template <LINE_LIKE GEOM_A, LINE_LIKE GEOM_B>
void checkCollinearOverlap( const GEOM_A& aA, const GEOM_B& aB, INTERSECTION_CONTACT* aContact )
{
    if( !aContact || aContact->m_Overlapping )
        return;

    const SEG& refSeg = carrierSeg( aA );

    if( !refSeg.Collinear( carrierSeg( aB ) ) )
        return;

    const auto [aLow, aHigh] = extentAlong( aA, refSeg );
    const auto [bLow, bHigh] = extentAlong( aB, refSeg );

    if( std::min( aHigh, bHigh ) - std::max( aLow, bLow ) > OVERLAP_EPSILON )
        aContact->m_Overlapping = true;
}

/// A graze touches the carrier circle once.
template <LINE_LIKE LINE_GEOM, CIRCULAR CIRCULAR_GEOM>
void checkLineTangency( const LINE_GEOM& aLine, const CIRCULAR_GEOM& aCircular, INTERSECTION_CONTACT* aContact )
{
    if( !aContact || aContact->m_Tangent )
        return;

    std::vector<VECTOR2I> touches = carrierCircle( aCircular ).IntersectLine( carrierSeg( aLine ) );

    if( touches.size() == 1 && extentContains( aLine, touches.front() )
        && extentContains( aCircular, touches.front() ) )
    {
        aContact->m_Tangent = true;
    }
}

/// A full circle covers its whole carrier.  Only two arcs can miss.
template <CIRCULAR GEOM_A, CIRCULAR GEOM_B>
bool sharesArcExtent( const GEOM_A&, const GEOM_B& )
{
    return true;
}

bool sharesArcExtent( const SHAPE_ARC& aA, const SHAPE_ARC& aB )
{
    return aA.Collide( aB.GetArcMid() ) || aB.Collide( aA.GetArcMid() );
}

/// Circular pairs graze at one point, or share a carrier and run together.
template <CIRCULAR GEOM_A, CIRCULAR GEOM_B>
void checkCircularContact( const GEOM_A& aA, const GEOM_B& aB, INTERSECTION_CONTACT* aContact )
{
    if( !aContact )
        return;

    const CIRCLE circleA = carrierCircle( aA );
    const CIRCLE circleB = carrierCircle( aB );

    if( circleA.Center == circleB.Center && circleA.Radius == circleB.Radius )
    {
        if( !aContact->m_Overlapping && sharesArcExtent( aA, aB ) )
            aContact->m_Overlapping = true;

        return;
    }

    if( aContact->m_Tangent )
        return;

    std::vector<VECTOR2I> touches = circleA.Intersect( circleB );

    if( touches.size() == 1 && extentContains( aA, touches.front() ) && extentContains( aB, touches.front() ) )
        aContact->m_Tangent = true;
}

void findIntersections( const SEG& aSegA, const SEG& aSegB, std::vector<VECTOR2I>& aIntersections,
                        INTERSECTION_CONTACT* aContact )
{
    checkCollinearOverlap( aSegA, aSegB, aContact );

    const OPT_VECTOR2I intersection = aSegA.Intersect( aSegB );

    if( intersection )
    {
        aIntersections.push_back( *intersection );
    }
}

void findIntersections( const SEG& aSeg, const LINE& aLine, std::vector<VECTOR2I>& aIntersections,
                        INTERSECTION_CONTACT* aContact )
{
    checkCollinearOverlap( aSeg, aLine, aContact );

    OPT_VECTOR2I intersection = aLine.Intersect( aSeg );

    if( intersection )
    {
        aIntersections.push_back( *intersection );
    }
}

void findIntersections( const SEG& aSeg, const HALF_LINE& aHalfLine,
                        std::vector<VECTOR2I>& aIntersections, INTERSECTION_CONTACT* aContact )
{
    checkCollinearOverlap( aSeg, aHalfLine, aContact );

    OPT_VECTOR2I intersection = aHalfLine.Intersect( aSeg );

    if( intersection )
    {
        aIntersections.push_back( *intersection );
    }
}

void findIntersections( const SEG& aSeg, const CIRCLE& aCircle,
                        std::vector<VECTOR2I>& aIntersections, INTERSECTION_CONTACT* aContact )
{
    checkLineTangency( aSeg, aCircle, aContact );

    std::vector<VECTOR2I> intersections = aCircle.Intersect( aSeg );

    aIntersections.insert( aIntersections.end(), intersections.begin(), intersections.end() );
}

void findIntersections( const SEG& aSeg, const SHAPE_ARC& aArc,
                        std::vector<VECTOR2I>& aIntersections, INTERSECTION_CONTACT* aContact )
{
    checkLineTangency( aSeg, aArc, aContact );

    std::vector<VECTOR2I> intersections;
    aArc.IntersectLine( aSeg, &intersections );

    // Find only the intersections that are within the segment
    for( const VECTOR2I& intersection : intersections )
    {
        if( aSeg.Contains( intersection ) )
        {
            aIntersections.emplace_back( intersection );
        }
    }
}

void findIntersections( const LINE& aLineA, const LINE& aLineB,
                        std::vector<VECTOR2I>& aIntersections, INTERSECTION_CONTACT* aContact )
{
    checkCollinearOverlap( aLineA, aLineB, aContact );

    OPT_VECTOR2I intersection = aLineA.Intersect( aLineB );

    if( intersection )
    {
        aIntersections.push_back( *intersection );
    }
}

void findIntersections( const LINE& aLine, const HALF_LINE& aHalfLine,
                        std::vector<VECTOR2I>& aIntersections, INTERSECTION_CONTACT* aContact )
{
    checkCollinearOverlap( aLine, aHalfLine, aContact );

    // Intersect as two infinite lines
    OPT_VECTOR2I intersection =
            aHalfLine.GetContainedSeg().Intersect( aLine.GetContainedSeg(), false, true );

    // No intersection at all (parallel, or passes on the other side of the start point)
    if( !intersection )
    {
        return;
    }

    if( aHalfLine.Contains( *intersection ) )
    {
        aIntersections.push_back( *intersection );
    }
}

void findIntersections( const HALF_LINE& aHalfLineA, const HALF_LINE& aHalfLineB,
                        std::vector<VECTOR2I>& aIntersections, INTERSECTION_CONTACT* aContact )
{
    checkCollinearOverlap( aHalfLineA, aHalfLineB, aContact );

    OPT_VECTOR2I intersection = aHalfLineA.Intersect( aHalfLineB );

    if( intersection )
    {
        aIntersections.push_back( *intersection );
    }
}

void findIntersections( const CIRCLE& aCircle, const LINE& aLine,
                        std::vector<VECTOR2I>& aIntersections, INTERSECTION_CONTACT* aContact )
{
    checkLineTangency( aLine, aCircle, aContact );

    std::vector<VECTOR2I> intersections = aCircle.IntersectLine( aLine.GetContainedSeg() );

    aIntersections.insert( aIntersections.end(), intersections.begin(), intersections.end() );
}

void findIntersections( const CIRCLE& aCircle, const HALF_LINE& aHalfLine,
                        std::vector<VECTOR2I>& aIntersections, INTERSECTION_CONTACT* aContact )
{
    checkLineTangency( aHalfLine, aCircle, aContact );

    std::vector<VECTOR2I> intersections = aCircle.IntersectLine( aHalfLine.GetContainedSeg() );

    for( const VECTOR2I& intersection : intersections )
    {
        if( aHalfLine.Contains( intersection ) )
        {
            aIntersections.push_back( intersection );
        }
    }
}

void findIntersections( const CIRCLE& aCircleA, const CIRCLE& aCircleB,
                        std::vector<VECTOR2I>& aIntersections, INTERSECTION_CONTACT* aContact )
{
    checkCircularContact( aCircleA, aCircleB, aContact );

    std::vector<VECTOR2I> intersections = aCircleA.Intersect( aCircleB );
    aIntersections.insert( aIntersections.end(), intersections.begin(), intersections.end() );
}

void findIntersections( const CIRCLE& aCircle, const SHAPE_ARC& aArc,
                        std::vector<VECTOR2I>& aIntersections, INTERSECTION_CONTACT* aContact )
{
    checkCircularContact( aCircle, aArc, aContact );

    aArc.Intersect( aCircle, &aIntersections );
}

void findIntersections( const SHAPE_ARC& aArcA, const SHAPE_ARC& aArcB,
                        std::vector<VECTOR2I>& aIntersections, INTERSECTION_CONTACT* aContact )
{
    checkCircularContact( aArcA, aArcB, aContact );

    aArcA.Intersect( aArcB, &aIntersections );
}

void findIntersections( const SHAPE_ARC& aArc, const LINE& aLine,
                        std::vector<VECTOR2I>& aIntersections, INTERSECTION_CONTACT* aContact )
{
    checkLineTangency( aLine, aArc, aContact );

    std::vector<VECTOR2I> intersections;
    aArc.IntersectLine( aLine.GetContainedSeg(), &intersections );

    aIntersections.insert( aIntersections.end(), intersections.begin(), intersections.end() );
}

void findIntersections( const SHAPE_ARC& aArc, const HALF_LINE& aHalfLine,
                        std::vector<VECTOR2I>& aIntersections, INTERSECTION_CONTACT* aContact )
{
    checkLineTangency( aHalfLine, aArc, aContact );

    std::vector<VECTOR2I> intersections;
    aArc.IntersectLine( aHalfLine.GetContainedSeg(), &intersections );

    for( const VECTOR2I& intersection : intersections )
    {
        if( aHalfLine.Contains( intersection ) )
        {
            aIntersections.push_back( intersection );
        }
    }
}

/*
 * Ellipse overloads take a contact but never set one.  An ellipse has no carrier seg or circle to
 * reduce to.  A caller that refuses ambiguity accepts a grazing ellipse as a clean crossing.
 */
void findIntersections( const SHAPE_ELLIPSE& aEllipse, const SEG& aSeg, std::vector<VECTOR2I>& aIntersections,
                        INTERSECTION_CONTACT* )
{
    std::vector<VECTOR2I> intersections = aEllipse.Intersect( aSeg );

    aIntersections.insert( aIntersections.end(), intersections.begin(), intersections.end() );
}

void findIntersections( const SHAPE_ELLIPSE& aEllipse, const LINE& aLine, std::vector<VECTOR2I>& aIntersections,
                        INTERSECTION_CONTACT* )
{
    std::vector<VECTOR2I> intersections = aEllipse.Intersect( aLine.GetContainedSeg(), true );

    aIntersections.insert( aIntersections.end(), intersections.begin(), intersections.end() );
}

void findIntersections( const SHAPE_ELLIPSE& aEllipse, const HALF_LINE& aHalfLine,
                        std::vector<VECTOR2I>& aIntersections, INTERSECTION_CONTACT* )
{
    std::vector<VECTOR2I> intersections = aEllipse.Intersect( aHalfLine.GetContainedSeg(), true );

    for( const VECTOR2I& intersection : intersections )
    {
        if( aHalfLine.Contains( intersection ) )
        {
            aIntersections.push_back( intersection );
        }
    }
}

void findIntersections( const SHAPE_ELLIPSE& aEllipse, const CIRCLE& aCircle, std::vector<VECTOR2I>& aIntersections,
                        INTERSECTION_CONTACT* )
{
    std::vector<VECTOR2I> intersections = aEllipse.Intersect( aCircle );

    aIntersections.insert( aIntersections.end(), intersections.begin(), intersections.end() );
}

void findIntersections( const SHAPE_ELLIPSE& aEllipse, const SHAPE_ARC& aArc, std::vector<VECTOR2I>& aIntersections,
                        INTERSECTION_CONTACT* )
{
    std::vector<VECTOR2I> intersections = aEllipse.Intersect( aArc );

    aIntersections.insert( aIntersections.end(), intersections.begin(), intersections.end() );
}

void findIntersections( const SHAPE_ELLIPSE& aEllipseA, const SHAPE_ELLIPSE& aEllipseB,
                        std::vector<VECTOR2I>& aIntersections, INTERSECTION_CONTACT* )
{
    std::vector<VECTOR2I> intersections = aEllipseA.Intersect( aEllipseB );

    aIntersections.insert( aIntersections.end(), intersections.begin(), intersections.end() );
}

} // namespace


INTERSECTION_VISITOR::INTERSECTION_VISITOR( const INTERSECTABLE_GEOM& aOtherGeometry,
                                            std::vector<VECTOR2I>&    aIntersections ) :
        m_otherGeometry( aOtherGeometry ), m_intersections( aIntersections )
{
}


INTERSECTION_VISITOR::INTERSECTION_VISITOR( const INTERSECTABLE_GEOM& aOtherGeometry,
                                            std::vector<VECTOR2I>& aIntersections,
                                            INTERSECTION_CONTACT&  aContact ) :
        m_otherGeometry( aOtherGeometry ), m_intersections( aIntersections ), m_contact( &aContact )
{
}

/*
 * The operator() functions are the entry points for the visitor.
 *
 * Dispatch to the correct function based on the type of the "otherGeometry"
 * which is held as state. This is also where the order of the parameters is
 * determined, which avoids having to define a 'reverse' function for each
 * intersection type.
 */

void INTERSECTION_VISITOR::operator()( const SEG& aSeg ) const
{
    // Dispatch to the correct function
    return std::visit(
            [&]( const auto& otherGeom )
            {
                using OtherGeomType = std::decay_t<decltype( otherGeom )>;

                if constexpr( std::is_same_v<OtherGeomType, BOX2I> )
                {
                    // Seg-Rect via decomposition into segments
                    for( const SEG& aRectSeg : KIGEOM::BoxToSegs( otherGeom ) )
                    {
                        findIntersections( aSeg, aRectSeg, m_intersections, m_contact );
                    }
                }
                else if constexpr( std::is_same_v<OtherGeomType, SHAPE_ELLIPSE> )
                {
                    // Ellipse-Seg
                    findIntersections( otherGeom, aSeg, m_intersections, m_contact );
                }
                else
                {
                    // In all other segment comparisons, the SEG is the first argument
                    findIntersections( aSeg, otherGeom, m_intersections, m_contact );
                }
            },
            m_otherGeometry );
}

void INTERSECTION_VISITOR::operator()( const LINE& aLine ) const
{
    // Dispatch to the correct function
    return std::visit(
            [&]( const auto& otherGeom )
            {
                using OtherGeomType = std::decay_t<decltype( otherGeom )>;
                // Dispatch in the correct order
                if constexpr( std::is_same_v<OtherGeomType, SEG> || std::is_same_v<OtherGeomType, LINE>
                              || std::is_same_v<OtherGeomType, CIRCLE> || std::is_same_v<OtherGeomType, SHAPE_ARC>
                              || std::is_same_v<OtherGeomType, SHAPE_ELLIPSE> )
                {
                    // Seg-Line, Line-Line, Circle-Line, Arc-Line, Ellipse-Line
                    findIntersections( otherGeom, aLine, m_intersections, m_contact );
                }
                else if constexpr( std::is_same_v<OtherGeomType, HALF_LINE> )
                {
                    // Line-HalfLine
                    findIntersections( aLine, otherGeom, m_intersections, m_contact );
                }
                else if constexpr( std::is_same_v<OtherGeomType, BOX2I> )
                {
                    // Line-Rect via decomposition into segments
                    for( const SEG& aRectSeg : KIGEOM::BoxToSegs( otherGeom ) )
                    {
                        findIntersections( aRectSeg, aLine, m_intersections, m_contact );
                    }
                }
                else
                {
                    static_assert( always_false<OtherGeomType>::value,
                                   "Unhandled other geometry type" );
                }
            },
            m_otherGeometry );
};

void INTERSECTION_VISITOR::operator()( const HALF_LINE& aHalfLine ) const
{
    // Dispatch to the correct function
    return std::visit(
            [&]( const auto& otherGeom )
            {
                using OtherGeomType = std::decay_t<decltype( otherGeom )>;
                // Dispatch in the correct order
                if constexpr( std::is_same_v<OtherGeomType, SEG> || std::is_same_v<OtherGeomType, HALF_LINE>
                              || std::is_same_v<OtherGeomType, CIRCLE> || std::is_same_v<OtherGeomType, SHAPE_ARC>
                              || std::is_same_v<OtherGeomType, SHAPE_ELLIPSE> )
                {
                    // Seg-HalfLine, HalfLine-HalfLine, Circle-HalfLine, Arc-HalfLine,
                    // Ellipse-HalfLine
                    findIntersections( otherGeom, aHalfLine, m_intersections, m_contact );
                }
                else if constexpr( std::is_same_v<OtherGeomType, LINE> )
                {
                    // Line-HalfLine
                    findIntersections( otherGeom, aHalfLine, m_intersections, m_contact );
                }
                else if constexpr( std::is_same_v<OtherGeomType, BOX2I> )
                {
                    // HalfLine-Rect via decomposition into segments
                    for( const SEG& aRectSeg : KIGEOM::BoxToSegs( otherGeom ) )
                    {
                        findIntersections( aRectSeg, aHalfLine, m_intersections, m_contact );
                    }
                }
                else
                {
                    static_assert( always_false<OtherGeomType>::value,
                                   "Unhandled other geometry type" );
                }
            },
            m_otherGeometry );
};

void INTERSECTION_VISITOR::operator()( const CIRCLE& aCircle ) const
{
    // Dispatch to the correct function
    return std::visit(
            [&]( const auto& otherGeom )
            {
                using OtherGeomType = std::decay_t<decltype( otherGeom )>;
                // Dispatch in the correct order
                if constexpr( std::is_same_v<OtherGeomType, SEG> || std::is_same_v<OtherGeomType, CIRCLE>
                              || std::is_same_v<OtherGeomType, SHAPE_ELLIPSE> )
                {
                    // Seg-Circle, Circle-Circle, Ellipse-Circle
                    findIntersections( otherGeom, aCircle, m_intersections, m_contact );
                }
                else if constexpr( std::is_same_v<OtherGeomType, SHAPE_ARC>
                                   || std::is_same_v<OtherGeomType, LINE>
                                   || std::is_same_v<OtherGeomType, HALF_LINE> )
                {
                    // Circle-Arc, Circle-Line, Circle-HalfLine
                    findIntersections( aCircle, otherGeom, m_intersections, m_contact );
                }
                else if constexpr( std::is_same_v<OtherGeomType, BOX2I> )
                {
                    // Circle-Rect via decomposition into segments
                    for( const SEG& aRectSeg : KIGEOM::BoxToSegs( otherGeom ) )
                    {
                        findIntersections( aRectSeg, aCircle, m_intersections, m_contact );
                    }
                }
                else
                {
                    static_assert( always_false<OtherGeomType>::value,
                                   "Unhandled other geometry type" );
                }
            },
            m_otherGeometry );
}

void INTERSECTION_VISITOR::operator()( const SHAPE_ARC& aArc ) const
{
    // Dispatch to the correct function
    return std::visit(
            [&]( const auto& otherGeom )
            {
                using OtherGeomType = std::decay_t<decltype( otherGeom )>;
                // Dispatch in the correct order
                if constexpr( std::is_same_v<OtherGeomType, SEG> || std::is_same_v<OtherGeomType, CIRCLE>
                              || std::is_same_v<OtherGeomType, SHAPE_ARC>
                              || std::is_same_v<OtherGeomType, SHAPE_ELLIPSE> )
                {
                    // Seg-Arc, Circle-Arc, Arc-Arc, Ellipse-Arc
                    findIntersections( otherGeom, aArc, m_intersections, m_contact );
                }
                else if constexpr( std::is_same_v<OtherGeomType, LINE>
                                   || std::is_same_v<OtherGeomType, HALF_LINE> )
                {
                    // Arc-Line, Arc-HalfLine
                    findIntersections( aArc, otherGeom, m_intersections, m_contact );
                }
                else if constexpr( std::is_same_v<OtherGeomType, BOX2I> )
                {
                    // Arc-Rect via decomposition into segments
                    for( const SEG& aRectSeg : KIGEOM::BoxToSegs( otherGeom ) )
                    {
                        findIntersections( aRectSeg, aArc, m_intersections, m_contact );
                    }
                }
                else
                {
                    static_assert( always_false<OtherGeomType>::value,
                                   "Unhandled other geometry type" );
                }
            },
            m_otherGeometry );
};


void INTERSECTION_VISITOR::operator()( const SHAPE_ELLIPSE& aEllipse ) const
{
    // Dispatch to the correct function
    return std::visit(
            [&]( const auto& otherGeom )
            {
                using OtherGeomType = std::decay_t<decltype( otherGeom )>;
                // The ellipse is always the first argument
                if constexpr( std::is_same_v<OtherGeomType, SEG> || std::is_same_v<OtherGeomType, LINE>
                              || std::is_same_v<OtherGeomType, HALF_LINE> || std::is_same_v<OtherGeomType, CIRCLE>
                              || std::is_same_v<OtherGeomType, SHAPE_ARC>
                              || std::is_same_v<OtherGeomType, SHAPE_ELLIPSE> )
                {
                    findIntersections( aEllipse, otherGeom, m_intersections, m_contact );
                }
                else if constexpr( std::is_same_v<OtherGeomType, BOX2I> )
                {
                    // Ellipse-Rect via decomposition into segments
                    for( const SEG& aRectSeg : KIGEOM::BoxToSegs( otherGeom ) )
                    {
                        findIntersections( aEllipse, aRectSeg, m_intersections, m_contact );
                    }
                }
                else
                {
                    static_assert( always_false<OtherGeomType>::value, "Unhandled other geometry type" );
                }
            },
            m_otherGeometry );
};


void INTERSECTION_VISITOR::operator()( const BOX2I& aRect ) const
{
    // Defer to the SEG visitor repeatedly
    // Note - in some cases, points can be repeated in the intersection list
    // if that's an issue, both directions of the visitor can be implemented
    // to take care of that.
    const std::array<SEG, 4> segs = KIGEOM::BoxToSegs( aRect );

    for( const SEG& seg : segs )
    {
        ( *this )( seg );
    }
};