/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2022 KiCad Developers, see CHANGELOG.TXT for contributors.
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
 * along with this program; if not, you may find one here:
 * http://www.gnu.org/licenses/gpl-3.0.html
 * or you may search the http://www.gnu.org website for the version 3 license,
 * or you may write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
 */


#include <thread_pool.h>

// Under mingw, there is a problem with the destructor when creating a static instance
// of a thread_pool: probably the DTOR is called too late, and the application hangs.
// so we create it on the heap.
static thread_pool* tp = nullptr;

thread_pool& GetKiCadThreadPool()
{
#if 0   // Turn this on to disable multi-threading for debugging
    if( !tp ) tp = new thread_pool( 1 );
#else
    if( !tp ) tp = new thread_pool;
#endif

    return *tp;
}
