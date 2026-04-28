/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2013 CERN
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
 *
 * @author Tomasz Wlostowski <tomasz.wlostowski@cern.ch>
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

#ifndef __VIEW_RTREE_H
#define __VIEW_RTREE_H

#include <math/box2.h>

#include <geometry/rtree/dynamic_rtree.h>

namespace KIGFX
{

class VIEW_ITEM;

/**
 * Implement a non-owning R-tree for fast spatial indexing of VIEW items.
 *
 * Uses DYNAMIC_RTREE with stored insertion bboxes, eliminating the O(N)
 * fallback removal that the old RTree required when bbox was unavailable.
 */
class VIEW_RTREE
{
public:
    /**
     * Insert an item into the tree.
     *
     * Item's bounding box is taken via its ViewBBox() method.
     */
    void Insert( VIEW_ITEM* aItem, const BOX2I& bbox )
    {
        const int mmin[2] = { std::min( bbox.GetX(), bbox.GetRight() ),
                              std::min( bbox.GetY(), bbox.GetBottom() ) };
        const int mmax[2] = { std::max( bbox.GetX(), bbox.GetRight() ),
                              std::max( bbox.GetY(), bbox.GetBottom() ) };

        m_tree.Insert( mmin, mmax, aItem );
    }

    /**
     * Remove an item from the tree.
     *
     * Removal is done by comparing pointers, attempting to remove a copy of the item will fail.
     */
    void Remove( VIEW_ITEM* aItem, const BOX2I* aBbox )
    {
        if( aBbox )
        {
            const int mmin[2] = { std::min( aBbox->GetX(), aBbox->GetRight() ),
                                  std::min( aBbox->GetY(), aBbox->GetBottom() ) };
            const int mmax[2] = { std::max( aBbox->GetX(), aBbox->GetRight() ),
                                  std::max( aBbox->GetY(), aBbox->GetBottom() ) };
            m_tree.Remove( mmin, mmax, aItem );
            return;
        }

        // DYNAMIC_RTREE::Remove falls back to full-tree search when the provided
        // bbox doesn't match, so passing INT_MIN/INT_MAX triggers that path.
        const int mmin[2] = { INT_MIN, INT_MIN };
        const int mmax[2] = { INT_MAX, INT_MAX };
        m_tree.Remove( mmin, mmax, aItem );
    }

    /**
     * Execute a function object \a aVisitor for each item whose bounding box intersects
     * with \a aBounds.
     */
    template <class Visitor>
    void Query( const BOX2I& aBounds, Visitor& aVisitor ) const
    {
        int mmin[2] = { std::min( aBounds.GetX(), aBounds.GetRight() ),
                        std::min( aBounds.GetY(), aBounds.GetBottom() ) };
        int mmax[2] = { std::max( aBounds.GetX(), aBounds.GetRight() ),
                        std::max( aBounds.GetY(), aBounds.GetBottom() ) };

        // We frequently use the maximum bounding box to recache all items
        // or for any item that overflows the integer width limits of BBOX2I
        // in this case, we search the full rtree whose bounds are absolute
        // coordinates rather than relative
        BOX2I max_box;
        max_box.SetMaximum();

        if( aBounds == max_box )
        {
            mmin[0] = mmin[1] = INT_MIN;
            mmax[0] = mmax[1] = INT_MAX;
        }

        m_tree.Search( mmin, mmax, aVisitor );
    }

    /**
     * Remove all items from the tree.
     */
    void RemoveAll()
    {
        m_tree.RemoveAll();
    }

    /**
     * Build the R-tree from a batch of items using packed bulk loading.
     *
     * Dramatically faster than individual Insert calls when rebuilding from scratch.
     */
    void BulkLoad( std::vector<std::pair<VIEW_ITEM*, BOX2I>>& aItems )
    {
        using BULK_ENTRY = KIRTREE::DYNAMIC_RTREE<VIEW_ITEM*, int, 2>::BULK_ENTRY;
        std::vector<BULK_ENTRY> entries;
        entries.reserve( aItems.size() );

        for( const auto& [item, bbox] : aItems )
        {
            BULK_ENTRY e;
            e.min[0] = std::min( bbox.GetX(), bbox.GetRight() );
            e.min[1] = std::min( bbox.GetY(), bbox.GetBottom() );
            e.max[0] = std::max( bbox.GetX(), bbox.GetRight() );
            e.max[1] = std::max( bbox.GetY(), bbox.GetBottom() );
            e.data = item;
            entries.push_back( e );
        }

        m_tree.BulkLoad( entries );
    }

    bool IsEmpty() const { return m_tree.empty(); }

private:
    KIRTREE::DYNAMIC_RTREE<VIEW_ITEM*, int, 2> m_tree;
};

} // namespace KIGFX

#endif
