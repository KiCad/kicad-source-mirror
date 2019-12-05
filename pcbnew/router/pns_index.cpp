/*
 * KiRouter - a push-and-(sometimes-)shove PCB router
 *
 * Copyright (C) 2013-2014 CERN
 * Copyright (C) 2016 KiCad Developers, see AUTHORS.txt for contributors.
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

namespace PNS {

INDEX::INDEX()
{
    memset( m_subIndices, 0, sizeof( m_subIndices ) );
}


INDEX::~INDEX()
{
    Clear();
}


INDEX::ITEM_SHAPE_INDEX* INDEX::getSubindex( const ITEM* aItem )
{
    int idx_n = -1;

    const LAYER_RANGE& l = aItem->Layers();

    switch( aItem->Kind() )
    {
    case ITEM::VIA_T:
        idx_n = SI_Multilayer;
        break;

    case ITEM::SOLID_T:
        {
            if( l.IsMultilayer() )
                idx_n = SI_Multilayer;
            else if( l.Start() == B_Cu ) // fixme: use kicad layer codes
                idx_n = SI_PadsTop;
            else if( l.Start() == F_Cu )
                idx_n = SI_PadsBottom;
            else
                idx_n = SI_Traces + 2 * l.Start() + SI_SegStraight;
        }
        break;

    case ITEM::SEGMENT_T:
    case ITEM::LINE_T:
        idx_n = SI_Traces + 2 * l.Start() + SI_SegStraight;
        break;

    default:
        break;
    }

    if( idx_n < 0 || idx_n >= MaxSubIndices )
    {
        wxASSERT( idx_n >= 0 );
        wxASSERT( idx_n < MaxSubIndices );
        return nullptr;
    }

    if( !m_subIndices[idx_n] )
        m_subIndices[idx_n] = new ITEM_SHAPE_INDEX;

    return m_subIndices[idx_n];
}

void INDEX::Add( ITEM* aItem )
{
    ITEM_SHAPE_INDEX* idx = getSubindex( aItem );

    if( !idx )
        return;

    idx->Add( aItem );
    m_allItems.insert( aItem );
    int net = aItem->Net();

    if( net >= 0 )
    {
        m_netMap[net].push_back( aItem );
    }
}

void INDEX::Remove( ITEM* aItem )
{
    ITEM_SHAPE_INDEX* idx = getSubindex( aItem );

    if( !idx )
        return;

    idx->Remove( aItem );
    m_allItems.erase( aItem );
    int net = aItem->Net();

    if( net >= 0 && m_netMap.find( net ) != m_netMap.end() )
        m_netMap[net].remove( aItem );
}

void INDEX::Replace( ITEM* aOldItem, ITEM* aNewItem )
{
    Remove( aOldItem );
    Add( aNewItem );
}


void INDEX::Clear()
{
    for( int i = 0; i < MaxSubIndices; ++i )
    {
        ITEM_SHAPE_INDEX* idx = m_subIndices[i];

        if( idx )
            delete idx;

        m_subIndices[i] = NULL;
    }
}


INDEX::NET_ITEMS_LIST* INDEX::GetItemsForNet( int aNet )
{
    if( m_netMap.find( aNet ) == m_netMap.end() )
        return NULL;

    return &m_netMap[aNet];
}

};
