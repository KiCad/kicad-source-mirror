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

#include "conn_bindings.h"

#include <algorithm>
#include <map>
#include <stdexcept>
#include <set>
#include <utility>
#include <wx/thread.h>

namespace SCH_CONNECTIVITY
{
BUNDLE_INPUT PrepareBundle( const PARTITION& aPartition, const RECORD_STORE::RECORD_CACHE& aRecords,
                            SESSION_KEYS& aKeys )
{
    wxASSERT( wxThread::IsMain() );
    BUNDLE_INPUT result;
    result.parent = aPartition.anchor;

    for( const auto& [node, version] : aPartition.identity )
    {
        const NODE_KEY& nodeKey = aKeys.Node( node );

        if( std::holds_alternative<NAME_KEY>( nodeKey ) && version == 0 )
            continue;

        const auto* key = std::get_if<RECORD_NODE>( &nodeKey );
        const auto* entry = key ? aRecords.Find( key->record ) : nullptr;

        if( !entry || entry->version != version || key->kind != KIND::BUNDLE || entry->value.kind != KIND::BUNDLE )
            throw std::invalid_argument( "Bundle partition does not match current records" );

        const ISLAND_RECORD& record = entry->value;

        for( const KIID& item : record.items )
            result.items.push_back( { item, key->record.inst } );

        result.netclasses.insert( result.netclasses.end(), record.netclasses.begin(), record.netclasses.end() );

        for( const CLAIM& claim : record.claims )
        {
            if( claim.schema )
                result.claims.push_back( { key->record, claim, {}, &claim == &record.claims.front() } );
        }
    }

    std::ranges::sort( result.items, KEY_LESS{ aKeys } );
    std::ranges::sort( result.netclasses, NAME_LESS{ &aKeys } );
    result.netclasses.erase( std::ranges::unique( result.netclasses ).begin(), result.netclasses.end() );

    if( result.claims.empty() )
        return result;

    std::swap( result.claims.front(),
               *std::ranges::max_element( result.claims, CLAIM_LESS{ aKeys }, &BUNDLE_CLAIM::claim ) );

    for( BUNDLE_CLAIM& source : result.claims )
    {
        const auto&    leaves = source.claim.schema->leaves;
        const wxString path = aKeys.Name( source.claim.path );

        if( !leaves.empty() && !std::in_range<uint32_t>( leaves.size() - 1 ) )
            throw std::length_error( "Bus schema exceeds slot ordinal range" );

        source.leaves.reserve( leaves.size() );

        for( const BUS_SCHEMA::LEAF& leaf : leaves )
        {
            wxString local;

            for( const wxString& group : leaf.groupPath )
                local += group + ".";

            local += leaf.localName;
            source.leaves.push_back( { aKeys.InternName( leaf.name ), aKeys.InternName( path + leaf.name ),
                                       aKeys.InternName( local ) } );
        }
    }

    return result;
}

BUNDLE_BINDING BindBundle( const BUNDLE_INPUT& aInput, const SESSION_KEYS& aKeys )
{
    BUNDLE_BINDING result;
    result.items = aInput.items;
    result.netclasses = aInput.netclasses;

    if( aInput.claims.empty() )
        return result;

    const BUNDLE_CLAIM& canonical = aInput.claims.front();

    if( aInput.parent == INVALID_ID || !canonical.claim.schema )
        throw std::invalid_argument( "Bundle binding requires a parent and canonical schema" );

    result.canonical = canonical.claim;
    const auto makeSlot = [&]( const SLOT_KEY& key, const BUNDLE_CLAIM& owner, size_t aLeaf )
    {
        const BUNDLE_CLAIM::LEAF_NAME& leaf = owner.leaves.at( aLeaf );
        SLOT_INPUT                     slot;
        slot.key = key;
        slot.claim.priority = PRIORITY::BUS_MEMBER;
        slot.claim.depth = owner.claim.depth;
        slot.claim.path = owner.claim.path;
        slot.claim.name = slot.claim.ncName = leaf.name;
        slot.claim.fullName = leaf.fullName;
        slot.claim.source = owner.claim.source;
        slot.localName = leaf.localName;
        slot.parentNetclasses = aInput.netclasses;
        slot.parentBundle = aInput.parent;
        return slot;
    };

    for( size_t i = 0; i < canonical.leaves.size(); ++i )
    {
        const SLOT_KEY key{ canonical.claim.source, static_cast<uint32_t>( i ) };
        result.members.push_back( key );
        result.slots.push_back( makeSlot( key, canonical, i ) );
    }

    const CLAIM_LESS                                           claimLess{ aKeys };
    std::map<std::pair<INST_ID, NAME_ID>, const BUNDLE_CLAIM*> answers;

    for( const BUNDLE_CLAIM& source : aInput.claims )
    {
        if( source.claim.priority != PRIORITY::HIER_LABEL )
            continue;

        const auto [found, inserted] = answers.try_emplace( { source.record.inst, source.claim.name }, &source );

        if( !inserted && claimLess( found->second->claim, source.claim ) )
            found->second = &source;
    }

    for( const BUNDLE_CLAIM& source : aInput.claims )
    {
        if( !source.claim.schema || source.leaves.size() != source.claim.schema->leaves.size() )
            throw std::invalid_argument( "Prepared bundle leaves do not match their schema" );

        const bool          sheetPin = source.claim.priority == PRIORITY::SHEET_PIN;
        const BUNDLE_CLAIM* answer = nullptr;

        if( sheetPin && source.claim.portInstance )
        {
            if( auto found = answers.find( { *source.claim.portInstance, source.claim.name } ); found != answers.end() )
                answer = found->second;
        }

        // A sheet pin names parent members only while it drives its bus there, and answering labels cover children
        const bool named = !sheetPin || source.drivesRecord;

        // Such a pin would publish member nets that hold no item on either sheet
        if( !named && !answer )
            continue;

        const BUS_ALIGNMENT alignment = Align( *source.claim.schema, *canonical.claim.schema );
        const auto parentEdges = [&]( SLOT_INPUT& aSlot ) -> std::vector<NAME_KEY>&
        {
            return sheetPin ? aSlot.parentEdges : aSlot.edges;
        };

        if( named )
        {
            for( const auto& [from, to] : alignment.matched )
            {
                parentEdges( result.slots[to] )
                        .push_back( { SCOPE::SHEET, source.record.inst, source.leaves[from].name } );
            }
        }

        for( size_t from : alignment.unmappedLeft )
        {
            // An answered pin member takes the label's claim so that both slots name the child member alike
            const auto ordinal = static_cast<uint32_t>( from );
            SLOT_INPUT slot = makeSlot( { source.claim.source, ordinal }, answer ? *answer : source, from );

            if( named )
                parentEdges( slot ).push_back( { SCOPE::SHEET, source.record.inst, source.leaves[from].name } );

            if( answer )
                slot.edges.push_back( { SCOPE::SHEET, answer->record.inst, source.leaves[from].name } );

            result.slots.push_back( std::move( slot ) );
        }
    }

    const KEY_LESS keyLess{ aKeys };

    for( SLOT_INPUT& slot : result.slots )
    {
        for( std::vector<NAME_KEY>* edges : { &slot.edges, &slot.parentEdges } )
        {
            std::ranges::sort( *edges, keyLess );
            edges->erase( std::ranges::unique( *edges ).begin(), edges->end() );
        }
    }

    std::ranges::sort( result.slots, keyLess, &SLOT_INPUT::key );
    return result;
}

SLOT_STORE::SLOT_STORE( CACHE_VERSIONS& aVersions, SESSION_KEYS& aKeys ) :
        m_slots( aVersions, KEY_LESS{ aKeys } )
{
}

void SLOT_STORE::Update( const COMPONENT_CACHE<BUNDLE_BINDING>& aBundles )
{
    wxASSERT( wxThread::IsMain() );
    try
    {
        std::set<SLOT_KEY, KEY_LESS> live( m_slots.Entries().key_comp() );
        std::set<NODE_ID>            parents;

        for( const auto& bundle : aBundles.Entries() )
        {
            const NODE_ID parent = bundle->input.anchor;

            if( !parents.insert( parent ).second )
                throw std::invalid_argument( "Duplicate bundle component" );

            const auto [previous, inserted] = m_bundleInputs.try_emplace( parent, bundle->version );
            const bool changed = inserted || std::exchange( previous->second, bundle->version ) != bundle->version;

            for( const SLOT_INPUT& slot : bundle->value.slots )
            {
                if( slot.parentBundle != parent || !live.insert( slot.key ).second )
                    throw std::invalid_argument( "Slot has a duplicate key or inconsistent parent" );

                if( changed )
                    m_slots.Set( slot.key, slot );
            }
        }

        for( auto it = m_slots.Entries().begin(); it != m_slots.Entries().end(); )
        {
            // Erasing by a key stored in the erased node is not guaranteed safe
            const SLOT_KEY key = ( it++ )->first;

            if( !live.contains( key ) )
                m_slots.Erase( key );
        }

        std::erase_if( m_bundleInputs,
                       [&]( const auto& entry )
                       {
                           return !parents.contains( entry.first );
                       } );
    }
    catch( ... )
    {
        Clear();
        throw;
    }
}

void SLOT_STORE::Clear()
{
    m_slots.Clear();
    m_bundleInputs.clear();
}

std::vector<NODE_INPUT> SignalNodes( const RECORD_STORE::RECORD_CACHE& aRecords, const SLOT_STORE::SLOT_CACHE& aSlots,
                                     SESSION_KEYS& aKeys )
{
    auto result = RecordNodes( aRecords, KIND::SIGNAL, aKeys );
    result.reserve( result.size() + aSlots.Entries().size() );
    std::set<NODE_ID> claimedNames;

    for( const NODE_INPUT& record : result )
        claimedNames.insert( record.edges.begin(), record.edges.end() );

    for( const auto& [key, entry] : aSlots.Entries() )
    {
        NODE_INPUT input{ aKeys.InternNode( key ), entry->version, {} };
        input.edges.reserve( entry->value.edges.size() );

        for( const NAME_KEY& edge : entry->value.edges )
            input.edges.push_back( aKeys.InternNode( edge ) );

        // Two buses must not join through a parent name that no signal on that sheet claims
        for( const NAME_KEY& edge : entry->value.parentEdges )
        {
            if( const NODE_ID node = aKeys.InternNode( edge ); claimedNames.contains( node ) )
                input.edges.push_back( node );
        }

        result.push_back( std::move( input ) );
    }

    return result;
}

} // namespace SCH_CONNECTIVITY
