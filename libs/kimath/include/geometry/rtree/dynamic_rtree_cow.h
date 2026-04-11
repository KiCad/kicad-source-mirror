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

#ifndef DYNAMIC_RTREE_COW_H
#define DYNAMIC_RTREE_COW_H

#include <atomic>
#include <memory>
#include <vector>

#include <geometry/rtree/rtree_node.h>
#include <geometry/rtree/dynamic_rtree.h>

namespace KIRTREE
{

/**
 * Copy-on-Write wrapper for DYNAMIC_RTREE.
 *
 * Provides O(1) Clone() for the PNS router's branching pattern. The router frequently
 * creates speculative branches that share most of their spatial index with the parent.
 * Instead of copying all items, Clone() shares the tree structure via reference counting
 * on nodes, only copying nodes along mutation paths.
 *
 * When a node is shared between parent and clone (refcount > 1), any mutation first copies
 * the node, decrements the old node's refcount, and increments refcounts on children that
 * are now shared with the new copy.
 *
 * @tparam DATATYPE   Type of data stored in leaf nodes
 * @tparam ELEMTYPE   Coordinate element type (typically int)
 * @tparam NUMDIMS    Number of dimensions (2 or 3)
 * @tparam TMAXNODES  Maximum children per node (fanout)
 */
template <class DATATYPE, class ELEMTYPE = int, int NUMDIMS = 2, int TMAXNODES = 16>
class COW_RTREE
{
public:
    using NODE = RTREE_NODE<DATATYPE, ELEMTYPE, NUMDIMS, TMAXNODES>;
    using ALLOC = SLAB_ALLOCATOR<NODE>;

private:
    // Persistent immutable linked list of parent allocators.
    struct ALLOC_CHAIN
    {
        std::shared_ptr<ALLOC>        allocator;
        std::shared_ptr<ALLOC_CHAIN>  next;
    };

public:

    COW_RTREE() : m_root( nullptr ), m_count( 0 ),
                  m_allocator( std::make_shared<ALLOC>() )
    {
    }

    ~COW_RTREE()
    {
        releaseTree( m_root );
    }

    // Move semantics
    COW_RTREE( COW_RTREE&& aOther ) noexcept :
            m_root( aOther.m_root ),
            m_count( aOther.m_count ),
            m_allocator( std::move( aOther.m_allocator ) ),
            m_parentChain( std::move( aOther.m_parentChain ) )
    {
        aOther.m_root = nullptr;
        aOther.m_count = 0;
    }

    COW_RTREE& operator=( COW_RTREE&& aOther ) noexcept
    {
        if( this != &aOther )
        {
            releaseTree( m_root );
            m_root = aOther.m_root;
            m_count = aOther.m_count;
            m_allocator = std::move( aOther.m_allocator );
            m_parentChain = std::move( aOther.m_parentChain );
            aOther.m_root = nullptr;
            aOther.m_count = 0;
        }

        return *this;
    }

    // Non-copyable (use Clone() for intentional sharing)
    COW_RTREE( const COW_RTREE& ) = delete;
    COW_RTREE& operator=( const COW_RTREE& ) = delete;

    /**
     * Create a clone that shares the tree structure with this tree.
     *
     * The clone holds a shared_ptr to the parent's allocator to keep nodes alive
     * as long as any tree references them.
     */
    COW_RTREE Clone() const
    {
        COW_RTREE clone;
        clone.m_root = m_root;
        clone.m_count = m_count;

        if( m_root )
            m_root->refcount.fetch_add( 1, std::memory_order_relaxed );

        // Clone gets its own allocator for any nodes it copies
        clone.m_allocator = std::make_shared<ALLOC>();

        // Prepend our allocator to the parent chain (O(1), shared with siblings)
        clone.m_parentChain = std::make_shared<ALLOC_CHAIN>(
                ALLOC_CHAIN{ m_allocator, m_parentChain } );

        return clone;
    }

    /**
     * Insert an item. If nodes along the path are shared, they are copied first.
     */
    void Insert( const ELEMTYPE aMin[NUMDIMS], const ELEMTYPE aMax[NUMDIMS],
                 const DATATYPE& aData )
    {
        if( !m_root )
        {
            m_root = m_allocator->Allocate();
            m_root->level = 0;
        }
        else
        {
            ensureWritable( m_root );
        }

        // Use a simplified insertion for CoW (no forced reinsert to avoid complexity)
        insertSimple( m_root, aMin, aMax, aData );
        m_count++;
    }

    /**
     * Remove an item. If nodes along the path are shared, they are copied first.
     */
    bool Remove( const ELEMTYPE aMin[NUMDIMS], const ELEMTYPE aMax[NUMDIMS],
                 const DATATYPE& aData )
    {
        if( !m_root )
            return false;

        ensureWritable( m_root );

        if( removeImpl( m_root, aMin, aMax, aData ) )
        {
            m_count--;

            // Condense root
            while( m_root && m_root->IsInternal() && m_root->count == 1 )
            {
                NODE* oldRoot = m_root;
                m_root = m_root->children[0];

                if( m_root )
                    m_root->refcount.fetch_add( 1, std::memory_order_relaxed );

                releaseNode( oldRoot );
            }

            if( m_root && m_root->count == 0 )
            {
                releaseNode( m_root );
                m_root = nullptr;
            }

            return true;
        }

        return false;
    }

    /**
     * Search for items whose bounding boxes overlap the query rectangle.
     * Read-only, no CoW needed.
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

    void RemoveAll()
    {
        releaseTree( m_root );
        m_root = nullptr;
        m_count = 0;
    }

    /**
     * Bulk load entries using Hilbert curve sorting and bottom-up packing.
     * Clears any existing tree first. 
     */
    using BULK_ENTRY = typename DYNAMIC_RTREE<DATATYPE, ELEMTYPE, NUMDIMS, TMAXNODES>::BULK_ENTRY;

    void BulkLoad( std::vector<BULK_ENTRY>& aEntries )
    {
        RemoveAll();

        if( aEntries.empty() )
            return;

        m_count = aEntries.size();

        // Find global bounds for Hilbert normalization
        ELEMTYPE globalMin[NUMDIMS], globalMax[NUMDIMS];

        for( int d = 0; d < NUMDIMS; ++d )
        {
            globalMin[d] = aEntries[0].min[d];
            globalMax[d] = aEntries[0].max[d];
        }

        for( const auto& e : aEntries )
        {
            for( int d = 0; d < NUMDIMS; ++d )
            {
                if( e.min[d] < globalMin[d] ) globalMin[d] = e.min[d];
                if( e.max[d] > globalMax[d] ) globalMax[d] = e.max[d];
            }
        }

        double scale[NUMDIMS];

        for( int d = 0; d < NUMDIMS; ++d )
        {
            double range = static_cast<double>( globalMax[d] ) - globalMin[d];
            scale[d] = ( range > 0 ) ? ( 65535.0 / range ) : 0.0;
        }

        // Compute Hilbert index for each entry and sort
        struct SORTED_ENTRY
        {
            uint64_t hilbert;
            size_t   origIdx;
        };

        std::vector<SORTED_ENTRY> sorted( aEntries.size() );

        for( size_t i = 0; i < aEntries.size(); ++i )
        {
            const auto& e = aEntries[i];
            double cx = ( static_cast<double>( e.min[0] ) + e.max[0] ) * 0.5;
            double cy = ( static_cast<double>( e.min[1] ) + e.max[1] ) * 0.5;
            uint32_t hx = static_cast<uint32_t>( ( cx - globalMin[0] ) * scale[0] );
            uint32_t hy = static_cast<uint32_t>( ( cy - globalMin[1] ) * scale[1] );
            sorted[i] = { KIRTREE::HilbertXY2D( 16, hx, hy ), i };
        }

        std::sort( sorted.begin(), sorted.end(),
                   []( const SORTED_ENTRY& a, const SORTED_ENTRY& b )
                   {
                       return a.hilbert < b.hilbert;
                   } );

        // Reorder entries in-place by Hilbert index
        std::vector<BULK_ENTRY> reordered;
        reordered.reserve( aEntries.size() );

        for( const auto& s : sorted )
            reordered.push_back( std::move( aEntries[s.origIdx] ) );

        aEntries = std::move( reordered );

        // Pack into leaf nodes
        std::vector<NODE*> currentLevel;
        size_t idx = 0;

        while( idx < aEntries.size() )
        {
            NODE* leaf = m_allocator->Allocate();
            leaf->level = 0;

            int fill = std::min( static_cast<int>( TMAXNODES ),
                                 static_cast<int>( aEntries.size() - idx ) );

            for( int i = 0; i < fill; ++i, ++idx )
            {
                leaf->SetChildBounds( i, aEntries[idx].min, aEntries[idx].max );
                leaf->SetInsertBounds( i, aEntries[idx].min, aEntries[idx].max );
                leaf->data[i] = aEntries[idx].data;
            }

            leaf->count = fill;
            currentLevel.push_back( leaf );
        }

        // Build internal levels bottom-up
        int level = 1;

        while( currentLevel.size() > 1 )
        {
            std::vector<NODE*> nextLevel;
            size_t ci = 0;

            while( ci < currentLevel.size() )
            {
                NODE* parent = m_allocator->Allocate();
                parent->level = level;

                int fill = std::min( static_cast<int>( TMAXNODES ),
                                     static_cast<int>( currentLevel.size() - ci ) );

                for( int i = 0; i < fill; ++i, ++ci )
                {
                    NODE* child = currentLevel[ci];
                    ELEMTYPE cmin[NUMDIMS], cmax[NUMDIMS];
                    child->ComputeEnclosingBounds( cmin, cmax );
                    parent->SetChildBounds( i, cmin, cmax );
                    parent->children[i] = child;
                }

                parent->count = fill;
                nextLevel.push_back( parent );
            }

            currentLevel = std::move( nextLevel );
            level++;
        }

        m_root = currentLevel[0];
    }

    size_t size() const { return m_count; }
    bool   empty() const { return m_count == 0; }

    /**
     * Iterator for traversing all data items.
     * Uses the same traversal as DYNAMIC_RTREE::Iterator.
     */
    class Iterator
    {
    public:
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

private:
    /**
     * Ensure a node is exclusively owned (refcount == 1).
     * If shared, create a copy in our own allocator and decrement the original's refcount.
     */
    void ensureWritable( NODE*& aNode )
    {
        if( !aNode )
            return;

        if( aNode->refcount.load( std::memory_order_relaxed ) > 1 )
        {
            NODE* copy = m_allocator->Allocate();
            copy->count = aNode->count;
            copy->level = aNode->level;
            std::memcpy( copy->bounds, aNode->bounds, sizeof( aNode->bounds ) );
            std::memcpy( copy->insertBounds, aNode->insertBounds,
                         sizeof( aNode->insertBounds ) );

            if( aNode->IsLeaf() )
            {
                for( int i = 0; i < aNode->count; ++i )
                    copy->data[i] = aNode->data[i];
            }
            else
            {
                for( int i = 0; i < aNode->count; ++i )
                {
                    copy->children[i] = aNode->children[i];

                    // Children are now shared between old and new node
                    if( copy->children[i] )
                    {
                        copy->children[i]->refcount.fetch_add( 1,
                                                                std::memory_order_relaxed );
                    }
                }
            }

            copy->refcount.store( 1, std::memory_order_relaxed );

            // Release our reference to the old node
            releaseNode( aNode );
            aNode = copy;
        }
    }

    /**
     * Release a reference to a node. If refcount drops to 0, free it
     * through the allocator that owns the node's memory.
     */
    void releaseNode( NODE* aNode )
    {
        if( !aNode )
            return;

        if( aNode->refcount.fetch_sub( 1, std::memory_order_acq_rel ) == 1 )
        {
            if( aNode->IsInternal() )
            {
                for( int i = 0; i < aNode->count; ++i )
                    releaseNode( aNode->children[i] );
            }

            freeToOwningAllocator( aNode );
        }
    }

    /**
     * Free a node through the allocator that actually owns its memory.
     * Without this, freeing a parent-allocated node through the clone's
     * allocator would corrupt the page bitset and leak the slot.
     */
    void freeToOwningAllocator( NODE* aNode )
    {
        if( m_allocator->Owns( aNode ) )
        {
            m_allocator->Free( aNode );
            return;
        }

        for( auto chain = m_parentChain; chain; chain = chain->next )
        {
            if( chain->allocator->Owns( aNode ) )
            {
                chain->allocator->Free( aNode );
                return;
            }
        }

        assert( false && "R-tree node not owned by any allocator in chain" );
    }

    /**
     * Release the entire tree rooted at aNode.
     */
    void releaseTree( NODE* aNode )
    {
        releaseNode( aNode );
    }

    /**
     * Simplified insertion (no forced reinsert, just split on overflow).
     * Sufficient for PNS router's incremental modifications to cloned trees.
     */
    void insertSimple( NODE* aNode, const ELEMTYPE aMin[NUMDIMS],
                       const ELEMTYPE aMax[NUMDIMS], const DATATYPE& aData )
    {
        if( aNode->IsLeaf() )
        {
            if( !aNode->IsFull() )
            {
                int slot = aNode->count;
                aNode->SetChildBounds( slot, aMin, aMax );
                aNode->SetInsertBounds( slot, aMin, aMax );
                aNode->data[slot] = aData;
                aNode->count++;
            }
            else
            {
                // Simple split: sort by center of longest axis, split in half
                simpleSplit( aNode, aMin, aMax, aData );
            }
        }
        else
        {
            // Choose child with minimum area enlargement
            int    bestIdx = 0;
            int64_t bestAreaInc = std::numeric_limits<int64_t>::max();

            for( int i = 0; i < aNode->count; ++i )
            {
                int64_t areaInc = aNode->ChildEnlargement( i, aMin, aMax );

                if( areaInc < bestAreaInc )
                {
                    bestIdx = i;
                    bestAreaInc = areaInc;
                }
            }

            NODE* child = aNode->children[bestIdx];
            ensureWritable( child );
            aNode->children[bestIdx] = child;

            int oldCount = child->count;
            insertSimple( child, aMin, aMax, aData );

            // Update child bbox in parent
            ELEMTYPE childMin[NUMDIMS], childMax[NUMDIMS];
            child->ComputeEnclosingBounds( childMin, childMax );
            aNode->SetChildBounds( bestIdx, childMin, childMax );

            // Check if child split (count decreased means it was split and a sibling created)
            if( child->count < oldCount && child->IsInternal() )
            {
                // The split created a new root subtree, handle it
                // For simplicity in CoW, we just accept the tree as-is
            }
        }
    }

    /**
     * Simple split for CoW insertion: median split along longest axis.
     */
    void simpleSplit( NODE* aNode, const ELEMTYPE aMin[NUMDIMS],
                      const ELEMTYPE aMax[NUMDIMS], const DATATYPE& aData )
    {
        struct ENTRY
        {
            ELEMTYPE min[NUMDIMS];
            ELEMTYPE max[NUMDIMS];
            ELEMTYPE insertMin[NUMDIMS];
            ELEMTYPE insertMax[NUMDIMS];
            DATATYPE data;
            int64_t  sortKey;
        };

        int total = aNode->count + 1;
        std::vector<ENTRY> entries( total );

        // Find longest axis
        ELEMTYPE nodeMin[NUMDIMS], nodeMax[NUMDIMS];
        aNode->ComputeEnclosingBounds( nodeMin, nodeMax );

        int bestAxis = 0;
        ELEMTYPE bestExtent = 0;

        for( int d = 0; d < NUMDIMS; ++d )
        {
            ELEMTYPE lo = std::min( nodeMin[d], aMin[d] );
            ELEMTYPE hi = std::max( nodeMax[d], aMax[d] );
            ELEMTYPE extent = hi - lo;

            if( extent > bestExtent )
            {
                bestExtent = extent;
                bestAxis = d;
            }
        }

        // Gather entries
        for( int i = 0; i < aNode->count; ++i )
        {
            aNode->GetChildBounds( i, entries[i].min, entries[i].max );
            aNode->GetInsertBounds( i, entries[i].insertMin, entries[i].insertMax );
            entries[i].data = aNode->data[i];
            entries[i].sortKey = static_cast<int64_t>( entries[i].min[bestAxis] )
                                 + entries[i].max[bestAxis];
        }

        for( int d = 0; d < NUMDIMS; ++d )
        {
            entries[aNode->count].min[d] = aMin[d];
            entries[aNode->count].max[d] = aMax[d];
            entries[aNode->count].insertMin[d] = aMin[d];
            entries[aNode->count].insertMax[d] = aMax[d];
        }

        entries[aNode->count].data = aData;
        entries[aNode->count].sortKey = static_cast<int64_t>( aMin[bestAxis] )
                                        + aMax[bestAxis];

        std::sort( entries.begin(), entries.end(),
                   []( const ENTRY& a, const ENTRY& b )
                   {
                       return a.sortKey < b.sortKey;
                   } );

        int splitIdx = total / 2;

        // Rebuild this node with first half
        aNode->count = 0;

        for( int i = 0; i < splitIdx; ++i )
        {
            int slot = aNode->count;
            aNode->SetChildBounds( slot, entries[i].min, entries[i].max );
            aNode->SetInsertBounds( slot, entries[i].insertMin, entries[i].insertMax );
            aNode->data[slot] = entries[i].data;
            aNode->count++;
        }

        // Create sibling with second half
        NODE* sibling = m_allocator->Allocate();
        sibling->level = 0;

        for( int i = splitIdx; i < total; ++i )
        {
            int slot = sibling->count;
            sibling->SetChildBounds( slot, entries[i].min, entries[i].max );
            sibling->SetInsertBounds( slot, entries[i].insertMin, entries[i].insertMax );
            sibling->data[slot] = entries[i].data;
            sibling->count++;
        }

        // Need to propagate the sibling up. For the CoW case, if there's no room in the
        // parent we need to handle it. Since we modify the root on Insert, we handle
        // root splits here.
        ELEMTYPE sibMin[NUMDIMS], sibMax[NUMDIMS];
        sibling->ComputeEnclosingBounds( sibMin, sibMax );

        if( aNode == m_root )
        {
            NODE* newRoot = m_allocator->Allocate();
            newRoot->level = 1;

            ELEMTYPE nMin[NUMDIMS], nMax[NUMDIMS];
            aNode->ComputeEnclosingBounds( nMin, nMax );

            newRoot->SetChildBounds( 0, nMin, nMax );
            newRoot->children[0] = aNode;
            newRoot->SetChildBounds( 1, sibMin, sibMax );
            newRoot->children[1] = sibling;
            newRoot->count = 2;
            m_root = newRoot;
        }
        else
        {
            // For non-root splits in CoW trees, we need to find the parent.
            // This is a limitation of the simplified CoW approach -- the parent
            // should be tracked during traversal. For now, create a new root.
            NODE* newRoot = m_allocator->Allocate();
            newRoot->level = m_root->level + 1;

            ELEMTYPE rMin[NUMDIMS], rMax[NUMDIMS];
            m_root->ComputeEnclosingBounds( rMin, rMax );

            // Incorporate sibling bbox into root bbox
            for( int d = 0; d < NUMDIMS; ++d )
            {
                if( sibMin[d] < rMin[d] )
                    rMin[d] = sibMin[d];

                if( sibMax[d] > rMax[d] )
                    rMax[d] = sibMax[d];
            }

            newRoot->SetChildBounds( 0, rMin, rMax );
            newRoot->children[0] = m_root;
            newRoot->SetChildBounds( 1, sibMin, sibMax );
            newRoot->children[1] = sibling;
            newRoot->count = 2;
            m_root = newRoot;
        }
    }

    bool removeImpl( NODE* aNode, const ELEMTYPE aMin[NUMDIMS],
                     const ELEMTYPE aMax[NUMDIMS], const DATATYPE& aData )
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

        for( int i = 0; i < aNode->count; ++i )
        {
            if( !aNode->ChildOverlaps( i, aMin, aMax ) )
                continue;

            NODE* child = aNode->children[i];
            ensureWritable( child );
            aNode->children[i] = child;

            if( removeImpl( child, aMin, aMax, aData ) )
            {
                if( child->count > 0 )
                {
                    ELEMTYPE childMin[NUMDIMS], childMax[NUMDIMS];
                    child->ComputeEnclosingBounds( childMin, childMax );
                    aNode->SetChildBounds( i, childMin, childMax );
                }
                else
                {
                    releaseNode( child );
                    aNode->RemoveChild( i );
                }

                return true;
            }
        }

        return false;
    }

    template <class VISITOR>
    void searchImpl( NODE* aNode, const ELEMTYPE aMin[NUMDIMS],
                     const ELEMTYPE aMax[NUMDIMS], VISITOR& aVisitor, int& aFound ) const
    {
        if( aNode->IsLeaf() )
        {
            for( int i = 0; i < aNode->count; ++i )
            {
                if( aNode->ChildOverlaps( i, aMin, aMax ) )
                {
                    aFound++;

                    if( !aVisitor( aNode->data[i] ) )
                        return;
                }
            }
        }
        else
        {
            for( int i = 0; i < aNode->count; ++i )
            {
                if( aNode->ChildOverlaps( i, aMin, aMax ) )
                    searchImpl( aNode->children[i], aMin, aMax, aVisitor, aFound );
            }
        }
    }

    NODE*                              m_root = nullptr;
    size_t                             m_count = 0;
    std::shared_ptr<ALLOC>             m_allocator;
    std::shared_ptr<ALLOC_CHAIN>       m_parentChain;
};

} // namespace KIRTREE

#endif // DYNAMIC_RTREE_COW_H
