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

/**
 * @file view_item.h
 * @brief VIEW_ITEM class definition.
 */

#ifndef __VIEW_ITEM_H
#define __VIEW_ITEM_H

#include <vector>

#include <math/box2.h>

/**
 * Enum KICAD_T
 * is the set of class identification values, stored in VIEW_ITEM::m_StructType
 */
enum KICAD_T {
    NOT_USED = -1,          ///< the 3d code uses this value

    EOT = 0,                ///< search types array terminator (End Of Types)

    TYPE_NOT_INIT = 0,
    PCB_T,
    SCREEN_T,               ///< not really an item, used to identify a screen

    // Items in pcb
    PCB_MODULE_T,           ///< class MODULE, a footprint
    PCB_PAD_T,              ///< class D_PAD, a pad in a footprint
    PCB_LINE_T,             ///< class DRAWSEGMENT, a segment not on copper layers
    PCB_TEXT_T,             ///< class TEXTE_PCB, text on a layer
    PCB_MODULE_TEXT_T,      ///< class TEXTE_MODULE, text in a footprint
    PCB_MODULE_EDGE_T,      ///< class EDGE_MODULE, a footprint edge
    PCB_TRACE_T,            ///< class TRACKE, a track segment (segment on a copper layer)
    PCB_VIA_T,              ///< class SEGVIA, a via (like a track segment on a copper layer)
    PCB_ZONE_T,             ///< class SEGZONE, a segment used to fill a zone area (segment on a
                            ///< copper layer)
    PCB_MARKER_T,           ///< class MARKER_PCB, a marker used to show something
    PCB_DIMENSION_T,        ///< class DIMENSION, a dimension (graphic item)
    PCB_TARGET_T,           ///< class PCB_TARGET, a target (graphic item)
    PCB_ZONE_AREA_T,        ///< class ZONE_CONTAINER, a zone area
    PCB_ITEM_LIST_T,        ///< class BOARD_ITEM_LIST, a list of board items

    // Schematic draw Items.  The order of these items effects the sort order.
    // It is currently ordered to mimic the old Eeschema locate behavior where
    // the smallest item is the selected item.
    SCH_MARKER_T,
    SCH_JUNCTION_T,
    SCH_NO_CONNECT_T,
    SCH_BUS_WIRE_ENTRY_T,
    SCH_BUS_BUS_ENTRY_T,
    SCH_LINE_T,
    SCH_BITMAP_T,
    SCH_TEXT_T,
    SCH_LABEL_T,
    SCH_GLOBAL_LABEL_T,
    SCH_HIERARCHICAL_LABEL_T,
    SCH_FIELD_T,
    SCH_COMPONENT_T,
    SCH_SHEET_PIN_T,
    SCH_SHEET_T,

    // Be prudent with these 3 types:
    // they should be used only to locate a specific field type
    // among SCH_FIELD_T items types
    SCH_FIELD_LOCATE_REFERENCE_T,
    SCH_FIELD_LOCATE_VALUE_T,
    SCH_FIELD_LOCATE_FOOTPRINT_T,

    // General
    SCH_SCREEN_T,

    /*
     * Draw items in library component.
     *
     * The order of these items effects the sort order for items inside the
     * "DRAW/ENDDRAW" section of the component definition in a library file.
     * If you add a new draw item, type, please make sure you add it so the
     * sort order is logical.
     */
    LIB_COMPONENT_T,
    LIB_ALIAS_T,
    LIB_ARC_T,
    LIB_CIRCLE_T,
    LIB_TEXT_T,
    LIB_RECTANGLE_T,
    LIB_POLYLINE_T,
    LIB_BEZIER_T,
    LIB_PIN_T,

    /*
     * Fields are not saved inside the "DRAW/ENDDRAW".  Add new draw item
     * types before this line.
     */
    LIB_FIELD_T,

    /*
     * For GerbView: items type:
     */
    TYPE_GERBER_DRAW_ITEM,

    // End value
    MAX_STRUCT_TYPE_ID
};

namespace KiGfx
{
// Forward declarations
class GAL;
class PAINTER;
class VIEW;

/**
 * Class VIEW_ITEM -
 * is an abstract base class for deriving all objects that can be added to a VIEW.
 * It's role is to:
 * - communicte geometry, appearance and visibility updates to the associated dynamic VIEW,
 * - provide a bounding box for redraw area calculation,
 * - (optional) draw the object using the GAL API functions for PAINTER-less implementations.
 * VIEW_ITEM objects are never owned by a VIEW. A single VIEW_ITEM can belong to any number of
 * static VIEWs, but only one dynamic VIEW due to storage of only one VIEW reference.
 */
class VIEW_ITEM
{
public:
    /**
     * Enum ViewUpdateFlags.
     * Defines the how severely the shape/appearance of the item has been changed:
     * - APPEARANCE: shape or layer set of the item have not been affected,
     * only colors or visibility.
     * - GEOMETRY: shape or layer set of the item have changed, VIEW may need to reindex it.
     * - ALL: all flags above */

    enum ViewUpdateFlags {
        APPEARANCE  = 0x1,
        GEOMETRY    = 0x2,
        ALL = 0xff
    };

    VIEW_ITEM() : m_view( NULL ), m_viewVisible( true ), m_groups( NULL ), m_groupsSize( 0 ) {}

    /**
     * Destructor. For dynamic views, removes the item from the view.
     */
    virtual ~VIEW_ITEM()
    {
        ViewRelease();
        delete[] m_groups;
    };

    /**
     * Function ViewBBox()
     * returns the bounding box of the item covering all its layers.
     * @return BOX2I - the current bounding box
     */
    virtual const BOX2I ViewBBox() const = 0;

    /**
     * Function ViewDraw()
     * Draws the parts of the object belonging to layer aLayer.
     * viewDraw() is an alternative way for drawing objects if
     * if there is no PAINTER assigned for the view or if the PAINTER
     * doesn't know how to paint this particular implementation of
     * VIEW_ITEM. The preferred way of drawing is to design an
     * appropriate PAINTER object, the method below is intended only
     * for quick hacks and debugging purposes.
     *
     * @param aLayer: current drawing layer
     * @param aGal: pointer to the GAL device we are drawing on
     * @param aVisibleArea: area (in world space coordinates) that is relevant for drawing. For
     * example, when drawing a bitmap, one can clip the blitting area to aVisibleArea, reducing
     * drawing time.
     */
    virtual void ViewDraw( int aLayer, GAL* aGal, const BOX2I& aVisibleArea ) const { };

    /**
     * Function ViewGetLayers()
     * Returns the all the layers within the VIEW the object is painted on. For instance, a D_PAD
     * spans one or more copper layers and a few technical layers. ViewDraw() or PAINTER::Draw() is
     * repeatedly called for each of the layers returned by ViewGetLayers(), depending on the
     * rendering order.
     * @param aLayers[]: output layer index array
     * @param aCount: number of layer indices in aLayers[]
     */

    virtual void ViewGetLayers( int aLayers[], int& aCount ) const = 0;

    /**
     * Function ViewSetVisible()
     * Sets the item visibility.
     *
     * @param aIsVisible: whether the item is visible (on all layers), or not.
     */
    void ViewSetVisible( bool aIsVisible = true );


    /**
     * Function ViewIsVisible()
     * Returns if the item is visible (or not).
     *
     * @return when true, the item is visible (i.e. to be displayed, not visible in the
     * *current* viewport)
     */
    bool ViewIsVisible() const
    {
        return m_viewVisible;
    }

    /**
     * Function ViewUpdate()
     * For dynamic VIEWs, informs the associated VIEW that the graphical representation of
     * this item has changed. For static views calling has no effect.
     *
     * @param aUpdateFlags: how much the object has changed
     * @param aForceImmediateRedraw: when true, the VIEW is redrawn immediately,
     *  otherwise, it will be redrawn upon next call of VIEW::Update()
     */
    virtual void ViewUpdate( int aUpdateFlags = ALL, bool aForceImmediateRedraw = false );

    /**
     * Function ViewRelease()
     * Releases the item from an associated dynamic VIEW. For static views calling has no effect.
     */
    void ViewRelease();

    /// @copydoc EDA_ITEM::Type()
    virtual KICAD_T Type() const = 0;

protected:
    friend class VIEW;

    /**
     * Function viewAssign()
     * Assigns the item to a given dynamic VIEW. Called internally by the VIEW.
     *
     * @param aView[]: dynamic VIEW instance the item is being added to.
     */

    void viewAssign( VIEW* aView )
    {
        // release the item from a previously assigned dynamic view (if there is any)
        ViewRelease();
        m_view = aView;
        deleteGroups();
    }

    VIEW*   m_view;         ///* Current dynamic view the item is assigned to.
    bool    m_viewVisible;  ///* Are we visible in the current dynamic VIEW.

private:
    ///* Helper for storing cached items group ids
    typedef std::pair<int, int> GroupPair;

    ///* Indexes of cached GAL display lists corresponding to the item (for every layer it occupies).
    ///* (in the std::pair "first" stores layer number, "second" stores group id).
    GroupPair* m_groups;
    int        m_groupsSize;

    /**
     * Function getGroup()
     * Returns number of the group id for the given layer, or -1 in case it was not cached before.
     *
     * @param aLayer is the layer number for which group id is queried.
     * @return group id or -1 in case there is no group id (ie. item is not cached).
     */
    int getGroup( int aLayer ) const;

    /**
     * Function getAllGroups()
     * Returns all group ids for the item (collected from all layers the item occupies).
     *
     * @return vector of group ids.
     */
    std::vector<int> getAllGroups() const;

    /**
     * Function setGroup()
     * Sets a group id for the item and the layer combination.
     *
     * @param aLayer is the layer numbe.
     * @param aGroup is the group id.
     */
    void setGroup( int aLayer, int aGroup );

    /**
     * Function deleteGroups()
     * Removes all of the stored group ids. Forces recaching of the item.
     */
    void deleteGroups();

    /**
     * Function storesGroups()
     * Returns information if the item uses at least one group id (ie. if it is cached at all).
     *
     * @returns true in case it is cached at least for one layer.
     */
    bool storesGroups() const;
};
} // namespace KiGfx

#endif
