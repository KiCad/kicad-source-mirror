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

#ifndef KICAD_ACE_BACKEND_H
#define KICAD_ACE_BACKEND_H

#include <atomic>
#include <chrono>
#include <cstdint>
#include <functional>
#include <memory>
#include <optional>
#include <set>
#include <string>
#include <vector>

#include <database/database_backend.h>

#include "db_worker_thread.h"


/**
 * One caller operation's claim on the Access engine lock.  The deadline is fixed when the operation starts, and
 * every job the operation submits, including the worker's exit hook after it, waits for the lock no longer than
 * that.  The state is the operation's own, so a worker abandoned by an earlier operation never touches a later one.
 */
struct ACE_ENGINE_OP
{
    using CLOCK = std::chrono::steady_clock;

    enum class STATE
    {
        IDLE,
        WAITING,  ///< A job is blocked on the engine lock
        ACQUIRED, ///< The job took the lock
        EXPIRED   ///< The deadline passed first; never cleared
    };

    explicit ACE_ENGINE_OP( CLOCK::time_point aDeadline ) :
            deadline( aDeadline )
    {
    }

    /// True if the operation is waiting for the engine lock or gave up on it
    bool Busy() const
    {
        const STATE current = state;
        return current == STATE::WAITING || current == STATE::EXPIRED;
    }

    const CLOCK::time_point deadline;
    std::atomic<STATE>      state{ STATE::IDLE };
};


/**
 * Reads a Microsoft Access database (.mdb or .accdb) through ADO and the ACE OLE DB provider, read only and
 * shared, so the file can stay open in Access.  Every COM object lives on the worker thread, an apartment of
 * its own, so a file on a share that stopped answering costs the caller a bounded wait and not a hung program.
 * The provider cannot cancel a read that is blocked on file I/O, so such a thread is detached and finishes
 * when the operating system gives up on the share.
 */
class ACE_BACKEND : public DATABASE_BACKEND
{
public:
    /**
     * @param aPath the database file, as UTF-8
     * @param aPassword the database password, empty if the file has none; never appears in a message or trace
     * @param aTimeoutSeconds the provider's connection and command timeout, zero for its default; a call waits on
     *                        the worker without progress for at most this plus 10 s, or 30 s when it is zero, and
     *                        the provider probe of a first connect may add up to 10 s
     */
    ACE_BACKEND( std::string aPath, std::string aPassword, int aTimeoutSeconds );

    ~ACE_BACKEND() override;

    bool Connect() override;

    void Disconnect() override;

    bool IsConnected() const override { return m_connected; }

    bool ListTables( std::vector<std::string>& aTables ) override;

    std::optional<DB_TABLE_INFO> DescribeTable( const std::string& aTable ) override;

    bool ReadTable( const DB_TABLE_INFO& aTable, const std::vector<std::string>& aColumns, size_t aMaxRows,
                    std::vector<std::string>& aLabels, const DB_ROW_SINK& aSink ) override;

    /**
     * A verified read.  Every pass runs on a session of the load cycle that has not read the table, since a session
     * keeps serving the pages it read for seconds after another user changed them; a session is opened when none
     * is left, and the cycle keeps at most two.  The first pass only hashes the rows.  Every later pass streams its
     * rows to aSink and hashes them, and is compared with the pass before it.  A pass that disagrees is followed by
     * aRestart and another pass after a pause of 50 to 250 ms.  The first two passes always run.  No retry starts
     * once the caller's bound, counted from the end of the second pass, has passed, and no open or pass starts once
     * the cycle's stop check is true; the read then fails, as FILE_IN_USE if the bound ran out, and delivers nothing
     * that may be kept.  A read outside a load cycle runs in a cycle of its own.
     *
     * A running open or pass is bounded only by the worker's progress bound, so a live read of a slow share is never
     * cut short.  Its wait for the engine lock ends a bound after it started, or for a retry when the bound runs out.
     *
     * A table is published only when two consecutive complete passes, on two sessions that had not read the table
     * before, returned identical content (row count, column names and an ordered hash over every column). The published
     * table then equals the table's state at one instant between the passes, provided that (a) no row returns to an
     * earlier value between the passes, (b) no single writer commit takes longer than a pass to become fully visible,
     * and (c) neither session serves a page older than the other pass's read of it. This detects disagreement; it does
     * not certify transactional consistency. There is no cross-table consistency.
     */
    bool ReadAllRows( const DB_TABLE_INFO& aTable, const std::vector<std::string>& aColumns,
                      std::vector<std::string>& aLabels, const DB_ROW_SINK& aSink,
                      const DB_RESTART_SINK& aRestart ) override;

    void BeginLoadCycle( const DB_STOP_CHECK& aStopped ) override;

    /// Closes the sessions the cycle opened, on the worker thread
    void EndLoadCycle() override;

    std::string GetLastError() const override { return m_lastError; }

    DB_FAILURE GetLastFailure() const override { return m_lastFailure; }

    enum class PROVIDER_STATE
    {
        AVAILABLE,
        MISSING,
        PENDING, ///< The check has not finished, so nothing is known yet
        BUSY     ///< The check could not start because the engine lock stayed out of reach
    };

    /**
     * Tells whether the ACE OLE DB provider is installed and loads.  The check runs on first use, on a COM
     * thread of its own, and a finished answer is kept for the life of the process.  This does not wait, so a
     * check still running reports PENDING and Connect() is the one place that waits for it.  Nothing calls this
     * at startup.
     */
    static PROVIDER_STATE ProviderState();

    /// The message for a source whose provider check has not finished
    static std::string ProviderPendingMessage();

    /// The message for a source that needs the provider when ProviderState() is MISSING
    static std::string ProviderMissingMessage();

private:
    /// A session opened for one load cycle, and the tables it has read, which it never reads again
    struct CYCLE_SESSION
    {
        uint64_t              id = 0;
        std::set<std::string> tables;
    };

    /// What one complete pass over a table returned, as a verified read compares it
    struct PASS_DIGEST
    {
        size_t                   rows = 0;
        uint64_t                 hash = 0;
        std::vector<std::string> labels;

        bool operator==( const PASS_DIGEST& aOther ) const
        {
            return rows == aOther.rows && hash == aOther.hash && labels == aOther.labels;
        }
    };

    /**
     * Runs one pass of aSql on aSession, recording a failure as the backend's.
     *
     * @param aSink receives the rows; null only hashes them
     * @param aDeadline is a verified read's deadline for the engine lock, which leaves the pass bounded by progress
     * @return the pass's digest, or nullopt if it failed
     */
    std::optional<PASS_DIGEST> readPass( uint64_t aSession, const std::string& aSql, const std::string& aUnorderedSql,
                                         size_t aMaxRows, std::vector<std::string>& aLabels, const DB_ROW_SINK* aSink,
                                         std::optional<ACE_ENGINE_OP::CLOCK::time_point> aDeadline = std::nullopt );

    /// A session of the cycle that has not read aTable, now marked as having read it, or nullopt if one could not
    /// open before aDeadline.  An open the file's lock refuses is tried again after a growing pause.  No open starts
    /// unless aMayStart, which records its own failure, allows it.
    std::optional<uint64_t> freshSession( const std::string& aTable, ACE_ENGINE_OP::CLOCK::time_point aDeadline,
                                          const std::function<bool()>& aMayStart );

    /// Closes the oldest sessions of the cycle, which have all read the current table, so that one more fits
    bool closeSpentSessions( ACE_ENGINE_OP::CLOCK::time_point aDeadline );

    bool cycleStopped() const;

    /// Sleeps for aWait; false if the cycle's stop check came true first
    bool pauseUnlessStopped( std::chrono::milliseconds aWait );

    void failStopped( const std::string& aTable );

    /// The verified read itself, inside a load cycle
    bool readVerified( const DB_TABLE_INFO& aTable, const std::vector<std::string>& aColumns,
                       std::vector<std::string>& aLabels, const DB_ROW_SINK& aSink, const DB_RESTART_SINK& aRestart );

    /// Runs aJob on the worker, recording why it did not finish as the failure
    template <typename R>
    std::optional<R> runOnWorker( std::function<R( DB_JOB_CONTEXT& )> aJob, const DB_ROW_SINK* aSink,
                                  std::shared_ptr<ACE_ENGINE_OP>           aOp = nullptr,
                                  std::optional<std::chrono::milliseconds> aProgressBound = std::nullopt );

    /// Forgets the connection and the cycle's sessions and closes them on the worker thread, never on the caller.  A
    /// live worker also learns the operation for its exit hook when aForExit is set, even with no connection to
    /// close.  An abandoned or absent worker is left alone, since its sessions are not reachable from another thread.
    void dropConnection( std::shared_ptr<ACE_ENGINE_OP> aOp = nullptr, bool aForExit = false );

    /// Closes aIds on the worker thread under aOp, recording a close that lost the engine lock; false if the wait
    /// for the worker, aBound or else what is left of aOp, expired
    bool closeOnWorker( std::vector<uint64_t> aIds, std::shared_ptr<ACE_ENGINE_OP> aOp,
                        std::optional<std::chrono::milliseconds> aBound = std::nullopt );

    void fail( DB_FAILURE aFailure, std::string aError );

    /// @return a fresh operation whose deadline is one caller bound from now
    std::shared_ptr<ACE_ENGINE_OP> newOp() const;

    /// Records a failure of a call that ran, and drops the connection if the failure means it cannot be trusted
    void failCall( DB_FAILURE aFailure, std::string aMessage );

    /// Fails with a message and returns false unless a connection is open
    bool requireConnection();

    std::string m_path;
    std::string m_password;
    int         m_timeout;

    /// The operation the caller last ran, which a failure that drops the connection continues
    std::shared_ptr<ACE_ENGINE_OP> m_lastOp;

    std::unique_ptr<DB_WORKER_THREAD> m_worker;

    /// Names this backend's connection among those the worker thread holds
    uint64_t m_session = 0;

    /// The provider that opened the connection, which the cycle's sessions use too
    std::wstring m_progId;

    /// The most sessions a load cycle holds at once
    static constexpr size_t MAX_CYCLE_SESSIONS = 2;

    std::vector<CYCLE_SESSION> m_cycleSessions;

    /// Set by the outermost load cycle, which a verified read checks before every open and pass
    DB_STOP_CHECK m_cycleStop;

    /// How many load cycles are open; the sessions are closed when the last one ends
    int m_cycleDepth = 0;

    std::atomic<bool> m_connected{ false };

    std::string m_lastError;
    DB_FAILURE  m_lastFailure = DB_FAILURE::UNKNOWN;
};

#endif // KICAD_ACE_BACKEND_H
