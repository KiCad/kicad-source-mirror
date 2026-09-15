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

#include "conn_engine.h"
#include <wx/thread.h>
#include <algorithm>
#include <iterator>
#include <ranges>
#include <stdexcept>
#include <string_utils.h>

namespace SCH_CONNECTIVITY
{
namespace
{
struct NC_SOURCE
{
    KICAD_T  type = TYPE_NOT_INIT;
    VECTOR2I position;
    bool     pin = false;
    bool     nc = false;
    bool     powerFlag = false;
};

bool isLabel( KICAD_T aType )
{
    return aType == SCH_LABEL_T || aType == SCH_GLOBAL_LABEL_T || aType == SCH_HIER_LABEL_T;
}

bool isLabelOrSheetPin( KICAD_T aType )
{
    return isLabel( aType ) || aType == SCH_SHEET_PIN_T;
}

// Screen item facts are sorted by id
const ITEM_FACT* findFact( const std::vector<ITEM_FACT>& aFacts, const KIID& aId )
{
    const auto it = std::ranges::lower_bound( aFacts, aId, []( const KIID& a, const KIID& b ) { return a < b; },
                                              &ITEM_FACT::id );
    return it != aFacts.end() && it->id == aId ? &*it : nullptr;
}

template <typename MAP, typename BUILD>
const typename MAP::mapped_type& cached( MAP& aCache, const typename MAP::key_type& aKey, BUILD&& aBuild )
{
    if( const auto it = aCache.find( aKey ); it != aCache.end() )
        return it->second;

    return aCache.emplace( aKey, aBuild() ).first->second;
}

template <typename MAP>
std::vector<typename MAP::mapped_type> moveValues( MAP& aMap )
{
    std::vector<typename MAP::mapped_type> result;
    result.reserve( aMap.size() );

    for( auto& [key, value] : aMap )
        result.push_back( std::move( value ) );

    return result;
}

std::vector<INST_ID> islandInstances( const AUXILIARY::ISLANDS& aIslands )
{
    std::set<INST_ID> seen;
    std::vector<INST_ID> result;

    for( const auto& [key, island] : aIslands )
    {
        if( seen.insert( key.inst ).second )
            result.push_back( key.inst );
    }

    return result;
}

std::map<KIID, VECTOR2I> itemPositions( const SCREEN_FACTS& aFacts )
{
    std::map<KIID, VECTOR2I> result;

    for( const ITEM_FACT& fact : aFacts.items )
    {
        if( !fact.ports.empty() )
            result.emplace( fact.id, fact.ports.front().position );

        for( const PIN_FACT& pin : fact.pins )
            result.emplace( pin.id, pin.position );
    }

    return result;
}

// Pins are inserted before their group fact, which shares the smallest pin id
std::map<KIID, NC_SOURCE> noConnectSources( const SCREEN_FACTS& aFacts )
{
    std::map<KIID, NC_SOURCE> result;

    for( const ITEM_FACT& fact : aFacts.items )
    {
        for( const PIN_FACT& pin : fact.pins )
        {
            result.emplace( pin.id, NC_SOURCE{ SCH_PIN_T, pin.position, true, pin.type == ELECTRICAL_PINTYPE::PT_NC,
                                               ( pin.globalPowerParent || pin.localPowerParent )
                                                       && pin.type == ELECTRICAL_PINTYPE::PT_POWER_OUT } );
        }

        if( !fact.ports.empty() )
            result.emplace( fact.id, NC_SOURCE{ fact.type, fact.ports.front().position } );
    }

    return result;
}

std::set<std::pair<NODE_ID, INST_ID>> busNoConnectMembers( const PUBLICATION& aPublished )
{
    std::set<std::pair<NODE_ID, INST_ID>> result;

    for( const auto& [key, island] : aPublished.Auxiliary().Islands() )
    {
        if( !island.value.atoms.busNoConnect )
            continue;

        const auto& row = aPublished.Rows().at( { *island.value.atoms.noConnect, key.inst } );
        const auto& bundle = *aPublished.Components().at( row.component ).content;

        for( const SLOT_KEY& slot : bundle.slots )
            result.emplace( aPublished.SlotComponents().at( slot ), key.inst );
    }

    return result;
}
} // namespace

std::vector<INST_ID> ENGINE::instancesInPageOrder() const
{
    std::vector<INST_ID> result;
    std::ranges::copy( std::views::keys( m_inputs.Instances().Entries() ), std::back_inserter( result ) );
    std::ranges::sort( result, {}, [&]( INST_ID id )
    {
        return std::tie( m_inputs.FindInstance( id )->value.pageOrder, m_keys.Instance( id ) );
    } );
    return result;
}

std::set<RECORD_KEY, KEY_LESS> ENGINE::ercIslands() const
{
    std::map<INST_ID, size_t> rank;

    for( INST_ID instance : instancesInPageOrder() )
        rank.emplace( instance, rank.size() );

    std::map<std::pair<SCREEN_ID, KIID>, RECORD_KEY> driven;
    std::set<RECORD_KEY, KEY_LESS> result( KEY_LESS{ m_keys } );

    for( const auto& [key, island] : m_published.Auxiliary().Islands() )
    {
        const auto& claims = m_records.Records().Entries().at( key )->value.claims;

        if( claims.empty() )
        {
            result.insert( key );
            continue;
        }

        const std::pair<SCREEN_ID, KIID> driver{ m_inputs.InstanceScreen( key.inst ), claims.front().source.item };
        const auto [first, inserted] = driven.try_emplace( driver, key );

        if( !inserted && rank.at( key.inst ) < rank.at( first->second.inst ) )
            first->second = key;
    }

    for( const auto& [driver, key] : driven )
        result.insert( key );

    return result;
}

std::vector<DRIVER_CONFLICT> ENGINE::DriverConflicts() const
{
    wxASSERT( wxThread::IsMain() );
    std::vector<DRIVER_CONFLICT> result;
    std::map<INST_ID, std::map<KIID, VECTOR2I>> positions;
    const auto reported = ercIslands();

    for( const auto& [key, island] : m_published.Auxiliary().Islands() )
    {
        if( island.value.atoms.strongNames.size() < 2 || !reported.contains( key ) )
            continue;

        const auto& claims = m_records.Records().Entries().at( key )->value.claims;
        const CLAIM& first = claims.front();
        const auto second = std::ranges::find_if( claims,
                [&]( const CLAIM& claim ) { return claim.Strong() && claim.name != first.name; } );

        if( second == claims.end() )
            continue;

        const auto& points = cached( positions, key.inst,
                [&] { return itemPositions( m_inputs.InstanceScreenFacts( key.inst ) ); } );

        result.push_back( { m_keys.Instance( key.inst ), first.source.item, second->source.item,
                            m_keys.Name( first.name ), m_keys.Name( second->name ),
                            points.at( second->source.item ) } );
    }

    return result;
}

std::vector<WIRE_ENDPOINT> ENGINE::DanglingWireEndpoints() const
{
    wxASSERT( wxThread::IsMain() );
    std::vector<WIRE_ENDPOINT> result;
    const auto reported = ercIslands();

    for( const auto& [key, island] : m_published.Auxiliary().Islands() )
    {
        if( !reported.contains( key ) )
            continue;

        const auto& facts = m_inputs.InstanceScreenFacts( key.inst ).items;

        for( const auto& [item, dangling] : island.value.dangling )
        {
            const ITEM_FACT* fact = dangling ? findFact( facts, item ) : nullptr;

            if( !fact )
                continue;

            const bool entry = fact->type == SCH_BUS_WIRE_ENTRY_T;
            const bool wire = fact->type == SCH_LINE_T && !fact->ports.empty()
                              && fact->ports.front().kind == PORT_KIND::WIRE;

            if( !entry && !wire )
                continue;

            for( size_t endpoint = 0; endpoint < fact->ports.size(); ++endpoint )
            {
                if( !( dangling & ( 1U << endpoint ) ) )
                    continue;

                // Coincident ends have the same marker identity
                if( endpoint != 0 && ( dangling & 1 )
                    && fact->ports[endpoint].position == fact->ports.front().position )
                {
                    continue;
                }

                result.push_back( { m_keys.Instance( key.inst ), item, fact->ports[endpoint].position, entry } );
            }
        }
    }

    return result;
}

std::vector<FLOATING_WIRE> ENGINE::FloatingWires() const
{
    wxASSERT( wxThread::IsMain() );
    std::vector<FLOATING_WIRE> result;

    for( const auto& [key, island] : m_published.Auxiliary().Islands() )
    {
        const auto& row = m_published.Rows().at( { island.value.items.front(), key.inst } );

        if( m_published.Components().at( row.component ).content->best )
            continue;

        const auto& facts = m_inputs.InstanceScreenFacts( key.inst ).items;
        FLOATING_WIRE group{ m_keys.Instance( key.inst ), {}, {} };

        for( const KIID& item : island.value.items )
        {
            const ITEM_FACT* fact = findFact( facts, item );

            if( !fact || fact->ports.empty() )
                continue;

            const bool wire = fact->type == SCH_LINE_T && fact->ports.front().kind == PORT_KIND::WIRE;

            if( !wire && fact->type != SCH_BUS_WIRE_ENTRY_T )
                continue;

            if( group.items.empty() )
                group.position = fact->ports.front().position;

            group.items.push_back( item );

            if( group.items.size() == 4 )
                break;
        }

        if( !group.items.empty() )
            result.push_back( std::move( group ) );
    }

    return result;
}

std::vector<BUS_NET_CONFLICT> ENGINE::BusNetConflicts() const
{
    wxASSERT( wxThread::IsMain() );
    std::vector<BUS_NET_CONFLICT> result;
    const auto reported = ercIslands();

    for( const auto& [key, island] : m_published.Auxiliary().Islands() )
    {
        if( !island.value.atoms.kindConflict || !reported.contains( key ) )
            continue;

        const auto& facts = m_inputs.InstanceScreenFacts( key.inst ).items;
        const auto& claims = m_records.Records().Entries().at( key )->value.claims;
        const ITEM_FACT* net = nullptr;
        const ITEM_FACT* bus = nullptr;

        for( const KIID& id : island.value.items )
        {
            const ITEM_FACT* fact = findFact( facts, id );

            if( !fact || fact->ports.empty() )
                continue;

            bool isBus = false;

            if( fact->type == SCH_LINE_T )
            {
                isBus = fact->ports.front().kind != PORT_KIND::WIRE;
            }
            else if( isLabelOrSheetPin( fact->type ) )
            {
                const auto claim = std::ranges::find_if( claims,
                        [&]( const CLAIM& candidate ) { return candidate.source.item == id; } );
                isBus = claim != claims.end() && bool( claim->schema );
            }
            else
            {
                continue;
            }

            if( isBus && !bus )
                bus = fact;
            else if( !isBus && !net )
                net = fact;

            if( net && bus )
                break;
        }

        if( net && bus )
            result.push_back( { m_keys.Instance( key.inst ), net->id, bus->id, net->ports.front().position } );
    }

    return result;
}

std::vector<BUS_BUS_CONFLICT> ENGINE::BusBusConflicts() const
{
    wxASSERT( wxThread::IsMain() );
    std::vector<BUS_BUS_CONFLICT> result;
    std::map<INST_ID, std::map<KIID, VECTOR2I>> positions;
    const auto reported = ercIslands();

    for( const auto& [key, island] : m_published.Auxiliary().Islands() )
    {
        if( !reported.contains( key ) )
            continue;

        const auto& record = m_records.Records().Entries().at( key )->value;

        if( record.atoms.mixedBusShapes )
        {
            const auto first = std::ranges::find_if( record.claims,
                    []( const CLAIM& claim ) { return bool( claim.schema ); } );
            const auto second = std::find_if( first, record.claims.end(),
                    [&]( const CLAIM& claim )
                    {
                        return claim.schema && claim.schema->shape != first->schema->shape;
                    } );
            wxCHECK2( first != record.claims.end() && second != record.claims.end(), continue );
            const auto& points = cached( positions, key.inst,
                    [&] { return itemPositions( m_inputs.InstanceScreenFacts( key.inst ) ); } );

            result.push_back( { m_keys.Instance( key.inst ), first->source.item, second->source.item,
                                points.at( first->source.item ), true } );
            continue;
        }

        if( record.kind != KIND::BUNDLE || record.claims.size() < 2 )
            continue;

        const auto& facts = m_inputs.InstanceScreenFacts( key.inst ).items;
        const CLAIM* canonical = nullptr;
        VECTOR2I position;
        std::set<wxString> leaves;

        for( const CLAIM& claim : record.claims )
        {
            const ITEM_FACT* fact = claim.schema ? findFact( facts, claim.source.item ) : nullptr;

            if( !fact || fact->ports.empty() || !isLabelOrSheetPin( fact->type ) )
                continue;

            if( !canonical )
            {
                canonical = &claim;
                position = fact->ports.front().position;

                for( const auto& leaf : claim.schema->leaves )
                    leaves.insert( leaf.name );

                continue;
            }

            if( std::ranges::any_of( claim.schema->leaves,
                                     [&]( const BUS_SCHEMA::LEAF& leaf ) { return leaves.contains( leaf.name ); } ) )
            {
                continue;
            }

            result.push_back( { m_keys.Instance( key.inst ), canonical->source.item, claim.source.item, position } );
            break;
        }
    }

    return result;
}

std::vector<BUS_ENTRY_CONFLICT> ENGINE::BusEntryConflicts() const
{
    wxASSERT( wxThread::IsMain() );
    std::vector<BUS_ENTRY_CONFLICT> result;
    const auto& rows = m_published.Rows();
    const auto& islandOf = m_published.Auxiliary().IslandOf();
    const auto reported = ercIslands();
    const auto name = [&]( NAME_ID id ) { return id == INVALID_ID ? wxString() : m_keys.Name( id ); };

    for( const auto& [key, island] : m_published.Auxiliary().Islands() )
    {
        if( !reported.contains( key ) )
            continue;

        KIID previous = niluuid;
        const auto& facts = m_inputs.InstanceScreenFacts( key.inst ).items;

        for( const auto& [entry, bus] : island.value.busEntryLinks )
        {
            // Each contact is published on both islands; report from the entry side only
            if( islandOf.at( { entry, key.inst } ) != key || entry == previous )
                continue;

            const ITEM_FACT* busFact = findFact( facts, bus );

            if( !busFact || busFact->type != SCH_LINE_T || busFact->ports.empty()
                || busFact->ports.front().kind != PORT_KIND::BUS )
            {
                continue;
            }

            previous = entry;
            const auto signalRow = rows.find( { entry, key.inst } );
            const auto busRow = rows.find( { bus, key.inst } );

            if( signalRow == rows.end() || busRow == rows.end()
                || signalRow->second.kind != KIND::SIGNAL || busRow->second.kind != KIND::BUNDLE )
            {
                continue;
            }

            const auto& signal = *m_published.Components().at( signalRow->second.component ).content;
            const auto& bundle = *m_published.Components().at( busRow->second.component ).content;

            if( !signal.best || signal.best->priority >= PRIORITY::GLOBAL_POWER_PIN )
                continue;

            // Slot membership includes alternate strong names and hierarchy-renamed members
            const bool member = std::ranges::any_of( bundle.members,
                    [&]( const SLOT_KEY& slot )
                    {
                        return m_published.SlotComponents().at( slot ) == signalRow->second.component;
                    } );

            if( member )
                continue;

            const ITEM_FACT* fact = findFact( facts, entry );

            if( !fact || fact->type != SCH_BUS_WIRE_ENTRY_T || fact->ports.empty() )
                continue;

            result.push_back( { m_keys.Instance( key.inst ), entry, bus, name( signalRow->second.name ),
                                name( busRow->second.name ), fact->ports.front().position } );
        }
    }

    return result;
}

std::vector<NO_CONNECT_PIN_CONFLICT> ENGINE::NoConnectPinConflicts() const
{
    wxASSERT( wxThread::IsMain() );
    struct GROUP
    {
        std::set<KIID> pins;
        std::set<KIID> others;
    };
    std::map<INST_ID, std::map<KIID, NC_SOURCE>> sources;
    std::map<std::pair<KIID_PATH, VECTOR2I>, GROUP, SHEET_POSITION_LESS> groups;

    for( const auto& [key, island] : m_published.Auxiliary().Islands() )
    {
        if( island.value.ncContacts.empty() )
            continue;

        const auto& items = cached( sources, key.inst,
                [&] { return noConnectSources( m_inputs.InstanceScreenFacts( key.inst ) ); } );

        for( const auto& [first, second] : island.value.ncContacts )
        {
            const NC_SOURCE& a = items.at( first );
            const NC_SOURCE& b = items.at( second );

            if( a.nc == b.nc || a.powerFlag || b.powerFlag )
                continue;

            const NC_SOURCE& pin = a.nc ? a : b;
            const NC_SOURCE& other = a.nc ? b : a;

            if( other.type == SCH_NO_CONNECT_T )
                continue;

            auto& group = groups[{ m_keys.Instance( key.inst ), pin.position }];
            group.pins.insert( a.nc ? first : second );
            group.others.insert( a.nc ? second : first );
        }
    }

    std::vector<NO_CONNECT_PIN_CONFLICT> result;

    for( const auto& [key, group] : groups )
    {
        result.push_back( { key.first, { group.pins.begin(), group.pins.end() },
                            { group.others.begin(), group.others.end() }, key.second } );
    }

    return result;
}

std::vector<NO_CONNECT_FLAG_ERROR> ENGINE::NoConnectFlagErrors() const
{
    wxASSERT( wxThread::IsMain() );
    struct NET
    {
        uint32_t pins = 0;
        bool     label = false;
    };
    std::map<INST_ID, std::map<KIID, NC_SOURCE>> sources;
    std::map<NODE_ID, NET> nets;
    const auto source = [&]( INST_ID inst, const KIID& id ) -> const NC_SOURCE&
    {
        return cached( sources, inst, [&] { return noConnectSources( m_inputs.InstanceScreenFacts( inst ) ); } )
                .at( id );
    };
    const auto pinWitness = []( const NC_SOURCE& item ) { return item.pin && !item.powerFlag; };
    const auto& islands = m_published.Auxiliary().Islands();
    const auto reported = ercIslands();
    std::vector<NO_CONNECT_FLAG_ERROR> result;

    for( const auto& [key, island] : islands )
    {
        if( !island.value.atoms.noConnect || island.value.atoms.busNoConnect || !reported.contains( key ) )
            continue;

        const KIID& flag = *island.value.atoms.noConnect;
        const VECTOR2I position = source( key.inst, flag ).position;
        bool hierarchy = false;
        bool clean = true;
        bool attached = false;
        KIID pin = niluuid;

        for( const KIID& id : island.value.items )
        {
            const NC_SOURCE& item = source( key.inst, id );
            const bool hier = item.type == SCH_SHEET_PIN_T || item.type == SCH_HIER_LABEL_T;
            hierarchy |= hier;
            attached |= hier && item.position == position;
            clean &= !pinWitness( item ) && item.type != SCH_LABEL_T && item.type != SCH_GLOBAL_LABEL_T
                     && item.type != SCH_DIRECTIVE_LABEL_T;

            if( pinWitness( item ) && ( pin == niluuid || id < pin ) )
                pin = id;
        }

        for( const auto& [first, second] : island.value.ncContacts )
        {
            if( first == flag || second == flag )
                attached |= source( key.inst, first == flag ? second : first ).nc;
        }

        if( attached || ( hierarchy && clean ) )
            continue;

        const auto& row = m_published.Rows().at( { flag, key.inst } );
        const NET& net = cached( nets, row.component, [&]
        {
            NET value;
            const auto& component = *m_published.Components().at( row.component ).content;

            for( const RECORD_KEY& record : component.records )
                value.pins += islands.at( record ).value.atoms.pinCount;

            for( const ITEM_KEY& id : component.items )
                value.label |= isLabel( source( id.inst, id.item ).type );

            return value;
        } );

        const bool connected = net.pins > 1;

        if( connected || ( net.pins == 0 && !net.label ) )
        {
            result.push_back( { m_keys.Instance( key.inst ), flag, connected ? pin : niluuid,
                                connected && pin != niluuid ? source( key.inst, pin ).position : position,
                                connected } );
        }
    }

    return result;
}

std::vector<UNCONNECTED_PIN> ENGINE::UnconnectedPins() const
{
    wxASSERT( wxThread::IsMain() );
    struct SOURCE
    {
        const PIN_FACT* pin = nullptr;
        KICAD_T         type = TYPE_NOT_INIT;
        KIID            owner;
        KIID            group;
        PORT_KIND       port = PORT_KIND::ANCHOR;

        bool PowerSymbol() const { return pin && ( pin->globalPowerParent || pin->localPowerParent ); }
        bool PowerFlag() const { return PowerSymbol() && pin->type == ELECTRICAL_PINTYPE::PT_POWER_OUT; }
        bool Label() const { return isLabelOrSheetPin( type ); }
    };
    const auto samePin = []( const SOURCE& a, const SOURCE& b )
    {
        if( a.group == b.group )
            return true;

        if( !a.pin || !b.pin || a.owner != b.owner || a.pin->position != b.pin->position
            || a.pin->shownName != b.pin->shownName )
            return false;

        const auto passive = []( ELECTRICAL_PINTYPE type )
        {
            return type == ELECTRICAL_PINTYPE::PT_PASSIVE || type == ELECTRICAL_PINTYPE::PT_NIC;
        };
        return a.pin->type == b.pin->type || passive( a.pin->type ) || passive( b.pin->type );
    };
    std::map<INST_ID, std::map<KIID, SOURCE>> sources;
    std::map<NODE_ID, std::vector<CLAIM>> drivers;
    std::map<NODE_ID, bool> powerConnections;
    const auto source = [&]( INST_ID inst, const KIID& id ) -> const SOURCE&
    {
        return cached( sources, inst, [&]
        {
            std::map<KIID, SOURCE> items;

            for( const ITEM_FACT& fact : m_inputs.InstanceScreenFacts( inst ).items )
            {
                for( const PIN_FACT& pin : fact.pins )
                    items.emplace( pin.id, SOURCE{ &pin, SCH_PIN_T, fact.owner, fact.id } );

                if( !fact.ports.empty() )
                {
                    items.emplace( fact.id,
                                   SOURCE{ nullptr, fact.type, fact.owner, fact.id, fact.ports.front().kind } );
                }
            }

            return items;
        } ).at( id );
    };
    const auto candidates = [&]( NODE_ID component ) -> const std::vector<CLAIM>&
    {
        return cached( drivers, component, [&] { return DriverCandidates( component ); } );
    };
    const auto& auxiliary = m_published.Auxiliary();
    const auto busNoConnect = busNoConnectMembers( m_published );

    const auto reported = ercIslands();
    std::vector<UNCONNECTED_PIN> result;

    for( const auto& [key, island] : auxiliary.Islands() )
    {
        if( island.value.atoms.noConnect || !island.value.atoms.hasSymbolPin || !reported.contains( key ) )
            continue;

        const auto& row = m_published.Rows().at( { island.value.items.front(), key.inst } );

        if( busNoConnect.contains( { row.component, key.inst } ) )
            continue;

        std::vector<const SOURCE*> pins;
        bool connected = false;

        for( const KIID& id : island.value.items )
        {
            const SOURCE& item = source( key.inst, id );
            connected |= item.Label();

            if( item.pin && !item.PowerFlag() )
                pins.push_back( &item );
        }

        const SOURCE* selected = nullptr;

        for( const SOURCE* item : pins )
        {
            const PIN_FACT& pin = *item->pin;

            if( pin.type == ELECTRICAL_PINTYPE::PT_NC || pin.type == ELECTRICAL_PINTYPE::PT_NIC )
                continue;

            if( !item->PowerSymbol() )
            {
                if( !selected || ( selected->pin->invisible && !pin.invisible )
                    || ( pin.invisible == selected->pin->invisible
                         && pin.type == ELECTRICAL_PINTYPE::PT_POWER_IN
                         && selected->pin->type != ELECTRICAL_PINTYPE::PT_POWER_IN ) )
                    selected = item;

                continue;
            }

            bool attached = false;
            const auto contact = [&]( const KIID& otherId )
            {
                const SOURCE& other = source( key.inst, otherId );

                if( other.pin )
                    return !other.PowerFlag() && other.owner != item->owner;

                return other.Label() || other.type == SCH_DIRECTIVE_LABEL_T || other.type == SCH_JUNCTION_T
                       || other.type == SCH_NO_CONNECT_T
                       || ( other.type == SCH_LINE_T && other.port == PORT_KIND::WIRE );
            };

            if( island.value.dangling.at( pin.id ) == 0 )
            {
                if( const auto found = auxiliary.NeighborsOf().find( { pin.id, key.inst } );
                    found != auxiliary.NeighborsOf().end() )
                    attached = std::ranges::any_of( found->second, contact );

                for( const auto& [first, second] : island.value.ncContacts )
                {
                    if( first == pin.id || second == pin.id )
                        attached |= contact( first == pin.id ? second : first );
                }
            }

            const bool electrical = cached( powerConnections, row.component, [&]
            {
                bool value = false;
                const auto& component = *m_published.Components().at( row.component ).content;

                for( const ITEM_KEY& member : component.items )
                {
                    const SOURCE& other = source( member.inst, member.item );
                    value |= other.Label() || ( other.pin && !other.PowerSymbol() );
                }

                for( const CLAIM& claim : candidates( row.component ) )
                    value |= source( claim.source.inst, claim.source.item ).Label();

                return value;
            } );

            if( !attached || !electrical )
                result.push_back( { m_keys.Instance( key.inst ), pin.id, pin.position } );
        }

        if( !selected || connected )
            continue;

        for( size_t first = 0; first < pins.size() && !connected; ++first )
        {
            if( pins[first]->PowerSymbol() )
                continue;

            connected = std::any_of( pins.begin() + first + 1, pins.end(),
                    [&]( const SOURCE* b ) { return !b->PowerSymbol() && !samePin( *pins[first], *b ); } );
        }

        if( !connected )
        {
            connected = std::ranges::any_of( candidates( row.component ),
                    [&]( const CLAIM& claim )
                    {
                        if( claim.priority < PRIORITY::HIER_LABEL )
                            return false;

                        const SOURCE& driver = source( claim.source.inst, claim.source.item );
                        return !driver.PowerFlag()
                               && ( claim.source.inst != key.inst || !samePin( driver, *selected ) );
                    } );
        }

        if( !connected )
            result.push_back( { m_keys.Instance( key.inst ), selected->pin->id, selected->pin->position } );
    }

    return result;
}

std::vector<LABEL_WIRE_CONFLICT> ENGINE::LabelWireConflicts() const
{
    wxASSERT( wxThread::IsMain() );
    std::map<std::pair<KIID_PATH, VECTOR2I>, LABEL_WIRE_CONFLICT, SHEET_POSITION_LESS> locations;

    for( const auto& [key, neighbors] : m_published.Auxiliary().NeighborsOf() )
    {
        const auto& facts = m_inputs.InstanceScreenFacts( key.inst ).items;
        const ITEM_FACT* label = findFact( facts, key.item );

        if( !label || !isLabel( label->type ) || label->ports.empty() )
            continue;

        const VECTOR2I position = label->ports.front().position;
        std::vector<KIID> wires;

        for( const KIID& id : neighbors )
        {
            const ITEM_FACT* wire = findFact( facts, id );

            if( wire && wire->type == SCH_LINE_T && wire->segment
                && position != wire->segment->A && position != wire->segment->B )
            {
                wires.push_back( id );
            }
        }

        if( wires.size() < 2 )
            continue;

        std::sort( wires.begin(), wires.end() );
        const KIID_PATH& sheet = m_keys.Instance( key.inst );
        auto [entry, inserted] = locations.try_emplace( std::pair{ sheet, position },
                LABEL_WIRE_CONFLICT{ sheet, key.item, position, std::move( wires ) } );

        if( !inserted && key.item < entry->second.label )
            entry->second.label = key.item;
    }

    return moveValues( locations );
}

std::vector<FOUR_WAY_JUNCTION> ENGINE::FourWayJunctions() const
{
    wxASSERT( wxThread::IsMain() );
    struct CONTACTS
    {
        std::map<KIID, const PIN_FACT*> pins;
        std::vector<KIID>               lines;
        std::vector<KIID>               members;
    };
    std::vector<FOUR_WAY_JUNCTION> result;

    for( INST_ID inst : islandInstances( m_published.Auxiliary().Islands() ) )
    {
        const auto* instance = m_inputs.FindInstance( inst );

        if( !instance )
            throw std::logic_error( "Missing instance inputs in four-way connectivity query" );

        const auto& units = instance->value.units;
        std::map<VECTOR2I, CONTACTS> locations;

        for( const ITEM_FACT& fact : m_inputs.InstanceScreenFacts( inst ).items )
        {
            if( fact.type == SCH_PIN_T )
            {
                const auto unit = std::lower_bound( units.begin(), units.end(), fact.owner,
                        []( const auto& entry, const KIID& owner ) { return entry.first < owner; } );

                if( unit == units.end() || unit->first != fact.owner )
                    throw std::invalid_argument( "Missing symbol unit in connectivity geometry" );

                for( const PIN_FACT& pin : fact.pins )
                {
                    if( pin.unit != 0 && unit->second != 0 && pin.unit != unit->second )
                        continue;

                    auto& contacts = locations[pin.position];
                    contacts.members.push_back( pin.id );
                    auto& selected = contacts.pins[fact.owner];

                    if( !selected || std::tie( pin.invisible, pin.id )
                                         < std::tie( selected->invisible, selected->id ) )
                        selected = &pin;
                }
            }
            else if( fact.type == SCH_LINE_T )
            {
                for( const PORT_FACT& port : fact.ports )
                {
                    auto& contacts = locations[port.position];
                    contacts.lines.push_back( fact.id );
                    contacts.members.push_back( fact.id );
                }
            }
        }

        for( auto& [position, contacts] : locations )
        {
            if( contacts.pins.size() + contacts.lines.size() < 4 )
                continue;

            std::sort( contacts.members.begin(), contacts.members.end() );
            contacts.members.erase( std::unique( contacts.members.begin(), contacts.members.end() ),
                                    contacts.members.end() );
            FOUR_WAY_JUNCTION junction{ m_keys.Instance( inst ), position, {}, std::move( contacts.members ) };

            for( const auto& [owner, pin] : contacts.pins )
                junction.items.push_back( pin->id );

            std::sort( junction.items.begin(), junction.items.end() );
            std::sort( contacts.lines.begin(), contacts.lines.end() );
            junction.items.insert( junction.items.end(), contacts.lines.begin(), contacts.lines.end() );
            result.push_back( std::move( junction ) );
        }
    }

    std::ranges::sort( result, {}, []( const auto& e ) { return std::tie( e.sheet, e.position.x, e.position.y ); } );
    return result;
}

std::vector<NETCLASS_REFERENCE> ENGINE::NetclassReferences() const
{
    wxASSERT( wxThread::IsMain() );
    std::vector<NETCLASS_REFERENCE> result;

    for( const auto& [instance, entry] : m_inputs.Instances().Entries() )
        std::ranges::copy( entry->value.netclassReferences, std::back_inserter( result ) );

    std::ranges::sort( result, {}, []( const auto& e ) { return std::tie( e.sheet, e.item, e.name ); } );
    return result;
}

std::vector<UNMAPPED_PIN_CANDIDATE> ENGINE::UnmappedPinCandidates() const
{
    wxASSERT( wxThread::IsMain() );
    std::vector<UNMAPPED_PIN_CANDIDATE> result;
    const auto& auxiliary = m_published.Auxiliary();

    for( INST_ID inst : instancesInPageOrder() )
    {
        const auto* entry = m_inputs.FindInstance( inst );

        for( const auto& pin : entry->value.pinMapCandidates )
        {
            const auto island = auxiliary.IslandOf().find( { pin.id, inst } );

            if( island == auxiliary.IslandOf().end() )
                continue;

            if( !pin.ignoresDangling && auxiliary.Islands().at( island->second ).value.dangling.at( pin.id ) )
                continue;

            result.push_back( { m_keys.Instance( inst ), pin.id, pin.position, pin.number, pin.footprint } );
        }
    }

    return result;
}

std::vector<PIN_MAP_FACT> ENGINE::PinMapSymbols( SCREEN_ID aScreen ) const
{
    wxASSERT( wxThread::IsMain() );
    const auto* screen = m_inputs.FindScreen( aScreen );
    return screen ? screen->value.pinMaps : std::vector<PIN_MAP_FACT>();
}

std::vector<LIBRARY_SYMBOL_FACT> ENGINE::LibrarySymbols( const KIID_PATH& aPath ) const
{
    wxASSERT( wxThread::IsMain() );
    const auto id = m_keys.FindInstance( aPath );
    const auto* entry = id ? m_inputs.Instances().Find( *id ) : nullptr;
    return entry ? m_inputs.InstanceScreenFacts( *id ).LibrarySymbols()
                 : std::vector<LIBRARY_SYMBOL_FACT>();
}

std::vector<VARIANT_SYMBOL_FACT> ENGINE::VariantSymbols( const KIID_PATH& aPath ) const
{
    wxASSERT( wxThread::IsMain() );
    const auto id = m_keys.FindInstance( aPath );
    const auto* entry = id ? m_inputs.Instances().Find( *id ) : nullptr;
    return entry ? entry->value.variantSymbols : std::vector<VARIANT_SYMBOL_FACT>();
}

std::vector<FOOTPRINT_SOURCE> ENGINE::FootprintSources() const
{
    wxASSERT( wxThread::IsMain() );
    std::vector<FOOTPRINT_SOURCE> result;

    for( INST_ID id : instancesInPageOrder() )
    {
        const auto*      entry = m_inputs.FindInstance( id );
        const auto& sources = m_inputs.InstanceScreenFacts( id ).footprints;
        const auto& footprints = entry->value.footprints;
        const KIID_PATH& sheet = m_keys.Instance( id );

        if( sources.size() != footprints.size() )
            throw std::runtime_error( "Footprint instance facts do not match screen facts" );

        for( size_t i = 0; i < footprints.size(); ++i )
        {
            const auto& source = sources[i];
            const auto& [item, footprint] = footprints[i];

            if( source.id != item )
                throw std::runtime_error( "Footprint instance facts do not match screen facts" );

            result.push_back( { sheet, item, source.position, footprint, source.filters } );
        }
    }

    return result;
}

std::vector<MULTI_UNIT_GROUP> ENGINE::MultiUnitSymbols() const
{
    wxASSERT( wxThread::IsMain() );
    std::map<wxString, MULTI_UNIT_GROUP> groups;

    for( INST_ID id : instancesInPageOrder() )
    {
        std::map<KIID, const MULTI_UNIT_FACT*> sources;

        for( const MULTI_UNIT_FACT& source : m_inputs.InstanceScreenFacts( id ).multiUnits )
            sources.emplace( source.id, &source );

        for( const MULTI_UNIT_REFERENCE& reference : m_inputs.FindInstance( id )->value.multiUnits )
        {
            if( reference.reference.IsEmpty() || reference.reference.EndsWith( "?" ) )
                continue;

            const auto& source = *sources.at( reference.id );
            auto [group, inserted] = groups.try_emplace( reference.reference );

            if( inserted )
            {
                group->second.reference = reference.reference;
                group->second.units = source.units;
            }

            group->second.instances.push_back( { m_keys.Instance( id ), source.id, source.position,
                    reference.name, reference.footprint, reference.unit } );
        }
    }

    return moveValues( groups );
}

std::vector<DUPLICATE_SHEET_ERROR> ENGINE::DuplicateSheetNames() const
{
    wxASSERT( wxThread::IsMain() );
    std::vector<DUPLICATE_SHEET_ERROR> result;

    for( const auto& [instance, entry] : m_inputs.Instances().Entries() )
    {
        const auto& children = entry->value.childSheets;

        for( size_t i = 0; i < children.size(); ++i )
        {
            for( size_t j = i + 1; j < children.size(); ++j )
            {
                if( children[i].name.IsSameAs( children[j].name, false ) )
                {
                    result.push_back( { m_keys.Instance( instance ), children[i].id,
                                        children[j].id, children[i].position } );
                }
            }
        }
    }

    std::ranges::sort( result, {}, []( const auto& e ) { return std::tie( e.sheet, e.main, e.auxiliary ); } );
    return result;
}

std::vector<FIELD_NAME_ERROR> ENGINE::InvalidFieldNames() const
{
    wxASSERT( wxThread::IsMain() );
    std::vector<FIELD_NAME_ERROR> result;

    for( const auto& [instance, entry] : m_inputs.Instances().Entries() )
    {
        for( const FIELD_NAME_FACT& field : m_inputs.InstanceScreenFacts( instance ).invalidFieldNames )
            result.push_back( { m_keys.Instance( instance ), field } );
    }

    std::ranges::sort( result, {}, []( const auto& e ) { return std::tie( e.sheet, e.field.owner, e.field.field ); } );
    return result;
}

std::vector<SOURCE_LOCATION> ENGINE::EmptyLabels() const
{
    wxASSERT( wxThread::IsMain() );
    std::vector<SOURCE_LOCATION> result;

    for( INST_ID instance : instancesInPageOrder() )
    {
        for( const ITEM_FACT& fact : m_inputs.InstanceScreenFacts( instance ).items )
        {
            if( !isLabel( fact.type ) )
                continue;

            wxString text = fact.rawText;

            if( text.Trim( false ).Trim( true ).IsEmpty() )
                result.push_back( { m_keys.Instance( instance ), fact.id, fact.ports.front().position } );
        }
    }

    return result;
}

std::vector<SOURCE_LOCATION> ENGINE::WiredImplicitPowerPins() const
{
    wxASSERT( wxThread::IsMain() );
    std::vector<SOURCE_LOCATION> result;
    const auto& auxiliary = m_published.Auxiliary();

    for( INST_ID instance : instancesInPageOrder() )
    {
        for( const ITEM_FACT& fact : m_inputs.InstanceScreenFacts( instance ).items )
        {
            for( const PIN_FACT& pin : fact.pins )
            {
                const ITEM_KEY key{ pin.id, instance };
                const auto* text = m_inputs.Text( key );

                if( !text || !text->value.canDrive || !pin.globalPower || !pin.invisible
                    || pin.type != ELECTRICAL_PINTYPE::PT_POWER_IN
                    || pin.globalPowerParent || pin.localPowerParent )
                    continue;

                const auto& island = auxiliary.Islands().at( auxiliary.IslandOf().at( key ) );

                if( island.value.atoms.invisiblePowerWired )
                {
                    result.push_back( { m_keys.Instance( instance ), pin.id, pin.position } );
                    break;
                }
            }
        }
    }

    return result;
}

std::vector<SOURCE_LOCATION> ENGINE::InvalidPinNotation() const
{
    wxASSERT( wxThread::IsMain() );
    std::vector<SOURCE_LOCATION> result;

    for( INST_ID instance : instancesInPageOrder() )
    {
        for( const ITEM_FACT& fact : m_inputs.InstanceScreenFacts( instance ).items )
        {
            for( const PIN_FACT& pin : fact.pins )
            {
                if( !m_inputs.Text( { pin.id, instance } ) )
                    continue;

                bool valid = false;
                ExpandStackedPinNotation( pin.shownNumber, &valid );

                if( !valid )
                    result.push_back( { m_keys.Instance( instance ), pin.id, pin.position } );
            }
        }
    }

    return result;
}

std::vector<OFF_GRID_ENDPOINT> ENGINE::OffGridEndpoints( int aGrid ) const
{
    wxASSERT( wxThread::IsMain() );
    std::vector<OFF_GRID_ENDPOINT> result;
    const auto offGrid = [aGrid]( const VECTOR2I& point )
    {
        return point.x % aGrid != 0 || point.y % aGrid != 0;
    };

    std::set<SCREEN_ID> screens;

    for( INST_ID instance : instancesInPageOrder() )
    {
        // Screen geometry is the same on every instance, so only the first one in page order reports
        if( !screens.insert( m_inputs.InstanceScreen( instance ) ).second )
            continue;

        const KIID_PATH& path = m_keys.Instance( instance );
        std::map<KIID, OFF_GRID_ENDPOINT> symbols;

        for( const ITEM_FACT& fact : m_inputs.InstanceScreenFacts( instance ).items )
        {
            if( fact.type == SCH_LINE_T && fact.segment )
            {
                if( offGrid( fact.segment->A ) )
                    result.push_back( { path, fact.id, fact.segment->A, {} } );
                else if( offGrid( fact.segment->B ) )
                    result.push_back( { path, fact.id, fact.segment->B, {} } );
            }
            else if( fact.type == SCH_BUS_WIRE_ENTRY_T )
            {
                for( const PORT_FACT& port : fact.ports )
                {
                    if( offGrid( port.position ) )
                        result.push_back( { path, fact.id, port.position, {} } );
                }
            }

            for( const PIN_FACT& pin : fact.pins )
            {
                if( pin.type == ELECTRICAL_PINTYPE::PT_NC || !m_inputs.Text( { pin.id, instance } )
                    || !offGrid( pin.position ) )
                    continue;

                auto [found, inserted] = symbols.try_emplace( fact.owner );
                OFF_GRID_ENDPOINT& diagnostic = found->second;

                if( inserted || pin.id < diagnostic.item )
                {
                    diagnostic.sheet = path;
                    diagnostic.item = pin.id;
                    diagnostic.position = pin.position;
                }

                diagnostic.equivalentPins.emplace_back( pin.id, pin.position );
            }
        }

        std::ranges::move( std::views::values( symbols ), std::back_inserter( result ) );
    }

    return result;
}

std::vector<GROUND_PIN_ERROR> ENGINE::GroundPinErrors() const
{
    wxASSERT( wxThread::IsMain() );
    const auto isGround = []( const wxString& name )
    {
        const wxString upper = name.Upper();
        return upper.Contains( wxS( "GND" ) ) || upper == wxS( "EARTH" ) || upper.StartsWith( wxS( "EARTH_" ) )
               || upper == wxS( "VSS" ) || upper == wxS( "VSSA" );
    };
    struct SYMBOL_PINS
    {
        bool                          hasGroundNet = false;
        std::vector<GROUND_PIN_ERROR> mismatched;
    };
    std::vector<GROUND_PIN_ERROR> result;

    for( INST_ID inst : islandInstances( m_published.Auxiliary().Islands() ) )
    {
        std::map<KIID, SYMBOL_PINS> symbols;

        for( const ITEM_FACT& fact : m_inputs.InstanceScreenFacts( inst ).items )
        {
            for( const PIN_FACT& pin : fact.pins )
            {
                if( ( pin.type != ELECTRICAL_PINTYPE::PT_POWER_IN && pin.type != ELECTRICAL_PINTYPE::PT_POWER_OUT )
                    || !m_inputs.Text( { pin.id, inst } ) )
                    continue;

                wxString name;
                const auto row = m_published.Rows().find( { pin.id, inst } );

                if( row != m_published.Rows().end() )
                {
                    const auto& component = m_published.Components().at( row->second.component );

                    if( component.name != INVALID_ID )
                    {
                        name = m_keys.Name( component.name );

                        if( component.content->best && component.content->best->path != INVALID_ID )
                            name = name.Mid( m_keys.Name( component.content->best->path ).length() );
                    }
                }

                auto& symbol = symbols[fact.owner];
                const bool ground = isGround( name );
                symbol.hasGroundNet |= ground;

                if( isGround( pin.shownName ) && !ground )
                    symbol.mismatched.push_back( { m_keys.Instance( inst ), pin.id, pin.position, pin.shownName } );
            }
        }

        for( auto& [owner, symbol] : symbols )
        {
            if( symbol.hasGroundNet )
                std::ranges::move( symbol.mismatched, std::back_inserter( result ) );
        }
    }

    std::ranges::sort( result, {}, []( const auto& e ) { return std::tie( e.sheet, e.pin ); } );
    return result;
}

std::vector<LABEL_CONNECTION_ERROR> ENGINE::LabelConnectionErrors() const
{
    wxASSERT( wxThread::IsMain() );
    struct CONNECTIONS
    {
        size_t                    count = 0;
        bool                      noConnect = false;
        std::map<INST_ID, size_t> slots;
        size_t                    slotCount = 0;
        size_t                    pins = 0;
        std::map<INST_ID, size_t> localPins;
        std::map<INST_ID, size_t> hierarchy;
        bool                      busMember = false;
    };
    std::map<NODE_ID, CONNECTIONS> connections;
    const auto& auxiliary = m_published.Auxiliary();
    const auto busNoConnect = busNoConnectMembers( m_published );
    std::map<SCREEN_ID, std::set<KIID>> screenPinIds;

    const auto screenPins = [&]( INST_ID aInstance ) -> const std::set<KIID>&
    {
        return cached( screenPinIds, m_inputs.InstanceScreen( aInstance ), [&]
        {
            std::set<KIID> ids;

            for( const ITEM_FACT& item : m_inputs.InstanceScreenFacts( aInstance ).items )
            {
                for( const PIN_FACT& pin : item.pins )
                    ids.insert( pin.id );
            }

            return ids;
        } );
    };

    const auto netConnections = [&]( NODE_ID id ) -> const CONNECTIONS&
    {
        return cached( connections, id, [&]
        {
            CONNECTIONS net;
            const auto& component = *m_published.Components().at( id ).content;
            std::set<std::pair<INST_ID, wxString>> ports;
            std::set<std::pair<INST_ID, wxString>> hierLabels;
            net.busMember = !component.slots.empty();

            for( const RECORD_KEY& key : component.records )
            {
                const auto& island = auxiliary.Islands().at( key ).value;
                const auto& facts = m_inputs.InstanceScreenFacts( key.inst ).items;
                const auto& pins = screenPins( key.inst );
                net.count += island.atoms.pinCount;
                net.noConnect |= island.atoms.noConnect.has_value();

                for( const KIID& item : island.items )
                {
                    // Power symbol pins count here even though they are not naming witnesses
                    if( pins.contains( item ) )
                    {
                        ++net.pins;
                        ++net.localPins[key.inst];
                        continue;
                    }

                    const ITEM_FACT* source = findFact( facts, item );

                    if( !source )
                        continue;

                    if( source->type == SCH_SHEET_PIN_T )
                    {
                        ++net.count;
                        ++net.hierarchy[key.inst];
                    }
                    else if( source->type == SCH_HIER_LABEL_T )
                    {
                        const wxString& name = m_inputs.Text( { item, key.inst } )->value.name;
                        hierLabels.emplace( key.inst, name );

                        if( m_keys.Instance( key.inst ).size() > 1 )
                            ports.emplace( key.inst, name );
                    }
                }
            }

            net.count += ports.size();

            for( const auto& [instance, name] : hierLabels )
                ++net.hierarchy[instance];

            std::set<NAME_KEY, KEY_LESS> neighbors( KEY_LESS{ m_keys } );

            for( const SLOT_KEY& slot : component.slots )
            {
                for( const NAME_KEY& edge : m_slots.Slots().Entries().at( slot )->value.edges )
                {
                    if( edge.scope == SCOPE::SHEET )
                        neighbors.insert( edge );
                }
            }

            for( const NAME_KEY& neighbor : neighbors )
            {
                ++net.slots[neighbor.inst];
                ++net.slotCount;
            }

            return net;
        } );
    };
    const auto reported = ercIslands();
    std::vector<LABEL_CONNECTION_ERROR> result;

    for( const auto& [key, island] : auxiliary.Islands() )
    {
        if( !reported.contains( key ) )
            continue;

        const auto& facts = m_inputs.InstanceScreenFacts( key.inst ).items;

        for( const KIID& id : island.value.items )
        {
            const ITEM_FACT* label = findFact( facts, id );

            if( !label || !isLabel( label->type ) )
                continue;

            const auto& row = m_published.Rows().at( { id, key.inst } );

            if( row.kind == KIND::BUNDLE )
                continue;

            const CONNECTIONS& net = netConnections( row.component );
            const auto localSlots = net.slots.find( key.inst );
            const size_t count = net.count + net.slotCount
                                 - ( localSlots == net.slots.end() ? 0 : localSlots->second );
            const auto hierarchy = net.hierarchy.find( key.inst );
            const bool busNc = busNoConnect.contains( { row.component, key.inst } );

            // A local label without pins on its sheet may still route hierarchy or join a net flagged elsewhere
            const bool routesHierarchy = hierarchy != net.hierarchy.end()
                                         && hierarchy->second > ( net.busMember ? 0U : 1U );
            const bool orphan = label->type == SCH_LABEL_T && !net.localPins.contains( key.inst ) && net.pins > 1
                                && !net.noConnect && !busNc && !routesHierarchy;
            const bool unconnected = island.value.dangling.at( id ) != 0 || count == 0 || orphan;
            const bool singlePin = count == 1 && !net.noConnect && !busNc;

            if( unconnected )
                result.push_back( { m_keys.Instance( key.inst ), id, label->ports.front().position, false } );

            if( singlePin )
                result.push_back( { m_keys.Instance( key.inst ), id, label->ports.front().position, true } );
        }
    }

    return result;
}

std::vector<LABEL_LOCATION> ENGINE::DanglingDirectives() const
{
    wxASSERT( wxThread::IsMain() );
    std::vector<LABEL_LOCATION> result;

    for( const auto& [key, island] : m_published.Auxiliary().Islands() )
    {
        const auto& facts = m_inputs.InstanceScreenFacts( key.inst ).items;

        for( const auto& [id, dangling] : island.value.dangling )
        {
            const ITEM_FACT* fact = dangling ? findFact( facts, id ) : nullptr;

            if( fact && fact->type == SCH_DIRECTIVE_LABEL_T )
                result.push_back( { m_keys.Instance( key.inst ), id, fact->ports.front().position } );
        }
    }

    return result;
}

std::vector<ERC_PIN_NET> ENGINE::PinNets() const
{
    wxASSERT( wxThread::IsMain() );
    std::map<NAME_ID, ERC_PIN_NET, NAME_LESS> nets( NAME_LESS{ &m_keys } );
    const auto netFor = [&]( const ITEM_KEY& key ) -> ERC_PIN_NET*
    {
        // Active pins and no-connect flags are all island members and must have published rows
        const auto& row = m_published.Rows().at( key );

        if( row.name == INVALID_ID )
            return nullptr;

        auto [it, inserted] = nets.try_emplace( row.name );

        if( inserted )
            it->second.name = m_keys.Name( row.name );

        return &it->second;
    };

    for( INST_ID inst : islandInstances( m_published.Auxiliary().Islands() ) )
    {
        for( const ITEM_FACT& fact : m_inputs.InstanceScreenFacts( inst ).items )
        {
            if( fact.type == SCH_NO_CONNECT_T )
            {
                if( auto* net = netFor( { fact.id, inst } ) )
                    net->noConnect = true;
            }

            for( const PIN_FACT& pin : fact.pins )
            {
                const auto* text = m_inputs.Text( { pin.id, inst } );
                auto* net = text ? netFor( { pin.id, inst } ) : nullptr;

                if( !net )
                    continue;

                net->noConnect |= pin.type == ELECTRICAL_PINTYPE::PT_NC;
                net->powerDriven |= pin.type == ELECTRICAL_PINTYPE::PT_POWER_OUT;
                net->pins.push_back( { m_keys.Instance( inst ), fact.owner, text->value.reference, pin } );
            }
        }
    }

    return moveValues( nets );
}

std::vector<MULTI_UNIT_CONFLICT> ENGINE::MultiUnitPinConflicts() const
{
    wxASSERT( wxThread::IsMain() );
    struct PIN_ON_NET
    {
        LABEL_LOCATION location;
        NAME_ID        net;
    };
    std::map<std::pair<std::string, std::string>, std::vector<PIN_ON_NET>> groups;

    for( INST_ID inst : islandInstances( m_published.Auxiliary().Islands() ) )
    {
        for( const ITEM_FACT& fact : m_inputs.InstanceScreenFacts( inst ).items )
        {
            if( !fact.multiUnit )
                continue;

            for( const PIN_FACT& pin : fact.pins )
            {
                const auto* text = m_inputs.Text( { pin.id, inst } );

                if( !text )
                    continue;

                const auto& row = m_published.Rows().at( { pin.id, inst } );

                if( row.name == INVALID_ID )
                    continue;

                groups[{ text->value.reference.utf8_string(), pin.shownNumber.utf8_string() }].push_back(
                        { { m_keys.Instance( inst ), pin.id, pin.position }, row.name } );
            }
        }
    }

    std::vector<MULTI_UNIT_CONFLICT> result;

    for( auto& [key, pins] : groups )
    {
        std::ranges::sort( pins, [&]( const PIN_ON_NET& a, const PIN_ON_NET& b )
        {
            if( a.net != b.net )
                return m_keys.NameLess( a.net, b.net );

            if( a.location.item != b.location.item )
                return a.location.item < b.location.item;

            // Tuple comparison would select vector's lexical ordering for KIID_PATH
            return a.location.sheet < b.location.sheet;
        } );
        const PIN_ON_NET& first = pins.front();
        const auto other = std::ranges::find_if( pins, [&]( const PIN_ON_NET& pin ) { return pin.net != first.net; } );

        if( other != pins.end() )
        {
            result.push_back( { first.location, other->location, wxString::FromUTF8( key.second ),
                                m_keys.Name( first.net ), m_keys.Name( other->net ) } );
        }
    }

    return result;
}

std::vector<NAMED_ITEM> ENGINE::NamedItems() const
{
    wxASSERT( wxThread::IsMain() );
    std::vector<NAMED_ITEM> result;
    const auto unescapeNetName = []( wxString name )
    {
        // CTX_NETNAME only escapes slashes; other brace tokens are literal name text
        name.Replace( wxS( "{slash}" ), wxS( "/" ) );
        return name;
    };

    for( INST_ID inst : islandInstances( m_published.Auxiliary().Islands() ) )
    {
        const KIID_PATH& path = m_keys.Instance( inst );

        for( const ITEM_FACT& fact : m_inputs.InstanceScreenFacts( inst ).items )
        {
            if( isLabel( fact.type ) )
            {
                const auto* text = m_inputs.Text( { fact.id, inst } );
                result.push_back( { path, fact.id, fact.ports.front().position,
                                    unescapeNetName( text->value.name ), fact.type, fact.type == SCH_GLOBAL_LABEL_T } );
            }

            for( const PIN_FACT& pin : fact.pins )
            {
                if( !pin.globalPower && !pin.localPower )
                    continue;

                // Pins from unselected units have no per-instance text entry
                if( const auto* text = m_inputs.Text( { pin.id, inst } ) )
                {
                    result.push_back( { path, pin.id, pin.position, unescapeNetName( text->value.name ),
                                        SCH_PIN_T, pin.globalPower } );
                }
            }
        }
    }

    return result;
}

std::vector<LABEL_LOCATION> ENGINE::SingleGlobalLabels() const
{
    wxASSERT( wxThread::IsMain() );
    std::map<std::string, std::optional<LABEL_LOCATION>> labels;

    for( INST_ID inst : islandInstances( m_published.Auxiliary().Islands() ) )
    {
        for( const ITEM_FACT& fact : m_inputs.InstanceScreenFacts( inst ).items )
        {
            if( fact.type != SCH_GLOBAL_LABEL_T )
                continue;

            const wxString& name = m_inputs.Text( { fact.id, inst } )->value.name;
            auto [entry, inserted] = labels.try_emplace( name.utf8_string(),
                    LABEL_LOCATION{ m_keys.Instance( inst ), fact.id, fact.ports.front().position } );

            if( !inserted )
                entry->second.reset();
        }
    }

    std::vector<LABEL_LOCATION> result;

    for( const auto& [name, label] : labels )
    {
        if( label )
            result.push_back( *label );
    }

    return result;
}

std::vector<HIERARCHY_ERROR> ENGINE::HierarchyErrors() const
{
    wxASSERT( wxThread::IsMain() );
    using ERROR_KIND = HIERARCHY_ERROR::KIND;
    struct PORTS
    {
        std::map<wxString, std::vector<const ITEM_FACT*>>                  labels;
        std::map<std::pair<KIID, wxString>, std::vector<const ITEM_FACT*>> pins;
    };
    std::map<INST_ID, PORTS> instances;
    std::vector<HIERARCHY_ERROR> result;
    const auto& auxiliary = m_published.Auxiliary();

    for( INST_ID inst : islandInstances( auxiliary.Islands() ) )
    {
        PORTS& ports = instances[inst];

        for( const ITEM_FACT& fact : m_inputs.InstanceScreenFacts( inst ).items )
        {
            if( fact.type != SCH_HIER_LABEL_T && fact.type != SCH_SHEET_PIN_T )
                continue;

            const wxString& name = m_inputs.Text( { fact.id, inst } )->value.name;

            if( fact.type == SCH_HIER_LABEL_T )
            {
                ports.labels[name].push_back( &fact );
            }
            else
            {
                ports.pins[{ fact.owner, name }].push_back( &fact );
                const auto& record = auxiliary.IslandOf().at( { fact.id, inst } );

                if( auxiliary.Islands().at( record ).value.dangling.at( fact.id ) != 0 )
                {
                    result.push_back( { m_keys.Instance( inst ), fact.id, fact.ports.front().position,
                                        name, ERROR_KIND::DANGLING_PIN, {} } );
                }
            }
        }
    }

    const auto mismatch = [&]( const KIID_PATH& path, const std::vector<const ITEM_FACT*>& items,
                               const wxString& name, ERROR_KIND kind )
    {
        const ITEM_FACT& first = *items.front();
        HIERARCHY_ERROR error{ path, first.id, first.ports.front().position, name, kind, {} };

        for( const ITEM_FACT* item : items )
            error.equivalentItems.push_back( item->id );

        result.push_back( std::move( error ) );
    };

    for( const auto& [inst, ports] : instances )
    {
        const KIID_PATH& path = m_keys.Instance( inst );
        KIID_PATH parentPath = path;
        const KIID owner = parentPath.back();
        parentPath.pop_back();
        const auto parent = m_keys.FindInstance( parentPath );
        const bool hasParent = parent && m_inputs.FindInstance( *parent );

        const auto parentPorts = hasParent ? instances.find( *parent ) : instances.end();

        for( const auto& [name, labels] : ports.labels )
        {
            if( !hasParent )
            {
                for( const ITEM_FACT* label : labels )
                {
                    result.push_back( { path, label->id, label->ports.front().position, name,
                                        ERROR_KIND::ROOT_LABEL, {} } );
                }
            }
            else if( parentPorts == instances.end() || !parentPorts->second.pins.contains( { owner, name } ) )
            {
                mismatch( path, labels, name, ERROR_KIND::MISSING_PIN );
            }
        }

        for( const auto& [key, pins] : ports.pins )
        {
            KIID_PATH childPath = path;
            childPath.push_back( key.first );
            const auto child = m_keys.FindInstance( childPath );
            const auto found = child ? instances.find( *child ) : instances.end();

            if( found == instances.end() || !found->second.labels.contains( key.second ) )
                mismatch( path, pins, key.second, ERROR_KIND::MISSING_LABEL );
        }
    }

    std::ranges::sort( result, []( const auto& a, const auto& b )
    {
        if( a.sheet != b.sheet )
            return a.sheet < b.sheet;

        // Keep KIID_PATH out of tuple comparisons, which select vector's lexical ordering
        return std::tie( a.item, a.kind ) < std::tie( b.item, b.kind );
    } );
    return result;
}
} // namespace SCH_CONNECTIVITY
