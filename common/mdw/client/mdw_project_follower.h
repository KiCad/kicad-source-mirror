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
 * @file mdw_project_follower.h
 * Follows the project's session onto the client bus. Client implementations only.
 */

#ifndef MDW_PROJECT_FOLLOWER_H
#define MDW_PROJECT_FOLLOWER_H

#include <kicommon.h>
#include <mdw/client/mdw_pending_changes.h>
#include <mdw/client/mdw_subscription.h>

#include <cstdint>
#include <memory>
#include <mutex>

#include <wx/string.h>

class MDW_EVENT_BUS;
class MDW_SESSION;
class MDW_SOURCE;

/**
 * Forwards the followed session's events to a bus with their source filled in, and announces on that bus
 * WORKSPACE_BOUND when a session is followed, WORKSPACE_UNBOUND when it goes, and WORKSPACE_RECONNECTED when
 * its backend comes back from a respawn or its server answers again. The announcements are local and
 * carry no sequence number.
 */
class KICOMMON_API MDW_PROJECT_FOLLOWER
{
public:
    /// aPending receives the changes a followed session queued under its whole binding
    explicit MDW_PROJECT_FOLLOWER( MDW_EVENT_BUS& aBus, MDW_PENDING_CHANGES& aPending = MDW_PENDING_CHANGES::Get() );

    ~MDW_PROJECT_FOLLOWER();

    /// The source for aSession, or null without one. Announces what changed since the last call
    std::shared_ptr<MDW_SOURCE> Follow( const std::shared_ptr<MDW_SESSION>& aSession );

private:
    MDW_EVENT_BUS&              m_bus;
    MDW_PENDING_CHANGES&        m_pending;
    std::mutex                  m_mutex;
    std::weak_ptr<MDW_SESSION>  m_session;
    std::shared_ptr<MDW_SOURCE> m_source;
    MDW_SUBSCRIPTION            m_events;
    wxString                    m_locator;
    std::uint64_t               m_generation = 0;
    bool                        m_online = false;
};

#endif // MDW_PROJECT_FOLLOWER_H
