/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 3
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, you may find one here:
 * http://www.gnu.org/licenses/gpl-3.0.html
 */

#ifndef DIALOG_KICAD_DIFF_H
#define DIALOG_KICAD_DIFF_H

#include "dialog_kicad_diff_base.h"

#include <diff_merge/diff_scene.h>
#include <diff_merge/kicad_diff_types.h>

#include <functional>
#include <map>
#include <set>


class WIDGET_DIFF_CANVAS;
class wxChoice;


/**
 * File-compare dialog (Phase 7).
 *
 * Shows a DOCUMENT_DIFF as a hierarchical tree of changes on the left and the
 * property delta list for the selected change on the right. The change
 * geometry renders on a GAL-backed WIDGET_DIFF_CANVAS that cross-probes with
 * the tree through OnTreeSelectionChanged and the canvas's pick handler.
 *
 * The dialog is constructed with a pre-computed DOCUMENT_DIFF; the caller is
 * responsible for running the appropriate differ.
 */
class DIALOG_KICAD_DIFF : public DIALOG_KICAD_DIFF_BASE
{
public:
    using SHEET_SWITCHER = std::function<void( WIDGET_DIFF_CANVAS&, const KIID_PATH& )>;

    DIALOG_KICAD_DIFF( wxWindow* aParent, const wxString& aReferencePath, const wxString& aComparisonPath,
                       const KICAD_DIFF::DOCUMENT_DIFF& aDiff, KICAD_DIFF::DOCUMENT_GEOMETRY aReferenceGeometry = {},
                       KICAD_DIFF::DOCUMENT_GEOMETRY aComparisonGeometry = {}, SHEET_SWITCHER aSheetSwitcher = {},
                       KIID_PATH aInitialSheet = {} );

    ~DIALOG_KICAD_DIFF() override = default;

    void                SwitchCanvasToSheet( const KIID_PATH& aSheetPath );
    const KIID_PATH&    CurrentCanvasSheet() const { return m_currentCanvasSheet; }
    WIDGET_DIFF_CANVAS* DiffCanvas() const { return m_canvas; }

    /// Swap in a fresh diff with new schematics. Rebuilds tree and scene,
    /// clears selection and properties.
    void Reload( const wxString& aReferencePath, const wxString& aComparisonPath, KICAD_DIFF::DOCUMENT_DIFF aDiff,
                 KICAD_DIFF::DOCUMENT_GEOMETRY aReferenceGeometry, KICAD_DIFF::DOCUMENT_GEOMETRY aComparisonGeometry,
                 SHEET_SWITCHER aSheetSwitcher, KIID_PATH aInitialSheet );

    using REVISION_HANDLER = std::function<void( int aIndex )>;

    /// Add a revision dropdown (with prev/next) at the top. Fired on change with
    /// the chosen index. Only call this for local-history compares; without it
    /// no chooser is shown.
    void SetRevisionChooser( const std::vector<wxString>& aLabels, int aSelected, REVISION_HANDLER aOnChange );

    using CHANGE_SELECTED_FN = std::function<void( const KIID_PATH& )>;

    void SetChangeSelectedHandler( CHANGE_SELECTED_FN aFn ) { m_changeSelectedFn = std::move( aFn ); }

protected:
    void OnClose( wxCloseEvent& aEvent ) override;
    void OnTreeSelectionChanged( wxTreeEvent& aEvent ) override;
    void OnTreeItemMenu( wxTreeEvent& aEvent );
    void OnOK( wxCommandEvent& aEvent ) override;

private:
    /// Populate the tree from m_diff. Groups by CHANGE_KIND.
    void buildTree();

    /// Populate the property list for the change associated with the selected
    /// tree node, or clear the list if no change is selected (e.g. group node).
    void showChange( const KICAD_DIFF::ITEM_CHANGE* aChange );

    /// Select the tree row whose change has the given KIID_PATH. Wired to
    /// the canvas's pick handler so clicking a rectangle on the canvas
    /// scrolls/selects the matching change in the tree. Returns false when
    /// the id isn't present (typically because its category is filtered out).
    bool selectChangeById( const KIID_PATH& aChangeId );

    /// Grey out tree rows whose change is in m_hiddenChanges, restore the rest.
    void applyHiddenToTree();

    /// Change ids a tree row stands for. A routing net row collapses every
    /// segment of the net, so hiding it must hide them all.
    std::vector<KIID_PATH> changeRowIds( const KICAD_DIFF::ITEM_CHANGE& aChange ) const;

    KICAD_DIFF::DOCUMENT_DIFF                                m_diff;

    /// Changes the user has muted via the tree's right-click menu.
    std::set<KIID_PATH> m_hiddenChanges;

    REVISION_HANDLER m_revisionHandler;
    wxChoice*        m_revisionChoice = nullptr;

    /// Maps tree item IDs to the underlying change record so selection can
    /// find the data without walking the diff again. Group nodes have no
    /// entry here (selecting a group clears the property list).
    std::map<wxUIntPtr, const KICAD_DIFF::ITEM_CHANGE*>      m_changeByTreeId;

    /// Lowercased free-text filter applied to typeName / refdes. Empty
    /// string means no filter.
    wxString                                                 m_searchFilter;

    /// GAL-backed canvas showing the DIFF_SCENE shape rectangles. Pick
    /// callbacks cross-probe to the tree.
    WIDGET_DIFF_CANVAS*                                      m_canvas = nullptr;

    /// True while a canvas click is propagating into the tree, so the tree-
    /// selection handler skips re-centering on a shape that is already under
    /// the cursor.
    bool m_suppressCenter = false;

    SHEET_SWITCHER m_sheetSwitcher;
    KIID_PATH      m_currentCanvasSheet;

    CHANGE_SELECTED_FN m_changeSelectedFn;
};

#endif // DIALOG_KICAD_DIFF_H
