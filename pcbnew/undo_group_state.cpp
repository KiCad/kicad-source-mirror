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

#include "undo_group_state.h"

#include <board.h>
#include <pcb_group.h>
#include <undo_redo_container.h>

#include <unordered_set>


UNDO_GROUP_STATE::UNDO_GROUP_STATE( BOARD& aBoard, const PICKED_ITEMS_LIST& aList ) :
        m_board( aBoard )
{
    for( unsigned ii = 0; ii < aList.GetCount(); ++ii )
    {
        const ITEM_PICKER& wrapper = aList.GetItemWrapper( ii );

        if( wrapper.GetStatus() == UNDO_REDO::DELETED )
            continue;

        // A swap such as ExchangeFootprint can leave the picker holding a replaced item
        BOARD_ITEM* item = m_board.ResolveItem( wrapper.GetItem()->m_Uuid, true );

        if( !item )
            continue;

        STATE& state = m_captured[wrapper.GetItem()];
        state.m_groupId = item->GetParentGroupId();

        if( PCB_GROUP* group = dynamic_cast<PCB_GROUP*>( item ) )
            state.m_members = group->GetGroupMemberIds();
    }
}


void UNDO_GROUP_STATE::Restore( PICKED_ITEMS_LIST& aList )
{
    // An appended command can hold the same item twice, and its first picker is the state to restore
    std::unordered_set<BOARD_ITEM*>                        restored;
    std::vector<std::pair<PCB_GROUP*, const ITEM_PICKER*>> groups;

    for( unsigned ii = 0; ii < aList.GetCount(); ++ii )
    {
        const ITEM_PICKER& wrapper = aList.GetItemWrapper( ii );

        if( wrapper.GetStatus() == UNDO_REDO::DELETED )
            continue;

        BOARD_ITEM* item = m_board.ResolveItem( wrapper.GetItem()->m_Uuid, true );

        if( !item || !restored.insert( item ).second )
            continue;

        PCB_GROUP* parentGroup = dynamic_cast<PCB_GROUP*>( m_board.ResolveItem( wrapper.GetGroupId(), true ) );
        item->SetParentGroup( parentGroup );

        // BOARD::Remove() dropped the item from its group's member list while the list was applied
        if( parentGroup )
            parentGroup->GetItems().insert( item );

        if( PCB_GROUP* group = dynamic_cast<PCB_GROUP*>( item ) )
            groups.emplace_back( group, &wrapper );
    }

    // Groups go last so their recorded members win over an item that left the group before the push
    for( const auto& [group, wrapper] : groups )
    {
        // Items list may contain dodgy pointers, so don't use RemoveAll()
        group->GetItems().clear();

        for( const KIID& member : wrapper->GetGroupMembers() )
        {
            if( BOARD_ITEM* memberItem = m_board.ResolveItem( member, true ) )
                group->AddItem( memberItem );
        }
    }

    for( unsigned ii = 0; ii < aList.GetCount(); ++ii )
    {
        ITEM_PICKER& wrapper = aList.GetItemWrapper( ii );

        if( auto it = m_captured.find( wrapper.GetItem() ); it != m_captured.end() )
            wrapper.SetGroupState( it->second.m_groupId, it->second.m_members );
    }
}
