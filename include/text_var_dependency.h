/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, you may find one here:
 * http://www.gnu.org/licenses/old-licenses/gpl-2.0.html
 */

#ifndef TEXT_VAR_DEPENDENCY_H_
#define TEXT_VAR_DEPENDENCY_H_

#include <kicommon.h>
#include <cstdint>
#include <functional>
#include <initializer_list>
#include <shared_mutex>
#include <unordered_map>
#include <unordered_set>
#include <vector>
#include <wx/string.h>


class EDA_ITEM;
class EDA_TEXT;


/**
 * Identifies a single resolvable source that a text item's `${...}` reference depends on.
 *
 * Keys are the currency of the dependency index. An edit that invalidates a key fans
 * out to every EDA_ITEM registered under that key. The encoding mirrors the shape of
 * the source token so that lexical extraction maps 1:1 to key creation with no
 * resolver context needed.
 */
struct KICOMMON_API TEXT_VAR_REF_KEY
{
    /**
     * Categorizes a reference by the source that will produce its value. The
     * listener adapter for each module routes invalidation based on KIND, so
     * expanding this taxonomy was a prerequisite for accurate fan-out
     * (see Phase 3b plan).
     *
     * - LOCAL:        `${VALUE}`, `${REFERENCE}` etc. — resolved against the
     *                 item's own context (a footprint's fields, a symbol's
     *                 fields). Changes fan out only to self.
     * - CROSS_REF:    `${REFDES:FIELD}` — primary=refdes, secondary=field.
     *                 Keyed by refdes string so renames fire on both old/new.
     * - PROJECT_VAR:  user-defined text var in project settings. Invalidated
     *                 when PROJECT::IncrementTextVarsTicker fires.
     * - ENV_VAR:      `${KICAD_FOOTPRINT_DIR}` etc. — OS/KiCad env vars.
     * - TITLE_BLOCK:  `${PROJECTNAME}`, `${REVISION}`, `${ISSUE_DATE}`, etc.
     *                 Owned by PROJECT/title block, not any item.
     * - SPECIAL:      `${SHEETNAME}`, `${SHEETPATH}`, `${FILENAME}`,
     *                 `${CURRENT_DATE}`, `${VCSHASH}`. Context-dependent
     *                 resolution; mostly non-user-editable.
     * - OP:           `${OP}`, `${OP:port}` — SPICE operating points.
     *                 Recomputed from simulation state, not edits; tracker
     *                 deliberately does not register OP-kind keys as
     *                 dependencies (no listener drives them).
     */
    enum class KIND : std::uint8_t
    {
        LOCAL,
        CROSS_REF,
        PROJECT_VAR,
        ENV_VAR,
        TITLE_BLOCK,
        SPECIAL,
        OP
    };

    KIND     kind = KIND::LOCAL;
    wxString primary;
    wxString secondary;

    bool operator==( const TEXT_VAR_REF_KEY& ) const = default;

    /**
     * Parse a raw token (the text between `${` and `}`) into a key using
     * lexical classification only — no lookup against project state or
     * item context. Tokens with an embedded `:` become CROSS_REF or OP; tokens
     * whose name matches a known built-in (PROJECTNAME, SHEETNAME, etc.) are
     * categorized into TITLE_BLOCK / SPECIAL; everything else defaults to
     * LOCAL and the adapter promotes to PROJECT_VAR or ENV_VAR based on the
     * project state at registration time.
     */
    static TEXT_VAR_REF_KEY FromToken( const wxString& aToken );

    bool IsTrackable() const { return kind != KIND::OP; }
};


/**
 * Filter @p aRefs down to the subset that should be registered in the index
 * (drops OP and any future non-trackable kinds). Shared helper so per-module
 * adapters don't each re-implement the filter.
 */
KICOMMON_API std::vector<TEXT_VAR_REF_KEY> FilterTrackable(
        const std::vector<TEXT_VAR_REF_KEY>& aRefs );


struct KICOMMON_API TEXT_VAR_REF_KEY_HASH
{
    std::size_t operator()( const TEXT_VAR_REF_KEY& aKey ) const;
};


/**
 * Bidirectional index mapping TEXT_VAR_REF_KEY → dependent items and item → keys.
 *
 * The item-keyed direction exists so Unregister is O(refs-per-item) rather than
 * having to scan the entire forward index. All mutators are thread-safe; readers
 * (ForEachDependent) take a shared lock.
 */
class KICOMMON_API TEXT_VAR_DEPENDENCY_INDEX
{
public:
    TEXT_VAR_DEPENDENCY_INDEX() = default;

    /**
     * Replace the key set for @p aItem with @p aKeys. Clears any prior
     * registration for the item first.
     */
    void Register( EDA_ITEM* aItem, const std::vector<TEXT_VAR_REF_KEY>& aKeys );

    /**
     * Remove @p aItem from the index entirely. Safe to call on an item that was
     * never registered.
     */
    void Unregister( EDA_ITEM* aItem );

    /**
     * Invoke @p aFn exactly once per item registered under @p aKey.
     *
     * The callback runs while the shared lock is held, so it must not call
     * Register/Unregister (which would deadlock on the same mutex).
     */
    void ForEachDependent( const TEXT_VAR_REF_KEY& aKey,
                           const std::function<void( EDA_ITEM* )>& aFn ) const;

    /**
     * Drop every entry. Intended for teardown of the owning BOARD/SCHEMATIC.
     */
    void Clear();

    std::size_t DependentCount( const TEXT_VAR_REF_KEY& aKey ) const;
    std::size_t ItemCount() const;

    /**
     * Enumerate every key currently present in the forward index. The snapshot
     * is taken under the shared lock and returned by value, so callers may
     * safely mutate the index while iterating (e.g., fire an InvalidateKey on
     * each returned key).
     */
    std::vector<TEXT_VAR_REF_KEY> GetRegisteredKeys() const;

private:
    mutable std::shared_mutex m_mutex;

    std::unordered_map<TEXT_VAR_REF_KEY,
                       std::unordered_set<EDA_ITEM*>,
                       TEXT_VAR_REF_KEY_HASH> m_dependents;

    std::unordered_map<EDA_ITEM*, std::vector<TEXT_VAR_REF_KEY>> m_itemKeys;
};


/**
 * Coordinates the dependency index with change notifications.
 *
 * The tracker is the generic core of the reactive text-variable system. It owns a
 * TEXT_VAR_DEPENDENCY_INDEX and exposes a uniform entry point (OnItems*) for
 * per-module listener adapters. Module-specific logic (which fields on a
 * SCH_SYMBOL or FOOTPRINT can be sourced via `${REFDES:FIELD}`) is plugged in
 * via the SourceKeyExtractor functor, keeping this class free of board/schematic
 * dependencies.
 *
 * Invalidation is delivered via the consumer-supplied callback so the tracker
 * does not have to know how each kind of dependent (EDA_TEXT glyph cache, title
 * block resolved string, dialog row) wants to refresh itself.
 */
class KICOMMON_API TEXT_VAR_TRACKER
{
public:
    /**
     * Return the keys @p aItem could source as a cross-reference target.
     *
     * For a footprint U1 with fields REFERENCE/VALUE/MPN this returns keys for
     * `${U1:VALUE}`, `${U1:MPN}`, etc. Returns empty for items that cannot be
     * cross-reference sources.
     */
    using SourceKeyExtractor = std::function<std::vector<TEXT_VAR_REF_KEY>( EDA_ITEM* )>;

    /**
     * Called when a dependent item should recompute its resolved text. The
     * callback runs on the thread that drove the originating edit (the
     * BOARD/SCHEMATIC main thread for commits and undo/redo).
     */
    using InvalidateCallback = std::function<void( EDA_ITEM* aDependent,
                                                   const TEXT_VAR_REF_KEY& aKey )>;

    /**
     * Opaque handle returned by AddInvalidateListener and consumed by
     * RemoveInvalidateListener. A default-constructed handle is inert.
     */
    using ListenerHandle = std::size_t;
    static constexpr ListenerHandle INVALID_LISTENER = 0;

    explicit TEXT_VAR_TRACKER( SourceKeyExtractor aSourceKeyExtractor = {} );

    TEXT_VAR_DEPENDENCY_INDEX&       Index()       { return m_index; }
    const TEXT_VAR_DEPENDENCY_INDEX& Index() const { return m_index; }

    void SetSourceKeyExtractor( SourceKeyExtractor aExtractor );

    /**
     * Register a listener that fires for every invalidation. Multiple
     * listeners may coexist — each consumer (frame, drawing-sheet proxy,
     * dialog) adds its own. The returned handle must be passed to
     * RemoveInvalidateListener when the consumer is destroyed, otherwise the
     * lambda's captured state (often `this`) outlives its owner and becomes
     * a dangling reference on the next invalidation.
     */
    [[nodiscard]] ListenerHandle AddInvalidateListener( InvalidateCallback aCallback );

    /**
     * Drop a previously-registered listener. Safe to call with
     * INVALID_LISTENER or a handle that was already removed.
     */
    void RemoveInvalidateListener( ListenerHandle aHandle );

    /**
     * Register @p aItem in the index. Caller has already extracted @p aKeys
     * (typically via EDA_TEXT::GetTextVarReferences). An empty key vector
     * unregisters. This entry point does no RTTI — safe to call across DSO
     * boundaries.
     */
    void RegisterItem( EDA_ITEM* aItem, const std::vector<TEXT_VAR_REF_KEY>& aKeys );

    /**
     * Remove @p aItem from the index. No-op if not registered.
     */
    void UnregisterItem( EDA_ITEM* aItem );

    /**
     * Handle a composite change for @p aItem: register its updated keys and
     * fan out invalidation for every key it could source. Either step is
     * skipped if the corresponding hook isn't configured.
     */
    void HandleItemChanged( EDA_ITEM* aItem, const std::vector<TEXT_VAR_REF_KEY>& aUpdatedKeys );

    /**
     * Fan out invalidation for a single explicit key — used when a non-item
     * source changes (e.g., project text variables, env vars, title block fields).
     */
    void InvalidateKey( const TEXT_VAR_REF_KEY& aKey );

    /**
     * Fan out invalidation for every registered key whose KIND is one of the
     * supplied set. Used by project-settings / text-var / env-var change
     * handlers that don't know precisely which tokens moved but want to
     * bulk-refresh every project-scoped dependent.
     */
    void InvalidateByKind( std::initializer_list<TEXT_VAR_REF_KEY::KIND> aKinds );

    /**
     * Invalidate every non-item source: title-block, special, project/env
     * vars, and LOCAL (which may be a project text var that we couldn't
     * classify at lex time). Excludes CROSS_REF (driven by per-item commit
     * events) and OP (not tracked). Called by frames after
     * `PROJECT::IncrementTextVarsTicker()`.
     */
    void InvalidateProjectScoped();

    /**
     * Invalidate every source whose resolved value can differ between
     * variants: CROSS_REF (footprint/symbol field values with variant
     * overrides) and LOCAL (a symbol's own variant-overridden VALUE/MPN/etc.).
     * Called after `BOARD::SetCurrentVariant` / `SCHEMATIC::SetCurrentVariant`.
     */
    void InvalidateVariantScoped();

    void Clear() { m_index.Clear(); }

private:
    void fanOutSourceKeys( EDA_ITEM* aItem );

    TEXT_VAR_DEPENDENCY_INDEX                              m_index;
    SourceKeyExtractor                                     m_extractSourceKeys;

    // m_invalidateListeners is read from InvalidateKey (possibly re-entrantly
    // via a callback that triggers another invalidation) and mutated by
    // Add/RemoveInvalidateListener. A shared_mutex protects both paths and
    // is held only long enough to snapshot the callbacks before invoking
    // them unlocked.
    mutable std::shared_mutex                              m_listenersMutex;
    std::unordered_map<ListenerHandle, InvalidateCallback> m_invalidateListeners;
    ListenerHandle                                         m_nextListenerHandle = 1;
};


#endif // TEXT_VAR_DEPENDENCY_H_
