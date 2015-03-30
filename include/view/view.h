/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2013 CERN
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

#ifndef __VIEW_H
#define __VIEW_H

#include <vector>
#include <set>
#include <boost/unordered/unordered_map.hpp>

#include <math/box2.h>
#include <gal/definitions.h>

namespace KIGFX
{
class PAINTER;
class GAL;
class VIEW_ITEM;
class VIEW_GROUP;
class VIEW_RTREE;

/**
 * Class VIEW.
 * Holds a (potentially large) number of VIEW_ITEMs and renders them on a graphics device
 * provided by the GAL. VIEWs can exist in two flavors:
 * - dynamic - where items can be added, removed or changed anytime, intended for the main
 *    editing panel. Each VIEW_ITEM can be added to a single dynamic view.
 * - static - where items are added once at the startup and are not linked with the VIEW.
 *    Foreseen for preview windows and printing.
 * Items in a view are grouped in layers (not to be confused with Kicad's PCB layers). Each layer is
 * identified by an integer number. Visibility and rendering order can be set individually for each
 * of the layers. Future versions of the VIEW will also allow to assign different layers to different
 * rendering targets, which will be composited at the final stage by the GAL.
 * The VIEW class also provides fast methods for finding all visible objects that are within a given
 * rectangular area, useful for object selection/hit testing.
 */
class VIEW
{
public:
    friend class VIEW_ITEM;

    typedef std::pair<VIEW_ITEM*, int> LAYER_ITEM_PAIR;

    /**
     * Constructor.
     * @param aIsDynamic decides whether we are creating a static or a dynamic VIEW.
     */
    VIEW( bool aIsDynamic = true );

    ~VIEW();

    /**
     * Function Add()
     * Adds a VIEW_ITEM to the view.
     * @param aItem: item to be added. No ownership is given
     */
    void Add( VIEW_ITEM* aItem );

    /**
     * Function Remove()
     * Removes a VIEW_ITEM from the view.
     * @param aItem: item to be removed. Caller must dispose the removed item if necessary
     */
    void Remove( VIEW_ITEM* aItem );

    /**
     * Function Query()
     * Finds all visible items that touch or are within the rectangle aRect.
     * @param aRect area to search for items
     * @param aResult result of the search, containing VIEW_ITEMs associated with their layers.
     *  Sorted according to the rendering order (items that are on top of the rendering stack as
     *  first).
     * @return Number of found items.
     */
    int Query( const BOX2I& aRect, std::vector<LAYER_ITEM_PAIR>& aResult ) const;

    /**
     * Function SetRequired()
     * Marks the aRequiredId layer as required for the aLayerId layer. In order to display the
     * layer, all of its required layers have to be enabled.
     * @param aLayerId is the id of the layer for which we enable/disable the required layer.
     * @param aRequiredId is the id of the required layer.
     * @param aRequired tells if the required layer should be added or removed from the list.
     */
    void SetRequired( int aLayerId, int aRequiredId, bool aRequired = true );

    /**
     * Function CopySettings()
     * Copies layers and visibility settings from another view.
     * @param aOtherView: view from which settings will be copied.
     */
    void CopySettings( const VIEW* aOtherView );

    /*
     *  Convenience wrappers for adding multiple items
     *  template <class T> void AddItems( const T& aItems );
     *  template <class T> void RemoveItems( const T& aItems );
     */

    /**
     * Function SetGAL()
     * Assigns a rendering device for the VIEW.
     * @param aGal: pointer to the GAL output device
     */
    void SetGAL( GAL* aGal );

    /**
     * Function GetGAL()
     * Returns the GAL this view is using to draw graphical primitives.
     * @return Pointer to the currently used GAL instance.
     */
    GAL* GetGAL() const
    {
        return m_gal;
    }

    /**
     * Function SetPainter()
     * Sets the painter object used by the view for drawing VIEW_ITEMS.
     */
    void SetPainter( PAINTER* aPainter )
    {
        m_painter = aPainter;
    }

    /**
     * Function GetPainter()
     * Returns the painter object used by the view for drawing VIEW_ITEMS.
     * @return Pointer to the currently used Painter instance.
     */
    PAINTER* GetPainter() const
    {
        return m_painter;
    }

    /**
     * Function SetViewport()
     * Sets the visible area of the VIEW.
     * @param aViewport: desired visible area, in world space coordinates.
     */
    void SetViewport( const BOX2D& aViewport );

    /**
     * Function GetViewport()
     * Returns the current viewport visible area rectangle.
     * @return Current viewport rectangle
     */
    BOX2D GetViewport() const;

    /**
     * Function SetMirror()
     *  Controls the mirroring of the VIEW.
     *  @param aMirrorX: when true, the X axis is mirrored
     *  @param aMirrorY: when true, the Y axis is mirrored.
     */
    void SetMirror( bool aMirrorX, bool aMirrorY );

    /**
     * Function SetScale()
     * Sets the scaling factor. Scale = 1 corresponds to the real world size of the objects
     * (depending on correct GAL unit length & DPI settings).
     * @param aScale: the scalefactor
     */
    void SetScale( double aScale )
    {
        SetScale( aScale, m_center );
    }

    /**
     * Function SetScale()
     * Sets the scaling factor, zooming around a given anchor point.
     * (depending on correct GAL unit length & DPI settings).
     * @param aAnchor: the zooming  anchor point
     * @param aScale: the scale factor
     */
    void SetScale( double aScale, const VECTOR2D& aAnchor );

    /**
     * Function GetScale()
     * @return Current scalefactor of this VIEW
     */
    double GetScale() const
    {
        return m_scale;
    }

    /**
     * Function SetCenter()
     * Sets the center point of the VIEW (i.e. the point in world space that will be drawn in the middle
     * of the screen).
     * @param aCenter: the new center point, in world space coordinates.
     */
    void SetCenter( const VECTOR2D& aCenter );

    /**
     * Function GetCenter()
     * Returns the center point of this VIEW (in world space coordinates)
     * @return center point of the view
     */
    const VECTOR2D& GetCenter() const
    {
        return m_center;
    }

    /**
     * Function ToWorld()
     * Converts a screen space point/vector to a point/vector in world space coordinates.
     * @param aCoord: the point/vector to be converted
     * @param aAbsolute: when true, aCoord is treated as a point, otherwise - as a direction (vector)
     */
    VECTOR2D ToWorld( const VECTOR2D& aCoord, bool aAbsolute = true ) const;

    /**
     * Function ToWorld()
     * Converts a screen space one dimensional size to a one dimensional size in world
     * space coordinates.
     * @param aSize : the size to be converted
     */
    double ToWorld( double aSize ) const;

    /**
     * Function ToScreen()
     * Converts a world space point/vector to a point/vector in screen space coordinates.
     * @param aCoord: the point/vector to be converted
     * @param aAbsolute: when true, aCoord is treated as a point, otherwise - as a direction (vector)
     */
    VECTOR2D ToScreen( const VECTOR2D& aCoord, bool aAbsolute = true ) const;

    /**
     * Function ToScreen()
     * Converts a world space coordinate to a coordinate in screen space coordinates.
     * @param aCoord: the coordinate to be transformed.
     * @param aAbsolute: when true, aCoord is treated as a point, otherwise - as a direction (vector)
     */
    double ToScreen( double aCoord, bool aAbsolute = true ) const;

    /**
     * Function GetScreenPixelSize()
     * Returns the size of the our rendering area, in pixels.
     * @return viewport screen size
     */
    const VECTOR2I& GetScreenPixelSize() const;

    /**
     * Function AddLayer()
     * Adds a new layer to the view.
     * @param aLayer: unique ID of the layer to be added.
     * @param aDisplayOnly: layer is display-only (example: selection boxes, floating hints/menus).
     * Objects belonging to this layer are not taken into account by Query() method.
     */
    void AddLayer( int aLayer, bool aDisplayOnly = false );

    /**
     * Function ClearLayer()
     * Removes all items from a given layer.
     * @param aLayer: ID of the layer to be cleared
     */
    void ClearLayer( int aLayer );

    /**
     * Function Clear()
     * Removes all items from the view.
     */
    void Clear();

    /**
     * Function SetLayerVisible()
     * Controls the visibility of a particular layer.
     * @param aLayer: the layer to show/hide.
     * @param aVisible: the obvious.
     */
    inline void SetLayerVisible( int aLayer, bool aVisible = true )
    {
        wxASSERT( aLayer < (int) m_layers.size() );

        if( m_layers[aLayer].visible != aVisible )
        {
            // Target has to be redrawn after changing its visibility
            MarkTargetDirty( m_layers[aLayer].target );
            m_layers[aLayer].visible = aVisible;
        }
    }

    /**
     * Function IsLayerVisible()
     * Returns information about visibility of a particular layer.
     * @param aLayer: true if the layer is visible, false otherwise
     */
    inline bool IsLayerVisible( int aLayer ) const
    {
        wxASSERT( aLayer < (int) m_layers.size() );

        return m_layers.at( aLayer ).visible;
    }

    inline void SetLayerDisplayOnly( int aLayer, bool aDisplayOnly = true )
    {
        wxASSERT( aLayer < (int) m_layers.size() );

        m_layers[aLayer].displayOnly = aDisplayOnly;
    }

    /**
     * Function SetLayerTarget()
     * Changes the rendering target for a particular layer.
     * @param aLayer is the layer.
     * @param aTarget is the rendering target.
     */
    inline void SetLayerTarget( int aLayer, RENDER_TARGET aTarget )
    {
        wxASSERT( aLayer < (int) m_layers.size() );

        m_layers[aLayer].target = aTarget;
    }

    /**
     * Function SetLayerOrder()
     * Sets rendering order of a particular layer. Lower values are rendered first.
     * @param aLayer: the layer
     * @param aRenderingOrder: arbitrary number denoting the rendering order.
     */
    void SetLayerOrder( int aLayer, int aRenderingOrder );

    /**
     * Function GetLayerOrder()
     * Returns rendering order of a particular layer. Lower values are rendered first.
     * @param aLayer: the layer
     * @return Rendering order of a particular layer.
     */
    int GetLayerOrder( int aLayer ) const;

    /**
     * Function SortLayers()
     * Changes the order of given layer ids, so after sorting the order corresponds to layers
     * rendering order (descending, ie. order in which layers should be drawn - from the bottom to
     * the top).
     * @param aLayers stores id of layers to be sorted.
     * @param aCount stores the number of layers.
     */
    void SortLayers( int aLayers[], int& aCount ) const;

    /**
     * Function UpdateLayerColor()
     * Applies the new coloring scheme held by RENDER_SETTINGS in case that it has changed.
     * @param aLayer is a number of the layer to be updated.
     * @see RENDER_SETTINGS
     */
    void UpdateLayerColor( int aLayer );

    /**
     * Function UpdateAllLayersColor()
     * Applies the new coloring scheme to all layers. The used scheme is held by RENDER_SETTINGS.
     * @see RENDER_SETTINGS
     */
    void UpdateAllLayersColor();

    /**
     * Function ChangeLayerDepth()
     * Changes the depth of items on the given layer.
     * @param aLayer is a number of the layer to be updated.
     * @param aDepth is the new depth.
     */
    void ChangeLayerDepth( int aLayer, int aDepth );

    /**
     * Function SetTopLayer()
     * Sets given layer to be displayed on the top or sets back the default order of layers.
     * @param aLayer: the layer or -1 in case when no particular layer should
     * be displayed on the top.
     */
    void SetTopLayer( int aLayer, bool aEnabled = true );

    /**
     * Function EnableTopLayer()
     * Enables or disables display of the top layer. When disabled - layers are rendered as usual
     * with no influence from SetTopLayer function. Otherwise on the top there is displayed the
     * layer set previously with SetTopLayer function.
     * @param aEnable whether to enable or disable display of the top layer.
     */
    void EnableTopLayer( bool aEnable );

    int GetTopLayer() const;

    /**
     * Function ClearTopLayers()
     * Removes all layers from the on-the-top set (they are no longer displayed over the rest of
     * layers).
     */
    void ClearTopLayers();

    /**
     * Function UpdateLayerOrder()
     * Does everything that is needed to apply the rendering order of layers. It has to be called
     * after modification of renderingOrder field of LAYER.
     */
    void UpdateAllLayersOrder();

    /**
     * Function ClearTargets()
     * Clears targets that are marked as dirty.
     */
    void ClearTargets();

    /**
     * Function Redraw()
     * Immediately redraws the whole view.
     */
    void Redraw();

    /**
     * Function RecacheAllItems()
     * Rebuilds GAL display lists.
     * @param aForceNow decides if every item should be instantly recached. Otherwise items are
     * going to be recached when they become visible.
     */
    void RecacheAllItems( bool aForceNow = false );

    /**
     * Function IsDynamic()
     * Tells if the VIEW is dynamic (ie. can be changed, for example displaying PCBs in a window)
     * or static (that cannot be modified, eg. displaying image/PDF).
     */
    bool IsDynamic() const
    {
        return m_dynamic;
    }

    /**
     * Function IsDirty()
     * Returns true if any of the VIEW layers needs to be refreshened.
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
     * Function IsTargetDirty()
     * Returns true if any of layers belonging to the target or the target itself should be
     * redrawn.
     * @return True if the above condition is fulfilled.
     */
    bool IsTargetDirty( int aTarget ) const
    {
        wxASSERT( aTarget < TARGETS_NUMBER );

        return m_dirtyTargets[aTarget];
    }

    /**
     * Function MarkTargetDirty()
     * Sets or clears target 'dirty' flag.
     * @param aTarget is the target to set.
     */
    inline void MarkTargetDirty( int aTarget )
    {
        wxASSERT( aTarget < TARGETS_NUMBER );

        m_dirtyTargets[aTarget] = true;
    }

    /// Returns true if the layer is cached
    inline bool IsCached( int aLayer ) const
    {
        wxASSERT( aLayer < (int) m_layers.size() );

        try
        {
            return m_layers.at( aLayer ).target == TARGET_CACHED;
        }
        catch( std::out_of_range )
        {
            return false;
        }
    }

    /**
     * Function MarkDirty()
     * Forces redraw of view on the next rendering.
     */
    void MarkDirty()
    {
        for( int i = 0; i < TARGETS_NUMBER; ++i )
            m_dirtyTargets[i] = true;
    }

    /**
     * Function MarkForUpdate()
     * Adds an item to a list of items that are going to be refreshed upon the next frame rendering.
     * @param aItem is the item to be refreshed.
     */
    void MarkForUpdate( VIEW_ITEM* aItem )
    {
        m_needsUpdate.push_back( aItem );
    }

    /**
     * Function UpdateItems()
     * Iterates through the list of items that asked for updating and updates them.
     */
    void UpdateItems();

    const BOX2I CalculateExtents() ;

    static const int VIEW_MAX_LAYERS = 128;      ///< maximum number of layers that may be shown

private:
    struct VIEW_LAYER
    {
        bool                    visible;         ///< is the layer to be rendered?
        bool                    displayOnly;     ///< is the layer display only?
        VIEW_RTREE*             items;           ///< R-tree indexing all items on this layer.
        int                     renderingOrder;  ///< rendering order of this layer
        int                     id;              ///< layer ID
        RENDER_TARGET           target;          ///< where the layer should be rendered
        std::set<int>           requiredLayers;  ///< layers that have to be enabled to show the layer
    };

    // Convenience typedefs
    typedef boost::unordered_map<int, VIEW_LAYER>   LAYER_MAP;
    typedef LAYER_MAP::iterator                     LAYER_MAP_ITER;
    typedef std::vector<VIEW_LAYER*>                LAYER_ORDER;
    typedef std::vector<VIEW_LAYER*>::iterator      LAYER_ORDER_ITER;

    // Function objects that need to access VIEW/VIEW_ITEM private/protected members
    struct clearLayerCache;
    struct recacheItem;
    struct drawItem;
    struct unlinkItem;
    struct updateItemsColor;
    struct changeItemsDepth;
    struct extentsVisitor;


    ///* Redraws contents within rect aRect
    void redrawRect( const BOX2I& aRect );

    inline void markTargetClean( int aTarget )
    {
        wxASSERT( aTarget < TARGETS_NUMBER );

        m_dirtyTargets[aTarget] = false;
    }

    /**
     * Function draw()
     * Draws an item, but on a specified layers. It has to be marked that some of drawing settings
     * are based on the layer on which an item is drawn.
     *
     * @param aItem is the item to be drawn.
     * @param aLayer is the layer which should be drawn.
     * @param aImmediate dictates the way of drawing - it allows to force immediate drawing mode
     * for cached items.
     */
    void draw( VIEW_ITEM* aItem, int aLayer, bool aImmediate = false );

    /**
     * Function draw()
     * Draws an item on all layers that the item uses.
     *
     * @param aItem is the item to be drawn.
     * @param aImmediate dictates the way of drawing - it allows to force immediate drawing mode
     * for cached items.
     */
    void draw( VIEW_ITEM* aItem, bool aImmediate = false );

    /**
     * Function draw()
     * Draws a group of items on all layers that those items use.
     *
     * @param aGroup is the group to be drawn.
     * @param aImmediate dictates the way of drawing - it allows to force immediate drawing mode
     * for cached items.
     */
    void draw( VIEW_GROUP* aGroup, bool aImmediate = false );

    ///* Sorts m_orderedLayers when layer rendering order has changed
    void sortLayers();

    ///* Clears cached GAL group numbers (*ONLY* numbers stored in VIEW_ITEMs, not group objects
    ///* used by GAL)
    void clearGroupCache();

    /**
     * Function invalidateItem()
     * Manages dirty flags & redraw queueing when updating an item.
     * @param aItem is the item to be updated.
     * @param aUpdateFlags determines the way an item is refreshed.
     */
    void invalidateItem( VIEW_ITEM* aItem, int aUpdateFlags );

    /// Updates colors that are used for an item to be drawn
    void updateItemColor( VIEW_ITEM* aItem, int aLayer );

    /// Updates all informations needed to draw an item
    void updateItemGeometry( VIEW_ITEM* aItem, int aLayer );

    /// Updates bounding box of an item
    void updateBbox( VIEW_ITEM* aItem );

    /// Updates set of layers that an item occupies
    void updateLayers( VIEW_ITEM* aItem );

    /// Determines rendering order of layers. Used in display order sorting function.
    static bool compareRenderingOrder( VIEW_LAYER* aI, VIEW_LAYER* aJ )
    {
        return aI->renderingOrder > aJ->renderingOrder;
    }

    /// Checks if every layer required by the aLayerId layer is enabled.
    bool areRequiredLayersEnabled( int aLayerId ) const;

    ///* Whether to use rendering order modifier or not
    bool m_enableOrderModifier;

    /// Contains set of possible displayed layers and its properties
    LAYER_MAP m_layers;

    /// Sorted list of pointers to members of m_layers
    LAYER_ORDER m_orderedLayers;

    /// Stores set of layers that are displayed on the top
    std::set<unsigned int> m_topLayers;

    /// Center point of the VIEW (the point at which we are looking at)
    VECTOR2D m_center;

    /// Scale of displayed VIEW_ITEMs
    double m_scale;

    /// PAINTER contains information how do draw items
    PAINTER* m_painter;

    /// Gives interface to PAINTER, that is used to draw items
    GAL* m_gal;

    /// Dynamic VIEW (eg. display PCB in window) allows changes once it is built,
    /// static (eg. image/PDF) - does not.
    bool m_dynamic;

    /// Flags to mark targets as dirty, so they have to be redrawn on the next refresh event
    bool m_dirtyTargets[TARGETS_NUMBER];

    /// Rendering order modifier for layers that are marked as top layers
    static const int TOP_LAYER_MODIFIER;

    /// Items to be updated
    std::vector<VIEW_ITEM*> m_needsUpdate;
};
} // namespace KIGFX

#endif
