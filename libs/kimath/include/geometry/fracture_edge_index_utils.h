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
 * along with this program; if not, see <https://www.gnu.org/licenses/>.
 */

#ifndef FRACTURE_EDGE_INDEX_UTILS_H
#define FRACTURE_EDGE_INDEX_UTILS_H

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <utility>

namespace KIGEOM::FRACTURE_INDEX
{
constexpr uint32_t MAX_STRIPES = 65536;
constexpr uint32_t MAX_BUCKET_SPAN = 8;
// Polygons needing a larger index fall back to the linear scan, trading the worst case on the
// very largest polygons for a bounded zone fill footprint
constexpr size_t   MAX_CAPACITY_BYTES = size_t( 8 ) * 1024 * 1024;
constexpr uint64_t MIN_EDGE_VISITS = 32768;
constexpr size_t   MIN_HOLE_COUNT = 8;

inline uint32_t StripeCountFor( size_t aEdgeCount )
{
    const double count = std::clamp( std::sqrt( static_cast<double>( aEdgeCount ) ), 1.0,
                                     static_cast<double>( MAX_STRIPES ) );
    return static_cast<uint32_t>( count );
}

inline uint32_t MapYToStripe( int aY, int aMinY, int aMaxY, uint32_t aStripeCount )
{
    // An inverted extent would divide by a non-positive range and violate the clamp bounds
    if( aMaxY < aMinY )
        return 0;

    const int64_t  range = int64_t( aMaxY ) - int64_t( aMinY ) + 1;
    const int64_t  offset = std::clamp<int64_t>( int64_t( aY ) - int64_t( aMinY ), 0, range - 1 );
    const uint64_t mapped = uint64_t( offset ) * aStripeCount / uint64_t( range );
    return std::min<uint32_t>( static_cast<uint32_t>( mapped ), aStripeCount - 1 );
}

inline std::pair<uint32_t, uint32_t> StripeSpan( int aY1, int aY2, int aMinY, int aMaxY, uint32_t aStripeCount )
{
    return { MapYToStripe( std::min( aY1, aY2 ), aMinY, aMaxY, aStripeCount ),
             MapYToStripe( std::max( aY1, aY2 ), aMinY, aMaxY, aStripeCount ) };
}

inline bool CheckedAdd( size_t& aTotal, size_t aCount, size_t aElementSize )
{
    if( aElementSize == 0 )
        return true;

    if( aCount > ( std::numeric_limits<size_t>::max() - aTotal ) / aElementSize )
        return false;

    aTotal += aCount * aElementSize;
    return true;
}

inline bool CapacityFits( size_t aBucketIds, size_t aLongIds, size_t aStripeCount, size_t aHoleCount, size_t aNodeSize )
{
    size_t bytes = 0;

    return CheckedAdd( bytes, aBucketIds, sizeof( uint32_t ) ) && CheckedAdd( bytes, aLongIds, sizeof( uint32_t ) )
           && CheckedAdd( bytes, aStripeCount + 1, sizeof( uint32_t ) )
           && CheckedAdd( bytes, aStripeCount, sizeof( uint32_t ) )
           && CheckedAdd( bytes, aStripeCount + 1, sizeof( uint32_t ) )
           && CheckedAdd( bytes, ( MAX_BUCKET_SPAN + 2 ) * aHoleCount, aNodeSize ) && bytes <= MAX_CAPACITY_BYTES;
}

inline bool ActualCapacityFits( size_t aBucketIds, size_t aLongIds, size_t aOffsets, size_t aScratch, size_t aHeads,
                                size_t aNodes, size_t aNodeSize, size_t* aBytes = nullptr )
{
    size_t bytes = 0;

    const bool valid = CheckedAdd( bytes, aBucketIds, sizeof( uint32_t ) )
                       && CheckedAdd( bytes, aLongIds, sizeof( uint32_t ) )
                       && CheckedAdd( bytes, aOffsets, sizeof( uint32_t ) )
                       && CheckedAdd( bytes, aScratch, sizeof( uint32_t ) )
                       && CheckedAdd( bytes, aHeads, sizeof( uint32_t ) ) && CheckedAdd( bytes, aNodes, aNodeSize );

    if( aBytes )
        *aBytes = valid ? bytes : 0;

    return valid && bytes <= MAX_CAPACITY_BYTES;
}

inline bool ShouldIndex( uint64_t aEstimatedVisits, size_t aHoleCount )
{
    return aHoleCount >= MIN_HOLE_COUNT && aEstimatedVisits >= MIN_EDGE_VISITS;
}
} // namespace KIGEOM::FRACTURE_INDEX

#endif
