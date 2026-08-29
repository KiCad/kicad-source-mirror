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
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#pragma once

#include <any>
#include <boost/algorithm/string.hpp>
#include <functional>
#include <json_common.h>
#include <mutex>

#include "http_lib/http_lib_settings.h"
#include <kicad_curl/kicad_curl_easy.h>

extern const char* const traceHTTPLib;


/**
 * Parse the exclusion flags and field content from a part's JSON record into @p aPart.
 *
 * The category listing and per-part detail endpoints share this shape but the listing may
 * omit the fields; parsing is tolerant of missing keys and of flags encoded as either JSON
 * strings or native booleans.
 *
 * @return true if the record carried a fields object (the part's full detail is present).
 */
bool setPartExtendedData( const nlohmann::json& aPartJson, HTTP_LIB_PART& aPart );


class HTTP_LIB_CONNECTION
{
public:
    /**
     * Allows replacing CURL fetches with a mock for testing
     *
     * @param aUrl the URL to retrieve
     * @param aStatusCode receives the HTTP status (0 when no response was received)
     * @param aBody receives the response body
     * @param aError receives a failure reason when the request could not be completed
     * @return true when a response was obtained (regardless of status code); false when the
     *         request could not be completed at the transport level
     */
    using RETRIEVER = std::function<bool( const std::string& aUrl, int& aStatusCode,
                                          std::string& aBody, std::string& aError )>;

    HTTP_LIB_CONNECTION( const HTTP_LIB_SOURCE& aSource, bool aTestConnectionNow );

    HTTP_LIB_CONNECTION( const HTTP_LIB_SOURCE& aSource, bool aTestConnectionNow, RETRIEVER aRetriever );

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

    std::string GetLastError() const
    {
        std::lock_guard lock( m_queryMutex );
        return m_lastError;
    }

    std::vector<HTTP_LIB_CATEGORY> getCategories() const
    {
        std::lock_guard lock( m_queryMutex );
        return m_categories;
    }

    std::string getCategoryDescription( const std::string& aCategoryName ) const
    {
        std::lock_guard lock( m_queryMutex );

        if( m_categoryDescriptions.contains( aCategoryName ) )
            return m_categoryDescriptions.at( aCategoryName );

        return "";
    }

    bool HasCachedParts() const
    {
        std::lock_guard lock( m_queryMutex );
        return !m_cache.empty();
    }

    /**
     * Resolve a cached part name to its part ID and category ID.
     *
     * @param aPartName the name to look up
     * @param aPartId receives the part ID on a hit
     * @param aCategoryId receives the category ID on a hit
     * @return true if the name was found; false otherwise
     */
    bool GetCachedPartRelation( const std::string& aPartName, std::string& aPartId, std::string& aCategoryId ) const
    {
        std::lock_guard lock( m_queryMutex );

        auto it = m_cache.find( aPartName );

        if( it == m_cache.end() )
            return false;

        aPartId = std::get<0>( it->second );
        aCategoryId = std::get<1>( it->second );
        return true;
    }

    void ClearPartCache()
    {
        std::lock_guard lock( m_queryMutex );
        m_cachedParts.clear();
        m_cache.clear();
    }

private:
    bool validateHttpLibraryEndpoints();

    bool syncCategories();

    /**
     * Fetch @p aUrl through the transport.
     *
     * @return false (with m_lastError populated) when the transport could not complete the
     *         request; true otherwise, even when @p aStatusCode is not 200.
     */
    bool retrieve( const std::string& aUrl, int& aStatusCode, std::string& aBody );

    bool checkServerResponse( int aStatusCode );

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
    mutable std::mutex m_queryMutex;

    HTTP_LIB_SOURCE m_source;
    RETRIEVER       m_retriever;
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
