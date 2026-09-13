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

#include <connection_graph.h>
#include <schematic.h>
#include <sch_item.h>
#include <algorithm>

namespace SCH_CONNECTIVITY
{
NAVIGATION_QUERY::NAVIGATION_QUERY( const SCHEMATIC& aSchematic ) : m_schematic( aSchematic )
{}


std::vector<wxString> NAVIGATION_QUERY::NetNames() const
{
    if( !m_schematic.IsValid() )
        return {};

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


bool NAVIGATION_QUERY::HasNet( const wxString& aName ) const
{
    return !netSubgraphs( aName, false, false ).empty();
}


std::set<KIID_PATH> NAVIGATION_QUERY::NetSheets( const wxString& aName ) const
{
    std::set<KIID_PATH> sheets;

    for( const CONNECTION_SUBGRAPH* subgraph : netSubgraphs( aName, false, false ) )
        sheets.insert( subgraph->GetSheet().PathRef() );

    return sheets;
}


NET_ITEMS_BY_SHEET NAVIGATION_QUERY::NetItems( const wxString& aName, bool aIncludeBusParents,
                                              bool aIncludeBusMembers ) const
{
    NET_ITEMS_BY_SHEET result;

    for( const CONNECTION_SUBGRAPH* subgraph : netSubgraphs( aName, aIncludeBusParents, aIncludeBusMembers ) )
    {
        auto& items = result[subgraph->GetSheet()];
        items.insert( items.end(), subgraph->GetItems().begin(), subgraph->GetItems().end() );
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
    for( const CONNECTION_SUBGRAPH* subgraph : netSubgraphs( aName, aIncludeBusParents, aIncludeBusMembers ) )
    {
        if( subgraph->GetSheet() == aSheet )
            aItems.insert( subgraph->GetItems().begin(), subgraph->GetItems().end() );
    }
}
}
