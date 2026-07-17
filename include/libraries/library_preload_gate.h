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

#pragma once

#include <functional>
#include <mutex>
#include <utility>

/**
 * Admits one background library preload at a time without dropping a request made while one runs. The running
 * preload loads again instead, so it also picks up library tables changed after it started.
 */
class LIBRARY_PRELOAD_GATE
{
public:
    /// True when the caller must start the preload; false when the running one will load again instead
    bool Enter()
    {
        std::lock_guard lock( m_mutex );

        if( m_running )
        {
            m_again = true;
            return false;
        }

        m_running = true;
        m_again = false;
        return true;
    }

    /**
     * Called by the preload after each load. True when a request arrived during it and the preload must load
     * again. Otherwise aExit runs and the gate opens, both under the lock, so no request falls in between.
     */
    bool Again( bool aAborted, const std::function<void()>& aExit )
    {
        std::lock_guard lock( m_mutex );

        if( !aAborted && std::exchange( m_again, false ) )
            return true;

        aExit();
        m_running = false;
        return false;
    }

    bool IsRunning() const
    {
        std::lock_guard lock( m_mutex );
        return m_running;
    }

private:
    mutable std::mutex m_mutex;
    bool               m_running = false;
    bool               m_again = false;
};
