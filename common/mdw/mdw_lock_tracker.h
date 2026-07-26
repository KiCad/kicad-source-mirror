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
 * or you may search the http://www.gnu.org website for the version 2 license,
 * or you may write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
 */

/**
 * @file mdw_lock_tracker.h
 * @brief The last-known per-user lock state for managed items.
 *
 * Shared across every open frame, and independent of the lifecycle category, which it never encodes.
 */

#ifndef MDW_LOCK_TRACKER_H
#define MDW_LOCK_TRACKER_H

#include <kicommon.h>
#include <mdw/client/mdw_lock.h>

struct MDW_EVENT;

#include <cstdint>
#include <map>
#include <mutex>

#include <wx/string.h>

class KICOMMON_API MDW_LOCK_TRACKER
{
public:
    void Set( const wxString& aItemId, const MDW_LOCK_INFO& aInfo );

    MDW_LOCK_INFO Get( const wxString& aItemId ) const;

    /// When aItemId last changed, so a slow query can tell its answer went stale.
    std::uint64_t Generation( const wxString& aItemId ) const;

    /// Store a queried lock only if aItemId has not changed since \a aGeneration was read.
    bool SetIfUnchanged( const wxString& aItemId, const MDW_LOCK_INFO& aInfo, std::uint64_t aGeneration );

    /// The clock to pass to ReplaceAll() for a query about to start.
    std::uint64_t Clock() const;

    /// Take aLocks as every lock there is, except on items that changed after aSince, since
    /// those changes are newer than the query that produced aLocks.
    void ReplaceAll( const std::map<wxString, MDW_LOCK_INFO>& aLocks, std::uint64_t aSince );

    /// Read every item as unknown until the next ReplaceAll(), as events missed while no one listened leave it.
    void Invalidate();

    /// Read free items as free from now on, for a backend whose events are its only report of locks.
    void TrustEvents();

    /// Fold an accepted backend lock or unlock event into the tracked state. A lock event's
    /// payload, when it parses, names the owner; otherwise the envelope's client identity does.
    void ApplyEvent( const MDW_EVENT& aEvent );

private:
    std::uint64_t generationOf( const wxString& aItemId ) const;

    mutable std::mutex                m_mutex;
    std::map<wxString, MDW_LOCK_INFO> m_locks;
    std::map<wxString, std::uint64_t> m_changed;        ///< Clock reading at each item's last change
    std::uint64_t                     m_clock = 0;
    std::uint64_t                     m_snapshot = 0;   ///< Clock reading at the last ReplaceAll(), every item's floor
    bool                              m_seeded = false; ///< A ReplaceAll() since the last Invalidate()
};

#endif // MDW_LOCK_TRACKER_H
