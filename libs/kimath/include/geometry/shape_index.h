/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2013 CERN
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
 *
 * @author Jacobo Aragunde Pérez
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
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#ifndef __SHAPE_INDEX_H
#define __SHAPE_INDEX_H

#include <vector>

#include <geometry/rtree/dynamic_rtree_cow.h>
#include <geometry/shape.h>
#include <math/box2.h>

/**
 * Used by #SHAPE_INDEX to get a SHAPE* from another type.
 *
 * By default relies on T::GetShape() method, should be specialized if the T object
 * doesn't allow that method.
 *
 * @param aItem generic T object.
 * @return a SHAPE* object equivalent to object.
 */
template <class T>
static const SHAPE* shapeFunctor( T aItem, int aLayer )
{
    return aItem->Shape( aLayer );
}

/**
 * Used by #SHAPE_INDEX to get the bounding box of a generic T object.
 *
 * By default relies on T::BBox() method, should be specialized if the T object
 * doesn't allow that method.
 *
 * @param aObject is a generic T object.
 * @return a BOX2I object containing the bounding box of the T object.
 */
template <class T>
BOX2I boundingBox( T aObject, int aLayer )
{
    BOX2I bbox = shapeFunctor( aObject, aLayer )->BBox();

    return bbox;
}

/**
 * Used by #SHAPE_INDEX to implement Accept().
 *
 * By default relies on V::operation() redefinition, should be specialized if V class
 * doesn't have its () operation defined to accept T objects.
 *
 * @param aObject is a generic T object.
 * @param aVisitor is a visitor object.
 */
template <class T, class V>
void acceptVisitor( T aObject, V aVisitor )
{
    aVisitor( aObject );
}

/**
 * Used by #SHAPE_INDEX to implement Query().
 *
 * By default relies on T::Collide(U) method, should be specialized if the T object
 * doesn't allow that method.
 *
 * @param aObject is a generic T object.
 * @param aAnotherObject is a generic U object.
 * @param aLayer is the layer to test
 * @param aMinDistance is the minimum collision distance.
 * @return true if object and anotherObject collide.
 */
template <class T, class U>
bool collide( T aObject, U aAnotherObject, int aLayer, int aMinDistance )
{
    return shapeFunctor( aObject, aLayer )->Collide( aAnotherObject, aMinDistance );
}

template <class T, class V>
bool queryCallback( T aShape, void* aContext )
{
    V* visitor = (V*) aContext;

    acceptVisitor<T, V>( aShape, *visitor );

    return true;
}

template <class T = SHAPE*>
class SHAPE_INDEX
{
    public:
        using TREE_TYPE = KIRTREE::COW_RTREE<T, int, 2>;

        class Iterator
        {
        private:
            using TreeIterator = typename TREE_TYPE::Iterator;
            TreeIterator m_current;
            TreeIterator m_end;

        public:
            Iterator( const TREE_TYPE& aTree ) :
                    m_current( aTree.begin() ),
                    m_end( aTree.end() )
            {
            }

            T operator*()
            {
                return *m_current;
            }

            /**
             * Shift the iterator to the next element.
             */
            bool operator++()
            {
                ++m_current;
                return m_current != m_end;
            }

            /**
             * Shift the iterator to the next element.
             */
            bool operator++( int )
            {
                ++m_current;
                return m_current != m_end;
            }

            /**
             * Check if the iterator has reached the end.
             *
             * @return true if it is in an invalid position (data finished).
             */
            bool IsNull() const
            {
                return m_current == m_end;
            }

            /**
             * Check if the iterator has not reached the end.
             *
             * @return true if it is in an valid position (data not finished).
             */
            bool IsNotNull() const
            {
                return m_current != m_end;
            }

            /**
             * Return the current element of the iterator and moves to the next position.
             *
             * @return a #SHAPE object pointed by the iterator before moving to the next position.
             */
            T Next()
            {
                T object = *m_current;
                ++m_current;

                return object;
            }
        };

        explicit SHAPE_INDEX( int aLayer ) : m_shapeLayer( aLayer ) {}

        ~SHAPE_INDEX() = default;

        // Move semantics
        SHAPE_INDEX( SHAPE_INDEX&& aOther ) noexcept = default;
        SHAPE_INDEX& operator=( SHAPE_INDEX&& aOther ) noexcept = default;

        // Non-copyable (use Clone() for intentional sharing)
        SHAPE_INDEX( const SHAPE_INDEX& ) = delete;
        SHAPE_INDEX& operator=( const SHAPE_INDEX& ) = delete;

        /**
         * Create a CoW clone that shares tree structure with this index.
         * O(1) -- only increments the root node's refcount.
         */
        SHAPE_INDEX Clone() const
        {
            SHAPE_INDEX clone( m_shapeLayer );
            clone.m_tree = m_tree.Clone();
            return clone;
        }

        /**
         * Add a #SHAPE to the index.
         *
         * @param aShape is the new SHAPE.
         */
        void Add( T aShape )
        {
            BOX2I box = boundingBox( aShape, m_shapeLayer );
            int min[2] = { box.GetX(), box.GetY() };
            int max[2] = { box.GetRight(), box.GetBottom() };

            m_tree.Insert( min, max, aShape );
        }

        /**
         * Add a shape with alternate BBox.
         *
         * @param aShape Shape (Item) to add.
         * @param aBbox alternate bounding box.  This should be a subset of the item's bbox
         */
        void Add( T aShape, const BOX2I& aBbox )
        {
            int min[2] = { aBbox.GetX(), aBbox.GetY() };
            int max[2] = { aBbox.GetRight(), aBbox.GetBottom() };

            m_tree.Insert( min, max, aShape );
        }

        /**
         * Remove a #SHAPE from the index.
         *
         * @param aShape is the #SHAPE to remove.
         */
        void Remove( T aShape )
        {
            BOX2I box = boundingBox( aShape, m_shapeLayer );
            int min[2] = { box.GetX(), box.GetY() };
            int max[2] = { box.GetRight(), box.GetBottom() };

            m_tree.Remove( min, max, aShape );
        }

        /**
         * Remove all the contents of the index.
         */
        void RemoveAll()
        {
            m_tree.RemoveAll();
        }

        /**
         * Accept a visitor for every #SHAPE object contained in this INDEX.
         *
         * @param aVisitor is the visitor object to be run.
         */
        template <class V>
        void Accept( V aVisitor )
        {
            for( const T& item : m_tree )
                acceptVisitor( item, aVisitor );
        }

        /**
         * Build from a batch of items using Hilbert-curve bulk loading.
         * Replaces all existing content. O(n log n).
         */
        void BulkLoad( std::vector<std::pair<T, BOX2I>>& aItems )
        {
            using BULK_ENTRY = typename TREE_TYPE::BULK_ENTRY;
            std::vector<BULK_ENTRY> entries;
            entries.reserve( aItems.size() );

            for( const auto& [item, box] : aItems )
            {
                BULK_ENTRY e;
                e.min[0] = box.GetX();
                e.min[1] = box.GetY();
                e.max[0] = box.GetRight();
                e.max[1] = box.GetBottom();
                e.data = item;
                entries.push_back( e );
            }

            m_tree.BulkLoad( entries );
        }

        /**
         * Rebuild the index.
         *
         * This should be used if the geometry of the objects contained by the index has changed.
         */
        void Reindex()
        {
            std::vector<T> items;
            items.reserve( m_tree.size() );

            for( const T& item : m_tree )
                items.push_back( item );

            m_tree.RemoveAll();

            for( T& item : items )
                Add( item );
        }

        /**
         * Run a callback on every #SHAPE object contained in the bounding box of (shape).
         *
         * @param aShape is the shape to search against.
         * @param aMinDistance is the distance threshold.
         * @param aVisitor is the object to be invoked on every object contained in the search area.
         */
        template <class V>
        int Query( const SHAPE *aShape, int aMinDistance, V& aVisitor) const
        {
            BOX2I box = aShape->BBox();
            box.Inflate( aMinDistance );

            int min[2] = { box.GetX(),         box.GetY() };
            int max[2] = { box.GetRight(),     box.GetBottom() };

            return m_tree.Search( min, max, aVisitor );
        }

        /**
         * Create an iterator for the current index object.
         *
         * @return iterator to the first object.
         */
        Iterator Begin() const
        {
            return Iterator( m_tree );
        }

        // Range-for support
        typename TREE_TYPE::Iterator begin() const { return m_tree.begin(); }
        typename TREE_TYPE::Iterator end() const { return m_tree.end(); }

        size_t Size() const { return m_tree.size(); }

    private:
        TREE_TYPE m_tree;
        int m_shapeLayer;
};

#endif /* __SHAPE_INDEX_H */
