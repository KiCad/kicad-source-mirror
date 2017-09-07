/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright 2017 CERN
 * @author Maciej Suminski <maciej.suminski@cern.ch>
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
#include <map>

class LIB_ITEM;

/**
 * Helper for defining a list of library draw object pointers.  The Boost
 * pointer containers are responsible for deleting object pointers placed
 * in them.  If you access a object pointer from the list, do not delete
 * it directly.
 */
typedef boost::ptr_vector<LIB_ITEM> LIB_ITEMS;

/**
 * LIB_ITEM container to keep them sorted by their type.
 */
typedef std::map<int, LIB_ITEMS> LIB_ITEMS_MAP;

/**
 * Wrapper for LIB_ITEMS_MAP to provide flat (list-like) access to the items
 * stored in the map (@see LIB_ITEMS_MAP).
 */
class LIB_ITEMS_LIST
{
public:
    class ITERATOR
    {
    public:
        ITERATOR& operator++();

        LIB_ITEM& operator*()
        {
            return *m_it;
        }

        LIB_ITEM* operator->()
        {
            return &(*m_it);
        }

        bool operator!=( const ITERATOR& aOther ) const;

    private:
        /**
         * Constructor.
         * @param aItems is the container to wrap.
         * @param aType enables item type filtering. When aType is TYPE_NOT_INIT, there is no
         * filtering and all item types are accessible by the iterator.
         */
        ITERATOR( LIB_ITEMS_MAP& aItems, int aType = TYPE_NOT_INIT );

        ///> Assures the iterator is in a valid state.
        void validate();

        ///> Wrapped container
        LIB_ITEMS_MAP* m_parent;

        ///> Flag indicating whether type filtering is enabled
        bool m_filter;

        ///> Type of the currently iterated items (@see KICAD_T)
        int m_type;

        ///> Iterator for one of the LIB_ITEMS containers stored in the map
        LIB_ITEMS::iterator m_it;

        // Range of valid types handled by the iterator
        static constexpr KICAD_T FIRST_TYPE = LIB_ARC_T;
        static constexpr KICAD_T LAST_TYPE = LIB_FIELD_T;

        friend class LIB_ITEMS_LIST;
    };


    LIB_ITEMS_LIST( LIB_ITEMS_MAP& aData )
        : m_data( aData )
    {
    }

    ITERATOR begin( int aType = TYPE_NOT_INIT ) const;
    ITERATOR end( int aType = TYPE_NOT_INIT ) const;

    void push_back( LIB_ITEM* aItem );
    ITERATOR erase( const ITERATOR& aIterator );

    size_t size() const;
    bool empty() const;
    void sort();

    LIB_ITEM& operator[]( unsigned int aIdx );
    const LIB_ITEM& operator[]( unsigned int aIdx ) const;

private:
    ///> Wrapped container
    LIB_ITEMS_MAP& m_data;
};


#endif /* LIB_ITEMS_H */
