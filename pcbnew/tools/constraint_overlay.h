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

#include <cstdint>
#include <map>
#include <memory>
#include <optional>
#include <vector>

#include <eda_item.h>
#include <kiid.h>
#include <layer_ids.h>
#include <math/box2.h>
#include <math/vector2d.h>
#include <geometry/seg.h>
#include <gal/color4d.h>

#include <tools/view_overlay_holder.h>
#include <constraints/board_constraint_adapter.h>

class BOARD;
class BITMAP_BASE;

namespace KIGFX
{
class VIEW;
}


/// An on-canvas constraint badge. Holds its anchor (the world point it labels), the constraint type
/// (which picks the icon) and colour to draw, and which constraint it represents for hit-testing. The
/// on-screen draw position is computed at draw time by CONSTRAINT_OVERLAY::LayoutBadges.
struct CONSTRAINT_BADGE
{
    VECTOR2I            pos;
    KIID                constraint;
    PCB_CONSTRAINT_TYPE type;
    KIGFX::COLOR4D      color;
    std::vector<SEG>    avoid; // constrained geometry the badge stays clear of
};


/**
 * Draws the constraint type-glyph badges at a constant on-screen size, like the edit handles. They
 * read as annotations instead of scaling with zoom like board geometry. The size is taken from the
 * view scale every frame, so this is a ViewDraw item rather than a cached overlay.
 */
class CONSTRAINT_BADGE_ITEM : public EDA_ITEM
{
public:
    CONSTRAINT_BADGE_ITEM();

    ~CONSTRAINT_BADGE_ITEM() override;

    void SetBadges( const std::vector<CONSTRAINT_BADGE>& aBadges, const KIID& aSelected )
    {
        m_badges = aBadges;
        m_selected = aSelected;
        m_layoutScale = -1.0;   // invalidate the cached layout; the badge set changed
    }

    /// Pick preview anchor drawn at constant on screen size like edit handles nullopt draws none
    void SetPreviewAnchor( const std::optional<VECTOR2I>& aAnchor ) { m_previewAnchor = aAnchor; }

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
    /// The badge icon for a constraint type, loaded and cached on first use (nullptr for UNDEFINED).
    BITMAP_BASE* icon( PCB_CONSTRAINT_TYPE aType ) const;

    /// A filled disc of the given colour, built and cached on first use. Drawn as a bitmap (not a
    /// GAL circle) so it and the icon share the immediate-mode path and always layer in draw order.
    BITMAP_BASE* disc( const KIGFX::COLOR4D& aColor ) const;

    std::vector<CONSTRAINT_BADGE> m_badges;
    KIID                          m_selected;
    std::optional<VECTOR2I>       m_previewAnchor;

    mutable std::map<PCB_CONSTRAINT_TYPE, std::unique_ptr<BITMAP_BASE>> m_icons;
    mutable std::map<uint32_t, std::unique_ptr<BITMAP_BASE>>            m_discs;

    // Screen-space layout cached per view scale, recomputed only when the zoom or badge set changes.
    mutable std::vector<VECTOR2D> m_layout;
    mutable double                m_layoutScale = -1.0;
};


/**
 * Draws the geometric-constraint diagnostics tint onto a GAL overlay (issue #2329): every
 * constrained shape is outlined in a colour reflecting its state -- under-constrained (amber),
 * fully constrained (green), over-constrained or referencing a deleted item (red).
 *
 * The constraint objects themselves carry no geometry, so this overlay is the only way to see
 * them on the canvas.  Call Update() whenever the board changes; Clear() hides it.
 */
/// How much of the constraint overlay is shown.  ALWAYS draws every constraint (the classic
/// toggle); HOVER draws only the constraints of the hovered shape, or nothing when none is hovered.
enum class OVERLAY_MODE
{
    HOVER,
    ALWAYS
};


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

    /// Set the visibility mode (HOVER vs ALWAYS). Returns true if it changed.
    bool SetVisibilityMode( OVERLAY_MODE aMode );

    OVERLAY_MODE GetVisibilityMode() const { return m_mode; }

    /// In HOVER mode, show only this shape's constraints; niluuid draws nothing. Independent of the
    /// panel-row isolation. Returns true if it changed.
    bool SetHoverShape( const KIID& aShape );

    const KIID& GetHoverShape() const { return m_hoverShape; }

    /// Preview next constraint pick target while authoring a point based constraint
    /// @p aElement is shape or dimension outlined @p aAnchor marks point unless @p aWholeElement niluuid clears
    bool SetPickPreview( const KIID& aElement, bool aWholeElement,
                         const std::optional<VECTOR2I>& aAnchor );

    /// Drop any pick preview.
    void ClearPickPreview();

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

    /// The on-screen draw position (world units) of each badge at the given scale: the anchor offset
    /// by BadgeScreenOffset() plus a screen-constant fan-out that de-overlaps badges sharing (or near)
    /// an anchor.  Because it is a pure function of the badge set and @p aWorldPerPx, drawing and
    /// hit-testing call it identically so they can never diverge.
    static std::vector<VECTOR2D> LayoutBadges( const std::vector<CONSTRAINT_BADGE>& aBadges,
                                               double aWorldPerPx );

private:
    /// Redraw the tint and badges from m_lastDiag (no re-solve).
    void render();

    BOARD*                        m_board;
    BOARD_CONSTRAINT_DIAGNOSTICS  m_lastDiag;
    std::vector<CONSTRAINT_BADGE> m_badges;
    KIID                          m_selected;
    KIID                          m_isolated;
    OVERLAY_MODE                  m_mode = OVERLAY_MODE::ALWAYS;
    KIID                          m_hoverShape = niluuid;
    CONSTRAINT_BADGE_ITEM         m_badgeItem;

    // Pick preview channel drawn on top of diagnostics tint while authoring a point based constraint
    // m_previewElement is outlined shape a point pick also marks m_previewAnchor
    KIID                          m_previewElement = niluuid;
    bool                          m_previewWhole = true;
    std::optional<VECTOR2I>       m_previewAnchor;
};
