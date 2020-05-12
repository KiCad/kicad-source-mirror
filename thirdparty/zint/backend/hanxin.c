/*  hanxin.c - Han Xin Code

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

/* This code attempts to implement Han Xin Code according to AIMD-015:2010 (Rev 0.8) */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#ifdef _MSC_VER
#include <malloc.h>
#endif
#include "common.h"
#include "reedsol.h"
#include "hanxin.h"
#include "gb2312.h"
#include "gb18030.h"
#include "assert.h"

/* Find which submode to use for a text character */
int getsubmode(char input) {
    int submode = 2;

    if ((input >= '0') && (input <= '9')) {
        submode = 1;
    }

    if ((input >= 'A') && (input <= 'Z')) {
        submode = 1;
    }

    if ((input >= 'a') && (input <= 'z')) {
        submode = 1;
    }

    return submode;
}

/* Calculate the approximate length of the binary string */
static int calculate_binlength(char mode[], int source[], const size_t length, int eci) {
    size_t i;
    char lastmode = 't';
    int est_binlen = 0;
    int submode = 1;

    if (eci != 3) {
        est_binlen += 12;
    }

    i = 0;
    do {
        switch (mode[i]) {
            case 'n':
                if (lastmode != 'n') {
                    est_binlen += 14;
                    lastmode = 'n';
                }
                est_binlen += 4;
                break;
            case 't':
                if (lastmode != 't') {
                    est_binlen += 10;
                    lastmode = 't';
                    submode = 1;
                }
                if (getsubmode((char) source[i]) != submode) {
                    est_binlen += 6;
                    submode = getsubmode((char) source[i]);
                }
                est_binlen += 6;
                break;
            case 'b':
                if (lastmode != 'b') {
                    est_binlen += 17;
                    lastmode = 'b';
                }
                est_binlen += 8;
                break;
            case '1':
                if (lastmode != '1') {
                    est_binlen += 16;
                    lastmode = '1';
                }
                est_binlen += 12;
                break;
            case '2':
                if (lastmode != '2') {
                    est_binlen += 16;
                    lastmode = '2';
                }
                est_binlen += 12;
                break;
            case 'd':
                if (lastmode != 'd') {
                    est_binlen += 16;
                    lastmode = 'd';
                }
                est_binlen += 15;
                break;
            case 'f':
                if (lastmode != 'f') {
                    est_binlen += 4;
                    lastmode = 'f';
                }
                est_binlen += 21;
                i++;
                break;
        }
        i++;
    } while (i < length);

    return est_binlen;
}

int isRegion1(int glyph) {
    int first_byte, second_byte;
    int valid = 0;

    first_byte = (glyph & 0xff00) >> 8;
    second_byte = glyph & 0xff;

    if ((first_byte >= 0xb0) && (first_byte <= 0xd7)) {
        if ((second_byte >= 0xa1) && (second_byte <= 0xfe)) {
            valid = 1;
        }
    }

    if ((first_byte >= 0xa1) && (first_byte <= 0xa3)) {
        if ((second_byte >= 0xa1) && (second_byte <= 0xfe)) {
            valid = 1;
        }
    }

    if ((glyph >= 0xa8a1) && (glyph <= 0xa8c0)) {
        valid = 1;
    }

    return valid;
}

int isRegion2(int glyph) {
    int first_byte, second_byte;
    int valid = 0;

    first_byte = (glyph & 0xff00) >> 8;
    second_byte = glyph & 0xff;

    if ((first_byte >= 0xd8) && (first_byte <= 0xf7)) {
        if ((second_byte >= 0xa1) && (second_byte <= 0xfe)) {
            valid = 1;
        }
    }

    return valid;
}

int isDoubleByte(int glyph) {
    int first_byte, second_byte;
    int valid = 0;

    first_byte = (glyph & 0xff00) >> 8;
    second_byte = glyph & 0xff;

    if ((first_byte >= 0x81) && (first_byte <= 0xfe)) {
        if ((second_byte >= 0x40) && (second_byte <= 0x7e)) {
            valid = 1;
        }

        if ((second_byte >= 0x80) && (second_byte <= 0xfe)) {
            valid = 1;
        }
    }

    return valid;
}

int isFourByte(int glyph, int glyph2) {
    int first_byte, second_byte;
    int third_byte, fourth_byte;
    int valid = 0;

    first_byte = (glyph & 0xff00) >> 8;
    second_byte = glyph & 0xff;
    third_byte = (glyph2 & 0xff00) >> 8;
    fourth_byte = glyph2 & 0xff;

    if ((first_byte >= 0x81) && (first_byte <= 0xfe)) {
        if ((second_byte >= 0x30) && (second_byte <= 0x39)) {
            if ((third_byte >= 0x81) && (third_byte <= 0xfe)) {
                if ((fourth_byte >= 0x30) && (fourth_byte <= 0x39)) {
                    valid = 1;
                }
            }
        }
    }

    return valid;
}

/* Calculate mode switching */
static void hx_define_mode(char mode[], int source[], const size_t length) {
    size_t i;
    char lastmode = 't';

    i = 0;
    do {
        int done = 0;

        if (isRegion1(source[i])) {
            mode[i] = '1';
            done = 1;
            i++;
        }

        if ((done == 0) && (isRegion2(source[i]))) {
            mode[i] = '2';
            done = 1;
            i++;
        }

        if ((done == 0) && (isDoubleByte(source[i]))) {
            mode[i] = 'd';
            done = 1;
            i++;
        }

        if ((done == 0) && (i < length - 1)) {
            if (isFourByte(source[i], source[i + 1])) {
                mode[i] = 'f';
                mode[i + 1] = 'f';
                done = 1;
                i += 2;
            }
        }

        if (done == 0) {
            if ((source[i] >= '0') && (source[i] <= '9')) {
                mode[i] = 'n';
                if (lastmode != 'n') {
                    lastmode = 'n';
                }
            } else {
                if ((source[i] <= 127) && ((source[i] <= 27) || (source[i] >= 32))) {
                    mode[i] = 't';
                    if (lastmode != 't') {
                        lastmode = 't';
                    }
                } else {
                    mode[i] = 'b';
                    if (lastmode != 'b') {
                        lastmode = 'b';
                    }
                }
            }
            i++;
        }
    } while (i < length);
    mode[length] = '\0';
}

/* Convert Text 1 sub-mode character to encoding value, as given in table 3 */
int lookup_text1(char input) {
    int encoding_value = 0;

    if ((input >= '0') && (input <= '9')) {
        encoding_value = input - '0';
    }

    if ((input >= 'A') && (input <= 'Z')) {
        encoding_value = input - 'A' + 10;
    }

    if ((input >= 'a') && (input <= 'z')) {
        encoding_value = input - 'a' + 36;
    }

    return encoding_value;
}

/* Convert Text 2 sub-mode character to encoding value, as given in table 4 */
int lookup_text2(char input) {
    int encoding_value = 0;

    if ((input >= 0) && (input <= 27)) {
        encoding_value = input;
    }

    if ((input >= ' ') && (input <= '/')) {
        encoding_value = input - ' ' + 28;
    }

    if ((input >= '[') && (input <= 96)) {
        encoding_value = input - '[' + 51;
    }

    if ((input >= '{') && (input <= 127)) {
        encoding_value = input - '{' + 57;
    }

    return encoding_value;
}

/* Convert input data to binary stream */
static void calculate_binary(char binary[], char mode[], int source[], const size_t length, const int eci, int debug) {
    int position = 0;
    int i, count, encoding_value;
    int first_byte, second_byte;
    int third_byte, fourth_byte;
    int glyph;
    int submode;

    if (eci != 3) {
        /* Encoding ECI assignment number, according to Table 5 */
        bin_append(8, 4, binary); // ECI
        if (eci <= 127) {
            bin_append(eci, 8, binary);
        }
        if ((eci >= 128) && (eci <= 16383)) {
            strcat(binary, "10");
            bin_append(eci, 14, binary);
        }
        if (eci >= 16384) {
            strcat(binary, "110");
            bin_append(eci, 21, binary);
        }
    }

    do {
        int block_length = 0;
        do {
            block_length++;
        } while (mode[position + block_length] == mode[position]);

        switch (mode[position]) {
            case 'n':
                /* Numeric mode */
                /* Mode indicator */
                bin_append(1, 4, binary);

                if (debug) {
                    printf("Numeric\n");
                }

                i = 0;

                while (i < block_length) {
                    int first = 0;

                    first = posn(NEON, (char) source[position + i]);
                    count = 1;
                    encoding_value = first;

                    if (i + 1 < block_length && mode[position + i + 1] == 'n') {
                        int second = posn(NEON, (char) source[position + i + 1]);
                        count = 2;
                        encoding_value = (encoding_value * 10) + second;

                        if (i + 2 < block_length && mode[position + i + 2] == 'n') {
                            int third = posn(NEON, (char) source[position + i + 2]);
                            count = 3;
                            encoding_value = (encoding_value * 10) + third;
                        }
                    }

                    bin_append(encoding_value, 10, binary);

                    if (debug) {
                        printf("0x%4x (%d)", encoding_value, encoding_value);
                    }

                    i += count;
                }

                /* Mode terminator depends on number of characters in last group (Table 2) */
                switch (count) {
                    case 1:
                        bin_append(1021, 10, binary);
                        break;
                    case 2:
                        bin_append(1022, 10, binary);
                        break;
                    case 3:
                        bin_append(1023, 10, binary);
                        break;
                }

                if (debug) {
                    printf(" (TERM %d)\n", count);
                }

                break;
            case 't':
                /* Text mode */
                if (position != 0) {
                    /* Mode indicator */
                    bin_append(2, 4, binary);

                    if (debug) {
                        printf("Text\n");
                    }
                }

                submode = 1;

                i = 0;

                while (i < block_length) {

                    if (getsubmode((char) source[i + position]) != submode) {
                        /* Change submode */
                        bin_append(62, 6, binary);
                        submode = getsubmode((char) source[i + position]);
                        if (debug) {
                            printf("SWITCH ");
                        }
                    }

                    if (submode == 1) {
                        encoding_value = lookup_text1((char) source[i + position]);
                    } else {
                        encoding_value = lookup_text2((char) source[i + position]);
                    }

                    bin_append(encoding_value, 6, binary);

                    if (debug) {
                        printf("%c (%d) ", (char) source[i], encoding_value);
                    }
                    i++;
                }

                /* Terminator */
                bin_append(63, 6, binary);

                if (debug) {
                    printf("\n");
                }
                break;
            case 'b':
                /* Binary Mode */
                /* Mode indicator */
                bin_append(3, 4, binary);

                /* Count indicator */
                bin_append(block_length, 13, binary);

                if (debug) {
                    printf("Binary (length %d)\n", block_length);
                }

                i = 0;

                while (i < block_length) {

                    /* 8-bit bytes with no conversion */
                    bin_append(source[i + position], 8, binary);

                    if (debug) {
                        printf("%d ", source[i + position]);
                    }

                    i++;
                }

                if (debug) {
                    printf("\n");
                }
                break;
            case '1':
                /* Region 1 encoding */
                /* Mode indicator */
                bin_append(4, 4, binary);

                if (debug) {
                    printf("Region 1\n");
                }

                i = 0;

                while (i < block_length) {
                    first_byte = (source[i + position] & 0xff00) >> 8;
                    second_byte = source[i + position] & 0xff;

                    /* Subset 1 */
                    glyph = (0x5e * (first_byte - 0xb0)) + (second_byte - 0xa1);

                    /* Subset 2 */
                    if ((first_byte >= 0xa1) && (first_byte <= 0xa3)) {
                        if ((second_byte >= 0xa1) && (second_byte <= 0xfe)) {
                            glyph = (0x5e * first_byte - 0xa1) + (second_byte - 0xa1) + 0xeb0;
                        }
                    }

                    /* Subset 3 */
                    if ((source[i + position] >= 0xa8a1) && (source[i + position] <= 0xa8c0)) {
                        glyph = (second_byte - 0xa1) + 0xfca;
                    }

                    if (debug) {
                        printf("%d ", glyph);
                    }

                    bin_append(glyph, 12, binary);
                    i++;
                }

                /* Terminator */
                bin_append(4095, 12, binary);

                if (debug) {
                    printf("\n");
                }

                break;
            case '2':
                /* Region 2 encoding */
                /* Mode indicator */
                bin_append(5, 4, binary);

                if (debug) {
                    printf("Region 2\n");
                }

                i = 0;

                while (i < block_length) {
                    first_byte = (source[i + position] & 0xff00) >> 8;
                    second_byte = source[i + position] & 0xff;

                    glyph = (0x5e * (first_byte - 0xd8)) + (second_byte - 0xa1);

                    if (debug) {
                        printf("%d ", glyph);
                    }

                    bin_append(glyph, 12, binary);
                    i++;
                }

                /* Terminator */
                bin_append(4095, 12, binary);

                if (debug) {
                    printf("\n");
                }
                break;
            case 'd':
                /* Double byte encoding */
                /* Mode indicator */
                bin_append(6, 4, binary);

                if (debug) {
                    printf("Double byte\n");
                }

                i = 0;

                while (i < block_length) {
                    first_byte = (source[i + position] & 0xff00) >> 8;
                    second_byte = source[i + position] & 0xff;

                    if (second_byte <= 0x7e) {
                        glyph = (0xbe * (first_byte - 0x81)) + (second_byte - 0x40);
                    } else {
                        glyph = (0xbe * (first_byte - 0x81)) + (second_byte - 0x41);
                    }

                    if (debug) {
                        printf("%d ", glyph);
                    }

                    bin_append(glyph, 15, binary);
                    i++;
                }

                /* Terminator */
                bin_append(32767, 15, binary);
                /* Terminator sequence of length 12 is a mistake
                   - confirmed by Wang Yi */

                if (debug) {
                    printf("\n");
                }
                break;
            case 'f':
                /* Four-byte encoding */
                if (debug) {
                    printf("Four byte\n");
                }

                i = 0;

                while (i < block_length) {

                    /* Mode indicator */
                    bin_append(7, 4, binary);

                    first_byte = (source[i + position] & 0xff00) >> 8;
                    second_byte = source[i + position] & 0xff;
                    third_byte = (source[i + position + 1] & 0xff00) >> 8;
                    fourth_byte = source[i + position + 1] & 0xff;

                    glyph = (0x3138 * (first_byte - 0x81)) + (0x04ec * (second_byte - 0x30)) +
                            (0x0a * (third_byte - 0x81)) + (fourth_byte - 0x30);

                    if (debug) {
                        printf("%d ", glyph);
                    }

                    bin_append(glyph, 15, binary);
                    i += 2;
                }

                /* No terminator */

                if (debug) {
                    printf("\n");
                }
                break;

        }

        position += block_length;

    } while (position < length);
}

/* Finder pattern for top left of symbol */
void hx_place_finder_top_left(unsigned char* grid, int size) {
    int xp, yp;
    int x = 0, y = 0;
    char finder[] = {0x7F, 0x40, 0x5F, 0x50, 0x57, 0x57, 0x57};

    for (xp = 0; xp < 7; xp++) {
        for (yp = 0; yp < 7; yp++) {
            if (finder[yp] & 0x40 >> xp) {
                grid[((yp + y) * size) + (xp + x)] = 0x11;
            } else {
                grid[((yp + y) * size) + (xp + x)] = 0x10;
            }
        }
    }
}

/* Finder pattern for top right and bottom left of symbol */
void hx_place_finder(unsigned char* grid, int size, int x, int y) {
    int xp, yp;
    char finder[] = {0x7F, 0x01, 0x7D, 0x05, 0x75, 0x75, 0x75};

    for (xp = 0; xp < 7; xp++) {
        for (yp = 0; yp < 7; yp++) {
            if (finder[yp] & 0x40 >> xp) {
                grid[((yp + y) * size) + (xp + x)] = 0x11;
            } else {
                grid[((yp + y) * size) + (xp + x)] = 0x10;
            }
        }
    }
}

/* Finder pattern for bottom right of symbol */
void hx_place_finder_bottom_right(unsigned char* grid, int size) {
    int xp, yp;
    int x = size - 7, y = size - 7;
    char finder[] = {0x75, 0x75, 0x75, 0x05, 0x7D, 0x01, 0x7F};

    for (xp = 0; xp < 7; xp++) {
        for (yp = 0; yp < 7; yp++) {
            if (finder[yp] & 0x40 >> xp) {
                grid[((yp + y) * size) + (xp + x)] = 0x11;
            } else {
                grid[((yp + y) * size) + (xp + x)] = 0x10;
            }
        }
    }
}

/* Avoid plotting outside symbol or over finder patterns */
void hx_safe_plot(unsigned char *grid, int size, int x, int y, int value) {
    if ((x >= 0) && (x < size)) {
        if ((y >= 0) && (y < size)) {
            if (grid[(y * size) + x] == 0) {
                grid[(y * size) + x] = value;
            }
        }
    }
}

/* Plot an alignment pattern around top and right of a module */
void hx_plot_alignment(unsigned char *grid, int size, int x, int y, int w, int h) {
    int i;
    hx_safe_plot(grid, size, x, y, 0x11);
    hx_safe_plot(grid, size, x - 1, y + 1, 0x10);

    for (i = 1; i <= w; i++) {
        /* Top */
        hx_safe_plot(grid, size, x - i, y, 0x11);
        hx_safe_plot(grid, size, x - i - 1, y + 1, 0x10);
    }

    for (i = 1; i < h; i++) {
        /* Right */
        hx_safe_plot(grid, size, x, y + i, 0x11);
        hx_safe_plot(grid, size, x - 1, y + i + 1, 0x10);
    }
}

/* Plot assistant alignment patterns */
void hx_plot_assistant(unsigned char *grid, int size, int x, int y) {
    hx_safe_plot(grid, size, x - 1, y - 1, 0x10);
    hx_safe_plot(grid, size, x, y - 1, 0x10);
    hx_safe_plot(grid, size, x + 1, y - 1, 0x10);
    hx_safe_plot(grid, size, x - 1, y, 0x10);
    hx_safe_plot(grid, size, x, y, 0x11);
    hx_safe_plot(grid, size, x + 1, y, 0x10);
    hx_safe_plot(grid, size, x - 1, y + 1, 0x10);
    hx_safe_plot(grid, size, x, y + 1, 0x10);
    hx_safe_plot(grid, size, x + 1, y + 1, 0x10);
}

/* Put static elements in the grid */
void hx_setup_grid(unsigned char* grid, int size, int version) {
    int i, j;

    for (i = 0; i < size; i++) {
        for (j = 0; j < size; j++) {
            grid[(i * size) + j] = 0;
        }
    }

    /* Add finder patterns */
    hx_place_finder_top_left(grid, size);
    hx_place_finder(grid, size, 0, size - 7);
    hx_place_finder(grid, size, size - 7, 0);
    hx_place_finder_bottom_right(grid, size);

    /* Add finder pattern separator region */
    for (i = 0; i < 8; i++) {
        /* Top left */
        grid[(7 * size) + i] = 0x10;
        grid[(i * size) + 7] = 0x10;

        /* Top right */
        grid[(7 * size) + (size - i - 1)] = 0x10;
        grid[((size - i - 1) * size) + 7] = 0x10;

        /* Bottom left */
        grid[(i * size) + (size - 8)] = 0x10;
        grid[((size - 8) * size) + i] = 0x10;

        /* Bottom right */
        grid[((size - 8) * size) + (size - i - 1)] = 0x10;
        grid[((size - i - 1) * size) + (size - 8)] = 0x10;
    }

    /* Reserve function information region */
    for (i = 0; i < 9; i++) {
        /* Top left */
        grid[(8 * size) + i] = 0x10;
        grid[(i * size) + 8] = 0x10;

        /* Top right */
        grid[(8 * size) + (size - i - 1)] = 0x10;
        grid[((size - i - 1) * size) + 8] = 0x10;

        /* Bottom left */
        grid[(i * size) + (size - 9)] = 0x10;
        grid[((size - 9) * size) + i] = 0x10;

        /* Bottom right */
        grid[((size - 9) * size) + (size - i - 1)] = 0x10;
        grid[((size - i - 1) * size) + (size - 9)] = 0x10;
    }

    if (version > 3) {
        int k = hx_module_k[version - 1];
        int r = hx_module_r[version - 1];
        int m = hx_module_m[version - 1];
        int x, y, row_switch, column_switch;
        int module_height, module_width;
        int mod_x, mod_y;

        /* Add assistant alignment patterns to left and right */
        y = 0;
        mod_y = 0;
        do {
            if (mod_y < m) {
                module_height = k;
            } else {
                module_height = r - 1;
            }

            if ((mod_y % 2) == 0) {
                if ((m % 2) == 1) {
                    hx_plot_assistant(grid, size, 0, y);
                }
            } else {
                if ((m % 2) == 0) {
                    hx_plot_assistant(grid, size, 0, y);
                }
                hx_plot_assistant(grid, size, size - 1, y);
            }

            mod_y++;
            y += module_height;
        } while (y < size);

        /* Add assistant alignment patterns to top and bottom */
        x = (size - 1);
        mod_x = 0;
        do {
            if (mod_x < m) {
                module_width = k;
            } else {
                module_width = r - 1;
            }

            if ((mod_x % 2) == 0) {
                if ((m % 2) == 1) {
                    hx_plot_assistant(grid, size, x, (size - 1));
                }
            } else {
                if ((m % 2) == 0) {
                    hx_plot_assistant(grid, size, x, (size - 1));
                }
                hx_plot_assistant(grid, size, x, 0);
            }

            mod_x++;
            x -= module_width;
        } while (x >= 0);

        /* Add alignment pattern */
        column_switch = 1;
        y = 0;
        mod_y = 0;
        do {
            if (mod_y < m) {
                module_height = k;
            } else {
                module_height = r - 1;
            }

            if (column_switch == 1) {
                row_switch = 1;
                column_switch = 0;
            } else {
                row_switch = 0;
                column_switch = 1;
            }

            x = (size - 1);
            mod_x = 0;
            do {
                if (mod_x < m) {
                    module_width = k;
                } else {
                    module_width = r - 1;
                }

                if (row_switch == 1) {
                    if (!(y == 0 && x == (size - 1))) {
                        hx_plot_alignment(grid, size, x, y, module_width, module_height);
                    }
                    row_switch = 0;
                } else {
                    row_switch = 1;
                }
                mod_x++;
                x -= module_width;
            } while (x >= 0);

            mod_y++;
            y += module_height;
        } while (y < size);
    }
}

/* Calculate error correction codes */
void hx_add_ecc(unsigned char fullstream[], unsigned char datastream[], int version, int ecc_level) {
    unsigned char data_block[180];
    unsigned char ecc_block[36];
    int i, j, block;
    int input_position = -1;
    int output_position = -1;
    int table_d1_pos = ((version - 1) * 36) + ((ecc_level - 1) * 9);

    for (i = 0; i < 3; i++) {
        int batch_size = hx_table_d1[table_d1_pos + (3 * i)];
        int data_length = hx_table_d1[table_d1_pos + (3 * i) + 1];
        int ecc_length = hx_table_d1[table_d1_pos + (3 * i) + 2];

        for (block = 0; block < batch_size; block++) {
            for (j = 0; j < data_length; j++) {
                input_position++;
                output_position++;
                data_block[j] = datastream[input_position];
                fullstream[output_position] = datastream[input_position];
            }

            rs_init_gf(0x163); // x^8 + x^6 + x^5 + x + 1 = 0
            rs_init_code(ecc_length, 1);
            rs_encode(data_length, data_block, ecc_block);
            rs_free();

            for (j = 0; j < ecc_length; j++) {
                output_position++;
                fullstream[output_position] = ecc_block[ecc_length - j - 1];
            }
        }
    }
}

/* Rearrange data in batches of 13 codewords (section 5.8.2) */
void make_picket_fence(unsigned char fullstream[], unsigned char picket_fence[], int streamsize) {
    int i, start;
    int output_position = 0;

    for (start = 0; start < 13; start++) {
        for (i = start; i < streamsize; i += 13) {
            if (i < streamsize) {
                picket_fence[output_position] = fullstream[i];
                output_position++;
            }
        }
    }
}

/* Evaluate a bitmask according to table 9 */
int hx_evaluate(unsigned char *eval, int size, int pattern) {
    int x, y, block, weight;
    int result = 0;
    char state;
    int p;
    int a, b, afterCount, beforeCount;
#ifndef _MSC_VER
    char local[size * size];
#else
    char* local = (char *) _alloca((size * size) * sizeof (char));
#endif

    /* all four bitmask variants have been encoded in the 4 bits of the bytes
     * that make up the grid array. select them for evaluation according to the
     * desired pattern.*/
    for (x = 0; x < size; x++) {
        for (y = 0; y < size; y++) {
            if ((eval[(y * size) + x] & (0x01 << pattern)) != 0) {
                local[(y * size) + x] = '1';
            } else {
                local[(y * size) + x] = '0';
            }
        }
    }

    /* Test 1: 1:1:1:1:3  or 3:1:1:1:1 ratio pattern in row/column */
    /* Vertical */
    for (x = 0; x < size; x++) {
        for (y = 0; y < (size - 7); y++) {
            p = 0;
            for (weight = 0; weight < 7; weight++) {
                if (local[((y + weight) * size) + x] == '1') {
                    p += (0x40 >> weight);
                }
            }
            if ((p == 0x57) || (p == 0x75)) {
                /* Pattern found, check before and after */
                beforeCount = 0;
                for (b = (y - 3); b < y; b++) {
                    if (b < 0) {
                        beforeCount++;
                    } else {
                        if (local[(b * size) + x] == '0') {
                            beforeCount++;
                        } else {
                            beforeCount = 0;
                        }
                    }
                }

                afterCount = 0;
                for (a = (y + 7); a <= (y + 9); a++) {
                    if (a >= size) {
                        afterCount++;
                    } else {
                        if (local[(a * size) + x] == '0') {
                            afterCount++;
                        } else {
                            afterCount = 0;
                        }
                    }
                }

                if ((beforeCount == 3) || (afterCount == 3)) {
                    /* Pattern is preceeded or followed by light area
                       3 modules wide */
                    result += 50;
                }
            }
        }
    }

    /* Horizontal */
    for (y = 0; y < size; y++) {
        for (x = 0; x < (size - 7); x++) {
            p = 0;
            for (weight = 0; weight < 7; weight++) {
                if (local[(y * size) + x + weight] == '1') {
                    p += (0x40 >> weight);
                }
            }
            if ((p == 0x57) || (p == 0x75)) {
                /* Pattern found, check before and after */
                beforeCount = 0;
                for (b = (x - 3); b < x; b++) {
                    if (b < 0) {
                        beforeCount++;
                    } else {
                        if (local[(y * size) + b] == '0') {
                            beforeCount++;
                        } else {
                            beforeCount = 0;
                        }
                    }
                }

                afterCount = 0;
                for (a = (x + 7); a <= (x + 9); a++) {
                    if (a >= size) {
                        afterCount++;
                    } else {
                        if (local[(y * size) + a] == '0') {
                            afterCount++;
                        } else {
                            afterCount = 0;
                        }
                    }
                }

                if ((beforeCount == 3) || (afterCount == 3)) {
                    /* Pattern is preceeded or followed by light area
                       3 modules wide */
                    result += 50;
                }
            }
        }
    }

    /* Test 2: Adjacent modules in row/column in same colour */
    /* In AIMD-15 section 5.8.3.2 it is stated... “In Table 9 below, i refers to the row
     * position of the module.” - however i being the length of the run of the
     * same colour (i.e. "block" below) in the same fashion as ISO/IEC 18004
     * makes more sense. -- Confirmed by Wang Yi */

    /* Vertical */
    for (x = 0; x < size; x++) {
        state = local[x];
        block = 0;
        for (y = 0; y < size; y++) {
            if (local[(y * size) + x] == state) {
                block++;
            } else {
                if (block > 3) {
                    result += (3 + block) * 4;
                }
                block = 0;
                state = local[(y * size) + x];
            }
        }
        if (block > 3) {
            result += (3 + block) * 4;
        }
    }

    /* Horizontal */
    for (y = 0; y < size; y++) {
        state = local[y * size];
        block = 0;
        for (x = 0; x < size; x++) {
            if (local[(y * size) + x] == state) {
                block++;
            } else {
                if (block > 3) {
                    result += (3 + block) * 4;
                }
                block = 0;
                state = local[(y * size) + x];
            }
        }
        if (block > 3) {
            result += (3 + block) * 4;
        }
    }

    return result;
}

/* Apply the four possible bitmasks for evaluation */
int hx_apply_bitmask(unsigned char *grid, int size) {
    int x, y;
    int i, j;
    int pattern, penalty[4];
    int best_pattern, best_val;
    int bit;
    unsigned char p;

#ifndef _MSC_VER
    unsigned char mask[size * size];
    unsigned char eval[size * size];
#else
    unsigned char* mask = (unsigned char *) _alloca((size * size) * sizeof (unsigned char));
    unsigned char* eval = (unsigned char *) _alloca((size * size) * sizeof (unsigned char));
#endif

    /* Perform data masking */
    for (x = 0; x < size; x++) {
        for (y = 0; y < size; y++) {
            mask[(y * size) + x] = 0x00;
            j = x + 1;
            i = y + 1;

            if (!(grid[(y * size) + x] & 0xf0)) {
                if ((i + j) % 2 == 0) {
                    mask[(y * size) + x] += 0x02;
                }
                if ((((i + j) % 3) + (j % 3)) % 2 == 0) {
                    mask[(y * size) + x] += 0x04;
                }
                if (((i % j) + (j % i) + (i % 3) + (j % 3)) % 2 == 0) {
                    mask[(y * size) + x] += 0x08;
                }
            }
        }
    }

    // apply data masks to grid, result in eval
    for (x = 0; x < size; x++) {
        for (y = 0; y < size; y++) {
            if (grid[(y * size) + x] & 0x01) {
                p = 0xff;
            } else {
                p = 0x00;
            }

            eval[(y * size) + x] = mask[(y * size) + x] ^ p;
        }
    }

    /* Evaluate result */
    for (pattern = 0; pattern < 4; pattern++) {
        penalty[pattern] = hx_evaluate(eval, size, pattern);
    }

    best_pattern = 0;
    best_val = penalty[0];
    for (pattern = 1; pattern < 4; pattern++) {
        if (penalty[pattern] < best_val) {
            best_pattern = pattern;
            best_val = penalty[pattern];
        }
    }

    /* Apply mask */
    for (x = 0; x < size; x++) {
        for (y = 0; y < size; y++) {
            bit = 0;
            switch (best_pattern) {
                case 0: if (mask[(y * size) + x] & 0x01) {
                        bit = 1;
                    }
                    break;
                case 1: if (mask[(y * size) + x] & 0x02) {
                        bit = 1;
                    }
                    break;
                case 2: if (mask[(y * size) + x] & 0x04) {
                        bit = 1;
                    }
                    break;
                case 3: if (mask[(y * size) + x] & 0x08) {
                        bit = 1;
                    }
                    break;
            }
            if (bit == 1) {
                if (grid[(y * size) + x] & 0x01) {
                    grid[(y * size) + x] = 0x00;
                } else {
                    grid[(y * size) + x] = 0x01;
                }
            }
        }
    }

    return best_pattern;
}

/* Han Xin Code - main */
int han_xin(struct zint_symbol *symbol, const unsigned char source[], size_t length) {
    int est_binlen;
    int ecc_level = symbol->option_1;
    int i, j, version;
    int data_codewords = 0, size;
    int codewords;
    int bitmask;
    int bin_len;
    char function_information[36];
    unsigned char fi_cw[3] = {0, 0, 0};
    unsigned char fi_ecc[4];

#ifndef _MSC_VER
    int utfdata[length + 1];
    int gbdata[(length + 1) * 2];
    char mode[length + 1];
#else
    int* utfdata = (int *) _alloca((length + 1) * sizeof (int));
    int* gbdata = (int *) _alloca(((length + 1) * 2) * sizeof (int));
    char* mode = (char *) _alloca((length + 1) * sizeof (char));
    char* binary;
    unsigned char *datastream;
    unsigned char *fullstream;
    unsigned char *picket_fence;
    unsigned char *grid;
#endif

    if ((symbol->input_mode == DATA_MODE) || (symbol->eci != 3)) {
        for (i = 0; i < length; i++) {
            gbdata[i] = (int) source[i];
        }
    } else {
        int posn;
        /* Convert Unicode input to GB-18030 */
        int error_number = utf8toutf16(symbol, source, utfdata, &length);
        if (error_number != 0) {
            return error_number;
        }

        posn = 0;
        for (i = 0; i < length; i++) {
            int done = 0;
            gbdata[posn] = 0;

            /* Single byte characters in range U+0000 -> U+007F */
            if (utfdata[i] <= 0x7f) {
                gbdata[posn] = utfdata[i];
                posn++;
                done = 1;
            }

            /* Two bytes characters in GB-2312 */
            if (done == 0) {
                j = 0;
                do {
                    if (gb2312_lookup[j * 2] == utfdata[i]) {
                        gbdata[posn] = gb2312_lookup[(j * 2) + 1];
                        posn++;
                        done = 1;
                    }
                    j++;
                } while ((j < 7445) && (done == 0));
            }

            /* Two byte characters in GB-18030 */
            if (done == 0) {
                j = 0;
                do {
                    if (gb18030_twobyte_lookup[j * 2] == utfdata[i]) {
                        gbdata[posn] = gb18030_twobyte_lookup[(j * 2) + 1];
                        posn++;
                        done = 1;
                    }
                    j++;
                } while ((j < 16495) && (done == 0));
            }

            /* Four byte characters in range U+0080 -> U+FFFF */
            if (done == 0) {
                j = 0;
                do {
                    if (gb18030_fourbyte_lookup[j * 3] == utfdata[i]) {
                        gbdata[posn] = gb18030_fourbyte_lookup[(j * 3) + 1];
                        gbdata[posn + 1] = gb18030_fourbyte_lookup[(j * 3) + 2];
                        posn += 2;
                        done = 1;
                    }
                    j++;
                } while ((j < 6793) && (done == 0));
            }

            /* Supplementary planes U+10000 -> U+1FFFF */
            if (done == 0) {
                if (utfdata[i] >= 0x10000 && utfdata[i] < 0x110000) {
                    /* algorithm from libiconv-1.15\lib\gb18030.h */
                    int j, r3, r2, r1, r0;

                    j = utfdata[i] - 0x10000;
                    r3 = (j % 10) + 0x30; j = j / 10;
                    r2 = (j % 126) + 0x81; j = j / 126;
                    r1 = (j % 10) + 0x30; j = j / 10;
                    r0 = j + 0x90;
                    gbdata[posn] = (r0 << 8) + r1;
                    gbdata[posn + 1] = (r2 << 8) + r3;
                    posn += 2;
                    done = 1;
                }
            }

            /* Character not found */
            if (done == 0) {
                strcpy(symbol->errtxt, "540: Unknown character in input data");
                return ZINT_ERROR_INVALID_DATA;
            }
        }
        length = posn;
    }

    hx_define_mode(mode, gbdata, length);

    est_binlen = calculate_binlength(mode, gbdata, length, symbol->eci);

#ifndef _MSC_VER
    char binary[est_binlen + 10];
#else
    binary = (char *) _alloca((est_binlen + 10) * sizeof (char));
#endif
    memset(binary, 0, (est_binlen + 1) * sizeof (char));

    if ((ecc_level <= 0) || (ecc_level >= 5)) {
        ecc_level = 1;
    }

    calculate_binary(binary, mode, gbdata, length, symbol->eci, symbol->debug);
    bin_len = strlen(binary);
    codewords = bin_len / 8;
    if (bin_len % 8 != 0) {
        codewords++;
    }

    version = 85;
    for (i = 84; i > 0; i--) {
        switch (ecc_level) {
            case 1:
                if (hx_data_codewords_L1[i - 1] > codewords) {
                    version = i;
                    data_codewords = hx_data_codewords_L1[i - 1];
                }
                break;
            case 2:
                if (hx_data_codewords_L2[i - 1] > codewords) {
                    version = i;
                    data_codewords = hx_data_codewords_L2[i - 1];
                }
                break;
            case 3:
                if (hx_data_codewords_L3[i - 1] > codewords) {
                    version = i;
                    data_codewords = hx_data_codewords_L3[i - 1];
                }
                break;
            case 4:
                if (hx_data_codewords_L4[i - 1] > codewords) {
                    version = i;
                    data_codewords = hx_data_codewords_L4[i - 1];
                }
                break;
            default:
                assert(0);
                break;
        }
    }

    if (version == 85) {
        strcpy(symbol->errtxt, "541: Input too long for selected error correction level");
        return ZINT_ERROR_TOO_LONG;
    }

    if ((symbol->option_2 < 0) || (symbol->option_2 > 84)) {
        symbol->option_2 = 0;
    }

    if (symbol->option_2 > version) {
        version = symbol->option_2;
    }

    if ((symbol->option_2 != 0) && (symbol->option_2 < version)) {
        strcpy(symbol->errtxt, "542: Input too long for selected symbol size");
        return ZINT_ERROR_TOO_LONG;
    }

    /* If there is spare capacity, increase the level of ECC */

    if ((ecc_level == 1) && (codewords < hx_data_codewords_L2[version - 1])) {
        ecc_level = 2;
        data_codewords = hx_data_codewords_L2[version - 1];
    }

    if ((ecc_level == 2) && (codewords < hx_data_codewords_L3[version - 1])) {
        ecc_level = 3;
        data_codewords = hx_data_codewords_L3[version - 1];
    }

    if ((ecc_level == 3) && (codewords < hx_data_codewords_L4[version - 1])) {
        ecc_level = 4;
        data_codewords = hx_data_codewords_L4[version - 1];
    }

    //printf("Version %d, ECC %d\n", version, ecc_level);

    size = (version * 2) + 21;

#ifndef _MSC_VER
    unsigned char datastream[data_codewords];
    unsigned char fullstream[hx_total_codewords[version - 1]];
    unsigned char picket_fence[hx_total_codewords[version - 1]];
    unsigned char grid[size * size];
#else
    datastream = (unsigned char *) _alloca((data_codewords) * sizeof (unsigned char));
    fullstream = (unsigned char *) _alloca((hx_total_codewords[version - 1]) * sizeof (unsigned char));
    picket_fence = (unsigned char *) _alloca((hx_total_codewords[version - 1]) * sizeof (unsigned char));
    grid = (unsigned char *) _alloca((size * size) * sizeof (unsigned char));
#endif

    for (i = 0; i < data_codewords; i++) {
        datastream[i] = 0;
    }

    for (i = 0; i < bin_len; i++) {
        if (binary[i] == '1') {
            datastream[i / 8] += 0x80 >> (i % 8);
        }
    }

    hx_setup_grid(grid, size, version);

    hx_add_ecc(fullstream, datastream, version, ecc_level);

    make_picket_fence(fullstream, picket_fence, hx_total_codewords[version - 1]);

    /* Populate grid */
    j = 0;
    for (i = 0; i < (size * size); i++) {
        if (grid[i] == 0x00) {
            if (j < (hx_total_codewords[version - 1] * 8)) {
                if (picket_fence[(j / 8)] & (0x80 >> (j % 8))) {
                    grid[i] = 0x01;
                }
                j++;
            }
        }
    }

    bitmask = hx_apply_bitmask(grid, size);

    /* Form function information string */
    for (i = 0; i < 34; i++) {
        if (i % 2) {
            function_information[i] = '1';
        } else {
            function_information[i] = '0';
        }
    }
    function_information[34] = '\0';

    for (i = 0; i < 8; i++) {
        if ((version + 20) & (0x80 >> i)) {
            function_information[i] = '1';
        } else {
            function_information[i] = '0';
        }
    }

    for (i = 0; i < 2; i++) {
        if ((ecc_level - 1) & (0x02 >> i)) {
            function_information[i + 8] = '1';
        } else {
            function_information[i + 8] = '0';
        }
    }

    for (i = 0; i < 2; i++) {
        if (bitmask & (0x02 >> i)) {
            function_information[i + 10] = '1';
        } else {
            function_information[i + 10] = '0';
        }
    }



    for (i = 0; i < 3; i++) {
        for (j = 0; j < 4; j++) {
            if (function_information[(i * 4) + j] == '1') {
                fi_cw[i] += (0x08 >> j);
            }
        }
    }

    rs_init_gf(0x13);
    rs_init_code(4, 1);
    rs_encode(3, fi_cw, fi_ecc);
    rs_free();

    for (i = 0; i < 4; i++) {
        for (j = 0; j < 4; j++) {
            if (fi_ecc[3 - i] & (0x08 >> j)) {
                function_information[(i * 4) + j + 12] = '1';
            } else {
                function_information[(i * 4) + j + 12] = '0';
            }
        }
    }

    /* Add function information to symbol */
    for (i = 0; i < 9; i++) {
        if (function_information[i] == '1') {
            grid[(8 * size) + i] = 0x01;
            grid[((size - 8 - 1) * size) + (size - i - 1)] = 0x01;
        }
        if (function_information[i + 8] == '1') {
            grid[((8 - i) * size) + 8] = 0x01;
            grid[((size - 8 - 1 + i) * size) + (size - 8 - 1)] = 0x01;
        }
        if (function_information[i + 17] == '1') {
            grid[(i * size) + (size - 1 - 8)] = 0x01;
            grid[((size - 1 - i) * size) + 8] = 0x01;
        }
        if (function_information[i + 25] == '1') {
            grid[(8 * size) + (size - 1 - 8 + i)] = 0x01;
            grid[((size - 1 - 8) * size) + (8 - i)] = 0x01;
        }
    }

    symbol->width = size;
    symbol->rows = size;

    for (i = 0; i < size; i++) {
        for (j = 0; j < size; j++) {
            if (grid[(i * size) + j] & 0x01) {
                set_module(symbol, i, j);
            }
        }
        symbol->row_height[i] = 1;
    }

    return 0;
}


