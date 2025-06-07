/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 3 of the License, or (at your
 * option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, you may find one here:
 * http://www.gnu.org/licenses/gpl-3.0.html
 * or you may write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
 */

#include <wx/tokenzr.h>
#include <wx/uri.h>

#include <remote_login_server.h>


wxDEFINE_EVENT( EVT_REMOTE_SYMBOL_LOGIN_RESULT, wxCommandEvent );

REMOTE_LOGIN_SERVER::REMOTE_LOGIN_SERVER( wxEvtHandler* aOwner, const wxString& aRedirectUrl ) :
        m_owner( aOwner ),
        m_redirectUrl( aRedirectUrl ),
        m_port( 0 ),
        m_done( false )
{
    m_timeout.SetOwner( this );
    Bind( wxEVT_TIMER, &REMOTE_LOGIN_SERVER::OnTimeout, this, m_timeout.GetId() );
}

REMOTE_LOGIN_SERVER::~REMOTE_LOGIN_SERVER()
{
    m_timeout.Stop();
    Unbind( wxEVT_SOCKET, &REMOTE_LOGIN_SERVER::OnSocketEvent, this );
    Unbind( wxEVT_TIMER, &REMOTE_LOGIN_SERVER::OnTimeout, this, m_timeout.GetId() );
    Shutdown();
    m_timeout.SetOwner( nullptr );
}

bool REMOTE_LOGIN_SERVER::Start()
{
    wxIPV4address addr;
    addr.AnyAddress();
    addr.Service( 0 );

    std::unique_ptr<wxSocketServer> server =
            std::make_unique<wxSocketServer>( addr, wxSOCKET_REUSEADDR );

    if( !server->IsOk() )
        return false;

    server->SetEventHandler( *this );
    server->SetNotify( wxSOCKET_CONNECTION_FLAG );
    server->Notify( true );

    wxIPV4address local;
    server->GetLocal( local );
    m_port = local.Service();

    Bind( wxEVT_SOCKET, &REMOTE_LOGIN_SERVER::OnSocketEvent, this );

    m_server = std::move( server );
    m_timeout.StartOnce( 120000 );
    return m_port != 0;
}

void REMOTE_LOGIN_SERVER::OnSocketEvent( wxSocketEvent& aEvent )
{
    if( !m_server || aEvent.GetSocketEvent() != wxSOCKET_CONNECTION )
        return;

    std::unique_ptr<wxSocketBase> client( m_server->Accept( false ) );

    if( !client )
        return;

    HandleClient( client.get() );
}

void REMOTE_LOGIN_SERVER::OnTimeout( wxTimerEvent& aEvent )
{
    wxUnusedVar( aEvent );
    Finish( false, wxString() );
}

void REMOTE_LOGIN_SERVER::HandleClient( wxSocketBase* aClient )
{
    if( !aClient )
        return;

    aClient->SetTimeout( 5 );
    aClient->SetFlags( wxSOCKET_NONE );

    std::string request;
    request.reserve( 512 );

    char buffer[512];

    while( aClient->IsConnected() )
    {
        if( !aClient->WaitForRead( 1, 0 ) )
            break;

        aClient->Read( buffer, sizeof( buffer ) );
        size_t count = aClient->LastCount();

        if( count == 0 )
            break;

        request.append( buffer, count );

        if( request.find( "\r\n\r\n" ) != std::string::npos || request.size() > 4096 )
            break;
    }

    SendHttpResponse( aClient );

    wxString requestWx = wxString::FromUTF8( request.data(), request.size() );
    int      endOfLine = requestWx.Find( wxS( "\r\n" ) );
    wxString requestLine = endOfLine == wxNOT_FOUND ? requestWx : requestWx.Mid( 0, endOfLine );

    wxString userId = ExtractUserId( requestLine );
    Finish( !userId.IsEmpty(), userId );
}

wxString REMOTE_LOGIN_SERVER::ExtractUserId( const wxString& aRequestLine ) const
{
    wxStringTokenizer tokenizer( aRequestLine, wxS( " " ) );

    if( !tokenizer.HasMoreTokens() )
        return wxString();

    tokenizer.GetNextToken();

    if( !tokenizer.HasMoreTokens() )
        return wxString();

    wxString path = tokenizer.GetNextToken();
    int      queryPos = path.Find( '?' );

    if( queryPos == wxNOT_FOUND )
        return wxString();

    wxString query = path.Mid( queryPos + 1 );
    wxStringTokenizer queryTokenizer( query, wxS( "&" ) );

    while( queryTokenizer.HasMoreTokens() )
    {
        wxString pair = queryTokenizer.GetNextToken();
        int      eqPos = pair.Find( '=' );

        if( eqPos == wxNOT_FOUND )
            continue;

        wxString name = pair.Left( eqPos );
        wxString value = pair.Mid( eqPos + 1 );

        if( name == wxS( "user_id" ) )
            return wxURI::Unescape( value );
    }

    return wxString();
}

void REMOTE_LOGIN_SERVER::SendHttpResponse( wxSocketBase* aClient )
{
    if( !aClient )
        return;

    wxString redirect = m_redirectUrl;

    if( redirect.IsEmpty() )
        redirect = wxS( "about:blank" );

    wxString html;
    html << wxS( "<!DOCTYPE html><html><head>" )
         << wxS( "<meta http-equiv=\"refresh\" content=\"0;url=" ) << redirect << wxS( "\">" )
         << wxS( "</head><body>" )
         << wxS( "<script>window.location.href = '" ) << redirect << wxS( "';</script>" )
         << wxS( "<p>Login successful. Redirecting...</p>" )
         << wxS( "</body></html>" );

    wxScopedCharBuffer body = html.ToUTF8();

    wxString response;
    response << wxS( "HTTP/1.1 200 OK\r\n" )
             << wxS( "Content-Type: text/html; charset=utf-8\r\n" )
             << wxS( "Access-Control-Allow-Origin: *\r\n" )
             << wxS( "Cache-Control: no-store\r\n" )
             << wxS( "Connection: close\r\n" )
             << wxS( "Content-Length: " ) << body.length() << wxS( "\r\n\r\n" );

    wxScopedCharBuffer header = response.ToUTF8();

    aClient->Write( header.data(), header.length() );
    aClient->Write( body.data(), body.length() );
    aClient->Close();
}

void REMOTE_LOGIN_SERVER::Finish( bool aSuccess, const wxString& aUserId )
{
    if( m_done )
        return;

    m_done = true;
    m_timeout.Stop();

    wxCommandEvent evt( EVT_REMOTE_SYMBOL_LOGIN_RESULT );
    evt.SetInt( aSuccess ? 1 : 0 );
    evt.SetString( aUserId );
    wxQueueEvent( m_owner, evt.Clone() );

    Shutdown();
}

void REMOTE_LOGIN_SERVER::Shutdown()
{
    if( m_server )
    {
        m_server->Notify( false );
        m_server.reset();
    }
}