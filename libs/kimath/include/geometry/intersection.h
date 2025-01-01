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
 * along with this program; if not, you may find one here:
 * http://www.gnu.org/licenses/old-licenses/gpl-2.0.html
 * or you may search the http://www.gnu.org website for the version 2 license,
 * or you may write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
 */

#pragma once

#include <variant>
#include <vector>

#include <math/vector2d.h>
#include <math/box2.h>

#include <geometry/circle.h>
#include <geometry/half_line.h>
#include <geometry/line.h>
#include <geometry/seg.h>
#include <geometry/shape_arc.h>
#include <geometry/shape_rect.h>

/**
 * A variant type that can hold any of the supported geometry types
 * for intersection calculations.
 */
using INTERSECTABLE_GEOM = std::variant<LINE, HALF_LINE, SEG, CIRCLE, SHAPE_ARC, BOX2I>;

/**
 * A visitor that visits INTERSECTABLE_GEOM variant objects with another
 * (which is held as state: m_otherGeometry).
 *
 * This provides a unified way to intersect any supported geometry with
 * any other supported geometry.
 */
struct INTERSECTION_VISITOR
{
public:
    /**
     * @param aOtherGeometry The other geometry to intersect the visited geometry with.
     * @param aIntersections A vector to store the intersections in. Does not have to
     *                       be empty, the visitor will append to it.
     */
    INTERSECTION_VISITOR( const INTERSECTABLE_GEOM& aOtherGeometry,
                          std::vector<VECTOR2I>&    aIntersections );

    /*
     * One of these operator() overloads will be called by std::visit
     * as needed to visit (i.e. intersect) the geometry with the (stored)
     * other geometry.
     */
    void operator()( const SEG& aSeg ) const;
    void operator()( const LINE& aLine ) const;
    void operator()( const HALF_LINE& aLine ) const;
    void operator()( const CIRCLE& aCircle ) const;
    void operator()( const SHAPE_ARC& aArc ) const;
    void operator()( const BOX2I& aArc ) const;

private:
    const INTERSECTABLE_GEOM& m_otherGeometry;
    std::vector<VECTOR2I>&    m_intersections;
};