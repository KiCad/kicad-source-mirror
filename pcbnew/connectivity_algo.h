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

#include <connectivity_data.h>

class CN_ITEM;
class CN_CONNECTIVITY_ALGO_IMPL;
class CN_RATSNEST_NODES;
class CN_CLUSTER;
class BOARD;
class BOARD_CONNECTED_ITEM;
class BOARD_ITEM;
class ZONE_CONTAINER;

class CN_ANCHOR
{
public:
    CN_ANCHOR()
    {
        m_item = nullptr;
    }

    CN_ANCHOR( const VECTOR2I& aPos, CN_ITEM* aItem )
    {
        m_pos   = aPos;
        m_item  = aItem;
        assert( m_item );
    }

    bool Valid() const;


    CN_ITEM* Item() const
    {
        return m_item;
    }

    BOARD_CONNECTED_ITEM* Parent() const;

    const VECTOR2I& Pos() const
    {
        return m_pos;
    }

    bool IsDirty() const;

    /// Returns tag, common identifier for connected nodes
    inline int GetTag() const
    {
        return m_tag;
    }

    /// Sets tag, common identifier for connected nodes
    inline void SetTag( int aTag )
    {
        m_tag = aTag;
    }

    /// Decides whether this node can be a ratsnest line target
    inline void SetNoLine( bool aEnable )
    {
        m_noline = aEnable;
    }

    /// Returns true if this node can be a target for ratsnest lines
    inline const bool& GetNoLine() const
    {
        return m_noline;
    }

    inline void SetCluster( std::shared_ptr<CN_CLUSTER> aCluster )
    {
        m_cluster = aCluster;
    }

    inline std::shared_ptr<CN_CLUSTER> GetCluster() const
    {
        return m_cluster;
    }

    bool IsDangling() const;

    // Tag used for unconnected items.
    static const int TAG_UNCONNECTED = -1;

private:
    /// Position of the anchor
    VECTOR2I m_pos;

    /// Item owning the anchor
    CN_ITEM* m_item = nullptr;

    /// Tag for quick connection resolution
    int m_tag = -1;

    /// Whether it the node can be a target for ratsnest lines
    bool m_noline = false;

    /// Cluster to which the anchor belongs
    std::shared_ptr<CN_CLUSTER> m_cluster;
};


typedef std::shared_ptr<CN_ANCHOR>  CN_ANCHOR_PTR;
typedef std::vector<CN_ANCHOR_PTR>  CN_ANCHORS;


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


class CN_CLUSTER
{
private:

    bool m_conflicting = false;
    int m_originNet = 0;
    CN_ITEM* m_originPad = nullptr;
    std::vector<CN_ITEM*> m_items;

public:
    CN_CLUSTER();
    ~CN_CLUSTER();

    bool HasValidNet() const
    {
        return m_originNet >= 0;
    }

    int OriginNet() const
    {
        return m_originNet;
    }

    wxString OriginNetName() const;

    bool    Contains( const CN_ITEM* aItem );
    bool    Contains( const BOARD_CONNECTED_ITEM* aItem );
    void    Dump();

    int Size() const
    {
        return m_items.size();
    }

    bool HasNet() const
    {
        return m_originNet >= 0;
    }

    bool IsOrphaned() const
    {
        return m_originPad == nullptr;
    }

    bool IsConflicting() const
    {
        return m_conflicting;
    }

    void Add( CN_ITEM* item );

    using ITER = decltype(m_items)::iterator;

    ITER begin() { return m_items.begin(); };
    ITER end() { return m_items.end(); };
};

typedef std::shared_ptr<CN_CLUSTER> CN_CLUSTER_PTR;


// basic connectivity item
class CN_ITEM : public INTRUSIVE_LIST<CN_ITEM>
{
private:
    BOARD_CONNECTED_ITEM* m_parent;

    using CONNECTED_ITEMS = std::vector<CN_ITEM*>;

    ///> list of items physically connected (touching)
    CONNECTED_ITEMS m_connected;

    CN_ANCHORS m_anchors;

    ///> visited flag for the BFS scan
    bool m_visited;

    ///> can the net propagator modify the netcode?
    bool m_canChangeNet;

    ///> valid flag, used to identify garbage items (we use lazy removal)
    bool m_valid;

    ///> dirty flag, used to identify recently added item not yet scanned into the connectivity search
    bool m_dirty;

public:
    void Dump();

    CN_ITEM( BOARD_CONNECTED_ITEM* aParent, bool aCanChangeNet, int aAnchorCount = 2 )
    {
        m_parent = aParent;
        m_canChangeNet = aCanChangeNet;
        m_visited = false;
        m_valid = true;
        m_dirty = true;
        m_anchors.reserve( 2 );
    }

    virtual ~CN_ITEM() {};

    CN_ANCHOR_PTR AddAnchor( const VECTOR2I& aPos )
    {
        m_anchors.emplace_back( std::make_shared<CN_ANCHOR>( aPos, this ) );
        //printf("%p add %d\n", this, m_anchors.size() );
        return m_anchors.back();
    }

    CN_ANCHORS& Anchors()
    {
        return m_anchors;
    }

    void SetValid( bool aValid )
    {
        m_valid = aValid;
    }

    bool Valid() const
    {
        return m_valid;
    }

    void SetDirty( bool aDirty )
    {
        m_dirty = aDirty;
    }

    bool Dirty() const
    {
        return m_dirty;
    }

    BOARD_CONNECTED_ITEM* Parent() const
    {
        return m_parent;
    }

    const CONNECTED_ITEMS& ConnectedItems()  const
    {
        return m_connected;
    }

    void ClearConnections()
    {
        m_connected.clear();
    }

    void SetVisited( bool aVisited )
    {
        m_visited = aVisited;
    }

    bool Visited() const
    {
        return m_visited;
    }

    bool CanChangeNet() const
    {
        return m_canChangeNet;
    }

    static void Connect( CN_ITEM* a, CN_ITEM* b )
    {
        bool foundA = false, foundB = false;

        for( auto item : a->m_connected )
        {
            if( item == b )
            {
                foundA = true;
                break;
            }
        }

        for( auto item : b->m_connected )
        {
            if( item == a )
            {
                foundB = true;
                break;
            }
        }

        if( !foundA )
            a->m_connected.push_back( b );

        if( !foundB )
            b->m_connected.push_back( a );
    }

    void RemoveInvalidRefs();

    virtual int             AnchorCount() const;
    virtual const VECTOR2I  GetAnchor( int n ) const;

    int Net() const;
};

typedef std::shared_ptr<CN_ITEM> CN_ITEM_PTR;


class CN_LIST
{
private:
    bool m_dirty;
    std::vector<CN_ANCHOR_PTR> m_anchors;

protected:
    std::vector<CN_ITEM*> m_items;

    void addAnchor( VECTOR2I pos, CN_ITEM* item )
    {
        m_anchors.push_back( item->AddAnchor( pos ) );
    }

private:

    void sort()
    {
        if( m_dirty )
        {
            std::sort( m_anchors.begin(), m_anchors.end() );

            m_dirty = false;
        }
    }

public:
    CN_LIST()
    {
        m_dirty = false;
    }

    void Clear()
    {
        for( auto item : m_items )
            delete item;

        m_items.clear();
    }

    using ITER = decltype(m_items)::iterator;

    ITER begin() { return m_items.begin(); };
    ITER end() { return m_items.end(); };

    std::vector<CN_ANCHOR_PTR>& Anchors() { return m_anchors; }

    template <class T>
    void FindNearby( VECTOR2I aPosition, int aDistMax, T aFunc, bool aDirtyOnly = false );

    template <class T>
    void FindNearby( BOX2I aBBox, T aFunc, bool aDirtyOnly = false );

    void SetDirty( bool aDirty = true )
    {
        m_dirty = aDirty;
    }

    bool IsDirty() const
    {
        return m_dirty;
    }

    void ClearConnections()
    {
        for( auto& anchor : m_anchors )
            anchor->Item()->ClearConnections();
    }

    void RemoveInvalidItems( std::vector<CN_ITEM*>& aGarbage );

    void ClearDirtyFlags()
    {
        for( auto item : m_items )
            item->SetDirty( false );

        SetDirty( false );
    }

    void MarkAllAsDirty()
    {
        for( auto item : m_items )
            item->SetDirty( true );

        SetDirty( true );
    }

    int Size() const
    {
        return m_items.size();
    }
};


class CN_PAD_LIST : public CN_LIST
{
public:
    CN_ITEM* Add( D_PAD* pad )
    {
        auto item = new CN_ITEM( pad, false, 2 );

        addAnchor( pad->ShapePos(), item );
        m_items.push_back( item );

        SetDirty();
        return item;
    }
};


class CN_TRACK_LIST : public CN_LIST
{
public:
    CN_ITEM* Add( TRACK* track )
    {
        auto item = new CN_ITEM( track, true );

        m_items.push_back( item );

        addAnchor( track->GetStart(), item );
        addAnchor( track->GetEnd(), item );
        SetDirty();

        return item;
    }
};


class CN_VIA_LIST : public CN_LIST
{
public:
    CN_ITEM* Add( VIA* via )
    {
        auto item = new CN_ITEM( via, true );

        m_items.push_back( item );
        addAnchor( via->GetStart(), item );
        SetDirty();
        return item;
    }
};


class CN_ZONE : public CN_ITEM
{
public:
    CN_ZONE( ZONE_CONTAINER* aParent, bool aCanChangeNet, int aSubpolyIndex ) :
        CN_ITEM( aParent, aCanChangeNet ),
        m_subpolyIndex( aSubpolyIndex )
    {
        SHAPE_LINE_CHAIN outline = aParent->GetFilledPolysList().COutline( aSubpolyIndex );

        outline.SetClosed( true );
        outline.Simplify();

        m_cachedPoly.reset( new POLY_GRID_PARTITION( outline, 16 ) );
    }

    int SubpolyIndex() const
    {
        return m_subpolyIndex;
    }

    bool ContainsAnchor( const CN_ANCHOR_PTR anchor ) const
    {
        auto zone = static_cast<ZONE_CONTAINER*> ( Parent() );
        return m_cachedPoly->ContainsPoint( anchor->Pos(), zone->GetMinThickness() );
    }

    bool ContainsPoint( const VECTOR2I p ) const
    {
        auto zone = static_cast<ZONE_CONTAINER*> ( Parent() );
        return m_cachedPoly->ContainsPoint( p, zone->GetMinThickness() );
    }

    const BOX2I& BBox() const
    {
        return m_cachedPoly->BBox();
    }

    virtual int             AnchorCount() const override;
    virtual const VECTOR2I  GetAnchor( int n ) const override;

private:
    std::vector<VECTOR2I> m_testOutlinePoints;
    std::unique_ptr<POLY_GRID_PARTITION> m_cachedPoly;
    int m_subpolyIndex;
};


class CN_ZONE_LIST : public CN_LIST
{
public:
    CN_ZONE_LIST() {}

    const std::vector<CN_ITEM*> Add( ZONE_CONTAINER* zone )
    {
        const auto& polys = zone->GetFilledPolysList();

        std::vector<CN_ITEM*> rv;

        for( int j = 0; j < polys.OutlineCount(); j++ )
        {
            CN_ZONE* zitem = new CN_ZONE( zone, false, j );
            const auto& outline = zone->GetFilledPolysList().COutline( j );

            for( int k = 0; k < outline.PointCount(); k++ )
                addAnchor( outline.CPoint( k ), zitem );

            m_items.push_back( zitem );
            rv.push_back( zitem );
            SetDirty();
        }

        return rv;
    }

    template <class T>
    void FindNearbyZones( BOX2I aBBox, T aFunc, bool aDirtyOnly = false );
};


template <class T>
void CN_LIST::FindNearby( BOX2I aBBox, T aFunc, bool aDirtyOnly )
{
    for( auto p : m_anchors )
    {
        if( p->Valid() && aBBox.Contains( p->Pos() ) )
        {
            if( !aDirtyOnly || p->IsDirty() )
                aFunc( p );
        }
    }
}


template <class T>
void CN_ZONE_LIST::FindNearbyZones( BOX2I aBBox, T aFunc, bool aDirtyOnly )
{
    for( auto item : m_items )
    {
        auto zone = static_cast<CN_ZONE*>( item );

        if( aBBox.Intersects( zone->BBox() ) )
        {
            if( !aDirtyOnly || zone->Dirty() )
            {
                aFunc( zone );
            }
        }
    }
}


template <class T>
void CN_LIST::FindNearby( VECTOR2I aPosition, int aDistMax, T aFunc, bool aDirtyOnly )
{
    /* Search items in m_Candidates that position is <= aDistMax from aPosition
     * (Rectilinear distance)
     * m_Candidates is sorted by X then Y values, so a fast binary search is used
     * to locate the "best" entry point in list
     * The best entry is a pad having its m_Pos.x == (or near) aPosition.x
     * All candidates are near this candidate in list
     * So from this entry point, a linear search is made to find all candidates
     */

    sort();

    int idxmax = m_anchors.size() - 1;

    int delta = idxmax + 1;
    int idx = 0;        // Starting index is the beginning of list

    while( delta )
    {
        // Calculate half size of remaining interval to test.
        // Ensure the computed value is not truncated (too small)
        if( ( delta & 1 ) && ( delta > 1 ) )
            delta++;

        delta /= 2;

        auto p = m_anchors[idx];

        int dist = p->Pos().x - aPosition.x;

        if( std::abs( dist ) <= aDistMax )
        {
            break;                              // A good entry point is found. The list can be scanned from this point.
        }
        else if( p->Pos().x < aPosition.x )     // We should search after this point
        {
            idx += delta;

            if( idx > idxmax )
                idx = idxmax;
        }
        else    // We should search before this p
        {
            idx -= delta;

            if( idx < 0 )
                idx = 0;
        }
    }

    /* Now explore the candidate list from the "best" entry point found
     * (candidate "near" aPosition.x)
     * We exp the list until abs(candidate->m_Point.x - aPosition.x) > aDistMashar* Currently a linear search is made because the number of candidates
     * having the right X position is usually small
     */
    // search next candidates in list
    VECTOR2I diff;

    for( int ii = idx; ii <= idxmax; ii++ )
    {
        auto& p = m_anchors[ii];
        diff = p->Pos() - aPosition;;

        if( std::abs( diff.x ) > aDistMax )
            break; // Exit: the distance is to long, we cannot find other candidates

        if( std::abs( diff.y ) > aDistMax )
            continue; // the y distance is to long, but we can find other candidates

        // We have here a good candidate: add it
        if( p->Valid() )
            if( !aDirtyOnly || p->IsDirty() )
                aFunc( p );
    }

    // search previous candidates in list
    for( int ii = idx - 1; ii >=0; ii-- )
    {
        auto& p = m_anchors[ii];
        diff = p->Pos() - aPosition;

        if( abs( diff.x ) > aDistMax )
            break;

        if( abs( diff.y ) > aDistMax )
            continue;

        // We have here a good candidate:add it
        if( p->Valid() )
        {
            if( !aDirtyOnly || p->IsDirty() )
                aFunc( p );
        }
    }
}


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

    bool m_lastSearchWithZones = false;

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


    CN_PAD_LIST m_padList;
    CN_TRACK_LIST m_trackList;
    CN_VIA_LIST m_viaList;
    CN_ZONE_LIST m_zoneList;

    using ITEM_MAP_PAIR = std::pair <const BOARD_CONNECTED_ITEM*, ITEM_MAP_ENTRY>;

    std::unordered_map<const BOARD_CONNECTED_ITEM*, ITEM_MAP_ENTRY> m_itemMap;

    CLUSTERS m_connClusters;
    CLUSTERS m_ratsnestClusters;
    std::vector<bool> m_dirtyNets;

    void    searchConnections( bool aIncludeZones = false );

    void    update();
    void    propagateConnections();

    template <class Container, class BItem>
    void add( Container& c, BItem brditem )
    {
        auto item = c.Add( brditem );

        m_itemMap[ brditem ] = ITEM_MAP_ENTRY( item );
    }

    bool addConnectedItem( BOARD_CONNECTED_ITEM* aItem );
    bool isDirty() const;

    void markItemNetAsDirty( const BOARD_ITEM* aItem );

public:

    CN_CONNECTIVITY_ALGO();
    ~CN_CONNECTIVITY_ALGO();

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
        for( auto cl : m_ratsnestClusters )
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

    void    PropagateNets();
    void    FindIsolatedCopperIslands( ZONE_CONTAINER* aZone, std::vector<int>& aIslands );
    bool    CheckConnectivity( std::vector<CN_DISJOINT_NET_ENTRY>& aReport );

    const CLUSTERS& GetClusters();
    int             GetUnconnectedCount();

    CN_PAD_LIST& PadList() { return m_padList; }

    void ForEachAnchor(  std::function<void(CN_ANCHOR_PTR)> aFunc );
    void ForEachItem(  std::function<void(CN_ITEM*)> aFunc );

    void MarkNetAsDirty( int aNet );

};

bool operator<( const CN_ANCHOR_PTR a, const CN_ANCHOR_PTR b );

#endif
