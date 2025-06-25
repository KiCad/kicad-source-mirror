/*
 * This program source code file is part of KICAD, a free EDA CAD application.
 *
 * Copyright (C) 2013-2017 CERN
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
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

#include <memory>
#include <algorithm>
#include <functional>
#include <vector>
#include <deque>

#include <connectivity/connectivity_rtree.h>
#include <connectivity/connectivity_data.h>
#include <connectivity/connectivity_items.h>

class CN_RATSNEST_NODES;
class BOARD;
class BOARD_CONNECTED_ITEM;
class BOARD_ITEM;
class ZONE;
class PROGRESS_REPORTER;


/**
 * CN_EDGE represents a point-to-point connection, whether realized or unrealized (ie: tracks etc.
 * or a ratsnest line).
 */
class CN_EDGE
{
public:
    CN_EDGE() :
            m_weight( 0 ),
            m_visible( true )
    {}

    CN_EDGE( const std::shared_ptr<CN_ANCHOR>& aSource, const std::shared_ptr<CN_ANCHOR>& aTarget,
             unsigned aWeight = 0 ) :
            m_source( aSource ),
            m_target( aTarget ),
            m_weight( aWeight ),
            m_visible( true )
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

    /**
     * Comparison operator for std::stable_sort.
     *
     * @param aOther the other edge to compare.
     * @return true if this edge should come before aOther in the sorted order.
     *
     * Comparison order:
     * 1. Compare source nodes by position (x, then y)
     * 2. Then compare by weight
     * 3. Then by visibility
     * 4. If everything is equal, return false for stable ordering
     */
    bool StableSortCompare( const CN_EDGE& aOther ) const
    {
        const VECTOR2I& thisPos = GetSourcePos();
        const VECTOR2I& otherPos = aOther.GetSourcePos();

        // First compare by source node position
        if( thisPos.x != otherPos.x )
            return thisPos.x < otherPos.x;

        if( thisPos.y != otherPos.y )
            return thisPos.y < otherPos.y;

        // Then compare by weight
        if( m_weight != aOther.m_weight )
            return m_weight < aOther.m_weight;

        // Then by visibility
        if( m_visible != aOther.m_visible )
            return m_visible && !aOther.m_visible;

        // If everything is equal, return false for stable ordering
        return false;
    }

    std::shared_ptr<const CN_ANCHOR> GetSourceNode() const { return m_source; }
    std::shared_ptr<const CN_ANCHOR> GetTargetNode() const { return m_target; }

    void SetSourceNode( const std::shared_ptr<const CN_ANCHOR>& aNode ) { m_source = aNode; }
    void SetTargetNode( const std::shared_ptr<const CN_ANCHOR>& aNode ) { m_target = aNode; }

    void RemoveInvalidRefs()
    {
        if( m_source && !m_source->Valid() )
            m_source.reset();

        if( m_target && !m_target->Valid() )
            m_target.reset();
    }

    void SetWeight( unsigned weight ) { m_weight = weight; }
    unsigned GetWeight() const { return m_weight; }

    void SetVisible( bool aVisible ) { m_visible = aVisible; }
    bool IsVisible() const { return m_visible; }

    const VECTOR2I GetSourcePos() const { return m_source->Pos(); }
    const VECTOR2I GetTargetPos() const { return m_target->Pos(); }
    unsigned GetLength() const
    {
        return ( m_target->Pos() - m_source->Pos() ).EuclideanNorm();
    }

private:
    std::shared_ptr<const CN_ANCHOR> m_source;
    std::shared_ptr<const CN_ANCHOR> m_target;
    unsigned                         m_weight;
    bool                             m_visible;
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

    using CLUSTERS = std::vector<std::shared_ptr<CN_CLUSTER>>;

    /*
     * Holds a list of CN_ITEMs for a given BOARD_CONNECTED_ITEM.  For most items (pads, tracks,
     * etc.) the list will have a single CN_ITEM, but for ZONEs it will have one item for each
     * distinct outline on each layer.
     */
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
            for( CN_ITEM* item : m_items )
                item->SetValid( false );
        }

        void Link( CN_ITEM* aItem )
        {
            m_items.push_back( aItem );
        }

        const std::list<CN_ITEM*>& GetItems() const
        {
            return m_items;
        }

        std::list<CN_ITEM*> m_items;
    };

    CN_CONNECTIVITY_ALGO( CONNECTIVITY_DATA* aParentConnectivityData ) :
            m_parentConnectivityData( aParentConnectivityData ),
            m_isLocal( false )
    {}

    ~CN_CONNECTIVITY_ALGO()
    {
        Clear();
    }

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
        for( size_t ii = 0; ii < m_dirtyNets.size(); ii++ )
            m_dirtyNets[ii] = false;
    }

    void GetDirtyClusters( CLUSTERS& aClusters ) const
    {
        for( const std::shared_ptr<CN_CLUSTER>& cl : m_ratsnestClusters )
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
    void LocalBuild( const std::shared_ptr<CONNECTIVITY_DATA>& aGlobalConnectivity,
                     const std::vector<BOARD_ITEM*>& aLocalItems );

    void Clear();

    bool Remove( BOARD_ITEM* aItem );
    bool Add( BOARD_ITEM* aItem );

    const CLUSTERS SearchClusters( CLUSTER_SEARCH_MODE aMode, bool aExcludeZones, int aSingleNet );
    const CLUSTERS SearchClusters( CLUSTER_SEARCH_MODE aMode );

    /**
     * Propagate nets from pads to other items in clusters.
     * @param aCommit is used to store undo information for items modified by the call.
     */
    void PropagateNets( BOARD_COMMIT* aCommit = nullptr );

    /**
     * Fill in the isolated islands map with copper islands that are not connected to a net.
     */
    void FillIsolatedIslandsMap( std::map<ZONE*, std::map<PCB_LAYER_ID, ISOLATED_ISLANDS>>& aMap,
                                 bool aConnectivityAlreadyRebuilt );

    const CLUSTERS& GetClusters();

    const CN_LIST& ItemList() const
    {
        return m_itemList;
    }

    template <typename Func>
    void ForEachAnchor( Func&& aFunc ) const
    {
        for( CN_ITEM* item : m_itemList )
        {
            for( std::shared_ptr<CN_ANCHOR>& anchor : item->Anchors() )
                aFunc( *anchor );
        }
    }

    template <typename Func>
    void ForEachItem( Func&& aFunc ) const
    {
        for( CN_ITEM* item : m_itemList )
            aFunc( *item );
    }

    void MarkNetAsDirty( int aNet );
    void RemoveInvalidRefs();

    void SetProgressReporter( PROGRESS_REPORTER* aReporter );

private:
    void searchConnections();

    void propagateConnections( BOARD_COMMIT* aCommit = nullptr );

    template <class Container, class BItem>
    void add( Container& c, BItem brditem )
    {
        CN_ITEM* item = c.Add( brditem );

        m_itemMap[ brditem ] = ITEM_MAP_ENTRY( item );
    }

    void markItemNetAsDirty( const BOARD_ITEM* aItem );

    void updateJumperPads();

private:
    CONNECTIVITY_DATA*                                    m_parentConnectivityData;
    CN_LIST                                               m_itemList;
    std::unordered_map<const BOARD_ITEM*, ITEM_MAP_ENTRY> m_itemMap;

    std::vector<std::shared_ptr<CN_CLUSTER>>              m_connClusters;
    std::vector<std::shared_ptr<CN_CLUSTER>>              m_ratsnestClusters;
    std::vector<bool>                                     m_dirtyNets;

    bool                                                  m_isLocal;
    std::shared_ptr<CONNECTIVITY_DATA>                    m_globalConnectivityData;

    std::mutex m_mutex;

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

protected:
    CN_ITEM* m_item;        ///< The item we are looking for connections to.
};

#endif
