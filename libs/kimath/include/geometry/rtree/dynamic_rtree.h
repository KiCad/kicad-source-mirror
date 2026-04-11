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

#ifndef DYNAMIC_RTREE_H
#define DYNAMIC_RTREE_H

#include <algorithm>
#include <bit>
#include <cassert>
#include <climits>
#include <cstdint>
#include <cstring>
#include <functional>
#include <limits>
#include <queue>
#include <utility>
#include <vector>

#include <geometry/rtree/rtree_node.h>

namespace KIRTREE
{

/**
 * Dynamic R*-tree with SoA node layout and stored insertion bounding boxes.
 *
 * Supports O(log N) insert, remove, and query operations. Uses R*-tree split
 * and forced reinsert heuristics for optimal tree quality.
 *
 * Key features:
 *   - SoA node layout with configurable fanout for cache efficiency
 *   - Stored insertion bboxes eliminate O(N) fallback removal
 *   - Slab allocator for node memory
 *   - Move semantics (no raw pointer ownership issues)
 *   - Dual API: new ELEMTYPE-array API + raw-array compat
 *
 * @tparam DATATYPE   Type of data stored in leaf nodes
 * @tparam ELEMTYPE   Coordinate element type (typically int)
 * @tparam NUMDIMS    Number of dimensions (2 or 3)
 * @tparam TMAXNODES  Maximum children per node (fanout)
 */
template <class DATATYPE, class ELEMTYPE = int, int NUMDIMS = 2, int TMAXNODES = 16>
class DYNAMIC_RTREE
{
public:
    using NODE = RTREE_NODE<DATATYPE, ELEMTYPE, NUMDIMS, TMAXNODES>;

    static constexpr int MAXNODES = TMAXNODES;
    static constexpr int MINNODES = NODE::MINNODES;

    // Fraction of entries to reinsert on overflow (30% per R*-tree paper)
    static constexpr int REINSERT_COUNT = MAXNODES * 3 / 10;

    DYNAMIC_RTREE() = default;

    ~DYNAMIC_RTREE()
    {
        removeAllNodes( m_root );
        m_root = nullptr;
        m_count = 0;
    }

    // Move semantics
    DYNAMIC_RTREE( DYNAMIC_RTREE&& aOther ) noexcept :
            m_root( aOther.m_root ),
            m_count( aOther.m_count ),
            m_allocator( std::move( aOther.m_allocator ) )
    {
        aOther.m_root = nullptr;
        aOther.m_count = 0;
    }

    DYNAMIC_RTREE& operator=( DYNAMIC_RTREE&& aOther ) noexcept
    {
        if( this != &aOther )
        {
            removeAllNodes( m_root );
            m_root = aOther.m_root;
            m_count = aOther.m_count;
            m_allocator = std::move( aOther.m_allocator );
            aOther.m_root = nullptr;
            aOther.m_count = 0;
        }

        return *this;
    }

    // Non-copyable
    DYNAMIC_RTREE( const DYNAMIC_RTREE& ) = delete;
    DYNAMIC_RTREE& operator=( const DYNAMIC_RTREE& ) = delete;

    /**
     * Insert an item with the given bounding box.
     */
    void Insert( const ELEMTYPE aMin[NUMDIMS], const ELEMTYPE aMax[NUMDIMS],
                 const DATATYPE& aData )
    {
        if( !m_root )
        {
            m_root = allocNode();
            m_root->level = 0;
        }

        // Bitmask tracking which levels have had forced reinsert this insertion.
        // 32 bits handles tree depth up to 31, which covers > 16^31 items.
        uint32_t reinsertedLevels = 0;

        insertImpl( aMin, aMax, aData, reinsertedLevels );
        m_count++;
    }

    /**
     * Remove an item using its stored insertion bounding box.
     *
     * @return true if the item was found and removed, false otherwise
     */
    bool Remove( const ELEMTYPE aMin[NUMDIMS], const ELEMTYPE aMax[NUMDIMS],
                 const DATATYPE& aData )
    {
        if( !m_root )
            return false;

        // Try removal using the provided bbox first
        std::vector<NODE*> reinsertList;

        if( removeImpl( m_root, aMin, aMax, aData, reinsertList ) )
        {
            m_count--;
            reinsertOrphans( reinsertList );
            condenseRoot();
            return true;
        }

        // Fall back to full-tree search using stored insertion bboxes
        ELEMTYPE fullMin[NUMDIMS];
        ELEMTYPE fullMax[NUMDIMS];

        for( int d = 0; d < NUMDIMS; ++d )
        {
            fullMin[d] = std::numeric_limits<ELEMTYPE>::lowest();
            fullMax[d] = std::numeric_limits<ELEMTYPE>::max();
        }

        reinsertList.clear();

        if( removeImpl( m_root, fullMin, fullMax, aData, reinsertList ) )
        {
            m_count--;
            reinsertOrphans( reinsertList );
            condenseRoot();
            return true;
        }

        return false;
    }

    /**
     * Search for items whose bounding boxes overlap the query rectangle.
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
        int found = 0;

        if( m_root )
            searchImpl( m_root, aMin, aMax, aVisitor, found );

        return found;
    }

    /**
     * Remove all items from the tree.
     */
    void RemoveAll()
    {
        removeAllNodes( m_root );
        m_root = nullptr;
        m_count = 0;
    }

    /**
     * Entry type for bulk loading.
     */
    struct BULK_ENTRY
    {
        ELEMTYPE min[NUMDIMS];
        ELEMTYPE max[NUMDIMS];
        DATATYPE data;
    };

    /**
     * Build the tree from a batch of entries using Hilbert-curve packed bulk loading.
     *
     * This is dramatically faster than inserting items individually because it avoids
     * the R*-tree forced-reinsertion cascades. Items are sorted by Hilbert curve index
     * of their bounding box centers, then packed bottom-up into nodes.
     *
     * Any existing tree contents are discarded.
     *
     * @param aEntries  Entries to load. The vector may be reordered.
     */
    void BulkLoad( std::vector<BULK_ENTRY>& aEntries )
    {
        removeAllNodes( m_root );
        m_root = nullptr;
        m_count = 0;

        if( aEntries.empty() )
            return;

        m_count = aEntries.size();

        // Find global bounds for Hilbert normalization
        ELEMTYPE globalMin[NUMDIMS], globalMax[NUMDIMS];

        for( int d = 0; d < NUMDIMS; ++d )
        {
            globalMin[d] = std::numeric_limits<ELEMTYPE>::max();
            globalMax[d] = std::numeric_limits<ELEMTYPE>::lowest();
        }

        for( const auto& entry : aEntries )
        {
            for( int d = 0; d < NUMDIMS; ++d )
            {
                if( entry.min[d] < globalMin[d] )
                    globalMin[d] = entry.min[d];

                if( entry.max[d] > globalMax[d] )
                    globalMax[d] = entry.max[d];
            }
        }

        // Sort entries by Hilbert index of bbox center
        double range[NUMDIMS];

        for( int d = 0; d < NUMDIMS; ++d )
        {
            range[d] = static_cast<double>( globalMax[d] ) - globalMin[d];

            if( range[d] <= 0.0 )
                range[d] = 1.0;
        }

        std::sort( aEntries.begin(), aEntries.end(),
                   [&]( const BULK_ENTRY& a, const BULK_ENTRY& b )
                   {
                       uint32_t coordsA[NUMDIMS], coordsB[NUMDIMS];

                       for( int d = 0; d < NUMDIMS; ++d )
                       {
                           double centerA = ( static_cast<double>( a.min[d] ) + a.max[d] ) / 2.0;
                           double centerB = ( static_cast<double>( b.min[d] ) + b.max[d] ) / 2.0;

                           coordsA[d] = static_cast<uint32_t>(
                                   ( ( centerA - globalMin[d] ) / range[d] )
                                   * static_cast<double>( UINT32_MAX ) );
                           coordsB[d] = static_cast<uint32_t>(
                                   ( ( centerB - globalMin[d] ) / range[d] )
                                   * static_cast<double>( UINT32_MAX ) );
                       }

                       return HilbertND2D<NUMDIMS>( 32, coordsA )
                              < HilbertND2D<NUMDIMS>( 32, coordsB );
                   } );

        // Pack entries into leaf nodes
        std::vector<NODE*> currentLevel;
        size_t n = aEntries.size();
        currentLevel.reserve( ( n + MAXNODES - 1 ) / MAXNODES );

        for( size_t i = 0; i < n; i += MAXNODES )
        {
            NODE* leaf = allocNode();
            leaf->level = 0;
            int cnt = static_cast<int>( std::min<size_t>( MAXNODES, n - i ) );

            for( int j = 0; j < cnt; ++j )
            {
                const auto& entry = aEntries[i + j];
                leaf->SetChildBounds( j, entry.min, entry.max );
                leaf->SetInsertBounds( j, entry.min, entry.max );
                leaf->data[j] = entry.data;
            }

            leaf->count = cnt;
            currentLevel.push_back( leaf );
        }

        // Build internal levels bottom-up
        int level = 1;

        while( currentLevel.size() > 1 )
        {
            size_t levelSize = currentLevel.size();
            std::vector<NODE*> nextLevel;
            nextLevel.reserve( ( levelSize + MAXNODES - 1 ) / MAXNODES );

            for( size_t i = 0; i < levelSize; i += MAXNODES )
            {
                NODE* internal = allocNode();
                internal->level = level;
                int cnt = static_cast<int>( std::min<size_t>( MAXNODES, levelSize - i ) );

                for( int j = 0; j < cnt; ++j )
                {
                    NODE* child = currentLevel[i + j];
                    ELEMTYPE childMin[NUMDIMS], childMax[NUMDIMS];
                    child->ComputeEnclosingBounds( childMin, childMax );
                    internal->SetChildBounds( j, childMin, childMax );
                    internal->children[j] = child;
                }

                internal->count = cnt;
                nextLevel.push_back( internal );
            }

            currentLevel = std::move( nextLevel );
            level++;
        }

        m_root = currentLevel[0];
    }

    size_t size() const { return m_count; }
    bool   empty() const { return m_count == 0; }

    /**
     * Return approximate memory usage in bytes.
     */
    size_t MemoryUsage() const
    {
        return m_allocator.MemoryUsage();
    }

    /**
     * Iterator for traversing all data items in the tree.
     */
    class Iterator
    {
    public:
        using iterator_category = std::forward_iterator_tag;
        using value_type = DATATYPE;
        using difference_type = ptrdiff_t;
        using pointer = const DATATYPE*;
        using reference = const DATATYPE&;

        Iterator() : m_atEnd( true ) {}

        explicit Iterator( NODE* aRoot )
        {
            if( aRoot && aRoot->count > 0 )
            {
                m_stack.push_back( { aRoot, 0 } );
                advance();
            }
            else
            {
                m_atEnd = true;
            }
        }

        const DATATYPE& operator*() const { return m_current; }
        const DATATYPE* operator->() const { return &m_current; }

        Iterator& operator++()
        {
            advance();
            return *this;
        }

        bool operator==( const Iterator& aOther ) const
        {
            return m_atEnd == aOther.m_atEnd;
        }

        bool operator!=( const Iterator& aOther ) const
        {
            return !( *this == aOther );
        }

    private:
        struct STACK_ENTRY
        {
            NODE* node;
            int   childIdx;
        };

        void advance()
        {
            while( !m_stack.empty() )
            {
                STACK_ENTRY& top = m_stack.back();

                if( top.node->IsLeaf() )
                {
                    if( top.childIdx < top.node->count )
                    {
                        m_current = top.node->data[top.childIdx];
                        top.childIdx++;
                        m_atEnd = false;
                        return;
                    }

                    m_stack.pop_back();
                }
                else
                {
                    if( top.childIdx < top.node->count )
                    {
                        NODE* child = top.node->children[top.childIdx];
                        top.childIdx++;
                        m_stack.push_back( { child, 0 } );
                    }
                    else
                    {
                        m_stack.pop_back();
                    }
                }
            }

            m_atEnd = true;
        }

        std::vector<STACK_ENTRY> m_stack;
        DATATYPE                m_current = {};
        bool                    m_atEnd = true;
    };

    Iterator begin() const { return Iterator( m_root ); }
    Iterator end() const { return Iterator(); }

    /**
     * Lazy iterator that traverses only nodes overlapping a query rectangle.
     * Uses a small traversal stack rather than storing all results into a vector.
     */
    class SearchIterator
    {
    public:
        using iterator_category = std::input_iterator_tag;
        using value_type = DATATYPE;
        using difference_type = ptrdiff_t;
        using pointer = const DATATYPE*;
        using reference = const DATATYPE&;

        SearchIterator() : m_atEnd( true ) {}

        SearchIterator( NODE* aRoot, const ELEMTYPE aMin[NUMDIMS],
                        const ELEMTYPE aMax[NUMDIMS] )
        {
            for( int d = 0; d < NUMDIMS; ++d )
            {
                m_min[d] = aMin[d];
                m_max[d] = aMax[d];
            }

            if( aRoot && aRoot->count > 0 )
            {
                m_stack.push_back( { aRoot, 0 } );
                advance();
            }
            else
            {
                m_atEnd = true;
            }
        }

        const DATATYPE& operator*() const { return m_current; }

        SearchIterator& operator++()
        {
            advance();
            return *this;
        }

        bool operator==( const SearchIterator& aOther ) const
        {
            return m_atEnd == aOther.m_atEnd;
        }

        bool operator!=( const SearchIterator& aOther ) const
        {
            return !( *this == aOther );
        }

    private:
        struct STACK_ENTRY
        {
            NODE* node;
            int   childIdx;
        };

        void advance()
        {
            while( !m_stack.empty() )
            {
                STACK_ENTRY& top = m_stack.back();

                if( top.node->IsLeaf() )
                {
                    while( top.childIdx < top.node->count )
                    {
                        if( top.node->ChildOverlaps( top.childIdx, m_min, m_max ) )
                        {
                            m_current = top.node->data[top.childIdx];
                            top.childIdx++;
                            m_atEnd = false;
                            return;
                        }

                        top.childIdx++;
                    }

                    m_stack.pop_back();
                }
                else
                {
                    bool descended = false;

                    while( top.childIdx < top.node->count )
                    {
                        if( top.node->ChildOverlaps( top.childIdx, m_min, m_max ) )
                        {
                            NODE* child = top.node->children[top.childIdx];
                            top.childIdx++;
                            m_stack.push_back( { child, 0 } );
                            descended = true;
                            break;
                        }

                        top.childIdx++;
                    }

                    if( !descended )
                        m_stack.pop_back();
                }
            }

            m_atEnd = true;
        }

        std::vector<STACK_ENTRY> m_stack;
        ELEMTYPE                m_min[NUMDIMS];
        ELEMTYPE                m_max[NUMDIMS];
        DATATYPE                m_current = {};
        bool                    m_atEnd = true;
    };

    /**
     * Lazy range for iterating over items whose bounding boxes overlap a query rectangle.
     * Traversal happens incrementally during iteration.
     */
    class SearchRange
    {
    public:
        SearchRange( NODE* aRoot, const ELEMTYPE aMin[NUMDIMS],
                     const ELEMTYPE aMax[NUMDIMS] ) :
                m_root( aRoot )
        {
            for( int d = 0; d < NUMDIMS; ++d )
            {
                m_min[d] = aMin[d];
                m_max[d] = aMax[d];
            }
        }

        SearchIterator begin() const { return SearchIterator( m_root, m_min, m_max ); }
        SearchIterator end() const { return SearchIterator(); }
        bool empty() const { return begin() == end(); }

    private:
        NODE*    m_root;
        ELEMTYPE m_min[NUMDIMS];
        ELEMTYPE m_max[NUMDIMS];
    };

    /**
     * Return a lazy range of items overlapping the query rectangle.
     * More efficient than Search() when callers may break early or
     * only need to check emptiness.
     */
    SearchRange Overlapping( const ELEMTYPE aMin[NUMDIMS],
                             const ELEMTYPE aMax[NUMDIMS] ) const
    {
        return SearchRange( m_root, aMin, aMax );
    }

    /**
     * Nearest-neighbor search using Hjaltason & Samet's algorithm.
     *
     * @param aPoint    Query point coordinates
     * @param aK        Maximum number of neighbors to return
     * @param aResults  Output vector of (squared_distance, data) pairs, sorted by distance
     */
    void NearestNeighbors( const ELEMTYPE aPoint[NUMDIMS], int aK,
                           std::vector<std::pair<int64_t, DATATYPE>>& aResults ) const
    {
        aResults.clear();

        if( !m_root || aK <= 0 )
            return;

        using QueueEntry = std::pair<int64_t, std::pair<NODE*, int>>;
        // Min-heap: smallest distance first
        auto cmp = []( const QueueEntry& a, const QueueEntry& b )
        {
            return a.first > b.first;
        };
        std::priority_queue<QueueEntry, std::vector<QueueEntry>, decltype( cmp )> pq( cmp );

        // Seed with root's children
        for( int i = 0; i < m_root->count; ++i )
        {
            int64_t dist = minDistSq( m_root, i, aPoint );
            pq.push( { dist, { m_root, i } } );
        }

        while( !pq.empty() && static_cast<int>( aResults.size() ) < aK )
        {
            auto [dist, entry] = pq.top();
            pq.pop();
            NODE* node = entry.first;
            int   slot = entry.second;

            if( node->IsLeaf() )
            {
                aResults.push_back( { dist, node->data[slot] } );
            }
            else
            {
                NODE* child = node->children[slot];

                for( int i = 0; i < child->count; ++i )
                {
                    int64_t childDist = minDistSq( child, i, aPoint );
                    pq.push( { childDist, { child, i } } );
                }
            }
        }
    }

private:
    // Entry type used by both leaf and internal node split algorithms
    struct SPLIT_ENTRY
    {
        ELEMTYPE min[NUMDIMS];
        ELEMTYPE max[NUMDIMS];
        ELEMTYPE insertMin[NUMDIMS];
        ELEMTYPE insertMax[NUMDIMS];
        DATATYPE data;
        NODE*    child;
    };

    NODE* allocNode()
    {
        return m_allocator.Allocate();
    }

    void freeNode( NODE* aNode )
    {
        m_allocator.Free( aNode );
    }

    void removeAllNodes( NODE* aNode )
    {
        if( !aNode )
            return;

        if( aNode->IsInternal() )
        {
            for( int i = 0; i < aNode->count; ++i )
                removeAllNodes( aNode->children[i] );
        }

        freeNode( aNode );
    }

    /**
     * Core insertion with forced reinsert tracking.
     */
    void insertImpl( const ELEMTYPE aMin[NUMDIMS], const ELEMTYPE aMax[NUMDIMS],
                     const DATATYPE& aData, uint32_t& aReinsertedLevels )
    {
        // ChooseSubtree to find the leaf
        std::vector<NODE*> path;
        NODE* leaf = chooseSubtree( m_root, aMin, aMax, path );

        // Insert into leaf
        if( !leaf->IsFull() )
        {
            int slot = leaf->count;
            leaf->SetChildBounds( slot, aMin, aMax );
            leaf->SetInsertBounds( slot, aMin, aMax );
            leaf->data[slot] = aData;
            leaf->count++;

            adjustPath( path, leaf );
        }
        else
        {
            overflowTreatment( leaf, aMin, aMax, aData, path, aReinsertedLevels );
        }
    }

    /**
     * R*-tree ChooseSubtree.
     *
     * At leaf level (level==1): minimize overlap increase.
     * At higher levels: minimize area increase, tie-break by smallest area.
     */
    NODE* chooseSubtree( NODE* aNode, const ELEMTYPE aMin[NUMDIMS],
                         const ELEMTYPE aMax[NUMDIMS], std::vector<NODE*>& aPath )
    {
        aPath.clear();
        NODE* node = aNode;

        while( node->IsInternal() )
        {
            aPath.push_back( node );

            if( node->level == 1 )
            {
                // At the level just above leaves: minimize overlap increase
                int    bestIdx = 0;
                int64_t bestOverlapInc = std::numeric_limits<int64_t>::max();
                int64_t bestAreaInc = std::numeric_limits<int64_t>::max();
                int64_t bestArea = std::numeric_limits<int64_t>::max();

                for( int i = 0; i < node->count; ++i )
                {
                    int64_t overlapBefore = computeOverlap( node, i );
                    int64_t overlapAfter = computeOverlapEnlarged( node, i, aMin, aMax );
                    int64_t overlapInc = overlapAfter - overlapBefore;
                    int64_t areaInc = node->ChildEnlargement( i, aMin, aMax );
                    int64_t area = node->ChildArea( i );

                    if( overlapInc < bestOverlapInc
                        || ( overlapInc == bestOverlapInc && areaInc < bestAreaInc )
                        || ( overlapInc == bestOverlapInc && areaInc == bestAreaInc
                             && area < bestArea ) )
                    {
                        bestIdx = i;
                        bestOverlapInc = overlapInc;
                        bestAreaInc = areaInc;
                        bestArea = area;
                    }
                }

                node = node->children[bestIdx];
            }
            else
            {
                // Higher levels: minimize area increase, tie-break by smallest area
                int    bestIdx = 0;
                int64_t bestAreaInc = std::numeric_limits<int64_t>::max();
                int64_t bestArea = std::numeric_limits<int64_t>::max();

                for( int i = 0; i < node->count; ++i )
                {
                    int64_t areaInc = node->ChildEnlargement( i, aMin, aMax );
                    int64_t area = node->ChildArea( i );

                    if( areaInc < bestAreaInc
                        || ( areaInc == bestAreaInc && area < bestArea ) )
                    {
                        bestIdx = i;
                        bestAreaInc = areaInc;
                        bestArea = area;
                    }
                }

                node = node->children[bestIdx];
            }
        }

        return node;
    }

    /**
     * Handle overflow: forced reinsert or split.
     */
    void overflowTreatment( NODE* aNode, const ELEMTYPE aMin[NUMDIMS],
                            const ELEMTYPE aMax[NUMDIMS], const DATATYPE& aData,
                            std::vector<NODE*>& aPath, uint32_t& aReinsertedLevels )
    {
        int level = aNode->level;

        // Guard against UB from shifting by >= 32. At fanout 16 this would
        // require > 16^32 items, but protect the generic template regardless.
        if( level >= 32 )
        {
            splitNode( aNode, aMin, aMax, aData, aPath, aReinsertedLevels );
            return;
        }

        const uint32_t levelMask = 1U << level;

        if( !( aReinsertedLevels & levelMask ) )
        {
            aReinsertedLevels |= levelMask;
            forcedReinsert( aNode, aMin, aMax, aData, aPath, aReinsertedLevels );
        }
        else
        {
            splitNode( aNode, aMin, aMax, aData, aPath, aReinsertedLevels );
        }
    }

    /**
     * R*-tree forced reinsert.
     *
     * Temporarily adds the new entry, then removes the REINSERT_COUNT entries farthest
     * from the node center, and reinserts them.
     */
    void forcedReinsert( NODE* aNode, const ELEMTYPE aMin[NUMDIMS],
                         const ELEMTYPE aMax[NUMDIMS], const DATATYPE& aData,
                         std::vector<NODE*>& aPath, uint32_t& aReinsertedLevels )
    {
        // Collect all entries including the new one
        struct ENTRY
        {
            ELEMTYPE min[NUMDIMS];
            ELEMTYPE max[NUMDIMS];
            ELEMTYPE insertMin[NUMDIMS];
            ELEMTYPE insertMax[NUMDIMS];
            DATATYPE data;
            NODE*    child;  // Only for internal nodes
            int64_t  distSq;
        };

        int totalEntries = aNode->count + 1;
        std::vector<ENTRY> entries( totalEntries );

        // Compute node center
        ELEMTYPE nodeMin[NUMDIMS];
        ELEMTYPE nodeMax[NUMDIMS];
        aNode->ComputeEnclosingBounds( nodeMin, nodeMax );

        double center[NUMDIMS];

        for( int d = 0; d < NUMDIMS; ++d )
            center[d] = ( static_cast<double>( nodeMin[d] ) + nodeMax[d] ) / 2.0;

        // Gather existing entries
        for( int i = 0; i < aNode->count; ++i )
        {
            aNode->GetChildBounds( i, entries[i].min, entries[i].max );

            if( aNode->IsLeaf() )
            {
                aNode->GetInsertBounds( i, entries[i].insertMin, entries[i].insertMax );
                entries[i].data = aNode->data[i];
                entries[i].child = nullptr;
            }
            else
            {
                entries[i].child = aNode->children[i];
            }

            // Distance from entry center to node center
            int64_t distSq = 0;

            for( int d = 0; d < NUMDIMS; ++d )
            {
                double entryCenter = ( static_cast<double>( entries[i].min[d] )
                                       + entries[i].max[d] ) / 2.0;
                double diff = entryCenter - center[d];
                distSq += static_cast<int64_t>( diff * diff );
            }

            entries[i].distSq = distSq;
        }

        // Add the new entry
        ENTRY& newEntry = entries[aNode->count];

        for( int d = 0; d < NUMDIMS; ++d )
        {
            newEntry.min[d] = aMin[d];
            newEntry.max[d] = aMax[d];
            newEntry.insertMin[d] = aMin[d];
            newEntry.insertMax[d] = aMax[d];
        }

        newEntry.data = aData;
        newEntry.child = nullptr;

        int64_t distSq = 0;

        for( int d = 0; d < NUMDIMS; ++d )
        {
            double entryCenter = ( static_cast<double>( aMin[d] ) + aMax[d] ) / 2.0;
            double diff = entryCenter - center[d];
            distSq += static_cast<int64_t>( diff * diff );
        }

        newEntry.distSq = distSq;

        // Sort by distance descending to find the farthest
        std::sort( entries.begin(), entries.end(),
                   []( const ENTRY& a, const ENTRY& b )
                   {
                       return a.distSq > b.distSq;
                   } );

        // Keep close entries in the node, reinsert far ones
        int reinsertCount = std::min( REINSERT_COUNT, totalEntries - MINNODES );

        if( reinsertCount <= 0 )
        {
            // Can't reinsert without underflow, fall back to split
            splitNode( aNode, aMin, aMax, aData, aPath, aReinsertedLevels );
            return;
        }

        // Rebuild the node with the close entries
        aNode->count = 0;

        for( int i = reinsertCount; i < totalEntries; ++i )
        {
            int slot = aNode->count;
            aNode->SetChildBounds( slot, entries[i].min, entries[i].max );

            if( aNode->IsLeaf() )
            {
                aNode->SetInsertBounds( slot, entries[i].insertMin, entries[i].insertMax );
                aNode->data[slot] = entries[i].data;
            }
            else
            {
                aNode->children[slot] = entries[i].child;
            }

            aNode->count++;
        }

        adjustPath( aPath, aNode );

        // Reinsert the far entries
        for( int i = 0; i < reinsertCount; ++i )
        {
            if( aNode->IsLeaf() )
            {
                insertImpl( entries[i].insertMin, entries[i].insertMax,
                            entries[i].data, aReinsertedLevels );
            }
            else
            {
                reinsertNode( entries[i].child, entries[i].min, entries[i].max,
                              aNode->level - 1, aReinsertedLevels );
            }
        }
    }

    /**
     * Reinsert an internal node's child at its correct level.
     */
    void reinsertNode( NODE* aChild, const ELEMTYPE aMin[NUMDIMS],
                       const ELEMTYPE aMax[NUMDIMS], int aLevel,
                       uint32_t& aReinsertedLevels )
    {
        // Find a node at the correct level
        std::vector<NODE*> path;
        NODE* target = chooseSubtreeAtLevel( m_root, aMin, aMax, aLevel + 1, path );

        if( !target->IsFull() )
        {
            int slot = target->count;
            target->SetChildBounds( slot, aMin, aMax );
            target->children[slot] = aChild;
            target->count++;
            adjustPath( path, target );
        }
        else
        {
            splitNodeInternal( target, aMin, aMax, aChild, path, aReinsertedLevels );
        }
    }

    /**
     * ChooseSubtree targeting a specific level.
     */
    NODE* chooseSubtreeAtLevel( NODE* aNode, const ELEMTYPE aMin[NUMDIMS],
                                const ELEMTYPE aMax[NUMDIMS], int aTargetLevel,
                                std::vector<NODE*>& aPath )
    {
        aPath.clear();
        NODE* node = aNode;

        while( node->level > aTargetLevel )
        {
            aPath.push_back( node );

            int    bestIdx = 0;
            int64_t bestAreaInc = std::numeric_limits<int64_t>::max();
            int64_t bestArea = std::numeric_limits<int64_t>::max();

            for( int i = 0; i < node->count; ++i )
            {
                int64_t areaInc = node->ChildEnlargement( i, aMin, aMax );
                int64_t area = node->ChildArea( i );

                if( areaInc < bestAreaInc
                    || ( areaInc == bestAreaInc && area < bestArea ) )
                {
                    bestIdx = i;
                    bestAreaInc = areaInc;
                    bestArea = area;
                }
            }

            node = node->children[bestIdx];
        }

        return node;
    }

    /**
     * R*-tree split.
     *
     * ChooseSplitAxis: for each axis, sort by min then max. Choose axis minimizing
     * sum of perimeters across all valid distributions.
     *
     * ChooseSplitIndex: along chosen axis, choose distribution minimizing overlap.
     */
    void splitNode( NODE* aNode, const ELEMTYPE aMin[NUMDIMS],
                    const ELEMTYPE aMax[NUMDIMS], const DATATYPE& aData,
                    std::vector<NODE*>& aPath, uint32_t& aReinsertedLevels )
    {
        // Collect all entries including the overflow entry
        int totalEntries = aNode->count + 1;
        std::vector<SPLIT_ENTRY> entries( totalEntries );

        for( int i = 0; i < aNode->count; ++i )
        {
            aNode->GetChildBounds( i, entries[i].min, entries[i].max );

            if( aNode->IsLeaf() )
            {
                aNode->GetInsertBounds( i, entries[i].insertMin, entries[i].insertMax );
                entries[i].data = aNode->data[i];
                entries[i].child = nullptr;
            }
            else
            {
                entries[i].child = aNode->children[i];
            }
        }

        // The overflow entry
        for( int d = 0; d < NUMDIMS; ++d )
        {
            entries[aNode->count].min[d] = aMin[d];
            entries[aNode->count].max[d] = aMax[d];
            entries[aNode->count].insertMin[d] = aMin[d];
            entries[aNode->count].insertMax[d] = aMax[d];
        }

        entries[aNode->count].data = aData;
        entries[aNode->count].child = nullptr;

        // ChooseSplitAxis: minimize sum of perimeters
        int    bestAxis = 0;
        int64_t bestPerimeterSum = std::numeric_limits<int64_t>::max();

        for( int axis = 0; axis < NUMDIMS; ++axis )
        {
            int64_t perimeterSum = 0;

            // Sort by min bound on this axis
            std::sort( entries.begin(), entries.end(),
                       [axis]( const SPLIT_ENTRY& a, const SPLIT_ENTRY& b )
                       {
                           return a.min[axis] < b.min[axis]
                                  || ( a.min[axis] == b.min[axis]
                                       && a.max[axis] < b.max[axis] );
                       } );

            perimeterSum += computeSplitPerimeters( entries, totalEntries );

            // Sort by max bound on this axis
            std::sort( entries.begin(), entries.end(),
                       [axis]( const SPLIT_ENTRY& a, const SPLIT_ENTRY& b )
                       {
                           return a.max[axis] < b.max[axis]
                                  || ( a.max[axis] == b.max[axis]
                                       && a.min[axis] < b.min[axis] );
                       } );

            perimeterSum += computeSplitPerimeters( entries, totalEntries );

            if( perimeterSum < bestPerimeterSum )
            {
                bestPerimeterSum = perimeterSum;
                bestAxis = axis;
            }
        }

        // ChooseSplitIndex along bestAxis: minimize overlap
        // Re-sort by min on best axis
        std::sort( entries.begin(), entries.end(),
                   [bestAxis]( const SPLIT_ENTRY& a, const SPLIT_ENTRY& b )
                   {
                       return a.min[bestAxis] < b.min[bestAxis]
                              || ( a.min[bestAxis] == b.min[bestAxis]
                                   && a.max[bestAxis] < b.max[bestAxis] );
                   } );

        int bestSplit = findBestSplitIndex( entries, totalEntries );

        // Also try sorting by max
        std::vector<SPLIT_ENTRY> entriesByMax = entries;

        std::sort( entriesByMax.begin(), entriesByMax.end(),
                   [bestAxis]( const SPLIT_ENTRY& a, const SPLIT_ENTRY& b )
                   {
                       return a.max[bestAxis] < b.max[bestAxis]
                              || ( a.max[bestAxis] == b.max[bestAxis]
                                   && a.min[bestAxis] < b.min[bestAxis] );
                   } );

        int bestSplitMax = findBestSplitIndex( entriesByMax, totalEntries );
        int64_t overlapMin = computeSplitOverlap( entries, bestSplit, totalEntries );
        int64_t overlapMax = computeSplitOverlap( entriesByMax, bestSplitMax, totalEntries );

        if( overlapMax < overlapMin )
        {
            entries = std::move( entriesByMax );
            bestSplit = bestSplitMax;
        }

        // Create new sibling node
        NODE* sibling = allocNode();
        sibling->level = aNode->level;

        // Distribute entries
        aNode->count = 0;

        for( int i = 0; i < bestSplit; ++i )
        {
            int slot = aNode->count;
            aNode->SetChildBounds( slot, entries[i].min, entries[i].max );

            if( aNode->IsLeaf() )
            {
                aNode->SetInsertBounds( slot, entries[i].insertMin, entries[i].insertMax );
                aNode->data[slot] = entries[i].data;
            }
            else
            {
                aNode->children[slot] = entries[i].child;
            }

            aNode->count++;
        }

        for( int i = bestSplit; i < totalEntries; ++i )
        {
            int slot = sibling->count;
            sibling->SetChildBounds( slot, entries[i].min, entries[i].max );

            if( aNode->IsLeaf() )
            {
                sibling->SetInsertBounds( slot, entries[i].insertMin,
                                          entries[i].insertMax );
                sibling->data[slot] = entries[i].data;
            }
            else
            {
                sibling->children[slot] = entries[i].child;
            }

            sibling->count++;
        }

        // Propagate the split upward
        ELEMTYPE sibMin[NUMDIMS], sibMax[NUMDIMS];
        sibling->ComputeEnclosingBounds( sibMin, sibMax );

        if( aPath.empty() )
        {
            // Splitting the root: create new root
            NODE* newRoot = allocNode();
            newRoot->level = m_root->level + 1;

            ELEMTYPE nodeMin[NUMDIMS], nodeMax[NUMDIMS];
            aNode->ComputeEnclosingBounds( nodeMin, nodeMax );

            newRoot->SetChildBounds( 0, nodeMin, nodeMax );
            newRoot->children[0] = aNode;
            newRoot->SetChildBounds( 1, sibMin, sibMax );
            newRoot->children[1] = sibling;
            newRoot->count = 2;
            m_root = newRoot;
        }
        else
        {
            NODE* parent = aPath.back();

            // Update the existing child's bbox in parent
            int childSlot = findChildSlot( parent, aNode );

            if( childSlot >= 0 )
            {
                ELEMTYPE nodeMin[NUMDIMS], nodeMax[NUMDIMS];
                aNode->ComputeEnclosingBounds( nodeMin, nodeMax );
                parent->SetChildBounds( childSlot, nodeMin, nodeMax );
            }

            // Insert sibling into parent
            if( !parent->IsFull() )
            {
                int slot = parent->count;
                parent->SetChildBounds( slot, sibMin, sibMax );
                parent->children[slot] = sibling;
                parent->count++;

                aPath.pop_back();
                adjustPath( aPath, parent );
            }
            else
            {
                aPath.pop_back();
                splitNodeInternal( parent, sibMin, sibMax, sibling, aPath, aReinsertedLevels );
            }
        }
    }

    /**
     * Split an internal node to insert a new child.
     */
    void splitNodeInternal( NODE* aNode, const ELEMTYPE aMin[NUMDIMS],
                            const ELEMTYPE aMax[NUMDIMS], NODE* aChild,
                            std::vector<NODE*>& aPath,
                            uint32_t& aReinsertedLevels )
    {
        int totalEntries = aNode->count + 1;
        std::vector<SPLIT_ENTRY> entries( totalEntries );

        for( int i = 0; i < aNode->count; ++i )
        {
            aNode->GetChildBounds( i, entries[i].min, entries[i].max );
            entries[i].child = aNode->children[i];
        }

        for( int d = 0; d < NUMDIMS; ++d )
        {
            entries[aNode->count].min[d] = aMin[d];
            entries[aNode->count].max[d] = aMax[d];
        }

        entries[aNode->count].child = aChild;

        // Choose best axis
        int    bestAxis = 0;
        int64_t bestPerimeterSum = std::numeric_limits<int64_t>::max();

        for( int axis = 0; axis < NUMDIMS; ++axis )
        {
            std::sort( entries.begin(), entries.end(),
                       [axis]( const SPLIT_ENTRY& a, const SPLIT_ENTRY& b )
                       {
                           return a.min[axis] < b.min[axis];
                       } );

            int64_t perimSum = computeSplitPerimeters( entries, totalEntries );

            if( perimSum < bestPerimeterSum )
            {
                bestPerimeterSum = perimSum;
                bestAxis = axis;
            }
        }

        // Re-sort on best axis, find best split
        std::sort( entries.begin(), entries.end(),
                   [bestAxis]( const SPLIT_ENTRY& a, const SPLIT_ENTRY& b )
                   {
                       return a.min[bestAxis] < b.min[bestAxis];
                   } );

        int bestSplit = findBestSplitIndex( entries, totalEntries );

        // Create sibling
        NODE* sibling = allocNode();
        sibling->level = aNode->level;

        aNode->count = 0;

        for( int i = 0; i < bestSplit; ++i )
        {
            int slot = aNode->count;
            aNode->SetChildBounds( slot, entries[i].min, entries[i].max );
            aNode->children[slot] = entries[i].child;
            aNode->count++;
        }

        for( int i = bestSplit; i < totalEntries; ++i )
        {
            int slot = sibling->count;
            sibling->SetChildBounds( slot, entries[i].min, entries[i].max );
            sibling->children[slot] = entries[i].child;
            sibling->count++;
        }

        ELEMTYPE sibMin[NUMDIMS], sibMax[NUMDIMS];
        sibling->ComputeEnclosingBounds( sibMin, sibMax );

        if( aPath.empty() )
        {
            NODE* newRoot = allocNode();
            newRoot->level = m_root->level + 1;

            ELEMTYPE nodeMin[NUMDIMS], nodeMax[NUMDIMS];
            aNode->ComputeEnclosingBounds( nodeMin, nodeMax );

            newRoot->SetChildBounds( 0, nodeMin, nodeMax );
            newRoot->children[0] = aNode;
            newRoot->SetChildBounds( 1, sibMin, sibMax );
            newRoot->children[1] = sibling;
            newRoot->count = 2;
            m_root = newRoot;
        }
        else
        {
            NODE* parent = aPath.back();
            int childSlot = findChildSlot( parent, aNode );

            if( childSlot >= 0 )
            {
                ELEMTYPE nodeMin[NUMDIMS], nodeMax[NUMDIMS];
                aNode->ComputeEnclosingBounds( nodeMin, nodeMax );
                parent->SetChildBounds( childSlot, nodeMin, nodeMax );
            }

            if( !parent->IsFull() )
            {
                int slot = parent->count;
                parent->SetChildBounds( slot, sibMin, sibMax );
                parent->children[slot] = sibling;
                parent->count++;
                aPath.pop_back();
                adjustPath( aPath, parent );
            }
            else
            {
                aPath.pop_back();
                splitNodeInternal( parent, sibMin, sibMax, sibling, aPath,
                                   aReinsertedLevels );
            }
        }
    }

    /**
     * Compute sum of perimeters for all valid split distributions.
     */
    template <class ENTRY_VEC>
    int64_t computeSplitPerimeters( const ENTRY_VEC& aEntries, int aTotalEntries ) const
    {
        int64_t sum = 0;

        for( int k = MINNODES; k <= aTotalEntries - MINNODES; ++k )
        {
            // Group 1: entries [0, k)
            // Group 2: entries [k, totalEntries)
            for( int grp = 0; grp < 2; ++grp )
            {
                int start = ( grp == 0 ) ? 0 : k;
                int end = ( grp == 0 ) ? k : aTotalEntries;

                int64_t perimeter = 0;

                for( int d = 0; d < NUMDIMS; ++d )
                {
                    ELEMTYPE mn = std::numeric_limits<ELEMTYPE>::max();
                    ELEMTYPE mx = std::numeric_limits<ELEMTYPE>::lowest();

                    for( int i = start; i < end; ++i )
                    {
                        if( aEntries[i].min[d] < mn )
                            mn = aEntries[i].min[d];

                        if( aEntries[i].max[d] > mx )
                            mx = aEntries[i].max[d];
                    }

                    perimeter += static_cast<int64_t>( mx ) - mn;
                }

                sum += 2 * perimeter;
            }
        }

        return sum;
    }

    /**
     * Find the split index that minimizes overlap between the two groups.
     */
    template <class ENTRY_VEC>
    int findBestSplitIndex( const ENTRY_VEC& aEntries, int aTotalEntries ) const
    {
        int    bestSplit = MINNODES;
        int64_t bestOverlap = std::numeric_limits<int64_t>::max();
        int64_t bestAreaSum = std::numeric_limits<int64_t>::max();

        for( int k = MINNODES; k <= aTotalEntries - MINNODES; ++k )
        {
            int64_t overlap = computeSplitOverlap( aEntries, k, aTotalEntries );

            // Compute area sum for tie-breaking
            int64_t areaSum = 0;

            for( int grp = 0; grp < 2; ++grp )
            {
                int start = ( grp == 0 ) ? 0 : k;
                int end = ( grp == 0 ) ? k : aTotalEntries;
                int64_t area = 1;

                for( int d = 0; d < NUMDIMS; ++d )
                {
                    ELEMTYPE mn = std::numeric_limits<ELEMTYPE>::max();
                    ELEMTYPE mx = std::numeric_limits<ELEMTYPE>::lowest();

                    for( int i = start; i < end; ++i )
                    {
                        if( aEntries[i].min[d] < mn )
                            mn = aEntries[i].min[d];

                        if( aEntries[i].max[d] > mx )
                            mx = aEntries[i].max[d];
                    }

                    area *= static_cast<int64_t>( mx ) - mn;
                }

                areaSum += area;
            }

            if( overlap < bestOverlap
                || ( overlap == bestOverlap && areaSum < bestAreaSum ) )
            {
                bestSplit = k;
                bestOverlap = overlap;
                bestAreaSum = areaSum;
            }
        }

        return bestSplit;
    }

    /**
     * Compute the overlap area between two split groups.
     */
    template <class ENTRY_VEC>
    int64_t computeSplitOverlap( const ENTRY_VEC& aEntries, int aSplitIdx,
                                 int aTotalEntries ) const
    {
        ELEMTYPE g1Min[NUMDIMS], g1Max[NUMDIMS];
        ELEMTYPE g2Min[NUMDIMS], g2Max[NUMDIMS];

        for( int d = 0; d < NUMDIMS; ++d )
        {
            g1Min[d] = g2Min[d] = std::numeric_limits<ELEMTYPE>::max();
            g1Max[d] = g2Max[d] = std::numeric_limits<ELEMTYPE>::lowest();
        }

        for( int i = 0; i < aSplitIdx; ++i )
        {
            for( int d = 0; d < NUMDIMS; ++d )
            {
                if( aEntries[i].min[d] < g1Min[d] )
                    g1Min[d] = aEntries[i].min[d];

                if( aEntries[i].max[d] > g1Max[d] )
                    g1Max[d] = aEntries[i].max[d];
            }
        }

        for( int i = aSplitIdx; i < aTotalEntries; ++i )
        {
            for( int d = 0; d < NUMDIMS; ++d )
            {
                if( aEntries[i].min[d] < g2Min[d] )
                    g2Min[d] = aEntries[i].min[d];

                if( aEntries[i].max[d] > g2Max[d] )
                    g2Max[d] = aEntries[i].max[d];
            }
        }

        // Compute overlap volume
        int64_t overlap = 1;

        for( int d = 0; d < NUMDIMS; ++d )
        {
            ELEMTYPE lo = std::max( g1Min[d], g2Min[d] );
            ELEMTYPE hi = std::min( g1Max[d], g2Max[d] );

            if( lo >= hi )
                return 0;

            overlap *= static_cast<int64_t>( hi ) - lo;
        }

        return overlap;
    }

    /**
     * Compute total overlap of child i with all other children in the node.
     */
    int64_t computeOverlap( NODE* aNode, int aIdx ) const
    {
        int64_t total = 0;
        ELEMTYPE iMin[NUMDIMS], iMax[NUMDIMS];
        aNode->GetChildBounds( aIdx, iMin, iMax );

        for( int j = 0; j < aNode->count; ++j )
        {
            if( j == aIdx )
                continue;

            total += aNode->ChildOverlapArea( j, iMin, iMax );
        }

        return total;
    }

    /**
     * Compute total overlap of child i (enlarged to include query box) with other children.
     */
    int64_t computeOverlapEnlarged( NODE* aNode, int aIdx, const ELEMTYPE aMin[NUMDIMS],
                                     const ELEMTYPE aMax[NUMDIMS] ) const
    {
        ELEMTYPE enlargedMin[NUMDIMS], enlargedMax[NUMDIMS];
        aNode->GetChildBounds( aIdx, enlargedMin, enlargedMax );

        for( int d = 0; d < NUMDIMS; ++d )
        {
            if( aMin[d] < enlargedMin[d] )
                enlargedMin[d] = aMin[d];

            if( aMax[d] > enlargedMax[d] )
                enlargedMax[d] = aMax[d];
        }

        int64_t total = 0;

        for( int j = 0; j < aNode->count; ++j )
        {
            if( j == aIdx )
                continue;

            total += aNode->ChildOverlapArea( j, enlargedMin, enlargedMax );
        }

        return total;
    }

    /**
     * Adjust bounding boxes for all nodes in the path (root to leaf).
     *
     * @param aPath        Path from root to the parent of the modified node
     * @param aBottomChild The modified node (leaf or internal) whose parent's bbox
     *                     needs updating. If nullptr, only updates between path entries.
     */
    void adjustPath( const std::vector<NODE*>& aPath, NODE* aBottomChild = nullptr )
    {
        NODE* childToUpdate = aBottomChild;

        for( int i = static_cast<int>( aPath.size() ) - 1; i >= 0; --i )
        {
            NODE* parent = aPath[i];

            if( childToUpdate )
            {
                int slot = findChildSlot( parent, childToUpdate );

                if( slot >= 0 )
                {
                    ELEMTYPE childMin[NUMDIMS], childMax[NUMDIMS];
                    childToUpdate->ComputeEnclosingBounds( childMin, childMax );
                    parent->SetChildBounds( slot, childMin, childMax );
                }
            }

            childToUpdate = parent;
        }
    }

    int findChildSlot( NODE* aParent, NODE* aChild ) const
    {
        for( int i = 0; i < aParent->count; ++i )
        {
            if( aParent->children[i] == aChild )
                return i;
        }

        return -1;
    }

    /**
     * Remove an item from the tree, collecting underflowing nodes for reinsertion.
     */
    bool removeImpl( NODE* aNode, const ELEMTYPE aMin[NUMDIMS],
                     const ELEMTYPE aMax[NUMDIMS], const DATATYPE& aData,
                     std::vector<NODE*>& aReinsertList )
    {
        if( aNode->IsLeaf() )
        {
            for( int i = 0; i < aNode->count; ++i )
            {
                if( aNode->data[i] == aData
                    && aNode->ChildOverlaps( i, aMin, aMax ) )
                {
                    aNode->RemoveChild( i );
                    return true;
                }
            }

            return false;
        }

        // Internal node: recurse into children whose bbox overlaps the query
        for( int i = 0; i < aNode->count; ++i )
        {
            if( !aNode->ChildOverlaps( i, aMin, aMax ) )
                continue;

            NODE* child = aNode->children[i];

            if( removeImpl( child, aMin, aMax, aData, aReinsertList ) )
            {
                // Update child's bbox in parent
                if( child->count > 0 )
                {
                    ELEMTYPE childMin[NUMDIMS], childMax[NUMDIMS];
                    child->ComputeEnclosingBounds( childMin, childMax );
                    aNode->SetChildBounds( i, childMin, childMax );

                    // Check for underflow
                    if( child->count < MINNODES && aNode != m_root )
                    {
                        aReinsertList.push_back( child );
                        aNode->RemoveChild( i );
                    }
                }
                else
                {
                    freeNode( child );
                    aNode->RemoveChild( i );
                }

                return true;
            }
        }

        return false;
    }

    /**
     * Reinsert all entries from orphaned underflowing nodes.
     */
    void reinsertOrphans( std::vector<NODE*>& aReinsertList )
    {
        for( NODE* orphan : aReinsertList )
        {
            if( orphan->IsLeaf() )
            {
                for( int i = 0; i < orphan->count; ++i )
                {
                    ELEMTYPE mn[NUMDIMS], mx[NUMDIMS];
                    orphan->GetInsertBounds( i, mn, mx );

                    uint32_t reinsertedLevels = 0;
                    insertImpl( mn, mx, orphan->data[i], reinsertedLevels );
                }
            }
            else
            {
                for( int i = 0; i < orphan->count; ++i )
                {
                    ELEMTYPE mn[NUMDIMS], mx[NUMDIMS];
                    orphan->GetChildBounds( i, mn, mx );

                    uint32_t reinsertedLevels = 0;
                    reinsertNode( orphan->children[i], mn, mx, orphan->level - 1,
                                  reinsertedLevels );
                }
            }

            freeNode( orphan );
        }
    }

    /**
     * If the root has only one child, replace it with that child.
     */
    void condenseRoot()
    {
        while( m_root && m_root->IsInternal() && m_root->count == 1 )
        {
            NODE* oldRoot = m_root;
            m_root = m_root->children[0];
            freeNode( oldRoot );
        }

        if( m_root && m_root->count == 0 )
        {
            freeNode( m_root );
            m_root = nullptr;
        }
    }

    /**
     * Recursive search. Returns true if search should continue, false if visitor stopped early.
     */
    template <class VISITOR>
    bool searchImpl( NODE* aNode, const ELEMTYPE aMin[NUMDIMS],
                     const ELEMTYPE aMax[NUMDIMS], VISITOR& aVisitor, int& aFound ) const
    {
        uint32_t mask = aNode->ChildOverlapMask( aMin, aMax );

        if( aNode->IsLeaf() )
        {
            while( mask )
            {
                int i = std::countr_zero( mask );
                mask &= mask - 1;
                aFound++;

                if( !aVisitor( aNode->data[i] ) )
                    return false;
            }
        }
        else
        {
            while( mask )
            {
                int i = std::countr_zero( mask );
                mask &= mask - 1;

                if( !searchImpl( aNode->children[i], aMin, aMax, aVisitor, aFound ) )
                    return false;
            }
        }

        return true;
    }

    /**
     * Compute minimum squared distance from a point to a child's bounding box.
     */
    int64_t minDistSq( NODE* aNode, int aSlot, const ELEMTYPE aPoint[NUMDIMS] ) const
    {
        int64_t dist = 0;

        for( int d = 0; d < NUMDIMS; ++d )
        {
            ELEMTYPE lo = aNode->bounds[d * 2][aSlot];
            ELEMTYPE hi = aNode->bounds[d * 2 + 1][aSlot];

            if( aPoint[d] < lo )
            {
                int64_t diff = static_cast<int64_t>( lo ) - aPoint[d];
                dist += diff * diff;
            }
            else if( aPoint[d] > hi )
            {
                int64_t diff = static_cast<int64_t>( aPoint[d] ) - hi;
                dist += diff * diff;
            }
        }

        return dist;
    }

    NODE*                        m_root = nullptr;
    size_t                       m_count = 0;
    SLAB_ALLOCATOR<NODE>         m_allocator;
};

} // namespace KIRTREE

#endif // DYNAMIC_RTREE_H
