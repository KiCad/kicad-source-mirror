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

#ifndef KICAD_DIFF_MERGE_VALIDATION_PIPELINE_H
#define KICAD_DIFF_MERGE_VALIDATION_PIPELINE_H

#include <diff_merge/merge_validators.h>

#include <kicommon.h>


namespace KICAD_DIFF
{

/**
 * Inputs needed to run the post-apply validator pipeline.
 *
 * The applier collects this struct after the merged document is finalized
 * and passes it to RunPostApplyValidators().  The struct intentionally
 * carries only loose primitives (refdes vector, two bools, three ints) so
 * the pipeline is testable without document-class fixtures.
 *
 * - refdesEntries: every refdes-bearing item in the merged document, paired
 *   with its KIID_PATH for cross-probe.  Empty refdes entries are skipped
 *   by CheckRefdesUniqueness.
 * - planRequiredRebuild: copy of MERGE_PLAN::requiresConnectivityRebuild --
 *   the engine's verdict.
 * - applierReportedRebuild: true iff the applier actually performed the
 *   rebuild it was told to.  This is the "did the caller honour the
 *   contract?" check.
 * - ancestorSchemaVersion / oursSchemaVersion / theirsSchemaVersion: the
 *   per-side serialization versions.  Mismatches surface as ERROR so the
 *   merge driver can refuse to write a file the loaders won't read.
 */
struct KICOMMON_API VALIDATION_INPUT
{
    std::vector<REFDES_ENTRY> refdesEntries;
    bool                      planRequiredRebuild    = false;
    bool                      applierReportedRebuild = false;
    int                       ancestorSchemaVersion  = 0;
    int                       oursSchemaVersion      = 0;
    int                       theirsSchemaVersion    = 0;
};


/**
 * Run every standard post-apply validator and merge their reports.
 *
 * Pure function: no I/O, no global state.  Composes
 *   - CheckRefdesUniqueness(refdesEntries)
 *   - CheckConnectivityRebuildFlag(planRequiredRebuild, applierReportedRebuild)
 *   - CheckSchemaVersions(ancestor, ours, theirs)
 * and aggregates their failures via VALIDATION_REPORT::Merge.
 *
 * The applier stores the result on its REPORT::validation; the CLI merge
 * handlers surface the failures through the job reporter.
 */
KICOMMON_API VALIDATION_REPORT RunPostApplyValidators( const VALIDATION_INPUT& aInput );

} // namespace KICAD_DIFF

#endif // KICAD_DIFF_MERGE_VALIDATION_PIPELINE_H
