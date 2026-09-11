/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include "footprint_library_query.h"

#include <exception>

#include <kiface_ids.h>
#include <kiway.h>
#include <json_common.h>


FOOTPRINT_MATCH_RESULT FOOTPRINT_MATCH_RESULT::FromJsonStr( const wxString& aJson )
{
    using json = nlohmann::json;

    FOOTPRINT_MATCH_RESULT result;

    try
    {
        const json responseJson = json::parse( aJson.utf8_string() );

        // Only a result object reporting a performed query carries matches.
        if( !responseJson.is_object() )
            return result;

        result.m_Success = responseJson.value( "success", false );

        if( !result.m_Success || !responseJson.contains( "matches" ) || !responseJson["matches"].is_array() )
        {
            result.m_Success = false;
            return result;
        }

        const json& matches = responseJson["matches"];

        result.m_MatchingNames.reserve( matches.size() );

        for( const json& match : matches )
        {
            if( match.is_string() )
                result.m_MatchingNames.emplace_back( wxString::FromUTF8( match.get<std::string>() ) );
        }

        result.m_IsLimited = responseJson.value( "limited", false );
    }
    catch( const std::exception& )
    {
        result.m_MatchingNames.clear();
        result.m_Success = false;
    }

    return result;
}


wxString FOOTPRINT_MATCH_RESULT::ToJsonStr() const
{
    using json = nlohmann::json;

    json output = json::object();
    output["matches"] = json::array();
    output["limited"] = m_IsLimited;
    output["success"] = m_Success;

    for( const wxString& match : m_MatchingNames )
        output["matches"].push_back( match.utf8_string() );

    return wxString::FromUTF8( output.dump() );
}


FOOTPRINT_MATCH_RESULT QueryMatchingFootprints( KIWAY& aKiway, const FOOTPRINT_MATCH_QUERY& aQuery )
{
    using json = nlohmann::json;

    json request;

    int rawPinCount = aQuery.m_PinCount.value_or( 0 );
    request["pin_count"] = rawPinCount > 0 ? rawPinCount : 0;
    request["zero_filters"] = aQuery.m_ZeroFilters;
    request["max_results"] = aQuery.m_MaxResults;

    json filters = json::array();

    for( const wxString& pattern : aQuery.m_Patterns )
        filters.push_back( pattern.utf8_string() );

    request["filters"] = filters;

    const auto returnFailure = []() -> FOOTPRINT_MATCH_RESULT
    {
        FOOTPRINT_MATCH_RESULT result;
        result.m_Success = false;
        return result;
    };

    // Loading or reaching the pcbnew kiface, and the JSON round-trip itself, can all fail.
    // Callers cannot act on the difference, so every failure is reported as an empty result.
    try
    {
        KIFACE* kiface = aKiway.KiFACE( KIWAY::FACE_PCB );

        if( !kiface )
            return returnFailure();

        void* address = kiface->IfaceOrAddress( KIFACE_FILTER_FOOTPRINTS );

        if( !address )
            return returnFailure();

        using FILTER_FUNC = wxString ( * )( const wxString& );
        FILTER_FUNC filterFootprints = reinterpret_cast<FILTER_FUNC>( address );

        wxString response = filterFootprints( wxString::FromUTF8( request.dump() ) );

        return FOOTPRINT_MATCH_RESULT::FromJsonStr( response );
    }
    catch( const std::exception& )
    {
        return returnFailure();
    }

    return returnFailure();
}


bool StartFootprintLibrariesLoad( KIWAY& aKiway )
{
    try
    {
        KIFACE* kiface = aKiway.KiFACE( KIWAY::FACE_PCB );

        if( !kiface )
            return false;

        void* address = kiface->IfaceOrAddress( KIFACE_TRIGGER_FOOTPRINTS_LOAD );

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


float GetFootprintLibrariesLoadProgress( KIWAY& aKiway )
{
    try
    {
        KIFACE* kiface = aKiway.KiFACE( KIWAY::FACE_PCB );

        if( !kiface )
            return 1.0f;

        void* address = kiface->IfaceOrAddress( KIFACE_FOOTPRINTS_LOAD_PROGRESS );

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
