
/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2012 SoftPLC Corporation, Dick Hollenbeck <dick@softplc.com>
 * Copyright (C) 2012 KiCad Developers, see change_log.txt for contributors.
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


#include <config.h>
#include <common.h>

#if defined(_WIN32)

#define WIN32_LEAN_AND_MEAN 1
#include <windows.h>

unsigned GetRunningMicroSecs()
{
    FILETIME    now;

    GetSystemTimeAsFileTime( &now );
    unsigned long long t = (UINT64(now.dwHighDateTime) << 32) + now.dwLowDateTime;
    t /= 10;

    return unsigned( t );
}

#elif defined(HAVE_CLOCK_GETTIME)

#include <time.h>

unsigned GetRunningMicroSecs()
{
    struct timespec	now;

    clock_gettime( CLOCK_MONOTONIC, &now );

    unsigned usecs = ((unsigned)now.tv_nsec)/1000 + ((unsigned)now.tv_sec) * 1000000;
//    unsigned msecs = (now.tv_nsec / (1000*1000)) + now.tv_sec * 1000;

    return usecs;
}


#elif defined(HAVE_GETTIMEOFDAY_FUNC)

#include <sys/time.h>
unsigned GetRunningMicroSecs()
{
    timeval tv;

    gettimeofday( &tv, 0 );

    return (tv.tv_sec * 1000000) + tv.tv_usec;
}

#endif

