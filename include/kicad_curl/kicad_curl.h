/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2015 Mark Roszko <mark.roszko@gmail.com>
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
#ifndef KICAD_CURL_H_
#define KICAD_CURL_H_

/*
 * KICAD_CURL.h must be included before wxWidgets because on Windows,
 * wxWidgets ends up including windows.h before winsocks2.h inside curl
 * this causes build warnings
 * Because we are before wx, we must explicitly define we are building with unicode.
 * wxWidgets defaults to supporting unicode now, so this should be safe.
 */
#if defined(_WIN32)
    #ifndef UNICODE
    #    define UNICODE
    #endif

    #ifndef _UNICODE
    #    define _UNICODE
    #endif
#endif

#include <kicommon.h>
#include <curl/curl.h>
#include <string>
#include <shared_mutex>

// CURL_EXTERN expands to dllimport on MinGW which causes gcc warnings.  This really should
// expand to nothing on MinGW.
#if defined( __MINGW32__)
#  if defined( CURL_EXTERN )
#    undef CURL_EXTERN
#    define CURL_EXTERN
#  endif
#endif

/**
 * Simple wrapper class to call curl_global_init and curl_global_cleanup for KiCad.
 */
class KICOMMON_API KICAD_CURL
{
public:
    /**
     * Call curl_global_init for the application. It must be used only once
     * and before any curl functions that perform requests.
     *
     * @return True if successful, false if CURL returned an error.
     * @throw IO_ERROR on failure, hopefully with helpful text in it.
     */
    static void Init();

    /**
     * Call curl_global_cleanup for the application. It must be used only after
     * curl_global_init was called.
     */
    static void Cleanup();

    /**
     * Returns the mutex for shared locking when performing curl operations.
     * Unique locking is performed when shutting down.
     */
    static std::shared_mutex& Mutex();

    /**
     * Returns true if all curl operations should terminate.
     */
    static bool IsShuttingDown();

    /**
     * Wrapper for curl_version(). Reports back a short string of loaded libraries.
     *
     * @return String reported by libcurl and owned by it.
     * @throw IO_ERROR on failure, hopefully with helpful text in it.
     */
    static const char* GetVersion()
    {
        return curl_version();
    }
};

#endif // KICAD_CURL_H_
