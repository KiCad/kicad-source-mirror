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
struct NETCLASS_DELTA
{
    std::vector<std::pair<NAME_ID, std::vector<NAME_ID>>> assigned;
    std::vector<NAME_ID>                                  removed;
    bool                                                  Empty() const { return assigned.empty() && removed.empty(); }
};

struct CHANGE_SET
{
    std::vector<std::pair<NAME_ID, NAME_ID>>              renamedNets;
    std::vector<std::pair<std::vector<NAME_ID>, NAME_ID>> mergedNets;
    std::vector<std::pair<NAME_ID, std::vector<NAME_ID>>> splitNets;
    std::vector<NAME_ID>                                  netsAdded;
    std::vector<NAME_ID>                                  netsRemoved;
    // Old and current names whose membership or presentation inputs changed.
    std::vector<NAME_ID>                                  netsChanged;

    std::vector<ITEM_KEY>   changedItems;
    std::vector<ITEM_KEY>   driverChangedItems;
    NETCLASS_DELTA          netclasses;
    std::vector<RECORD_KEY> changedIslands;
    std::vector<ITEM_KEY>   changedRuleAreas;

    bool Empty() const
    {
        return changedIslands.empty() && changedRuleAreas.empty() && changedItems.empty() && driverChangedItems.empty()
               && netclasses.Empty() && renamedNets.empty() && mergedNets.empty() && splitNets.empty()
               && netsAdded.empty() && netsRemoved.empty() && netsChanged.empty();
    }
};

} // namespace SCH_CONNECTIVITY
