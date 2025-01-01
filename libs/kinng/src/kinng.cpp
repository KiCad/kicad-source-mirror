/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2023 Jon Evans <jon@craftyjon.com>
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
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
 * You should have received a copy of the GNU General Public License along
 * with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <kinng.h>
#include <nng/nng.h>
#include <nng/protocol/reqrep0/rep.h>
#include <wx/log.h>


/**
 * Trace nng server debug output
 * @ingroup trace_env_vars
 */
static const wxChar TraceNng[] = wxT( "KINNG" );


KINNG_REQUEST_SERVER::KINNG_REQUEST_SERVER( const std::string& aSocketUrl ) :
        m_socketUrl( aSocketUrl ),
        m_callback()
{
    Start();
}


KINNG_REQUEST_SERVER::~KINNG_REQUEST_SERVER()
{
    Stop();
}


bool KINNG_REQUEST_SERVER::Running() const
{
    return m_thread.joinable();
}


bool KINNG_REQUEST_SERVER::Start()
{
    m_shutdown.store( false );
    m_thread = std::thread( [&]() { listenThread(); } );
    return true;
}


void KINNG_REQUEST_SERVER::Stop()
{
    if( !m_thread.joinable() )
        return;

    {
        std::lock_guard<std::mutex> lock( m_mutex );
        m_replyReady.notify_all();
    }

    m_shutdown.store( true );
    m_thread.join();
}


void KINNG_REQUEST_SERVER::Reply( const std::string& aReply )
{
    std::lock_guard<std::mutex> lock( m_mutex );
    m_pendingReply = aReply;
    m_replyReady.notify_all();
}


void KINNG_REQUEST_SERVER::listenThread()
{
    nng_socket   socket;
    nng_listener listener;
    int          retCode = 0;

    wxLogTrace( TraceNng, wxS( "KINNG_REQUEST_SERVER starting" ) );

    retCode = nng_rep0_open( &socket );

    if( retCode != 0 )
    {
        wxLogTrace( TraceNng,
                    wxString::Format( wxS( "Got error code %d from nng_rep0_open!" ), retCode ) );
        return;
    }

    retCode = nng_listener_create( &listener, socket, m_socketUrl.c_str() );

    if( retCode != 0 )
    {
        wxLogTrace( TraceNng,
                    wxString::Format( wxS( "Got error code %d from nng_listener_create!" ),
                                      retCode ) );
        return;
    }

    nng_socket_set_ms( socket, NNG_OPT_RECVTIMEO, 500 );

    nng_listener_start( listener, 0 );

    wxLogTrace( TraceNng, wxS( "KINNG_REQUEST_SERVER listener has started" ) );

    while( !m_shutdown.load() )
    {
        char*    buf = nullptr;
        size_t   sz;
        uint64_t val;

        retCode = nng_recv( socket, &buf, &sz, NNG_FLAG_ALLOC );

        if( retCode == NNG_ETIMEDOUT )
            continue;

        if( retCode != 0 )
        {
            nng_free( buf, sz );
            wxLogTrace( TraceNng,
                        wxString::Format( wxS( "Got error code %d from nngc_recv!" ), retCode ) );
            break;
        }

        m_sharedMessage.assign( buf, sz );

        if( m_callback )
            m_callback( &m_sharedMessage );

        std::unique_lock<std::mutex> lock( m_mutex );
        m_replyReady.wait( lock, [&]() { return !m_pendingReply.empty(); } );

        retCode = nng_send( socket, const_cast<std::string::value_type*>( m_pendingReply.c_str() ),
                            m_pendingReply.length(), 0 );

        if( retCode != 0 )
        {
            wxLogTrace( TraceNng,
                        wxString::Format( wxS( "Got error code %d from nng_send!" ), retCode ) );
        }

        m_pendingReply.clear();
    }

    wxLogTrace( TraceNng, wxS( "KINNG_REQUEST_SERVER shutting down" ) );

    nng_close( socket );
}
