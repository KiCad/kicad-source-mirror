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
 * @file mdw_serial_worker.h
 * @brief One thread that runs workspace calls in submission order, off the UI thread.
 *
 * Backend calls can block for a full request timeout, so they run here rather than
 * on the shared thread pool where they would starve unrelated work. Destruction
 * drops queued tasks and joins the running one, unless it runs on the worker itself, where
 * the thread is detached to finish once the task returns.
 */

#ifndef MDW_SERIAL_WORKER_H
#define MDW_SERIAL_WORKER_H

#include <kicommon.h>

#include <chrono>
#include <condition_variable>
#include <cstdint>
#include <deque>
#include <functional>
#include <memory>
#include <mutex>
#include <thread>

class wxEvtHandler;

class KICOMMON_API MDW_SERIAL_WORKER
{
public:
    MDW_SERIAL_WORKER();
    ~MDW_SERIAL_WORKER();

    MDW_SERIAL_WORKER( const MDW_SERIAL_WORKER& ) = delete;
    MDW_SERIAL_WORKER& operator=( const MDW_SERIAL_WORKER& ) = delete;

    void Post( std::function<void()> aTask );

    /// Drop queued tasks and wait up to aTimeout for the running one to return; false leaves its thread running
    bool Stop( std::chrono::milliseconds aTimeout );

private:
    /// Outlives the worker when the thread is detached
    struct STATE
    {
        std::mutex                        mutex;
        std::condition_variable           cv;
        std::deque<std::function<void()>> tasks;
        bool                              stop = false;
        bool                              done = false;
    };

    static void run( const std::shared_ptr<STATE>& aState );

    std::shared_ptr<STATE> m_state = std::make_shared<STATE>();
    std::thread            m_thread;
};

/**
 * Hands a worker's result back to the UI thread for an owner that may be destroyed
 * meanwhile. wxWeakRef is not thread-safe, so the owner is tracked here and only ever
 * touched on the UI thread; worker threads carry a plain ticket.
 */
namespace MDW_UI_COMPLETION
{
/// Releases its registration when the last copy goes, so a task dropped unrun leaks nothing.
using TICKET = std::shared_ptr<const std::uint64_t>;

/// UI thread only. Returns the ticket a worker passes to Post().
KICOMMON_API TICKET Register( wxEvtHandler* aOwner );

/// Any thread. Runs aDone on the UI thread if the owner is still alive, keeping the ticket for later calls.
KICOMMON_API void Post( const TICKET& aTicket, std::function<void()> aDone );
}

#endif // MDW_SERIAL_WORKER_H
