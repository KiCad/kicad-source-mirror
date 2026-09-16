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

#include <cstdint>
#include <limits>
#include <map>
#include <memory>
#include <stdexcept>
#include <utility>

namespace SCH_CONNECTIVITY
{
/**
 * One sequence for all cache tables for the lifetime of an engine session. Main thread only.
 *
 * Versions start at one and never repeat, so zero never names a cached value.
 */
class CACHE_VERSIONS
{
public:
    CACHE_VERSIONS() = default;
    CACHE_VERSIONS( const CACHE_VERSIONS& ) = delete;
    CACHE_VERSIONS& operator=( const CACHE_VERSIONS& ) = delete;

    uint64_t Next()
    {
        if( m_latest == std::numeric_limits<uint64_t>::max() )
            throw std::overflow_error( "Connectivity cache versions exhausted" );

        return ++m_latest;
    }

private:
    uint64_t m_latest = 0;
};


/**
 * Versioned value table. Set() keeps the old version for an equal value, which stops change propagation.
 */
template <typename KEY, typename VALUE, typename LESS = std::less<KEY>>
class CACHE_TABLE
{
public:
    struct ENTRY
    {
        VALUE    value;
        uint64_t version;
    };

    explicit CACHE_TABLE( CACHE_VERSIONS& aVersions, LESS aLess = {} ) :
            m_versions( aVersions ),
            m_entries( aLess )
    {
    }
    CACHE_TABLE( const CACHE_TABLE& ) = delete;
    CACHE_TABLE& operator=( const CACHE_TABLE& ) = delete;

    /**
     * Entry references survive unchanged writes, but not replacement or erasure of their key.
     */
    const ENTRY* Find( const KEY& aKey ) const { return Find<KEY>( aKey ); }

    template <typename LOOKUP_KEY>
    const ENTRY* Find( const LOOKUP_KEY& aKey ) const
    {
        auto it = m_entries.find( aKey );
        return it == m_entries.end() ? nullptr : it->second.get();
    }

    const ENTRY& Set( const KEY& aKey, VALUE aValue )
    {
        auto it = m_entries.find( aKey );

        if( it != m_entries.end() && it->second->value == aValue )
            return *it->second;

        const uint64_t version = m_versions.Next();
        // Construct before replacement so a failed value move cannot retain an obsolete version.
        auto entry = std::make_unique<const ENTRY>( ENTRY{ std::move( aValue ), version } );
        return *m_entries.insert_or_assign( aKey, std::move( entry ) ).first->second;
    }

    void Erase( const KEY& aKey ) { m_entries.erase( aKey ); }
    void Clear() { m_entries.clear(); }

    const auto& Entries() const { return m_entries; }

private:
    CACHE_VERSIONS&                                   m_versions;
    std::map<KEY, std::unique_ptr<const ENTRY>, LESS> m_entries;
};
} // namespace SCH_CONNECTIVITY
