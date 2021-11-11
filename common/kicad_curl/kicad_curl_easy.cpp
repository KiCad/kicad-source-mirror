/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2015 Mark Roszko <mark.roszko@gmail.com>
 * Copyright (C) 2015-2021 KiCad Developers, see AUTHORS.txt for contributors.
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

// kicad_curl_easy.h **must be** included before any wxWidgets header to avoid conflicts
// at least on Windows/msys2
#include <curl/curl.h>
#include <curl/easy.h>
#include <kicad_curl/kicad_curl.h>
#include <kicad_curl/kicad_curl_easy.h>

#include <cstdarg>
#include <cstddef>
#include <exception>
#include <sstream>
#include <wx/app.h>

#include <build_version.h>
#include <ki_exception.h>   // THROW_IO_ERROR
#include <kiplatform/app.h>
#include <kiplatform/environment.h>
#include <pgm_base.h>


struct CURL_PROGRESS
{
    KICAD_CURL_EASY*  curl;
    TRANSFER_CALLBACK callback;
    curl_off_t        last_run_time;
    curl_off_t        interval;
    CURL_PROGRESS( KICAD_CURL_EASY* c, TRANSFER_CALLBACK cb, curl_off_t i ) :
            curl( c ), callback( cb ), last_run_time( 0 ), interval( i )
    {
    }
};


static size_t write_callback( void* contents, size_t size, size_t nmemb, void* userp )
{
    size_t realsize = size * nmemb;

    std::string* p = (std::string*) userp;

    p->append( (const char*) contents, realsize );

    return realsize;
}


static size_t stream_write_callback( void* contents, size_t size, size_t nmemb, void* userp )
{
    size_t realsize = size * nmemb;

    std::ostream* p = (std::ostream*) userp;

    p->write( (const char*) contents, realsize );

    return realsize;
}


static int xferinfo( void* p, curl_off_t dltotal, curl_off_t dlnow, curl_off_t ultotal,
                     curl_off_t ulnow )
{
    CURL_PROGRESS* progress = (CURL_PROGRESS*) p;
    curl_off_t     curtime = 0;

    curl_easy_getinfo( progress->curl->GetCurl(), CURLINFO_TOTAL_TIME, &curtime );

    if( curtime - progress->last_run_time >= progress->interval )
    {
        progress->last_run_time = curtime;
        return progress->callback( dltotal, dlnow, ultotal, ulnow );
    }

    return CURLE_OK;
}


KICAD_CURL_EASY::KICAD_CURL_EASY() : m_headers( nullptr )
{
    // Call KICAD_CURL::Init() from in here every time, but only the first time
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

    // Only allow HTTP and HTTPS protocols
    curl_easy_setopt( m_CURL, CURLOPT_PROTOCOLS, CURLPROTO_HTTP | CURLPROTO_HTTPS );

    wxPlatformInfo platformInfo;
    wxString application( Pgm().App().GetAppName() );
    wxString version( GetBuildVersion() );
    wxString platform = "(" + wxGetOsDescription() + ";" + platformInfo.GetArchName();

#if defined( KICAD_BUILD_ARCH_X64 )
    platform << ";64-bit";
#elif defined( KICAD_BUILD_ARCH_X86 )
    platform << ";32-bit";
#elif defined( KICAD_BUILD_ARCH_ARM )
    platform << ";ARM 32-bit";
#elif defined( KICAD_BUILD_ARCH_ARM64 )
    platform << ";ARM 64-bit";
#endif

    platform << ")";

    wxString user_agent = "KiCad/" + version + " " + platform + " " + application;

    user_agent << "/" << GetBuildDate();
    setOption<const char*>( CURLOPT_USERAGENT, user_agent.ToStdString().c_str() );
    setOption( CURLOPT_ACCEPT_ENCODING, "gzip,deflate" );
}


KICAD_CURL_EASY::~KICAD_CURL_EASY()
{
    if( m_headers )
        curl_slist_free_all( m_headers );

    curl_easy_cleanup( m_CURL );
}


int KICAD_CURL_EASY::Perform()
{
    if( m_headers )
    {
        curl_easy_setopt( m_CURL, CURLOPT_HTTPHEADER, m_headers );
    }

    // bonus: retain worst case memory allocation, should re-use occur
    m_buffer.clear();

    CURLcode res = curl_easy_perform( m_CURL );

    return res;
}


void KICAD_CURL_EASY::SetHeader( const std::string& aName, const std::string& aValue )
{
    std::string header = aName + ':' + aValue;
    m_headers = curl_slist_append( m_headers, header.c_str() );
}


template <typename T>
int KICAD_CURL_EASY::setOption( int aOption, T aArg )
{
    return curl_easy_setopt( m_CURL, (CURLoption) aOption, aArg );
}


const std::string KICAD_CURL_EASY::GetErrorText( int aCode )
{
    return curl_easy_strerror( (CURLcode) aCode );
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
    if( setOption<const char*>( CURLOPT_URL, aURL.c_str() ) == CURLE_OK )
    {
        KIPLATFORM::ENV::PROXY_CONFIG cfg;

        // Unforunately on Windows land, proxies can be configured depending on destination url
        // So we also check and set any proxy config here
        if( KIPLATFORM::ENV::GetSystemProxyConfig( aURL, cfg ) )
        {
            curl_easy_setopt( m_CURL, CURLOPT_PROXY, static_cast<const char*>( cfg.host.c_str() ) );
            if( cfg.username != "" )
            {
                curl_easy_setopt( m_CURL, CURLOPT_PROXYUSERNAME,
                                  static_cast<const char*>( cfg.username.c_str() ) );
            }

            if( cfg.password != "" )
            {
                curl_easy_setopt( m_CURL, CURLOPT_PROXYPASSWORD,
                                  static_cast<const char*>( cfg.password.c_str() ) );
            }
        }

        return true;
    }

    return false;
}


bool KICAD_CURL_EASY::SetFollowRedirects( bool aFollow )
{
    if( setOption<long>( CURLOPT_FOLLOWLOCATION, ( aFollow ? 1 : 0 ) ) == CURLE_OK )
    {
        return true;
    }

    return false;
}


std::string KICAD_CURL_EASY::Escape( const std::string& aUrl )
{
    char* escaped = curl_easy_escape( m_CURL, aUrl.c_str(), aUrl.length() );

    std::string ret( escaped );
    curl_free( escaped );

    return ret;
}


bool KICAD_CURL_EASY::SetTransferCallback( const TRANSFER_CALLBACK& aCallback, size_t aInterval )
{
    progress = std::make_unique<CURL_PROGRESS>( this, aCallback, (curl_off_t) aInterval );
    setOption( CURLOPT_XFERINFOFUNCTION, xferinfo );
    setOption( CURLOPT_XFERINFODATA, progress.get() );
    setOption( CURLOPT_NOPROGRESS, 0L );
    return true;
}


bool KICAD_CURL_EASY::SetOutputStream( const std::ostream* aOutput )
{
    curl_easy_setopt( m_CURL, CURLOPT_WRITEFUNCTION, stream_write_callback );
    curl_easy_setopt( m_CURL, CURLOPT_WRITEDATA, (void*) aOutput );
    return true;
}


int KICAD_CURL_EASY::GetTransferTotal( uint64_t& aDownloadedBytes ) const
{
    curl_off_t dl;
    int        result = curl_easy_getinfo( m_CURL, CURLINFO_SIZE_DOWNLOAD_T, &dl );
    aDownloadedBytes = (uint64_t) dl;
    return result;
}
