/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2023 Jon Evans <jon@craftyjon.com>
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
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

#include <kinng.h>
#include <nng/nng.h>
#include <nng/protocol/reqrep0/rep.h>
#include <nng/protocol/reqrep0/req.h>
#include <nng/supplemental/util/platform.h>
#include <wx/debug.h>
#include <wx/log.h>

#include <future>
#include <utility>


/**
 * Trace nng server debug output
 * @ingroup trace_env_vars
 */
static const wxChar TraceNng[] = wxT( "KINNG" );

// The endpoint whose listener or reaper this thread is, so its Stop fails loudly instead of joining itself
static thread_local const void* s_endpointOfThisThread = nullptr;


KINNG_REQUEST_SERVER::KINNG_REQUEST_SERVER( const std::string& aSocketUrl, std::size_t aMaxMessageBytes ) :
        m_socketUrl( aSocketUrl ),
        m_maxMessageBytes( aMaxMessageBytes ),
        m_callback()
{
}


KINNG_REQUEST_SERVER::~KINNG_REQUEST_SERVER()
{
    Stop();
}


bool KINNG_REQUEST_SERVER::Running() const
{
    return m_thread.joinable();
}


bool KINNG_REQUEST_SERVER::Start()
{
    if( m_thread.joinable() )
        return true;

    m_shutdown.store( false );
    m_pendingReply.clear();
    m_hasReply = false;
    m_thread = std::thread( [&]() { listenThread(); } );
    return true;
}


void KINNG_REQUEST_SERVER::Stop()
{
    wxCHECK_RET( s_endpointOfThisThread != this,
                 wxS( "KINNG_REQUEST_SERVER joins its listener, so it must not be stopped, nor destroyed, from "
                      "the callback" ) );

    if( !m_thread.joinable() )
        return;

    {
        // Set under the lock so the listener cannot test the flag and then miss this wakeup
        std::lock_guard<std::mutex> lock( m_mutex );
        m_shutdown.store( true );
    }

    m_replyReady.notify_all();
    m_thread.join();
}


void KINNG_REQUEST_SERVER::Reply( const std::string& aReply )
{
    std::lock_guard<std::mutex> lock( m_mutex );
    m_pendingReply = aReply;
    m_hasReply = true;
    m_replyReady.notify_all();
}


void KINNG_REQUEST_SERVER::listenThread()
{
    nng_socket   socket;
    nng_listener listener;
    int          retCode = 0;

    s_endpointOfThisThread = this;

    wxLogTrace( TraceNng, wxS( "KINNG_REQUEST_SERVER starting" ) );

    retCode = nng_rep0_open( &socket );

    if( retCode != 0 )
    {
        wxLogTrace( TraceNng,
                    wxString::Format( wxS( "Got error code %d from nng_rep0_open!" ), retCode ) );
        return;
    }

    if( m_maxMessageBytes )
        nng_socket_set_size( socket, NNG_OPT_RECVMAXSZ, m_maxMessageBytes );

    retCode = nng_listener_create( &listener, socket, m_socketUrl.c_str() );

    if( retCode != 0 )
    {
        wxLogTrace( TraceNng,
                    wxString::Format( wxS( "Got error code %d from nng_listener_create!" ),
                                      retCode ) );
        nng_close( socket );
        return;
    }

    // Only this thread touches the socket, since nng can free it under a thread still expiring a receive
    // Stop therefore waits out this timeout rather than aborting the receive
    nng_socket_set_ms( socket, NNG_OPT_RECVTIMEO, 100 );

    retCode = nng_listener_start( listener, 0 );

    if( retCode != 0 )
    {
        wxLogTrace( TraceNng,
                    wxString::Format( wxS( "Got error code %d from nng_listener_start!" ),
                                      retCode ) );
        nng_close( socket );
        return;
    }

    wxLogTrace( TraceNng, wxS( "KINNG_REQUEST_SERVER listener has started" ) );

    while( !m_shutdown.load() )
    {
        char*  buf = nullptr;
        size_t sz = 0;

        retCode = nng_recv( socket, &buf, &sz, NNG_FLAG_ALLOC );

        if( retCode == NNG_ETIMEDOUT )
            continue;

        if( retCode != 0 )
        {
            wxLogTrace( TraceNng, wxString::Format( wxS( "Got error code %d from nng_recv!" ), retCode ) );
            break;
        }

        m_sharedMessage.assign( buf, sz );
        nng_free( buf, sz );

        if( m_callback )
            m_callback( &m_sharedMessage );

        std::unique_lock<std::mutex> lock( m_mutex );
        m_replyReady.wait( lock, [&]() { return m_shutdown.load() || m_hasReply; } );

        if( m_shutdown.load() )
            break;

        retCode = nng_send( socket, const_cast<std::string::value_type*>( m_pendingReply.c_str() ),
                            m_pendingReply.length(), 0 );

        if( retCode != 0 )
        {
            wxLogTrace( TraceNng,
                        wxString::Format( wxS( "Got error code %d from nng_send!" ), retCode ) );
        }

        m_pendingReply.clear();
        m_hasReply = false;
    }

    wxLogTrace( TraceNng, wxS( "KINNG_REQUEST_SERVER shutting down" ) );

    nng_close( socket );
}


static KINNG_REQUEST_CLIENT::RESULT makeErrorResult( int aNngError )
{
    using STATUS = KINNG_REQUEST_CLIENT::STATUS;

    switch( aNngError )
    {
    case NNG_ETIMEDOUT: return { STATUS::TIMED_OUT, {} };
    case NNG_ECANCELED: return { STATUS::CANCELLED, {} };
    case NNG_ECLOSED:   return { STATUS::CLOSED, {} };
    default:            return { STATUS::FAILED, {} };
    }
}


struct KINNG_REQUEST_CLIENT::REQUEST
{
    KINNG_REQUEST_CLIENT* client = nullptr;
    std::uint64_t         id = 0;
    nng_ctx               ctx = NNG_CTX_INITIALIZER;
    nng_aio*              aio = nullptr;
    ON_COMPLETE           callback;
    nng_time              deadline = 0;     ///< Absolute expiry shared by send and receive, 0 for none
    bool                  sending = true;
    std::atomic<bool>     withdrawn{ false }; ///< Cancel() was called, so a later Stop() is not reported as closed
    RESULT                result;
};


struct KINNG_REQUEST_CLIENT::IMPL
{
    nng_socket                        socket = NNG_SOCKET_INITIALIZER;
    std::atomic<bool>                 running{ false };
    std::atomic<std::uint64_t>        nextId{ 0 };
    std::mutex                        lifecycle;   ///< Serializes Start and Stop against each other
    std::mutex                        mutex;       ///< Guards outstanding, completed, and the cv
    std::condition_variable           cv;
    std::map<std::uint64_t, REQUEST*> outstanding;
    std::vector<REQUEST*>             completed;
    std::thread                       reaper;
};


KINNG_REQUEST_CLIENT::KINNG_REQUEST_CLIENT( const std::string& aSocketUrl, std::size_t aMaxMessageBytes ) :
        m_socketUrl( aSocketUrl ),
        m_maxMessageBytes( aMaxMessageBytes ),
        m_impl( std::make_unique<IMPL>() )
{
}


KINNG_REQUEST_CLIENT::~KINNG_REQUEST_CLIENT()
{
    Stop();
}


void KINNG_REQUEST_CLIENT::SetReconnect( int aMinMs, int aMaxMs )
{
    m_reconnectMinMs = aMinMs;
    m_reconnectMaxMs = aMaxMs;
}


bool KINNG_REQUEST_CLIENT::Start()
{
    std::lock_guard<std::mutex> lifecycle( m_impl->lifecycle );

    if( m_impl->running.load() )
        return true;

    int retCode = nng_req0_open( &m_impl->socket );

    if( retCode != 0 )
    {
        wxLogTrace( TraceNng,
                    wxString::Format( wxS( "Got error code %d from nng_req0_open!" ), retCode ) );
        return false;
    }

    if( m_maxMessageBytes )
        nng_socket_set_size( m_impl->socket, NNG_OPT_RECVMAXSZ, m_maxMessageBytes );

    // nng resends an unanswered request after a minute and then every second, and the peer runs each copy
    // A dropped pipe still resends, so only the timer goes
    nng_socket_set_ms( m_impl->socket, NNG_OPT_REQ_RESENDTIME, NNG_DURATION_INFINITE );

    if( m_reconnectMinMs > 0 )
    {
        nng_socket_set_ms( m_impl->socket, NNG_OPT_RECONNMINT, m_reconnectMinMs );
        nng_socket_set_ms( m_impl->socket, NNG_OPT_RECONNMAXT, m_reconnectMaxMs );
    }

    // Dial non-blocking to tolerate a peer that binds later, such as a just-spawned backend
    retCode = nng_dial( m_impl->socket, m_socketUrl.c_str(), nullptr, NNG_FLAG_NONBLOCK );

    if( retCode != 0 )
    {
        wxLogTrace( TraceNng,
                    wxString::Format( wxS( "Got error code %d from nng_dial!" ), retCode ) );
        nng_close( m_impl->socket );
        return false;
    }

    {
        std::lock_guard<std::mutex> lock( m_impl->mutex );
        m_impl->running.store( true );
    }

    m_impl->reaper = std::thread( [this]() { reaperThread(); } );

    return true;
}


void KINNG_REQUEST_CLIENT::Stop()
{
    // Before the lock, which a Stop on another thread holds while it joins this reaper
    wxCHECK_RET( s_endpointOfThisThread != this,
                 wxS( "KINNG_REQUEST_CLIENT joins its reaper, so it must not be stopped, nor destroyed, from an "
                      "ON_COMPLETE" ) );

    std::lock_guard<std::mutex> lifecycle( m_impl->lifecycle );

    {
        // Under the reaper's mutex, or its wait can miss the notify below and the join hangs
        std::lock_guard<std::mutex> lock( m_impl->mutex );

        if( !m_impl->running.exchange( false ) )
            return;

        // nng_close does not wait for a thread expiring a request, so end each one here and let the reaper
        // free its aio, which does wait, before the close
        for( const auto& [id, request] : m_impl->outstanding )
            nng_aio_abort( request->aio, NNG_ECLOSED );
    }

    m_impl->cv.notify_all();

    if( m_impl->reaper.joinable() )
        m_impl->reaper.join();

    nng_close( m_impl->socket );
}


std::uint64_t KINNG_REQUEST_CLIENT::Request( const std::string& aPayload, int aTimeoutMs,
                                             ON_COMPLETE aOnComplete, const ON_SEND& aOnSend )
{
    REQUEST* req = new REQUEST();
    req->client = this;
    req->callback = std::move( aOnComplete );

    int retCode = NNG_ECLOSED;

    {
        // Start replaces the socket only while not running, and Start and Stop change running under this mutex
        std::lock_guard<std::mutex> lock( m_impl->mutex );

        if( m_impl->running.load() )
            retCode = nng_ctx_open( &req->ctx, m_impl->socket );
    }

    if( retCode != 0 )
    {
        delete req;
        return 0;
    }

    // nng holds the request only as a void*, so this guard frees whatever was built until the handoff
    struct CLEANUP
    {
        ~CLEANUP()
        {
            if( !request )
                return;

            if( request->aio )
            {
                if( nng_msg* msg = nng_aio_get_msg( request->aio ) )
                    nng_msg_free( msg );

                nng_aio_free( request->aio );
            }

            nng_ctx_close( request->ctx );
            delete request;
        }

        void Release() { request = nullptr; }

        REQUEST* request;
    } cleanup{ req };

    retCode = nng_aio_alloc( &req->aio, &KINNG_REQUEST_CLIENT::aioCallback, req );

    if( retCode != 0 )
        return 0;

    if( aTimeoutMs > 0 )
    {
        req->deadline = nng_clock() + aTimeoutMs;
        nng_aio_set_expire( req->aio, req->deadline );
    }

    nng_msg* msg = nullptr;
    retCode = nng_msg_alloc( &msg, 0 );

    if( retCode != 0 )
        return 0;

    if( !aPayload.empty() )
        nng_msg_append( msg, aPayload.data(), aPayload.size() );

    nng_aio_set_msg( req->aio, msg );

    // Sent under the lock, so a concurrent Stop either finds the send to abort or rejects the request
    // nng finishes a send on its own threads, never inline, so this lock cannot deadlock the callback
    std::lock_guard<std::mutex> lock( m_impl->mutex );

    if( !m_impl->running.load() )
        return 0;

    // Before the request is recorded, so a throwing hook leaves nothing for Stop to wait on
    if( aOnSend )
        aOnSend();

    std::uint64_t id = ++m_impl->nextId;
    req->id = id;
    m_impl->outstanding[id] = req;
    cleanup.Release();
    nng_ctx_send( req->ctx, req->aio );

    return id;
}


void KINNG_REQUEST_CLIENT::Cancel( std::uint64_t aRequestId )
{
    std::lock_guard<std::mutex> lock( m_impl->mutex );
    auto                        it = m_impl->outstanding.find( aRequestId );

    if( it != m_impl->outstanding.end() )
    {
        it->second->withdrawn = true;
        nng_aio_cancel( it->second->aio );
    }
}


KINNG_REQUEST_CLIENT::RESULT KINNG_REQUEST_CLIENT::RequestSync( const std::string& aPayload,
                                                               int aTimeoutMs, const ON_SEND& aOnSend )
{
    std::promise<RESULT> promise;
    std::future<RESULT>  future = promise.get_future();

    std::uint64_t id = Request( aPayload, aTimeoutMs,
                                [&promise]( const RESULT& aResult ) { promise.set_value( aResult ); }, aOnSend );

    if( id == 0 )
        return makeErrorResult( NNG_ECLOSED );

    return future.get();
}


void KINNG_REQUEST_CLIENT::aioCallback( void* aArg )
{
    REQUEST* req = static_cast<REQUEST*>( aArg );
    int      retCode = nng_aio_result( req->aio );

    if( req->sending )
    {
        if( retCode != 0 )
        {
            // A failed send leaves our message on the aio
            if( nng_msg* msg = nng_aio_get_msg( req->aio ) )
                nng_msg_free( msg );

            req->result = makeErrorResult( retCode );
            req->client->finishRequest( req );
            return;
        }

        bool withdrawn = false;

        {
            // Cancel() and Stop() hold this mutex, so either finds the receive scheduled or it is never issued
            std::lock_guard<std::mutex> lock( req->client->m_impl->mutex );
            req->sending = false;
            withdrawn = req->withdrawn;

            if( !withdrawn && req->client->m_impl->running.load() )
            {
                // Each operation consumes the expiry, so the receive gets whatever the send left
                if( req->deadline )
                    nng_aio_set_expire( req->aio, req->deadline );

                nng_ctx_recv( req->ctx, req->aio );
                return;
            }
        }

        req->result = makeErrorResult( withdrawn ? NNG_ECANCELED : NNG_ECLOSED );
        req->client->finishRequest( req );
        return;
    }

    if( retCode != 0 )
    {
        req->result = makeErrorResult( retCode );
        req->client->finishRequest( req );
        return;
    }

    if( nng_msg* msg = nng_aio_get_msg( req->aio ) )
    {
        req->result.status = STATUS::RECEIVED;
        req->result.reply.assign( static_cast<const char*>( nng_msg_body( msg ) ),
                                  nng_msg_len( msg ) );
        nng_msg_free( msg );
    }
    else
    {
        // A successful receive without a message is an nng anomaly
        req->result = makeErrorResult( NNG_EINTERNAL );
    }

    req->client->finishRequest( req );
}


void KINNG_REQUEST_CLIENT::finishRequest( REQUEST* aRequest )
{
    // A caller that let go sees its cancel, never the close that happened to end the request
    if( aRequest->withdrawn && aRequest->result.status == STATUS::CLOSED )
        aRequest->result.status = STATUS::CANCELLED;

    {
        std::lock_guard<std::mutex> lock( m_impl->mutex );
        m_impl->outstanding.erase( aRequest->id );
        m_impl->completed.push_back( aRequest );
    }

    m_impl->cv.notify_all();
}


void KINNG_REQUEST_CLIENT::reaperThread()
{
    s_endpointOfThisThread = this;

    std::unique_lock<std::mutex> lock( m_impl->mutex );

    while( true )
    {
        m_impl->cv.wait( lock, [this]()
                         {
                             return !m_impl->completed.empty()
                                    || ( !m_impl->running.load() && m_impl->outstanding.empty() );
                         } );

        // Taking the whole batch keeps completion order and cycles the lock once per batch
        std::vector<REQUEST*> batch = std::exchange( m_impl->completed, {} );

        if( !batch.empty() )
        {
            lock.unlock();

            for( REQUEST* req : batch )
            {
                // nng_aio_free deadlocks when called from its own callback
                nng_aio_free( req->aio );
                nng_ctx_close( req->ctx );

                if( req->callback )
                    req->callback( req->result );

                delete req;
            }

            lock.lock();
        }

        if( !m_impl->running.load() && m_impl->outstanding.empty() && m_impl->completed.empty() )
            break;
    }
}
