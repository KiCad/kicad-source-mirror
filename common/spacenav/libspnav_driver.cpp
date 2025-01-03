/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright The KiCad Developers, see CHANGELOG.txt for contributors.
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
 * or you may search the http://www.gnu.org website for the version 3 license,
 * or you may write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
 */

#include "libspnav_driver.h"

#include <cstring>
#include <mutex>

// Static member definitions
std::mutex LIBSPNAV_DRIVER::s_mutex;
int LIBSPNAV_DRIVER::s_connection_count = 0;
bool LIBSPNAV_DRIVER::s_spnav_connected = false;

LIBSPNAV_DRIVER::LIBSPNAV_DRIVER()
{
}

LIBSPNAV_DRIVER::~LIBSPNAV_DRIVER()
{
    Disconnect();
}

bool LIBSPNAV_DRIVER::Connect()
{
    std::lock_guard<std::mutex> lock( s_mutex );

    if( m_client_connected )
        return true;

    // If this is the first client, establish the connection to spacenavd
    if( s_connection_count == 0 )
    {
        if( spnav_open() == -1 )
            return false;

        s_spnav_connected = true;
    }

    s_connection_count++;
    m_client_connected = true;
    return true;
}

void LIBSPNAV_DRIVER::Disconnect()
{
    std::lock_guard<std::mutex> lock( s_mutex );

    if( !m_client_connected )
        return;

    m_client_connected = false;
    s_connection_count--;

    // Close the connection when the last client disconnects
    if( s_connection_count == 0 && s_spnav_connected )
    {
        spnav_close();
        s_spnav_connected = false;
    }
}

void LIBSPNAV_DRIVER::Poll()
{
    // Only poll if this client is connected and has a handler
    if( !m_client_connected || !m_handler )
        return;

    // Lock only for the duration of polling to prevent race conditions
    // but allow other clients to poll independently
    std::lock_guard<std::mutex> lock( s_mutex );

    if( !s_spnav_connected )
        return;

    spnav_event sev;
    while( spnav_poll_event( &sev ) > 0 )
    {
        if( sev.type == SPNAV_EVENT_MOTION )
        {
            m_handler->OnPan( sev.motion.x, sev.motion.y, sev.motion.z );
            m_handler->OnRotate( sev.motion.rx, sev.motion.ry, sev.motion.rz );
        }
        else if( sev.type == SPNAV_EVENT_BUTTON )
        {
            m_handler->OnButton( sev.button.bnum, sev.button.press != 0 );
        }
    }
}