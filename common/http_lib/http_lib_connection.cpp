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
#include <ctime>

#include <boost/algorithm/string.hpp>
#include <nlohmann/json.hpp>
#include <wx/base64.h>

#include <kicad_curl/kicad_curl_easy.h>
#include <curl/curl.h>

#include <http_lib/http_lib_connection.h>

const char* const traceHTTPLib = "KICAD_HTTP_LIB";


HTTP_LIB_CONNECTION::HTTP_LIB_CONNECTION( const HTTP_LIB_SOURCE& aSource, bool aTestConnectionNow )
{
    m_source = aSource;

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
    m_endpointValid = false;
    std::string res = "";

    std::unique_ptr<KICAD_CURL_EASY> curl = createCurlEasyObject();
    curl->SetURL( m_source.root_url );

    try
    {
        curl->Perform();

        res = curl->GetBuffer();

        if( !checkServerResponse( curl ) )
            return false;

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
            {
                m_endpointValid = true;
            }
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

        m_endpointValid = false;
    }

    if( m_endpointValid )
    {
        syncCategories();
    }

    return m_endpointValid;
}


bool HTTP_LIB_CONNECTION::IsValidEndpoint() const
{
    return m_endpointValid;
}


bool HTTP_LIB_CONNECTION::syncCategories()
{
    if( !IsValidEndpoint() )
    {
        wxLogTrace( traceHTTPLib, wxT( "syncCategories: without valid connection!" ) );
        return false;
    }

    std::string res = "";

    std::unique_ptr<KICAD_CURL_EASY> curl = createCurlEasyObject();
    curl->SetURL( m_source.root_url + http_endpoint_categories + ".json" );

    try
    {
        curl->Perform();

        res = curl->GetBuffer();

        if( !checkServerResponse( curl ) )
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


bool HTTP_LIB_CONNECTION::SelectOne( const std::string& aPartID, HTTP_LIB_PART& aFetchedPart )
{
    if( !IsValidEndpoint() )
    {
        wxLogTrace( traceHTTPLib, wxT( "SelectOne: without valid connection!" ) );
        return false;
    }

    // Check if there is already a part in our cache, if not fetch it
    if( m_cachedParts.find( aPartID ) != m_cachedParts.end() )
    {
        // check if it's outdated, if so re-fetch
        if( std::difftime( std::time( nullptr ), m_cachedParts[aPartID].lastCached ) < m_source.timeout_parts )
        {
            aFetchedPart = m_cachedParts[aPartID];
            return true;
        }

    }

    std::string res = "";

    std::unique_ptr<KICAD_CURL_EASY> curl = createCurlEasyObject();
    std::string url = m_source.root_url + fmt::format( "{}/{}.json", http_endpoint_parts, aPartID );
    curl->SetURL( url );

    try
    {
        curl->Perform();

        res = curl->GetBuffer();

        if( !checkServerResponse( curl ) )
        {
            return false;
        }

        nlohmann::json response = nlohmann::json::parse( res );
        std::string    key = "";
        std::string    value = "";

        // the id used to identify the part, the name is needed to show a human-readable
        // part description to the user inside the symbol chooser dialog
        aFetchedPart.id = response.at( "id" );

        // get a timestamp for caching
        aFetchedPart.lastCached = std::time( nullptr );

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

        // initially assume no exclusion
        std::string exclude;

        if( response.contains( "exclude_from_bom" ) )
        {
            // if key value doesn't exists default to false
            exclude = response.at( "exclude_from_bom" );
            aFetchedPart.exclude_from_bom = boolFromString( exclude, false );
        }

        // initially assume no exclusion
        if( response.contains( "exclude_from_board" ) )
        {
            // if key value doesn't exists default to false
            exclude = response.at( "exclude_from_board" );
            aFetchedPart.exclude_from_board = boolFromString( exclude, false );
        }

        // initially assume no exclusion
        if( response.contains( "exclude_from_sim" ) )
        {
            // if key value doesn't exists default to false
            exclude = response.at( "exclude_from_sim" );
            aFetchedPart.exclude_from_sim = boolFromString( exclude, false );
        }

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
                std::string vis = properties.at( "visible" );
                visible = boolFromString( vis, true );
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

    m_cachedParts[aFetchedPart.id] = aFetchedPart;

    return true;
}


bool HTTP_LIB_CONNECTION::SelectAll( const HTTP_LIB_CATEGORY& aCategory,
                                     std::vector<HTTP_LIB_PART>& aParts )
{
    if( !IsValidEndpoint() )
    {
        wxLogTrace( traceHTTPLib, wxT( "SelectAll: without valid connection!" ) );
        return false;
    }

    std::string res = "";

    std::unique_ptr<KICAD_CURL_EASY> curl = createCurlEasyObject();

    curl->SetURL( m_source.root_url
                  + fmt::format( "{}/category/{}.json", http_endpoint_parts, aCategory.id ) );

    try
    {
        curl->Perform();

        res = curl->GetBuffer();

        nlohmann::json response = nlohmann::json::parse( res );
        std::string    key = "";
        std::string    value = "";

        for( nlohmann::json& item : response )
        {
            //PART result;
            HTTP_LIB_PART part;

            part.id = item.at( "id" );

            if( item.contains( "description" ) )
            {
                // At this point we don't display anything so just set it to false
                part.fields["description"] = std::make_tuple( item.at( "description" ), false );
            }

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


bool HTTP_LIB_CONNECTION::checkServerResponse( std::unique_ptr<KICAD_CURL_EASY>& aCurl )
{
    int statusCode = aCurl->GetResponseStatusCode();
    if( statusCode != 200 )
    {
        m_lastError += wxString::Format( _( "API responded with error code: %s" ) + "\n",
                                         httpErrorCodeDescription( statusCode ) );
        return false;
    }

    return true;
}


bool HTTP_LIB_CONNECTION::boolFromString( const std::any& aVal, bool aDefaultValue )
{
    try
    {
        wxString strval( std::any_cast<std::string>( aVal ).c_str(), wxConvUTF8 );

        if( strval.IsEmpty() )
            return aDefaultValue;

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

    return aDefaultValue;
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
wxString HTTP_LIB_CONNECTION::httpErrorCodeDescription( uint16_t aHttpCode )
{
    auto codeDescription =
            []( uint16_t aCode ) -> wxString
            {
                switch( aCode )
                {
                case 100: return wxS( "Continue" );
                case 101: return wxS( "Switching Protocols" );
                case 102: return wxS( "Processing" );
                case 103: return wxS( "Early Hints" );

                case 200: return wxS( "OK" );
                case 201: return wxS( "Created" );
                case 203: return wxS( "Non-Authoritative Information" );
                case 204: return wxS( "No Content" );
                case 205: return wxS( "Reset Content" );
                case 206: return wxS( "Partial Content" );
                case 207: return wxS( "Multi-Status" );
                case 208: return wxS( "Already Reporte" );
                case 226: return wxS( "IM Used" );

                case 300: return wxS( "Multiple Choices" );
                case 301: return wxS( "Moved Permanently" );
                case 302: return wxS( "Found" );
                case 303: return wxS( "See Other" );
                case 304: return wxS( "Not Modified" );
                case 305: return wxS( "Use Proxy (Deprecated)" );
                case 306: return wxS( "Unused" );
                case 307: return wxS( "Temporary Redirect" );
                case 308: return wxS( "Permanent Redirect" );

                case 400: return wxS( "Bad Request" );
                case 401: return wxS( "Unauthorized" );
                case 402: return wxS( "Payment Required (Experimental)" );
                case 403: return wxS( "Forbidden" );
                case 404: return wxS( "Not Found" );
                case 405: return wxS( "Method Not Allowed" );
                case 406: return wxS( "Not Acceptable" );
                case 407: return wxS( "Proxy Authentication Required" );
                case 408: return wxS( "Request Timeout" );
                case 409: return wxS( "Conflict" );
                case 410: return wxS( "Gone" );
                case 411: return wxS( "Length Required" );
                case 412: return wxS( "Payload Too Large" );
                case 414: return wxS( "URI Too Long" );
                case 415: return wxS( "Unsupported Media Type" );
                case 416: return wxS( "Range Not Satisfiable" );
                case 417: return wxS( "Expectation Failed" );
                case 418: return wxS( "I'm a teapot" );
                case 421: return wxS( "Misdirected Request" );
                case 422: return wxS( "Unprocessable Conten" );
                case 423: return wxS( "Locked" );
                case 424: return wxS( "Failed Dependency" );
                case 425: return wxS( "Too Early (Experimental)" );
                case 426: return wxS( "Upgrade Required" );
                case 428: return wxS( "Precondition Required" );
                case 429: return wxS( "Too Many Requests" );
                case 431: return wxS( "Request Header Fields Too Large" );
                case 451: return wxS( "Unavailable For Legal Reasons" );

                case 500: return wxS( "Internal Server Error" );
                case 501: return wxS( "Not Implemented" );
                case 502: return wxS( "Bad Gateway" );
                case 503: return wxS( "Service Unavailable" );
                case 504: return wxS( "Gateway Timeout" );
                case 505: return wxS( "HTTP Version Not Supported" );
                case 506: return wxS( "Variant Also Negotiates" );
                case 507: return wxS( "Insufficient Storag" );
                case 508: return wxS( "Loop Detecte" );
                case 510: return wxS( "Not Extended" );
                case 511: return wxS( "Network Authentication Required" );
                default:  return wxS( "Unknown" );
                }
            };

    return wxString::Format( wxS( "%d: %s" ), aHttpCode, codeDescription( aHttpCode ) );
}
