/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
 * Copyright (C) 2020 CERN
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
 * along with this program; if not, you may find one here:
 * http://www.gnu.org/licenses/old-licenses/gpl-3.0.html
 * or you may search the http://www.gnu.org website for the version 3 license,
 * or you may write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
 */

#ifndef EESCHEMA_SCH_RTREE_H_
#define EESCHEMA_SCH_RTREE_H_

#include <core/typeinfo.h>
#include <sch_item.h>
#include <set>
#include <vector>

#include <geometry/rtree.h>

/**
 * Implement an R-tree for fast spatial and type indexing of schematic items.
 * Non-owning.
 */
class EE_RTREE
{
private:
    using ee_rtree = RTree<SCH_ITEM*, int, 3, double>;

public:
    EE_RTREE()
    {
        this->m_tree = new ee_rtree();
        m_count      = 0;
    }

    ~EE_RTREE()
    {
        delete this->m_tree;
    }

    /**
     * Insert an item into the tree. Item's bounding box is taken via its BBox() method.
     */
    void insert( SCH_ITEM* aItem )
    {
        BOX2I bbox = aItem->GetBoundingBox();

        // Inflate a bit for safety, selection shadows, etc.
        bbox.Inflate( aItem->GetPenWidth() );

        const int type    = int( aItem->Type() );
        const int mmin[3] = { type, bbox.GetX(), bbox.GetY() };
        const int mmax[3] = { type, bbox.GetRight(), bbox.GetBottom() };

        m_tree->Insert( mmin, mmax, aItem );
        m_count++;
    }

    /**
     * Remove an item from the tree.
     *
     * Removal is done by comparing pointers, attempting to remove a copy of the item will fail.
     */
    bool remove( SCH_ITEM* aItem )
    {
        // First, attempt to remove the item using its given BBox
        BOX2I bbox = aItem->GetBoundingBox();

        // Inflate a bit for safety, selection shadows, etc.
        bbox.Inflate( aItem->GetPenWidth() );

        const int type    = int( aItem->Type() );
        const int mmin[3] = { type, bbox.GetX(), bbox.GetY() };
        const int mmax[3] = { type, bbox.GetRight(), bbox.GetBottom() };

        // If we are not successful ( true == not found ), then we expand
        // the search to the full tree
        if( m_tree->Remove( mmin, mmax, aItem ) )
        {
            // N.B. We must search the whole tree for the pointer to remove
            // because the item may have been moved before we have the chance to
            // delete it from the tree
            const int mmin2[3] = { INT_MIN, INT_MIN, INT_MIN };
            const int mmax2[3] = { INT_MAX, INT_MAX, INT_MAX };

            if( m_tree->Remove( mmin2, mmax2, aItem ) )
                return false;
        }

        m_count--;
        return true;
    }

    /**
     * Remove all items from the RTree
     */
    void clear()
    {
        m_tree->RemoveAll();
        m_count = 0;
    }

    /**
     * Determine if a given item exists in the tree.
     *
     * @note This does not search the full tree so if the item has been moved, this will return
     *       false when it should be true.
     *
     * @param aItem Item that may potentially exist in the tree.
     * @param aRobust If true, search the whole tree, not just the bounding box.
     * @return true if the item definitely exists, false if it does not exist within bbox.
     */
    bool contains( const SCH_ITEM* aItem, bool aRobust = false ) const
    {
        BOX2I bbox = aItem->GetBoundingBox();

        // Inflate a bit for safety, selection shadows, etc.
        bbox.Inflate( aItem->GetPenWidth() );

        const int type    = int( aItem->Type() );
        const int mmin[3] = { type, bbox.GetX(), bbox.GetY() };
        const int mmax[3] = { type, bbox.GetRight(), bbox.GetBottom() };
        bool      found   = false;

        auto search =
                [&found, &aItem]( const SCH_ITEM* aSearchItem )
                {
                    if( aSearchItem == aItem )
                    {
                        found = true;
                        return false;
                    }

                    return true;
                };

        m_tree->Search( mmin, mmax, search );

        if( !found && aRobust )
        {
            // N.B. We must search the whole tree for the pointer to remove
            // because the item may have been moved.  We do not expand the item
            // type search as this should not change.

            const int mmin2[3] = { type, INT_MIN, INT_MIN };
            const int mmax2[3] = { type, INT_MAX, INT_MAX };

            m_tree->Search( mmin2, mmax2, search );
        }

        return found;
    }

    /**
     * Return the number of items in the tree.
     *
     * @return number of elements in the tree.
     */
    size_t size() const
    {
        return m_count;
    }

    bool empty() const
    {
        return m_count == 0;
    }

    using iterator = typename ee_rtree::Iterator;

    /**
     * The #EE_TYPE struct provides a type-specific auto-range iterator to the RTree.  Using
     * this struct, one can write lines like:
     *
     * for( auto item : rtree.OfType( SCH_SYMBOL_T ) )
     *
     * and iterate over the RTree items that are symbols only
     */
    struct EE_TYPE
    {
        EE_TYPE( ee_rtree* aTree, KICAD_T aType ) : type_tree( aTree )
        {
            KICAD_T type = BaseType( aType );

            if( type == SCH_LOCATE_ANY_T )
                m_rect = { { INT_MIN, INT_MIN, INT_MIN }, { INT_MAX, INT_MAX, INT_MAX } };
            else
                m_rect = { { type, INT_MIN, INT_MIN }, { type, INT_MAX, INT_MAX } };
        };

        EE_TYPE( ee_rtree* aTree, KICAD_T aType, const BOX2I& aRect ) : type_tree( aTree )
        {
            KICAD_T type = BaseType( aType );

            if( type == SCH_LOCATE_ANY_T )
            {
                m_rect = { { INT_MIN, aRect.GetX(),     aRect.GetY() },
                           { INT_MAX, aRect.GetRight(), aRect.GetBottom() } };
            }
            else
            {
                m_rect = { { type, aRect.GetX(),     aRect.GetY() },
                           { type, aRect.GetRight(), aRect.GetBottom() } };
            }
        };

        ee_rtree::Rect m_rect;
        ee_rtree*      type_tree;

        iterator begin()
        {
            return type_tree->begin( m_rect );
        }

        iterator end()
        {
            return type_tree->end( m_rect );
        }

        bool empty()
        {
            return type_tree->Count() == 0;
        }
    };

    EE_TYPE OfType( KICAD_T aType ) const
    {
        return EE_TYPE( m_tree, aType );
    }

    EE_TYPE Overlapping( const BOX2I& aRect ) const
    {
        return EE_TYPE( m_tree, SCH_LOCATE_ANY_T, aRect );
    }

    EE_TYPE Overlapping( const VECTOR2I& aPoint, int aAccuracy = 0 ) const
    {
        BOX2I rect( aPoint, VECTOR2I( 0, 0 ) );
        rect.Inflate( aAccuracy );
        return EE_TYPE( m_tree, SCH_LOCATE_ANY_T, rect );
    }

    EE_TYPE Overlapping( KICAD_T aType, const VECTOR2I& aPoint, int aAccuracy = 0 ) const
    {
        BOX2I rect( aPoint, VECTOR2I( 0, 0 ) );
        rect.Inflate( aAccuracy );
        return EE_TYPE( m_tree, aType, rect );
    }

    EE_TYPE Overlapping( KICAD_T aType, const BOX2I& aRect ) const
    {
        return EE_TYPE( m_tree, aType, aRect );
    }

    /**
     * Return a read/write iterator that points to the first.
     * element in the #EE_RTREE.
     *
     * @note The iteration order of the RTree is not readily apparent and will change
     * if/when you add or move items and the RTree is re-balanced.  Any exposure of the
     * RTree contents to the user MUST be sorted before being presented.  See
     * SCH_IO_KICAD_SEXPR::Format() or SCH_EDITOR_CONTROL::nextMatch() for examples.
     *
     * @return Complete RTree of the screen's items.
     */
    iterator begin()
    {
        return m_tree->begin();
    }

    /**
     * Return a read/write iterator that points to one past the last element in the #EE_RTREE.
     */
    iterator end()
    {
        return m_tree->end();
    }


    const iterator begin() const
    {
        return m_tree->begin();
    }

    const iterator end() const
    {
        return m_tree->end();
    }


private:
    ee_rtree* m_tree;
    size_t    m_count;
};


#endif /* EESCHEMA_SCH_RTREE_H_ */
