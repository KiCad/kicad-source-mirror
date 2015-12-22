/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2015 Mark Roszko <mark.roszko@gmail.com>
 * Copyright (C) 2015 KiCad Developers, see CHANGELOG.TXT for contributors.
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
#if defined(WIN32)
    #ifndef UNICODE
    #    define UNICODE
    #endif

    #ifndef _UNICODE
    #    define _UNICODE
    #endif
#endif

#include <curl/curl.h>
#include <string>

/**
 * Class KICAD_CURL
 * simple wrapper class to call curl_global_init and curl_global_cleanup for KiCad.
 */
class KICAD_CURL
{
public:
    /**
     * Function Init
     * calls curl_global_init for the application. It must be used only once
     * and before any curl functions that perform requests.
     *
     * @return bool - True if successful, false if CURL returned an error
     */
    static bool Init();

    /**
     * Function Cleanup
     * calls curl_global_cleanup for the application. It must be used only after
     * curl_global_init was called.
     */
    static void Cleanup();

    /**
     * Function GetVersion
     * wrapper for curl_version(). Reports back a short string of loaded libraries.
     *
     * @return std::string - String reported by libcurl
     */
    static std::string GetVersion();

    /**
     * Function GetSimpleVersion
     * Reports back curl version only and SSL library support
     *
     * @return std::string - Generated version string
     */
    static std::string GetSimpleVersion();
private:
    static bool m_initialized;
};

#endif // KICAD_CURL_H_