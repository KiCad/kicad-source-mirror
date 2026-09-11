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

#include "conn_frame.h"

#include <sch_screen.h>
#include <sch_sheet_path.h>
#include <limits>
#include <stdexcept>
#include <wx/thread.h>

namespace SCH_CONNECTIVITY
{
std::vector<FRAME_INSTANCE> CaptureHierarchy( const SCH_SHEET_LIST& aPaths, SESSION_KEYS& aKeys )
{
    wxASSERT( wxThread::IsMain() );
    std::map<KIID_PATH, std::pair<const SCH_SHEET_PATH*, size_t>> paths;

    for( const SCH_SHEET_PATH& path : aPaths )
    {
        if( path.empty() || !path.LastScreen() )
            continue;

        if( !paths.try_emplace( path.Path(), &path, 0 ).second )
            throw std::invalid_argument( "Connectivity hierarchy has duplicate instance paths" );
    }

    std::vector<FRAME_INSTANCE> result;
    result.reserve( paths.size() );

    for( auto& [key, entry] : paths )
    {
        auto& [path, index] = entry;

        if( path->size() > std::numeric_limits<uint16_t>::max() )
            throw std::overflow_error( "Connectivity hierarchy depth exhausted" );

        INSTANCE_SCOPE scope;
        scope.instance = aKeys.InternInstance( key );
        scope.path = path->PathHumanReadable( true, false, true );
        scope.depth = static_cast<uint16_t>( path->size() );
        index = result.size();
        KIID_PATH parent = key;
        parent.pop_back();

        // KIID_PATH orders shorter paths first, so a parent is always already captured
        if( const auto found = paths.find( parent ); found != paths.end() )
            result[found->second.second].scope.children.emplace( key.back(), scope.instance );

        result.push_back( { std::move( scope ), path->LastScreen()->ConnectivityId() } );
    }

    return result;
}

} // namespace SCH_CONNECTIVITY
