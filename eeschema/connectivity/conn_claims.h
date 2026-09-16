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
/**
 * Driver priority of a claim, from low to high. A higher value always names the net.
 */
enum class PRIORITY : uint8_t
{
    NONE,             ///< The item makes no claim.
    PIN,              ///< Symbol pin that is not a power input. The name is "Net-(...)".
    SHEET_PIN,        ///< Sheet pin. The name belongs to the child instance.
    HIER_LABEL,       ///< Hierarchical label. The first strong priority.
    LOCAL_LABEL,      ///< Local label.
    LOCAL_POWER_PIN,  ///< Power input pin of a local power symbol.
    BUS_MEMBER,       ///< Synthetic claim of a bus member slot, set by BindBundle().
    GLOBAL_POWER_PIN, ///< Power input pin of a global power symbol, or a hidden power input pin.
    GLOBAL            ///< Global label.
};

/**
 * The claim of one item for the name of its island.
 */
struct CLAIM
{
    PRIORITY                          priority = PRIORITY::NONE;
    uint16_t                          depth = 0;                 ///< Sheet path size, or zero if unscoped.
    NAME_ID                           path = INVALID_ID;         ///< Sheet prefix, or the empty name if unscoped.
    NAME_ID                           name = INVALID_ID;         ///< Item text without the sheet prefix.
    NAME_ID                           ncName = INVALID_ID;       ///< Name of an unconnected pin net.
    NAME_ID                           fullName = INVALID_ID;     ///< path followed by name.
    ITEM_KEY                          source;                    ///< The claiming item, the last tie-break.
    bool                              globalPowerParent = false; ///< Pin of a global power symbol.
    bool                              localPowerParent = false;  ///< Pin of a local power symbol.
    bool                              outputShape = false;       ///< Sheet pin with output shape.
    bool                              hasPad = false;            ///< The name contains "-Pad".
    std::shared_ptr<const BUS_SCHEMA> schema;                    ///< Parsed bus, or null for a signal.
    std::optional<INST_ID>            portInstance; ///< Sheet pin child. Members bind there, not in the parent.

    /** True for a name that the user placed. */
    bool Strong() const { return priority >= PRIORITY::HIER_LABEL; }
    bool operator==( const CLAIM& aOther ) const;
};

/**
 * Strict weak order on claims by value. The greater claim drives. SESSION_KEYS ids never decide.
 */
struct CLAIM_LESS
{
    const SESSION_KEYS& keys;
    bool                operator()( const CLAIM& aLeft, const CLAIM& aRight ) const;
};

struct SOURCE_CLAIMS
{
    KICAD_T                type = TYPE_NOT_INIT;
    KIID                   pinGroup = niluuid;
    bool                   pinWitness = false; ///< Pin whose symbol is not a power symbol.
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
    std::vector<NAME_ID>               strongNames;  ///< Sorted names of the strong claims.
    std::array<std::optional<KIID>, 2> pinWitnesses; ///< The two smallest witness pin ids.
    std::optional<KIID>                noConnect;    ///< The smallest no-connect marker id.
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
    std::vector<CLAIM>    claims; ///< Descending by CLAIM_LESS, so front() is the island driver.
    std::vector<NAME_KEY> edges;
    std::vector<NAME_ID>  netclasses;
    ERC_ATOMS             atoms;
    bool                  operator==( const ISLAND_RECORD& ) const = default;
};

/** Physical inputs to the island record fold. Auxiliary island outputs cannot affect records. */
struct RECORD_GEOMETRY
{
    std::span<const KIID> items;
    bool                  hasWire;
    bool                  hasBusLine;
};

/**
 * Main-thread interning and parse preparation, followed by a pure per-island fold.
 *
 * PrepareInstanceClaims() assigns the priority, scope and names of each claim.
 */
INSTANCE_CLAIMS PrepareInstanceClaims( const SCREEN_FACTS& aFacts, const INSTANCE_FACTS& aText,
                                       const INSTANCE_SCOPE& aScope, SESSION_KEYS& aKeys, BUS_PARSE_CACHE& aParses );
ISLAND_RECORD   BuildIslandRecord( const RECORD_GEOMETRY& aIsland, const INSTANCE_CLAIMS& aInputs,
                                   const SESSION_KEYS& aKeys );
} // namespace SCH_CONNECTIVITY
