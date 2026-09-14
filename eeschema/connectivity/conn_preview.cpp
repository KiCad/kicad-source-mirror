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

#include "conn_preview.h"
#include "conn_facade.h"
#include <sch_item.h>
#include <sch_screen.h>
#include <sch_sheet_path.h>

namespace SCH_CONNECTIVITY
{
PREVIEW_NET_CODES CapturePreviewNetCodes( const FACADE& aFacade, const SCH_SHEET_PATH& aPath )
{
    PREVIEW_NET_CODES result;
    SCH_SCREEN* screen = aPath.LastScreen();

    if( !screen )
        return result;

    result.reserve( screen->Items().size() );
    const KIID_PATH& path = aPath.PathRef();
    const auto capture = [&]( SCH_ITEM* item )
    {
        if( !item->IsConnectable() )
            return;

        const auto connection = aFacade.Connection( item->m_Uuid, path );
        const int code = connection && connection->IsNet() && !connection->IsUnconnected() ? connection->NetCode() : 0;
        result.emplace( item, code > 0 ? std::optional<int>( code ) : std::nullopt );
    };

    for( SCH_ITEM* item : screen->Items() )
    {
        capture( item );
        item->RunOnChildren( capture, RECURSE_MODE::NO_RECURSE );
    }

    return result;
}
} // namespace SCH_CONNECTIVITY
