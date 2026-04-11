/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright The KiCad Developers, see CHANGELOG.TXT for contributors.
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

#pragma once
#ifndef INCLUDE_THREAD_POOL_H_
#define INCLUDE_THREAD_POOL_H_

#include <bs_thread_pool.hpp>
#include <import_export.h>

using thread_pool = BS::priority_thread_pool;

/**
 * Get a reference to the current thread pool.  N.B., you cannot copy the thread pool
 * so if you accidentally write thread_pool tp = GetKiCadThreadPool(), you will break
 * your compilation
 *
 * @return Reference to the current (potentially newly constructed) thread pool
 */
APIEXPORT thread_pool& GetKiCadThreadPool();

/**
 * Invalidate the cached thread pool pointer.
 *
 * This must be called after the thread pool owned by PGM_BASE is destroyed
 * (via KICAD_SINGLETON::Shutdown()) to prevent dangling pointer access.
 * Any subsequent calls to GetKiCadThreadPool() will re-initialize the cache.
 */
APIEXPORT void InvalidateKiCadThreadPool();


#endif /* INCLUDE_THREAD_POOL_H_ */
