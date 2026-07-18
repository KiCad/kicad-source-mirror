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

#include "mdw_op_state.h"

#include <mdw/mdw_serial_worker.h>
#include <trace_helpers.h>

#include <condition_variable>
#include <map>
#include <optional>
#include <thread>
#include <utility>

#include <wx/app.h>
#include <wx/debug.h>
#include <wx/log.h>
#include <wx/thread.h>
#include <wx/translation.h>
#include <wx/intl.h>


namespace
{

std::mutex                         s_workerMutex;
std::unique_ptr<MDW_SERIAL_WORKER> s_worker;
std::thread::id                    s_workerThread;
std::unique_ptr<MDW_SERIAL_WORKER> s_settler;
std::atomic<bool>                    s_closed = false;
thread_local int                   s_syncScopes = 0;
thread_local const MDW_CANCEL_SCOPE* s_cancelScope = nullptr;
thread_local const MDW_PROGRESS_SCOPE* s_progressScope = nullptr;

/// The lock and signal of each op a thread waits on, so stopping the workers wakes it. Guarded by s_workerMutex
std::multimap<std::mutex*, std::condition_variable*> s_waiting;


void postToWorker( std::function<void()> aTask )
{
    std::lock_guard<std::mutex> lock( s_workerMutex );

    if( s_closed )
        return;

    if( !s_worker )
    {
        s_worker = std::make_unique<MDW_SERIAL_WORKER>();
        s_worker->Post(
                []()
                {
                    std::lock_guard<std::mutex> workerLock( s_workerMutex );
                    s_workerThread = std::this_thread::get_id();
                } );
    }

    s_worker->Post( std::move( aTask ) );
}


bool deliversOnUiThread()
{
    // Without an app there is no UI loop to deliver on, so headless callers always get the worker
    return wxTheApp != nullptr;
}


bool onWorkerThread()
{
    std::lock_guard<std::mutex> lock( s_workerMutex );
    return s_worker && s_workerThread == std::this_thread::get_id();
}

} // namespace


bool MdwStopWorkers( std::chrono::milliseconds aTimeout )
{
    std::unique_ptr<MDW_SERIAL_WORKER> workers[2];

    {
        std::lock_guard<std::mutex> lock( s_workerMutex );

        s_closed = true;
        workers[0] = std::move( s_worker );
        workers[1] = std::move( s_settler );

        for( const auto& [mutex, cv] : s_waiting )
        {
            std::lock_guard<std::mutex> waiter( *mutex );
            cv->notify_all();
        }
    }

    bool stopped = true;

    for( std::unique_ptr<MDW_SERIAL_WORKER>& worker : workers )
    {
        // A task stuck behind the UI keeps its thread, which only a fast exit can leave safely
        if( worker && !worker->Stop( aTimeout ) )
        {
            MDW_SERIAL_WORKER* running = worker.release();
            wxLogTrace( traceMdw, wxS( "Workspace worker %p still running at shutdown" ), running );
            stopped = false;
        }
    }

    return stopped;
}


void MdwStartWorkers()
{
    std::lock_guard<std::mutex> lock( s_workerMutex );
    s_closed = false;
}


MDW_SYNC_SCOPE::MDW_SYNC_SCOPE()
{
    ++s_syncScopes;
}


MDW_SYNC_SCOPE::~MDW_SYNC_SCOPE()
{
    --s_syncScopes;
}


bool MDW_SYNC_SCOPE::IsOpen()
{
    return s_syncScopes > 0;
}


MDW_CANCEL_SCOPE::MDW_CANCEL_SCOPE( std::shared_ptr<const std::atomic<bool>> aCancelled ) :
        m_cancelled( std::move( aCancelled ) ),
        m_outer( s_cancelScope )
{
    s_cancelScope = this;
}


MDW_CANCEL_SCOPE::~MDW_CANCEL_SCOPE()
{
    s_cancelScope = m_outer;
}


bool MDW_CANCEL_SCOPE::IsOpen()
{
    return s_cancelScope != nullptr;
}


bool MDW_CANCEL_SCOPE::Requested()
{
    for( const MDW_CANCEL_SCOPE* scope = s_cancelScope; scope; scope = scope->m_outer )
    {
        if( scope->m_cancelled && scope->m_cancelled->load() )
            return true;
    }

    return false;
}


MDW_PROGRESS_SCOPE::MDW_PROGRESS_SCOPE( SINK aSink ) :
        m_sink( std::move( aSink ) ),
        m_outer( s_progressScope )
{
    s_progressScope = this;
}


MDW_PROGRESS_SCOPE::~MDW_PROGRESS_SCOPE()
{
    s_progressScope = m_outer;
}


MDW_PROGRESS_SCOPE::SINK MDW_PROGRESS_SCOPE::Current()
{
    return s_progressScope ? s_progressScope->m_sink : SINK();
}


wxString MdwProgressText( const MDW_PROGRESS& aProgress )
{
    wxString what = aProgress.phase;

    if( aProgress.phase == wxS( "receive" ) )
        what = _( "Receiving objects" );
    else if( aProgress.phase == wxS( "checkout" ) )
        what = _( "Writing files" );
    else if( aProgress.phase == wxS( "index" ) )
        what = _( "Reading libraries" );

    if( aProgress.total == 0 )
        return what;

    return wxString::Format( _( "%s: %llu of %llu" ), what, static_cast<unsigned long long>( aProgress.done ),
                             static_cast<unsigned long long>( aProgress.total ) );
}


MDW_OP_STATE::MDW_OP_STATE() :
        m_direct( MDW_SYNC_SCOPE::IsOpen() )
{
}


bool MDW_OP_STATE::onDeliveryThread() const
{
    if( !m_direct && deliversOnUiThread() )
        return wxIsMainThread();

    return onWorkerThread();
}


void MDW_OP_STATE::Settle( std::function<void()> aTask )
{
    std::lock_guard<std::mutex> lock( s_workerMutex );

    if( s_closed )
        return;

    if( !s_settler )
        s_settler = std::make_unique<MDW_SERIAL_WORKER>();

    s_settler->Post( std::move( aTask ) );
}


void MDW_OP_STATE::SetCanceller( std::function<void()> aCanceller )
{
    std::lock_guard<std::mutex> lock( m_mutex );
    m_canceller = std::move( aCanceller );
}


void MDW_OP_STATE::SetStopper( std::function<void()> aStopper )
{
    std::lock_guard<std::mutex> lock( m_mutex );
    m_stopper = std::move( aStopper );
}


void MDW_OP_STATE::Complete( std::function<void()> aCallback )
{
    completeHeld(
            [callback = std::move( aCallback )]( HELD_BASE* )
            {
                if( callback )
                    callback();
            } );
}


void MDW_OP_STATE::completeHeld( std::function<void( HELD_BASE* )> aInvoke )
{
    {
        std::lock_guard<std::mutex> lock( m_mutex );
        m_answered = true;
    }

    std::shared_ptr<MDW_OP_STATE> self = shared_from_this();
    std::function<void()>         task = [self, invoke = std::move( aInvoke )]()
    {
        // An op chained from a sync caller's callback must not complete where that caller blocks
        std::optional<MDW_SYNC_SCOPE> scope;

        if( self->m_direct )
            scope.emplace();

        self->deliver( invoke );
    };

    if( !m_direct && deliversOnUiThread() )
        wxTheApp->CallAfter( std::move( task ) );
    else
        postToWorker( std::move( task ) );
}


void MDW_OP_STATE::deliver( const std::function<void( HELD_BASE* )>& aInvoke )
{
    std::unique_ptr<HELD_BASE> held;
    bool                       skip = false;

    {
        std::lock_guard<std::mutex> lock( m_mutex );

        held = std::move( m_held );
        skip = m_cancelled || m_done;

        if( !skip )
        {
            m_running = true;
            m_runningOn = std::this_thread::get_id();
        }
    }

    if( !skip && aInvoke )
        aInvoke( held.get() );

    // Here, so the callback ends on the delivering thread before a waiter can see the op done
    held.reset();

    std::lock_guard<std::mutex> lock( m_mutex );
    m_running = false;
    m_done = true;
    m_cv.notify_all();
}


void MDW_OP_STATE::Cancel()
{
    std::function<void()>      canceller;
    std::unique_ptr<HELD_BASE> held;

    {
        std::unique_lock<std::mutex> lock( m_mutex );

        // A relist from a page's own callback would otherwise send a cancel for a request already answered. A wait
        // that ended at shutdown has already set m_cancelled, and the canceller still breaks the request's cycle
        if( !m_done && !m_answered )
            canceller = std::exchange( m_canceller, nullptr );

        m_cancelled = true;

        // A callback cancelling its own op must not wait for itself
        if( m_running && m_runningOn != std::this_thread::get_id() )
            m_cv.wait( lock,
                       [this]()
                       {
                           return !m_running;
                       } );

        // Ended here on the owner's thread, since nothing will deliver it now. Delivery already took a running one
        held = std::move( m_held );
    }

    held.reset();

    if( canceller )
        canceller();
}


bool MDW_OP_STATE::RequestStop()
{
    std::function<void()> stopper;

    {
        std::lock_guard<std::mutex> lock( m_mutex );

        if( m_done )
            return true;

        if( m_cancelled || !m_stopper )
            return false;

        stopper = std::move( m_stopper );
    }

    stopper();
    return true;
}


bool MDW_OP_STATE::IsDone() const
{
    std::lock_guard<std::mutex> lock( m_mutex );
    return m_done;
}


bool MDW_OP_STATE::waitSettled( std::optional<std::chrono::milliseconds> aTimeout )
{
    std::multimap<std::mutex*, std::condition_variable*>::iterator entry;

    {
        std::lock_guard<std::mutex> lock( s_workerMutex );
        entry = s_waiting.emplace( &m_mutex, &m_cv );
    }

    // Stopped workers drop the queued completions and take no more, so nothing else is coming
    auto settled = [this]()
    {
        return m_done || ( s_closed && !m_running );
    };

    bool ended = true;

    {
        std::unique_lock<std::mutex> lock( m_mutex );

        if( aTimeout )
            ended = m_cv.wait_for( lock, *aTimeout, settled );
        else
            m_cv.wait( lock, settled );

        // A completion the worker already took, or one queued on the UI, must not run once the wait has ended
        if( ended && !m_done )
            m_cancelled = true;
    }

    std::lock_guard<std::mutex> lock( s_workerMutex );
    s_waiting.erase( entry );
    return ended;
}


void MDW_OP_STATE::Wait()
{
    wxCHECK_RET( !onDeliveryThread(), wxS( "Waiting on the thread that delivers the completion" ) );

    waitSettled( std::nullopt );
}


bool MDW_OP_STATE::WaitFor( std::chrono::milliseconds aTimeout )
{
    wxCHECK_MSG( !onDeliveryThread(), true, wxS( "Waiting on the thread that delivers the completion" ) );

    return waitSettled( aTimeout );
}


MDW_OP::MDW_OP() = default;


MDW_OP::MDW_OP( std::shared_ptr<MDW_OP_STATE> aState ) :
        m_state( std::move( aState ) )
{
}


MDW_OP::MDW_OP( MDW_OP&& aOther ) noexcept = default;


MDW_OP& MDW_OP::operator=( MDW_OP&& aOther ) noexcept
{
    if( this != &aOther )
    {
        Cancel();
        m_state = std::move( aOther.m_state );
    }

    return *this;
}


MDW_OP::~MDW_OP()
{
    Cancel();
}


void MDW_OP::Cancel()
{
    if( m_state )
        m_state->Cancel();
}


bool MDW_OP::RequestStop()
{
    return m_state && m_state->RequestStop();
}


void MDW_OP::Detach()
{
    m_state.reset();
}


bool MDW_OP::IsDone() const
{
    return !m_state || m_state->IsDone();
}


void MDW_OP::Wait()
{
    if( m_state )
        m_state->Wait();
}


bool MDW_OP::WaitFor( std::chrono::milliseconds aTimeout )
{
    return !m_state || m_state->WaitFor( aTimeout );
}


MDW_RESULT MdwRunUntilStopped( std::function<MDW_RESULT()> aTask, std::chrono::milliseconds aDeadline,
                               std::chrono::milliseconds aGrace, const std::function<bool()>& aCancelled,
                               const std::function<void()>& aOnStop )
{
    struct STATE
    {
        std::mutex                mutex;
        std::condition_variable   done;
        std::optional<MDW_RESULT> result;
    };

    auto state = std::make_shared<STATE>();

    // Detached, since a task that ignores aOnStop must not hold the caller past its grace
    std::thread(
            [state, task = std::move( aTask )]()
            {
                MDW_RESULT result = task();

                std::lock_guard<std::mutex> lock( state->mutex );
                state->result = result;
                state->done.notify_all();
            } )
            .detach();

    auto finished = [&]( std::chrono::steady_clock::time_point aUntil )
    {
        std::unique_lock<std::mutex> lock( state->mutex );
        return state->done.wait_until( lock, aUntil,
                                       [&]()
                                       {
                                           return state->result.has_value();
                                       } );
    };

    const auto                          end = std::chrono::steady_clock::now() + aDeadline;
    std::optional<MDW_ERROR_CODE>       stopped;
    constexpr std::chrono::milliseconds POLL( 50 );

    while( !stopped )
    {
        if( finished( std::min( std::chrono::steady_clock::now() + POLL, end ) ) )
        {
            std::lock_guard<std::mutex> lock( state->mutex );
            return *state->result;
        }

        if( aCancelled && aCancelled() )
            stopped = MDW_ERROR_CODE::CANCELLED;
        else if( std::chrono::steady_clock::now() >= end )
            stopped = MDW_ERROR_CODE::TIMEOUT;
    }

    if( aOnStop )
        aOnStop();

    finished( std::chrono::steady_clock::now() + aGrace );

    return MDW_RESULT::Fail( *stopped );
}
