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

#ifndef KICAD_DIFF_MERGE_VALIDATORS_H
#define KICAD_DIFF_MERGE_VALIDATORS_H

#include <kicommon.h>
#include <kiid.h>
#include <widgets/report_severity.h>

#include <wx/string.h>

#include <vector>


namespace KICAD_DIFF
{

/**
 * Outcome of a single validator run.
 *
 * Validators are advisory: a non-empty failure list does not automatically
 * abort the merge. The caller (CLI driver, mergetool app, git driver)
 * decides whether to surface the failures to the user or block the write.
 */
struct KICOMMON_API VALIDATION_FAILURE
{
    SEVERITY                 severity = RPT_SEVERITY_WARNING;
    wxString                 validator;        // human-readable validator name
    wxString                 message;          // failure description for UI
    std::vector<KIID_PATH>   relatedItems;     // items implicated, for cross-probe
};


struct KICOMMON_API VALIDATION_REPORT
{
    std::vector<VALIDATION_FAILURE> failures;

    bool   Passed() const  { return failures.empty(); }
    bool   HasErrors() const;
    size_t Count() const   { return failures.size(); }

    void   Merge( VALIDATION_REPORT&& aOther );
};


/**
 * Reference-designator uniqueness over a flat list of (refdes, id) pairs.
 *
 * Cross-document utility: PCB validates that no two footprints share a refdes,
 * SCH validates symbol references across the hierarchy (instance-aware), CLI
 * uses it for project-level checks.
 */
struct KICOMMON_API REFDES_ENTRY
{
    wxString  refdes;
    KIID_PATH id;
};


/**
 * Run refdes-uniqueness checks. Empty refdes entries are skipped (unassigned
 * components are not collisions).
 */
KICOMMON_API VALIDATION_REPORT CheckRefdesUniqueness(
        const std::vector<REFDES_ENTRY>& aEntries );


/**
 * Information needed for the connectivity-rebuild side-effect check.
 *
 * The merge engine sets `MERGE_PLAN::requiresConnectivityRebuild` when a
 * connectivity-affecting item changes. The applier must have called the
 * appropriate rebuild before the merged document is written; this validator
 * is a guard against missed rebuilds.
 */
KICOMMON_API VALIDATION_REPORT CheckConnectivityRebuildFlag(
        bool aPlanRequiredRebuild, bool aApplierReportedRebuild );


/**
 * Schema-version compatibility check: every input to the merge must use a
 * compatible serialization version. Mismatches are surfaced as ERROR so the
 * caller can refuse the merge before producing a file the loaders won't read.
 */
KICOMMON_API VALIDATION_REPORT CheckSchemaVersions( int aAncestorVersion,
                                                   int aOursVersion,
                                                   int aTheirsVersion );

} // namespace KICAD_DIFF

#endif // KICAD_DIFF_MERGE_VALIDATORS_H
