/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2013 CERN
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
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
#include <algorithm>
#include <view/view_group.h>
#include <view/view.h>
#include <view/view_item.h>
#include <gal/painter.h>
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


wxString VIEW_GROUP::GetClass() const
{
    return wxT( "VIEW_GROUP" );
}


void VIEW_GROUP::Add( VIEW_ITEM* aItem )
{
    m_groupItems.push_back( aItem );
}


void VIEW_GROUP::Remove( VIEW_ITEM* aItem )
{
    std::erase( m_groupItems, aItem );
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

    std::map<int, std::vector<VIEW_ITEM*>> layer_item_map;

    // Build a list of layers used by the items in the group
    for( VIEW_ITEM* item : drawList )
    {
        if( aView->IsHiddenOnOverlay( item ) )
            continue;

        std::vector<int> layers = item->ViewGetLayers();

        for( auto layer : layers )
        {
            wxCHECK2_MSG( layer <= LAYER_ID_COUNT, continue, wxT( "Invalid item layer" ) );
            layer_item_map[ layer ].push_back( item );
        }
    }

    if( layer_item_map.empty() )
        return;

    std::vector<int> layers;
    layers.reserve( layer_item_map.size() );

    for( const std::pair<const int, std::vector<VIEW_ITEM*>>& entry : layer_item_map )
        layers.push_back( entry.first );

    aView->SortLayers( layers );

    // Now draw the layers in sorted order

    GAL_SCOPED_ATTRS scopedAttrs( *gal, GAL_SCOPED_ATTRS::LAYER_DEPTH );

    for( int layer : layers )
    {
        bool draw = aView->IsLayerVisible( layer );

        if( IsZoneFillLayer( layer ) )
        {
            // The visibility of solid areas must follow the visiblility of the zone layer
            int zone_main_layer = layer - LAYER_ZONE_START;
            draw = aView->IsLayerVisible( zone_main_layer );
        }
        else if( IsPadCopperLayer( layer ) )
        {
            draw = aView->IsLayerVisible( layer - LAYER_PAD_COPPER_START );
        }
        else if( IsViaCopperLayer( layer ) )
        {
            draw = aView->IsLayerVisible( layer - LAYER_VIA_COPPER_START );
        }
        else if( IsClearanceLayer( layer ) )
        {
            draw = aView->IsLayerVisible( layer - LAYER_CLEARANCE_START );
        }
        else if( IsPointsLayer( layer ) )
        {
            draw = aView->IsLayerVisible( layer - LAYER_POINT_START );
        }

        if( isSelection )
        {
            switch( layer )
            {
            case LAYER_PAD_PLATEDHOLES:
            case LAYER_PAD_HOLEWALLS:
                draw = true;
                break;

            default:
                break;
            }
        }

        if( draw )
        {
            gal->AdvanceDepth();

            for( VIEW_ITEM* item : layer_item_map[ layer ] )
            {
                // Ignore LOD scale for selected items, but don't ignore things explicitly
                // hidden.
                if( item->ViewGetLOD( layer, aView ) == LOD_HIDE )
                    continue;

                if( !painter->Draw( item, layer ) )
                    item->ViewDraw( layer, aView ); // Alternative drawing method
            }
        }
    }
}


std::vector<int> VIEW_GROUP::ViewGetLayers() const
{
    // Everything is displayed on a single layer
    return { m_layer };
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
