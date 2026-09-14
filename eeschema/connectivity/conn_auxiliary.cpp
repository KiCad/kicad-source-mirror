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

#include "conn_auxiliary.h"
#include <algorithm>
#include <iterator>
#include <memory>
#include <stdexcept>
#include <unordered_map>

namespace SCH_CONNECTIVITY
{
AUXILIARY::AUXILIARY( SESSION_KEYS& aKeys ) :
        m_keys( aKeys ),
        m_islands( KEY_LESS{ aKeys } ),
        m_areas( KEY_LESS{ aKeys } ),
        m_islandOf( KEY_LESS{ aKeys } ),
        m_ruleAreasOf( KEY_LESS{ aKeys } ),
        m_neighborsOf( KEY_LESS{ aKeys } )
{
}

void AUXILIARY::Update( std::span<const FRAME_INSTANCE> aFrame, const INPUT_STORE& aInputs,
                        const RECORD_STORE::RECORD_CACHE& aRecords, CHANGE_SET& aChanges )
{
    struct ISLAND_STAGE
    {
        uint64_t                    islandVersion = 0;
        uint64_t                    recordVersion = 0;
        std::unique_ptr<ISLAND_AUX> value;
    };
    struct AREA_STAGE
    {
        uint64_t                        version = 0;
        std::optional<RULE_AREA_RESULT> value;
    };
    std::vector<std::pair<RECORD_KEY, ISLAND_STAGE>> islands;
    std::map<ITEM_KEY, AREA_STAGE, KEY_LESS>         areas( KEY_LESS{ m_keys } );
    std::set<INST_ID>                                instances;
    uint64_t                                         sourceVersion = m_sourceVersion;
    std::map<SCREEN_ID, std::vector<KIID>>           sourceChanges;
    islands.reserve( m_islands.size() );
    std::vector<const FRAME_INSTANCE*> frames;
    frames.reserve( aFrame.size() );

    for( const FRAME_INSTANCE& frame : aFrame )
        frames.push_back( &frame );

    // Each screen's islands are anchor-sorted; canonical instance order also orders the staged records
    std::sort( frames.begin(), frames.end(),
               [&]( const FRAME_INSTANCE* a, const FRAME_INSTANCE* b )
               {
                   return m_keys.Instance( a->scope.instance ) < m_keys.Instance( b->scope.instance );
               } );
    const auto less = m_islands.key_comp();
    auto       recordPosition = aRecords.Entries().begin();
    auto       oldPosition = m_islands.cbegin();

    for( const FRAME_INSTANCE* captured : frames )
    {
        const FRAME_INSTANCE& frame = *captured;
        const INST_ID         instance = frame.scope.instance;
        const auto*           facts = aInputs.FindInstance( instance );

        if( !facts )
            throw std::invalid_argument( "Auxiliary publication requires captured instance facts" );

        const auto* screen = aInputs.FindScreen( frame.screen );

        if( !screen )
            throw std::invalid_argument( "Auxiliary publication requires captured screen facts" );

        sourceVersion = std::max( { sourceVersion, screen->version, facts->version } );

        instances.insert( instance );

        // A new instance has new item identities even when its shared screen is unchanged
        if( !m_instances.contains( instance ) )
        {
            for( const ITEM_FACT& item : screen->value.items )
            {
                aChanges.changedItems.push_back( { item.id, instance } );

                for( const PIN_FACT& pin : item.pins )
                    aChanges.changedItems.push_back( { pin.id, instance } );
            }
        }
        else if( screen->version > m_sourceVersion )
        {
            auto [changed, inserted] = sourceChanges.try_emplace( frame.screen );

            if( inserted )
            {
                for( const ITEM_FACT& item : screen->value.items )
                {
                    const auto* source = aInputs.Fact( { frame.screen, item.id } );

                    if( source && source->version > m_sourceVersion )
                    {
                        changed->second.push_back( item.id );

                        for( const PIN_FACT& pin : item.pins )
                            changed->second.push_back( pin.id );
                    }
                }
            }

            for( const KIID& item : changed->second )
                aChanges.changedItems.push_back( { item, instance } );
        }

        // Aggregate entries are versioned after their rows on the same monotonic clock
        if( facts->version > m_sourceVersion )
        {
            for( const ITEM_TEXT_FACT& item : facts->value.items )
            {
                const auto* text = aInputs.Text( { item.id, instance } );

                if( text && text->version > m_sourceVersion )
                    aChanges.changedItems.push_back( { item.id, instance } );
            }
        }

        const auto* geometry = aInputs.FindIslands( frame.screen, facts->value.units );

        if( !geometry )
            throw std::invalid_argument( "Auxiliary publication requires captured islands" );

        for( const ISLAND& island : geometry->value.islands )
        {
            const RECORD_KEY key{ instance, island.anchor };

            while( recordPosition != aRecords.Entries().end() && less( recordPosition->first, key ) )
                ++recordPosition;

            if( recordPosition == aRecords.Entries().end() || recordPosition->first != key )
                throw std::invalid_argument( "Auxiliary island has no electrical record" );

            const auto* record = recordPosition->second.get();
            auto&       stage = islands.emplace_back( key, ISLAND_STAGE{} ).second;
            stage.islandVersion = geometry->version;
            stage.recordVersion = record->version;

            while( oldPosition != m_islands.cend() && less( oldPosition->first, key ) )
                ++oldPosition;

            if( oldPosition != m_islands.cend() && oldPosition->first == key )
            {
                if( oldPosition->second.islandVersion == stage.islandVersion
                    && oldPosition->second.recordVersion == stage.recordVersion )
                    continue;

                const ISLAND_AUX& value = oldPosition->second.value;

                if( value.items == island.items && value.adjacency == island.adjacency
                    && value.dangling == island.dangling && value.busEntryLinks == island.busEntryLinks
                    && value.ncContacts == island.ncContacts && value.atoms == record->value.atoms )
                    continue;
            }

            stage.value = std::make_unique<ISLAND_AUX>( island.items, island.adjacency, island.dangling,
                                                        island.busEntryLinks, island.ncContacts, record->value.atoms );
        }

        for( const auto& area : facts->value.ruleAreas )
        {
            const ITEM_KEY key{ area.id, instance };
            const auto*    source = aInputs.Area( key );

            if( !source )
                throw std::invalid_argument( "Auxiliary rule area has no captured source" );

            auto& stage = areas[key];
            stage.version = source->version;
            const auto old = m_areas.find( key );

            if( old != m_areas.end() && old->second.version == source->version )
                continue;

            RULE_AREA_RESULT value{ source->value.containedItems, source->value.attachedDirectives, {} };

            for( const wxString& name : source->value.netclasses )
                value.netclasses.push_back( m_keys.InternName( name ) );

            std::ranges::sort( value.netclasses, NAME_LESS{ &m_keys } );
            value.netclasses.erase( std::ranges::unique( value.netclasses ).begin(), value.netclasses.end() );

            if( old == m_areas.end() || old->second.value != value )
                stage.value = std::move( value );
        }
    }

    auto replacement = islands.cbegin();

    // Remove old memberships before adding replacements, including items whose island anchor changed
    for( auto it = m_islands.begin(); it != m_islands.end(); )
    {
        while( replacement != islands.cend() && less( replacement->first, it->first ) )
            ++replacement;

        const bool removed = replacement == islands.cend() || less( it->first, replacement->first );

        if( removed || bool( replacement->second.value ) )
        {
            aChanges.changedIslands.push_back( it->first );

            for( const KIID& id : it->second.value.items )
            {
                const ITEM_KEY item{ id, it->first.inst };
                m_islandOf.erase( item );
                m_neighborsOf.erase( item );
                aChanges.changedItems.push_back( item );
            }
        }

        if( removed )
            it = m_islands.erase( it );
        else
            ++it;
    }

    auto hint = m_islands.cbegin();

    for( auto& [key, entry] : islands )
    {
        if( entry.value )
        {
            aChanges.changedIslands.push_back( key );
            std::unordered_map<KIID, std::vector<KIID>*> neighborLists;

            // An endpoint's map value remains stable while its island's adjacency is appended
            const auto appendNeighbor = [&]( const KIID& item, const KIID& neighbor )
            {
                auto [found, inserted] = neighborLists.try_emplace( item, nullptr );

                if( inserted )
                    found->second = &m_neighborsOf[{ item, key.inst }];

                found->second->push_back( neighbor );
            };

            for( const auto& [first, second] : entry.value->adjacency )
            {
                appendNeighbor( first, second );
                appendNeighbor( second, first );
            }

            for( const KIID& id : entry.value->items )
            {
                const ITEM_KEY item{ id, key.inst };
                const auto     neighbors = m_neighborsOf.find( item );

                if( neighbors != m_neighborsOf.end() )
                {
                    auto& values = neighbors->second;
                    std::sort( values.begin(), values.end() );
                    values.erase( std::unique( values.begin(), values.end() ), values.end() );
                }

                m_islandOf.insert_or_assign( item, key );
                aChanges.changedItems.push_back( item );
            }
        }

        const auto current = m_islands.try_emplace( hint, key );
        hint = std::next( current );
        auto& published = current->second;
        published.islandVersion = entry.islandVersion;
        published.recordVersion = entry.recordVersion;

        if( entry.value )
            published.value = std::move( *entry.value );
    }

    const auto areaItems = []( const RULE_AREA_RESULT& value )
    {
        std::vector<KIID> items;
        items.reserve( value.containedItems.size() + value.attachedDirectives.size() );
        std::set_union( value.containedItems.begin(), value.containedItems.end(), value.attachedDirectives.begin(),
                        value.attachedDirectives.end(), std::back_inserter( items ) );
        return items;
    };

    for( auto it = m_areas.begin(); it != m_areas.end(); )
    {
        const auto updatedArea = areas.find( it->first );
        const bool removed = updatedArea == areas.end();

        if( removed || updatedArea->second.value.has_value() )
        {
            aChanges.changedRuleAreas.push_back( it->first );

            for( const KIID& id : areaItems( it->second.value ) )
            {
                const ITEM_KEY item{ id, it->first.inst };
                auto           found = m_ruleAreasOf.find( item );

                if( found != m_ruleAreasOf.end() )
                {
                    std::erase( found->second, it->first.item );

                    if( found->second.empty() )
                        m_ruleAreasOf.erase( found );
                }

                aChanges.changedItems.push_back( item );
            }
        }

        if( removed )
            it = m_areas.erase( it );
        else
            ++it;
    }

    for( auto& [key, entry] : areas )
    {
        if( entry.value )
        {
            aChanges.changedRuleAreas.push_back( key );

            for( const KIID& id : areaItems( *entry.value ) )
            {
                const ITEM_KEY item{ id, key.inst };
                auto&          memberships = m_ruleAreasOf[item];
                memberships.insert( std::lower_bound( memberships.begin(), memberships.end(), key.item ), key.item );
                aChanges.changedItems.push_back( item );
            }
        }

        auto& published = m_areas.try_emplace( key ).first->second;
        published.version = entry.version;

        if( entry.value )
            published.value = std::move( *entry.value );
    }

    const auto canonicalize = [&]( auto& values )
    {
        std::ranges::sort( values, KEY_LESS{ m_keys } );
        values.erase( std::ranges::unique( values ).begin(), values.end() );
    };
    canonicalize( aChanges.changedItems );
    canonicalize( aChanges.changedIslands );
    canonicalize( aChanges.changedRuleAreas );
    m_screenRevisions = aInputs.ScreenRevisions();
    m_instances = std::move( instances );
    m_sourceVersion = sourceVersion;
}

void AUXILIARY::Clear()
{
    m_islands.clear();
    m_areas.clear();
    m_islandOf.clear();
    m_ruleAreasOf.clear();
    m_neighborsOf.clear();
    m_screenRevisions.clear();
    m_instances.clear();
    m_sourceVersion = 0;
}
} // namespace SCH_CONNECTIVITY
