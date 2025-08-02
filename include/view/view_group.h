/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2013 CERN
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
 *
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
 * along with this program; if not, you may find one here:
 * http://www.gnu.org/licenses/old-licenses/gpl-2.0.html
 * or you may search the http://www.gnu.org website for the version 2 license,
 * or you may write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
 *
 */

#pragma once

#include <gal/gal.h>
#include <view/view_item.h>
#include <deque>

namespace KIGFX
{
/**
 * Extend #VIEW_ITEM by possibility of grouping items into a single object.
 *
 * VIEW_GROUP does not take over ownership of the held items. The main purpose of this class is
 * to group items and draw them on a single layer (in particular the overlay).
 */
class GAL_API VIEW_GROUP : public VIEW_ITEM
{
public:
    VIEW_GROUP( VIEW* aView = nullptr );
    virtual ~VIEW_GROUP();

    // We own at least one list of raw pointers.  Don't let the compiler fill in copy c'tors that
    // will only land us in trouble.
    VIEW_GROUP( const VIEW_GROUP& ) = delete;
    VIEW_GROUP& operator=( const VIEW_GROUP& ) = delete;

    wxString GetClass() const override;

    /**
     * Return the number of stored items.
     */
    virtual unsigned int GetSize() const;

    /**
     * Add an item to the group.
     */
    virtual void Add( VIEW_ITEM* aItem );

    /**
     * Remove an item from the group.
     */
    virtual void Remove( VIEW_ITEM* aItem );

    /**
     * Remove all the stored items from the group.
     */
    virtual void Clear();

    virtual VIEW_ITEM* GetItem( unsigned int aIdx ) const;

    /**
     * Return the bounding box for all stored items covering all its layers.
     */
    virtual const BOX2I ViewBBox() const override;

    /**
     * Draw all the stored items in the group on the given layer.
     *
     * @param aLayer is the layer which should be drawn.
     * @param aView is the VIEW that should be used for drawing.
     */
    virtual void ViewDraw( int aLayer, VIEW* aView ) const override;

    ///@copydoc VIEW_ITEM::ViewGetLayers
    std::vector<int> ViewGetLayers() const override;

    /**
     * Set layer used to draw the group.
     */
    inline virtual void SetLayer( int aLayer )
    {
        m_layer = aLayer;
    }

    /**
     * Free all the items that were added to the group.
     */
    void FreeItems();

protected:
    virtual const std::vector<VIEW_ITEM*> updateDrawList() const;

protected:
    int                     m_layer;
    std::vector<VIEW_ITEM*> m_groupItems;       // No ownership.
};

} // namespace KIGFX
