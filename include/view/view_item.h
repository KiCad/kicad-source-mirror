/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2013-2016 CERN
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
#include <bitset>
#include <math/box2.h>

namespace KIGFX
{
// Forward declarations
class VIEW;
class VIEW_ITEM_DATA;

 /**
  * Enum VIEW_UPDATE_FLAGS.
  * Defines the how severely the shape/appearance of the item has been changed:
  * - NONE: TODO
  * - APPEARANCE: shape or layer set of the item have not been affected,
  * only colors or visibility.
  * - COLOR:
  * - GEOMETRY: shape or layer set of the item have changed, VIEW may need to reindex it.
  * - LAYERS: TODO
  * - ALL: all the flags above */
enum VIEW_UPDATE_FLAGS {
    NONE        = 0x00,     /// No updates are required
    APPEARANCE  = 0x01,     /// Visibility flag has changed
    COLOR       = 0x02,     /// Color has changed
    GEOMETRY    = 0x04,     /// Position or shape has changed
    LAYERS      = 0x08,     /// Layers have changed
    ALL         = 0xff
};

/**
 * Enum VIEW_VISIBILITY_FLAGS.
 * Defines the visibility of the item (temporarily hidden, invisible, etc).
 */
enum VIEW_VISIBILITY_FLAGS {
    VISIBLE     = 0x01,     /// Item is visible (in general)
    HIDDEN      = 0x02      /// Item is temporarily hidden (e.g. being used by a tool). Overrides VISIBLE flag.
};

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
    VIEW_ITEM() : m_viewPrivData( nullptr )
    {

    }

    virtual ~VIEW_ITEM();

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
     */
    virtual void ViewDraw( int aLayer, VIEW* aView ) const
    {}

    /**
     * Function ViewGetLayers()
     * Returns the all the layers within the VIEW the object is painted on. For instance, a D_PAD
     * spans zero or more copper layers and a few technical layers. ViewDraw() or PAINTER::Draw() is
     * repeatedly called for each of the layers returned by ViewGetLayers(), depending on the
     * rendering order.
     * @param aLayers[]: output layer index array
     * @param aCount: number of layer indices in aLayers[]
     */
    virtual void ViewGetLayers( int aLayers[], int& aCount ) const = 0;

    /**
     * Function ViewGetLOD()
     * Returns the level of detail of the item. A level of detail is the minimal VIEW scale that
     * is sufficient for an item to be shown on a given layer.
     */
    virtual unsigned int ViewGetLOD( int aLayer, VIEW* aView ) const
    {
        // By default always show the item
        return 0;
    }

public:

    VIEW_ITEM_DATA* viewPrivData() const
    {
        return m_viewPrivData;
    }

private:
    friend class VIEW;

    VIEW_ITEM_DATA* m_viewPrivData;
};

} // namespace KIGFX

#endif
