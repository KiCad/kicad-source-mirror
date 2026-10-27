/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
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

#ifndef KICAD_DB_WORKER_THREAD_H
#define KICAD_DB_WORKER_THREAD_H

#include <chrono>
#include <cstddef>
#include <functional>
#include <memory>
#include <optional>
#include <string>
#include <thread>
#include <vector>

#include <database/database_backend.h>

struct DB_WORKER_CORE;
struct DB_WORKER_STATE;


/**
 * What a job running on a DB_WORKER_THREAD uses to hand rows to the caller.  Rows are grouped in batches
 * of BATCH_ROWS, and at most MAX_QUEUED_BATCHES wait for the caller at once, so a table is never copied
 * in full.
 */
class DB_JOB_CONTEXT
{
public:
    static constexpr size_t BATCH_ROWS = 512;
    static constexpr size_t MAX_QUEUED_BATCHES = 4;

    /**
     * Queues a row for the caller's sink.  Blocks while the caller is behind.
     *
     * @return false once the caller's sink rejected a batch or the caller gave up on the job; the job must
     *         then stop and undo its work
     */
    bool Emit( std::vector<DB_VALUE>&& aRow );

    /**
     * Delivers the rows still held back and waits until the caller's sink has taken every one.  A job
     * commits only after this returns true.
     */
    bool Flush();

    /// @return true once the caller's sink rejected a batch or the caller gave up on the job
    bool Cancelled() const;

private:
    friend struct DB_WORKER_CORE;

    explicit DB_JOB_CONTEXT( DB_WORKER_CORE& aCore ) : m_core( aCore ) {}

    bool pushBatch();

    DB_WORKER_CORE&                    m_core;
    std::vector<std::vector<DB_VALUE>> m_pending;
};


/**
 * Runs one job at a time on its own thread so that a call stuck in filesystem I/O, which nothing can
 * interrupt, cannot hold up the caller past a bound.
 *
 * A job that makes no progress for its bound is abandoned.  The thread keeps the job and
 * everything it owns until the blocked call returns, publishes nothing further and then exits.  The next
 * Run starts a fresh thread.  At most MAX_ABANDONED abandoned threads may be outstanding per source key,
 * process wide, so that repeated timeouts against a dead share cannot pile up threads.  A job holds a slot
 * of that allowance from the moment it is accepted until its thread returns, so concurrent callers cannot
 * overshoot the cap.
 *
 * This class depends on the standard library and the database settings, with the message
 * dispatch on Windows.  A worker serves one calling thread at a time.
 */
class DB_WORKER_THREAD
{
public:
    static constexpr size_t MAX_ABANDONED = 2;

    /**
     * @param aSourceKey names the source the jobs touch; workers with equal keys share the abandoned cap
     * @param aThreadInit runs on the worker thread before its first job; false marks the worker unusable
     * @param aThreadExit runs on the worker thread after its last job, and only if aThreadInit succeeded
     * @param aPumpMessages on Windows, the worker dispatches its thread's window messages while it waits for a job
     *                      and while it waits for the caller to take rows, as a COM apartment needs; ignored
     *                      elsewhere
     * @param aOnAbandon runs on the owner's thread each time a worker blocked in a job is detached
     */
    DB_WORKER_THREAD( std::string aSourceKey, std::function<bool()> aThreadInit = nullptr,
                      std::function<void()> aThreadExit = nullptr, bool aPumpMessages = false,
                      std::function<void()> aOnAbandon = nullptr );

    /// Waits up to the shutdown bound for a running job, then abandons it
    ~DB_WORKER_THREAD();

    /// Why the last Run or RunCleanup returned without a result
    enum class OUTCOME
    {
        OK,
        TIMED_OUT,   ///< no progress within the bound, so the job was abandoned
        TOO_LATE,    ///< a slot freed with under half the bound left, so nothing was started
        REFUSED,     ///< two abandoned jobs hold the source, or no slot freed within the bound
        UNAVAILABLE  ///< the worker thread could not start
    };

    DB_WORKER_THREAD( const DB_WORKER_THREAD& ) = delete;
    DB_WORKER_THREAD& operator=( const DB_WORKER_THREAD& ) = delete;

    /**
     * Runs aJob on the worker thread and waits for it.  Rows the job emits reach aSink on the calling
     * thread, in order.  A sink that throws stops the job; the exception is dropped and Run returns what
     * the job returned.  An exception thrown by the job is rethrown on the calling thread.
     *
     * @param aBound the longest the caller waits without progress, counted from the call so waiting for a slot
     *               uses it up.  Each batch handed to aSink starts the wait again, and time spent inside aSink
     *               is not counted.
     * @param aSink receives the emitted rows; may be null if the job emits none
     * @return the job's result, or nullopt if the bound expired and the job was abandoned, the worker could
     *         not start, or too many earlier jobs for this source are still stuck
     */
    template <typename R>
    std::optional<R> Run( std::function<R( DB_JOB_CONTEXT& )> aJob, std::chrono::milliseconds aBound,
                          const DB_ROW_SINK* aSink )
    {
        auto result = std::make_shared<std::optional<R>>();

        auto wrapped = [job = std::move( aJob ), result]( DB_JOB_CONTEXT& aContext )
                       {
                           *result = job( aContext );
                       };

        if( !runJob( std::move( wrapped ), aBound, aSink, false ) )
            return std::nullopt;

        return std::move( *result );
    }

    /**
     * Runs aCleanup on a worker thread for work that must not happen on the caller, such as closing a
     * connection.  It skips the per-source admission cap so teardown still works when earlier jobs are stuck,
     * and the wait is bounded like Run.  What aCleanup owns is released on the worker even if the wait expires.
     *
     * @return false if the bound expired or the worker could not start
     */
    bool RunCleanup( std::function<void()> aCleanup, std::chrono::milliseconds aBound );

    /// @return false if the last Run was abandoned or the worker thread could not start
    bool IsHealthy() const;

    /// @return true only while a worker thread exists that has neither been abandoned nor failed to start
    bool HasLiveThread() const;

    OUTCOME LastOutcome() const { return *m_outcome; }

    /// @return true on a thread started by a DB_WORKER_THREAD
    static bool OnWorkerThread();

    /// Sets how long the destructor waits for a running job before abandoning it
    void SetShutdownBound( std::chrono::milliseconds aBound ) { m_shutdownBound = aBound; }

    /// @return the key naming the file at aPath, which does not touch the file system
    static std::string SourceKeyFor( const std::string& aPath );

    /// As SourceKeyFor, for a path whose separators and prefixes follow Windows rules or not; relative paths stay so
    static std::string SourceKeyFor( const std::string& aPath, bool aWindowsPath );

    /**
     * Rewrites the spellings of one file that Windows treats alike into one form, without touching the file
     * system: backslashes become slashes, and \\?\C:\ and \\?\UNC\server\ lose the long-path prefix.  The
     * source key and the URI builder share this so they agree on which paths are one file.
     */
    static std::string NormalizePath( const std::string& aPath, bool aWindowsPath );

    /// @return the longest a caller waits on a source with the given timeout: the timeout plus 10 s, or 30 s for 0
    static std::chrono::milliseconds CallerBound( int aTimeoutSeconds );

private:
    bool runJob( std::function<void( DB_JOB_CONTEXT& )> aJob, std::chrono::milliseconds aBound,
                 const DB_ROW_SINK* aSink, bool aCleanup );

    std::string m_key;
    bool        m_pumpMessages;

    std::chrono::milliseconds m_shutdownBound{ std::chrono::seconds( 30 ) };
    std::shared_ptr<OUTCOME>  m_outcome = std::make_shared<OUTCOME>( OUTCOME::OK );

    // Everything a caller still waiting in admission touches lives here, so destroying the worker cannot free it
    std::shared_ptr<DB_WORKER_STATE> m_state;
};

#endif // KICAD_DB_WORKER_THREAD_H
