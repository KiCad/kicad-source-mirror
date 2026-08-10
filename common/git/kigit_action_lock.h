/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright The KiCad Developers, see AUTHORS.TXT for contributors.
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

#ifndef KIGIT_ACTION_LOCK_H
#define KIGIT_ACTION_LOCK_H

#include <git/kigit_lock_retry.h>

#include <memory>
#include <mutex>

namespace KIGIT
{

/**
 * KIGIT_COMMON's action lock, the session's mutex and then the working tree's REPO_ACTION_LOCK, so another
 * process waits for the action too. It owns the lock only while it holds both.
 */
class ACTION_LOCK
{
public:
    ACTION_LOCK() = default;

    /// The mutex alone, which no other process sees
    explicit ACTION_LOCK( std::unique_lock<std::mutex>&& aMutex ) :
            m_mutex( std::move( aMutex ) )
    {
    }

    ACTION_LOCK( std::unique_lock<std::mutex>&& aMutex, std::unique_ptr<REPO_ACTION_LOCK> aRepo ) :
            m_mutex( std::move( aMutex ) ),
            m_repo( std::move( aRepo ) )
    {
    }

    ACTION_LOCK( ACTION_LOCK&& ) noexcept = default;

    ACTION_LOCK& operator=( ACTION_LOCK&& aOther ) noexcept
    {
        if( this != &aOther )
        {
            unlock();
            m_mutex = std::move( aOther.m_mutex );
            m_repo = std::move( aOther.m_repo );
        }

        return *this;
    }

    ~ACTION_LOCK() { unlock(); }

    bool owns_lock() const { return m_mutex.owns_lock() && ( !m_repo || m_repo->Held() ); }

    explicit operator bool() const { return owns_lock(); }

    void unlock()
    {
        m_repo.reset();

        if( m_mutex.owns_lock() )
            m_mutex.unlock();
    }

private:
    std::unique_lock<std::mutex>      m_mutex;
    std::unique_ptr<REPO_ACTION_LOCK> m_repo; ///< Null when nothing outside this process can share the tree
};

} // namespace KIGIT

#endif // KIGIT_ACTION_LOCK_H
