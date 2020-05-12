/*  emf.c - Support for Microsoft Enhanced Metafile Format

    libzint - the open source barcode library
    Copyright (C) 2016-2018 Robin Stuart <rstuart114@gmail.com>

    Redistribution and use in source and binary forms, with or without
    modification, are permitted provided that the following conditions
    are met:

    1. Redistributions of source code must retain the above copyright
       notice, this list of conditions and the following disclaimer.
    2. Redistributions in binary form must reproduce the above copyright
       notice, this list of conditions and the following disclaimer in the
       documentation and/or other materials provided with the distribution.
    3. Neither the name of the project nor the names of its contributors
       may be used to endorse or promote products derived from this software
       without specific prior written permission.

    THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
    ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
    IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
    ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE
    FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
    DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
    OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
    HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
    LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
    OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
    SUCH DAMAGE.
 */

/* Developed according to [MS-EMF] - v20160714, Released July 14, 2016
 * and [MS-WMF] - v20160714, Released July 14, 2016 */

#include <stdio.h>
#include <string.h>
#include <math.h>
#ifdef _MSC_VER
#include <malloc.h>
#endif
#include "common.h"
#include "emf.h"

int count_rectangles(struct zint_symbol *symbol) {
    int rectangles = 0;
    struct zint_vector_rect *rect;

    rect = symbol->vector->rectangles;
    while (rect) {
        rectangles++;
        rect = rect->next;
    }

    return rectangles;
}

int count_circles(struct zint_symbol *symbol) {
    int circles = 0;
    struct zint_vector_circle *circ;

    circ = symbol->vector->circles;
    while (circ) {
        circles++;
        circ = circ->next;
    }

    return circles;
}

int count_hexagons(struct zint_symbol *symbol) {
    int hexagons = 0;
    struct zint_vector_hexagon *hex;

    hex = symbol->vector->hexagons;
    while (hex) {
        hexagons++;
        hex = hex->next;
    }

    return hexagons;
}

int count_strings(struct zint_symbol *symbol) {
    int strings = 0;
    struct zint_vector_string *str;

    str = symbol->vector->strings;
    while (str) {
        strings++;
        str = str->next;
    }

    return strings;
}

void utfle_copy(unsigned char *output, unsigned char *input, int length) {
    int i;
    int o;

    /* Convert UTF-8 to UTF-16LE - only needs to handle characters <= U+00FF */
    i = 0;
    o = 0;
    do {
        if (input[i] <= 0x7f) {
            /* 1 byte mode (7-bit ASCII) */
            output[o] = input[i];
            output[o + 1] = 0x00;
            o += 2;
            i++;
        } else {
            /* 2 byte mode */
            output[o] = ((input[i] & 0x1f) << 6) + (input[i + 1] & 0x3f);
            output[o + 1] = 0x00;
            o += 2;
            i += 2;
        }
    } while (i < length);
}

int bump_up(int input) {
    /* Strings length must be a multiple of 4 bytes */
    if ((input % 2) == 1) {
        input++;
    }
    return input;
}

int emf_plot(struct zint_symbol *symbol) {
    int i,j;
    FILE *emf_file;
    int fgred, fggrn, fgblu, bgred, bggrn, bgblu;
    int error_number = 0;
    int rectangle_count, this_rectangle;
    int circle_count, this_circle;
    int hexagon_count, this_hexagon;
    int string_count, this_text;
    int bytecount, recordcount;
    float radius;
    unsigned char *this_string[6];
    uint32_t spacing;

    float ax, ay, bx, by, cx, cy, dx, dy, ex, ey, fx, fy;

    struct zint_vector_rect *rect;
    struct zint_vector_circle *circ;
    struct zint_vector_hexagon *hex;
    struct zint_vector_string *str;

    emr_header_t emr_header;
    emr_eof_t emr_eof;
    emr_createbrushindirect_t emr_createbrushindirect_fg;
    emr_createbrushindirect_t emr_createbrushindirect_bg;
    emr_selectobject_t emr_selectobject_fgbrush;
    emr_selectobject_t emr_selectobject_bgbrush;
    emr_createpen_t emr_createpen;
    emr_selectobject_t emr_selectobject_pen;
    emr_rectangle_t background;
    emr_extcreatefontindirectw_t emr_extcreatefontindirectw;
    emr_selectobject_t emr_selectobject_font;
    //emr_extcreatefontindirectw_t emr_extcreatefontindirectw_big;
    //emr_selectobject_t emr_selectobject_font_big;

#ifdef _MSC_VER
    emr_rectangle_t *rectangle;
    emr_ellipse_t *circle;
    emr_polygon_t *hexagon;
    emr_exttextoutw_t *text;
#endif

    fgred = (16 * ctoi(symbol->fgcolour[0])) + ctoi(symbol->fgcolour[1]);
    fggrn = (16 * ctoi(symbol->fgcolour[2])) + ctoi(symbol->fgcolour[3]);
    fgblu = (16 * ctoi(symbol->fgcolour[4])) + ctoi(symbol->fgcolour[5]);
    bgred = (16 * ctoi(symbol->bgcolour[0])) + ctoi(symbol->bgcolour[1]);
    bggrn = (16 * ctoi(symbol->bgcolour[2])) + ctoi(symbol->bgcolour[3]);
    bgblu = (16 * ctoi(symbol->bgcolour[4])) + ctoi(symbol->bgcolour[5]);

    rectangle_count = count_rectangles(symbol);
    circle_count = count_circles(symbol);
    hexagon_count = count_hexagons(symbol);
    string_count = count_strings(symbol);

#ifndef _MSC_VER
    emr_rectangle_t rectangle[rectangle_count];
    emr_ellipse_t circle[circle_count];
    emr_polygon_t hexagon[hexagon_count];
    emr_exttextoutw_t text[string_count];
#else
    rectangle = (emr_rectangle_t*) _alloca(rectangle_count * sizeof (emr_rectangle_t));
    circle = (emr_ellipse_t*) _alloca(circle_count * sizeof (emr_ellipse_t));
    hexagon = (emr_polygon_t*) _alloca(hexagon_count * sizeof (emr_polygon_t));
    text = (emr_exttextoutw_t*) _alloca(string_count * sizeof (emr_exttextoutw_t));
#endif
    
    /* Header */
    emr_header.type = 0x00000001; // EMR_HEADER
    emr_header.size = 88; // Assuming no additional data in header
    emr_header.emf_header.bounds.left = 0;
    emr_header.emf_header.bounds.right = ceil(symbol->vector->width);
    emr_header.emf_header.bounds.bottom = ceil(symbol->vector->height);
    emr_header.emf_header.bounds.top = 0;
    emr_header.emf_header.frame.left = 0;
    emr_header.emf_header.frame.right = emr_header.emf_header.bounds.right * 30;
    emr_header.emf_header.frame.top = 0;
    emr_header.emf_header.frame.bottom = emr_header.emf_header.bounds.bottom * 30;
    emr_header.emf_header.record_signature = 0x464d4520; // ENHMETA_SIGNATURE
    emr_header.emf_header.version = 0x00010000;
    emr_header.emf_header.handles = 4; // Number of graphics objects
    emr_header.emf_header.reserved = 0x0000;
    emr_header.emf_header.n_description = 0;
    emr_header.emf_header.off_description = 0;
    emr_header.emf_header.n_pal_entries = 0;
    emr_header.emf_header.device.cx = 1000;
    emr_header.emf_header.device.cy = 1000;
    emr_header.emf_header.millimeters.cx = 300;
    emr_header.emf_header.millimeters.cy = 300;
    bytecount = 88;
    recordcount = 1;

    /* Create Brushes */
    emr_createbrushindirect_fg.type = 0x00000027; // EMR_CREATEBRUSHINDIRECT
    emr_createbrushindirect_fg.size = 24;
    emr_createbrushindirect_fg.ih_brush = 1;
    emr_createbrushindirect_fg.log_brush.brush_style = 0x0000; // BS_SOLID
    emr_createbrushindirect_fg.log_brush.color.red = fgred;
    emr_createbrushindirect_fg.log_brush.color.green = fggrn;
    emr_createbrushindirect_fg.log_brush.color.blue = fgblu;
    emr_createbrushindirect_fg.log_brush.color.reserved = 0;
    emr_createbrushindirect_fg.log_brush.brush_hatch = 0x0006; // HS_SOLIDCLR
    bytecount += 24;
    recordcount++;

    emr_createbrushindirect_bg.type = 0x00000027; // EMR_CREATEBRUSHINDIRECT
    emr_createbrushindirect_bg.size = 24;
    emr_createbrushindirect_bg.ih_brush = 2;
    emr_createbrushindirect_bg.log_brush.brush_style = 0x0000; // BS_SOLID
    emr_createbrushindirect_bg.log_brush.color.red = bgred;
    emr_createbrushindirect_bg.log_brush.color.green = bggrn;
    emr_createbrushindirect_bg.log_brush.color.blue = bgblu;
    emr_createbrushindirect_bg.log_brush.color.reserved = 0;
    emr_createbrushindirect_bg.log_brush.brush_hatch = 0x0006; // HS_SOLIDCLR
    bytecount += 24;
    recordcount++;

    emr_selectobject_fgbrush.type = 0x00000025; // EMR_SELECTOBJECT
    emr_selectobject_fgbrush.size = 12;
    emr_selectobject_fgbrush.ih_object = 1;
    bytecount += 12;
    recordcount++;

    emr_selectobject_bgbrush.type = 0x00000025; // EMR_SELECTOBJECT
    emr_selectobject_bgbrush.size = 12;
    emr_selectobject_bgbrush.ih_object = 2;
    bytecount += 12;
    recordcount++;

    /* Create Pens */
    emr_createpen.type = 0x00000026; // EMR_CREATEPEN
    emr_createpen.size = 28;
    emr_createpen.ih_pen = 3;
    emr_createpen.log_pen.pen_style = 0x00000005; // PS_NULL
    emr_createpen.log_pen.width.x = 1;
    emr_createpen.log_pen.width.y = 0; // ignored
    emr_createpen.log_pen.color_ref.red = 0;
    emr_createpen.log_pen.color_ref.green = 0;
    emr_createpen.log_pen.color_ref.blue = 0;
    emr_createpen.log_pen.color_ref.reserved = 0;
    bytecount += 28;
    recordcount++;

    emr_selectobject_pen.type = 0x00000025; // EMR_SELECTOBJECT
    emr_selectobject_pen.size = 12;
    emr_selectobject_pen.ih_object = 3;
    bytecount += 12;
    recordcount++;

    /* Make background from a rectangle */
    background.type = 0x0000002b; // EMR_RECTANGLE;
    background.size = 24;
    background.box.top = 0;
    background.box.left = 0;
    background.box.right = emr_header.emf_header.bounds.right;
    background.box.bottom = emr_header.emf_header.bounds.bottom;
    bytecount += 24;
    recordcount++;

    //Rectangles
    rect = symbol->vector->rectangles;
    this_rectangle = 0;
    while (rect) {
        rectangle[this_rectangle].type = 0x0000002b; // EMR_RECTANGLE;
        rectangle[this_rectangle].size = 24;
        rectangle[this_rectangle].box.top = rect->y;
        rectangle[this_rectangle].box.bottom = rect->y + rect->height;
        rectangle[this_rectangle].box.left = rect->x;
        rectangle[this_rectangle].box.right = rect->x + rect->width;
        this_rectangle++;
        bytecount += 24;
        recordcount++;
        rect = rect->next;
    }

    //Circles
    circ = symbol->vector->circles;
    this_circle = 0;
    while (circ) {
        radius = circ->diameter / 2.0;
        circle[this_circle].type = 0x0000002a; // EMR_ELLIPSE
        circle[this_circle].size = 24;
        circle[this_circle].box.top = circ->y - radius;
        circle[this_circle].box.bottom = circ->y + radius;
        circle[this_circle].box.left = circ->x - radius;
        circle[this_circle].box.right = circ->x + radius;
        this_circle++;
        bytecount += 24;
        recordcount++;
        circ = circ->next;
    }

    //Hexagons
    hex = symbol->vector->hexagons;
    this_hexagon = 0;
    while (hex) {
        hexagon[this_hexagon].type = 0x00000003; // EMR_POLYGON
        hexagon[this_hexagon].size = 76;
        hexagon[this_hexagon].count = 6;

        radius = hex->diameter / 2.0;
        ay = hex->y + (1.0 * radius);
        by = hex->y + (0.5 * radius);
        cy = hex->y - (0.5 * radius);
        dy = hex->y - (1.0 * radius);
        ey = hex->y - (0.5 * radius);
        fy = hex->y + (0.5 * radius);
        ax = hex->x;
        bx = hex->x + (0.86 * radius);
        cx = hex->x + (0.86 * radius);
        dx = hex->x;
        ex = hex->x - (0.86 * radius);
        fx = hex->x - (0.86 * radius);

        hexagon[this_hexagon].a_points_a.x = ax;
        hexagon[this_hexagon].a_points_a.y = ay;
        hexagon[this_hexagon].a_points_b.x = bx;
        hexagon[this_hexagon].a_points_b.y = by;
        hexagon[this_hexagon].a_points_c.x = cx;
        hexagon[this_hexagon].a_points_c.y = cy;
        hexagon[this_hexagon].a_points_d.x = dx;
        hexagon[this_hexagon].a_points_d.y = dy;
        hexagon[this_hexagon].a_points_e.x = ex;
        hexagon[this_hexagon].a_points_e.y = ey;
        hexagon[this_hexagon].a_points_f.x = fx;
        hexagon[this_hexagon].a_points_f.y = fy;

        hexagon[this_hexagon].bounds.top = hexagon[this_hexagon].a_points_d.y;
        hexagon[this_hexagon].bounds.bottom = hexagon[this_hexagon].a_points_a.y;
        hexagon[this_hexagon].bounds.left = hexagon[this_hexagon].a_points_e.x;
        hexagon[this_hexagon].bounds.right = hexagon[this_hexagon].a_points_c.x;
        this_hexagon++;
        bytecount += 76;
        recordcount++;
        hex = hex->next;
    }

    /* Create font records */
    if (symbol->vector->strings) {
        emr_extcreatefontindirectw.type = 0x00000052; // EMR_EXTCREATEFONTINDIRECTW
        emr_extcreatefontindirectw.size = 104;
        emr_extcreatefontindirectw.ih_fonts = 4;
        emr_extcreatefontindirectw.elw.height = 16 * symbol->scale;
        emr_extcreatefontindirectw.elw.width = 0; // automatic
        emr_extcreatefontindirectw.elw.escapement = 0;
        emr_extcreatefontindirectw.elw.orientation = 0;
        emr_extcreatefontindirectw.elw.weight = 400;
        emr_extcreatefontindirectw.elw.italic = 0x00;
        emr_extcreatefontindirectw.elw.underline = 0x00;
        emr_extcreatefontindirectw.elw.strike_out = 0x00;
        emr_extcreatefontindirectw.elw.char_set = 0x01;
        emr_extcreatefontindirectw.elw.out_precision = 0x00; // OUT_DEFAULT_PRECIS
        emr_extcreatefontindirectw.elw.clip_precision = 0x00; // CLIP_DEFAULT_PRECIS
        emr_extcreatefontindirectw.elw.quality = 0x00;
        emr_extcreatefontindirectw.elw.pitch_and_family = 0x00;
        for (i = 0; i < 64; i++) {
            emr_extcreatefontindirectw.elw.facename[i] = '\0';
        }
        utfle_copy(emr_extcreatefontindirectw.elw.facename, (unsigned char*) "sans-serif", 10);
        bytecount += 104;
        recordcount++;

        emr_selectobject_font.type = 0x00000025; // EMR_SELECTOBJECT
        emr_selectobject_font.size = 12;
        emr_selectobject_font.ih_object = 4;
        bytecount += 12;
        recordcount++;
    }

    //Text
    str = symbol->vector->strings;
    this_text = 0;
    while (str) {
        this_string[this_text] = (unsigned char *) malloc(bump_up(str->length + 1) * 2);
        text[this_text].type = 0x00000054; // EMR_EXTTEXTOUTW
        text[this_text].size = 76 + (6 * bump_up(str->length + 1));
        text[this_text].bounds.top = 0; // ignored
        text[this_text].bounds.left = 0; // ignored
        text[this_text].bounds.right = 0xffffffff; // ignored
        text[this_text].bounds.bottom = 0xffffffff; // ignored
        text[this_text].i_graphics_mode = 0x00000001; // GM_COMPATIBLE
        text[this_text].ex_scale = 1.0;
        text[this_text].ey_scale = 1.0;
        text[this_text].w_emr_text.reference.x = str->x - (4 * str->length * symbol->scale); // text left
        text[this_text].w_emr_text.reference.y = str->y - (16 * symbol->scale); // text top
        text[this_text].w_emr_text.chars = str->length;
        text[this_text].w_emr_text.off_string = 76;
        text[this_text].w_emr_text.options = 0;
        text[this_text].w_emr_text.rectangle.top = 0;
        text[this_text].w_emr_text.rectangle.left = 0;
        text[this_text].w_emr_text.rectangle.right = 0xffffffff;
        text[this_text].w_emr_text.rectangle.bottom = 0xffffffff;
        text[this_text].w_emr_text.off_dx = 76 + (2 * bump_up(str->length + 1));
        for (i = 0; i < bump_up(str->length + 1) * 2; i++) {
            this_string[this_text][i] = '\0';
        }
        utfle_copy(this_string[this_text], str->text, str->length);
        bytecount += 76 + (6 * bump_up(str->length + 1));
        recordcount++;

        this_text++;
        str = str->next;
    }

    /* Create EOF record */
    emr_eof.type = 0x0000000e; // EMR_EOF
    emr_eof.size = 18; // Assuming no palette entries
    emr_eof.n_pal_entries = 0;
    emr_eof.off_pal_entries = 0;
    emr_eof.size_last = emr_eof.size;
    bytecount += 18;
    recordcount++;

    if (symbol->symbology == BARCODE_MAXICODE) {
        bytecount += 5 * sizeof (emr_selectobject_t);
        recordcount += 5;
    }

    /* Put final counts in header */
    emr_header.emf_header.bytes = bytecount;
    emr_header.emf_header.records = recordcount;

    /* Send EMF data to file */
    if (symbol->output_options & BARCODE_STDOUT) {
        emf_file = stdout;
    } else {
        emf_file = fopen(symbol->outfile, "wb");
    }
    if (emf_file == NULL) {
        strcpy(symbol->errtxt, "640: Could not open output file");
        return ZINT_ERROR_FILE_ACCESS;
    }

    fwrite(&emr_header, sizeof (emr_header_t), 1, emf_file);

    fwrite(&emr_createbrushindirect_fg, sizeof (emr_createbrushindirect_t), 1, emf_file);
    fwrite(&emr_createbrushindirect_bg, sizeof (emr_createbrushindirect_t), 1, emf_file);
    fwrite(&emr_createpen, sizeof (emr_createpen_t), 1, emf_file);

    if (symbol->vector->strings) {
        fwrite(&emr_extcreatefontindirectw, sizeof (emr_extcreatefontindirectw_t), 1, emf_file);
    }

    fwrite(&emr_selectobject_bgbrush, sizeof (emr_selectobject_t), 1, emf_file);
    fwrite(&emr_selectobject_pen, sizeof (emr_selectobject_t), 1, emf_file);
    fwrite(&background, sizeof (emr_rectangle_t), 1, emf_file);

    fwrite(&emr_selectobject_fgbrush, sizeof (emr_selectobject_t), 1, emf_file);

    // Rectangles
    for (i = 0; i < rectangle_count; i++) {
        fwrite(&rectangle[i], sizeof (emr_rectangle_t), 1, emf_file);
    }

    // Hexagons
    for (i = 0; i < hexagon_count; i++) {
        fwrite(&hexagon[i], sizeof (emr_polygon_t), 1, emf_file);
    }

    // Circles
    if (symbol->symbology == BARCODE_MAXICODE) {
        // Bullseye needed
        for (i = 0; i < circle_count; i++) {
            fwrite(&circle[i], sizeof (emr_ellipse_t), 1, emf_file);
            if (i < circle_count - 1) {
                if (i % 2) {
                    fwrite(&emr_selectobject_fgbrush, sizeof (emr_selectobject_t), 1, emf_file);
                } else {
                    fwrite(&emr_selectobject_bgbrush, sizeof (emr_selectobject_t), 1, emf_file);
                }
            }
        }
    } else {
        for (i = 0; i < circle_count; i++) {
            fwrite(&circle[i], sizeof (emr_ellipse_t), 1, emf_file);
        }
    }

    // Text
    for (i = 0; i < string_count; i++) {
        spacing = 8 * symbol->scale;
        fwrite(&emr_selectobject_font, sizeof (emr_selectobject_t), 1, emf_file);
        fwrite(&text[i], sizeof (emr_exttextoutw_t), 1, emf_file);
        fwrite(this_string[i], bump_up(text[i].w_emr_text.chars + 1) * 2, 1, emf_file);
        for (j = 0; j < bump_up(text[i].w_emr_text.chars + 1); j++) {
            fwrite(&spacing, 4, 1, emf_file);
        }
    }

    fwrite(&emr_eof, sizeof (emr_eof_t), 1, emf_file);

    if (symbol->output_options & BARCODE_STDOUT) {
        fflush(emf_file);
    } else {
        fclose(emf_file);
    }
    return error_number;
}
