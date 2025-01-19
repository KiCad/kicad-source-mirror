/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2015-2016 Mario Luzeiro <mrluzeiro@ua.pt>
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

/**
 * @file color_rgba.h
 */

#ifndef COLOR_RGBA_H
#define COLOR_RGBA_H

#include <plugins/3dapi/xv3d_types.h>

union COLOR_RGBA
{
    unsigned char c[4];

    struct
    {
        unsigned char r;
        unsigned char g;
        unsigned char b;
        unsigned char a;
    };

    COLOR_RGBA( const SFVEC3F& aColor );
    COLOR_RGBA( const SFVEC4F& aColor );
    COLOR_RGBA() { r = 0; g = 0; b = 0; a = 0; }
    COLOR_RGBA( unsigned char aR, unsigned char aG, unsigned char aB )
    {
        r = aR;
        g = aG;
        b = aB;
    }

    COLOR_RGBA( unsigned char aR, unsigned char aG, unsigned char aB, unsigned char aA )
    {
        r = aR;
        g = aG;
        b = aB;
        a = aA;
    }

    operator SFVEC4F() const;
};


COLOR_RGBA BlendColor( const COLOR_RGBA& aC1, const COLOR_RGBA& aC2 );
COLOR_RGBA BlendColor( const COLOR_RGBA& aC1, const COLOR_RGBA& aC2, const COLOR_RGBA& aC3 );
COLOR_RGBA BlendColor( const COLOR_RGBA& aC1, const COLOR_RGBA& aC2, const COLOR_RGBA& aC3,
                       const COLOR_RGBA& aC4 );

#endif   // COLOR_RGBA_H
