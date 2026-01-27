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
 * along with this program; if not, you may find one here:
 * http://www.gnu.org/licenses/old-licenses/gpl-2.0.html
 * or you may search the http://www.gnu.org website for the version 2 license,
 * or you may write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
 */

#ifndef SCH_ITEM_ALIGNMENT_H
#define SCH_ITEM_ALIGNMENT_H

#include <functional>
#include <vector>
#include <math/vector2d.h>
#include <tool/grid_helper.h>

class EDA_ITEM;
class SCH_ITEM;
class SCH_SCREEN;
class EE_GRID_HELPER;

using EDA_ITEMS = std::vector<EDA_ITEM*>;


/**
 * Callbacks for alignment operations.
 *
 * These allow the alignment logic to be separated from the tool infrastructure,
 * making it testable in isolation.
 */
struct SCH_ALIGNMENT_CALLBACKS
{
    /**
     * Callback to move an item by a delta.
     * Responsible for committing the change and updating the display.
     */
    std::function<void( EDA_ITEM* aItem, const VECTOR2I& aDelta )> m_doMoveItem;

    /**
     * Callback to get items connected to a given item at a specific point.
     * These are the "drag items" that should move along with the primary item.
     */
    std::function<void( SCH_ITEM* aItem, const VECTOR2I& aPoint, EDA_ITEMS& aList )> m_getConnectedDragItems;

    /**
     * Optional callback to update an item's display after modification.
     * Used for operations like SetSize that don't go through m_doMoveItem.
     */
    std::function<void( EDA_ITEM* aItem )> m_updateItem;
};


/**
 * Move a schematic item by a delta.
 *
 * This function implements the move logic for different item types, matching
 * the behavior of SCH_MOVE_TOOL::moveItem in DRAG mode:
 * - SCH_LINE_T: Moves only flagged endpoints (STARTPOINT/ENDPOINT)
 * - SCH_SHEET_PIN_T: Uses SetStoredPos + ConstrainOnEdge
 * - Other items: Calls Move()
 *
 * @param aItem The item to move
 * @param aDelta The movement delta
 */
void MoveSchematicItem( EDA_ITEM* aItem, const VECTOR2I& aDelta );


/**
 * Align a set of schematic items to the grid.
 *
 * This function implements the core alignment logic used by the "Align Items to Grid"
 * action. It handles different item types appropriately: lines align their endpoints,
 * sheets align their corners and resize, sheet pins align to grid while maintaining
 * connectivity with their connected wires, etc.
 *
 * @param aScreen The schematic screen containing the items
 * @param aItems The items to align (typically the current selection)
 * @param aGrid The grid helper used for alignment calculations
 * @param aSelectionGrid The grid type to use for alignment
 * @param aCallbacks Callbacks for moving items and getting connected drag items
 */
void AlignSchematicItemsToGrid( SCH_SCREEN* aScreen,
                                const std::vector<EDA_ITEM*>& aItems,
                                EE_GRID_HELPER& aGrid,
                                GRID_HELPER_GRIDS aSelectionGrid,
                                const SCH_ALIGNMENT_CALLBACKS& aCallbacks );


#endif // SCH_ITEM_ALIGNMENT_H
