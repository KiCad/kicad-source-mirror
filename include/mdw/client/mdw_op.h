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
 * @file mdw_op.h
 * The handle every asynchronous workspace operation returns, and the one way to wait for one.
 */

#ifndef MDW_CLIENT_OP_H
#define MDW_CLIENT_OP_H

#include <kicommon.h>
#include <mdw/client/mdw_types.h>

#include <atomic>
#include <chrono>
#include <cstdint>
#include <functional>
#include <memory>
#include <utility>

/// An operation's completion. The payload is default-constructed on failure
template <typename T>
using MDW_DONE = std::function<void( const MDW_RESULT&, const T& )>;

/// The payload of an operation that returns nothing but its status.
struct MDW_NONE
{
};

class MDW_OP_STATE;

/// How long a backend has to answer a cancel
constexpr int MDW_CANCEL_TIMEOUT_MS = 5000;

/**
 * A move-only handle to one in-flight operation. Destroying it cancels the operation.
 *
 * Completion never runs inside the call that started the operation, not even for an immediate
 * failure, so a caller may store the handle before its callback can observe it. Cancel() asks the
 * backend to abandon the request and suppresses a callback that has not started, but cannot stop one
 * already running.
 *
 * With an app, callbacks run on the UI thread, where handles must also be cancelled and destroyed,
 * so the two never overlap. Headless, Cancel() and destruction wait for a running callback to
 * return unless called from inside it, so the state a callback captured may be destroyed as soon
 * as its handle is.
 */
class KICOMMON_API MDW_OP
{
public:
    MDW_OP();
    explicit MDW_OP( std::shared_ptr<MDW_OP_STATE> aState );
    MDW_OP( MDW_OP&& aOther ) noexcept;
    MDW_OP& operator=( MDW_OP&& aOther ) noexcept;
    ~MDW_OP();

    MDW_OP( const MDW_OP& ) = delete;
    MDW_OP& operator=( const MDW_OP& ) = delete;

    void Cancel();

    /**
     * Ask the backend to stop serving the operation without suppressing its callback, which may still
     * report success. False when the operation has no request to stop.
     */
    bool RequestStop();

    /// Let the operation run to completion without this handle.
    void Detach();

    /// True once the callback has returned or been suppressed.
    bool IsDone() const;

    /**
     * Block until the callback has returned or been suppressed. Asserts on the thread that
     * delivers this operation's completion, where it would wait forever.
     */
    void Wait();

    /**
     * Wait() for at most aTimeout, true when the operation is done. Also true, after asserting, on the thread
     * that delivers this operation's completion, which cannot wait for it.
     */
    bool WaitFor( std::chrono::milliseconds aTimeout );

private:
    std::shared_ptr<MDW_OP_STATE> m_state;
};

/**
 * Stop the client worker and settler, dropping what they have queued and refusing new work until
 * MdwStartWorkers(). A wait on an op they will not deliver returns without its callback. Waits up to
 * aTimeout for each running task; false means one is still running and process teardown is unsafe.
 */
KICOMMON_API bool MdwStopWorkers( std::chrono::milliseconds aTimeout );

/// Let the client worker and settler take work again. Only creating the framework calls this
KICOMMON_API void MdwStartWorkers();

/**
 * While one is open on a thread, the operations that thread starts complete on the client worker
 * whatever the delivery mode, so the thread may block on them. Callbacks of such operations run
 * inside a scope too, so an operation chained from one completes on the worker as well.
 */
class KICOMMON_API MDW_SYNC_SCOPE
{
public:
    MDW_SYNC_SCOPE();
    ~MDW_SYNC_SCOPE();

    MDW_SYNC_SCOPE( const MDW_SYNC_SCOPE& ) = delete;
    MDW_SYNC_SCOPE& operator=( const MDW_SYNC_SCOPE& ) = delete;

    /// True while a scope is open on the calling thread
    static bool IsOpen();
};

/**
 * While one is open on a thread, MdwSync on that thread stops waiting once aCancelled turns true. It asks the
 * backend to stop, starts no new operation and answers CANCELLED, unless the backend still reports success
 * within MDW_CANCEL_TIMEOUT_MS. Another thread sets the flag, as a progress dialog does when the user stops
 * the task blocked here. Scopes nest, and any open one cancels.
 */
class KICOMMON_API MDW_CANCEL_SCOPE
{
public:
    explicit MDW_CANCEL_SCOPE( std::shared_ptr<const std::atomic<bool>> aCancelled );
    ~MDW_CANCEL_SCOPE();

    MDW_CANCEL_SCOPE( const MDW_CANCEL_SCOPE& ) = delete;
    MDW_CANCEL_SCOPE& operator=( const MDW_CANCEL_SCOPE& ) = delete;

    /// True while a scope is open on the calling thread
    static bool IsOpen();

    /// True when a scope open on the calling thread was cancelled
    static bool Requested();

private:
    std::shared_ptr<const std::atomic<bool>> m_cancelled;
    const MDW_CANCEL_SCOPE*                  m_outer;
};

/// How far a backend says a running operation has got
struct MDW_PROGRESS
{
    std::uint64_t done = 0;
    std::uint64_t total = 0; ///< 0 while unknown
    wxString      phase;     ///< "receive", "checkout", "index", or text a backend wants shown as given
};

/// What to tell the user about aProgress, such as "Receiving objects: 12 of 40"
KICOMMON_API wxString MdwProgressText( const MDW_PROGRESS& aProgress );

/**
 * While one is open on a thread, the operations that thread starts pass the progress their backend reports to
 * aSink, until they complete. A backend need not report any. aSink runs on a transport thread, so it only
 * records the progress for the caller to show. Scopes nest, and the innermost one takes the progress.
 */
class KICOMMON_API MDW_PROGRESS_SCOPE
{
public:
    using SINK = std::function<void( const MDW_PROGRESS& )>;

    explicit MDW_PROGRESS_SCOPE( SINK aSink );
    ~MDW_PROGRESS_SCOPE();

    MDW_PROGRESS_SCOPE( const MDW_PROGRESS_SCOPE& ) = delete;
    MDW_PROGRESS_SCOPE& operator=( const MDW_PROGRESS_SCOPE& ) = delete;

    /// The sink of the innermost scope open on the calling thread, empty when none is
    static SINK Current();

private:
    SINK                      m_sink;
    const MDW_PROGRESS_SCOPE* m_outer;
};

/**
 * Run aTask on its own thread and wait for its result, polling aCancelled. Past aDeadline or on a cancel it calls
 * aOnStop once, which should make aTask return, and waits up to aGrace more. It then answers TIMEOUT or CANCELLED
 * whether or not aTask returned, so the wait is bounded by aDeadline plus aGrace and aTask may outlive this call.
 * aTask therefore owns, not borrows, what it uses.
 */
KICOMMON_API MDW_RESULT MdwRunUntilStopped( std::function<MDW_RESULT()> aTask, std::chrono::milliseconds aDeadline,
                                            std::chrono::milliseconds aGrace, const std::function<bool()>& aCancelled,
                                            const std::function<void()>& aOnStop );

/**
 * Start an operation and block until it completes, returning its status and payload.
 *
 * Callable on any thread but the client worker. Its completion runs on the worker, so the UI thread
 * may block on it. Inside a cancelled MDW_CANCEL_SCOPE it answers CANCELLED.
 */
template <typename T>
std::pair<MDW_RESULT, T> MdwSync( const std::function<MDW_OP( MDW_DONE<T> )>& aStart )
{
    using OUTCOME = std::pair<MDW_RESULT, T>;

    if( MDW_CANCEL_SCOPE::Requested() )
        return OUTCOME( MDW_RESULT::Fail( MDW_ERROR_CODE::CANCELLED ), T() );

    auto   out = std::make_shared<OUTCOME>();
    auto   delivered = std::make_shared<std::atomic<bool>>( false );
    MDW_OP op;

    {
        MDW_SYNC_SCOPE scope;

        op = aStart(
                [out, delivered]( const MDW_RESULT& aResult, const T& aValue )
                {
                    out->first = aResult;
                    out->second = aValue;
                    delivered->store( true );
                } );
    }

    // Polled, since the flag is set on another thread
    while( MDW_CANCEL_SCOPE::IsOpen() && !op.WaitFor( std::chrono::milliseconds( 50 ) ) )
    {
        if( !MDW_CANCEL_SCOPE::Requested() )
            continue;

        // A backend that finished first still answers, and its success must reach the caller to be recorded
        if( op.RequestStop() )
            op.WaitFor( std::chrono::milliseconds( MDW_CANCEL_TIMEOUT_MS ) );

        // Cancel waits out a callback already running, so an answer that beat it is seen here
        op.Cancel();

        if( delivered->load() && out->first.IsOk() )
            return std::move( *out );

        return OUTCOME( MDW_RESULT::Fail( MDW_ERROR_CODE::CANCELLED ), T() );
    }

    op.Wait();

    // An op that ended without its callback, as when the workers stopped, has no answer to give
    if( !delivered->load() )
        return OUTCOME( MDW_RESULT::Fail( MDW_ERROR_CODE::CANCELLED ), T() );

    return std::move( *out );
}

#endif // MDW_CLIENT_OP_H
