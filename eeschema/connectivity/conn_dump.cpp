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

#include "conn_dump.h"
#include "conn_facade.h"

#include <json_common.h>
#include <netclass.h>
#include <project.h>
#include <project/net_settings.h>
#include <project/project_file.h>
#include <sch_bus_entry.h>
#include <sch_connection.h>
#include <sch_line.h>
#include <sch_pin.h>
#include <sch_screen.h>
#include <sch_sheet.h>
#include <sch_sheet_pin.h>
#include <sch_symbol.h>
#include <schematic.h>

#include <map>
#include <set>
#include <stdexcept>


namespace
{
using JSON = nlohmann::json;
using KEY = std::pair<std::string, std::string>;
using SCH_CONNECTIVITY::DUMP_ROW;
using SCH_CONNECTIVITY::DUMP_ROWS;
using SCH_CONNECTIVITY::DUMP_VALUE;

std::string utf8( const wxString& aText )
{
    return aText.ToStdString( wxConvUTF8 );
}


const char* typeName( CONNECTION_TYPE aType )
{
    switch( aType )
    {
    case CONNECTION_TYPE::NET:       return "net";
    case CONNECTION_TYPE::BUS:       return "vector";
    case CONNECTION_TYPE::BUS_GROUP: return "group";
    case CONNECTION_TYPE::NONE:      return "none";
    }

    return "none";
}


JSON valueJson( const DUMP_VALUE& aValue )
{
    std::set<std::string> classes;

    for( const wxString& name : aValue.netclasses )
        classes.insert( utf8( name ) );

    JSON row = { { "name", utf8( aValue.name ) }, { "local_name", utf8( aValue.localName ) },
                 { "type", typeName( aValue.type ) },
                 { "driver", aValue.driver ? utf8( aValue.driver->AsString() ) : "" },
                 { "driver_path", aValue.driver ? utf8( aValue.driverPath.AsString() ) : "" },
                 { "members", JSON::array() }, { "netclasses", classes } };

    for( const DUMP_VALUE& member : aValue.members )
        row["members"].push_back( valueJson( member ) );

    return row;
}


std::set<wxString> resolvedClasses( const wxString& aName, NET_SETTINGS* aSettings )
{
    std::set<wxString> classes;

    if( aSettings )
    {
        for( const NETCLASS* constituent : aSettings->GetEffectiveNetClass( aName )->GetConstituentNetclasses() )
            classes.insert( constituent->GetName() );
    }

    return classes;
}


DUMP_VALUE valueRow( const wxString& aName, const wxString& aLocalName, CONNECTION_TYPE aType,
                     SCH_ITEM* aDriver, const KIID_PATH& aDriverPath, NET_SETTINGS* aSettings )
{
    DUMP_VALUE value;
    value.name = aName;
    value.localName = aLocalName;
    value.type = aType;
    value.netclasses = resolvedClasses( aName, aSettings );

    if( aDriver )
    {
        value.driver = aDriver->m_Uuid;
        value.driverPath = aDriverPath;
    }

    return value;
}


DUMP_VALUE memberRow( const SCH_CONNECTIVITY::BUS_SCHEMA::NODE& aNode,
                      const SCH_CONNECTIVITY::BUS_MEMBERS& aMembers, const wxString& aPath,
                      const wxString& aNamePrefix, const wxString& aLocalPrefix, SCH_ITEM* aDriver,
                      const KIID_PATH& aDriverPath, NET_SETTINGS* aSettings )
{
    using NODE = SCH_CONNECTIVITY::BUS_SCHEMA::NODE;

    if( aNode.kind == NODE::KIND::NET )
    {
        const auto& leaf = aMembers.leaves.at( aNode.leaf.value() );
        return valueRow( leaf.Name(), aMembers.schema->leaves.at( aNode.leaf.value() ).name,
                         CONNECTION_TYPE::NET, leaf.Driver(), leaf.Sheet(), aSettings );
    }

    const wxString localName = aLocalPrefix + aNode.text;
    DUMP_VALUE row = valueRow( aPath + aNamePrefix + aNode.text, localName,
                               aNode.kind == NODE::KIND::VECTOR ? CONNECTION_TYPE::BUS : CONNECTION_TYPE::BUS_GROUP,
                               aDriver, aDriverPath, aSettings );
    wxString namePrefix = aNamePrefix;
    wxString localPrefix = aLocalPrefix;

    if( aNode.kind == NODE::KIND::GROUP && !aNode.prefix.empty() )
    {
        namePrefix += aNode.prefix + ".";
        localPrefix += aNode.prefix + ".";
    }

    for( const auto& member : aNode.members )
    {
        row.members.push_back( memberRow( member, aMembers, aPath, namePrefix, localPrefix, aDriver, aDriverPath,
                                          aSettings ) );
    }

    return row;
}


DUMP_VALUE connectionRow( const SCH_CONNECTIVITY::ITEM_VIEW& aView,
                          const SCH_CONNECTIVITY::PUBLISHED_COMPONENT& aComponent,
                          CONNECTION_TYPE aType, NET_SETTINGS* aSettings )
{
    const wxString name = aView.Name();
    SCH_ITEM* driver = aView.Driver();
    const KIID_PATH driverPath = aView.Sheet();
    DUMP_VALUE row = valueRow( name, aView.FullLocalName(), aType, driver, driverPath, aSettings );
    const auto members = aView.Members();

    if( !members.tree )
        return row;

    const auto groupPrefix = [&]( const wxString& aPrefix )
    {
        if( aPrefix.empty() )
            return wxString();

        return aComponent.suffix ? wxString::Format( "%s_%u.", aPrefix, aComponent.suffix ) : aPrefix + wxS( "." );
    };
    const bool group = members.tree->kind == SCH_CONNECTIVITY::BUS_SCHEMA::NODE::KIND::GROUP;
    const wxString localPrefix = group ? groupPrefix( members.tree->prefix ) : wxString();
    const auto& best = aComponent.content->best;

    // Members are named after the canonical group even where this row spells another prefix
    const wxString namePrefix = group && best && best->schema
                                                && best->schema->shape == SCH_CONNECTIVITY::BUS_SCHEMA::SHAPE::GROUP
                                        ? groupPrefix( best->schema->prefix )
                                        : localPrefix;
    const wxString path = name.Left( name.length() - aView.Name( true ).length() );

    for( const auto& member : members.tree->members )
    {
        row.members.push_back( memberRow( member, members, path, namePrefix, localPrefix, driver, driverPath,
                                          aSettings ) );
    }

    return row;
}


DUMP_VALUE connectionRow( const SCH_CONNECTION* aConnection, NET_SETTINGS* aSettings )
{
    if( !aConnection )
        return valueRow( wxString(), wxString(), CONNECTION_TYPE::NONE, nullptr, KIID_PATH(), nullptr );

    const wxString name = !aConnection->Driver() && aConnection->Name( true ) == "<NO NET>"
                                  ? wxString() : aConnection->Name();
    DUMP_VALUE row = valueRow( name, aConnection->FullLocalName(), aConnection->Type(), aConnection->Driver(),
                               aConnection->Sheet().Path(), aSettings );

    // Member order encodes positional bus alignment, so only item rows are sorted
    for( const auto& member : aConnection->Members() )
        row.members.push_back( connectionRow( member.get(), aSettings ) );

    return row;
}
}


std::string SCH_CONNECTIVITY::Dump( const DUMP_ROWS& aRows )
{
    std::map<KEY, JSON> sorted;

    for( const auto& [key, row] : aRows )
    {
        const std::string pathId = utf8( key.first.AsString() );
        const std::string itemId = utf8( key.second.AsString() );
        std::set<std::string> connected;

        for( const KIID& neighbour : row.connected )
            connected.insert( utf8( neighbour.AsString() ) );

        JSON json = valueJson( row.value );
        json["path"] = pathId;
        json["item"] = itemId;
        json["dangling"] = row.dangling;
        json["dangling_ends"] = row.danglingEnds;
        json["connected"] = connected;
        sorted.emplace( KEY{ pathId, itemId }, std::move( json ) );
    }

    JSON result = JSON::array();

    for( auto& [key, row] : sorted )
        result.push_back( std::move( row ) );

    return result.dump( 2 ) + "\n";
}


std::string SCH_CONNECTIVITY::Dump( SCHEMATIC& aSchematic )
{
    return Dump( CaptureRows( aSchematic ) );
}


std::string SCH_CONNECTIVITY::Dump( SCHEMATIC& aSchematic, const FACADE& aFacade )
{
    return Dump( CaptureRows( aSchematic, aFacade ) );
}


SCH_CONNECTIVITY::DUMP_ROWS SCH_CONNECTIVITY::CaptureRows( SCHEMATIC& aSchematic )
{
    DUMP_ROWS rows;
    NET_SETTINGS* settings = aSchematic.IsValid()
                                     ? aSchematic.Project().GetProjectFile().NetSettings().get()
                                     : nullptr;

    for( const SCH_SHEET_PATH& path : aSchematic.Hierarchy() )
    {
        const KIID_PATH pathId = path.Path();
        const auto addItem =
                [&]( SCH_ITEM* aItem )
                {
                    DUMP_ROW row;
                    row.value = connectionRow( aItem->Connection( &path ), settings );
                    row.dangling = aItem->IsDangling();
                    row.danglingEnds = { aItem->IsDangling() };

                    if( auto* line = dynamic_cast<SCH_LINE*>( aItem ) )
                    {
                        row.danglingEnds = { line->IsStartDangling(), line->IsEndDangling() };
                    }
                    else if( auto* entry = dynamic_cast<SCH_BUS_ENTRY_BASE*>( aItem ) )
                    {
                        row.danglingEnds = { entry->IsStartDangling(), entry->IsEndDangling() };
                    }

                    for( const SCH_ITEM* neighbour : aItem->ConnectedItems( path ) )
                        row.connected.insert( neighbour->m_Uuid );

                    if( !rows.emplace( std::pair( pathId, aItem->m_Uuid ), std::move( row ) ).second )
                    {
                        throw std::runtime_error( "Duplicate connectivity item " + utf8( pathId.AsString() )
                                                  + utf8( aItem->m_Uuid.AsString() ) );
                    }
                };

        for( SCH_ITEM* item : path.LastScreen()->Items() )
        {
            if( item->Type() == SCH_SYMBOL_T )
            {
                for( SCH_PIN* pin : static_cast<SCH_SYMBOL*>( item )->GetPins( &path ) )
                    addItem( pin );
            }
            else if( item->Type() == SCH_SHEET_T )
            {
                for( SCH_SHEET_PIN* pin : static_cast<SCH_SHEET*>( item )->GetPins() )
                    addItem( pin );
            }
            else if( item->IsConnectable() )
            {
                addItem( item );
            }
        }
    }

    return rows;
}


SCH_CONNECTIVITY::DUMP_ROWS SCH_CONNECTIVITY::CaptureRows( SCHEMATIC& aSchematic, const FACADE& aFacade )
{
    const auto& publication = aFacade.Published();
    const auto& keys = aFacade.Keys();
    const auto& auxiliary = publication.Auxiliary();
    std::unique_ptr<NET_SETTINGS> settings;

    if( aSchematic.IsValid() )
    {
        auto source = aSchematic.Project().GetProjectFile().NetSettings();
        settings = std::make_unique<NET_SETTINGS>( nullptr, "" );
        settings->CopyFrom( *source );
        aFacade.ApplyNetclasses( *settings );

        // Chain patterns are shared consumer context, not the legacy graph's label assignments
        for( const auto& [owner, patterns] : source->GetChainPatternAssignments() )
        {
            for( const auto& [matcher, name] : patterns )
                settings->SetChainPatternAssignment( owner, matcher->GetPattern(), name );
        }
    }

    std::map<INST_ID, SCH_SCREEN*> screens;

    for( const SCH_SHEET_PATH& path : aSchematic.BuildSheetListSortedByPageNumbers() )
    {
        if( auto instance = keys.FindInstance( path.Path() ) )
            screens.emplace( *instance, path.LastScreen() );
    }

    DUMP_ROWS rows;

    for( const auto& [key, result] : publication.Rows() )
    {
        const KIID_PATH& path = keys.Instance( key.inst );
        auto view = aFacade.Connection( key.item, path );

        if( !view )
            throw std::runtime_error( "Cannot dump stale connectivity publication" );

        DUMP_ROW row;
        row.value = connectionRow( *view, publication.Components().at( result.component ), result.itemType,
                                   settings.get() );
        const auto& island = auxiliary.Islands().at( auxiliary.IslandOf().at( key ) ).value;
        const auto dangling = island.dangling.at( key.item );
        row.dangling = dangling != 0;
        row.danglingEnds = { dangling != 0 };
        const auto screen = screens.find( key.inst );

        if( screen == screens.end() || !screen->second )
            throw std::runtime_error( "Cannot dump connectivity without its source instance" );

        SCH_ITEM* item = screen->second->GetConnectivityItem( key.item );

        if( !item )
            throw std::runtime_error( "Cannot dump connectivity without its source item" );

        if( dynamic_cast<SCH_LINE*>( item ) || dynamic_cast<SCH_BUS_ENTRY_BASE*>( item ) )
            row.danglingEnds = { ( dangling & 1 ) != 0, ( dangling & 2 ) != 0 };

        // Undriven items keep the type their connection gets when it is created
        if( result.itemType == CONNECTION_TYPE::NONE && result.kind == SCH_CONNECTIVITY::KIND::SIGNAL
            && ( item->Type() == SCH_PIN_T || item->Type() == SCH_BUS_WIRE_ENTRY_T
                 || ( item->Type() == SCH_LINE_T && item->GetLayer() != LAYER_BUS ) ) )
        {
            row.value.type = CONNECTION_TYPE::NET;
        }

        const auto neighbors = auxiliary.NeighborsOf().find( key );

        if( neighbors != auxiliary.NeighborsOf().end() )
            row.connected.insert( neighbors->second.begin(), neighbors->second.end() );

        rows.emplace( std::pair( path, key.item ), std::move( row ) );
    }

    return rows;
}
