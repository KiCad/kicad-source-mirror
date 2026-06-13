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

#ifndef PCB_DIFFER_H
#define PCB_DIFFER_H

#include <diff_merge/kicad_differ.h>
#include <diff_merge/identity_reconciler.h>

#include <wx/string.h>


class BOARD;
class BOARD_ITEM;
class FOOTPRINT;


namespace KICAD_DIFF
{

/**
 * Diff two already-parsed BOARDs and produce a DOCUMENT_DIFF.
 *
 * Identity is UUID-keyed primarily, with similarity fallback via
 * IDENTITY_RECONCILER for cases where copy-paste or third-party imports
 * have rewritten KIIDs.
 *
 * The differ walks the union of `BOARD::GetItemSet()` on both boards plus
 * each footprint's children (pads, text, graphics) so that "inside the
 * footprint" diffs surface as nested ITEM_CHANGE::children entries on the
 * footprint's record. This mirrors how KiCad organizes the data model and
 * lets UI consumers fold and unfold per-footprint detail.
 */
class PCB_DIFFER : public KICAD_DIFFER
{
public:
    PCB_DIFFER( const BOARD* aBefore, const BOARD* aAfter, const wxString& aPath = wxEmptyString );
    ~PCB_DIFFER() override;

    DOCUMENT_DIFF Diff() override;

    /// Expose the underlying boards for callers that want to drive their own walk
    /// (e.g., the merge applier reaches in to materialize ITEM_CHANGE records).
    const BOARD* Before() const { return m_before; }
    const BOARD* After() const { return m_after; }

private:
    /// Compute property deltas between two items of the same dynamic type.
    /// Returns the populated PROPERTY_DELTA list; empty if items compare equal
    /// via PROPERTY_MANAGER (i.e., no surfaced property changed).
    std::vector<PROPERTY_DELTA> diffProperties( const BOARD_ITEM* aBefore, const BOARD_ITEM* aAfter ) const;

    /// Build the ITEM_DESCRIPTOR for the reconciler from a BOARD_ITEM.
    ITEM_DESCRIPTOR makeDescriptor( const BOARD_ITEM* aItem ) const;

    /// Construct a child-level diff for nested items inside a footprint pair.
    std::vector<ITEM_CHANGE> diffFootprintChildren( const FOOTPRINT* aBefore, const FOOTPRINT* aAfter ) const;

    /// Stable, deterministic sort of ITEM_CHANGEs (by id, then typeName, then kind).
    static void sortChanges( std::vector<ITEM_CHANGE>& aChanges );

    /// Convert the dynamic class string for an item into the type name used in diffs.
    static wxString itemTypeName( const BOARD_ITEM* aItem );

    /// Extract a presentation label: footprint refdes, or routing net name for tracks/vias.
    static std::optional<wxString> itemRefdes( const BOARD_ITEM* aItem );

private:
    const BOARD* m_before;
    const BOARD* m_after;
    wxString     m_path;
};

} // namespace KICAD_DIFF

#endif // PCB_DIFFER_H
