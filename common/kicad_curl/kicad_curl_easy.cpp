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

#include <kicad_curl/kicad_curl_easy.h>

#include <cstddef>
#include <exception>
#include <stdarg.h>
#include <sstream>
#include <richio.h>


static size_t write_callback( void* contents, size_t size, size_t nmemb, void* userp )
{
    size_t realsize = size * nmemb;

    std::string* p = (std::string*) userp;

    p->append( (const char*) contents, realsize );

    return realsize;
}


KICAD_CURL_EASY::KICAD_CURL_EASY() :
    m_headers( NULL )
{
    // Call KICAD_CURL::Init() from in here everytime, but only the first time
    // will incur any overhead.  This strategy ensures that libcurl is never loaded
    // unless it is needed.

    KICAD_CURL::Init();

    m_CURL = curl_easy_init();

    if( !m_CURL )
    {
        THROW_IO_ERROR( "Unable to initialize CURL session" );
    }

    curl_easy_setopt( m_CURL, CURLOPT_WRITEFUNCTION, write_callback );
    curl_easy_setopt( m_CURL, CURLOPT_WRITEDATA, (void*) &m_buffer );
}


KICAD_CURL_EASY::~KICAD_CURL_EASY()
{
    if( m_headers )
        curl_slist_free_all( m_headers );

    curl_easy_cleanup( m_CURL );
}


void KICAD_CURL_EASY::Perform()
{
    if( m_headers )
    {
        curl_easy_setopt( m_CURL, CURLOPT_HTTPHEADER, m_headers );
    }

    // bonus: retain worst case memory allocation, should re-use occur
    m_buffer.clear();

    CURLcode res = curl_easy_perform( m_CURL );

    if( res != CURLE_OK )
    {
        std::string msg = StrPrintf( "curl_easy_perform()=%d: %s",
                            res, GetErrorText( res ).c_str() );
        THROW_IO_ERROR( msg );
    }
}
