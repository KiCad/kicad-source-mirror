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

#include <mdw_runtime/mdw_event_publisher.h>
#include <mdw_runtime/mdw_backend_server.h>

#include <api/mdw/mdw_envelope.pb.h>

#include <nng/nng.h>
#include <nng/protocol/reqrep0/req.h>

#include <algorithm>
#include <chrono>
#include <map>
#include <cstdint>
#include <cstring>
#include <utility>

namespace
{
/// The aio is reused across attempts, so a stale pointer must not outlive the message
void releaseMsg( nng_aio* aAio )
{
    if( nng_msg* msg = nng_aio_get_msg( aAio ) )
        nng_msg_free( msg );

    nng_aio_set_msg( aAio, nullptr );
}
}


struct MDW_EVENT_PUBLISHER::IMPL
{
    /// An event, serialized once when queued
    struct QUEUED
    {
        std::uint64_t seq = 0;
        std::string   bytes;
        std::string   transientKey; ///< The operation of a progress report, empty for a numbered event
        std::string   phase;

        /// True when aOther is a progress report this one may replace
        bool Supersedes( const QUEUED& aOther ) const
        {
            return !transientKey.empty() && transientKey == aOther.transientKey && phase == aOther.phase;
        }
    };

    nng_socket                           socket = NNG_SOCKET_INITIALIZER;
    nng_aio*                             aio = nullptr; ///< Reused by the worker for every attempt
    std::atomic<bool>                    running{ false };
    std::mutex                           mutex;
    std::condition_variable              cv;
    std::deque<QUEUED>                   queue;
    std::map<std::string, std::uint64_t> lastSeq; ///< Per workspace, since gaps are checked per workspace
    std::thread                          worker;

    /// True when a report waiting in the queue would replace aEvent
    bool Superseded( const QUEUED& aEvent )
    {
        std::lock_guard<std::mutex> lock( mutex );

        return std::any_of( queue.begin(), queue.end(),
                            [&]( const QUEUED& aWaiting )
                            {
                                return aWaiting.Supersedes( aEvent );
                            } );
    }
};


MDW_EVENT_PUBLISHER::MDW_EVENT_PUBLISHER( const std::string& aChannelToken ) :
        m_channelToken( aChannelToken ),
        m_impl( std::make_unique<IMPL>() )
{
}


MDW_EVENT_PUBLISHER::~MDW_EVENT_PUBLISHER()
{
    Stop();
}


bool MDW_EVENT_PUBLISHER::Start( const std::string& aEventsUrl )
{
    if( m_impl->running.load() )
        return true;

    if( nng_req0_open( &m_impl->socket ) != 0 )
        return false;

    // Non-blocking dial tolerates the event server binding slightly later
    if( nng_socket_set_size( m_impl->socket, NNG_OPT_RECVMAXSZ, MDW_CONTROL_MAX_BYTES ) != 0
        || nng_dial( m_impl->socket, aEventsUrl.c_str(), nullptr, NNG_FLAG_NONBLOCK ) != 0
        || nng_aio_alloc( &m_impl->aio, nullptr, nullptr ) != 0 )
    {
        nng_close( m_impl->socket );
        return false;
    }

    m_impl->running.store( true );
    m_impl->worker = std::thread( [this]() { publishLoop(); } );

    return true;
}


void MDW_EVENT_PUBLISHER::Stop()
{
    if( !m_impl->running.exchange( false ) )
        return;

    // Close the socket before joining so a worker blocked in nng_aio_wait aborts
    // at once rather than waiting out the per-message timeout
    nng_close( m_impl->socket );
    m_impl->cv.notify_all();

    if( m_impl->worker.joinable() )
        m_impl->worker.join();

    nng_aio_free( m_impl->aio );
    m_impl->aio = nullptr;
}


bool MDW_EVENT_PUBLISHER::Publish( const kiapi::workspace::EventEnvelope& aEvent, const std::string& aOperationId )
{
    if( aEvent.type() == kiapi::workspace::ET_PROGRESS )
        return publishProgress( aEvent, aOperationId );

    if( !m_impl->running.load() )
        return false;

    kiapi::mdw::PublishEvent publish;
    publish.set_channel_token( m_channelToken );
    *publish.mutable_event() = aEvent;

    if( !aOperationId.empty() )
        publish.mutable_event()->set_operation_id( aOperationId );

    {
        // Numbering under the queue lock keeps sequence order and delivery order the same
        std::lock_guard<std::mutex> lock( m_impl->mutex );
        IMPL::QUEUED                queued;
        std::uint64_t&              lastSeq = m_impl->lastSeq[aEvent.workspace_id()];

        queued.seq = lastSeq + 1;
        publish.mutable_event()->set_seq( queued.seq );

        // Number only an event that will be sent, since a skipped number reads as a lost event
        if( !publish.SerializeToString( &queued.bytes ) || queued.bytes.size() > MDW_CONTROL_MAX_BYTES )
            return false;

        lastSeq = queued.seq;
        m_impl->queue.push_back( std::move( queued ) );
    }

    m_impl->cv.notify_all();

    return true;
}


bool MDW_EVENT_PUBLISHER::publishProgress( const kiapi::workspace::EventEnvelope& aEvent,
                                           const std::string&                     aOperationId )
{
    if( !m_impl->running.load() || aOperationId.empty() )
        return false;

    kiapi::mdw::PublishEvent publish;
    publish.set_channel_token( m_channelToken );
    *publish.mutable_event() = aEvent;
    publish.mutable_event()->set_operation_id( aOperationId );
    publish.mutable_event()->set_seq( 0 );

    IMPL::QUEUED queued;
    queued.transientKey = aOperationId;
    queued.phase = aEvent.progress().phase();

    if( !publish.SerializeToString( &queued.bytes ) || queued.bytes.size() > MDW_CONTROL_MAX_BYTES )
        return false;

    {
        std::lock_guard<std::mutex> lock( m_impl->mutex );
        auto                        newest = std::find_if( m_impl->queue.rbegin(), m_impl->queue.rend(),
                                                           [&]( const IMPL::QUEUED& aWaiting )
                                                           {
                                                               return aWaiting.transientKey == aOperationId;
                                                           } );

        // A later phase queues behind, so a slow client still sees where each phase ended
        if( newest != m_impl->queue.rend() && queued.Supersedes( *newest ) )
            *newest = std::move( queued );
        else
            m_impl->queue.push_back( std::move( queued ) );
    }

    m_impl->cv.notify_all();

    return true;
}


void MDW_EVENT_PUBLISHER::publishLoop()
{
    nng_aio* aio = m_impl->aio;

    while( true )
    {
        IMPL::QUEUED event;

        {
            std::unique_lock<std::mutex> lock( m_impl->mutex );
            m_impl->cv.wait( lock, [this]()
                             { return !m_impl->running.load() || !m_impl->queue.empty(); } );

            if( !m_impl->running.load() && m_impl->queue.empty() )
                break;

            if( m_impl->queue.empty() )
                continue;

            event = std::move( m_impl->queue.front() );
            m_impl->queue.pop_front();
        }

        bool delivered = false;
        bool retried = false;

        // The receiver dedups any duplicate delivery by sequence number
        while( m_impl->running.load() && !delivered )
        {
            nng_msg* msg = nullptr;

            // Retry rather than drop, since a dropped event leaves a gap in the sequence
            if( nng_msg_alloc( &msg, event.bytes.size() ) != 0 )
            {
                std::this_thread::sleep_for( std::chrono::milliseconds( 200 ) );
                continue;
            }

            std::memcpy( nng_msg_body( msg ), event.bytes.data(), event.bytes.size() );

            nng_aio_set_timeout( aio, 2000 );
            nng_aio_set_msg( aio, msg );
            nng_send_aio( m_impl->socket, aio );
            nng_aio_wait( aio );

            // A failed send hands the message back; a successful exchange leaves
            // the acknowledgement. Either way the message is ours to release
            if( nng_aio_result( aio ) != 0 )
            {
                releaseMsg( aio );
            }
            else
            {
                nng_recv_aio( m_impl->socket, aio );
                nng_aio_wait( aio );

                if( nng_aio_result( aio ) == 0 )
                {
                    nng_msg*                    reply = nng_aio_get_msg( aio );
                    kiapi::mdw::PublishEventAck ack;

                    // Any other reply, an empty one included, is not proof this event arrived
                    delivered = reply
                                && ack.ParseFromArray( nng_msg_body( reply ), static_cast<int>( nng_msg_len( reply ) ) )
                                && ack.channel_token() == m_channelToken && ack.seq() == event.seq;

                    releaseMsg( aio );
                }
            }

            // A report that missed its turn gets one more try unless a newer one of its phase is waiting
            if( !delivered && !event.transientKey.empty() && ( retried || m_impl->Superseded( event ) ) )
                break;

            retried = !delivered;

            if( !delivered && m_impl->running.load() )
                std::this_thread::sleep_for( std::chrono::milliseconds( 200 ) );
        }
    }
}
