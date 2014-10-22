/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2013-2014 SoftPLC Corporation, Dick Hollenbeck <dick@softplc.com>
 * Copyright (C) 1992-2014 KiCad Developers, see CHANGELOG.TXT for contributors.
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

#ifndef KI_MUTEX_H_
#define KI_MUTEX_H_


/// Establish KiCad MUTEX choices here in this file:
///    typedef MUTEX and typedef MUTLOCK.
///
/// Using an unnamed resource is easier, providing a textual name for a
/// constructor is cumbersome, so we make choice on that criteria mostly:

#if 1

// This is a fine choice between the two, but requires linking to ${Boost_LIBRARIES}

#include <boost/interprocess/sync/interprocess_mutex.hpp>
#include <boost/interprocess/sync/scoped_lock.hpp>

typedef boost::interprocess::interprocess_mutex     MUTEX;
typedef boost::interprocess::scoped_lock<MUTEX>     MUTLOCK;

#else

// This choice also works.

#include <wx/thread.h>

typedef wxMutex             MUTEX;
typedef wxMutexLocker       MUTLOCK;

#endif

#endif  // KI_MUTEX_H_
