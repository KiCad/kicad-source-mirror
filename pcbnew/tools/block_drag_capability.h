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

#ifndef BLOCK_DRAG_CAPABILITY_H
#define BLOCK_DRAG_CAPABILITY_H

#include <vector>

class BOARD_ITEM;
class PCB_BASE_FRAME;

/**
 * Whether the user has turned on editing anchor points on a multi-selection.
 *
 * Both editors read the board editor's copy of the setting, the way the footprint editor
 * already reads the flip direction from there. One reader so the two tools cannot drift.
 */
bool BlockDragAnchorEditingEnabled( const PCB_BASE_FRAME* aFrame );

/**
 * How one selected item takes part in a block drag.
 *
 * Every item in an expanded selection gets exactly one of these.
 */
enum class BLOCK_DRAG_CAPABILITY
{
    /// Shows handles, and reshapes as the user drags the ones they picked. An item like
    /// this joins the drag once one of its handles is picked. With none picked it stays
    /// put, though it can still follow by a shared endpoint like any unselected item.
    DEFORMABLE,

    /// Moves with the block using its own move operation.
    RIGID,

    /// Moves with the block through the router, which repairs the copper at the edge
    /// of the block.
    ROUTER,

    /// Stops the drag with a message before anything moves.
    UNSUPPORTED
};

/**
 * Classify one board item for a block drag.
 *
 * A footprint holder board has no router, which is how the footprint editor is told
 * apart. Pads are RIGID there, and copper and whole footprints are UNSUPPORTED. Those
 * last rows are a guard, since that selection cannot hold them. On a board, the
 * graphics and zones inside a footprint are RIGID, so they travel without deforming.
 *
 * Expand groups with BlockDragExpandSelection() first. A group container on its own is
 * UNSUPPORTED. A generator is RIGID only when its Move() takes all of its members with
 * it. A via stitch is DEFORMABLE, since it is an outline whose vias are re-placed by the
 * regeneration that follows the drag.
 */
BLOCK_DRAG_CAPABILITY BlockDragCapability( const BOARD_ITEM& aItem );

/**
 * True when the item belongs to any geometric constraint on its board or inside one of
 * its footprints.
 *
 * Plain membership, so every kind of constraint counts. It tells the gesture which
 * moved shapes need their clusters solved again. Walks every constraint on each call.
 */
bool BlockDragIsConstraintMember( const BOARD_ITEM& aItem );

/**
 * Replace group containers with their members, through nested groups, and drop
 * duplicates.
 *
 * The result holds no group containers and no markers, which no tool moves. A generator
 * is a kind of group but travels as one object, so it stays whole. An item selected both
 * on its own and through a group appears once. Directly selected items keep their order,
 * and a group's members follow the place the group held. A footprint's own child is
 * dropped when the footprint is in the result too, because the footprint carries it.
 */
std::vector<BOARD_ITEM*> BlockDragExpandSelection( const std::vector<BOARD_ITEM*>& aSelection );

#endif // BLOCK_DRAG_CAPABILITY_H
