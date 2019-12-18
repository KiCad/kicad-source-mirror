/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2012-2019 KiCad Developers, see AUTHORS.txt for contributors.
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
 * along with this program; if not, you may find one here:
 * http://www.gnu.org/licenses/old-licenses/gpl-2.0.html
 * or you may search the http://www.gnu.org website for the version 2 license,
 * or you may write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
 */

#ifndef SINGLE_ACCESS__H
#define SINGLE_ACCESS__H

#include <mutex>


/**
 * Simple RAII wrapper for an object's pointer,
 * which allows thread-safe access to objects which cannot be accessed
 * concurrently.
 *
 * This wrapper is used to allow functions to return "locked handles" to the
 * protected objects. This means that, if all access to the object is via
 * this wrapper, the object can never be accessed in an unlocked state by client
 * code.
 */
template <typename T>
class SINGLE_ACCESS
{
public:
    SINGLE_ACCESS( T* aProtected, std::mutex& aMutex ) : m_protected( aProtected ), m_lock( aMutex )
    {
    }

    SINGLE_ACCESS( SINGLE_ACCESS<T>& ) = delete;

    /**
     * Moving a SINGLE_ACCESS moves the lock to the new item
     */
    SINGLE_ACCESS( SINGLE_ACCESS<T>&& aOther )
            : m_protected( aOther.m_protected ), m_lock( std::move( aOther.m_lock ) )
    {
        aOther.m_protected = nullptr;
    }

    /**
     * Is this class holding a set protected item?
     */
    operator bool() const
    {
        return m_protected != nullptr;
    }

    /**
     * Access the protected item.
     */
    operator T*() const
    {
        return m_protected;
    }

    /**
     * Access the protected item.
     */
    T* operator->() const
    {
        return m_protected;
    }

private:
    /**
     * The item that is protected by the lock. As long as this class exists,
     * client code has exclusive access to this object.
     */
    T*                           m_protected;

    /**
     * The lock that enforces the exclusive access.
     */
    std::unique_lock<std::mutex> m_lock;
};

#endif // SINGLE_ACCESS__T