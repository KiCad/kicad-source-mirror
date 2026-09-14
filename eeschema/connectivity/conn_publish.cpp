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
#include <iterator>
#include <limits>
#include <ranges>
#include <set>
#include <stdexcept>
#include <wx/thread.h>

namespace SCH_CONNECTIVITY
{
wxString ApplyNameSuffix( const wxString& aName, const BUS_SCHEMA* aSchema, uint32_t aSuffix )
{
    wxString result = aName;

    if( aSuffix )
    {
        const size_t offset = aSchema && aSchema->shape == BUS_SCHEMA::SHAPE::GROUP && !aSchema->prefix.IsEmpty()
                                      ? aSchema->prefixEnd
                                      : result.length();
        result.insert( offset, wxString::Format( "_%u", aSuffix ) );
    }

    return result;
}

namespace
{
    struct PREDECESSOR
    {
        NODE_ID node;
        size_t  weight;
    };

    struct SUCCESSION
    {
        std::vector<NODE_ID> oldNodes;
        std::vector<NODE_ID> newNodes;
    };

    using PREDECESSORS = std::map<NODE_ID, PREDECESSOR>;

    struct NODE_LESS
    {
        const SESSION_KEYS& keys;

        bool operator()( NODE_ID aLeft, NODE_ID aRight ) const
        {
            return keys.Less( keys.Node( aLeft ), keys.Node( aRight ) );
        }
    };

    std::vector<NODE_ID> OrderedNodes( const PUBLICATION::COMPONENTS& aComponents, const SESSION_KEYS& aKeys )
    {
        const auto           nodes = std::views::keys( aComponents );
        std::vector<NODE_ID> result( nodes.begin(), nodes.end() );
        std::ranges::sort( result, NODE_LESS{ aKeys } );
        return result;
    }

    std::vector<RECORD_KEY> SortedRecords( const PARTITION& aPartition, const SESSION_KEYS& aKeys )
    {
        std::vector<RECORD_KEY> result;

        for( const auto& [node, version] : aPartition.identity )
        {
            if( const auto* record = std::get_if<RECORD_NODE>( &aKeys.Node( node ) ) )
                result.push_back( record->record );
        }

        std::ranges::sort( result, KEY_LESS{ aKeys } );
        return result;
    }

    std::vector<SUCCESSION> FindSuccessions( const PUBLICATION::COMPONENTS& aOld, const PUBLICATION::COMPONENTS& aNew,
                                             const std::set<NODE_ID>& aUnchanged, PREDECESSORS& aPredecessors,
                                             const SESSION_KEYS& aKeys )
    {
        std::vector<SUCCESSION> result;

        for( KIND kind : { KIND::BUNDLE, KIND::SIGNAL } )
        {
            std::map<ITEM_KEY, NODE_ID, KEY_LESS>   items( KEY_LESS{ aKeys } );
            std::map<SLOT_KEY, NODE_ID, KEY_LESS>   slots( KEY_LESS{ aKeys } );
            std::map<NODE_ID, std::vector<NODE_ID>> oldEdges;
            std::map<NODE_ID, std::vector<NODE_ID>> newEdges;

            for( const auto& [node, component] : aOld )
            {
                if( component.content->kind != kind || aUnchanged.contains( node ) )
                    continue;

                oldEdges[node];

                for( const ITEM_KEY& item : component.content->items )
                    items.emplace( item, node );

                for( const SLOT_KEY& slot : component.content->slots )
                    slots.emplace( slot, node );
            }

            for( const auto& [node, component] : aNew )
            {
                if( component.content->kind != kind )
                    continue;

                if( aUnchanged.contains( node ) )
                {
                    aPredecessors.emplace( node, PREDECESSOR{ node, component.content->items.size()
                                                                            + component.content->slots.size() } );
                    continue;
                }

                auto&                     edges = newEdges[node];
                std::map<NODE_ID, size_t> weights;

                for( const ITEM_KEY& item : component.content->items )
                {
                    if( const auto found = items.find( item ); found != items.end() )
                        ++weights[found->second];
                }

                for( const SLOT_KEY& slot : component.content->slots )
                {
                    if( const auto found = slots.find( slot ); found != slots.end() )
                        ++weights[found->second];
                }

                for( const auto& [old, weight] : weights )
                {
                    edges.push_back( old );
                    oldEdges[old].push_back( node );
                    auto [found, inserted] = aPredecessors.emplace( node, PREDECESSOR{ old, weight } );

                    if( !inserted
                        && ( weight > found->second.weight
                             || ( weight == found->second.weight && NODE_LESS{ aKeys }( old, found->second.node ) ) ) )
                        found->second = { old, weight };
                }
            }

            std::set<NODE_ID> oldVisited;
            std::set<NODE_ID> newVisited;
            const auto        visit = [&]( bool aOldSide, NODE_ID aNode )
            {
                SUCCESSION                            group;
                std::vector<std::pair<bool, NODE_ID>> pending{ { aOldSide, aNode } };

                while( !pending.empty() )
                {
                    const auto [oldSide, node] = pending.back();
                    pending.pop_back();
                    auto& visited = oldSide ? oldVisited : newVisited;

                    if( !visited.insert( node ).second )
                        continue;

                    ( oldSide ? group.oldNodes : group.newNodes ).push_back( node );

                    for( NODE_ID adjacent : ( oldSide ? oldEdges : newEdges ).at( node ) )
                        pending.emplace_back( !oldSide, adjacent );
                }

                result.push_back( std::move( group ) );
            };

            for( const auto& [node, edges] : oldEdges )
            {
                if( !oldVisited.contains( node ) )
                    visit( true, node );
            }

            for( const auto& [node, edges] : newEdges )
            {
                if( !newVisited.contains( node ) )
                    visit( false, node );
            }
        }

        return result;
    }

    wxString Suffix( uint32_t aSuffix )
    {
        return aSuffix ? wxString::Format( "_%u", aSuffix ) : wxString();
    }

    wxString BundleScope( const CLAIM& aClaim, const SESSION_KEYS& aKeys )
    {
        const wxString& full = aKeys.Name( aClaim.fullName );
        return full.Left( full.length() - aKeys.Name( aClaim.name ).length() );
    }

    wxString RenderName( const PUBLISHED_COMPONENT& aComponent, uint32_t aSuffix, const SESSION_KEYS& aKeys )
    {
        if( aComponent.content->kind == KIND::BUNDLE
            && aComponent.content->best->schema->shape == BUS_SCHEMA::SHAPE::GROUP
            && !aComponent.content->best->schema->prefix.IsEmpty() )
        {
            const CLAIM& claim = *aComponent.content->best;
            return BundleScope( claim, aKeys )
                   + ApplyNameSuffix( aKeys.Name( claim.name ), claim.schema.get(), aSuffix );
        }

        return aKeys.Name( aComponent.baseName ) + Suffix( aSuffix );
    }

    void AssignNames( KIND aKind, PUBLICATION::COMPONENTS& aCurrent, const PUBLICATION::COMPONENTS& aOld,
                      const PREDECESSORS& aPredecessors, const std::vector<NODE_ID>& aOrder, SESSION_KEYS& aKeys )
    {
        std::map<NAME_ID, std::vector<NODE_ID>> buckets;
        std::set<NAME_ID>                       occupied;

        for( NODE_ID node : aOrder )
        {
            const auto& component = aCurrent.at( node );

            if( component.content->kind == aKind && component.content->best )
                buckets[component.collisionBase].push_back( node );
            else if( component.name != INVALID_ID )
                occupied.insert( component.name );
        }

        const auto previous = [&]( NODE_ID node ) -> const PUBLISHED_COMPONENT*
        {
            const auto found = aPredecessors.find( node );
            return found == aPredecessors.end() ? nullptr : &aOld.at( found->second.node );
        };
        const auto rendered = [&]( NODE_ID node, uint32_t suffix )
        {
            const auto& component = aCurrent.at( node );
            const auto* old = previous( node );

            if( old && old->baseName == component.baseName && old->collisionBase == component.collisionBase
                && old->suffix == suffix )
            {
                if( aKind == KIND::SIGNAL
                    || ( old->content->best->name == component.content->best->name
                         && old->content->best->schema->shape == component.content->best->schema->shape
                         && old->content->best->schema->prefix == component.content->best->schema->prefix ) )
                    return old->name;
            }

            return aKeys.InternName( RenderName( component, suffix, aKeys ) );
        };
        const CLAIM_LESS     less{ aKeys };
        const NODE_LESS      nodeLess{ aKeys };
        std::vector<NODE_ID> remaining;

        for( const auto& [base, members] : buckets )
        {
            const auto preferred = [&]( NODE_ID a, NODE_ID b )
            {
                const CLAIM& left = *aCurrent.at( a ).content->best;
                const CLAIM& right = *aCurrent.at( b ).content->best;

                if( left.Strong() != right.Strong() )
                    return left.Strong();

                if( left.Strong() )
                {
                    if( less( right, left ) )
                        return true;

                    if( less( left, right ) )
                        return false;
                }
                else
                {
                    const auto weight = [&]( NODE_ID node ) -> size_t
                    {
                        const auto* old = previous( node );
                        return old && old->collisionBase == base && old->suffix == 0 ? aPredecessors.at( node ).weight
                                                                                     : 0;
                    };

                    if( weight( a ) != weight( b ) )
                        return weight( a ) > weight( b );
                }

                return nodeLess( a, b );
            };

            const NODE_ID holder = *std::ranges::min_element( members, preferred );
            auto&         component = aCurrent.at( holder );
            const NAME_ID name = rendered( holder, 0 );

            if( !occupied.insert( name ).second )
                throw std::invalid_argument( "Distinct collision bases render name: "
                                             + std::string( aKeys.Name( name ).utf8_str() ) );

            component.name = name;
            std::ranges::remove_copy( members, std::back_inserter( remaining ), holder );
        }

        std::ranges::sort( remaining, nodeLess );
        const auto assign = [&]( NODE_ID node, uint32_t suffix )
        {
            auto&         component = aCurrent.at( node );
            const NAME_ID name = rendered( node, suffix );

            if( !occupied.insert( name ).second )
                return false;

            component.suffix = suffix;
            component.name = name;
            return true;
        };

        for( NODE_ID node : remaining )
        {
            if( const auto* old = previous( node );
                old && old->suffix && old->collisionBase == aCurrent.at( node ).collisionBase )
                assign( node, old->suffix );
        }

        std::map<NAME_ID, uint32_t> nextSuffix;

        for( NODE_ID node : remaining )
        {
            if( aCurrent.at( node ).name != INVALID_ID )
                continue;

            // Different bus expressions can share a collision bucket but have independent rendered suffixes
            uint32_t& suffix = nextSuffix.try_emplace( aCurrent.at( node ).baseName, 1 ).first->second;

            while( !assign( node, suffix ) )
            {
                if( suffix == std::numeric_limits<uint32_t>::max() )
                    throw std::overflow_error( "Connectivity suffix space exhausted" );

                ++suffix;
            }
        }
    }

    CHANGE_SET DescribeChanges( const std::vector<SUCCESSION>& aGroups, const std::set<NODE_ID>& aUnchanged,
                                const PUBLICATION::COMPONENTS& aOld, const PUBLICATION::COMPONENTS& aNew,
                                const SESSION_KEYS& aKeys )
    {
        CHANGE_SET      result;
        const NAME_LESS nameLess{ &aKeys };
        const auto      names = [&]( const auto& nodes, const auto& components )
        {
            std::vector<NAME_ID> values;

            for( NODE_ID node : nodes )
            {
                if( const NAME_ID name = components.at( node ).name; name != INVALID_ID )
                    values.push_back( name );
            }

            std::ranges::sort( values, nameLess );
            return values;
        };

        const auto changedNames = [&]( const auto& nodes, const auto& before, const auto& after )
        {
            for( NODE_ID node : nodes )
            {
                const auto& previous = before.at( node );
                const auto next = after.find( node );

                if( previous.name != INVALID_ID && ( next == after.end() || previous != next->second ) )
                    result.netsChanged.push_back( previous.name );
            }
        };

        for( const SUCCESSION& group : aGroups )
        {
            auto old = names( group.oldNodes, aOld );
            auto current = names( group.newNodes, aNew );
            changedNames( group.oldNodes, aOld, aNew );
            changedNames( group.newNodes, aNew, aOld );

            if( old.size() == 1 && current.size() == 1 )
            {
                if( old.front() != current.front() )
                    result.renamedNets.emplace_back( old.front(), current.front() );
            }
            else if( old.size() > 1 && current.size() == 1 )
            {
                result.mergedNets.emplace_back( std::move( old ), current.front() );
            }
            else if( old.size() == 1 && current.size() > 1 )
            {
                result.splitNets.emplace_back( old.front(), std::move( current ) );
            }
            else
            {
                result.netsRemoved.insert( result.netsRemoved.end(), old.begin(), old.end() );
                result.netsAdded.insert( result.netsAdded.end(), current.begin(), current.end() );
            }
        }

        for( NODE_ID node : aUnchanged )
        {
            const NAME_ID old = aOld.at( node ).name;
            const NAME_ID current = aNew.at( node ).name;

            if( old != current )
            {
                result.renamedNets.emplace_back( old, current );
                result.netsChanged.push_back( old );
                result.netsChanged.push_back( current );
            }
        }

        std::ranges::sort( result.netsAdded, nameLess );
        std::ranges::sort( result.netsRemoved, nameLess );
        std::ranges::sort( result.renamedNets, nameLess, &std::pair<NAME_ID, NAME_ID>::first );
        std::ranges::sort( result.mergedNets, nameLess, &std::pair<std::vector<NAME_ID>, NAME_ID>::second );
        std::ranges::sort( result.splitNets, nameLess, &std::pair<NAME_ID, std::vector<NAME_ID>>::first );
        return result;
    }
} // namespace

void PUBLICATION::Update( const COMPONENT_CACHE<BUNDLE_BINDING>& aBundles,
                          const COMPONENT_CACHE<SIGNAL_RESULT>& aSignals,
                          const RECORD_STORE::RECORD_CACHE& aRecords, std::span<const FRAME_INSTANCE> aFrame,
                          const INPUT_STORE& aInputs )
{
    wxASSERT( wxThread::IsMain() );
    COMPONENTS                  current;
    std::map<NODE_ID, uint64_t> versions;
    std::set<NODE_ID>           unchanged;
    SLOT_INPUTS                 slots( KEY_LESS{ m_keys } );
    const auto                  retained = [&]( NODE_ID node, uint64_t version )
    {
        versions.emplace( node, version );
        const auto found = m_versions.find( node );

        if( found != m_versions.end() && found->second == version )
        {
            unchanged.insert( node );
            return m_components.at( node ).content;
        }

        return std::shared_ptr<const COMPONENT_CONTENT>();
    };

    for( const auto& entry : aBundles.Entries() )
    {
        auto& component = current[entry->input.anchor];
        component.content = retained( entry->input.anchor, entry->version );

        if( !component.content )
        {
            auto content = std::make_shared<COMPONENT_CONTENT>();
            content->kind = KIND::BUNDLE;
            content->best = entry->value.canonical;
            content->items = entry->value.items;
            content->records = SortedRecords( entry->input, m_keys );
            content->netclasses = entry->value.netclasses;
            content->members = entry->value.members;
            std::ranges::transform( entry->value.slots, std::back_inserter( content->slots ), &SLOT_INPUT::key );

            if( content->best )
                content->baseName = content->best->fullName;

            component.content = std::move( content );
        }

        for( const SLOT_INPUT& slot : entry->value.slots )
            slots.emplace( slot.key, &slot );

        component.baseName = component.content->baseName;

        if( component.content->best )
        {
            const CLAIM& claim = *component.content->best;

            if( !claim.schema )
                throw std::invalid_argument( "Driven bundle publication requires a schema" );

            // Vectors sharing a prefix collide across ranges, but groups collide only on the whole name
            component.collisionBase =
                    claim.schema->shape == BUS_SCHEMA::SHAPE::VECTOR
                            ? m_keys.InternName( BundleScope( claim, m_keys ) + claim.schema->prefix + "[]" )
                            : claim.fullName;
        }
    }

    for( const auto& entry : aSignals.Entries() )
    {
        auto& component = current[entry->input.anchor];
        component.content = retained( entry->input.anchor, entry->version );

        if( !component.content )
        {
            auto content = std::make_shared<COMPONENT_CONTENT>();
            content->best = entry->value.summary.best;
            content->items = entry->value.items;
            content->slots = entry->value.slots;
            content->nameSlot = entry->value.nameSlot;
            content->netclasses = entry->value.summary.netclasses;
            content->baseName = entry->value.baseName;
            content->records = SortedRecords( entry->input, m_keys );
            component.content = std::move( content );
        }

        component.baseName = component.content->baseName;
    }

    PREDECESSORS predecessors;
    const auto   groups = FindSuccessions( m_components, current, unchanged, predecessors, m_keys );
    const auto   order = OrderedNodes( current, m_keys );
    AssignNames( KIND::BUNDLE, current, m_components, predecessors, order, m_keys );

    for( auto& [node, component] : current )
    {
        if( component.content->kind != KIND::SIGNAL )
            continue;

        if( component.content->nameSlot )
        {
            const SLOT_INPUT* naming = slots.at( *component.content->nameSlot );
            const CLAIM&      best = *component.content->best;
            const auto        memberSuffix = [&]( const SLOT_INPUT& slot )
            {
                const auto& parent = current.at( slot.parentBundle );
                const auto& schema = *parent.content->best->schema;
                return schema.shape == BUS_SCHEMA::SHAPE::GROUP && !schema.prefix.IsEmpty() ? parent.suffix : 0;
            };

            // Driver ties precede publication, so they cannot see which bundle retained the base
            for( const SLOT_KEY& key : component.content->slots )
            {
                const SLOT_INPUT* candidate = slots.at( key );
                CLAIM             comparable = candidate->claim;
                comparable.source = best.source;

                if( comparable != best )
                    continue;

                if( memberSuffix( *candidate ) < memberSuffix( *naming )
                    || ( memberSuffix( *candidate ) == memberSuffix( *naming )
                         && m_keys.Less( candidate->key, naming->key ) ) )
                    naming = candidate;
            }

            component.nameSlot = naming->key;
            const auto&  parent = current.at( naming->parentBundle );
            const CLAIM& claim = *parent.content->best;

            if( claim.schema->shape == BUS_SCHEMA::SHAPE::GROUP )
            {
                const auto old = m_components.find( node );
                const auto oldParent = m_components.find( naming->parentBundle );

                if( old != m_components.end() && oldParent != m_components.end()
                    && old->second.content == component.content && old->second.nameSlot == component.nameSlot
                    && oldParent->second.content == parent.content && oldParent->second.suffix == parent.suffix )
                {
                    component.baseName = old->second.baseName;
                    component.collisionBase = component.baseName;
                    continue;
                }

                wxString prefix = claim.schema->prefix;

                if( !prefix.IsEmpty() )
                    prefix += Suffix( parent.suffix ) + ".";

                // An extra member slot keeps the sheet path of its own declaration, not the canonical one
                component.baseName = m_keys.InternName( m_keys.Name( naming->claim.path ) + prefix
                                                        + m_keys.Name( naming->localName ) );
            }
        }

        component.collisionBase = component.baseName;
    }

    AssignNames( KIND::SIGNAL, current, m_components, predecessors, order, m_keys );
    std::map<NODE_ID, NODE_ID> continuations;

    for( NODE_ID node : order )
    {
        const auto pred = predecessors.find( node );

        if( pred == predecessors.end() || !current.at( node ).content->best )
            continue;

        auto [found, inserted] = continuations.emplace( pred->second.node, node );

        if( !inserted && pred->second.weight > predecessors.at( found->second ).weight )
            found->second = node;
    }

    int      nextNetCode = m_nextNetCode;
    uint32_t nextSubgraphCode = m_nextSubgraphCode;

    for( NODE_ID node : order )
    {
        auto& component = current.at( node );

        if( !component.content->best )
            continue;

        const auto pred = predecessors.find( node );

        if( pred != predecessors.end() && continuations.at( pred->second.node ) == node )
        {
            const auto& old = m_components.at( pred->second.node );
            component.netCode = old.netCode;
            component.subgraphCode = old.subgraphCode;
        }

        if( !component.subgraphCode )
        {
            if( nextSubgraphCode == std::numeric_limits<uint32_t>::max() )
                throw std::overflow_error( "Connectivity subgraph code space exhausted" );

            component.subgraphCode = nextSubgraphCode++;
        }

        if( component.content->kind == KIND::SIGNAL && !component.netCode )
        {
            if( nextNetCode == std::numeric_limits<int>::max() )
                throw std::overflow_error( "Connectivity net code space exhausted" );

            component.netCode = nextNetCode++;
        }
    }

    CHANGE_SET changes = DescribeChanges( groups, unchanged, m_components, current, m_keys );
    std::set<NAME_ID, NAME_LESS> changedNames( NAME_LESS{ &m_keys } );
    const auto collectDependents = [&]( const COMPONENTS& components )
    {
        std::vector<NODE_ID> pending;
        std::set<NODE_ID> visited;
        const auto add = [&]( NODE_ID node )
        {
            if( visited.insert( node ).second )
                pending.push_back( node );
        };

        for( NAME_ID name : changes.netsChanged )
        {
            if( name == INVALID_ID )
                continue;

            changedNames.insert( name );

            if( const auto found = m_byName.find( name ); found != m_byName.end() )
                add( found->second );
        }

        for( size_t i = 0; i < pending.size(); ++i )
        {
            const NODE_ID node = pending[i];
            const NAME_ID name = components.at( node ).name;

            if( name != INVALID_ID )
                changedNames.insert( name );

            for( NODE_ID member : MembersOf( node ) )
                add( member );

            if( const auto signature = m_busSignatures.find( node ); signature != m_busSignatures.end() )
            {
                for( NODE_ID equivalent : signature->second->second )
                    add( equivalent );
            }
        }
    };

    ROW_UPDATE rows = PrepareRows( current, aRecords, slots, changes );
    m_auxiliary.Update( aFrame, aInputs, aRecords, changes );

    for( const ITEM_KEY& item : changes.changedItems )
    {
        if( const auto old = m_rows.find( item ); old != m_rows.end() )
            changes.netsChanged.push_back( old->second.name );

        if( const auto replacement = rows.upserts.find( item ); replacement != rows.upserts.end() )
            changes.netsChanged.push_back( replacement->second.name );
    }

    // Removed bus memberships still invalidate the member's containing-bus navigation
    collectDependents( m_components );
    ApplyRows( std::move( rows ) );
    UpdateIndexes( current, slots );
    collectDependents( current );
    changes.netsChanged.assign( changedNames.begin(), changedNames.end() );
    m_components = std::move( current );
    m_versions = std::move( versions );
    m_changes = std::move( changes );
    m_nextNetCode = nextNetCode;
    m_nextSubgraphCode = nextSubgraphCode;
}

void PUBLICATION::Clear()
{
    m_auxiliary.Clear();
    m_byName.clear();
    m_slotComponents.clear();
    m_busMembers.clear();
    m_busParents.clear();
    m_busSignatures.clear();
    m_equivalentBuses.clear();
    m_rows.clear();
    m_rowInputs.clear();
    m_netclasses.clear();
    m_components.clear();
    m_versions.clear();
    m_changes = {};
}
} // namespace SCH_CONNECTIVITY
