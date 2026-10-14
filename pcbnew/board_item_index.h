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

#pragma once

#include <unordered_map>

#include <kiid.h>

class BOARD;
class BOARD_ITEM;

/**
 * Bi-directional index mapping a board's items keyed by identity (KIID).
 *
 * This index maintains a two-way mapping between KIIDs and board items' pointer
 * identities. It is used to quickly look up board items by their KIIDs, in particular
 * by functions such as BOARD::ResolveItem(), where it provides O(1) lookup for indexed
 * items.
 *
 * This index does not have any thread-safety guarantees.
 *
 * Implications of being indexed:
 *
 *   - Two maps record the forward and reverse mappings between KIIDs and board items.
 *   - Duplicate KIIDs are tolerated, with the later call evicting the previous entry,
 *     from the forward map but remains in the reverse map until it is re-bound or removed
 *     which is what BOARD::RepairDuplicateItemUuids() handles.
 *
 * BOARD is the client of this index, and users of BOARDs interact with it only via
 * the BOARD API (some of which are facades for this index).
 *
 * FOOTPRINT boards keep an empty index: BindItem() and Rebind() return early there.
 */
class BOARD_ITEM_INDEX
{
public:
    explicit BOARD_ITEM_INDEX( BOARD& aOwner ) :
            m_owner( aOwner )
    {
    }

    /**
     * Return a cached item for @a aId if the entry is still self-consistent.
     *
     * UUIDs can still be rewritten in-place in some attached-item paths.  When that happens, the
     * cache may temporarily contain a stale alias from the old UUID to the live item.  Drop those
     * aliases on read so lookups never return an item whose current UUID no longer matches the key.
     */
    BOARD_ITEM* Find( const KIID& aId );

    /**
     * Index @a aItem, replacing any previous entry for its UUID or address.
     */
    void BindItem( BOARD_ITEM* aItem );

    /**
     * Remove an item from the item-by-id cache.
     */
    void Unbind( const KIID& aId );

    /**
     * Drop @a aItem itself.  Pointer-keyed, so a temporary copy sharing its UUID is left alone,
     * and safe to call from ~BOARD_ITEM and the UUID rebind paths.
     */
    void UnbindByPtr( const BOARD_ITEM* aItem );

    /**
     * Drop every entry, clearing each item's link back to the board.
     */
    void Clear();

    /**
     * Move @a aItem to @a aNewId, keeping the index in step with the item.
     */
    void Rebind( BOARD_ITEM* aItem, const KIID& aNewId );

    void BindSubtree( BOARD_ITEM* aItem );
    void BindChildren( const BOARD_ITEM* aParent );
    void UnbindSubtree( const BOARD_ITEM* aItem );
    void UnbindChildren( const BOARD_ITEM* aParent );

    const std::unordered_map<KIID, BOARD_ITEM*>& Map() const { return m_itemsById; }
    bool IsIndexed( const BOARD_ITEM* aItem ) const { return m_idsByItem.contains( aItem ); }

private:
    BOARD& m_owner;

    /// Forward map from KIIDs to board items.
    std::unordered_map<KIID, BOARD_ITEM*> m_itemsById;

    /// Reverse map from board items to their KIIDs.
    std::unordered_map<const BOARD_ITEM*, KIID> m_idsByItem;
};
