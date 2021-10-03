/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2013 CERN
 * Copyright (C) 2021 KiCad Developers, see AUTHORS.txt for contributors.
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
#include <core/kicad_algo.h>
#include <view/view_group.h>
#include <view/view.h>
#include <painter.h>
#include <gal/graphics_abstraction_layer.h>
#include <layer_ids.h>

using namespace KIGFX;

VIEW_GROUP::VIEW_GROUP( VIEW* aView ) :
    VIEW_ITEM(),
    m_layer( LAYER_SELECT_OVERLAY )
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
    alg::delete_matching( m_groupItems, aItem );
}


void VIEW_GROUP::Clear()
{
    m_groupItems.clear();
}


unsigned int VIEW_GROUP::GetSize() const
{
    return m_groupItems.size();
}


VIEW_ITEM *VIEW_GROUP::GetItem( unsigned int idx ) const
{
    return m_groupItems[idx];
}


const BOX2I VIEW_GROUP::ViewBBox() const
{
    BOX2I bb;

    if( !m_groupItems.size() )
    {
        bb.SetMaximum();
    }
    else
    {
        bb = m_groupItems[0]->ViewBBox();

        for( VIEW_ITEM* item : m_groupItems )
            bb.Merge( item->ViewBBox() );
    }

    return bb;
}


void VIEW_GROUP::ViewDraw( int aLayer, VIEW* aView ) const
{
    KIGFX::GAL* gal = aView->GetGAL();
    PAINTER*    painter = aView->GetPainter();
    bool        isSelection = m_layer == LAYER_SELECT_OVERLAY;

    const std::vector<VIEW_ITEM*> drawList = updateDrawList();

    std::unordered_map<int, std::vector<VIEW_ITEM*>> layer_item_map;

    // Build a list of layers used by the items in the group
    for( VIEW_ITEM* item : drawList )
    {
        int item_layers[VIEW::VIEW_MAX_LAYERS], item_layers_count;
        item->ViewGetLayers( item_layers, item_layers_count );

        for( int i = 0; i < item_layers_count; i++ )
        {
            if( layer_item_map.count( item_layers[i] ) == 0 )
            {
                layer_item_map.emplace( std::make_pair( item_layers[i],
                                                        std::vector<VIEW_ITEM*>() ) );
            }

            layer_item_map[ item_layers[i] ].push_back( item );
        }
    }

    int layers[VIEW::VIEW_MAX_LAYERS] = { 0 };
    int layers_count = 0;

    for( const std::pair<const int, std::vector<VIEW_ITEM*>>& entry : layer_item_map )
        layers[ layers_count++ ] = entry.first;

    aView->SortLayers( layers, layers_count );

    // Now draw the layers in sorted order

    gal->PushDepth();

    for( int i = 0; i < layers_count; i++ )
    {
        int  layer = layers[i];
        bool draw = aView->IsLayerVisible( layer );

        if( isSelection )
        {
            switch( layer )
            {
            case LAYER_PADS_TH:
            case LAYER_PAD_PLATEDHOLES:
            case LAYER_PAD_HOLEWALLS:
            case LAYER_PAD_FR:
            case LAYER_PAD_BK:
                draw = true;
                break;
            default:
                break;
            }
        }

        if( draw )
        {
            gal->AdvanceDepth();

            for( VIEW_ITEM* item : layer_item_map[ layers[i] ] )
            {
                if( !painter->Draw( item, layers[i] ) )
                    item->ViewDraw( layers[i], aView ); // Alternative drawing method
            }
        }
    }

    gal->PopDepth();
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


const std::vector<VIEW_ITEM*> VIEW_GROUP::updateDrawList() const
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
