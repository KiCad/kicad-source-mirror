/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2022 Jon Evans <jon@craftyjon.com>
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
 * You should have received a copy of the GNU General Public License along
 * with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef KICAD_DATABASE_CACHE_H
#define KICAD_DATABASE_CACHE_H

#include <chrono>
#include <list>
#include <string>
#include <unordered_map>

#include <database/database_connection.h>


template<typename CacheValueType>
class DATABASE_CACHE
{
public:
    typedef std::pair<std::string, std::pair<time_t, CacheValueType>> CACHE_ENTRY;

    typedef std::unordered_map<std::string, typename std::list<CACHE_ENTRY>::iterator> CACHE_TYPE;

    typedef typename CACHE_TYPE::const_iterator CACHE_CITER;

    typedef CacheValueType CACHE_VALUE;

    DATABASE_CACHE( size_t aMaxSize, time_t aMaxAge ) :
            m_maxSize( aMaxSize ),
            m_maxAge( aMaxAge )
    {}

    void Put( const std::string& aQuery, const CacheValueType& aResult )
    {
        auto it = m_cache.find( aQuery );

        time_t time = std::chrono::system_clock::to_time_t( std::chrono::system_clock::now() );

        m_cacheMru.push_front( std::make_pair( aQuery,
                                               std::make_pair( time, aResult ) ) );

        if( it != m_cache.end() )
        {
            m_cacheMru.erase( it->second );
            m_cache.erase( it );
        }

        m_cache[aQuery] = m_cacheMru.begin();

        if( m_cache.size() > m_maxSize )
        {
            auto last = m_cacheMru.end();
            last--;
            m_cache.erase( last->first );
            m_cacheMru.pop_back();
        }
    }

    bool Get( const std::string& aQuery, CacheValueType& aResult )
    {
        auto it = m_cache.find( aQuery );

        if( it == m_cache.end() )
            return false;

        time_t time = std::chrono::system_clock::to_time_t( std::chrono::system_clock::now() );

        if( time - it->second->second.first > m_maxAge )
        {
            m_cacheMru.erase( it->second );
            m_cache.erase( it );
            return false;
        }

        m_cacheMru.splice( m_cacheMru.begin(), m_cacheMru, it->second );

        aResult = it->second->second.second;
        return true;
    }

    void SetMaxSize( size_t aMaxSize ) { m_maxSize = aMaxSize; }
    void SetMaxAge( time_t aMaxAge ) { m_maxAge = aMaxAge; }

private:
    size_t m_maxSize;
    time_t m_maxAge;
    std::list<CACHE_ENTRY> m_cacheMru;
    CACHE_TYPE m_cache;
};

#endif //KICAD_DATABASE_CACHE_H
