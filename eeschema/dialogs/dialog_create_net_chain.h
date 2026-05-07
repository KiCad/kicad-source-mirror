/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation, either version 3 of the License, or (at your
 * option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef DIALOG_CREATE_NET_CHAIN_H
#define DIALOG_CREATE_NET_CHAIN_H

#include <dialogs/dialog_create_net_chain_base.h>

#include <set>
#include <vector>
#include <wx/string.h>
#include <kiid.h>
#include <math/box2.h>
#include <sch_sheet_path.h>

class SCH_EDIT_FRAME;
class SCH_NETCHAIN;
class SCH_SCREEN;
class SCH_SYMBOL;


class DIALOG_CREATE_NET_CHAIN : public DIALOG_CREATE_NET_CHAIN_BASE
{
public:
    /// Optional context derived from the user selection that opened the dialog.  Any field may
    /// be empty.  When set, the dialog narrows the grid (and may auto-select a row) so the user
    /// lands on the chain that matches their right-click instead of the full potentials list.
    struct FOCUS_HINT
    {
        wxString fromRef;   ///< Selected symbol reference (or first of two)
        wxString toRef;     ///< Second selected symbol reference, if any
        wxString netName;   ///< Net name from a selected pin or wire/bus
    };

    DIALOG_CREATE_NET_CHAIN( SCH_EDIT_FRAME* aParent, const FOCUS_HINT& aHint = {} );

    ~DIALOG_CREATE_NET_CHAIN() override;

    bool TransferDataToWindow() override;
    bool TransferDataFromWindow() override;

protected:
    void OnChainSelected( wxGridEvent& aEvent ) override;
    void OnFilterChanged( wxCommandEvent& aEvent ) override;
    void OnRefreshClicked( wxCommandEvent& aEvent ) override;
    void OnFindPathClicked( wxCommandEvent& aEvent ) override;

private:
    /**
     * Row backing data for the chains grid.
     *
     * livePtr is a non-owning pointer into the current contents of
     * CONNECTION_GRAPH::m_potentialNetChains and remains valid only until that pool is rebuilt
     * or cleared.  RebuildNetChains() (invoked via Recalculate) destroys every entry, so any
     * path that triggers a recalc must clear m_rows BEFORE the recalc — recalculateAndReload()
     * centralizes that ordering for the dialog's own refresh paths.
     */
    struct POTENTIAL_ROW
    {
        SCH_NETCHAIN*      livePtr = nullptr; ///< null for force-created manual rows
        wxString           suggestedName;
        std::set<wxString> memberNets;
        wxString           terminals; ///< e.g. "J1:1 -> J2:1"
        bool               isManual = false;

        /// For force-created rows without a livePtr.  Ref/pin strings are cached at
        /// row build time to avoid re-resolving the owning sheet path at commit.
        KIID     forceFromUuid;
        KIID     forceToUuid;
        KIID     forceFromPinUuid;
        KIID     forceToPinUuid;
        wxString forceFromRef;
        wxString forceToRef;
        wxString forceFromPinNum;
        wxString forceToPinNum;

        /// Lazily-resolved sheet to navigate to when this row is selected.  Filled in by
        /// findSheetForRow() on first access; reset when the row pool is rebuilt.
        SCH_SHEET_PATH cachedSheet;
        bool           cachedSheetResolved = false;
    };

    void loadPotentials();
    void rebuildGrid();
    void updateMemberDetail( int aRow );
    void populateComponentCombos();

    /**
     * Walk @p aScreen and brighten/un-brighten items whose connection name is in @p aNets.
     * Returns the union bounding box of items that ended up brightened (empty if none).
     * Pass an empty @p aNets set to clear all brightening on the screen.  When @p aScreen is
     * null, the call is a no-op.
     *
     * Note: this brightens SCH_SYMBOL pins and connectable items.  Sheet pins and power-symbol
     * fields highlighted by SCH_EDITOR_CONTROL::UpdateNetHighlighting are NOT covered here; a
     * future refactor could route potential-chain highlight through that path once potentials
     * grow a stable temporary identifier.
     */
    BOX2I highlightChainNets( const std::set<wxString>& aNets, SCH_SCREEN* aScreen );

    /**
     * Switch to the sheet owning @p aRow (if different from the current sheet), brighten the
     * chain's nets there, and zoom-fit on the result.  Highlights on the previously-touched
     * sheet are cleared first so navigating between rows does not leave stale brightening
     * behind.  A chain may span sheets; we deliberately focus only the sheet containing the
     * chain's terminal ref, matching how cross-probing behaves elsewhere in eeschema.
     */
    void navigateAndHighlightChain( POTENTIAL_ROW& aRow );

    /// Resolve and cache the sheet path to navigate to for @p aRow.  Returns the cached path
    /// (which may be the root sheet if no terminal ref or member-net match is found).
    const SCH_SHEET_PATH& findSheetForRow( POTENTIAL_ROW& aRow );

    /// Apply the focus hint after rows are loaded — set the filter input, optionally trigger
    /// the existing two-ref find-path flow, auto-select if exactly one row matches, or surface
    /// an empty-state message when no row matches.
    void applyFocusHint();

    int  selectedRow() const;

    /**
     * Tear down row state, optionally trigger a connectivity recalculation, then reload
     * rows and refresh the grid. Centralizes the lifetime contract for POTENTIAL_ROW::livePtr
     * so callers cannot leave dangling pointers reachable during a recalc.
     */
    void recalculateAndReload( bool aRunRecalculate );

    bool validateAndCreate();

    /// Position the grid cursor on the first filtered row, sync the name input, run the
    /// member-detail update, and trigger navigateAndHighlightChain.  Used after every flow
    /// that auto-selects a row (focus hint, both-refs find-path, force-create).
    void autoSelectFirstRow();

    SCH_EDIT_FRAME*            m_frame;
    FOCUS_HINT                 m_hint;
    std::vector<POTENTIAL_ROW> m_rows;
    std::vector<int>           m_filteredIndices; ///< indices into m_rows shown in grid
    int                        m_createdCount = 0;
    bool                       m_rebuilding = false;
    SCH_SHEET_PATH             m_lastHighlightedSheet; ///< empty when no prior highlight
};

#endif // DIALOG_CREATE_NET_CHAIN_H
