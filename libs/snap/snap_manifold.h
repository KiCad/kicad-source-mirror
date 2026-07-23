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

#ifndef SNAP_MANIFOLD_H
#define SNAP_MANIFOLD_H

#include <geometry/intersection.h>
#include <math/vector2d.h>

#include <cmath>
#include <type_traits>
#include <variant>

/**
 * Distance from a point to the nearest point of a snap manifold shape.
 *
 * The inference and resolver stages both measure candidate proximity, so the per-shape distance
 * handling lives here to keep the two stages in agreement.
 */
inline double snapManifoldDistance( const INTERSECTABLE_GEOM& aGeometry, const VECTOR2I& aPoint )
{
    return std::visit(
            [&]( const auto& aShape ) -> double
            {
                using MANIFOLD_TYPE = std::decay_t<decltype( aShape )>;

                if constexpr( std::is_same_v<MANIFOLD_TYPE, LINE> )
                    return aShape.Distance( aPoint );
                else if constexpr( std::is_same_v<MANIFOLD_TYPE, HALF_LINE> )
                    return aShape.NearestPoint( aPoint ).Distance( aPoint );
                else if constexpr( std::is_same_v<MANIFOLD_TYPE, SEG> )
                    return aShape.Distance( aPoint );
                else if constexpr( std::is_same_v<MANIFOLD_TYPE, CIRCLE> )
                    return std::abs( aShape.Center.Distance( aPoint ) - aShape.Radius );
                else if constexpr( std::is_same_v<MANIFOLD_TYPE, SHAPE_ARC> )
                    return aShape.NearestPoint( aPoint ).Distance( aPoint );
                else
                    return aShape.Distance( aPoint );
            },
            aGeometry );
}

#endif
