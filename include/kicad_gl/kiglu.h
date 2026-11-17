/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
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
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

/**
 * This file is used to include proper OpenGL loader and GLU headers.
 */

#ifndef KIGLU_H_
#define KIGLU_H_

#include <kicad_gl/kiglad.h>

// System GL headers (assumed to be loaded via wxGLCanvas or other dependencies)
#ifdef __APPLE__
#include <OpenGL/gl.h>
#include <OpenGL/glu.h>
#else
#include <GL/gl.h>
#include <GL/glu.h>
#endif

// Windows headers define CALLBACK, so a Windows build without it should fail rather than lose __stdcall
#if !defined( CALLBACK ) && !defined( _WIN32 )
#define CALLBACK
#endif

/**
 * Convert a tessellator callback to the untyped pointer gluTessCallback takes.
 *
 * A direct cast between function pointer types raises MSVC C4191; the hop through void* does not.
 */
template <typename FN>
inline auto ToGluCallback( FN* aCallback )
{
    return reinterpret_cast<void( CALLBACK* )()>( reinterpret_cast<void*>( aCallback ) );
}

#endif  // KIGLU_H_
