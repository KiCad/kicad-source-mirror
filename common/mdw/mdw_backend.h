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
 * @file mdw_backend.h
 * @brief The in-process proxy for a single out-of-process backend.
 *
 * The proxy owns the transport and handshake, caches the advertised identity, capabilities and schema, and turns
 * typed protobuf requests into envelope exchanges. Callers above it never see a protobuf or a socket.
 */

#ifndef MDW_BACKEND_H
#define MDW_BACKEND_H

#include <kicommon.h>

#include "mdw_proto_convert.h"
#include <mdw/client/mdw_auth.h>
#include <mdw/client/mdw_capabilities.h>
#include <mdw/client/mdw_types.h>

#include <api/mdw/backend_base.pb.h>
#include <api/mdw/mdw_envelope.pb.h>

#include <atomic>
#include <chrono>
#include <cstdint>
#include <functional>
#include <memory>
#include <mutex>
#include <optional>
#include <string>

#include <wx/string.h>
#include <wx/translation.h>

class KINNG_REQUEST_CLIENT;

/// How one control-channel exchange ended
enum class MDW_TRANSPORT_STATUS
{
    OK,           ///< A response envelope was received and parsed
    TIMED_OUT,
    CANCELLED,    ///< Cancel or shutdown aborted it
    BACKEND_LOST  ///< Transport failed, or the reply did not parse
};


struct MDW_TRANSPORT_RESULT
{
    MDW_TRANSPORT_STATUS    status = MDW_TRANSPORT_STATUS::BACKEND_LOST;
    kiapi::mdw::MdwResponse response;  ///< Valid when status is OK
};


/// Where a call stands among every call sent in this process, so no two compare equal
using MDW_SEND_ORDER = std::uint64_t;

/// The result of a call the backend answered, and the order it was sent in
using MDW_OUTCOME_HOOK = std::function<void( const MDW_RESULT&, MDW_SEND_ORDER aSent )>;


class KICOMMON_API MDW_BACKEND
{
public:
    /// A call in flight, for Cancel
    struct CALL
    {
        std::uint64_t handle = 0;
        std::string   requestId;

        explicit operator bool() const { return handle != 0; }
    };

    MDW_BACKEND( const std::string& aControlUrl, const wxString& aChannelToken );

    ~MDW_BACKEND();

    bool Start();

    void Stop();

    /**
     * Perform the Hello handshake and fetch the schema. aHello carries the workspace binding;
     * the protocol version is filled here. Blocking; run it on a worker thread at spawn time.
     * Returns false on version mismatch, timeout, or transport failure.
     */
    bool Handshake( kiapi::mdw::Hello aHello, int aTimeoutMs );

    const MDW_BACKEND_INFO& Info() const { return m_info; }

    /// The credential form the handshake asked for, or none when the workspace needs no credentials
    const std::optional<MDW_AUTH_CHALLENGE>& AuthChallenge() const { return m_authChallenge; }

    /**
     * Ask the backend to contact its server. Ok only when the server answered and took the credentials, and
     * then aAccount, when given, receives who the server knows the user as. aSent, when given, receives the
     * check's send order once it is sent.
     */
    MDW_RESULT CheckConnection( int aTimeoutMs, MDW_ACCOUNT* aAccount = nullptr, MDW_SEND_ORDER* aSent = nullptr );

    MDW_CAPABILITIES Capabilities() const;

    bool Has( MDW_CAPABILITY_GROUP aGroup ) const;

    /**
     * Re-read the capabilities and schema, which a backend may refine once the user is authenticated.
     * Blocking. Returns false when the capabilities could not be read, leaving both as they were.
     */
    bool RefreshCapabilities( int aTimeoutMs );

    /**
     * Send a typed request asynchronously. aOnComplete receives the backend's whole error, its message and any
     * retry, auth or host-key detail included. It always runs, inside this call for a request that cannot be sent.
     */
    template <typename Req, typename Resp>
    CALL Call( const Req& aRequest, int aTimeoutMs, std::function<void( const MDW_RESULT&, const Resp& )> aOnComplete )
    {
        kiapi::mdw::MdwRequest envelope;

        if( !makeEnvelope( aRequest, envelope ) || tooLarge( envelope ) )
        {
            aOnComplete( MDW_RESULT::Fail( MDW_ERROR_CODE::INVALID_PARAMETER ), Resp() );
            return {};
        }

        // Shared, since it is numbered as the request goes out, after this completion is built
        auto sent = std::make_shared<MDW_SEND_ORDER>( 0 );

        auto onReply = [this, aOnComplete, requestId = envelope.request_id(), token = m_channelToken,
                        outcome = m_onOutcome, sent]( const MDW_TRANSPORT_RESULT& aResult )
        {
            // Left running, the backend would finish work nobody waits for, like a copy no index records
            if( aResult.status == MDW_TRANSPORT_STATUS::TIMED_OUT )
                cancelServing( requestId );

            Resp           resp;
            MDW_ERROR_CODE code = decode( aResult, resp, token );

            reportOutcome( outcome, aResult, code, *sent );
            aOnComplete( toResult( aResult, code ), resp );
        };

        std::uint64_t handle = send( envelope, aTimeoutMs, onReply, sent.get() );

        if( handle == 0 )
        {
            aOnComplete( MDW_RESULT::Fail( MDW_ERROR_CODE::BACKEND_LOST, _( "The workspace backend is not running." ) ),
                         Resp() );
            return {};
        }

        return { handle, envelope.request_id() };
    }

    /// Blocking typed request, answering the whole error as Call does. aSent receives its send order once sent
    template <typename Req, typename Resp>
    MDW_RESULT CallSync( const Req& aRequest, Resp& aResponse, int aTimeoutMs, MDW_SEND_ORDER* aSent = nullptr )
    {
        kiapi::mdw::MdwRequest envelope;

        if( !makeEnvelope( aRequest, envelope ) || tooLarge( envelope ) )
            return MDW_RESULT::Fail( MDW_ERROR_CODE::INVALID_PARAMETER );

        MDW_SEND_ORDER       sent = 0;
        MDW_TRANSPORT_RESULT result = sendSync( envelope, aTimeoutMs, &sent );

        if( aSent && sent )
            *aSent = sent;

        if( result.status == MDW_TRANSPORT_STATUS::TIMED_OUT )
            cancelServing( envelope.request_id() );

        MDW_ERROR_CODE code = decode( result, aResponse, m_channelToken );
        reportOutcome( m_onOutcome, result, code, sent );

        return toResult( result, code );
    }

    /**
     * Called on the completing thread with the result of every typed call the backend itself answered, so
     * the session can tell a server it cannot reach from a process it cannot reach. Set before Start().
     */
    void SetOutcomeHook( MDW_OUTCOME_HOOK aHook ) { m_onOutcome = std::move( aHook ); }

    /// Number a call about to be sent. One counter serves every backend, so a respawn never restarts it
    static MDW_SEND_ORDER NextSendOrder();

    /// Abandon aCall's reply and ask the backend to stop serving it
    void Cancel( const CALL& aCall );

    /// Ask the backend to stop serving aCall, still taking its reply
    void StopServing( const CALL& aCall ) { cancelServing( aCall.requestId ); }

    bool Ping( int aTimeoutMs );

    /// Ask the backend to exit, without waiting for its reply. The caller watches the process instead.
    void Bye( int aTimeoutMs );

private:
    /**
     * Returns a handle for the client's Cancel, or 0 without calling aOnReply when the client is not running.
     * aSent, when given, receives the send order as the request goes out, numbered under the send lock so one
     * backend's orders follow the order its sends leave in. It is untouched when nothing is sent.
     */
    std::uint64_t send( const kiapi::mdw::MdwRequest& aEnvelope, int aTimeoutMs,
                        std::function<void( const MDW_TRANSPORT_RESULT& )> aOnReply, MDW_SEND_ORDER* aSent = nullptr );

    /// Blocking send(), with aSent numbered the same way
    MDW_TRANSPORT_RESULT sendSync( const kiapi::mdw::MdwRequest& aEnvelope, int aTimeoutMs,
                                   MDW_SEND_ORDER* aSent = nullptr );

    /// Ask the backend to stop serving aRequestId, not waiting for its answer. Safe on the transport's thread
    void cancelServing( const std::string& aRequestId );

    void commitCapabilities( const kiapi::workspace::Capabilities& aCapabilities );

    /// False when aPayload cannot be packed
    bool makeEnvelope( const google::protobuf::Message& aPayload, kiapi::mdw::MdwRequest& aEnvelope );

    /// True when the backend's receive limit would drop the envelope unread
    static bool tooLarge( const kiapi::mdw::MdwRequest& aEnvelope );

    static void reportOutcome( const MDW_OUTCOME_HOOK& aHook, const MDW_TRANSPORT_RESULT& aResult, MDW_ERROR_CODE aCode,
                               MDW_SEND_ORDER aSent )
    {
        if( aHook && aResult.status == MDW_TRANSPORT_STATUS::OK )
            aHook( toResult( aResult, aCode ), aSent );
    }

    /// The backend's whole error when aCode is its own, else the bare code from transport or decoding
    static MDW_RESULT toResult( const MDW_TRANSPORT_RESULT& aResult, MDW_ERROR_CODE aCode )
    {
        if( aCode == MDW_ERROR_CODE::SUCCESS )
            return MDW_RESULT::Ok();

        const kiapi::workspace::WorkspaceError& error = aResult.response.error();

        if( aResult.status == MDW_TRANSPORT_STATUS::OK && MdwFromProto( error.code() ) == aCode )
            return MdwToResult( error );

        return MDW_RESULT::Fail( aCode );
    }

    /// Map a transport result plus its envelope error onto a framework code and
    /// unpack the payload into aResponse when the call succeeded.
    template <typename Resp>
    static MDW_ERROR_CODE decode( const MDW_TRANSPORT_RESULT& aResult, Resp& aResponse,
                                  const wxString& aChannelToken )
    {
        switch( aResult.status )
        {
        case MDW_TRANSPORT_STATUS::TIMED_OUT:    return MDW_ERROR_CODE::TIMEOUT;
        case MDW_TRANSPORT_STATUS::CANCELLED:    return MDW_ERROR_CODE::CANCELLED;
        case MDW_TRANSPORT_STATUS::BACKEND_LOST: return MDW_ERROR_CODE::BACKEND_LOST;
        case MDW_TRANSPORT_STATUS::OK:           break;
        }

        if( wxString::FromUTF8( aResult.response.channel_token() ) != aChannelToken )
            return MDW_ERROR_CODE::PROTOCOL;

        MDW_ERROR_CODE code = MdwFromProto( aResult.response.error().code() );

        // A success without the typed reply would otherwise read as a default-valued answer
        if( code == MDW_ERROR_CODE::SUCCESS
            && ( !aResult.response.has_payload() || !aResult.response.payload().UnpackTo( &aResponse ) ) )
        {
            return MDW_ERROR_CODE::PROTOCOL;
        }

        return code;
    }

    std::unique_ptr<KINNG_REQUEST_CLIENT> m_client;
    wxString                              m_channelToken;
    MDW_BACKEND_INFO                      m_info;
    std::optional<MDW_AUTH_CHALLENGE>     m_authChallenge;
    std::mutex                            m_refreshMutex;    ///< Commits refreshes in the order they read
    mutable std::mutex                    m_descriptorMutex; ///< Guards m_capabilities
    MDW_CAPABILITIES                      m_capabilities;
    std::atomic<std::uint64_t>            m_requestSeq{ 0 };
    MDW_OUTCOME_HOOK                      m_onOutcome;
};

#endif // MDW_BACKEND_H
