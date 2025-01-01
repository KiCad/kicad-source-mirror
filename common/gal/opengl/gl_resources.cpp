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
* along with this program; if not, you may find one here:
* http://www.gnu.org/licenses/old-licenses/gpl-2.0.html
* or you may search the http://www.gnu.org website for the version 2 license,
* or you may write to the Free Software Foundation, Inc.,
* 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
*/

// The current font is "Ubuntu Mono" available under Ubuntu Font Licence 1.0
// (see ubuntu-font-licence-1.0.txt for details)
#include <algorithm>
#include "gl_resources.h"

#define BITMAP_FONT_USE_SPANS

namespace KIGFX {
namespace BUILTIN_FONT {

#include "bitmap_font_img.c"
#include "bitmap_font_desc.c"

const FONT_GLYPH_TYPE* LookupGlyph( unsigned int aCodepoint )
{
#ifdef BITMAP_FONT_USE_SPANS
    auto *end = font_codepoint_spans + sizeof( font_codepoint_spans ) / sizeof(FONT_SPAN_TYPE);

    auto ptr = std::upper_bound( font_codepoint_spans, end, aCodepoint,
            []( unsigned int codepoint, const FONT_SPAN_TYPE& span )
            {
                return codepoint < span.end;
            } );

    if( ptr != end && ptr->start <= aCodepoint )
    {
        unsigned int index = aCodepoint - ptr->start + ptr->cumulative;
        return &font_codepoint_infos[ index ];
    }
    else
    {
        return nullptr;
    }
#else
    return &bitmap_chars[codepoint];
#endif
}

}
}
