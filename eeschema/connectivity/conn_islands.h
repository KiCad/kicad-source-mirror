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

#pragma once

#include <connectivity/conn_facts.h>
#include <map>

namespace SCH_CONNECTIVITY
{
struct PIN_GEOMETRY
{
    KIID               id = niluuid;
    VECTOR2I           position;
    int                unit = 0;
    ELECTRICAL_PINTYPE type = ELECTRICAL_PINTYPE::PT_UNSPECIFIED;
    bool               operator==( const PIN_GEOMETRY& ) const = default;
};

struct ITEM_GEOMETRY
{
    KIID                      id = niluuid;
    KICAD_T                   type = TYPE_NOT_INIT;
    KIID                      owner = niluuid;
    std::vector<PORT_FACT>    ports;
    std::optional<SEG>        segment;
    std::vector<PIN_GEOMETRY> pins;
    std::vector<KIID>         jumperedWith;
    bool                      operator==( const ITEM_GEOMETRY& ) const = default;
};

struct SCREEN_GEOMETRY
{
    std::vector<ITEM_GEOMETRY> items;
    std::vector<KIID>          attachedDirectives;
    bool                       operator==( const SCREEN_GEOMETRY& ) const = default;
};

SCREEN_GEOMETRY GeometryOf( const SCREEN_FACTS& aFacts );

struct ISLAND
{
    KIID                               anchor = niluuid;
    bool                               hasWire = false;
    bool                               hasBusLine = false;
    std::vector<KIID>                  vertices;
    std::vector<KIID>                  items;
    std::vector<std::pair<KIID, KIID>> adjacency;
    std::map<KIID, uint8_t>            dangling;
    std::vector<std::pair<KIID, KIID>> busEntryLinks;
    std::vector<std::pair<KIID, KIID>> ncContacts;
    bool                               operator==( const ISLAND& ) const = default;
};

struct SCREEN_ISLANDS
{
    std::vector<ISLAND> islands;
    bool                operator==( const SCREEN_ISLANDS& ) const = default;
};

/**
 * Kindless geometry for one unit signature. Vertices retain fact keys; items retain active
 * original keys, including every member of grouped pins. Bus-line evidence includes bus-to-bus
 * entries. Unit zero selects every unit. Dangling bits follow port order
 * (start = 1, end = 2); pin members each have their own single-port state.
 * Adjacency and NC pairs are undirected and ordered; bus links are (entry, bus item).
 */
SCREEN_ISLANDS BuildScreenIslands( const SCREEN_GEOMETRY& aGeometry, const std::vector<std::pair<KIID, int>>& aUnits );
} // namespace SCH_CONNECTIVITY
