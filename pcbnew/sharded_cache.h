/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#ifndef SHARDED_CACHE_H
#define SHARDED_CACHE_H

#include <array>
#include <shared_mutex>
#include <unordered_map>

/**
 * A concurrent key/value cache split into independently locked shards.
 *
 * The DRC expression evaluator populates per-item-pair predicate caches from every worker
 * thread at once. A single mutex over one map serializes all of them, so the multi-threaded
 * check collapses to single-threaded throughput. Striping the map across many shards, each with
 * its own lock, lets threads working on different keys proceed without contending. The shard is
 * chosen by the key hash, so a given key always maps to the same shard and the cache stays
 * coherent.
 */
template <typename KEY, typename VALUE, std::size_t SHARDS = 256>
class SHARDED_CACHE
{
public:
    /// Look up a key. Returns false on a miss and leaves aValue untouched.
    bool Get( const KEY& aKey, VALUE& aValue ) const
    {
        const SHARD&                        shard = shardFor( aKey );
        std::shared_lock<std::shared_mutex> lock( shard.mutex );

        auto it = shard.map.find( aKey );

        if( it == shard.map.end() )
            return false;

        aValue = it->second;
        return true;
    }

    void Set( const KEY& aKey, const VALUE& aValue )
    {
        SHARD&                              shard = shardFor( aKey );
        std::unique_lock<std::shared_mutex> lock( shard.mutex );

        shard.map[aKey] = aValue;
    }

    void Clear()
    {
        for( SHARD& shard : m_shards )
        {
            std::unique_lock<std::shared_mutex> lock( shard.mutex );
            shard.map.clear();
        }
    }

    bool Empty() const
    {
        for( const SHARD& shard : m_shards )
        {
            std::shared_lock<std::shared_mutex> lock( shard.mutex );

            if( !shard.map.empty() )
                return false;
        }

        return true;
    }

private:
    struct SHARD
    {
        mutable std::shared_mutex      mutex;
        std::unordered_map<KEY, VALUE> map;
    };

    SHARD& shardFor( const KEY& aKey )
    {
        return m_shards[std::hash<KEY>{}( aKey ) % SHARDS];
    }

    const SHARD& shardFor( const KEY& aKey ) const
    {
        return m_shards[std::hash<KEY>{}( aKey ) % SHARDS];
    }

    std::array<SHARD, SHARDS> m_shards;
};

#endif // SHARDED_CACHE_H
