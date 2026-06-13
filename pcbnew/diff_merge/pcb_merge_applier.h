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

#ifndef PCB_MERGE_APPLIER_H
#define PCB_MERGE_APPLIER_H

#include <diff_merge/kicad_merge_engine.h>
#include <diff_merge/merge_validators.h>

#include <wx/string.h>

#include <memory>


class BOARD;
class BOARD_ITEM;


namespace KICAD_DIFF
{

/**
 * Materialize a MERGE_PLAN into a real merged BOARD.
 *
 * Construction takes the three input boards (ancestor, ours, theirs) by
 * reference and a plan. `Apply()` produces a new BOARD that owns its items;
 * the input boards are never mutated.
 *
 * Resolution semantics:
 *   - TAKE_OURS / TAKE_THEIRS / TAKE_ANCESTOR: clone the chosen side's item
 *     into the result with the same KIID.
 *   - DELETE: skip the item entirely (no record in the result).
 *   - KEEP: clone ancestor's item; used for "conservatively conflicted"
 *     items where the plan flagged unresolved.
 *   - MERGE_PROPS: clone ours's item as the starting point, then apply each
 *     PROPERTY_RESOLUTION via PROPERTY_MANAGER::SetValue. Properties not in
 *     the resolution list are inherited from ours.
 *
 * Items that exist on no side, or are listed in the plan with no surviving
 * source, are silently dropped.
 *
 * The applier never invents item state. If a MERGE_PROPS resolution references
 * a property that cannot be set (read-only, hidden, missing on the target),
 * the property is left at the OURS value and the failure is logged via
 * wxLogTrace( traceDiffMerge ).
 *
 * The plan is stored by value so a temporary `engine.Plan(...)` result is
 * safe to pass into the constructor.
 */
class PCB_MERGE_APPLIER
{
public:
    PCB_MERGE_APPLIER( const BOARD* aAncestor, const BOARD* aOurs, const BOARD* aTheirs,
                       MERGE_PLAN aPlan );

    /**
     * Produce the merged board. Returns a unique_ptr; on null indicates a
     * fatal application error (currently only "no input boards").
     */
    std::unique_ptr<BOARD> Apply();

    /**
     * Report on the application after Apply() runs.
     */
    struct REPORT
    {
        std::size_t itemsTakenOurs    = 0;
        std::size_t itemsTakenTheirs  = 0;
        std::size_t itemsMergedProps  = 0;
        std::size_t itemsDeleted      = 0;
        std::size_t itemsKept         = 0;
        std::size_t propertiesApplied = 0;
        std::size_t propertiesFailed  = 0;

        bool requiresZoneRefill          = false;
        bool requiresConnectivityRebuild = false;

        /// True iff the applier resolved state that lives in the .kicad_pro
        /// or a project sibling file. The field-specific flags below identify
        /// which project subtrees should be patched so unrelated data remains
        /// untouched.
        bool projectFileTouched = false;
        bool drcSeveritiesTouched = false;
        bool netClassesTouched = false;

        /// Drawing sheet path the applier resolved (from a doc-level
        /// resolution). Empty until drawingSheetFileSet flips true. The
        /// applier can't reach the result BOARD's project directly (the
        /// result is a fresh project-less BOARD), so it stages the value
        /// here for the handler to apply onto ancestor's project before
        /// SaveProjectCopy.
        wxString drawingSheetFile;
        bool     drawingSheetFileSet = false;

        /// Custom DRC rules (.kicad_dru) content the applier resolved.
        /// Empty until customDrcRulesSet flips true.  The rules content
        /// lives in a sibling file rather than the project file, so the
        /// handler writes it out separately from SaveProjectCopy.
        wxString customDrcRules;
        bool     customDrcRulesSet = false;

        /// fp-lib-table content the applier resolved.  Empty until
        /// fpLibTableSet flips true.  The handler writes it into the
        /// merged project directory.
        wxString fpLibTable;
        bool     fpLibTableSet = false;

        /// sym-lib-table content the applier resolved.  Same staging
        /// model as fpLibTable.
        wxString symLibTable;
        bool     symLibTableSet = false;

        /// Post-apply validator pipeline result (refdes uniqueness,
        /// connectivity-rebuild-ack, schema-version compatibility).
        /// Populated unconditionally by Apply() so handlers can surface
        /// the failures through the job reporter.
        VALIDATION_REPORT validation;

        /// True iff the caller invoked connectivity recomputation after
        /// the applier produced the merged document.  The applier itself
        /// doesn't run rebuilds (it has no connectivity engine attached),
        /// so this stays false unless the caller flips it before reading
        /// `validation`.  CheckConnectivityRebuildFlag uses this against
        /// the plan's requiresConnectivityRebuild signal.
        bool connectivityRebuildPerformed = false;
    };

    const REPORT& GetReport() const { return m_report; }

private:
    /// Locate an item (top-level or footprint child) by UUID on one of the
    /// source boards.  Delegates to BOARD::ResolveItem, which maintains its own
    /// KIID->item cache and resolves footprint children under their own UUIDs.
    const BOARD_ITEM* findItem( const BOARD* aBoard, const KIID& aId ) const;

    /// Clone a board item using its virtual Clone(); returns nullptr if the
    /// source is null, the clone fails, or `BOARD::Add()` rejects the item.
    BOARD_ITEM* cloneInto( BOARD* aTarget, const BOARD_ITEM* aSource ) const;

    /// Apply property-level resolutions to a clone of `aOurs` (or `aTheirs`
    /// per PROP_RES). Returns the number of properties successfully applied.
    std::size_t applyPropertyResolutions( BOARD_ITEM*                             aTarget,
                                          const std::vector<PROPERTY_RESOLUTION>& aProps,
                                          const BOARD_ITEM*                       aOurs,
                                          const BOARD_ITEM*                       aTheirs,
                                          const BOARD_ITEM*                       aAncestor );

private:
    const BOARD* m_ancestor;
    const BOARD* m_ours;
    const BOARD* m_theirs;
    MERGE_PLAN   m_plan;
    REPORT       m_report;
};

} // namespace KICAD_DIFF

#endif // PCB_MERGE_APPLIER_H
