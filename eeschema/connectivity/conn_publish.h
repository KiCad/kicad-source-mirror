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

#include "conn_signals.h"
#include "conn_auxiliary.h"
#include <set>

enum class CONNECTION_TYPE;

namespace SCH_CONNECTIVITY
{
struct COMPONENT_CONTENT
{
    KIND                    kind = KIND::SIGNAL;
    std::optional<CLAIM>    best;
    std::vector<ITEM_KEY>   items;
    std::vector<RECORD_KEY> records;
    std::vector<SLOT_KEY>   slots;
    // Canonical driver leaf order; slots also retains noncanonical extras.
    std::vector<SLOT_KEY>   members;
    std::optional<SLOT_KEY> nameSlot;
    std::vector<NAME_ID>    netclasses;
    NAME_ID                 baseName = INVALID_ID;
    bool                    operator==( const COMPONENT_CONTENT& ) const = default;
};

struct PUBLISHED_COMPONENT
{
    std::shared_ptr<const COMPONENT_CONTENT> content;
    std::optional<SLOT_KEY>                  nameSlot;
    NAME_ID                                  baseName = INVALID_ID;
    NAME_ID                                  collisionBase = INVALID_ID;
    NAME_ID                                  name = INVALID_ID;
    uint32_t                                 suffix = 0;
    int                                      netCode = 0;
    uint32_t                                 subgraphCode = 0;

    bool operator==( const PUBLISHED_COMPONENT& aOther ) const
    {
        return ( content == aOther.content || ( content && aOther.content && *content == *aOther.content ) )
               && nameSlot == aOther.nameSlot && baseName == aOther.baseName && collisionBase == aOther.collisionBase
               && name == aOther.name && suffix == aOther.suffix && netCode == aOther.netCode
               && subgraphCode == aOther.subgraphCode;
    }
};

struct ITEM_RESULT
{
    struct BUS_SOURCE
    {
        ITEM_KEY                          source;
        std::shared_ptr<const BUS_SCHEMA> schema;
        bool                              operator==( const BUS_SOURCE& aOther ) const;
    };

    std::shared_ptr<const BUS_SOURCE> busSource;
    NODE_ID                           component = INVALID_ID;
    KIND                              kind = KIND::SIGNAL;
    CONNECTION_TYPE                   itemType{};
    NAME_ID                           name = INVALID_ID;
    NAME_ID                           localName = INVALID_ID;
    NAME_ID                           fullLocalName = INVALID_ID;
    std::optional<ITEM_KEY>           driver;
    int                               netCode = 0;
    uint32_t                          subgraphCode = 0;
    // Null denotes an empty class set; ownership does not retain component membership.
    std::shared_ptr<const std::vector<NAME_ID>> netclasses;

    bool operator==( const ITEM_RESULT& aOther ) const;
};

// If aSchema is provided, aName must be the exact text it was parsed from.
wxString ApplyNameSuffix( const wxString& aName, const BUS_SCHEMA* aSchema, uint32_t aSuffix );

// Main-thread publication. Previous memberships provide suffix, code and succession continuity.
class PUBLICATION
{
public:
    using COMPONENTS = std::map<NODE_ID, PUBLISHED_COMPONENT>;

    using NAME_INDEX = std::map<NAME_ID, NODE_ID, NAME_LESS>;
    using SLOT_INDEX = std::map<SLOT_KEY, NODE_ID, KEY_LESS>;
    using COMPONENT_LINKS = std::map<NODE_ID, std::vector<NODE_ID>>;

    using ROWS = std::map<ITEM_KEY, ITEM_RESULT, KEY_LESS>;
    using CLASS_ASSIGNMENTS = std::map<NAME_ID, std::shared_ptr<const std::vector<NAME_ID>>, NAME_LESS>;

    explicit PUBLICATION( SESSION_KEYS& aKeys );
    void Update( const COMPONENT_CACHE<BUNDLE_BINDING>& aBundles, const COMPONENT_CACHE<SIGNAL_RESULT>& aSignals,
                 const RECORD_STORE::RECORD_CACHE& aRecords, std::span<const FRAME_INSTANCE> aFrame,
                 const INPUT_STORE& aInputs );
    void Clear();

    const COMPONENTS& Components() const { return m_components; }
    const CHANGE_SET& Changes() const { return m_changes; }

    const NAME_INDEX&      ByName() const { return m_byName; }
    const SLOT_INDEX&      SlotComponents() const { return m_slotComponents; }
    std::optional<NODE_ID> FindByName( const wxString& aName ) const;
    // Other published unnamed groups with the same scoped canonical leaf multiset, in name order.
    std::vector<wxString> EquivalentBusNames( const wxString& aName ) const;
    // Borrowed spans; callers must reacquire them after Update or Clear.
    std::span<const NODE_ID> MembersOf( NODE_ID aBundle ) const;
    std::span<const NODE_ID> ParentsOf( NODE_ID aSignal ) const;

    const AUXILIARY& Auxiliary() const { return m_auxiliary; }

    const ROWS&              Rows() const { return m_rows; }
    const CLASS_ASSIGNMENTS& Netclasses() const { return m_netclasses; }

private:
    using BUS_SIGNATURE = std::pair<NAME_ID, std::vector<NAME_ID>>;
    using EQUIVALENT_BUSES = std::map<BUS_SIGNATURE, std::set<NODE_ID>>;
    using BUS_SIGNATURES = std::map<NODE_ID, EQUIVALENT_BUSES::iterator>;
    using SLOT_INPUTS = std::map<SLOT_KEY, const SLOT_INPUT*, KEY_LESS>;

    struct ROW_INPUT
    {
        uint64_t          version = 0;
        ITEM_RESULT       connection;
        std::vector<KIID> items;

        // An island driver keeps its own local name when a same-sheet merge renames its other items.
        std::optional<std::pair<KIID, ITEM_RESULT>> driver;
    };
    using ROW_INPUTS = std::map<RECORD_KEY, std::shared_ptr<const ROW_INPUT>, KEY_LESS>;

    struct ROW_UPDATE
    {
        explicit ROW_UPDATE( const SESSION_KEYS& aKeys ) :
                upserts( KEY_LESS{ aKeys } ),
                inputs( KEY_LESS{ aKeys } ),
                netclasses( NAME_LESS{ &aKeys } )
        {
        }

        ROWS                    upserts;
        std::vector<ITEM_KEY>   removed;
        std::vector<RECORD_KEY> removedInputs;
        ROW_INPUTS              inputs;
        CLASS_ASSIGNMENTS       netclasses;
    };

    ROW_UPDATE PrepareRows( const COMPONENTS& aCurrent, const RECORD_STORE::RECORD_CACHE& aRecords,
                            const SLOT_INPUTS& aSlots, CHANGE_SET& aChanges );
    void ApplyRows( ROW_UPDATE&& aUpdate );
    void UpdateIndexes( const COMPONENTS& aCurrent, const SLOT_INPUTS& aSlots );
    void UpdateBusSignatures( const COMPONENTS& aCurrent, const SLOT_INPUTS& aSlots );

    SESSION_KEYS&               m_keys;
    AUXILIARY                   m_auxiliary;
    ROWS                        m_rows;
    ROW_INPUTS                  m_rowInputs;
    CLASS_ASSIGNMENTS           m_netclasses;
    NAME_INDEX                  m_byName;
    SLOT_INDEX                  m_slotComponents;
    COMPONENT_LINKS             m_busMembers;
    COMPONENT_LINKS             m_busParents;
    BUS_SIGNATURES              m_busSignatures;
    EQUIVALENT_BUSES            m_equivalentBuses;
    COMPONENTS                  m_components;
    std::map<NODE_ID, uint64_t> m_versions;
    CHANGE_SET                  m_changes;
    int                         m_nextNetCode = 1;
    uint32_t                    m_nextSubgraphCode = 1;
};
} // namespace SCH_CONNECTIVITY
