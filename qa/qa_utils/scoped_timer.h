/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2018 KiCad Developers, see AUTHORS.txt for contributors.
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

#ifndef SCOPED_TIMER_H
#define SCOPED_TIMER_H

#include <chrono>

/**
 * A simple RAII class to measure the time of an operation.
 *
 * ON construction, a timer is started, and on destruction, the timer is
 * ended, and the time difference is written into the given duration
 */
template<typename DURATION>
class SCOPED_TIMER
{
    using CLOCK = std::chrono::steady_clock;
    using TIME_PT = std::chrono::time_point<CLOCK>;

public:
    SCOPED_TIMER( DURATION& aDuration ):
        m_duration( aDuration )
    {
        m_start = CLOCK::now();
    }

    ~SCOPED_TIMER()
    {
        const auto end = CLOCK::now();

        // update the output
        m_duration = std::chrono::duration_cast<DURATION>( end - m_start );
    }

private:

    DURATION& m_duration;
    TIME_PT m_start;
};

#endif // SCOPED_TIMER_h