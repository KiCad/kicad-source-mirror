/* code.c - Handles Code 39, 39+, 93 and VIN */
/*
    libzint - the open source barcode library
    Copyright (C) 2008-2025 Robin Stuart <rstuart114@gmail.com>

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
/* SPDX-License-Identifier: BSD-3-Clause */

/* In version 0.5 this file was 1,553 lines long! */

#include <assert.h>
#include <stdio.h>
#include "common.h"

/* Same as TECHNETIUM (HIBC) with "abcd" added for CODE93 */
static const char SILVER[] = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ-. $/+%abcd";

#define ARSENIC_F       (IS_NUM_F | IS_ARS_F) /* ARSENIC "0123456789ABCDEFGHJKLMNPRSTUVWXYZ" */

/* Code 39 character assignments (ISO/IEC 16388:2007 Table 1 and Table A.1) */
static const char C39Table[43 + 1][10] = {
    {'1','1','1','2','2','1','2','1','1','1'}, {'2','1','1','2','1','1','1','1','2','1'},
    {'1','1','2','2','1','1','1','1','2','1'}, {'2','1','2','2','1','1','1','1','1','1'},
    {'1','1','1','2','2','1','1','1','2','1'}, {'2','1','1','2','2','1','1','1','1','1'},
    {'1','1','2','2','2','1','1','1','1','1'}, {'1','1','1','2','1','1','2','1','2','1'},
    {'2','1','1','2','1','1','2','1','1','1'}, {'1','1','2','2','1','1','2','1','1','1'},
    {'2','1','1','1','1','2','1','1','2','1'}, {'1','1','2','1','1','2','1','1','2','1'},
    {'2','1','2','1','1','2','1','1','1','1'}, {'1','1','1','1','2','2','1','1','2','1'},
    {'2','1','1','1','2','2','1','1','1','1'}, {'1','1','2','1','2','2','1','1','1','1'},
    {'1','1','1','1','1','2','2','1','2','1'}, {'2','1','1','1','1','2','2','1','1','1'},
    {'1','1','2','1','1','2','2','1','1','1'}, {'1','1','1','1','2','2','2','1','1','1'},
    {'2','1','1','1','1','1','1','2','2','1'}, {'1','1','2','1','1','1','1','2','2','1'},
    {'2','1','2','1','1','1','1','2','1','1'}, {'1','1','1','1','2','1','1','2','2','1'},
    {'2','1','1','1','2','1','1','2','1','1'}, {'1','1','2','1','2','1','1','2','1','1'},
    {'1','1','1','1','1','1','2','2','2','1'}, {'2','1','1','1','1','1','2','2','1','1'},
    {'1','1','2','1','1','1','2','2','1','1'}, {'1','1','1','1','2','1','2','2','1','1'},
    {'2','2','1','1','1','1','1','1','2','1'}, {'1','2','2','1','1','1','1','1','2','1'},
    {'2','2','2','1','1','1','1','1','1','1'}, {'1','2','1','1','2','1','1','1','2','1'},
    {'2','2','1','1','2','1','1','1','1','1'}, {'1','2','2','1','2','1','1','1','1','1'},
    {'1','2','1','1','1','1','2','1','2','1'}, {'2','2','1','1','1','1','2','1','1','1'},
    {'1','2','2','1','1','1','2','1','1','1'}, {'1','2','1','2','1','2','1','1','1','1'},
    {'1','2','1','2','1','1','1','2','1','1'}, {'1','2','1','1','1','2','1','2','1','1'},
    {'1','1','1','2','1','2','1','2','1','1'},
    {'1','2','1','1','2','1','2','1','1','1'} /* Start character (full 10), Stop character (first 9) */
};

/* Encoding the full ASCII character set in Code 39 (ISO/IEC 16388:2007 Table A.2) */
static const char EC39Ctrl[128][2] = {
    {'%','U'}, {'$','A'}, {'$','B'}, {'$','C'}, {'$','D'}, {'$','E'}, {'$','F'}, {'$','G'}, {'$','H'}, {'$','I'},
    {'$','J'}, {'$','K'}, {'$','L'}, {'$','M'}, {'$','N'}, {'$','O'}, {'$','P'}, {'$','Q'}, {'$','R'}, {'$','S'},
    {'$','T'}, {'$','U'}, {'$','V'}, {'$','W'}, {'$','X'}, {'$','Y'}, {'$','Z'}, {'%','A'}, {'%','B'}, {'%','C'},
    {'%','D'}, {'%','E'}, {  " "  }, {'/','A'}, {'/','B'}, {'/','C'}, {'/','D'}, {'/','E'}, {'/','F'}, {'/','G'},
    {'/','H'}, {'/','I'}, {'/','J'}, {'/','K'}, {'/','L'}, {  "-"  }, {  "."  }, {'/','O'}, {  "0"  }, {  "1"  },
    {  "2"  }, {  "3"  }, {  "4"  }, {  "5"  }, {  "6"  }, {  "7"  }, {  "8"  }, {  "9"  }, {'/','Z'}, {'%','F'},
    {'%','G'}, {'%','H'}, {'%','I'}, {'%','J'}, {'%','V'}, {  "A"  }, {  "B"  }, {  "C"  }, {  "D"  }, {  "E"  },
    {  "F"  }, {  "G"  }, {  "H"  }, {  "I"  }, {  "J"  }, {  "K"  }, {  "L"  }, {  "M"  }, {  "N"  }, {  "O"  },
    {  "P"  }, {  "Q"  }, {  "R"  }, {  "S"  }, {  "T"  }, {  "U"  }, {  "V"  }, {  "W"  }, {  "X"  }, {  "Y"  },
    {  "Z"  }, {'%','K'}, {'%','L'}, {'%','M'}, {'%','N'}, {'%','O'}, {'%','W'}, {'+','A'}, {'+','B'}, {'+','C'},
    {'+','D'}, {'+','E'}, {'+','F'}, {'+','G'}, {'+','H'}, {'+','I'}, {'+','J'}, {'+','K'}, {'+','L'}, {'+','M'},
    {'+','N'}, {'+','O'}, {'+','P'}, {'+','Q'}, {'+','R'}, {'+','S'}, {'+','T'}, {'+','U'}, {'+','V'}, {'+','W'},
    {'+','X'}, {'+','Y'}, {'+','Z'}, {'%','P'}, {'%','Q'}, {'%','R'}, {'%','S'}, {'%','T'}
};

/* Code 93 ANSI/AIM BC5-1995 Table 3 */
static const char C93Ctrl[128][2] = {
    {'b','U'}, {'a','A'}, {'a','B'}, {'a','C'}, {'a','D'}, {'a','E'}, {'a','F'}, {'a','G'}, {'a','H'}, {'a','I'},
    {'a','J'}, {'a','K'}, {'a','L'}, {'a','M'}, {'a','N'}, {'a','O'}, {'a','P'}, {'a','Q'}, {'a','R'}, {'a','S'},
    {'a','T'}, {'a','U'}, {'a','V'}, {'a','W'}, {'a','X'}, {'a','Y'}, {'a','Z'}, {'b','A'}, {'b','B'}, {'b','C'},
    {'b','D'}, {'b','E'}, {  " "  }, {'c','A'}, {'c','B'}, {'c','C'}, {  "$"  }, {  "%"  }, {'c','F'}, {'c','G'},
    {'c','H'}, {'c','I'}, {'c','J'}, {  "+"  }, {'c','L'}, {  "-"  }, {  "."  }, {  "/"  }, {  "0"  }, {  "1"  },
    {  "2"  }, {  "3"  }, {  "4"  }, {  "5"  }, {  "6"  }, {  "7"  }, {  "8"  }, {  "9"  }, {'c','Z'}, {'b','F'},
    {'b','G'}, {'b','H'}, {'b','I'}, {'b','J'}, {'b','V'}, {  "A"  }, {  "B"  }, {  "C"  }, {  "D"  }, {  "E"  },
    {  "F"  }, {  "G"  }, {  "H"  }, {  "I"  }, {  "J"  }, {  "K"  }, {  "L"  }, {  "M"  }, {  "N"  }, {  "O"  },
    {  "P"  }, {  "Q"  }, {  "R"  }, {  "S"  }, {  "T"  }, {  "U"  }, {  "V"  }, {  "W"  }, {  "X"  }, {  "Y"  },
    {  "Z"  }, {'b','K'}, {'b','L'}, {'b','M'}, {'b','N'}, {'b','O'}, {'b','W'}, {'d','A'}, {'d','B'}, {'d','C'},
    {'d','D'}, {'d','E'}, {'d','F'}, {'d','G'}, {'d','H'}, {'d','I'}, {'d','J'}, {'d','K'}, {'d','L'}, {'d','M'},
    {'d','N'}, {'d','O'}, {'d','P'}, {'d','Q'}, {'d','R'}, {'d','S'}, {'d','T'}, {'d','U'}, {'d','V'}, {'d','W'},
    {'d','X'}, {'d','Y'}, {'d','Z'}, {'b','P'}, {'b','Q'}, {'b','R'}, {'b','S'}, {'b','T'}
};

/* Code 93 ANSI/AIM BC5-1995 Table 2 */
static const char C93Table[47][6] = {
    {'1','3','1','1','1','2'}, {'1','1','1','2','1','3'}, {'1','1','1','3','1','2'}, {'1','1','1','4','1','1'},
    {'1','2','1','1','1','3'}, {'1','2','1','2','1','2'}, {'1','2','1','3','1','1'}, {'1','1','1','1','1','4'},
    {'1','3','1','2','1','1'}, {'1','4','1','1','1','1'}, {'2','1','1','1','1','3'}, {'2','1','1','2','1','2'},
    {'2','1','1','3','1','1'}, {'2','2','1','1','1','2'}, {'2','2','1','2','1','1'}, {'2','3','1','1','1','1'},
    {'1','1','2','1','1','3'}, {'1','1','2','2','1','2'}, {'1','1','2','3','1','1'}, {'1','2','2','1','1','2'},
    {'1','3','2','1','1','1'}, {'1','1','1','1','2','3'}, {'1','1','1','2','2','2'}, {'1','1','1','3','2','1'},
    {'1','2','1','1','2','2'}, {'1','3','1','1','2','1'}, {'2','1','2','1','1','2'}, {'2','1','2','2','1','1'},
    {'2','1','1','1','2','2'}, {'2','1','1','2','2','1'}, {'2','2','1','1','2','1'}, {'2','2','2','1','1','1'},
    {'1','1','2','1','2','2'}, {'1','1','2','2','2','1'}, {'1','2','2','1','2','1'}, {'1','2','3','1','1','1'},
    {'1','2','1','1','3','1'}, {'3','1','1','1','1','2'}, {'3','1','1','2','1','1'}, {'3','2','1','1','1','1'},
    {'1','1','2','1','3','1'}, {'1','1','3','1','2','1'}, {'2','1','1','1','3','1'}, {'1','2','1','2','2','1'},
    {'3','1','2','1','1','1'}, {'3','1','1','1','2','1'}, {'1','2','2','2','1','1'}
};

/* Code 39 */
INTERNAL int code39(struct zint_symbol *symbol, unsigned char source[], int length) {
    int i;
    int counter;
    int posns[86];
    char dest[890]; /* 10 (Start) + 86 * 10 + 10 (Check) + 9 (Stop) + 1 = 890 */
    char *d = dest;
    char check_digit = '\0';
    int error_number = 0;
    const int raw_text = symbol->output_options & BARCODE_RAW_TEXT;

    if ((symbol->option_2 < 0) || (symbol->option_2 > 2)) {
        symbol->option_2 = 0;
    }

    /* LOGMARS MIL-STD-1189 Rev. B https://apps.dtic.mil/dtic/tr/fulltext/u2/a473534.pdf */
    if ((symbol->symbology == BARCODE_LOGMARS) && (length > 30)) { /* MIL-STD-1189 Rev. B Section 5.2.6.2 */
        return errtxtf(ZINT_ERROR_TOO_LONG, symbol, 322, "Input length %d too long (maximum 30)", length);
    /* Prevent encoded_data out-of-bounds >= 143 for BARCODE_HIBC_39 due to wider 'wide' bars */
    } else if ((symbol->symbology == BARCODE_HIBC_39) && (length > 70)) { /* 16 (Start) + 70*16 + 15 (Stop) = 1151 */
        /* 70 less '+' and check */
        return errtxtf(ZINT_ERROR_TOO_LONG, symbol, 319, "Input length %d too long (maximum 68)", length - 2);
    } else if (length > 86) { /* 13 (Start) + 86*13 + 12 (Stop) = 1143 */
        return errtxtf(ZINT_ERROR_TOO_LONG, symbol, 323, "Input length %d too long (maximum 86)", length);
    }

    to_upper(source, length);
    if ((i = not_sane_lookup(SILVER, 43 /* Up to "%" */, source, length, posns))) {
        return errtxtf(ZINT_ERROR_INVALID_DATA, symbol, 324,
                        "Invalid character at position %d in input (alphanumerics, space and \"-.$/+%%\" only)", i);
    }

    /* Start character */
    memcpy(d, C39Table[43], 10);
    d += 10;

    for (i = 0, counter = 0; i < length; i++, d += 10) {
        memcpy(d, C39Table[posns[i]], 10);
        counter += posns[i];
    }

    if (symbol->option_2 == 1 || symbol->option_2 == 2) { /* Visible or hidden check digit */
        counter %= 43;
        check_digit = SILVER[counter];
        memcpy(d, C39Table[counter], 10);
        d += 10;

        if (symbol->debug & ZINT_DEBUG_PRINT) printf("Check digit: %c\n", check_digit);
    }

    /* Stop character */
    memcpy(d, C39Table[43], 9);
    d += 9;

    if ((symbol->symbology == BARCODE_LOGMARS) || (symbol->symbology == BARCODE_HIBC_39)) {
        /* LOGMARS and HIBC use wider 'wide' bars than normal Code 39 */
        counter = d - dest;
        for (i = 0; i < counter; i++) {
            if (dest[i] == '2') {
                dest[i] = '3';
            }
        }
    }

    if (symbol->debug & ZINT_DEBUG_PRINT) {
        printf("Barspaces: %.*s\n", (int) (d - dest), dest);
    }

    expand(symbol, dest, d - dest);

    if (symbol->output_options & COMPLIANT_HEIGHT) {
        if (symbol->symbology == BARCODE_LOGMARS) {
            /* MIL-STD-1189 Rev. B Section 5.2
               Min height 0.25" / 0.04" (X max) = 6.25
               Default height 0.625" (average of 0.375" - 0.875") / 0.01375" (average of 0.0075" - 0.02") ~ 45.45 */
            const float default_height = 45.4545441f; /* 0.625 / 0.01375 */
            const float max_height = 116.666664f; /* 0.875 / 0.0075 */
            error_number = set_height(symbol, 6.25f, default_height, max_height, 0 /*no_errtxt*/);
        } else if (symbol->symbology == BARCODE_CODE39 || symbol->symbology == BARCODE_EXCODE39
                    || symbol->symbology == BARCODE_HIBC_39) {
            /* ISO/IEC 16388:2007 4.4 (e) recommended min height 5.0mm or 15% of width excluding quiet zones;
               as X left to application specification use
               width = (C + 2) * (3 * N + 6) * X + (C + 1) * I = (C + 2) * 9 + C + 1) * X = (10 * C + 19);
               use 50 as default as none recommended */
            const float min_height = stripf((10.0f * (symbol->option_2 == 1 ? length + 1 : length) + 19.0f) * 0.15f);
            error_number = set_height(symbol, min_height, min_height > 50.0f ? min_height : 50.0f, 0.0f,
                                        0 /*no_errtxt*/);
        }
        /* PZN and CODE32 set their own heights */
    } else {
        (void) set_height(symbol, 0.0f, 50.f, 0.0f, 1 /*no_errtxt*/);
    }

    /* Display a space check digit as _, otherwise it looks like an error */
    if (symbol->option_2 == 1 && check_digit == ' ') {
        check_digit = '_';
    }
    if (symbol->symbology == BARCODE_CODE39) {
        hrt_cpy_chr(symbol, '*');
        hrt_cat_nochk(symbol, source, length);
        if (symbol->option_2 == 1) { /* Visible check digit */
            hrt_cat_chr_nochk(symbol, check_digit);
        }
        hrt_cat_chr_nochk(symbol, '*');
    } else {
        hrt_cpy_cat_nochk(symbol, source, length, symbol->option_2 == 1 ? check_digit : '\xFF', NULL /*cat*/, 0);
    }

    if (raw_text) {
        if (rt_cpy_cat(symbol, source, length, check_digit ? check_digit == '_' ? ' ' : check_digit : '\xFF',
                        NULL /*cat*/, 0)) {
            return ZINT_ERROR_MEMORY; /* `rt_cpy_cat()` only fails with OOM */
        }
    }

    return error_number;
}

/* Extended Code 39 - ISO/IEC 16388:2007 Annex A */
INTERNAL int excode39(struct zint_symbol *symbol, unsigned char source[], int length) {
    int i;
    unsigned char buffer[86 * 2 + 1] = {0};
    unsigned char *b = buffer;
    unsigned char check_digit = '\0';
    int error_number;
    const int saved_option_2 = symbol->option_2;
    const int raw_text = symbol->output_options & BARCODE_RAW_TEXT;

    if (length > 86) {
        return errtxtf(ZINT_ERROR_TOO_LONG, symbol, 328, "Input length %d too long (maximum 86)", length);
    }

    /* Create a buffer string and place control characters into it */
    for (i = 0; i < length; i++) {
        if (!z_isascii(source[i])) {
            /* Cannot encode extended ASCII */
            return errtxtf(ZINT_ERROR_INVALID_DATA, symbol, 329,
                            "Invalid character at position %d in input, extended ASCII not allowed", i + 1);
        }
        memcpy(b, EC39Ctrl[source[i]], 2);
        b += EC39Ctrl[source[i]][1] ? 2 : 1;
    }
    if (b - buffer > 86) {
        return errtxtf(ZINT_ERROR_TOO_LONG, symbol, 317, "Input too long, requires %d symbol characters (maximum 86)",
                        (int) (b - buffer));
    }
    *b = '\0';

    if (saved_option_2 == 2) {
        symbol->option_2 = 1; /* Make hidden check digit visible so returned in HRT */
    }
    if (raw_text) {
        symbol->output_options &= ~BARCODE_RAW_TEXT; /* Don't use `code39()`'s `raw_text` */
    }

    /* Then send the buffer to the C39 function */
    if ((error_number = code39(symbol, buffer, b - buffer)) >= ZINT_ERROR) {
        return error_number;
    }

    if (saved_option_2 == 2) {
        symbol->option_2 = 2; /* Restore */
    }
    if (raw_text) {
        symbol->output_options |= BARCODE_RAW_TEXT; /* Restore */
    }

    /* Save visible (or BARCODE_RAW_TEXT) check digit */
    if (symbol->option_2 == 1 || (raw_text && symbol->option_2 == 2)) {
        check_digit = symbol->text[symbol->text_length - 1];
    }

    /* Copy over source to HRT, subbing space for unprintables */
    (void) hrt_cpy_iso8859_1(symbol, source, length); /* Will fit (ASCII, length <= 86) */
    if (symbol->option_2 == 1) {
        hrt_cat_chr_nochk(symbol, check_digit);
    }

    if (raw_text && rt_cpy_cat(symbol, source, length, check_digit ? check_digit == '_' ? ' ' : check_digit : '\xFF',
                                NULL /*cat*/, 0)) {
        return ZINT_ERROR_MEMORY; /* `rt_cpy_cat()` only fails with OOM */
    }

    return error_number;
}

/* Code 93 is an advancement on Code 39 and the definition is a lot tighter */
INTERNAL int code93(struct zint_symbol *symbol, unsigned char source[], int length) {

    /* SILVER includes the extra characters a, b, c and d to represent Code 93 specific
       shift characters 1, 2, 3 and 4 respectively. These characters are never used by
       `code39()` and `excode39()` */

    int i;
    int h, weight, c, k, error_number = 0;
    int values[125]; /* 123 + 2 (Checks) */
    char buffer[247]; /* 123*2 (123 full ASCII) + 1 = 247 */
    char *b = buffer;
    char dest[764]; /* 6 (Start) + 123*6 + 2*6 (Checks) + 7 (Stop) + 1 (NUL) = 764 */
    char *d = dest;
    const int raw_text = symbol->output_options & BARCODE_RAW_TEXT;

    /* Suppresses clang-tidy clang-analyzer-core.CallAndMessage warning */
    assert(length > 0);

    if (length > 123) { /* 9 (Start) + 123*9 + 2*9 (Checks) + 10 (Stop) = 1144 */
        return errtxtf(ZINT_ERROR_TOO_LONG, symbol, 330, "Input length %d too long (maximum 123)", length);
    }

    /* Message Content */
    for (i = 0; i < length; i++) {
        if (!z_isascii(source[i])) {
            /* Cannot encode extended ASCII */
            return errtxtf(ZINT_ERROR_INVALID_DATA, symbol, 331,
                            "Invalid character at position %d in input, extended ASCII not allowed", i + 1);
        }
        memcpy(b, C93Ctrl[source[i]], 2);
        b += C93Ctrl[source[i]][1] ? 2 : 1;
    }

    /* Now we can check the true length of the barcode */
    h = b - buffer;
    if (h > 123) {
        return errtxtf(ZINT_ERROR_TOO_LONG, symbol, 332,
                        "Input too long, requires %d symbol characters (maximum 123)", h);
    }

    for (i = 0; i < h; i++) {
        values[i] = posn(SILVER, buffer[i]);
    }

    /* Putting the data into dest[] is not done until after check digits are calculated */

    /* Check digit C */
    c = 0;
    weight = 1;
    for (i = h - 1; i >= 0; i--) {
        c += values[i] * weight;
        weight++;
        if (weight == 21)
            weight = 1;
    }
    c = c % 47;
    values[h] = c;

    /* Check digit K */
    k = 0;
    weight = 1;
    for (i = h; i >= 0; i--) {
        k += values[i] * weight;
        weight++;
        if (weight == 16)
            weight = 1;
    }
    k = k % 47;
    values[h + 1] = k;
    h += 2;

    if (symbol->debug & ZINT_DEBUG_PRINT) {
        printf("Check digit c: %c (%d), k: %c (%d)\n", SILVER[c], c, SILVER[k], k);
    }

    /* Start character */
    memcpy(d, "111141", 6);
    d += 6;

    for (i = 0; i < h; i++, d += 6) {
        memcpy(d, C93Table[values[i]], 6);
    }

    /* Stop character */
    memcpy(d, "1111411", 7);
    d += 7;

    expand(symbol, dest, d - dest);

    if (symbol->output_options & COMPLIANT_HEIGHT) {
        /* ANSI/AIM BC5-1995 Section 2.6 minimum height 0.2" or 15% of symbol length, whichever is greater
           no max X given so for min height use symbol length = (9 * (C + 4) + 1) * X + 2 * Q = symbol->width + 20;
           use 40 as default height based on figures in spec */
        const float min_height = stripf((symbol->width + 20) * 0.15f);
        error_number = set_height(symbol, min_height, min_height > 40.0f ? min_height : 40.0f, 0.0f, 0 /*no_errtxt*/);
    } else {
        (void) set_height(symbol, 0.0f, 50.0f, 0.0f, 1 /*no_errtxt*/);
    }

    (void) hrt_cpy_iso8859_1(symbol, source, length); /* Will fit (ASCII, length <= 123) */
    if (symbol->option_2 == 1) {
        hrt_cat_chr_nochk(symbol, SILVER[c]);
        hrt_cat_chr_nochk(symbol, SILVER[k]);
    }

    if (raw_text && rt_cpy_cat(symbol, source, length, SILVER[c], (const unsigned char *) SILVER + k, 1)) {
        return ZINT_ERROR_MEMORY; /* `rt_cpy_cat()` only fails with OOM */
    }

    return error_number;
}

/* Vehicle Identification Number (VIN) */
INTERNAL int vin(struct zint_symbol *symbol, unsigned char source[], int length) {

    /* This code verifies the check digit present in North American VIN codes */

    char dest[200]; /* 10 + 10 + 17 * 10 + 9 + 1 = 200 */
    char *d = dest;
    char input_check;
    char output_check;
    int sum;
    int i;
    const int raw_text = symbol->output_options & BARCODE_RAW_TEXT;
    static const char weight[17] = { 8, 7, 6, 5, 4, 3, 2, 10, 0, 9, 8, 7, 6, 5, 4, 3, 2 };

    /* Check length */
    if (length != 17) {
        return errtxtf(ZINT_ERROR_TOO_LONG, symbol, 336, "Input length %d wrong (17 characters required)", length);
    }

    /* Check input characters, I, O and Q are not allowed */
    if ((i = not_sane(ARSENIC_F, source, length))) {
        return errtxtf(ZINT_ERROR_INVALID_DATA, symbol, 337,
                        "Invalid character at position %d in input (alphanumerics only, excluding \"IOQ\")", i);
    }

    to_upper(source, length);

    /* Check digit only valid for North America */
    if (source[0] >= '1' && source[0] <= '5') {
        input_check = source[8];

        sum = 0;
        for (i = 0; i < 17; i++) {
            int value;
            if (source[i] <= '9') {
                value = source[i] - '0';
            } else if (source[i] <= 'H') {
                value = (source[i] - 'A') + 1;
            } else if (source[i] <= 'R') {
                value = (source[i] - 'J') + 1;
            } else { /* (source[i] >= 'S') && (source[i] <= 'Z') */
                value = (source[i] - 'S') + 2;
            }
            sum += value * weight[i];
        }

        output_check = '0' + (sum % 11);

        if (output_check == ':') {
            /* Check digit was 10 */
            output_check = 'X';
        }

        if (symbol->debug & ZINT_DEBUG_PRINT) {
            printf("Producing VIN code: %s\n", source);
            printf("Input check was %c, calculated check is %c\n", input_check, output_check);
        }

        if (input_check != output_check) {
            return ZEXT errtxtf(ZINT_ERROR_INVALID_CHECK, symbol, 338,
                                "Invalid check digit '%1$c' (position 9), expecting '%2$c'", input_check,
                                output_check);
        }
    }

    /* Start character */
    memcpy(d, C39Table[43], 10);
    d += 10;

    /* Import character 'I' prefix? */
    if (symbol->option_2 == 1) {
        memcpy(d, C39Table[18], 10);
        d += 10;
    }

    /* Copy glyphs to symbol */
    for (i = 0; i < 17; i++, d += 10) {
        memcpy(d, C39Table[posn(SILVER, source[i])], 10);
    }

    /* Stop character */
    memcpy(d, C39Table[43], 9);
    d += 9;

    expand(symbol, dest, d - dest);

    hrt_cpy_nochk(symbol, source, length);

    if (raw_text && rt_cpy_cat(symbol, NULL /*source*/, 0, symbol->option_2 == 1 ? 'I' : '\xFF', source, length)) {
        return ZINT_ERROR_MEMORY; /* `rt_cpy_cat()` only fails with OOM */
    }

    /* Specification of dimensions/height for BARCODE_VIN unlikely */

    return 0;
}

/* vim: set ts=4 sw=4 et : */
