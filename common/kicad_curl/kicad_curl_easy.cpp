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
#include <shared_mutex>
#include <wx/app.h>

#include <build_version.h>
#include <ki_exception.h>   // THROW_IO_ERROR
#include <kiplatform/app.h>
#include <kiplatform/environment.h>

#include <kiplatform/policy.h>
#include <policy_keys.h>

struct CURL_PROGRESS
{
    KICAD_CURL_EASY*  m_Curl;
    TRANSFER_CALLBACK m_Callback;
    curl_off_t        m_Last_run_time;
    curl_off_t        m_Interval;

    CURL_PROGRESS( KICAD_CURL_EASY* aCURL, TRANSFER_CALLBACK aCallback, curl_off_t aInterval ) :
            m_Curl( aCURL ),
            m_Callback( aCallback ),
            m_Last_run_time( 0 ),
            m_Interval( aInterval )
    {
    }
};


static size_t write_callback( void* aContents, size_t aSize, size_t aNmemb, void* aUserp )
{
    size_t realsize = aSize * aNmemb;

    std::string* p = static_cast<std::string*>( aUserp );

    p->append( static_cast<const char*>( aContents ), realsize );

    return realsize;
}


static size_t stream_write_callback( void* aContents, size_t aSize, size_t aNmemb, void* aUserp )
{
    size_t realsize = aSize * aNmemb;

    std::ostream* p = static_cast<std::ostream*>( aUserp );

    p->write( static_cast<const char*>( aContents ), realsize );

    return realsize;
}


#if LIBCURL_VERSION_NUM >= 0x072000 // 7.32.0

static int xferinfo( void* aProgress, curl_off_t aDLtotal, curl_off_t aDLnow, curl_off_t aULtotal,
                     curl_off_t aULnow )
{
    if( KICAD_CURL::IsShuttingDown() )
        return 1; // Should abort the operation

    CURL_PROGRESS* progress = static_cast<CURL_PROGRESS*>( aProgress );
    curl_off_t     curtime  = 0;

    curl_easy_getinfo( progress->m_Curl->GetCurl(), CURLINFO_TOTAL_TIME, &curtime );

    if( curtime - progress->m_Last_run_time >= progress->m_Interval )
    {
        progress->m_Last_run_time = curtime;
        return progress->m_Callback( aDLtotal, aDLnow, aULtotal, aULnow );
    }

    return CURLE_OK;
}

#else

static int progressinfo( void* aProgress, double aDLtotal, double aDLnow, double aULtotal,
                         double aULnow )
{
    return xferinfo( aProgress, static_cast<curl_off_t>( aDLtotal ),
                     static_cast<curl_off_t>( aDLnow ), static_cast<curl_off_t>( aULtotal ),
                     static_cast<curl_off_t>( aULnow ) );
}

#endif


KICAD_CURL_EASY::KICAD_CURL_EASY() :
        m_headers( nullptr ),
        m_curlSharedLock( KICAD_CURL::Mutex() )
{
    m_CURL = curl_easy_init();

    if( !m_CURL )
        THROW_IO_ERROR( "Unable to initialize CURL session" );

    curl_easy_setopt( m_CURL, CURLOPT_WRITEFUNCTION, write_callback );
    curl_easy_setopt( m_CURL, CURLOPT_WRITEDATA, static_cast<void*>( &m_buffer ) );

    // Only allow HTTP and HTTPS protocols
#if LIBCURL_VERSION_NUM >= 0x075500     // version 7.85.0
    curl_easy_setopt(m_CURL, CURLOPT_PROTOCOLS_STR, "http,https");
#else
    curl_easy_setopt( m_CURL, CURLOPT_PROTOCOLS, CURLPROTO_HTTP | CURLPROTO_HTTPS );
#endif

#ifdef _WIN32
    long sslOpts = CURLSSLOPT_NATIVE_CA;

    POLICY_CURL_SSL_REVOKE policyState = KIPLATFORM::POLICY::GetPolicyEnum<POLICY_CURL_SSL_REVOKE>(
            POLICY_KEY_REQUESTS_CURL_REVOKE );

    if( policyState == POLICY_CURL_SSL_REVOKE::BEST_EFFORT )
    {
        sslOpts |= CURLSSLOPT_REVOKE_BEST_EFFORT;
    }
    else if( policyState == POLICY_CURL_SSL_REVOKE::NONE )
    {
        sslOpts |= CURLSSLOPT_NO_REVOKE;
    }

    // We need this to use the Windows Certificate store
    curl_easy_setopt( m_CURL, CURLOPT_SSL_OPTIONS, sslOpts );
#endif

    if( wxGetEnv( wxT( "KICAD_CURL_VERBOSE" ), nullptr ) )
    {
        // note: curl verbose will end up in stderr
        curl_easy_setopt( m_CURL, CURLOPT_VERBOSE, 1L );
    }

    wxString application( wxS( "KiCad" ) );
    wxString version( GetBuildVersion() );
    wxString platform = wxS( "(" ) + wxGetOsDescription() + wxS( ";" ) + GetPlatformGetBitnessName();

#if defined( KICAD_BUILD_ARCH_X64 )
    platform << wxS( ";64-bit" );
#elif defined( KICAD_BUILD_ARCH_X86 )
    platform << wxS( ";32-bit" );
#elif defined( KICAD_BUILD_ARCH_ARM )
    platform << wxS( ";ARM 32-bit" );
#elif defined( KICAD_BUILD_ARCH_ARM64 )
    platform << wxS( ";ARM 64-bit" );
#endif

    platform << wxS( ")" );

    wxString user_agent = wxS( "KiCad/" ) + version + wxS( " " ) + platform + wxS( " " ) + application;

    user_agent << wxS( "/" ) << GetBuildDate();
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
        curl_easy_setopt( m_CURL, CURLOPT_HTTPHEADER, m_headers );

    // bonus: retain worst case memory allocation, should re-use occur
    m_buffer.clear();

    return curl_easy_perform( m_CURL );
}


void KICAD_CURL_EASY::SetHeader( const std::string& aName, const std::string& aValue )
{
    std::string header = aName + ':' + aValue;
    m_headers = curl_slist_append( m_headers, header.c_str() );
}


template <typename T>
int KICAD_CURL_EASY::setOption( int aOption, T aArg )
{
    return curl_easy_setopt( m_CURL, static_cast<CURLoption>( aOption ), aArg );
}


const std::string KICAD_CURL_EASY::GetErrorText( int aCode )
{
    return curl_easy_strerror( static_cast<CURLcode>( aCode ) );
}


bool KICAD_CURL_EASY::SetUserAgent( const std::string& aAgent )
{
    if( setOption<const char*>( CURLOPT_USERAGENT, aAgent.c_str() ) == CURLE_OK )
        return true;

    return false;
}


bool KICAD_CURL_EASY::SetPostFields( const std::vector<std::pair<std::string, std::string>>& aFields )
{
    std::string postfields;

    for( size_t i = 0; i < aFields.size(); i++ )
    {
        if( i > 0 )
            postfields += "&";

        postfields += Escape( aFields[i].first );
        postfields += "=";
        postfields += Escape( aFields[i].second );
    }

    if( setOption<const char*>( CURLOPT_COPYPOSTFIELDS, postfields.c_str() ) != CURLE_OK )
        return false;

    return true;
}


bool KICAD_CURL_EASY::SetPostFields( const std::string& aField )
{
    if( setOption<const char*>( CURLOPT_COPYPOSTFIELDS, aField.c_str() ) != CURLE_OK )
        return false;

    return true;
}


bool KICAD_CURL_EASY::SetURL( const std::string& aURL )
{
    if( setOption<const char*>( CURLOPT_URL, aURL.c_str() ) == CURLE_OK )
    {
        KIPLATFORM::ENV::PROXY_CONFIG cfg;

        // Unfortunately on Windows land, proxies can be configured depending on destination url
        // So we also check and set any proxy config here
        if( KIPLATFORM::ENV::GetSystemProxyConfig( aURL, cfg ) )
        {
            curl_easy_setopt( m_CURL, CURLOPT_PROXY, static_cast<const char*>( cfg.host.c_str() ) );

            if( !cfg.username.empty() )
            {
                curl_easy_setopt( m_CURL, CURLOPT_PROXYUSERNAME,
                                  static_cast<const char*>( cfg.username.c_str() ) );
            }

            if( !cfg.password.empty() )
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
        return true;

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
    progress = std::make_unique<CURL_PROGRESS>( this, aCallback, static_cast<curl_off_t>( aInterval ) );

#if LIBCURL_VERSION_NUM >= 0x072000 // 7.32.0
    setOption( CURLOPT_XFERINFOFUNCTION, xferinfo );
    setOption( CURLOPT_XFERINFODATA, progress.get() );
#else
    setOption( CURLOPT_PROGRESSFUNCTION, progressinfo );
    setOption( CURLOPT_PROGRESSDATA, progress.get() );
#endif

    setOption( CURLOPT_NOPROGRESS, 0L );
    return true;
}


bool KICAD_CURL_EASY::SetOutputStream( const std::ostream* aOutput )
{
    curl_easy_setopt( m_CURL, CURLOPT_WRITEFUNCTION, stream_write_callback );
    curl_easy_setopt( m_CURL, CURLOPT_WRITEDATA, reinterpret_cast<const void*>( aOutput ) );
    return true;
}


int KICAD_CURL_EASY::GetTransferTotal( uint64_t& aDownloadedBytes ) const
{
#if LIBCURL_VERSION_NUM >= 0x073700 // 7.55.0
    curl_off_t dl;
    int        result = curl_easy_getinfo( m_CURL, CURLINFO_SIZE_DOWNLOAD_T, &dl );
    aDownloadedBytes  = static_cast<uint64_t>( dl );
#else
    double dl;
    int    result = curl_easy_getinfo( m_CURL, CURLINFO_SIZE_DOWNLOAD, &dl );
    aDownloadedBytes = static_cast<uint64_t>( dl );
#endif

    return result;
}


int KICAD_CURL_EASY::GetResponseStatusCode()
{
    long http_code = 0;
    curl_easy_getinfo( m_CURL, CURLINFO_RESPONSE_CODE, &http_code );

    return static_cast<int>( http_code );
}
