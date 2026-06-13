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

#ifndef KICAD_DIFF_DIFF_TREE_GROUPING_H
#define KICAD_DIFF_DIFF_TREE_GROUPING_H

#include <kicommon.h>

#include <diff_merge/diff_scene.h>
#include <diff_merge/kicad_diff_types.h>

#include <wx/string.h>

#include <array>
#include <vector>


namespace KICAD_DIFF
{

/**
 * One row in a grouped change tree — an `ITEM_CHANGE` plus its display
 * label (typeName + optional refdes in `[brackets]`).
 *
 * The `change` pointer references an item owned by the source
 * `DOCUMENT_DIFF`; the entry must not outlive the diff.
 */
struct KICOMMON_API CHANGE_TREE_ENTRY
{
    const ITEM_CHANGE* change;
    wxString           itemLabel;
};


/**
 * One bucket in a grouped change tree — a `CHANGE_KIND`, a human-
 * readable group label, and the entries belonging to that bucket.
 *
 * The group label encodes the filter state: `"Kind (N)"` when all
 * `N` entries of this kind are visible, `"Kind (V/N)"` when only `V`
 * of the `N` total pass the active search filter.
 */
struct KICOMMON_API CHANGE_TREE_GROUP
{
    CHANGE_KIND                    kind;
    wxString                       groupLabel;
    std::vector<CHANGE_TREE_ENTRY> entries;
};


/**
 * Human-readable label for a CHANGE_KIND (e.g. "Added", "Removed",
 * "Modified", "Collision", "Duplicate ID"). Goes through the wx
 * translation machinery via `_()` so the result is locale-aware.
 *
 * Pure dispatch over the enum; useful as a standalone helper for any
 * UI surface that displays change kinds.
 */
KICOMMON_API wxString ChangeKindLabel( CHANGE_KIND aKind );


/**
 * Predicate: does this item change match the active search filter?
 *
 * Match rule: filter empty -> true; otherwise the lowercased filter
 * must appear as a substring of the item's `typeName.Lower()` or, if
 * the item has a refdes, the refdes lowercased. `aFilter` is expected
 * to already be lowercased by the caller — this lets repeated calls
 * over a list avoid re-lowercasing the same string.
 */
KICOMMON_API bool ChangeMatchesSearchFilter( const ITEM_CHANGE& aChange, const wxString& aLowercaseFilter );


/**
 * Group the changes in a `DOCUMENT_DIFF` by kind, apply category and
 * search filters, and return the resulting tree structure for a UI
 * to render.
 *
 * Recursion into `ITEM_CHANGE::children` is automatic — nested footprint
 * pads, sub-sheet symbols, etc. each appear as their own entry under
 * the appropriate kind bucket. PCB routing changes (tracks/arcs/vias) with
 * the same net-name refdes are collapsed into a single `NET [name]` entry
 * per kind for display only.
 *
 * `aVisibleCategories` is indexed by `static_cast<size_t>( CATEGORY )`;
 * a kind whose `CategoryFor()` maps to a hidden category is dropped
 * entirely. `aSearchFilter` is the raw user input — this function
 * lowercases it once internally and passes the result to
 * `ChangeMatchesSearchFilter`.
 *
 * Buckets with zero visible entries after filtering are omitted from
 * the result (the UI doesn't show empty groups).
 *
 * Pure function — no I/O, no wx widget interaction, no globals.
 */
KICOMMON_API std::vector<CHANGE_TREE_GROUP>
             BuildChangeTreeGroups( const DOCUMENT_DIFF& aDiff, const wxString& aSearchFilter,
                                    const std::array<bool, CATEGORY_COUNT>& aVisibleCategories );

} // namespace KICAD_DIFF

#endif // KICAD_DIFF_DIFF_TREE_GROUPING_H
