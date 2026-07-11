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

#pragma once

#include <vector>

#include <kiid.h>
#include <math/vector2d.h>

#include <constraints/pcb_constraint.h>
#include <tools/view_overlay_holder.h>

class BOARD;
class PCB_SHAPE;


/**
 * Draws selectable endpoint markers for the geometric-constraint authoring flow (issue #2329).
 *
 * When several graphical items are selected their bindable anchors (segment START/END, arc
 * START/END/CENTER, circle CENTER) are shown as markers; the user clicks markers to build a
 * point-set that a point-anchored constraint (coincident, ...) is then created from.  Bound
 * markers draw 50% larger.
 *
 * The point-set is stored as stable {KIID, anchor} members, never as transient handles, so it
 * survives marker rebuilds.  Mutators only update state; call Redraw() once afterwards to refresh
 * the canvas.
 */
class CONSTRAINT_ENDPOINT_OVERLAY : public VIEW_OVERLAY_HOLDER
{
public:
    CONSTRAINT_ENDPOINT_OVERLAY( BOARD* aBoard, KIGFX::VIEW* aView ) :
            VIEW_OVERLAY_HOLDER( aView ),
            m_board( aBoard )
    {
    }

    /// Set the shapes whose endpoints are offered.  Anchors whose shape is no longer offered are
    /// pruned from the point-set.
    void SetShapes( const std::vector<PCB_SHAPE*>& aShapes );

    /// Toggle membership of the anchor nearest @p aPos within @p aTol; returns true if one was hit.
    bool ToggleNearest( const VECTOR2I& aPos, double aTol );

    /// Toggle membership of an exact member (used to mirror an externally-resolved pick).
    void ToggleMember( const CONSTRAINT_MEMBER& aMember );

    const std::vector<CONSTRAINT_MEMBER>& PointSet() const { return m_pointSet; }
    void                                  ClearPointSet() { m_pointSet.clear(); }

    /// Rebuild the overlay graphics from the current shapes and point-set.
    void Redraw();

private:
    bool inPointSet( const CONSTRAINT_MEMBER& aMember ) const;

    /// The offered shapes resolved from their KIIDs against the live board (skipping any deleted).
    std::vector<PCB_SHAPE*> resolveShapes() const;

    BOARD*                         m_board;
    std::vector<KIID>              m_shapeIds;
    std::vector<CONSTRAINT_MEMBER> m_pointSet;
};
