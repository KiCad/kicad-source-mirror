/*
* This program source code file is part of KICAD, a free EDA CAD application.
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

#ifndef GAL_OPENGL_RESOURCES_H___
#define GAL_OPENGL_RESOURCES_H___

#include <gal/gal.h>

#define BITMAP_FONT_USE_SPANS

namespace KIGFX {

    namespace BUILTIN_FONT {

        struct FONT_IMAGE_TYPE
        {
            unsigned int width, height;
            unsigned int char_border;
            unsigned int spacing;
            // Height must match the atlas in bitmap_font_img.c
            unsigned char pixels[1024 * 1107 * 3];
        };

        struct FONT_INFO_TYPE
        {
            unsigned int smooth_pixels;
            float min_y;
            float max_y;
        };

        struct FONT_SPAN_TYPE
        {
            unsigned int start;
            unsigned int end;
            unsigned int cumulative;
        };

        struct FONT_GLYPH_TYPE
        {
            unsigned int atlas_x, atlas_y;
            unsigned int atlas_w, atlas_h;
            float minx, maxx;
            float miny, maxy;
            float advance;
        };

        extern GAL_API FONT_IMAGE_TYPE font_image;
        extern FONT_INFO_TYPE  font_information;

        GAL_API const FONT_GLYPH_TYPE* LookupGlyph( unsigned int aCodePoint );

    }

}

#endif
