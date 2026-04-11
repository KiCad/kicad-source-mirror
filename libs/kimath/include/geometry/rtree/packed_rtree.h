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

#ifndef PACKED_RTREE_H
#define PACKED_RTREE_H

#include <algorithm>
#include <bit>
#include <cassert>
#include <climits>
#include <cstdint>
#include <cstring>
#include <functional>
#include <limits>
#include <utility>
#include <vector>

#include <geometry/rtree/rtree_node.h>

namespace KIRTREE
{

/**
 * Static (immutable) packed R-tree built via Hilbert-curve bulk loading.
 *
 * Provides optimal query performance for build-once-query-many workloads such as DRC
 * spatial indexing. The tree is constructed in O(N log N) time via Hilbert curve sorting
 * and stored in a single contiguous buffer with bottom-up level ordering.
 *
 * Node bounding boxes use Structure-of-Arrays layout with fanout=FANOUT for cache-efficient
 * scanning during queries.
 *
 * Thread-safe for concurrent reads after construction.
 *
 * @tparam DATATYPE  Type of data stored (e.g. pointer to item)
 * @tparam ELEMTYPE  Coordinate element type (typically int)
 * @tparam NUMDIMS   Number of dimensions (2 or 3)
 * @tparam FANOUT    Maximum children per node (default 16 for SIMD alignment)
 */
template <class DATATYPE, class ELEMTYPE = int, int NUMDIMS = 2, int FANOUT = 16>
class PACKED_RTREE
{
public:
    PACKED_RTREE() = default;

    // Move-only
    PACKED_RTREE( PACKED_RTREE&& aOther ) noexcept :
            m_counts( std::move( aOther.m_counts ) ),
            m_data( std::move( aOther.m_data ) ),
            m_levelOffsets( std::move( aOther.m_levelOffsets ) ),
            m_size( std::exchange( aOther.m_size, size_t{ 0 } ) )
    {
        for( int i = 0; i < NUMDIMS * 2; ++i )
            m_bounds[i] = std::move( aOther.m_bounds[i] );
    }

    PACKED_RTREE& operator=( PACKED_RTREE&& aOther ) noexcept
    {
        if( this != &aOther )
        {
            for( int i = 0; i < NUMDIMS * 2; ++i )
                m_bounds[i] = std::move( aOther.m_bounds[i] );

            m_counts = std::move( aOther.m_counts );
            m_data = std::move( aOther.m_data );
            m_levelOffsets = std::move( aOther.m_levelOffsets );
            m_size = aOther.m_size;
            aOther.m_size = 0;
        }

        return *this;
    }

    PACKED_RTREE( const PACKED_RTREE& ) = delete;
    PACKED_RTREE& operator=( const PACKED_RTREE& ) = delete;

    /**
     * Search for all items whose bounding boxes overlap the query rectangle.
     *
     * @param aMin     Minimum corner of query rectangle
     * @param aMax     Maximum corner of query rectangle
     * @param aVisitor Callback invoked for each matching item. Return false to stop early.
     * @return Number of items reported to visitor
     */
    template <class VISITOR>
    int Search( const ELEMTYPE aMin[NUMDIMS], const ELEMTYPE aMax[NUMDIMS],
                VISITOR& aVisitor ) const
    {
        if( m_size == 0 )
            return 0;

        int found = 0;
        int rootLevel = static_cast<int>( m_levelOffsets.size() ) - 1;
        searchNode( rootLevel, m_levelOffsets[rootLevel], aMin, aMax, aVisitor, found );
        return found;
    }

    /**
     * Iterator for sequential access to all stored data items.
     */
    class Iterator
    {
    public:
        Iterator( const DATATYPE* aPtr ) : m_ptr( aPtr ) {}

        const DATATYPE& operator*() const { return *m_ptr; }
        const DATATYPE* operator->() const { return m_ptr; }

        Iterator& operator++()
        {
            ++m_ptr;
            return *this;
        }

        Iterator operator++( int )
        {
            Iterator tmp = *this;
            ++m_ptr;
            return tmp;
        }

        bool operator==( const Iterator& aOther ) const { return m_ptr == aOther.m_ptr; }
        bool operator!=( const Iterator& aOther ) const { return m_ptr != aOther.m_ptr; }

    private:
        const DATATYPE* m_ptr;
    };

    Iterator begin() const { return Iterator( m_data.data() ); }
    Iterator end() const { return Iterator( m_data.data() + m_size ); }

    size_t size() const { return m_size; }
    bool   empty() const { return m_size == 0; }

    /**
     * Return approximate memory usage in bytes.
     */
    size_t MemoryUsage() const
    {
        size_t usage = 0;

        for( int axis = 0; axis < NUMDIMS * 2; ++axis )
            usage += m_bounds[axis].capacity() * sizeof( ELEMTYPE );

        usage += m_counts.capacity() * sizeof( int );
        usage += m_data.capacity() * sizeof( DATATYPE );
        usage += m_levelOffsets.capacity() * sizeof( size_t );
        return usage;
    }

    /**
     * Builder for constructing a PACKED_RTREE from a set of items.
     *
     * Usage:
     *   Builder builder;
     *   builder.Reserve( N );
     *   for each item: builder.Add( min, max, data );
     *   PACKED_RTREE tree = builder.Build();
     */
    class Builder
    {
    public:
        Builder() = default;

        void Reserve( size_t aCount )
        {
            m_items.reserve( aCount );
        }

        void Add( const ELEMTYPE aMin[NUMDIMS], const ELEMTYPE aMax[NUMDIMS],
                  const DATATYPE& aData )
        {
            ITEM item;
            item.data = aData;

            for( int d = 0; d < NUMDIMS; ++d )
            {
                item.min[d] = aMin[d];
                item.max[d] = aMax[d];
            }

            m_items.push_back( item );
        }

        PACKED_RTREE Build()
        {
            PACKED_RTREE tree;
            size_t       n = m_items.size();
            tree.m_size = n;

            if( n == 0 )
                return tree;

            // Compute global bounding box of all item centers for Hilbert normalization
            ELEMTYPE globalMin[NUMDIMS];
            ELEMTYPE globalMax[NUMDIMS];

            for( int d = 0; d < NUMDIMS; ++d )
            {
                globalMin[d] = std::numeric_limits<ELEMTYPE>::max();
                globalMax[d] = std::numeric_limits<ELEMTYPE>::lowest();
            }

            for( const ITEM& item : m_items )
            {
                for( int d = 0; d < NUMDIMS; ++d )
                {
                    ELEMTYPE center = item.min[d] / 2 + item.max[d] / 2;

                    if( center < globalMin[d] )
                        globalMin[d] = center;

                    if( center > globalMax[d] )
                        globalMax[d] = center;
                }
            }

            // Compute Hilbert indices for sorting
            std::vector<uint64_t> hilbertValues( n );

            for( size_t i = 0; i < n; ++i )
            {
                uint32_t coords[NUMDIMS];

                for( int d = 0; d < NUMDIMS; ++d )
                {
                    ELEMTYPE center = m_items[i].min[d] / 2 + m_items[i].max[d] / 2;
                    double   range = static_cast<double>( globalMax[d] ) - globalMin[d];

                    if( range > 0.0 )
                    {
                        double normalized = ( static_cast<double>( center ) - globalMin[d] )
                                            / range;
                        coords[d] = static_cast<uint32_t>(
                                normalized * static_cast<double>( UINT32_MAX ) );
                    }
                    else
                    {
                        coords[d] = 0;
                    }
                }

                hilbertValues[i] = HilbertND2D<NUMDIMS>( 32, coords );
            }

            // Sort items by Hilbert value
            std::vector<size_t> indices( n );

            for( size_t i = 0; i < n; ++i )
                indices[i] = i;

            std::sort( indices.begin(), indices.end(),
                       [&hilbertValues]( size_t a, size_t b )
                       {
                           return hilbertValues[a] < hilbertValues[b];
                       } );

            // Store sorted data
            tree.m_data.resize( n );

            for( size_t i = 0; i < n; ++i )
                tree.m_data[i] = m_items[indices[i]].data;

            // Build leaf nodes: group every FANOUT items
            size_t numLeafNodes = ( n + FANOUT - 1 ) / FANOUT;

            // We'll build the tree bottom-up, tracking nodes per level
            std::vector<size_t> nodesPerLevel;
            nodesPerLevel.push_back( numLeafNodes );

            size_t levelNodes = numLeafNodes;

            while( levelNodes > 1 )
            {
                levelNodes = ( levelNodes + FANOUT - 1 ) / FANOUT;
                nodesPerLevel.push_back( levelNodes );
            }

            // If we had exactly one item, we have one leaf node and that's the root
            size_t totalNodes = 0;

            for( size_t cnt : nodesPerLevel )
                totalNodes += cnt;

            // Allocate SoA bounds storage and counts
            for( int axis = 0; axis < NUMDIMS * 2; ++axis )
                tree.m_bounds[axis].resize( totalNodes * FANOUT,
                                            std::numeric_limits<ELEMTYPE>::max() );

            tree.m_counts.resize( totalNodes, 0 );

            // Compute level offsets (leaf level = 0, root = highest)
            tree.m_levelOffsets.resize( nodesPerLevel.size() );
            size_t offset = 0;

            for( size_t lev = 0; lev < nodesPerLevel.size(); ++lev )
            {
                tree.m_levelOffsets[lev] = offset;
                offset += nodesPerLevel[lev];
            }

            // Fill leaf nodes with item bounding boxes
            for( size_t i = 0; i < n; ++i )
            {
                size_t nodeIdx = i / FANOUT;
                int    slot = static_cast<int>( i % FANOUT );
                size_t globalNodeIdx = tree.m_levelOffsets[0] + nodeIdx;

                const ITEM& item = m_items[indices[i]];

                for( int d = 0; d < NUMDIMS; ++d )
                {
                    tree.m_bounds[d * 2][globalNodeIdx * FANOUT + slot] = item.min[d];
                    tree.m_bounds[d * 2 + 1][globalNodeIdx * FANOUT + slot] = item.max[d];
                }

                tree.m_counts[globalNodeIdx] = slot + 1;
            }

            // Build internal levels bottom-up
            for( size_t lev = 1; lev < nodesPerLevel.size(); ++lev )
            {
                size_t childLevelOffset = tree.m_levelOffsets[lev - 1];
                size_t childLevelCount = nodesPerLevel[lev - 1];
                size_t parentLevelOffset = tree.m_levelOffsets[lev];

                for( size_t ci = 0; ci < childLevelCount; ++ci )
                {
                    size_t parentIdx = ci / FANOUT;
                    int    slot = static_cast<int>( ci % FANOUT );
                    size_t globalParentIdx = parentLevelOffset + parentIdx;
                    size_t globalChildIdx = childLevelOffset + ci;

                    // Compute the bounding box of the child node (union of all its slots)
                    ELEMTYPE childMin[NUMDIMS];
                    ELEMTYPE childMax[NUMDIMS];

                    for( int d = 0; d < NUMDIMS; ++d )
                    {
                        childMin[d] = std::numeric_limits<ELEMTYPE>::max();
                        childMax[d] = std::numeric_limits<ELEMTYPE>::lowest();
                    }

                    int childCount = tree.m_counts[globalChildIdx];

                    for( int s = 0; s < childCount; ++s )
                    {
                        for( int d = 0; d < NUMDIMS; ++d )
                        {
                            ELEMTYPE mn = tree.m_bounds[d * 2][globalChildIdx * FANOUT + s];
                            ELEMTYPE mx = tree.m_bounds[d * 2 + 1][globalChildIdx * FANOUT + s];

                            if( mn < childMin[d] )
                                childMin[d] = mn;

                            if( mx > childMax[d] )
                                childMax[d] = mx;
                        }
                    }

                    // Store this child node's bbox in the parent
                    for( int d = 0; d < NUMDIMS; ++d )
                    {
                        tree.m_bounds[d * 2][globalParentIdx * FANOUT + slot] = childMin[d];
                        tree.m_bounds[d * 2 + 1][globalParentIdx * FANOUT + slot] = childMax[d];
                    }

                    tree.m_counts[globalParentIdx] = slot + 1;
                }
            }

            m_items.clear();
            return tree;
        }

    private:
        struct ITEM
        {
            DATATYPE data;
            ELEMTYPE min[NUMDIMS];
            ELEMTYPE max[NUMDIMS];
        };

        std::vector<ITEM> m_items;
    };

private:
    // Returns true if search should continue, false if visitor terminated early
    template <class VISITOR>
    bool searchNode( int aLevel, size_t aNodeIdx, const ELEMTYPE aMin[NUMDIMS],
                     const ELEMTYPE aMax[NUMDIMS], VISITOR& aVisitor, int& aFound ) const
    {
        if constexpr( NUMDIMS == 2 )
        {
            // Test all FANOUT children at once via SIMD overlap mask.
            // Each bounds array is contiguous for FANOUT slots at [nodeIdx * FANOUT].
            size_t base = aNodeIdx * FANOUT;
            uint32_t mask = OverlapMask2D<ELEMTYPE, FANOUT>(
                    &m_bounds[0][base], &m_bounds[1][base],
                    &m_bounds[2][base], &m_bounds[3][base],
                    m_counts[aNodeIdx], aMin, aMax );

            if( aLevel == 0 )
            {
                size_t dataBase = ( aNodeIdx - m_levelOffsets[0] ) * FANOUT;

                // Iterate set bits to visit only overlapping leaf children
                while( mask )
                {
                    int i = std::countr_zero( mask );
                    mask &= mask - 1;

                    size_t dataIdx = dataBase + i;

                    if( dataIdx < m_size )
                    {
                        aFound++;

                        if( !aVisitor( m_data[dataIdx] ) )
                            return false;
                    }
                }
            }
            else
            {
                size_t childLevelOffset = m_levelOffsets[aLevel - 1];
                size_t nodeOffset = aNodeIdx - m_levelOffsets[aLevel];

                // Iterate set bits to recurse only into overlapping internal children
                while( mask )
                {
                    int i = std::countr_zero( mask );
                    mask &= mask - 1;

                    size_t childGlobalIdx = childLevelOffset + nodeOffset * FANOUT + i;

                    if( !searchNode( aLevel - 1, childGlobalIdx, aMin, aMax, aVisitor,
                                     aFound ) )
                    {
                        return false;
                    }
                }
            }
        }
        else
        {
            // Generic N-dimensional fallback: per-child scalar overlap test
            int nodeCount = m_counts[aNodeIdx];

            if( aLevel == 0 )
            {
                for( int i = 0; i < nodeCount; ++i )
                {
                    if( childOverlaps( aNodeIdx, i, aMin, aMax ) )
                    {
                        size_t dataIdx = ( aNodeIdx - m_levelOffsets[0] ) * FANOUT + i;

                        if( dataIdx < m_size )
                        {
                            aFound++;

                            if( !aVisitor( m_data[dataIdx] ) )
                                return false;
                        }
                    }
                }
            }
            else
            {
                size_t childLevelOffset = m_levelOffsets[aLevel - 1];

                for( int i = 0; i < nodeCount; ++i )
                {
                    if( childOverlaps( aNodeIdx, i, aMin, aMax ) )
                    {
                        size_t childBaseIdx =
                                ( aNodeIdx - m_levelOffsets[aLevel] ) * FANOUT + i;
                        size_t childGlobalIdx = childLevelOffset + childBaseIdx;

                        if( !searchNode( aLevel - 1, childGlobalIdx, aMin, aMax, aVisitor,
                                         aFound ) )
                        {
                            return false;
                        }
                    }
                }
            }
        }

        return true;
    }

    bool childOverlaps( size_t aNodeIdx, int aSlot, const ELEMTYPE aMin[NUMDIMS],
                        const ELEMTYPE aMax[NUMDIMS] ) const
    {
        size_t idx = aNodeIdx * FANOUT + aSlot;

        for( int d = 0; d < NUMDIMS; ++d )
        {
            if( m_bounds[d * 2][idx] > aMax[d] || m_bounds[d * 2 + 1][idx] < aMin[d] )
                return false;
        }

        return true;
    }

    // SoA bounding boxes: m_bounds[axis][globalNodeIdx * FANOUT + slot]
    std::vector<ELEMTYPE> m_bounds[NUMDIMS * 2];

    // Number of valid children per node
    std::vector<int> m_counts;

    // Sorted data items (Hilbert-ordered)
    std::vector<DATATYPE> m_data;

    // Node index where each level starts (level 0 = leaves, last = root)
    std::vector<size_t> m_levelOffsets;

    // Number of data items
    size_t m_size = 0;
};

} // namespace KIRTREE

#endif // PACKED_RTREE_H
