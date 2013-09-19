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
 * Function rdtsc
 * Returns processor's time-stamp counter. Main purpose is precise time measuring of code
 * execution time.
 * @return unsigned long long - Value of time-stamp counter.
 */
#if defined(__i386__)
static __inline__ unsigned long long rdtsc()
{
    unsigned long long int x;
    __asm__ volatile ( ".byte 0x0f, 0x31" : "=A" ( x ) );

    return x;
}


#elif defined(__x86_64__)
static __inline__ unsigned long long rdtsc()
{
    unsigned hi, lo;
    __asm__ __volatile__ ( "rdtsc" : "=a" ( lo ), "=d" ( hi ) );

    return ( (unsigned long long) lo ) | ( ( (unsigned long long) hi ) << 32 );
}


#elif defined(__powerpc__)
static __inline__ unsigned long long rdtsc()
{
    unsigned long long int  result = 0;
    unsigned long int       upper, lower, tmp;
    __asm__ volatile (
        "0:                  \n"
        "\tmftbu   %0           \n"
        "\tmftb    %1           \n"
        "\tmftbu   %2           \n"
        "\tcmpw    %2,%0        \n"
        "\tbne     0b         \n"
        : "=r" ( upper ), "=r" ( lower ), "=r" ( tmp )
        );

    result  = upper;
    result  = result << 32;
    result  = result | lower;

    return result;
}


#endif /* __powerpc__ */

// Fixme: OS X version
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
    uint64_t    value;          /// Stored timer value
    bool        use_rdtsc;      /// Method of time measuring (rdtsc or tics)
};

/**
 * Function prof_start
 * Begins code execution time counting for a given profiling counter.
 * @param cnt is the counter which should be started.
 * @param use_rdtsc tells if processor's time-stamp counter should be used for time counting.
 *      Otherwise is system tics method will be used. IMPORTANT: time-stamp counter should not
 *      be used on multicore machines executing threaded code.
 */
static inline void prof_start( prof_counter* cnt, bool use_rdtsc )
{
    cnt->use_rdtsc = use_rdtsc;

    if( use_rdtsc )
    {
        cnt->value = rdtsc();
    }
    else
    {
        cnt->value = get_tics();
    }
}


/**
 * Function prof_stop
 * Ends code execution time counting for a given profiling counter.
 * @param cnt is the counter which should be stopped.
 */
static inline void prof_end( prof_counter* cnt )
{
    if( cnt->use_rdtsc )
        cnt->value = rdtsc() - cnt->value;
    else
        cnt->value = get_tics() - cnt->value;
}

#endif
