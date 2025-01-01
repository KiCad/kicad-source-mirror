/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright 2017 CERN
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
 *
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

#ifndef MULTIVECTOR_H
#define MULTIVECTOR_H

#include <wx/debug.h>
#include <boost/ptr_container/ptr_vector.hpp>
#include <stdexcept>

/**
 * Multivector container type.
 *
 * Keeps items segregated by their type in multiple ptr_vectors. Provides both
 * access as a flat list as well as access by type of item.
 *
 * T is the stored type, needs to provide Type() method used to segregate items.
 * FIRST_TYPE_VAL is the lower boundary value of the types stored in the container.
 * LAST_TYPE_VAL is the upper boundary value of the types stored in the container.
 */
template<typename T, int FIRST_TYPE_VAL, int LAST_TYPE_VAL>
class MULTIVECTOR
{
public:
    /**
     * Type value to indicate no specific type. Mostly used to access the container as a flat list
     * or to return data for the whole container.
     */
    static constexpr int UNDEFINED_TYPE = 0;
    static_assert( FIRST_TYPE_VAL > UNDEFINED_TYPE,
                   "FIRST_TYPE_VAL must be greater than UNDEFINED_TYPE" );
    static_assert( FIRST_TYPE_VAL < LAST_TYPE_VAL,
                   "FIRST_TYPE_VAL must be greater than LAST_TYPE_VAL" );

    /**
    * Helper for defining a list of library draw object pointers.
    *
    * The Boost pointer containers are responsible for deleting object pointers placed
    * in them.  If you access a object pointer from the list, do not delete it directly.
    */
    typedef boost::ptr_vector<T> ITEM_PTR_VECTOR;

    /**
     * Generic implementation of a flat const/non-const iterator over contained items.
     */
    template<typename ITEM_TYPE, typename ITEM_CONTAINER, typename ITEM_CONTAINER_IT>
    class ITERATOR_BASE
    {
    public:
        ITEM_TYPE& operator*()
        {
            return *m_it;
        }

        ITEM_TYPE* operator->()
        {
            return &( *m_it );
        }

        ITERATOR_BASE& operator++()
        {
            if( m_it != (*m_parent)[ m_curType ].end() )
                ++m_it;

            validate();

            return *this;
        }

        bool operator!=( const ITERATOR_BASE& aOther ) const
        {
            if( aOther.m_parent != m_parent )
                return true;

            if( aOther.m_filter != m_filter )
                return true;

            if( aOther.m_curType != m_curType )
                return true;

            return aOther.m_it != m_it;
        }

    protected:
        /**
         * @param aItems is the container to wrap.
         * @param aIt is the iterator to initialize this iterator (usually some begin() or end()
         * iterator).
         * @param aBucket is the type ID of the given iterator.
         * @param aType enables item type filtering. When aType is UNDEFINED_TYPE, there is no
         * filtering and all item types are accessible by the iterator.
         */
        ITERATOR_BASE( ITEM_CONTAINER* aItems, ITEM_CONTAINER_IT aIt,
                       int aBucket, int aType = UNDEFINED_TYPE )
            : m_parent( aItems ), m_it( aIt ), m_curType( aBucket )
        {
            m_filter = ( aType != UNDEFINED_TYPE );
        }

        ///< Assures the iterator is in a valid state.
        void validate()
        {
            // for all-items iterators (unfiltered): check if this is the end of the
            // current type container, if so switch to the next non-empty container
            if( !m_filter && m_it == (*m_parent)[ m_curType ].end() )
            {
                // switch to the next type (look for a not empty container)
                int nextType = m_curType;

                do
                    ++nextType;
                while( ( nextType <= LAST_TYPE ) && (*m_parent)[ nextType ].empty() );

                // there is another not empty container, so make the iterator point to it,
                // otherwise it means the iterator points to the last item
                if( nextType <= LAST_TYPE )
                {
                    m_curType = nextType;
                    m_it = (*m_parent)[ m_curType ].begin();
                }
            }
        }

        ///< Wrapped container
        ITEM_CONTAINER* m_parent;

        ///< Iterator for one of the ptr_vector containers stored in the array
        ITEM_CONTAINER_IT m_it;

        ///< Flag indicating whether type filtering is enabled
        bool m_filter;

        ///< Type of the currently iterated items
        int m_curType;

        friend class MULTIVECTOR;
    };

    ///< The non-const iterator
    typedef ITERATOR_BASE<T, MULTIVECTOR<T, FIRST_TYPE_VAL, LAST_TYPE_VAL>,
                          typename ITEM_PTR_VECTOR::iterator> ITERATOR;
    ///< The const iterator
    typedef ITERATOR_BASE<const T, const MULTIVECTOR<T, FIRST_TYPE_VAL, LAST_TYPE_VAL>,
                          typename ITEM_PTR_VECTOR::const_iterator> CONST_ITERATOR;


    MULTIVECTOR()
    {
    }

    void push_back( T* aItem )
    {
        operator[]( aItem->Type() ).push_back( aItem );
    }

    ITERATOR erase( const ITERATOR& aIterator )
    {
        ITERATOR it( aIterator );
        it.m_it = (*aIterator.m_parent)[ aIterator.m_curType ].erase( aIterator.m_it );
        it.validate();

        return it;
    }

    ITERATOR begin( int aType = UNDEFINED_TYPE )
    {
        int bucket = ( aType != UNDEFINED_TYPE ) ? aType : first();
        return ITERATOR( this, operator[]( bucket ).begin(), bucket, aType );
    }

    ITERATOR end( int aType = UNDEFINED_TYPE )
    {
        int bucket = ( aType != UNDEFINED_TYPE ) ? aType : last();
        return ITERATOR( this, operator[]( bucket ).end(), bucket, aType );
    }

    CONST_ITERATOR begin( int aType = UNDEFINED_TYPE ) const
    {
        int bucket = ( aType != UNDEFINED_TYPE ) ? aType : first();
        return CONST_ITERATOR( this, operator[]( bucket ).begin(), bucket, aType );
    }

    CONST_ITERATOR end( int aType = UNDEFINED_TYPE ) const
    {
        int bucket = ( aType != UNDEFINED_TYPE ) ? aType : last();
        return CONST_ITERATOR( this, operator[]( bucket ).end(), bucket, aType );
    }

    void clear( int aType = UNDEFINED_TYPE )
    {
        if( aType != UNDEFINED_TYPE )
        {
            operator[]( aType ).clear();
        }
        else
        {
            for( int i = 0; i < TYPES_COUNT; ++i)
                m_data[ i ].clear();
        }
    }

    size_t size( int aType = UNDEFINED_TYPE ) const
    {
        if( aType != UNDEFINED_TYPE )
        {
            return operator[]( aType ).size();
        }
        else
        {
            size_t cnt = 0;

            for( int i = 0; i < TYPES_COUNT; ++i)
                cnt += m_data[ i ].size();

            return cnt;
        }
    }

    bool empty( int aType = UNDEFINED_TYPE ) const
    {
        return ( size( aType ) == 0 );
    }

    void sort()
    {
        for( int i = 0; i < TYPES_COUNT; ++i )
            m_data[ i ].sort();
    }

    /**
     * Remove duplicate elements in list
     */
    void unique()
    {
        for( int i = 0; i < TYPES_COUNT; ++i )
        {
            if( m_data[ i ].size() > 1 )
                m_data[ i ].unique();
        }
    }

    ITEM_PTR_VECTOR& operator[]( int aType )
    {
        if( ( aType < FIRST_TYPE ) || ( aType > LAST_TYPE ) )
        {
            wxFAIL_MSG( wxT( "Attempted access to type not within MULTIVECTOR" ) );

            // return type is a reference so we have to return something...
            aType = FIRST_TYPE;
        }

        return m_data[ aType - FIRST_TYPE ];
    }

    const ITEM_PTR_VECTOR& operator[]( int aType ) const
    {
        if( ( aType < FIRST_TYPE ) || ( aType > LAST_TYPE ) )
        {
            wxFAIL_MSG( wxT( "Attempted access to type not within MULTIVECTOR" ) );

            // return type is a reference so we have to return something...
            aType = FIRST_TYPE;
        }

        return m_data[ aType - FIRST_TYPE ];
    }

    // Range of valid types handled by the iterator
    static constexpr int FIRST_TYPE = FIRST_TYPE_VAL;
    static constexpr int LAST_TYPE = LAST_TYPE_VAL;
    static constexpr int TYPES_COUNT = LAST_TYPE - FIRST_TYPE + 1;

private:
    ///< Get first non-empty type or first type if all are empty.
    int first() const
    {
        int i = 0;

        while( ( i < TYPES_COUNT ) && ( m_data[ i ].empty() ) )
            ++i;

        return ( i == TYPES_COUNT ) ? FIRST_TYPE : FIRST_TYPE + i;
    }

    ///< Get last non-empty type or first type if all are empty.
    int last() const
    {
        int i = TYPES_COUNT - 1;

        while( ( i >= 0 ) && ( m_data[ i ].empty() ) )
            --i;

        return ( i < 0 ) ? FIRST_TYPE : FIRST_TYPE + i;
    }

    ///< Contained items by type
    ITEM_PTR_VECTOR m_data[TYPES_COUNT];
};

#endif /* MULTIVECTOR_H */
