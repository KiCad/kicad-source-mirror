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

/**
 * @file navlib_safe_init.h
 * @brief Safe initialization wrapper for the 3Dconnexion navlib SDK.
 *
 * The 3Dconnexion driver on some macOS versions calls std::terminate() during
 * initialization (NlCreate), which cannot be caught by normal C++ exception
 * handling. This wrapper installs a temporary SIGABRT handler to recover from
 * the abort() that std::terminate() triggers.
 */

#ifndef NAVLIB_SAFE_INIT_H_
#define NAVLIB_SAFE_INIT_H_

#include <kicommon.h>

#include <functional>

/**
 * Attempt to run the given function, recovering from both C++ exceptions and
 * abort() calls triggered by buggy third-party drivers.
 *
 * On macOS, temporarily installs a SIGABRT handler that uses siglongjmp to
 * recover from abort(). On other platforms, wraps the call in a try/catch.
 *
 * @param aInitFunc the initialization function to call
 * @return true if the function completed without error, false otherwise
 */
KICOMMON_API bool SafeNavlibInit( const std::function<void()>& aInitFunc );

/**
 * Returns true if a previous SafeNavlibInit call detected a driver crash.
 * Once the driver is known to be broken, all future SafeNavlibInit calls
 * return false immediately without attempting initialization.
 */
KICOMMON_API bool NavlibDriverCrashed();

#endif // NAVLIB_SAFE_INIT_H_
