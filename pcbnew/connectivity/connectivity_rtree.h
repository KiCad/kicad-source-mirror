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
 * along with this program; if not, you may find one here:
 * http://www.gnu.org/licenses/old-licenses/gpl-2.0.html
 * or you may search the http://www.gnu.org website for the version 2 license,
 * or you may write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
 */

#ifndef PCBNEW_CONNECTIVITY_RTREE_H_
#define PCBNEW_CONNECTIVITY_RTREE_H_

#include <math/box2.h>
#include <router/pns_layerset.h>

#include <geometry/rtree.h>


/**
 * CN_RTREE -
 * Implements an R-tree for fast spatial indexing of connectivity items.
 * Non-owning.
 */
template< class T >
class CN_RTREE
{
public:

    CN_RTREE()
    {
        this->m_tree = new RTree<T, int, 3, double>();
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
        const BOX2I& bbox = aItem->BBox();

        const int mmin[3] = { aItem->StartLayer(), bbox.GetX(), bbox.GetY() };
        const int mmax[3] = { aItem->EndLayer(), bbox.GetRight(), bbox.GetBottom() };

        m_tree->Insert( mmin, mmax, aItem );
    }

    /**
     * Function Remove()
     * Removes an item from the tree. Removal is done by comparing pointers, attempting
     * to remove a copy of the item will fail.
     */
    void Remove( T aItem )
    {

        // First, attempt to remove the item using its given BBox
        const BOX2I&        bbox    = aItem->BBox();

        const int           mmin[3] = { aItem->StartLayer(), bbox.GetX(), bbox.GetY() };
        const int           mmax[3] = { aItem->EndLayer(), bbox.GetRight(), bbox.GetBottom() };

        // If we are not successful ( 1 == not found ), then we expand
        // the search to the full tree
        if( m_tree->Remove( mmin, mmax, aItem ) )
        {
            // N.B. We must search the whole tree for the pointer to remove
            // because the item may have been moved before we have the chance to
            // delete it from the tree
            const int       mmin2[3] = { INT_MIN, INT_MIN, INT_MIN };
            const int       mmax2[3] = { INT_MAX, INT_MAX, INT_MAX };
            m_tree->Remove( mmin2, mmax2, aItem );
        }
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
    void Query( const BOX2I& aBounds, int aStartLayer, int aEndLayer, Visitor& aVisitor ) const
    {
        int start_layer = aStartLayer == B_Cu ? INT_MAX : aStartLayer;
        int end_layer = aEndLayer == B_Cu ? INT_MAX : aEndLayer;

        const int   mmin[3] = { start_layer, aBounds.GetX(), aBounds.GetY() };
        const int   mmax[3] = { end_layer, aBounds.GetRight(), aBounds.GetBottom() };

        m_tree->Search( mmin, mmax, aVisitor );
    }

private:

    RTree<T, int, 3, double>* m_tree;
};


#endif /* PCBNEW_CONNECTIVITY_RTREE_H_ */
