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

#include "geometry/nearest.h"

#include <wx/debug.h>

#include <core/type_helpers.h>

#include <geometry/shape_utils.h>


namespace
{

VECTOR2I NearestPoint( const BOX2I& aBox, const VECTOR2I& aPt )
{
    VECTOR2I nearest;
    int      bestDistance = std::numeric_limits<int>::max();

    for( const SEG& seg : KIGEOM::BoxToSegs( aBox ) )
    {
        const VECTOR2I nearestSegPt = seg.NearestPoint( aPt );
        const int      thisDistance = nearestSegPt.Distance( aPt );

        if( thisDistance <= bestDistance )
        {
            nearest = nearestSegPt;
            bestDistance = thisDistance;
        }
    }
    return nearest;
};

} // namespace


VECTOR2I GetNearestPoint( const NEARABLE_GEOM& aGeom, const VECTOR2I& aPt )
{
    VECTOR2I nearest;

    std::visit(
            [&]( const auto& geom )
            {
                using GeomType = std::decay_t<decltype( geom )>;

                if constexpr( std::is_same_v<GeomType, LINE>
                                || std::is_same_v<GeomType, HALF_LINE>
                                || std::is_same_v<GeomType, SEG>
                                || std::is_same_v<GeomType, CIRCLE>
                                || std::is_same_v<GeomType, SHAPE_ARC> )
                {
                    // Same signatures for all these types
                    // But they're not in the same polymorphic hierarchy
                    nearest = geom.NearestPoint( aPt );
                }
                else if constexpr( std::is_same_v<GeomType, BOX2I> )
                {
                    // Defer to the utils function
                    nearest = NearestPoint( geom, aPt );
                }
                else if constexpr( std::is_same_v<GeomType, VECTOR2I> )
                {
                    nearest = geom;
                }
                else
                {
                    static_assert( always_false<GeomType>::value, "non-exhaustive visitor" );
                }
            },
            aGeom );

    return nearest;
}

OPT_VECTOR2I GetNearestPoint( const std::vector<NEARABLE_GEOM>& aGeoms, const VECTOR2I& aPt )
{
    OPT_VECTOR2I nearestPointOnAny;
    int          bestDistance = std::numeric_limits<int>::max();

    for( const NEARABLE_GEOM& geom : aGeoms )
    {
        const VECTOR2I thisNearest = GetNearestPoint( geom, aPt );
        const int      thisDistance = thisNearest.Distance( aPt );

        if( !nearestPointOnAny || thisDistance < bestDistance )
        {
            nearestPointOnAny = thisNearest;
            bestDistance = thisDistance;
        }
    }

    return nearestPointOnAny;
}