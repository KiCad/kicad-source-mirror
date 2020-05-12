/* dotcode.c - Handles DotCode */

/*
    libzint - the open source barcode library
    Copyright (C) 2017 Robin Stuart <rstuart114@gmail.com>

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

/*
 * Attempts to encode DotCode according to AIMD013 Rev 1.34a, dated Feb 19, 2009
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>
#ifndef _MSC_VER
#include <stdint.h>
#else
#include "ms_stdint.h"
#include <malloc.h>
#endif
#include "common.h"
#include "gs1.h"

#define GF 113
#define PM 3

/* DotCode symbol character dot patterns, from Annex C */
static const unsigned short int dot_patterns[113] = {
    0x155, 0x0ab, 0x0ad, 0x0b5, 0x0d5, 0x156, 0x15a, 0x16a, 0x1aa, 0x0ae,
    0x0b6, 0x0ba, 0x0d6, 0x0da, 0x0ea, 0x12b, 0x12d, 0x135, 0x14b, 0x14d,
    0x153, 0x159, 0x165, 0x169, 0x195, 0x1a5, 0x1a9, 0x057, 0x05b, 0x05d,
    0x06b, 0x06d, 0x075, 0x097, 0x09b, 0x09d, 0x0a7, 0x0b3, 0x0b9, 0x0cb,
    0x0cd, 0x0d3, 0x0d9, 0x0e5, 0x0e9, 0x12e, 0x136, 0x13a, 0x14e, 0x15c,
    0x166, 0x16c, 0x172, 0x174, 0x196, 0x19a, 0x1a6, 0x1ac, 0x1b2, 0x1b4,
    0x1ca, 0x1d2, 0x1d4, 0x05e, 0x06e, 0x076, 0x07a, 0x09e, 0x0bc, 0x0ce,
    0x0dc, 0x0e6, 0x0ec, 0x0f2, 0x0f4, 0x117, 0x11b, 0x11d, 0x127, 0x133,
    0x139, 0x147, 0x163, 0x171, 0x18b, 0x18d, 0x193, 0x199, 0x1a3, 0x1b1,
    0x1c5, 0x1c9, 0x1d1, 0x02f, 0x037, 0x03b, 0x03d, 0x04f, 0x067, 0x073,
    0x079, 0x08f, 0x0c7, 0x0e3, 0x0f1, 0x11e, 0x13c, 0x178, 0x18e, 0x19c,
    0x1b8, 0x1c6, 0x1cc
};

static int get_dot(char Dots[], const int Hgt, const int Wid, const int x, const int y) {
    int retval = 0;

    if ((x >= 0) && (x < Wid) && (y >= 0) && (y < Hgt)) {
        if (Dots[(y * Wid) + x] == '1') {
            retval = 1;
        }
    }

    return retval;
}

static int clr_col(char *Dots, const int Hgt, const int Wid, const int x) {
    int y;
    for (y = x & 1; y < Hgt; y += 2) {
        if (get_dot(Dots, Hgt, Wid, x, y)) {
            return 0;
        }
    }

    return 1;
}

static int clr_row(char *Dots, const int Hgt, const int Wid, const int y) {
    int x;
    for (x = y & 1; x < Wid; x += 2) {
        if (get_dot(Dots, Hgt, Wid, x, y)) {
            return 0;
        }
    }

    return 1;
}

/* Dot pattern scoring routine from Annex A */
const int score_array(char Dots[], int Hgt, int Wid) {
    int x, y, worstedge, first, last, sum;
    int penalty_local = 0;
    int penalty = 0;
    int t = 0, b = 0, l = 0, r = 0;

    // first, guard against "pathelogical" gaps in the array
    if (Hgt & 1) {
        if (Hgt < 12) {
            sum = 0;
            for (x = 1; x < Wid - 1; x++) {
                if (!(clr_col(Dots, Hgt, Wid, x))) {
                    sum = 0;
                    if (penalty_local) {
                        penalty += penalty_local;
                        penalty_local = 0;
                    }
                } else {
                    sum++;
                    if (sum == 1) {
                        penalty_local = Hgt;
                    } else {
                        penalty_local *= Hgt;
                    }
                }
            }
        }
    } else {
        if (Wid < 12) {
            sum = 0;
            for (y = 1; y < Hgt - 1; y++) {
                if (!(clr_row(Dots, Hgt, Wid, y))) {
                    sum = 0;
                    if (penalty_local) {
                        penalty += penalty_local;
                        penalty_local = 0;
                    }
                } else {
                    sum++;
                    if (sum == 1) {
                        penalty_local = Wid;
                    } else {
                        penalty_local *= Wid;
                    }
                }
            }
        }
    }

    sum = 0;
    first = -1;
    last = -1;

    // across the top edge, count printed dots and measure their extent
    for (x = 0; x < Wid; x += 2)
        if (get_dot(Dots, Hgt, Wid, x, 0)) {
            if (first < 0) {
                first = x;
            }
            last = x;
            sum++;
        }
    worstedge = sum + last - first;
    worstedge *= Hgt;
    
    if (sum == 0) {
        t = 1;
    }

    sum = 0;
    first = -1;
    last = -1;

    //across the bottom edge, ditto
    for (x = Wid & 1; x < Wid; x += 2)
        if (get_dot(Dots, Hgt, Wid, x, Hgt - 1)) {
            if (first < 0) {
                first = x;
            }
            last = x;
            sum++;
        }
    sum += last - first;
    sum *= Hgt;
    if (sum < worstedge) {
        worstedge = sum;
    }
    
    if (sum == 0) {
        b = 2;
    }

    sum = 0;
    first = -1;
    last = -1;

    //down the left edge, ditto
    for (y = 0; y < Hgt; y += 2)
        if (get_dot(Dots, Hgt, Wid, 0, y)) {
            if (first < 0) {
                first = y;
            }
            last = y;
            sum++;
        }
    sum += last - first;
    sum *= Wid;
    if (sum < worstedge) {
        worstedge = sum;
    }
    
    if (sum == 0) {
        l = 4;
    }

    sum = 0;
    first = -1;
    last = -1;

    //down the right edge, ditto
    for (y = Hgt & 1; y < Hgt; y += 2)
        if (get_dot(Dots, Hgt, Wid, Wid - 1, y)) {
            if (first < 0) {
                first = y;
            }
            last = y;
            sum++;
        }
    sum += last - first;
    sum *= Wid;
    if (sum < worstedge) {
        worstedge = sum;
    }
    
    if (sum == 0) {
        r = 8;
    }

    penalty += (t + b + l + r) * 100000;

    // throughout the array, count the # of unprinted 5-somes (cross patterns)
    // plus the # of printed dots surrounded by 8 unprinted neighbors
    sum = 0;
    for (y = 0; y < Hgt; y++) {
        for (x = y & 1; x < Wid; x += 2) {
            if ((!get_dot(Dots, Hgt, Wid, x - 1, y - 1))
                    && (!get_dot(Dots, Hgt, Wid, x + 1, y - 1))
                    && (!get_dot(Dots, Hgt, Wid, x - 1, y + 1))
                    && (!get_dot(Dots, Hgt, Wid, x + 1, y + 1))
                    && ((!get_dot(Dots, Hgt, Wid, x, y))
                    || ((!get_dot(Dots, Hgt, Wid, x - 2, y))
                    && (!get_dot(Dots, Hgt, Wid, x, y - 2))
                    && (!get_dot(Dots, Hgt, Wid, x + 2, y))
                    && (!get_dot(Dots, Hgt, Wid, x, y + 2))))) {
                sum++;
            }
        }
    }

    return (worstedge - sum * sum - penalty);
}

//-------------------------------------------------------------------------
// "rsencode(nd,nc)" adds "nc" R-S check words to "nd" data words in wd[]
// employing Galois Field GF, where GF is prime, with a prime modulus of PM
//-------------------------------------------------------------------------

void rsencode(int nd, int nc, unsigned char *wd) {
    int i, j, k, nw, start, step, root[GF], c[GF];

    // Start by generating "nc" roots (antilogs):
    root[0] = 1;
    for (i = 1; (i <= nc) && (i < GF); i++)
        root[i] = (PM * root[i - 1]) % GF;

    // Here we compute how many interleaved R-S blocks will be needed
    nw = nd + nc;
    step = (nw + GF - 2) / (GF - 1);

    // ...& then for each such block:
    for (start = 0; start < step; start++) {
        int ND = (nd - start + step - 1) / step, NW = (nw - start + step - 1) / step, NC = NW - ND;

        // first compute the generator polynomial "c" of order "NC":
        for (i = 1; i <= NC; i++)
            c[i] = 0;
        c[0] = 1;

        for (i = 1; i <= NC; i++) {
            for (j = NC; j >= 1; j--) {
                c[j] = (GF + c[j] - (root[i] * c[j - 1]) % GF) % GF;
            }
        }

        // & then compute the corresponding checkword values into wd[]
        // ... (a) starting at wd[start] & (b) stepping by step
        for (i = ND; i < NW; i++)
            wd[start + i * step] = 0;
        for (i = 0; i < ND; i++) {
            k = (wd[start + i * step] + wd[start + ND * step]) % GF;
            for (j = 0; j < NC - 1; j++) {
                wd[start + (ND + j) * step] = (GF - ((c[j + 1] * k) % GF) + wd[start + (ND + j + 1) * step]) % GF;
            }
            wd[start + (ND + NC - 1) * step] = (GF - ((c[NC] * k) % GF)) % GF;
        }
        for (i = ND; i < NW; i++)
            wd[start + i * step] = (GF - wd[start + i * step]) % GF;
    }
}

/* Check if the next character is directly encodable in code set A (Annex F.II.D) */
int datum_a(const unsigned char source[], int position, int length) {
    int retval = 0;

    if (position < length) {
        if (source[position] <= 95) {
            retval = 1;
        }
    }

    return retval;
}

/* Check if the next character is directly encodable in code set B (Annex F.II.D) */
int datum_b(const unsigned char source[], int position, int length) {
    int retval = 0;

    if (position < length) {
        if ((source[position] >= 32) && (source[position] <= 127)) {
            retval = 1;
        }

        switch (source[position]) {
            case 9: // HT
            case 28: // FS
            case 29: // GS
            case 30: // RS
                retval = 1;
        }

        if (position != length - 2) {
            if ((source[position] == 13) && (source[position + 1] == 10)) { // CRLF
                retval = 1;
            }
        }
    }

    return retval;
}

/* Check if the next characters are directly encodable in code set C (Annex F.II.D) */
int datum_c(const unsigned char source[], int position, int length) {
    int retval = 0;

    if (position <= length - 2) {
        if (((source[position] >= '0') && (source[position] <= '9'))
                && ((source[position + 1] >= '0') && (source[position + 1] <= '9')))
            retval = 1;
    }

    return retval;
}

/* Returns how many consecutive digits lie immediately ahead (Annex F.II.A) */
int n_digits(const unsigned char source[], int position, int length) {
    int i;

    for (i = position; ((source[i] >= '0') && (source[i] <= '9')) && (i < length); i++);

    return i - position;
}

/* checks ahead for 10 or more digits starting "17xxxxxx10..." (Annex F.II.B) */
int seventeen_ten(const unsigned char source[], int position, int length) {
    int found = 0;

    if (n_digits(source, position, length) >= 10) {
        if (((source[position] == '1') && (source[position + 1] == '7'))
                && ((source[position + 8] == '1') && (source[position + 9] == '0'))) {
            found = 1;
        }
    }

    return found;
}

/*  checks how many characters ahead can be reached while datum_c is true,
 *  returning the resulting number of codewords (Annex F.II.E)
 */
int ahead_c(const unsigned char source[], int position, int length) {
    int count = 0;
    int i;

    for (i = position; (i < length) && datum_c(source, i, length); i += 2) {
        count++;
    }

    return count;
}

/* Annex F.II.F */
int try_c(const unsigned char source[], int position, int length) {
    int retval = 0;

    if (n_digits(source, position, length) > 0) {
        if (ahead_c(source, position, length) > ahead_c(source, position + 1, length)) {
            retval = ahead_c(source, position, length);
        }
    }

    return retval;
}

/* Annex F.II.G */
int ahead_a(const unsigned char source[], int position, int length) {
    int count = 0;
    int i;

    for (i = position; ((i < length) && datum_a(source, i, length))
            && (try_c(source, i, length) < 2); i++) {
        count++;
    }

    return count;
}

/* Annex F.II.H */
int ahead_b(const unsigned char source[], int position, int length) {
    int count = 0;
    int i;

    for (i = position; ((i < length) && datum_b(source, i, length))
            && (try_c(source, i, length) < 2); i++) {
        count++;
    }

    return count;
}

/* checks if the next character is in the range 128 to 255  (Annex F.II.I) */
int binary(const unsigned char source[], int position) {
    int retval = 0;

    if (source[position] >= 128) {
        retval = 1;
    }

    return retval;
}

/* Analyse input data stream and encode using algorithm from Annex F */
int dotcode_encode_message(struct zint_symbol *symbol, const unsigned char source[], int length, unsigned char *codeword_array, int *binary_finish) {
    int input_position, array_length, i;
    char encoding_mode;
    int inside_macro;
    int debug = symbol->debug;
    int binary_buffer_size = 0;
    int lawrencium[6]; // Reversed radix 103 values

#if defined(_MSC_VER) && _MSC_VER == 1200
    uint64_t binary_buffer = 0;
#else
    uint64_t binary_buffer = 0ULL;
#endif

    input_position = 0;
    array_length = 0;
    encoding_mode = 'C';
    inside_macro = 0;

    if (symbol->output_options & READER_INIT) {
        codeword_array[array_length] = 109; // FNC3
        array_length++;
    }

    if (symbol->input_mode != GS1_MODE) {
        if (length > 2) {
            if (((source[input_position] >= '0') && (source[input_position] <= '9')) &&
                    ((source[input_position + 1] >= '0') && (source[input_position + 1] <= '9'))) {
                codeword_array[array_length] = 107; // FNC1
                array_length++;
            }
        }
    }

    if (symbol->eci > 3) {
        codeword_array[array_length] = 108; // FNC2
        array_length++;
        if (symbol->eci <= 39) {
            codeword_array[array_length] = symbol->eci;
            array_length++;
        } else {
            // the next three codewords valued A, B & C encode the ECI value of
            // (A - 40) * 12769 + B * 113 + C + 40 (Section 5.2.1)
            int a, b, c;
            a = (symbol->eci - 40) % 12769;
            b = ((symbol->eci - 40) - (12769 * a)) % 113;
            c = (symbol->eci - 40) - (12769 * a) - (113 * b);

            codeword_array[array_length] = a + 40;
            array_length++;
            codeword_array[array_length] = b;
            array_length++;
            codeword_array[array_length] = c;
            array_length++;
        }
    }

    // Prevent encodation as a macro if a special character is in first position
    if (source[input_position] == 9) {
        codeword_array[array_length] = 101; // Latch A
        array_length++;
        codeword_array[array_length] = 73; // HT
        array_length++;
        encoding_mode = 'A';
    }

    if (source[input_position] == 28) {
        codeword_array[array_length] = 101; // Latch A
        array_length++;
        codeword_array[array_length] = 92; // FS
        array_length++;
        encoding_mode = 'A';
    }

    if (source[input_position] == 29) {
        codeword_array[array_length] = 101; // Latch A
        array_length++;
        codeword_array[array_length] = 93; // GS
        array_length++;
        encoding_mode = 'A';
    }

    if (source[input_position] == 30) {
        codeword_array[array_length] = 101; // Latch A
        array_length++;
        codeword_array[array_length] = 94; // RS
        array_length++;
        encoding_mode = 'A';
    }

    do {
        int done = 0;
        /* Step A */
        if ((input_position == length - 2) && (inside_macro != 0) && (inside_macro != 100)) {
            // inside_macro only gets set to 97, 98 or 99 if the last two characters are RS/EOT
            input_position += 2;
            done = 1;
            if (debug) {
                printf("A ");
            }
        }

        if ((input_position == length - 1) && (inside_macro == 100)) {
            // inside_macro only gets set to 100 if the last character is EOT
            input_position++;
            done = 1;
            if (debug) {
                printf("A ");
            }
        }

        /* Step B1 */
        if ((!done) && (encoding_mode == 'C')) {
            if ((array_length == 0) && (length > 9)) {
                if ((source[input_position] == '[')
                        && (source[input_position + 1] == ')')
                        && (source[input_position + 2] == '>')
                        && (source[input_position + 3] == 30) // RS
                        && (source[length - 1] == 04)) { // EOT


                    if ((source[input_position + 6] == 29) && (source[length - 2] == 30)) { // GS/RS
                        if ((source[input_position + 4] == '0') && (source[input_position + 5] == '5')) {
                            codeword_array[array_length] = 102; // Shift B
                            array_length++;
                            codeword_array[array_length] = 97; // Macro
                            array_length++;
                            input_position += 7;
                            inside_macro = 97;
                            done = 1;
                            if (debug) {
                                printf("B1/1 ");
                            }
                        }

                        if ((source[input_position + 4] == '0') && (source[input_position + 5] == '6')) {
                            codeword_array[array_length] = 102; // Shift B
                            array_length++;
                            codeword_array[array_length] = 98; // Macro
                            array_length++;
                            input_position += 7;
                            inside_macro = 98;
                            done = 1;
                            if (debug) {
                                printf("B1/2 ");
                            }
                        }

                        if ((source[input_position + 4] == '1') && (source[input_position + 5] == '2')) {
                            codeword_array[array_length] = 102; // Shift B
                            array_length++;
                            codeword_array[array_length] = 99; // Macro
                            array_length++;
                            input_position += 7;
                            inside_macro = 99;
                            done = 1;
                            if (debug) {
                                printf("B1/3 ");
                            }
                        }
                    }

                    if ((!done) && (source[input_position] >= '0') && (source[input_position] <= '9') &&
                            (source[input_position + 1] >= '0') && (source[input_position + 1] <= '9')) {
                        codeword_array[array_length] = 102; // Shift B
                        array_length++;
                        codeword_array[array_length] = 100; // Macro
                        array_length++;
                        input_position += 4;
                        inside_macro = 100;
                        done = 1;
                        if (debug) {
                            printf("B1/4 ");
                        }
                    }
                }
            }
        }

        /* Step B2 */
        if ((!done) && (encoding_mode == 'C')) {
            if (seventeen_ten(source, input_position, length)) {
                codeword_array[array_length] = 100; // (17)...(10)
                array_length++;
                codeword_array[array_length] = ((source[input_position + 2] - '0') * 10) + (source[input_position + 3] - '0');
                array_length++;
                codeword_array[array_length] = ((source[input_position + 4] - '0') * 10) + (source[input_position + 5] - '0');
                array_length++;
                codeword_array[array_length] = ((source[input_position + 6] - '0') * 10) + (source[input_position + 7] - '0');
                array_length++;
                input_position += 10;
                done = 1;
                if (debug) {
                    printf("B2/1 ");
                }
            }
        }

        if ((!done) && (encoding_mode == 'C')) {
            if (datum_c(source, input_position, length) || ((source[input_position] == '[') && (symbol->input_mode == GS1_MODE))) {
                if (source[input_position] == '[') {
                    codeword_array[array_length] = 107; // FNC1
                    input_position++;
                } else {
                    codeword_array[array_length] = ((source[input_position] - '0') * 10) + (source[input_position + 1] - '0');
                    input_position += 2;
                }
                array_length++;
                done = 1;
                if (debug) {
                    printf("B2/2 ");
                }
            }
        }

        /* Setp B3 */
        if ((!done) && (encoding_mode == 'C')) {
            if (binary(source, input_position)) {
                if (n_digits(source, input_position + 1, length) > 0) {
                    if ((source[input_position] - 128) < 32) {
                        codeword_array[array_length] = 110; // Bin Shift A
                        array_length++;
                        codeword_array[array_length] = source[input_position] - 128 + 64;
                        array_length++;
                    } else {
                        codeword_array[array_length] = 111; // Bin Shift B
                        array_length++;
                        codeword_array[array_length] = source[input_position] - 128 - 32;
                        array_length++;
                    }
                    input_position++;
                } else {
                    codeword_array[array_length] = 112; // Bin Latch
                    array_length++;
                    encoding_mode = 'X';
                }
                done = 1;
                if (debug) {
                    printf("B3 ");
                }
            }
        }

        /* Step B4 */
        if ((!done) && (encoding_mode == 'C')) {
            int m = ahead_a(source, input_position, length);
            int n = ahead_b(source, input_position, length);
            if (m > n) {
                codeword_array[array_length] = 101; // Latch A
                array_length++;
                encoding_mode = 'A';
            } else {
                if (n <= 4) {
                    codeword_array[array_length] = 101 + n; // nx Shift B
                    array_length++;

                    for (i = 0; i < n; i++) {
                        codeword_array[array_length] = source[input_position] - 32;
                        array_length++;
                        input_position++;
                    }
                } else {
                    codeword_array[array_length] = 106; // Latch B
                    array_length++;
                    encoding_mode = 'B';
                }
            }
            done = 1;
            if (debug) {
                printf("B4 ");
            }
        }

        /* Step C1 */
        if ((!done) && (encoding_mode == 'B')) {
            int n = try_c(source, input_position, length);

            if (n >= 2) {
                if (n <= 4) {
                    codeword_array[array_length] = 103 + (n - 2); // nx Shift C
                    array_length++;
                    for (i = 0; i < n; i++) {
                        codeword_array[array_length] = ((source[input_position] - '0') * 10) + (source[input_position + 1] - '0');
                        array_length++;
                        input_position += 2;
                    }
                } else {
                    codeword_array[array_length] = 106; // Latch C
                    array_length++;
                    encoding_mode = 'C';
                }
                done = 1;
                if (debug) {
                    printf("C1 ");
                }
            }
        }

        /* Step C2 */
        if ((!done) && (encoding_mode == 'B')) {
            if ((source[input_position] == '[') && (symbol->input_mode == GS1_MODE)) {
                codeword_array[array_length] = 107; // FNC1
                array_length++;
                input_position++;
                done = 1;
                if (debug) {
                    printf("C2/1 ");
                }
            } else {
                if (datum_b(source, input_position, length)) {

                    if ((source[input_position] >= 32) && (source[input_position] <= 127)) {
                        codeword_array[array_length] = source[input_position] - 32;
                        done = 1;

                    } else if (source[input_position] == 13) {
                        /* CR/LF */
                        codeword_array[array_length] = 96;
                        input_position++;
                        done = 1;

                    } else if (input_position != 0) {
                        /* HT, FS, GS and RS in the first data position would be interpreted as a macro (see table 2) */
                        switch(source[input_position]) {
                            case 9: // HT
                                codeword_array[array_length] = 97;
                                break;
                            case 28: // FS
                                codeword_array[array_length] = 98;
                                break;
                            case 29: // GS
                                codeword_array[array_length] = 99;
                                break;
                            case 30: // RS
                                codeword_array[array_length] = 100;
                                break;
                        }
                        done = 1;
                    }

                    if (done == 1) {
                        array_length++;
                        input_position++;
                        if (debug) {
                            printf("C2/2 ");
                        }
                    }
                }
            }
        }

        /* Step C3 */
        if ((!done) && (encoding_mode == 'B')) {
            if (binary(source, input_position)) {
                if (datum_b(source, input_position + 1, length)) {
                    if ((source[input_position] - 128) < 32) {
                        codeword_array[array_length] = 110; // Bin Shift A
                        array_length++;
                        codeword_array[array_length] = source[input_position] - 128 + 64;
                        array_length++;
                    } else {
                        codeword_array[array_length] = 111; // Bin Shift B
                        array_length++;
                        codeword_array[array_length] = source[input_position] - 128 - 32;
                        array_length++;
                    }
                    input_position++;
                } else {
                    codeword_array[array_length] = 112; // Bin Latch
                    array_length++;
                    encoding_mode = 'X';
                }
                done = 1;
                if (debug) {
                    printf("C3 ");
                }
            }
        }

        /* Step C4 */
        if ((!done) && (encoding_mode == 'B')) {
            if (ahead_a(source, input_position, length) == 1) {
                codeword_array[array_length] = 101; // Shift A
                array_length++;
                if (source[input_position] < 32) {
                    codeword_array[array_length] = source[input_position] + 64;
                } else {
                    codeword_array[array_length] = source[input_position] - 32;
                }
                array_length++;
                input_position++;
            } else {
                codeword_array[array_length] = 102; // Latch A
                array_length++;
                encoding_mode = 'A';
            }
            done = 1;
            if (debug) {
                printf("C4 ");
            }
        }

        /* Step D1 */
        if ((!done) && (encoding_mode == 'A')) {
            int n = try_c(source, input_position, length);
            if (n >= 2) {
                if (n <= 4) {
                    codeword_array[array_length] = 103 + (n - 2); // nx Shift C
                    array_length++;
                    for (i = 0; i < n; i++) {
                        codeword_array[array_length] = ((source[input_position] - '0') * 10) + (source[input_position + 1] - '0');
                        array_length++;
                        input_position += 2;
                    }
                } else {
                    codeword_array[array_length] = 106; // Latch C
                    array_length++;
                    encoding_mode = 'C';
                }
                done = 1;
                if (debug) {
                    printf("D1 ");
                }
            }
        }

        /* Step D2 */
        if ((!done) && (encoding_mode == 'A')) {
            if ((source[input_position] == '[') && (symbol->input_mode == GS1_MODE)) {
                codeword_array[array_length] = 107; // FNC1
                array_length++;
                input_position++;
                done = 1;
                if (debug) {
                    printf("D2/1 ");
                }
            } else {
                if (datum_a(source, input_position, length)) {
                    if (source[input_position] < 32) {
                        codeword_array[array_length] = source[input_position] + 64;
                    } else {
                        codeword_array[array_length] = source[input_position] - 32;
                    }
                    array_length++;
                    input_position++;
                    done = 1;
                    if (debug) {
                        printf("D2/2 ");
                    }
                }
            }
        }

        /* Step D3 */
        if ((!done) && (encoding_mode == 'A')) {
            if (binary(source, input_position)) {
                if (datum_a(source, input_position + 1, length)) {
                    if ((source[input_position] - 128) < 32) {
                        codeword_array[array_length] = 110; // Bin Shift A
                        array_length++;
                        codeword_array[array_length] = source[input_position] - 128 + 64;
                        array_length++;
                    } else {
                        codeword_array[array_length] = 111; // Bin Shift B
                        array_length++;
                        codeword_array[array_length] = source[input_position] - 128 - 32;
                        array_length++;
                    }
                    input_position++;
                } else {
                    codeword_array[array_length] = 112; // Bin Latch
                    array_length++;
                    encoding_mode = 'X';
                }
                done = 1;
                if (debug) {
                    printf("D3 ");
                }
            }
        }

        /* Step D4 */
        if ((!done) && (encoding_mode == 'A')) {
            int n = ahead_b(source, input_position, length);

            if (n <= 6) {
                codeword_array[array_length] = 95 + n; // nx Shift B
                array_length++;
                for (i = 0; i < n; i++) {
                    codeword_array[array_length] = source[input_position] - 32;
                    array_length++;
                    input_position++;
                }
            } else {
                codeword_array[array_length] = 102; // Latch B
                array_length++;
                encoding_mode = 'B';
            }
            done = 1;
            if (debug) {
                printf("D4 ");
            }
        }

        /* Step E1 */
        if ((!done) && (encoding_mode == 'X')) {
            int n = try_c(source, input_position, length);

            if (n >= 2) {
                /* Empty binary buffer */
                for (i = 0; i < (binary_buffer_size + 1); i++) {
                    lawrencium[i] = binary_buffer % 103;
                    binary_buffer /= 103;
                }

                for (i = 0; i < (binary_buffer_size + 1); i++) {
                    codeword_array[array_length] = lawrencium[binary_buffer_size - i];
                    array_length++;
                }
                binary_buffer = 0;
                binary_buffer_size = 0;

                if (n <= 7) {
                    codeword_array[array_length] = 101 + n; // Interrupt for nx Shift C
                    array_length++;
                    for (i = 0; i < n; i++) {
                        codeword_array[array_length] = ((source[input_position] - '0') * 10) + (source[input_position + 1] - '0');
                        array_length++;
                        input_position += 2;
                    }
                } else {
                    codeword_array[array_length] = 111; // Terminate with Latch to C
                    array_length++;
                    encoding_mode = 'C';
                }
                done = 1;
                if (debug) {
                    printf("E1 ");
                }
            }
        }

        /* Step E2 */
        /* Section 5.2.1.1 para D.2.i states:
         * "Groups of six codewords, each valued between 0 and 102, are radix converted from
         * base 103 into five base 259 values..."
         */
        if ((!done) && (encoding_mode == 'X')) {
            if (binary(source, input_position)
                    || binary(source, input_position + 1)
                    || binary(source, input_position + 2)
                    || binary(source, input_position + 3)) {
                binary_buffer *= 259;
                binary_buffer += source[input_position];
                binary_buffer_size++;

                if (binary_buffer_size == 5) {
                    for (i = 0; i < 6; i++) {
                        lawrencium[i] = binary_buffer % 103;
                        binary_buffer /= 103;
                    }

                    for (i = 0; i < 6; i++) {
                        codeword_array[array_length] = lawrencium[5 - i];
                        array_length++;
                    }
                    binary_buffer = 0;
                    binary_buffer_size = 0;
                }
                input_position++;
                done = 1;
                if (debug) {
                    printf("E2 ");
                }
            }
        }

        /* Step E3 */
        if ((!done) && (encoding_mode == 'X')) {
            /* Empty binary buffer */
            for (i = 0; i < (binary_buffer_size + 1); i++) {
                lawrencium[i] = binary_buffer % 103;
                binary_buffer /= 103;
            }

            for (i = 0; i < (binary_buffer_size + 1); i++) {
                codeword_array[array_length] = lawrencium[binary_buffer_size - i];
                array_length++;
            }
            binary_buffer = 0;
            binary_buffer_size = 0;

            if (ahead_a(source, input_position, length) > ahead_b(source, input_position, length)) {
                codeword_array[array_length] = 109; // Terminate with Latch to A
                encoding_mode = 'A';
            } else {
                codeword_array[array_length] = 110; // Terminate with Latch to B
                encoding_mode = 'B';
            }
            array_length++;
            done = 1;
            if (debug) {
                printf("E3 ");
            }
        }
    } while (input_position < length);

    if (encoding_mode == 'X') {
        if (binary_buffer_size != 0) {
            /* Empty binary buffer */
            for (i = 0; i < (binary_buffer_size + 1); i++) {
                lawrencium[i] = binary_buffer % 103;
                binary_buffer /= 103;
            }

            for (i = 0; i < (binary_buffer_size + 1); i++) {
                codeword_array[array_length] = lawrencium[binary_buffer_size - i];
                array_length++;
            }
        }
        *(binary_finish) = 1;
    }

    if (debug) {
        printf("\n");
    }

    return array_length;
}

/* Convert codewords to binary data stream */
static size_t make_dotstream(unsigned char masked_array[], int array_length, char dot_stream[]) {
    int i;

    dot_stream[0] = '\0';

    /* Mask value is encoded as two dots */
    bin_append(masked_array[0], 2, dot_stream);

    /* The rest of the data uses 9-bit dot patterns from Annex C */
    for (i = 1; i < array_length; i++) {
        bin_append(dot_patterns[masked_array[i]], 9, dot_stream);
    }

    return strlen(dot_stream);
}

/* Determines if a given dot is a reserved corner dot
 * to be used by one of the last six bits
 */
int is_corner(int column, int row, int width, int height) {
    int corner = 0;

    /* Top Left */
    if ((column == 0) && (row == 0)) {
        corner = 1;
    }

    /* Top Right */
    if (height % 2) {
        if (((column == width - 2) && (row == 0))
                || ((column == width - 1) && (row == 1))) {
            corner = 1;
        }
    } else {
        if ((column == width - 1) && (row == 0)) {
            corner = 1;
        }
    }

    /* Bottom Left */
    if (height % 2) {
        if ((column == 0) && (row == height - 1)) {
            corner = 1;
        }
    } else {
        if (((column == 0) && (row == height - 2))
                || ((column == 1) && (row == height - 1))) {
            corner = 1;
        }
    }

    /* Bottom Right */
    if (((column == width - 2) && (row == height - 1))
            || ((column == width - 1) && (row == height - 2))) {
        corner = 1;
    }

    return corner;
}

/* Place the dots in the symbol*/
void fold_dotstream(char dot_stream[], int width, int height, char dot_array[]) {
    int column, row;
    int input_position = 0;

    if (height % 2) {
        /* Horizontal folding */
        for (row = 0; row < height; row++) {
            for (column = 0; column < width; column++) {
                if (!((column + row) % 2)) {
                    if (is_corner(column, row, width, height)) {
                        dot_array[(row * width) + column] = 'C';
                    } else {
                        dot_array[((height - row - 1) * width) + column] = dot_stream[input_position];
                        input_position++;
                    }
                } else {
                    dot_array[((height - row - 1) * width) + column] = ' '; // Non-data position
                }
            }
        }

        /* Corners */
        dot_array[width - 2] = dot_stream[input_position];
        input_position++;
        dot_array[(height * width) - 2] = dot_stream[input_position];
        input_position++;
        dot_array[(width * 2) - 1] = dot_stream[input_position];
        input_position++;
        dot_array[((height - 1) * width) - 1] = dot_stream[input_position];
        input_position++;
        dot_array[0] = dot_stream[input_position];
        input_position++;
        dot_array[(height - 1) * width] = dot_stream[input_position];
    } else {
        /* Vertical folding */
        for (column = 0; column < width; column++) {
            for (row = 0; row < height; row++) {
                if (!((column + row) % 2)) {
                    if (is_corner(column, row, width, height)) {
                        dot_array[(row * width) + column] = 'C';
                    } else {
                        dot_array[(row * width) + column] = dot_stream[input_position];
                        input_position++;
                    }
                } else {
                    dot_array[(row * width) + column] = ' '; // Non-data position
                }
            }
        }

        /* Corners */
        dot_array[((height - 1) * width) - 1] = dot_stream[input_position];
        input_position++;
        dot_array[(height - 2) * width] = dot_stream[input_position];
        input_position++;
        dot_array[(height * width) - 2] = dot_stream[input_position];
        input_position++;
        dot_array[((height - 1) * width) + 1] = dot_stream[input_position];
        input_position++;
        dot_array[width - 1] = dot_stream[input_position];
        input_position++;
        dot_array[0] = dot_stream[input_position];
    }
}

void apply_mask(int mask, int data_length, unsigned char *masked_codeword_array, unsigned char *codeword_array, int ecc_length, char *dot_stream) {
    int weight = 0;
    int j;
    
    switch (mask) {
        case 0:
            masked_codeword_array[0] = 0;
            for (j = 0; j < data_length; j++) {
                masked_codeword_array[j + 1] = codeword_array[j];
            }
            break;
        case 1:
            masked_codeword_array[0] = 1;
            for (j = 0; j < data_length; j++) {
                masked_codeword_array[j + 1] = (weight + codeword_array[j]) % 113;
                weight += 3;
            }
            break;
        case 2:
            masked_codeword_array[0] = 2;
            for (j = 0; j < data_length; j++) {
                masked_codeword_array[j + 1] = (weight + codeword_array[j]) % 113;
                weight += 7;
            }
            break;
        case 3:
            masked_codeword_array[0] = 3;
            for (j = 0; j < data_length; j++) {
                masked_codeword_array[j + 1] = (weight + codeword_array[j]) % 113;
                weight += 17;
            }
            break;
    }

    rsencode(data_length + 1, ecc_length, masked_codeword_array);
}

void force_corners(int width, int height, char *dot_array) {
    if (width % 2) {
        // "Vertical" symbol
        dot_array[0] = '1';
        dot_array[width - 1] = '1';
        dot_array[(height - 2) * width] = '1';
        dot_array[((height - 1) * width) - 1] = '1';
        dot_array[((height - 1) * width) + 1] = '1';
        dot_array[(height * width) - 2] = '1';
    } else {
        // "Horizontal" symbol
        dot_array[0] = '1';
        dot_array[width - 2] = '1';
        dot_array[(2 * width) - 1] = '1';
        dot_array[((height - 1) * width) - 1] = '1';
        dot_array[(height - 1) * width] = '1';
        dot_array[(height * width) - 2] = '1';
    }
}

int dotcode(struct zint_symbol *symbol, const unsigned char source[], int length) {
    int i, j, k;
    size_t jc, n_dots;
    int data_length, ecc_length;
    int min_dots, min_area;
    int height, width;
    int mask_score[8];
    size_t dot_stream_length;
    int high_score, best_mask;
    int binary_finish = 0;
    int debug = symbol->debug;
    int padding_dots, is_first;
#ifdef _MSC_VER
    unsigned char* masked_codeword_array;
#endif

#ifndef _MSC_VER
    unsigned char codeword_array[length * 3];
#else
    char* dot_stream;
    char* dot_array;
    unsigned char* codeword_array = (unsigned char *) _alloca(length * 3 * sizeof (unsigned char));
#endif /* _MSC_VER */

    if (symbol->eci > 811799) {
        strcpy(symbol->errtxt, "525: Invalid ECI");
        return ZINT_ERROR_INVALID_OPTION;
    }

    data_length = dotcode_encode_message(symbol, source, length, codeword_array, &binary_finish);

    ecc_length = 3 + (data_length / 2);

    if (debug) {
        printf("Codeword length = %d, ECC length = %d\n", data_length, ecc_length);
        printf("Codewords: ");
        for (i = 0; i < data_length; i++) {
            printf("[%d] ",codeword_array[i]);
        }
        printf("\n");
    }

    min_dots = 9 * (data_length + 3 + (data_length / 2)) + 2;
    min_area = min_dots * 2;

    if (symbol->option_2 == 0) {
        /* Automatic sizing */
        /* Following Rule 3 (Section 5.2.2) and applying a recommended width to height ratio 3:2 */
        /* Eliminates under sized symbols */

        float h = (float) (sqrt(min_area * 0.666));
        float w = (float) (sqrt(min_area * 1.5));

        height = (int) h;
        width = (int) w;

        if ((width + height) % 2 == 1) {
            if ((width * height) < min_area) {
                width++;
                height++;
            }
        } else {
            if ((h * width) < (w * height)) {
                width++;
                if ((width * height) < min_area) {
                    width--;
                    height++;
                    if ((width * height) < min_area) {
                        width += 2;
                    }
                }
            } else {
                height++;
                if ((width * height) < min_area) {
                    width++;
                    height--;
                    if ((width * height) < min_area) {
                        height += 2;
                    }
                }
            }
        }

    } else {
        /* User defined width */
        /* Eliminates under sized symbols */

        width = symbol->option_2;
        height = (min_area + (width - 1)) / width;

        if (!((width + height) % 2)) {
            height++;
        }
    }

    if ((height > 200) || (width > 200)) {
        strcpy(symbol->errtxt, "526: Specified symbol size is too large");
        return ZINT_ERROR_INVALID_OPTION;
    }

    if ((height < 5) || (width < 5)) {
        strcpy(symbol->errtxt, "527: Specified symbol size has a dimension which is too small");
        return ZINT_ERROR_INVALID_OPTION;
    }

    n_dots = (height * width) / 2;

#ifndef _MSC_VER
    char dot_stream[height * width * 3];
    char dot_array[width * height * sizeof (char) ];
#else
    dot_stream = (char *) _alloca(height * width * 3);
    if (!dot_stream) return ZINT_ERROR_MEMORY;

    dot_array = (char *) _alloca(width * height * sizeof (char));
    if (!dot_array) return ZINT_ERROR_MEMORY;
#endif

    /* Add pad characters */
    padding_dots = n_dots - min_dots; /* get the number of free dots available for padding */
    is_first = 1; /* first padding character flag */

    while (padding_dots >= 9) {
        if (padding_dots < 18 && ((data_length % 2) == 0))
            padding_dots -= 9;

        else if (padding_dots >= 18) {
            if ((data_length % 2) == 0)
                padding_dots -= 9;
            else
                padding_dots -= 18;
        } else
            break; /* not enough padding dots left for padding */

        if ((is_first == 1) && (binary_finish == 1))
            codeword_array[data_length] = 109;
        else
            codeword_array[data_length] = 106;

        data_length++;
        is_first = 0;
    }

    ecc_length = 3 + (data_length / 2);

#ifndef _MSC_VER
    unsigned char masked_codeword_array[data_length + 1 + ecc_length];
#else
    masked_codeword_array = (unsigned char *) _alloca((data_length + 1 + ecc_length) * sizeof (unsigned char));
#endif /* _MSC_VER */

    /* Evaluate data mask options */
    for (i = 0; i < 4; i++) {

        apply_mask(i, data_length, masked_codeword_array, codeword_array, ecc_length, dot_stream);
        
        dot_stream_length = make_dotstream(masked_codeword_array, (data_length + ecc_length + 1), dot_stream);

        /* Add pad bits */
        for (jc = dot_stream_length; jc < n_dots; jc++) {
            strcat(dot_stream, "1");
        }

        fold_dotstream(dot_stream, width, height, dot_array);

        mask_score[i] = score_array(dot_array, height, width);

        if (debug) {
            printf("Mask %d score is %d\n", i, mask_score[i]);
        }
    }

    high_score = mask_score[0];
    best_mask = 0;

    for (i = 1; i < 4; i++) {
        if (mask_score[i] > high_score) {
            high_score = mask_score[i];
            best_mask = i;
        }
    }
    
    /* Re-evaluate using forced corners if needed */
    if (best_mask <= (height * width) / 2) {
        for (i = 0; i < 4; i++) {

            apply_mask(i, data_length, masked_codeword_array, codeword_array, ecc_length, dot_stream);
            
            dot_stream_length = make_dotstream(masked_codeword_array, (data_length + ecc_length + 1), dot_stream);

            /* Add pad bits */
            for (jc = dot_stream_length; jc < n_dots; jc++) {
                strcat(dot_stream, "1");
            }

            fold_dotstream(dot_stream, width, height, dot_array);
            
            force_corners(width, height, dot_array);

            mask_score[i + 4] = score_array(dot_array, height, width);

            if (debug) {
                printf("Mask %d score is %d\n", i + 4, mask_score[i + 4]);
            }
        }
        
        for (i = 4; i < 8; i++) {
            if (mask_score[i] > high_score) {
                high_score = mask_score[i];
                best_mask = i;
            }
        }
    }
    
    if (debug) {
        printf("Applying mask %d\n", best_mask);
    }

    /* Apply best mask */
    apply_mask(best_mask % 4, data_length, masked_codeword_array, codeword_array, ecc_length, dot_stream);
    
    dot_stream_length = make_dotstream(masked_codeword_array, (data_length + ecc_length + 1), dot_stream);

    /* Add pad bits */
    for (jc = dot_stream_length; jc < n_dots; jc++) {
        strcat(dot_stream, "1");
    }

    fold_dotstream(dot_stream, width, height, dot_array);
    
    if (best_mask >= 4) {
        force_corners(width, height, dot_array);
    }

    /* Copy values to symbol */
    symbol->width = width;
    symbol->rows = height;

    for (k = 0; k < height; k++) {
        for (j = 0; j < width; j++) {
            if (dot_array[(k * width) + j] == '1') {
                set_module(symbol, k, j);
            }
        }
        symbol->row_height[k] = 1;
    }

    if (!(symbol->output_options & BARCODE_DOTTY_MODE)) {
        symbol->output_options += BARCODE_DOTTY_MODE;
    }

    return 0;
}

