/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation, either version 3 of the License, or (at your
 * option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include <tools/hover_picker.h>

#include <board.h>
#include <board_item.h>
#include <collectors.h>
#include <tool/tool_manager.h>
#include <tools/pcb_selection_tool.h>

#include <limits>
#include <set>


HOVER_PICKER::HOVER_PICKER( TOOL_MANAGER* aToolMgr ) :
        m_toolMgr( aToolMgr ),
        m_selectionTool( aToolMgr->GetTool<PCB_SELECTION_TOOL>() )
{
}


HOVER_PICKER::~HOVER_PICKER()
{
    ClearBrightening();
}


BOARD_ITEM* HOVER_PICKER::Pick( const VECTOR2I& aPointer, const ACCEPTS& aAccepts,
                                const PROXIMITY& aProximity ) const
{
    std::vector<BOARD_ITEM*> candidates = m_selectionTool->CollectPoint(
            aPointer,
            [&aAccepts]( const VECTOR2I&, GENERAL_COLLECTOR& aCollector, PCB_SELECTION_TOOL* )
            {
                for( int i = aCollector.GetCount() - 1; i >= 0; --i )
                {
                    if( !aAccepts( *aCollector[i] ) )
                        aCollector.Remove( i );
                }
            } );

    if( candidates.empty() )
        return nullptr;

    if( !aProximity )
        return candidates.front();

    BOARD_ITEM* best = nullptr;
    double      bestProximity = std::numeric_limits<double>::infinity();

    for( BOARD_ITEM* item : candidates )
    {
        double proximity = aProximity( *item, aPointer );

        if( proximity < bestProximity )
        {
            bestProximity = proximity;
            best = item;
        }
    }

    return best;
}


void HOVER_PICKER::Brighten( BOARD_ITEM* aItem )
{
    m_brightened.push_back( aItem->m_Uuid );
    m_selectionTool->BrightenItem( aItem );
}


void HOVER_PICKER::BrightenOnly( const std::vector<BOARD_ITEM*>& aItems )
{
    std::set<KIID> wanted;

    for( BOARD_ITEM* item : aItems )
        wanted.insert( item->m_Uuid );

    BOARD* board = static_cast<BOARD*>( m_toolMgr->GetModel() );

    for( const KIID& id : m_brightened )
    {
        if( !wanted.contains( id ) )
        {
            if( BOARD_ITEM* item = board->ResolveItem( id, true ) )
                m_selectionTool->UnbrightenItem( item );
        }
    }

    const std::set<KIID> lit( m_brightened.begin(), m_brightened.end() );

    for( BOARD_ITEM* item : aItems )
    {
        if( !lit.contains( item->m_Uuid ) )
            m_selectionTool->BrightenItem( item );
    }

    m_brightened.assign( wanted.begin(), wanted.end() );
}


void HOVER_PICKER::ClearBrightening()
{
    BOARD* board = static_cast<BOARD*>( m_toolMgr->GetModel() );

    for( const KIID& id : m_brightened )
    {
        if( BOARD_ITEM* item = board->ResolveItem( id, true ) )
            m_selectionTool->UnbrightenItem( item );
    }

    m_brightened.clear();
}
