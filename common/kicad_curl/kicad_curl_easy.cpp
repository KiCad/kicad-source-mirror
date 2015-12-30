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

static size_t write_callback (void *contents, size_t size, size_t nmemb, void *userp);


KICAD_CURL_EASY::KICAD_CURL_EASY()
    : m_headers( NULL )
{
    m_CURL = curl_easy_init();

    if( m_CURL == NULL )
    {
        THROW_IO_ERROR( "Unable to initialize CURL session" );
    }

    m_Buffer.Payload = (char*)malloc( 1 );
    m_Buffer.Size = 0;

    curl_easy_setopt( m_CURL, CURLOPT_WRITEFUNCTION, write_callback );
    curl_easy_setopt( m_CURL, CURLOPT_WRITEDATA, (void *)&m_Buffer );
}


KICAD_CURL_EASY::~KICAD_CURL_EASY()
{
    free(m_Buffer.Payload);
    curl_easy_cleanup(m_CURL);
}


bool KICAD_CURL_EASY::SetURL( const std::string& aURL )
{
    if( SetOption<const char *>( CURLOPT_URL, aURL.c_str() ) == CURLE_OK )
    {
        return true;
    }
    return false;
}


bool KICAD_CURL_EASY::SetUserAgent( const std::string& aAgent )
{
    if( SetOption<const char *>( CURLOPT_USERAGENT, aAgent.c_str() ) == CURLE_OK )
    {
        return true;
    }
    return false;
}


bool KICAD_CURL_EASY::SetFollowRedirects( bool aFollow )
{
    if( SetOption<long>( CURLOPT_FOLLOWLOCATION , (aFollow ? 1 : 0) ) == CURLE_OK )
    {
        return true;
    }
    return false;
}


void KICAD_CURL_EASY::SetHeader( const std::string& aName, const std::string& aValue )
{
    std::string header = aName + ':' + aValue;
    m_headers = curl_slist_append( m_headers, header.c_str() );
}


std::string KICAD_CURL_EASY::GetErrorText(CURLcode code)
{
    return curl_easy_strerror(code);
}


static size_t write_callback( void *contents, size_t size, size_t nmemb, void *userp )
{
    /* calculate buffer size */
    size_t realsize = size * nmemb;

    /* cast pointer to fetch struct */
    struct KICAD_EASY_CURL_BUFFER *p = ( struct KICAD_EASY_CURL_BUFFER * ) userp;

    /* expand buffer */
    p->Payload = (char *) realloc( p->Payload, p->Size + realsize + 1 );

    /* check buffer */
    if ( p->Payload == NULL )
    {
        wxLogError( wxT( "Failed to expand buffer in curl_callback" ) );

        /* free buffer */
        free( p->Payload );

        return -1;
    }

    /* copy contents to buffer */
    memcpy( &(p->Payload[p->Size]), contents, realsize );

    /* set new buffer size */
    p->Size += realsize;

    /* ensure null termination */
    p->Payload[p->Size] = 0;

    /* return size */
    return realsize;
}


void KICAD_CURL_EASY::Perform()
{
    if( m_headers != NULL )
    {
        curl_easy_setopt( m_CURL, CURLOPT_HTTPHEADER, m_headers );
    }

    if( m_Buffer.Size > 0 )
    {
        free( m_Buffer.Payload );
        m_Buffer.Payload = (char*)malloc( 1 );
        m_Buffer.Size = 0;
    }

    CURLcode res = curl_easy_perform( m_CURL );
    if( res != CURLE_OK )
    {
        wxString msg = wxString::Format(
            _( "CURL Request Failed: %s" ),
            GetErrorText( res ) );

        THROW_IO_ERROR( msg );
    }
}