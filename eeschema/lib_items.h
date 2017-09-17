/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright 2017 CERN
 * @author Maciej Suminski <maciej.suminski@cern.ch>
 * @author Bernhard Stegmaier <stegmaier@sw-systems.de>
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

#ifndef LIB_ITEMS_H
#define LIB_ITEMS_H

/**
 * LIB_ITEM containers.
 */

#include <core/typeinfo.h>
#include <boost/ptr_container/ptr_vector.hpp>

class LIB_ITEM;

/**
 * Helper for defining a list of library draw object pointers.  The Boost
 * pointer containers are responsible for deleting object pointers placed
 * in them.  If you access a object pointer from the list, do not delete
 * it directly.
 */
typedef boost::ptr_vector<LIB_ITEM> LIB_ITEMS;


/**
 * LIB_ITEM container class. Provides both access as a flat list as well as
 * access by type of item.
 */
class LIB_ITEMS_CONTAINER
{
public:
    /**
     * Compile-time helper class to define some types depending on const/non-const iterator.
     */
    template< typename ITEM_TYPE > struct ITERATOR_ADAPTER;

    /**
     * Generic implementation of a flat const/non-const iterator over contained items.
     */
    template< typename ITEM_TYPE > class ITERATOR_BASE
    {
    public:
        ITERATOR_BASE& operator++();

        ITEM_TYPE& operator*()
        {
            return *m_it;
        }

        ITEM_TYPE* operator->()
        {
            return &( *m_it );
        }

        bool operator!=( const ITERATOR_BASE& aOther ) const;

    protected:
        /**
         * Constructor.
         * @param aItems is the container to wrap.
         * @param aIt is the iterator to initialize this iterator (usually some begin() or end()
         * iterator).
         * @param aBucket is the type ID of the given iterator.
         * @param aType enables item type filtering. When aType is TYPE_NOT_INIT, there is no
         * filtering and all item types are accessible by the iterator.
         */
        ITERATOR_BASE( typename ITERATOR_ADAPTER<ITEM_TYPE>::CONTAINER* aItems,
                       typename ITERATOR_ADAPTER<ITEM_TYPE>::ITERATOR aIt,
                       int aBucket, int aType = TYPE_NOT_INIT );

        ///> Assures the iterator is in a valid state.
        void validate();

        ///> Wrapped container
        typename ITERATOR_ADAPTER<ITEM_TYPE>::CONTAINER* m_parent;

        ///> Iterator for one of the LIB_ITEMS containers stored in the map
        typename ITERATOR_ADAPTER<ITEM_TYPE>::ITERATOR m_it;

        ///> Flag indicating whether type filtering is enabled
        bool m_filter;

        ///> Type of the currently iterated items (@see KICAD_T)
        int m_curType;

        friend class LIB_ITEMS_CONTAINER;
    };

    ///> The non-const iterator
    typedef ITERATOR_BASE< LIB_ITEM > ITERATOR;
    ///> The const iterator
    typedef ITERATOR_BASE< const LIB_ITEM > CONST_ITERATOR;


    LIB_ITEMS_CONTAINER()
    {
    }

    void push_back( LIB_ITEM* aItem );
    ITERATOR erase( const ITERATOR& aIterator );

    ITERATOR begin( int aType = TYPE_NOT_INIT );
    ITERATOR end( int aType = TYPE_NOT_INIT );
    CONST_ITERATOR begin( int aType = TYPE_NOT_INIT ) const;
    CONST_ITERATOR end( int aType = TYPE_NOT_INIT ) const;

    size_t size( int aType = TYPE_NOT_INIT ) const;
    bool empty( int aType = TYPE_NOT_INIT ) const;
    void sort();
    void unique();

    LIB_ITEMS& operator[]( int aType );
    const LIB_ITEMS& operator[]( int aType ) const;

    // Range of valid types handled by the iterator
    static constexpr KICAD_T FIRST_TYPE = LIB_ARC_T;
    static constexpr KICAD_T LAST_TYPE = LIB_FIELD_T;
    static constexpr size_t TYPES_COUNT = LAST_TYPE - FIRST_TYPE + 1;

private:
    ///> Get first non-empty type or first type if all are empty.
    size_t first() const;

    ///> Get last non-empty type or first type if all are empty.
    size_t last() const;

    ///> Contained items by type
    LIB_ITEMS m_data[ TYPES_COUNT ];
};

/*
 * Definitions for non-const iterator
 */
template<>
struct LIB_ITEMS_CONTAINER::ITERATOR_ADAPTER< LIB_ITEM >
{
    typedef LIB_ITEMS::iterator ITERATOR;
    typedef LIB_ITEMS_CONTAINER CONTAINER;
};

/*
 * Definitions for const iterator
 */
template<>
struct LIB_ITEMS_CONTAINER::ITERATOR_ADAPTER< const LIB_ITEM >
{
    typedef LIB_ITEMS::const_iterator ITERATOR;
    typedef const LIB_ITEMS_CONTAINER CONTAINER;
};

#endif /* LIB_ITEMS_H */
