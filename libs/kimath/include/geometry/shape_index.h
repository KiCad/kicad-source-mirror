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
 * along with this program; if not, you may find one here:
 * http://www.gnu.org/licenses/old-licenses/gpl-2.0.html
 * or you may search the http://www.gnu.org website for the version 2 license,
 * or you may write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
 */

#ifndef __SHAPE_INDEX_H
#define __SHAPE_INDEX_H

#include <vector>

#include <geometry/rtree.h>
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
        class Iterator
        {
        private:
            typedef typename RTree<T, int, 2, double>::Iterator RTreeIterator;
            RTreeIterator iterator;

            /**
             * Setup the internal tree iterator.
             *
             * @param aTree is a #RTREE object/
             */
            void Init( RTree<T, int, 2, double>* aTree )
            {
                aTree->GetFirst( iterator );
            }

        public:
            /**
             * Create an iterator for the index object.
             *
             * @param aIndex is a #SHAPE_INDEX object to iterate.
             */
            Iterator( SHAPE_INDEX* aIndex )
            {
                Init( aIndex->m_tree );
            }

            /**
             * Return the next data element.
             */
            T operator*()
            {
                return *iterator;
            }

            /**
             * Shift the iterator to the next element.
             */
            bool operator++()
            {
                return ++iterator;
            }

            /**
             * Shift the iterator to the next element.
             */
            bool operator++( int )
            {
                return ++iterator;
            }

            /**
             * Check if the iterator has reached the end.
             *
             * @return true if it is in an invalid position (data finished).
             */
            bool IsNull() const
            {
                return iterator.IsNull();
            }

            /**
             * Check if the iterator has not reached the end.
             *
             * @return true if it is in an valid position (data not finished).
             */
            bool IsNotNull() const
            {
                return iterator.IsNotNull();
            }

            /**
             * Return the current element of the iterator and moves to the next position.
             *
             * @return a #SHAPE object pointed by the iterator before moving to the next position.
             */
            T Next()
            {
                T object = *iterator;
                ++iterator;

                return object;
            }
        };

        explicit SHAPE_INDEX( int aLayer );

        ~SHAPE_INDEX();

        /**
         * Add a #SHAPE to the index.
         *
         * @param aShape is the new SHAPE.
         */
        void Add( T aShape );

        /**
         * Add a shape with alternate BBox.
         *
         * @param aShape Shape (Item) to add.
         * @param aBbox alternate bounding box.  This should be a subset of the item's bbox
         */
        void Add( T aShape, const BOX2I& aBbox );

        /**
         * Remove a #SHAPE from the index.
         *
         * @param aShape is the #SHAPE to remove.
         */
        void Remove( T aShape );

        /**
         * Remove all the contents of the index.
         */
        void RemoveAll();

        /**
         * Accept a visitor for every #SHAPE object contained in this INDEX.
         *
         * @param aVisitor is the visitor object to be run.
         */
        template <class V>
        void Accept( V aVisitor )
        {
            Iterator iter = this->Begin();

            while( !iter.IsNull() )
            {
                T shape = *iter;
                acceptVisitor( shape, aVisitor );
                iter++;
            }
        }

        /**
         * Rebuild the index.
         *
         * This should be used if the geometry of the objects contained by the index has changed.
         */
        void Reindex();

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

            return this->m_tree->Search( min, max, aVisitor );
        }

        /**
         * Create an iterator for the current index object.
         *
         * @return iterator to the first object.
         */
        Iterator Begin();

    private:
        RTree<T, int, 2, double>* m_tree;
        int m_shapeLayer;
};

/*
 * Class members implementation
 */

template <class T>
SHAPE_INDEX<T>::SHAPE_INDEX( int aLayer )
{
    this->m_tree = new RTree<T, int, 2, double>();
    this->m_shapeLayer = aLayer;
}

template <class T>
SHAPE_INDEX<T>::~SHAPE_INDEX()
{
    delete this->m_tree;
}

template <class T>
void SHAPE_INDEX<T>::Add( T aShape, const BOX2I& aBbox )
{
    int min[2] = { aBbox.GetX(), aBbox.GetY() };
    int max[2] = { aBbox.GetRight(), aBbox.GetBottom() };

    this->m_tree->Insert( min, max, aShape );
}

template <class T>
void SHAPE_INDEX<T>::Add( T aShape )
{
    BOX2I box = boundingBox( aShape, this->m_shapeLayer );
    int min[2] = { box.GetX(), box.GetY() };
    int max[2] = { box.GetRight(), box.GetBottom() };

    this->m_tree->Insert( min, max, aShape );
}

template <class T>
void SHAPE_INDEX<T>::Remove( T aShape )
{
    BOX2I box = boundingBox( aShape, this->m_shapeLayer );
    int min[2] = { box.GetX(), box.GetY() };
    int max[2] = { box.GetRight(), box.GetBottom() };

    this->m_tree->Remove( min, max, aShape );
}

template <class T>
void SHAPE_INDEX<T>::RemoveAll()
{
    this->m_tree->RemoveAll();
}

template <class T>
void SHAPE_INDEX<T>::Reindex()
{
    RTree<T, int, 2, double>* newTree;
    newTree = new RTree<T, int, 2, double>();

    Iterator iter = this->Begin();

    while( !iter.IsNull() )
    {
        T shape = *iter;
        BOX2I box = boundingBox( shape, this->m_shapeLayer );
        int min[2] = { box.GetX(), box.GetY() };
        int max[2] = { box.GetRight(), box.GetBottom() };
        newTree->Insert( min, max, shape );
        iter++;
    }

    delete this->m_tree;
    this->m_tree = newTree;
}

template <class T>
typename SHAPE_INDEX<T>::Iterator SHAPE_INDEX<T>::Begin()
{
    return Iterator( this );
}

#endif /* __SHAPE_INDEX_H */
