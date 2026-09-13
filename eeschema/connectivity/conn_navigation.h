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
#include <map>
#include <set>
#include <unordered_set>
#include <vector>

class CONNECTION_SUBGRAPH;
class SCHEMATIC;
class SCH_ITEM;

namespace SCH_CONNECTIVITY
{
using NET_ITEMS_BY_SHEET = std::map<SCH_SHEET_PATH, std::vector<SCH_ITEM*>, SHEET_PATH_CMP>;

/** Net membership for one UI operation. Recreate after the schematic changes.
 * Returned item pointers are borrowed for immediate use.
 */
class NAVIGATION_QUERY
{
public:
    explicit NAVIGATION_QUERY( const SCHEMATIC& aSchematic );
    std::vector<wxString> NetNames() const;
    /** Signal itself or leaf signals of a bus, suitable for PCB cross-probing. */
    std::vector<wxString> SignalNames( const wxString& aName ) const;
    bool HasNet( const wxString& aName ) const;
    std::set<KIID_PATH> NetSheets( const wxString& aName ) const;
    NET_ITEMS_BY_SHEET NetItems( const wxString& aName, bool aIncludeBusParents,
                                 bool aIncludeBusMembers = false ) const;
    /** Add the items of a net on one sheet to \a aItems without building other sheets. */
    void CollectNetItems( const wxString& aName, const SCH_SHEET_PATH& aSheet,
                          std::unordered_set<SCH_ITEM*>& aItems, bool aIncludeBusParents = false,
                          bool aIncludeBusMembers = false ) const;
    /** Whole nets containing the seeds on this instance; excludes bus parents and members. */
    std::vector<SCH_ITEM*> WholeNetItems( const std::vector<SCH_ITEM*>& aSeeds,
                                        const SCH_SHEET_PATH& aSheet ) const;

private:
    std::set<const CONNECTION_SUBGRAPH*> netSubgraphs( const wxString& aName, bool aIncludeBusParents,
                                                       bool aIncludeBusMembers ) const;

    const SCHEMATIC& m_schematic;
};
}
