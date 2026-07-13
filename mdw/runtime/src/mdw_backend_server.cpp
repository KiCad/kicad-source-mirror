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

#include <mdw_runtime/mdw_backend_server.h>
#include <mdw_runtime/mdw_event_publisher.h>
#include <mdw_runtime/mdw_strings.h>

#include "mdw_idempotency.h"
#include "mdw_runtime_internal.h"

#include <nng/nng.h>
#include <nng/protocol/reqrep0/rep.h>

#include <google/protobuf/util/message_differencer.h>

#include <algorithm>
#include <chrono>
#include <condition_variable>
#include <cstdio>
#include <map>
#include <mutex>
#include <thread>
#include <utility>
#include <vector>

namespace
{
/// Handlers run on the worker that sends their reply, so the request in flight is per thread
thread_local bool                                     t_byeAfterReply = false;
thread_local std::shared_ptr<const std::atomic<bool>> t_cancelled;
thread_local std::shared_ptr<MDW_REQUEST_PROGRESS>    t_progress;

// A few a second is enough to watch, and more only queues work for a client that paints no faster
constexpr std::chrono::milliseconds PROGRESS_INTERVAL( 250 );
} // namespace


std::shared_ptr<MDW_REQUEST_PROGRESS> MdwCurrentRequestProgress()
{
    return t_progress;
}


MDW_REQUEST_PROGRESS_SCOPE::MDW_REQUEST_PROGRESS_SCOPE( std::shared_ptr<MDW_REQUEST_PROGRESS> aProgress ) :
        m_outer( std::move( t_progress ) )
{
    t_progress = std::move( aProgress );
}


MDW_REQUEST_PROGRESS_SCOPE::~MDW_REQUEST_PROGRESS_SCOPE()
{
    t_progress = std::move( m_outer );
}


static void reportTo( MDW_REQUEST_PROGRESS& aProgress, std::uint64_t aDone, std::uint64_t aTotal,
                      const std::string& aPhase )
{
    const auto now = std::chrono::steady_clock::now();
    const bool finished = aTotal != 0 && aDone >= aTotal;

    {
        std::lock_guard<std::mutex> lock( aProgress.mutex );

        // The start and end of a phase always go, so the display never misses where a step began or ended
        const bool edge = aPhase != aProgress.lastPhase || ( finished && !aProgress.lastFinished );

        if( !edge && now - aProgress.last < PROGRESS_INTERVAL )
            return;

        aProgress.last = now;
        aProgress.lastPhase = aPhase;
        aProgress.lastFinished = finished;
    }

    kiapi::workspace::EventEnvelope event;
    event.set_type( kiapi::workspace::ET_PROGRESS );
    event.set_workspace_id( aProgress.workspaceId );
    event.mutable_progress()->set_done( aDone );
    event.mutable_progress()->set_total( aTotal );
    event.mutable_progress()->set_phase( aPhase );

    aProgress.publisher->Publish( event, aProgress.operationId );
}


void MdwReportProgress( std::uint64_t aDone, std::uint64_t aTotal, const std::string& aPhase )
{
    if( t_progress )
        reportTo( *t_progress, aDone, aTotal, aPhase );
}


MDW_PROGRESS_REPORTER MdwCurrentRequestProgressReporter()
{
    std::shared_ptr<MDW_REQUEST_PROGRESS> progress = t_progress;

    if( !progress )
        return nullptr;

    return [progress]( std::uint64_t aDone, std::uint64_t aTotal, const std::string& aPhase )
    {
        reportTo( *progress, aDone, aTotal, aPhase );
    };
}


bool MdwCurrentRequestCancelled()
{
    return t_cancelled && t_cancelled->load();
}


std::shared_ptr<const std::atomic<bool>> MdwCurrentRequestCancelToken()
{
    return t_cancelled;
}


MDW_REQUEST_CANCEL_SCOPE::MDW_REQUEST_CANCEL_SCOPE( std::shared_ptr<const std::atomic<bool>> aFlag ) :
        m_outer( std::move( t_cancelled ) )
{
    t_cancelled = std::move( aFlag );
}


MDW_REQUEST_CANCEL_SCOPE::~MDW_REQUEST_CANCEL_SCOPE()
{
    t_cancelled = std::move( m_outer );
}


struct MDW_BACKEND_SERVER::IMPL
{
    nng_socket               socket = NNG_SOCKET_INITIALIZER;
    std::atomic<bool>        running{ false };
    std::vector<nng_ctx>     contexts; ///< One per worker, closed by Stop before the socket
    std::vector<std::thread> workers;

    /// Every delivery of one request id shares a flag, since a resend can overlap the original
    struct IN_FLIGHT
    {
        std::shared_ptr<std::atomic<bool>> cancelled = std::make_shared<std::atomic<bool>>( false );
        int                                deliveries = 0;
    };

    std::mutex                       inFlightMutex;
    std::map<std::string, IN_FLIGHT> inFlight;

    std::mutex              byeMutex;
    std::condition_variable byeWake;
};


MDW_BACKEND_SERVER::MDW_BACKEND_SERVER() :
        m_impl( std::make_unique<IMPL>() )
{
}


MDW_BACKEND_SERVER::~MDW_BACKEND_SERVER()
{
    Stop();
}


namespace
{

bool answersBeforeHello( const std::string& aTypeName )
{
    return aTypeName == kiapi::mdw::Hello::descriptor()->full_name()
           || aTypeName == kiapi::mdw::PingRequest::descriptor()->full_name()
           || aTypeName == kiapi::mdw::ByeRequest::descriptor()->full_name()
           || aTypeName == kiapi::mdw::CancelRequestRequest::descriptor()->full_name();
}

} // namespace


kiapi::workspace::WorkspaceError MDW_BACKEND_SERVER::acceptHello( const kiapi::mdw::Hello& aHello )
{
    std::lock_guard<std::mutex> serialize( m_helloMutex );

    // The journal is keyed by this binding, so a session never switches workspace
    if( m_helloAccepted.load() )
    {
        // Only a resent copy of the first Hello reaches here, since the host sends one per process
        if( !google::protobuf::util::MessageDifferencer::Equals( HelloContext().workspace(), aHello.workspace() ) )
            return MdwFail( kiapi::workspace::WE_PROTOCOL, "This session already serves another workspace" );

        return MdwOk();
    }

    std::function<std::string( const kiapi::mdw::Hello& )> scope = Descriptor().idempotency_scope;
    const std::string                                      key = scope ? scope( aHello ) : std::string();

    // An empty scope would share one journal across workspaces and replay one's reply to another
    if( key.empty() )
        return MdwFail( kiapi::workspace::WE_INTERNAL, "The workspace has no idempotency scope" );

    auto store = std::make_shared<MDW_IDEMPOTENCY_STORE>( m_journalDir / MdwHashHex( key ) );

    if( !store->IsOpen() )
        return MdwFail( kiapi::workspace::WE_INTERNAL, "The idempotency journal cannot be opened" );

    {
        std::lock_guard<std::mutex> lock( m_contextMutex );
        m_hello = aHello;
        m_idempotency = std::move( store );
    }

    m_helloAccepted.store( true );
    return MdwOk();
}


void MDW_BACKEND_SERVER::SetDescriptor( const MDW_BACKEND_DESCRIPTOR& aDescriptor )
{
    std::lock_guard<std::mutex> lock( m_contextMutex );
    m_descriptor = aDescriptor;
}


MDW_BACKEND_DESCRIPTOR MDW_BACKEND_SERVER::Descriptor() const
{
    std::lock_guard<std::mutex> lock( m_contextMutex );
    return m_descriptor;
}


kiapi::mdw::Hello MDW_BACKEND_SERVER::HelloContext() const
{
    std::lock_guard<std::mutex> lock( m_contextMutex );
    return m_hello;
}


void MDW_BACKEND_SERVER::cancelRequest( const std::string& aRequestId )
{
    std::lock_guard<std::mutex> lock( m_impl->inFlightMutex );
    auto                        it = m_impl->inFlight.find( aRequestId );

    if( it != m_impl->inFlight.end() )
        it->second.cancelled->store( true );
}


void MDW_BACKEND_SERVER::signalByeAfterReply()
{
    t_byeAfterReply = true;
}


bool MDW_BACKEND_SERVER::waitForBye( std::chrono::milliseconds aTimeout )
{
    std::unique_lock<std::mutex> lock( m_impl->byeMutex );

    return m_impl->byeWake.wait_for( lock, aTimeout,
                                     [this]()
                                     {
                                         return m_bye.load();
                                     } );
}


bool MDW_BACKEND_SERVER::Running() const
{
    return m_impl->running.load();
}


bool MDW_BACKEND_SERVER::Start( const std::string& aControlUrl, int aWorkers )
{
    if( m_impl->running.load() )
        return true;

    if( nng_rep0_open( &m_impl->socket ) != 0 )
        return false;

    if( nng_socket_set_size( m_impl->socket, NNG_OPT_RECVMAXSZ, MDW_CONTROL_MAX_BYTES ) != 0
        || nng_listen( m_impl->socket, aControlUrl.c_str(), nullptr, 0 ) != 0 )
    {
        nng_close( m_impl->socket );
        return false;
    }

    for( int i = 0; i < std::max( aWorkers, 1 ); ++i )
    {
        nng_ctx ctx;

        if( nng_ctx_open( &ctx, m_impl->socket ) != 0 )
        {
            m_impl->contexts.clear();
            nng_close( m_impl->socket );
            return false;
        }

        m_impl->contexts.push_back( ctx );
    }

    m_impl->running.store( true );

    for( size_t i = 0; i < m_impl->contexts.size(); ++i )
        m_impl->workers.emplace_back( [this, i]() { workerLoop( i ); } );

    return true;
}


void MDW_BACKEND_SERVER::Stop()
{
    if( !m_impl->running.exchange( false ) )
        return;

    // Left running, a long handler outlasts the client's wait and is killed halfway through
    {
        std::lock_guard<std::mutex> lock( m_impl->inFlightMutex );

        for( auto& [requestId, entry] : m_impl->inFlight )
            entry.cancelled->store( true );
    }

    // Closing a context fails its worker's receive, or a reply it is about to send, with NNG_ECLOSED under nng's own
    // lock. The socket goes only once no worker can reach it, since a send racing its close panics nng
    for( nng_ctx ctx : m_impl->contexts )
        nng_ctx_close( ctx );

    for( std::thread& worker : m_impl->workers )
    {
        if( worker.joinable() )
            worker.join();
    }

    m_impl->workers.clear();
    m_impl->contexts.clear();
    nng_close( m_impl->socket );
}


void MDW_BACKEND_SERVER::dispatchSafely( const kiapi::mdw::MdwRequest& aRequest,
                                         kiapi::mdw::MdwResponse&      aResponse )
{
    auto cancelled = std::make_shared<std::atomic<bool>>( false );

    if( !aRequest.request_id().empty() )
    {
        std::lock_guard<std::mutex> lock( m_impl->inFlightMutex );
        IMPL::IN_FLIGHT&            entry = m_impl->inFlight[aRequest.request_id()];

        entry.deliveries++;
        cancelled = entry.cancelled;

        // Stop cancels under this lock what it finds, so one registered after it must cancel itself
        if( !m_impl->running.load() )
            cancelled->store( true );
    }

    t_cancelled = cancelled;

    try
    {
        dispatch( aRequest, aResponse );
    }
    catch( ... )
    {
        // A handler must never take down the process
        aResponse.Clear();
        aResponse.set_request_id( aRequest.request_id() );

        if( aRequest.channel_token() == m_channelToken )
            aResponse.set_channel_token( m_channelToken );

        aResponse.mutable_error()->set_code( kiapi::workspace::WE_INTERNAL );
        aResponse.mutable_error()->set_message( "Handler raised an exception" );
    }

    if( !aRequest.request_id().empty() )
    {
        std::lock_guard<std::mutex> lock( m_impl->inFlightMutex );
        auto                        it = m_impl->inFlight.find( aRequest.request_id() );

        if( it != m_impl->inFlight.end() && --it->second.deliveries == 0 )
            m_impl->inFlight.erase( it );
    }

    t_cancelled.reset();
}


void MDW_BACKEND_SERVER::dispatch( const kiapi::mdw::MdwRequest& aRequest,
                                   kiapi::mdw::MdwResponse&      aResponse )
{
    aResponse.set_request_id( aRequest.request_id() );

    // A sender that did not present the token must never learn it from the reply
    if( !m_channelToken.empty() && aRequest.channel_token() != m_channelToken )
    {
        aResponse.mutable_error()->set_code( kiapi::workspace::WE_PROTOCOL );
        aResponse.mutable_error()->set_message( "Channel token mismatch" );
        return;
    }

    aResponse.set_channel_token( m_channelToken );

    const std::string& typeUrl = aRequest.payload().type_url();
    std::string::size_type slash = typeUrl.find_last_of( '/' );
    std::string typeName = slash == std::string::npos ? typeUrl : typeUrl.substr( slash + 1 );

    // Whatever its idempotency_id, nothing may run before the journal it would need exists
    if( !m_journalDir.empty() && !m_helloAccepted.load() && !answersBeforeHello( typeName ) )
    {
        aResponse.mutable_error()->set_code( kiapi::workspace::WE_PROTOCOL );
        aResponse.mutable_error()->set_message( "Hello first" );
        return;
    }

    auto it = m_handlers.find( typeName );

    // The client words an unsupported request itself; the type name is only for whoever reads the log
    if( it == m_handlers.end() )
    {
        std::fprintf( stderr, "kicad-mdw: no handler for %s\n", typeName.c_str() );
        aResponse.mutable_error()->set_code( kiapi::workspace::WE_NOT_SUPPORTED );
        return;
    }

    if( m_filter )
    {
        if( std::optional<kiapi::workspace::WorkspaceError> refused = m_filter( typeName, aRequest ) )
        {
            *aResponse.mutable_error() = *refused;
            return;
        }
    }

    std::shared_ptr<MDW_IDEMPOTENCY_STORE> journal;

    {
        std::lock_guard<std::mutex> lock( m_contextMutex );
        journal = m_idempotency;
    }

    const HANDLER& handler = it->second;
    const std::string operationId = handler.idempotencyId ? handler.idempotencyId( aRequest.payload() ) : std::string();
    std::string       id = journal ? operationId : std::string();

    // The client matches progress to its op by the idempotency id, which a read has none of
    std::shared_ptr<MDW_REQUEST_PROGRESS> progress;

    if( m_publisher && !operationId.empty() )
    {
        progress = std::make_shared<MDW_REQUEST_PROGRESS>();
        progress->publisher = m_publisher;
        progress->workspaceId = HelloContext().workspace().workspace_id();
        progress->operationId = operationId;
    }

    MDW_REQUEST_PROGRESS_SCOPE progressScope( std::move( progress ) );

    if( id.empty() )
    {
        handler.run( aRequest.payload(), std::string(), aResponse );
        return;
    }

    MDW_IDEMPOTENCY_STORE::RECOVERY recovery;

    if( handler.token && handler.recover )
    {
        recovery.token = [&]( std::string& aToken, kiapi::mdw::MdwResponse& aOut )
        {
            return handler.token( aRequest.payload(), aToken, aOut );
        };

        recovery.recover = [&]( const std::string& aToken, kiapi::mdw::MdwResponse& aOut )
        {
            return handler.recover( aRequest.payload(), aToken, aOut );
        };
    }

    // Keyed by type too, so one id reused across request types never replays the wrong reply
    journal->Run(
            typeName + "/" + id, aRequest.payload().value(), aResponse,
            [&]( const std::string& aToken )
            {
                handler.run( aRequest.payload(), aToken, aResponse );
            },
            recovery );
}


void MDW_BACKEND_SERVER::workerLoop( size_t aContext )
{
    nng_ctx  ctx = m_impl->contexts[aContext];
    nng_aio* aio = nullptr;

    if( nng_aio_alloc( &aio, nullptr, nullptr ) != 0 )
        return;

    while( m_impl->running.load() )
    {
        nng_ctx_recv( ctx, aio );
        nng_aio_wait( aio );

        if( nng_aio_result( aio ) != 0 )
        {
            // NNG_ECLOSED on shutdown; a persistent error otherwise must not spin
            // this worker at full core, so back off before retrying
            if( !m_impl->running.load() )
                break;

            std::this_thread::sleep_for( std::chrono::milliseconds( 10 ) );
            continue;
        }

        nng_msg* msg = nng_aio_get_msg( aio );

        kiapi::mdw::MdwRequest  requestEnvelope;
        kiapi::mdw::MdwResponse responseEnvelope;

        bool parsed = requestEnvelope.ParseFromArray( nng_msg_body( msg ),
                                                      static_cast<int>( nng_msg_len( msg ) ) );
        nng_msg_free( msg );

        t_byeAfterReply = false;

        if( parsed )
        {
            dispatchSafely( requestEnvelope, responseEnvelope );
        }
        else
        {
            responseEnvelope.mutable_error()->set_code( kiapi::workspace::WE_PROTOCOL );
            responseEnvelope.mutable_error()->set_message( "Malformed request envelope" );
        }

        std::string out;

        // The receiving transport drops an oversize reply, so the caller would only see a timeout
        if( !responseEnvelope.SerializeToString( &out ) || out.size() > MDW_CONTROL_MAX_BYTES )
        {
            kiapi::mdw::MdwResponse refused;
            refused.set_channel_token( responseEnvelope.channel_token() );
            refused.set_request_id( requestEnvelope.request_id() );
            refused.mutable_error()->set_code( kiapi::workspace::WE_INVALID_PARAMETER );
            refused.mutable_error()->set_message( "Reply exceeds the message size limit" );
            out = refused.SerializeAsString();
        }

        // The REP context owes a reply once a request was received; never skip
        // the send, or the next receive fails NNG_ESTATE and the worker spins
        nng_msg* reply = nullptr;

        while( nng_msg_alloc( &reply, 0 ) != 0 )
        {
            if( !m_impl->running.load() )
                break;

            std::this_thread::sleep_for( std::chrono::milliseconds( 10 ) );
        }

        if( !reply )
            break;

        nng_msg_append( reply, out.data(), out.size() );
        nng_aio_set_msg( aio, reply );
        nng_ctx_send( ctx, aio );
        nng_aio_wait( aio );

        if( nng_aio_result( aio ) != 0 )
        {
            if( nng_msg* unsent = nng_aio_get_msg( aio ) )
                nng_msg_free( unsent );

            // The aio outlives the attempt, so the freed message must not stay
            // referenced by it even though the next send overwrites the slot
            nng_aio_set_msg( aio, nullptr );
        }

        // Signalled even when the send failed, since the peer asked to stop either way
        if( t_byeAfterReply )
        {
            std::lock_guard<std::mutex> lock( m_impl->byeMutex );
            m_bye.store( true );
            m_impl->byeWake.notify_all();
        }
    }

    nng_aio_free( aio );
}
