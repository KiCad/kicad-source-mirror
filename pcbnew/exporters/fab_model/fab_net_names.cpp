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

#include "fab_net_names.h"

#include <board.h>
#include <netinfo.h>


std::map<int, wxString> FabAnonymousNetNames( const BOARD& aBoard )
{
    std::map<int, wxString> names;

    for( const NETINFO_ITEM* net : aBoard.GetNetInfo() )
    {
        if( net->GetNetCode() > 0 )
            names.emplace( net->GetNetCode(), wxString() );
    }

    size_t ordinal = 0;

    for( auto& [code, name] : names )
        name = wxString::Format( wxS( "NET_%zu" ), ++ordinal );

    return names;
}
