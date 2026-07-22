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

#include <mdw/client/mdw_op.h>
#include <mdw/mdw_backend.h>
#include <mdw/mdw_credentials.h>
#include <mdw/mdw_proto_convert.h>
#include <mdw_runtime/mdw_backend_server.h>

#include <api/mdw/backend_base.pb.h>

#include <kinng.h>

#include <string>

#include <wx/intl.h>


MDW_BACKEND::MDW_BACKEND( const std::string& aControlUrl, const wxString& aChannelToken ) :
        m_client( std::make_unique<KINNG_REQUEST_CLIENT>( aControlUrl, MDW_CONTROL_MAX_BYTES ) ),
        m_channelToken( aChannelToken )
{
    // A just-spawned backend listens within milliseconds, which nng would wait out for up to a second
    m_client->SetReconnect( 10, 250 );
}


MDW_BACKEND::~MDW_BACKEND()
{
    Stop();
}


MDW_SEND_ORDER MDW_BACKEND::NextSendOrder()
{
    static std::atomic<MDW_SEND_ORDER> s_sent = 0;

    return ++s_sent;
}


bool MDW_BACKEND::Start()
{
    return m_client->Start();
}


void MDW_BACKEND::Stop()
{
    m_client->Stop();
}


static MDW_TRANSPORT_RESULT toTransportResult( const KINNG_REQUEST_CLIENT::RESULT& aRaw )
{
    using STATUS = KINNG_REQUEST_CLIENT::STATUS;

    MDW_TRANSPORT_RESULT result;

    switch( aRaw.status )
    {
    case STATUS::RECEIVED:
        // An unparseable reply is indistinguishable from a dead backend as far as the caller is concerned
        if( result.response.ParseFromString( aRaw.reply ) )
            result.status = MDW_TRANSPORT_STATUS::OK;

        break;

    case STATUS::TIMED_OUT: result.status = MDW_TRANSPORT_STATUS::TIMED_OUT; break;
    case STATUS::CANCELLED: result.status = MDW_TRANSPORT_STATUS::CANCELLED; break;

    // A close stopped the call under us, as when the backend died and its session is torn down
    case STATUS::CLOSED:
    case STATUS::FAILED: break;
    }

    return result;
}


std::uint64_t MDW_BACKEND::send( const kiapi::mdw::MdwRequest& aEnvelope, int aTimeoutMs,
                                 std::function<void( const MDW_TRANSPORT_RESULT& )> aOnReply, MDW_SEND_ORDER* aSent )
{
    return m_client->Request(
            aEnvelope.SerializeAsString(), aTimeoutMs,
            [aOnReply]( const KINNG_REQUEST_CLIENT::RESULT& aRaw )
            {
                aOnReply( toTransportResult( aRaw ) );
            },
            [aSent]()
            {
                if( aSent )
                    *aSent = NextSendOrder();
            } );
}


MDW_TRANSPORT_RESULT MDW_BACKEND::sendSync( const kiapi::mdw::MdwRequest& aEnvelope, int aTimeoutMs,
                                          MDW_SEND_ORDER* aSent )
{
    return toTransportResult( m_client->RequestSync( aEnvelope.SerializeAsString(), aTimeoutMs,
                                                     [aSent]()
                                                     {
                                                         if( aSent )
                                                             *aSent = NextSendOrder();
                                                     } ) );
}


bool MDW_BACKEND::makeEnvelope( const google::protobuf::Message& aPayload, kiapi::mdw::MdwRequest& aEnvelope )
{
    aEnvelope.set_channel_token( MdwToUtf8( m_channelToken ) );
    aEnvelope.set_request_id( std::to_string( ++m_requestSeq ) );

    return aEnvelope.mutable_payload()->PackFrom( aPayload );
}


bool MDW_BACKEND::Handshake( kiapi::mdw::Hello aHello, int aTimeoutMs )
{
    kiapi::mdw::Hello& hello = aHello;
    hello.mutable_protocol_version()->set_major( MDW_PROTOCOL_VERSION_MAJOR );
    hello.mutable_protocol_version()->set_minor( MDW_PROTOCOL_VERSION_MINOR );

    kiapi::mdw::HelloReply reply;

    if( !CallSync( hello, reply, aTimeoutMs ).IsOk() )
        return false;

    m_info = MdwToStruct( reply.backend_info() );

    if( reply.has_auth() )
    {
        m_authChallenge = MdwToStruct( reply.auth() );

        // Credentials are keyed by the realm, so a backend can neither widen it nor name another server's
        m_authChallenge->realm =
                MDW_CREDENTIALS::BoundRealm( m_authChallenge->realm, MdwFromUtf8( hello.workspace().locator() ),
                                             MdwFromUtf8( hello.workspace().local_root() ) );
    }

    commitCapabilities( reply.capabilities() );

    return true;
}


void MDW_BACKEND::commitCapabilities( const kiapi::workspace::Capabilities& aCapabilities )
{
    std::lock_guard<std::mutex> lock( m_descriptorMutex );
    m_capabilities = MdwToStruct( aCapabilities );
}


MDW_RESULT MDW_BACKEND::CheckConnection( int aTimeoutMs, MDW_ACCOUNT* aAccount, MDW_SEND_ORDER* aSent )
{
    kiapi::mdw::CheckConnectionRequest request;
    kiapi::mdw::CheckConnectionReply   reply;
    MDW_RESULT                         result = CallSync( request, reply, aTimeoutMs, aSent );

    // A backend from before CheckConnection never contacted its server, whatever it answers
    if( result.error == MDW_ERROR_CODE::NOT_SUPPORTED )
    {
        return MDW_RESULT::Fail( MDW_ERROR_CODE::PROTOCOL,
                                 _( "The workspace backend is too old for this version of KiCad. "
                                    "Reinstall or rebuild it." ) );
    }

    if( result.IsOk() && aAccount )
        *aAccount = MdwToStruct( reply.account() );

    return result;
}


MDW_CAPABILITIES MDW_BACKEND::Capabilities() const
{
    std::lock_guard<std::mutex> lock( m_descriptorMutex );
    return m_capabilities;
}


bool MDW_BACKEND::Has( MDW_CAPABILITY_GROUP aGroup ) const
{
    std::lock_guard<std::mutex> lock( m_descriptorMutex );
    return m_capabilities.Has( aGroup );
}


bool MDW_BACKEND::RefreshCapabilities( int aTimeoutMs )
{
    // A refresh that read before a login must not commit after the one that read after it
    std::lock_guard<std::mutex> refresh( m_refreshMutex );

    kiapi::mdw::GetCapabilitiesRequest request;
    kiapi::mdw::GetCapabilitiesReply   reply;

    if( !CallSync( request, reply, aTimeoutMs ).IsOk() )
        return false;

    commitCapabilities( reply.capabilities() );
    return true;
}


bool MDW_BACKEND::tooLarge( const kiapi::mdw::MdwRequest& aEnvelope )
{
    return aEnvelope.ByteSizeLong() > MDW_CONTROL_MAX_BYTES;
}


bool MDW_BACKEND::Ping( int aTimeoutMs )
{
    kiapi::mdw::PingRequest request;
    kiapi::mdw::PingReply   reply;

    return CallSync( request, reply, aTimeoutMs ).IsOk();
}


void MDW_BACKEND::Cancel( const CALL& aCall )
{
    if( !aCall )
        return;

    m_client->Cancel( aCall.handle );
    cancelServing( aCall.requestId );
}


void MDW_BACKEND::cancelServing( const std::string& aRequestId )
{
    kiapi::mdw::CancelRequestRequest request;
    kiapi::mdw::MdwRequest           envelope;

    request.set_request_id( aRequestId );

    // Sent past Call, so a cancel that times out is not cancelled in turn
    if( makeEnvelope( request, envelope ) )
        send( envelope, MDW_CANCEL_TIMEOUT_MS,
              []( const MDW_TRANSPORT_RESULT& )
              {
              } );
}


void MDW_BACKEND::Bye( int aTimeoutMs )
{
    kiapi::mdw::ByeRequest request;
    kiapi::mdw::MdwRequest envelope;

    // A backend that already exited never answers, so waiting for the reply would cost the whole timeout
    if( makeEnvelope( request, envelope ) )
    {
        send( envelope, aTimeoutMs,
              []( const MDW_TRANSPORT_RESULT& )
              {
              } );
    }
}
