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

#ifndef SCH_MERGE_APPLIER_H
#define SCH_MERGE_APPLIER_H

#include <diff_merge/kicad_merge_engine.h>
#include <diff_merge/merge_validators.h>

#include <map>
#include <memory>
#include <vector>


class SCHEMATIC;
class SCH_ITEM;
class SCH_SHEET_PATH;


namespace KICAD_DIFF
{

/**
 * Materialize a MERGE_PLAN into a merged SCHEMATIC by mutating the ancestor in
 * place.
 *
 * Schematics are hierarchical: an item lives on one SCH_SCREEN identified by a
 * sheet path. The applier indexes every (KIID_PATH, SCH_ITEM*) tuple in the
 * three input schematics, then for each ITEM_RESOLUTION it either removes from
 * the target, clones an item onto the correct sheet, or applies a property
 * delta against the target item.
 *
 * Unlike the PCB applier which builds a fresh output BOARD, the SCH applier
 * mutates the ancestor schematic in place. The reason: SCHEMATIC carries
 * hierarchy, project-relative paths, sheet UUIDs, and instance data that
 * cannot be cleanly recomputed from a fresh empty schematic. Working off the
 * ancestor preserves all of that automatically.
 */
class SCH_MERGE_APPLIER
{
public:
    SCH_MERGE_APPLIER( SCHEMATIC* aAncestor, const SCHEMATIC* aOurs,
                       const SCHEMATIC* aTheirs, MERGE_PLAN aPlan );

    /**
     * Apply the plan to the ancestor. Returns true on success.
     */
    bool Apply();

    struct REPORT
    {
        std::size_t itemsTakenOurs    = 0;
        std::size_t itemsTakenTheirs  = 0;
        std::size_t itemsMergedProps  = 0;
        std::size_t itemsDeleted      = 0;
        std::size_t itemsKept         = 0;
        std::size_t propertiesApplied = 0;
        std::size_t propertiesFailed  = 0;

        /// Number of actions skipped because they targeted a SCH_SHEET. Sheet
        /// re-parenting requires deep clone of the sheet+screen pair and the
        /// path-graph rebuild that follows; this applier handles
        /// non-sheet items only.
        std::size_t sheetActionsSkipped = 0;

        bool requiresConnectivityRebuild = false;

        /// True iff the applier resolved state that lives in the .kicad_pro.
        /// The field-specific flags below identify which subtrees should be
        /// patched so unrelated project data remains untouched.
        bool projectFileTouched = false;
        bool ercSeveritiesTouched = false;
        bool drawingSheetFileTouched = false;

        /// Post-apply validator pipeline result.  Populated unconditionally
        /// by Apply().  Handlers surface failures through the job reporter.
        VALIDATION_REPORT validation;

        /// True iff the caller performed connectivity recomputation after
        /// the applier finished.  CheckConnectivityRebuildFlag uses this
        /// against the plan's requiresConnectivityRebuild signal.
        bool connectivityRebuildPerformed = false;
    };

    const REPORT& GetReport() const { return m_report; }

private:
    struct PathedItem
    {
        SCH_ITEM*       item;
        SCH_SHEET_PATH* sheetPath;
    };

    /// Walk a schematic into a KIID_PATH-keyed table of (item, sheet_path).
    std::map<KIID_PATH, PathedItem>
    indexSchematic( const SCHEMATIC* aSchematic,
                    std::vector<std::unique_ptr<SCH_SHEET_PATH>>& aStorage ) const;

    std::size_t applyPropertyResolutions( SCH_ITEM*                               aTarget,
                                          const std::vector<PROPERTY_RESOLUTION>& aProps,
                                          const SCH_ITEM*                         aOurs,
                                          const SCH_ITEM*                         aTheirs,
                                          const SCH_ITEM*                         aAncestor );

private:
    SCHEMATIC*       m_ancestor;
    const SCHEMATIC* m_ours;
    const SCHEMATIC* m_theirs;
    MERGE_PLAN       m_plan;
    REPORT           m_report;
};

} // namespace KICAD_DIFF

#endif // SCH_MERGE_APPLIER_H
