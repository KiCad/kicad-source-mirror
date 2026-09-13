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

#include "conn_claims.h"

namespace SCH_CONNECTIVITY
{
struct SUMMARY
{
    std::optional<CLAIM>                   best;
    std::array<std::optional<ITEM_KEY>, 2> pinWitnesses;
    std::optional<ITEM_KEY>                noConnect;
    std::vector<NAME_ID>                   netclasses;

    bool operator==( const SUMMARY& ) const = default;

    // Inputs are canonical folds from one source snapshot; each source has one claim.
    static SUMMARY Join( const SUMMARY& aLeft, const SUMMARY& aRight, const SESSION_KEYS& aKeys );
};

// Consumes a canonical BuildIslandRecord result; strong names and counts remain island ERC inputs.
SUMMARY MakeSummary( const RECORD_KEY& aKey, const ISLAND_RECORD& aRecord );
} // namespace SCH_CONNECTIVITY
