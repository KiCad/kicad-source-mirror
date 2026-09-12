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

#include <connectivity/conn_islands.h>
#include <core/union_find.h>
#include <geometry/rtree/packed_rtree.h>

#include <algorithm>
#include <array>
#include <cassert>
#include <limits>
#include <stdexcept>
#include <unordered_map>

namespace SCH_CONNECTIVITY
{
namespace
{
    enum CONTACT_KIND
    {
        ANCHOR,
        WIRE,
        BUS,
        ENTRY,
        ENTRY_BUS,
        NC,
        BUS_SIDE,
        KIND_COUNT
    };

    // clang-format off
    constexpr bool PROPAGATES[KIND_COUNT][KIND_COUNT] = {
        { true,  true,  true,  true,  true,  false, false },
        { true,  true,  false, true,  false, false, false },
        { true,  false, true,  false, true,  false, false },
        { true,  true,  false, false, false, false, false },
        { true,  false, true,  false, true,  false, false },
        { false, false, false, false, false, false, false },
        { false, false, false, false, false, false, false }
    };
    // clang-format on

    static_assert(
            []
            {
                for( size_t i = 0; i < KIND_COUNT; ++i )
                {
                    for( size_t j = 0; j < KIND_COUNT; ++j )
                    {
                        if( PROPAGATES[i][j] != PROPAGATES[j][i] )
                            return false;
                    }
                }

                return true;
            }(),
            "Connectivity propagation must be symmetric" );

    struct PORT
    {
        size_t       vertex;
        KIID         item;
        CONTACT_KIND kind;
        uint8_t      bit;
    };

    struct POINT
    {
        VECTOR2I          position;
        std::vector<PORT> ports;
    };

    int Bound( int aCoordinate, int aMargin )
    {
        return int( std::clamp<int64_t>( int64_t( aCoordinate ) + aMargin, std::numeric_limits<int>::min(),
                                         std::numeric_limits<int>::max() ) );
    }

    uint64_t PointKey( VECTOR2I aPoint )
    {
        return ( uint64_t( uint32_t( aPoint.x ) ) << 32 ) | uint32_t( aPoint.y );
    }

    template <typename T>
    void SortUnique( std::vector<T>& aValues )
    {
        std::sort( aValues.begin(), aValues.end() );
        aValues.erase( std::unique( aValues.begin(), aValues.end() ), aValues.end() );
    }

    bool IsLabel( KICAD_T aType )
    {
        return aType == SCH_LABEL_T || aType == SCH_GLOBAL_LABEL_T || aType == SCH_HIER_LABEL_T
               || aType == SCH_DIRECTIVE_LABEL_T || aType == SCH_SHEET_PIN_T;
    }

    bool ClosesDangling( const ITEM_GEOMETRY& aItem, const ITEM_GEOMETRY& aOther )
    {
        if( aItem.type == SCH_PIN_T )
        {
            if( aOther.type == SCH_PIN_T && aItem.owner == aOther.owner )
                return false;

            return aOther.type == SCH_PIN_T || IsLabel( aOther.type ) || aOther.type == SCH_NO_CONNECT_T
                   || aOther.type == SCH_JUNCTION_T
                   || ( aOther.segment && aOther.ports.front().kind == PORT_KIND::WIRE );
        }

        if( aItem.segment )
        {
            if( aItem.ports.front().kind == PORT_KIND::WIRE )
            {
                return aOther.type != SCH_BUS_BUS_ENTRY_T
                       && !( aOther.segment && aOther.ports.front().kind == PORT_KIND::BUS );
            }

            return aOther.type != SCH_PIN_T && !( aOther.segment && aOther.ports.front().kind == PORT_KIND::WIRE );
        }

        if( IsLabel( aItem.type ) )
        {
            return aOther.type == SCH_PIN_T || aOther.type == SCH_NO_CONNECT_T || IsLabel( aOther.type )
                   || aOther.segment.has_value();
        }

        return false;
    }

    CONTACT_KIND Kind( PORT_KIND aKind )
    {
        switch( aKind )
        {
        case PORT_KIND::ANCHOR: return ANCHOR;
        case PORT_KIND::WIRE: return WIRE;
        case PORT_KIND::BUS: return BUS;
        case PORT_KIND::ENTRY: return ENTRY;
        case PORT_KIND::ENTRY_BUS: return ENTRY_BUS;
        }

        return ANCHOR;
    }
} // namespace

SCREEN_GEOMETRY GeometryOf( const SCREEN_FACTS& aFacts )
{
    SCREEN_GEOMETRY result;
    result.items.reserve( aFacts.items.size() );

    for( const ITEM_FACT& fact : aFacts.items )
    {
        ITEM_GEOMETRY item;
        item.id = fact.id;
        item.type = fact.type;
        item.owner = fact.owner;
        item.ports = fact.ports;
        item.segment = fact.segment;
        item.jumperedWith = fact.jumperedWith;
        item.pins.reserve( fact.pins.size() );

        for( const PIN_FACT& pin : fact.pins )
            item.pins.push_back( { pin.id, pin.position, pin.unit, pin.type } );

        result.items.push_back( std::move( item ) );
    }

    for( const RULE_AREA_FACT& area : aFacts.ruleAreas )
    {
        result.attachedDirectives.insert( result.attachedDirectives.end(), area.attachedDirectives.begin(),
                                          area.attachedDirectives.end() );
    }

    SortUnique( result.attachedDirectives );
    return result;
}

SCREEN_ISLANDS BuildScreenIslands( const SCREEN_GEOMETRY& aFacts, const std::vector<std::pair<KIID, int>>& aUnits )
{
    std::map<KIID, int>                    units( aUnits.begin(), aUnits.end() );
    std::vector<const ITEM_GEOMETRY*>      vertices;
    std::vector<std::vector<KIID>>         members;
    std::unordered_map<KIID, size_t>       byId;
    std::unordered_map<uint64_t, POINT>    points;
    std::unordered_map<KIID, uint8_t>      dangling;
    KIRTREE::PACKED_RTREE<size_t>::Builder lines;

    auto addPort = [&]( size_t aVertex, const KIID& aId, VECTOR2I aPosition, CONTACT_KIND aKind, uint8_t aBit )
    {
        POINT& point = points[PointKey( aPosition )];
        point.position = aPosition;
        point.ports.push_back( { aVertex, aId, aKind, aBit } );
        dangling[aId] |= aBit;
    };

    for( const ITEM_GEOMETRY& fact : aFacts.items )
    {
        assert( fact.ports.size() <= 2 && ( !fact.segment || fact.ports.size() == 2 ) );
        size_t            index = vertices.size();
        std::vector<KIID> active;

        if( fact.type == SCH_PIN_T )
        {
            auto unit = units.find( fact.owner );

            if( unit == units.end() )
                throw std::invalid_argument( "Missing symbol unit in connectivity geometry" );

            for( const PIN_GEOMETRY& pin : fact.pins )
            {
                if( pin.unit != 0 && unit->second != 0 && pin.unit != unit->second )
                    continue;

                active.push_back( pin.id );
                addPort( index, pin.id, pin.position, pin.type == ELECTRICAL_PINTYPE::PT_NC ? NC : ANCHOR, 1 );

                if( pin.type == ELECTRICAL_PINTYPE::PT_NC || pin.type == ELECTRICAL_PINTYPE::PT_NIC )
                    dangling[pin.id] = 0;
            }
        }
        else
        {
            active.push_back( fact.id );

            for( size_t port = 0; port < fact.ports.size(); ++port )
                addPort( index, fact.id, fact.ports[port].position, Kind( fact.ports[port].kind ),
                         uint8_t( 1 << port ) );

            if( fact.type == SCH_JUNCTION_T || fact.type == SCH_NO_CONNECT_T )
                dangling[fact.id] = 0;
        }

        if( active.empty() )
            continue;

        vertices.push_back( &fact );
        SortUnique( active );
        members.push_back( std::move( active ) );
        byId.emplace( fact.id, index );

        if( fact.segment )
        {
            const SEG& segment = *fact.segment;
            // SEG::Contains allows a small rounding tolerance; the broadphase must contain it
            int min[2] = { Bound( std::min( segment.A.x, segment.B.x ), -2 ),
                           Bound( std::min( segment.A.y, segment.B.y ), -2 ) };
            int max[2] = { Bound( std::max( segment.A.x, segment.B.x ), 2 ),
                           Bound( std::max( segment.A.y, segment.B.y ), 2 ) };
            lines.Add( min, max, index );
        }
    }

    auto                               tree = lines.Build();
    KI_UNION_FIND                      sets( vertices.size() );
    std::vector<std::array<bool, 2>>   entryWire( vertices.size() );
    std::vector<std::array<bool, 2>>   entryBus( vertices.size() );
    std::vector<std::pair<KIID, KIID>> adjacency;
    std::vector<std::pair<KIID, KIID>> ncContacts;
    std::vector<std::pair<KIID, KIID>> busLinks;

    auto entryContact = [&]( const PORT& aEntry, const PORT& aOther )
    {
        if( aEntry.kind != ENTRY && aEntry.kind != BUS_SIDE && aEntry.kind != ENTRY_BUS )
            return;

        size_t end = aEntry.bit == 2 ? 1 : 0;

        if( aOther.kind == WIRE )
            entryWire[aEntry.vertex][end] = true;

        if( aOther.kind == BUS || aOther.kind == ENTRY_BUS )
        {
            entryBus[aEntry.vertex][end] = true;

            if( aEntry.kind != ENTRY_BUS )
                busLinks.emplace_back( aEntry.item, aOther.item );
        }
    };

    for( auto& [key, point] : points )
    {
        int  position[2] = { point.position.x, point.position.y };
        auto visitor = [&]( size_t aIndex )
        {
            const ITEM_GEOMETRY& fact = *vertices[aIndex];

            if( fact.segment->Contains( point.position ) && point.position != fact.segment->A
                && point.position != fact.segment->B )
            {
                point.ports.push_back( { aIndex, fact.id, Kind( fact.ports.front().kind ), 0 } );
            }

            return true;
        };
        tree.Search( position, position, visitor );
        bool hasBus = std::any_of( point.ports.begin(), point.ports.end(),
                                   []( const PORT& aPort )
                                   {
                                       return aPort.kind == BUS || aPort.kind == ENTRY_BUS;
                                   } );

        for( PORT& port : point.ports )
        {
            if( port.kind == ENTRY && hasBus )
                port.kind = BUS_SIDE;
            else if( vertices[port.vertex]->type == SCH_JUNCTION_T && hasBus )
                port.kind = BUS;
        }

        for( size_t first = 0; first < point.ports.size(); ++first )
        {
            const PORT& a = point.ports[first];

            for( size_t second = first + 1; second < point.ports.size(); ++second )
            {
                const PORT& b = point.ports[second];

                if( a.vertex == b.vertex )
                    continue;

                auto pair = std::minmax( a.item, b.item );

                if( ClosesDangling( *vertices[a.vertex], *vertices[b.vertex] ) )
                    dangling[a.item] &= ~a.bit;

                if( ClosesDangling( *vertices[b.vertex], *vertices[a.vertex] ) )
                    dangling[b.item] &= ~b.bit;

                if( a.kind == NC || b.kind == NC )
                {
                    ncContacts.emplace_back( pair.first, pair.second );
                    continue;
                }

                entryContact( a, b );
                entryContact( b, a );

                if( PROPAGATES[a.kind][b.kind] )
                {
                    sets.Unite( a.vertex, b.vertex );
                    adjacency.emplace_back( pair.first, pair.second );
                }
            }
        }
    }

    for( size_t index = 0; index < vertices.size(); ++index )
    {
        const ITEM_GEOMETRY&     fact = *vertices[index];
        const std::vector<KIID>& merged = members[index];

        // Pins merged by number are stacked or jumpered, so each touches the others wherever they sit
        for( size_t first = 0; first < merged.size(); ++first )
        {
            for( size_t second = first + 1; second < merged.size(); ++second )
                adjacency.emplace_back( merged[first], merged[second] );
        }

        for( const KIID& other : fact.jumperedWith )
        {
            auto found = byId.find( other );

            if( found != byId.end() && vertices[found->second]->owner == fact.owner )
            {
                sets.Unite( index, found->second );

                for( const KIID& first : merged )
                {
                    for( const KIID& second : members[found->second] )
                    {
                        auto pair = std::minmax( first, second );
                        adjacency.emplace_back( pair.first, pair.second );
                    }
                }
            }
        }

        if( fact.type == SCH_BUS_WIRE_ENTRY_T )
        {
            const auto& wire = entryWire[index];
            const auto& bus = entryBus[index];
            uint8_t     state = 3;

            if( ( wire[0] && bus[1] ) || ( wire[1] && bus[0] ) )
                state = 0;
            else if( wire[0] || bus[0] )
                state = 2;
            else if( wire[1] || bus[1] )
                state = 1;

            dangling[fact.id] = state;
        }
        else if( fact.type == SCH_BUS_BUS_ENTRY_T )
        {
            const auto& bus = entryBus[index];
            dangling[fact.id] = ( bus[0] ? 0 : 1 ) | ( bus[1] ? 0 : 2 );
        }
    }

    for( const KIID& directive : aFacts.attachedDirectives )
    {
        if( auto found = dangling.find( directive ); found != dangling.end() )
            found->second = 0;
    }

    SCREEN_ISLANDS                   result;
    std::map<size_t, size_t>         roots;
    std::unordered_map<KIID, size_t> islandOf;

    for( size_t index = 0; index < vertices.size(); ++index )
    {
        auto [root, inserted] = roots.emplace( sets.FindCompress( index ), result.islands.size() );

        if( inserted )
            result.islands.emplace_back();

        ISLAND&              island = result.islands[root->second];
        const ITEM_GEOMETRY& fact = *vertices[index];
        island.vertices.push_back( fact.id );
        island.items.insert( island.items.end(), members[index].begin(), members[index].end() );
        island.hasWire |= fact.segment && fact.ports.front().kind == PORT_KIND::WIRE;
        island.hasBusLine |=
                fact.type == SCH_BUS_BUS_ENTRY_T || ( fact.segment && fact.ports.front().kind == PORT_KIND::BUS );

        for( const KIID& item : members[index] )
        {
            islandOf.emplace( item, root->second );
            island.dangling[item] = dangling[item];
        }
    }

    auto distribute = [&]( auto& aPairs, auto aMember )
    {
        SortUnique( aPairs );

        for( const auto& pair : aPairs )
        {
            size_t first = islandOf.at( pair.first );
            size_t second = islandOf.at( pair.second );
            ( result.islands[first].*aMember ).push_back( pair );

            if( first != second )
                ( result.islands[second].*aMember ).push_back( pair );
        }
    };
    distribute( adjacency, &ISLAND::adjacency );
    distribute( ncContacts, &ISLAND::ncContacts );
    distribute( busLinks, &ISLAND::busEntryLinks );

    for( ISLAND& island : result.islands )
    {
        SortUnique( island.items );
        SortUnique( island.vertices );
        island.anchor = island.items.front();
    }

    std::ranges::sort( result.islands, std::less<KIID>(), &ISLAND::anchor );
    return result;
}
} // namespace SCH_CONNECTIVITY
