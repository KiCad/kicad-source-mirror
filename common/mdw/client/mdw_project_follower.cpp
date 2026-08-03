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

#include "mdw_project_follower.h"


#include <mdw/client/mdw_source.h>
#include <mdw/mdw_event_bus.h>
#include <mdw/mdw_session.h>

#include <wx/datetime.h>


namespace
{

MDW_EVENT localEvent( MDW_EVENT_TYPE aType, const wxString& aLocator )
{
    MDW_EVENT event;
    event.type = aType;
    event.source = aLocator;
    event.timestamp = wxDateTime::UNow();
    return event;
}

} // namespace


MDW_PROJECT_FOLLOWER::MDW_PROJECT_FOLLOWER( MDW_EVENT_BUS& aBus, MDW_PENDING_CHANGES& aPending ) :
        m_bus( aBus ),
        m_pending( aPending )
{
}


MDW_PROJECT_FOLLOWER::~MDW_PROJECT_FOLLOWER() = default;


std::shared_ptr<MDW_SOURCE> MDW_PROJECT_FOLLOWER::Follow( const std::shared_ptr<MDW_SESSION>& aSession )
{
    // Released after the lock, since releasing waits for a handler that may call back in here
    MDW_SUBSCRIPTION            stale;
    std::shared_ptr<MDW_SOURCE> source;

    {
        // Publishing queues under the lock so a racing Follow cannot reorder BOUND and UNBOUND
        std::lock_guard<std::mutex> lock( m_mutex );

        if( aSession == m_session.lock() && ( aSession || !m_source ) )
        {
            // A respawn or a server answering again brings a session back; consumed only once announced
            if( aSession )
            {
                bool online = aSession->Status().phase == MDW_CONNECTION::ONLINE;

                if( online && ( aSession->BackendGeneration() != m_generation || !m_online ) )
                {
                    m_generation = aSession->BackendGeneration();
                    m_bus.Publish( localEvent( MDW_EVENT_TYPE::WORKSPACE_RECONNECTED, m_locator ) );
                }

                m_online = online;
            }

            source = m_source;
        }
        else
        {
            if( m_source )
                m_bus.Publish( localEvent( MDW_EVENT_TYPE::WORKSPACE_UNBOUND, m_locator ) );

            stale = std::move( m_events );
            m_session = aSession;
            m_source.reset();
            m_locator.clear();

            if( aSession )
            {
                m_source = std::make_shared<MDW_SOURCE>( aSession );
                m_locator = aSession->SourceLocator();

                // Changes queued before the binding target left the identity are filed under the whole binding
                if( m_locator != aSession->Binding().binding )
                    m_pending.Rehome( aSession->Binding().binding, m_locator );

                m_generation = aSession->BackendGeneration();
                m_online = aSession->Status().phase == MDW_CONNECTION::ONLINE;
                m_bus.Publish( localEvent( MDW_EVENT_TYPE::WORKSPACE_BOUND, m_locator ) );

                m_events = aSession->EventBus().Subscribe(
                        [this, locator = m_locator]( const MDW_EVENT& aEvent )
                        {
                            MDW_EVENT event = aEvent;
                            event.source = locator;
                            m_bus.Publish( event );
                        } );
            }

            source = m_source;
        }
    }

    return source;
}
