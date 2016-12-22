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
            auto *end = font_codepoint_spans
                      + sizeof( font_codepoint_spans ) / sizeof(FONT_SPAN_TYPE);
            auto ptr = std::upper_bound( font_codepoint_spans, end, aCodepoint,
                []( unsigned int codepoint, const FONT_SPAN_TYPE& span )
                {
                    return codepoint < span.end;
                }
            );

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
