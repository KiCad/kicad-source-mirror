/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2012 SoftPLC Corporation, Dick Hollenbeck <dick@softplc.com>
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


#include <config.h>
#include <cstdint>

#if defined( _WIN32 )

#include <windows.h>

int64_t GetRunningMicroSecs()
{
    FILETIME now;

    GetSystemTimeAsFileTime( &now );
    uint64_t t = ( UINT64( now.dwHighDateTime ) << 32 ) + now.dwLowDateTime;
    t /= 10;

    return int64_t( t );
}

#elif defined( HAVE_CLOCK_GETTIME )

#include <ctime>

int64_t GetRunningMicroSecs()
{
    struct timespec now;

    clock_gettime( CLOCK_MONOTONIC, &now );

    int64_t usecs = (int64_t) now.tv_sec * 1000000 + now.tv_nsec / 1000;
    //    unsigned msecs = (now.tv_nsec / (1000*1000)) + now.tv_sec * 1000;

    return usecs;
}


#elif defined( HAVE_GETTIMEOFDAY_FUNC )

#include <sys/time.h>
int64_t GetRunningMicroSecs()
{
    timeval tv;

    gettimeofday( &tv, 0 );

    return (int64_t) tv.tv_sec * 1000000 + tv.tv_usec;
}

#endif
