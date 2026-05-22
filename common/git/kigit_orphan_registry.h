/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright The KiCad Developers, see AUTHORS.TXT for contributors.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 3
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, you may find one here:
 * http://www.gnu.org/licenses/gpl-3.0.html
 * or you may search the http://www.gnu.org website for the version 3 license,
 * or you may write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
 */

#ifndef KIGIT_ORPHAN_REGISTRY_H_
#define KIGIT_ORPHAN_REGISTRY_H_

#include <import_export.h>

#include <atomic>
#include <chrono>
#include <condition_variable>
#include <memory>
#include <mutex>
#include <string>
#include <thread>
#include <utility>
#include <vector>

#include <wx/log.h>

#include <trace_helpers.h>

/**
 * Registry of background git cleanup threads that outlive the owning project
 * or dialog.
 *
 * When a user abandons a slow git close (for example, a fetch stuck under a
 * blocking recv) we cannot wait for the libgit2 worker to notice cancellation
 * before the UI returns, but we also cannot call git_libgit2_shutdown() while
 * that worker is still inside libgit2. The registry lets us track such
 * "orphan" threads and join them with a bounded timeout during application
 * shutdown.
 *
 * Each entry is reference-counted via std::shared_ptr so that a detached
 * worker never touches freed registry state after the registry has given up
 * on it: the worker and the registry each hold their own shared reference to
 * a small state block.
 */
class APIEXPORT KIGIT_ORPHAN_REGISTRY
{
public:
    KIGIT_ORPHAN_REGISTRY();

    ~KIGIT_ORPHAN_REGISTRY();

    KIGIT_ORPHAN_REGISTRY( const KIGIT_ORPHAN_REGISTRY& ) = delete;
    KIGIT_ORPHAN_REGISTRY& operator=( const KIGIT_ORPHAN_REGISTRY& ) = delete;

    /**
     * Spawn a tracked orphan thread running @a aWork.  The thread is owned by
     * the registry and will be joined during JoinAll() or on destruction.
     *
     * The callable is accepted by forwarding reference so move-only work
     * objects (for example lambdas capturing a std::unique_ptr) are
     * supported without forcing shared ownership on the caller.
     *
     * If the registry has already begun shutdown via JoinAll() the call
     * returns false and @a aWork is not invoked; the caller is expected to
     * run the cleanup synchronously or drop it.
     *
     * @param aLabel     Human-readable tag used in trace logs, e.g. "abandon
     *                   close /home/user/proj.kicad_pro".
     * @param aWork      Work function executed on the new thread.  It should
     *                   honour any cooperative cancellation flag supplied by
     *                   the caller before performing blocking libgit2 calls.
     * @return true if the thread was registered, false if the registry is
     *         shutting down.
     */
    template <typename F>
    bool Register( const std::string& aLabel, F&& aWork )
    {
        auto state = std::make_shared<SHARED_STATE>();

        std::lock_guard<std::mutex> lock( m_mutex );

        if( m_shuttingDown )
            return false;

        reapLocked();

        // Reserve the slot before starting the thread so we don't leak a
        // joinable std::thread if vector growth throws; emplace the entry
        // with the state already wired up and then assign m_thread.

        m_entries.emplace_back();
        ENTRY& entry = m_entries.back();
        entry.m_label = aLabel;
        entry.m_created = std::chrono::steady_clock::now();
        entry.m_state = state;

        try
        {
            entry.m_thread = std::thread(
                    [state,
                     work = std::forward<F>( aWork ),
                     label = aLabel]() mutable
                    {
                        try
                        {
                            work();
                        }
                        catch( ... )
                        {
                            wxLogTrace( traceGit,
                                        "Orphan git thread [%s] threw an exception",
                                        label.c_str() );
                        }

                        {
                            std::lock_guard<std::mutex> doneLock( state->m_doneMutex );
                            state->m_done.store( true, std::memory_order_release );
                        }

                        state->m_doneCv.notify_all();
                    } );
        }
        catch( ... )
        {
            m_entries.pop_back();
            throw;
        }

        wxLogTrace( traceGit, "Registered orphan git thread [%s]", aLabel.c_str() );
        return true;
    }

    /**
     * Return the number of tracked orphan threads that have not yet
     * finished executing.  Threads that JoinAll() detached after a timeout
     * are no longer tracked and do not contribute to this count.
     */
    size_t TrackedCount() const;

    /**
     * Join all registered orphan threads, giving them up to @a aTimeout to
     * finish cooperatively.  After the timeout the remaining threads are
     * detached so the caller can proceed with teardown; the shared per-entry
     * state outlives the registry until the worker itself drops its
     * reference, so detached workers are safe to keep running.
     *
     * Once JoinAll() has been called the registry enters a shutting-down
     * state and Register() returns false; this prevents a late caller from
     * queueing a new orphan after the join window has already closed.
     *
     * @return Number of threads that failed to join within the timeout.
     */
    size_t JoinAll( std::chrono::milliseconds aTimeout );

private:
    struct SHARED_STATE
    {
        std::atomic<bool>                m_done{ false };
        std::mutex                       m_doneMutex;
        std::condition_variable          m_doneCv;
    };

    struct ENTRY
    {
        std::thread                           m_thread;
        std::string                           m_label;
        std::chrono::steady_clock::time_point m_created;
        std::shared_ptr<SHARED_STATE>         m_state;
    };

    // Remove finished entries by joining their threads.  Must be called
    // with m_mutex held.
    void reapLocked();

    mutable std::mutex           m_mutex;
    std::vector<ENTRY>           m_entries;
    bool                         m_shuttingDown = false;
};

#endif // KIGIT_ORPHAN_REGISTRY_H_
