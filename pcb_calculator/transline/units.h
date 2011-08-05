/*
 * units.h - some conversion definitions
 *
 * Copyright (C) 1992-2011 jean-pierre.charras
 * Copyright (C) 1992-2011 Kicad Developers, see change_log.txt for contributors.
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
 * along with this package; see the file COPYING.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street - Fifth Floor,
 * Boston, MA 02110-1301, USA.
 *
 */

#ifndef __UNITS_H
#define __UNITS_H

#include <math.h>
#include "units_scales.h"

#ifdef __MINGW32__
#define atanh(x) (0.5 * log((1.0 + (x)) / (1.0 - (x))))
#define asinh(x) log((x) + sqrt((x) * (x) + 1.0))
#define acosh(x) log((x) + sqrt((x) * (x) - 1.0))
#endif

#ifndef M_PI
#define M_PI           3.1415926535897932384626433832795029  /* pi */
#endif

#ifndef M_E
#define M_E            2.7182818284590452353602874713526625   /* e */
#endif

#define MU0  12.566370614e-7          // magnetic constant
#define C0   299792458.0              // speed of light in vacuum
#define ZF0  376.73031346958504364963 // wave resistance in vacuum

#endif /* __UNITS_H */
