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

#include <kicad_curl/kicad_curl.h>
#include <kicad_curl/kicad_curl_easy.h>

#include <cstdarg>
#include <cstddef>
#include <exception>
#include <ki_exception.h>   // THROW_IO_ERROR
#include <sstream>


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
        std::string msg = "curl_easy_perform()=";
        msg += (int)res; msg += " "; msg += GetErrorText( res ).c_str();
        THROW_IO_ERROR( msg );
    }
}


void KICAD_CURL_EASY::SetHeader( const std::string& aName, const std::string& aValue )
{
    std::string header = aName + ':' + aValue;
    m_headers = curl_slist_append( m_headers, header.c_str() );
}


template <typename T> int KICAD_CURL_EASY::setOption( int aOption, T aArg )
{
    return curl_easy_setopt( m_CURL, (CURLoption)aOption, aArg );
}


const std::string KICAD_CURL_EASY::GetErrorText( int aCode )
{
    return curl_easy_strerror( (CURLcode)aCode );
}


bool KICAD_CURL_EASY::SetUserAgent( const std::string& aAgent )
{
    if( setOption<const char*>( CURLOPT_USERAGENT, aAgent.c_str() ) == CURLE_OK )
    {
        return true;
    }
    return false;
}


bool KICAD_CURL_EASY::SetURL( const std::string& aURL )
{
    if( setOption<const char *>( CURLOPT_URL, aURL.c_str() ) == CURLE_OK )
    {
        return true;
    }
    return false;
}


bool KICAD_CURL_EASY::SetFollowRedirects( bool aFollow )
{
    if( setOption<long>( CURLOPT_FOLLOWLOCATION , (aFollow ? 1 : 0) ) == CURLE_OK )
    {
        return true;
    }
    return false;
}
