/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2013 CERN
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

/**
 * @file view_group.h
 * @brief VIEW_GROUP extends VIEW_ITEM by possibility of grouping items into a single object.
 * VIEW_GROUP does not take over ownership of the held items. The main purpose of this class is
 * to group items and draw them on a single layer (in particular the overlay).
 */

#ifndef VIEW_GROUP_H_
#define VIEW_GROUP_H_

#include <view/view_item.h>
#include <deque>

namespace KIGFX
{
class VIEW_GROUP : public VIEW_ITEM
{
public:
    VIEW_GROUP( VIEW* aView = NULL );
    virtual ~VIEW_GROUP();

    /// Helper typedefs
    typedef std::set<VIEW_ITEM*>::const_iterator const_iter;
    typedef std::set<VIEW_ITEM*>::iterator iter;

    /**
     * Function Add()
     * Adds an item to the group.
     *
     * @param aItem is the item to be added.
     */
    virtual void Add( VIEW_ITEM* aItem );

    /**
     * Function Remove()
     * Removes an item from the group.
     *
     * @param aItem is the item to be removed.
     */
    virtual void Remove( VIEW_ITEM* aItem );

    /**
     * Function Clear()
     * Removes all the stored items from the group.
     */
    virtual void Clear();

    /**
     * Function Begin()
     * Returns iterator to beginning.
     */
    inline const_iter Begin() const
    {
        return m_items.begin();
    }

    /**
     * Function End()
     * Returns iterator to end.
     */
    inline const_iter End() const
    {
        return m_items.end();
    }

    /**
     * Function GetSize()
     * Returns the number of stored items.
     *
     * @return Number of stored items.
     */
    virtual unsigned int GetSize() const;

    /**
     * Function ViewBBox()
     * Returns the bounding box for all stored items covering all its layers.
     *
     * @return The current bounding box
     */
    virtual const BOX2I ViewBBox() const override;

    /**
     * Function ViewDraw()
     * Draws all the stored items in the group on the given layer.
     *
     * @param aLayer is the layer which should be drawn.
     * @param aGal is the GAL that should be used for drawing.
     */
    virtual void ViewDraw( int aLayer, GAL* aGal ) const override;

    /**
     * Function ViewGetLayers()
     * Returns all the layers used by the stored items.
     *
     * @param aLayers[] is the output layer index array.
     * @param aCount is the number of layer indices in aLayers[].
     */
    virtual void ViewGetLayers( int aLayers[], int& aCount ) const override;

    /**
     * Function SetLayer()
     * Sets layer used to draw the group.
     *
     * @param aLayer is the layer used for drawing.
     */
    inline virtual void SetLayer( int aLayer )
    {
        m_layer = aLayer;
        ViewUpdate();
    }

    /**
     * Function FreeItems()
     * Frees all the items that were added to the group.
     */
    void FreeItems();

    /**
     * Function GetView()
     * Returns pointer to the VIEW instance used by items.
     *
     * @return Pointer to the VIEW instance.
     */
    KIGFX::VIEW* GetView() const
    {
        return m_view;
    }

    /**
     * Function ItemsSetVisibility()
     * Sets visibility of items stored in the VIEW_GROUP.
     *
     * @param aVisible decides if items should be visible or not.
     */
    virtual void ItemsSetVisibility( bool aVisible );

    /**
     * Function ItemsViewUpdate()
     * Updates items stored in the VIEW_GROUP.
     *
     * @param aFlags determines the way in which items will be updated.
     */
    virtual void ItemsViewUpdate( VIEW_ITEM::VIEW_UPDATE_FLAGS aFlags );

protected:
    /// These functions cannot be used with VIEW_GROUP as they are intended only to work with
    /// singular VIEW_ITEMs (there is only one-to-one relation between item/layer combination and
    /// its group).
    int getGroup( int aLayer ) const override
    {
        return -1;
    }

    std::vector<int> getAllGroups() const override
    {
        return std::vector<int>();
    }

    void setGroup( int aLayer, int aGroup ) override
    {}

    void deleteGroups() override
    {}

    bool storesGroups() const override
    {
        return false;
    }

    /// Layer on which the group is drawn
    int m_layer;

private:
    void updateBbox();

    /// Container for storing VIEW_ITEMs
    std::set<VIEW_ITEM*> m_items;
};
} // namespace KIGFX

#endif // VIEW_GROUP_H_
