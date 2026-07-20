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

#ifndef CONSTRAINT_EDIT_TOOL_H_
#define CONSTRAINT_EDIT_TOOL_H_

#include <map>
#include <memory>
#include <unordered_set>
#include <vector>

#include <tools/pcb_tool_base.h>
#include <constraints/board_constraint_adapter.h>

class PCB_SHAPE;
class PCB_CONSTRAINT;
class PCB_SELECTION_TOOL;
class CONDITIONAL_MENU;
class CONSTRAINT_OVERLAY;

namespace KIGFX
{
class PCB_RENDER_SETTINGS;
}


/**
 * Interactive authoring of geometric constraints (issue #2329).
 *
 * Operates on the current selection: addConstraint creates a constraint of the type passed as the
 * action parameter from the selected whole shapes; removeConstraints deletes every constraint that
 * references a selected item.  The selection-to-constraint logic lives in BuildConstraintFromItems
 * so it can be unit-tested without the tool framework.
 */
class CONSTRAINT_EDIT_TOOL : public PCB_TOOL_BASE
{
public:
    CONSTRAINT_EDIT_TOOL();
    ~CONSTRAINT_EDIT_TOOL() override = default;

    void Reset( RESET_REASON aReason ) override;
    bool Init() override;

    int AddConstraint( const TOOL_EVENT& aEvent );
    int AddPointConstraint( const TOOL_EVENT& aEvent );
    int RemoveConstraints( const TOOL_EVENT& aEvent );
    int ShowConstraints( const TOOL_EVENT& aEvent );
    int ManageConstraints( const TOOL_EVENT& aEvent );

    /// Select the constraint whose on-canvas badge is at @p aPos (enlarge it, highlight the panel
    /// row); returns true if a badge was hit.  Called by the selection tool so badge clicks work
    /// mode-lessly while the diagnostics overlay is shown.
    bool SelectConstraintAt( const VECTOR2I& aPos );

    /// Open the value dialog for the constraint badged at @p aPos; returns true if one was hit.
    bool EditConstraintAt( const VECTOR2I& aPos );

    /// Clear any badge-selected constraint.  Returns true if a constraint was selected and is now cleared.
    bool ClearConstraintSelection();

    /// Delete the currently badge-selected constraint; returns true if one was selected and removed.
    /// Called by EDIT_TOOL::Remove so the Delete key targets a selected relation.
    bool TryDeleteSelectedConstraint();

    /// Edit the currently badge-selected constraint's value; returns true if one was selected.
    /// Called by EDIT_TOOL::Properties so the Edit key targets a selected relation.
    bool TryEditSelectedConstraint();

    /// Re-solve the clusters of @p aShapes after a whole-shape edit, folded into that edit's undo.
    void SolveAfterMove( const std::vector<PCB_SHAPE*>& aShapes );

    /// Re-solve after a panel or dialog edit holding @p aShapes fixed so typed values survive
    /// Only their constrained neighbors move folded into the same undo
    void SolveAfterEdit( const std::vector<PCB_SHAPE*>& aShapes );

    /// Re-diagnose and refresh the overlay and info bar after the caller re-solves @p aShapes
    /// into its own commit for example a live move drag solving each tick
    void DiagnoseAfterMove( const std::vector<PCB_SHAPE*>& aShapes );

    /// Intent entry points for the constraints panel, so it never mutates the board or the selection
    /// itself -- the tool stays the single owner of constraint edit/remove/highlight + refresh.
    void RemoveConstraintById( const KIID& aId );
    void EditConstraintById( const KIID& aId );
    void HighlightConstraintMembers( const KIID& aId, int aMemberIndex );

    /// Refresh the diagnostics overlay if it is currently shown.
    int refreshOverlay( const TOOL_EVENT& aEvent );

    /// Clear stale badge/isolation state when the board selection changes.
    int onSelectionChanged( const TOOL_EVENT& aEvent );

    /// In HOVER mode, reveal the constraints of the shape under the cursor (nothing when none).
    int onHoverMotion( const TOOL_EVENT& aEvent );

private:
    void setTransitions() override;

    /// The owner a new constraint should be parented to (the footprint in the footprint editor).
    BOARD_ITEM* constraintParent() const;

    /// Push @p aCommit, snap the geometry for the added constraints, and refresh the overlays.
    void finishConstraintCommit( BOARD_COMMIT& aCommit,
                                 const std::vector<PCB_CONSTRAINT*>& aAdded );

    /// Confirm any value, reject a duplicate, and commit @p aConstraint. Returns true if it was added.
    bool commitConstraint( std::unique_ptr<PCB_CONSTRAINT> aConstraint );

    /// Author a whole-shape constraint by clicking its items on the canvas (the no-selection path).
    int pickShapeConstraint( PCB_CONSTRAINT_TYPE aType, const TOOL_EVENT& aEvent );

    /// Author a horizontal or vertical constraint from a whole segment or two point anchors
    /// Clicking a point first starts the two point path so a corner can be levelled
    int pickLinearConstraint( PCB_CONSTRAINT_TYPE aType, const TOOL_EVENT& aEvent );

    /// Solve a just-added constraint so the geometry snaps to satisfy it, in its own commit.
    void solveAddedConstraint( PCB_CONSTRAINT* aConstraint );

    /// The constraint whose badge is within the hit radius of @p aPos, or nullptr.
    PCB_CONSTRAINT* hitTestBadge( const VECTOR2I& aPos ) const;

    /// Mark @p aConstraint selected on the overlay and in the panel (nullptr clears).
    void setSelectedConstraint( PCB_CONSTRAINT* aConstraint );

    /// The active PCB render settings, or nullptr when no painter is attached (headless).
    KIGFX::PCB_RENDER_SETTINGS* pcbRenderSettings() const;

    /// Stash the constrained-item set (the shadow-layer gate) into the render settings and repaint
    /// the overlay target.  Pass an empty diagnosis to clear it.
    void updateConstrainedItems( const BOARD_CONSTRAINT_DIAGNOSTICS& aDiag );

    /// Stash the selected constraint's members (the highlighted-shadow set) into the render settings
    /// and repaint the overlay target.  Pass nullptr to clear it.
    void updateHighlightedConstraintMembers( PCB_CONSTRAINT* aConstraint );

    /// Re-cache the items whose shadow-set membership changed between @p aOld and @p aNew, so the
    /// cached constraint-shadow layer reflects the new set (a plain target-dirty would not rebuild it).
    void repaintShadowItems( const std::unordered_set<KIID>& aOld, const std::unordered_set<KIID>& aNew );

    /// Show the selected constraint's type, items and state in the bottom message panel. Restore the
    /// default board readout when nothing is selected.
    void updateConstraintMsgPanel( PCB_CONSTRAINT* aConstraint );

    /// Mark the diagnosis and candidate caches stale (the model changed) and re-render the views.
    void refreshDiagnostics();

    /// Re-render the shown views from the cached diagnosis without invalidating it -- for a bare
    /// visibility change (a hover) that did not touch the model.
    void renderConstraintViews();

    /// The cached board diagnosis, solved only when the model changed since the last call, so a
    /// hover acquisition reuses the last solve instead of re-solving on every mouse move.
    const BOARD_CONSTRAINT_DIAGNOSTICS& ensureDiagnosis();

    /// The shapes referenced by any constraint, the hover-hit candidate set (cached per model).
    const std::vector<PCB_SHAPE*>& hoverCandidates();

    /// Push an already-computed diagnosis into every shown view, so a caller that already solved
    /// does not solve again.
    void applyDiagnostics( const BOARD_CONSTRAINT_DIAGNOSTICS& aDiag );

    /// Remove a constraint in its own commit and refresh the diagnostics.
    void removeConstraint( PCB_CONSTRAINT* aConstraint );

    /// Resolve a constraint by KIID against the live board, or nullptr.
    PCB_CONSTRAINT* resolveConstraint( const KIID& aId ) const;

    /// Open the value dialog for a constraint and, on accept, commit + re-solve + refresh.
    void editConstraint( PCB_CONSTRAINT* aConstraint );

    /// True if every item @p aConstraint references resolves and is locked (nothing the solver may
    /// move), so the relation was recorded but cannot adjust geometry.
    bool allMembersLocked( const PCB_CONSTRAINT* aConstraint ) const;

    /// True if the board already holds a constraint equal to @p aConstraint (same type and members),
    /// so authoring it would only add a redundant copy.
    bool isDuplicateConstraint( const PCB_CONSTRAINT* aConstraint ) const;

private:
    PCB_SELECTION_TOOL*                          m_selectionTool;
    CONDITIONAL_MENU*                            m_menu;
    std::unique_ptr<CONSTRAINT_OVERLAY>          m_overlay;
    BOARD_CONSTRAINT_DIAGNOSTICS                 m_cachedDiag;       ///< Reused until the model changes.

    /// Incremental board diagnoser for the hot edit path, so editing one shape re-solves only its
    /// own cluster instead of every cluster on the board.
    BOARD_CONSTRAINT_DIAGNOSER                   m_diagnoser;
    bool                                         m_diagDirty = true;
    std::optional<std::vector<PCB_SHAPE*>>       m_hoverCandidates;  ///< Constrained shapes, per model.

    /// Last value and driving choice per constraint type this session so same type runs keep one size
    /// Keyed by type so length and angle values never mix cleared on tool Reset
    std::map<PCB_CONSTRAINT_TYPE, double>        m_lastConstraintValue;
    std::map<PCB_CONSTRAINT_TYPE, bool>          m_lastConstraintDriving;
};

#endif // CONSTRAINT_EDIT_TOOL_H_
