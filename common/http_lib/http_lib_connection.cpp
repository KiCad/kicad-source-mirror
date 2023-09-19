/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2023 Andre F. K. Iwers <iwers11@gmail.com>
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation, either version 3 of the License, or (at your
 * option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <wx/log.h>
#include <fmt/core.h>
#include <wx/translation.h>

#include <boost/algorithm/string.hpp>
#include <nlohmann/json.hpp>
#include <wx/base64.h>

#include <kicad_curl/kicad_curl_easy.h>
#include <curl/curl.h>

#include <http_lib/http_lib_connection.h>

const char* const traceHTTPLib = "KICAD_HTTP_LIB";


HTTP_LIB_CONNECTION::HTTP_LIB_CONNECTION( const HTTP_LIB_SOURCE& aSource, bool aTestConnectionNow )
{
    m_rootURL = aSource.root_url;
    m_token = aSource.token;
    m_sourceType = aSource.type;

    if( aTestConnectionNow )
    {
        ValidateHTTPLibraryEndpoints();
    }
}


HTTP_LIB_CONNECTION::~HTTP_LIB_CONNECTION()
{
    // Do nothing
}


bool HTTP_LIB_CONNECTION::ValidateHTTPLibraryEndpoints()
{
    std::string res = "";

    std::unique_ptr<KICAD_CURL_EASY> m_curl = createCurlEasyObject();
    m_curl->SetURL( m_rootURL );

    try
    {
        m_curl->Perform();

        res = m_curl->GetBuffer();

        if( !checkServerResponse( m_curl ) )
        {
            return false;
        }

        if( res.length() == 0 )
        {
            m_lastError += wxString::Format( _( "KiCad received an empty response!" ) + "\n" );
        }
        else
        {
            nlohmann::json response = nlohmann::json::parse( res );

            // Check that the endpoints exist, if not fail.
            if( !response.at( http_endpoint_categories ).empty()
                && !response.at( http_endpoint_parts ).empty() )
                m_enpointValid = true;
            else
                m_enpointValid = false;
        }
    }
    catch( const std::exception& e )
    {
        m_lastError += wxString::Format( _( "Error: %s" ) + "\n" + _( "API Response:  %s" ) + "\n",
                                         e.what(), res );

        wxLogTrace( traceHTTPLib,
                    wxT( "ValidateHTTPLibraryEndpoints: Exception occurred while testing the API "
                         "connection: %s" ),
                    m_lastError );

        m_enpointValid = false;
    }

    if( IsValidEnpoint() )
    {
        syncCategories();
    }

    return IsValidEnpoint();
}


bool HTTP_LIB_CONNECTION::IsValidEnpoint() const
{
    return m_enpointValid;
}


bool HTTP_LIB_CONNECTION::syncCategories()
{
    if( !IsValidEnpoint() )
    {
        wxLogTrace( traceHTTPLib, wxT( "syncCategories: without valid connection!" ) );
        return false;
    }

    std::string res = "";

    std::unique_ptr<KICAD_CURL_EASY> m_curl = createCurlEasyObject();
    m_curl->SetURL( m_rootURL + http_endpoint_categories + ".json" );

    try
    {
        m_curl->Perform();

        res = m_curl->GetBuffer();

        if( !checkServerResponse( m_curl ) )
        {
            return false;
        }

        nlohmann::json response = nlohmann::json::parse( res );

        // collect the categories in vector
        for( const auto& item : response.items() )
        {
            HTTP_LIB_CATEGORY category;

            category.id = item.value()["id"].get<std::string>();
            category.name = item.value()["name"].get<std::string>();

            m_categories.push_back( category );
        }
    }
    catch( const std::exception& e )
    {
        m_lastError += wxString::Format( _( "Error: %s" ) + "\n" + _( "API Response:  %s" ) + "\n",
                                         e.what(), res );

        wxLogTrace( traceHTTPLib,
                    wxT( "syncCategories: Exception occurred while syncing categories: %s" ),
                    m_lastError );

        m_categories.clear();

        return false;
    }

    return true;
}


bool HTTP_LIB_CONNECTION::SelectOne( const std::string aPartID, HTTP_LIB_PART& aFetchedPart )
{
    if( !IsValidEnpoint() )
    {
        wxLogTrace( traceHTTPLib, wxT( "SelectOne: without valid connection!" ) );
        return false;
    }

    // if the same part is selected again, use cached part
    // instead to minimise http requests.
    if( m_cached_part.id == aPartID )
    {
        aFetchedPart = m_cached_part;
        return true;
    }

    std::string res = "";

    std::unique_ptr<KICAD_CURL_EASY> m_curl = createCurlEasyObject();
    m_curl->SetURL( m_rootURL + fmt::format( http_endpoint_parts + "/{}.json", aPartID ) );

    try
    {
        m_curl->Perform();

        res = m_curl->GetBuffer();

        if( !checkServerResponse( m_curl ) )
        {
            return false;
        }

        nlohmann::json response = nlohmann::json::parse( res );
        std::string    key = "";
        std::string    value = "";

        // the id used to identify the part, the name is needed to show a human-readable
        // part descirption to the user inside the symbol chooser dialog
        aFetchedPart.id = response.at( "id" );

        // API might not want to return an optional name.
        if( response.contains( "name" ) )
        {
            aFetchedPart.name = response.at( "name" );
        }
        else
        {
            aFetchedPart.name = aFetchedPart.id;
        }

        aFetchedPart.symbolIdStr = response.at( "symbolIdStr" );

        // Extract available fields
        for( const auto& field : response.at( "fields" ).items() )
        {
            bool visible = true;

            // name of the field
            key = field.key();

            // this is a dict
            auto& properties = field.value();

            value = properties.at( "value" );

            // check if user wants to display field in schematic
            if( properties.contains( "visible" ) )
            {
                std::string buf = properties.at( "visible" );
                visible = boolFromString( buf );
            }

            // Add field to fields list
            if( key.length() )
            {
                aFetchedPart.fields[key] = std::make_tuple( value, visible );
            }
        }
    }
    catch( const std::exception& e )
    {
        m_lastError += wxString::Format( _( "Error: %s" ) + "\n" + _( "API Response:  %s" ) + "\n",
                                         e.what(), res );

        wxLogTrace( traceHTTPLib,
                    wxT( "SelectOne: Exception occurred while retrieving part from REST API: %s" ),
                    m_lastError );

        return false;
    }

    m_cached_part = aFetchedPart;

    return true;
}


bool HTTP_LIB_CONNECTION::SelectAll( const HTTP_LIB_CATEGORY& aCategory,
                                     std::vector<HTTP_LIB_PART>& aParts )
{
    if( !IsValidEnpoint() )
    {
        wxLogTrace( traceHTTPLib, wxT( "SelectAll: without valid connection!" ) );
        return false;
    }

    std::string res = "";

    std::unique_ptr<KICAD_CURL_EASY> m_curl = createCurlEasyObject();
    m_curl->SetURL( m_rootURL
                    + fmt::format( http_endpoint_parts + "/category/{}.json", aCategory.id ) );

    try
    {
        m_curl->Perform();

        res = m_curl->GetBuffer();

        nlohmann::json response = nlohmann::json::parse( res );
        std::string    key = "";
        std::string    value = "";

        for( nlohmann::json item : response )
        {
            //PART result;
            HTTP_LIB_PART part;

            part.id = item.at( "id" );

            // API might not want to return an optional name.
            if( item.contains( "name" ) )
            {
                part.name = item.at( "name" );
            }
            else
            {
                part.name = part.id;
            }

            // add to cache
            m_cache[part.name] = std::make_tuple( part.id, aCategory.id );

            aParts.emplace_back( std::move( part ) );
        }
    }
    catch( const std::exception& e )
    {
        m_lastError += wxString::Format( _( "Error: %s" ) + "\n" + _( "API Response:  %s" ) + "\n",
                                         e.what(), res );

        wxLogTrace( traceHTTPLib, wxT( "Exception occurred while syncing parts from REST API: %s" ),
                    m_lastError );

        return false;
    }

    return true;
}


bool HTTP_LIB_CONNECTION::checkServerResponse( std::unique_ptr<KICAD_CURL_EASY>& m_curl )
{

    long http_code = 0;

    curl_easy_getinfo( m_curl->GetCurl(), CURLINFO_RESPONSE_CODE, &http_code );

    if( http_code != 200 )
    {
        m_lastError += wxString::Format( _( "API responded with error code: %s" ) + "\n",
                                         httpErrorCodeDescription( http_code ) );
        return false;
    }

    return true;
}


bool HTTP_LIB_CONNECTION::boolFromString( const std::any& aVal )
{
    try
    {
        wxString strval( std::any_cast<std::string>( aVal ).c_str(), wxConvUTF8 );

        if( strval.IsEmpty() )
            return true;

        strval.MakeLower();

        for( const auto& trueVal : { wxS( "true" ), wxS( "yes" ), wxS( "y" ), wxS( "1" ) } )
        {
            if( strval.Matches( trueVal ) )
                return true;
        }

        for( const auto& falseVal : { wxS( "false" ), wxS( "no" ), wxS( "n" ), wxS( "0" ) } )
        {
            if( strval.Matches( falseVal ) )
                return false;
        }
    }
    catch( const std::bad_any_cast& )
    {
    }

    return true;
}

/*
* HTTP response status codes indicate whether a specific HTTP request has been successfully completed.
* Responses are grouped in five classes:
*    Informational responses (100 ? 199)
*    Successful responses (200 ? 299)
*    Redirection messages (300 ? 399)
*    Client error responses (400 ? 499)
*    Server error responses (500 ? 599)
*
*    see: https://developer.mozilla.org/en-US/docs/Web/HTTP/Status
*/
wxString HTTP_LIB_CONNECTION::httpErrorCodeDescription( uint16_t http_code )
{
    switch( http_code )
    {
    case 100: return "100" + _( "Continue" );
    case 101: return "101" + _( "Switching Protocols" );
    case 102: return "102" + _( "Processing" );
    case 103: return "103" + _( "Early Hints" );

    case 200: return "200" + _( "OK" );
    case 201: return "201" + _( "Created" );
    case 203: return "203" + _( "Non-Authoritative Information" );
    case 204: return "204" + _( "No Content" );
    case 205: return "205" + _( "Reset Content" );
    case 206: return "206" + _( "Partial Content" );
    case 207: return "207" + _( "Multi-Status" );
    case 208: return "208" + _( "Already Reporte" );
    case 226: return "226" + _( "IM Used" );

    case 300: return "300" + _( "Multiple Choices" );
    case 301: return "301" + _( "Moved Permanently" );
    case 302: return "302" + _( "Found" );
    case 303: return "303" + _( "See Other" );
    case 304: return "304" + _( "Not Modified" );
    case 305: return "305" + _( "Use Proxy (Deprecated)" );
    case 306: return "306" + _( "Unused" );
    case 307: return "307" + _( "Temporary Redirect" );
    case 308: return "308" + _( "Permanent Redirect" );

    case 400: return "400" + _( "Bad Request" );
    case 401: return "401" + _( "Unauthorized" );
    case 402: return "402" + _( "Payment Required (Experimental)" );
    case 403: return "403" + _( "Forbidden" );
    case 404: return "404" + _( "Not Found" );
    case 405: return "405" + _( "Method Not Allowed" );
    case 406: return "406" + _( "Not Acceptable" );
    case 407: return "407" + _( "Proxy Authentication Required" );
    case 408: return "408" + _( "Request Timeout" );
    case 409: return "409" + _( "Conflict" );
    case 410: return "410" + _( "Gone" );
    case 411: return "411" + _( "Length Required" );
    case 412: return "413" + _( "Payload Too Large" );
    case 414: return "414" + _( "URI Too Long" );
    case 415: return "415" + _( "Unsupported Media Type" );
    case 416: return "416" + _( "Range Not Satisfiable" );
    case 417: return "417" + _( "Expectation Failed" );
    case 418: return "418" + _( "I'm a teapot" );
    case 421: return "421" + _( "Misdirected Request" );
    case 422: return "422" + _( "Unprocessable Conten" );
    case 423: return "423" + _( "Locked" );
    case 424: return "424" + _( "Failed Dependency" );
    case 425: return "425" + _( "Too Early (Experimental)" );
    case 426: return "426" + _( "Upgrade Required" );
    case 428: return "428" + _( "Precondition Required" );
    case 429: return "429" + _( "Too Many Requests" );
    case 431: return "431" + _( "Request Header Fields Too Large" );
    case 451: return "451" + _( "Unavailable For Legal Reasons" );

    case 500: return "500" + _( "Internal Server Error" );
    case 501: return "501" + _( "Not Implemented" );
    case 502: return "502" + _( "Bad Gateway" );
    case 503: return "503" + _( "Service Unavailable" );
    case 504: return "504" + _( "Gateway Timeout" );
    case 505: return "505" + _( "HTTP Version Not Supported" );
    case 506: return "506" + _( "Variant Also Negotiates" );
    case 507: return "507" + _( "Insufficient Storag" );
    case 508: return "508" + _( "Loop Detecte" );
    case 510: return "510" + _( "Not Extended" );
    case 511: return "511" + _( "Network Authentication Required" );
    }

    return wxString::Format( _( "Code Unkonwn: %d" ), http_code );
}