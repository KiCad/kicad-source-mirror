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

#include "conn_keys.h"

namespace SCH_CONNECTIVITY
{
/** Difference between the previous and the current net name to netclass map. */
struct NETCLASS_DELTA
{
    /** Names whose netclass list is new or different, in name order, with the new list. */
    std::vector<std::pair<NAME_ID, std::vector<NAME_ID>>> assigned;
    std::vector<NAME_ID>                                  removed; ///< Names that no longer have netclasses.
    bool                                                  Empty() const { return assigned.empty() && removed.empty(); }
};

/**
 * Difference between two publications. PUBLICATION::Update() makes a new change set on each
 * update. FACADE::Recalculate() sends a copy to subscribers, with display state changes added to
 * changedItems.
 */
struct CHANGE_SET
{
    /** Old and new name of each one to one succession group whose name changed. */
    std::vector<std::pair<NAME_ID, NAME_ID>>              renamedNets;
    std::vector<std::pair<std::vector<NAME_ID>, NAME_ID>> mergedNets; ///< Old names and the one new name.
    std::vector<std::pair<NAME_ID, std::vector<NAME_ID>>> splitNets;  ///< One old name and the new names.
    std::vector<NAME_ID>                                  netsAdded;  ///< New names of other succession groups.
    std::vector<NAME_ID>                                  netsRemoved; ///< Old names of other succession groups.

    /**
     * Old and current names whose membership or presentation inputs changed, with their bus
     * dependents. The net navigator rebuilds only these nodes.
     */
    std::vector<NAME_ID>                                  netsChanged;

    /** Items whose row, island, rule area, source or text changed. The facade adds display changes. */
    std::vector<ITEM_KEY>   changedItems;
    std::vector<ITEM_KEY>   driverChangedItems; ///< Items whose ITEM_RESULT::driver changed or was removed.
    NETCLASS_DELTA          netclasses;
    std::vector<RECORD_KEY> changedIslands;   ///< Removed, replaced and new island keys.
    std::vector<ITEM_KEY>   changedRuleAreas; ///< Removed, replaced and new rule area keys.

    bool Empty() const
    {
        return changedIslands.empty() && changedRuleAreas.empty() && changedItems.empty() && driverChangedItems.empty()
               && netclasses.Empty() && renamedNets.empty() && mergedNets.empty() && splitNets.empty()
               && netsAdded.empty() && netsRemoved.empty() && netsChanged.empty();
    }
};

} // namespace SCH_CONNECTIVITY
