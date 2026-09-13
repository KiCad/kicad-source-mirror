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
#include "conn_inputs.h"

namespace SCH_CONNECTIVITY
{
// Main-thread island record preparation and cache writes. Record folding consumes only owned values.
class RECORD_STORE
{
public:
    using SOURCE_CACHE = CACHE_TABLE<ITEM_KEY, SOURCE_CLAIMS, KEY_LESS>;
    using RECORD_CACHE = CACHE_TABLE<RECORD_KEY, ISLAND_RECORD, KEY_LESS>;

    RECORD_STORE( CACHE_VERSIONS& aVersions, SESSION_KEYS& aKeys );

    // Frame and inputs must come from the same INPUT_STORE capture and key session.
    void Update( const std::vector<FRAME_INSTANCE>& aFrame, INPUT_STORE& aInputs, const BUS_ALIASES& aAliases );
    const RECORD_CACHE& Records() const { return m_records; }
    void               Clear();

    // Presentation is stored separately from the electrical schemas retained by claims.
    std::shared_ptr<const BUS_SCHEMA::NODE> FindTree( const wxString& aText ) const
    {
        return m_parses.FindTree( aText );
    }

private:
    struct PREPARED_INSTANCE
    {
        uint64_t                                      sourceVersion;
        uint64_t                                      textVersion;
        INSTANCE_SCOPE                                scope;
        std::optional<std::pair<SCREEN_ID, uint64_t>> symbolSource;
        size_t                                        factCount;
        std::vector<KIID>                             items;
    };

    struct RECORD_INPUT
    {
        bool                                   hasWire;
        bool                                   hasBusLine;
        std::vector<std::pair<KIID, uint64_t>> items;
        bool                                   operator==( const RECORD_INPUT& ) const = default;
    };

    struct RECORD_ORDER
    {
        bool operator()( const RECORD_KEY& aLeft, const RECORD_KEY& aRight ) const
        {
            return aLeft.inst != aRight.inst ? aLeft.inst < aRight.inst : aLeft.anchor < aRight.anchor;
        }
    };

    SESSION_KEYS&                                    m_keys;
    BUS_PARSE_CACHE                                  m_parses;
    SOURCE_CACHE                                     m_sources;
    RECORD_CACHE                                     m_records;
    std::map<INST_ID, PREPARED_INSTANCE>             m_prepared;
    std::map<RECORD_KEY, RECORD_INPUT, RECORD_ORDER> m_recordInputs;
};
} // namespace SCH_CONNECTIVITY
