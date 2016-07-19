/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2015 Mario Luzeiro <mrluzeiro@ua.pt>
 * Copyright (C) 1992-2015 KiCad Developers, see AUTHORS.txt for contributors.
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

/**
 * @file  lru_cache.h
 * @brief Template define a least-recently-used cache algo based on wxHashMap and wxString
 *        http://docs.wxwidgets.org/3.0/classwx_hash_map.html
 */

#ifndef LRU_CACHE_H
#define LRU_CACHE_H


#include <list>
#include <iterator>
#include <map>
#include <stdexcept>
#include <wx/string.h>


/**
 *  Template LRU_WXSTR_CACHE
 *  template for a wxString key based LRU cache
 *
 * @code
 * LRU_WXSTR_CACHE< int > cache;
 *
 * cache.Resize( 3 );
 *
 * cache.Insert( "First",  1 );
 * cache.Insert( "Second", 2 );
 *
 * printf(" cache.Size() %d \n", cache.Size() ); // size == 2
 * printf(" cache.MaxSize() %d \n", cache.MaxSize() ); // max size == 3
 * @endcode
 */
template< typename t_value >
class LRU_WXSTR_CACHE
{

private:

    /**
     * Declares KEY_VALUE_PAIR
     * Declares a pair with the key (wxString) and a value
     */
    typedef std::pair< wxString, t_value > KEY_VALUE_PAIR;

    /**
     * Declares LIST_ITERATOR
     * Declares a iterator type for a list of KEY_VALUE_PAIR
     */
    typedef std::list< KEY_VALUE_PAIR > CACHED_LIST;


    /**
     * Declares LIST_ITERATOR
     * Declares a iterator type for a list of KEY_VALUE_PAIR
     */
    typedef typename CACHED_LIST::iterator LIST_ITERATOR;


    /**
     * Declares WXSTR_HASH_MAP
     * Declares a map type of LIST_ITERATOR based on a wxString key
     */
    typedef std::map< wxString, LIST_ITERATOR > WXSTR_HASH_MAP;

    /**
     * Declares MAP_ITERATOR
     * Declares a iterator for the map
     */
    typedef typename WXSTR_HASH_MAP::iterator MAP_ITERATOR;


    /**
     * list of cached items
     */
    CACHED_LIST m_cached_list;

     /**
     * Cache map with iterators of the list
     */
     WXSTR_HASH_MAP m_map_iterators;


    /**
     *  Max capacity of the cache
     */
    size_t m_maxSize;

public:


     /**
     * Constructor LRU_WXSTR_CACHE
     * @param aMaxSize - initial max number of items of the cache
     */
    LRU_WXSTR_CACHE( size_t aMaxSize = 1 ) : m_maxSize( aMaxSize ) {}


    /**
     * Function Insert
     * @param aKey = the string key that is the reference of this entry
     * @param aValue = the value to add
     */
     void Insert( const wxString &aKey, const t_value &aValue )
     {
        MAP_ITERATOR it = m_map_iterators.find( aKey );

        if( it != m_map_iterators.end() )
        {
            // It already exists, so must remove it from list and form the map
            // it->second have a iterator from the list m_cached_list
            m_cached_list.erase( it->second );
            m_map_iterators.erase( it );
        }

        // Inserts a new element at the beginning of the list, a pair of <aKey, aValue>
        m_cached_list.push_front( KEY_VALUE_PAIR( aKey, aValue) );

        // Insert a new key and the added list iterator to the map
        m_map_iterators[aKey] = m_cached_list.begin();

        // Manage the size of the list
        if( m_cached_list.size() > m_maxSize )
        {
            // Get an iterator to the end of the list
            LIST_ITERATOR last_it = m_cached_list.end();

            // This gets the real iterator that is the latest one
            last_it--;

            // Remove the key from the map
            m_map_iterators.erase( last_it->first );

            // Removes the last element in the list
            m_cached_list.pop_back();
        }
     }


    /**
     * Function Get
     * Returns an existent value from the given key.
     * The key must exists, if not it will throw an error.
     * Use function Exists to check first if you can get that key
     * @param aKey = an existent key
     * @return t_value
     */
    const t_value& Get( const wxString &aKey )
    {
        MAP_ITERATOR map_it = m_map_iterators.find( aKey );

        if( map_it == m_map_iterators.end() )
        {
            throw std::range_error( "Requested a key that dont exists" );
        }

        // This will update the list and put in the beginning the iterator that we are getting
        m_cached_list.splice( m_cached_list.begin(), m_cached_list, map_it->second );

        // Return the t_value from the <key, value> pair that was in the list
        return map_it->second->second;
    }


    /**
     * Function Exists
     * @param aKey key to look for
     * @return true if the aKey exists
     */
    bool Exists( const wxString &aKey ) const
    {
        return ( m_map_iterators.find( aKey ) != m_map_iterators.end() );
    }


    /**
     * Function Resize
     * If aNewSize is smaller than the current maxSize then the items back in the list are discarded
     * This function can be used to empty the cache, setting the new size to 0
     * @param aNewSize - resize the store capability of the list to aNewSize
     */
    void Resize( size_t aNewSize )
    {
        m_maxSize = aNewSize;

        while( m_map_iterators.size() > m_maxSize )
        {
            // Remove the key from the map
            m_map_iterators.erase( m_cached_list.back().first );

            // Remove the back of the list
            m_cached_list.pop_back();
        }
    }


    /**
     * Function Size
     * @return size_t current size of the cache
     */
    size_t Size() const
    {
        return m_map_iterators.size();
    }


    /**
     * Function MaxSize
     * @return size_t current max size of the cache
     */
    size_t MaxSize() const
    {
        return m_maxSize;
    }


};

#endif // LRU_CACHE_H
