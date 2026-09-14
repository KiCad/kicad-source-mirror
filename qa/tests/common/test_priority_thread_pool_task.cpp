/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright Quilter
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 3
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

#include <boost/test/unit_test.hpp>

#include <atomic>
#include <stdexcept>
#include <vector>

#include <priority_thread_pool_task.h>


namespace
{

/**
 * Simple task that counts how many items it processed, throwing an
 * exception if it encounters a negative item.
 */
class COUNTING_TASK : public PRIORITY_THREAD_POOL_TASK<std::vector<int>>
{
public:
    std::atomic<int> m_processed{ 0 };

private:
    int computePriorityKey( const int& aItem ) const override { return aItem; }

    size_t task( int& aItem ) override
    {
        if( aItem < 0 )
            throw std::runtime_error( "negative item" );

        ++m_processed;
        return 1;
    }
};

} // namespace


BOOST_AUTO_TEST_SUITE( PriorityThreadPoolTask )


BOOST_AUTO_TEST_CASE( AllItemsProcessed )
{
    COUNTING_TASK    task;
    std::vector<int> items = { 5, 3, 8, 1, 9 };

    BOOST_CHECK_NO_THROW( task.Execute( items ) );

    // All items should have been processed by now
    BOOST_CHECK_EQUAL( task.m_processed.load(), (int) items.size() );
}


BOOST_AUTO_TEST_CASE( ExceptionPropagates )
{
    COUNTING_TASK    task;
    std::vector<int> items = { 5, 3, -1, 1, 9 };

    BOOST_CHECK_THROW( task.Execute( items ), std::runtime_error );
}


BOOST_AUTO_TEST_SUITE_END()
