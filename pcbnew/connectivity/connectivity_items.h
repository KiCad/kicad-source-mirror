/*
 * This program source code file is part of KICAD, a free EDA CAD application.
 *
 * Copyright (C) 2013-2017 CERN
 * Copyright (C) 2018  KiCad Developers, see AUTHORS.txt for contributors.
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


#ifndef PCBNEW_CONNECTIVITY_CONNECTIVITY_ITEMS_H_
#define PCBNEW_CONNECTIVITY_CONNECTIVITY_ITEMS_H_

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

class CN_ITEM;
class CN_CLUSTER;

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

    /**
     * has meaning only for tracks and vias.
     * @return true if this anchor is dangling
     * The anchor point is dangling if the parent is a track
     * and this anchor point is not connected to another item
     * ( track, vas pad or zone) or if the parent is a via and this anchor point
     * is connected to only one track and not to another item
     */
    bool IsDangling() const;

    /**
     * has meaning only for tracks and vias.
     * @return the count of items connected to this anchor
     */
    int ConnectedItemsCount() const;

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


// basic connectivity item
class CN_ITEM : public INTRUSIVE_LIST<CN_ITEM>
{
public:
    using CONNECTED_ITEMS = std::set<CN_ITEM*>;

private:
    BOARD_CONNECTED_ITEM* m_parent;

    ///> list of items physically connected (touching)
    CONNECTED_ITEMS m_connected;

    CN_ANCHORS m_anchors;

    ///> visited flag for the BFS scan
    bool m_visited;

    ///> can the net propagator modify the netcode?
    bool m_canChangeNet;

    ///> valid flag, used to identify garbage items (we use lazy removal)
    bool m_valid;

    ///> mutex protecting this item's connected_items set to allow parallel connection threads
    std::mutex m_listLock;

protected:
    ///> dirty flag, used to identify recently added item not yet scanned into the connectivity search
    bool m_dirty;

    ///> layer range over which the item exists
    LAYER_RANGE m_layers;

    ///> bounding box for the item
    BOX2I m_bbox;

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
        m_layers = LAYER_RANGE( 0, PCB_LAYER_ID_COUNT );
    }

    virtual ~CN_ITEM() {};

    void AddAnchor( const VECTOR2I& aPos )
    {
        m_anchors.emplace_back( std::make_unique<CN_ANCHOR>( aPos, this ) );
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

    /**
     * Function SetLayers()
     *
     * Sets the layers spanned by the item to aLayers.
     */
    void SetLayers( const LAYER_RANGE& aLayers )
    {
        m_layers = aLayers;
    }

    /**
     * Function SetLayer()
     *
     * Sets the layers spanned by the item to a single layer aLayer.
     */
    void SetLayer( int aLayer )
    {
        m_layers = LAYER_RANGE( aLayer, aLayer );
    }

    /**
     * Function Layers()
     *
     * Returns the contiguous set of layers spanned by the item.
     */
    const LAYER_RANGE& Layers() const
    {
        return m_layers;
    }

    /**
     * Function Layer()
     *
     * Returns the item's layer, for single-layered items only.
     */
    virtual int Layer() const
    {
        return Layers().Start();
    }

    const BOX2I& BBox()
    {
        if( m_dirty && m_valid )
        {
            EDA_RECT box = m_parent->GetBoundingBox();
            m_bbox = BOX2I( box.GetPosition(), box.GetSize() );
        }
        return m_bbox;
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

    void Connect( CN_ITEM* b )
    {
        std::lock_guard<std::mutex> lock( m_listLock );
        m_connected.insert( b );
    }

    void RemoveInvalidRefs();

    virtual int             AnchorCount() const;
    virtual const VECTOR2I  GetAnchor( int n ) const;

    int Net() const;
};

typedef std::shared_ptr<CN_ITEM> CN_ITEM_PTR;

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
        return ContainsPoint( anchor->Pos() );
    }

    bool ContainsPoint( const VECTOR2I p ) const
    {
        auto zone = static_cast<ZONE_CONTAINER*> ( Parent() );
        int clearance = zone->GetFilledPolysUseThickness() ? zone->GetMinThickness() : 0;
        return m_cachedPoly->ContainsPoint( p, clearance );
    }

    const BOX2I& BBox()
    {
        if( m_dirty )
            m_bbox = m_cachedPoly->BBox();

        return m_bbox;
    }

    virtual int             AnchorCount() const override;
    virtual const VECTOR2I  GetAnchor( int n ) const override;

private:
    std::vector<VECTOR2I> m_testOutlinePoints;
    std::unique_ptr<POLY_GRID_PARTITION> m_cachedPoly;
    int m_subpolyIndex;
};

class CN_LIST
{
private:
    bool m_dirty;
    bool m_hasInvalid;

    CN_RTREE<CN_ITEM*> m_index;

protected:
    std::vector<CN_ITEM*> m_items;

    void addItemtoTree( CN_ITEM* item )
    {
        m_index.Insert( item );
    }

public:
    CN_LIST()
    {
        m_dirty = false;
        m_hasInvalid = false;
    }

    void Clear()
    {
        for( auto item : m_items )
            delete item;

        m_items.clear();
        m_index.RemoveAll();
    }

    using ITER = decltype(m_items)::iterator;

    ITER begin() { return m_items.begin(); };
    ITER end() { return m_items.end(); };

    CN_ITEM* operator[] ( int aIndex ) { return m_items[aIndex]; }

    template <class T>
    void FindNearby( CN_ITEM *aItem, T aFunc )
    {
        m_index.Query( aItem->BBox(), aItem->Layers(), aFunc );
    }

    void SetHasInvalid( bool aInvalid = true )
    {
        m_hasInvalid = aInvalid;
    }

    void SetDirty( bool aDirty = true )
    {
        m_dirty = aDirty;
    }

    bool IsDirty() const
    {
        return m_dirty;
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

    CN_ITEM* Add( D_PAD* pad );

    CN_ITEM* Add( TRACK* track );

    CN_ITEM* Add( VIA* via );

    const std::vector<CN_ITEM*> Add( ZONE_CONTAINER* zone );
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
        return m_originNet > 0;
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
        return m_originNet > 0;
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


#endif /* PCBNEW_CONNECTIVITY_CONNECTIVITY_ITEMS_H_ */
