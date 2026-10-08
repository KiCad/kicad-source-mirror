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

// The current font is "Ubuntu Mono" available under Ubuntu Font Licence 1.0
// (see ubuntu-font-licence-1.0.txt for details)
#include <algorithm>
#include <limits>
#include <core/utf8.h>
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


GLYPH_BOX ComputeGlyphBox( const FONT_GLYPH_TYPE& aGlyph )
{
    const float border = font_information.smooth_pixels;

    GLYPH_BOX box;
    box.w = aGlyph.atlas_w - border * 2;
    box.h = aGlyph.atlas_h - border * 2;
    box.atlasX = aGlyph.atlas_x + border;
    box.atlasY = aGlyph.atlas_y + border;
    box.x = aGlyph.minx;

    // The atlas cell height is rounded to whole texels, and every glyph hangs from the font's top
    const float roundAdjust = ( aGlyph.maxy - aGlyph.miny ) - box.h;
    const float topAdjust = font_information.max_y - aGlyph.maxy;
    box.y = roundAdjust + topAdjust;

    return box;
}


double SpaceAdvance()
{
    const FONT_GLYPH_TYPE* x = LookupGlyph( 'x' );

    return x ? x->advance * 0.74 : 0.0;
}


std::pair<VECTOR2D, float> ComputeBitmapTextSize( const UTF8& aText )
{
    static const FONT_GLYPH_TYPE* defaultGlyph = LookupGlyph( '(' ); // for strange chars

    VECTOR2D textSize( 0, 0 );
    float    commonOffset = std::numeric_limits<float>::max();
    float    charHeight = font_information.max_y - defaultGlyph->miny;
    int      overbarDepth = -1;
    int braceNesting = 0;

    for( UTF8::uni_iter chIt = aText.ubegin(), end = aText.uend(); chIt < end; ++chIt )
    {
        if( *chIt == '~' && overbarDepth == -1 )
        {
            UTF8::uni_iter lookahead = chIt;

            if( ++lookahead != end && *lookahead == '{' )
            {
                chIt = lookahead;
                overbarDepth = braceNesting;
                braceNesting++;
                continue;
            }
        }
        else if( *chIt == '{' )
        {
            braceNesting++;
        }
        else if( *chIt == '}' )
        {
            if( braceNesting > 0 )
                braceNesting--;

            if( braceNesting == overbarDepth )
            {
                overbarDepth = -1;
                continue;
            }
        }

        const FONT_GLYPH_TYPE* glyph = LookupGlyph( *chIt );

        if( !glyph                            // Not coded in font
            || *chIt == '-' || *chIt == '_' ) // Strange size of these 2 chars
        {
            glyph = defaultGlyph;
        }

        if( glyph )
            textSize.x += glyph->advance;
    }

    textSize.y = std::max<float>( textSize.y, charHeight );
    commonOffset = std::min<float>( font_information.max_y - defaultGlyph->maxy, commonOffset );
    textSize.y -= commonOffset;

    return std::make_pair( textSize, commonOffset );
}

}
}
