/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2017 KiCad Developers, see AUTHORS.txt for contributors.
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

#ifndef SYNC_QUEUE_H
#define SYNC_QUEUE_H

#include <mutex>
#include <queue>

/**
 * Synchronized, locking queue. Safe for multiple producer/multiple consumer environments with
 * nontrivial data (though bear in mind data needs to be copied in and out).
 */
template <typename T> class SYNC_QUEUE
{
    typedef std::lock_guard<std::mutex> GUARD;

    std::queue<T>      m_queue;
    mutable std::mutex m_mutex;

public:
    SYNC_QUEUE()
    {
    }

    /**
     * Push a value onto the queue.
     */
    void push( T const& aValue )
    {
        GUARD guard( m_mutex );
        m_queue.push( aValue );
    }

    /**
     * Move a value onto the queue. Useful for e.g. unique_ptr.
     */
    void move_push( T&& aValue )
    {
        GUARD guard( m_mutex );
        m_queue.push( std::move( aValue ) );
    }

    /**
     * Pop a value off the queue into the provided variable. If the queue is empty, the
     * variable is not touched.
     *
     * @return true iff a value was popped.
     */
    bool pop( T& aReceiver )
    {
        GUARD guard( m_mutex );

        if( m_queue.empty() )
        {
            return false;
        }
        else
        {
            aReceiver = std::move( m_queue.front() );
            m_queue.pop();
            return true;
        }
    }

    /**
     * Return true iff the queue is empty.
     */
    bool empty() const
    {
        GUARD guard( m_mutex );
        return m_queue.empty();
    }

    /**
     * Return the size of the queue.
     */
    size_t size() const
    {
        GUARD guard( m_mutex );
        return m_queue.size();
    }

    /**
     * Clear the queue.
     */
    void clear()
    {
        GUARD guard( m_mutex );

        while( !m_queue.empty() )
        {
            m_queue.pop();
        }
    }
};

#endif // SYNC_QUEUE_H
