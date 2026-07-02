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

#pragma once

/**
 * @file geometry/geometry_predicates.h
 * @brief Exact orientation and in-circle predicates over integer coordinates.
 */

#include <algorithm>
#include <cmath>
#include <cstdint>

#include <math/vector2d.h>

namespace KIGEOM
{

/// Orientation of triangle (a, b, c): +1 counter-clockwise, -1 clockwise, 0 collinear.
inline int OrientationSign( const VECTOR2I& a, const VECTOR2I& b, const VECTOR2I& c )
{
    // Widen before subtracting; a 32-bit coordinate difference overflows past ~2.1e9 nm.
    int64_t abX = static_cast<int64_t>( b.x ) - a.x;
    int64_t abY = static_cast<int64_t>( b.y ) - a.y;
    int64_t acX = static_cast<int64_t>( c.x ) - a.x;
    int64_t acY = static_cast<int64_t>( c.y ) - a.y;

#if defined( __SIZEOF_INT128__ )
    __int128 cross = static_cast<__int128>( abX ) * acY - static_cast<__int128>( abY ) * acX;
#else
    double cross = static_cast<double>( abX ) * static_cast<double>( acY )
                   - static_cast<double>( abY ) * static_cast<double>( acX );
#endif

    return cross > 0 ? 1 : ( cross < 0 ? -1 : 0 );
}


namespace detail
{
/// Filtered-double legality test; the tie margin treats a near-cocircular quad as legal to keep a
/// flip loop from oscillating.
inline bool inCircleLegalDouble( const VECTOR2I& a, const VECTOR2I& b, const VECTOR2I& c,
                                 const VECTOR2I& p )
{
    double paX = static_cast<double>( a.x ) - p.x, paY = static_cast<double>( a.y ) - p.y;
    double pbX = static_cast<double>( b.x ) - p.x, pbY = static_cast<double>( b.y ) - p.y;
    double pcX = static_cast<double>( c.x ) - p.x, pcY = static_cast<double>( c.y ) - p.y;

    double paSq = paX * paX + paY * paY;
    double pbSq = pbX * pbX + pbY * pbY;
    double pcSq = pcX * pcX + pcY * pcY;

    double det = paX * ( pbY * pcSq - pbSq * pcY ) - paY * ( pbX * pcSq - pbSq * pcX )
                 + paSq * ( pbX * pcY - pbY * pcX );
    double sumSq = paSq + pbSq + pcSq;

    return det <= 1e-13 * sumSq * sumSq;
}
} // namespace detail


/// True when p is outside the circumcircle of CCW triangle (a, b, c): the shared edge is already
/// Delaunay and must not be flipped. Exact, so an exact cocircular result never oscillates a flip.
inline bool InCircleDelaunayLegal( const VECTOR2I& a, const VECTOR2I& b, const VECTOR2I& c,
                                   const VECTOR2I& p )
{
#if defined( __SIZEOF_INT128__ )
    // Widen before subtracting; an int32 difference overflows past ~2.1e9 nm.
    int64_t paX = static_cast<int64_t>( a.x ) - p.x, paY = static_cast<int64_t>( a.y ) - p.y;
    int64_t pbX = static_cast<int64_t>( b.x ) - p.x, pbY = static_cast<int64_t>( b.y ) - p.y;
    int64_t pcX = static_cast<int64_t>( c.x ) - p.x, pcY = static_cast<int64_t>( c.y ) - p.y;

    // Beyond this the degree-4 determinant (~12*M^4) can exceed signed int128.
    constexpr int64_t kMaxSafeDiff = 1500000000LL;

    if( std::abs( paX ) > kMaxSafeDiff || std::abs( paY ) > kMaxSafeDiff
        || std::abs( pbX ) > kMaxSafeDiff || std::abs( pbY ) > kMaxSafeDiff
        || std::abs( pcX ) > kMaxSafeDiff || std::abs( pcY ) > kMaxSafeDiff )
    {
        return detail::inCircleLegalDouble( a, b, c, p );
    }

    __int128 paSq = static_cast<__int128>( paX ) * paX + static_cast<__int128>( paY ) * paY;
    __int128 pbSq = static_cast<__int128>( pbX ) * pbX + static_cast<__int128>( pbY ) * pbY;
    __int128 pcSq = static_cast<__int128>( pcX ) * pcX + static_cast<__int128>( pcY ) * pcY;

    __int128 det = static_cast<__int128>( paX ) * ( pbY * pcSq - pbSq * pcY )
                   - static_cast<__int128>( paY ) * ( pbX * pcSq - pbSq * pcX )
                   + paSq * ( static_cast<__int128>( pbX ) * pcY - static_cast<__int128>( pbY ) * pcX );

    return det <= 0;
#else
    return detail::inCircleLegalDouble( a, b, c, p );
#endif
}


/// A triangle is a sliver when its longest edge exceeds ten times its shortest.
inline bool IsSliverTriangle( const VECTOR2I& a, const VECTOR2I& b, const VECTOR2I& c )
{
    double abX = static_cast<double>( b.x ) - a.x, abY = static_cast<double>( b.y ) - a.y;
    double bcX = static_cast<double>( c.x ) - b.x, bcY = static_cast<double>( c.y ) - b.y;
    double caX = static_cast<double>( a.x ) - c.x, caY = static_cast<double>( a.y ) - c.y;

    double abSquared = abX * abX + abY * abY;
    double bcSquared = bcX * bcX + bcY * bcY;
    double caSquared = caX * caX + caY * caY;

    double longestSquared  = std::max( { abSquared, bcSquared, caSquared } );
    double shortestSquared = std::min( { abSquared, bcSquared, caSquared } );

    return shortestSquared > 0.0 && longestSquared > 100.0 * shortestSquared;
}

} // namespace KIGEOM
