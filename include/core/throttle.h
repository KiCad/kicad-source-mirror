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

#pragma once

#include <chrono>

/**
 * Rate-limiter that fires at most once per interval.
 *
 * The first call to Ready() always returns true so callers can emit an
 * initial update immediately.
 */
class THROTTLE
{
public:
    explicit THROTTLE( std::chrono::milliseconds aInterval ) :
            m_interval( aInterval ),
            m_last( std::chrono::steady_clock::now() - aInterval )
    {
    }

    /**
     * @return true when at least \a m_interval has elapsed since the previous
     *         true return (or since construction).  Resets the internal clock
     *         on a true return.
     */
    bool Ready()
    {
        auto now = std::chrono::steady_clock::now();

        if( now - m_last >= m_interval )
        {
            m_last = now;
            return true;
        }

        return false;
    }

private:
    std::chrono::milliseconds             m_interval;
    std::chrono::steady_clock::time_point m_last;
};
