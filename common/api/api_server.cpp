/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2023 Jon Evans <jon@craftyjon.com>
 * Copyright (C) 2023 KiCad Developers, see AUTHORS.txt for contributors.
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

#include <wx/app.h>
#include <wx/datetime.h>
#include <wx/event.h>

#include <advanced_config.h>
#include <api/api_server.h>
#include <api/api_handler_common.h>
#include <kiid.h>
#include <kinng.h>
#include <paths.h>
#include <pgm_base.h>
#include <string_utils.h>

#include <api/common/envelope.pb.h>

using kiapi::common::ApiRequest, kiapi::common::ApiResponse, kiapi::common::ApiStatusCode;


wxString KICAD_API_SERVER::s_logFileName = "api.log";


wxDEFINE_EVENT( API_REQUEST_EVENT, wxCommandEvent );


KICAD_API_SERVER::KICAD_API_SERVER() :
        wxEvtHandler(),
        m_token( KIID().AsStdString() ),
        m_readyToReply( false )
{
    m_server = std::make_unique<KINNG_REQUEST_SERVER>();
    m_server->SetCallback( [&]( std::string* aRequest ) { onApiRequest( aRequest ); } );
    m_socketPath = m_server->SocketPath();

    m_commonHandler = std::make_unique<API_HANDLER_COMMON>();
    RegisterHandler( m_commonHandler.get() );

    m_logFilePath.AssignDir( PATHS::GetLogsPath() );
    m_logFilePath.SetName( s_logFileName );

    if( ADVANCED_CFG::GetCfg().m_EnableAPILogging )
        PATHS::EnsurePathExists( PATHS::GetLogsPath() );

    log( "--- KiCad API server started ---\n" );

    Bind( API_REQUEST_EVENT, &KICAD_API_SERVER::handleApiEvent, this );
}


KICAD_API_SERVER::~KICAD_API_SERVER()
{
}


void KICAD_API_SERVER::RegisterHandler( API_HANDLER* aHandler )
{
    wxCHECK( aHandler, /* void */ );
    m_handlers.insert( aHandler );
}


void KICAD_API_SERVER::DeregisterHandler( API_HANDLER* aHandler )
{
    m_handlers.erase( aHandler );
}


void KICAD_API_SERVER::onApiRequest( std::string* aRequest )
{
    if( !m_readyToReply )
    {
        ApiResponse notHandled;
        notHandled.mutable_status()->set_status( ApiStatusCode::AS_NOT_READY );
        notHandled.mutable_status()->set_error_message( "KiCad is not ready to reply" );
        m_server->Reply( notHandled.SerializeAsString() );
        log( "Got incoming request but was not yet ready to reply." );
        return;
    }

    wxCommandEvent* evt = new wxCommandEvent( API_REQUEST_EVENT );

    // We don't actually need write access to this string, but client data is non-const
    evt->SetClientData( static_cast<void*>( aRequest ) );

    // Takes ownership and frees the wxCommandEvent
    QueueEvent( evt );
}


void KICAD_API_SERVER::handleApiEvent( wxCommandEvent& aEvent )
{
    std::string& requestString = *static_cast<std::string*>( aEvent.GetClientData() );
    ApiRequest request;

    if( !request.ParseFromString( requestString ) )
    {
        ApiResponse error;
        error.mutable_header()->set_kicad_token( m_token );
        error.mutable_status()->set_status( ApiStatusCode::AS_BAD_REQUEST );
        error.mutable_status()->set_error_message( "request could not be parsed" );
        m_server->Reply( error.SerializeAsString() );
        log( "Response (ERROR): " + error.Utf8DebugString() );
    }

    log( "Request: " + request.Utf8DebugString() );

    if( !request.header().kicad_token().empty() &&
        request.header().kicad_token().compare( m_token ) != 0 )
    {
        ApiResponse error;
        error.mutable_header()->set_kicad_token( m_token );
        error.mutable_status()->set_status( ApiStatusCode::AS_TOKEN_MISMATCH );
        error.mutable_status()->set_error_message(
                "the provided kicad_token did not match this KiCad instance's token" );
        m_server->Reply( error.SerializeAsString() );
        log( "Response (ERROR): " + error.Utf8DebugString() );
    }

    API_RESULT result;

    for( API_HANDLER* handler : m_handlers )
    {
        result = handler->Handle( request );

        if( result.has_value() )
            break;
        else if( result.error().status() != ApiStatusCode::AS_UNHANDLED )
            break;
    }

    // Note: at the point we call Reply(), we no longer own requestString.

    if( result.has_value() )
    {
        result->mutable_header()->set_kicad_token( m_token );
        m_server->Reply( result->SerializeAsString() );
        log( "Response: " + result->Utf8DebugString() );
    }
    else
    {
        ApiResponse error;
        error.mutable_status()->CopyFrom( result.error() );
        error.mutable_header()->set_kicad_token( m_token );

        if( result.error().status() == ApiStatusCode::AS_UNHANDLED )
        {
            std::string type = "<unparseable Any>";
            google::protobuf::Any::ParseAnyTypeUrl( request.message().type_url(), &type );
            std::string msg = fmt::format( "no handler available for request of type {}", type );
            error.mutable_status()->set_error_message( msg );
        }

        m_server->Reply( error.SerializeAsString() );
        log( "Response (ERROR): " + error.Utf8DebugString() );
    }
}


void KICAD_API_SERVER::log( const std::string& aOutput )
{
    if( !ADVANCED_CFG::GetCfg().m_EnableAPILogging )
        return;

    FILE* fp = wxFopen( m_logFilePath.GetFullPath(), wxT( "a" ) );

    if( !fp )
        return;

    wxString out;
    wxDateTime now = wxDateTime::Now();

    fprintf( fp, "%s", TO_UTF8( out.Format( wxS( "%s: %s" ),
                                            now.FormatISOCombined(), aOutput ) ) );
    fclose( fp );
}
