/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2013 CERN
 * @author Tomasz Wlostowski <tomasz.wlostowski@cern.ch>
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
 * @file profile.h:
 * @brief Simple profiling functions for measuring code execution time.
 */

#ifndef __TPROFILE_H
#define __TPROFILE_H

#include <sys/time.h>
#include <stdint.h>

/**
 * Function get_tics
 * Returns the number of microseconds that have elapsed since the system was started.
 * @return uint64_t Number of microseconds.
 */
static inline uint64_t get_tics()
{
    struct timeval tv;
    gettimeofday( &tv, NULL );

    return (uint64_t) tv.tv_sec * 1000000ULL + (uint64_t) tv.tv_usec;
}

/**
 * Structure for storing data related to profiling counters.
 */
struct prof_counter
{
    uint64_t start, end;         // Stored timer value

    uint64_t usecs() const
    {
        return end - start;
    }

    float msecs() const
    {
        return ( end - start ) / 1000.0;
    }
};

/**
 * Function prof_start
 * Begins code execution time counting for a given profiling counter.
 * @param aCnt is the counter which should be started.
 * use_rdtsc tells if processor's time-stamp counter should be used for time counting.
 *      Otherwise is system tics method will be used. IMPORTANT: time-stamp counter should not
 *      be used on multicore machines executing threaded code.
 */
static inline void prof_start( prof_counter* aCnt )
{
    aCnt->start = get_tics();
}

/**
 * Function prof_stop
 * Ends code execution time counting for a given profiling counter.
 * @param aCnt is the counter which should be stopped.
 */
static inline void prof_end( prof_counter* aCnt )
{
    aCnt->end = get_tics();
}

#endif
