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

/**
 * @file mdw_poller.h
 * @brief A thread that runs a backend's pass over its server every interval.
 *
 * A server that announces no changes, such as a git remote, is read again on a timer so a teammate's claim or
 * library change arrives unasked.
 */

#ifndef MDW_POLLER_H
#define MDW_POLLER_H

#include <api/workspace/workspace_types.pb.h>

#include <atomic>
#include <chrono>
#include <condition_variable>
#include <functional>
#include <memory>
#include <mutex>
#include <thread>


/// How a pass went, which decides when the next one runs
enum class MDW_POLL
{
    OK,          ///< The next pass runs one interval on
    UNREACHABLE, ///< The server did not answer, so passes back off, doubling up to MDW_POLLER::MAX_BACKOFF
    REFUSED      ///< The server refused this session's credentials or host, so no pass runs until Resume()
};


/// The MDW_POLL a pass that ended in aError reports. A busy or cancelled pass says nothing of the server
MDW_POLL MdwPollOutcome( const kiapi::workspace::WorkspaceError& aError );


/// aDefault, or the interval in milliseconds that KICAD_MDW_POLL_INTERVAL_MS sets for every backend's poller
std::chrono::milliseconds MdwPollInterval( std::chrono::milliseconds aDefault );


class MDW_POLLER
{
public:
    /**
     * Run aPass on a thread of its own every aInterval, the first time an interval from now. A pass runs as a
     * request would, so MdwCurrentRequestCancelled() turns true inside it once the poller stops. An exception
     * from a pass is dropped.
     */
    MDW_POLLER( std::chrono::milliseconds aInterval, std::function<MDW_POLL()> aPass );

    /// Stop, cancelling a pass under way and waiting for it to end
    ~MDW_POLLER();

    MDW_POLLER( const MDW_POLLER& ) = delete;
    MDW_POLLER& operator=( const MDW_POLLER& ) = delete;

    /**
     * The server answered or took new credentials, so lift a hold and a back-off. A pass runs at once unless the
     * last one went well, so a routine connection check costs nothing.
     */
    void Resume();

    /// The server refused this session's credentials, so no pass runs until Resume()
    void Hold();

private:
    /// The longest an unreachable server waits between passes
    static constexpr std::chrono::minutes MAX_BACKOFF{ 5 };

    void run();

    /// Call with m_mutex held
    void settle( MDW_POLL aOutcome );

    const std::chrono::milliseconds    m_interval;
    const std::function<MDW_POLL()>    m_pass;
    std::shared_ptr<std::atomic<bool>> m_cancel;
    std::mutex                         m_mutex;
    std::condition_variable            m_wake;
    bool                               m_stop = false;
    bool                               m_woken = false;
    bool                               m_held = false;
    bool                               m_passed = false; ///< The last pass went well
    int                                m_failures = 0;   ///< Unanswered passes in a row
    unsigned                           m_rulings = 0;    ///< Resume() and Hold() calls, which outrank a pass
    std::thread                        m_thread;         ///< Last, so it starts once everything it reads is built
};

#endif // MDW_POLLER_H
