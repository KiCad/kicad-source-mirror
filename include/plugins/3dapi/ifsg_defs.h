/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2011 SoftPLC Corporation, Dick Hollenbeck <dick@softplc.com>
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
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

/*
 * Note: this is based on kicad's import_export.h file but is
 * reproduced here in order to facilitate support of out-of-tree
 * builds for 3D plugins.
 */

#ifndef IFSG_DEFS_H
#define IFSG_DEFS_H

#if defined(_WIN32)
    #define APIEXPORT __declspec(dllexport)
    #define APIIMPORT __declspec(dllimport)
    #define APILOCAL

#elif defined(__GNUC__) && __GNUC__ >= 4
    // On ELF, we compile with hidden visibility, so unwrap that for specific symbols:
    #define APIEXPORT __attribute__ ((visibility("default")))
    #define APIIMPORT __attribute__ ((visibility("default")))
    #define APILOCAL  __attribute__ ((visibility("hidden")))

#else
    #pragma message ( "warning: a supported C++ compiler is required" )
    #define APIEXPORT
    #define APIIMPORT
    #define APILOCAL
#endif

#if defined (COMPILE_SGLIB)
    #define SGLIB_API APIEXPORT
    #define MASK_3D_SG "3D_SG"
#else
    #define SGLIB_API APIIMPORT
#endif

#endif  // IFSG_DEFS_H
