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

#include <kiface_ids.h>
#include <kiway.h>
#include <json_common.h>

#include <exception>


bool QueryMatchingFootprints( KIWAY& aKiway, const FOOTPRINT_MATCH_QUERY& aQuery, std::vector<wxString>& aMatches )
{
    using json = nlohmann::json;

    aMatches.clear();

    json request;
    request["pin_count"] = aQuery.pinCount;
    request["zero_filters"] = aQuery.zeroFilters;
    request["max_results"] = aQuery.maxResults;

    json filters = json::array();

    for( const wxString& pattern : aQuery.patterns )
        filters.push_back( pattern.ToStdString() );

    request["filters"] = filters;

    // Loading or reaching the pcbnew kiface, and the JSON round-trip itself, can all fail.
    // Callers cannot act on the difference, so every failure is reported as an empty result.
    try
    {
        KIFACE* kiface = aKiway.KiFACE( KIWAY::FACE_PCB );

        if( !kiface )
            return false;

        void* address = kiface->IfaceOrAddress( KIFACE_FILTER_FOOTPRINTS );

        if( !address )
            return false;

        using FILTER_FUNC = wxString ( * )( const wxString& );
        FILTER_FUNC filterFootprints = reinterpret_cast<FILTER_FUNC>( address );

        wxString response = filterFootprints( wxString::FromUTF8( request.dump() ) );
        json     matches = json::parse( response.ToStdString() );

        if( !matches.is_array() )
            return false;

        for( const json& match : matches )
        {
            if( match.is_string() )
                aMatches.emplace_back( wxString::FromUTF8( match.get<std::string>() ) );
        }
    }
    catch( const std::exception& )
    {
        aMatches.clear();
        return false;
    }

    return true;
}
