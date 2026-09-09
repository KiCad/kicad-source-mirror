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
#include <memory_resource>
#include <vector>
#include <unordered_map>
#include <wx/string.h>

class SCH_ITEM;

namespace SCH_CONNECTIVITY
{
/** Immediate-use netchain input; shared-screen items are qualified by their instance. */
struct NETCHAIN_INPUT
{
    struct NET
    {
        wxString name;
        wxString key;
    };

    struct SHEET
    {
        SHEET( const SCH_SHEET_PATH& aPath, std::pmr::memory_resource* aStorage ) :
                path( aPath ), nets( aStorage )
        {
        }

        SCH_SHEET_PATH path;
        std::pmr::unordered_map<const SCH_ITEM*, NET> nets;

        const NET* Find( const SCH_ITEM* aItem ) const
        {
            const auto item = nets.find( aItem );
            return item == nets.end() ? nullptr : &item->second;
        }

        const wxString& Key( const SCH_ITEM* aItem ) const
        {
            static const wxString missing;
            const NET*            net = Find( aItem );
            return net ? net->key : missing;
        }
    };

    // The maps must destroy their strings before their shared allocation storage is released.
    std::pmr::monotonic_buffer_resource storage;
    std::vector<SHEET> sheets;
};
}
