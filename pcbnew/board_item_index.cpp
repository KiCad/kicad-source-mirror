/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright The KiCad Developers.
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

#include "board_item_index.h"

#include <functional>

#include <wx/log.h>

#include <board.h>
#include <board_item.h>


BOARD_ITEM* BOARD_ITEM_INDEX::Find( const KIID& aId )
{
    auto it = m_itemsById.find( aId );

    if( it == m_itemsById.end() )
        return nullptr;

    BOARD_ITEM* item = it->second;

    if( item && item->m_Uuid == aId )
        return item;

    Unbind( aId );
    return nullptr;
}


void BOARD_ITEM_INDEX::BindItem( BOARD_ITEM* aItem )
{
    if( m_owner.IsFootprintHolder() )
        return;

    // Hand the item to this board. A stale owner cannot evict it.
    if( aItem->m_boardCacheOwner && aItem->m_boardCacheOwner != &m_owner )
        aItem->m_boardCacheOwner->UncacheItemByPtr( aItem );

    // Called once per item on load, so probe and insert in one lookup per map and pay for the
    // aliasing fixups only when a key was already taken
    auto [idIt, idInserted] = m_itemsById.try_emplace( aItem->m_Uuid, aItem );
    auto [itemIt, itemInserted] = m_idsByItem.try_emplace( aItem, aItem->m_Uuid );

    if( !itemInserted && itemIt->second != aItem->m_Uuid )
    {
        // The item was indexed under an older UUID; drop that forward alias
        auto prevIt = m_itemsById.find( itemIt->second );

        if( prevIt != m_itemsById.end() && prevIt->second == aItem )
            m_itemsById.erase( prevIt );

        itemIt->second = aItem->m_Uuid;
    }

    if( !idInserted && idIt->second != aItem )
    {
        // Another item already claims this UUID; evict it
        if( auto prev = m_idsByItem.find( idIt->second );
            prev != m_idsByItem.end() && prev->second == aItem->m_Uuid )
        {
            idIt->second->m_boardCacheOwner = nullptr;
            m_idsByItem.erase( prev );
        }

        idIt->second = aItem;
    }

    // Set owner last, see BindAndReturnItem()
    aItem->m_boardCacheOwner = &m_owner;
}


void BOARD_ITEM_INDEX::Unbind( const KIID& aId )
{
    auto it = m_itemsById.find( aId );

    if( it == m_itemsById.end() )
        return;

    const BOARD_ITEM* item = it->second;

    m_itemsById.erase( it );

    if( auto cached = m_idsByItem.find( item );
        cached != m_idsByItem.end() && cached->second == aId )
    {
        item->m_boardCacheOwner = nullptr;
        m_idsByItem.erase( cached );
    }
}


void BOARD_ITEM_INDEX::UnbindByPtr( const BOARD_ITEM* aItem )
{
    // Clear owner first, the item no longer points to this board
    aItem->m_boardCacheOwner = nullptr;

    if( auto cached = m_idsByItem.find( aItem ); cached != m_idsByItem.end() )
    {
        auto it = m_itemsById.find( cached->second );

        if( it != m_itemsById.end() && it->second == aItem )
            m_itemsById.erase( it );

        m_idsByItem.erase( cached );
        return;
    }

    std::erase_if( m_itemsById, [aItem]( const auto& entry ) { return entry.second == aItem; } );
}


void BOARD_ITEM_INDEX::Clear()
{
    for( const auto& [item, id] : m_idsByItem )
        item->m_boardCacheOwner = nullptr;

    m_itemsById.clear();
    m_idsByItem.clear();
}


void BOARD_ITEM_INDEX::Rebind( BOARD_ITEM* aItem, const KIID& aNewId )
{
    wxCHECK_RET( aItem, "BOARD_ITEM_INDEX::Rebind() requires a valid item" );

    if( m_owner.IsFootprintHolder() )
        return;

    if( aItem->m_Uuid == aNewId )
    {
        BindItem( aItem );
        return;
    }

    if( BOARD_ITEM* existing = Find( aNewId ); existing && existing != aItem )
    {
        wxFAIL_MSG( wxString::Format( "BOARD_ITEM_INDEX::Rebind() duplicate target UUID: %s", aNewId.AsString() ) );
        return;
    }

    UnbindByPtr( aItem );
    aItem->SetUuidDirect( aNewId );
    BindItem( aItem );
}


void BOARD_ITEM_INDEX::BindSubtree( BOARD_ITEM* aItem )
{
    wxCHECK( aItem, /* void */ );

    BindItem( aItem );

    aItem->RunOnChildren(
            [this]( BOARD_ITEM* aChild )
            {
                BindSubtree( aChild );
            },
            RECURSE_MODE::NO_RECURSE );
}

void BOARD_ITEM_INDEX::BindChildren( const BOARD_ITEM* aParent )
{
    wxCHECK( aParent, /* void */ );

    aParent->RunOnChildren(
            [this]( BOARD_ITEM* aChild )
            {
                BindSubtree( aChild );
            },
            RECURSE_MODE::NO_RECURSE );
}


void BOARD_ITEM_INDEX::UnbindSubtree( const BOARD_ITEM* aItem )
{
    wxCHECK( aItem, /* void */ );

    // Pointer-keyed eviction: never remove an entry that belongs to a
    // different live item with the same UUID (e.g. a temporary copy).
    UnbindByPtr( aItem );

    aItem->RunOnChildren(
            [this]( BOARD_ITEM* aChild )
            {
                UnbindSubtree( aChild );
            },
            RECURSE_MODE::NO_RECURSE );
}


void BOARD_ITEM_INDEX::UnbindChildren( const BOARD_ITEM* aParent )
{
    wxCHECK( aParent, /* void */ );

    aParent->RunOnChildren(
            [this]( BOARD_ITEM* aChild )
            {
                UnbindSubtree( aChild );
            },
            RECURSE_MODE::NO_RECURSE );
}
