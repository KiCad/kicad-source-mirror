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

#include "conn_records.h"
#include "conn_changes.h"

#include <set>

namespace SCH_CONNECTIVITY
{
struct ISLAND_AUX
{
    std::vector<KIID>                  items;
    std::vector<std::pair<KIID, KIID>> adjacency;
    std::map<KIID, uint8_t>            dangling;
    std::vector<std::pair<KIID, KIID>> busEntryLinks;
    std::vector<std::pair<KIID, KIID>> ncContacts;
    ERC_ATOMS                          atoms;
};

struct RULE_AREA_RESULT
{
    std::vector<KIID>    containedItems;
    std::vector<KIID>    attachedDirectives;
    std::vector<NAME_ID> netclasses;
    bool                 operator==( const RULE_AREA_RESULT& ) const = default;
};

/**
 * Source-keyed publication independent of electrical component identity.
 */
class AUXILIARY
{
public:
    struct ISLAND_ENTRY
    {
        uint64_t   islandVersion = 0; ///< Geometry version of the screen islands.
        uint64_t   recordVersion = 0; ///< Version of the electrical record of the island.
        ISLAND_AUX value;
    };
    struct AREA_ENTRY
    {
        uint64_t         version = 0;
        RULE_AREA_RESULT value;
    };
    using ISLANDS = std::map<RECORD_KEY, ISLAND_ENTRY, KEY_LESS>;
    using AREAS = std::map<ITEM_KEY, AREA_ENTRY, KEY_LESS>;
    using ISLAND_INDEX = std::map<ITEM_KEY, RECORD_KEY, KEY_LESS>;
    using AREA_INDEX = std::map<ITEM_KEY, std::vector<KIID>, KEY_LESS>;

    using NEIGHBOR_INDEX = std::map<ITEM_KEY, std::vector<KIID>, KEY_LESS>;

    explicit AUXILIARY( SESSION_KEYS& aKeys );

    /**
     * Publish the changed islands and rule areas. Add their items, the items of new instances and
     * the items with changed sources or text to aChanges, then sort and deduplicate the item, island
     * and rule area lists.
     */
    void Update( std::span<const FRAME_INSTANCE> aFrame, const INPUT_STORE& aInputs,
                 const RECORD_STORE::RECORD_CACHE& aRecords, CHANGE_SET& aChanges );
    void Clear();

    const ISLANDS&                       Islands() const { return m_islands; }
    const AREAS&                         RuleAreas() const { return m_areas; }
    const ISLAND_INDEX&                  IslandOf() const { return m_islandOf; }
    const NEIGHBOR_INDEX&                NeighborsOf() const { return m_neighborsOf; }
    const AREA_INDEX&                    RuleAreasOf() const { return m_ruleAreasOf; }
    const std::map<SCREEN_ID, uint64_t>& ScreenRevisions() const { return m_screenRevisions; }

private:
    SESSION_KEYS&                 m_keys;
    ISLANDS                       m_islands;
    AREAS                         m_areas;
    ISLAND_INDEX                  m_islandOf;
    AREA_INDEX                    m_ruleAreasOf;
    NEIGHBOR_INDEX                m_neighborsOf;
    std::map<SCREEN_ID, uint64_t> m_screenRevisions;
    std::set<INST_ID>             m_instances;
    uint64_t                      m_sourceVersion = 0;
};
} // namespace SCH_CONNECTIVITY
