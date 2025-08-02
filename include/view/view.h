/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2013-2016 CERN
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
 *
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

#pragma once

#include <gal/gal.h>
#include <vector>
#include <set>
#include <unordered_map>
#include <memory>

#include <math/box2.h>
#include <gal/definitions.h>

#include <view/view_overlay.h>

namespace KIGFX
{
class PAINTER;
class GAL;
class VIEW_ITEM;
class VIEW_GROUP;
class VIEW_RTREE;

/**
 * Hold a (potentially large) number of VIEW_ITEMs and renders them on a graphics device
 * provided by the GAL.
 *
 * VIEWs can exist in two flavors:
 * - dynamic - where items can be added, removed or changed anytime, intended for the main
 *   editing panel. Each VIEW_ITEM can be added to a single dynamic view.
 * - static - where items are added once at the startup and are not linked with the VIEW.
 *   Foreseen for preview windows and printing.
 *
 * Items in a view are grouped in layers (not to be confused with Kicad's PCB layers). Each
 * layer is identified by an integer number. Visibility and rendering order can be set
 * individually for each of the layers. Future versions of the VIEW will also allows one to
 * assign different layers to different rendering targets, which will be composited at the
 * final stage by the GAL.  The VIEW class also provides fast methods for finding all visible
 * objects that are within a given rectangular area, useful for object selection/hit testing.
 */
class GAL_API VIEW
{
public:
    friend class VIEW_ITEM;

    typedef std::pair<VIEW_ITEM*, int> LAYER_ITEM_PAIR;

    VIEW();
    virtual ~VIEW();

    // We own at least one list of raw pointers.  Don't let the compiler fill in copy c'tors that
    // will only land us in trouble.
    VIEW( const VIEW& ) = delete;
    VIEW& operator=( const VIEW& ) = delete;

    /**
     * Nasty hack, invoked by the destructor of VIEW_ITEM to auto-remove the item
     * from the owning VIEW if there is any.
     *
     * KiCad relies too much on this mechanism.  This is the only linking dependency now
     * between #EDA_ITEM and VIEW class. In near future I'll replace it with observers.
     */
    static void OnDestroy( VIEW_ITEM* aItem );

    /**
     * Add a #VIEW_ITEM to the view.
     *
     * Set \a aDrawPriority to -1 to assign sequential priorities.
     *
     * @param aItem: item to be added. No ownership is given
     * @param aDrawPriority: priority to draw this item on its layer, lowest first.
     */
    virtual void Add( VIEW_ITEM* aItem, int aDrawPriority = -1 );

    /**
     * Remove a #VIEW_ITEM from the view.
     *
     * @param aItem: item to be removed. Caller must dispose the removed item if necessary
     */
    virtual void Remove( VIEW_ITEM* aItem );


    /**
     * Find all visible items that touch or are within the rectangle \a aRect.
     *
     * @param aResult result of the search, containing VIEW_ITEMs associated with their layers.
     *                Sorted according to the rendering order (items that are on top of the
     *                rendering stack as first).
     * @return Number of found items.
     */
    int Query( const BOX2I& aRect, std::vector<LAYER_ITEM_PAIR>& aResult ) const;

    /**
     * Run a function on all visible items that touch or are within the rectangle \a aRect.
     *
     * @param aFunc the function to be executed; return true to continue, false to end query.
     */
    void Query( const BOX2I& aRect, const std::function<bool( VIEW_ITEM* )>& aFunc ) const;

    /**
     * Set the item visibility.
     *
     * @param aItem: the item to modify.
     * @param aIsVisible: whether the item is visible (on all layers), or not.
     */
    void SetVisible( VIEW_ITEM* aItem, bool aIsVisible = true );

    /**
     * Temporarily hide the item in the view (e.g. for overlaying).
     *
     * @param aItem: the item to modify.
     * @param aHide: whether the item is hidden, or not.
     * @param aHideOverlay: whether the item should also be hidden on overlays.
     */
    void Hide( VIEW_ITEM* aItem, bool aHide = true, bool aHideOverlay = false );

    /**
     * Return information if the item is visible (or not).
     *
     * @param aItem: the item to test.
     * @return when true, the item is visible (i.e. to be displayed, not visible in the
     * *current* viewport)
     */
    bool IsVisible( const VIEW_ITEM* aItem ) const;

    bool IsHiddenOnOverlay( const VIEW_ITEM* aItem ) const;

    /**
     * Indicates whether or not the given item has been added to the view.
     */
    bool HasItem( const VIEW_ITEM* aItem ) const;

    /**
     * For dynamic VIEWs, inform the associated VIEW that the graphical representation of
     * this item has changed. For static views calling has no effect.
     *
     * @param aItem: the item to update.
     * @param aUpdateFlags: how much the object has changed.
     */
    virtual void Update( const VIEW_ITEM* aItem, int aUpdateFlags ) const;
    virtual void Update( const VIEW_ITEM* aItem ) const;

    /**
     * Mark the \a aRequiredId layer as required for the aLayerId layer. In order to display the
     * layer, all of its required layers have to be enabled.
     *
     * @param aLayerId is the id of the layer for which we enable/disable the required layer.
     * @param aRequiredId is the id of the required layer.
     * @param aRequired tells if the required layer should be added or removed from the list.
     */
    void SetRequired( int aLayerId, int aRequiredId, bool aRequired = true );

    /**
     * Copy layers and visibility settings from another view.
     *
     * @param aOtherView: view from which settings will be copied.
     */
    void CopySettings( const VIEW* aOtherView );

    /*
     *  Convenience wrappers for adding multiple items
     *  template <class T> void AddItems( const T& aItems );
     *  template <class T> void RemoveItems( const T& aItems );
     */

    /**
     * Assign a rendering device for the VIEW.
     *
     * @param aGal: pointer to the GAL output device.
     */
    void SetGAL( GAL* aGal );

    /**
     * Return the #GAL this view is using to draw graphical primitives.
     *
     * @return Pointer to the currently used GAL instance.
     */
    inline GAL* GetGAL() const
    {
        return m_gal;
    }

    /**
     * Set the painter object used by the view for drawing #VIEW_ITEMS.
     */
    inline void SetPainter( PAINTER* aPainter )
    {
        m_painter = aPainter;
    }

    /**
     * Return the painter object used by the view for drawing #VIEW_ITEMS.
     *
     * @return Pointer to the currently used Painter instance.
     */
    inline PAINTER* GetPainter() const
    {
        return m_painter;
    }

    /**
     * Set the visible area of the VIEW.
     *
     * @param aViewport: desired visible area, in world space coordinates.
     */
    void SetViewport( const BOX2D& aViewport );

    /**
     * Return the current viewport visible area rectangle.
     *
     * @return Current viewport rectangle.
     */
    BOX2D GetViewport() const;

    /**
     * Control the mirroring of the VIEW.
     *
     * @param aMirrorX: when true, the X axis is mirrored.
     * @param aMirrorY: when true, the Y axis is mirrored.
     */
    void SetMirror( bool aMirrorX, bool aMirrorY );

    /**
     * Return true if view is flipped across the X axis.
     */
    bool IsMirroredX() const
    {
        return m_mirrorX;
    }

    /**
     * Return true if view is flipped across the Y axis.
     */
    bool IsMirroredY() const
    {
        return m_mirrorY;
    }

    /**
     * Set the scaling factor, zooming around a given anchor point.
     *
     * (depending on correct GAL unit length & DPI settings).
     *
     * @param aAnchor is the zooming  anchor point.
     * @param aScale is the scale factor.
     */
    virtual void SetScale( double aScale, VECTOR2D aAnchor = { 0, 0 } );

    /**
     * @return Current scale factor of this VIEW.
     */
    inline double GetScale() const
    {
        return m_scale;
    }

    /**
     * Set limits for view area.
     *
     * @param aBoundary is the box that limits view area.
     */
    inline void SetBoundary( const BOX2D& aBoundary )
    {
        m_boundary = aBoundary;
    }

     /**
     * Set limits for view area.
     *
     * @param aBoundary is the box that limits view area.
     */
    inline void SetBoundary( const BOX2I& aBoundary )
    {
        m_boundary.SetOrigin( aBoundary.GetOrigin() );
        m_boundary.SetEnd( aBoundary.GetEnd() );
    }

    /**
     * @return Current view area boundary.
     */
    inline const BOX2D& GetBoundary() const
    {
        return m_boundary;
    }

    /**
     * Set minimum and maximum values for scale.
     *
     * @param aMaximum is the maximum value for scale.
     * @param aMinimum is the minimum value for scale.
     */
    void SetScaleLimits( double aMaximum, double aMinimum )
    {
        wxASSERT_MSG( aMaximum > aMinimum, wxT( "I guess you passed parameters in wrong order" ) );

        m_minScale = aMinimum;
        m_maxScale = aMaximum;
    }

    /**
     * Set the center point of the VIEW (i.e. the point in world space that will be drawn in
     * the middle of the screen).
     *
     * @param aCenter: the new center point, in world space coordinates.
     */
    void SetCenter( const VECTOR2D& aCenter );

    /**
     * Set the center point of the VIEW, attempting to avoid \a obscuringScreenRects (for
     * instance, the screen rect of a modeless dialog in front of the VIEW).
     *
     * @param aCenter: the new center point, in world space coordinates.
     * @param obscuringScreenRects: the obscuring rects, in screen space coordinates.
     */
    void SetCenter( const VECTOR2D& aCenter, const std::vector<BOX2D>& obscuringScreenRects );

    /**
     * Return the center point of this VIEW (in world space coordinates).
     *
     * @return center point of the view
     */
    const VECTOR2D& GetCenter() const
    {
        return m_center;
    }

    /**
     * Converts a screen space point/vector to a point/vector in world space coordinates.
     *
     * @param aCoord is the point/vector to be converted.
     * @param aAbsolute when true aCoord is treated as a point, otherwise as a direction (vector).
     */
    VECTOR2D ToWorld( const VECTOR2D& aCoord, bool aAbsolute = true ) const;

    /**
     * Converts a screen space one dimensional size to a one dimensional size in world
     * space coordinates.
     *
     * @param aSize is the size to be converted.
     */
    double ToWorld( double aSize ) const;

    /**
     * Convert a world space point/vector to a point/vector in screen space coordinates.
     *
     * @param aCoord is the point/vector to be converted.
     * @param aAbsolute when true aCoord is treated as a point, otherwise as a direction (vector).
     */
    VECTOR2D ToScreen( const VECTOR2D& aCoord, bool aAbsolute = true ) const;

    /**
     * Convert a world space one dimensional size to a one dimensional size in screen space.
     *
     * @param aSize: the size to be transformed.
     */
    double ToScreen( double aSize ) const;

    /**
     * Return the size of the our rendering area in pixels.
     *
     * @return viewport screen size.
     */
    const VECTOR2I& GetScreenPixelSize() const;

    /**
     * Remove all items from the view.
     */
    void Clear();

    /**
     * Control the visibility of a particular layer.
     *
     * @param aLayer is the layer to show/hide.
     * @param aVisible is the layer visibility state.
     */
    inline void SetLayerVisible( int aLayer, bool aVisible = true )
    {
        auto it = m_layers.find( aLayer );

        if( it == m_layers.end() )
            return;

        VIEW_LAYER& layer = it->second;

        if( layer.visible != aVisible )
        {
            // Target has to be redrawn after changing its visibility
            MarkTargetDirty( layer.target );
            layer.visible = aVisible;
        }
    }

    /**
     * Return information about visibility of a particular layer.
     *
     * @param aLayer true if the layer is visible, false otherwise.
     */
    inline bool IsLayerVisible( int aLayer ) const
    {
        auto it = m_layers.find( aLayer );

        if( it == m_layers.end() )
            return false;

        return it->second.visible;
    }

    /**
     * Set the whether the layer should drawn differentially.
     *
     * @param aLayer is the layer to set to be draw differentially
     * @param aDiff is the layer diff'ing state.
     */
    inline void SetLayerDiff( int aLayer, bool aDiff = true )
    {
        auto it = m_layers.find( aLayer );

        if( it == m_layers.end() )
            return;

        VIEW_LAYER& layer = it->second;

        if( layer.diffLayer != aDiff )
        {
            // Target has to be redrawn after changing its layers' diff status
            MarkTargetDirty( layer.target );
            layer.diffLayer = aDiff;
        }
    }

    /**
     * Set the status of negatives presense in a particular layer.
     *
     * @param aLayer is the layer to set as containing negatives (or not).
     * @param aNegatives is the layer negatives state.
     */
    inline void SetLayerHasNegatives( int aLayer, bool aNegatives = true )
    {
        auto it = m_layers.find( aLayer );

        if( it == m_layers.end() )
            return;

        VIEW_LAYER& layer = it->second;

        if( layer.hasNegatives != aNegatives )
        {
            // Target has to be redrawn after changing a layers' negatives
            MarkTargetDirty( layer.target );
            layer.hasNegatives = aNegatives;
        }
    }

    /**
     * Set a layer display-only (ie: to be rendered but not returned by hit test queries).
     */
    inline void SetLayerDisplayOnly( int aLayer, bool aDisplayOnly = true )
    {
        auto it = m_layers.find( aLayer );

        if( it == m_layers.end() )
            return;

        it->second.displayOnly = aDisplayOnly;
    }

    /**
     * Change the rendering target for a particular layer.
     *
     * @param aLayer is the layer.
     * @param aTarget is the rendering target.
     */
    inline void SetLayerTarget( int aLayer, RENDER_TARGET aTarget )
    {
        auto it = m_layers.find( aLayer );

        if( it == m_layers.end() )
            return;

        it->second.target = aTarget;
    }

    /**
     * Set rendering order of a particular layer. Lower values are rendered first.
     *
     * @param aLayer is the layer.
     * @param aRenderingOrder is an arbitrary number denoting the rendering order.
     */
    void SetLayerOrder( int aLayer, int aRenderingOrder );

    /**
     * Return rendering order of a particular layer. Lower values are rendered first.
     *
     * @param aLayer is the layer.
     * @return Rendering order of a particular layer.
     */
    int GetLayerOrder( int aLayer ) const;

    /**
     * Change the order of given layer ids, so after sorting the order corresponds to layers
     * rendering order (descending, ie. order in which layers should be drawn - from the bottom to
     * the top).
     *
     * @param aLayers stores id of layers to be sorted.
     */
    void SortLayers( std::vector<int>& aLayers ) const;

    /**
     * Remap the data between layer ids without invalidating that data.
     *
     * Used by GerbView for the "Sort by X2" functionality.
     *
     * @param aReorderMap is a mapping of old to new layer ids.
     */
    void ReorderLayerData( std::unordered_map<int, int> aReorderMap );

    /**
     * Apply the new coloring scheme held by RENDER_SETTINGS in case that it has changed.
     *
     * @param aLayer is a number of the layer to be updated.
     * @see RENDER_SETTINGS
     */
    void UpdateLayerColor( int aLayer );

    /**
     * Apply the new coloring scheme to all layers. The used scheme is held by #RENDER_SETTINGS.
     *
     * @see RENDER_SETTINGS
     */
    void UpdateAllLayersColor();

    /**
     * Set given layer to be displayed on the top or sets back the default order of layers.
     *
     * @param aEnabled = true to display aLayer on the top.
     * @param aLayer is the layer or -1 in case when no particular layer should be displayed
     *               on the top.
     */
    virtual void SetTopLayer( int aLayer, bool aEnabled = true );

    /**
     * Enable or disable display of the top layer.
     *
     * When disabled, layers are rendered as usual with no influence from SetTopLayer
     * function.  Otherwise on the top there is displayed the layer set previously with
     * SetTopLayer function.
     *
     * @param aEnable whether to enable or disable display of the top layer.
     */
    virtual void EnableTopLayer( bool aEnable );

    virtual int GetTopLayer() const;

    /**
     * Remove all layers from the on-the-top set (they are no longer displayed over the rest of
     * layers).
     */
    void ClearTopLayers();

    /**
     * Do everything that is needed to apply the rendering order of layers.
     *
     * It has to be called after modification of renderingOrder field of LAYER.
     */
    void UpdateAllLayersOrder();

    /**
     * Clear targets that are marked as dirty.
     */
    void ClearTargets();

    /**
     * Immediately redraws the whole view.
     */
    virtual void Redraw();

    /**
     * Rebuild GAL display lists.
     */
    void RecacheAllItems();

    /**
     * Return true if any of the VIEW layers needs to be refreshened.
     *
     * @return True in case if any of layers is marked as dirty.
     */
    bool IsDirty() const
    {
        for( int i = 0; i < TARGETS_NUMBER; ++i )
        {
            if( IsTargetDirty( i ) )
                return true;
        }

        return false;
    }

    /**
     * Return true if any of layers belonging to the target or the target itself should be
     * redrawn.
     *
     * @return True if the above condition is fulfilled.
     */
    bool IsTargetDirty( int aTarget ) const
    {
        wxCHECK( aTarget < TARGETS_NUMBER, false );
        return m_dirtyTargets[aTarget];
    }

    /**
     * Set or clear target 'dirty' flag.
     *
     * @param aTarget is the target to set.
     */
    inline void MarkTargetDirty( int aTarget )
    {
        wxCHECK( aTarget < TARGETS_NUMBER, /* void */ );
        m_dirtyTargets[aTarget] = true;
    }

    /// Return true if the layer is cached.
    inline bool IsCached( int aLayer ) const
    {
        auto it = m_layers.find( aLayer );

        if( it == m_layers.end() )
            return false;

        return it->second.target == TARGET_CACHED;
    }

    /**
     * Force redraw of view on the next rendering.
     */
    void MarkDirty()
    {
        for( int i = 0; i < TARGETS_NUMBER; ++i )
            m_dirtyTargets[i] = true;
    }

    /**
     * Force redraw of view on the next rendering.
     */
    void MarkClean()
    {
        for( int i = 0; i < TARGETS_NUMBER; ++i )
            m_dirtyTargets[i] = false;
    }

    /**
     * Iterate through the list of items that asked for updating and updates them.
     */
    void UpdateItems();

    /**
     * Update all items in the view according to the given flags.
     *
     * @param aUpdateFlags is is according to KIGFX::VIEW_UPDATE_FLAGS
     */
    void UpdateAllItems( int aUpdateFlags );

    /**
     * Update items in the view according to the given flags and condition.
     *
     * @param aUpdateFlags is is according to KIGFX::VIEW_UPDATE_FLAGS.
     * @param aCondition is a function returning true if the item should be updated.
     */
    void UpdateAllItemsConditionally( int aUpdateFlags,
                                      std::function<bool( VIEW_ITEM* )> aCondition );

    /**
     * Update items in the view according to the flags returned by the callback.
     * @param aItemFlagsProvider is a function returning any KIGFX::VIEW_UPDATE_FLAGS that should
     *                           be set on the VIEW_ITEM.
     */
    void UpdateAllItemsConditionally( std::function<int( VIEW_ITEM* )> aItemFlagsProvider );

    /**
     * @return true if draw priority is being respected while redrawing.
     */
    bool IsUsingDrawPriority() const
    {
        return m_useDrawPriority;
    }

    /**
     * @param aFlag is true if draw priority should be respected while redrawing.
     */
    void UseDrawPriority( bool aFlag )
    {
        m_useDrawPriority = aFlag;
    }

    /**
     * Only takes effect if UseDrawPriority is true.
     *
     * @param aFlag is true if draw order should be reversed
     */
    void ReverseDrawOrder( bool aFlag )
    {
        m_reverseDrawOrder = aFlag;
    }

    std::shared_ptr<VIEW_OVERLAY> MakeOverlay();

    void InitPreview();

    void ClearPreview();
    void AddToPreview( VIEW_ITEM* aItem, bool aTakeOwnership = true );

    void ShowPreview( bool aShow = true );

    /**
     * Return a new VIEW object that shares the same set of VIEW_ITEMs and LAYERs.
     *
     * GAL, PAINTER and other properties are left uninitialized.
     */
    std::unique_ptr<VIEW> DataReference() const;

    /// Maximum number of layers that may be shown.
    static constexpr int VIEW_MAX_LAYERS = MAX_LAYERS_FOR_VIEW;

    /// Rendering order modifier for layers that are marked as top layers.
    static constexpr int TOP_LAYER_MODIFIER = -MAX_LAYERS_FOR_VIEW;

protected:
    struct VIEW_LAYER
    {
        bool                    visible;         ///< Is the layer to be rendered?
        bool                    displayOnly;     ///< Is the layer display only?

        /// Layer should be drawn differentially over lower layers.
        bool                    diffLayer;

        /// Layer should be drawn separately to not delete lower layers.
        bool                    hasNegatives;
        std::shared_ptr<VIEW_RTREE> items;       ///< R-tree indexing all items on this layer.
        int                     renderingOrder;  ///< Rendering order of this layer.
        int                     id;              ///< Layer ID.
        RENDER_TARGET           target;          ///< Where the layer should be rendered.

        ///< Layers that have to be enabled to show the layer.
        std::set<int>           requiredLayers;

        bool operator< ( const VIEW_LAYER& aOther ) const
        {
            return id < aOther.id;
        }
    };

    /// Redraw contents within rectangle \a aRect.
    void redrawRect( const BOX2I& aRect );

    inline void markTargetClean( int aTarget )
    {
        wxCHECK( aTarget < TARGETS_NUMBER, /* void */ );
        m_dirtyTargets[aTarget] = false;
    }

    /**
     * Draw an item, but on a specified layers.
     *
     * It has to be marked that some of drawing settings are based on the layer on which
     * an item is drawn.
     *
     * @param aItem is the item to be drawn.
     * @param aLayer is the layer which should be drawn.
     * @param aImmediate dictates the way of drawing - it allows one to force immediate
     *                   drawing mode for cached items.
     */
    void draw( VIEW_ITEM* aItem, int aLayer, bool aImmediate = false );

    /**
     * Draw an item on all layers that the item uses.
     *
     * @param aItem is the item to be drawn.
     * @param aImmediate dictates the way of drawing - it allows one to force immediate
     *                   drawing mode for cached items.
     */
    void draw( VIEW_ITEM* aItem, bool aImmediate = false );

    /**
     * Draw a group of items on all layers that those items use.
     *
     * @param aGroup is the group to be drawn.
     * @param aImmediate dictates the way of drawing - it allows one to force immediate
     *                   drawing mode for cached items.
     */
    void draw( VIEW_GROUP* aGroup, bool aImmediate = false );

    /// Sort m_orderedLayers when layer rendering order has changed.
    void sortOrderedLayers();

    /// Clear cached GAL group numbers (*ONLY* numbers stored in VIEW_ITEMs, not group objects
    /// used by GAL).
    void clearGroupCache();

    /**
     * Manage dirty flags & redraw queuing when updating an item.
     *
     * @param aItem is the item to be updated.
     * @param aUpdateFlags determines the way an item is refreshed.
     */
    void invalidateItem( VIEW_ITEM* aItem, int aUpdateFlags );

    /// Update colors that are used for an item to be drawn.
    void updateItemColor( VIEW_ITEM* aItem, int aLayer );

    /// Update all information needed to draw an item.
    void updateItemGeometry( VIEW_ITEM* aItem, int aLayer );

    /// Update bounding box of an item.
    void updateBbox( VIEW_ITEM* aItem );

    /// Update set of layers that an item occupies.
    void updateLayers( VIEW_ITEM* aItem );

    /// Determine rendering order of layers. Used in display order sorting function.
    static bool compareRenderingOrder( VIEW_LAYER* aI, VIEW_LAYER* aJ )
    {
        return aI->renderingOrder > aJ->renderingOrder;
    }

    /// Check if every layer required by the aLayerId layer is enabled.
    bool areRequiredLayersEnabled( int aLayerId ) const;

    // Function objects that need to access VIEW/VIEW_ITEM private/protected members
    struct CLEAR_LAYER_CACHE_VISITOR;
    struct RECACHE_ITEM_VISITOR;
    struct DRAW_ITEM_VISITOR;
    struct UPDATE_COLOR_VISITOR;
    struct UPDATE_DEPTH_VISITOR;

    std::unique_ptr<KIGFX::VIEW_GROUP> m_preview;
    std::vector<VIEW_ITEM*>            m_ownedItems;

    /// Whether to use rendering order modifier or not.
    bool                               m_enableOrderModifier;

    /// The set of possible displayed layers and its properties.
    std::map<int, VIEW_LAYER>          m_layers;

    /// Sorted list of pointers to members of m_layers.
    std::vector<VIEW_LAYER*>           m_orderedLayers;

    /// Flat list of all items.
    std::shared_ptr<std::vector<VIEW_ITEM*>> m_allItems;

    /// The set of layers that are displayed on the top.
    std::set<unsigned int>             m_topLayers;

    /// Center point of the VIEW (the point at which we are looking at).
    VECTOR2D                           m_center;

    double                             m_scale;
    BOX2D                              m_boundary;
    double                             m_minScale;
    double                             m_maxScale;

    bool                               m_mirrorX;
    bool                               m_mirrorY;

    /// PAINTER contains information how do draw items.
    PAINTER* m_painter;

    /// Interface to #PAINTER that is used to draw items.
    GAL* m_gal;

    /// Flag to mark targets as dirty so they have to be redrawn on the next refresh event.
    bool m_dirtyTargets[TARGETS_NUMBER];

    /// Flag to respect draw priority when drawing items.
    bool m_useDrawPriority;

    /// The next sequential drawing priority.
    int m_nextDrawPriority;

    /// Flag to reverse the draw order when using draw priority.
    bool m_reverseDrawOrder;
};
} // namespace KIGFX

