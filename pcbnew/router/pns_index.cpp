/*
 * KiRouter - a push-and-(sometimes-)shove PCB router
 *
 * Copyright (C) 2013-2014 CERN
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
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

#include "pns_index.h"
#include "pns_router.h"

namespace PNS {


void INDEX::Add( ITEM* aItem )
{
    const PNS_LAYER_RANGE& range = aItem->Layers();
    assert( range.Start() != -1 && range.End() != -1 );

    if( m_subIndices.size() <= static_cast<size_t>( range.End() ) )
    {
        for( int i = 0; i <= range.End(); ++i )
            m_subIndices.emplace_back( std::make_unique<ITEM_SHAPE_INDEX>( i ) );
    }

    for( int i = range.Start(); i <= range.End(); ++i )
        m_subIndices[i]->Add( aItem );

    m_allItems.insert( aItem );
    NET_HANDLE net = aItem->Net();

    if( net )
        m_netMap[net].push_back( aItem );
}


void INDEX::Remove( ITEM* aItem )
{
    const PNS_LAYER_RANGE& range = aItem->Layers();
    assert( range.Start() != -1 && range.End() != -1 );

    if( m_subIndices.size() <= static_cast<size_t>( range.End() ) )
        return;

    for( int i = range.Start(); i <= range.End(); ++i )
        m_subIndices[i]->Remove( aItem );

    m_allItems.erase( aItem );
    NET_HANDLE net = aItem->Net();

    if( net && m_netMap.find( net ) != m_netMap.end() )
        m_netMap[net].remove( aItem );
}


void INDEX::Replace( ITEM* aOldItem, ITEM* aNewItem )
{
    Remove( aOldItem );
    Add( aNewItem );
}


INDEX::NET_ITEMS_LIST* INDEX::GetItemsForNet( NET_HANDLE aNet )
{
    if( m_netMap.find( aNet ) == m_netMap.end() )
        return nullptr;

    return &m_netMap[aNet];
}

};
