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


PNS_ITEMSET::~PNS_ITEMSET()
{
}


void PNS_ITEMSET::Add( const PNS_LINE& aLine )
{
    PNS_LINE* copy = aLine.Clone();
    m_items.push_back( ENTRY( copy, true ) );
}


void PNS_ITEMSET::Prepend( const PNS_LINE& aLine )
{
    PNS_LINE* copy = aLine.Clone();
    m_items.insert( m_items.begin(), ENTRY( copy, true ) );
}


PNS_ITEMSET& PNS_ITEMSET::FilterLayers( int aStart, int aEnd, bool aInvert )
{
    ENTRIES newItems;
    PNS_LAYERSET l;

    if( aEnd < 0 )
        l = PNS_LAYERSET( aStart );
    else
        l = PNS_LAYERSET( aStart, aEnd );

    BOOST_FOREACH( const ENTRY& ent, m_items )
    {
        if( ent.item->Layers().Overlaps( l ) ^ aInvert )
        {
            newItems.push_back( ent );
        }
    }

    m_items = newItems;

    return *this;
}


PNS_ITEMSET& PNS_ITEMSET::FilterKinds( int aKindMask, bool aInvert )
{
    ENTRIES newItems;

    BOOST_FOREACH( const ENTRY& ent, m_items )
    {
        if( ent.item->OfKind( aKindMask ) ^ aInvert )
        {
            newItems.push_back( ent );
        }
    }

    m_items = newItems;

    return *this;
}


PNS_ITEMSET& PNS_ITEMSET::FilterMarker( int aMarker, bool aInvert )
{
    ENTRIES newItems;

    BOOST_FOREACH( const ENTRY& ent, m_items )
    {
        if( ent.item->Marker() & aMarker )
        {
            newItems.push_back( ent );
        }
    }

    m_items = newItems;

    return *this;
}


PNS_ITEMSET& PNS_ITEMSET::FilterNet( int aNet, bool aInvert )
{
    ENTRIES newItems;

    BOOST_FOREACH( const ENTRY& ent, m_items )
    {
        if( ( ent.item->Net() == aNet ) ^ aInvert )
        {
            newItems.push_back( ent );
        }
    }

    m_items = newItems;

    return *this;
}


PNS_ITEMSET& PNS_ITEMSET::ExcludeItem( const PNS_ITEM* aItem )
{
    ENTRIES newItems;

    BOOST_FOREACH( const ENTRY& ent, m_items )
    {
        if( ent.item != aItem )

        newItems.push_back( ent );
    }

    m_items = newItems;

    return *this;
}
