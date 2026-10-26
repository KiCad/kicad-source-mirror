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

#include <algorithm>
#include <functional>
#include <set>

class PROGRESS_REPORTER;


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

    /**
     * One loop's share of the diff's progress, from @p aFrom to @p aTo over @p aSteps steps. Reports every few steps,
     * so a cancel acts within a moment even when each step is slow, as in a debug build.
     */
    class PROGRESS_SPAN
    {
    public:
        PROGRESS_SPAN( const OPTIONS& aOptions, double aFrom, double aTo, size_t aSteps ) :
                m_progress( aOptions.progress ),
                m_from( aFrom ),
                m_to( aTo ),
                m_steps( std::max<size_t>( aSteps, 1 ) )
        {
        }

        void Step()
        {
            if( m_progress && ++m_done % STRIDE == 0 )
                report();
        }

        /// Reports from within a step with many parts, such as a footprint's pads, without moving the bar on
        void Tick()
        {
            if( m_progress && ++m_ticks % STRIDE == 0 )
                report();
        }

        /// Tick() as a callback, or none without a progress reporter so a loop of millions skips the call
        std::function<void()> Ticker()
        {
            if( !m_progress )
                return {};

            return [this]()
            {
                Tick();
            };
        }

    private:
        void report() { m_progress( m_from + ( m_to - m_from ) * double( m_done ) / double( m_steps ) ); }

        static constexpr size_t STRIDE = 64;

        const std::function<void( double )>& m_progress;
        double                               m_from;
        double                               m_to;
        size_t                               m_steps;
        size_t                               m_done = 0;
        size_t                               m_ticks = 0;
    };

protected:
    KICAD_DIFFER() = default;

    /// These options with progress mapped onto @p aFrom to @p aTo, for a pass that reports through a shared routine.
    OPTIONS scaledOptions( double aFrom, double aTo ) const
    {
        OPTIONS options = m_options;

        if( m_options.progress )
        {
            options.progress = [progress = m_options.progress, aFrom, aTo]( double aFraction )
            {
                progress( aFrom + ( aTo - aFrom ) * aFraction );
            };
        }

        return options;
    }

    OPTIONS m_options;
};


/**
 * Start a "Comparing" phase on @p aProgress and advance it as @p aDiffer runs, so a diff on the UI
 * thread keeps the window responding. Once the user cancels, the differ's Diff() throws IO_CANCELLED.
 * Does nothing without a reporter.
 */
KICOMMON_API void ReportDiffProgress( KICAD_DIFFER& aDiffer, PROGRESS_REPORTER* aProgress );


/**
 * Refresh @p aProgress while one side of a comparison loads, and throw IO_CANCELLED once the user cancels.
 * Does nothing without a reporter.
 */
KICOMMON_API void KeepLoading( PROGRESS_REPORTER* aProgress );


/**
 * The progress message while one side of a comparison loads. Names the file and @p aLabel, the side's version,
 * but not the folder, which for a downloaded version is a temporary one that names the user.
 */
KICOMMON_API wxString LoadingMessage( const wxString& aFile, const wxString& aLabel );


/**
 * Shared name-keyed library diff used by FP_LIB_DIFFER and SYM_LIB_DIFFER.
 *
 * Walks the union of names in @p aBefore and @p aAfter, classifying each entry
 * as ADDED, REMOVED, or MODIFIED. @p aBBox yields the bounding box stored on
 * each change; @p aChanged decides whether two present items differ, calling the tick
 * it is given through long work so a cancel is noticed. Library
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

    KICAD_DIFFER::PROGRESS_SPAN naming( aOptions, 0.1, 1.0, allNames.size() );
    const std::function<void()> tick = naming.Ticker();

    for( const wxString& name : allNames )
    {
        naming.Step();

        auto        beforeIt = aBefore.find( name );
        auto        afterIt  = aAfter.find( name );
        const auto* before   = beforeIt == aBefore.end() ? nullptr : beforeIt->second;
        const auto* after    = afterIt == aAfter.end() ? nullptr : afterIt->second;

        ITEM_CHANGE c;
        c.id        = LibraryItemKiidPath( name );
        c.typeName  = aTypeName;
        c.typeLabel = ( after ? after : before )->GetFriendlyName();
        c.refdes    = name;

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
        else if( before && after && aChanged( before, after, tick ) )
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
