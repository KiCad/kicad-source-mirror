/*
 * KiRouter - a push-and-(sometimes-)shove PCB router
 *
 * Copyright (C) 2013  CERN
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
 * with this program.  If not, see <http://www.gnu.or/licenses/>.
 */

#include <boost/foreach.hpp>

#include "pns_itemset.h"


PNS_ITEMSET::PNS_ITEMSET()
{
}


PNS_ITEMSET::~PNS_ITEMSET()
{
}


PNS_ITEMSET& PNS_ITEMSET::FilterLayers( int aStart, int aEnd )
{
    ItemVector newItems;
    PNS_LAYERSET l;

    if( aEnd < 0 )
        l = PNS_LAYERSET( aStart );
    else
        l = PNS_LAYERSET( aStart, aEnd );

    BOOST_FOREACH( PNS_ITEM * item, m_items )

    if( item->GetLayers().Overlaps( l ) )
        newItems.push_back( item );

    m_items = newItems;
    return *this;
}


PNS_ITEMSET& PNS_ITEMSET::FilterKinds( int aKindMask )
{
    ItemVector newItems;

    BOOST_FOREACH( PNS_ITEM * item, m_items )

    if( item->GetKind() & aKindMask )
        newItems.push_back( item );

    m_items = newItems;
    return *this;
}


PNS_ITEMSET& PNS_ITEMSET::FilterNet( int aNet )
{
    ItemVector newItems;

    BOOST_FOREACH( PNS_ITEM * item, m_items )

    if( item->GetNet() == aNet )
        newItems.push_back( item );

    m_items = newItems;
    return *this;
}
