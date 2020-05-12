/* ps.c - Post Script output */

/*
    libzint - the open source barcode library
    Copyright (C) 2009-2018 Robin Stuart <rstuart114@gmail.com>

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

#include <locale.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include "common.h"

int ps_plot(struct zint_symbol *symbol) {
    FILE *feps;
    int fgred, fggrn, fgblu, bgred, bggrn, bgblu;
    float red_ink, green_ink, blue_ink, red_paper, green_paper, blue_paper;
    float cyan_ink, magenta_ink, yellow_ink, black_ink;
    float cyan_paper, magenta_paper, yellow_paper, black_paper;
    int error_number = 0;
    float ax, ay, bx, by, cx, cy, dx, dy, ex, ey, fx, fy;
    float radius;
    
    struct zint_vector_rect *rect;
    struct zint_vector_hexagon *hex;
    struct zint_vector_circle *circle;
    struct zint_vector_string *string;
    const char *locale = NULL;

    if (symbol->output_options & BARCODE_STDOUT) {
        feps = stdout;
    } else {
        feps = fopen(symbol->outfile, "w");
    }
    if (feps == NULL) {
        strcpy(symbol->errtxt, "645: Could not open output file");
        return ZINT_ERROR_FILE_ACCESS;
    }

    locale = setlocale(LC_ALL, "C");

    fgred = (16 * ctoi(symbol->fgcolour[0])) + ctoi(symbol->fgcolour[1]);
    fggrn = (16 * ctoi(symbol->fgcolour[2])) + ctoi(symbol->fgcolour[3]);
    fgblu = (16 * ctoi(symbol->fgcolour[4])) + ctoi(symbol->fgcolour[5]);
    bgred = (16 * ctoi(symbol->bgcolour[0])) + ctoi(symbol->bgcolour[1]);
    bggrn = (16 * ctoi(symbol->bgcolour[2])) + ctoi(symbol->bgcolour[3]);
    bgblu = (16 * ctoi(symbol->bgcolour[4])) + ctoi(symbol->bgcolour[5]);
    red_ink = fgred / 256.0;
    green_ink = fggrn / 256.0;
    blue_ink = fgblu / 256.0;
    red_paper = bgred / 256.0;
    green_paper = bggrn / 256.0;
    blue_paper = bgblu / 256.0;

    /* Convert RGB to CMYK */
    if (red_ink > green_ink) {
        if (blue_ink > red_ink) {
            black_ink = 1 - blue_ink;
        } else {
            black_ink = 1 - red_ink;
        }
    } else {
        if (blue_ink > red_ink) {
            black_ink = 1 - blue_ink;
        } else {
            black_ink = 1 - green_ink;
        }
    }
    if (black_ink < 1.0) {
        cyan_ink = (1 - red_ink - black_ink) / (1 - black_ink);
        magenta_ink = (1 - green_ink - black_ink) / (1 - black_ink);
        yellow_ink = (1 - blue_ink - black_ink) / (1 - black_ink);
    } else {
        cyan_ink = 0.0;
        magenta_ink = 0.0;
        yellow_ink = 0.0;
    }

    if (red_paper > green_paper) {
        if (blue_paper > red_paper) {
            black_paper = 1 - blue_paper;
        } else {
            black_paper = 1 - red_paper;
        }
    } else {
        if (blue_paper > red_paper) {
            black_paper = 1 - blue_paper;
        } else {
            black_paper = 1 - green_paper;
        }
    }
    if (black_paper < 1.0) {
        cyan_paper = (1 - red_paper - black_paper) / (1 - black_paper);
        magenta_paper = (1 - green_paper - black_paper) / (1 - black_paper);
        yellow_paper = (1 - blue_paper - black_paper) / (1 - black_paper);
    } else {
        cyan_paper = 0.0;
        magenta_paper = 0.0;
        yellow_paper = 0.0;
    }

    /* Start writing the header */
    fprintf(feps, "%%!PS-Adobe-3.0 EPSF-3.0\n");
    fprintf(feps, "%%%%Creator: Zint %d.%d.%d\n", ZINT_VERSION_MAJOR, ZINT_VERSION_MINOR, ZINT_VERSION_RELEASE);
    fprintf(feps, "%%%%Title: Zint Generated Symbol\n");
    fprintf(feps, "%%%%Pages: 0\n");
    fprintf(feps, "%%%%BoundingBox: 0 0 %d %d\n", (int) ceil(symbol->vector->width), (int) ceil(symbol->vector->height));
    fprintf(feps, "%%%%EndComments\n");

    /* Definitions */
    fprintf(feps, "/TL { setlinewidth moveto lineto stroke } bind def\n");
    fprintf(feps, "/TD { newpath 0 360 arc fill } bind def\n");
    fprintf(feps, "/TH { 0 setlinewidth moveto lineto lineto lineto lineto lineto closepath fill } bind def\n");
    fprintf(feps, "/TB { 2 copy } bind def\n");
    fprintf(feps, "/TR { newpath 4 1 roll exch moveto 1 index 0 rlineto 0 exch rlineto neg 0 rlineto closepath fill } bind def\n");
    fprintf(feps, "/TE { pop pop } bind def\n");

    fprintf(feps, "newpath\n");

    /* Now the actual representation */
    if ((symbol->output_options & CMYK_COLOUR) == 0) {
        fprintf(feps, "%.2f %.2f %.2f setrgbcolor\n", red_paper, green_paper, blue_paper);
    } else {
        fprintf(feps, "%.2f %.2f %.2f %.2f setcmykcolor\n", cyan_paper, magenta_paper, yellow_paper, black_paper);
    }
    fprintf(feps, "%.2f 0.00 TB 0.00 %.2f TR\n", symbol->vector->height, symbol->vector->width);

    fprintf(feps, "TE\n");
    if ((symbol->output_options & CMYK_COLOUR) == 0) {
        fprintf(feps, "%.2f %.2f %.2f setrgbcolor\n", red_ink, green_ink, blue_ink);
    } else {
        fprintf(feps, "%.2f %.2f %.2f %.2f setcmykcolor\n", cyan_ink, magenta_ink, yellow_ink, black_ink);
    }
    
    // Rectangles
    rect = symbol->vector->rectangles;
    while (rect) {
        fprintf(feps, "%.2f %.2f TB %.2f %.2f TR\n", rect->height, (symbol->vector->height - rect->y) - rect->height, rect->x, rect->width);
        rect = rect->next;
    }
    
    // Hexagons
    hex = symbol->vector->hexagons;
    while (hex) {
        radius = hex->diameter / 2.0;
        ay = (symbol->vector->height - hex->y) + (1.0 * radius);
        by = (symbol->vector->height - hex->y) + (0.5 * radius);
        cy = (symbol->vector->height - hex->y) - (0.5 * radius);
        dy = (symbol->vector->height - hex->y) - (1.0 * radius);
        ey = (symbol->vector->height - hex->y) - (0.5 * radius);
        fy = (symbol->vector->height - hex->y) + (0.5 * radius);
        ax = hex->x;
        bx = hex->x + (0.86 * radius);
        cx = hex->x + (0.86 * radius);
        dx = hex->x;
        ex = hex->x - (0.86 * radius);
        fx = hex->x - (0.86 * radius);
        fprintf(feps, "%.2f %.2f %.2f %.2f %.2f %.2f %.2f %.2f %.2f %.2f %.2f %.2f TH\n", ax, ay, bx, by, cx, cy, dx, dy, ex, ey, fx, fy);
        hex = hex->next;
    }
    
    // Circles
    circle = symbol->vector->circles;
    while (circle) {
        if (circle->colour) {
            // A 'white' circle
            if ((symbol->output_options & CMYK_COLOUR) == 0) {
                fprintf(feps, "%.2f %.2f %.2f setrgbcolor\n", red_paper, green_paper, blue_paper);
            } else {
                fprintf(feps, "%.2f %.2f %.2f %.2f setcmykcolor\n", cyan_paper, magenta_paper, yellow_paper, black_paper);
            }
            fprintf(feps, "%.2f %.2f %.2f TD\n", circle->x, (symbol->vector->height - circle->y), circle->diameter / 2.0);
            if (circle->next) {
                if ((symbol->output_options & CMYK_COLOUR) == 0) {
                    fprintf(feps, "%.2f %.2f %.2f setrgbcolor\n", red_ink, green_ink, blue_ink);
                } else {
                    fprintf(feps, "%.2f %.2f %.2f %.2f setcmykcolor\n", cyan_ink, magenta_ink, yellow_ink, black_ink);
                }
            }
        } else {
            // A 'black' circle
            fprintf(feps, "%.2f %.2f %.2f TD\n", circle->x, (symbol->vector->height - circle->y), circle->diameter / 2.0);
        }
        circle = circle->next;
    }
    
    // Text
    string = symbol->vector->strings;
    while (string) {
        fprintf(feps, "matrix currentmatrix\n");
        fprintf(feps, "/Helvetica findfont\n");
        fprintf(feps, "%.2f scalefont setfont\n", string->fsize);
        fprintf(feps, " 0 0 moveto %.2f %.2f translate 0.00 rotate 0 0 moveto\n", string->x, (symbol->vector->height - string->y));
        fprintf(feps, " (%s) stringwidth\n", string->text);
        fprintf(feps, "pop\n");
        fprintf(feps, "-2 div 0 rmoveto\n");
        fprintf(feps, " (%s) show\n", string->text);
        fprintf(feps, "setmatrix\n");
        string = string->next;
    }
    
    fprintf(feps, "\nshowpage\n");

    if (symbol->output_options & BARCODE_STDOUT) {
        fflush(feps);
    } else {
        fclose(feps);
    }

    if (locale)
        setlocale(LC_ALL, locale);

    return error_number;
}
