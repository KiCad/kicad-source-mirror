/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2007-2017 Jean-Pierre Charras, jp.charras at wanadoo.fr
 * Copyright (C) 1992-2021 KiCad Developers, see AUTHORS.txt for contributors.
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

#ifndef KICAD_BITMAP_OPAQUE_H
#define KICAD_BITMAP_OPAQUE_H

/// PNG memory record (file in memory).
struct BITMAP_OPAQUE
{
    const unsigned char* png;
    int         byteCount;
    const char* name;       // for debug, or future lazy dynamic linking
};

// declared as single element _array_, so its name assigns to pointer
#define EXTERN_BITMAP( x ) extern const BITMAP_OPAQUE x[1];


/// a BITMAP_DEF is really a const pointer to an opaque
/// structure.  So you should never need to use 'const' with it.
typedef const BITMAP_OPAQUE *BITMAP_DEF;

#endif // KICAD_BITMAP_OPAQUE_H
