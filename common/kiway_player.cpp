/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2014 SoftPLC Corporation, Dick Hollenbeck <dick@softplc.com>
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


#include <kiway_player.h>
#include <kiway_mail.h>
#include <kiway.h>
#include <id.h>
#include <macros.h>
#include <typeinfo>
#include <wx/utils.h>
#include <wx/evtloop.h>
#include <wx/socket.h>
#include <core/raii.h>
#include <wx/log.h>


static const wxString HOSTNAME( wxT( "localhost" ) );

// buffer for read and write data in socket connections
#define IPC_BUF_SIZE 4096
static char client_ipc_buffer[IPC_BUF_SIZE];

BEGIN_EVENT_TABLE( KIWAY_PLAYER, EDA_BASE_FRAME )
    EVT_KIWAY_EXPRESS( KIWAY_PLAYER::kiway_express )
END_EVENT_TABLE()


KIWAY_PLAYER::KIWAY_PLAYER( KIWAY* aKiway, wxWindow* aParent, FRAME_T aFrameType,
                            const wxString& aTitle, const wxPoint& aPos, const wxSize& aSize,
                            long aStyle, const wxString& aFrameName,
                            const EDA_IU_SCALE& aIuScale ) :
        EDA_BASE_FRAME( aParent, aFrameType, aTitle, aPos, aSize, aStyle, aFrameName, aKiway,
                        aIuScale ),
        m_modal( false ),
        m_modal_loop( nullptr ),
        m_modal_resultant_parent( nullptr ),
        m_modal_ret_val( false ),
        m_socketServer( nullptr )
{
}


KIWAY_PLAYER::~KIWAY_PLAYER() throw()
{
    // socket server must be destructed before we complete
    // destructing the frame or else we could crash
    // as the socket server holds a reference to this frame
    if( m_socketServer )
    {
        // ensure any event handling stops
        m_socketServer->Notify( false );

        delete m_socketServer;
        m_socketServer = nullptr;
    }

    // remove active sockets as well
    for( wxSocketBase* socket : m_sockets )
    {
        if( !socket )
            continue;

        // ensure any event handling stops
        socket->Notify( false );

        delete socket;
    }

    m_sockets.clear();
}


void KIWAY_PLAYER::KiwayMailIn( KIWAY_MAIL_EVENT& aEvent )
{
    // override this in derived classes.
}


bool KIWAY_PLAYER::ShowModal( wxString* aResult, wxWindow* aResultantFocusWindow )
{
    wxASSERT_MSG( IsModal(), wxT( "ShowModal() shouldn't be called on non-modal frame" ) );

    /*
        This function has a nice interface but a necessarily unsightly implementation.
        Now the implementation is encapsulated, localizing future changes.

        It works in tandem with DismissModal().  But only ShowModal() is in the
        vtable and therefore cross-module capable.
    */

    NULLER raii_nuller( (void*&) m_modal_loop );

    m_modal_resultant_parent = aResultantFocusWindow;

    Show( true );
    Raise();    // Needed on some Window managers to always display the frame

    SetFocus();

    {
        // Using wxWindowDisabler() has two issues: it will disable top-level windows that are
        // our *children* (such as sub-frames), and it will disable all context menus we try to
        // put up.  Fortunatly we already had to cross this Rubicon for QuasiModal dialogs, so
        // we re-use that strategy.
        wxWindow* parent = GetParent();

        while( parent && !parent->IsTopLevel() )
            parent = parent->GetParent();

        WINDOW_DISABLER raii_parent_disabler( parent );

        wxGUIEventLoop event_loop;
        m_modal_loop = &event_loop;
        event_loop.Run();
    }

    if( aResult )
        *aResult = m_modal_string;

    if( aResultantFocusWindow )
    {
        aResultantFocusWindow->Raise();

        // have the final say, after wxWindowDisabler reenables my parent and
        // the events settle down, set the focus
        wxSafeYield();
        aResultantFocusWindow->SetFocus();
    }

    return m_modal_ret_val;
}


bool KIWAY_PLAYER::Destroy()
{
    Kiway().PlayerDidClose( GetFrameType() );

    return EDA_BASE_FRAME::Destroy();
}


bool KIWAY_PLAYER::IsDismissed()
{
    return !m_modal_loop;
}


void KIWAY_PLAYER::DismissModal( bool aRetVal, const wxString& aResult )
{
    m_modal_ret_val = aRetVal;
    m_modal_string  = aResult;

    if( m_modal_loop )
    {
        m_modal_loop->Exit();
        m_modal_loop = nullptr;      // this marks it as dismissed.
    }

    Show( false );
}


void KIWAY_PLAYER::kiway_express( KIWAY_MAIL_EVENT& aEvent )
{
    // logging support
    KiwayMailIn( aEvent );     // call the virtual, override in derived.
}


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
        wxLogError( wxT( "EDA_DRAW_FRAME::OnSockRequest() error: Invalid event !" ) );
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

