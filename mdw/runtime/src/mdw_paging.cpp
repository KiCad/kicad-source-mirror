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

#include <mdw_runtime/mdw_paging.h>
#include <mdw_runtime/mdw_backend_server.h>

#include <algorithm>
#include <cstdlib>

namespace ws = kiapi::workspace;


ws::WorkspaceError MdwPageRange( const kiapi::mdw::PageRequest& aPage, std::size_t aCount, const std::string& aSnapshot,
                                 std::size_t& aBegin, std::size_t& aEnd, std::string& aNextToken )
{
    const std::size_t  size = aPage.page_size() ? aPage.page_size() : 500;
    const std::string  query = ":" + std::to_string( size ) + ":";
    const std::string& token = aPage.page_token();

    aBegin = 0;

    if( !token.empty() )
    {
        const std::size_t first = token.find( ':' );
        const std::size_t second = first == std::string::npos ? first : token.find( ':', first + 1 );

        if( second == std::string::npos || token.compare( first, second + 1 - first, query ) != 0 )
            return MdwFail( ws::WE_INVALID_PARAMETER, "The page token belongs to another query" );

        if( token.compare( second + 1, std::string::npos, aSnapshot ) != 0 )
            return MdwFail( ws::WE_VERSION_CONFLICT, "The listing changed between pages" );

        aBegin = std::strtoull( token.c_str(), nullptr, 10 );

        if( aBegin == 0 || aBegin > aCount )
            return MdwFail( ws::WE_INVALID_PARAMETER, "The page token belongs to another query" );
    }

    aEnd = std::min( aCount, aBegin + size );
    aNextToken = aEnd < aCount ? std::to_string( aEnd ) + query + aSnapshot : std::string();
    return MdwOk();
}
