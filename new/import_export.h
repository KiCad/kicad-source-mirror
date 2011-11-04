/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2011 SoftPLC Corporation, Dick Hollenbeck <dick@softplc.com>
 * Copyright (C) 2011 KiCad Developers, see change_log.txt for contributors.
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

#ifndef IMPORT_EXPORT_H_
#define IMPORT_EXPORT_H_

// macros which export functions from a DLL/DSO.  Not yet important with GCC,
// (either linux or mingw maybe OSX), until you compile with the hidden attribute:
// -fvisibility=hidden -fvisibility-inlines-hidden, which we are not yet.


// GCC >= 4.x
#if defined(__GNUC__) && __GNUC__ >= 4
# define APIEXPORT __attribute__ ((visibility("default")))
# define APIIMPORT __attribute__ ((visibility("default")))
# define APILOCAL  __attribute__ ((visibility("hidden")))

// windows
#elif (defined(__WINDOWS__) || defined(__CYGWIN__) || defined(_WIN32))
# define APIEXPORT __declspec(dllexport)
# define APIIMPORT __declspec(dllimport)
# define APILOCAL

#else  // not windows nor GCC >= 4.x
# define APIEXPORT
# define APIIMPORT
# define APILOCAL
#endif


#ifdef COMPILING_DLL   // by CMake magically when compiling implementation.
# define MY_API     APIEXPORT
#else
# define MY_API     APIIMPORT
#endif

#define MY_LOCAL    APILOCAL

#endif  // IMPORT_EXPORTS_H_
