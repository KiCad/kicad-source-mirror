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

#ifndef KICAD_HTTP_LIB_CONNECTION_H
#define KICAD_HTTP_LIB_CONNECTION_H

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

    ~HTTP_LIB_CONNECTION();

    bool IsValidEndpoint() const;

    /**
     * Retrieves a single part with full details from the HTTP library.
     * @param aPk is the primary key of the part
     * @param aResult will conatain the part if one was found
     * @return true if aResult was filled; false otherwise
     */
    bool SelectOne( const std::string& aPartID, HTTP_LIB_PART& aFetchedPart );

    /**
     * Retrieves all parts from a specific category from the HTTP library.
     * @param aPk is the primary key of the category
     * @param aResults will be filled with all parts in that category
     * @return true if the query succeeded and at least one part was found, false otherwise
     */
    bool SelectAll( const HTTP_LIB_CATEGORY& aCategory, std::vector<HTTP_LIB_PART>& aParts );

    std::string GetLastError() const { return m_lastError; }

    std::vector<HTTP_LIB_CATEGORY> getCategories() const { return m_categories; }

    std::string getCategoryDescription( const std::string& aCategoryName ) const
    {
        return m_categoryDescriptions.at( aCategoryName );
    }

    auto& getCachedParts() { return m_cache; }

private:
    // This is clunky but at the moment the only way to free the pointer after use without KiCad crashing.
    // at this point we can't use smart pointers as there is a problem with the order of how things are deleted/freed
    std::unique_ptr<KICAD_CURL_EASY> createCurlEasyObject()
    {
        std::unique_ptr<KICAD_CURL_EASY> aCurl( new KICAD_CURL_EASY() );

        // prepare curl
        aCurl->SetHeader( "Accept", "application/json" );
        aCurl->SetHeader( "Authorization", "Token " + m_source.token );

        return aCurl;
    }

    bool ValidateHTTPLibraryEndpoints();

    bool syncCategories();

    bool checkServerResponse( std::unique_ptr<KICAD_CURL_EASY>& aCurl );

    bool boolFromString( const std::any& aVal, bool aDefaultValue = false );

    wxString httpErrorCodeDescription( uint16_t aHttpCode );

    HTTP_LIB_SOURCE m_source;

    //          part.id     part
    std::map<std::string, HTTP_LIB_PART> m_cachedParts;

    //        part.name               part.id     category.id
    std::map<std::string, std::tuple<std::string, std::string>> m_cache;

    bool m_endpointValid = false;

    std::string m_lastError;

    std::vector<HTTP_LIB_CATEGORY>     m_categories;
    std::map<std::string, std::string> m_categoryDescriptions;

    std::map<std::string, std::string> m_parts;

    const std::string http_endpoint_categories = "categories";
    const std::string http_endpoint_parts = "parts";
    const std::string http_endpoint_settings = "settings";
    const std::string http_endpoint_auth = "authentication";
};

#endif //KICAD_HTTP_LIB_CONNECTION_H
