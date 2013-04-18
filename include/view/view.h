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
#include <boost/unordered/unordered_map.hpp>

#include <math/box2.h>

namespace KiGfx
{
class PAINTER;
class GAL;
class VIEW_ITEM;
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

    typedef std::pair<VIEW_ITEM*, int> LayerItemPair;

    /**
     * Constructor.
     * @param aIsDynamic: decides whether we are creating a static or a dynamic VIEW.
     * @param aUseGroups: fixme
     */
    VIEW( bool aIsDynamic = true, bool aUseGroups = false );

    ~VIEW();

    /**
     * Function Add()
     * Adds a VIEW_ITEM to the view.
     * @param aItem: item to be added. No ownership is given
     */
    void    Add( VIEW_ITEM* aItem );

    /**
     * Function Remove()
     * Removes a VIEW_ITEM from the view.
     * @param aItem: item to be removed. Caller must dispose the removed item if necessary
     */
    void    Remove( VIEW_ITEM* aItem );

    /** Function Query()
     * Finds all visible items that touch or are within the rectangle aRect.
     * @param aRect area to search for items
     * @param aResult result of the search, containing VIEW_ITEMs associated with their layers.
     *  Sorted according to the rendering order (items that are on top of the rendering stack as first).
     * @return Number of found items.
     */
    int     Query( const BOX2I& aRect, std::vector<LayerItemPair>& aResult );

    /**
     * Function CopySettings()
     * Copies layers and visibility settings from another view.
     * @param aOtherView: view from which settings will be copied.
     */
    void    CopySettings( const VIEW* aOtherView );

    /*
     *  Convenience wrappers for adding multiple items
     *  template<class T> void AddItems( const T& aItems );
     *  template<class T> void RemoveItems( const T& aItems );
     */

    /**
     * Function SetGAL()
     * Assigns a rendering device for the VIEW.
     * @param aGal: pointer to the GAL output device
     */
    void    SetGAL( GAL* aGal );

    /**
     * Function GetGAL()
     * Returns the GAL this view is using to draw graphical primitives.
     * @return Pointer to the currently used GAL instance.
     */
    GAL* GetGAL() const { return m_gal; }

    /**
     * Function SetPainter()
     * Sets the painter object used by the view for drawing VIEW_ITEMS.
     */
    void    SetPainter( PAINTER* aPainter );

    /**
     * Function GetPainter()
     * Returns the painter object used by the view for drawing VIEW_ITEMS.
     * @return Pointer to the currently used Painter instance.
     */
    PAINTER*    GetPainter() const { return m_painter; };

    /**
     * Function SetViewport()
     * Sets the visible area of the VIEW.
     * @param aViewport: desired visible area, in world space coordinates.
     * @param aKeepProportions: when true, the X/Y size proportions are kept.
     */
    void    SetViewport( const BOX2D& aViewport, bool aKeepProportions = true );

    /**
     * Function GetViewport()
     * Returns the current viewport visible area rectangle.
     * @return Current viewport rectangle
     */
    BOX2D   GetViewport() const;

    /**
     * Function SetMirror()
     *  Controls the mirroring of the VIEW.
     *  @param aMirrorX: when true, the X axis is mirrored
     *  @param aMirrorY: when true, the Y axis is mirrored.
     */
    void    SetMirror( bool aMirrorX, bool aMirrorY );

    /**
     * Function SetScale()
     * Sets the scaling factor. Scale = 1 corresponds to the real world size of the objects
     * (depending on correct GAL unit length & DPI settings).
     * @param aScale: the scalefactor
     */
    void    SetScale( double aScale );

    /**
     * Function SetScale()
     * Sets the scaling factor, zooming around a given anchor point.
     * (depending on correct GAL unit length & DPI settings).
     * @param aScale: the scale factor
     */
    void    SetScale( double aScale, const VECTOR2D& aAnchor );

    /**
     * Function GetScale()
     * @return Current scalefactor of this VIEW
     */
    double  GetScale() const { return m_scale; }

    /**
     * Function SetCenter()
     * Sets the center point of the VIEW (i.e. the point in world space that will be drawn in the middle
     * of the screen).
     * @param aCenter: the new center point, in world space coordinates.
     */
    void    SetCenter( const VECTOR2D& aCenter );

    /**
     * Function GetCenter()
     * Returns the center point of this VIEW (in world space coordinates)
     * @return center point of the view
     */
    const VECTOR2D& GetCenter() const { return m_center; }

    /**
     * Function ToWorld()
     * Converts a screen space point/vector to a point/vector in world space coordinates.
     * @param aCoord: the point/vector to be converted
     * @param aAbsolute: when true, aCoord is treated as a point, otherwise - as a direction (vector)
     */
    VECTOR2D   ToWorld( const VECTOR2D& aCoord, bool aAbsolute = true ) const;

    /**
     * Function ToScreen()
     * Converts a world space point/vector to a point/vector in screen space coordinates.
     * @param aCoord: the point/vector to be converted
     * @param aAbsolute: when true, aCoord is treated as a point, otherwise - as a direction (vector)
     */
    VECTOR2D   ToScreen( const VECTOR2D& aCoord, bool aAbsolute = true ) const;

    /**
     * Function ToScreen()
     * Converts a world space coordinate to a coordinate in screen space coordinates.
     * @param aCoord: the coordinate to be transformed.
     * @param aAbsolute: when true, aCoord is treated as a point, otherwise - as a direction (vector)
     */
    double   ToScreen( double aCoord, bool aAbsolute = true ) const;


    /**
     * Function GetScreenPixelSize()
     * Returns the size of the our rendering area, in pixels.
     * @return viewport screen size
     */
    VECTOR2D   GetScreenPixelSize() const;

    /**
     * Function AddLayer()
     * Adds a new layer to the view.
     * @param aLayer: unique ID of the layer to be added.
     * @param aDisplayOnly: layer is display-only (example: selection boxes, floating hints/menus).
     * Objects belonging to this layer are not taken into account by Query() method.
     */
    void    AddLayer( int aLayer, bool aDisplayOnly = false );


    /**
     * Function ClearLayer()
     * Removes all items from a given layer.
     * @param aLayer: ID of the layer to be cleared
     */
    void    ClearLayer( int aLayer );

    /**
     * Function Clear()
     * Removes all items from the view.
     */
    void    Clear();

    /**
     * Function SetLayerVisible()
     * Controls the visibility of a particular layer.
     * @param aLayer: the layer to show/hide. When ALL_LAYERS constant is given, all layers'
     * visibility is updated
     * @param aVisible: the obivous
     */
    void    SetLayerVisible( int aLayer, bool aVisible = true );

    /**
     * Function SetLayerOrder()
     * Sets rendering order of a particular layer.
     * @param aLayer: the layer
     * @param aRenderingOrder: arbitrary number denoting the rendering order.
     * Lower values are rendered first.
     */
    void    SetLayerOrder( int aLayer, int aRenderingOrder );

    /**
     * Function SetTopLayer()
     * Sets given layer to be displayed on the top or sets back the default order of layers.
     * @param aLayer: the layer or -1 in case when no particular layer should
     * be displayed on the top.
     */
    void    SetTopLayer( int aLayer );

    /**
     * Function EnableTopLayer()
     * Enables or disables display of the top layer. When disabled - layers are rendered as usual
     * with no influence from SetTopLayer function. Otherwise on the top there is displayed the
     * layer set previously with SetTopLayer function.
     * @param aEnabled: whether to enable or disable display of the top layer.
     */
    void    EnableTopLayer( bool aEnable );

    /**
     * Function Redraw()
     * Immediately redraws the whole view.
     */
    void    Redraw();

    /**
     * Function PartialRedraw()
     * Redraws only the parts of the view that have been affected by items
     * for which ViewUpdate() function has been called since last redraw.
     */
    void    PartialRedraw();

    /**
     * Function IsDynamic()
     * Tells if the VIEW is dynamic (ie. can be changed, for example displaying PCBs in a window)
     * or static (that cannot be modified, eg. displaying image/PDF).
     */
    bool IsDynamic() const { return m_dynamic; }

    static const unsigned int VIEW_MAX_LAYERS;   ///* maximum number of layers that may be shown
    static const int          TOP_LAYER;         ///* layer number for displaying items on the top

private:

    struct VIEW_LAYER
    {
        bool                    enabled;         ///* is the layer to be rendered?
        bool                    isDirty;         ///* does it contain any dirty items (updated since last redraw)
        bool                    displayOnly;     ///* is the layer display only?
        VIEW_RTREE*             items;           ///* R-tree indexing all items on this layer.
        std::vector<VIEW_ITEM*> dirtyItems;      ///* set of dirty items collected since last redraw
        int                     renderingOrder;  ///* rendering order of this layer
        int                     id;              ///* layer ID
        BOX2I                   extents;         ///* sum of bboxes of all items on the layer
        BOX2I                   dirtyExtents;    ///* sum of bboxes of all dirty items on the layer
    };

    // Convenience typedefs
    typedef boost::unordered_map<int, VIEW_LAYER>   LayerMap;
    typedef LayerMap::iterator                      LayerMapIter;
    typedef std::vector<VIEW_LAYER*>                LayerOrder;
    typedef std::vector<VIEW_LAYER*>::iterator      LayerOrderIter;

    // Function objects that need to access VIEW/VIEW_ITEM private/protected members
    struct clearItemCache;
    struct unlinkItem;
    struct recacheItem;
    struct drawItem;

    ///* Saves current top layer settings in order to restore it when it's not top anymore
    VIEW_LAYER m_topLayer;

    ///* Whether to use top layer settings or not
    bool m_enableTopLayer;

    ///* Redraws contents within rect aRect
    void    redrawRect( const BOX2I& aRect );

    ///* Manages dirty flags & redraw queueing when updating an item. Called internally
    ///  via VIEW_ITEM::ViewUpdate()
    void    invalidateItem( VIEW_ITEM* aItem, int aUpdateFlags );

    ///* Sorts m_orderedLayers when layer rendering order has changed
    void    sortLayers();

    ///* Clears cached GAL display lists
    void    clearGroupCache();

    ///* Rebuilds GAL display lists
    void    itemsRecache();

    /// Determines rendering order of layers. Used in display order sorting function.
    static bool compareRenderingOrder( VIEW_LAYER* i, VIEW_LAYER* j )
    {
        return i->renderingOrder > j->renderingOrder;
    };

    /// Contains set of possible displayed layers and its properties
    LayerMap    m_layers;

    /// Sorted list of pointers to members of m_layers.
    LayerOrder  m_orderedLayers;

    /// Center point of the VIEW (the point at which we are looking at)
    VECTOR2D    m_center;

    /// Scale of displayed VIEW_ITEMs
    double      m_scale;

    /// PAINTER contains information how do draw items
    PAINTER*    m_painter;

    /// Gives interface to PAINTER, that is used to draw items
    GAL*        m_gal;

    /// Dynamic VIEW (eg. display PCB in window) allows changes once it is built,
    /// static (eg. image/PDF) - does not.
    bool        m_dynamic;

    /// Determines whether to use cached groups of objects for displaying.
    bool        m_useGroups;
};
} // namespace KiGfx

#endif
