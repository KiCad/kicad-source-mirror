#ifndef NAVLIB_DEFINES_H_INCLUDED_
#define NAVLIB_DEFINES_H_INCLUDED_
/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (c) 2014-2021 3Dconnexion.
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
 * @file navlib_defines.h
 * @brief the macros used in the 3dconnexion interface and header files.
 */

// Invalid handle
#define INVALID_NAVLIB_HANDLE 0

// Navlib facility used to generate error codes
// Note this is identical to FACILITY_ITF on windows
#if _WIN32
#define FACILITY_NAVLIB 4
#else
#define FACILITY_NAVLIB 4
#endif

// resources
#define NAVLIB_IDB_ManualPivot 0x6004
#define NAVLIB_IDB_AutoPivot 0x6005

#if __cplusplus
#define _NAVLIB_BEGIN namespace navlib {
#define _NAVLIB_END }
#define _NAVLIB ::navlib::
#define _USING_NAVLIB using namespace navlib;
#else
#define _NAVLIB_BEGIN
#define _NAVLIB_END
#define _NAVLIB
#define _USING_NAVLIB
#endif

#if defined(_MSC_VER) && defined(NAVLIB_EXPORTS)
#define _NAVLIB_DLLAPI extern "C" __declspec(dllexport)
#elif __cplusplus
#define _NAVLIB_DLLAPI extern "C"
#else
#define _NAVLIB_DLLAPI
#endif

// __cdecl is a Windows-specific calling convention.
// - On Windows with GCC (MinGW), define it using the attribute
// - On non-Windows platforms, define it as empty since the default ABI is equivalent
#ifdef _WIN32
#if defined(__GNUC__) && !defined(__clang__)
#define __cdecl __attribute__((__cdecl__))
#endif
#else
#ifndef __cdecl
#define __cdecl
#endif
#endif

#endif // NAVLIB_DEFINES_H_INCLUDED_
