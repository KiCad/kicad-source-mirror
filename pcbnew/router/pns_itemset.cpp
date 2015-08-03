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

#include <boost/foreach.hpp>

#include "pns_itemset.h"
#include "pns_line.h"


void PNS_ITEMSET::release()
{
    BOOST_FOREACH( PNS_ITEM* item, m_items )
    {
        if( item->BelongsTo( this ) )
            delete item;
    }

    m_items.clear();
}


PNS_ITEMSET::~PNS_ITEMSET()
{
    release();
}


void PNS_ITEMSET::Add( const PNS_LINE& aLine )
{
    PNS_LINE* copy = aLine.Clone();
    copy->SetOwner( this );
    m_items.push_back( copy );
}


void PNS_ITEMSET::Prepend( const PNS_LINE& aLine )
{
    PNS_LINE* copy = aLine.Clone();
    copy->SetOwner( this );
    m_items.push_front( copy );
}


PNS_ITEMSET& PNS_ITEMSET::FilterLayers( int aStart, int aEnd, bool aInvert )
{
    ITEMS newItems;
    PNS_LAYERSET l;

    if( aEnd < 0 )
        l = PNS_LAYERSET( aStart );
    else
        l = PNS_LAYERSET( aStart, aEnd );

    BOOST_FOREACH( PNS_ITEM* item, m_items )

    if( item->Layers().Overlaps( l ) ^ aInvert )
        newItems.push_back( item );

    m_items = newItems;

    return *this;
}


PNS_ITEMSET& PNS_ITEMSET::FilterKinds( int aKindMask, bool aInvert )
{
    ITEMS newItems;

    BOOST_FOREACH( PNS_ITEM* item, m_items )
    {
        if( item->OfKind( aKindMask ) ^ aInvert )
            newItems.push_back( item );
    }

    m_items = newItems;

    return *this;
}


PNS_ITEMSET& PNS_ITEMSET::FilterMarker( int aMarker, bool aInvert )
{
    ITEMS newItems;

    BOOST_FOREACH( PNS_ITEM* item, m_items )
    {
        if( item->Marker() & aMarker )
            newItems.push_back( item );
    }

    m_items = newItems;

    return *this;
}


PNS_ITEMSET& PNS_ITEMSET::FilterNet( int aNet, bool aInvert )
{
    ITEMS newItems;

    BOOST_FOREACH( PNS_ITEM* item, m_items )
    {
        if( ( item->Net() == aNet ) ^ aInvert )
            newItems.push_back( item );
    }

    m_items = newItems;

    return *this;
}


PNS_ITEMSET& PNS_ITEMSET::ExcludeItem( const PNS_ITEM* aItem )
{
    ITEMS newItems;

    BOOST_FOREACH( PNS_ITEM* item, m_items )
    {
        if( item != aItem )
            newItems.push_back( item );
    }

    m_items = newItems;

    return *this;
}
