/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
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

#include "symbol_library_query.h"

#include <exception>

#include <kiface_ids.h>
#include <kiway.h>
#include <json_common.h>


FOOTPRINT_USERS_RESULT FOOTPRINT_USERS_RESULT::FromJsonStr( const wxString& aJson )
{
    using json = nlohmann::json;

    FOOTPRINT_USERS_RESULT result;
    result.m_Success = false;

    try
    {
        const json responseJson = json::parse( aJson.utf8_string() );

        if( !responseJson.is_object() )
            return result;

        result.m_Success = responseJson.value( "success", false );

        if( !result.m_Success || !responseJson.contains( "matches" ) || !responseJson["matches"].is_array() )
        {
            result.m_Success = false;
            return result;
        }

        const json& matches = responseJson["matches"];

        result.m_Matches.reserve( matches.size() );

        for( const json& match : matches )
        {
            if( !match.is_object() )
                continue;

            FOOTPRINT_USER_MATCH entry;

            if( match.contains( "symbol" ) && match["symbol"].is_string() )
                entry.m_Symbol.Parse( wxString::FromUTF8( match["symbol"].get<std::string>() ) );

            entry.m_PinCount = match.value( "pins", 0 );
            entry.m_IsDerived = match.value( "derived", false );

            if( match.contains( "filter" ) && match["filter"].is_string() )
                entry.m_MatchedFilter = wxString::FromUTF8( match["filter"].get<std::string>() );

            entry.m_MatchesFootprintField = match.value( "footprint_field", false );

            if( match.contains( "pin_maps" ) && match["pin_maps"].is_array() )
            {
                for( const json& mapName : match["pin_maps"] )
                {
                    if( mapName.is_string() )
                    {
                        entry.m_MatchedPinMaps.push_back( wxString::FromUTF8( mapName.get<std::string>() ) );
                    }
                }
            }

            result.m_Matches.push_back( std::move( entry ) );
        }

        result.m_IsLimited = responseJson.value( "limited", false );
    }
    catch( const std::exception& )
    {
        result.m_Matches.clear();
        result.m_Success = false;
    }

    return result;
}


wxString FOOTPRINT_USERS_RESULT::ToJsonStr() const
{
    using json = nlohmann::json;

    json output = json::object();
    output["matches"] = json::array();
    output["limited"] = m_IsLimited;
    output["success"] = m_Success;

    for( const FOOTPRINT_USER_MATCH& match : m_Matches )
    {
        json entry = json::object();
        entry["symbol"] = match.m_Symbol.GetUniStringLibId().utf8_string();
        entry["pins"] = match.m_PinCount;
        entry["derived"] = match.m_IsDerived;

        if( !match.m_MatchedFilter.IsEmpty() )
            entry["filter"] = match.m_MatchedFilter.utf8_string();

        entry["footprint_field"] = match.m_MatchesFootprintField;

        if( !match.m_MatchedPinMaps.empty() )
        {
            json pinMaps = json::array();

            for( const wxString& mapName : match.m_MatchedPinMaps )
                pinMaps.push_back( mapName.utf8_string() );

            entry["pin_maps"] = std::move( pinMaps );
        }

        output["matches"].push_back( std::move( entry ) );
    }

    return wxString::FromUTF8( output.dump() );
}


FOOTPRINT_USERS_QUERY FOOTPRINT_USERS_QUERY::FromJsonStr( const wxString& aJson )
{
    using json = nlohmann::json;

    FOOTPRINT_USERS_QUERY query;

    try
    {
        const json requestJson = json::parse( aJson.utf8_string() );

        if( !requestJson.is_object() )
            return query;

        if( requestJson.contains( "footprint" ) && requestJson["footprint"].is_string() )
            query.m_Footprint.Parse( wxString::FromUTF8( requestJson["footprint"].get<std::string>() ) );

        query.m_MatchFilters = requestJson.value( "match_filters", query.m_MatchFilters );
        query.m_MatchFootprintField = requestJson.value( "match_footprint_field", query.m_MatchFootprintField );
        query.m_MatchPinMaps = requestJson.value( "match_pin_maps", query.m_MatchPinMaps );
        query.m_MaxResults = requestJson.value( "max_results", query.m_MaxResults );
    }
    catch( const std::exception& )
    {
        query = FOOTPRINT_USERS_QUERY();
    }

    return query;
}


wxString FOOTPRINT_USERS_QUERY::ToJsonStr() const
{
    using json = nlohmann::json;

    json request = json::object();
    request["footprint"] = m_Footprint.GetUniStringLibId().utf8_string();
    request["match_filters"] = m_MatchFilters;
    request["match_footprint_field"] = m_MatchFootprintField;
    request["match_pin_maps"] = m_MatchPinMaps;
    request["max_results"] = m_MaxResults;

    return wxString::FromUTF8( request.dump() );
}


FOOTPRINT_USERS_RESULT QueryFootprintUsers( KIWAY& aKiway, const FOOTPRINT_USERS_QUERY& aQuery )
{
    const auto returnFailure = []() -> FOOTPRINT_USERS_RESULT
    {
        FOOTPRINT_USERS_RESULT result;
        result.m_Success = false;
        return result;
    };

    // Loading or reaching the eeschema kiface, and the JSON round-trip itself, can all fail.
    try
    {
        KIFACE* kiface = aKiway.KiFACE( KIWAY::FACE_SCH );

        if( !kiface )
            return returnFailure();

        void* address = kiface->IfaceOrAddress( KIFACE_FILTER_FOOTPRINT_USERS );

        if( !address )
            return returnFailure();

        using FILTER_FUNC = wxString ( * )( const wxString& );
        FILTER_FUNC filterUsers = reinterpret_cast<FILTER_FUNC>( address );

        wxString response = filterUsers( aQuery.ToJsonStr() );

        return FOOTPRINT_USERS_RESULT::FromJsonStr( response );
    }
    catch( const std::exception& )
    {
        return returnFailure();
    }

    return returnFailure();
}


bool StartSymbolLibrariesLoad( KIWAY& aKiway )
{
    // The implementation here is very similar to the footprint users query mechanism.
    // Cf. StartFootprintLibrariesLoad()

    try
    {
        KIFACE* kiface = aKiway.KiFACE( KIWAY::FACE_SCH );

        if( !kiface )
            return false;

        void* address = kiface->IfaceOrAddress( KIFACE_TRIGGER_SYMBOLS_LOAD );

        if( !address )
            return false;

        using LOAD_FUNC = bool ( * )();
        LOAD_FUNC startLoad = reinterpret_cast<LOAD_FUNC>( address );

        return startLoad();
    }
    catch( const std::exception& )
    {
        return false;
    }

    return false;
}


float GetSymbolLibrariesLoadProgress( KIWAY& aKiway )
{
    try
    {
        KIFACE* kiface = aKiway.KiFACE( KIWAY::FACE_SCH );

        if( !kiface )
            return 1.0f;

        void* address = kiface->IfaceOrAddress( KIFACE_SYMBOLS_LOAD_PROGRESS );

        if( !address )
            return 1.0f;

        using PROGRESS_FUNC = float ( * )();
        PROGRESS_FUNC getProgress = reinterpret_cast<PROGRESS_FUNC>( address );

        return getProgress();
    }
    catch( const std::exception& )
    {
        return 1.0f;
    }
}
