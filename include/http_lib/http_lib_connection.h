/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2023 Andre F. K. Iwers <iwers11@gmail.com>
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
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

#pragma once

#include <any>
#include <boost/algorithm/string.hpp>

#include "http_lib/http_lib_settings.h"
#include <kicad_curl/kicad_curl_easy.h>

extern const char* const traceHTTPLib;


class HTTP_LIB_CONNECTION
{
public:
    static const long DEFAULT_TIMEOUT = 10;

    HTTP_LIB_CONNECTION( const HTTP_LIB_SOURCE& aSource, bool aTestConnectionNow );
    virtual ~HTTP_LIB_CONNECTION() = default;

    bool IsValidEndpoint() const { return m_endpointValid; }

    /**
     * Retrieve a single part with full details from the HTTP library.
     *
     * @param aPartID is the unique ID of the part
     * @param aFetchedPart will contain the part if one was found
     * @return true if aResult was filled; false otherwise
     */
    bool SelectOne( const std::string& aPartID, HTTP_LIB_PART& aFetchedPart );

    /**
     * Retrieve all parts from a specific category from the HTTP library.
     *
     * @param aCategory is the category to fetch parts from
     * @param aParts will be filled with all parts in that category
     * @return true if the query succeeded and at least one part was found, false otherwise
     */
    bool SelectAll( const HTTP_LIB_CATEGORY& aCategory, std::vector<HTTP_LIB_PART>& aParts );

    std::string GetLastError() const { return m_lastError; }

    std::vector<HTTP_LIB_CATEGORY> getCategories() const { return m_categories; }

    std::string getCategoryDescription( const std::string& aCategoryName ) const
    {
        if( m_categoryDescriptions.contains( aCategoryName ) )
           return m_categoryDescriptions.at( aCategoryName );
        else
           return "";
    }

    auto& GetCachedParts() { return m_cache; }

private:
    // This is clunky but at the moment the only way to free the pointer after use without
    // KiCad crashing.  At this point we can't use smart pointers as there is a problem with
    // the order of how things are deleted/freed
    std::unique_ptr<KICAD_CURL_EASY> createCurlEasyObject()
    {
        std::unique_ptr<KICAD_CURL_EASY> aCurl( new KICAD_CURL_EASY() );

        // prepare curl
        aCurl->SetHeader( "Accept", "application/json" );
        aCurl->SetHeader( "Authorization", "Token " + m_source.token );
        aCurl->SetFollowRedirects( true );

        return aCurl;
    }

    bool validateHttpLibraryEndpoints();

    bool syncCategories();

    bool checkServerResponse( std::unique_ptr<KICAD_CURL_EASY>& aCurl );

    /**
     * HTTP response status codes indicate whether a specific HTTP request has been
     * successfully completed.
     *
     * Responses are grouped in five classes:
     *  -  Informational responses (100 ? 199)
     *  -  Successful responses (200 ? 299)
     *  -  Redirection messages (300 ? 399)
     *  -  Client error responses (400 ? 499)
     *  -  Server error responses (500 ? 599)
     *
     *    see: https://developer.mozilla.org/en-US/docs/Web/HTTP/Status
     */
    wxString httpErrorCodeDescription( uint16_t aHttpCode );

private:
    HTTP_LIB_SOURCE m_source;
    bool            m_endpointValid = false;
    std::string     m_lastError;

    std::vector<HTTP_LIB_CATEGORY>     m_categories;
    std::map<std::string, std::string> m_categoryDescriptions;
    std::map<std::string, std::string> m_parts;

    //          part.id     part
    std::map<std::string, HTTP_LIB_PART> m_cachedParts;

    //        part.name               part.id     category.id
    std::map<std::string, std::tuple<std::string, std::string>> m_cache;
};
