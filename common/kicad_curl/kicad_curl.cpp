/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2015 Mark Roszko <mark.roszko@gmail.com>
 * Copyright (C) 2016 SoftPLC Corporation, Dick Hollenbeck <dick@softplc.com>
 * Copyright (C) 2015-2020 KiCad Developers, see CHANGELOG.TXT for contributors.
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
 * http://www.gnu.org/licenses/old-licenses/gpl-2.0.html
 * or you may search the http://www.gnu.org website for the version 2 license,
 * or you may write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
 */

// kicad_curl.h must be included before wx headers, to avoid
// conflicts for some defines, at least on Windows
#include <kicad_curl/kicad_curl.h>

#include <mutex>
#include <ki_exception.h>   // THROW_IO_ERROR



// These are even more private than class members, and since there is only
// one instance of KICAD_CURL ever, these statics are hidden here to simplify the
// client (API) header file.
static volatile bool s_initialized;

static std::mutex s_lock;        // for s_initialized

/// At process termination, using atexit() keeps the CURL stuff out of the
/// singletops and PGM_BASE.
static void at_terminate()
{
    KICAD_CURL::Cleanup();
}


void KICAD_CURL::Init()
{
    // We test s_initialized twice in an effort to avoid
    // unnecessarily locking s_lock.  This understands that the common case
    // will not need to lock.
    if( !s_initialized )
    {
        std::lock_guard<std::mutex> lock( s_lock );

        if( !s_initialized )
        {
            if( curl_global_init( CURL_GLOBAL_ALL ) != CURLE_OK )
            {
                THROW_IO_ERROR( "curl_global_init() failed." );
            }

            s_initialized = true;
        }
    }
}


void KICAD_CURL::Cleanup()
{
    /*

    Calling lock_guard() from a static destructor will typically be bad, since the
    s_lock may already have been statically destroyed itself leading to a boost
    exception. (Remember C++ does not provide certain sequencing of static
    destructor invocation.)

    To prevent this we test s_initialized twice, which ensures that the lock_guard
    is only instantiated on the first call, which should be from
    PGM_BASE::destroy() which is first called earlier than static destruction.
    Then when called again from the actual PGM_BASE::~PGM_BASE() function,
    lock_guard will not be instantiated because s_initialized will be false.

    */

    if( s_initialized )
    {
        std::lock_guard<std::mutex> lock( s_lock );

        if( s_initialized )
        {
            curl_global_cleanup();

            atexit( &at_terminate );

            s_initialized = false;
        }
    }
}


std::string KICAD_CURL::GetSimpleVersion()
{
    if( !s_initialized )
        Init();

    curl_version_info_data* info = curl_version_info( CURLVERSION_NOW );

    std::string res;

    if( info->version )
    {
        res += "libcurl version: " + std::string( info->version );
    }

    res += " (";

    if( info->features & CURL_VERSION_SSL )
    {
        res += "with SSL - ";
        res += std::string( info->ssl_version );
    }
    else
    {
        res += "without SSL";
    }
    res += ")";

    return res;
}

std::string GetKicadCurlVersion()
{
    return KICAD_CURL::GetVersion();
}

std::string GetCurlLibVersion()
{
    return LIBCURL_VERSION;
}
