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

#include "conn_facts.h"
#include "conn_keys.h"
#include "conn_cache.h"
#include "conn_islands.h"
#include "conn_frame.h"
#include <text_eval/text_eval_environment.h>
#include <set>
#include <span>
#include <algorithm>
#include <optional>

namespace SCH_CONNECTIVITY
{
/**
 * Main-thread extraction cache. Returned entries follow CACHE_TABLE's reference lifetime.
 *
 * @see @ref schematic_connectivity
 */
class INPUT_STORE
{
public:
    using SOURCE_KEY = std::pair<SCREEN_ID, KIID>;
    using SCREEN_CACHE = CACHE_TABLE<SCREEN_ID, SCREEN_FACTS>;
    using FACT_CACHE = CACHE_TABLE<SOURCE_KEY, ITEM_FACT>;
    using INSTANCE_CACHE = CACHE_TABLE<INST_ID, INSTANCE_FACTS>;
    using TEXT_CACHE = CACHE_TABLE<ITEM_KEY, ITEM_TEXT_FACT, KEY_LESS>;
    using AREA_CACHE = CACHE_TABLE<ITEM_KEY, INSTANCE_RULE_AREA_FACT, KEY_LESS>;
    using UNIT_SIGNATURE = std::vector<std::pair<KIID, int>>;
    using ISLAND_KEY = std::pair<SCREEN_ID, UNIT_SIGNATURE>;
    using UNIT_VIEW = std::span<const std::pair<KIID, int>>;
    using ISLAND_LOOKUP = std::pair<SCREEN_ID, UNIT_VIEW>;

    struct ISLAND_LESS
    {
        using is_transparent = void;

        template <typename LEFT, typename RIGHT>
        bool operator()( const LEFT& aLeft, const RIGHT& aRight ) const
        {
            if( aLeft.first != aRight.first )
                return aLeft.first < aRight.first;

            return std::lexicographical_compare( aLeft.second.begin(), aLeft.second.end(), aRight.second.begin(),
                                                 aRight.second.end() );
        }
    };
    using GEOMETRY_CACHE = CACHE_TABLE<SCREEN_ID, SCREEN_GEOMETRY>;
    using ISLAND_CACHE = CACHE_TABLE<ISLAND_KEY, SCREEN_ISLANDS, ISLAND_LESS>;

    INPUT_STORE( CACHE_VERSIONS& aVersions, SESSION_KEYS& aKeys );

    // Refresh current instance inputs, then reconcile deletions after successful extraction.
    std::vector<FRAME_INSTANCE> Capture( const SCH_SHEET_LIST& aPaths, uint64_t aTextEpoch );

    /**
     * Compare retained external inputs for current instance/screen pairs, excluding model/revision changes.
     * Run outside SOURCE_SCOPE collection in a fresh text frame for each update, keeping that frame alive
     * through the following Capture. Reads populate its cache; reusing an older frame retains older values.
     * Without an active frame, this call owns one. Clock consultations can change on every fresh frame.
     * If the caller has compared global text context, unchanged screen revisions can skip reference replay.
     */
    bool ExternalSourcesChanged( const SCH_SHEET_LIST& aPaths, bool aContextUnchanged = false ) const;

    const SCREEN_CACHE::ENTRY& Screen( const SCH_SCREEN& aScreen );

    // The caller advances textEpoch for cross-object text, hierarchy, project and variant edits.
    const INSTANCE_CACHE::ENTRY& Instance( const SCH_SCREEN& aScreen, const SCH_SHEET_PATH& aPath,
                                           uint64_t aTextEpoch );

    const FACT_CACHE::ENTRY*     Fact( const SOURCE_KEY& aKey ) const { return m_facts.Find( aKey ); }
    const TEXT_CACHE::ENTRY*     Text( const ITEM_KEY& aKey ) const { return m_text.Find( aKey ); }
    const AREA_CACHE::ENTRY*     Area( const ITEM_KEY& aKey ) const { return m_areas.Find( aKey ); }
    const SCREEN_CACHE::ENTRY*   FindScreen( SCREEN_ID aScreen ) const { return m_screens.Find( aScreen ); }
    const INSTANCE_CACHE::ENTRY* FindInstance( INST_ID aInstance ) const { return m_instances.Find( aInstance ); }
    const INSTANCE_CACHE& Instances() const { return m_instances; }

    SCREEN_ID InstanceScreen( INST_ID aInstance ) const { return m_instanceInputs.at( aInstance ).screen; }

    const SCREEN_FACTS& InstanceScreenFacts( INST_ID aInstance ) const
    {
        return m_screens.Entries().at( InstanceScreen( aInstance ) )->value;
    }

    const GEOMETRY_CACHE::ENTRY& Geometry( SCREEN_ID aScreen );
    const ISLAND_CACHE::ENTRY&   Islands( SCREEN_ID aScreen, const UNIT_SIGNATURE& aUnits );

    const ISLAND_CACHE::ENTRY* FindIslands( SCREEN_ID aScreen, UNIT_VIEW aUnits ) const
    {
        return m_islands.Find( ISLAND_LOOKUP{ aScreen, aUnits } );
    }

    const std::map<SCREEN_ID, uint64_t>& ScreenRevisions() const { return m_screenRevisions; }

    // Verified non-line source identity of a captured screen.
    std::optional<std::pair<SCREEN_ID, uint64_t>> VerifiedSymbolSource( SCREEN_ID aScreen ) const
    {
        const auto found = m_symbolRevisions.find( aScreen );

        if( found == m_symbolRevisions.end() )
            return std::nullopt;

        return std::pair{ aScreen, found->second };
    }

    // Consulted values from the last extraction; pointer valid until this instance changes or is erased.
    const TEXT_EVAL::ENVIRONMENT::SOURCE_VALUES* Sources( INST_ID aInstance ) const
    {
        const auto found = m_instanceSources.find( aInstance );
        return found == m_instanceSources.end() ? nullptr : &found->second;
    }

    void Retain( const std::set<SCREEN_ID>& aScreens, const std::set<INST_ID>& aInstances );
    /**
     * Force fresh extraction and geometry, retaining primitive values only for change comparison.
     *
     * Retained values keep their versions when extraction produces an equal value.
     */
    void Invalidate();
    void Clear();

private:
    const SCREEN_CACHE::ENTRY& storeScreen( const SCH_SCREEN& aScreen, SCREEN_FACTS aFacts );

    struct INSTANCE_INPUT
    {
        SCREEN_ID screen;
        uint64_t  revision;
        uint64_t  textEpoch;
        int       pageOrder;
        uint64_t  symbolRevision;
        bool      operator==( const INSTANCE_INPUT& ) const = default;
    };

    SESSION_KEYS&                                            m_keys;
    SCREEN_CACHE                                             m_screens;
    FACT_CACHE                                               m_facts;
    INSTANCE_CACHE                                           m_instances;
    TEXT_CACHE                                               m_text;
    AREA_CACHE                                               m_areas;
    GEOMETRY_CACHE                                           m_geometry;
    ISLAND_CACHE                                             m_islands;
    std::map<SCREEN_ID, uint64_t>                            m_geometryInputs;
    std::map<ISLAND_KEY, uint64_t, ISLAND_LESS>              m_islandInputs;
    std::map<SCREEN_ID, uint64_t>                            m_screenRevisions;
    std::map<SCREEN_ID, uint64_t>                            m_symbolRevisions;
    std::map<INST_ID, INSTANCE_INPUT>                        m_instanceInputs;
    std::map<INST_ID, TEXT_EVAL::ENVIRONMENT::SOURCE_VALUES> m_instanceSources;
};
} // namespace SCH_CONNECTIVITY
