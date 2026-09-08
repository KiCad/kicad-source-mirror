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

#include <sch_sheet_path.h>
#include <unordered_map>
#include <vector>

class SCH_ITEM;

namespace SCH_CONNECTIVITY
{
/** Immediate-use input for net-chain inference. */
struct NETCHAIN_INPUT
{
    struct NET
    {
        wxString name;
        wxString key;
        long component;
    };

    SCH_SHEET_LIST sheets;
    std::vector<SCH_ITEM*> items;
    std::unordered_map<const SCH_ITEM*, NET> nets;

    const NET* Find( const SCH_ITEM* aItem ) const
    {
        const auto item = nets.find( aItem );
        return item == nets.end() ? nullptr : &item->second;
    }
};
}
