/*
 * KiRouter - a push-and-(sometimes-)shove PCB router
 *
 * Copyright (C) 2013-2014 CERN
 * Copyright (C) 2016-2021 KiCad Developers, see AUTHORS.txt for contributors.
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

#include <vector>
#include <core/kicad_algo.h>
#include "pns_item.h"

namespace PNS {

/**
 * Hold a list of board items, that can be filtered against net, kinds, layers, etc.
 */
class LINE;

class ITEM_SET
{
public:
    struct ENTRY
    {
        ENTRY( ITEM* aItem, bool aOwned = false ) :
            item( aItem ),
            owned( aOwned )
        {}

        ENTRY( const ENTRY& aOther )
        {
            owned = aOther.owned;

            if( aOther.owned )
                item = aOther.item->Clone();
            else
                item = aOther.item;
        }

        ~ENTRY()
        {
            if( owned )
                delete item;
        }

        bool operator==( const ENTRY& b ) const
        {
            return item == b.item;
        }

        bool operator<( const ENTRY& b ) const
        {
            return item < b.item;
        }

        ENTRY& operator=( const ENTRY& aOther )
        {
            owned = aOther.owned;

            if( aOther.owned )
                item = aOther.item->Clone();
            else
                item = aOther.item;

            return *this;
        }

        operator ITEM* () const
        {
            return item;
        }

        ITEM *item;
        bool owned;
    };

    typedef std::vector<ENTRY> ENTRIES;

    ITEM_SET( ITEM* aInitialItem = nullptr, bool aBecomeOwner = false )
    {
        if( aInitialItem )
            m_items.emplace_back( ENTRY( aInitialItem, aBecomeOwner ) );
    }

    ITEM_SET( const ITEM_SET& aOther )
    {
        m_items = aOther.m_items;
    }

    ~ITEM_SET();

    ITEM_SET& operator=( const ITEM_SET& aOther )
    {
        m_items = aOther.m_items;
        return *this;
    }

    int Count( int aKindMask = -1 ) const
    {
        int n = 0;

        if( aKindMask == -1 || aKindMask == ITEM::ANY_T )
            return static_cast<int>( m_items.size() );

        for( ITEM* item : m_items )
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

    ENTRIES& Items() { return m_items; }
    const ENTRIES& CItems() const { return m_items; }

    ITEM_SET& FilterLayers( int aStart, int aEnd = -1, bool aInvert = false );
    ITEM_SET& FilterKinds( int aKindMask, bool aInvert = false );
    ITEM_SET& FilterNet( int aNet, bool aInvert = false );
    ITEM_SET& FilterMarker( int aMarker, bool aInvert = false );

    ITEM_SET& ExcludeLayers( int aStart, int aEnd = -1 )
    {
        return FilterLayers( aStart, aEnd, true );
    }

    ITEM_SET& ExcludeKinds( int aKindMask )
    {
        return FilterKinds( aKindMask, true );
    }

    ITEM_SET& ExcludeNet( int aNet )
    {
        return FilterNet( aNet, true );
    }

    ITEM_SET& ExcludeItem( const ITEM* aItem );

    int Size() const
    {
        return static_cast<int>( m_items.size() );
    }

    void Add( const LINE& aLine );
    void Prepend( const LINE& aLine );

    ITEM* operator[]( size_t aIndex ) const
    {
        return m_items[aIndex].item;
    }

    ENTRIES::iterator begin() { return m_items.begin(); }
    ENTRIES::iterator end() { return m_items.end(); }
    ENTRIES::const_iterator cbegin() const { return m_items.cbegin(); }
    ENTRIES::const_iterator cend() const { return m_items.cend(); }

    void Add( ITEM* aItem, bool aBecomeOwner = false )
    {
        m_items.emplace_back( ENTRY( aItem, aBecomeOwner ) );
    }

    void Prepend( ITEM* aItem, bool aBecomeOwner = false )
    {
         m_items.emplace( m_items.begin(), ENTRY( aItem, aBecomeOwner ) );
    }

    void Clear()
    {
        m_items.clear();
    }

    bool Contains( ITEM* aItem ) const
    {
        const ENTRY ent( aItem );
        return alg::contains( m_items, ent );
    }

    void Erase( ITEM* aItem )
    {
        ENTRY ent( aItem );
        ENTRIES::iterator f = std::find( m_items.begin(), m_items.end(), ent );

        if( f != m_items.end() )
            m_items.erase( f );
    }

    template<class T>
    T* FindByKind( ITEM::PnsKind kind, int index = 0 )
    {
        int n = 0;

        for( const ITEM* item : m_items )
        {
            if( item->OfKind( kind ) )
            {
                if( index == n )
                    return static_cast<T*>( item );
                else
                    n++;
            }
        }

        return nullptr;
    }

private:
    ENTRIES m_items;
};

}

#endif
