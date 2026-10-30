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

#ifndef BLOCK_DRAG_DEPENDENTS_H
#define BLOCK_DRAG_DEPENDENTS_H

#include <vector>

#include <math/vector2d.h>

class PCB_SHAPE;

/**
 * How a shape follows the endpoint it is joined to.
 */
enum class BLOCK_DRAG_FOLLOW
{
    /// Only this end moves, so the shape stretches. The name ENDPOINT is taken by a
    /// macro in eda_item_flags.h.
    STRETCH,

    /// The whole shape moves, for a shape that moving one end alone would distort. Its
    /// far end travels too, so the chain carries on from there.
    WHOLE
};

/**
 * One end of a graphic shape, used both for the ends that moved and for the ones found
 * to follow them.
 */
struct BLOCK_DRAG_ENDPOINT
{
    PCB_SHAPE*        shape = nullptr;
    bool              isStart = false;
    VECTOR2I          position;
    BLOCK_DRAG_FOLLOW follow = BLOCK_DRAG_FOLLOW::STRETCH;
};

enum class BLOCK_DRAG_DEPENDENT_RESULT
{
    OK,

    /// A shape that would have to follow is locked and Override Locks is off, so the
    /// drag has to stop.
    LOCKED
};

/**
 * Find the joined endpoints that have to follow the ones the drag moves.
 *
 * An endpoint follows when it shares a layer with a moved endpoint and sits within
 * 1 micrometre of it, or on Edge.Cuts within the board's outline chaining epsilon.
 * Only ends count, never a point along a shape. When the endpoints gathered around
 * one moved endpoint are not all within tolerance of each other the match is
 * ambiguous, so that endpoint is left alone and the tolerance stays too small to
 * close a gap the user left on purpose.
 *
 * A shape that moving one end alone would distort follows WHOLE, and its far end starts
 * the search again, so a chain of joined shapes follows as one group. The walk stops at
 * the first shape that stretches, because that shape's far end stays where it is, and
 * each carried shape is reached once.
 *
 * The caller builds aCandidates from the graphics of the editing scope that the gesture
 * is not moving. That scope also settles footprint children: the board editor passes
 * board drawings, and the footprint editor passes the footprint's own children.
 *
 * @return LOCKED when a shape that would follow, or its parent footprint, is locked and
 *         aOverrideLocks is false, with aDependents cleared.
 */
BLOCK_DRAG_DEPENDENT_RESULT
BlockDragFindDependents( const std::vector<PCB_SHAPE*>&          aCandidates,
                         const std::vector<BLOCK_DRAG_ENDPOINT>& aMovedEndpoints, bool aOverrideLocks,
                         std::vector<BLOCK_DRAG_ENDPOINT>& aDependents );

#endif // BLOCK_DRAG_DEPENDENTS_H
