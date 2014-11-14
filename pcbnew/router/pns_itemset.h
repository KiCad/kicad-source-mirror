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

#include <vector>
#include <boost/foreach.hpp>

#include "pns_item.h"

/**
 * Class PNS_ITEMSET
 *
 * Holds a list of board items, that can be filtered against net, kinds,
 * layers, etc.
 **/

class PNS_ITEMSET
{
public:
    typedef std::vector<PNS_ITEM*> ITEMS;

    PNS_ITEMSET( PNS_ITEM* aInitialItem = NULL );

    PNS_ITEMSET( const PNS_ITEMSET& aOther )
    {
        m_items = aOther.m_items;
    }

    const PNS_ITEMSET& operator=( const PNS_ITEMSET& aOther )
    {
        m_items = aOther.m_items;
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

    ITEMS& Items() { return m_items; }
    const ITEMS& CItems() const { return m_items; }

    PNS_ITEMSET& FilterLayers( int aStart, int aEnd = -1, bool aInvert = false );
    PNS_ITEMSET& FilterKinds( int aKindMask, bool aInvert = false );
    PNS_ITEMSET& FilterNet( int aNet, bool aInvert = false );

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

    void Add( PNS_ITEM* aItem )
    {
        m_items.push_back( aItem );
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
        m_items.clear();
    }

    bool Contains( const PNS_ITEM* aItem ) const
    {
        return std::find( m_items.begin(), m_items.end(), aItem ) != m_items.end();
    }

    void Erase( const PNS_ITEM* aItem )
    {
        ITEMS::iterator f = std::find( m_items.begin(), m_items.end(), aItem );

        if( f != m_items.end() )
            m_items.erase( f );
    }

private:
    ITEMS m_items;
};

#endif
