/**
 * @file eda_dde.cpp
 */

#include <fctsys.h>
#include <eda_dde.h>
#include <draw_frame.h>
#include <id.h>
#include <common.h>
#include <macros.h>

static const wxString HOSTNAME( wxT( "localhost" ) );

// buffer for read and write data in socket connections
#define IPC_BUF_SIZE 4096
static char client_ipc_buffer[IPC_BUF_SIZE];

static wxSocketServer* server;


/**********************************/
/* Routines related to the server */
/**********************************/

/* Function to initialize a server socket
 */
wxSocketServer* CreateServer( wxWindow* window, int service, bool local )
{
    wxIPV4address addr;

    // Set the port number
    addr.Service( service );

    // Listen on localhost only if requested
    if( local )
        addr.Hostname( HOSTNAME );

    server = new wxSocketServer( addr );

    if( server )
    {
        server->SetNotify( wxSOCKET_CONNECTION_FLAG );
        server->SetEventHandler( *window, ID_EDA_SOCKET_EVENT_SERV );
        server->Notify( true );
    }

    return server;
}


/* Function called on every client request.
 */
void EDA_DRAW_FRAME::OnSockRequest( wxSocketEvent& evt )
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


/* Function called when a connection is requested by a client.
 */
void EDA_DRAW_FRAME::OnSockRequestServer( wxSocketEvent& evt )
{
    wxSocketBase*   sock2;
    wxSocketServer* server = (wxSocketServer*) evt.GetSocket();

    sock2 = server->Accept();

    if( sock2 == NULL )
        return;

    sock2->Notify( true );
    sock2->SetEventHandler( *this, ID_EDA_SOCKET_EVENT );
    sock2->SetNotify( wxSOCKET_INPUT_FLAG | wxSOCKET_LOST_FLAG );
}


/**********************************/
/* Routines related to the CLIENT */
/**********************************/

/* Used by a client to sent (by a socket connection) a data to a server.
 *  - Open a Socket Client connection
 *  - Send the buffer cmdline
 *  - Close the socket connection
 *
 *  service is the service number for the TC/IP connection
 */
bool SendCommand( int service, const char* cmdline )
{
    wxSocketClient* sock_client;
    bool            success = false;
    wxIPV4address   addr;

    // Create a connexion
    addr.Hostname( HOSTNAME );
    addr.Service( service );

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

    sock_client = new wxSocketClient();
    sock_client->SetTimeout( 2 ); // Time out in Seconds
    sock_client->Connect( addr, false );
    sock_client->WaitOnConnect( 0, 100 );

    if( sock_client->Ok() && sock_client->IsConnected() )
    {
        success = true;
        sock_client->SetFlags( wxSOCKET_NOWAIT /*wxSOCKET_WAITALL*/ );
        sock_client->Write( cmdline, strlen( cmdline ) );
    }

    sock_client->Close();
    sock_client->Destroy();
    return success;
}
