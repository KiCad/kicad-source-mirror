/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
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
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include "fab_layer_index.h"

#include "fab_item_order.h"
#include <algorithm>
#include <board.h>
#include <board_connected_item.h>
#include <footprint.h>
#include <pad.h>
#include <pcb_field.h>
#include <pcb_shape.h>
#include <pcb_track.h>
#include <zone.h>


FAB_LAYER_INDEX::FAB_LAYER_INDEX( BOARD& aBoard )
{
    LSET enabled = aBoard.GetEnabledLayers();

    for( PCB_TRACK* track : aBoard.Tracks() )
    {
        if( track->Type() == PCB_VIA_T )
        {
            PCB_VIA* via = static_cast<PCB_VIA*>( track );

            for( PCB_LAYER_ID layer : enabled )
            {
                if( via->FlashLayer( layer ) )
                    m_items[layer][via->GetNetCode()].push_back( via );
            }
        }
        else
        {
            for( PCB_LAYER_ID layer : track->GetLayerSet().Seq() )
                m_items[layer][track->GetNetCode()].push_back( track );
        }
    }

    for( ZONE* zone : aBoard.Zones() )
    {
        for( PCB_LAYER_ID layer : zone->GetLayerSet().Seq() )
            m_items[layer][zone->GetNetCode()].push_back( zone );
    }

    for( BOARD_ITEM* item : aBoard.Drawings() )
    {
        int netCode = 0;

        if( BOARD_CONNECTED_ITEM* connected = dynamic_cast<BOARD_CONNECTED_ITEM*>( item ) )
            netCode = connected->GetNetCode();

        for( PCB_LAYER_ID layer : item->GetLayerSet().Seq() )
            m_items[layer][netCode].push_back( item );
    }

    for( FOOTPRINT* footprint : aBoard.Footprints() )
    {
        for( PCB_FIELD* field : footprint->GetFields() )
            m_items[field->GetLayer()][0].push_back( field );

        for( BOARD_ITEM* item : footprint->GraphicalItems() )
        {
            for( PCB_LAYER_ID layer : item->GetLayerSet().Seq() )
                m_items[layer][0].push_back( item );
        }

        for( PAD* pad : footprint->Pads() )
        {
            for( PCB_LAYER_ID layer : pad->GetLayerSet().Seq() )
            {
                if( pad->FlashLayer( layer ) )
                    m_items[layer][pad->GetNetCode()].push_back( pad );
            }
        }
    }

    for( auto& [layer, nets] : m_items )
    {
        for( auto& [netCode, items] : nets )
            std::stable_sort( items.begin(), items.end(), FabItemLess );
    }
}


const FAB_LAYER_INDEX::NET_ITEMS& FAB_LAYER_INDEX::Items( PCB_LAYER_ID aLayer ) const
{
    static const NET_ITEMS empty;
    auto                   it = m_items.find( aLayer );
    return it != m_items.end() ? it->second : empty;
}


int FabTrackWidth( const PCB_TRACK& aTrack, PCB_LAYER_ID aLayer )
{
    int width = aTrack.GetWidth();

    if( IsSolderMaskLayer( aLayer ) )
        width += 2 * aTrack.GetSolderMaskExpansion();

    return width;
}


bool FabMaskShape( const PCB_SHAPE& aShape, PCB_LAYER_ID aLayer, PCB_SHAPE& aResult )
{
    if( !IsSolderMaskLayer( aLayer ) || !aShape.HasSolderMask() || !IsExternalCopperLayer( aShape.GetLayer() )
        || !aShape.GetSolderMaskExpansion() )
    {
        return false;
    }

    aResult = aShape;
    int expansion = aShape.GetSolderMaskExpansion();
    int thickness = aShape.GetWidth() + 2 * expansion;
    aResult.SetWidth( std::max( thickness, 0 ) );

    // Negative expansion also shrinks closed shapes
    if( thickness < 0 )
    {
        int excess = -thickness;

        switch( aShape.GetShape() )
        {
        case SHAPE_T::CIRCLE: aResult.SetRadius( std::max( aShape.GetRadius() - excess, 1 ) ); break;

        case SHAPE_T::RECTANGLE:
            aResult.SetRectangleWidth( aShape.GetRectangleWidth() - 2 * excess );
            aResult.SetRectangleHeight( aShape.GetRectangleHeight() - 2 * excess );
            aResult.SetCornerRadius( std::max( aShape.GetCornerRadius() - excess, 0 ) );
            break;

        case SHAPE_T::POLY:
            if( aShape.IsPolyShapeValid() )
            {
                SHAPE_POLY_SET& poly = aResult.GetPolyShape();
                poly.Fracture();
                poly.Deflate( excess, CORNER_STRATEGY::ROUND_ALL_CORNERS, aShape.GetMaxError() );
            }

            break;

        default: break;
        }
    }

    return true;
}
