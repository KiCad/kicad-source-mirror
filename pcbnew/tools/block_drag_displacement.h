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

#ifndef BLOCK_DRAG_DISPLACEMENT_H
#define BLOCK_DRAG_DISPLACEMENT_H

#include <map>
#include <set>
#include <vector>

#include <math/vector2d.h>
#include <tools/pcb_handle_key.h>

class EDIT_POINTS;
class GRID_HELPER;

/**
 * Note every handle the drag will move, and where each one started.
 *
 * A picked edge adds itself and its two corners. Each frame places the corners first
 * and then drives the edge, so the edge follows the same rule it would follow if the
 * user dragged it on its own. A corner picked both directly and through an edge is
 * listed once, so it moves once.
 *
 * @return one entry per handle, or an empty map when a picked handle no longer
 *         resolves, which means the gesture cannot start.
 */
std::map<PCB_HANDLE_KEY, VECTOR2I> BlockDragMoveOrigins( const std::vector<EDIT_POINTS*>& aCollections,
                                                         const std::set<PCB_HANDLE_KEY>&  aPicked );

/**
 * Put every handle at its starting position plus the drag's offset.
 *
 * Each position comes from the stored starting point, so a long drag with many mouse
 * events cannot drift. The offset is already snapped to the grid by the caller. A
 * picked edge's corners are placed like any other handle and the edge is then asked to
 * follow them, which is what holds a zone edge square to its neighbours.
 *
 * @return false when a handle fails to resolve, in which case the caller drops the
 *         gesture and puts the original geometry back.
 */
bool BlockDragApplyDisplacement( const std::vector<EDIT_POINTS*>&          aCollections,
                                 const std::map<PCB_HANDLE_KEY, VECTOR2I>& aOrigins, const VECTOR2I& aDelta,
                                 const GRID_HELPER& aGrid );

#endif // BLOCK_DRAG_DISPLACEMENT_H
