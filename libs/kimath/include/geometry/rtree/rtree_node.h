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

#ifndef RTREE_NODE_H
#define RTREE_NODE_H

#include <atomic>
#include <bitset>
#include <cassert>
#include <cstdint>
#include <cstring>
#include <limits>
#include <memory>
#include <vector>

#if defined( __SSE2__ )
#include <emmintrin.h>
#elif defined( __ARM_NEON ) || defined( __ARM_NEON__ )
#include <arm_neon.h>
#endif

namespace KIRTREE
{

/**
 * Compute a 64-bit Hilbert curve index from 2D unsigned coordinates.
 *
 * Uses the standard iterative bit-manipulation algorithm for order-32 Hilbert curve,
 * mapping 2 uint32 coordinates to a single uint64 index. This provides excellent spatial
 * locality for R-tree bulk loading.
 *
 * @param aOrder Hilbert curve order (bits per axis, typically 32)
 * @param aX     X coordinate in [0, 2^aOrder)
 * @param aY     Y coordinate in [0, 2^aOrder)
 * @return 64-bit Hilbert index
 */
inline uint64_t HilbertXY2D( int aOrder, uint32_t aX, uint32_t aY )
{
    uint64_t d = 0;
    uint32_t x = aX;
    uint32_t y = aY;

    for( uint32_t s = ( 1U << ( aOrder - 1 ) ); s > 0; s >>= 1 )
    {
        uint32_t rx = ( x & s ) ? 1 : 0;
        uint32_t ry = ( y & s ) ? 1 : 0;

        d += static_cast<uint64_t>( s ) * s * ( ( 3 * rx ) ^ ry );

        // Rotate quadrant
        if( ry == 0 )
        {
            if( rx == 1 )
            {
                x = s * 2 - 1 - x;
                y = s * 2 - 1 - y;
            }

            uint32_t tmp = x;
            x = y;
            y = tmp;
        }
    }

    return d;
}


/**
 * Compute Hilbert index for N-dimensional coordinates.
 *
 * For 2D, delegates to the optimized HilbertXY2D. For higher dimensions, uses a generalized
 * algorithm that interleaves bits across dimensions with appropriate rotations.
 *
 * @tparam NUMDIMS number of dimensions
 * @param aOrder   Hilbert curve order (bits per axis)
 * @param aCoords  array of NUMDIMS unsigned coordinates
 * @return 64-bit Hilbert index
 */
template <int NUMDIMS>
inline uint64_t HilbertND2D( int aOrder, const uint32_t aCoords[NUMDIMS] )
{
    if constexpr( NUMDIMS == 2 )
    {
        return HilbertXY2D( aOrder, aCoords[0], aCoords[1] );
    }
    else
    {
        // For 3+ dimensions, fall back to Z-order (Morton) curve interleaving.
        // This gives good-enough clustering without the complexity of generalized
        // Hilbert in N dimensions. Each coordinate contributes aOrder bits, interleaved.
        uint64_t d = 0;

        for( int bit = aOrder - 1; bit >= 0; --bit )
        {
            for( int dim = NUMDIMS - 1; dim >= 0; --dim )
            {
                d <<= 1;
                d |= ( aCoords[dim] >> bit ) & 1;
            }
        }

        return d;
    }
}


/**
 * Compute a bitmask of which child slots overlap a 2D query rectangle.
 *
 * Bit i of the result is set when the bounding box in slot i overlaps the query.
 * Each bounds pointer must address FANOUT contiguous elements in SoA layout.
 *
 * Uses SSE2 on x86-64, NEON on ARM, or an auto-vectorization-friendly scalar
 * fallback on other architectures.
 *
 * @tparam ELEMTYPE  Coordinate type (SIMD paths require 32-bit integer)
 * @tparam FANOUT    Number of child slots per node (must be a multiple of 4, max 31)
 * @param aBoundsMin0  Dim-0 lower bounds array [FANOUT]
 * @param aBoundsMax0  Dim-0 upper bounds array [FANOUT]
 * @param aBoundsMin1  Dim-1 lower bounds array [FANOUT]
 * @param aBoundsMax1  Dim-1 upper bounds array [FANOUT]
 * @param aCount       Number of valid children (0..FANOUT)
 * @param aMin         Query rectangle minimum corner [2]
 * @param aMax         Query rectangle maximum corner [2]
 */
template <class ELEMTYPE, int FANOUT>
inline uint32_t OverlapMask2D( const ELEMTYPE* aBoundsMin0, const ELEMTYPE* aBoundsMax0,
                               const ELEMTYPE* aBoundsMin1, const ELEMTYPE* aBoundsMax1,
                               int aCount,
                               const ELEMTYPE aMin[2], const ELEMTYPE aMax[2] )
{
    static_assert( FANOUT <= 31, "OverlapMask2D requires FANOUT <= 31" );
    static_assert( FANOUT % 4 == 0, "OverlapMask2D requires FANOUT divisible by 4 for SIMD" );

#if defined( __SSE2__ )
    if constexpr( sizeof( ELEMTYPE ) == 4 )
    {
        // Broadcast query bounds to all 4 SIMD lanes
        __m128i qmin0 = _mm_set1_epi32( static_cast<int>( aMin[0] ) );
        __m128i qmax0 = _mm_set1_epi32( static_cast<int>( aMax[0] ) );
        __m128i qmin1 = _mm_set1_epi32( static_cast<int>( aMin[1] ) );
        __m128i qmax1 = _mm_set1_epi32( static_cast<int>( aMax[1] ) );

        uint32_t mask = 0;

        // Test 4 children per iteration using 128-bit packed integer comparisons.
        // Each iteration loads one 16-byte chunk from each of the 4 bounds arrays,
        // tests all 4 separation axes, and extracts the result to 4 mask bits via
        // movemask (single instruction to extract the top bit of each 32-bit lane).
        for( int j = 0; j < FANOUT; j += 4 )
        {
            __m128i bmin0 = _mm_loadu_si128(
                    reinterpret_cast<const __m128i*>( &aBoundsMin0[j] ) );
            __m128i bmax0 = _mm_loadu_si128(
                    reinterpret_cast<const __m128i*>( &aBoundsMax0[j] ) );
            __m128i bmin1 = _mm_loadu_si128(
                    reinterpret_cast<const __m128i*>( &aBoundsMin1[j] ) );
            __m128i bmax1 = _mm_loadu_si128(
                    reinterpret_cast<const __m128i*>( &aBoundsMax1[j] ) );

            // Any separation axis proves non-overlap
            __m128i fail = _mm_or_si128(
                    _mm_or_si128( _mm_cmpgt_epi32( bmin0, qmax0 ),
                                  _mm_cmpgt_epi32( qmin0, bmax0 ) ),
                    _mm_or_si128( _mm_cmpgt_epi32( bmin1, qmax1 ),
                                  _mm_cmpgt_epi32( qmin1, bmax1 ) ) );

            // Extract top bit of each 32-bit lane into a 4-bit integer
            int failBits = _mm_movemask_ps( _mm_castsi128_ps( fail ) );
            mask |= static_cast<uint32_t>( ~failBits & 0xF ) << j;
        }

        return mask & ( ( 1U << aCount ) - 1 );
    }
    else
#elif defined( __ARM_NEON ) || defined( __ARM_NEON__ )
    if constexpr( sizeof( ELEMTYPE ) == 4 )
    {
        // Broadcast query bounds to all 4 NEON lanes
        int32x4_t qmin0 = vdupq_n_s32( static_cast<int32_t>( aMin[0] ) );
        int32x4_t qmax0 = vdupq_n_s32( static_cast<int32_t>( aMax[0] ) );
        int32x4_t qmin1 = vdupq_n_s32( static_cast<int32_t>( aMin[1] ) );
        int32x4_t qmax1 = vdupq_n_s32( static_cast<int32_t>( aMax[1] ) );

        // Bit-position shifts for converting per-lane fail bits into a bitmask.
        // Lane 0 stays at bit 0, lane 1 shifts left by 1, etc.
        static const int32_t kBitShifts[4] = { 0, 1, 2, 3 };
        int32x4_t shifts = vld1q_s32( kBitShifts );

        uint32_t mask = 0;

        // Test 4 children per iteration using 128-bit NEON comparisons.
        // NEON lacks movemask, so we extract each lane's 1-bit result via
        // right-shift-31, position it with a variable left shift, and
        // OR-reduce the 4 lanes to a 4-bit chunk.
        for( int j = 0; j < FANOUT; j += 4 )
        {
            int32x4_t bmin0 = vld1q_s32(
                    reinterpret_cast<const int32_t*>( &aBoundsMin0[j] ) );
            int32x4_t bmax0 = vld1q_s32(
                    reinterpret_cast<const int32_t*>( &aBoundsMax0[j] ) );
            int32x4_t bmin1 = vld1q_s32(
                    reinterpret_cast<const int32_t*>( &aBoundsMin1[j] ) );
            int32x4_t bmax1 = vld1q_s32(
                    reinterpret_cast<const int32_t*>( &aBoundsMax1[j] ) );

            // Any separation axis proves non-overlap (vcgtq_s32 yields all-1s on true)
            uint32x4_t fail = vorrq_u32(
                    vorrq_u32( vcgtq_s32( bmin0, qmax0 ),
                               vcgtq_s32( qmin0, bmax0 ) ),
                    vorrq_u32( vcgtq_s32( bmin1, qmax1 ),
                               vcgtq_s32( qmin1, bmax1 ) ) );

            // Extract sign bit (0 or 1) from each lane, shift to bit position
            uint32x4_t bits = vshrq_n_u32( fail, 31 );
            uint32x4_t positioned = vshlq_u32( bits, shifts );

            // OR-reduce 4 lanes into a 4-bit fail mask
            uint32x2_t half = vorr_u32( vget_low_u32( positioned ),
                                        vget_high_u32( positioned ) );
            uint32_t failMask = vget_lane_u32( half, 0 ) | vget_lane_u32( half, 1 );

            mask |= static_cast<uint32_t>( ~failMask & 0xF ) << j;
        }

        return mask & ( ( 1U << aCount ) - 1 );
    }
    else
#endif
    {
        // Scalar fallback with a two-pass structure for auto-vectorization.
        //
        // The first loop performs branchless comparisons across all FANOUT slots,
        // writing boolean results to a flat array. This fixed-trip-count pattern
        // without data-dependent exits allows GCC and Clang to auto-vectorize the
        // comparisons even at baseline SSE2/NEON instruction levels.
        //
        // The second loop packs the booleans into a bitmask. The variable-shift
        // bit-packing is inherently scalar but operates on only FANOUT iterations.
        int result[FANOUT];

        for( int i = 0; i < FANOUT; ++i )
        {
            result[i] = ( aBoundsMin0[i] <= aMax[0] )
                       & ( aBoundsMax0[i] >= aMin[0] )
                       & ( aBoundsMin1[i] <= aMax[1] )
                       & ( aBoundsMax1[i] >= aMin[1] );
        }

        uint32_t mask = 0;

        for( int i = 0; i < FANOUT; ++i )
            mask |= ( static_cast<uint32_t>( result[i] ) << i );

        return mask & ( ( 1U << aCount ) - 1 );
    }
}


/**
 * R-tree node with Structure-of-Arrays bounding box layout.
 *
 * Bounding boxes are stored in SoA format for cache-efficient scanning:
 *   bounds[axis * 2 + 0][slot] = min bound on axis
 *   bounds[axis * 2 + 1][slot] = max bound on axis
 *
 * For 2D with MAXNODES=16, this means 4 arrays of 16 ints each (256 bytes total),
 * fitting in exactly 4 cache lines. This enables compiler auto-vectorization of
 * overlap checks.
 *
 * @tparam DATATYPE  Type of data stored in leaf nodes
 * @tparam ELEMTYPE  Coordinate element type (typically int)
 * @tparam NUMDIMS   Number of dimensions (2 or 3)
 * @tparam MAXNODES  Maximum children per node (fanout), should be 16 for SIMD alignment
 */
template <class DATATYPE, class ELEMTYPE, int NUMDIMS, int MAXNODES>
struct RTREE_NODE
{
    static constexpr int MINNODES = MAXNODES * 2 / 5;  // ~40%, R*-tree convention

    int      count = 0;                           // Number of valid children
    int      level = 0;                           // 0 = leaf, higher = internal

    // SoA bounding boxes: bounds[axis*2+side][slot]
    // side 0 = min, side 1 = max
    ELEMTYPE bounds[NUMDIMS * 2][MAXNODES];

    union
    {
        RTREE_NODE* children[MAXNODES];          // Internal node children
        DATATYPE    data[MAXNODES];              // Leaf node data
    };

    // Stored insertion bboxes for leaf entries, enabling O(log N) removal
    // even when the item has moved since insertion
    ELEMTYPE insertBounds[NUMDIMS * 2][MAXNODES];

    std::atomic<int> refcount;                   // For CoW support

    RTREE_NODE() : refcount( 1 )
    {
        std::memset( bounds, 0, sizeof( bounds ) );
        std::memset( insertBounds, 0, sizeof( insertBounds ) );
        std::memset( &children, 0, sizeof( children ) );
    }

    bool IsLeaf() const { return level == 0; }
    bool IsInternal() const { return level > 0; }
    bool IsFull() const { return count >= MAXNODES; }
    bool IsUnderflow() const { return count < MINNODES; }

    /**
     * Compute the bounding box that encloses all children in this node.
     */
    void ComputeEnclosingBounds( ELEMTYPE aMin[NUMDIMS], ELEMTYPE aMax[NUMDIMS] ) const
    {
        for( int d = 0; d < NUMDIMS; ++d )
        {
            aMin[d] = std::numeric_limits<ELEMTYPE>::max();
            aMax[d] = std::numeric_limits<ELEMTYPE>::lowest();
        }

        for( int i = 0; i < count; ++i )
        {
            for( int d = 0; d < NUMDIMS; ++d )
            {
                if( bounds[d * 2][i] < aMin[d] )
                    aMin[d] = bounds[d * 2][i];

                if( bounds[d * 2 + 1][i] > aMax[d] )
                    aMax[d] = bounds[d * 2 + 1][i];
            }
        }
    }

    /**
     * Compute the area (or volume for 3D) of child slot i's bounding box.
     */
    int64_t ChildArea( int i ) const
    {
        int64_t area = 1;

        for( int d = 0; d < NUMDIMS; ++d )
            area *= static_cast<int64_t>( bounds[d * 2 + 1][i] ) - bounds[d * 2][i];

        return area;
    }

    /**
     * Test whether child slot i's bounding box overlaps with the given query box.
     */
    bool ChildOverlaps( int i, const ELEMTYPE aMin[NUMDIMS], const ELEMTYPE aMax[NUMDIMS] ) const
    {
        for( int d = 0; d < NUMDIMS; ++d )
        {
            if( bounds[d * 2][i] > aMax[d] || bounds[d * 2 + 1][i] < aMin[d] )
                return false;
        }

        return true;
    }

    /**
     * Bitmask of children whose bounding boxes overlap the query rectangle.
     * Bit i is set if child i overlaps. Uses SIMD on x86-64 and ARM for 2D.
     */
    uint32_t ChildOverlapMask( const ELEMTYPE aMin[NUMDIMS],
                               const ELEMTYPE aMax[NUMDIMS] ) const
    {
        if constexpr( NUMDIMS == 2 )
        {
            return OverlapMask2D<ELEMTYPE, MAXNODES>(
                    bounds[0], bounds[1], bounds[2], bounds[3], count, aMin, aMax );
        }
        else
        {
            uint32_t mask = 0;

            for( int i = 0; i < count; ++i )
            {
                if( ChildOverlaps( i, aMin, aMax ) )
                    mask |= ( 1U << i );
            }

            return mask;
        }
    }

    /**
     * Compute the overlap area between child slot i and the given box.
     */
    int64_t ChildOverlapArea( int i, const ELEMTYPE aMin[NUMDIMS],
                              const ELEMTYPE aMax[NUMDIMS] ) const
    {
        int64_t overlap = 1;

        for( int d = 0; d < NUMDIMS; ++d )
        {
            ELEMTYPE lo = std::max( bounds[d * 2][i], aMin[d] );
            ELEMTYPE hi = std::min( bounds[d * 2 + 1][i], aMax[d] );

            if( lo > hi )
                return 0;

            overlap *= static_cast<int64_t>( hi ) - lo;
        }

        return overlap;
    }

    /**
     * Compute how much child slot i's area would increase if it were enlarged to include
     * the given bounding box.
     */
    int64_t ChildEnlargement( int i, const ELEMTYPE aMin[NUMDIMS],
                               const ELEMTYPE aMax[NUMDIMS] ) const
    {
        int64_t originalArea = ChildArea( i );
        int64_t enlargedArea = 1;

        for( int d = 0; d < NUMDIMS; ++d )
        {
            ELEMTYPE lo = std::min( bounds[d * 2][i], aMin[d] );
            ELEMTYPE hi = std::max( bounds[d * 2 + 1][i], aMax[d] );
            enlargedArea *= static_cast<int64_t>( hi ) - lo;
        }

        return enlargedArea - originalArea;
    }

    /**
     * Compute the perimeter (or margin for 3D) of child slot i's bounding box.
     * Used for split axis selection in R*-tree.
     */
    int64_t ChildPerimeter( int i ) const
    {
        int64_t perimeter = 0;

        for( int d = 0; d < NUMDIMS; ++d )
            perimeter += static_cast<int64_t>( bounds[d * 2 + 1][i] ) - bounds[d * 2][i];

        return 2 * perimeter;
    }

    /**
     * Set the bounding box for child slot i.
     */
    void SetChildBounds( int i, const ELEMTYPE aMin[NUMDIMS], const ELEMTYPE aMax[NUMDIMS] )
    {
        for( int d = 0; d < NUMDIMS; ++d )
        {
            bounds[d * 2][i] = aMin[d];
            bounds[d * 2 + 1][i] = aMax[d];
        }
    }

    /**
     * Get the bounding box for child slot i.
     */
    void GetChildBounds( int i, ELEMTYPE aMin[NUMDIMS], ELEMTYPE aMax[NUMDIMS] ) const
    {
        for( int d = 0; d < NUMDIMS; ++d )
        {
            aMin[d] = bounds[d * 2][i];
            aMax[d] = bounds[d * 2 + 1][i];
        }
    }

    /**
     * Store the insertion bounding box for leaf entry i.
     */
    void SetInsertBounds( int i, const ELEMTYPE aMin[NUMDIMS], const ELEMTYPE aMax[NUMDIMS] )
    {
        for( int d = 0; d < NUMDIMS; ++d )
        {
            insertBounds[d * 2][i] = aMin[d];
            insertBounds[d * 2 + 1][i] = aMax[d];
        }
    }

    /**
     * Get the stored insertion bounding box for leaf entry i.
     */
    void GetInsertBounds( int i, ELEMTYPE aMin[NUMDIMS], ELEMTYPE aMax[NUMDIMS] ) const
    {
        for( int d = 0; d < NUMDIMS; ++d )
        {
            aMin[d] = insertBounds[d * 2][i];
            aMax[d] = insertBounds[d * 2 + 1][i];
        }
    }

    /**
     * Remove child at slot i by swapping with last entry.
     */
    void RemoveChild( int i )
    {
        assert( i < count );
        int last = count - 1;

        if( i != last )
        {
            for( int d = 0; d < NUMDIMS * 2; ++d )
            {
                bounds[d][i] = bounds[d][last];
                insertBounds[d][i] = insertBounds[d][last];
            }

            if( IsLeaf() )
                data[i] = data[last];
            else
                children[i] = children[last];
        }

        count--;
    }
};


/**
 * Pool allocator for R-tree nodes.
 *
 * Allocates nodes in fixed-size pages (slabs) to reduce allocation overhead and improve
 * cache locality. Freed nodes are recycled via a free list. All pages are freed on
 * allocator destruction.
 *
 * @tparam NODE the RTREE_NODE type
 */
template <class NODE>
class SLAB_ALLOCATOR
{
public:
    static constexpr size_t NODES_PER_PAGE = 256;

    struct PAGE
    {
        alignas( 64 ) NODE nodes[NODES_PER_PAGE];
        std::bitset<NODES_PER_PAGE> used;

        PAGE() : used() {}
    };

    SLAB_ALLOCATOR() = default;

    ~SLAB_ALLOCATOR() = default;

    // Non-copyable
    SLAB_ALLOCATOR( const SLAB_ALLOCATOR& ) = delete;
    SLAB_ALLOCATOR& operator=( const SLAB_ALLOCATOR& ) = delete;

    // Movable
    SLAB_ALLOCATOR( SLAB_ALLOCATOR&& aOther ) noexcept :
            m_pages( std::move( aOther.m_pages ) ),
            m_freeList( std::move( aOther.m_freeList ) )
    {
    }

    SLAB_ALLOCATOR& operator=( SLAB_ALLOCATOR&& aOther ) noexcept
    {
        if( this != &aOther )
        {
            m_pages = std::move( aOther.m_pages );
            m_freeList = std::move( aOther.m_freeList );
        }

        return *this;
    }

    /**
     * Allocate a new node, either from the free list or from a new page.
     */
    NODE* Allocate()
    {
        if( !m_freeList.empty() )
        {
            NODE* node = m_freeList.back();
            m_freeList.pop_back();

            // Re-mark as used in the owning page's bitset
            markUsed( node );

            new( node ) NODE();
            return node;
        }

        // Find a page with free slots, or create one
        for( auto& page : m_pages )
        {
            if( page->used.count() < NODES_PER_PAGE )
            {
                for( size_t i = 0; i < NODES_PER_PAGE; ++i )
                {
                    if( !page->used[i] )
                    {
                        page->used.set( i );
                        NODE* node = &page->nodes[i];
                        new( node ) NODE();
                        return node;
                    }
                }
            }
        }

        // All pages full, allocate a new one
        m_pages.push_back( std::make_unique<PAGE>() );
        PAGE* page = m_pages.back().get();
        page->used.set( 0 );
        NODE* node = &page->nodes[0];
        new( node ) NODE();
        return node;
    }

    /**
     * Return a node to the free list for reuse.
     */
    void Free( NODE* aNode )
    {
        if( !aNode )
            return;

        // Clear the used bit so the page-scan fallback path stays consistent
        markUnused( aNode );

        aNode->~NODE();
        m_freeList.push_back( aNode );
    }

    /**
     * Return approximate memory usage in bytes.
     */
    size_t MemoryUsage() const
    {
        return m_pages.size() * sizeof( PAGE )
             + m_freeList.capacity() * sizeof( NODE* );
    }

    /**
     * Check if a node was allocated from one of this allocator's pages.
     */
    bool Owns( const NODE* aNode ) const
    {
        size_t ignored = 0;
        return findOwningPage( aNode, ignored ) != nullptr;
    }

private:
    /**
     * Find the page that contains aNode using address-range comparison (no UB).
     * Returns the page and sets aOffset to the slot index, or nullptr if not found.
     */
    PAGE* findOwningPage( const NODE* aNode, size_t& aOffset ) const
    {
        const auto addr = reinterpret_cast<uintptr_t>( aNode );

        for( auto& page : m_pages )
        {
            const auto begin = reinterpret_cast<uintptr_t>( &page->nodes[0] );
            const auto end = reinterpret_cast<uintptr_t>( &page->nodes[NODES_PER_PAGE] );

            if( addr >= begin && addr < end )
            {
                // Safe to subtract now since both pointers are in the same array
                aOffset = static_cast<size_t>( aNode - &page->nodes[0] );
                return page.get();
            }
        }

        return nullptr;
    }

    void markUsed( NODE* aNode )
    {
        size_t offset = 0;
        PAGE*  page = findOwningPage( aNode, offset );

        if( page )
            page->used.set( offset );
    }

    void markUnused( NODE* aNode )
    {
        size_t offset = 0;
        PAGE*  page = findOwningPage( aNode, offset );

        if( page )
            page->used.reset( offset );
    }

    std::vector<std::unique_ptr<PAGE>> m_pages;
    std::vector<NODE*>                 m_freeList;
};

} // namespace KIRTREE

#endif // RTREE_NODE_H
