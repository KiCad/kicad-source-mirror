/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2024 KiCad Developers, see AUTHORS.txt for contributors.
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

#include "geometry/intersection.h"

#include <core/type_helpers.h>

#include <geometry/seg.h>
#include <geometry/circle.h>
#include <geometry/shape_arc.h>
#include <geometry/shape_rect.h>

/*
 * Helper functions that dispatch to the correct intersection function
 * in one of the geometry classes.
 */
namespace
{

void findIntersections( const SEG& aSegA, const SEG& aSegB, std::vector<VECTOR2I>& aIntersections )
{
    const OPT_VECTOR2I intersection = aSegA.Intersect( aSegB );

    if( intersection )
    {
        aIntersections.push_back( *intersection );
    }
}

void findIntersections( const SEG& aSeg, const CIRCLE& aCircle,
                        std::vector<VECTOR2I>& aIntersections )
{
    std::vector<VECTOR2I> intersections = aCircle.Intersect( aSeg );

    aIntersections.insert( aIntersections.end(), intersections.begin(), intersections.end() );
}

void findIntersections( const SEG& aSeg, const SHAPE_ARC& aArc,
                        std::vector<VECTOR2I>& aIntersections )
{
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


void findIntersections( const CIRCLE& aCircleA, const CIRCLE& aCircleB,
                        std::vector<VECTOR2I>& aIntersections )
{
    std::vector<VECTOR2I> intersections = aCircleA.Intersect( aCircleB );
    aIntersections.insert( aIntersections.end(), intersections.begin(), intersections.end() );
}

void findIntersections( const CIRCLE& aCircle, const SHAPE_ARC& aArc,
                        std::vector<VECTOR2I>& aIntersections )
{
    aArc.Intersect( aCircle, &aIntersections );
}

void findIntersections( const SHAPE_ARC& aArcA, const SHAPE_ARC& aArcB,
                        std::vector<VECTOR2I>& aIntersections )
{
    aArcA.Intersect( aArcB, &aIntersections );
}


std::vector<SEG> RectToSegs( const SHAPE_RECT& aRect )
{
    const VECTOR2I corner = aRect.GetPosition();
    const int      w = aRect.GetWidth();
    const int      h = aRect.GetHeight();

    return {
        SEG( corner, { corner + VECTOR2I( w, 0 ) } ),
        SEG( { corner + VECTOR2I( w, 0 ) }, { corner + VECTOR2I( w, h ) } ),
        SEG( { corner + VECTOR2I( w, h ) }, { corner + VECTOR2I( 0, h ) } ),
        SEG( { corner + VECTOR2I( 0, h ) }, corner ),
    };
}

} // namespace


INTERSECTION_VISITOR::INTERSECTION_VISITOR( const INTERSECTABLE_GEOM& aOtherGeometry,
                                            std::vector<VECTOR2I>&    aIntersections ) :
        m_otherGeometry( aOtherGeometry ), m_intersections( aIntersections )
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

                if constexpr( std::is_same_v<OtherGeomType, SHAPE_RECT> )
                {
                    // Seg-Rect via decomposition into segments
                    for( const SEG& aRectSeg : RectToSegs( otherGeom ) )
                    {
                        findIntersections( aSeg, aRectSeg, m_intersections );
                    }
                }
                else
                {
                    // In all other segment comparisons, the SEG is the first argument
                    findIntersections( aSeg, otherGeom, m_intersections );
                }
            },
            m_otherGeometry );
}

void INTERSECTION_VISITOR::operator()( const CIRCLE& aCircle ) const
{
    // Dispatch to the correct function
    return std::visit(
            [&]( const auto& otherGeom )
            {
                using OtherGeomType = std::decay_t<decltype( otherGeom )>;
                // Dispatch in the correct order
                if constexpr( std::is_same_v<OtherGeomType, SEG>
                              || std::is_same_v<OtherGeomType, CIRCLE> )
                {
                    // Seg-Circle, Circle-Circle
                    findIntersections( otherGeom, aCircle, m_intersections );
                }
                else if constexpr( std::is_same_v<OtherGeomType, SHAPE_ARC> )
                {
                    // Circle-Arc
                    findIntersections( aCircle, otherGeom, m_intersections );
                }
                else if constexpr( std::is_same_v<OtherGeomType, SHAPE_RECT> )
                {
                    // Circle-Rect via decomposition into segments
                    for( const SEG& aRectSeg : RectToSegs( otherGeom ) )
                    {
                        findIntersections( aRectSeg, aCircle, m_intersections );
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
                if constexpr( std::is_same_v<OtherGeomType, SEG>
                              || std::is_same_v<OtherGeomType, CIRCLE>
                              || std::is_same_v<OtherGeomType, SHAPE_ARC> )
                {
                    // Seg-Arc, Circle-Arc, Arc-Arc
                    findIntersections( otherGeom, aArc, m_intersections );
                }
                else if constexpr( std::is_same_v<OtherGeomType, SHAPE_RECT> )
                {
                    // Arc-Rect via decomposition into segments
                    for( const SEG& aRectSeg : RectToSegs( otherGeom ) )
                    {
                        findIntersections( aRectSeg, aArc, m_intersections );
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


void INTERSECTION_VISITOR::operator()( const SHAPE_RECT& aRect ) const
{
    // Defer to the SEG visitor repeatedly
    // Note - in some cases, points can be repeated in the intersection list
    // if that's an issue, both directions of the visitor can be implemented
    // to take care of that.
    const std::vector<SEG> segs = RectToSegs( aRect );

    for( const SEG& seg : segs )
    {
        ( *this )( seg );
    }
};