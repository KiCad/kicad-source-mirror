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

#ifndef KICAD_MERGE_ENGINE_H
#define KICAD_MERGE_ENGINE_H

#include <kicommon.h>

#include <diff_merge/kicad_diff_types.h>

#include <nlohmann/json_fwd.hpp>

#include <string>
#include <vector>


namespace KICAD_DIFF
{

/**
 * Resolution kind for a single property of a single item.
 *
 * Conflicts are resolved at the *property* level, not the item level — the
 * engine takes "we moved it" + "they renamed it" as auto-merge candidates,
 * not as a single conflict on the item. CUSTOM lets the user supply a value
 * that came from neither side (used by the 3-way merge dialog).
 */
enum class PROP_RES
{
    OURS,
    THEIRS,
    ANCESTOR,
    CUSTOM
};


/**
 * Resolution kind for a whole item.
 *
 * MERGE_PROPS is the common case: produce a new item by taking each property
 * from whichever side the per-property resolution says. The other kinds let
 * the user take one side wholesale (after a property conflict that's
 * impractical to resolve property-by-property — e.g., zone outline edits on
 * both sides).
 */
enum class ITEM_RES
{
    TAKE_OURS,
    TAKE_THEIRS,
    TAKE_ANCESTOR,
    MERGE_PROPS,
    DELETE_ITEM,
    KEEP
};


/// Canonical snake_case spellings used in MERGE_PLAN JSON serialization
/// (take_ours / take_theirs / take_ancestor / merge_props / delete / keep).
/// Reused by KICAD_MERGETOOL_AUTO so scripted resolutions can copy enum
/// strings directly out of a plan JSON file.
KICOMMON_API const char* ItemResToString( ITEM_RES aRes );
KICOMMON_API ITEM_RES    ItemResFromString( const std::string& aStr );


/// Canonical lower-case spellings for PROP_RES used inside the JSON
/// serialization of PROPERTY_RESOLUTION (ours / theirs / ancestor /
/// custom). Public so external mergetool scripts can produce/consume
/// the strings directly without depending on the internal serializer.
KICOMMON_API const char* PropResToString( PROP_RES aRes );
KICOMMON_API PROP_RES    PropResFromString( const std::string& aStr );


struct KICOMMON_API PROPERTY_RESOLUTION
{
    wxString   name;
    PROP_RES   kind = PROP_RES::OURS;
    DIFF_VALUE customValue;   // populated only when kind == CUSTOM

    bool                  operator==( const PROPERTY_RESOLUTION& aOther ) const;
    nlohmann::json        ToJson() const;
    static PROPERTY_RESOLUTION FromJson( const nlohmann::json& aJson );
};


struct KICOMMON_API ITEM_RESOLUTION
{
    KIID_PATH                          id;
    ITEM_RES                           kind = ITEM_RES::KEEP;
    std::vector<PROPERTY_RESOLUTION>   props;

    bool                  operator==( const ITEM_RESOLUTION& aOther ) const;
    nlohmann::json        ToJson() const;
    static ITEM_RESOLUTION FromJson( const nlohmann::json& aJson );
};


/**
 * Result of planning a 3-way merge.
 *
 * Plan generation is pure: feeds three DOCUMENT_DIFFs (ancestor-vs-ours,
 * ancestor-vs-theirs, ours-vs-theirs) and produces actions + unresolved.
 * The applier consumes this; validators run after applying.
 */
struct KICOMMON_API MERGE_PLAN
{
    std::vector<ITEM_RESOLUTION>  actions;
    std::vector<KIID_PATH>        unresolved;
    bool                          requiresZoneRefill           = false;
    bool                          requiresConnectivityRebuild  = false;

    bool                  Resolved() const { return unresolved.empty(); }
    std::size_t           ConflictCount() const { return unresolved.size(); }

    nlohmann::json        ToJson() const;
    static MERGE_PLAN     FromJson( const nlohmann::json& aJson );
};


/**
 * Three-way merge plan generator.
 *
 * The engine takes three pre-computed `DOCUMENT_DIFF`s. The pair of diffs
 * (ancestor-vs-ours and ancestor-vs-theirs) is the canonical 3-way input.
 * The third (ours-vs-theirs) is optional but lets the engine emit explicit
 * collision records when both sides independently added the same KIID.
 *
 * The engine does not parse files, does not touch I/O, and does not produce
 * the merged document — those are the applier's job. This separation keeps
 * plan generation trivially unit-testable and deterministic.
 */
class KICOMMON_API KICAD_MERGE_ENGINE
{
public:
    struct OPTIONS
    {
        /// When true, property-orthogonal edits auto-merge silently. When false,
        /// every touched-on-both-sides item becomes a conflict.
        bool preferAutoMerge = true;

        /// When true and a property edit conflicts but the side values are equal,
        /// the resolution is automatically OURS (the equal value).
        bool autoResolveEqualValues = true;
    };

    KICAD_MERGE_ENGINE() = default;
    explicit KICAD_MERGE_ENGINE( const OPTIONS& aOptions ) : m_options( aOptions ) {}

    const OPTIONS& GetOptions() const { return m_options; }
    void           SetOptions( const OPTIONS& aOptions ) { m_options = aOptions; }

    /**
     * Plan the merge given the canonical pair of diffs.
     *
     * `aAncestorOurs` is the diff from ancestor to ours.
     * `aAncestorTheirs` is the diff from ancestor to theirs.
     */
    MERGE_PLAN Plan( const DOCUMENT_DIFF& aAncestorOurs,
                     const DOCUMENT_DIFF& aAncestorTheirs ) const;

private:
    OPTIONS m_options;
};


/**
 * Per-property merge decision result.
 *
 * `kind` is the picked side (OURS or THEIRS — CUSTOM is never produced by
 * the auto-resolver). `isUnresolved` is true when the engine couldn't
 * safely auto-resolve; downstream code records the parent item's id in
 * MERGE_PLAN::unresolved so the user sees a real conflict count.
 */
struct KICOMMON_API PROPERTY_RESOLUTION_OUTCOME
{
    PROP_RES kind         = PROP_RES::OURS;
    bool     isUnresolved = false;
};


/**
 * Decide how to resolve a single property edit between two sides.
 *
 * Pass `nullptr` for whichever side didn't touch the property. The function
 * encodes the engine's documented auto-merge contract:
 *   1. only-ours-touched           -> OURS (clean)
 *   2. only-theirs-touched         -> THEIRS (clean)
 *   3. both touched, after values equal AND options.autoResolveEqualValues
 *                                   -> OURS (equal-value auto-merge)
 *   4. both touched, ours is a no-op (before == after) AND both deltas
 *      share the same before value (no stale baseline) AND
 *      options.preferAutoMerge     -> THEIRS
 *   5. both touched, theirs is a no-op AND both deltas share the same before
 *      value AND options.preferAutoMerge
 *                                   -> OURS
 *   6. otherwise                   -> OURS + unresolved flag
 *
 * Branch order is load-bearing — the equal-end-value auto-resolve precedes
 * the no-op detectors so converging edits land cleanly. The no-op detectors
 * require matching `before` values on both sides; without that check a stale
 * baseline (e.g., theirs computed against a different ancestor) would
 * silently override a real edit on the other side.
 *
 * Pure function — extracted so the decision table can be regression-tested
 * directly without driving PlanMerge through full DOCUMENT_DIFF fixtures.
 */
KICOMMON_API PROPERTY_RESOLUTION_OUTCOME
ResolvePropertyConflict( const PROPERTY_DELTA* aOurs, const PROPERTY_DELTA* aTheirs,
                         const KICAD_MERGE_ENGINE::OPTIONS& aOptions );

} // namespace KICAD_DIFF

#endif // KICAD_MERGE_ENGINE_H
