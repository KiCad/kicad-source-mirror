/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2013-2017 CERN
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
 *
 * @author Tomasz Wlostowski <tomasz.wlostowski@cern.ch>
 * @author Maciej Suminski <maciej.suminski@cern.ch>
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
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */


#include <layer_ids.h>
#include <trace_helpers.h>
#include <wx/log.h>

#include <view/view.h>
#include <view/view_group.h>
#include <view/view_item.h>
#include <view/view_rtree.h>
#include <view/view_overlay.h>

#include <gal/definitions.h>
#include <gal/graphics_abstraction_layer.h>
#include <gal/painter.h>
#include <algorithm>
#include <array>
#include <chrono>
#include <cinttypes>
#include <cmath>
#include <limits>
#include <tuple>
#include <unordered_map>
#include <unordered_set>

#include <core/profile.h>


#ifdef KICAD_GAL_PROFILE
#include <wx/log.h>
#include <trace_helpers.h>
#endif

namespace KIGFX {

class VIEW;

class VIEW_ITEM_DATA
{
public:
    VIEW_ITEM_DATA() :
        m_view( nullptr ),
        m_flags( KIGFX::VISIBLE ),
        m_requiredUpdate( KIGFX::NONE ),
        m_pendingIndex( -1 ),
        m_drawPriority( 0 ),
        m_cachedIndex( -1 ),
        m_groups( nullptr ),
        m_groupsSize( 0 ) {}

    ~VIEW_ITEM_DATA()
    {
        deleteGroups();
    }

    int GetFlags() const
    {
        return m_flags;
    }

private:
    friend class VIEW;

    /**
     * Return number of the group id for the given layer, or -1 in case it was not cached before.
     *
     * @param aLayer is the layer number for which group id is queried.
     * @return group id or -1 in case there is no group id (ie. item is not cached).
     */
    int getGroup( int aLayer ) const
    {
        for( int i = 0; i < m_groupsSize; ++i )
        {
            if( m_groups[i].first == aLayer )
                return m_groups[i].second;
        }

        return -1;
    }

    /**
     * Set a group id for the item and the layer combination.
     *
     * @param aLayer is the layer number.
     * @param aGroup is the group id.
     */
    void setGroup( int aLayer, int aGroup )
    {
        // Look if there is already an entry for the layer
        for( int i = 0; i < m_groupsSize; ++i )
        {
            if( m_groups[i].first == aLayer )
            {
                m_groups[i].second = aGroup;
                return;
            }
        }

        // If there was no entry for the given layer - create one
        std::pair<int, int>* newGroups = new std::pair<int, int>[m_groupsSize + 1];

        if( m_groupsSize > 0 )
        {
            std::copy( m_groups, m_groups + m_groupsSize, newGroups );
            delete[] m_groups;
        }

        m_groups = newGroups;
        newGroups[m_groupsSize++] = { aLayer, aGroup };
    }


    /**
     * Remove all of the stored group ids. Forces recaching of the item.
     */
    void deleteGroups()
    {
        delete[] m_groups;
        m_groups = nullptr;
        m_groupsSize = 0;
    }

    /**
     * Return information if the item uses at least one group id (ie. if it is cached at all).
     *
     * @returns true in case it is cached at least for one layer.
     */
    inline bool storesGroups() const
    {
        return m_groupsSize > 0;
    }

    /**
     * Reorder the stored groups (to facilitate reordering of layers).
     *
     * @see VIEW::ReorderLayerData
     *
     * @param aReorderMap is the mapping of old to new layer ids
     */
    void reorderGroups( const std::unordered_map<int, int>& aReorderMap )
    {
        for( int i = 0; i < m_groupsSize; ++i )
        {
            auto it = aReorderMap.find( m_groups[i].first );

            if( it != aReorderMap.end() )
                m_groups[i].first = it->second;
        }
    }

    /**
     * Save layers used by the item.
     *
     * @param aLayers is an array containing layer numbers to be saved.
     */
    void saveLayers( const std::vector<int>& aLayers )
    {
        m_layers.clear();

        for( int layer : aLayers )
        {
            wxCHECK2_MSG( layer >= 0 && layer < VIEW::VIEW_MAX_LAYERS, continue,
                          wxString::Format( wxT( "Invalid layer number: %d" ), layer ) );
            m_layers.push_back( layer );
        }
    }

    /**
     * Return current update flag for an item.
     */
    int requiredUpdate() const
    {
        return m_requiredUpdate;
    }

    /**
     * Mark an item as already updated, so it is not going to be redrawn.
     */
    void clearUpdateFlags()
    {
        m_requiredUpdate = NONE;
    }

    /**
     * Return if the item should be drawn or not.
     */
    bool isRenderable() const
    {
        return m_flags == VISIBLE;
    }

    VIEW*                m_view;             ///< Current dynamic view the item is assigned to.
    int                  m_flags;            ///< Visibility flags
    int                  m_requiredUpdate;   ///< Flag required for updating
    int                  m_pendingIndex;     // Place in the view's pending list, or -1
    int                  m_drawPriority;     ///< Order to draw this item in a layer, lowest first
    int                  m_cachedIndex;      ///< Cached index in m_allItems.

    std::pair<int, int>* m_groups;           ///< layer_number:group_id pairs for each layer the
                                             ///< item occupies.
    int                  m_groupsSize;

    std::vector<int>     m_layers;           /// Stores layer numbers used by the item.

    BOX2I                m_bbox;             /// Cached inserted Bbox for faster removals.
};


struct RETAINED_ORDER_ITEM
{
    VIEW_ITEM*           item;
    int                  layer;
    int                  group;
    RETAINED_GROUP_ORDER order;
};


static RETAINED_GROUP_ORDER retainedOrderKey( VIEW_ITEM* aItem, int aPriority, const BOX2I& aBBox,
                                               bool aUseDrawPriority, bool aReverseDrawOrder,
                                               double aLodScale )
{
    RETAINED_GROUP_ORDER key;
    key.priority = aUseDrawPriority ? ( aReverseDrawOrder ? -int64_t( aPriority ) : int64_t( aPriority ) ) : 0;

    key.bbox = { aBBox.GetLeft(), aBBox.GetTop(), aBBox.GetRight(), aBBox.GetBottom() };
    key.className = aItem->GetClass();
    key.lodScale = std::isnan( aLodScale ) ? std::numeric_limits<double>::infinity() : aLodScale;

    return key;
}


template <typename T>
static void sortRetainedOrder( VIEW* aView, std::vector<T>& aEntries )
{
    std::sort( aEntries.begin(), aEntries.end(), []( const T& a, const T& b )
               {
                   return a.order < b.order;
               } );

    for( size_t first = 0; first < aEntries.size(); )
    {
        size_t last = first + 1;

        while( last < aEntries.size() && aEntries[first].order.SamePrefix( aEntries[last].order ) )
            ++last;

        if( last - first > 1 )
        {
            for( size_t i = first; i < last; ++i )
            {
                T& entry = aEntries[i];
                entry.order.contentHash = aView->GetGAL()->GroupContentFingerprint( entry.group );
                entry.order.contentTie = true;
            }

            std::sort( aEntries.begin() + first, aEntries.begin() + last,
                       []( const T& a, const T& b ) { return a.order < b.order; } );
        }

        first = last;
    }
}


void VIEW::OnDestroy( VIEW_ITEM* aItem )
{
    if( aItem->m_viewPrivData )
    {
        if( aItem->m_viewPrivData->m_view )
            aItem->m_viewPrivData->m_view->unlinkItem( aItem );

        delete aItem->m_viewPrivData;
        aItem->m_viewPrivData = nullptr;
    }
}


VIEW::VIEW() :
    m_enableOrderModifier( true ),
    m_scale( 4.0 ),
    m_minScale( 0.2 ), m_maxScale( 50000.0 ),
    m_mirrorX( false ), m_mirrorY( false ),
    m_layerVisibilityCache( VIEW_MAX_LAYERS ),
    m_layerCachedFlagCache( VIEW_MAX_LAYERS ),
    m_painter( nullptr ),
    m_gal( nullptr ),
    m_useDrawPriority( false ),
    m_nextDrawPriority( 0 ),
    m_reverseDrawOrder( false ),
    m_hasPendingItemUpdates( false ),
    m_cameraDirtyOnly( false )
{
    // Set m_boundary to define the max area size. The default area size
    // is defined here as the max value of a int.
    // this is a default value acceptable for Pcbnew and Gerbview, but too large for Eeschema.
    // So in eeschema a call to SetBoundary() with a smaller value will be needed.
    typedef std::numeric_limits<int> coord_limits;
    double pos = coord_limits::lowest() / 2 + coord_limits::epsilon();
    double size = coord_limits::max() - coord_limits::epsilon();
    m_boundary.SetOrigin( pos, pos );
    m_boundary.SetSize( size, size );

    m_allItems.reset( new std::vector<VIEW_ITEM*> );
    m_allItems->reserve( 32768 );

    // Redraw everything at the beginning
    MarkDirty();

    // View uses layers to display EDA_ITEMs (item may be displayed on several layers, for example
    // pad may be shown on pad, pad hole and solder paste layers). There are usual copper layers
    // (eg. F.Cu, B.Cu, internal and so on) and layers for displaying objects such as texts,
    // silkscreen, pads, vias, etc.
    for( int ii = 0; ii < VIEW_MAX_LAYERS; ++ii )
    {
        auto [it, _] = m_layers.emplace( ii, VIEW_LAYER() );
        VIEW_LAYER& l = it->second;

        l.items          = std::make_shared<VIEW_RTREE>();
        l.id             = ii;
        l.renderingOrder = ii;
        l.visible        = true;
        l.displayOnly    = false;
        l.diffLayer      = false;
        l.hasNegatives   = false;
        l.target         = TARGET_CACHED;
    }

    SortOrderedLayers();

    m_preview.reset( new KIGFX::VIEW_GROUP() );
    Add( m_preview.get() );
}


VIEW::~VIEW()
{
    Remove( m_preview.get() );
}


void VIEW::Add( VIEW_ITEM* aItem, int aDrawPriority )
{
    if( aDrawPriority < 0 )
        aDrawPriority = m_nextDrawPriority++;

    if( !aItem->m_viewPrivData )
        aItem->m_viewPrivData = new VIEW_ITEM_DATA;

    // One view pointer and one index per item, so re-registering strands the first entry to
    // dangle at free time
    if( VIEW* previous = aItem->m_viewPrivData->m_view )
        previous->unlinkItem( aItem );

    aItem->m_viewPrivData->m_view = this;
    aItem->m_viewPrivData->m_pendingIndex = -1;
    aItem->m_viewPrivData->m_drawPriority = aDrawPriority;
    const BOX2I bbox = aItem->ViewBBox();
    aItem->m_viewPrivData->m_bbox = bbox;
    aItem->m_viewPrivData->m_cachedIndex = m_allItems->size();

    std::vector<int> layers = aItem->ViewGetLayers();

    std::erase_if( layers, []( int layer )
    {
        return layer < 0 || layer >= VIEW_MAX_LAYERS;
    } );

    if( layers.empty() )
        return;

    aItem->viewPrivData()->saveLayers( layers );

    m_allItems->push_back( aItem );

    for( int layer : layers )
    {
        VIEW_LAYER& l = m_layers[layer];
        l.items->Insert( aItem, bbox );
        MarkTargetDirty( l.target );
    }

    // The camera-reuse path would replay a scene without this item.
    markSceneEdited();

    SetVisible( aItem, true );
    Update( aItem, KIGFX::INITIAL_ADD );
}


void VIEW::AddBatch( const std::vector<VIEW_ITEM*>& aItems )
{
    // Phase 1: Register all items and collect per-layer data
    std::unordered_map<int, std::vector<std::pair<VIEW_ITEM*, BOX2I>>> layerBulk;
    std::unordered_set<VIEW_ITEM*>                                     seen;
    std::vector<VIEW_ITEM*>                                            registered;

    seen.reserve( aItems.size() );
    registered.reserve( aItems.size() );

    for( VIEW_ITEM* item : aItems )
    {
        // A repeat within the batch cannot be detached below because its layer entries are not
        // inserted until phase 2, so it would reach BulkLoad twice
        if( !item || !seen.insert( item ).second )
            continue;

        int drawPriority = m_nextDrawPriority++;

        if( !item->m_viewPrivData )
            item->m_viewPrivData = new VIEW_ITEM_DATA;

        // Same single-registration invariant as Add()
        if( VIEW* previous = item->m_viewPrivData->m_view )
            previous->unlinkItem( item );

        item->m_viewPrivData->m_view = this;
        item->m_viewPrivData->m_pendingIndex = -1;
        item->m_viewPrivData->m_drawPriority = drawPriority;
        const BOX2I bbox = item->ViewBBox();
        item->m_viewPrivData->m_bbox = bbox;
        item->m_viewPrivData->m_cachedIndex = m_allItems->size();

        std::vector<int> layers = item->ViewGetLayers();

        std::erase_if( layers, []( int layer )
        {
            return layer < 0 || layer >= VIEW_MAX_LAYERS;
        } );

        if( layers.empty() )
            continue;

        item->viewPrivData()->saveLayers( layers );
        m_allItems->push_back( item );
        registered.push_back( item );

        for( int layer : layers )
            layerBulk[layer].emplace_back( item, bbox );
    }

    // Phase 2: Bulk load each layer's R-tree
    for( auto& [layerId, items] : layerBulk )
    {
        VIEW_LAYER& l = m_layers[layerId];
        l.items->BulkLoad( items );
        MarkTargetDirty( l.target );
    }

    if( !aItems.empty() )
        markSceneEdited();

    // Phase 3: Set visibility and queue initial invalidation.
    // INITIAL_ADD is required even though VIEW_ITEM_DATA defaults VISIBLE=true (making
    // SetVisible a no-op). Without it, items reused after VIEW::Clear() retain stale GAL
    // cache group IDs that point to freed memory.
    for( VIEW_ITEM* item : registered )
    {
        SetVisible( item, true );
        Update( item, KIGFX::INITIAL_ADD );
    }
}


void VIEW::Remove( VIEW_ITEM* aItem )
{
    unlinkItem( aItem );
}


void VIEW::unlinkItem( VIEW_ITEM* aItem )
{
    static int s_gcCounter = 0;

    if( aItem && aItem->m_viewPrivData )
    {
        if( aItem->m_viewPrivData->m_view != nullptr && aItem->m_viewPrivData->m_view != this )
        {
            wxLogDebug( wxT( "VIEW::Remove: item %s belongs to a different view" ), aItem->GetClass() );
            return;
        }

        std::vector<VIEW_ITEM*>::iterator item = m_allItems->end();
        int                               cachedIndex = aItem->m_viewPrivData->m_cachedIndex;

        if( cachedIndex >= 0
            && cachedIndex < static_cast<ssize_t>( m_allItems->size() )
            && ( *m_allItems )[cachedIndex] == aItem )
        {
            item = m_allItems->begin() + cachedIndex;
        }
        else
        {
            item = std::find( m_allItems->begin(), m_allItems->end(), aItem );
        }

        if( aItem->m_viewPrivData->m_pendingIndex >= 0 )
        {
            m_pendingItems[aItem->m_viewPrivData->m_pendingIndex] = nullptr;
            aItem->m_viewPrivData->m_pendingIndex = -1;
        }

        if( item != m_allItems->end() )
        {
            *item = nullptr;
            aItem->m_viewPrivData->clearUpdateFlags();

            s_gcCounter++;

            if( s_gcCounter > 4096 )
            {
                // Perform defragmentation
                std::erase_if( *m_allItems,
                                []( VIEW_ITEM* it )
                                {
                                    return it == nullptr;
                                } );

                // Update cached indices
                for( size_t idx = 0; idx < m_allItems->size(); idx++ )
                    ( *m_allItems )[idx]->m_viewPrivData->m_cachedIndex = idx;

                s_gcCounter = 0;
            }
        }

        const BOX2I* bbox = &aItem->m_viewPrivData->m_bbox;

        for( int layer : aItem->m_viewPrivData->m_layers )
        {
            VIEW_LAYER& l = m_layers[layer];
            l.items->Remove( aItem, bbox );
            MarkTargetDirty( l.target );

            // Clear the GAL cache
            int prevGroup = aItem->m_viewPrivData->getGroup( layer );

            if( prevGroup >= 0 )
                m_gal->DeleteGroup( prevGroup );
        }

        aItem->m_viewPrivData->deleteGroups();
        aItem->m_viewPrivData->m_view = nullptr;

        // The camera-reuse path would still draw the removed item.
        markSceneEdited();
    }
}


void VIEW::SetRequired( int aLayerId, int aRequiredId, bool aRequired )
{
    wxCHECK( (unsigned) aLayerId < m_layers.size(), /*void*/ );
    wxCHECK( (unsigned) aRequiredId < m_layers.size(), /*void*/ );

    if( aRequired )
        m_layers[aLayerId].requiredLayers.insert( aRequiredId );
    else
        m_layers[aLayerId].requiredLayers.erase( aRequiredId );

    // redrawRect() skips layers whose required layers are hidden, so the cached output is stale.
    layerStateChanged( aLayerId );
}


void VIEW::layerStateChanged( int aLayer )
{
    if( !m_gal || !m_gal->HasRetainedScene() )
    {
        if( IsCached( aLayer ) )
        {
            MarkGeometryDirty();
            MarkStaticDirty();
        }

        return;
    }

    m_sceneLayersStale = true;
    m_lodRefreshLayers.push_back( aLayer );

    // Items of the kept non-cached targets can read the layer's visibility as well
    MarkTargetDirty( TARGET_CACHED );
    MarkTargetDirty( TARGET_NONCACHED );
    MarkTargetDirty( TARGET_OVERLAY );
    markSceneEdited();
}


void VIEW::noteLodRead( int aLayer ) const
{
    if( aLayer < 0 || aLayer >= VIEW_MAX_LAYERS )
        return;

    std::vector<bool>& readers = m_lodReads[aLayer];

    if( readers.empty() )
        readers.resize( VIEW_MAX_LAYERS );

    readers[m_lodProbeLayer] = true;
}


void VIEW::SyncRetainedScene()
{
    if( !m_gal || !m_gal->HasRetainedScene() )
        return;

    m_gal->SetSceneScale( m_scale );
    m_gal->SetSceneDrawOrdered( m_useDrawPriority );

    if( m_sceneLayersStale )
    {
        for( const auto& [id, l] : m_layers )
        {
            const bool drawn = l.target == TARGET_CACHED && l.visible && areRequiredLayersEnabled( id );

            m_gal->SetSceneLayerState( id, l.renderingOrder, drawn, l.hasNegatives );

            COLOR4D toward;
            double  keep = 1.0;

            if( l.target == TARGET_CACHED && m_painter && m_painter->GetSettings()->GetLayerDim( id, toward, keep ) )
                m_gal->SetSceneLayerDim( id, toward, keep );
            else
                m_gal->SetSceneLayerDim( id, COLOR4D::BLACK, 1.0 );
        }

        m_sceneLayersStale = false;
    }

    if( m_lodRefreshLayers.empty() )
        return;

    // A rebuild asks every item anyway
    if( m_gal->HasStaticRebuildPending() || m_lodReads.empty() )
    {
        m_lodRefreshLayers.clear();
        return;
    }

    if( !m_gal->IsVisible() || !m_gal->IsInitialized() )
        return;

    GAL_UPDATE_CONTEXT ctx( m_gal );

    if( !ctx.IsUpdating() )
        return;

    SyncLayerVisibilityCache();

    std::vector<bool> readers( VIEW_MAX_LAYERS );

    for( int read : m_lodRefreshLayers )
    {
        auto it = m_lodReads.find( read );

        if( it == m_lodReads.end() )
            continue;

        for( int reader = 0; reader < VIEW_MAX_LAYERS; ++reader )
        {
            if( it->second[reader] )
                readers[reader] = true;
        }
    }

    m_lodRefreshLayers.clear();

    BOX2I everywhere;
    everywhere.SetMaximum();

    for( auto& [id, l] : m_layers )
    {
        if( !readers[id] || l.target != TARGET_CACHED )
            continue;

        const int layer = id;

        auto place = [&]( VIEW_ITEM* aItem ) -> bool
        {
            placeCachedGroup( aItem, layer );
            return true;
        };

        l.items->Query( everywhere, place );
    }
}


void VIEW::MarkStaticDirty()
{
    if( !m_gal )
        return;

    m_gal->MarkStaticDirty();

    // Whatever rebuilds the cached scene, such as showing a layer, can also change what the kept
    // non-cached targets draw
    if( m_gal->HasRetainedScene() )
    {
        MarkTargetDirty( TARGET_NONCACHED );
        MarkTargetDirty( TARGET_OVERLAY );
        m_sceneLayersStale = true;
    }
}


void VIEW::markSceneEdited()
{
    // Slots are handed out in arrival order, which cannot follow a draw priority
    if( !m_gal || !m_gal->HasRetainedScene() || m_useDrawPriority )
    {
        MarkGeometryDirty();
        return;
    }

    m_cameraDirtyOnly = false;
}


int VIEW::Query( const BOX2I& aRect, std::vector<LAYER_ITEM_PAIR>& aResult ) const
{
    if( m_orderedLayers.empty() )
        return 0;

    int  layer = UNDEFINED_LAYER;
    auto visitor =
            [&]( VIEW_ITEM* item ) -> bool
            {
                aResult.push_back( VIEW::LAYER_ITEM_PAIR( item, layer ) );
                return true;
            };

    std::vector<VIEW_LAYER*>::const_reverse_iterator i;

    // execute queries in reverse direction, so that items that are on the top of
    // the rendering stack are returned first.
    for( i = m_orderedLayers.rbegin(); i != m_orderedLayers.rend(); ++i )
    {
        // ignore layers that do not contain actual items (i.e. the selection box, menus, floats)
        if( ( *i )->displayOnly || !( *i )->visible )
            continue;

        layer = ( *i )->id;
        ( *i )->items->Query( aRect, visitor );
    }

    return aResult.size();
}


void VIEW::Query( const BOX2I& aRect, const std::function<bool( VIEW_ITEM* )>& aFunc ) const
{
    if( m_orderedLayers.empty() )
        return;

    for( const auto& i : m_orderedLayers )
    {
        // ignore layers that do not contain actual items (i.e. the selection box, menus, floats)
        if( i->displayOnly || !i->visible )
            continue;

        i->items->Query( aRect, aFunc );
    }
}


VECTOR2D VIEW::ToWorld( const VECTOR2D& aCoord, bool aAbsolute ) const
{
    const MATRIX3x3D& matrix = m_gal->GetScreenWorldMatrix();

    if( aAbsolute )
        return VECTOR2D( matrix * aCoord );
    else
        return VECTOR2D( matrix.GetScale().x * aCoord.x, matrix.GetScale().y * aCoord.y );
}


double VIEW::ToWorld( double aSize ) const
{
    const MATRIX3x3D& matrix = m_gal->GetScreenWorldMatrix();

    return fabs( matrix.GetScale().x * aSize );
}


VECTOR2D VIEW::ToScreen( const VECTOR2D& aCoord, bool aAbsolute ) const
{
    const MATRIX3x3D& matrix = m_gal->GetWorldScreenMatrix();

    if( aAbsolute )
        return VECTOR2D( matrix * aCoord );
    else
        return VECTOR2D( matrix.GetScale().x * aCoord.x, matrix.GetScale().y * aCoord.y );
}


double VIEW::ToScreen( double aSize ) const
{
    const MATRIX3x3D& matrix = m_gal->GetWorldScreenMatrix();

    return matrix.GetScale().x * aSize;
}


void VIEW::CopySettings( const VIEW* aOtherView )
{
    wxASSERT_MSG( false, wxT( "This is not implemented" ) );
}


void VIEW::SetGAL( GAL* aGal )
{
    bool recacheGroups = ( m_gal != nullptr );    // recache groups only if GAL is reassigned
    m_gal = aGal;

    // A new retained GAL knows no layer state and would show every layer
    m_sceneLayersStale = true;

    // clear group numbers, so everything is going to be recached
    if( recacheGroups )
    {
        clearGroupCache();
        UpdateAllItems( REPAINT );
    }

    // every target has to be refreshed
    MarkDirty();

    // SetScale() would derive the centre from a GAL still at its default zoom, shifting the view a
    // little on every backend switch.
    SetMirror( m_mirrorX, m_mirrorY );
    m_gal->SetZoomFactor( m_scale );
    SetCenter( m_center );
}


BOX2D VIEW::GetViewport() const
{
    BOX2D    rect;
    VECTOR2D screenSize = m_gal->GetScreenPixelSize();

    rect.SetOrigin( ToWorld( VECTOR2D( 0, 0 ) ) );
    rect.SetEnd( ToWorld( screenSize ) );

    return rect.Normalize();
}


void VIEW::SetViewport( const BOX2D& aViewport )
{
    VECTOR2D ssize = ToWorld( m_gal->GetScreenPixelSize(), false );

    wxCHECK( fabs(ssize.x) > 0 && fabs(ssize.y) > 0, /*void*/ );

    VECTOR2D centre = aViewport.Centre();
    VECTOR2D vsize  = aViewport.GetSize();
    double   zoom   = 1.0 / std::max( fabs( vsize.x / ssize.x ), fabs( vsize.y / ssize.y ) );

    SetCenter( centre );
    SetScale( GetScale() * zoom );
}


void VIEW::SetMirror( bool aMirrorX, bool aMirrorY )
{
    wxASSERT_MSG( !aMirrorY, _( "Mirroring for Y axis is not supported yet" ) );

    m_mirrorX = aMirrorX;
    m_mirrorY = aMirrorY;
    m_gal->SetFlip( aMirrorX, aMirrorY );

    // Redraw everything
    MarkDirty();
}


void VIEW::SetScale( double aScale, VECTOR2D aAnchor )
{
    if( aAnchor == VECTOR2D( 0, 0 ) )
        aAnchor = m_center;

    VECTOR2D     a = ToScreen( aAnchor );
    const double oldScale = m_scale;

    if( aScale < m_minScale )
        m_scale = m_minScale;
    else if( aScale > m_maxScale )
        m_scale = m_maxScale;
    else
        m_scale = aScale;

    m_gal->SetZoomFactor( m_scale );
    m_gal->ComputeWorldScreenMatrix();

    VECTOR2D delta = ToWorld( a ) - aAnchor;

    SetCenter( m_center - delta );

    // Redraw everything after the viewport has changed
    MarkViewportDirty();

    if( m_scale != oldScale )
        rescaleKeptTargets();
}


bool VIEW::IsShownAtScale( double aLod ) const
{
    noteKeptLod( aLod );

    return aLod < m_scale;
}


void VIEW::noteKeptLod( double aLod ) const
{
    // A kept target has to be emitted again once the scale crosses one of these
    if( !m_keptScaleUse || aLod == VIEW_ITEM::LOD_SHOW || aLod == VIEW_ITEM::LOD_HIDE )
        return;

    if( aLod < m_scale )
        m_keptScaleUse->lodBelow = std::max( m_keptScaleUse->lodBelow, aLod );
    else
        m_keptScaleUse->lodAbove = std::min( m_keptScaleUse->lodAbove, aLod );
}


void VIEW::rescaleKeptTargets()
{
    if( !m_gal || !m_gal->HasRetainedScene() )
        return;

    for( int target : { TARGET_NONCACHED, TARGET_OVERLAY } )
    {
        const SCALE_USE& use = m_scaleUse[target];

        // A pan reuses the target, and so does a zoom that changes nothing it holds
        if( use.read || m_scale <= use.lodBelow || m_scale > use.lodAbove )
        {
            MarkTargetDirty( target );
            m_cameraDirtyOnly = false;
        }
    }
}


void VIEW::SetCenter( const VECTOR2D& aCenter )
{
    m_center = aCenter;

    if( !m_boundary.Contains( aCenter ) )
    {
        if( m_center.x < m_boundary.GetLeft() )
            m_center.x = m_boundary.GetLeft();
        else if( aCenter.x > m_boundary.GetRight() )
            m_center.x = m_boundary.GetRight();

        if( m_center.y < m_boundary.GetTop() )
            m_center.y = m_boundary.GetTop();
        else if( m_center.y > m_boundary.GetBottom() )
            m_center.y = m_boundary.GetBottom();
    }

    m_gal->SetLookAtPoint( m_center );
    m_gal->ComputeWorldScreenMatrix();

    // Redraw everything after the viewport has changed
    MarkViewportDirty();
}


void VIEW::SetCenter( const VECTOR2D& aCenter, const std::vector<BOX2D>& obscuringScreenRects )
{
    if( obscuringScreenRects.empty() )
        return SetCenter( aCenter );

    BOX2D          screenRect( { 0, 0 }, m_gal->GetScreenPixelSize() );
    SHAPE_POLY_SET unobscuredPoly( screenRect );
    VECTOR2D       unobscuredCenter = screenRect.Centre();

    for( const BOX2D& obscuringScreenRect : obscuringScreenRects )
    {
        SHAPE_POLY_SET obscuringPoly( obscuringScreenRect );
        unobscuredPoly.BooleanSubtract( obscuringPoly );
    }

    /*
     * Perform a step-wise deflate to find the center of the largest unobscured area
     */

    BOX2I bbox = unobscuredPoly.BBox();
    int   step = std::min( bbox.GetWidth(), bbox.GetHeight() ) / 10;

    if( step < 20 )
        step = 20;

    while( !unobscuredPoly.IsEmpty() )
    {
        unobscuredCenter = unobscuredPoly.BBox().Centre();
        unobscuredPoly.Deflate( step, CORNER_STRATEGY::ALLOW_ACUTE_CORNERS, ARC_LOW_DEF );
    }

    SetCenter( aCenter - ToWorld( unobscuredCenter - screenRect.Centre(), false ) );
}


void VIEW::SetLayerOrder( int aLayer, int aRenderingOrder, bool aAutoSort )
{
    m_layers[aLayer].renderingOrder = aRenderingOrder;

    if( aAutoSort )
        SortOrderedLayers();
}


int VIEW::GetLayerOrder( int aLayer ) const
{
    return m_layers.at( aLayer ).renderingOrder;
}


void VIEW::SortLayers( std::vector<int>& aLayers ) const
{
    std::sort( aLayers.begin(), aLayers.end(),
               [this]( int a, int b )
               {
                   return GetLayerOrder( a ) > GetLayerOrder( b );
               } );
}


void VIEW::ReorderLayerData( std::unordered_map<int, int> aReorderMap )
{
    // GerbView's remapping is not a permutation once an image is deleted, so permute in place.
    // Rebuilding the map can drop an id, and m_layers[] then default-constructs a null R-tree
    std::map<int, VIEW_LAYER> moved;
    std::unordered_set<int>   destinations;

    for( const auto& [from, to] : aReorderMap )
    {
        if( from == to )
            continue;

        auto source = m_layers.find( from );

        if( source == m_layers.end() || !m_layers.count( to ) )
            continue;

        // Every caller permutes or compacts, so two sources landing on one destination means the
        // caller built a bad map and the loser's items are about to be discarded
        if( destinations.count( to ) )
        {
            wxLogDebug( wxT( "VIEW::ReorderLayerData: layer %d also maps to %d, which already "
                             "takes a layer; its items will be lost" ), from, to );
        }

        moved.emplace( from, source->second );
        destinations.insert( to );
    }

    // Every mover was copied before any of them was overwritten, so these can run in any order
    for( const auto& [from, layer] : moved )
    {
        auto destination = m_layers.find( aReorderMap.at( from ) );

        destination->second = layer;
        destination->second.id = destination->first;
    }

    // A source nothing moves onto keeps its settings but must own an empty tree rather than the
    // one it just handed to its destination
    for( const auto& [from, _] : moved )
    {
        if( !destinations.count( from ) )
            m_layers.find( from )->second.items = std::make_shared<VIEW_RTREE>();
    }

    // Visibility and targets moved between ids
    m_layerCachesStale = true;

    SortOrderedLayers();

    // A recolor leaves each retained group on the scene layer of its old id
    if( m_gal && m_gal->HasRetainedScene() )
        MarkStaticDirty();

    for( VIEW_ITEM* item : *m_allItems )
    {
        if( !item )
            continue;

        VIEW_ITEM_DATA* viewData = item->viewPrivData();

        if( !viewData )
            continue;

        std::vector<int> layers = item->ViewGetLayers();
        viewData->saveLayers( layers );

        viewData->reorderGroups( aReorderMap );

        viewData->m_requiredUpdate |= COLOR;
        m_hasPendingItemUpdates = true;
        m_pendingListIncomplete = true;
    }

    UpdateItems();
}


void VIEW::UpdateLayerColor( int aLayer )
{
    // There is no point in updating non-cached layers
    if( !IsCached( aLayer ) )
        return;

    // The layer's fade can follow the same settings as its colours
    m_sceneLayersStale = true;

    BOX2I r;

    r.SetMaximum();

    if( m_gal->IsVisible() )
    {
        GAL_UPDATE_CONTEXT ctx( m_gal );

        if( ctx.IsUpdating() )
        {
            std::vector<std::pair<VIEW_ITEM*, int>> groups;

            auto collect = [&]( VIEW_ITEM* aItem )
            {
                if( int group = aItem->viewPrivData()->getGroup( aLayer ); group >= 0 )
                    groups.emplace_back( aItem, group );

                return true;
            };

            m_layers[aLayer].items->Query( r, collect );

            // Items without a uniform color are drawn again, as in recolorAllLayers()
            for( const auto& [item, group] : groups )
                recolorGroup( item, aLayer, group );

            MarkTargetDirty( m_layers[aLayer].target );

            // A retained scene's ChangeGroupColor() does not re-upload the static buffer
            MarkGeometryDirty();
            MarkStaticDirty();
        }
    }
}


void VIEW::UpdateAllLayersColor()
{
    recolorAllLayers( false );
}


void VIEW::UpdateAllLayersColorAndVisibility()
{
    recolorAllLayers( m_gal->HasRetainedScene() );
}


void VIEW::UpdateLayersColor( bool aVisibility, const std::function<bool( int )>& aLayers )
{
    recolorAllLayers( aVisibility && m_gal->HasRetainedScene(), aLayers );
}


void VIEW::recolorAllLayers( bool aReplace, const std::function<bool( int )>& aLayers )
{
    // A layer's fade can follow the same settings as its colours
    m_sceneLayersStale = true;

    if( aReplace )
    {
        SyncLayerVisibilityCache();
        m_gal->SetSceneScale( m_scale );
        markSceneEdited();
    }

    if( m_gal->IsVisible() )
    {
        GAL_UPDATE_CONTEXT ctx( m_gal );

        if( ctx.IsUpdating() )
        {
            for( VIEW_ITEM* item : *m_allItems )
            {
                if( !item )
                    continue;

                VIEW_ITEM_DATA* viewData = item->viewPrivData();

                if( !viewData )
                    continue;

                for( int layer : viewData->m_layers )
                {
                    if( aLayers && !aLayers( layer ) )
                        continue;

                    int group = viewData->getGroup( layer );

                    if( group >= 0 )
                        recolorGroup( item, layer, group );

                    if( group >= 0 && aReplace )
                        placeCachedGroup( item, layer );
                }
            }
        }
    }

    MarkDirty();
}


int VIEW::GetTopLayer() const
{
    if( m_topLayers.size() == 0 )
        return 0;

    return *m_topLayers.begin();
}


void VIEW::SetTopLayer( int aLayer, bool aEnabled )
{
    if( aEnabled )
    {
        if( m_topLayers.count( aLayer ) == 1 )
            return;

        m_topLayers.insert( aLayer );

        // Move the layer closer to front
        if( m_enableOrderModifier )
            m_layers[aLayer].renderingOrder += TOP_LAYER_MODIFIER;
    }
    else
    {
        if( m_topLayers.count( aLayer ) == 0 )
            return;

        m_topLayers.erase( aLayer );

        // Restore the previous rendering order
        if( m_enableOrderModifier )
            m_layers[aLayer].renderingOrder -= TOP_LAYER_MODIFIER;
    }
}


void VIEW::EnableTopLayer( bool aEnable )
{
    if( aEnable == m_enableOrderModifier )
        return;

    m_enableOrderModifier = aEnable;

    std::set<unsigned int>::iterator it;

    if( aEnable )
    {
        for( it = m_topLayers.begin(); it != m_topLayers.end(); ++it )
            m_layers[*it].renderingOrder += TOP_LAYER_MODIFIER;
    }
    else
    {
        for( it = m_topLayers.begin(); it != m_topLayers.end(); ++it )
            m_layers[*it].renderingOrder -= TOP_LAYER_MODIFIER;
    }

    UpdateAllLayersOrder();
    UpdateAllLayersColor();
}


void VIEW::ClearTopLayers()
{
    std::set<unsigned int>::iterator it;

    if( m_enableOrderModifier )
    {
        // Restore the previous rendering order for layers that were marked as top
        for( it = m_topLayers.begin(); it != m_topLayers.end(); ++it )
            m_layers[*it].renderingOrder -= TOP_LAYER_MODIFIER;
    }

    m_topLayers.clear();
}


void VIEW::UpdateAllLayersOrder()
{
    SortOrderedLayers();

    // A retained scene keeps depth per layer, not per group
    if( m_gal->IsVisible() && !m_gal->HasRetainedScene() )
    {
        GAL_UPDATE_CONTEXT ctx( m_gal );

        if( ctx.IsUpdating() )
        {
            for( VIEW_ITEM* item : *m_allItems )
            {
                if( !item )
                    continue;

                VIEW_ITEM_DATA* viewData = item->viewPrivData();

                if( !viewData )
                    continue;

                for( int layer : viewData->m_layers )
                {
                    int group = viewData->getGroup( layer );

                    if( group >= 0 )
                        m_gal->ChangeGroupDepth( group, m_layers[layer].renderingOrder );
                }
            }
        }
    }

    MarkDirty();
}


struct VIEW::DRAW_ITEM_VISITOR
{
    DRAW_ITEM_VISITOR( VIEW* aView, int aLayer, bool aUseDrawPriority, bool aReverseDrawOrder ) :
        view( aView ),
        layer( aLayer ),
        useDrawPriority( aUseDrawPriority ),
        reverseDrawOrder( aReverseDrawOrder ),
        drawForcedTransparent( false ),
        foundForcedTransparent( false ),
        placing( false )
    {
    }

    bool operator()( VIEW_ITEM* aItem )
    {
        wxCHECK( aItem->viewPrivData(), false );

        if( aItem->m_forcedTransparency > 0 && !drawForcedTransparent )
        {
            foundForcedTransparent = true;

            if( placing )
                drawItems.push_back( aItem );

            return true;
        }

        // A retained scene decides what is drawn itself, so it is told about every item
        if( placing && !drawForcedTransparent )
        {
            drawItems.push_back( aItem );

            return true;
        }

        const double itemLOD = aItem->ViewGetLOD( layer, view );

        view->noteKeptLod( itemLOD );

        // Conditions that have to be fulfilled for an item to be drawn
        bool drawCondition = aItem->viewPrivData()->isRenderable() && view->IsShownAtScale( itemLOD );

        if( !drawCondition )
            return true;

        if( useDrawPriority )
            drawItems.push_back( aItem );
        else
            view->draw( aItem, layer );

        return true;
    }

    void deferredDraw()
    {
        if( placing )
        {
            std::vector<VIEW::LAYER_ITEM_PAIR> items;
            items.reserve( drawItems.size() );

            for( VIEW_ITEM* item : drawItems )
                items.emplace_back( item, layer );

            view->placeRetainedInOrder( items, false );
            return;
        }

        if( reverseDrawOrder )
        {
            std::sort( drawItems.begin(), drawItems.end(),
                       []( VIEW_ITEM* a, VIEW_ITEM* b ) -> bool
                       {
                           return b->viewPrivData()->m_drawPriority
                                  < a->viewPrivData()->m_drawPriority;
                       } );
        }
        else
        {
            std::sort( drawItems.begin(), drawItems.end(),
                       []( VIEW_ITEM* a, VIEW_ITEM* b ) -> bool
                       {
                           return a->viewPrivData()->m_drawPriority
                                  < b->viewPrivData()->m_drawPriority;
                       } );
        }

        for( VIEW_ITEM* item : drawItems )
            view->draw( item, layer );
    }

    VIEW* view;
    int layer, layers[VIEW_MAX_LAYERS];
    bool useDrawPriority, reverseDrawOrder;
    std::vector<VIEW_ITEM*> drawItems;
    bool drawForcedTransparent;
    bool foundForcedTransparent;
    bool placing;
};


void VIEW::placeRetainedInOrder( const std::vector<LAYER_ITEM_PAIR>& aItems, bool aReplacing )
{
    std::vector<RETAINED_ORDER_ITEM> ordered;
    ordered.reserve( aItems.size() );

    for( const auto& [item, layer] : aItems )
    {
        VIEW_ITEM_DATA* data = item->viewPrivData();

        if( !data || !IsCached( layer ) )
            continue;

        if( data->getGroup( layer ) < 0 )
            updateItemGeometry( item, layer, false );

        ordered.push_back( { item, layer, data->getGroup( layer ),
                             retainedOrderKey( item, data->m_drawPriority, data->m_bbox, m_useDrawPriority,
                                               m_reverseDrawOrder, cachedGroupLOD( item, layer ) ) } );
    }

    sortRetainedOrder( this, ordered );

    for( const RETAINED_ORDER_ITEM& entry : ordered )
    {
        if( aReplacing )
            releaseReplacedGroup( entry.item, entry.layer );

        placeCachedGroup( entry.item, entry.layer, &entry.order );
    }

    if( aReplacing )
        releaseReplacedGroups();
}


void VIEW::placeCachedGroup( VIEW_ITEM* aItem, int aLayer,
                             const RETAINED_GROUP_ORDER* aOrder )
{
    VIEW_ITEM_DATA* viewData = aItem->viewPrivData();

    if( !viewData )
        return;

    if( m_deferRetainedPlacement )
    {
        m_deferredRetainedPlacements.emplace_back( aItem, aLayer );
        return;
    }

    if( viewData->getGroup( aLayer ) < 0 )
        updateItemGeometry( aItem, aLayer );

    int group = viewData->getGroup( aLayer );

    if( group < 0 )
        return;

    // cachedGroupLOD() records which other layers the item reads, so showing or hiding one of them
    // places it again. The layer's own visibility is layer state.
    const double lod = aOrder ? aOrder->lodScale : cachedGroupLOD( aItem, aLayer );
    RETAINED_GROUP_ORDER order = aOrder ? *aOrder
                                       : retainedOrderKey( aItem, viewData->m_drawPriority, viewData->m_bbox,
                                                           m_useDrawPriority,
                                                           m_reverseDrawOrder, lod );

    if( !aOrder )
    {
        order.contentHash = m_gal->GroupContentFingerprint( group );
        order.contentTie = true;
    }

    m_gal->SetGroupPlacement( group, aLayer, lod,
                              viewData->isRenderable() && aItem->m_forcedTransparency <= 0, order );
}


double VIEW::cachedGroupLOD( VIEW_ITEM* aItem, int aLayer )
{
    m_lodProbeLayer = aLayer;
    const double lod = aItem->ViewGetLOD( aLayer, this );
    m_lodProbeLayer = -1;
    return lod;
}


void VIEW::redrawRect( const BOX2I& aRect )
{
    SyncLayerVisibilityCache();

    const bool retained = m_gal && m_gal->HasRetainedScene();

    SyncRetainedScene();

    if( retained )
    {
        for( int target : { TARGET_NONCACHED, TARGET_OVERLAY } )
        {
            if( IsTargetDirty( target ) || m_gal->RefillsNonCachedTargets() )
                m_scaleUse[target] = SCALE_USE();
        }
    }

    for( VIEW_LAYER* l : m_orderedLayers )
    {
        if( l->items->IsEmpty() )
            continue;

        const bool placing = retained && l->target == TARGET_CACHED;

        // A backend that empties its non-cached targets on each drawn frame needs a clean
        // target emitted along with the dirty one, or it goes missing
        const bool refilled = l->target != TARGET_CACHED && m_gal && m_gal->RefillsNonCachedTargets();

        // A retained scene keeps its cached layers until the backend asks for all of them again. A
        // pending rebuild empties the static pool even when TARGET_CACHED is clean.
        const bool targetDirty = placing ? m_gal->HasStaticRebuildPending()
                                         : ( IsTargetDirty( l->target ) || refilled
                                             || ( l->target == TARGET_CACHED && m_gal
                                                  && m_gal->HasStaticRebuildPending() ) );

        // A rebuild places the groups of hidden layers too, as the scene leaves those layers out
        if( targetDirty && ( placing || ( l->visible && areRequiredLayersEnabled( l->id ) ) ) )
        {
            DRAW_ITEM_VISITOR drawFunc( this, l->id, m_useDrawPriority, m_reverseDrawOrder );

            m_gal->SetTarget( l->target );
            m_gal->SetLayerDepth( l->renderingOrder );

            // Differential layer also work for the negatives, since both special layer types
            // will composite on separate layers (at least in Cairo)
            if( l->diffLayer )
                m_gal->StartDiffLayer();
            else if( l->hasNegatives )
                m_gal->StartNegativesLayer();

            if( placing )
            {
                drawFunc.placing = true;
                m_gal->BeginSceneLayer( l->id );
            }

            const bool kept = retained && l->target != TARGET_CACHED;

            if( kept )
            {
                m_keptScaleUse = &m_scaleUse[l->target];
                m_scaleRead = false;
                m_gal->TakeWorldScaleRead();

                double below, above;
                m_gal->TakeWorldScaleLimits( below, above );
            }

            l->items->Query( aRect, drawFunc );

            if( m_useDrawPriority || placing )
                drawFunc.deferredDraw();

            m_keptScaleUse = nullptr;

            if( kept && ( m_gal->TakeWorldScaleRead() || m_scaleRead ) )
                m_scaleUse[l->target].read = true;

            // Limits on the world scale become limits on the view's, which is proportional
            if( kept && m_gal->GetWorldScaleForScreenSpace() > 0.0 )
            {
                const double toView = m_scale / m_gal->GetWorldScaleForScreenSpace();
                double       below, above;

                m_gal->TakeWorldScaleLimits( below, above );

                SCALE_USE& use = m_scaleUse[l->target];
                use.lodBelow = std::max( use.lodBelow, below * toView );

                if( above < std::numeric_limits<double>::max() )
                    use.lodAbove = std::min( use.lodAbove, above * toView );
            }

            if( l->diffLayer )
                m_gal->EndDiffLayer();
            else if( l->hasNegatives )
                m_gal->EndNegativesLayer();

            if( drawFunc.foundForcedTransparent )
            {
                drawFunc.drawForcedTransparent = true;

                m_gal->SetTarget( TARGET_NONCACHED );
                m_gal->EnableDepthTest( true );
                m_gal->SetLayerDepth( l->renderingOrder );

                l->items->Query( aRect, drawFunc );
            }
        }
    }
}


void VIEW::draw( VIEW_ITEM* aItem, int aLayer, bool aImmediate )
{
    VIEW_ITEM_DATA* viewData = aItem->viewPrivData();

    if( !viewData )
        return;

    if( m_layerCachedFlagCache[ aLayer ] && !aImmediate )
    {
        int group = viewData->getGroup( aLayer );

        if( group >= 0 )
        {
            m_gal->DrawGroup( group );
        }
        else
        {
            wxLogTrace( traceRhi, wxS( "ViewCache: miss layer=%d item=%p drawingMode=%d" ),
                        aLayer, static_cast<const void*>( aItem ), m_gal->IsDrawingMode() ? 1 : 0 );

            // Only a retained scene builds a missed group mid-frame. Other backends wait for the next
            // update, since opening a group in Cairo ends its drawing surface.
            if( !m_gal->HasRetainedScene() )
            {
                Update( aItem );
                return;
            }

            if( m_gal->IsDrawingMode() )
            {
                updateItemGeometry( aItem, aLayer, false );
                group = viewData->getGroup( aLayer );

                if( group >= 0 )
                {
                    m_gal->DrawGroup( group );
                    return;
                }
            }

            // Outside drawing mode there is no open group to draw into, so rebuild on the next pass
            wxLogTrace( traceRhi, wxS( "ViewCache: defer-build layer=%d item=%p" ),
                        aLayer, static_cast<const void*>( aItem ) );

            Update( aItem, REPAINT );
        }
    }
    else
    {
        // Immediate mode
        if( !m_painter->Draw( aItem, aLayer ) )
            aItem->ViewDraw( aLayer, this );
    }
}


void VIEW::draw( VIEW_ITEM* aItem, bool aImmediate )
{
    std::vector<int> layers = aItem->ViewGetLayers();

    // Sorting is needed for drawing order dependent GALs (like Cairo)
    if( !m_gal || !m_gal->IsOpenGlEngine())
        SortLayers( layers );

    for( int layer : layers )
    {
        auto it = m_layers.find( layer );

        if( it == m_layers.end() )
            continue;

        if( m_gal )
            m_gal->SetLayerDepth( it->second.renderingOrder );

        draw( aItem, layer, aImmediate );
    }
}


void VIEW::draw( VIEW_GROUP* aGroup, bool aImmediate )
{
    for( unsigned int i = 0; i < aGroup->GetSize(); i++ )
        draw( aGroup->GetItem(i), aImmediate );
}


struct VIEW::RECACHE_ITEM_VISITOR
{
    RECACHE_ITEM_VISITOR( VIEW* aView, GAL* aGal, int aLayer ) :
        view( aView ),
        gal( aGal ),
        layer( aLayer )
    {
    }

    bool operator()( VIEW_ITEM* aItem )
    {
        VIEW_ITEM_DATA* viewData = aItem->viewPrivData();

        if( !viewData )
            return false;

        // A retained scene keeps the cached group until the item is drawn again, so its replacement
        // takes the same place in the draw order, see updateItemGeometry()
        if( !gal->HasRetainedScene() )
        {
            int group = viewData->getGroup( layer );

            if( group >= 0 )
                gal->DeleteGroup( group );

            viewData->setGroup( layer, -1 );
        }

        view->Update( aItem, KIGFX::REPAINT );

        return true;
    }

    VIEW* view;
    GAL* gal;
    int layer;
};


void VIEW::Clear()
{
    BOX2I r;
    r.SetMaximum();

    // Invalidate viewPrivData for all items before clearing. This ensures that items
    // which persist outside the view (like selection groups) won't have stale references
    // to this view, which could cause issues if they're later removed and re-added.
    clearPendingItems();

    for( VIEW_ITEM* item : *m_allItems )
    {
        if( item && item->m_viewPrivData )
            item->m_viewPrivData->m_view = nullptr;
    }

    m_allItems->clear();

    for( auto& [_, layer] : m_layers )
        layer.items->RemoveAll();

    m_nextDrawPriority = 0;

    m_gal->ClearCache();
}


void VIEW::ClearTargets()
{
    // A kept target that only the camera moved still holds valid content
    if( m_gal && m_gal->HasRetainedScene() && !m_gal->RefillsNonCachedTargets() )
    {
        for( int target : { TARGET_NONCACHED, TARGET_OVERLAY } )
        {
            if( !m_targetEdited[target] )
                m_dirtyTargets[target] = false;
        }
    }

    // Without a retained scene the cached and non-cached targets share surfaces, so they clear
    // together
    if( m_gal && !m_gal->HasRetainedScene() )
    {
        if( IsTargetDirty( TARGET_CACHED ) || IsTargetDirty( TARGET_NONCACHED ) )
        {
            m_gal->ClearTarget( TARGET_NONCACHED );
            m_gal->ClearTarget( TARGET_CACHED );

            MarkDirty();
        }
    }
    else
    {
        if( IsTargetDirty( TARGET_CACHED ) )
            m_gal->ClearTarget( TARGET_CACHED );

        if( IsTargetDirty( TARGET_NONCACHED ) )
            m_gal->ClearTarget( TARGET_NONCACHED );
    }

    if( IsTargetDirty( TARGET_OVERLAY ) )
    {
        m_gal->ClearTarget( TARGET_OVERLAY );
    }
}


void VIEW::Redraw()
{
#ifdef KICAD_GAL_PROFILE
    PROF_TIMER totalRealTime("view-redraw-total");
    latencyProbeZoomToRender.Checkpoint("view-redraw-start");
#endif /* KICAD_GAL_PROFILE */

    BOX2I recti;

    // A retained scene holds every item, not only the visible ones, so a full redraw visits them all
    if( m_gal && m_gal->HasRetainedScene() && m_gal->IsInitialized() )
    {
        recti.SetMaximum();
    }
    else
    {
        VECTOR2D screenSize = m_gal->GetScreenPixelSize();
        BOX2D    rect( ToWorld( VECTOR2D( 0, 0 ) ),
                       ToWorld( screenSize ) - ToWorld( VECTOR2D( 0, 0 ) ) );

        rect.Normalize();
        recti = BOX2ISafe( rect );
    }

    redrawRect( recti );

    // All targets were redrawn, so nothing is dirty
    MarkClean();

#ifdef KICAD_GAL_PROFILE
    totalRealTime.Stop();
    wxLogTrace( traceGalProfile, wxS( "VIEW::Redraw(): %.1f ms" ), totalRealTime.msecs() );
    latencyProbeZoomToRender.AddTimer( totalRealTime );
    latencyProbeZoomToRender.Checkpoint("view-redraw-end");
    
#endif /* KICAD_GAL_PROFILE */
}


const VECTOR2I& VIEW::GetScreenPixelSize() const
{
    return m_gal->GetScreenPixelSize();
}


struct VIEW::CLEAR_LAYER_CACHE_VISITOR
{
    CLEAR_LAYER_CACHE_VISITOR( VIEW* aView ) :
        view( aView )
    {
    }

    bool operator()( VIEW_ITEM* aItem )
    {
        aItem->viewPrivData()->deleteGroups();

        return true;
    }

    VIEW* view;
};


void VIEW::clearGroupCache()
{
    BOX2I r;

    r.SetMaximum();
    CLEAR_LAYER_CACHE_VISITOR visitor( this );

    for( auto& [_, layer] : m_layers )
        layer.items->Query( r, visitor );
}


void VIEW::invalidateItem( VIEW_ITEM* aItem, int aUpdateFlags )
{
    // updateLayers updates geometry too, so we do not have to update both of them at the
    // same time
    if( aUpdateFlags & LAYERS )
        updateLayers( aItem );
    else if( aUpdateFlags & GEOMETRY )
        updateBbox( aItem );

    // Now that we have initialized, set flags to ALL for the code below
    if( aUpdateFlags & INITIAL_ADD )
        aUpdateFlags = ALL;

    std::vector<int> layers = aItem->ViewGetLayers();
    bool             cachedMutation = false;
    const bool       retained = m_gal && m_gal->HasRetainedScene();

    // Iterate through layers used by the item and recache it immediately
    for( int layer : layers )
    {
        if( IsCached( layer ) )
        {
            cachedMutation = true;

            if( aUpdateFlags & ( GEOMETRY | LAYERS | REPAINT ) )
                updateItemGeometry( aItem, layer );
            else if( aUpdateFlags & COLOR )
                updateItemColor( aItem, layer );

            // Visibility is a placement, the group itself is unchanged
            if( retained && ( aUpdateFlags & APPEARANCE ) )
                placeCachedGroup( aItem, layer );
        }

        // Mark those layers as dirty, so the VIEW will be refreshed
        MarkTargetDirty( m_layers[layer].target );
    }

    if( retained && !m_useDrawPriority && aItem->m_forcedTransparency <= 0 )
    {
        // The groups above are already patched into the scene, but a non-cached layer still needs
        // the frame off the camera-reuse path
        markSceneEdited();
    }
    else if( aUpdateFlags & ( GEOMETRY | LAYERS | REPAINT ) )
    {
        MarkGeometryDirty();
    }
    else if( cachedMutation && ( aUpdateFlags & ( APPEARANCE | COLOR ) ) )
    {
        // A retained scene patches a color change into the group. Visibility changes cannot be
        // patched that way and rebuild.
        const bool colorOnlyPatchable = ( aUpdateFlags & COLOR ) && !( aUpdateFlags & APPEARANCE )
                                        && m_gal && m_gal->HasRetainedScene();

        if( !colorOnlyPatchable )
            MarkGeometryDirty();
    }

    aItem->viewPrivData()->clearUpdateFlags();
}


void VIEW::SortOrderedLayers()
{
    int n = 0;

    m_orderedLayers.resize( m_layers.size() );

    for( auto& [layer_id, layer] : m_layers )
        m_orderedLayers[n++] = &layer;

    sort( m_orderedLayers.begin(), m_orderedLayers.end(), compareRenderingOrder );

    // A retained scene takes the new order as layer state
    m_sceneLayersStale = true;

    MarkDirty();
}


void VIEW::recolorGroup( VIEW_ITEM* aItem, int aLayer, int aGroup )
{
    if( m_painter->HasUniformColor( aItem, aLayer ) )
    {
        m_gal->ChangeGroupColor( aGroup, m_painter->GetSettings()->GetColor( aItem, aLayer ) );
        markDimExemption( aItem, aGroup );
    }
    else
    {
        updateItemGeometry( aItem, aLayer );
    }
}


void VIEW::markDimExemption( const VIEW_ITEM* aItem, int aGroup )
{
    if( m_gal->HasRetainedScene() )
        m_gal->ChangeGroupUndimmed( aGroup, m_painter->GetSettings()->IsDimExempt( aItem ) );
}


void VIEW::updateItemColor( VIEW_ITEM* aItem, int aLayer )
{
    VIEW_ITEM_DATA* viewData = aItem->viewPrivData();
    wxCHECK( IsCached( aLayer ), /*void*/ ); // This will check if the layer exists

    if( !viewData )
        return;

    int group = viewData->getGroup( aLayer );

    if( group >= 0 )
        recolorGroup( aItem, aLayer, group );
}


void VIEW::updateItemGeometry( VIEW_ITEM* aItem, int aLayer, bool aPlace )
{
    VIEW_ITEM_DATA* viewData = aItem->viewPrivData();

    if( !viewData )
        return;

    auto it = m_layers.find( aLayer );

    if( it == m_layers.end() )
        return;

    VIEW_LAYER& l = it->second;

    // Save the extra map lookup in IsCached by open coding here
    if( l.target != TARGET_CACHED )
        return;

    int group = viewData->getGroup( aLayer );

    m_gal->SetTarget( l.target );
    m_gal->SetLayerDepth( l.renderingOrder );

    // A batch draws all of its items again before it places any. Deleting each old group here
    // would hand the freed slots back in reverse, so the old group waits for its replacement.
    const bool keepForPlacement = group >= 0 && aPlace && m_deferRetainedPlacement && m_gal->HasRetainedScene()
                                  && m_replacedRetainedGroups.emplace( std::make_pair( aItem, aLayer ), group ).second;

    if( group >= 0 && !keepForPlacement )
        m_gal->DeleteGroup( group );

    group = m_gal->BeginGroup();
    viewData->setGroup( aLayer, group );

    wxLogTrace( traceGalGroups, wxS( "updateItemGeometry() item=%p layer=%d newGroup=%d" ),
                static_cast<void*>( aItem ), aLayer, group );

    if( !m_painter->Draw( aItem, aLayer ) )
        aItem->ViewDraw( aLayer, this );

    m_gal->EndGroup();
    markDimExemption( aItem, group );

    if( aPlace && m_gal->HasRetainedScene() )
        placeCachedGroup( aItem, aLayer );
}


void VIEW::releaseReplacedGroup( VIEW_ITEM* aItem, int aLayer )
{
    auto it = m_replacedRetainedGroups.find( std::make_pair( aItem, aLayer ) );

    if( it == m_replacedRetainedGroups.end() )
        return;

    m_gal->DeleteGroup( it->second );
    m_replacedRetainedGroups.erase( it );
}


void VIEW::releaseReplacedGroups()
{
    for( const auto& [key, group] : m_replacedRetainedGroups )
        m_gal->DeleteGroup( group );

    m_replacedRetainedGroups.clear();
}


void VIEW::updateBbox( VIEW_ITEM* aItem )
{
    std::vector<int> layers = aItem->ViewGetLayers();

    wxASSERT( aItem->m_viewPrivData ); //must have a viewPrivData

    const BOX2I  new_bbox = aItem->ViewBBox();

    // The R-tree removal below keys on the bbox the item was inserted with, so it must be
    // copied before the m_bbox overwrite that follows rather than aliased to it. Otherwise
    // a moved item is removed with the wrong box and the R-tree falls back to a full-tree
    // search instead of the targeted removal.
    const BOX2I  old_bbox = aItem->m_viewPrivData->m_bbox;

    if( new_bbox == old_bbox )
        return;

    aItem->m_viewPrivData->m_bbox = new_bbox;

    for( int layer : layers )
    {
        auto it = m_layers.find( layer );

        if( it == m_layers.end() )
            continue;

        VIEW_LAYER& l = it->second;
        l.items->Remove( aItem, &old_bbox );
        l.items->Insert( aItem, new_bbox );
        MarkTargetDirty( l.target );
    }

    markSceneEdited();
}


void VIEW::updateLayers( VIEW_ITEM* aItem )
{
    VIEW_ITEM_DATA* viewData = aItem->viewPrivData();

    if( !viewData )
        return;

    const BOX2I  new_bbox = aItem->ViewBBox();
    std::vector<int> newLayers = aItem->ViewGetLayers();

    // If neither the layers nor the bbox have changed, skip the expensive R-tree operations
    if( newLayers == viewData->m_layers && new_bbox == viewData->m_bbox )
        return;

    // Remove the item from previous layer set
    const BOX2I& old_bbox = aItem->m_viewPrivData->m_bbox;

    for( int layer : aItem->m_viewPrivData->m_layers )
    {
        auto it = m_layers.find( layer );

        if( it == m_layers.end() )
            continue;

        VIEW_LAYER& l = it->second;
        l.items->Remove( aItem, &old_bbox );
        MarkTargetDirty( l.target );

        if( IsCached( l.id ) )
        {
            // Redraw the item from scratch
            int prevGroup = viewData->getGroup( layer );

            if( prevGroup >= 0 )
            {
                m_gal->DeleteGroup( prevGroup );
                viewData->setGroup( l.id, -1 );
            }
        }
    }

    aItem->m_viewPrivData->m_bbox = new_bbox;
    viewData->saveLayers( newLayers );

    for( int layer : newLayers )
    {
        auto it = m_layers.find( layer );

        if( it == m_layers.end() )
            continue;

        VIEW_LAYER& l = it->second;
        l.items->Insert( aItem, new_bbox );
        MarkTargetDirty( l.target );
    }

    markSceneEdited();
}


bool VIEW::areRequiredLayersEnabled( int aLayerId ) const
{
    auto it = m_layers.find( aLayerId );

    if( it == m_layers.end() )
        return false;

    for( int layer : it->second.requiredLayers )
    {
        // That is enough if just one layer is not enabled

        auto it2 = m_layers.find( layer );

        if( it2 == m_layers.end() || !it2->second.visible )
            return false;

        if( !areRequiredLayersEnabled( layer ) )
            return false;
    }

    return true;
}


void VIEW::RecacheAllItems()
{
    BOX2I r;

    r.SetMaximum();

    for( const auto& [_, l] : m_layers )
    {
        if( IsCached( l.id ) )
        {
            RECACHE_ITEM_VISITOR visitor( this, m_gal, l.id );
            l.items->Query( r, visitor );
        }
    }
}


void VIEW::UpdateItems()
{
    if( !m_gal->IsVisible() || !m_gal->IsInitialized() )
    {
        wxLogTrace( traceGalGroups,
                    wxS( "UpdateItems() SKIP: visible=%d initialized=%d" ),
                    m_gal->IsVisible(), m_gal->IsInitialized() );
        return;
    }

#ifdef KICAD_GAL_PROFILE
    latencyProbeZoomToRender.Checkpoint("view-update-items");
#endif

    if( !m_hasPendingItemUpdates )
        return;

    // Groups cached below are placed against the scale they will be drawn at
    if( m_gal->HasRetainedScene() )
        m_gal->SetSceneScale( m_scale );

    unsigned int cntGeomUpdate = 0;
    bool         anyUpdated = false;

    // Other backends scan every item, since the shape of their R-trees depends on that order
    const bool retained = m_gal->HasRetainedScene();
    const bool listed = retained && !m_pendingListIncomplete;

    const std::vector<VIEW_ITEM*>& flagged = listed ? m_pendingItems : *m_allItems;

    for( VIEW_ITEM* item : flagged )
    {
        if( !item )
            continue;

        auto vpd = item->viewPrivData();

        if( !vpd )
            continue;

        if( vpd->m_requiredUpdate != NONE )
        {
            anyUpdated = true;

            if( vpd->m_requiredUpdate & ( GEOMETRY | LAYERS ) )
                cntGeomUpdate++;
        }
    }

    unsigned int cntTotal = m_allItems->size();

    double ratio = (double) cntGeomUpdate / (double) cntTotal;

    // R*-tree individual inserts use forced reinsertion on node overflow, making them
    // significantly more expensive than simple R-tree inserts. At ~5% changed items
    // the cost of individual Remove+Insert operations exceeds a full bulk rebuild.
    if( ratio > 0.05 )
    {
        auto allItems = *m_allItems;

        // Clear all R-trees
        for( auto& [_, layer] : m_layers )
            layer.items->RemoveAll();

        // Collect items per layer for bulk loading
        std::unordered_map<int, std::vector<std::pair<VIEW_ITEM*, BOX2I>>> layerBulk;

        for( VIEW_ITEM* item : allItems )
        {
            if( !item )
                continue;

            const BOX2I bbox = item->ViewBBox();
            item->m_viewPrivData->m_bbox = bbox;

            std::vector<int> layers = item->ViewGetLayers();
            item->viewPrivData()->saveLayers( layers );

            for( int layer : layers )
            {
                auto it = m_layers.find( layer );

                wxCHECK2_MSG( it != m_layers.end(), continue, wxS( "Invalid layer" ) );
                layerBulk[layer].emplace_back( item, bbox );
            }

            // The bulk rebuild handled R-tree reinsertion, so clear LAYERS|GEOMETRY
            // to avoid redundant R-tree work in invalidateItem(). Replace with REPAINT
            // so items still reach updateItemGeometry() for GAL cache rebuilds.
            if( item->viewPrivData()->m_requiredUpdate & ( LAYERS | GEOMETRY ) )
            {
                item->viewPrivData()->m_requiredUpdate &= ~( LAYERS | GEOMETRY );
                item->viewPrivData()->m_requiredUpdate |= REPAINT;
            }
        }

        // Bulk load each layer's R-tree
        for( auto& [layerId, items] : layerBulk )
        {
            auto it = m_layers.find( layerId );

            if( it != m_layers.end() )
            {
                it->second.items->BulkLoad( items );
                MarkTargetDirty( it->second.target );
            }
        }

        markSceneEdited();
    }

    // Skipping the loop below leaves m_requiredUpdate and m_hasPendingItemUpdates set so the
    // work is redone once the canvas has a valid context again
    bool updatesProcessed = true;

    if( anyUpdated )
    {
        unsigned int cntInvalidated = 0;
        GAL_UPDATE_CONTEXT ctx( m_gal );

        if( ctx.IsUpdating() )
        {
            auto invalidate = [&]( VIEW_ITEM* item )
            {
                if( item && item->viewPrivData() && item->viewPrivData()->m_requiredUpdate != NONE )
                {
                    cntInvalidated++;
                    invalidateItem( item, item->viewPrivData()->m_requiredUpdate );
                    item->viewPrivData()->m_requiredUpdate = NONE;
                }
            };

            if( retained )
            {
                if( m_gal->HasStaticRebuildPending() )
                {
                    // The full redraw places every cached group, including those hidden at this scale
                    m_deferRetainedPlacement = true;

                    for( size_t next = 0; next < flagged.size(); ++next )
                        invalidate( flagged[next] );

                    m_deferRetainedPlacement = false;
                    m_deferredRetainedPlacements.clear();
                    releaseReplacedGroups();
                }
                else
                {
                    size_t next = 0;

                    // An invalidation may queue another item, so sort each newly queued batch.
                    while( next < flagged.size() )
                    {
                        std::vector<RETAINED_ORDER_ITEM> batch;

                        while( next < flagged.size() )
                        {
                            VIEW_ITEM* item = flagged[next++];

                            if( !item || !item->viewPrivData() || item->viewPrivData()->m_requiredUpdate == NONE )
                                continue;

                            const VIEW_ITEM_DATA* data = item->viewPrivData();
                            const int front = data->m_layers.empty() ? -1 : data->m_layers.front();

                            batch.push_back(
                                    { item, front, front < 0 ? -1 : data->getGroup( front ),
                                      retainedOrderKey( item, data->m_drawPriority, data->m_bbox, m_useDrawPriority,
                                                        m_reverseDrawOrder,
                                                        data->m_layers.empty()
                                                                ? 0.0
                                                                : cachedGroupLOD( item, data->m_layers.front() ) ) } );
                        }

                        std::sort( batch.begin(), batch.end(),
                                   []( const RETAINED_ORDER_ITEM& a, const RETAINED_ORDER_ITEM& b )
                                   {
                                       return a.order < b.order;
                                   } );

                        m_deferRetainedPlacement = true;

                        for( const RETAINED_ORDER_ITEM& entry : batch )
                            invalidate( entry.item );

                        m_deferRetainedPlacement = false;

                        placeRetainedInOrder( std::exchange( m_deferredRetainedPlacements, {} ), true );
                    }
                }
            }
            else
            {
                // Invalidating an item can flag another, which grows the list being walked.
                for( size_t ii = 0; ii < flagged.size(); ++ii )
                    invalidate( flagged[ii] );
            }
        }
        else
        {
            updatesProcessed = false;
        }

        wxLogTrace( traceGalGroups, wxS( "UpdateItems() invalidated %u items" ), cntInvalidated );
    }

    wxLogTrace( traceGalGroups,
                wxS( "UpdateItems() total=%u geomUpdate=%u anyUpdated=%d bulkRebuild=%d" ),
                cntTotal, cntGeomUpdate, anyUpdated, ratio > 0.05 );

#ifdef KICAD_GAL_PROFILE
    wxLogTrace( traceGalProfile, wxS( "View update: total items %u, geom %u anyUpdated %u" ),
                cntTotal, cntGeomUpdate, (unsigned) anyUpdated );
#endif

    if( updatesProcessed )
    {
        m_hasPendingItemUpdates = false;
        m_pendingListIncomplete = false;
        clearPendingItems();
    }
}


void VIEW::clearPendingItems()
{
    for( VIEW_ITEM* item : m_pendingItems )
    {
        if( item && item->viewPrivData() )
            item->viewPrivData()->m_pendingIndex = -1;
    }

    m_pendingItems.clear();
}


void VIEW::UpdateAllItems( int aUpdateFlags )
{
    if( aUpdateFlags == NONE )
        return;

    for( VIEW_ITEM* item : *m_allItems )
    {
        if( item && item->viewPrivData() )
        {
            item->viewPrivData()->m_requiredUpdate |= aUpdateFlags;
            m_hasPendingItemUpdates = true;
            m_pendingListIncomplete = true;
        }
    }
}


void VIEW::UpdateAllItemsConditionally( int aUpdateFlags,
                                        std::function<bool( VIEW_ITEM* )> aCondition )
{
    if( aUpdateFlags == NONE )
        return;

    for( VIEW_ITEM* item : *m_allItems )
    {
        if( !item )
            continue;

        if( aCondition( item ) )
        {
            if( item->viewPrivData() )
            {
                item->viewPrivData()->m_requiredUpdate |= aUpdateFlags;
                m_hasPendingItemUpdates = true;
                m_pendingListIncomplete = true;
            }
        }
    }
}


void VIEW::UpdateAllItemsConditionally( std::function<int( VIEW_ITEM* )> aItemFlagsProvider )
{
    for( VIEW_ITEM* item : *m_allItems )
    {
        if( !item )
            continue;

        if( item->viewPrivData() )
        {
            int flags = aItemFlagsProvider( item );
            item->viewPrivData()->m_requiredUpdate |= flags;

            if( flags != NONE )
            {
                m_hasPendingItemUpdates = true;
                m_pendingListIncomplete = true;
            }
        }
    }
}



std::unique_ptr<VIEW> VIEW::DataReference() const
{
    std::unique_ptr<VIEW> ret = std::make_unique<VIEW>();
    ret->m_allItems = m_allItems;
    ret->m_layers = m_layers;
    ret->m_hasPendingItemUpdates = m_hasPendingItemUpdates;
    ret->m_pendingListIncomplete = true;
    ret->m_cameraDirtyOnly = m_cameraDirtyOnly;
    ret->SortOrderedLayers();
    return ret;
}


void VIEW::SetVisible( VIEW_ITEM* aItem, bool aIsVisible )
{
    if( !aItem )
        return;

    VIEW_ITEM_DATA* viewData = aItem->viewPrivData();

    if( !viewData )
        return;

    bool cur_visible = viewData->m_flags & VISIBLE;

    if( cur_visible != aIsVisible )
    {
        if( aIsVisible )
            viewData->m_flags |= VISIBLE;
        else
            viewData->m_flags &= ~VISIBLE;

        Update( aItem, APPEARANCE | COLOR );
    }
}


void VIEW::Hide( VIEW_ITEM* aItem, bool aHide, bool aHideOverlay )
{
    if( !aItem )
        return;

    VIEW_ITEM_DATA* viewData = aItem->viewPrivData();

    if( !viewData )
        return;

    if( !( viewData->m_flags & VISIBLE ) )
        return;

    if( aHideOverlay )
        viewData->m_flags |= OVERLAY_HIDDEN;

    if( aHide )
        viewData->m_flags |= HIDDEN;
    else
        viewData->m_flags &= ~( HIDDEN | OVERLAY_HIDDEN );

    Update( aItem, APPEARANCE );
}


bool VIEW::IsVisible( const VIEW_ITEM* aItem ) const
{
    const VIEW_ITEM_DATA* viewData = aItem->viewPrivData();

    return viewData && ( viewData->m_flags & VISIBLE );
}


bool VIEW::IsHiddenOnOverlay( const VIEW_ITEM* aItem ) const
{
    const VIEW_ITEM_DATA* viewData = aItem->viewPrivData();

    return viewData && ( viewData->m_flags & OVERLAY_HIDDEN );
}


bool VIEW::HasItem( const VIEW_ITEM* aItem ) const
{
    const VIEW_ITEM_DATA* viewData = aItem->viewPrivData();

    return viewData && viewData->m_view == this;
}


void VIEW::Update( const VIEW_ITEM* aItem ) const
{
    Update( aItem, ALL );
}


void VIEW::Update( const VIEW_ITEM* aItem, int aUpdateFlags ) const
{
    VIEW_ITEM_DATA* viewData = aItem->viewPrivData();

    if( !viewData )
        return;

    assert( aUpdateFlags != NONE );

    viewData->m_requiredUpdate |= aUpdateFlags;
    m_hasPendingItemUpdates = true;

    // An item records a single list position, so only this view's own items are listed
    if( viewData->m_view != this )
    {
        m_pendingListIncomplete = true;
    }
    else if( viewData->m_pendingIndex < 0 )
    {
        viewData->m_pendingIndex = static_cast<int>( m_pendingItems.size() );
        m_pendingItems.push_back( const_cast<VIEW_ITEM*>( aItem ) );
    }
}


void VIEW::RepaintLayers( const BOX2I& aRect, const std::function<bool( int )>& aLayerFilter,
                          const std::function<bool( VIEW_ITEM*, int )>& aItemFilter )
{
    if( !m_gal->IsVisible() || !m_gal->IsInitialized() )
        return;

    std::vector<LAYER_ITEM_PAIR> stale;

    for( VIEW_LAYER* l : m_orderedLayers )
    {
        if( l->target != TARGET_CACHED || !l->visible || !aLayerFilter( l->id ) )
            continue;

        auto collect = [&]( VIEW_ITEM* aItem ) -> bool
        {
            if( aItemFilter( aItem, l->id ) )
                stale.emplace_back( aItem, l->id );

            return true;
        };

        l->items->Query( aRect, collect );
    }

    if( stale.empty() )
        return;

    GAL_UPDATE_CONTEXT ctx( m_gal );

    if( !ctx.IsUpdating() )
        return;

    if( m_gal->HasRetainedScene() )
        m_gal->SetSceneScale( m_scale );

    for( const LAYER_ITEM_PAIR& pair : stale )
        updateItemGeometry( pair.first, pair.second );

    if( m_gal->HasRetainedScene() && !m_useDrawPriority )
    {
        // The camera path uploads the already placed groups, so a zoom and the net names it redraws
        // still go out as one camera frame
        const bool cameraOnly = m_cameraDirtyOnly || !IsDirty();

        MarkTargetDirty( TARGET_CACHED );
        m_cameraDirtyOnly = cameraOnly;
        return;
    }

    MarkTargetDirty( TARGET_CACHED );
    markSceneEdited();
}


std::shared_ptr<VIEW_OVERLAY> VIEW::MakeOverlay()
{
    std::shared_ptr<VIEW_OVERLAY> overlay = std::make_shared<VIEW_OVERLAY>();

    Add( overlay.get() );
    return overlay;
}


void VIEW::ClearPreview()
{
    if( !m_preview )
        return;

    m_preview->Clear();

    for( VIEW_ITEM* item : m_ownedItems )
        delete item;

    m_ownedItems.clear();
    Update( m_preview.get() );
}


void VIEW::InitPreview()
{
   m_preview.reset( new KIGFX::VIEW_GROUP() );
   Add( m_preview.get() );
}


void VIEW::AddToPreview( VIEW_ITEM* aItem, bool aTakeOwnership )
{
   Hide( aItem, false );
   m_preview->Add( aItem );

   if( aTakeOwnership )
       m_ownedItems.push_back( aItem );

   SetVisible( m_preview.get(), true );
   Hide( m_preview.get(), false );
   Update( m_preview.get() );
}


void VIEW::ShowPreview( bool aShow )
{
   SetVisible( m_preview.get(), aShow );
}


void VIEW::SyncLayerVisibilityCache()
{
    // Tools ask for this once per item they look at, far more often than a layer changes
    if( !m_layerCachesStale )
        return;

    for( const auto& layer : m_layers )
    {
        m_layerVisibilityCache[ layer.first ] = layer.second.visible;
        m_layerCachedFlagCache[ layer.first ] = (layer.second.target == TARGET_CACHED);
    }

    m_layerCachesStale = false;
}

} // namespace KIGFX
