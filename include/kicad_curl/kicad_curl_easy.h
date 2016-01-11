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
#ifndef KICAD_CURL_EASY_H_
#define KICAD_CURL_EASY_H_

/*
 * KICAD_CURL_EASY.h must included before wxWidgets because on Windows,
 * wxWidgets ends up including windows.h before winsocks2.h inside curl
 * this causes build warnings
 * Because we are before wx, we must explicitly define we are building with unicode
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


#include <string>
#include <curl/curl.h>
#include <kicad_curl/kicad_curl.h>


/**
 * Class KICAD_CURL_EASY
 * wrapper interface around the curl_easy API
 *
 * Handling of using the curl_easy API to make a request and save the response to
 * a memory buffer
 *
 * Here is a small example usage:
 * @code
 *   KICAD_CURL_EASY curl;
 *
 *   curl.SetURL( "http://github.com" );
 *   curl.SetUserAgent( <http-client-indentifier> );
 *   curl.SetHeader( "Accept", "application/json" );
 *   curl.Perform();
 * @endcode
 */
class KICAD_CURL_EASY
{
public:
    KICAD_CURL_EASY();
    ~KICAD_CURL_EASY();

    /**
     * Function perform
     * equivalent to curl_easy_perform. Executes the request
     * that was previously setup.
     *
     * @throw IO_ERROR, if there is a CURL request error
     */
    void Perform();

    /**
     * Function SetHeader
     * sets an arbitrary header for the HTTP(s) request.
     *
     * @param aName is the left hand side of the header, i.e. Accept without the colon
     * @param aValue is the right hand side of the header, i.e. application/json
     */
    void SetHeader( const std::string& aName, const std::string& aValue )
    {
        std::string header = aName + ':' + aValue;
        m_headers = KICAD_CURL::slist_append( m_headers, header.c_str() );
    }

    /**
     * Function SetUserAgent
     * sets the request user agent
     *
     * @param aAgent is the string to set for the user agent
     * @return bool - True if successful, false if not
     */
    bool SetUserAgent( const std::string& aAgent )
    {
        if( SetOption<const char*>( CURLOPT_USERAGENT, aAgent.c_str() ) == CURLE_OK )
        {
            return true;
        }
        return false;
    }

    /**
     * Function SetURL
     * sets the request URL
     *
     * @param aURL is the URL
     * @return bool - True if successful, false if not
     */
    bool SetURL( const std::string& aURL )
    {
        if( SetOption<const char *>( CURLOPT_URL, aURL.c_str() ) == CURLE_OK )
        {
            return true;
        }
        return false;
    }

    /**
     * Function SetFollowRedirects
     * enables the following of HTTP(s) and other redirects, by default curl
     * does not follow redirects.
     *
     * @param aFollow is a boolean where true will enable following redirects
     * @return bool - True if successful, false if not
     */
    bool SetFollowRedirects( bool aFollow )
    {
        if( SetOption<long>( CURLOPT_FOLLOWLOCATION , (aFollow ? 1 : 0) ) == CURLE_OK )
        {
            return true;
        }
        return false;
    }

    /**
     * Function GetErrorText
     * fetches CURL's "friendly" error string for a given error code
     *
     * @param aCode is CURL error code
     * @return const std::string - the corresponding error string for the given code
     */
    const std::string GetErrorText( CURLcode aCode )
    {
        return KICAD_CURL::easy_strerror( aCode );
    }

    /**
     * Function SetOption
     * sets a curl option, only supports single parameter curl options
     *
     * @param aOption is CURL option, see CURL manual for options
     * @param aArg is the argument being passed to CURL, ensure it is the right type per manual
     * @return CURLcode - CURL error code, will return CURLE_OK unless a problem was encountered
     */
    template <typename T> CURLcode SetOption( CURLoption aOption, T aArg )
    {
        return KICAD_CURL::easy_setopt( m_CURL, aOption, aArg );
    }

    /**
     * Function GetBuffer
     * returns a const reference to the recevied data buffer
     */
    const std::string& GetBuffer()
    {
        return m_buffer;
    }

private:
    CURL*           m_CURL;
    curl_slist*     m_headers;
    std::string     m_buffer;
};

#endif // KICAD_CURL_EASY_H_
