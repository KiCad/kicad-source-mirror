/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2026 KiCad Developers, see AUTHORS.txt for contributors.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation, either version 3 of the License, or (at your
 * option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

/**
 * @file mdw_event_publisher.h
 * @brief The backend-side event channel client.
 *
 * Events go over a REQ socket that the client acknowledges per message. Delivery is reliable because lock and
 * lifecycle events are authoritative tracker inputs, so an unacknowledged event is retried and the client drops
 * duplicates by sequence number.
 */

#ifndef MDW_EVENT_PUBLISHER_H
#define MDW_EVENT_PUBLISHER_H

#include <api/workspace/workspace_types.pb.h>

#include <atomic>
#include <condition_variable>
#include <deque>
#include <memory>
#include <mutex>
#include <string>
#include <thread>

class MDW_EVENT_PUBLISHER
{
public:
    MDW_EVENT_PUBLISHER( const std::string& aChannelToken );

    ~MDW_EVENT_PUBLISHER();

    bool Start( const std::string& aEventsUrl );

    void Stop();

    /**
     * Queue an event for delivery. False if not running, or if the event cannot be serialized or would exceed
     * MDW_CONTROL_MAX_BYTES, which the client refuses unread.
     *
     * The publisher replaces any seq in aEvent, because the client drops an event numbered below one it already
     * saw and only the queue knows the send order. Each workspace_id is numbered on its own, since a gap reads as
     * a lost event.
     *
     * aOperationId is the idempotency_id of the request that caused the event, or empty. The client suppresses
     * an event as its own echo only when it matches a request it sent, so a teammate's change is never mistaken
     * for the user's.
     *
     * An ET_PROGRESS event is advisory and needs aOperationId. It carries no sequence number and replaces the
     * operation's waiting report of the same phase, so a slow client's backlog is one report per phase. A failed
     * send is tried once more unless a newer one of its phase is waiting, so a report can arrive twice.
     */
    bool Publish( const kiapi::workspace::EventEnvelope& aEvent, const std::string& aOperationId = std::string() );

private:
    bool publishProgress( const kiapi::workspace::EventEnvelope& aEvent, const std::string& aOperationId );

    void publishLoop();

    struct IMPL;

    std::string           m_channelToken;
    std::unique_ptr<IMPL> m_impl;
};

#endif // MDW_EVENT_PUBLISHER_H
