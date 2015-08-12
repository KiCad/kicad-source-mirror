/*
 * KiRouter - a push-and-(sometimes-)shove PCB router
 *
 * Copyright (C) 2013-2014 CERN
 * Author: Tomasz Wlostowski <tomasz.wlostowski@cern.ch>
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation, either version 3 of the License, or (at your
 * option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef __PNS_ITEMSET_H
#define __PNS_ITEMSET_H

#include <deque>
#include <boost/foreach.hpp>

#include "pns_item.h"

/**
 * Class PNS_ITEMSET
 *
 * Holds a list of board items, that can be filtered against net, kinds,
 * layers, etc.
 **/
class PNS_LINE;

class PNS_ITEMSET : public PNS_OBJECT
{
public:
    typedef std::deque<PNS_ITEM*> ITEMS;

    PNS_ITEMSET( PNS_ITEM* aInitialItem = NULL, bool aBecomeOwner = false )
    {
        if( aInitialItem )
        {
            if( aBecomeOwner )
                aInitialItem->SetOwner( this );

            m_items.push_back( aInitialItem );
        }
    }

    PNS_ITEMSET( const PNS_ITEMSET& aOther )
    {
        copyFrom( aOther );
    }

    ~PNS_ITEMSET();

    const PNS_ITEMSET& operator=( const PNS_ITEMSET& aOther )
    {
        copyFrom( aOther );
        return *this;
    }

    int Count( int aKindMask = -1 ) const
    {
        int n = 0;

        BOOST_FOREACH( PNS_ITEM* item, m_items )
        {
            if( item->Kind() & aKindMask )
                n++;
        }

        return n;
    }

    bool Empty() const
    {
        return m_items.empty();
    }

    ITEMS& Items() { return m_items; }
    const ITEMS& CItems() const { return m_items; }

    PNS_ITEMSET& FilterLayers( int aStart, int aEnd = -1, bool aInvert = false );
    PNS_ITEMSET& FilterKinds( int aKindMask, bool aInvert = false );
    PNS_ITEMSET& FilterNet( int aNet, bool aInvert = false );
    PNS_ITEMSET& FilterMarker( int aMarker, bool aInvert = false );

    PNS_ITEMSET& ExcludeLayers( int aStart, int aEnd = -1 )
    {
        return FilterLayers( aStart, aEnd, true );
    }

    PNS_ITEMSET& ExcludeKinds( int aKindMask )
    {
        return FilterKinds( aKindMask, true );
    }

    PNS_ITEMSET& ExcludeNet( int aNet )
    {
        return FilterNet( aNet, true );
    }

    PNS_ITEMSET& ExcludeItem( const PNS_ITEM* aItem );

    int Size() const
    {
        return m_items.size();
    }

    void Add( const PNS_LINE& aLine );
    void Prepend( const PNS_LINE& aLine );

    void Add( PNS_ITEM* aItem, bool aBecomeOwner = false )
    {
        if( aBecomeOwner )
            aItem->SetOwner( this );

        m_items.push_back( aItem );
    }

    void Prepend( PNS_ITEM* aItem, bool aBecomeOwner = false )
    {
        if( aBecomeOwner )
            aItem->SetOwner( this );

        m_items.push_front( aItem );
    }

    PNS_ITEM* Get( int index ) const
    {
        return m_items[index];
    }

    PNS_ITEM* operator[] ( int index ) const
    {
        return m_items[index];
    }

    void Clear()
    {
        BOOST_FOREACH( PNS_ITEM* item, m_items )
        {
            if( item->BelongsTo( this ) )
                delete item;
        }

        m_items.clear();
    }

    bool Contains( const PNS_ITEM* aItem ) const
    {
        return std::find( m_items.begin(), m_items.end(), aItem ) != m_items.end();
    }

    void Erase( PNS_ITEM* aItem )
    {
        ITEMS::iterator f = std::find( m_items.begin(), m_items.end(), aItem );

        if( f != m_items.end() )
            m_items.erase( f );

        if( aItem->BelongsTo( this ) )
            delete aItem;
    }

    template<class T>
    T* FindByKind( PNS_ITEM::PnsKind kind, int index = 0 )
    {
        int n = 0;

        BOOST_FOREACH( PNS_ITEM* item, m_items )
        {
            if( item->OfKind( kind ) )
            {
                if( index == n )
                    return static_cast<T*>( item );
                else
                    n++;
            }
        }

        return NULL;
    }

private:
    void release();

    void copyFrom( const PNS_ITEMSET& aOther )
    {
        release();

        BOOST_FOREACH( PNS_ITEM* item, aOther.m_items )
        {
            if( item->BelongsTo( &aOther ) )
                m_items.push_back( item->Clone() );
            else
                m_items.push_back( item );
        }
    }

    ITEMS m_items;
};

#endif
