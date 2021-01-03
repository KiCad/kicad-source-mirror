/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2015-2016 Mario Luzeiro <mrluzeiro@ua.pt>
 * Copyright (C) 2015-2020 KiCad Developers, see AUTHORS.txt for contributors.
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
 * @file color_rgb.h
 */

#ifndef COLOR_RGB_H
#define COLOR_RGB_H

#include <plugins/3dapi/xv3d_types.h>

union COLOR_RGB
{
    unsigned char c[3];

    struct
    {
        unsigned char r;
        unsigned char g;
        unsigned char b;
    };

    COLOR_RGB( const SFVEC3F& aColor );
    COLOR_RGB() { r = 0; g = 0; b = 0; }
    COLOR_RGB( unsigned char aR, unsigned char aG, unsigned char aB )
    {
        r = aR;
        g = aG;
        b = aB;
    }
};


COLOR_RGB BlendColor( const COLOR_RGB& aC1, const COLOR_RGB& aC2 );
COLOR_RGB BlendColor( const COLOR_RGB& aC1, const COLOR_RGB& aC2, const COLOR_RGB& aC3 );
COLOR_RGB BlendColor( const COLOR_RGB& aC1, const COLOR_RGB& aC2, const COLOR_RGB& aC3,
                      const COLOR_RGB& aC4 );

#endif   // COLOR_RGB_H
