/*
 * This program source code file is part of KICAD, a free EDA CAD application.
 *
 * Copyright (C) 2013-2017 CERN
 * @author Maciej Suminski <maciej.suminski@cern.ch>
 * @author Tomasz Wlostowski <tomasz.wlostowski@cern.ch>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, you may find one here:
 * http://www.gnu.org/licenses/old-licenses/gpl-2.0.html
 * or you may search the http://www.gnu.org website for the version 2 license,
 * or you may write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
 */

// #define CONNECTIVITY_DEBUG

#ifndef __CONNECTIVITY_ALGO_H
#define __CONNECTIVITY_ALGO_H

#include <class_board.h>
#include <class_pad.h>
#include <class_module.h>
#include <class_zone.h>

#include <geometry/shape_poly_set.h>
#include <geometry/poly_grid_partition.h>

#include <memory>
#include <algorithm>
#include <functional>
#include <vector>
#include <deque>
#include <intrusive_list.h>

#include <connectivity/connectivity_rtree.h>
#include <connectivity/connectivity_data.h>
#include <connectivity/connectivity_items.h>

class CN_CONNECTIVITY_ALGO_IMPL;
class CN_RATSNEST_NODES;
class BOARD;
class BOARD_CONNECTED_ITEM;
class BOARD_ITEM;
class ZONE_CONTAINER;
class PROGRESS_REPORTER;

class CN_EDGE
{
public:
    CN_EDGE() {};
    CN_EDGE( CN_ANCHOR_PTR aSource, CN_ANCHOR_PTR aTarget, int aWeight = 0 ) :
        m_source( aSource ),
        m_target( aTarget ),
        m_weight( aWeight ) {}

    CN_ANCHOR_PTR GetSourceNode() const { return m_source; }
    CN_ANCHOR_PTR GetTargetNode() const { return m_target; }
    int GetWeight() const { return m_weight; }

    void SetSourceNode( const CN_ANCHOR_PTR& aNode ) { m_source = aNode; }
    void SetTargetNode( const CN_ANCHOR_PTR& aNode ) { m_target = aNode; }
    void SetWeight( unsigned int weight ) { m_weight = weight; }

    void SetVisible( bool aVisible )
    {
        m_visible = aVisible;
    }

    bool IsVisible() const
    {
        return m_visible;
    }

    const VECTOR2I GetSourcePos() const
    {
        return m_source->Pos();
    }

    const VECTOR2I GetTargetPos() const
    {
        return m_target->Pos();
    }

private:
    CN_ANCHOR_PTR m_source;
    CN_ANCHOR_PTR m_target;
    unsigned int m_weight = 0;
    bool m_visible = true;
};

class CN_CONNECTIVITY_ALGO
{
public:
    enum CLUSTER_SEARCH_MODE
    {
        CSM_PROPAGATE,
        CSM_CONNECTIVITY_CHECK,
        CSM_RATSNEST
    };

    using CLUSTERS = std::vector<CN_CLUSTER_PTR>;

private:

    class ITEM_MAP_ENTRY
    {
    public:
        ITEM_MAP_ENTRY( CN_ITEM* aItem = nullptr )
        {
            if( aItem )
                m_items.push_back( aItem );
        }

        void MarkItemsAsInvalid()
        {
            for( auto item : m_items )
            {
                item->SetValid( false );
            }
        }

        void Link( CN_ITEM* aItem )
        {
            m_items.push_back( aItem );
        }

        const std::list<CN_ITEM*> GetItems() const
        {
            return m_items;
        }

        std::list<CN_ITEM*> m_items;
    };

    CN_LIST m_itemList;

    std::unordered_map<const BOARD_CONNECTED_ITEM*, ITEM_MAP_ENTRY> m_itemMap;

    CLUSTERS m_connClusters;
    CLUSTERS m_ratsnestClusters;
    std::vector<bool> m_dirtyNets;
    PROGRESS_REPORTER* m_progressReporter = nullptr;

    void    searchConnections();

    void    update();

    void    propagateConnections( BOARD_COMMIT* aCommit = nullptr );

    template <class Container, class BItem>
    void add( Container& c, BItem brditem )
    {
        auto item = c.Add( brditem );

        m_itemMap[ brditem ] = ITEM_MAP_ENTRY( item );
    }

    void markItemNetAsDirty( const BOARD_ITEM* aItem );

public:

    CN_CONNECTIVITY_ALGO() {}
    ~CN_CONNECTIVITY_ALGO() { Clear(); }

    bool ItemExists( const BOARD_CONNECTED_ITEM* aItem )
    {
        return m_itemMap.find( aItem ) != m_itemMap.end();
    }

    ITEM_MAP_ENTRY& ItemEntry( const BOARD_CONNECTED_ITEM* aItem )
    {
        return m_itemMap[ aItem ];
    }

    bool IsNetDirty( int aNet ) const
    {
        if( aNet < 0 )
            return false;

        return m_dirtyNets[ aNet ];
    }

    void ClearDirtyFlags()
    {
        for( auto i = m_dirtyNets.begin(); i != m_dirtyNets.end(); ++i )
            *i = false;
    }

    void GetDirtyClusters( CLUSTERS& aClusters )
    {
        for( const auto& cl : m_ratsnestClusters )
        {
            int net = cl->OriginNet();

            if( net >= 0 && m_dirtyNets[net] )
                aClusters.push_back( cl );
        }
    }

    int NetCount() const
    {
        return m_dirtyNets.size();
    }

    void    Build( BOARD* aBoard );
    void    Build( const std::vector<BOARD_ITEM*>& aItems );

    void Clear();

    bool    Remove( BOARD_ITEM* aItem );
    bool    Add( BOARD_ITEM* aItem );

    const CLUSTERS  SearchClusters( CLUSTER_SEARCH_MODE aMode, const KICAD_T aTypes[], int aSingleNet );
    const CLUSTERS  SearchClusters( CLUSTER_SEARCH_MODE aMode );

    /**
     * Propagates nets from pads to other items in clusters
     * @param aCommit is used to store undo information for items modified by the call
     */
    void    PropagateNets( BOARD_COMMIT* aCommit = nullptr );

    void    FindIsolatedCopperIslands( ZONE_CONTAINER* aZone, std::vector<int>& aIslands );

    /**
     * Finds the copper islands that are not connected to a net.  These are added to
     * the m_islands vector.
     * N.B. This must be called after aZones has been refreshed.
     * @param: aZones The set of zones to search for islands
     */
    void    FindIsolatedCopperIslands( std::vector<CN_ZONE_ISOLATED_ISLAND_LIST>& aZones );

    bool    CheckConnectivity( std::vector<CN_DISJOINT_NET_ENTRY>& aReport );

    const CLUSTERS& GetClusters();
    int             GetUnconnectedCount();

    CN_LIST& ItemList() { return m_itemList; }

    void ForEachAnchor( const std::function<void( CN_ANCHOR& )>& aFunc );
    void ForEachItem( const std::function<void( CN_ITEM& )>& aFunc );

    void MarkNetAsDirty( int aNet );
    void SetProgressReporter( PROGRESS_REPORTER* aReporter );

};

/**
 * Struct CN_VISTOR
 **/
class CN_VISITOR {

public:

    CN_VISITOR( CN_ITEM* aItem ) :
        m_item( aItem )
    {}

    bool operator()( CN_ITEM* aCandidate );

protected:

    void checkZoneItemConnection( CN_ZONE* aZone, CN_ITEM* aItem );

    void checkZoneZoneConnection( CN_ZONE* aZoneA, CN_ZONE* aZoneB );

    ///> the item we are looking for connections to
    CN_ITEM* m_item;
};

#endif
