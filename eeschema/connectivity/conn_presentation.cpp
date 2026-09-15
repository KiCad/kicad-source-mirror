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
#include <netclass.h>
#include <sch_item.h>
#include <connectivity/conn_facade.h>
#include <advanced_config.h>
#include <bus_alias.h>
#include <schematic.h>
#include <project/net_settings.h>
#include <units_provider.h>
#include <boost/algorithm/string/join.hpp>
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

    if( ADVANCED_CFG::GetCfg().m_ConnectivityEngine )
    {
        SCHEMATIC* schematic = aItem.Schematic();

        if( !schematic || !schematic->IsValid() )
            return std::nullopt;

        const SCH_SHEET_PATH& path = aPath ? *aPath : schematic->CurrentSheet();
        const auto connection = schematic->Connectivity().Connection( aItem.m_Uuid, path.PathRef() );

        if( !connection )
            return std::nullopt;

        name = connection->Name();
        isBus = connection->IsBus();
        aList.emplace_back( _( "Connection Name" ), UnescapeString( name ) );

        if( isBus )
        {
            auto appendAlias = [&]( const std::shared_ptr<BUS_ALIAS>& alias )
            {
                aList.emplace_back( wxString::Format( _( "Bus Alias %s Members" ), alias->GetName() ),
                                    boost::algorithm::join( alias->Members(), " " ) );
            };
            const wxString localName = connection->Name( true );

            if( const auto alias = schematic->GetBusAlias( localName ) )
            {
                appendAlias( alias );
            }
            else
            {
                wxString group;
                std::vector<wxString> members;

                if( NET_SETTINGS::ParseBusGroup( localName, &group, &members ) )
                {
                    for( const wxString& member : members )
                    {
                        if( const auto memberAlias = schematic->GetBusAlias( member ) )
                            appendAlias( memberAlias );
                    }
                }
            }
        }

#if defined(DEBUG)
        aList.emplace_back( "Subgraph Code", wxString::Format( "%u", connection->SubgraphCode() ) );

        if( SCH_ITEM* driver = connection->Driver() )
        {
            UNITS_PROVIDER units( schIUScale, EDA_UNITS::MM );
            aList.emplace_back( "Connection Source", wxString::Format( "%s at %p",
                                driver->GetItemDescription( &units, false ), driver ) );
        }
#endif
    }
    else
    {
        const SCH_CONNECTION* connection = aItem.Connection( aPath );

        if( !connection )
            return std::nullopt;

        connection->AppendInfoToMsgPanel( aList );
        name = connection->Name();
        isBus = connection->IsBus();
    }

    if( isBus )
        return std::nullopt;

    aList.emplace_back( _( "Resolved Netclass" ),
                        UnescapeString( aItem.GetEffectiveNetClass( aPath )->GetHumanReadableName() ) );
    return name;
}
