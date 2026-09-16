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
/**
 * One bus member as a node of the signal stratum.
 *
 * BindBundle() makes one slot for each leaf of the canonical claim and one extra slot for each leaf of another claim
 * that does not align, except for a sheet pin that neither drives its island nor reaches a child label. SignalNodes()
 * joins the slot to the signal records that claim the same name keys.
 */
struct SLOT_INPUT
{
    SLOT_KEY key;   ///< Source item of the declaring claim and the leaf ordinal in its schema.
    CLAIM    claim; ///< PRIORITY::BUS_MEMBER claim with the leaf name and the path of its owning claim.

    /** Group path and leaf local name joined with dots. Publication uses it to name group members. */
    NAME_ID               localName = INVALID_ID;
    std::vector<NAME_KEY> edges; ///< Sheet name keys that always join this slot.

    /** Parent-sheet names from a driving sheet pin, which join only names a signal record already claims. */
    std::vector<NAME_KEY> parentEdges;
    std::vector<NAME_ID>  parentNetclasses;          ///< Netclasses of the bus component.
    NODE_ID               parentBundle = INVALID_ID; ///< Anchor node of the bus component.
    bool                  operator==( const SLOT_INPUT& ) const = default;
};

/**
 * One bus claim of a bundle partition, with its leaf names interned.
 */
struct BUNDLE_CLAIM
{
    struct LEAF_NAME
    {
        NAME_ID name;      ///< Leaf name with all group prefixes.
        NAME_ID fullName;  ///< Leaf name after the sheet path of this claim.
        NAME_ID localName; ///< Group path and leaf local name joined with dots.
    };

    RECORD_KEY             record; ///< Record of the island that holds the claim.
    CLAIM                  claim;
    std::vector<LEAF_NAME> leaves; ///< One entry for each leaf of the claim schema, in the same order.

    /** The claim drives its own island, which lets a sheet pin name members in the parent sheet. */
    bool                   drivesRecord = false;
};

/**
 * Main-thread input of BindBundle() for one bundle partition.
 */
struct BUNDLE_INPUT
{
    NODE_ID                   parent = INVALID_ID; ///< Anchor node of the partition.
    std::vector<ITEM_KEY>     items;
    std::vector<BUNDLE_CLAIM> claims;              ///< Bus claims. The canonical claim is first.
    std::vector<NAME_ID>      netclasses;
};

/**
 * Bound result of one bus component.
 */
struct BUNDLE_BINDING
{
    std::vector<ITEM_KEY>   items;
    std::vector<NAME_ID>    netclasses;
    std::optional<CLAIM>    canonical; ///< Strongest bus claim. It is empty for a bus without a bus claim.
    std::vector<SLOT_KEY>   members;   ///< Canonical slots in leaf order. Extra slots are not listed.
    std::vector<SLOT_INPUT> slots;     ///< Canonical and extra slots, sorted by key.
};

/**
 * Collect the bus claims of a bundle partition and intern their leaf names on the main thread.
 *
 * The canonical claim is placed first for the pure binding step.
 *
 * @throw std::invalid_argument if the partition does not match the current records.
 * @throw std::length_error if a schema has more leaves than a slot ordinal can hold.
 */
BUNDLE_INPUT PrepareBundle( const PARTITION& aPartition, const RECORD_STORE::RECORD_CACHE& aRecords,
                            SESSION_KEYS& aKeys );

/**
 * Align the prepared claims to the canonical claim and make the member slots.
 *
 * This function is pure, so the engine runs it on worker threads.
 */
BUNDLE_BINDING BindBundle( const BUNDLE_INPUT& aInput, const SESSION_KEYS& aKeys );

/**
 * Reconciles current bundles with persistent, exact-value slot versions.
 *
 * The store replaces the slots of a bundle only when the bundle version changed.
 */
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

/**
 * Main-thread interning of current signal records and slots for the signal fold.
 *
 * A slot parent edge becomes a node edge only if a signal record claims the same name key. Two buses thus never join
 * through a parent name that no net on that sheet uses.
 */
std::vector<NODE_INPUT> SignalNodes( const RECORD_STORE::RECORD_CACHE& aRecords, const SLOT_STORE::SLOT_CACHE& aSlots,
                                     SESSION_KEYS& aKeys );
} // namespace SCH_CONNECTIVITY
