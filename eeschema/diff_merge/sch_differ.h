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

#ifndef SCH_DIFFER_H
#define SCH_DIFFER_H

#include <diff_merge/kicad_differ.h>

#include <wx/string.h>


class SCHEMATIC;
class SCH_ITEM;
class SCH_SHEET_PATH;


namespace KICAD_DIFF
{

/**
 * Diff two already-parsed SCHEMATICs and produce a DOCUMENT_DIFF.
 *
 * Identity is `KIID_PATH`-keyed: every item's identifier is the sheet path
 * (a vector of sheet KIIDs from root) plus the item's own KIID. This is the
 * canonical KiCad identity model for hierarchical schematics — the same item
 * KIID can appear in multiple sheet instances with different `KIID_PATH`s.
 *
 * The differ walks each schematic's full sheet list, collects items per
 * `KIID_PATH`, runs IDENTITY_RECONCILER, then per matched pair uses
 * PROPERTY_MANAGER for per-property delta enumeration.
 *
 * Sheet renames are handled by the identity reconciler's similarity fallback:
 * a renamed sheet's children have a different KIID_PATH prefix, but the same
 * (type, position, key properties) — the reconciler will match them across
 * the rename when the threshold is met.
 */
class SCH_DIFFER : public KICAD_DIFFER
{
public:
    SCH_DIFFER( const SCHEMATIC* aBefore, const SCHEMATIC* aAfter,
                const wxString& aPath = wxEmptyString );
    ~SCH_DIFFER() override;

    /// Restrict the diff to one sheet on each side. Items get normalized
    /// KIID_PATHs so they align across mismatched hierarchies. Empty paths
    /// disable scoping.
    void SetScope( const KIID_PATH& aBeforeScope, const KIID_PATH& aAfterScope );

    DOCUMENT_DIFF Diff() override;

    const SCHEMATIC* Before() const { return m_before; }
    const SCHEMATIC* After()  const { return m_after; }

private:
    struct WalkedItem
    {
        KIID_PATH       id;
        const SCH_ITEM* item;
        SCH_SHEET_PATH* sheetPath;  // raw pointer into m_pathStorage
    };

    /// Walk a schematic and produce a flat list of (KIID_PATH, item) tuples.
    /// `aStorage` retains the SCH_SHEET_PATH objects for the lifetime of the diff
    /// because WalkedItem::sheetPath references them. Non-empty `aScope`
    /// limits the walk to that one SCH_SHEET_PATH.
    void walk( const SCHEMATIC* aSchematic, std::vector<WalkedItem>& aOut,
               std::vector<std::unique_ptr<SCH_SHEET_PATH>>& aStorage, const KIID_PATH& aScope = {} ) const;

    /// Property-level delta via PROPERTY_MANAGER. The sheet paths anchor each
    /// item to its instance; SCH_SYMBOL property getters (Reference, Value,
    /// footprint, unit) read m_schematic->CurrentSheet() under the hood, so
    /// the differ has to switch CurrentSheet to the walked path around each
    /// read or it would compare the wrong instance on multi-sheet
    /// hierarchies / repeated sheets.
    std::vector<PROPERTY_DELTA> diffProperties( const SCH_ITEM* aBefore,
                                                const SCH_ITEM* aAfter,
                                                const SCH_SHEET_PATH* aBeforePath,
                                                const SCH_SHEET_PATH* aAfterPath ) const;

    /// Type name for an SCH_ITEM (used in diff records).
    static wxString itemTypeName( const SCH_ITEM* aItem );

    /// Refdes if the item is a symbol; nullopt otherwise.
    static std::optional<wxString> itemRefdes( const SCH_ITEM* aItem,
                                               const SCH_SHEET_PATH* aPath );

    static void sortChanges( std::vector<ITEM_CHANGE>& aChanges );

private:
    const SCHEMATIC* m_before;
    const SCHEMATIC* m_after;
    wxString         m_path;
    KIID_PATH        m_scopeBefore;
    KIID_PATH        m_scopeAfter;
};

} // namespace KICAD_DIFF

#endif // SCH_DIFFER_H
