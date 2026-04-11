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
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#ifndef __PNS_INDEX_H
#define __PNS_INDEX_H

#include <deque>
#include <list>
#include <map>
#include <unordered_set>

#include <layer_ids.h>
#include <geometry/shape_index.h>

#include "pns_item.h"
#include "pns_node.h"

namespace PNS {


/**
 * INDEX
 *
 * Custom spatial index, holding our board items and allowing for very fast searches. Items
 * are assigned to separate R-Tree subindices depending on their type and spanned layers, reducing
 * overlap and improving search time.
 **/
class INDEX
{
public:
    typedef std::list<ITEM*>            NET_ITEMS_LIST;
    typedef SHAPE_INDEX<ITEM*>          ITEM_SHAPE_INDEX;
    typedef std::unordered_set<ITEM*>   ITEM_SET;

    INDEX() : m_deferred( false ) {};

    /**
     * When deferred, Add() registers items in metadata but skips spatial index
     * insertion. Call BuildSpatialIndex() to bulk load all items at once.
     */
    void SetDeferred( bool aDeferred );

    /**
     * Bulk load the spatial sub-indices from all registered items.
     * Call after SetDeferred(false) to populate the R-trees.
     */
    void BuildSpatialIndex();

    /**
     * Create a copy-on-write clone of this index. The spatial tree structure is
     * shared (O(1)) while metadata (net map, item set) is copied.
     */
    std::unique_ptr<INDEX> Clone() const
    {
        auto clone = std::make_unique<INDEX>();

        for( const auto& si : m_subIndices )
            clone->m_subIndices.emplace_back( std::make_unique<ITEM_SHAPE_INDEX>( si->Clone() ) );

        clone->m_netMap = m_netMap;
        clone->m_allItems = m_allItems;
        return clone;
    }

    /**
     * Adds item to the spatial index.
     */
    void Add( ITEM* aItem );

    /**
     * Removes an item from the spatial index.
     */
    void Remove( ITEM* aItem );

    /**
     * Replaces one item with another.
     */
    void Replace( ITEM* aOldItem, ITEM* aNewItem );

    /**
     * Searches items in the index that are in proximity of aItem.
     * For each item, function object aVisitor is called. Only items on
     * overlapping layers are considered.
     *
     * @param aItem item to search against
     * @param aMinDistance proximity distance (wrs to the item's shape)
     * @param aVisitor function object called on each found item. Return
              false from the visitor to stop searching.
     * @return number of items found.
     */
    template<class Visitor>
    int Query( const ITEM* aItem, int aMinDistance, Visitor& aVisitor ) const;

    /**
     * Searches items in the index that are in proximity of aShape.
     * For each item, function object aVisitor is called. Treats all
     * layers as colliding.
     *
     * @param aShape shape to search against
     * @param aMinDistance proximity distance (wrs to the item's shape)
     * @param aVisitor function object called on each found item. Return
              false from the visitor to stop searching.
     * @return number of items found.
     */
    template<class Visitor>
    int Query( const SHAPE* aShape, int aMinDistance, Visitor& aVisitor ) const;

    /**
     * Returns list of all items in a given net.
     */
    NET_ITEMS_LIST* GetItemsForNet( NET_HANDLE aNet );

    /**
     * Function Contains()
     *
     * Returns true if item aItem exists in the index.
     */
    bool Contains( ITEM* aItem ) const
    {
        return m_allItems.find( aItem ) != m_allItems.end();
    }

    /**
     * Returns number of items stored in the index.
     */
    int Size() const { return m_allItems.size(); }

    ITEM_SET::iterator begin() { return m_allItems.begin(); }
    ITEM_SET::iterator end() { return m_allItems.end(); }

private:
    template <class Visitor>
    int querySingle( std::size_t aIndex, const SHAPE* aShape, int aMinDistance, Visitor& aVisitor ) const;

private:
    std::deque<std::unique_ptr<ITEM_SHAPE_INDEX>> m_subIndices;
    std::map<NET_HANDLE, NET_ITEMS_LIST> m_netMap;
    ITEM_SET                             m_allItems;
    bool                                 m_deferred;
};


template<class Visitor>
int INDEX::querySingle( std::size_t aIndex, const SHAPE* aShape, int aMinDistance, Visitor& aVisitor ) const
{
    if( aIndex >= m_subIndices.size() )
        return 0;

    LAYER_CONTEXT_SETTER layerContext( aVisitor, aIndex );
    return m_subIndices[aIndex]->Query( aShape, aMinDistance, aVisitor);
}

template<class Visitor>
int INDEX::Query( const ITEM* aItem, int aMinDistance, Visitor& aVisitor ) const
{
    int total = 0;

    wxCHECK( aItem->Kind() != ITEM::INVALID_T, 0 );

    const PNS_LAYER_RANGE& layers = aItem->Layers();

    for( int i = layers.Start(); i <= layers.End(); ++i )
    {
        const SHAPE* shape = aItem->Shape( i );

        if( shape )
            total += querySingle( i, shape, aMinDistance, aVisitor );
    }

    return total;
}

template<class Visitor>
int INDEX::Query( const SHAPE* aShape, int aMinDistance, Visitor& aVisitor ) const
{
    int total = 0;

    for( std::size_t i = 0; i < m_subIndices.size(); ++i )
        total += querySingle( i, aShape, aMinDistance, aVisitor );

    return total;
}

};

#endif
