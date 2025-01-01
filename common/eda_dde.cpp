/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2014 Jean-Pierre Charras, jp.charras at wanadoo.fr
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

#include <condition_variable>
#include <mutex>
#include <thread>

#include <eda_dde.h>
#include <kiway_player.h>
#include <id.h>

#include <wx/crt.h>


static const wxString HOSTNAME( wxT( "localhost" ) );

// buffer for read and write data in socket connections
#define IPC_BUF_SIZE 4096
static char client_ipc_buffer[IPC_BUF_SIZE];


void KIWAY_PLAYER::CreateServer( int service, bool local )
{
    wxIPV4address addr;

    // Set the port number
    addr.Service( service );

    // Listen on localhost only if requested
    if( local )
        addr.Hostname( HOSTNAME );

    if( m_socketServer )
    {
        // this helps prevent any events that could come in during deletion
        m_socketServer->Notify( false );
        delete m_socketServer;
    }

    m_socketServer = new wxSocketServer( addr );

    m_socketServer->SetNotify( wxSOCKET_CONNECTION_FLAG );
    m_socketServer->SetEventHandler( *this, ID_EDA_SOCKET_EVENT_SERV );
    m_socketServer->Notify( true );
}


void KIWAY_PLAYER::OnSockRequest( wxSocketEvent& evt )
{
    size_t        len;
    wxSocketBase* sock = evt.GetSocket();

    switch( evt.GetSocketEvent() )
    {
    case wxSOCKET_INPUT:
        sock->Read( client_ipc_buffer, 1 );

        if( sock->LastCount() == 0 )
            break;                    // No data, occurs on opening connection

        sock->Read( client_ipc_buffer + 1, IPC_BUF_SIZE - 2 );
        len = 1 + sock->LastCount();
        client_ipc_buffer[len] = 0;
        ExecuteRemoteCommand( client_ipc_buffer );
        break;

    case wxSOCKET_LOST:
        return;
        break;

    default:
        wxPrintf( wxT( "EDA_DRAW_FRAME::OnSockRequest() error: Invalid event !" ) );
        break;
    }
}


void KIWAY_PLAYER::OnSockRequestServer( wxSocketEvent& evt )
{
    wxSocketBase*   socket;
    wxSocketServer* server = (wxSocketServer*) evt.GetSocket();

    socket = server->Accept();

    if( socket == nullptr )
        return;

    m_sockets.push_back( socket );

    socket->Notify( true );
    socket->SetEventHandler( *this, ID_EDA_SOCKET_EVENT );
    socket->SetNotify( wxSOCKET_INPUT_FLAG | wxSOCKET_LOST_FLAG );
}


/**
 * Spin up a thread to send messages via a socket.
 *
 * No message queuing, if a message is in flight when another is posted with Send(), the
 * second is just dropped.  This is a workaround for "non-blocking" sockets not always being
 * non-blocking, especially on Windows.  It is kept fairly simple and not exposed to the
 * outside world because it should be replaced in a future KiCad version with a real message
 * queue of some sort, and unified with the Kiway messaging system.
 */
class ASYNC_SOCKET_HOLDER
{
public:
    ASYNC_SOCKET_HOLDER() :
            m_messageReady( false ),
            m_shutdown( false )
    {
        // Do a dummy Connect so that wx will set up the socket stuff on the main thread, which is
        // required even if you later make socket connections on another thread.
        wxSocketClient* client = new wxSocketClient;
        wxIPV4address   addr;

        addr.Hostname( HOSTNAME );
        addr.Service( KICAD_PCB_PORT_SERVICE_NUMBER );

        client->Connect( addr, false );

        client->Close();
        client->Destroy();

        m_thread = std::thread( &ASYNC_SOCKET_HOLDER::worker, this );
    }

    ~ASYNC_SOCKET_HOLDER()
    {
        {
            std::lock_guard<std::mutex> lock( m_mutex );
            m_shutdown = true;
        }

        m_cv.notify_one();

        try
        {
            if( m_thread.joinable() )
                m_thread.join();
        }
        catch( ... )
        {
        }
    }

    /**
     * Attempt to send a message if the thread is available.
     *
     * @param aService is the port number (i.e. service) to send to.
     * @param aMessage is the message to send.
     * @return true if the message was queued.
    */
    bool Send( int aService, const std::string& aMessage )
    {
        if( m_messageReady )
            return false;

        std::lock_guard<std::mutex> lock( m_mutex );

        m_message      = std::make_pair( aService, aMessage );
        m_messageReady = true;
        m_cv.notify_one();

        return true;
    }

private:
    /**
     * Actual task that sends data to the socket server
     */
    void worker()
    {
        int         port;
        std::string message;

        std::unique_lock<std::mutex> lock( m_mutex );

        while( !m_shutdown )
        {
            m_cv.wait( lock, [&]() { return m_messageReady || m_shutdown; } );

            if( m_shutdown )
                break;

            port    = m_message.first;
            message = m_message.second;

            lock.unlock();

            wxSocketClient* sock_client;
            wxIPV4address   addr;

            // Create a connection
            addr.Hostname( HOSTNAME );
            addr.Service( port );

            // Mini-tutorial for Connect() :-)
            // (JP CHARRAS Note: see wxWidgets: sockets/client.cpp sample)
            // ---------------------------
            //
            // There are two ways to use Connect(): blocking and non-blocking,
            // depending on the value passed as the 'wait' (2nd) parameter.
            //
            // Connect(addr, true) will wait until the connection completes,
            // returning true on success and false on failure. This call blocks
            // the GUI (this might be changed in future releases to honor the
            // wxSOCKET_BLOCK flag).
            //
            // Connect(addr, false) will issue a nonblocking connection request
            // and return immediately. If the return value is true, then the
            // connection has been already successfully established. If it is
            // false, you must wait for the request to complete, either with
            // WaitOnConnect() or by watching wxSOCKET_CONNECTION / LOST
            // events (please read the documentation).
            //
            // WaitOnConnect() itself never blocks the GUI (this might change
            // in the future to honor the wxSOCKET_BLOCK flag). This call will
            // return false on timeout, or true if the connection request
            // completes, which in turn might mean:
            //
            //   a) That the connection was successfully established
            //   b) That the connection request failed (for example, because
            //      it was refused by the peer.
            //
            // Use IsConnected() to distinguish between these two.
            //
            // So, in a brief, you should do one of the following things:
            //
            // For blocking Connect:
            //
            //   bool success = client->Connect(addr, true);
            //
            // For nonblocking Connect:
            //
            //   client->Connect(addr, false);
            //
            //   bool waitmore = true;
            //   while (! client->WaitOnConnect(seconds, millis) && waitmore )
            //   {
            //     // possibly give some feedback to the user,
            //     // update waitmore if needed.
            //   }
            //   bool success = client->IsConnected();
            //
            // And that's all :-)

            sock_client = new wxSocketClient( wxSOCKET_BLOCK );
            sock_client->SetTimeout( 1 ); // Time out in Seconds
            sock_client->Connect( addr, false );
            sock_client->WaitOnConnect( 0, 250 );

            if( sock_client->Ok() && sock_client->IsConnected() )
            {
                sock_client->SetFlags( wxSOCKET_NOWAIT /*wxSOCKET_WAITALL*/ );
                sock_client->Write( message.c_str(), message.length() );
            }

            sock_client->Close();
            sock_client->Destroy();

            m_messageReady = false;

            lock.lock();
        }
    }

    std::thread                 m_thread;
    std::pair<int, std::string> m_message;
    bool                        m_messageReady;
    mutable std::mutex          m_mutex;
    std::condition_variable     m_cv;
    bool                        m_shutdown;
};


std::unique_ptr<ASYNC_SOCKET_HOLDER> socketHolder = nullptr;


/**
 * Used by a client to sent (by a socket connection) a data to a server.
 *  - Open a Socket Client connection.
 *  - Send the buffer cmdline.
 *  - Close the socket connection.
 *
 * @param aService is the service number for the TC/IP connection.
 * @param aMessage is the message to send.
 */
bool SendCommand( int aService, const std::string& aMessage )
{
    if( !socketHolder )
        socketHolder.reset( new ASYNC_SOCKET_HOLDER() );

    return socketHolder->Send( aService, aMessage );
}


void SocketCleanup()
{
    if( socketHolder )
        socketHolder.reset();
}
