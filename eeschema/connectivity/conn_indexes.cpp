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
#include <algorithm>
#include <stdexcept>

namespace SCH_CONNECTIVITY
{
std::optional<NODE_ID> PUBLICATION::FindByName( const wxString& aName ) const
{
    const auto name = m_keys.FindName( aName );

    if( !name )
        return std::nullopt;

    const auto found = m_byName.find( *name );
    return found == m_byName.end() ? std::nullopt : std::optional<NODE_ID>( found->second );
}

std::span<const NODE_ID> PUBLICATION::MembersOf( NODE_ID aBundle ) const
{
    const auto found = m_busMembers.find( aBundle );
    return found == m_busMembers.end() ? std::span<const NODE_ID>() : found->second;
}

std::span<const NODE_ID> PUBLICATION::ParentsOf( NODE_ID aSignal ) const
{
    const auto found = m_busParents.find( aSignal );
    return found == m_busParents.end() ? std::span<const NODE_ID>() : found->second;
}

std::vector<wxString> PUBLICATION::EquivalentBusNames( const wxString& aName ) const
{
    const auto node = FindByName( aName );

    if( !node )
        return {};

    const auto signature = m_busSignatures.find( *node );

    if( signature == m_busSignatures.end() )
        return {};

    std::vector<NAME_ID> names;

    for( NODE_ID other : signature->second->second )
    {
        if( other != *node )
            names.push_back( m_components.at( other ).name );
    }

    std::ranges::sort( names, NAME_LESS{ &m_keys } );
    std::vector<wxString> result;
    result.reserve( names.size() );

    for( NAME_ID name : names )
        result.push_back( m_keys.Name( name ) );

    return result;
}

void PUBLICATION::UpdateBusSignatures( const COMPONENTS& aCurrent, const SLOT_INPUTS& aSlots )
{
    for( auto it = m_busSignatures.begin(); it != m_busSignatures.end(); )
    {
        const auto current = aCurrent.find( it->first );

        if( current != aCurrent.end() && current->second.content == m_components.at( it->first ).content )
        {
            ++it;
            continue;
        }

        const auto group = it->second;
        group->second.erase( it->first );

        if( group->second.empty() )
            m_equivalentBuses.erase( group );

        it = m_busSignatures.erase( it );
    }

    for( const auto& [node, component] : aCurrent )
    {
        if( component.content->kind != KIND::BUNDLE || m_busSignatures.contains( node ) )
            continue;

        const auto& best = component.content->best;

        if( !best || !best->schema || best->schema->shape != BUS_SCHEMA::SHAPE::GROUP
            || !best->schema->prefix.IsEmpty() )
            continue;

        BUS_SIGNATURE signature{ best->path, {} };
        signature.second.reserve( component.content->members.size() );

        for( const SLOT_KEY& slot : component.content->members )
            signature.second.push_back( aSlots.at( slot )->claim.name );

        // Private lookup identity only; query results use canonical published-name ordering
        std::sort( signature.second.begin(), signature.second.end() );
        const auto group = m_equivalentBuses.try_emplace( std::move( signature ) ).first;
        group->second.insert( node );
        m_busSignatures.emplace( node, group );
    }
}

void PUBLICATION::UpdateIndexes( const COMPONENTS& aCurrent, const SLOT_INPUTS& aSlots )
{
    const auto nodeLess = [&]( NODE_ID a, NODE_ID b )
    {
        return m_keys.Less( m_keys.Node( a ), m_keys.Node( b ) );
    };
    const auto setParents = [&]( NODE_ID node, std::vector<NODE_ID> parents )
    {
        const auto before = ParentsOf( node );

        if( std::ranges::equal( before, parents ) )
            return;

        for( NODE_ID parent : before )
        {
            if( !std::binary_search( parents.begin(), parents.end(), parent, nodeLess ) )
            {
                auto found = m_busMembers.find( parent );

                if( found == m_busMembers.end() )
                    throw std::invalid_argument( "Published bundle membership is missing its reverse entry" );

                std::erase( found->second, node );

                if( found->second.empty() )
                    m_busMembers.erase( found );
            }
        }

        for( NODE_ID parent : parents )
        {
            if( !std::binary_search( before.begin(), before.end(), parent, nodeLess ) )
            {
                auto& members = m_busMembers[parent];
                members.insert( std::lower_bound( members.begin(), members.end(), node, nodeLess ), node );
            }
        }

        if( parents.empty() )
            m_busParents.erase( node );
        else
            m_busParents.insert_or_assign( node, std::move( parents ) );
    };

    // Remove old keys first so names and slots can transfer between surviving components
    for( const auto& [node, component] : m_components )
    {
        const auto current = aCurrent.find( node );

        if( component.name != INVALID_ID && ( current == aCurrent.end() || current->second.name != component.name ) )
            m_byName.erase( component.name );

        if( component.content->kind != KIND::SIGNAL )
            continue;

        if( current == aCurrent.end()
            || ( component.content != current->second.content
                 && component.content->slots != current->second.content->slots ) )
        {
            for( const SLOT_KEY& slot : component.content->slots )
                m_slotComponents.erase( slot );
        }

        if( current == aCurrent.end() )
            setParents( node, {} );
    }

    for( const auto& [node, component] : aCurrent )
    {
        const auto old = m_components.find( node );

        if( component.name != INVALID_ID && ( old == m_components.end() || old->second.name != component.name ) )
        {
            const auto [found, inserted] = m_byName.emplace( component.name, node );

            if( !inserted && found->second != node )
                throw std::invalid_argument( "Published name belongs to multiple components: "
                                             + m_keys.Name( component.name ).utf8_string() );
        }

        if( component.content->kind != KIND::SIGNAL
            || ( old != m_components.end() && old->second.content == component.content ) )
            continue;

        const bool slotsChanged = old == m_components.end() || old->second.content->slots != component.content->slots;
        std::vector<NODE_ID> parents;
        parents.reserve( component.content->slots.size() );

        for( const SLOT_KEY& slot : component.content->slots )
        {
            if( slotsChanged )
            {
                const auto [found, inserted] = m_slotComponents.emplace( slot, node );

                if( !inserted && found->second != node )
                    throw std::invalid_argument( "Published slot belongs to multiple signal components" );
            }

            const NODE_ID parent = aSlots.at( slot )->parentBundle;
            const auto    bundle = aCurrent.find( parent );

            if( bundle == aCurrent.end() || bundle->second.content->kind != KIND::BUNDLE )
                throw std::invalid_argument( "Published slot has no current bundle parent" );

            parents.push_back( parent );
        }

        std::sort( parents.begin(), parents.end(), nodeLess );
        parents.erase( std::unique( parents.begin(), parents.end() ), parents.end() );
        setParents( node, std::move( parents ) );
    }

    UpdateBusSignatures( aCurrent, aSlots );
}
} // namespace SCH_CONNECTIVITY
