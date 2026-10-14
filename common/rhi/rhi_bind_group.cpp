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
 * You should have received a copy of the GNU General Public License along
 * with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <rhi/rhi_bind_group.h>

#include <algorithm>
#include <unordered_set>


bool RHI_BIND_GROUP_LAYOUT_DESC::Validate() const
{
    int groupIndex = static_cast<int>( frequency );

    if( groupIndex < 0 || groupIndex >= RHI_MAX_BIND_GROUPS )
        return false;

    if( entries.empty() )
        return true;

    std::unordered_set<uint32_t> seen;
    seen.reserve( entries.size() );

    for( const auto& entry : entries )
    {
        if( !seen.insert( entry.binding ).second )
            return false;

        if( !entry.stages )
            return false;
    }

    return true;
}
