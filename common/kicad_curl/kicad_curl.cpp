/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2015 Mark Roszko <mark.roszko@gmail.com>
 * Copyright (C) 2016 SoftPLC Corporation, Dick Hollenbeck <dick@softplc.com>
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
#include <atomic>
#include <ki_exception.h>   // THROW_IO_ERROR


static std::shared_mutex s_curlMutex;
static std::atomic<bool> s_curlShuttingDown = false;


void KICAD_CURL::Init()
{
    s_curlShuttingDown = false;

    if( curl_global_init( CURL_GLOBAL_ALL ) != CURLE_OK )
        THROW_IO_ERROR( "curl_global_init() failed." );
}


void KICAD_CURL::Cleanup()
{
    s_curlShuttingDown = true;

    std::unique_lock lock( s_curlMutex );

    curl_global_cleanup();
}


std::shared_mutex& KICAD_CURL::Mutex()
{
    return s_curlMutex;
}


bool KICAD_CURL::IsShuttingDown()
{
    return s_curlShuttingDown;
}


std::string GetKicadCurlVersion()
{
    return KICAD_CURL::GetVersion();
}


std::string GetCurlLibVersion()
{
    return LIBCURL_VERSION;
}
