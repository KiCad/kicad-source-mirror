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

#include <lib_items.h>
#include <lib_draw_item.h>
#include <stdexcept>
#include <wx/debug.h>


LIB_ITEMS_LIST::ITERATOR::ITERATOR( LIB_ITEMS_MAP& aItems, int aType )
    : m_parent( &aItems )
{
    m_filter = ( aType != TYPE_NOT_INIT );
    m_type = m_filter ? aType : FIRST_TYPE;
    m_it = (*m_parent)[m_type].begin();

    // be sure the enum order is correct for the type
    static_assert( (int) ( ITERATOR::FIRST_TYPE ) < (int) ( ITERATOR::LAST_TYPE ),
            "fix FIRST_TYPE and LAST_TYPE definitions" );
    wxASSERT( m_type >= ITERATOR::FIRST_TYPE && m_type <= ITERATOR::LAST_TYPE );
}


LIB_ITEMS_LIST::ITERATOR LIB_ITEMS_LIST::begin( int aType ) const
{
    LIB_ITEMS_LIST::ITERATOR it( m_data, aType );

    if( it.m_filter )       // iterates over a specific type
    {
        it.m_it = (*it.m_parent)[it.m_type].begin();
    }
    else                    // iterates over all items
    {
        // find a not empty container
        auto i = m_data.begin();

        while( i->second.empty() && i != m_data.end() )
            ++i;

        if( i == m_data.end() )
            --i;

        it.m_it = i->second.begin();
        it.m_type = i->first;
    }

    return it;
}


LIB_ITEMS_LIST::ITERATOR LIB_ITEMS_LIST::end( int aType ) const
{
    LIB_ITEMS_LIST::ITERATOR it( m_data, aType );

    if( it.m_filter )       // iterates over a specific type
    {
        it.m_it = (*it.m_parent)[it.m_type].end();
    }
    else                    // iterates over all items
    {
        // find a not empty container, starting from the last type
        auto i = m_data.rbegin();

        while( i->second.empty() && i != m_data.rend() )
            ++i;

        if( i == m_data.rend() )
            --i;

        it.m_it = i->second.end();
        it.m_type = i->first;
    }

    return it;
}


void LIB_ITEMS_LIST::push_back( LIB_ITEM* aItem )
{
    wxASSERT( aItem->Type() >= ITERATOR::FIRST_TYPE && aItem->Type() <= ITERATOR::LAST_TYPE );
    m_data[aItem->Type()].push_back( aItem );
}


LIB_ITEMS_LIST::ITERATOR LIB_ITEMS_LIST::erase( const ITERATOR& aIterator )
{
    LIB_ITEMS_LIST::ITERATOR it( aIterator );
    it.m_it = (*aIterator.m_parent)[aIterator.m_type].erase( aIterator.m_it );
    it.validate();

    return it;
}


size_t LIB_ITEMS_LIST::size() const
{
    size_t counter = 0;

    for( auto& type : m_data )
        counter += type.second.size();

    return counter;
}


bool LIB_ITEMS_LIST::empty() const
{
    for( auto& type : m_data )
    {
        if( !type.second.empty() )
            return false;
    }

    return true;
}


void LIB_ITEMS_LIST::sort()
{
    for( auto& itemType : m_data )
        itemType.second.sort();
}


LIB_ITEM& LIB_ITEMS_LIST::operator[]( unsigned int aIdx )
{
    int counter = 0;

    for( auto& type : m_data )
    {
        if( aIdx < counter + type.second.size() )
            return type.second[aIdx - counter];
        else
            counter += type.second.size();
    }

    throw std::out_of_range( "LIB_ITEMS_LIST out of range" );
}


const LIB_ITEM& LIB_ITEMS_LIST::operator[]( unsigned int aIdx ) const
{
    int counter = 0;

    for( const auto& type : m_data )
    {
        if( aIdx < counter + type.second.size() )
            return type.second[aIdx - counter];
        else
            counter += type.second.size();
    }

    throw std::out_of_range( "LIB_ITEMS_LIST out of range" );
}


LIB_ITEMS_LIST::ITERATOR& LIB_ITEMS_LIST::ITERATOR::operator++()
{
    if( m_it != (*m_parent)[m_type].end() )
        ++m_it;

    validate();

    return *this;
}


bool LIB_ITEMS_LIST::ITERATOR::operator!=( const LIB_ITEMS_LIST::ITERATOR& aOther ) const
{
    wxASSERT( aOther.m_parent == m_parent );
    wxASSERT( aOther.m_filter == m_filter );
    wxASSERT( !m_filter || aOther.m_type == m_type );

    return aOther.m_it != m_it;
}


void LIB_ITEMS_LIST::ITERATOR::validate()
{
    // for all-items iterators (unfiltered): check if this is the end of the
    // current type container, if so switch to the next non-empty container
    if( m_it == (*m_parent)[m_type].end() && !m_filter )
    {
        auto typeIt = m_parent->find( m_type );
        wxASSERT( typeIt != m_parent->end() );

        // switch to the next type (look for a not empty container)
        do
            ++typeIt;
        while( typeIt != m_parent->end() && typeIt->second.empty() );

        // there is another not empty container, so make the iterator point to it,
        // otherwise it means the iterator points to the last item
        if( typeIt != m_parent->end() )
        {
            m_it = typeIt->second.begin();
            m_type = typeIt->first;
        }
    }
}
