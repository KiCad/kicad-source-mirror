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
 * @file mdw_event_ingress.h
 * @brief Receives a backend's pushed workspace events and puts them on the bus.
 *
 * A backend pushes each event over a REQ socket and waits for the PublishEventAck, because lock and lifecycle
 * events are authoritative tracker inputs. The sink runs on the server's thread and must outlive this object.
 */

#ifndef MDW_EVENT_INGRESS_H
#define MDW_EVENT_INGRESS_H

#include <kicommon.h>

#include <functional>
#include <memory>
#include <string>

#include <wx/string.h>

class KINNG_REQUEST_SERVER;
struct MDW_EVENT;


class KICOMMON_API MDW_EVENT_INGRESS
{
public:
    using SINK = std::function<void( const MDW_EVENT& )>;

    /**
     * Forward acknowledged events to aSink. aChannelToken is the per-session secret the
     * supervisor gave the backend; a message that does not carry it gets an empty reply and is
     * dropped, so a stray sender cannot inject workspace state.
     */
    MDW_EVENT_INGRESS( SINK aSink, const wxString& aChannelToken );

    ~MDW_EVENT_INGRESS();

    /// Bind the socket. Returns false when the address is unavailable.
    bool Start( const wxString& aUrl );

    void Stop();

private:
    void onMessage( std::string* aMessage );

    SINK                                  m_sink;
    std::string                          m_channelToken;
    std::unique_ptr<KINNG_REQUEST_SERVER> m_server;
};

#endif // MDW_EVENT_INGRESS_H
