/* raster.c - Handles output to raster files */

/*
    libzint - the open source barcode library
    Copyright (C) 2009-2017 Robin Stuart <rstuart114@gmail.com>

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

#include <stdio.h>
#ifdef _MSC_VER
#include <fcntl.h>
#include <io.h>
#endif
#include <stdlib.h>
#include <string.h>
#include "common.h"

#ifdef _MSC_VER
#include <malloc.h>
#endif /* _MSC_VER */

#include "font.h" /* Font for human readable text */

#define SSET	"0123456789ABCDEF"

#ifndef NO_PNG
extern int png_pixel_plot(struct zint_symbol *symbol, char *pixelbuf);
#endif /* NO_PNG */
extern int bmp_pixel_plot(struct zint_symbol *symbol, char *pixelbuf);
extern int pcx_pixel_plot(struct zint_symbol *symbol, char *pixelbuf);
extern int gif_pixel_plot(struct zint_symbol *symbol, char *pixelbuf);
extern int tif_pixel_plot(struct zint_symbol *symbol, char *pixelbuf);

void buffer_plot(struct zint_symbol *symbol, char *pixelbuf) {
    /* Place pixelbuffer into symbol */
    int fgred, fggrn, fgblu, bgred, bggrn, bgblu;
    int row, column, i;

    symbol->bitmap = (char *) malloc(symbol->bitmap_width * symbol->bitmap_height * 3);

    fgred = (16 * ctoi(symbol->fgcolour[0])) + ctoi(symbol->fgcolour[1]);
    fggrn = (16 * ctoi(symbol->fgcolour[2])) + ctoi(symbol->fgcolour[3]);
    fgblu = (16 * ctoi(symbol->fgcolour[4])) + ctoi(symbol->fgcolour[5]);
    bgred = (16 * ctoi(symbol->bgcolour[0])) + ctoi(symbol->bgcolour[1]);
    bggrn = (16 * ctoi(symbol->bgcolour[2])) + ctoi(symbol->bgcolour[3]);
    bgblu = (16 * ctoi(symbol->bgcolour[4])) + ctoi(symbol->bgcolour[5]);

    for (row = 0; row < symbol->bitmap_height; row++) {
        for (column = 0; column < symbol->bitmap_width; column++) {
            i = ((row * symbol->bitmap_width) + column) * 3;
            switch (*(pixelbuf + (symbol->bitmap_width * row) + column)) {
                case '1':
                    symbol->bitmap[i] = fgred;
                    symbol->bitmap[i + 1] = fggrn;
                    symbol->bitmap[i + 2] = fgblu;
                    break;
                default:
                    symbol->bitmap[i] = bgred;
                    symbol->bitmap[i + 1] = bggrn;
                    symbol->bitmap[i + 2] = bgblu;
                    break;

            }
        }
    }
}

int save_raster_image_to_file(struct zint_symbol *symbol, int image_height, int image_width, char *pixelbuf, int rotate_angle, int image_type) {
    int error_number;
    int row, column;

    char *rotated_pixbuf;

    if (!(rotated_pixbuf = (char *) malloc(image_width * image_height))) {
        strcpy(symbol->errtxt, "650: Insufficient memory for pixel buffer");
        return ZINT_ERROR_ENCODING_PROBLEM;
    }

    switch (rotate_angle) {
        case 0:
        case 180:
            symbol->bitmap_width = image_width;
            symbol->bitmap_height = image_height;
            break;
        case 90:
        case 270:
            symbol->bitmap_width = image_height;
            symbol->bitmap_height = image_width;
            break;
    }

    /* sort out colour options */
    to_upper((unsigned char*) symbol->fgcolour);
    to_upper((unsigned char*) symbol->bgcolour);

    if (strlen(symbol->fgcolour) != 6) {
        strcpy(symbol->errtxt, "651: Malformed foreground colour target");
        free(rotated_pixbuf);
        return ZINT_ERROR_INVALID_OPTION;
    }
    if (strlen(symbol->bgcolour) != 6) {
        strcpy(symbol->errtxt, "652: Malformed background colour target");
        free(rotated_pixbuf);
        return ZINT_ERROR_INVALID_OPTION;
    }
    error_number = is_sane(SSET, (unsigned char*) symbol->fgcolour, strlen(symbol->fgcolour));
    if (error_number == ZINT_ERROR_INVALID_DATA) {
        strcpy(symbol->errtxt, "653: Malformed foreground colour target");
        free(rotated_pixbuf);
        return ZINT_ERROR_INVALID_OPTION;
    }
    error_number = is_sane(SSET, (unsigned char*) symbol->bgcolour, strlen(symbol->fgcolour));
    if (error_number == ZINT_ERROR_INVALID_DATA) {
        strcpy(symbol->errtxt, "654: Malformed background colour target");
        free(rotated_pixbuf);
        return ZINT_ERROR_INVALID_OPTION;
    }

    /* Rotate image before plotting */
    switch (rotate_angle) {
        case 0: /* Plot the right way up */
            for (row = 0; row < image_height; row++) {
                for (column = 0; column < image_width; column++) {
                    rotated_pixbuf[(row * image_width) + column] =
                            pixelbuf[(image_width * row) + column];
                }
            }
            break;
        case 90: /* Plot 90 degrees clockwise */
            for (row = 0; row < image_width; row++) {
                for (column = 0; column < image_height; column++) {
                    rotated_pixbuf[(row * image_height) + column] =
                            *(pixelbuf + (image_width * (image_height - column - 1)) + row);
                }
            }
            break;
        case 180: /* Plot upside down */
            for (row = 0; row < image_height; row++) {
                for (column = 0; column < image_width; column++) {
                    rotated_pixbuf[(row * image_width) + column] =
                            *(pixelbuf + (image_width * (image_height - row - 1)) + (image_width - column - 1));
                }
            }
            break;
        case 270: /* Plot 90 degrees anti-clockwise */
            for (row = 0; row < image_width; row++) {
                for (column = 0; column < image_height; column++) {
                    rotated_pixbuf[(row * image_height) + column] =
                            *(pixelbuf + (image_width * column) + (image_width - row - 1));
                }
            }
            break;
    }

    switch (image_type) {
        case OUT_BUFFER:
            buffer_plot(symbol, rotated_pixbuf);
            error_number = 0;
            break;
        case OUT_PNG_FILE:
#ifndef NO_PNG
            error_number = png_pixel_plot(symbol, rotated_pixbuf);
#else
            free(rotated_pixbuf);
            return ZINT_ERROR_INVALID_OPTION;
#endif
            break;
        case OUT_PCX_FILE:
            error_number = pcx_pixel_plot(symbol, rotated_pixbuf);
            break;
        case OUT_GIF_FILE:
            error_number = gif_pixel_plot(symbol, rotated_pixbuf);
            break;
        case OUT_TIF_FILE:
            error_number = tif_pixel_plot(symbol, rotated_pixbuf);
            break;
        default:
            error_number = bmp_pixel_plot(symbol, rotated_pixbuf);
            break;
    }

    free(rotated_pixbuf);
    return error_number;
}

void draw_bar(char *pixelbuf, int xpos, int xlen, int ypos, int ylen, int image_width, int image_height) {
    /* Draw a rectangle */
    int i, j, png_ypos;

    png_ypos = image_height - ypos - ylen;
    /* This fudge is needed because EPS measures height from the bottom up but
    PNG measures y position from the top down */

    for (i = (xpos); i < (xpos + xlen); i++) {
        for (j = (png_ypos); j < (png_ypos + ylen); j++) {
            *(pixelbuf + (image_width * j) + i) = '1';
        }
    }
}

void draw_circle(char *pixelbuf, int image_width, int image_height, int x0, int y0, float radius, char fill) {
    int x, y;
    int radius_i = (int) radius;

    for (y = -radius_i; y <= radius_i; y++) {
        for (x = -radius_i; x <= radius_i; x++) {
            if ((x * x) + (y * y) <= (radius_i * radius_i)) {
                if ((y + y0 >= 0) && (y + y0 < image_height)
                        && (x + x0 >= 0) && (x + x0 < image_width)) {
                    *(pixelbuf + ((y + y0) * image_width) + (x + x0)) = fill;
                }
            }
        }
    }
}

void draw_bullseye(char *pixelbuf, int image_width, int image_height, int cx, int cy, int scaler) {
    /* Central bullseye in Maxicode symbols */

    draw_circle(pixelbuf, image_width, image_height, cx, cy, (int)(4.571 * scaler) + 1, '1');
    draw_circle(pixelbuf, image_width, image_height, cx, cy, (int)(3.779 * scaler) + 1, '0');
    draw_circle(pixelbuf, image_width, image_height, cx, cy, (int)(2.988 * scaler) + 1, '1');
    draw_circle(pixelbuf, image_width, image_height, cx, cy, (int)(2.196 * scaler) + 1, '0');
    draw_circle(pixelbuf, image_width, image_height, cx, cy, (int)(1.394 * scaler) + 1, '1');
    draw_circle(pixelbuf, image_width, image_height, cx, cy, (int)(0.602 * scaler) + 1, '0');
}

void draw_hexagon(char *pixelbuf, int image_width, char *scaled_hexagon, int hexagon_size, int xposn, int yposn) {
    /* Put a hexagon into the pixel buffer */
    int i, j;

    for (i = 0; i < hexagon_size; i++) {
        for (j = 0; j < hexagon_size; j++) {
            if (scaled_hexagon[(i * hexagon_size) + j] == '1') {
                *(pixelbuf + (image_width * i) + (image_width * yposn) + xposn + j) = '1';
            }
        }
    }
}

void draw_letter(char *pixelbuf, unsigned char letter, int xposn, int yposn, int textflags, int image_width, int image_height) {
    /* Put a letter into a position */
    int skip;

    skip = 0;

    if (letter < 33) {
        skip = 1;
    }

    if ((letter > 127) && (letter < 161)) {
        skip = 1;
    }

    if (xposn < 0 || yposn < 0) {
        skip = 1;
    }

    if (skip == 0) {
        int glyph_no;
        int x, y;
        if (letter > 128) {
            glyph_no = letter - 66;
        } else {
            glyph_no = letter - 33;
        }


        switch (textflags) {
            int max_x, max_y;
            case 1: // small font 5x9
                max_x = 5;
                max_y = 9;

                if (xposn + max_x >= image_width) {
                    max_x = image_width - xposn - 1;
                }

                if (yposn + max_y >= image_height) {
                    max_y = image_height - yposn - 1;
                }

                for (y = 0; y < max_y; y++) {
                    for (x = 0; x < max_x; x++) {
                        if (small_font[(glyph_no * 9) + y] & (0x10 >> x)) {
                            *(pixelbuf + (y * image_width) + (yposn * image_width) + xposn + x) = '1';
                        }
                    }
                }
                break;

            case 2: // bold font -> twice the regular font
            {
                char * linePtr;
                max_x = 7;
                max_y = 14;

                if (xposn + max_x + 1 >= image_width) {
                    max_x = image_width - xposn - 2;
                }

                if (yposn + max_y >= image_height) {
                    max_y = image_height - yposn - 1;
                }

                linePtr = pixelbuf + (yposn * image_width) + xposn + 1;
                for (y = 0; y < max_y; y++) {
                    char * pixelPtr = linePtr;
                    int extra_dot = 0;
                    for (x = 0; x < 7; x++) {
                        if (ascii_font[(glyph_no * 14) + y] & (0x40 >> x)) {
                            *pixelPtr = '1';
                            extra_dot = 1;
                        } else {
                            if (extra_dot) {
                                *pixelPtr = '1';
                            }

                            extra_dot = 0;
                        }

                        ++pixelPtr;
                    }

                    if (extra_dot) {
                        *pixelPtr = '1';
                    }

                    linePtr += image_width;
                }
            }
                break;

            default: // regular font 7x15
                max_x = 7;
                max_y = 14;

                if (xposn + max_x >= image_width) {
                    max_x = image_width - xposn - 1;
                }

                if (yposn + max_y >= image_height) {
                    max_y = image_height - yposn - 1;
                }

                for (y = 0; y < max_y; y++) {
                    for (x = 0; x < 7; x++) {
                        if (ascii_font[(glyph_no * 14) + y] & (0x40 >> x)) {
                            *(pixelbuf + (y * image_width) + (yposn * image_width) + xposn + x) = '1';
                        }
                    }
                }
                break;
        }
    }
}

/* Plot a string into the pixel buffer */
void draw_string(char *pixbuf, char input_string[], int xposn, int yposn, int textflags, int image_width, int image_height) {
    int i, string_length, string_left_hand, letter_width = 7;

    switch (textflags) {
        case 1: // small font 5x9
            letter_width = 5;
            break;

        case 2: // bold font -> width of the regular font + 1 extra dot + 1 extra space
            letter_width = 9;
            break;

        default: // regular font 7x15
            letter_width = 7;
            break;
    }

    string_length = strlen(input_string);
    string_left_hand = xposn - ((letter_width * string_length) / 2);

    for (i = 0; i < string_length; i++) {
        draw_letter(pixbuf, input_string[i], string_left_hand + (i * letter_width), yposn, textflags, image_width, image_height);
    }

}

void plot_hexline(char *scaled_hexagon, int hexagon_size, float start_x, float start_y, float end_x, float end_y) {
    /* Draw a straight line from start to end */
    int i;
    float inc_x, inc_y;

    inc_x = (end_x - start_x) / hexagon_size;
    inc_y = (end_y - start_y) / hexagon_size;

    for (i = 0; i < hexagon_size; i++) {
        float this_x = start_x + ((float)i * inc_x);
        float this_y = start_y + ((float)i * inc_y);
        if (((this_x >= 0) && (this_x < hexagon_size)) && ((this_y >= 0) && (this_y < hexagon_size))) {
                scaled_hexagon[(hexagon_size * (int)this_y) + (int)this_x] = '1';
        }
    }
}

void plot_hexagon(char *scaled_hexagon, int hexagon_size) {
    /* Create a hexagon shape and fill it */
    int line, i;

    float x_offset[6];
    float y_offset[6];
    float start_x, start_y;
    float end_x, end_y;

    x_offset[0] = 0.0;
    x_offset[1] = 0.86;
    x_offset[2] = 0.86;
    x_offset[3] = 0.0;
    x_offset[4] = -0.86;
    x_offset[5] = -0.86;

    y_offset[0] = 1.0;
    y_offset[1] = 0.5;
    y_offset[2] = -0.5;
    y_offset[3] = -1.0;
    y_offset[4] = -0.5;
    y_offset[5] = 0.5;

    /* Plot hexagon outline */
    for (line = 0; line < 5; line++) {
        start_x = ((float)hexagon_size / 2.0) + (((float)hexagon_size / 2.0) * x_offset[line]);
        start_y = ((float)hexagon_size / 2.0) + (((float)hexagon_size / 2.0) * y_offset[line]);
        end_x = ((float)hexagon_size / 2.0) + (((float)hexagon_size / 2.0) * x_offset[line + 1]);
        end_y = ((float)hexagon_size / 2.0) + (((float)hexagon_size / 2.0) * y_offset[line + 1]);
        plot_hexline(scaled_hexagon, hexagon_size, start_x, start_y, end_x, end_y);
    }
    start_x = ((float)hexagon_size / 2.0) + (((float)hexagon_size / 2.0) * x_offset[line]);
    start_y = ((float)hexagon_size / 2.0) + (((float)hexagon_size / 2.0) * y_offset[line]);
    end_x = ((float)hexagon_size / 2.0) + (((float)hexagon_size / 2.0) * x_offset[0]);
    end_y = ((float)hexagon_size / 2.0) + (((float)hexagon_size / 2.0) * y_offset[0]);
    plot_hexline(scaled_hexagon, hexagon_size, start_x, start_y, end_x, end_y);

    /* Fill hexagon */
    for (line = 0; line < hexagon_size; line++) {
        char ink = '0';
        for (i = 0; i < hexagon_size; i++) {
            if (scaled_hexagon[(hexagon_size * line) + i] == '1') {
                if (i < (hexagon_size / 2)) {
                    ink = '1';
                } else {
                    ink = '0';
                }
            }

            if (ink == '1') {
                scaled_hexagon[(hexagon_size * line) + i] = ink;
            }
        }
    }
}

int plot_raster_maxicode(struct zint_symbol *symbol, int rotate_angle, int data_type) {
    /* Plot a MaxiCode symbol with hexagons and bullseye */
    int i, row, column, xposn;
    int image_height, image_width;
    char *pixelbuf;
    int error_number;
    int xoffset, yoffset;
    float scaler = symbol->scale;
    char *scaled_hexagon;
    int hexagon_size;

    xoffset = symbol->border_width + symbol->whitespace_width;
    yoffset = symbol->border_width;
    image_width = (300 + (2 * xoffset * 2)) * scaler;
    image_height = (300 + (2 * yoffset * 2)) * scaler;

    if (!(pixelbuf = (char *) malloc(image_width * image_height))) {
        strcpy(symbol->errtxt, "655: Insufficient memory for pixel buffer");
        return ZINT_ERROR_ENCODING_PROBLEM;
    } else {
        for (i = 0; i < (image_width * image_height); i++) {
            *(pixelbuf + i) = '0';
        }
    }

    hexagon_size = (int)scaler * 10;

    if (!(scaled_hexagon = (char *) malloc(hexagon_size * hexagon_size))) {
        strcpy(symbol->errtxt, "656: Insufficient memory for pixel buffer");
        free(pixelbuf);
        return ZINT_ERROR_ENCODING_PROBLEM;
    } else {
        for (i = 0; i < (hexagon_size * hexagon_size); i++) {
            *(scaled_hexagon + i) = '0';
        }
    }

    plot_hexagon(scaled_hexagon, hexagon_size);

    for (row = 0; row < symbol->rows; row++) {
        int yposn = row * 9;
        for (column = 0; column < symbol->width; column++) {
            xposn = column * 10;
            if (module_is_set(symbol, row, column)) {
                if (row & 1) {
                    /* Odd (reduced) row */
                    xposn += 5;
                    draw_hexagon(pixelbuf, image_width, scaled_hexagon, hexagon_size, (xposn + (2 * xoffset)) * scaler, (yposn + (2 * yoffset)) * scaler);
                } else {
                    /* Even (full) row */
                    draw_hexagon(pixelbuf, image_width, scaled_hexagon, hexagon_size, (xposn + (2 * xoffset)) * scaler, (yposn + (2 * yoffset)) * scaler);
                }
            }
        }
    }

    draw_bullseye(pixelbuf, image_width, image_height, (int)(((14.5 * 10.0) + (2.0 * xoffset)) * scaler), (int)(((16.5 * 9.0) + (2.0 * yoffset)) * scaler), scaler * 10);

    // Virtual hexagon
    //draw_hexagon(pixelbuf, image_width, scaled_hexagon, hexagon_size, ((14 * 10) + (2 * xoffset)) * scaler, ((16 * 9) + (2 * yoffset)) * scaler);

    if ((symbol->output_options & BARCODE_BOX) || (symbol->output_options & BARCODE_BIND)) {
        /* boundary bars */
        draw_bar(pixelbuf, 0, image_width, 0, symbol->border_width * 2, image_width, image_height);
        draw_bar(pixelbuf, 0, image_width, 300 + (symbol->border_width * 2), symbol->border_width * 2, image_width, image_height);
    }

    if (symbol->output_options & BARCODE_BOX) {
        /* side bars */
        draw_bar(pixelbuf, 0, symbol->border_width * 2, 0, image_height, image_width, image_height);
        draw_bar(pixelbuf, 300 + ((symbol->border_width + symbol->whitespace_width + symbol->whitespace_width) * 2), symbol->border_width * 2, 0, image_height, image_width, image_height);
    }

    error_number = save_raster_image_to_file(symbol, image_height, image_width, pixelbuf, rotate_angle, data_type);
    free(scaled_hexagon);
    free(pixelbuf);
    return error_number;
}

/* Convert UTF-8 to Latin1 Codepage for the interpretation line */
void to_latin1(unsigned char source[], unsigned char preprocessed[]) {
    int j, i, input_length;

    input_length = ustrlen(source);

    j = 0;
    i = 0;
    while (i < input_length) {
        switch (source[i]) {
            case 0xC2:
                /* UTF-8 C2xxh */
                /* Character range: C280h (latin: 80h) to C2BFh (latin: BFh) */
                i++;
                preprocessed[j] = source[i];
                j++;
                break;
            case 0xC3:
                /* UTF-8 C3xx */
                /* Character range: C380h (latin: C0h) to C3BFh (latin: FFh) */
                i++;
                preprocessed[j] = source[i] + 64;
                j++;
                break;
            default:
                /* Process ASCII (< 80h), all other unicode points are ignored */
                if (source[i] < 128) {
                    preprocessed[j] = source[i];
                    j++;
                }
                break;
        }
        i++;
    }
    preprocessed[j] = '\0';

    return;
}

int plot_raster_dotty(struct zint_symbol *symbol, int rotate_angle, int data_type) {
    float scaler = 2 * symbol->scale;
    char *scaled_pixelbuf;
    int r, i;
    int scale_width, scale_height;
    int error_number = 0;
    int xoffset, yoffset, image_width, image_height;

    symbol->height = symbol->rows; // This is true because only 2d matrix symbols are processed here

    xoffset = symbol->border_width + symbol->whitespace_width;
    yoffset = symbol->border_width;
    image_width = symbol->width + xoffset + xoffset;
    image_height = symbol->height + yoffset + yoffset;

    if (scaler < 2.0) {
        scaler = 2.0;
    }
    scale_width = (image_width * scaler) + 1;
    scale_height = (image_height * scaler) + 1;

    /* Apply scale options by creating another pixel buffer */
    if (!(scaled_pixelbuf = (char *) malloc(scale_width * scale_height))) {
        strcpy(symbol->errtxt, "657: Insufficient memory for pixel buffer");
        return ZINT_ERROR_ENCODING_PROBLEM;
    } else {
        for (i = 0; i < (scale_width * scale_height); i++) {
            *(scaled_pixelbuf + i) = '0';
        }
    }

    /* Plot the body of the symbol to the pixel buffer */
    for (r = 0; r < symbol->rows; r++) {
        for (i = 0; i < symbol->width; i++) {
            if (module_is_set(symbol, r, i)) {
                draw_circle(scaled_pixelbuf, scale_width, scale_height,
                        (int) ((i + xoffset) * scaler) + (scaler / 2.0),
                        (int) ((r + yoffset) * scaler) + (scaler / 2.0),
                        (symbol->dot_size / 2.0) * scaler,
                        '1');
            }
        }
    }

    error_number = save_raster_image_to_file(symbol, scale_height, scale_width, scaled_pixelbuf, rotate_angle, data_type);
    free(scaled_pixelbuf);

    return error_number;
}

int plot_raster_default(struct zint_symbol *symbol, int rotate_angle, int data_type) {
    int textdone, main_width, comp_offset, large_bar_count;
    char textpart[10], addon[6];
    float addon_text_posn, preset_height, large_bar_height;
    int i, r, textoffset, yoffset, xoffset, latch, image_width, image_height;
    char *pixelbuf;
    int addon_latch = 0, textflags = 0;
    int block_width, textpos;
    float row_height, row_posn;
    int error_number;
    int default_text_posn;
    int next_yposn;
    float scaler = symbol->scale;
    char *scaled_pixelbuf;
    int horiz, vert;
    int scale_width, scale_height;
#ifndef _MSC_VER
    unsigned char local_text[ustrlen(symbol->text) + 1];
#else
    unsigned char* local_text = (unsigned char*) _alloca(ustrlen(symbol->text) + 1);
#endif

    if (symbol->show_hrt != 0) {
        /* Copy text from symbol */
        to_latin1(symbol->text, local_text);
    } else {
        /* No text needed */
        switch (symbol->symbology) {
            case BARCODE_EANX:
            case BARCODE_EANX_CC:
            case BARCODE_ISBNX:
            case BARCODE_UPCA:
            case BARCODE_UPCE:
            case BARCODE_UPCA_CC:
            case BARCODE_UPCE_CC:
                /* For these symbols use dummy text to ensure formatting is done
                 * properly even if no text is required */
                for (i = 0; i < ustrlen(symbol->text); i++) {
                    if (symbol->text[i] == '+') {
                        local_text[i] = '+';
                    } else {
                        local_text[i] = ' ';
                    }
                    local_text[ustrlen(symbol->text)] = '\0';
                }
                break;
            default:
                /* For everything else, just remove the text */
                local_text[0] = '\0';
                break;
        }
    }

    textdone = 0;
    main_width = symbol->width;
    strcpy(addon, "");
    comp_offset = 0;
    addon_text_posn = 0.0;
    row_height = 0;
    if (symbol->output_options & SMALL_TEXT) {
        textflags = 1;
    } else if (symbol->output_options & BOLD_TEXT) {
        textflags = 2;
    }

    if (symbol->height == 0) {
        symbol->height = 50;
    }

    large_bar_count = 0;
    preset_height = 0.0;
    for (i = 0; i < symbol->rows; i++) {
        preset_height += symbol->row_height[i];
        if (symbol->row_height[i] == 0) {
            large_bar_count++;
        }
    }

    if (large_bar_count == 0) {
        symbol->height = preset_height;
        large_bar_height = 10;
    } else {
        large_bar_height = (symbol->height - preset_height) / large_bar_count;
    }

    while (!(module_is_set(symbol, symbol->rows - 1, comp_offset))) {
        comp_offset++;
    }

    /* Certain symbols need whitespace otherwise characters get chopped off the sides */
    if ((((symbol->symbology == BARCODE_EANX) && (symbol->rows == 1)) || (symbol->symbology == BARCODE_EANX_CC))
            || (symbol->symbology == BARCODE_ISBNX)) {
        switch (ustrlen(local_text)) {
            case 13: /* EAN 13 */
            case 16:
            case 19:
                if (symbol->whitespace_width == 0) {
                    symbol->whitespace_width = 10;
                }
                main_width = 96 + comp_offset;
                break;
            default:
                main_width = 68 + comp_offset;
        }
    }

    if (((symbol->symbology == BARCODE_UPCA) && (symbol->rows == 1)) || (symbol->symbology == BARCODE_UPCA_CC)) {
        if (symbol->whitespace_width == 0) {
            symbol->whitespace_width = 10;
            main_width = 96 + comp_offset;
        }
    }

    if (((symbol->symbology == BARCODE_UPCE) && (symbol->rows == 1)) || (symbol->symbology == BARCODE_UPCE_CC)) {
        if (symbol->whitespace_width == 0) {
            symbol->whitespace_width = 10;
            main_width = 51 + comp_offset;
        }
    }

    latch = 0;
    r = 0;
    /* Isolate add-on text */
    if (is_extendable(symbol->symbology)) {
        for (i = 0; i < ustrlen(local_text); i++) {
            if (latch == 1) {
                addon[r] = local_text[i];
                r++;
            }
            if (symbol->text[i] == '+') {
                latch = 1;
            }
        }
    }
    addon[r] = '\0';

    if (ustrlen(local_text) != 0) {
        textoffset = 9;
    } else {
        textoffset = 0;
    }

    xoffset = symbol->border_width + symbol->whitespace_width;
    yoffset = symbol->border_width;
    image_width = 2 * (symbol->width + xoffset + xoffset);
    image_height = 2 * (symbol->height + textoffset + yoffset + yoffset);

    if (!(pixelbuf = (char *) malloc(image_width * image_height))) {
        strcpy(symbol->errtxt, "658: Insufficient memory for pixel buffer");
        return ZINT_ERROR_ENCODING_PROBLEM;
    } else {
        for (i = 0; i < (image_width * image_height); i++) {
            *(pixelbuf + i) = '0';
        }
    }

    if ((symbol->output_options & BARCODE_BOX) || (symbol->output_options & BARCODE_BIND)) {
        default_text_posn = image_height - 17;
    } else {
        default_text_posn = image_height - 17 - symbol->border_width - symbol->border_width;
    }

    row_posn = textoffset + yoffset;
    next_yposn = textoffset + yoffset;
    row_height = 0;

    /* Plot the body of the symbol to the pixel buffer */
    for (r = 0; r < symbol->rows; r++) {
        int plot_yposn;
        int plot_height;
        int this_row = symbol->rows - r - 1; /* invert r otherwise plots upside down */
        row_posn += row_height;
        plot_yposn = next_yposn;
        if (symbol->row_height[this_row] == 0) {
            row_height = large_bar_height;
        } else {
            row_height = symbol->row_height[this_row];
        }
        next_yposn = (int) (row_posn + row_height);
        plot_height = next_yposn - plot_yposn;

        i = 0;
        if (module_is_set(symbol, this_row, 0)) {
            latch = 1;
        } else {
            latch = 0;
        }

        do {
            block_width = 0;
            do {
                block_width++;
            } while ((i + block_width < symbol->width )&& module_is_set(symbol, this_row, i + block_width) == module_is_set(symbol, this_row, i));
            if ((addon_latch == 0) && (r == 0) && (i > main_width)) {
                plot_height = (int) (row_height - 5.0);
                plot_yposn = (int) (row_posn - 5.0);
                addon_text_posn = row_posn + row_height - 8.0;
                addon_latch = 1;
            }
            if (latch == 1) {
                /* a bar */
                draw_bar(pixelbuf, (i + xoffset) * 2, block_width * 2, plot_yposn * 2, plot_height * 2, image_width, image_height);
                latch = 0;
            } else {
                /* a space */
                latch = 1;
            }
            i += block_width;

        } while (i < symbol->width);
    }

    xoffset += comp_offset;

    if ((((symbol->symbology == BARCODE_EANX) && (symbol->rows == 1)) || (symbol->symbology == BARCODE_EANX_CC)) || (symbol->symbology == BARCODE_ISBNX)) {
        /* guard bar extensions and text formatting for EAN8 and EAN13 */
        switch (ustrlen(local_text)) {
            case 8: /* EAN-8 */
            case 11:
            case 14:
                draw_bar(pixelbuf, (0 + xoffset) * 2, 1 * 2, (4 + (int) yoffset) * 2, 5 * 2, image_width, image_height);
                draw_bar(pixelbuf, (2 + xoffset) * 2, 1 * 2, (4 + (int) yoffset) * 2, 5 * 2, image_width, image_height);
                draw_bar(pixelbuf, (32 + xoffset) * 2, 1 * 2, (4 + (int) yoffset) * 2, 5 * 2, image_width, image_height);
                draw_bar(pixelbuf, (34 + xoffset) * 2, 1 * 2, (4 + (int) yoffset) * 2, 5 * 2, image_width, image_height);
                draw_bar(pixelbuf, (64 + xoffset) * 2, 1 * 2, (4 + (int) yoffset) * 2, 5 * 2, image_width, image_height);
                draw_bar(pixelbuf, (66 + xoffset) * 2, 1 * 2, (4 + (int) yoffset) * 2, 5 * 2, image_width, image_height);
                for (i = 0; i < 4; i++) {
                    textpart[i] = local_text[i];
                }
                textpart[4] = '\0';
                textpos = 2 * (17 + xoffset);

                draw_string(pixelbuf, textpart, textpos, default_text_posn, textflags, image_width, image_height);
                for (i = 0; i < 4; i++) {
                    textpart[i] = local_text[i + 4];
                }
                textpart[4] = '\0';
                textpos = 2 * (50 + xoffset);
                draw_string(pixelbuf, textpart, textpos, default_text_posn, textflags, image_width, image_height);
                textdone = 1;
                switch (strlen(addon)) {
                    case 2:
                        textpos = 2 * (xoffset + 86);
                        draw_string(pixelbuf, addon, textpos, image_height - (addon_text_posn * 2) - 13, textflags, image_width, image_height);
                        break;
                    case 5:
                        textpos = 2 * (xoffset + 100);
                        draw_string(pixelbuf, addon, textpos, image_height - (addon_text_posn * 2) - 13, textflags, image_width, image_height);
                        break;
                }

                break;
            case 13: /* EAN 13 */
            case 16:
            case 19:
                draw_bar(pixelbuf, (0 + xoffset) * 2, 1 * 2, (4 + (int) yoffset) * 2, 5 * 2, image_width, image_height);
                draw_bar(pixelbuf, (2 + xoffset) * 2, 1 * 2, (4 + (int) yoffset) * 2, 5 * 2, image_width, image_height);
                draw_bar(pixelbuf, (46 + xoffset) * 2, 1 * 2, (4 + (int) yoffset) * 2, 5 * 2, image_width, image_height);
                draw_bar(pixelbuf, (48 + xoffset) * 2, 1 * 2, (4 + (int) yoffset) * 2, 5 * 2, image_width, image_height);
                draw_bar(pixelbuf, (92 + xoffset) * 2, 1 * 2, (4 + (int) yoffset) * 2, 5 * 2, image_width, image_height);
                draw_bar(pixelbuf, (94 + xoffset) * 2, 1 * 2, (4 + (int) yoffset) * 2, 5 * 2, image_width, image_height);

                textpart[0] = local_text[0];
                textpart[1] = '\0';
                textpos = 2 * (-7 + xoffset);
                draw_string(pixelbuf, textpart, textpos, default_text_posn, textflags, image_width, image_height);
                for (i = 0; i < 6; i++) {
                    textpart[i] = local_text[i + 1];
                }
                textpart[6] = '\0';
                textpos = 2 * (24 + xoffset);
                draw_string(pixelbuf, textpart, textpos, default_text_posn, textflags, image_width, image_height);
                for (i = 0; i < 6; i++) {
                    textpart[i] = local_text[i + 7];
                }
                textpart[6] = '\0';
                textpos = 2 * (71 + xoffset);
                draw_string(pixelbuf, textpart, textpos, default_text_posn, textflags, image_width, image_height);
                textdone = 1;
                switch (strlen(addon)) {
                    case 2:
                        textpos = 2 * (xoffset + 114);
                        draw_string(pixelbuf, addon, textpos, image_height - (addon_text_posn * 2) - 13, textflags, image_width, image_height);
                        break;
                    case 5:
                        textpos = 2 * (xoffset + 128);
                        draw_string(pixelbuf, addon, textpos, image_height - (addon_text_posn * 2) - 13, textflags, image_width, image_height);
                        break;
                }
                break;

        }
    }

    if (((symbol->symbology == BARCODE_UPCA) && (symbol->rows == 1)) || (symbol->symbology == BARCODE_UPCA_CC)) {
        /* guard bar extensions and text formatting for UPCA */
        latch = 1;

        i = 0 + comp_offset;
        do {
            block_width = 0;
            do {
                block_width++;
            } while (module_is_set(symbol, symbol->rows - 1, i + block_width) == module_is_set(symbol, symbol->rows - 1, i));
            if (latch == 1) {
                /* a bar */
                draw_bar(pixelbuf, (i + xoffset - comp_offset) * 2, block_width * 2, (4 + (int) yoffset) * 2, 5 * 2, image_width, image_height);
                latch = 0;
            } else {
                /* a space */
                latch = 1;
            }
            i += block_width;
        } while (i < 11 + comp_offset);
        draw_bar(pixelbuf, (46 + xoffset) * 2, 1 * 2, (4 + (int) yoffset) * 2, 5 * 2, image_width, image_height);
        draw_bar(pixelbuf, (48 + xoffset) * 2, 1 * 2, (4 + (int) yoffset) * 2, 5 * 2, image_width, image_height);
        latch = 1;
        i = 85 + comp_offset;
        do {
            block_width = 0;
            do {
                block_width++;
            } while (module_is_set(symbol, symbol->rows - 1, i + block_width) == module_is_set(symbol, symbol->rows - 1, i));
            if (latch == 1) {
                /* a bar */
                draw_bar(pixelbuf, (i + xoffset - comp_offset) * 2, block_width * 2, (4 + (int) yoffset) * 2, 5 * 2, image_width, image_height);
                latch = 0;
            } else {
                /* a space */
                latch = 1;
            }
            i += block_width;
        } while (i < 96 + comp_offset);
        textpart[0] = local_text[0];
        textpart[1] = '\0';
        textpos = 2 * (-5 + xoffset);
        draw_string(pixelbuf, textpart, textpos, default_text_posn, textflags, image_width, image_height);
        for (i = 0; i < 5; i++) {
            textpart[i] = local_text[i + 1];
        }
        textpart[5] = '\0';
        textpos = 2 * (27 + xoffset);
        draw_string(pixelbuf, textpart, textpos, default_text_posn, textflags, image_width, image_height);
        for (i = 0; i < 5; i++) {
            textpart[i] = local_text[i + 6];
        }
        textpart[6] = '\0';
        textpos = 2 * (68 + xoffset);
        draw_string(pixelbuf, textpart, textpos, default_text_posn, textflags, image_width, image_height);
        textpart[0] = local_text[11];
        textpart[1] = '\0';
        textpos = 2 * (100 + xoffset);
        draw_string(pixelbuf, textpart, textpos, default_text_posn, textflags, image_width, image_height);
        textdone = 1;
        switch (strlen(addon)) {
            case 2:
                textpos = 2 * (xoffset + 116);
                draw_string(pixelbuf, addon, textpos, image_height - (addon_text_posn * 2) - 13, textflags, image_width, image_height);
                break;
            case 5:
                textpos = 2 * (xoffset + 130);
                draw_string(pixelbuf, addon, textpos, image_height - (addon_text_posn * 2) - 13, textflags, image_width, image_height);
                break;
        }

    }

    if (((symbol->symbology == BARCODE_UPCE) && (symbol->rows == 1)) || (symbol->symbology == BARCODE_UPCE_CC)) {
        /* guard bar extensions and text formatting for UPCE */
        draw_bar(pixelbuf, (0 + xoffset) * 2, 1 * 2, (4 + (int) yoffset) * 2, 5 * 2, image_width, image_height);
        draw_bar(pixelbuf, (2 + xoffset) * 2, 1 * 2, (4 + (int) yoffset) * 2, 5 * 2, image_width, image_height);
        draw_bar(pixelbuf, (46 + xoffset) * 2, 1 * 2, (4 + (int) yoffset) * 2, 5 * 2, image_width, image_height);
        draw_bar(pixelbuf, (48 + xoffset) * 2, 1 * 2, (4 + (int) yoffset) * 2, 5 * 2, image_width, image_height);
        draw_bar(pixelbuf, (50 + xoffset) * 2, 1 * 2, (4 + (int) yoffset) * 2, 5 * 2, image_width, image_height);

        textpart[0] = local_text[0];
        textpart[1] = '\0';
        textpos = 2 * (-5 + xoffset);
        draw_string(pixelbuf, textpart, textpos, default_text_posn, textflags, image_width, image_height);
        for (i = 0; i < 6; i++) {
            textpart[i] = local_text[i + 1];
        }
        textpart[6] = '\0';
        textpos = 2 * (24 + xoffset);
        draw_string(pixelbuf, textpart, textpos, default_text_posn, textflags, image_width, image_height);
        textpart[0] = local_text[7];
        textpart[1] = '\0';
        textpos = 2 * (55 + xoffset);
        draw_string(pixelbuf, textpart, textpos, default_text_posn, textflags, image_width, image_height);
        textdone = 1;
        switch (strlen(addon)) {
            case 2:
                textpos = 2 * (xoffset + 70);
                draw_string(pixelbuf, addon, textpos, image_height - (addon_text_posn * 2) - 13, textflags, image_width, image_height);
                break;
            case 5:
                textpos = 2 * (xoffset + 84);
                draw_string(pixelbuf, addon, textpos, image_height - (addon_text_posn * 2) - 13, textflags, image_width, image_height);
                break;
        }

    }

    xoffset -= comp_offset;

    /* Put boundary bars or box around symbol */
    if ((symbol->output_options & BARCODE_BOX) || (symbol->output_options & BARCODE_BIND)) {
        /* boundary bars */
        if (symbol->symbology != BARCODE_CODABLOCKF) {
            draw_bar(pixelbuf, 0, (symbol->width + xoffset + xoffset) * 2, textoffset * 2, symbol->border_width * 2, image_width, image_height);
            draw_bar(pixelbuf, 0, (symbol->width + xoffset + xoffset) * 2, (textoffset + symbol->height + symbol->border_width) * 2, symbol->border_width * 2, image_width, image_height);
        } else {
            draw_bar(pixelbuf, xoffset * 2, symbol->width * 2, textoffset * 2, symbol->border_width * 2, image_width, image_height);
            draw_bar(pixelbuf, xoffset * 2, symbol->width * 2, (textoffset + symbol->height + symbol->border_width) * 2, symbol->border_width * 2, image_width, image_height);
        }
        if ((symbol->output_options & BARCODE_BIND) != 0) {
            if ((symbol->rows > 1) && (is_stackable(symbol->symbology) == 1)) {
                /* row binding */
                if (symbol->symbology != BARCODE_CODABLOCKF) {
                    for (r = 1; r < symbol->rows; r++) {
                        draw_bar(pixelbuf, xoffset * 2, symbol->width * 2, ((r * row_height) + textoffset + yoffset - 1) * 2, 2 * 2, image_width, image_height);
                    }
                } else {
                    for (r = 1; r < symbol->rows; r++) {
                        draw_bar(pixelbuf, (xoffset + 11) * 2 , (symbol->width - 25) * 2, ((r * row_height) + textoffset + yoffset - 1) * 2, 2 * 2, image_width, image_height);
                    }
                }
            }
        }
    }

    if (symbol->output_options & BARCODE_BOX) {
        /* side bars */
        draw_bar(pixelbuf, 0, symbol->border_width * 2, textoffset * 2, (symbol->height + (2 * symbol->border_width)) * 2, image_width, image_height);
        draw_bar(pixelbuf, (symbol->width + xoffset + xoffset - symbol->border_width) * 2, symbol->border_width * 2, textoffset * 2, (symbol->height + (2 * symbol->border_width)) * 2, image_width, image_height);
    }

    /* Put the human readable text at the bottom */
    if ((textdone == 0) && (ustrlen(local_text) != 0)) {
        textpos = (image_width / 2);
        draw_string(pixelbuf, (char*) local_text, textpos, default_text_posn, textflags, image_width, image_height);
    }


    if (scaler == 0) {
        scaler = 0.5;
    }
    scale_width = image_width * scaler;
    scale_height = image_height * scaler;

    /* Apply scale options by creating another pixel buffer */
    if (!(scaled_pixelbuf = (char *) malloc(scale_width * scale_height))) {
        free(pixelbuf);
        strcpy(symbol->errtxt, "659: Insufficient memory for pixel buffer");
        return ZINT_ERROR_ENCODING_PROBLEM;
    } else {
        for (i = 0; i < (scale_width * scale_height); i++) {
            *(scaled_pixelbuf + i) = '0';
        }
    }

    for (vert = 0; vert < scale_height; vert++) {
        for (horiz = 0; horiz < scale_width; horiz++) {
            *(scaled_pixelbuf + (vert * scale_width) + horiz) = *(pixelbuf + ((int) (vert / scaler) * image_width) + (int) (horiz / scaler));
        }
    }

    error_number = save_raster_image_to_file(symbol, scale_height, scale_width, scaled_pixelbuf, rotate_angle, data_type);
    free(scaled_pixelbuf);
    free(pixelbuf);
    return error_number;
}

int plot_raster(struct zint_symbol *symbol, int rotate_angle, int file_type) {
    int error;

#ifdef NO_PNG
    if (file_type == OUT_PNG_FILE) {
        return ZINT_ERROR_INVALID_OPTION;
    }
#endif /* NO_PNG */

    if (symbol->output_options & BARCODE_DOTTY_MODE) {
        error = plot_raster_dotty(symbol, rotate_angle, file_type);
    } else {
        if (symbol->symbology == BARCODE_MAXICODE) {
            error = plot_raster_maxicode(symbol, rotate_angle, file_type);
        } else {
            error = plot_raster_default(symbol, rotate_angle, file_type);
        }
    }

    return error;
}


