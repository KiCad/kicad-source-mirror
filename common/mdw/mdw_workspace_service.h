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
 * @file mdw_workspace_service.h
 * @brief The session bookkeeping around a backend process that MDW_SOURCE operations share.
 *
 * Tracker updates, local events and client identity are applied here and nowhere else.
 * The service shares ownership of its backend, bus and trackers, so one held across a backend restart
 * fails its calls against the stopped transport rather than crashing.
 *
 * A mutating operation's id is the request's idempotency_id when the caller gave one, so a retry reuses
 * it, and the same id pairs the local event with the backend's echo.
 */

#ifndef MDW_WORKSPACE_SERVICE_H
#define MDW_WORKSPACE_SERVICE_H

#include <kicommon.h>
#include "mdw_backend.h"
#include "mdw_event_bus.h"
#include "mdw_lock_tracker.h"
#include "mdw_dirty_tracker.h"
#include <mdw/client/mdw_lock.h>

#include <functional>
#include <memory>

#include <wx/string.h>

class KICOMMON_API MDW_WORKSPACE_SERVICE
{
public:
    MDW_WORKSPACE_SERVICE( std::shared_ptr<MDW_BACKEND> aBackend, std::shared_ptr<MDW_EVENT_BUS> aBus,
                           std::shared_ptr<MDW_LOCK_TRACKER> aLocks, std::shared_ptr<MDW_DIRTY_TRACKER> aDirty,
                           const wxString& aWorkspaceId, const wxString& aClientIdentity );

    /// True when the bound backend advertises the group an operation needs.
    bool Supports( MDW_CAPABILITY_GROUP aGroup ) const;

    /// Refresh the lock tracker with every lock the backend knows and announce LOCKS_CHANGED. Without
    /// CG_LOCKING or the request it answers NOT_SUPPORTED, leaves the tracker trusting events and still announces.
    /// Blocks until the backend replies.
    MDW_RESULT ListLocks();

    /// The backend this service talks to, so a session can tell a stale service apart.
    const MDW_BACKEND* BackendPtr() const { return m_backend.get(); }

    MDW_EVENT_BUS&     EventBus() { return *m_bus; }
    MDW_LOCK_TRACKER&  LockTracker() { return *m_locks; }
    MDW_DIRTY_TRACKER& DirtyTracker() { return *m_dirty; }

    /*
     * The bookkeeping around a request, for the client ops that send the request themselves. Safe from any thread.
     */

    /// The backend the bookkeeping belongs with, so a request is sent where its echo will come from.
    std::shared_ptr<MDW_BACKEND> Backend() const { return m_backend; }

    /// The caller's idempotency id, or a fresh one, announced to the bus as a pending echo.
    wxString BeginOperation( const wxString& aIdempotencyId );

    /// Emit a local event and update the lock tracker for a lock change.
    void ApplyLock( const wxString& aItemId, const MDW_LOCK_INFO& aLock, MDW_EVENT_TYPE aType,
                    const wxString& aOperationId );

    /// Publish the local event for a completed operation, applying aApply only when it is accepted.
    void Emit( MDW_EVENT_TYPE aType, const wxString& aItemId, const wxString& aOperationId,
               const std::function<void()>& aApply = nullptr );

    /// Settle an operation that failed with aCode. Unless the backend may still have done it, its
    /// echo will never come.
    void AbandonOperation( const wxString& aOperationId, MDW_ERROR_CODE aCode );

private:
    std::shared_ptr<MDW_BACKEND>       m_backend;
    std::shared_ptr<MDW_EVENT_BUS>     m_bus;
    std::shared_ptr<MDW_LOCK_TRACKER>  m_locks;
    std::shared_ptr<MDW_DIRTY_TRACKER> m_dirty;
    wxString                           m_workspaceId;
    wxString                           m_clientIdentity;
};

#endif // MDW_WORKSPACE_SERVICE_H
