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

#ifndef PCBNEW_CONNECTIVITY_RTREE_H_
#define PCBNEW_CONNECTIVITY_RTREE_H_

#include <layer_ids.h>
#include <math/box2.h>
#include <router/pns_layerset.h>

#include <geometry/rtree/dynamic_rtree.h>


/**
 * CN_RTREE -
 * Implements an R-tree for fast spatial indexing of connectivity items.
 * Non-owning.
 */
template< class T >
class CN_RTREE
{
public:
    using TREE = KIRTREE::DYNAMIC_RTREE<T, int, 3>;

    ///< Entry type accepted by BulkLoad().
    using BULK_ENTRY = typename TREE::BULK_ENTRY;

    CN_RTREE() = default;
    ~CN_RTREE() = default;

    CN_RTREE( CN_RTREE&& aOther ) noexcept = default;
    CN_RTREE& operator=( CN_RTREE&& aOther ) noexcept = default;

    CN_RTREE( const CN_RTREE& ) = delete;
    CN_RTREE& operator=( const CN_RTREE& ) = delete;

    /**
     * Function Insert()
     * Inserts an item into the tree. Item's bounding box is taken via its BBox() method.
     */
    void Insert( T aItem )
    {
        const BOX2I& bbox = aItem->BBox();

        const int mmin[3] = { aItem->StartLayer(), bbox.GetX(), bbox.GetY() };
        const int mmax[3] = { aItem->EndLayer(), bbox.GetRight(), bbox.GetBottom() };

        m_tree.Insert( mmin, mmax, aItem );
    }

    /**
     * Function Remove()
     * Removes an item from the tree. Removal is done by comparing pointers, attempting
     * to remove a copy of the item will fail.
     */
    void Remove( T aItem )
    {
        const BOX2I& bbox = aItem->BBox();

        const int mmin[3] = { aItem->StartLayer(), bbox.GetX(), bbox.GetY() };
        const int mmax[3] = { aItem->EndLayer(), bbox.GetRight(), bbox.GetBottom() };

        // DYNAMIC_RTREE::Remove tries the provided bbox first, then falls back
        // to full-tree search if the item has moved since insertion.
        m_tree.Remove( mmin, mmax, aItem );
    }

    /**
     * Function RemoveAll()
     * Removes all items from the RTree
     */
    void RemoveAll()
    {
        m_tree.RemoveAll();
    }

    /**
     * Function BulkLoad()
     * Replaces the tree contents with aEntries, packed bottom-up.  Much cheaper than the
     * same number of Insert() calls, which each pay for an R*-tree subtree choice and may
     * cascade into forced reinsertion.
     */
    void BulkLoad( std::vector<BULK_ENTRY>& aEntries )
    {
        m_tree.BulkLoad( aEntries );
    }

    /**
     * Function Query()
     * Executes a function object aVisitor for each item whose bounding box intersects
     * with aBounds.
     */
    template <class Visitor>
    void Query( const BOX2I& aBounds, int aStartLayer, int aEndLayer, Visitor& aVisitor ) const
    {
        // The layer bounds are copper layer ordinals, the same as Insert() stores.
        const int mmin[3] = { aStartLayer, aBounds.GetX(), aBounds.GetY() };
        const int mmax[3] = { aEndLayer, aBounds.GetRight(), aBounds.GetBottom() };

        m_tree.Search( mmin, mmax, aVisitor );
    }

private:
    TREE m_tree;
};


#endif /* PCBNEW_CONNECTIVITY_RTREE_H_ */
