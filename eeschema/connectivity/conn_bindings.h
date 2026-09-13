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

#include "conn_components.h"
#include "conn_component_cache.h"

namespace SCH_CONNECTIVITY
{
struct SLOT_INPUT
{
    SLOT_KEY              key;
    CLAIM                 claim;
    NAME_ID               localName = INVALID_ID;
    std::vector<NAME_KEY> edges;

    // Parent-sheet names from a driving sheet pin, which join only names a signal record already claims.
    std::vector<NAME_KEY> parentEdges;
    std::vector<NAME_ID>  parentNetclasses;
    NODE_ID               parentBundle = INVALID_ID;
    bool                  operator==( const SLOT_INPUT& ) const = default;
};

struct BUNDLE_CLAIM
{
    struct LEAF_NAME
    {
        NAME_ID name;
        NAME_ID fullName;
        NAME_ID localName;
    };

    RECORD_KEY             record;
    CLAIM                  claim;
    std::vector<LEAF_NAME> leaves;

    // The claim drives its own island, which lets a sheet pin name members in the parent sheet.
    bool                   drivesRecord = false;
};

struct BUNDLE_INPUT
{
    NODE_ID                   parent = INVALID_ID;
    std::vector<ITEM_KEY>     items;
    std::vector<BUNDLE_CLAIM> claims;
    std::vector<NAME_ID>      netclasses;
};

struct BUNDLE_BINDING
{
    std::vector<ITEM_KEY>   items;
    std::vector<NAME_ID>    netclasses;
    std::optional<CLAIM>    canonical;
    std::vector<SLOT_KEY>   members;
    std::vector<SLOT_INPUT> slots;
};

// Main-thread name interning; the canonical claim is placed first for the pure binding step.
BUNDLE_INPUT PrepareBundle( const PARTITION& aPartition, const RECORD_STORE::RECORD_CACHE& aRecords,
                            SESSION_KEYS& aKeys );
// Pure alignment of prepared claims.
BUNDLE_BINDING BindBundle( const BUNDLE_INPUT& aInput, const SESSION_KEYS& aKeys );

// Reconciles current bundles with persistent, exact-value slot versions.
class SLOT_STORE
{
public:
    using SLOT_CACHE = CACHE_TABLE<SLOT_KEY, SLOT_INPUT, KEY_LESS>;

    SLOT_STORE( CACHE_VERSIONS& aVersions, SESSION_KEYS& aKeys );
    void             Update( const COMPONENT_CACHE<BUNDLE_BINDING>& aBundles );
    const SLOT_CACHE& Slots() const { return m_slots; }
    void             Clear();

private:
    SLOT_CACHE                  m_slots;
    std::map<NODE_ID, uint64_t> m_bundleInputs;
};

// Main-thread interning of current signal records and slots for the signal fold.
std::vector<NODE_INPUT> SignalNodes( const RECORD_STORE::RECORD_CACHE& aRecords, const SLOT_STORE::SLOT_CACHE& aSlots,
                                     SESSION_KEYS& aKeys );
} // namespace SCH_CONNECTIVITY
