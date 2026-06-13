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
 * or you may search the http://www.gnu.org website for the version 3 license,
 * or you may write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
 */

#ifndef DIALOG_KICAD_MERGE_3WAY_H
#define DIALOG_KICAD_MERGE_3WAY_H

#include "dialog_kicad_merge_3way_base.h"

#include <diff_merge/diff_scene.h>
#include <diff_merge/kicad_merge_engine.h>

#include <map>


class WIDGET_DIFF_CANVAS;


/**
 * 3-way merge resolution dialog (Phase 8).
 *
 * Takes a MERGE_PLAN with unresolved items and lets the user pick which side
 * to keep for each. Stores the per-item resolution choice back into the
 * plan. On Apply, the plan is returned via GetResolvedPlan() with every
 * conflict assigned a concrete kind (TAKE_OURS / TAKE_THEIRS / TAKE_ANCESTOR).
 *
 * Future revision: replace the text detail view with a GAL canvas plus the
 * three ancestor/ours/theirs thumbnails the plan calls for. Today the dialog
 * is text-based so the resolution workflow can run headlessly under Cairo
 * for the mergetool app even on systems without OpenGL.
 */
class DIALOG_KICAD_MERGE_3WAY : public DIALOG_KICAD_MERGE_3WAY_BASE
{
public:
    /**
     * Phase 8 context for the conflict canvas. Each side carries the
     * document geometry extracted from that side's loaded BOARD or
     * SCHEMATIC plus a per-item bbox map; the dialog picks the side to
     * display based on the radio selection and outlines the focused
     * conflict at its on-that-side coordinates so a moved-on-theirs item
     * highlights at the moved location when previewing Theirs.
     */
    struct CONFLICT_CONTEXT
    {
        KICAD_DIFF::DOCUMENT_GEOMETRY ancestorGeometry;
        KICAD_DIFF::DOCUMENT_GEOMETRY oursGeometry;
        KICAD_DIFF::DOCUMENT_GEOMETRY theirsGeometry;

        /// Per-side bbox of each conflicting item. Empty entries fall back
        /// through ancestor → ours → theirs at lookup time.
        std::map<KIID_PATH, BOX2I>    ancestorBBoxes;
        std::map<KIID_PATH, BOX2I>    oursBBoxes;
        std::map<KIID_PATH, BOX2I>    theirsBBoxes;
    };

    DIALOG_KICAD_MERGE_3WAY( wxWindow* aParent, const KICAD_DIFF::MERGE_PLAN& aPlan,
                             CONFLICT_CONTEXT aContext = {} );

    ~DIALOG_KICAD_MERGE_3WAY() override = default;

    /// Returns the plan with the user's resolutions applied. Only meaningful
    /// after the dialog returns wxID_APPLY.
    const KICAD_DIFF::MERGE_PLAN& GetResolvedPlan() const { return m_plan; }

protected:
    void OnClose( wxCloseEvent& aEvent ) override;
    void OnConflictSelected( wxCommandEvent& aEvent ) override;
    void OnResolutionChanged( wxCommandEvent& aEvent ) override;
    void OnApply( wxCommandEvent& aEvent ) override;
    void OnCancel( wxCommandEvent& aEvent ) override;

private:
    /// Populate the conflict list from the plan's unresolved items.
    void buildList();

    /// Update the detail panel for the selected conflict.
    void showConflict( int aIndex );

    /// Rebuild the canvas scene from the current side (per radio) and the
    /// active conflict's bbox. Called whenever the picker selection or the
    /// side selection changes.
    void rebuildCanvas();

    /// Which side's geometry the canvas should currently display.
    enum class SIDE { OURS, THEIRS, ANCESTOR };
    SIDE activeSide() const;

    KICAD_DIFF::MERGE_PLAN          m_plan;
    CONFLICT_CONTEXT                m_context;

    /// Indices into m_plan.actions matching the order of m_listConflicts.
    std::vector<std::size_t>        m_conflictActionIndex;

    /// Currently-displayed conflict (index into m_conflictActionIndex), -1 if none.
    int                             m_currentConflict = -1;

    /// GAL-backed conflict viewer injected into the resolution panel.
    /// nullptr when the panel had no sizer to splice into.
    WIDGET_DIFF_CANVAS*             m_canvas = nullptr;
};

#endif // DIALOG_KICAD_MERGE_3WAY_H
