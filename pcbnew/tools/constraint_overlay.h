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

#include <eda_item.h>
#include <kiid.h>
#include <layer_ids.h>
#include <math/box2.h>
#include <math/vector2d.h>
#include <gal/color4d.h>

#include <tools/view_overlay_holder.h>
#include <constraints/board_constraint_adapter.h>

class BOARD;

namespace KIGFX
{
class VIEW;
}


/// An on-canvas constraint badge. Holds where the glyph is drawn, the glyph and colour to draw, and
/// which constraint it represents for hit-testing.
struct CONSTRAINT_BADGE
{
    VECTOR2I       pos;
    KIID           constraint;
    wxString       glyph;
    KIGFX::COLOR4D color;
};


/**
 * Draws the constraint type-glyph badges at a constant on-screen size, like the edit handles. They
 * read as annotations instead of scaling with zoom like board geometry. The size is taken from the
 * view scale every frame, so this is a ViewDraw item rather than a cached overlay.
 */
class CONSTRAINT_BADGE_ITEM : public EDA_ITEM
{
public:
    CONSTRAINT_BADGE_ITEM() :
            EDA_ITEM( NOT_USED )
    {
    }

    void SetBadges( const std::vector<CONSTRAINT_BADGE>& aBadges, const KIID& aSelected )
    {
        m_badges = aBadges;
        m_selected = aSelected;
    }

    const BOX2I ViewBBox() const override
    {
        BOX2I bbox;
        bbox.SetMaximum(); // Always drawn, so the per-frame screen-constant sizing stays current.
        return bbox;
    }

    std::vector<int> ViewGetLayers() const override { return { LAYER_GP_OVERLAY }; }

    void ViewDraw( int aLayer, KIGFX::VIEW* aView ) const override;

    bool HitTest( const VECTOR2I&, int = 0 ) const override { return false; }

    wxString GetClass() const override { return wxT( "CONSTRAINT_BADGE_ITEM" ); }

#if defined( DEBUG )
    void Show( int, std::ostream& ) const override {}
#endif

private:
    std::vector<CONSTRAINT_BADGE> m_badges;
    KIID                          m_selected;
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
    ~CONSTRAINT_OVERLAY() override;

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

    /// Show only this constraint, or niluuid to show all. Returns true if it changed.
    bool SetIsolated( const KIID& aConstraint );

    const KIID& GetIsolated() const { return m_isolated; }

    /// Badges placed by the last Update(), for canvas hit-testing.
    const std::vector<CONSTRAINT_BADGE>& Badges() const { return m_badges; }

    /// Click hit radius in screen pixels. The badges draw at a constant screen size, so the caller
    /// converts this to world units with the view scale.
    static double BadgeHitRadius();

    /// Screen-pixel offset from a badge's anchor to where its glyph draws and hit-tests, keeping
    /// the glyph clear of the geometry so clicks on the shape still select the shape.
    static VECTOR2D BadgeScreenOffset();

    /// World units per screen pixel for badge sizing, capped at far zoom-out. Shared by drawing
    /// and hit-testing.
    static double BadgeWorldPerPixel( double aWorldScale );

private:
    /// Redraw the tint and badges from m_lastDiag (no re-solve).
    void render();

    BOARD*                        m_board;
    BOARD_CONSTRAINT_DIAGNOSTICS  m_lastDiag;
    std::vector<CONSTRAINT_BADGE> m_badges;
    KIID                          m_selected;
    KIID                          m_isolated;
    CONSTRAINT_BADGE_ITEM         m_badgeItem;
};
