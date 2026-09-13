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

#include "conn_components.h"

#include <wx/thread.h>

namespace SCH_CONNECTIVITY
{
std::vector<NODE_INPUT> RecordNodes( const RECORD_STORE::RECORD_CACHE& aRecords, KIND aKind, SESSION_KEYS& aKeys )
{
    wxASSERT( wxThread::IsMain() );
    std::vector<NODE_INPUT> result;
    result.reserve( aRecords.Entries().size() );

    for( const auto& [key, entry] : aRecords.Entries() )
    {
        if( entry->value.kind != aKind )
            continue;

        NODE_INPUT input{ aKeys.InternNode( RECORD_NODE{ key, aKind } ), entry->version, {} };
        input.edges.reserve( entry->value.edges.size() );

        for( const NAME_KEY& edge : entry->value.edges )
            input.edges.push_back( aKeys.InternNode( edge ) );

        result.push_back( std::move( input ) );
    }

    return result;
}
} // namespace SCH_CONNECTIVITY
