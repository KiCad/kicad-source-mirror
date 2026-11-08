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

#include "operating_point.h"

#include <advanced_config.h>
#include <connection_graph.h>
#include <connectivity/conn_facade.h>
#include <sch_line.h>
#include <schematic.h>

#include <map>


void UpdateOperatingPointVoltages( SCHEMATIC& aSchematic, const std::function<void( SCH_LINE* )>& aOnChanged )
{
    const SCHEMATIC_SETTINGS& settings = aSchematic.Settings();
    const wxString            variant = aSchematic.GetCurrentVariant();

    auto assignVoltage = [&]( const wxString& aNetName, const SCH_SHEET_PATH& aSheet, const auto& aItems )
    {
        if( aSheet.GetExcludedFromSim( variant ) )
            return;

        wxString op = aSchematic.GetOperatingPoint( aNetName, settings.m_OPO_VPrecision, settings.m_OPO_VRange );

        if( op.IsEmpty() || op == wxS( "--" ) || op == wxS( "?" ) )
            return;

        SCH_LINE* longestWire = nullptr;
        double    length = 0.0;

        for( SCH_ITEM* item : aItems )
        {
            if( item->Type() == SCH_LINE_T )
            {
                SCH_LINE* wire = static_cast<SCH_LINE*>( item );

                if( wire->IsWire() && wire->GetLength() > length )
                {
                    longestWire = wire;
                    length = wire->GetLength();
                }
            }
        }

        if( longestWire )
        {
            longestWire->SetOperatingPoint( op );

            if( aOnChanged )
                aOnChanged( longestWire );
        }
    };

    if( ADVANCED_CFG::GetCfg().m_ConnectivityEngine )
    {
        const SCH_SHEET_LIST                       hierarchy = aSchematic.Hierarchy();
        std::map<KIID_PATH, const SCH_SHEET_PATH*> paths;

        for( const SCH_SHEET_PATH& path : hierarchy )
            paths.emplace( path.Path(), &path );

        // The active engine does not populate the legacy CONNECTION_GRAPH net map.
        for( const SCH_CONNECTIVITY::NET_GROUP& group : aSchematic.Connectivity().GetNetMap() )
        {
            for( const SCH_CONNECTIVITY::NET_VIEW& net : group.instances )
            {
                const auto path = paths.find( net.Instance() );

                if( net.IsNet() && path != paths.end() )
                    assignVoltage( group.name, *path->second, net.Items() );
            }
        }
    }
    else
    {
        for( const auto& [key, subgraphs] : aSchematic.ConnectionGraph()->GetNetMap() )
        {
            for( CONNECTION_SUBGRAPH* subgraph : subgraphs )
                assignVoltage( key.Name, subgraph->GetSheet(), subgraph->GetItems() );
        }
    }
}
