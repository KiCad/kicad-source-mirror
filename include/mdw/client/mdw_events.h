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
 * @file mdw_events.h
 * @brief Workspace event vocabulary consumed by the in-process event bus.
 *
 * The bus is fed by local operations and backend pushes. seq is monotonic per workspace so a
 * subscriber detects gaps and resyncs.
 */

#ifndef MDW_EVENTS_H
#define MDW_EVENTS_H

#include <wx/string.h>
#include <wx/datetime.h>
#include <cstdint>

/// Workspace state transitions. Numbering matches kiapi.workspace.EventType
enum class MDW_EVENT_TYPE
{
    UNKNOWN                    = 0,
    WORKSPACE_BOUND            = 1,
    WORKSPACE_UNBOUND          = 2,
    WORKSPACE_RECONNECTED      = 3,
    ITEM_LOCKED                = 4,
    ITEM_UNLOCKED              = 5,
    REVISION_CHECKED_IN        = 6,
    REVISION_CHECKED_OUT       = 7,
    LIFECYCLE_CATEGORY_CHANGED = 8,
    BACKEND_EVENT              = 10,
    REVISION_RELEASED          = 11,    ///< A release or baseline was applied
    PENDING_RESOLVED           = 12,    ///< A pending change was decided; change_id names it
    CAPABILITIES_CHANGED       = 13,    ///< Re-read capabilities and schema
    LIBRARY_CHANGED            = 14,    ///< A shared library has upstream changes; item_id is its library_id,
                                        ///< or empty when any of the source's may have changed
    PROGRESS                   = 15,    ///< A running request advanced; the session routes it and never publishes it
    LOCKS_CHANGED              = 16     ///< Every lock was read again; re-read the lock tracker
};


struct MDW_EVENT
{
    MDW_EVENT_TYPE type = MDW_EVENT_TYPE::UNKNOWN;
    wxString       workspace_id;
    wxString       item_id;
    wxString       path_hint;              ///< Display text for item_id, empty when the backend gave none
    wxString       client_identity;
    wxString       payload;                ///< JSON; framework treats as opaque
    wxString       backend_event_type;     ///< Set only for BACKEND_EVENT
    wxString       operation_id;           ///< Idempotency id of the request that caused it
    wxString       change_id;              ///< Set only for PENDING_RESOLVED
    wxString       source;                 ///< The source's locator, filled by the client
    std::uint64_t  progress_done = 0;      ///< Set only for PROGRESS
    std::uint64_t  progress_total = 0;     ///< Set only for PROGRESS, 0 while unknown
    wxString       progress_phase;         ///< Set only for PROGRESS
    bool           from_this_client = false; ///< Set by the bus for this client's own operations
    std::uint64_t  seq = 0;
    wxDateTime     timestamp;
};


#endif // MDW_EVENTS_H
