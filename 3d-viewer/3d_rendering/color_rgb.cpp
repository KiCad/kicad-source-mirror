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
 * @file color_rgb.cpp
 */


#include "color_rgb.h"


COLOR_RGB::COLOR_RGB( const SFVEC3F& aColor )
{
    r = (unsigned int) glm::clamp( (int) ( aColor.r * 255 ), 0, 255 );
    g = (unsigned int) glm::clamp( (int) ( aColor.g * 255 ), 0, 255 );
    b = (unsigned int) glm::clamp( (int) ( aColor.b * 255 ), 0, 255 );
}


COLOR_RGB BlendColor( const COLOR_RGB& aC1, const COLOR_RGB& aC2 )
{
    const unsigned int r = aC1.r + aC2.r;
    const unsigned int g = aC1.g + aC2.g;
    const unsigned int b = aC1.b + aC2.b;

    return COLOR_RGB( ( r >> 1 ), ( g >> 1 ), ( b >> 1 ) );
}


COLOR_RGB BlendColor( const COLOR_RGB& aC1, const COLOR_RGB& aC2, const COLOR_RGB& aC3 )
{
    const unsigned int r = aC1.r + aC2.r + aC3.r;
    const unsigned int g = aC1.g + aC2.g + aC3.g;
    const unsigned int b = aC1.b + aC2.b + aC3.b;

    return COLOR_RGB( ( r / 3 ), ( g / 3 ), ( b / 3 ) );
}


COLOR_RGB BlendColor( const COLOR_RGB& aC1, const COLOR_RGB& aC2, const COLOR_RGB& aC3,
                      const COLOR_RGB& aC4 )
{
    const unsigned int r = aC1.r + aC2.r + aC3.r + aC4.r;
    const unsigned int g = aC1.g + aC2.g + aC3.g + aC4.g;
    const unsigned int b = aC1.b + aC2.b + aC3.b + aC4.b;

    return COLOR_RGB( ( r >> 2 ), ( g >> 2 ), ( b >> 2 ) );
}
