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
 * @file mdw_restart_policy.h
 * @brief The backoff schedule that governs backend respawns.
 *
 * A crashed backend is respawned with a growing delay, and after too many
 * restarts in a window the framework gives up and drops the session offline.
 * The policy is a pure function of the attempt timestamps so it can be tested
 * without spawning anything.
 */

#ifndef MDW_RESTART_POLICY_H
#define MDW_RESTART_POLICY_H

#include <cstdint>
#include <vector>

class MDW_RESTART_POLICY
{
public:
    /// Give up after this many restarts inside the window.
    static constexpr int MAX_ATTEMPTS = 3;

    static constexpr std::int64_t WINDOW_MS = 10LL * 60 * 1000;

    /**
     * Record a restart attempt at aNowMs, a monotonic clock, and return the delay before the respawn, or -1 when
     * too many restarts occurred in the window and the session should go offline instead.
     */
    int NextDelayMs( std::int64_t aNowMs )
    {
        prune( aNowMs );

        if( static_cast<int>( m_attempts.size() ) >= MAX_ATTEMPTS )
            return -1;

        static const int schedule[MAX_ATTEMPTS] = { 0, 2000, 10000 };

        int delay = schedule[m_attempts.size()];
        m_attempts.push_back( aNowMs );

        return delay;
    }

private:
    void prune( std::int64_t aNowMs )
    {
        std::erase_if( m_attempts,
                       [&]( std::int64_t aAt )
                       {
                           return aNowMs - aAt >= WINDOW_MS;
                       } );
    }

    std::vector<std::int64_t> m_attempts;
};

#endif // MDW_RESTART_POLICY_H
