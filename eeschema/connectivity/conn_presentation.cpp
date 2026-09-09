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

#include <connectivity/conn_presentation.h>
#include <sch_item.h>
#include <wx/thread.h>
#include <sch_connection.h>
#include <string_utils.h>
#include <widgets/msgpanel.h>

std::optional<wxString> SCH_CONNECTIVITY::AppendConnectionInfo(
        const SCH_ITEM& aItem, std::vector<MSG_PANEL_ITEM>& aList, const SCH_SHEET_PATH* aPath )
{
    wxASSERT( wxThread::IsMain() );
    wxString name;
    bool isBus;

    const SCH_CONNECTION* connection = aItem.Connection( aPath );

    if( !connection )
        return std::nullopt;

    connection->AppendInfoToMsgPanel( aList );
    name = connection->Name();
    isBus = connection->IsBus();

    if( isBus )
        return std::nullopt;

    aList.emplace_back( _( "Resolved Netclass" ),
                        UnescapeString( aItem.GetEffectiveNetClass( aPath )->GetHumanReadableName() ) );
    return name;
}
