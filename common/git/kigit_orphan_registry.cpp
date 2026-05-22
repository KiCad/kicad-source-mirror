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

#include "kigit_orphan_registry.h"


KIGIT_ORPHAN_REGISTRY::KIGIT_ORPHAN_REGISTRY() = default;


KIGIT_ORPHAN_REGISTRY::~KIGIT_ORPHAN_REGISTRY()
{
    // Any remaining entries represent threads we could not join during
    // shutdown.  Detach them so the OS can clean them up rather than calling
    // std::thread's destructor on a joinable thread (which would terminate
    // the process).  The per-thread SHARED_STATE is ref-counted and outlives
    // this registry, so the worker can still safely signal completion.

    std::lock_guard<std::mutex> lock( m_mutex );

    for( ENTRY& entry : m_entries )
    {
        if( entry.m_thread.joinable() )
            entry.m_thread.detach();
    }

    m_entries.clear();
}


size_t KIGIT_ORPHAN_REGISTRY::TrackedCount() const
{
    std::lock_guard<std::mutex> lock( m_mutex );
    size_t                      count = 0;

    for( const ENTRY& entry : m_entries )
    {
        if( !entry.m_state->m_done.load( std::memory_order_acquire ) )
            ++count;
    }

    return count;
}


size_t KIGIT_ORPHAN_REGISTRY::JoinAll( std::chrono::milliseconds aTimeout )
{
    const auto deadline = std::chrono::steady_clock::now() + aTimeout;

    // Close the registry so Register() rejects late arrivals, then move
    // pending entries out and release m_mutex while we wait.

    std::vector<ENTRY> pending;

    {
        std::lock_guard<std::mutex> lock( m_mutex );
        m_shuttingDown = true;
        pending = std::move( m_entries );
    }

    size_t stuck = 0;

    for( ENTRY& entry : pending )
    {
        std::shared_ptr<SHARED_STATE> state = entry.m_state;

        std::unique_lock<std::mutex> doneLock( state->m_doneMutex );

        bool finished = state->m_doneCv.wait_until( doneLock, deadline,
                                                    [&state]()
                                                    {
                                                        return state->m_done.load(
                                                                std::memory_order_acquire );
                                                    } );

        doneLock.unlock();

        if( finished )
        {
            if( entry.m_thread.joinable() )
                entry.m_thread.join();
        }
        else
        {
            wxLogTrace( traceGit,
                        "Orphan git thread [%s] did not finish in time; detaching",
                        entry.m_label.c_str() );

            if( entry.m_thread.joinable() )
                entry.m_thread.detach();

            ++stuck;
        }
    }

    return stuck;
}


void KIGIT_ORPHAN_REGISTRY::reapLocked()
{
    for( auto it = m_entries.begin(); it != m_entries.end(); )
    {
        if( it->m_state->m_done.load( std::memory_order_acquire ) )
        {
            if( it->m_thread.joinable() )
                it->m_thread.join();

            it = m_entries.erase( it );
        }
        else
        {
            ++it;
        }
    }
}
