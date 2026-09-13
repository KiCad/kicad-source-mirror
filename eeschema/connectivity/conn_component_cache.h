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

#include "conn_partition.h"
#include "conn_cache.h"
#include <algorithm>
#include <memory>
#include <stdexcept>
#include <unordered_map>
#include <utility>
#include <vector>

namespace SCH_CONNECTIVITY
{
// Cache current component evaluations by exact node/version identity, within one stratum and key session.
template <typename VALUE>
class COMPONENT_CACHE
{
public:
    struct ENTRY
    {
        PARTITION input;
        VALUE     value;
        uint64_t  version;
    };

    explicit COMPONENT_CACHE( CACHE_VERSIONS& aVersions ) :
            m_versions( aVersions )
    {
    }
    COMPONENT_CACHE( const COMPONENT_CACHE& ) = delete;
    COMPONENT_CACHE& operator=( const COMPONENT_CACHE& ) = delete;

    /**
     * Main-thread update that evaluates only cache misses, returning values in the supplied partition order.
     * The evaluator must not mutate this cache or its input tables. Versions must cover every input read by the
     * evaluator. Entries follow distinct input partitions. Reacquire references after UpdateBatch/Clear;
     * use versions rather than addresses for change detection.
     */
    template <typename EVALUATE>
    void UpdateBatch( std::span<const PARTITION> aPartitions, EVALUATE&& aEvaluate )
    {
        std::unordered_multimap<uint64_t, size_t> previous;
        previous.reserve( m_entries.size() );

        for( size_t i = 0; i < m_entries.size(); ++i )
            previous.emplace( m_entries[i]->input.hash, i );

        auto                                      stale = std::exchange( m_entries, {} );
        std::vector<std::unique_ptr<const ENTRY>> current( aPartitions.size() );
        std::vector<const PARTITION*>             missing;
        std::vector<size_t>                       ordinals;

        for( size_t ordinal = 0; ordinal < aPartitions.size(); ++ordinal )
        {
            const PARTITION& partition = aPartitions[ordinal];
            const auto [begin, end] = previous.equal_range( partition.hash );
            const auto found = std::find_if( begin, end,
                                             [&]( const auto& aCandidate )
                                             {
                                                 const auto& entry = stale[aCandidate.second];
                                                 return entry && entry->input == partition;
                                             } );

            if( found != end )
            {
                current[ordinal] = std::move( stale[found->second] );
            }
            else
            {
                missing.push_back( &partition );
                ordinals.push_back( ordinal );
            }
        }

        auto values = aEvaluate( missing );

        if( values.size() != missing.size() )
            throw std::logic_error( "Connectivity component batch returned an incorrect result count" );

        for( size_t i = 0; i < missing.size(); ++i )
        {
            current[ordinals[i]] =
                    std::make_unique<const ENTRY>( ENTRY{ *missing[i], std::move( values[i] ), m_versions.Next() } );
        }

        m_entries = std::move( current );
    }

    const auto& Entries() const { return m_entries; }
    void        Clear() { m_entries.clear(); }

private:
    CACHE_VERSIONS&                           m_versions;
    std::vector<std::unique_ptr<const ENTRY>> m_entries;
};
} // namespace SCH_CONNECTIVITY
