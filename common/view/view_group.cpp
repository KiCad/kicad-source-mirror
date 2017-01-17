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
 * @file view_group.cpp
 * @brief VIEW_GROUP extends VIEW_ITEM by possibility of grouping items into a single object.
 * VIEW_GROUP does not take over ownership of the held items. The main purpose of this class is
 * to group items and draw them on a single layer (in particular the overlay).
 */

#include <set>
#include <algorithm>
#include <view/view_group.h>
#include <view/view.h>
#include <painter.h>
#include <gal/graphics_abstraction_layer.h>
#include <layers_id_colors_and_visibility.h>

using namespace KIGFX;

VIEW_GROUP::VIEW_GROUP( VIEW* aView ) :
    m_layer( ITEM_GAL_LAYER( GP_OVERLAY ) )
{
}


VIEW_GROUP::~VIEW_GROUP()
{
    // VIEW_ITEM destructor removes the object from its parent view
}


void VIEW_GROUP::Add( VIEW_ITEM* aItem )
{
    m_groupItems.push_back( aItem );
}


void VIEW_GROUP::Remove( VIEW_ITEM* aItem )
{
    for( auto iter = m_groupItems.begin(); iter != m_groupItems.end(); ++iter)
    {
        if( aItem == *iter )
        {
            m_groupItems.erase( iter);
            break;
        }
    }
}


void VIEW_GROUP::Clear()
{
    m_groupItems.clear();
}


unsigned int VIEW_GROUP::GetSize() const
{
    return m_groupItems.size();
}

VIEW_ITEM *VIEW_GROUP::GetItem(unsigned int idx) const
{
    return m_groupItems[idx];
}

const BOX2I VIEW_GROUP::ViewBBox() const
{
    BOX2I maxBox;

    maxBox.SetMaximum();
    return maxBox;
}


void VIEW_GROUP::ViewDraw( int aLayer, VIEW* aView ) const
{
    auto gal = aView->GetGAL();
    PAINTER* painter = aView->GetPainter();

    const auto drawList = updateDrawList();

    // Draw all items immediately (without caching)
    for( auto item : drawList )
    {
        gal->PushDepth();

        int layers[VIEW::VIEW_MAX_LAYERS], layers_count;
        item->ViewGetLayers( layers, layers_count );
        aView->SortLayers( layers, layers_count );

        for( int i = 0; i < layers_count; i++ )
        {
            if( aView->IsLayerVisible( layers[i] ) )
            {
                gal->AdvanceDepth();

                if( !painter->Draw( item, layers[i] ) )
                    item->ViewDraw( layers[i], aView ); // Alternative drawing method
            }
        }

        gal->PopDepth();
    }
}


void VIEW_GROUP::ViewGetLayers( int aLayers[], int& aCount ) const
{
    // Everything is displayed on a single layer
    aLayers[0] = m_layer;
    aCount = 1;
}


void VIEW_GROUP::FreeItems()
{
    for( unsigned int i = 0 ; i < GetSize(); i++ )
        delete GetItem( i );

    Clear();
}


const VIEW_GROUP::ITEMS VIEW_GROUP::updateDrawList() const
{
    return m_groupItems;
}


/*void VIEW_GROUP::ItemsSetVisibility( bool aVisible )
{
    for(unsigned int i = 0 ; i < GetSize(); i++)
        GetItem(i)->ViewSetVisible( aVisible );
}


void VIEW_GROUP::ItemsViewUpdate( VIEW_ITEM::VIEW_UPDATE_FLAGS aFlags )
{
    for(unsigned int i = 0 ; i < GetSize(); i++)
        GetItem(i)->ViewUpdate( aFlags );
}*/


void VIEW_GROUP::updateBbox()
{

}
