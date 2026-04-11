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

#pragma once

#include <thread_pool.h>


/**
 * A helper class to execute tasks on a thread pool in priority order, with progress reporting.
 */
template <typename ContainerT>
class PRIORITY_THREAD_POOL_TASK
{
public:
    using ItemT = typename ContainerT::value_type;

    PRIORITY_THREAD_POOL_TASK() :
            m_reporter( nullptr ),
            m_highestPriority( BS::pr::high ),
            m_reporterInterval( std::chrono::milliseconds( 250 ) )
    {
    }

    void SetReporter( PROGRESS_REPORTER* aReporter ) { m_reporter = aReporter; }

    /**
     * Call this to execute the task on all items in aItems, using the thread pool
     * and dispatching the tasks in order of descending priority as determined by
     * comparePriority() implemented by the derived class.
     */
    void Execute( ContainerT& aItems )
    {
        thread_pool&                     tp = GetKiCadThreadPool();
        std::vector<std::future<size_t>> returns;

        // Compute priority keys paired to item indices
        using IndexedPriority = std::pair<size_t, int>;
        std::vector<IndexedPriority> indexedKeys( aItems.size() );
        for( size_t i = 0; i < aItems.size(); ++i )
        {
            indexedKeys[i] = { i, computePriorityKey( aItems[i] ) };
        }

        // Sort by descending priority key
        std::sort( indexedKeys.begin(), indexedKeys.end(),
                   []( const IndexedPriority& a, const IndexedPriority& b )
                   {
                       return a.second > b.second;
                   } );

        // Dispatch largest first
        const size_t numItems = aItems.size();
        for( size_t priorityRank = 0; priorityRank < numItems; ++priorityRank )
        {
            const size_t itemIndex = indexedKeys[priorityRank].first;
            ItemT&       item = aItems[itemIndex];

            // Earlier ranking -> higher key -> should be higher priority
            const size_t priority = ( ( numItems - priorityRank - 1 ) * m_highestPriority ) / numItems;

            returns.emplace_back( tp.submit_task(
                    [this, &item]
                    {
                        return task( item );
                    },
                    priority ) );
        }

        for( const std::future<size_t>& ret : returns )
        {
            std::future_status status = ret.wait_for( m_reporterInterval );

            while( status != std::future_status::ready )
            {
                if( m_reporter )
                    m_reporter->KeepRefreshing();

                status = ret.wait_for( m_reporterInterval );
            }
        }
    }

protected:
    PROGRESS_REPORTER* m_reporter;

private:
    /**
     * Implement this to compute a priority key for an item.
     *
     * Return a number representing priority, where a higher number means higher priority.
     * The actual values returned don't matter, only their relative order.
     *
     * (A relational a < b comparator would work too, but a unary key lets us compute it
     * once per item in O(n) and then sort indices cheaply, rather than calling it O(n log n)
     * times inside std::sort.)
     *
     * If you'd like to test the effect of this priority ordering, you
     * can return a constant value to disable sorting, or return the inverse
     * to sort backwards.
     */
    virtual int computePriorityKey( const ItemT& aItem ) const = 0;

    /**
     * Process one item in the thread pool.  Return the number of items processed.
     */
    virtual size_t task( ItemT& item ) = 0;

    BS::priority_t            m_highestPriority;
    std::chrono::milliseconds m_reporterInterval;
};
