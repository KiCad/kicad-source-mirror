/*
 * This program source code file is part of KICAD, a free EDA CAD application.
 *
 * Copyright (C) 2013-2017 CERN
 * Copyright (C) 2019-2021 KiCad Developers, see AUTHORS.txt for contributors.
 *
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

#include <board.h>
#include <pad.h>
#include <footprint.h>
#include <zone.h>

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
class ZONE;
class PROGRESS_REPORTER;

class CN_EDGE
{
public:
    CN_EDGE()
            : m_weight( 0 ), m_visible( true )
    {}

    CN_EDGE( CN_ANCHOR_PTR aSource, CN_ANCHOR_PTR aTarget, unsigned aWeight = 0 )
            : m_source( aSource ), m_target( aTarget ), m_weight( aWeight ), m_visible( true )
    {}

    /**
     * This sort operator provides a sort-by-weight for the ratsnest operation.
     *
     * @param aOther the other edge to compare.
     * @return true if our weight is smaller than the other weight.
     */
    bool operator<( CN_EDGE aOther ) const
    {
        return m_weight < aOther.m_weight;
    }

    CN_ANCHOR_PTR GetSourceNode() const { return m_source; }
    CN_ANCHOR_PTR GetTargetNode() const { return m_target; }
    unsigned GetWeight() const { return m_weight; }

    void SetSourceNode( const CN_ANCHOR_PTR& aNode ) { m_source = aNode; }
    void SetTargetNode( const CN_ANCHOR_PTR& aNode ) { m_target = aNode; }
    void SetWeight( unsigned weight ) { m_weight = weight; }

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
    unsigned m_weight;
    bool m_visible;
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

    CN_CONNECTIVITY_ALGO() {}
    ~CN_CONNECTIVITY_ALGO() { Clear(); }

    bool ItemExists( const BOARD_CONNECTED_ITEM* aItem ) const
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

    void GetDirtyClusters( CLUSTERS& aClusters ) const
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

    void Build( BOARD* aBoard, PROGRESS_REPORTER* aReporter = nullptr );
    void Build( const std::vector<BOARD_ITEM*>& aItems );

    void Clear();

    bool Remove( BOARD_ITEM* aItem );
    bool Add( BOARD_ITEM* aItem );

    const CLUSTERS SearchClusters( CLUSTER_SEARCH_MODE aMode, const KICAD_T aTypes[],
                                   int aSingleNet );
    const CLUSTERS SearchClusters( CLUSTER_SEARCH_MODE aMode );

    /**
     * Propagate nets from pads to other items in clusters.
     * @param aCommit is used to store undo information for items modified by the call.
     * @param aMode controls how clusters with conflicting nets are resolved.
     */
    void PropagateNets( BOARD_COMMIT* aCommit = nullptr,
                        PROPAGATE_MODE aMode = PROPAGATE_MODE::SKIP_CONFLICTS );

    void FindIsolatedCopperIslands( ZONE* aZone, PCB_LAYER_ID aLayer, std::vector<int>& aIslands );

    /**
     * Find the copper islands that are not connected to a net.
     *
     * These are added to the m_islands vector.
     * N.B. This must be called after aZones has been refreshed.
     *
     * @param: aZones is the set of zones to search for islands.
     */
    void FindIsolatedCopperIslands( std::vector<CN_ZONE_ISOLATED_ISLAND_LIST>& aZones );

    const CLUSTERS& GetClusters();

    const CN_LIST& ItemList() const
    {
        return m_itemList;
    }

    template <typename Func>
    void ForEachAnchor( Func&& aFunc ) const
    {
        for( auto&& item : m_itemList )
        {
            for( auto&& anchor : item->Anchors() )
                aFunc( *anchor );
        }
    }

    template <typename Func>
    void ForEachItem( Func&& aFunc ) const
    {
        for( auto&& item : m_itemList )
            aFunc( *item );
    }

    void MarkNetAsDirty( int aNet );
    void SetProgressReporter( PROGRESS_REPORTER* aReporter );

private:
    void searchConnections();

    void propagateConnections( BOARD_COMMIT* aCommit = nullptr,
                               PROPAGATE_MODE aMode = PROPAGATE_MODE::SKIP_CONFLICTS );

    template <class Container, class BItem>
    void add( Container& c, BItem brditem )
    {
        auto item = c.Add( brditem );

        m_itemMap[ brditem ] = ITEM_MAP_ENTRY( item );
    }

    void markItemNetAsDirty( const BOARD_ITEM* aItem );

    CN_LIST m_itemList;

    std::unordered_map<const BOARD_ITEM*, ITEM_MAP_ENTRY> m_itemMap;

    CLUSTERS m_connClusters;
    CLUSTERS m_ratsnestClusters;
    std::vector<bool> m_dirtyNets;
    PROGRESS_REPORTER* m_progressReporter = nullptr;

};

class CN_VISITOR
{
public:
    CN_VISITOR( CN_ITEM* aItem ) :
        m_item( aItem )
    {}

    bool operator()( CN_ITEM* aCandidate );

protected:
    void checkZoneItemConnection( CN_ZONE_LAYER* aZoneLayer, CN_ITEM* aItem );

    void checkZoneZoneConnection( CN_ZONE_LAYER* aZoneLayerA, CN_ZONE_LAYER* aZoneLayerB );

    ///< The item we are looking for connections to.
    CN_ITEM* m_item;
};

#endif
