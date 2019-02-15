/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2013 CERN
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

#include <geometry/rtree.h>

namespace KIGFX
{
typedef RTree<VIEW_ITEM*, int, 2, double> VIEW_RTREE_BASE;

/**
 * Class VIEW_RTREE -
 * Implements an R-tree for fast spatial indexing of VIEW items.
 * Non-owning.
 */
class VIEW_RTREE : public VIEW_RTREE_BASE
{
public:

    /**
     * Function Insert()
     * Inserts an item into the tree. Item's bounding box is taken via its ViewBBox() method.
     */
    void Insert( VIEW_ITEM* aItem )
    {
        const BOX2I&    bbox    = aItem->ViewBBox();
        const int       mmin[2] = { bbox.GetX(), bbox.GetY() };
        const int       mmax[2] = { bbox.GetRight(), bbox.GetBottom() };

        VIEW_RTREE_BASE::Insert( mmin, mmax, aItem );
    }

    /**
     * Function Remove()
     * Removes an item from the tree. Removal is done by comparing pointers, attepmting to remove a copy
     * of the item will fail.
     */
    void Remove( VIEW_ITEM* aItem )
    {
        // const BOX2I&    bbox    = aItem->ViewBBox();

        // FIXME: use cached bbox or ptr_map to speed up pointer <-> node lookups.
        const int       mmin[2] = { INT_MIN, INT_MIN };
        const int       mmax[2] = { INT_MAX, INT_MAX };

        VIEW_RTREE_BASE::Remove( mmin, mmax, aItem );
    }

    /**
     * Function Query()
     * Executes a function object aVisitor for each item whose bounding box intersects
     * with aBounds.
     */
    template <class Visitor>
    void Query( const BOX2I& aBounds, Visitor& aVisitor )    // const
    {
        int   mmin[2] = { aBounds.GetX(), aBounds.GetY() };
        int   mmax[2] = { aBounds.GetRight(), aBounds.GetBottom() };

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

        VIEW_RTREE_BASE::Search( mmin, mmax, aVisitor );
    }

private:
};
} // namespace KIGFX

#endif
