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

#include "conn_records.h"
#include "conn_tasks.h"

#include <wx/thread.h>
#include <algorithm>
#include <stdexcept>

namespace SCH_CONNECTIVITY
{
RECORD_STORE::RECORD_STORE( CACHE_VERSIONS& aVersions, SESSION_KEYS& aKeys ) :
        m_keys( aKeys ),
        m_parses( aVersions ),
        m_sources( aVersions, KEY_LESS{ aKeys } ),
        m_records( aVersions, KEY_LESS{ aKeys } )
{
}

void RECORD_STORE::Update( const std::vector<FRAME_INSTANCE>& aFrame, INPUT_STORE& aInputs,
                           const BUS_ALIASES& aAliases )
{
    wxASSERT( wxThread::IsMain() );
    try
    {
        const bool                             aliasesChanged = m_parses.SetAliases( aAliases );
        bool                                   claimNamesChanged = false;
        std::vector<const SOURCE_CACHE::ENTRY*> sources;
        RECORD_INPUT                           input{ false, false, {} };
        std::set<INST_ID>                      liveInstances;
        std::set<INST_ID>                      changedInstances;
        std::set<RECORD_KEY, RECORD_ORDER>     liveRecords;
        struct RECORD_JOB
        {
            RECORD_KEY      key;
            RECORD_INPUT    input;
            RECORD_GEOMETRY geometry;
            INSTANCE_CLAIMS claims;
        };
        std::vector<RECORD_JOB> jobs;
        const auto eraseSource = [&]( const ITEM_KEY& key )
        {
            if( !claimNamesChanged )
            {
                const auto* source = m_sources.Find( key );
                claimNamesChanged = source && source->value.claim.has_value();
            }

            m_sources.Erase( key );
        };

        for( const FRAME_INSTANCE& frame : aFrame )
        {
            const INST_ID instance = frame.scope.instance;

            if( !liveInstances.insert( instance ).second )
                throw std::invalid_argument( "Duplicate connectivity frame instance" );

            const auto* facts = aInputs.FindScreen( frame.screen );
            const auto* text = aInputs.FindInstance( instance );

            if( !facts || !text )
                throw std::invalid_argument( "Connectivity frame inputs were not captured" );

            auto old = m_prepared.find( instance );

            const bool instanceChanged = old == m_prepared.end() || aliasesChanged
                                         || old->second.sourceVersion != facts->version
                                         || old->second.textVersion != text->version
                                         || old->second.scope != frame.scope;

            if( !instanceChanged )
                continue;

            const auto symbolSource = aInputs.VerifiedSymbolSource( frame.screen );
            const bool reuseClaims = old != m_prepared.end() && symbolSource
                    && old->second.symbolSource == symbolSource && !aliasesChanged
                    && old->second.textVersion == text->version && old->second.scope == frame.scope
                    && facts->value.ruleAreas.empty() && text->value.ruleAreas.empty()
                    && old->second.factCount == facts->value.items.size()
                    && std::ranges::all_of( facts->value.items, [&]( const ITEM_FACT& fact )
                    {
                        if( fact.type != SCH_LINE_T )
                            return true;

                        const auto* source = m_sources.Find( { fact.id, instance } );
                        return source && source->value.type == SCH_LINE_T;
                    } );

            // Geometry still changes island membership even when each source's claims are unchanged
            if( reuseClaims )
            {
                old->second.sourceVersion = facts->version;
            }
            else
            {
                auto prepared = PrepareInstanceClaims( facts->value, text->value, frame.scope, m_keys, m_parses );
                PREPARED_INSTANCE stamp{ facts->version, text->version, frame.scope, symbolSource,
                                         facts->value.items.size(), {} };
                stamp.items.reserve( prepared.items.size() );

                for( auto& [id, source] : prepared.items )
                {
                    if( !claimNamesChanged )
                    {
                        const auto* previous = m_sources.Find( { id, instance } );
                        const auto* oldClaim = previous && previous->value.claim ? &*previous->value.claim : nullptr;
                        claimNamesChanged = source.claim ? !oldClaim || source.claim->name != oldClaim->name
                                                         : oldClaim != nullptr;
                    }

                    m_sources.Set( { id, instance }, std::move( source ) );
                    stamp.items.push_back( id );
                }

                if( old != m_prepared.end() )
                {
                    for( const KIID& id : old->second.items )
                    {
                        if( !std::binary_search( stamp.items.begin(), stamp.items.end(), id ) )
                            eraseSource( { id, instance } );
                    }
                }

                m_prepared.insert_or_assign( instance, std::move( stamp ) );
            }

            changedInstances.insert( instance );
            const auto& islands = aInputs.Islands( frame.screen, text->value.units ).value;

            for( const ISLAND& island : islands.islands )
            {
                const RECORD_KEY key{ instance, island.anchor };

                if( !liveRecords.insert( key ).second )
                    throw std::logic_error( "Duplicate connectivity island anchor" );

                sources.clear();
                sources.reserve( island.items.size() );
                input.hasWire = island.hasWire;
                input.hasBusLine = island.hasBusLine;
                input.items.clear();
                input.items.reserve( island.items.size() );

                for( const KIID& id : island.items )
                {
                    const auto* source = m_sources.Find( { id, instance } );

                    if( !source )
                        throw std::logic_error( "Connectivity island source was not prepared" );

                    sources.push_back( source );
                    input.items.emplace_back( id, source->version );
                }

                const auto previous = m_recordInputs.find( key );

                if( previous != m_recordInputs.end() && previous->second == input )
                    continue;

                INSTANCE_CLAIMS claims;
                claims.instance = instance;

                for( size_t i = 0; i < island.items.size(); ++i )
                    claims.items.emplace( island.items[i], sources[i]->value );

                jobs.push_back( { key, input, { island.items, input.hasWire, input.hasBusLine },
                                  std::move( claims ) } );
            }
        }

        std::vector<ISLAND_RECORD> results( jobs.size() );
        ParallelFor( jobs.size(), [&]( size_t i )
        {
            results[i] = BuildIslandRecord( jobs[i].geometry, jobs[i].claims, m_keys );
        } );

        for( size_t i = 0; i < jobs.size(); ++i )
        {
            m_records.Set( jobs[i].key, std::move( results[i] ) );
            m_recordInputs.insert_or_assign( jobs[i].key, std::move( jobs[i].input ) );
        }

        for( auto it = m_prepared.begin(); it != m_prepared.end(); )
        {
            if( liveInstances.contains( it->first ) )
            {
                ++it;
                continue;
            }

            for( const KIID& id : it->second.items )
                eraseSource( { id, it->first } );

            it = m_prepared.erase( it );
        }

        for( auto it = m_recordInputs.begin(); it != m_recordInputs.end(); )
        {
            if( ( liveInstances.contains( it->first.inst ) && !changedInstances.contains( it->first.inst ) )
                || liveRecords.contains( it->first ) )
            {
                ++it;
                continue;
            }

            m_records.Erase( it->first );
            it = m_recordInputs.erase( it );
        }

        if( claimNamesChanged )
        {
            std::set<wxString> liveText;

            for( const auto& [key, source] : m_sources.Entries() )
            {
                if( source->value.claim )
                    liveText.insert( m_keys.Name( source->value.claim->name ) );
            }

            m_parses.Retain( liveText );
        }
    }
    catch( ... )
    {
        Clear();
        throw;
    }
}

void RECORD_STORE::Clear()
{
    m_sources.Clear();
    m_records.Clear();
    m_prepared.clear();
    m_recordInputs.clear();
    m_parses.Retain( {} );
}
} // namespace SCH_CONNECTIVITY
