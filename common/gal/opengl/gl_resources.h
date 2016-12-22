#ifndef GAL_OPENGL_RESOURCES_H___
#define GAL_OPENGL_RESOURCES_H___

#define BITMAP_FONT_USE_SPANS

namespace KIGFX {

    namespace BUILTIN_FONT {

        struct FONT_IMAGE_TYPE {
            unsigned int width, height;
            unsigned int char_border;
            unsigned int spacing;
            unsigned char pixels[1024 * 1024 * 3];
        };

        struct FONT_INFO_TYPE {
            unsigned int smooth_pixels;
            float min_y;
            float max_y;
        };

        struct FONT_SPAN_TYPE {
            unsigned int start;
            unsigned int end;
            unsigned int cumulative;
        };

        struct FONT_GLYPH_TYPE {
            unsigned int atlas_x, atlas_y;
            unsigned int atlas_w, atlas_h;
            float minx, maxx;
            float miny, maxy;
            float advance;
        };

        extern FONT_IMAGE_TYPE font_image;
        extern FONT_INFO_TYPE  font_information;

        const FONT_GLYPH_TYPE* LookupGlyph( unsigned int aCodePoint );

    }

}

#endif
