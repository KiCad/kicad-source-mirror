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

#ifndef KICAD_DIFF_AUTO_RESOLUTION_H
#define KICAD_DIFF_AUTO_RESOLUTION_H

#include <kicommon.h>

#include <diff_merge/kicad_merge_engine.h>

#include <wx/string.h>

#include <map>
#include <string>
#include <vector>


namespace KICAD_DIFF
{

/**
 * Outcome of parsing a `KICAD_MERGETOOL_AUTO` JSON document.
 *
 * The dialog's headless path needs to distinguish three states: the env
 * variable is unset (interactive use, no auto-mode), the file was
 * successfully parsed (apply the resolutions), or the file existed but
 * something about it was wrong (CI must fail loudly rather than silently
 * fall back to an interactive UI with no human present).
 */
enum class AUTO_RESOLUTION_STATUS
{
    OK,             ///< parsed cleanly; `resolutions` is populated
    INVALID_JSON,   ///< input could not be parsed as JSON
    NOT_OBJECT,     ///< JSON parsed but the root was not an object
    BAD_VALUE,      ///< at least one entry's value wasn't a string
    UNKNOWN_KIND,   ///< at least one entry's string wasn't a known ITEM_RES
    ENGINE_INTERNAL_KIND   ///< at least one entry asked for an engine-internal
                           ///< kind (KEEP / DELETE / MERGE_PROPS); only the
                           ///< TAKE_* family makes sense as a scripted choice
};


/**
 * Result of `ParseAutoResolutionJson`. `resolutions` is populated only
 * when status == OK. `errorContext` carries the per-key context that
 * triggered a non-OK status, suitable for trace logging.
 */
struct KICOMMON_API AUTO_RESOLUTION_PARSE_RESULT
{
    AUTO_RESOLUTION_STATUS                  status = AUTO_RESOLUTION_STATUS::OK;
    std::map<wxString, ITEM_RES>            resolutions;
    wxString                                errorContext;
};


/**
 * Parse a `KICAD_MERGETOOL_AUTO`-format JSON object mapping item IDs
 * (KIID_PATH strings) to ITEM_RES spellings (the canonical snake_case
 * names emitted by `ItemResToString`).
 *
 * Only the TAKE_* family is accepted; KEEP / DELETE / MERGE_PROPS are
 * engine-internal and would corrupt a scripted resolution.
 *
 * Pure function — no I/O, no globals, no logging. Callers that want
 * trace output should log against the `errorContext` field.
 */
KICOMMON_API AUTO_RESOLUTION_PARSE_RESULT
ParseAutoResolutionJson( const std::string& aJsonContent );


/**
 * Outcome of applying an auto-resolution map to a MERGE_PLAN.
 *
 * `COMPLETE` — every conflict in the plan was covered by the auto map,
 * the plan's unresolved list was cleared, and the caller should apply
 * the plan (EndModal wxID_APPLY in the dialog case).
 *
 * `PARTIAL` — at least one conflict had no auto-resolution; the plan
 * has been left untouched and the caller should treat this as a
 * failure (EndModal wxID_CANCEL).
 *
 * `NO_CONFLICTS` — the plan had no conflicts to resolve in the first
 * place; trivially complete.
 */
enum class APPLY_AUTO_RESOLUTIONS_STATUS
{
    COMPLETE,
    PARTIAL,
    NO_CONFLICTS,
};


struct KICOMMON_API APPLY_AUTO_RESOLUTIONS_RESULT
{
    APPLY_AUTO_RESOLUTIONS_STATUS   status      = APPLY_AUTO_RESOLUTIONS_STATUS::NO_CONFLICTS;
    std::size_t                     appliedCount = 0;
    /// Set only when `status == PARTIAL` AND the partial result was caused
    /// by a missing entry in the auto-resolution map. A PARTIAL caused by
    /// an out-of-range `actionIndex` leaves this empty (the function has
    /// no valid id to report).
    KIID_PATH                       firstMissingId;
};


/**
 * Apply a parsed auto-resolution map to a MERGE_PLAN's conflicts.
 *
 * `aConflictActionIndices` is the list of `plan.actions` indices that
 * the dialog identified as conflicts (i.e. ids appearing in
 * `plan.unresolved`). Building it is the caller's job; this function
 * just iterates that list and applies the map.
 *
 * On COMPLETE the plan's `unresolved` vector is cleared. On PARTIAL
 * the plan is left untouched.
 */
KICOMMON_API APPLY_AUTO_RESOLUTIONS_RESULT
ApplyAutoResolutions( MERGE_PLAN&                          aPlan,
                      const std::vector<std::size_t>&      aConflictActionIndices,
                      const std::map<wxString, ITEM_RES>&  aResolutions );


/**
 * Build the list of conflict-action indices the dialog walks (i.e. the
 * positions in `plan.actions` whose ids appear in `plan.unresolved`).
 *
 * Each entry carries the action index plus a human-readable label
 * truncated to 40 chars (an ellipsis + the right-most 39 chars if the
 * id string is longer). Truncation matches the dialog's display rule
 * so headless tools can produce the same labels users see.
 */
struct KICOMMON_API CONFLICT_LIST_ENTRY
{
    std::size_t actionIndex;
    wxString    label;
    KIID_PATH   id;
};


KICOMMON_API std::vector<CONFLICT_LIST_ENTRY> BuildConflictList( const MERGE_PLAN& aPlan );


/**
 * Return the subset of conflict actions whose `kind` is NOT one of the
 * concrete TAKE_OURS / TAKE_THEIRS / TAKE_ANCESTOR — the dialog uses
 * this to refuse Apply when the user hasn't picked a side on every
 * conflict. KEEP / MERGE_PROPS at that point mean the engine's default
 * is still in effect, which silently loses user intent.
 */
KICOMMON_API std::vector<KIID_PATH>
CollectUnresolvedConflicts( const MERGE_PLAN&                aPlan,
                            const std::vector<std::size_t>&  aConflictActionIndices );


/**
 * Resolve the best bounding box for a conflicted item across the three
 * sides of a 3-way merge.
 *
 * The merge dialog hosts a canvas that previews each conflict on the
 * user's currently-selected side. Some items only exist on one or two
 * sides — a "Take ancestor" preview of an ours-only addition should
 * still highlight the item's bbox somewhere reasonable, not collapse
 * to a degenerate point.
 *
 * Lookup order: `aPrimary` (the user's currently-selected side, pass
 * an empty map for "no preferred side"), then ours, then theirs, then
 * ancestor. The first map that has a usable bbox for `aId` wins. A
 * bbox is considered usable if at least one of its dimensions is
 * positive — a 1-unit-wide segment is fine, but a zero-size bbox at
 * any origin is treated as absence and falls through to the next side.
 *
 * Returns `std::nullopt` if none of the four maps contain a usable
 * bbox.
 *
 * Pure function — no widget interaction. Used by the dialog's
 * `rebuildCanvas` and by any headless surface (e.g. a future kicad-cli
 * `mergetool --preview` flag) that wants to identify a conflict's
 * spatial location.
 */
KICOMMON_API std::optional<BOX2I>
ResolveConflictBBox( const KIID_PATH&                  aId,
                     const std::map<KIID_PATH, BOX2I>& aPrimary,
                     const std::map<KIID_PATH, BOX2I>& aOurs,
                     const std::map<KIID_PATH, BOX2I>& aTheirs,
                     const std::map<KIID_PATH, BOX2I>& aAncestor );


/**
 * Build the human-readable detail text shown in the merge dialog's
 * resolution panel for a given ITEM_RESOLUTION.
 *
 * Output format:
 *   "Item id: <KIID_PATH>\nCurrent resolution: <kind label>\n\n
 *    <N> property edit(s) in this resolution.\n"
 *
 * The kind labels are locale-aware (`_(...)`). Pure dispatch + text
 * formatting; useful for any UI surface displaying a resolution's
 * summary.
 */
KICOMMON_API wxString
BuildConflictDetailText( const ITEM_RESOLUTION& aResolution );

} // namespace KICAD_DIFF

#endif // KICAD_DIFF_AUTO_RESOLUTION_H
