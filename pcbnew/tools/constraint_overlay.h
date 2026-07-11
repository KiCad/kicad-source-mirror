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

#include <tools/view_overlay_holder.h>
#include <constraints/board_constraint_adapter.h>

class BOARD;

namespace KIGFX
{
class VIEW;
}


/// An on-canvas constraint badge: where its glyph was drawn and which constraint it represents.
struct CONSTRAINT_BADGE
{
    VECTOR2I pos;
    KIID     constraint;
};


/**
 * Draws the geometric-constraint diagnostics tint onto a GAL overlay (issue #2329): every
 * constrained shape is outlined in a colour reflecting its state -- under-constrained (amber),
 * fully constrained (green), over-constrained or referencing a deleted item (red).
 *
 * The constraint objects themselves carry no geometry, so this overlay is the only way to see
 * them on the canvas.  Call Update() whenever the board changes; Clear() hides it.
 */
class CONSTRAINT_OVERLAY : public VIEW_OVERLAY_HOLDER
{
public:
    CONSTRAINT_OVERLAY( BOARD* aBoard, KIGFX::VIEW* aView );

    /// Redraw the tint and badges from @p aDiag (the caller owns the one board-wide solve).
    void Update( const BOARD_CONSTRAINT_DIAGNOSTICS& aDiag );

    /// Redraw from the last diagnosis without re-solving (for a selection-only change).
    void RefreshSelection();

    /// Hide the overlay.
    void Clear();

    /// Mark a constraint as selected so its badge draws enlarged and ringed; pass niluuid to clear.
    /// Returns true if the selection changed (the caller should then Update()).
    bool SetSelected( const KIID& aConstraint );

    const KIID& GetSelected() const { return m_selected; }

    /// Badges placed by the last Update(), for canvas hit-testing.
    const std::vector<CONSTRAINT_BADGE>& Badges() const { return m_badges; }

    /// World-space radius within which a click counts as hitting a badge.
    static double BadgeHitRadius();

private:
    /// Redraw the tint and badges from m_lastDiag (no re-solve).
    void render();

    BOARD*                        m_board;
    BOARD_CONSTRAINT_DIAGNOSTICS  m_lastDiag;
    std::vector<CONSTRAINT_BADGE> m_badges;
    KIID                          m_selected;
};
