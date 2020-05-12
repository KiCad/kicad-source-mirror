/* dmatrix.c Handles Data Matrix ECC 200 symbols */

/*
    libzint - the open source barcode library
    Copyright (C) 2009-2017 Robin Stuart <rstuart114@gmail.com>

    developed from and including some functions from:
        IEC16022 bar code generation
        Adrian Kennard, Andrews & Arnold Ltd
        with help from Cliff Hones on the RS coding

        (c) 2004 Adrian Kennard, Andrews & Arnold Ltd
        (c) 2006 Stefan Schmidt <stefan@datenfreihafen.org>

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
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include <assert.h>
#include <math.h>
#ifdef _MSC_VER
#include <malloc.h>
/* ceilf (C99) not before MSVC++2013 (C++ 12.0) */
#if _MSC_VER < 1800
#define ceilf ceil
#endif
#endif
#include "reedsol.h"
#include "common.h"
#include "dmatrix.h"

/* Annex M placement alorithm low level */
static void ecc200placementbit(int *array, const int NR, const int NC, int r, int c, const int p, const char b) {
    if (r < 0) {
        r += NR;
        c += 4 - ((NR + 4) % 8);
    }
    if (c < 0) {
        c += NC;
        r += 4 - ((NC + 4) % 8);
    }
    // Necessary for 26x32,26x40,26x48,36x120,36x144,72x120,72x144
    if (r >= NR) {
#ifdef DEBUG
        fprintf(stderr, "r >= NR:%i,%i at r=%i->", p, b, r);
#endif
        r -= NR;
#ifdef DEBUG
        fprintf(stderr, "%i,c=%i\n", r, c);
#endif
    }
#ifdef DEBUG
    if (0 != array[r * NC + c]) {
        int a = array[r * NC + c];
        fprintf(stderr, "Double:%i,%i->%i,%i at r=%i,c=%i\n", a >> 3, a & 7, p, b, r, c);
        return;
    }
#endif
    // Check index limits
    assert(r < NR);
    assert(c < NC);
    // Check double-assignment
    assert(0 == array[r * NC + c]);
    array[r * NC + c] = (p << 3) + b;
}

static void ecc200placementblock(int *array, const int NR, const int NC, const int r,
        const int c, const int p) {
    ecc200placementbit(array, NR, NC, r - 2, c - 2, p, 7);
    ecc200placementbit(array, NR, NC, r - 2, c - 1, p, 6);
    ecc200placementbit(array, NR, NC, r - 1, c - 2, p, 5);
    ecc200placementbit(array, NR, NC, r - 1, c - 1, p, 4);
    ecc200placementbit(array, NR, NC, r - 1, c - 0, p, 3);
    ecc200placementbit(array, NR, NC, r - 0, c - 2, p, 2);
    ecc200placementbit(array, NR, NC, r - 0, c - 1, p, 1);
    ecc200placementbit(array, NR, NC, r - 0, c - 0, p, 0);
}

static void ecc200placementcornerA(int *array, const int NR, const int NC, const int p) {
    ecc200placementbit(array, NR, NC, NR - 1, 0, p, 7);
    ecc200placementbit(array, NR, NC, NR - 1, 1, p, 6);
    ecc200placementbit(array, NR, NC, NR - 1, 2, p, 5);
    ecc200placementbit(array, NR, NC, 0, NC - 2, p, 4);
    ecc200placementbit(array, NR, NC, 0, NC - 1, p, 3);
    ecc200placementbit(array, NR, NC, 1, NC - 1, p, 2);
    ecc200placementbit(array, NR, NC, 2, NC - 1, p, 1);
    ecc200placementbit(array, NR, NC, 3, NC - 1, p, 0);
}

static void ecc200placementcornerB(int *array, const int NR, const int NC, const int p) {
    ecc200placementbit(array, NR, NC, NR - 3, 0, p, 7);
    ecc200placementbit(array, NR, NC, NR - 2, 0, p, 6);
    ecc200placementbit(array, NR, NC, NR - 1, 0, p, 5);
    ecc200placementbit(array, NR, NC, 0, NC - 4, p, 4);
    ecc200placementbit(array, NR, NC, 0, NC - 3, p, 3);
    ecc200placementbit(array, NR, NC, 0, NC - 2, p, 2);
    ecc200placementbit(array, NR, NC, 0, NC - 1, p, 1);
    ecc200placementbit(array, NR, NC, 1, NC - 1, p, 0);
}

static void ecc200placementcornerC(int *array, const int NR, const int NC, const int p) {
    ecc200placementbit(array, NR, NC, NR - 3, 0, p, 7);
    ecc200placementbit(array, NR, NC, NR - 2, 0, p, 6);
    ecc200placementbit(array, NR, NC, NR - 1, 0, p, 5);
    ecc200placementbit(array, NR, NC, 0, NC - 2, p, 4);
    ecc200placementbit(array, NR, NC, 0, NC - 1, p, 3);
    ecc200placementbit(array, NR, NC, 1, NC - 1, p, 2);
    ecc200placementbit(array, NR, NC, 2, NC - 1, p, 1);
    ecc200placementbit(array, NR, NC, 3, NC - 1, p, 0);
}

static void ecc200placementcornerD(int *array, const int NR, const int NC, const int p) {
    ecc200placementbit(array, NR, NC, NR - 1, 0, p, 7);
    ecc200placementbit(array, NR, NC, NR - 1, NC - 1, p, 6);
    ecc200placementbit(array, NR, NC, 0, NC - 3, p, 5);
    ecc200placementbit(array, NR, NC, 0, NC - 2, p, 4);
    ecc200placementbit(array, NR, NC, 0, NC - 1, p, 3);
    ecc200placementbit(array, NR, NC, 1, NC - 3, p, 2);
    ecc200placementbit(array, NR, NC, 1, NC - 2, p, 1);
    ecc200placementbit(array, NR, NC, 1, NC - 1, p, 0);
}

/* Annex M placement alorithm main function */
static void ecc200placement(int *array, const int NR, const int NC) {
    int r, c, p;
    // invalidate
    for (r = 0; r < NR; r++)
        for (c = 0; c < NC; c++)
            array[r * NC + c] = 0;
    // start
    p = 1;
    r = 4;
    c = 0;
    do {
        // check corner
        if (r == NR && !c)
            ecc200placementcornerA(array, NR, NC, p++);
        if (r == NR - 2 && !c && NC % 4)
            ecc200placementcornerB(array, NR, NC, p++);
        if (r == NR - 2 && !c && (NC % 8) == 4)
            ecc200placementcornerC(array, NR, NC, p++);
        if (r == NR + 4 && c == 2 && !(NC % 8))
            ecc200placementcornerD(array, NR, NC, p++);
        // up/right
        do {
            if (r < NR && c >= 0 && !array[r * NC + c])
                ecc200placementblock(array, NR, NC, r, c, p++);
            r -= 2;
            c += 2;
        } while (r >= 0 && c < NC);
        r++;
        c += 3;
        // down/left
        do {
            if (r >= 0 && c < NC && !array[r * NC + c])
                ecc200placementblock(array, NR, NC, r, c, p++);
            r += 2;
            c -= 2;
        } while (r < NR && c >= 0);
        r += 3;
        c++;
    } while (r < NR || c < NC);
    // unfilled corner
    if (!array[NR * NC - 1])
        array[NR * NC - 1] = array[NR * NC - NC - 2] = 1;
}

/* calculate and append ecc code, and if necessary interleave */
static void ecc200(unsigned char *binary, const int bytes, const int datablock, const int rsblock, const int skew) {
    int blocks = (bytes + 2) / datablock, b;
    int n;
    rs_init_gf(0x12d);
    rs_init_code(rsblock, 1);
    for (b = 0; b < blocks; b++) {
        unsigned char buf[256], ecc[256];
        int p = 0;
        for (n = b; n < bytes; n += blocks)
            buf[p++] = binary[n];
        rs_encode(p, buf, ecc);
        p = rsblock - 1; // comes back reversed
        for (n = b; n < rsblock * blocks; n += blocks) {
            if (skew) {
                /* Rotate ecc data to make 144x144 size symbols acceptable */
                /* See http://groups.google.com/group/postscriptbarcode/msg/5ae8fda7757477da */
                if (b < 8) {
                    binary[bytes + n + 2] = ecc[p--];
                } else {
                    binary[bytes + n - 8] = ecc[p--];
                }
            } else {
                binary[bytes + n] = ecc[p--];
            }
        }
    }
    rs_free();
}

/* Return true (1) if a character is valid in X12 set */
static int isX12(const int source) {

    switch(source) {
        case 13: // CR
        case 42: // *
        case 62: // >
        case 32: // space
            return 1;
    }

    if ((source >= '0') && (source <= '9')) {
        return 1;
    }
    if ((source >= 'A') && (source <= 'Z')) {
        return 1;
    }

    return 0;
}

/* Insert a character into the middle of a string at position posn */
static void dminsert(char binary_string[], const int posn, const char newbit) {
    int i, end;

    end = (int) strlen(binary_string);
    for (i = end + 1; i > posn; i--) {
        binary_string[i] = binary_string[i - 1];
    }
    binary_string[posn] = newbit;
}

static void insert_value(unsigned char binary_stream[], const int posn, const int streamlen, const int newbit) {
    int i;

    for(i = (int)streamlen; i > posn; i--) {
        binary_stream[i] = binary_stream[i - 1];
    }
    binary_stream[posn] = (unsigned char) newbit;
}

static int p_r_6_2_1(const unsigned char inputData[], const size_t position, const size_t sourcelen) {
    /* Annex P section (r)(6)(ii)(I)
       "If one of the three X12 terminator/separator characters first
        occurs in the yet to be processed data before a non-X12 character..."
     */

    size_t i;
    size_t nonX12Position = 0;
    size_t specialX12Position = 0;
    int retval = 0;

    for (i = position; i < sourcelen; i++) {
        if (nonX12Position == 0) {
            if (isX12(inputData[i]) != 1) {
                nonX12Position = i;
            }
        }

        if (specialX12Position == 0) {
            if ((inputData[i] == (char) 13) ||
                    (inputData[i] == '*') ||
                    (inputData[i] == '>')) {
                specialX12Position = i;
            }
        }
    }

    if ((nonX12Position != 0) && (specialX12Position != 0)) {
        if (specialX12Position < nonX12Position) {
            retval = 1;
        }
    }

    return retval;
}

/* 'look ahead test' from Annex P */
static int look_ahead_test(const unsigned char inputData[], const size_t sourcelen, const size_t position, const int current_mode, const int gs1) {
    float ascii_count, c40_count, text_count, x12_count, edf_count, b256_count, best_count;
    const float stiction = (1.0F / 24.0F); // smallest change to act on, to get around floating point inaccuracies
    int    best_scheme;
    size_t sp;

    best_scheme = DM_NULL;

    /* step (j) */
    if (current_mode == DM_ASCII) {
        ascii_count = 0.0F;
        c40_count = 1.0F;
        text_count = 1.0F;
        x12_count = 1.0F;
        edf_count = 1.0F;
        b256_count = 1.25F;
    } else {
        ascii_count = 1.0F;
        c40_count = 2.0F;
        text_count = 2.0F;
        x12_count = 2.0F;
        edf_count = 2.0F;
        b256_count = 2.25F;
    }

    switch (current_mode) {
        case DM_C40: c40_count = 0.0F;
            break;
        case DM_TEXT: text_count = 0.0F;
            break;
        case DM_X12: x12_count = 0.0F;
            break;
        case DM_EDIFACT: edf_count = 0.0F;
            break;
        case DM_BASE256: b256_count = 0.0F;
            break;
    }

    sp = position;

    do {
        if (sp == sourcelen) {
            /* At the end of data ... step (k) */
            ascii_count = ceilf(ascii_count);
            b256_count = ceilf(b256_count);
            edf_count = ceilf(edf_count);
            text_count = ceilf(text_count);
            x12_count = ceilf(x12_count);
            c40_count = ceilf(c40_count);

            best_count = c40_count;
            best_scheme = DM_C40; // (k)(7)

            if (x12_count < (best_count - stiction)) {
                best_count = x12_count;
                best_scheme = DM_X12; // (k)(6)
            }

            if (text_count < (best_count - stiction)) {
                best_count = text_count;
                best_scheme = DM_TEXT; // (k)(5)
            }

            if (edf_count < (best_count - stiction)) {
                best_count = edf_count;
                best_scheme = DM_EDIFACT; // (k)(4)
            }

            if (b256_count < (best_count - stiction)) {
                best_count = b256_count;
                best_scheme = DM_BASE256; // (k)(3)
            }

            if (ascii_count <= (best_count + stiction)) {
                best_scheme = DM_ASCII; // (k)(2)
            }
        } else {

            /* ascii ... step (l) */
            if ((inputData[sp] >= '0') && (inputData[sp] <= '9')) {
                ascii_count += 0.5F; // (l)(1)
            } else {
                if (inputData[sp] > 127) {
                    ascii_count = ceilf(ascii_count) + 2.0F; // (l)(2)
                } else {
                    ascii_count = ceilf(ascii_count) + 1.0F; // (l)(3)
                }
            }

            /* c40 ... step (m) */
            if ((inputData[sp] == ' ') ||
                    (((inputData[sp] >= '0') && (inputData[sp] <= '9')) ||
                    ((inputData[sp] >= 'A') && (inputData[sp] <= 'Z')))) {
                c40_count += (2.0F / 3.0F); // (m)(1)
            } else {
                if (inputData[sp] > 127) {
                    c40_count += (8.0F / 3.0F); // (m)(2)
                } else {
                    c40_count += (4.0F / 3.0F); // (m)(3)
                }
            }

            /* text ... step (n) */
            if ((inputData[sp] == ' ') ||
                    (((inputData[sp] >= '0') && (inputData[sp] <= '9')) ||
                    ((inputData[sp] >= 'a') && (inputData[sp] <= 'z')))) {
                text_count += (2.0F / 3.0F); // (n)(1)
            } else {
                if (inputData[sp] > 127) {
                    text_count += (8.0F / 3.0F); // (n)(2)
                } else {
                    text_count += (4.0F / 3.0F); // (n)(3)
                }
            }

            /* x12 ... step (o) */
            if (isX12(inputData[sp])) {
                x12_count += (2.0F / 3.0F); // (o)(1)
            } else {
                if (inputData[sp] > 127) {
                    x12_count += (13.0F / 3.0F); // (o)(2)
                } else {
                    x12_count += (10.0F / 3.0F); // (o)(3)
                }
            }

            /* edifact ... step (p) */
            if ((inputData[sp] >= ' ') && (inputData[sp] <= '^')) {
                edf_count += (3.0F / 4.0F); // (p)(1)
            } else {
                if (inputData[sp] > 127) {
                    edf_count += 17.0F; // (p)(2) > Value changed from ISO
                } else {
                    edf_count += 13.0F; // (p)(3) > Value changed from ISO
                }
            }
            if ((gs1 == 1) && (inputData[sp] == '[')) {
                edf_count += 13.0F; //  > Value changed from ISO
            }

            /* base 256 ... step (q) */
            if ((gs1 == 1) && (inputData[sp] == '[')) {
                b256_count += 4.0F; // (q)(1)
            } else {
                b256_count += 1.0F; // (q)(2)
            }
        }


        if (sp > (position + 3)) {
            /* 4 data characters processed ... step (r) */

            /* step (r)(6) */
            if (((c40_count + 1.0F) < (ascii_count - stiction)) &&
                    ((c40_count + 1.0F) < (b256_count - stiction)) &&
                    ((c40_count + 1.0F) < (edf_count - stiction)) &&
                    ((c40_count + 1.0F) < (text_count - stiction))) {

                if (c40_count < (x12_count - stiction)) {
                    best_scheme = DM_C40;
                }

                if ((c40_count >= (x12_count - stiction))
                        && (c40_count <= (x12_count + stiction))) {
                    if (p_r_6_2_1(inputData, sp, sourcelen) == 1) {
                        // Test (r)(6)(ii)(i)
                        best_scheme = DM_X12;
                    } else {
                        best_scheme = DM_C40;
                    }
                }
            }

            /* step (r)(5) */
            if (((x12_count + 1.0F) < (ascii_count - stiction)) &&
                    ((x12_count + 1.0F) < (b256_count - stiction)) &&
                    ((x12_count + 1.0F) < (edf_count - stiction)) &&
                    ((x12_count + 1.0F) < (text_count - stiction)) &&
                    ((x12_count + 1.0F) < (c40_count - stiction))) {
                best_scheme = DM_X12;
            }

            /* step (r)(4) */
            if (((text_count + 1.0F) < (ascii_count - stiction)) &&
                    ((text_count + 1.0F) < (b256_count - stiction)) &&
                    ((text_count + 1.0F) < (edf_count - stiction)) &&
                    ((text_count + 1.0F) < (x12_count - stiction)) &&
                    ((text_count + 1.0F) < (c40_count - stiction))) {
                best_scheme = DM_TEXT;
            }

            /* step (r)(3) */
            if (((edf_count + 1.0F) < (ascii_count - stiction)) &&
                    ((edf_count + 1.0F) < (b256_count - stiction)) &&
                    ((edf_count + 1.0F) < (text_count - stiction)) &&
                    ((edf_count + 1.0F) < (x12_count - stiction)) &&
                    ((edf_count + 1.0F) < (c40_count - stiction))) {
                best_scheme = DM_EDIFACT;
            }

            /* step (r)(2) */
            if (((b256_count + 1.0F) <= (ascii_count + stiction)) ||
                    (((b256_count + 1.0F) < (edf_count - stiction)) &&
                    ((b256_count + 1.0F) < (text_count - stiction)) &&
                    ((b256_count + 1.0F) < (x12_count - stiction)) &&
                    ((b256_count + 1.0F) < (c40_count - stiction)))) {
                best_scheme = DM_BASE256;
            }

            /* step (r)(1) */
            if (((ascii_count + 1.0F) <= (b256_count + stiction)) &&
                    ((ascii_count + 1.0F) <= (edf_count + stiction)) &&
                    ((ascii_count + 1.0F) <= (text_count + stiction)) &&
                    ((ascii_count + 1.0F) <= (x12_count + stiction)) &&
                    ((ascii_count + 1.0F) <= (c40_count + stiction))) {
                best_scheme = DM_ASCII;
            }
        }

        sp++;
    } while (best_scheme == DM_NULL); // step (s)

    return best_scheme;
}

/* Encodes data using ASCII, C40, Text, X12, EDIFACT or Base 256 modes as appropriate
   Supports encoding FNC1 in supporting systems */
static int dm200encode(struct zint_symbol *symbol, const unsigned char source[], unsigned char target[], int *last_mode, size_t *length_p, int process_buffer[], int *process_p) {

    size_t sp;
    int tp, i, gs1;
    int current_mode, next_mode;
    size_t inputlen = *length_p;
    int debug = symbol->debug;
#ifndef _MSC_VER
    char binary[2 * inputlen];
#else
    char* binary = (char*) _alloca(2 * inputlen);
#endif

    sp = 0;
    tp = 0;
    memset(process_buffer, 0, 8);
    *process_p = 0;
    strcpy(binary, "");

    /* step (a) */
    current_mode = DM_ASCII;
    next_mode = DM_ASCII;

    if (symbol->input_mode == GS1_MODE) {
        gs1 = 1;
    } else {
        gs1 = 0;
    }

    if (gs1) {
        target[tp] = 232;
        tp++;
        strcat(binary, " ");
        if (debug) printf("FN1 ");
    } /* FNC1 */

    if (symbol->output_options & READER_INIT) {
        if (gs1) {
            strcpy(symbol->errtxt, "519: Cannot encode in GS1 mode and Reader Initialisation at the same time");
            return ZINT_ERROR_INVALID_OPTION;
        } else {
            target[tp] = 234;
            tp++; /* Reader Programming */
            strcat(binary, " ");
            if (debug) printf("RP ");
        }
    }

    if (symbol->eci > 3) {
        /* Encode ECI numbers according to Table 6 */
        target[tp] = 241; /* ECI Character */
        tp++;
        if (symbol->eci <= 126) {
            target[tp] = (unsigned char) symbol->eci + 1;
            tp++;
            strcat(binary, " ");
        }
        if ((symbol->eci >= 127) && (symbol->eci <= 16382)) {
            target[tp] = (unsigned char) ((symbol->eci - 127) / 254) + 128;
            tp++;
            target[tp] = (unsigned char) ((symbol->eci - 127) % 254) + 1;
            tp++;
            strcat(binary, "  ");
        }
        if (symbol->eci >= 16383) {
            target[tp] = (unsigned char) ((symbol->eci - 16383) / 64516) + 192;
            tp++;
            target[tp] = (unsigned char) (((symbol->eci - 16383) / 254) % 254) + 1;
            tp++;
            target[tp] = (unsigned char) ((symbol->eci - 16383) % 254) + 1;
            tp++;
            strcat(binary, "   ");
        }
        if (debug) printf("ECI %d ", symbol->eci + 1);
    }

    /* Check for Macro05/Macro06 */
    /* "[)>[RS]05[GS]...[RS][EOT]" -> CW 236 */
    /* "[)>[RS]06[GS]...[RS][EOT]" -> CW 237 */
    if (tp == 0 && sp == 0 && inputlen >= 9
            && source[0] == '[' && source[1] == ')' && source[2] == '>'
            && source[3] == '\x1e' && source[4] == '0'
            && (source[5] == '5' || source[5] == '6')
            && source[6] == '\x1d'
            && source[inputlen - 2] == '\x1e' && source[inputlen - 1] == '\x04') {
        /* Output macro Codeword */
        if (source[5] == '5') {
            target[tp] = 236;
            if (debug) printf("Macro05 ");
        } else {
            target[tp] = 237;
            if (debug) printf("Macro06 ");
        }
        tp++;
        strcat(binary, " ");
        /* Remove macro characters from input string */
        sp = 7;
        inputlen -= 2;
        *length_p -= 2;
    }


    while (sp < inputlen) {

        current_mode = next_mode;

        /* step (b) - ASCII encodation */
        if (current_mode == DM_ASCII) {
            next_mode = DM_ASCII;

            if (istwodigits(source, sp) && ((sp + 1) != inputlen)) {
                target[tp] = (unsigned char) ((10 * ctoi(source[sp])) + ctoi(source[sp + 1]) + 130);
                if (debug) printf("N%d ", target[tp] - 130);
                tp++;
                strcat(binary, " ");
                sp += 2;
            } else {
                next_mode = look_ahead_test(source, inputlen, sp, current_mode, gs1);

                if (next_mode != DM_ASCII) {
                    switch (next_mode) {
                        case DM_C40: target[tp] = 230;
                            tp++;
                            strcat(binary, " ");
                            if (debug) printf("C40 ");
                            break;
                        case DM_TEXT: target[tp] = 239;
                            tp++;
                            strcat(binary, " ");
                            if (debug) printf("TEX ");
                            break;
                        case DM_X12: target[tp] = 238;
                            tp++;
                            strcat(binary, " ");
                            if (debug) printf("X12 ");
                            break;
                        case DM_EDIFACT: target[tp] = 240;
                            tp++;
                            strcat(binary, " ");
                            if (debug) printf("EDI ");
                            break;
                        case DM_BASE256: target[tp] = 231;
                            tp++;
                            strcat(binary, " ");
                            if (debug) printf("BAS ");
                            break;
                    }
                } else {
                    if (source[sp] > 127) {
                        target[tp] = 235; /* FNC4 */
                        if (debug) printf("FN4 ");
                        tp++;
                        target[tp] = (source[sp] - 128) + 1;
                        if (debug) printf("A%02X ", target[tp] - 1);
                        tp++;
                        strcat(binary, "  ");
                    } else {
                        if (gs1 && (source[sp] == '[')) {
                            target[tp] = 232; /* FNC1 */
                            if (debug) printf("FN1 ");
                        } else {
                            target[tp] = source[sp] + 1;
                            if (debug) printf("A%02X ", target[tp] - 1);
                        }
                        tp++;
                        strcat(binary, " ");
                    }
                    sp++;
                }
            }

        }

        /* step (c) C40 encodation */
        if (current_mode == DM_C40) {

            next_mode = DM_C40;
            if (*process_p == 0) {
                next_mode = look_ahead_test(source, inputlen, sp, current_mode, gs1);
            }

            if (next_mode != DM_C40) {
                target[tp] = 254;
                tp++;
                strcat(binary, " "); /* Unlatch */
                next_mode = DM_ASCII;
                if (debug) printf("ASC ");
            } else {
                int shift_set, value;
                if (source[sp] > 127) {
                    process_buffer[*process_p] = 1;
                    (*process_p)++;
                    process_buffer[*process_p] = 30;
                    (*process_p)++; /* Upper Shift */
                    shift_set = c40_shift[source[sp] - 128];
                    value = c40_value[source[sp] - 128];
                } else {
                    shift_set = c40_shift[source[sp]];
                    value = c40_value[source[sp]];
                }

                if (gs1 && (source[sp] == '[')) {
                    shift_set = 2;
                    value = 27; /* FNC1 */
                }

                if (shift_set != 0) {
                    process_buffer[*process_p] = shift_set - 1;
                    (*process_p)++;
                }
                process_buffer[*process_p] = value;
                (*process_p)++;

                if (*process_p >= 3) {
                    int iv;

                    iv = (1600 * process_buffer[0]) + (40 * process_buffer[1]) + (process_buffer[2]) + 1;
                    target[tp] = (unsigned char) (iv / 256);
                    tp++;
                    target[tp] = iv % 256;
                    tp++;
                    strcat(binary, "  ");
                    if (debug) printf("[%d %d %d] ", process_buffer[0], process_buffer[1], process_buffer[2]);

                    process_buffer[0] = process_buffer[3];
                    process_buffer[1] = process_buffer[4];
                    process_buffer[2] = process_buffer[5];
                    process_buffer[3] = 0;
                    process_buffer[4] = 0;
                    process_buffer[5] = 0;
                    *process_p -= 3;
                }
                sp++;
            }
        }

        /* step (d) Text encodation */
        if (current_mode == DM_TEXT) {

            next_mode = DM_TEXT;
            if (*process_p == 0) {
                next_mode = look_ahead_test(source, inputlen, sp, current_mode, gs1);
            }

            if (next_mode != DM_TEXT) {
                target[tp] = 254;
                tp++;
                strcat(binary, " "); /* Unlatch */
                next_mode = DM_ASCII;
                if (debug) printf("ASC ");
            } else {
                int shift_set, value;
                if (source[sp] > 127) {
                    process_buffer[*process_p] = 1;
                    (*process_p)++;
                    process_buffer[*process_p] = 30;
                    (*process_p)++; /* Upper Shift */
                    shift_set = text_shift[source[sp] - 128];
                    value = text_value[source[sp] - 128];
                } else {
                    shift_set = text_shift[source[sp]];
                    value = text_value[source[sp]];
                }

                if (gs1 && (source[sp] == '[')) {
                    shift_set = 2;
                    value = 27; /* FNC1 */
                }

                if (shift_set != 0) {
                    process_buffer[*process_p] = shift_set - 1;
                    (*process_p)++;
                }
                process_buffer[*process_p] = value;
                (*process_p)++;

                if (*process_p >= 3) {
                    int iv;

                    iv = (1600 * process_buffer[0]) + (40 * process_buffer[1]) + (process_buffer[2]) + 1;
                    target[tp] = (unsigned char) (iv / 256);
                    tp++;
                    target[tp] = iv % 256;
                    tp++;
                    strcat(binary, "  ");
                    if (debug) printf("[%d %d %d] ", process_buffer[0], process_buffer[1], process_buffer[2]);

                    process_buffer[0] = process_buffer[3];
                    process_buffer[1] = process_buffer[4];
                    process_buffer[2] = process_buffer[5];
                    process_buffer[3] = 0;
                    process_buffer[4] = 0;
                    process_buffer[5] = 0;
                    *process_p -= 3;
                }
                sp++;
            }
        }

        /* step (e) X12 encodation */
        if (current_mode == DM_X12) {

            next_mode = DM_X12;
            if (*process_p == 0) {
                next_mode = look_ahead_test(source, inputlen, sp, current_mode, gs1);
            }

            if (next_mode != DM_X12) {
                target[tp] = 254;
                tp++;
                strcat(binary, " "); /* Unlatch */
                next_mode = DM_ASCII;
                if (debug) printf("ASC ");
            } else {
                int value = 0;
                if (source[sp] == 13) {
                    value = 0;
                }
                if (source[sp] == '*') {
                    value = 1;
                }
                if (source[sp] == '>') {
                    value = 2;
                }
                if (source[sp] == ' ') {
                    value = 3;
                }
                if ((source[sp] >= '0') && (source[sp] <= '9')) {
                    value = (source[sp] - '0') + 4;
                }
                if ((source[sp] >= 'A') && (source[sp] <= 'Z')) {
                    value = (source[sp] - 'A') + 14;
                }

                process_buffer[*process_p] = value;
                (*process_p)++;

                if (*process_p >= 3) {
                    int iv;

                    iv = (1600 * process_buffer[0]) + (40 * process_buffer[1]) + (process_buffer[2]) + 1;
                    target[tp] = (unsigned char) (iv / 256);
                    tp++;
                    target[tp] = iv % 256;
                    tp++;
                    strcat(binary, "  ");
                    if (debug) printf("[%d %d %d] ", process_buffer[0], process_buffer[1], process_buffer[2]);

                    process_buffer[0] = process_buffer[3];
                    process_buffer[1] = process_buffer[4];
                    process_buffer[2] = process_buffer[5];
                    process_buffer[3] = 0;
                    process_buffer[4] = 0;
                    process_buffer[5] = 0;
                    *process_p -= 3;
                }
                sp++;
            }
        }

        /* step (f) EDIFACT encodation */
        if (current_mode == DM_EDIFACT) {

            next_mode = DM_EDIFACT;
            if (*process_p == 3) {
                next_mode = look_ahead_test(source, inputlen, sp, current_mode, gs1);
            }

            if (next_mode != DM_EDIFACT) {
                process_buffer[*process_p] = 31;
                (*process_p)++;
                next_mode = DM_ASCII;
            } else {
                int value = source[sp];

                if (source[sp] >= 64) {  // '@'
                    value -= 64;
                }

                process_buffer[*process_p] = value;
                (*process_p)++;
                sp++;
            }

            if (*process_p >= 4) {
                target[tp] = (unsigned char) ((process_buffer[0] << 2) + ((process_buffer[1] & 0x30) >> 4));
                tp++;
                target[tp] = ((process_buffer[1] & 0x0f) << 4) + ((process_buffer[2] & 0x3c) >> 2);
                tp++;
                target[tp] = (unsigned char) (((process_buffer[2] & 0x03) << 6) + process_buffer[3]);
                tp++;
                strcat(binary, "   ");
                if (debug) printf("[%d %d %d %d] ", process_buffer[0], process_buffer[1], process_buffer[2], process_buffer[3]);

                process_buffer[0] = process_buffer[4];
                process_buffer[1] = process_buffer[5];
                process_buffer[2] = process_buffer[6];
                process_buffer[3] = process_buffer[7];
                process_buffer[4] = 0;
                process_buffer[5] = 0;
                process_buffer[6] = 0;
                process_buffer[7] = 0;
                *process_p -= 4;
            }
        }

        /* step (g) Base 256 encodation */
        if (current_mode == DM_BASE256) {
            next_mode = look_ahead_test(source, inputlen, sp, current_mode, gs1);

            if (next_mode == DM_BASE256) {
                target[tp] = source[sp];
                if (debug) printf("B%02X ", target[tp]);
                tp++;
                sp++;
                strcat(binary, "b");
            } else {
                next_mode = DM_ASCII;
                if (debug) printf("ASC ");
            }
        }

        if (tp > 1558) {
            return 0;
        }

    } /* while */

    /* Add length and randomising algorithm to b256 */
    i = 0;
    while (i < tp) {
        if (binary[i] == 'b') {
            if ((i == 0) || (binary[i - 1] != 'b')) {
                /* start of binary data */
                int binary_count; /* length of b256 data */

                for (binary_count = 0; binary_count + i < tp && binary[binary_count + i] == 'b'; binary_count++);

                if (binary_count <= 249) {
                    dminsert(binary, i, 'b');
                    insert_value(target, i, tp, binary_count);
                    tp++;
                } else {
                    dminsert(binary, i, 'b');
                    dminsert(binary, i + 1, 'b');
                    insert_value(target, i, tp, (binary_count / 250) + 249);
                    tp++;
                    insert_value(target, i + 1, tp, binary_count % 250);
                    tp++;
                }
            }
        }
        i++;
    }

    for (i = 0; i < tp; i++) {
        if (binary[i] == 'b') {
            int prn, temp;

            prn = ((149 * (i + 1)) % 255) + 1;
            temp = target[i] + prn;
            if (temp <= 255) {
                target[i] = (unsigned char) (temp);
            } else {
                target[i] = (unsigned char) (temp - 256);
            }
        }
    }

    *(last_mode) = current_mode;
    return tp;
}

static int dm200encode_remainder(unsigned char target[], int target_length, const unsigned char source[], const size_t inputlen, const int last_mode, const int process_buffer[], const int process_p, const int symbols_left) {
    int debug = 0;

    switch (last_mode) {
        case DM_C40:
        case DM_TEXT:
            if (process_p == 1) // 1 data character left to encode.
            {
                if (symbols_left > 1) {
                    target[target_length] = 254;
                    target_length++; // Unlatch and encode remaining data in ascii.
                }
                target[target_length] = source[inputlen - 1] + 1;
                target_length++;
            } else if (process_p == 2) // 2 data characters left to encode.
            {
                // Pad with shift 1 value (0) and encode as double.
                int intValue = (1600 * process_buffer[0]) + (40 * process_buffer[1]) + 1; // ie (0 + 1).
                target[target_length] = (unsigned char) (intValue / 256);
                target_length++;
                target[target_length] = (unsigned char) (intValue % 256);
                target_length++;
                if (symbols_left > 2) {
                    target[target_length] = 254; // Unlatch
                    target_length++;
                }
            } else {
                if (symbols_left > 0) {
                    target[target_length] = 254; // Unlatch
                    target_length++;
                }
            }
            break;

        case DM_X12:
            if ((symbols_left == process_p) && (process_p == 1)) {
                // Unlatch not required!
                target[target_length] = source[inputlen - 1] + 1;
                target_length++;
            } else {
                target[target_length] = (254);
                target_length++; // Unlatch.

                if (process_p == 1) {
                    target[target_length] = source[inputlen - 1] + 1;
                    target_length++;
                }

                if (process_p == 2) {
                    target[target_length] = source[inputlen - 2] + 1;
                    target_length++;
                    target[target_length] = source[inputlen - 1] + 1;
                    target_length++;
                }
            }
            break;

        case DM_EDIFACT:
            if (symbols_left <= 2) // Unlatch not required!
            {
                if (process_p == 1) {
                    target[target_length] = source[inputlen - 1] + 1;
                    target_length++;
                }

                if (process_p == 2) {
                    target[target_length] = source[inputlen - 2] + 1;
                    target_length++;
                    target[target_length] = source[inputlen - 1] + 1;
                    target_length++;
                }
            } else {
                // Append edifact unlatch value (31) and empty buffer
                if (process_p == 0) {
                    target[target_length] = (unsigned char) (31 << 2);
                    target_length++;
                }

                if (process_p == 1) {
                    target[target_length] = (unsigned char) ((process_buffer[0] << 2) + ((31 & 0x30) >> 4));
                    target_length++;
                    target[target_length] = (unsigned char) ((31 & 0x0f) << 4);
                    target_length++;
                }

                if (process_p == 2) {
                    target[target_length] = (unsigned char) ((process_buffer[0] << 2) + ((process_buffer[1] & 0x30) >> 4));
                    target_length++;
                    target[target_length] = (unsigned char) (((process_buffer[1] & 0x0f) << 4) + ((31 & 0x3c) >> 2));
                    target_length++;
                    target[target_length] = (unsigned char) (((31 & 0x03) << 6));
                    target_length++;
                }

                if (process_p == 3) {
                    target[target_length] = (unsigned char) ((process_buffer[0] << 2) + ((process_buffer[1] & 0x30) >> 4));
                    target_length++;
                    target[target_length] = (unsigned char) (((process_buffer[1] & 0x0f) << 4) + ((process_buffer[2] & 0x3c) >> 2));
                    target_length++;
                    target[target_length] = (unsigned char) (((process_buffer[2] & 0x03) << 6) + 31);
                    target_length++;
                }
            }
            break;
    }

    if (debug) {
        int i;
        printf("\n\n");
        for (i = 0; i < target_length; i++)
            printf("%03d ", target[i]);

        printf("\n");
    }

    return target_length;
}

/* add pad bits */
static void add_tail(unsigned char target[], int tp, const int tail_length) {
    int i, prn, temp;

    for (i = tail_length; i > 0; i--) {
        if (i == tail_length) {
            target[tp] = 129;
            tp++; /* Pad */
        } else {
            prn = ((149 * (tp + 1)) % 253) + 1;
            temp = 129 + prn;
            if (temp <= 254) {
                target[tp] = (unsigned char) (temp);
                tp++;
            } else {
                target[tp] = (unsigned char) (temp - 254);
                tp++;
            }
        }
    }
}

int data_matrix_200(struct zint_symbol *symbol,const unsigned char source[], const size_t in_length) {
	int i, skew = 0;
   size_t inputlen=in_length;
    unsigned char binary[2200];
    int binlen;
    int process_buffer[8]; /* holds remaining data to finalised */
    int process_p; /* number of characters left to finalise */
    int symbolsize, optionsize, calcsize;
    int taillength, error_number = 0;
    int H, W, FH, FW, datablock, bytes, rsblock;
    int last_mode = DM_ASCII;
    int symbols_left;

    /* inputlen may be decremented by 2 if macro character is used */
    binlen = dm200encode(symbol, source, binary, &last_mode, &inputlen, process_buffer, &process_p);

    if (binlen == 0) {
        strcpy(symbol->errtxt, "520: Data too long to fit in symbol");
        return ZINT_ERROR_TOO_LONG;
    }

    if ((symbol->option_2 >= 1) && (symbol->option_2 <= DMSIZESCOUNT)) {
        optionsize = intsymbol[symbol->option_2 - 1];
    } else {
        optionsize = -1;
    }

    calcsize = DMSIZESCOUNT - 1;
    for (i = DMSIZESCOUNT - 1; i > -1; i--) {
        if (matrixbytes[i] >= (binlen + process_p)) {
            // Allow for the remaining data characters
            calcsize = i;
        }
    }

    if (optionsize == -1) {
        // We are in automatic size mode as the exact symbol size was not given
        // Now check the detailed search options square only or no dmre
        if (symbol->option_3 == DM_SQUARE) {
            /* Skip rectangular symbols in square only mode */
            while (matrixH[calcsize] != matrixW[calcsize]) {
                calcsize++;
            }
        } else if (symbol->option_3 != DM_DMRE) {
            /* Skip DMRE symbols in no dmre mode */
            while (isDMRE[calcsize]) {
                calcsize++;
            }
        }
        symbolsize = calcsize;
    } else {
        // The symbol size was given by --ver (option_2)
        // Thus check if the data fits into this symbol size and use this size
        if (calcsize > optionsize) {
            strcpy(symbol->errtxt, "522: Input too long for selected symbol size");
            return ZINT_ERROR_TOO_LONG;
        }
        symbolsize = optionsize;
    }

    // Now we know the symbol size we can handle the remaining data in the process buffer.
    symbols_left = matrixbytes[symbolsize] - binlen;
    binlen = dm200encode_remainder(binary, binlen, source, inputlen, last_mode, process_buffer, process_p, symbols_left);

    if (binlen > matrixbytes[symbolsize]) {
        strcpy(symbol->errtxt, "523: Data too long to fit in symbol");
        return ZINT_ERROR_TOO_LONG;
    }

    H = matrixH[symbolsize];
    W = matrixW[symbolsize];
    FH = matrixFH[symbolsize];
    FW = matrixFW[symbolsize];
    bytes = matrixbytes[symbolsize];
    datablock = matrixdatablock[symbolsize];
    rsblock = matrixrsblock[symbolsize];

    taillength = bytes - binlen;

    if (taillength != 0) {
        add_tail(binary, binlen, taillength);
    }

    // ecc code
    if (symbolsize == INTSYMBOL144) {
        skew = 1;
    }
    ecc200(binary, bytes, datablock, rsblock, skew);
    // Print Codewords
#ifdef DEBUG
    {
        int CWCount;
		int posCur;
        if (skew)
            CWCount = 1558 + 620;
        else
            CWCount = bytes + rsblock * (bytes / datablock);
        printf("Codewords (%i):", CWCount);
        for (posCur = 0; posCur < CWCount; posCur++)
            printf(" %3i", binary[posCur]);
        puts("\n");
    }
#endif
    { // placement
        int x, y, NC, NR, *places;
        unsigned char *grid;
        NC = W - 2 * (W / FW);
        NR = H - 2 * (H / FH);
        places = (int*) malloc(NC * NR * sizeof (int));
        ecc200placement(places, NR, NC);
        grid = (unsigned char*) malloc(W * H);
        memset(grid, 0, W * H);
        for (y = 0; y < H; y += FH) {
            for (x = 0; x < W; x++)
                grid[y * W + x] = 1;
            for (x = 0; x < W; x += 2)
                grid[(y + FH - 1) * W + x] = 1;
        }
        for (x = 0; x < W; x += FW) {
            for (y = 0; y < H; y++)
                grid[y * W + x] = 1;
            for (y = 0; y < H; y += 2)
                grid[y * W + x + FW - 1] = 1;
        }
#ifdef DEBUG
        // Print position matrix as in standard
        for (y = NR - 1; y >= 0; y--) {
            for (x = 0; x < NC; x++) {
				int v;
                if (x != 0)
                    fprintf(stderr, "|");
                v = places[(NR - y - 1) * NC + x];
                fprintf(stderr, "%3d.%2d", (v >> 3), 8 - (v & 7));
            }
            fprintf(stderr, "\n");
        }
#endif
        for (y = 0; y < NR; y++) {
            for (x = 0; x < NC; x++) {
                int v = places[(NR - y - 1) * NC + x];
                //fprintf (stderr, "%4d", v);
                if (v == 1 || (v > 7 && (binary[(v >> 3) - 1] & (1 << (v & 7)))))
                    grid[(1 + y + 2 * (y / (FH - 2))) * W + 1 + x + 2 * (x / (FW - 2))] = 1;
            }
            //fprintf (stderr, "\n");
        }
        for (y = H - 1; y >= 0; y--) {
            int x;
            for (x = 0; x < W; x++) {
                if (grid[W * y + x]) {
                    set_module(symbol, (H - y) - 1, x);
                }
            }
            symbol->row_height[(H - y) - 1] = 1;
        }
        free(grid);
        free(places);
    }

    symbol->rows = H;
    symbol->width = W;

    return error_number;
}

int dmatrix(struct zint_symbol *symbol, const unsigned char source[], const size_t in_length) {
    int error_number;

    if (symbol->option_1 <= 1) {
        /* ECC 200 */
        error_number = data_matrix_200(symbol, source, in_length);
    } else {
        /* ECC 000 - 140 */
        strcpy(symbol->errtxt, "524: Older Data Matrix standards are no longer supported");
        error_number = ZINT_ERROR_INVALID_OPTION;
    }

    return error_number;
}


