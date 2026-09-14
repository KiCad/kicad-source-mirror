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

#include "conn_publish.h"
#include <sch_connection.h>
#include <algorithm>
#include <numeric>
#include <set>
#include <tuple>

namespace SCH_CONNECTIVITY
{
namespace
{
    template<typename T>
    bool SameValue( const std::shared_ptr<const T>& a, const std::shared_ptr<const T>& b )
    {
        return a == b || ( a && b && *a == *b );
    }
} // namespace

bool ITEM_RESULT::BUS_SOURCE::operator==( const BUS_SOURCE& aOther ) const
{
    return source == aOther.source && SameValue( schema, aOther.schema );
}

bool ITEM_RESULT::operator==( const ITEM_RESULT& aOther ) const
{
    return std::tie( component, kind, itemType, name, localName, fullLocalName, driver, netCode, subgraphCode )
                   == std::tie( aOther.component, aOther.kind, aOther.itemType, aOther.name, aOther.localName,
                                aOther.fullLocalName, aOther.driver, aOther.netCode, aOther.subgraphCode )
           && SameValue( netclasses, aOther.netclasses ) && SameValue( busSource, aOther.busSource );
}

PUBLICATION::PUBLICATION( SESSION_KEYS& aKeys ) :
        m_keys( aKeys ),
        m_auxiliary( aKeys ),
        m_rows( KEY_LESS{ aKeys } ),
        m_rowInputs( KEY_LESS{ aKeys } ),
        m_netclasses( NAME_LESS{ &aKeys } ),
        m_byName( NAME_LESS{ &aKeys } ),
        m_slotComponents( KEY_LESS{ aKeys } )
{
}

PUBLICATION::ROW_UPDATE PUBLICATION::PrepareRows( const COMPONENTS& aCurrent,
                                                  const RECORD_STORE::RECORD_CACHE& aRecords,
                                                  const SLOT_INPUTS& aSlots, CHANGE_SET& aChanges )
{
    ROW_UPDATE                     result( m_keys );
    std::set<RECORD_KEY, KEY_LESS> removedInputs( KEY_LESS{ m_keys } );
    const CLAIM_LESS               less{ m_keys };
    const auto sameComponent = []( const PUBLISHED_COMPONENT& a, const PUBLISHED_COMPONENT& b )
    {
        // Equal aggregate values can still hide changed record-local claims
        return a.content == b.content && a == b;
    };

    for( const auto& [node, component] : m_components )
    {
        const auto current = aCurrent.find( node );

        if( current == aCurrent.end() || !sameComponent( component, current->second ) )
            removedInputs.insert( component.content->records.begin(), component.content->records.end() );
    }

    for( const auto& [node, component] : aCurrent )
    {
        ITEM_RESULT connection;
        connection.component = node;
        connection.kind = component.content->kind;
        connection.itemType = connection.kind == KIND::BUNDLE ? CONNECTION_TYPE::BUS : CONNECTION_TYPE::NONE;
        connection.name = component.name;
        connection.netCode = component.netCode;
        connection.subgraphCode = component.subgraphCode;

        if( !component.content->netclasses.empty() )
        {
            // The netclass map orders by name text, which an unnamed component does not have
            const auto old = component.name == INVALID_ID ? m_netclasses.end() : m_netclasses.find( component.name );
            connection.netclasses =
                    old != m_netclasses.end() && *old->second == component.content->netclasses
                            ? old->second
                            : std::make_shared<const std::vector<NAME_ID>>( component.content->netclasses );
        }

        if( component.content->best )
        {
            connection.driver = component.content->best->source;
            connection.itemType = component.content->kind == KIND::SIGNAL ? CONNECTION_TYPE::NET
                                  : component.content->best->schema->shape == BUS_SCHEMA::SHAPE::VECTOR
                                          ? CONNECTION_TYPE::BUS
                                          : CONNECTION_TYPE::BUS_GROUP;
            connection.localName =
                    component.nameSlot ? aSlots.at( *component.nameSlot )->localName : component.content->best->name;
        }

        if( component.name != INVALID_ID && !component.content->netclasses.empty() )
            result.netclasses.emplace( component.name, connection.netclasses );

        const auto previous = m_components.find( node );

        if( previous != m_components.end() && sameComponent( previous->second, component ) )
            continue;

        // Islands on one sheet merge only through a shared driver name, which their sheet edges record
        const auto&                                  records = component.content->records;
        std::vector<size_t>                          parents( records.size() );
        std::vector<std::pair<size_t, const CLAIM*>> groups( records.size() );
        std::iota( parents.begin(), parents.end(), size_t( 0 ) );

        const auto root = [&]( size_t aIndex )
        {
            while( parents[aIndex] != aIndex )
                aIndex = parents[aIndex] = parents[parents[aIndex]];

            return aIndex;
        };

        if( component.content->kind == KIND::SIGNAL )
        {
            std::map<std::pair<INST_ID, NAME_ID>, size_t> names;

            for( size_t index = 0; index < records.size(); ++index )
            {
                const auto* record = aRecords.Find( records[index] );

                if( !record )
                    continue;

                for( const NAME_KEY& edge : record->value.edges )
                {
                    if( edge.scope != SCOPE::SHEET )
                        continue;

                    const auto [named, inserted] = names.try_emplace( { edge.inst, edge.text }, index );

                    if( !inserted )
                        parents[root( index )] = root( named->second );
                }
            }

            for( size_t index = 0; index < records.size(); ++index )
            {
                const auto* record = aRecords.Find( records[index] );

                if( !record || record->value.claims.empty() )
                    continue;

                auto& [count, best] = groups[root( index )];
                ++count;

                if( !best || less( *best, record->value.claims.front() ) )
                    best = &record->value.claims.front();
            }
        }

        for( size_t index = 0; index < records.size(); ++index )
        {
            const RECORD_KEY& key = records[index];
            removedInputs.erase( key );
            const auto* record = aRecords.Find( key );

            if( !record || record->value.kind != component.content->kind )
                throw std::invalid_argument( "Published component does not match its row records" );

            const auto   old = m_rowInputs.find( key );
            ITEM_RESULT  local = connection;
            const CLAIM* localClaim = component.content->best ? &*component.content->best : nullptr;

            if( !record->value.claims.empty() )
            {
                localClaim = &record->value.claims.front();
                local.localName = localClaim->name;

                if( record->value.claims.front().priority == PRIORITY::PIN && component.content->best
                    && component.content->best->priority == PRIORITY::PIN )
                {
                    local.localName = component.baseName;
                    localClaim = &*component.content->best;
                }
            }

            // Items of a merged group take the winner's local name, but each island driver keeps its own
            NAME_ID driverName = INVALID_ID;

            if( !record->value.claims.empty() && localClaim == &record->value.claims.front() )
            {
                const auto& [count, winner] = groups[root( index )];

                if( count > 1 && winner->name != localClaim->name )
                {
                    driverName = local.localName;
                    local.localName = winner->name;
                }
            }

            if( local.kind == KIND::BUNDLE && localClaim && localClaim->schema && component.content->best
                && localClaim->schema->shape == component.content->best->schema->shape
                && localClaim->name != component.content->best->name )
            {
                const ITEM_RESULT::BUS_SOURCE source{ localClaim->source, localClaim->schema };
                const auto previousSource = old != m_rowInputs.end() ? old->second->connection.busSource : nullptr;
                local.busSource = previousSource && *previousSource == source
                                          ? previousSource
                                          : std::make_shared<const ITEM_RESULT::BUS_SOURCE>( source );
            }

            // Cached row inputs and components always describe the same previous publication
            if( local.localName == INVALID_ID || !component.suffix )
                local.fullLocalName = local.localName;
            else if( old != m_rowInputs.end() && old->second->version == record->version
                     && old->second->connection.localName == local.localName
                     && old->second->connection.itemType == local.itemType
                     && m_components.at( old->second->connection.component ).suffix == component.suffix )
                local.fullLocalName = old->second->connection.fullLocalName;
            else
                local.fullLocalName = m_keys.InternName(
                        ApplyNameSuffix( m_keys.Name( local.localName ),
                                         localClaim ? localClaim->schema.get() : nullptr, component.suffix ) );

            std::optional<std::pair<KIID, ITEM_RESULT>> driver;

            if( driverName != INVALID_ID )
            {
                ITEM_RESULT row = local;
                row.localName = driverName;
                row.fullLocalName = driverName;

                if( component.suffix )
                {
                    row.fullLocalName = m_keys.InternName(
                            ApplyNameSuffix( m_keys.Name( driverName ), nullptr, component.suffix ) );
                }

                driver.emplace( localClaim->source.item, std::move( row ) );
            }

            if( old == m_rowInputs.end() || old->second->version != record->version
                || old->second->connection != local || old->second->driver != driver )
            {
                result.inputs.emplace( key, std::make_shared<const ROW_INPUT>( ROW_INPUT{
                                                    record->version, local, record->value.items,
                                                    std::move( driver ) } ) );
            }
        }
    }

    std::set<ITEM_KEY, KEY_LESS> removed( KEY_LESS{ m_keys } );
    const auto removePreviousItems = [&]( const RECORD_KEY& key )
    {
        const auto old = m_rowInputs.find( key );

        if( old != m_rowInputs.end() )
        {
            for( const KIID& item : old->second->items )
                removed.insert( { item, key.inst } );
        }
    };
    result.removedInputs.assign( removedInputs.begin(), removedInputs.end() );

    for( const RECORD_KEY& key : result.removedInputs )
        removePreviousItems( key );

    for( const auto& [key, input] : result.inputs )
        removePreviousItems( key );

    for( const auto& [key, replacement] : result.inputs )
    {
        const ROW_INPUT& input = *replacement;

        for( const KIID& id : input.items )
        {
            const ITEM_RESULT& row = input.driver && input.driver->first == id ? input.driver->second
                                                                                : input.connection;
            const ITEM_KEY item{ id, key.inst };

            removed.erase( item );
            const auto old = m_rows.find( item );

            if( old == m_rows.end() || old->second != row )
            {
                aChanges.changedItems.push_back( item );

                if( old == m_rows.end() ? row.driver.has_value() : old->second.driver != row.driver )
                    aChanges.driverChangedItems.push_back( item );

                result.upserts.emplace( item, row );
            }
        }
    }

    result.removed.assign( removed.begin(), removed.end() );

    for( const ITEM_KEY& item : result.removed )
    {
        const auto old = m_rows.find( item );

        if( old != m_rows.end() )
        {
            aChanges.changedItems.push_back( item );

            if( old->second.driver )
                aChanges.driverChangedItems.push_back( item );
        }
    }

    std::ranges::sort( aChanges.driverChangedItems, KEY_LESS{ m_keys } );

    for( const auto& [name, classes] : result.netclasses )
    {
        const auto old = m_netclasses.find( name );

        if( old == m_netclasses.end() || !SameValue( old->second, classes ) )
            aChanges.netclasses.assigned.emplace_back( name, *classes );
    }

    for( const auto& [name, classes] : m_netclasses )
    {
        if( !result.netclasses.contains( name ) )
            aChanges.netclasses.removed.push_back( name );
    }

    std::ranges::sort( aChanges.netclasses.removed, NAME_LESS{ &m_keys } );
    return result;
}

void PUBLICATION::ApplyRows( ROW_UPDATE&& aUpdate )
{
    // Merge splices new keys and leaves existing ones behind to be assigned
    const auto upsert = []( auto& aTarget, auto& aSource )
    {
        aTarget.merge( aSource );

        for( auto& [key, value] : aSource )
            aTarget.find( key )->second = std::move( value );
    };

    for( const ITEM_KEY& item : aUpdate.removed )
        m_rows.erase( item );

    upsert( m_rows, aUpdate.upserts );

    for( const RECORD_KEY& key : aUpdate.removedInputs )
        m_rowInputs.erase( key );

    upsert( m_rowInputs, aUpdate.inputs );
    m_netclasses = std::move( aUpdate.netclasses );
}
} // namespace SCH_CONNECTIVITY
