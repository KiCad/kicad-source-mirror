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
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include <oauth/oauth_loopback_server.h>

#include <wx/tokenzr.h>
#include <wx/uri.h>
#include <wx/intl.h>


wxDEFINE_EVENT( EVT_OAUTH_LOOPBACK_RESULT, wxCommandEvent );


namespace
{
wxString queryValue( const wxString& aQuery, const wxString& aKey )
{
    wxStringTokenizer queryTokenizer( aQuery, wxS( "&" ) );

    while( queryTokenizer.HasMoreTokens() )
    {
        const wxString pair = queryTokenizer.GetNextToken();
        const int eqPos = pair.Find( '=' );

        if( eqPos == wxNOT_FOUND )
            continue;

        const wxString name = pair.Left( eqPos );
        const wxString value = pair.Mid( eqPos + 1 );

        if( name == aKey )
            return wxURI::Unescape( value );
    }

    return wxEmptyString;
}
} // namespace


OAUTH_LOOPBACK_SERVER::OAUTH_LOOPBACK_SERVER( wxEvtHandler* aOwner, const wxString& aCallbackPath,
                                              const wxString& aExpectedState ) :
        m_owner( aOwner ),
        m_callbackPath( aCallbackPath ),
        m_expectedState( aExpectedState ),
        m_port( 0 ),
        m_done( false )
{
    m_timeout.SetOwner( this );
    Bind( wxEVT_TIMER, &OAUTH_LOOPBACK_SERVER::OnTimeout, this, m_timeout.GetId() );
}


OAUTH_LOOPBACK_SERVER::~OAUTH_LOOPBACK_SERVER()
{
    m_timeout.Stop();
    Unbind( wxEVT_SOCKET, &OAUTH_LOOPBACK_SERVER::OnSocketEvent, this );
    Unbind( wxEVT_TIMER, &OAUTH_LOOPBACK_SERVER::OnTimeout, this, m_timeout.GetId() );
    Shutdown();
    m_timeout.SetOwner( nullptr );
}


bool OAUTH_LOOPBACK_SERVER::Start()
{
    wxIPV4address addr;
    addr.Hostname( wxS( "127.0.0.1" ) );
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

    Bind( wxEVT_SOCKET, &OAUTH_LOOPBACK_SERVER::OnSocketEvent, this );

    m_server = std::move( server );
    m_timeout.StartOnce( 120000 );
    return m_port != 0;
}


wxString OAUTH_LOOPBACK_SERVER::GetRedirectUri() const
{
    return wxString::Format( wxS( "http://127.0.0.1:%u%s" ), m_port, m_callbackPath );
}


bool OAUTH_LOOPBACK_SERVER::ParseAuthorizationResponse( const wxString& aRequestLine,
                                                        const wxString& aExpectedPath,
                                                        const wxString& aExpectedState,
                                                        OAUTH_AUTHORIZATION_RESPONSE& aResponse,
                                                        wxString& aError )
{
    aResponse = OAUTH_AUTHORIZATION_RESPONSE();
    aError.clear();

    wxStringTokenizer tokenizer( aRequestLine, wxS( " " ) );

    if( !tokenizer.HasMoreTokens() )
    {
        aError = _( "Missing HTTP method in callback request." );
        return false;
    }

    tokenizer.GetNextToken();

    if( !tokenizer.HasMoreTokens() )
    {
        aError = _( "Missing callback URL in request." );
        return false;
    }

    const wxString target = tokenizer.GetNextToken();
    const int queryPos = target.Find( '?' );
    const wxString path = queryPos == wxNOT_FOUND ? target : target.Left( queryPos );
    const wxString query = queryPos == wxNOT_FOUND ? wxString() : target.Mid( queryPos + 1 );

    if( path != aExpectedPath )
    {
        aError = _( "Unexpected callback path." );
        return false;
    }

    wxString state = queryValue( query, wxS( "state" ) );

    if( state != aExpectedState )
    {
        aError = _( "OAuth callback state did not match." );
        return false;
    }

    aResponse.state = state;
    aResponse.error = queryValue( query, wxS( "error" ) );
    aResponse.error_description = queryValue( query, wxS( "error_description" ) );

    if( !aResponse.error.IsEmpty() )
    {
        aError = _( "Authorization server returned an OAuth error." );
        return false;
    }

    aResponse.code = queryValue( query, wxS( "code" ) );

    if( aResponse.code.IsEmpty() )
    {
        aError = _( "Missing authorization code in callback." );
        return false;
    }

    return true;
}


void OAUTH_LOOPBACK_SERVER::OnSocketEvent( wxSocketEvent& aEvent )
{
    if( !m_server || aEvent.GetSocketEvent() != wxSOCKET_CONNECTION )
        return;

    std::unique_ptr<wxSocketBase> client( m_server->Accept( false ) );

    if( client )
        HandleClient( client.get() );
}


void OAUTH_LOOPBACK_SERVER::OnTimeout( wxTimerEvent& aEvent )
{
    wxUnusedVar( aEvent );
    Finish( false );
}


void OAUTH_LOOPBACK_SERVER::HandleClient( wxSocketBase* aClient )
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
        const size_t count = aClient->LastCount();

        if( count == 0 )
            break;

        request.append( buffer, count );

        if( request.find( "\r\n\r\n" ) != std::string::npos || request.size() > 4096 )
            break;
    }

    const wxString requestWx = wxString::FromUTF8( request.data(), request.size() );
    const int endOfLine = requestWx.Find( wxS( "\r\n" ) );
    const wxString requestLine = endOfLine == wxNOT_FOUND ? requestWx : requestWx.Mid( 0, endOfLine );

    OAUTH_AUTHORIZATION_RESPONSE response;
    wxString error;
    const bool success = ParseAuthorizationResponse( requestLine, m_callbackPath, m_expectedState,
                                                     response, error );

    if( success )
        m_response = response;

    SendHttpResponse( aClient, success );
    Finish( success );
}


void OAUTH_LOOPBACK_SERVER::SendHttpResponse( wxSocketBase* aClient, bool aSuccess )
{
    if( !aClient )
        return;

    const wxString body = aSuccess
                                  ? wxS( "<!DOCTYPE html><html><body><p>Authentication complete.</p></body></html>" )
                                  : wxS( "<!DOCTYPE html><html><body><p>Authentication failed.</p></body></html>" );
    wxScopedCharBuffer bodyUtf8 = body.ToUTF8();

    wxString response;
    response << wxS( "HTTP/1.1 200 OK\r\n" )
             << wxS( "Content-Type: text/html; charset=utf-8\r\n" )
             << wxS( "Cache-Control: no-store\r\n" )
             << wxS( "Connection: close\r\n" )
             << wxS( "Content-Length: " ) << bodyUtf8.length() << wxS( "\r\n\r\n" );

    wxScopedCharBuffer header = response.ToUTF8();
    aClient->Write( header.data(), header.length() );
    aClient->Write( bodyUtf8.data(), bodyUtf8.length() );
    aClient->Close();
}


void OAUTH_LOOPBACK_SERVER::Finish( bool aSuccess )
{
    if( m_done )
        return;

    m_done = true;
    m_timeout.Stop();

    if( m_owner )
    {
        wxCommandEvent evt( EVT_OAUTH_LOOPBACK_RESULT );
        evt.SetInt( aSuccess ? 1 : 0 );

        if( aSuccess && m_response.has_value() )
            evt.SetString( m_response->code );

        wxQueueEvent( m_owner, evt.Clone() );
    }

    Shutdown();
}


void OAUTH_LOOPBACK_SERVER::Shutdown()
{
    if( m_server )
    {
        m_server->Notify( false );
        m_server.reset();
    }
}
