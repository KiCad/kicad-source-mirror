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

#include "conn_navigation.h"
#include "conn_facade.h"

#include <advanced_config.h>
#include <connection_graph.h>
#include <schematic.h>
#include <sch_item.h>
#include <algorithm>

namespace SCH_CONNECTIVITY
{
NAVIGATION_QUERY::NAVIGATION_QUERY( const SCHEMATIC& aSchematic ) : m_schematic( aSchematic )
{}


const NAVIGATION_QUERY::PATH_INDEX& NAVIGATION_QUERY::paths() const
{
    if( m_paths )
        return *m_paths;

    PATH_INDEX index;

    if( m_schematic.IsValid() && m_schematic.HasHierarchy() )
    {
        for( SCH_SHEET_PATH& path : m_schematic.Hierarchy() )
        {
            if( path.LastScreen() )
            {
                KIID_PATH key = path.PathRef();
                index.emplace( std::move( key ), std::move( path ) );
            }
        }
    }

    return m_paths.emplace( std::move( index ) );
}


std::vector<wxString> NAVIGATION_QUERY::NetNames() const
{
    if( !m_schematic.IsValid() )
        return {};

    if( ADVANCED_CFG::GetCfg().m_ConnectivityEngine )
    {
        auto names = m_schematic.Connectivity().NetNames();
        std::erase( names, wxString() );
        return names;
    }

    std::set<wxString> names;

    for( const auto& [key, subgraphs] : m_schematic.ConnectionGraph()->GetNetMap() )
    {
        if( !key.Name.IsEmpty() )
            names.insert( key.Name );
    }

    return { names.begin(), names.end() };
}


std::vector<SCH_ITEM*> NAVIGATION_QUERY::WholeNetItems( const std::vector<SCH_ITEM*>& aSeeds,
                                                      const SCH_SHEET_PATH& aSheet ) const
{
    std::set<wxString> names;

    for( SCH_ITEM* seed : aSeeds )
    {
        if( !seed )
            continue;

        if( const auto name = seed->GetConnectionName( &aSheet ); name && !name->IsEmpty() )
            names.insert( *name );
    }

    std::unordered_set<SCH_ITEM*> items;

    for( const wxString& name : names )
        CollectNetItems( name, aSheet, items );

    return { items.begin(), items.end() };
}


std::vector<wxString> NAVIGATION_QUERY::SignalNames( const wxString& aName ) const
{
    if( !m_schematic.IsValid() )
        return {};

    std::set<wxString> names;

    if( ADVANCED_CFG::GetCfg().m_ConnectivityEngine )
    {
        const auto group = m_schematic.Connectivity().NetByName( aName );

        if( !group || group->instances.empty() )
            return {};

        const auto& view = group->instances.front();

        if( view.IsNet() )
            names.insert( group->name );

        for( const auto& member : view.Members().leaves )
        {
            if( member.IsNet() )
                names.insert( member.Name() );
        }
    }
    else
    {
        for( const CONNECTION_SUBGRAPH* subgraph : m_schematic.ConnectionGraph()->GetAllSubgraphs( aName ) )
        {
            const SCH_CONNECTION* connection = subgraph->GetDriverConnection();

            if( !connection )
                continue;

            if( connection->IsNet() )
                names.insert( connection->Name() );

            for( const auto& member : connection->AllMembers() )
            {
                if( member && member->IsNet() )
                    names.insert( member->Name() );
            }
        }
    }

    names.erase( wxString() );
    return { names.begin(), names.end() };
}


std::set<const CONNECTION_SUBGRAPH*> NAVIGATION_QUERY::netSubgraphs( const wxString& aName,
                                                                     bool aIncludeBusParents,
                                                                     bool aIncludeBusMembers ) const
{
    if( !m_schematic.IsValid() )
        return {};

    const auto& graph = *m_schematic.ConnectionGraph();
    std::set<const CONNECTION_SUBGRAPH*> subgraphs;
    std::vector<const CONNECTION_SUBGRAPH*> pending;
    auto add = [&]( const wxString& name )
    {
        for( const CONNECTION_SUBGRAPH* subgraph : graph.GetAllSubgraphs( name ) )
        {
            if( subgraph && subgraphs.insert( subgraph ).second )
                pending.push_back( subgraph );
        }
    };
    add( aName );

    if( aIncludeBusMembers )
    {
        for( const wxString& member : SignalNames( aName ) )
            add( member );
    }

    if( aIncludeBusParents )
    {
        for( const wxString& equivalent : graph.GetEquivalentBusNames( aName ) )
            add( equivalent );

        for( size_t i = 0; i < pending.size(); ++i )
        {
            for( const auto& [member, parents] : pending[i]->GetBusParents() )
            {
                for( const CONNECTION_SUBGRAPH* parent : parents )
                {
                    if( parent )
                        add( parent->GetNetName() );
                }
            }
        }
    }

    std::erase_if( subgraphs,
                   []( const CONNECTION_SUBGRAPH* subgraph )
                   {
                       return !subgraph->GetSheet().LastScreen() || subgraph->GetItems().empty();
                   } );

    return subgraphs;
}


NET_ITEMS_BY_SHEET NAVIGATION_QUERY::netItemsByEngine( const wxString& aName, bool aIncludeBusParents,
                                                       bool aIncludeBusMembers ) const
{
    NET_ITEMS_BY_SHEET result;
    const auto& facade = m_schematic.Connectivity();
    std::set<NODE_ID> visited;
    std::vector<NET_GROUP> pending;
    auto add = [&]( NET_GROUP group )
    {
        if( !group.instances.empty() && visited.insert( group.instances.front().Component() ).second )
            pending.push_back( std::move( group ) );
    };

    if( auto group = facade.NetByName( aName ) )
        add( std::move( *group ) );

    if( aIncludeBusMembers )
    {
        for( auto& group : facade.BusWithMembers( aName ) )
            add( std::move( group ) );
    }

    if( aIncludeBusParents )
    {
        for( const wxString& equivalent : facade.GetEquivalentBusNames( aName ) )
        {
            if( auto group = facade.NetByName( equivalent ) )
                add( std::move( *group ) );
        }

        for( size_t i = 0; i < pending.size(); ++i )
        {
            for( auto& parent : facade.BundlesOf( pending[i].instances.front().Component() ) )
                add( std::move( parent ) );
        }
    }

    for( const NET_GROUP& group : pending )
    {
        for( const NET_VIEW& view : group.instances )
        {
            const auto items = view.Items();

            if( items.empty() )
                continue;

            const auto& index = paths();
            const auto path = index.find( view.Instance() );

            if( path != index.end() )
            {
                auto& collected = result[path->second];
                collected.insert( collected.end(), items.begin(), items.end() );
            }
        }
    }

    return result;
}


bool NAVIGATION_QUERY::HasNet( const wxString& aName ) const
{
    if( ADVANCED_CFG::GetCfg().m_ConnectivityEngine )
        return !netItemsByEngine( aName, false, false ).empty();

    return !netSubgraphs( aName, false, false ).empty();
}


std::set<KIID_PATH> NAVIGATION_QUERY::NetSheets( const wxString& aName ) const
{
    std::set<KIID_PATH> sheets;

    if( ADVANCED_CFG::GetCfg().m_ConnectivityEngine )
    {
        for( const auto& [sheet, items] : netItemsByEngine( aName, false, false ) )
            sheets.insert( sheet.PathRef() );

        return sheets;
    }

    for( const CONNECTION_SUBGRAPH* subgraph : netSubgraphs( aName, false, false ) )
        sheets.insert( subgraph->GetSheet().PathRef() );

    return sheets;
}


NET_ITEMS_BY_SHEET NAVIGATION_QUERY::NetItems( const wxString& aName, bool aIncludeBusParents,
                                              bool aIncludeBusMembers ) const
{
    NET_ITEMS_BY_SHEET result;

    if( ADVANCED_CFG::GetCfg().m_ConnectivityEngine )
    {
        result = netItemsByEngine( aName, aIncludeBusParents, aIncludeBusMembers );
    }
    else
    {
        for( const CONNECTION_SUBGRAPH* subgraph : netSubgraphs( aName, aIncludeBusParents, aIncludeBusMembers ) )
        {
            auto& items = result[subgraph->GetSheet()];
            items.insert( items.end(), subgraph->GetItems().begin(), subgraph->GetItems().end() );
        }
    }

    for( auto& [path, items] : result )
    {
        std::ranges::sort( items, std::less<>{}, &SCH_ITEM::m_Uuid );
        items.erase( std::unique( items.begin(), items.end() ), items.end() );
    }

    return result;
}


void NAVIGATION_QUERY::CollectNetItems( const wxString& aName, const SCH_SHEET_PATH& aSheet,
                                        std::unordered_set<SCH_ITEM*>& aItems, bool aIncludeBusParents,
                                        bool aIncludeBusMembers ) const
{
    if( ADVANCED_CFG::GetCfg().m_ConnectivityEngine )
    {
        NET_ITEMS_BY_SHEET result = netItemsByEngine( aName, aIncludeBusParents, aIncludeBusMembers );
        auto it = result.find( aSheet );

        if( it != result.end() )
            aItems.insert( it->second.begin(), it->second.end() );

        return;
    }

    for( const CONNECTION_SUBGRAPH* subgraph : netSubgraphs( aName, aIncludeBusParents, aIncludeBusMembers ) )
    {
        if( subgraph->GetSheet() == aSheet )
            aItems.insert( subgraph->GetItems().begin(), subgraph->GetItems().end() );
    }
}
}
