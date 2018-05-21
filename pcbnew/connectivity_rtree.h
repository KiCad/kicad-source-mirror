/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2018 KiCad Developers, see AUTHORS.txt for contributors.
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

#ifndef PCBNEW_CONNECTIVITY_RTREE_H_
#define PCBNEW_CONNECTIVITY_RTREE_H_

#include <math/box2.h>

#include <geometry/rtree.h>


/**
 * Class CN_RTREE -
 * Implements an R-tree for fast spatial indexing of connectivity items.
 * Non-owning.
 */
template< class T >
class CN_RTREE
{
public:

    CN_RTREE()
    {
        this->m_tree = new RTree<T, int, 2, float>();
    }

    ~CN_RTREE()
    {
        delete this->m_tree;
    }

    /**
     * Function Insert()
     * Inserts an item into the tree. Item's bounding box is taken via its BBox() method.
     */
    void Insert( T aItem )
    {
        const BOX2I&    bbox    = aItem->BBox();
        const int       mmin[2] = { bbox.GetX(), bbox.GetY() };
        const int       mmax[2] = { bbox.GetRight(), bbox.GetBottom() };

        m_tree->Insert( mmin, mmax, aItem );
    }

    /**
     * Function Remove()
     * Removes an item from the tree. Removal is done by comparing pointers, attempting
     * to remove a copy of the item will fail.
     */
    void Remove( T aItem )
    {
        const BOX2I&    bbox    = aItem->BBox();
        const int       mmin[2] = { bbox.GetX(), bbox.GetY() };
        const int       mmax[2] = { bbox.GetRight(), bbox.GetBottom() };

        m_tree->Remove( mmin, mmax, aItem );
    }

    /**
     * Function RemoveAll()
     * Removes all items from the RTree
     */
    void RemoveAll( )
    {
        m_tree->RemoveAll();
    }

    /**
     * Function Query()
     * Executes a function object aVisitor for each item whose bounding box intersects
     * with aBounds.
     */
    template <class Visitor>
    void Query( const BOX2I& aBounds, Visitor& aVisitor )
    {
        const int   mmin[2] = { aBounds.GetX(), aBounds.GetY() };
        const int   mmax[2] = { aBounds.GetRight(), aBounds.GetBottom() };

        m_tree->Search( mmin, mmax, aVisitor );
    }

private:

    RTree<T, int, 2, float>* m_tree;
};


#endif /* PCBNEW_CONNECTIVITY_RTREE_H_ */
