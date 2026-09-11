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

#include <thread_pool.h>
#include <algorithm>
#include <future>
#include <utility>
#include <vector>

namespace SCH_CONNECTIVITY
{
// Own submitted work until all references captured by workers can safely leave scope.
class TASK_GROUP
{
public:
    explicit TASK_GROUP( size_t aCount ) { m_tasks.reserve( aCount ); }
    TASK_GROUP( const TASK_GROUP& ) = delete;
    TASK_GROUP& operator=( const TASK_GROUP& ) = delete;

    ~TASK_GROUP()
    {
        for( auto& task : m_tasks )
        {
            if( task.valid() )
                task.wait();
        }
    }

    template <typename FUNCTION>
    void Submit( thread_pool& aPool, FUNCTION&& aFunction )
    {
        if( m_tasks.size() == m_tasks.capacity() )
            m_tasks.reserve( std::max<size_t>( 1, m_tasks.capacity() * 2 ) );

        m_tasks.push_back( aPool.submit_task( std::forward<FUNCTION>( aFunction ) ) );
    }

    void Get()
    {
        for( auto& task : m_tasks )
            task.get();
    }

private:
    std::vector<std::future<void>> m_tasks;
};

// Independent ordinal writes only; preparation and cache commits stay on the caller thread.
template <typename FUNCTION>
void ParallelFor( size_t aCount, FUNCTION&& aFunction, thread_pool& aPool = GetKiCadThreadPool() )
{
    if( aCount <= 1 )
    {
        if( aCount == 1 )
            aFunction( 0 );

        return;
    }

    const size_t workers = std::min<size_t>( aCount, aPool.get_thread_count() );
    TASK_GROUP tasks( workers );

    for( size_t worker = 0; worker < workers; ++worker )
    {
        tasks.Submit( aPool, [&, worker]
        {
            for( size_t ordinal = worker; ordinal < aCount; ordinal += workers )
                aFunction( ordinal );
        } );
    }

    tasks.Get();
}
} // namespace SCH_CONNECTIVITY
