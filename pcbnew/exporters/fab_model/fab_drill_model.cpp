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

#include <exporters/fab_model/fab_drill_model.h>

#include <board.h>
#include <drill/drill_enumerator.h>

#include <utility>


FAB_DRILL_MODEL::FAB_DRILL_MODEL( const BOARD& aBoard )
{
    for( const DRILL_SPAN& span : EnumerateDrillSpans( aBoard, true ) )
    {
        DRILL_QUERY query;
        query.m_Span = span;
        query.m_MergePTHNPTH = true;
        query.m_PadMachining = true;

        FAB_DRILL_LAYER layer;
        layer.m_Span = span;

        for( DRILL_OPERATION& operation : EnumerateDrillOperations( aBoard, query ) )
        {
            if( operation.m_IsSlot )
                layer.m_Slots.push_back( std::move( operation ) );
            else
                layer.m_Holes.push_back( std::move( operation ) );
        }

        m_layers.push_back( std::move( layer ) );
    }

    for( const FAB_DRILL_LAYER& layer : m_layers )
    {
        auto& items = m_index[{ layer.m_Span.Pair(), layer.m_Span.m_IsBackdrill }];

        for( const DRILL_OPERATION& op : layer.m_Holes )
            items.try_emplace( op.m_SourceItem, &op );

        for( const DRILL_OPERATION& op : layer.m_Slots )
            items.try_emplace( op.m_SourceItem, &op );
    }
}


const DRILL_OPERATION* FAB_DRILL_MODEL::Find( const DRILL_SPAN& aSpan, const BOARD_ITEM* aItem,
                                              bool aIgnorePlating ) const
{
    auto layer = m_index.find( { aSpan.Pair(), aSpan.m_IsBackdrill } );

    if( layer == m_index.end() )
        return nullptr;

    auto item = layer->second.find( aItem );

    if( item == layer->second.end() )
        return nullptr;

    const DRILL_OPERATION* op = item->second;
    return aIgnorePlating || op->m_NotPlated == aSpan.m_IsNonPlatedFile ? op : nullptr;
}
