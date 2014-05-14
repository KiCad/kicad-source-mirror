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
    typedef std::vector<PNS_ITEM*> ItemVector;

    PNS_ITEMSET( PNS_ITEM *aInitialItem = NULL );

    PNS_ITEMSET (const PNS_ITEMSET &aOther )
    {
        m_items = aOther.m_items;
        m_ownedItems = ItemVector();
    }
    
    const PNS_ITEMSET& operator= (const PNS_ITEMSET &aOther)
    {
        m_items = aOther.m_items;
        m_ownedItems = ItemVector();
        return *this;
    }

    ~PNS_ITEMSET();

    ItemVector& Items() { return m_items; }
    const ItemVector& CItems() const { return m_items; } 

    PNS_ITEMSET& FilterLayers( int aStart, int aEnd = -1 );
    PNS_ITEMSET& FilterKinds( int aKindMask );
    PNS_ITEMSET& FilterNet( int aNet );

    int Size() { return m_items.size(); }

    void Add( PNS_ITEM* item )
    {
        m_items.push_back( item );
    }

    PNS_ITEM* Get( int index ) const { return m_items[index]; }

    void Clear();

    void AddOwned ( PNS_ITEM *aItem )
    {
        m_items.push_back( aItem );
        m_ownedItems.push_back( aItem );
    }
    
private:
    ItemVector m_items;
    ItemVector m_ownedItems;
};

#endif
