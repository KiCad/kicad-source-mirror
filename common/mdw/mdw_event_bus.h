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
 * @file mdw_event_bus.h
 * @brief The in-process fan-out for workspace events.
 *
 * The bus carries local optimistic events and the backend's authoritative stream. Of an operation's local and
 * backend copies, whichever arrives first is delivered and the other dropped. Events without an operation id are
 * never suppressed, since a duplicate refresh is harmless and a lost change from another user is not.
 *
 * Handlers run on the bus's delivery thread one event at a time in accepted order, so UI subscribers must marshal
 * to the main thread. A publish returns once the event is accepted and its state applied, before any handler runs,
 * so a handler may start an operation that publishes and wait for it.
 */

#ifndef MDW_EVENT_BUS_H
#define MDW_EVENT_BUS_H

#include <kicommon.h>
#include <mdw/client/mdw_events.h>
#include <mdw/client/mdw_subscription.h>

#include <cstdint>
#include <functional>
#include <memory>


class KICOMMON_API MDW_EVENT_BUS
{
public:
    using HANDLER = std::function<void( const MDW_EVENT& )>;
    using ACCEPT_HOOK = std::function<void( const MDW_EVENT& )>;

    MDW_EVENT_BUS();
    ~MDW_EVENT_BUS();

    MDW_EVENT_BUS( const MDW_EVENT_BUS& ) = delete;
    MDW_EVENT_BUS& operator=( const MDW_EVENT_BUS& ) = delete;

    [[nodiscard]] MDW_SUBSCRIPTION Subscribe( HANDLER aHandler );

    /**
     * Announce an operation before its request is sent, so a backend event for it
     * that overtakes the reply is recognised. Pair with PublishLocal() on success
     * or CancelEcho() on failure.
     */
    void ExpectEcho( const wxString& aOperationId );

    void CancelEcho( const wxString& aOperationId );

    /**
     * Emit a local optimistic event. With an operation id announced through
     * ExpectEcho(), the event is skipped when the backend's copy of it
     * was already delivered and otherwise suppresses that copy when it arrives. One
     * operation may publish several items and types, and each absorbs its own echo.
     *
     * @param aApply folds the reply's state into local trackers. It runs only when the
     *               event is accepted, so a late reply never overwrites newer backend state.
     */
    void PublishLocal( const MDW_EVENT& aEvent, const std::function<void()>& aApply = nullptr );

    /**
     * Ingest an event from the backend stream. A re-delivered sequence number is
     * dropped. A gap in the sequence also dispatches WORKSPACE_RECONNECTED so
     * subscribers re-read state the missing events would have changed.
     *
     * @return false for a re-delivered event, which nobody sees.
     */
    bool PublishFromBackend( const MDW_EVENT& aEvent );

    /// Deliver an event as it is, with no echo or sequence handling, for a bus that only fans out.
    void Publish( const MDW_EVENT& aEvent );

    /**
     * Called for every backend event the bus accepts, including an echo it then
     * suppresses, before any subscriber sees it. The session folds authoritative
     * lock state in here so subscribers never read a stale tracker.
     */
    void SetAcceptHook( ACCEPT_HOOK aHook );

    /// Forget sequence numbers and pending echoes, for a replacement backend process.
    void ResetBackendState();

private:
    using STATE = MDW_SUBSCRIPTION::STATE;

    static void deliverQueued( STATE& aState );

    static void dispatch( const std::shared_ptr<STATE>& aState, const MDW_EVENT& aEvent );

    std::shared_ptr<STATE> m_state;
};

#endif // MDW_EVENT_BUS_H
