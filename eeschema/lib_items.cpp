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

#include <lib_items.h>
#include <lib_draw_item.h>
#include <stdexcept>
#include <wx/debug.h>


void LIB_ITEMS_CONTAINER::push_back( LIB_ITEM* aItem )
{
    operator[]( aItem->Type() ).push_back( aItem );
}


LIB_ITEMS_CONTAINER::ITERATOR LIB_ITEMS_CONTAINER::erase(
        const LIB_ITEMS_CONTAINER::ITERATOR& aIterator )
{
    LIB_ITEMS_CONTAINER::ITERATOR it( aIterator );
    it.m_it = (*aIterator.m_parent)[ aIterator.m_curType ].erase( aIterator.m_it );
    it.validate();

    return it;
}


LIB_ITEMS_CONTAINER::ITERATOR LIB_ITEMS_CONTAINER::begin( int aType )
{
    size_t bucket = ( aType != TYPE_NOT_INIT ) ? aType : first();
    return ITERATOR( this, operator[]( bucket ).begin(), bucket, aType );
}


LIB_ITEMS_CONTAINER::ITERATOR LIB_ITEMS_CONTAINER::end( int aType )
{
    size_t bucket = ( aType != TYPE_NOT_INIT ) ? aType : last();
    return ITERATOR( this, operator[]( bucket ).end(), bucket, aType );
}


LIB_ITEMS_CONTAINER::CONST_ITERATOR LIB_ITEMS_CONTAINER::begin( int aType ) const
{
    size_t bucket = ( aType != TYPE_NOT_INIT ) ? aType : first();
    return CONST_ITERATOR( this, operator[]( bucket ).begin(), bucket, aType );
}


LIB_ITEMS_CONTAINER::CONST_ITERATOR LIB_ITEMS_CONTAINER::end( int aType ) const
{
    size_t bucket = ( aType != TYPE_NOT_INIT ) ? aType : last();
    return CONST_ITERATOR( this, operator[]( bucket ).end(), bucket, aType );
}


size_t LIB_ITEMS_CONTAINER::size( int aType ) const
{
    if( aType != TYPE_NOT_INIT )
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


bool LIB_ITEMS_CONTAINER::empty( int aType ) const
{
    return ( size( aType ) == 0 );
}


void LIB_ITEMS_CONTAINER::sort()
{
    for( int i = 0; i < TYPES_COUNT; ++i )
        m_data[ i ].sort();
}


void LIB_ITEMS_CONTAINER::unique()
{
    for( int i = 0; i < TYPES_COUNT; ++i )
        m_data[ i ].unique();
}


LIB_ITEMS& LIB_ITEMS_CONTAINER::operator[]( int aType )
{
    if( ( aType < FIRST_TYPE ) || ( aType > LAST_TYPE ) )
        throw std::out_of_range( "LIB_ITEMS_CONTAINER out of range" );

    return m_data[ aType - FIRST_TYPE ];
}


const LIB_ITEMS& LIB_ITEMS_CONTAINER::operator[]( int aType ) const
{
    if( ( aType < FIRST_TYPE ) || ( aType > LAST_TYPE ) )
        throw std::out_of_range( "LIB_ITEMS_CONTAINER out of range" );

    return m_data[ aType - FIRST_TYPE ];
}


size_t LIB_ITEMS_CONTAINER::first() const
{
    int i = 0;

    while( ( i < TYPES_COUNT ) && ( m_data[ i ].empty() ) )
        ++i;

    return ( i == TYPES_COUNT ) ? FIRST_TYPE : FIRST_TYPE + i;
}


size_t LIB_ITEMS_CONTAINER::last() const
{
    int i = TYPES_COUNT - 1;

    while( ( i >= 0 ) && ( m_data[ i ].empty() ) )
        --i;

    return ( i < 0 ) ? FIRST_TYPE : FIRST_TYPE + i;
}


template< typename ITEM_TYPE >
LIB_ITEMS_CONTAINER::ITERATOR_BASE<ITEM_TYPE>& LIB_ITEMS_CONTAINER::ITERATOR_BASE<ITEM_TYPE>::operator++()
{
    if( m_it != (*m_parent)[ m_curType ].end() )
        ++m_it;

    validate();

    return *this;
}


template< typename ITEM_TYPE >
bool LIB_ITEMS_CONTAINER::ITERATOR_BASE<ITEM_TYPE>::operator!=(
        const LIB_ITEMS_CONTAINER::ITERATOR_BASE<ITEM_TYPE>& aOther ) const
{
    if( aOther.m_parent != m_parent )
        return true;

    if( aOther.m_filter != m_filter )
        return true;

    if( aOther.m_curType != m_curType )
        return true;

    return aOther.m_it != m_it;
}


template< typename ITEM_TYPE >
LIB_ITEMS_CONTAINER::ITERATOR_BASE<ITEM_TYPE>::ITERATOR_BASE(
        typename LIB_ITEMS_CONTAINER::ITERATOR_ADAPTER< ITEM_TYPE >::CONTAINER* aItems,
        typename LIB_ITEMS_CONTAINER::ITERATOR_ADAPTER< ITEM_TYPE >::ITERATOR aIt,
        int aBucket, int aType )
    : m_parent( aItems ), m_it( aIt ), m_curType( aBucket )
{
    m_filter = ( aType != TYPE_NOT_INIT );
}


template< typename ITEM_TYPE >
void LIB_ITEMS_CONTAINER::ITERATOR_BASE<ITEM_TYPE>::validate()
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

/*
 * Template instantiation for const/non-const iterator
 */
template class LIB_ITEMS_CONTAINER::ITERATOR_BASE< LIB_ITEM >;
template class LIB_ITEMS_CONTAINER::ITERATOR_BASE< const LIB_ITEM >;
