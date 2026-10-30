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

#ifndef BLOCK_DRAG_PICK_H
#define BLOCK_DRAG_PICK_H

#include <set>
#include <vector>

#include <math/vector2d.h>
#include <tools/pcb_handle_key.h>

class EDIT_POINT;
class EDIT_POINTS;
class SHAPE_LINE_CHAIN;

/**
 * True when the user is allowed to pick this handle.
 *
 * The parent item has to be DEFORMABLE. Corner handles can be picked. An edge handle
 * can be picked on a zone, a graphic polygon or a polygon generator such as a via
 * stitch, where dragging an edge is part of the normal point editor. Midpoints of
 * rectangles and segments are left out, because the two corners they drive are the handles
 * that really define the shape.
 *
 * Pass an item's own handle collection. The scaling box drawn around a multi-selection
 * borrows one member as its parent, so it does not belong here.
 */
bool BlockDragHandlePickable( const EDIT_POINTS& aCollection, const EDIT_POINT& aPoint );

/**
 * Add one clicked handle to the picked set.
 *
 * A plain click keeps just that handle. Shift-click toggles it and leaves the rest of
 * the set alone.
 */
void BlockDragApplyPick( std::set<PCB_HANDLE_KEY>& aPicked, const PCB_HANDLE_KEY& aKey, bool aAccumulate );

/// The closed rectangle a box pick covers, built from two opposite corners.
SHAPE_LINE_CHAIN BlockDragPickRect( const VECTOR2I& aFirst, const VECTOR2I& aSecond );

/**
 * Add the handles a lasso surrounds to the picked set.
 *
 * A plain lasso keeps just the corner handles it surrounds. Shift adds them and leaves
 * the rest of the set alone. A lasso picks corners, and an edge comes along once both
 * of its corners are picked.
 *
 * A handle sitting right on the lasso line counts as inside. Handles are single points
 * with no width, so the usual distinction between enclosing and touching a shape has
 * nothing to act on.
 */
void BlockDragLassoPick( const std::vector<EDIT_POINTS*>& aCollections, const SHAPE_LINE_CHAIN& aLasso,
                         bool aAccumulate, std::set<PCB_HANDLE_KEY>& aPicked );

#endif // BLOCK_DRAG_PICK_H
