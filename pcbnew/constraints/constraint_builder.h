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

#ifndef CONSTRAINT_BUILDER_H_
#define CONSTRAINT_BUILDER_H_

#include <memory>
#include <optional>
#include <vector>

#include <math/vector2d.h>

#include <constraints/pcb_constraint.h>

class BOARD;
class BOARD_ITEM;
class PCB_SHAPE;


/// A selectable feature of a shape (a segment endpoint, arc centre, ...) and its location.
struct CONSTRAINT_ANCHOR_POINT
{
    CONSTRAINT_ANCHOR anchor;
    VECTOR2I          pos;
};


/**
 * Enumerate the constraint anchors a shape exposes, with their current positions: segment
 * START/END, arc START/END/CENTER, circle CENTER.  Shapes without point anchors yield an empty
 * list.  Shared by anchor picking and the endpoint-selection overlay so both agree on what is
 * bindable.
 */
std::vector<CONSTRAINT_ANCHOR_POINT> ConstraintShapeAnchors( const PCB_SHAPE* aShape );


/// Every PCB_SHAPE on the board (drawings plus footprint graphics) -- the candidates constraints
/// can reference.  Shared by the anchor and segment hit-tests so the board walk lives in one place.
std::vector<PCB_SHAPE*> CollectConstraintShapes( BOARD* aBoard );


/**
 * The {shape, anchor} member nearest @p aPos within @p aMaxDist among @p aShapes, or std::nullopt.
 * Shared hit-test kernel for board-wide anchor picking and the selection-scoped endpoint overlay.
 */
std::optional<CONSTRAINT_MEMBER> NearestAnchorAmong( const std::vector<PCB_SHAPE*>& aShapes,
                                                     const VECTOR2I& aPos, double aMaxDist );


/**
 * Build a constraint of @p aType from a set of selected board items, or nullptr if the selection
 * does not fit the type (wrong count or kind of items).
 *
 * The authoring tool works on whole selected shapes, so every member uses the WHOLE anchor.  For
 * the fixed-length / fixed-radius families the driving value is measured from the current
 * geometry, so the constraint pins the shape at its present size until the user edits the value.
 *
 * This is split out from the tool so the selection-to-constraint logic is unit-testable without
 * the interactive tool framework.
 */
std::unique_ptr<PCB_CONSTRAINT> BuildConstraintFromItems( BOARD_ITEM* aParent,
                                                          PCB_CONSTRAINT_TYPE aType,
                                                          const std::vector<BOARD_ITEM*>& aItems );


/**
 * Find the shape anchor (segment/arc endpoint, arc/circle centre) nearest @p aPos within
 * @p aMaxDist, for point-anchored constraint authoring (coincident, midpoint, ...).
 *
 * @return the {shape, anchor} member, or std::nullopt if no anchor is close enough.
 */
std::optional<CONSTRAINT_MEMBER> NearestConstraintAnchor( BOARD* aBoard, const VECTOR2I& aPos,
                                                          double aMaxDist );


/**
 * Current location of a constraint member's anchor (its shape's START/END/CENTER), or
 * std::nullopt if the referenced item is gone or exposes no such anchor.
 */
std::optional<VECTOR2I> ConstraintAnchorPosition( BOARD* aBoard, const CONSTRAINT_MEMBER& aMember );

#endif // CONSTRAINT_BUILDER_H_
