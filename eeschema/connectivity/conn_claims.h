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

#include "conn_bus.h"
#include "conn_islands.h"
#include "conn_keys.h"
#include "conn_frame.h"

#include <array>
#include <memory>
#include <span>

namespace SCH_CONNECTIVITY
{
enum class PRIORITY : uint8_t
{
    NONE,
    PIN,
    SHEET_PIN,
    HIER_LABEL,
    LOCAL_LABEL,
    LOCAL_POWER_PIN,
    BUS_MEMBER,
    GLOBAL_POWER_PIN,
    GLOBAL
};

struct CLAIM
{
    PRIORITY                          priority = PRIORITY::NONE;
    uint16_t                          depth = 0;
    NAME_ID                           path = INVALID_ID;
    NAME_ID                           name = INVALID_ID;
    NAME_ID                           ncName = INVALID_ID;
    NAME_ID                           fullName = INVALID_ID;
    ITEM_KEY                          source;
    bool                              globalPowerParent = false;
    bool                              localPowerParent = false;
    bool                              outputShape = false;
    bool                              hasPad = false;
    std::shared_ptr<const BUS_SCHEMA> schema;
    // Sheet pins bind member names in the child instance, never the parent namespace.
    std::optional<INST_ID>            portInstance;

    bool Strong() const { return priority >= PRIORITY::HIER_LABEL; }
    bool operator==( const CLAIM& aOther ) const;
};

struct CLAIM_LESS
{
    const SESSION_KEYS& keys;
    bool                operator()( const CLAIM& aLeft, const CLAIM& aRight ) const;
};

struct SOURCE_CLAIMS
{
    KICAD_T                type = TYPE_NOT_INIT;
    KIID                   pinGroup = niluuid;
    bool                   pinWitness = false;
    bool                   invisiblePower = false;
    std::optional<INST_ID> portInstance;
    std::optional<CLAIM>   claim;
    std::vector<NAME_ID>   netclasses;
    bool                   operator==( const SOURCE_CLAIMS& ) const = default;
};

struct INSTANCE_CLAIMS
{
    INST_ID                       instance = INVALID_ID;
    std::map<KIID, SOURCE_CLAIMS> items;
};

struct ERC_ATOMS
{
    std::vector<NAME_ID>               strongNames;
    std::array<std::optional<KIID>, 2> pinWitnesses;
    std::optional<KIID>                noConnect;
    uint32_t                           pinCount = 0;
    bool                               hasSymbolPin = false;
    bool                               kindConflict = false;
    bool                               mixedBusShapes = false;
    bool                               invisiblePowerWired = false;
    bool                               busNoConnect = false;
    bool                               operator==( const ERC_ATOMS& ) const = default;
};

struct ISLAND_RECORD
{
    KIND                  kind = KIND::SIGNAL;
    std::vector<KIID>     items;
    std::vector<CLAIM>    claims;
    std::vector<NAME_KEY> edges;
    std::vector<NAME_ID>  netclasses;
    ERC_ATOMS             atoms;
    bool                  operator==( const ISLAND_RECORD& ) const = default;
};

// Physical inputs to the island record fold; auxiliary island outputs cannot affect records.
struct RECORD_GEOMETRY
{
    std::span<const KIID> items;
    bool                  hasWire;
    bool                  hasBusLine;
};

// Main-thread interning and parse preparation, followed by a pure per-island fold.
INSTANCE_CLAIMS PrepareInstanceClaims( const SCREEN_FACTS& aFacts, const INSTANCE_FACTS& aText,
                                       const INSTANCE_SCOPE& aScope, SESSION_KEYS& aKeys, BUS_PARSE_CACHE& aParses );
ISLAND_RECORD   BuildIslandRecord( const RECORD_GEOMETRY& aIsland, const INSTANCE_CLAIMS& aInputs,
                                   const SESSION_KEYS& aKeys );
} // namespace SCH_CONNECTIVITY
