/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2013-2016 CERN
 * Copyright (C) 2020-2021 KiCad Developers, see AUTHORS.txt for contributors.
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

#ifndef __VIEW_ITEM_H
#define __VIEW_ITEM_H

#include <gal/gal.h>
#include <vector>
#include <bitset>
#include <math/box2.h>
#include <inspectable.h>

#if defined( _MSC_VER )
#pragma warning( push )
#pragma warning( disable : 4275 )
#endif

namespace KIGFX
{
// Forward declarations
class VIEW;
class VIEW_ITEM_DATA;

 /**
  * Define the how severely the appearance of the item has been changed.
  */
enum VIEW_UPDATE_FLAGS {
    NONE        = 0x00,     ///< No updates are required.
    APPEARANCE  = 0x01,     ///< Visibility flag has changed.
    COLOR       = 0x02,     ///< Color has changed.
    GEOMETRY    = 0x04,     ///< Position or shape has changed.
    LAYERS      = 0x08,     ///< Layers have changed.
    INITIAL_ADD = 0x10,     ///< Item is being added to the view.
    REPAINT     = 0x20,     ///< Item needs to be redrawn.
    ALL         = 0xef      ///< All except INITIAL_ADD.
};

/**
 * Define the visibility of the item (temporarily hidden, invisible, etc).
 */
enum VIEW_VISIBILITY_FLAGS {
    VISIBLE        = 0x01,  ///< Item is visible (in general)
    HIDDEN         = 0x02,  ///< Item is temporarily hidden (usually in favor of a being drawn
                            ///<   from an overlay, such as a SELECTION).
                            ///<   Overrides VISIBLE flag.
    OVERLAY_HIDDEN = 0x04   ///< Item is temporarily hidden from being drawn on an overlay.
};

/**
 * An abstract base class for deriving all objects that can be added to a VIEW.
 *
 * Its role is to:
 * - communicate geometry, appearance and visibility updates to the associated dynamic VIEW,
 * - provide a bounding box for redraw area calculation,
 * - (optional) draw the object using the #GAL API functions for #PAINTER-less implementations.
 *
 * VIEW_ITEM objects are never owned by a #VIEW. A single VIEW_ITEM can belong to any number of
 * static VIEWs, but only one dynamic VIEW due to storage of only one VIEW reference.
 */
class GAL_API VIEW_ITEM : public INSPECTABLE
{
public:
    VIEW_ITEM( bool isSCH_ITEM = false, bool isBOARD_ITEM = false ) :
            m_isSCH_ITEM( isSCH_ITEM ),
            m_isBOARD_ITEM( isBOARD_ITEM ),
            m_viewPrivData( nullptr ),
            m_forcedTransparency( 0.0 )
    {
    }

    virtual ~VIEW_ITEM();

    VIEW_ITEM( const VIEW_ITEM& aOther ) = delete;
    VIEW_ITEM& operator=( const VIEW_ITEM& aOther ) = delete;

    bool IsSCH_ITEM() const { return m_isSCH_ITEM; }
    bool IsBOARD_ITEM() const { return m_isBOARD_ITEM; }
    /**
     * Return the bounding box of the item covering all its layers.
     *
     * @return the current bounding box.
     */
    virtual const BOX2I ViewBBox() const = 0;

    /**
     * Draw the parts of the object belonging to layer aLayer.
     *
     * An alternative way for drawing objects if there is no #PAINTER assigned for the view
     * or if the PAINTER doesn't know how to paint this particular implementation of VIEW_ITEM.
     * The preferred way of drawing is to design an appropriate PAINTER object, the method
     * below is intended only for quick hacks and debugging purposes.
     *
     * @param aLayer is the current drawing layer.
     * @param aView is a pointer to the #VIEW device we are drawing on.
     */
    virtual void ViewDraw( int aLayer, VIEW* aView ) const
    {}

    /**
     * Return the all the layers within the VIEW the object is painted on.
     *
     * For instance, a #PAD spans zero or more copper layers and a few technical layers.
     * ViewDraw() or PAINTER::Draw() is repeatedly called for each of the layers returned
     * by ViewGetLayers(), depending on the rendering order.
     *
     * @param aLayers[] is the output layer index array.
     * @param aCount is the number of layer indices in aLayers[].
     */
    virtual void ViewGetLayers( int aLayers[], int& aCount ) const = 0;

    /**
     * Return the level of detail (LOD) of the item.
     *
     * A level of detail is the minimal #VIEW scale that is sufficient for an item to be shown
     * on a given layer.
     *
     * @param aLayer is the current drawing layer.
     * @param aView is a pointer to the #VIEW device we are drawing on.
     * @return the level of detail. 0 always show the item, because the actual zoom level
     *         (or VIEW scale) is always > 0
     */
    virtual double ViewGetLOD( int aLayer, VIEW* aView ) const
    {
        // By default always show the item
        return 0.0;
    }

    VIEW_ITEM_DATA* viewPrivData() const
    {
        return m_viewPrivData;
    }

    void SetForcedTransparency( double aForcedTransparency )
    {
        m_forcedTransparency = aForcedTransparency;
    }

    double GetForcedTransparency() const
    {
        return m_forcedTransparency;
    }

private:
    friend class VIEW;

    bool            m_isSCH_ITEM;
    bool            m_isBOARD_ITEM;
    VIEW_ITEM_DATA* m_viewPrivData;
    double          m_forcedTransparency;  ///< Additional transparency for diff'ing items.
};

} // namespace KIGFX

#if defined( _MSC_VER )
#pragma warning( pop )
#endif

#endif
