/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
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
 * along with this program. If not, see <https://www.gnu.org/licenses/>.
 */

#ifndef QA_TIMING_H
#define QA_TIMING_H

#include <chrono>
#include <utility>

#if !defined( _WIN32 )
#include <time.h>
#endif

namespace KI_TEST
{
/**
 * Whether wall-clock bounds are meaningful in this build.
 *
 * Checked STL iterators (MSVC debug runtime, _GLIBCXX_DEBUG) slow STL-heavy code by 5-10x against the build the
 * bounds were calibrated on, so the tests only report their time there.
 */
inline bool TimingBoundsApply()
{
#if( defined( _ITERATOR_DEBUG_LEVEL ) && _ITERATOR_DEBUG_LEVEL > 0 ) || defined( _GLIBCXX_DEBUG )
    return false;
#else
    return true;
#endif
}

/**
 * CPU time used so far by this process, including helper threads such as the solver's std::async workers.
 *
 * Other processes on the machine do not inflate it.  Windows ticks its CPU clocks every 15.6 ms, too coarse for
 * the bounds, so it falls back to wall time there.
 */
inline std::chrono::nanoseconds ProcessCpuTime()
{
#if defined( _WIN32 )
    return std::chrono::steady_clock::now().time_since_epoch();
#else
    timespec ts;
    clock_gettime( CLOCK_PROCESS_CPUTIME_ID, &ts );

    return std::chrono::seconds( ts.tv_sec ) + std::chrono::nanoseconds( ts.tv_nsec );
#endif
}

/// Run a block and return the process CPU time it used.
template <typename FUNC>
std::chrono::microseconds ProcessCpuTimeOf( FUNC&& aBlock )
{
    std::chrono::nanoseconds start = ProcessCpuTime();
    aBlock();

    return std::chrono::duration_cast<std::chrono::microseconds>( ProcessCpuTime() - start );
}

/// ProcessCpuTimeOf in whole milliseconds.
template <typename FUNC>
long long ProcessCpuMsOf( FUNC&& aBlock )
{
    return ProcessCpuTimeOf( std::forward<FUNC>( aBlock ) ).count() / 1000;
}
} // namespace KI_TEST

#endif // QA_TIMING_H
