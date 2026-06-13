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

#ifndef KICAD_DIFFER_H
#define KICAD_DIFFER_H

#include <kicommon.h>

#include <diff_merge/kicad_diff_types.h>
#include <diff_merge/identity_reconciler.h>

#include <functional>
#include <set>


namespace KICAD_DIFF
{

/**
 * Abstract base for every per-document-type differ.
 *
 * Concrete differs live with their document type (PCB_DIFFER in pcbnew, SCH_DIFFER
 * in eeschema, etc.) so that the heavy includes stay confined to their kiface.
 *
 * Lifetime model: callers construct a differ with already-parsed documents,
 * configure it via SetOptions(), call Diff(), then discard. Differs do not
 * cache results internally; re-running Diff() reproduces the same DOCUMENT_DIFF
 * for the same inputs (deterministic).
 */
class KICOMMON_API KICAD_DIFFER
{
public:
    struct OPTIONS
    {
        /// Emit per-property deltas via PROPERTY_MANAGER. When false, only ADDED /
        /// REMOVED / coarse MODIFIED records are produced.
        bool deepCompare = true;

        /// Configuration for identity reconciliation. The differ owns the
        /// reconciler; callers can pre-tune it here for fixture-specific behavior.
        IDENTITY_RECONCILER::CONFIG identity;

        /// Optional progress reporter — invoked with a fraction in [0, 1].
        std::function<void( double )> progress;
    };

    virtual ~KICAD_DIFFER();

    void           SetOptions( const OPTIONS& aOptions ) { m_options = aOptions; }
    const OPTIONS& GetOptions() const { return m_options; }

    /**
     * Produce a DOCUMENT_DIFF of the inputs the concrete differ was constructed with.
     *
     * Must be deterministic for fixed inputs. Concrete differs sort their output by
     * KIID_PATH so the JSON serialization is bit-stable across runs.
     */
    virtual DOCUMENT_DIFF Diff() = 0;

protected:
    KICAD_DIFFER() = default;

    OPTIONS m_options;
};


/**
 * Shared name-keyed library diff used by FP_LIB_DIFFER and SYM_LIB_DIFFER.
 *
 * Walks the union of names in @p aBefore and @p aAfter, classifying each entry
 * as ADDED, REMOVED, or MODIFIED. @p aBBox yields the bounding box stored on
 * each change; @p aChanged decides whether two present items differ. Library
 * files aren't UUID-keyed, so each change's id is the deterministic synthetic
 * LibraryItemKiidPath( name ) the appliers look up by.
 */
template <typename MAP, typename BBoxFn, typename ChangedFn>
DOCUMENT_DIFF DiffLibraryByName( const MAP& aBefore, const MAP& aAfter, const wxString& aPath,
                                 const wxString& aDocType, const wxString& aTypeName,
                                 const KICAD_DIFFER::OPTIONS& aOptions, BBoxFn aBBox,
                                 ChangedFn aChanged )
{
    DOCUMENT_DIFF result;
    result.path    = aPath;
    result.docType = aDocType;

    std::set<wxString> allNames;

    for( const auto& [name, item] : aBefore )
        allNames.insert( name );

    for( const auto& [name, item] : aAfter )
        allNames.insert( name );

    if( aOptions.progress )
        aOptions.progress( 0.1 );

    std::size_t processed = 0;

    for( const wxString& name : allNames )
    {
        ++processed;

        if( aOptions.progress )
            aOptions.progress( 0.1 + 0.9 * processed / allNames.size() );

        auto        beforeIt = aBefore.find( name );
        auto        afterIt  = aAfter.find( name );
        const auto* before   = beforeIt == aBefore.end() ? nullptr : beforeIt->second;
        const auto* after    = afterIt == aAfter.end() ? nullptr : afterIt->second;

        ITEM_CHANGE c;
        c.id       = LibraryItemKiidPath( name );
        c.typeName = aTypeName;
        c.refdes   = name;

        if( before && !after )
        {
            c.kind = CHANGE_KIND::REMOVED;
            c.bbox = aBBox( before );
            result.changes.push_back( std::move( c ) );
        }
        else if( !before && after )
        {
            c.kind = CHANGE_KIND::ADDED;
            c.bbox = aBBox( after );
            result.changes.push_back( std::move( c ) );
        }
        else if( before && after && aChanged( before, after ) )
        {
            c.kind = CHANGE_KIND::MODIFIED;
            c.bbox = aBBox( after );
            result.changes.push_back( std::move( c ) );
        }
    }

    if( aOptions.progress )
        aOptions.progress( 1.0 );

    return result;
}

} // namespace KICAD_DIFF

#endif // KICAD_DIFFER_H
