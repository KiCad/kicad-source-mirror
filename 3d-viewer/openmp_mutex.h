/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2016 Mario Luzeiro <mrluzeiro@ua.pt>
 * Copyright (C) 1992-2016 KiCad Developers, see AUTHORS.txt for contributors.
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


/**
 * @file  openmp_mutex.h
 * @brief a mutex for openmp got from the website:
 * http://bisqwit.iki.fi/story/howto/openmp/
 * by Joel Yliluoma <bisqwit@iki.fi>
 */

#ifndef _OPENMP_MUTEX_H
#define _OPENMP_MUTEX_H

#ifdef _OPENMP

# include <omp.h>

struct MutexType
{
    MutexType() { omp_init_lock( &lock ); }
    ~MutexType() { omp_destroy_lock( &lock ); }
    void Lock() { omp_set_lock( &lock ); }
    void Unlock() { omp_unset_lock( &lock ); }

    MutexType( const MutexType& ) { omp_init_lock( &lock ); }
    MutexType& operator= ( const MutexType& ) { return *this; }
public:
    omp_lock_t lock;
};

#else

/// A dummy mutex that doesn't actually exclude anything,
/// but as there is no parallelism either, no worries.
struct MutexType
{
    void Lock() {}
    void Unlock() {}
};
#endif

/// An exception-safe scoped lock-keeper.
struct ScopedLock
{
    explicit ScopedLock( MutexType& m ) : mut( m ), locked( true ) { mut.Lock(); }
    ~ScopedLock() { Unlock(); }
    void Unlock() { if( !locked ) return; locked = false; mut.Unlock(); }
    void LockAgain() { if( locked ) return; mut.Lock(); locked = true; }

private:
    MutexType& mut;
    bool locked;

private: // prevent copying the scoped lock.
    void operator=(const ScopedLock&);
    ScopedLock(const ScopedLock&);
};

#endif // _OPENMP_MUTEX_H
