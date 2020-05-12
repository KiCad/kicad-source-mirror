/*  vector.c - Creates vector image objects

    libzint - the open source barcode library
    Copyright (C) 2018 Robin Stuart <rstuart114@gmail.com>

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
#include <string.h>
#include <stdlib.h>
#include <math.h>

#ifdef _MSC_VER
#include <malloc.h>
#endif

#include "common.h"

#define SSET "0123456789ABCDEF"

extern int ps_plot(struct zint_symbol *symbol);
extern int svg_plot(struct zint_symbol *symbol);
extern int emf_plot(struct zint_symbol *symbol);

struct zint_vector_rect *vector_plot_create_rect(float x, float y, float width, float height) {
    struct zint_vector_rect *rect;

    rect = (struct zint_vector_rect*) malloc(sizeof (struct zint_vector_rect));
    if (!rect) return NULL;

    rect->next = NULL;
    rect->x = x;
    rect->y = y;
    rect->width = width;
    rect->height = height;
    rect->colour = -1; // Default colour

    return rect;
}

int vector_plot_add_rect(struct zint_symbol *symbol, struct zint_vector_rect *rect, struct zint_vector_rect **last_rect) {
    if (!rect) return ZINT_ERROR_MEMORY;
    if (*last_rect)
        (*last_rect)->next = rect;
    else
        symbol->vector->rectangles = rect; // first rectangle

    *last_rect = rect;
    return 1;
}

struct zint_vector_hexagon *vector_plot_create_hexagon(float x, float y, float diameter) {
    struct zint_vector_hexagon *hexagon;

    hexagon = (struct zint_vector_hexagon*) malloc(sizeof (struct zint_vector_hexagon));
    if (!hexagon) return NULL;
    hexagon->next = NULL;
    hexagon->x = x;
    hexagon->y = y;
    hexagon->diameter = (float)((diameter * 5.0) / 4.0); // Ugly kludge for legacy support

    return hexagon;
}

int vector_plot_add_hexagon(struct zint_symbol *symbol, struct zint_vector_hexagon *hexagon, struct zint_vector_hexagon **last_hexagon) {
    if (!hexagon) return ZINT_ERROR_MEMORY;
    if (*last_hexagon)
        (*last_hexagon)->next = hexagon;
    else
        symbol->vector->hexagons = hexagon; // first hexagon

    *last_hexagon = hexagon;
    return 1;
}

struct zint_vector_circle *vector_plot_create_circle(float x, float y, float diameter, int colour) {
    struct zint_vector_circle *circle;

    circle = (struct zint_vector_circle *) malloc(sizeof (struct zint_vector_circle));
    if (!circle) return NULL;
    circle->next = NULL;
    circle->x = x;
    circle->y = y;
    circle->diameter = diameter;
    circle->colour = colour;

    return circle;
}

int vector_plot_add_circle(struct zint_symbol *symbol, struct zint_vector_circle *circle, struct zint_vector_circle **last_circle) {
    if (!circle) return ZINT_ERROR_MEMORY;
    if (*last_circle)
        (*last_circle)->next = circle;
    else
        symbol->vector->circles = circle; // first circle

    *last_circle = circle;
    return 1;
}

int vector_plot_add_string(struct zint_symbol *symbol,
        unsigned char *text, float x, float y, float fsize, float width,
        struct zint_vector_string **last_string) {
    struct zint_vector_string *string;

    string = (struct zint_vector_string*) malloc(sizeof (struct zint_vector_string));
    string->next = NULL;
    string->x = x;
    string->y = y;
    string->width = width;
    string->fsize = fsize;
    string->length = ustrlen(text);
    string->text = (unsigned char*) malloc(sizeof (unsigned char) * (ustrlen(text) + 1));
    ustrcpy(string->text, text);

    if (*last_string)
        (*last_string)->next = string;
    else
        symbol->vector->strings = string; // First text portion
    *last_string = string;

    return 1;
}

void vector_free(struct zint_symbol *symbol) {
    if (symbol->vector != NULL) {
        struct zint_vector_rect *rect;
        struct zint_vector_hexagon *hex;
        struct zint_vector_circle *circle;
        struct zint_vector_string *string;

        // Free Rectangles
        rect = symbol->vector->rectangles;
        while (rect) {
            struct zint_vector_rect *r = rect;
            rect = rect->next;
            free(r);
        }

        // Free Hexagons
        hex = symbol->vector->hexagons;
        while (hex) {
            struct zint_vector_hexagon *h = hex;
            hex = hex->next;
            free(h);
        }

        // Free Circles
        circle = symbol->vector->circles;
        while (circle) {
            struct zint_vector_circle *c = circle;
            circle = circle->next;
            free(c);
        }

        // Free Strings
        string = symbol->vector->strings;
        while (string) {
            struct zint_vector_string *s = string;
            string = string->next;
            free(s->text);
            free(s);
        }

        // Free vector
        free(symbol->vector);
        symbol->vector = NULL;
    }
}

void vector_scale(struct zint_symbol *symbol) {
    struct zint_vector_rect *rect;
    struct zint_vector_hexagon *hex;
    struct zint_vector_circle *circle;
    struct zint_vector_string *string;
    float scale = symbol->scale * 2.0f;

    symbol->vector->width *= scale;
    symbol->vector->height *= scale;

    rect = symbol->vector->rectangles;
    while (rect) {
        rect->x *= scale;
        rect->y *= scale;
        rect->height *= scale;
        rect->width *= scale;
        rect = rect->next;
    }

    hex = symbol->vector->hexagons;
    while (hex) {
        hex->x *= scale;
        hex->y *= scale;
        hex->diameter *= scale;
        hex = hex->next;
    }

    circle = symbol->vector->circles;
    while (circle) {
        circle->x *= scale;
        circle->y *= scale;
        circle->diameter *= scale;
        circle = circle->next;
    }

    string = symbol->vector->strings;
    while (string) {
        string->x *= scale;
        string->y *= scale;
        string->width *= scale;
        string->fsize *= scale;
        string = string->next;
    }
    return;
}

void delete_last_rect(struct zint_vector_rect *rect) {
    struct zint_vector_rect *local;
    
    local = rect;
    
    if (local->next == NULL) {
        free(local);
        return;
    }
    
    while (local->next->next != NULL) {
        local = local->next;
    }
    
    free(local->next);
    local->next = NULL;
}

void bump_up_rect(struct zint_vector_rect *rect) {
    struct zint_vector_rect *local;
    
    local = rect;
    
    while (local->next) {
        local->x = local->next->x;
        local->y = local->next->y;
        local->height = local->next->height;
        local->width = local->next->width;
        local = local->next;
    }
    
}

void vector_reduce_rectangles(struct zint_symbol *symbol) {
    // Looks for vertically aligned rectangles and merges them together
    struct zint_vector_rect *rect, *target = NULL;

    rect = symbol->vector->rectangles;
    while (rect) {
        target = rect->next;
        
        while (target) {
            
            if ((rect->x == target->x) && (rect->width == target->width) && ((rect->y + rect->height) == target->y)) {
                rect->height += target->height;
                bump_up_rect(target);
                delete_last_rect(symbol->vector->rectangles);
            } else {
                target = target->next;
            }
        }

        rect = rect->next;
    }

    return;
}

int plot_vector(struct zint_symbol *symbol, int rotate_angle, int file_type) {
    int error_number;
    struct zint_vector *vector;
    struct zint_vector_rect *rectangle, *rect, *last_rectangle = NULL;
    struct zint_vector_hexagon *last_hexagon = NULL;
    struct zint_vector_string *last_string = NULL;
    struct zint_vector_circle *last_circle = NULL;

    int i, r, latch;
    float textpos, large_bar_height, preset_height, row_height, row_posn = 0.0;
    float text_offset, text_height;
    int xoffset, yoffset, textdone, main_symbol_width_x;
    char addon[6];
    int large_bar_count, symbol_lead_in;
    float addon_text_posn;
    float default_text_posn;
    int hide_text = 0;
    int upceanflag = 0;
    int rect_count, last_row_start;
    int this_row;
    int addon_latch = 0;
    struct zint_vector_string *string;
    // Sanity check colours
    to_upper((unsigned char*) symbol->fgcolour);
    to_upper((unsigned char*) symbol->bgcolour);

    if (strlen(symbol->fgcolour) != 6) {
        strcpy(symbol->errtxt, "661: Malformed foreground colour target");
        return ZINT_ERROR_INVALID_OPTION;
    }
    if (strlen(symbol->bgcolour) != 6) {
        strcpy(symbol->errtxt, "662: Malformed background colour target");
        return ZINT_ERROR_INVALID_OPTION;
    }
    error_number = is_sane(SSET, (unsigned char*) symbol->fgcolour, strlen(symbol->fgcolour));
    if (error_number == ZINT_ERROR_INVALID_DATA) {
        strcpy(symbol->errtxt, "663: Malformed foreground colour target");
        return ZINT_ERROR_INVALID_OPTION;
    }
    error_number = is_sane(SSET, (unsigned char*) symbol->bgcolour, strlen(symbol->bgcolour));
    if (error_number == ZINT_ERROR_INVALID_DATA) {
        strcpy(symbol->errtxt, "664: Malformed background colour target");
        return ZINT_ERROR_INVALID_OPTION;
    }

    // Free any previous rendering structures
    vector_free(symbol);

    // Allocate memory
    vector = symbol->vector = (struct zint_vector *) malloc(sizeof (struct zint_vector));
    if (!vector) return ZINT_ERROR_MEMORY;
    vector->rectangles = NULL;
    vector->hexagons = NULL;
    vector->circles = NULL;
    vector->strings = NULL;

    row_height = 0;
    textdone = 0;
    textpos = 0.0;
    main_symbol_width_x = symbol->width;
    strcpy(addon, "");
    symbol_lead_in = 0;
    addon_text_posn = 0.0;
    rect_count = 0;
    last_row_start = 0;
    
    /*
     * Determine if there will be any addon texts and text height
     */
    latch = 0;
    r = 0;
    /* Isolate add-on text */
    if (is_extendable(symbol->symbology)) {
        for (i = 0; i < (int) ustrlen(symbol->text); i++) {
            if (latch == 1) {
                addon[r] = symbol->text[i];
                r++;
            }
            if (symbol->text[i] == '+') {
                latch = 1;
            }
        }
    }
    addon[r] = '\0';


    /*
     * Calculate the width of the barcode, especially if there are any extra
     * borders or white space to add.
     */

    while (!(module_is_set(symbol, symbol->rows - 1, symbol_lead_in))) {
        symbol_lead_in++;
    }

    /* Certain symbols need whitespace otherwise characters get chopped off the sides */
    if ((((symbol->symbology == BARCODE_EANX || symbol->symbology == BARCODE_EANX_CHK) && (symbol->rows == 1)) ||
            (symbol->symbology == BARCODE_EANX_CC)) || (symbol->symbology == BARCODE_ISBNX)) {
        switch (ustrlen(symbol->text)) {
            case 13: /* EAN 13 */
            case 16:
            case 19:
                if (symbol->whitespace_width == 0) {
                    symbol->whitespace_width = 10;
                }
                main_symbol_width_x = 96 + symbol_lead_in;
                upceanflag = 13;
                break;
            case 2:
                main_symbol_width_x = 22 + symbol_lead_in;
                upceanflag = 2;
                break;
            case 5:
                main_symbol_width_x = 49 + symbol_lead_in;
                upceanflag = 5;
                break;
            default:
                main_symbol_width_x = 68 + symbol_lead_in;
                upceanflag = 8;
        }
    } else if (((symbol->symbology == BARCODE_UPCA || symbol->symbology == BARCODE_UPCA_CHK) && (symbol->rows == 1))
            || (symbol->symbology == BARCODE_UPCA_CC)) {
        upceanflag = 12;
        if (symbol->whitespace_width < 10) {
            symbol->whitespace_width = 10;
            main_symbol_width_x = 96 + symbol_lead_in;
        }
    } else if (((symbol->symbology == BARCODE_UPCE || symbol->symbology == BARCODE_UPCE_CHK) && (symbol->rows == 1))
            || (symbol->symbology == BARCODE_UPCE_CC)) {
        upceanflag = 6;
        if (symbol->whitespace_width == 0) {
            symbol->whitespace_width = 10;
            main_symbol_width_x = 51 + symbol_lead_in;
        }
    }

    if ((!symbol->show_hrt) || (ustrlen(symbol->text) == 0)) {
        hide_text = 1;
        text_height = 0.0;
        text_offset = upceanflag ? 9.0f : 0.0f;
    } else {
        text_height = upceanflag ? 11.0f : 9.0f;
        text_offset = 9.0;
    }
    if (symbol->output_options & SMALL_TEXT)
        text_height *= 0.8f;

    xoffset = symbol->border_width + symbol->whitespace_width;
    yoffset = symbol->border_width;


    // Determine if height should be overridden
    large_bar_count = 0;
    preset_height = 0.0;
    for (i = 0; i < symbol->rows; i++) {
        preset_height += symbol->row_height[i];
        if (symbol->row_height[i] == 0) {
            large_bar_count++;
        }
    }

    vector->width = (float)ceil(symbol->width + (2.0f * xoffset));
    vector->height = (float)ceil(symbol->height + text_offset + (2.0f * yoffset));

    large_bar_height = (symbol->height - preset_height) / large_bar_count;

    if ((symbol->output_options & BARCODE_BOX) || (symbol->output_options & BARCODE_BIND)) {
        default_text_posn = symbol->height + text_offset + symbol->border_width + symbol->border_width;
    } else {
        default_text_posn = symbol->height + text_offset + symbol->border_width;
    }

    // Plot rectangles - most symbols created here
    if ((symbol->symbology != BARCODE_MAXICODE) && ((symbol->output_options & BARCODE_DOTTY_MODE) == 0)) {
        for (r = 0; r < symbol->rows; r++) {
            this_row = r;
            last_row_start = rect_count;
            if (symbol->row_height[this_row] == 0) {
                row_height = large_bar_height;
            } else {
                row_height = (float)symbol->row_height[this_row];
            }
            row_posn = 0;
            for (i = 0; i < r; i++) {
                if (symbol->row_height[i] == 0) {
                    row_posn += large_bar_height;
                } else {
                    row_posn += symbol->row_height[i];
                }
            }
            row_posn += yoffset;

            i = 0;
            if (module_is_set(symbol, this_row, 0)) {
                latch = 1;
            } else {
                latch = 0;
            }

            do {
                int block_width = 0;
                do {
                    block_width++;
                } while (module_is_set(symbol, this_row, i + block_width) == module_is_set(symbol, this_row, i));
                if ((addon_latch == 0) && (r == (symbol->rows - 1)) && (i > main_symbol_width_x)) {
                    addon_text_posn = row_posn + 8.0f;
                    addon_latch = 1;
                }
                if (latch == 1) {
                    /* a bar */
                    if (addon_latch == 0) {
                        rectangle = vector_plot_create_rect((float)(i + xoffset), row_posn, (float)block_width, row_height);
                    } else {
                        rectangle = vector_plot_create_rect((float)(i + xoffset), row_posn + 10.0f, (float)block_width, row_height - 5.0f);
                    }
                    latch = 0;

                    vector_plot_add_rect(symbol, rectangle, &last_rectangle);
                    rect_count++;
                } else {
                    /* a space */
                    latch = 1;
                }
                i += block_width;

            } while (i < symbol->width);
        }
    }

    // Plot Maxicode symbols
    if (symbol->symbology == BARCODE_MAXICODE) {
        struct zint_vector_circle *circle;
        vector->width = 37.0f + (2.0f * xoffset);
        vector->height = 36.0f + (2.0f * yoffset);

        // Bullseye
        circle = vector_plot_create_circle(17.88f + xoffset, 17.8f + yoffset, 10.85f, 0);
        vector_plot_add_circle(symbol, circle, &last_circle);
        circle = vector_plot_create_circle(17.88f + xoffset, 17.8f + yoffset, 8.97f, 1);
        vector_plot_add_circle(symbol, circle, &last_circle);
        circle = vector_plot_create_circle(17.88f + xoffset, 17.8f + yoffset, 7.10f, 0);
        vector_plot_add_circle(symbol, circle, &last_circle);
        circle = vector_plot_create_circle(17.88f + xoffset, 17.8f + yoffset, 5.22f, 1);
        vector_plot_add_circle(symbol, circle, &last_circle);
        circle = vector_plot_create_circle(17.88f + xoffset, 17.8f + yoffset, 3.31f, 0);
        vector_plot_add_circle(symbol, circle, &last_circle);
        circle = vector_plot_create_circle(17.88f + xoffset, 17.8f + yoffset, 1.43f, 1);
        vector_plot_add_circle(symbol, circle, &last_circle);

        /* Hexagons */
        for (r = 0; r < symbol->rows; r++) {
            for (i = 0; i < symbol->width; i++) {
                if (module_is_set(symbol, r, i)) {
                    //struct zint_vector_hexagon *hexagon = vector_plot_create_hexagon(((i * 0.88) + ((r & 1) ? 1.76 : 1.32)), ((r * 0.76) + 0.76), symbol->dot_size);
                    struct zint_vector_hexagon *hexagon = vector_plot_create_hexagon(((i * 1.23f) + 0.615f + ((r & 1) ? 0.615f : 0.0f)) + xoffset, 
                                                                                     ((r * 1.067f) + 0.715f) + yoffset, symbol->dot_size);
                    vector_plot_add_hexagon(symbol, hexagon, &last_hexagon);
                }
            }
        }
    }

    // Dotty mode
    if ((symbol->symbology != BARCODE_MAXICODE) && (symbol->output_options & BARCODE_DOTTY_MODE)) {
        for (r = 0; r < symbol->rows; r++) {
            for (i = 0; i < symbol->width; i++) {
                if (module_is_set(symbol, r, i)) {
                    struct zint_vector_circle *circle = vector_plot_create_circle(i + 0.5f + xoffset, r + 0.5f + yoffset, 1.0f, 0);
                    vector_plot_add_circle(symbol, circle, &last_circle);
                }
            }
        }
    }

    /* Guard bar extension */
    if (upceanflag == 6) {
        i = 0;
        for (rect = symbol->vector->rectangles; rect != NULL; rect = rect->next) {
            switch (i - last_row_start) {
                case 0:
                case 1:
                case 14:
                case 15:
                case 16:
                    rect->height += 5.0;
                    break;
            }
            i++;
        }
    }
                
    if (upceanflag == 8) {
        i = 0;
        for (rect = symbol->vector->rectangles; rect != NULL; rect = rect->next) {
            switch (i - last_row_start) {
                case 0:
                case 1:
                case 10:
                case 11:
                case 20:
                case 21:
                    rect->height += 5.0;
                    break;
            }
            i++;
        }
    }
    
    if (upceanflag == 12) {
        i = 0;
        for (rect = symbol->vector->rectangles; rect != NULL; rect = rect->next) {
            switch (i - last_row_start) {
                case 0:
                case 1:
                case 2:
                case 3:
                case 14:
                case 15:
                case 26:
                case 27:
                case 28:
                case 29:
                    rect->height += 5.0;
                    break;
            }
            i++;
        }
    }
    
    if (upceanflag == 13) {
        i = 0;
        for (rect = symbol->vector->rectangles; rect != NULL; rect = rect->next) {
            switch (i - last_row_start) {
                case 0:
                case 1:
                case 14:
                case 15:
                case 28:
                case 29:
                    rect->height += 5.0;
                    break;
            }
            i++;
        }
    }
    
    /* Add the text */
    xoffset += symbol_lead_in;
    row_posn = row_posn + large_bar_height;
    
    if (!hide_text) {
        char textpart[10];
        float textwidth;
        if (upceanflag == 8) {
            for (i = 0; i < 4; i++) {
                textpart[i] = symbol->text[i];
            }
            textpart[4] = '\0';
            textpos = 17;
            textwidth = 4.0 * 8.5;
            vector_plot_add_string(symbol, (unsigned char *) textpart, textpos + xoffset, default_text_posn, text_height, textwidth, &last_string);
            for (i = 0; i < 4; i++) {
                textpart[i] = symbol->text[i + 4];
            }
            textpart[4] = '\0';
            textpos = 50;
            vector_plot_add_string(symbol, (unsigned char *) textpart, (textpos + xoffset), default_text_posn, text_height, textwidth, &last_string);
            textdone = 1;
            switch (strlen(addon)) {
                case 2:
                    textpos = (float)(xoffset + 86);
                    textwidth = 2.0f * 8.5f;
                    vector_plot_add_string(symbol, (unsigned char *) addon, textpos, addon_text_posn, text_height, textwidth, &last_string);
                    break;
                case 5:
                    textpos = (float)(xoffset + 100);
                    textwidth = 5.0f * 8.5f;
                    vector_plot_add_string(symbol, (unsigned char *) addon, textpos, addon_text_posn, text_height, textwidth, &last_string);
                    break;
            }

        }

        if (upceanflag == 13) {
            textpart[0] = symbol->text[0];
            textpart[1] = '\0';
            textpos = -5; // 7
            textwidth = 8.5;
            vector_plot_add_string(symbol, (unsigned char *) textpart, textpos + xoffset, default_text_posn, text_height, textwidth, &last_string);

            for (i = 0; i < 6; i++) {
                textpart[i] = symbol->text[i + 1];
            }
            textpart[6] = '\0';
            textpos = 25;
            textwidth = 6.0 * 8.5;
            vector_plot_add_string(symbol, (unsigned char *) textpart, textpos + xoffset, default_text_posn, text_height, textwidth, &last_string);
            for (i = 0; i < 6; i++) {
                textpart[i] = symbol->text[i + 7];
            }
            textpart[6] = '\0';
            textpos = 72;
            vector_plot_add_string(symbol, (unsigned char *) textpart, textpos + xoffset, default_text_posn, text_height, textwidth, &last_string);
            textdone = 1;
            switch (strlen(addon)) {
                case 2:
                    textpos = (float)(xoffset + 114);
                    textwidth = 2.0f * 8.5f;
                    vector_plot_add_string(symbol, (unsigned char *) addon, textpos, addon_text_posn, text_height, textwidth, &last_string);
                    break;
                case 5:
                    textpos = (float)(xoffset + 128);
                    textwidth = 5.0f * 8.5f;
                    vector_plot_add_string(symbol, (unsigned char *) addon, textpos, addon_text_posn, text_height, textwidth, &last_string);
                    break;
            }
        }

        if (upceanflag == 12) {
            textpart[0] = symbol->text[0];
            textpart[1] = '\0';
            textpos = -5;
            textwidth = 6.2f;
            vector_plot_add_string(symbol, (unsigned char *) textpart, textpos + xoffset, default_text_posn - 2.0f, text_height * (8.0f / 11.0f), textwidth, &last_string);
            for (i = 0; i < 5; i++) {
                textpart[i] = symbol->text[i + 1];
            }
            textpart[5] = '\0';
            textpos = 27;
            textwidth = 5.0f * 8.5f;
            vector_plot_add_string(symbol, (unsigned char *) textpart, textpos + xoffset, default_text_posn, text_height, textwidth, &last_string);
            for (i = 0; i < 5; i++) {
                textpart[i] = symbol->text[i + 6];
            }
            textpos = 68;
            vector_plot_add_string(symbol, (unsigned char *) textpart, textpos + xoffset, default_text_posn, text_height, textwidth, &last_string);
            textpart[0] = symbol->text[11];
            textpart[1] = '\0';
            textpos = 100;
            textwidth = 6.2f;
            vector_plot_add_string(symbol, (unsigned char *) textpart, textpos + xoffset, default_text_posn - 2.0f, text_height * (8.0f / 11.0f), textwidth, &last_string);
            textdone = 1;
            switch (strlen(addon)) {
                case 2:
                    textpos = (float)(xoffset + 116);
                    textwidth = 2.0f * 8.5f;
                    vector_plot_add_string(symbol, (unsigned char *) addon, textpos, addon_text_posn, text_height, textwidth, &last_string);
                    break;
                case 5:
                    textpos = (float)(xoffset + 130);
                    textwidth = 5.0f * 8.5f;
                    vector_plot_add_string(symbol, (unsigned char *) addon, textpos, addon_text_posn, text_height, textwidth, &last_string);
                    break;
            }
        }

        if (upceanflag == 6) {
            textpart[0] = symbol->text[0];
            textpart[1] = '\0';
            textpos = -5;
            textwidth = 6.2f;
            vector_plot_add_string(symbol, (unsigned char *) textpart, textpos + xoffset, default_text_posn - 2.0f, text_height * (8.0f / 11.0f), textwidth, &last_string);
            for (i = 0; i < 6; i++) {
                textpart[i] = symbol->text[i + 1];
            }
            textpart[6] = '\0';
            textpos = 24;
            textwidth = 6.0 * 8.5;
            vector_plot_add_string(symbol, (unsigned char *) textpart, textpos + xoffset, default_text_posn, text_height, textwidth, &last_string);
            textpart[0] = symbol->text[7];
            textpart[1] = '\0';
            textpos = 55;
            textwidth = 6.2f;
            vector_plot_add_string(symbol, (unsigned char *) textpart, textpos + xoffset, default_text_posn - 2.0f, text_height * (8.0f / 11.0f), textwidth, &last_string);
            textdone = 1;
            switch (strlen(addon)) {
                case 2:
                    textpos = (float)(xoffset + 70);
                    textwidth = 2.0f * 8.5f;
                    vector_plot_add_string(symbol, (unsigned char *) addon, textpos, addon_text_posn, text_height, textwidth, &last_string);
                    break;
                case 5:
                    textpos = (float)(xoffset + 84);
                    textwidth = 5.0f * 8.5f;
                    vector_plot_add_string(symbol, (unsigned char *) addon, textpos, addon_text_posn, text_height, textwidth, &last_string);
                    break;
            }
        }

        /* Put normal human readable text at the bottom (and centered) */
        if (textdone == 0) {
            // caculate start xoffset to center text
            vector_plot_add_string(symbol, symbol->text, (symbol->width / 2.0f) + xoffset, default_text_posn, text_height, (float)symbol->width, &last_string);
        }
    }

    //Remove control characters from readable text
    // This only applies to Code 128
    string = symbol->vector->strings;
    if (string) {
        for (i = 0; i < string->length; i++) {
            if (string->text[i] < ' ') {
                string->text[i] = ' ';
            }
        }
    }

    // Binding and boxes
    if ((symbol->output_options & BARCODE_BIND) != 0) {
        if ((symbol->rows > 1) && (is_stackable(symbol->symbology) == 1)) {
            /* row binding */
            for (r = 1; r < symbol->rows; r++) {
                if (symbol->symbology != BARCODE_CODABLOCKF) {
                    rectangle = vector_plot_create_rect((float)xoffset, (r * row_height) + yoffset - 1, (float)symbol->width, 2.0f);
                    vector_plot_add_rect(symbol, rectangle, &last_rectangle);
                } else {
                    rectangle = vector_plot_create_rect(xoffset + 11.0f, (r * row_height) + yoffset - 1, symbol->width - 25.0f, 2.0);
                    vector_plot_add_rect(symbol, rectangle, &last_rectangle);
                }
            }
        }
    }
    if ((symbol->output_options & BARCODE_BOX) || (symbol->output_options & BARCODE_BIND)) {
        // Top
        rectangle = vector_plot_create_rect(0.0f, 0.0f, vector->width, (float)symbol->border_width);
        if (symbol->symbology == BARCODE_CODABLOCKF) {
            rectangle->x = (float)xoffset;
            rectangle->width -= (2.0f * xoffset);
        }
        vector_plot_add_rect(symbol, rectangle, &last_rectangle);
        // Bottom
        rectangle = vector_plot_create_rect(0.0f, vector->height - symbol->border_width - text_offset, vector->width, (float)symbol->border_width);
        if (symbol->symbology == BARCODE_CODABLOCKF) {
            rectangle->x = (float)xoffset;
            rectangle->width -= (2.0f * xoffset);
        }
        vector_plot_add_rect(symbol, rectangle, &last_rectangle);
    }
    if (symbol->output_options & BARCODE_BOX) {
        // Left
        rectangle = vector_plot_create_rect(0.0f, 0.0f, (float)symbol->border_width, vector->height - text_offset);
        vector_plot_add_rect(symbol, rectangle, &last_rectangle);
        // Right
        rectangle = vector_plot_create_rect(vector->width - symbol->border_width, 0.0f, (float)symbol->border_width, vector->height - text_offset);
        vector_plot_add_rect(symbol, rectangle, &last_rectangle);
    }

    vector_reduce_rectangles(symbol);

    vector_scale(symbol);

    switch (file_type) {
        case OUT_EPS_FILE:
            error_number = ps_plot(symbol);
            break;
        case OUT_SVG_FILE:
            error_number = svg_plot(symbol);
            break;
        case OUT_EMF_FILE:
            error_number = emf_plot(symbol);
            break;
        /* case OUT_BUFFER: No more work needed */
    }

    return error_number;

}
