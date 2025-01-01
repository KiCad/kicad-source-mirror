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
* You should have received a copy of the GNU General Public License along
* with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef __KICAD_SPINLOCK_H
#define __KICAD_SPINLOCK_H

#include <atomic>


/**
 * A trivial spinlock implementation with no optimization.  Don't use if congestion is expected!
 */
class KISPINLOCK
{
public:
    KISPINLOCK() :
        m_lock( false )
    {}

    void lock()
    {
        while( m_lock.exchange( true, std::memory_order_acquire ) );
    }

    bool try_lock()
    {
        return !m_lock.exchange( true, std::memory_order_acquire );
    }

    void unlock()
    {
        m_lock.store( false, std::memory_order_release );
    }

    bool test()
    {
        return m_lock.load( std::memory_order_acquire );
    }

private:
    std::atomic<bool> m_lock;
};

#endif
