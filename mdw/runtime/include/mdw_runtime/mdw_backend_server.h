/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2026 KiCad Developers, see AUTHORS.txt for contributors.
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
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

/**
 * @file mdw_backend_server.h
 * @brief The MDW backend request server and the scaffold every backend's main runs.
 *
 * The server checks the channel token of each MdwRequest and routes its payload to the handler registered for
 * that protobuf type. A request with no handler answers WE_NOT_SUPPORTED.
 *
 * The runtime links kiapi and nng only and never kicommon or wxWidgets, which keeps wxString out of the protocol.
 */

#ifndef MDW_BACKEND_SERVER_H
#define MDW_BACKEND_SERVER_H

#include <api/mdw/mdw_envelope.pb.h>
#include <api/workspace/workspace_types.pb.h>

#include <google/protobuf/any.pb.h>

#include <api/mdw/backend_base.pb.h>

#include <atomic>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <functional>
#include <map>
#include <memory>
#include <mutex>
#include <optional>
#include <stdexcept>
#include <string>

class MDW_EVENT_PUBLISHER;
class MDW_IDEMPOTENCY_STORE;
struct MDW_LIFECYCLE_HOOKS;

/// The largest envelope the control and event channels carry
constexpr std::size_t MDW_CONTROL_MAX_BYTES = std::size_t( 16 ) << 20;

/// The protocol version the host and its backends speak. Major must match the client's and minor is additive
constexpr unsigned MDW_PROTOCOL_VERSION_MAJOR = 1;
constexpr unsigned MDW_PROTOCOL_VERSION_MINOR = 0;

/// What a backend found when asked whether an interrupted attempt took effect
enum class MDW_RECOVERY
{
    MISS,   ///< Verified not applied, so the handler may run
    REPLAY, ///< Applied, and the response now holds the reply it earned
    FAIL    ///< Unknown or conflicting, and the response holds the error to answer with
};

struct MDW_BACKEND_DESCRIPTOR
{
    kiapi::workspace::BackendInfo  info;
    kiapi::workspace::Capabilities capabilities;

    /**
     * The idempotency scope of the workspace a Hello binds, such as a repository. The journal opens under it at
     * the first Hello, and a session serves that one workspace. Hello fails WE_INTERNAL without a scope.
     */
    std::function<std::string( const kiapi::mdw::Hello& aHello )> idempotency_scope;

    /// The credential form answered as HelloReply.auth. Unset or empty, the workspace is never sent Authenticate
    std::function<std::optional<kiapi::workspace::AuthChallenge>( const kiapi::mdw::Hello& aHello )> auth;
};

struct MDW_BACKEND_LAUNCH
{
    std::string control_url;
    std::string events_url;
    std::string channel_token;
    long        parent_pid = 0;
    bool        valid = false;
};

/// Parse the launch parameters from argv and the KICAD_MDW_TOKEN environment
MDW_BACKEND_LAUNCH MDW_ParseLaunch( int argc, char** argv );

/**
 * The directory a backend keeps state in across restarts. KICAD_MDW_DATA_DIR overrides the default of
 * kicad/mdw/<aBackendId> under the platform's local state directory. Empty when no location is known.
 */
std::filesystem::path MDW_BackendDataDir( const std::string& aBackendId );

/// True once the request the calling handler serves was cancelled. Long handlers poll it
bool MdwCurrentRequestCancelled();

/**
 * The cancellation of the request the calling handler serves, or null outside one. MdwCurrentRequestCancelled()
 * answers only on the handler's own thread, so a library's worker threads read this instead.
 */
std::shared_ptr<const std::atomic<bool>> MdwCurrentRequestCancelToken();

/**
 * Report aDone of aTotal units of aPhase for the request the calling handler serves, with aTotal 0 while unknown.
 * aPhase is "receive", "checkout", "index" or other short text shown as given. Reports are throttled, so call it
 * as often as convenient. Does nothing for a request without an idempotency_id or an event channel, or from a
 * thread MdwFillStagedFolder did not start.
 */
void MdwReportProgress( std::uint64_t aDone, std::uint64_t aTotal, const std::string& aPhase );

/// MdwReportProgress bound to the calling thread's request, for a library's worker threads
using MDW_PROGRESS_REPORTER =
        std::function<void( std::uint64_t aDone, std::uint64_t aTotal, const std::string& aPhase )>;

/// Empty outside a request, or for one that cannot report
MDW_PROGRESS_REPORTER MdwCurrentRequestProgressReporter();

inline kiapi::workspace::WorkspaceError MdwOk()
{
    kiapi::workspace::WorkspaceError error;
    error.set_code( kiapi::workspace::WE_OK );

    return error;
}


/// A failure result carrying a message for the user
inline kiapi::workspace::WorkspaceError MdwFail( kiapi::workspace::WorkspaceErrorCode aCode,
                                                 const std::string&                   aMessage )
{
    kiapi::workspace::WorkspaceError error;
    error.set_code( aCode );
    error.set_message( aMessage );

    return error;
}


/// The result a handler returns once MdwCurrentRequestCancelled() is true
inline kiapi::workspace::WorkspaceError MdwCancelled()
{
    return MdwFail( kiapi::workspace::WE_CANCELLED, "Cancelled" );
}


/// The document kind a file extension names. A footprint library folder counts as a footprint
inline kiapi::workspace::DocumentKind MdwDocumentKind( const std::string& aPath )
{
    using namespace kiapi::workspace;

    static const std::pair<const char*, DocumentKind> kinds[] = {
        { ".kicad_pro", DK_PROJECT },        { ".kicad_sch", DK_SCHEMATIC }, { ".kicad_pcb", DK_BOARD },
        { ".kicad_sym", DK_SYMBOL_LIBRARY }, { ".kicad_mod", DK_FOOTPRINT }, { ".pretty", DK_FOOTPRINT },
    };

    for( const auto& [suffix, kind] : kinds )
    {
        if( aPath.ends_with( suffix ) )
            return kind;
    }

    return DK_OTHER;
}


class MDW_BACKEND_SERVER
{
public:
    MDW_BACKEND_SERVER();

    ~MDW_BACKEND_SERVER();

    /// What Hello and GetCapabilities answer. A backend may refine it after Authenticate
    void                   SetDescriptor( const MDW_BACKEND_DESCRIPTOR& aDescriptor );
    MDW_BACKEND_DESCRIPTOR Descriptor() const;

    /// The last Hello accepted, with its workspace binding
    kiapi::mdw::Hello HelloContext() const;

    /// Sees each request bound for a registered handler by full type name. An error it returns is the answer
    using REQUEST_FILTER = std::function<std::optional<kiapi::workspace::WorkspaceError>(
            const std::string& aRequestType, const kiapi::mdw::MdwRequest& aRequest )>;

    /// Call before Start. Sees built-in requests too
    void SetRequestFilter( REQUEST_FILTER aFilter ) { m_filter = std::move( aFilter ); }

    /**
     * Register a handler for request Req answering Resp. On WE_OK the filled Resp is the payload, otherwise the
     * error rides the envelope. Register every handler before Start, since the handler map is unguarded.
     *
     * A request type with an idempotency_id field is replayed from the idempotency store rather than run twice.
     * Its handler must return WE_OK whenever it left any effect, because an error promises nothing changed and
     * lets the same id run again. A handler that changed something but cannot finish its reply throws, leaving
     * the attempt for RegisterRecovery to judge.
     */
    template <typename Req, typename Resp>
    void RegisterHandler( std::function<kiapi::workspace::WorkspaceError( const Req&, Resp& )> aFn )
    {
        registerTokenHandler<Req, Resp>(
                [aFn]( const Req& aRequest, const std::string&, Resp& aReply )
                {
                    return aFn( aRequest, aReply );
                } );
    }

    /**
     * Register how a retry of an interrupted Req tells whether that attempt took effect. Call after
     * RegisterHandler for the same Req.
     *
     * aToken runs before every journalled first attempt and captures the backend state the request is about to
     * change, such as a branch head. The token is stored durably before the handler runs. A non-OK result refuses
     * the request.
     *
     * aRecover gets that token when a retry finds an attempt interrupted by a crash or an exception. It returns
     * MISS only when it can show the attempt left no effect, REPLAY with Resp filled when it finds the effect, and
     * FAIL with aError set when it cannot tell. Without recovery such a retry answers WE_VERSION_CONFLICT.
     */
    template <typename Req, typename Resp>
    void RegisterRecovery( std::function<kiapi::workspace::WorkspaceError( const Req&, std::string& aToken )> aToken,
                           std::function<MDW_RECOVERY( const Req&, const std::string& aToken, Resp&,
                                                       kiapi::workspace::WorkspaceError& aError )>
                                   aRecover )
    {
        auto it = m_handlers.find( std::string( Req::descriptor()->full_name() ) );

        if( it == m_handlers.end() )
            return;

        it->second.token = [aToken]( const google::protobuf::Any& aRequest, std::string& aOut,
                                     kiapi::mdw::MdwResponse& aResponse )
        {
            Req req;

            if( !aRequest.UnpackTo( &req ) )
            {
                aResponse.mutable_error()->set_code( kiapi::workspace::WE_PROTOCOL );
                aResponse.mutable_error()->set_message( "Malformed request payload" );
                return false;
            }

            *aResponse.mutable_error() = aToken( req, aOut );
            return aResponse.error().code() == kiapi::workspace::WE_OK;
        };

        it->second.recover = [aRecover]( const google::protobuf::Any& aRequest, const std::string& aStored,
                                         kiapi::mdw::MdwResponse& aResponse )
        {
            Req                              req;
            Resp                             resp;
            kiapi::workspace::WorkspaceError error;

            if( !aRequest.UnpackTo( &req ) )
            {
                aResponse.mutable_error()->set_code( kiapi::workspace::WE_PROTOCOL );
                aResponse.mutable_error()->set_message( "Malformed request payload" );
                return MDW_RECOVERY::FAIL;
            }

            MDW_RECOVERY found = aRecover( req, aStored, resp, error );

            if( found == MDW_RECOVERY::REPLAY )
            {
                aResponse.mutable_error()->set_code( kiapi::workspace::WE_OK );

                if( !aResponse.mutable_payload()->PackFrom( resp ) )
                    throw std::runtime_error( "reply could not be serialized" );
            }
            else if( found == MDW_RECOVERY::FAIL )
            {
                *aResponse.mutable_error() = error;
            }

            return found;
        };
    }

    /// Bind the control socket and spawn aWorkers blocking worker threads
    bool Start( const std::string& aControlUrl, int aWorkers );

    void Stop();

    bool Running() const;

    /**
     * Run a backend to completion and return a process exit code. Registers the built-in Hello, GetCapabilities,
     * Ping, Bye and CancelRequest handlers, which answer from aDescriptor, and a GetPendingChange that answers
     * WE_ITEM_NOT_FOUND.
     *
     * Serves this server and aPublisher, if any, until a Bye, a termination signal, or parent death. Requests
     * carrying an idempotency_id are journalled under MDW_BackendDataDir, in a journal the first Hello opens.
     * Until then only Hello, Ping, Bye and CancelRequest are answered, so no mutation runs unjournalled.
     */
    int Run( const MDW_BACKEND_LAUNCH& aLaunch, const MDW_BACKEND_DESCRIPTOR& aDescriptor,
             MDW_EVENT_PUBLISHER* aPublisher );

private:
    // Registers Release through registerTokenHandler
    friend void MdwServeLifecycle( MDW_BACKEND_SERVER& aServer, const MDW_LIFECYCLE_HOOKS& aHooks );

    /// Decodes the request payload and fills the response. aToken is what the RegisterRecovery token captured
    /// before this attempt, or empty
    using RAW_HANDLER = std::function<void( const google::protobuf::Any& aRequest, const std::string& aToken,
                                            kiapi::mdw::MdwResponse& aResponse )>;

    struct IMPL;

    struct HANDLER
    {
        RAW_HANDLER                                                run;
        std::function<std::string( const google::protobuf::Any& )> idempotencyId;
        std::function<bool( const google::protobuf::Any&, std::string&, kiapi::mdw::MdwResponse& )> token;
        std::function<MDW_RECOVERY( const google::protobuf::Any&, const std::string&, kiapi::mdw::MdwResponse& )>
                recover;
    };

    /**
     * RegisterHandler for a handler that acts on what the RegisterRecovery token captured, so its first attempt
     * and any recovery work from the same state. An unjournalled attempt gets an empty token.
     */
    template <typename Req, typename Resp>
    void registerTokenHandler(
            std::function<kiapi::workspace::WorkspaceError( const Req&, const std::string& aToken, Resp& )> aFn )
    {
        RAW_HANDLER raw = [aFn]( const google::protobuf::Any& aRequest, const std::string& aToken,
                                 kiapi::mdw::MdwResponse& aResponse )
        {
            Req req;

            if( !aRequest.UnpackTo( &req ) )
            {
                aResponse.mutable_error()->set_code( kiapi::workspace::WE_PROTOCOL );
                aResponse.mutable_error()->set_message( "Malformed request payload" );
                return;
            }

            Resp                             resp;
            kiapi::workspace::WorkspaceError error = aFn( req, aToken, resp );
            *aResponse.mutable_error() = error;

            // A returned failure frees the idempotency id, but the change already happened
            if( error.code() == kiapi::workspace::WE_OK && !aResponse.mutable_payload()->PackFrom( resp ) )
                throw std::runtime_error( "reply could not be serialized" );
        };

        HANDLER handler;
        handler.run = std::move( raw );

        if( const google::protobuf::FieldDescriptor* field = Req::descriptor()->FindFieldByName( "idempotency_id" ) )
        {
            handler.idempotencyId = [field]( const google::protobuf::Any& aRequest )
            {
                Req req;

                if( !aRequest.UnpackTo( &req ) )
                    return std::string();

                return req.GetReflection()->GetString( req, field );
            };
        }

        m_handlers[std::string( Req::descriptor()->full_name() )] = std::move( handler );
    }

    /// The built-in handlers Run serves
    void registerBuiltins();

    /**
     * Bind the workspace aHello names, as the built-in Hello does. The first accepted Hello opens the journal,
     * and a later Hello must name the same binding. A refused Hello changes nothing.
     */
    kiapi::workspace::WorkspaceError acceptHello( const kiapi::mdw::Hello& aHello );

    /// Cancel a request a worker is serving; its handler sees MdwCurrentRequestCancelled()
    void cancelRequest( const std::string& aRequestId );

    /// True once a Bye reply has been sent, waiting up to aTimeout for it
    bool waitForBye( std::chrono::milliseconds aTimeout );

    /// Called from a handler to report a Bye once this reply is sent, so the socket never closes before it
    void signalByeAfterReply();

    /// Serve requests on the context Start opened at aContext until Stop
    void workerLoop( size_t aContext );

    /// dispatch, with a throwing handler answered as WE_INTERNAL
    void dispatchSafely( const kiapi::mdw::MdwRequest& aRequest, kiapi::mdw::MdwResponse& aResponse );

    void dispatch( const kiapi::mdw::MdwRequest& aRequest, kiapi::mdw::MdwResponse& aResponse );

    std::string                            m_channelToken;
    std::filesystem::path                  m_journalDir; ///< Empty unless Run serves, so nothing waits for Hello
    std::mutex                             m_helloMutex; ///< Serializes Hello acceptance
    std::atomic<bool>                      m_helloAccepted{ false };
    mutable std::mutex                     m_contextMutex;
    MDW_BACKEND_DESCRIPTOR                 m_descriptor;
    kiapi::mdw::Hello                      m_hello;
    std::map<std::string, HANDLER>         m_handlers;
    REQUEST_FILTER                         m_filter;
    std::atomic<bool>                      m_bye{ false };
    MDW_EVENT_PUBLISHER*                   m_publisher = nullptr;
    std::shared_ptr<MDW_IDEMPOTENCY_STORE> m_idempotency; ///< Under m_contextMutex
    std::unique_ptr<IMPL>                  m_impl;
};

#endif // MDW_BACKEND_SERVER_H
