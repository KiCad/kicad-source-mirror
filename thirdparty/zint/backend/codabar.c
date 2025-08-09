/* codabar.c - Handles Codabar */
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

/* Was in "medical.c" */

#include <stdio.h>
#include "common.h"

static const char CALCIUM[] = "0123456789-$:/.+ABCD";
#define CALCIUM_INNER_F (IS_NUM_F | IS_MNS_F | IS_CLI_F | IS_PLS_F) /* CALCIUM_INNER "0123456789-$:/.+" */

/* Codabar table checked against EN 798:1995 */
static const char CodaTable[20][8] = {
    {'1','1','1','1','1','2','2','1'}, {'1','1','1','1','2','2','1','1'}, {'1','1','1','2','1','1','2','1'},
    {'2','2','1','1','1','1','1','1'}, {'1','1','2','1','1','2','1','1'}, {'2','1','1','1','1','2','1','1'},
    {'1','2','1','1','1','1','2','1'}, {'1','2','1','1','2','1','1','1'}, {'1','2','2','1','1','1','1','1'},
    {'2','1','1','2','1','1','1','1'}, {'1','1','1','2','2','1','1','1'}, {'1','1','2','2','1','1','1','1'},
    {'2','1','1','1','2','1','2','1'}, {'2','1','2','1','1','1','2','1'}, {'2','1','2','1','2','1','1','1'},
    {'1','1','2','1','2','1','2','1'}, {'1','1','2','2','1','2','1','1'}, {'1','2','1','2','1','1','2','1'},
    {'1','1','1','2','1','2','2','1'}, {'1','1','1','2','2','2','1','1'}
};

/* The Codabar system consisting of simple substitution */
INTERNAL int codabar(struct zint_symbol *symbol, unsigned char source[], int length) {

    int i, error_number = 0;
    int posns[103];
    char dest[833]; /* (103 + 1) * 8 + 1 == 833 */
    char *d = dest;
    int add_checksum, count = 0, checksum = 0;
    int d_chars = 0;
    const int raw_text = symbol->output_options & BARCODE_RAW_TEXT;

    if (length > 103) { /* No stack smashing please (103 + 1) * 11 = 1144 */
        return errtxtf(ZINT_ERROR_TOO_LONG, symbol, 356, "Input length %d too long (maximum 103)", length);
    }
    /* BS EN 798:1995 4.2 "'Codabar' symbols shall consist of ... b) start character;
       c) one or more symbol characters representing data ... d) stop character ..." */
    if (length < 3) {
        return errtxtf(ZINT_ERROR_TOO_LONG, symbol, 362, "Input length %d too short (minimum 3)", length);
    }
    to_upper(source, length);

    /* Codabar must begin and end with the characters A, B, C or D */
    if ((source[0] != 'A') && (source[0] != 'B') && (source[0] != 'C')
            && (source[0] != 'D')) {
        return errtxt(ZINT_ERROR_INVALID_DATA, symbol, 358, "Does not begin with \"A\", \"B\", \"C\" or \"D\"");
    }
    if ((source[length - 1] != 'A') && (source[length - 1] != 'B') &&
            (source[length - 1] != 'C') && (source[length - 1] != 'D')) {
        return errtxt(ZINT_ERROR_INVALID_DATA, symbol, 359, "Does not end with \"A\", \"B\", \"C\" or \"D\"");
    }
    if ((i = not_sane_lookup(CALCIUM, sizeof(CALCIUM) - 1, source, length, posns))) {
        return ZEXT errtxtf(ZINT_ERROR_INVALID_DATA, symbol, 357,
                            "Invalid character at position %1$d in input (\"%2$s\" only)", i, CALCIUM);
    }
    /* And must not use A, B, C or D otherwise (BS EN 798:1995 4.3.2) */
    if ((i = not_sane(CALCIUM_INNER_F, source + 1, length - 2))) {
        return errtxtf(ZINT_ERROR_INVALID_DATA, symbol, 363,
                        "Invalid character at position %d in input (cannot contain \"A\", \"B\", \"C\" or \"D\")", i);
    }

    /* Add check character: 1 don't show to HRT, 2 do show to HRT
      (unfortunately to maintain back-compatibility, this is reverse of C25) */
    add_checksum = symbol->option_2 == 1 || symbol->option_2 == 2;

    for (i = 0; i < length; i++, d += 8) {
        if (add_checksum) {
            /* BS EN 798:1995 A.3 suggests using ISO 7064 algorithm but leaves it application defined.
               Following BWIPP and TEC-IT, use this simple mod-16 algorithm (not in ISO 7064) */
            count += posns[i];
            if (i + 1 == length) {
                checksum = count % 16;
                if (checksum) {
                    checksum = 16 - checksum;
                }
                if (symbol->debug & ZINT_DEBUG_PRINT) {
                    printf("Codabar: %s, count %d, checksum %d (%c)\n", source, count, checksum, CALCIUM[checksum]);
                }
                memcpy(d, CodaTable[checksum], 8);
                d += 8;
            }
        }
        memcpy(d, CodaTable[posns[i]], 8);
        if (source[i] == '/' || source[i] == ':' || source[i] == '.' || source[i] == '+') { /* Wide data characters */
            d_chars++;
        }
    }

    expand(symbol, dest, d - dest);

    if (symbol->output_options & COMPLIANT_HEIGHT) {
        /* BS EN 798:1995 4.4.1 (d) max of 5mm / 0.43mm (X max) ~ 11.628 or 15% of width where (taking N =
           narrow/wide ratio as 2 and I = X) width = ((2 * N + 5) * C + (N – 1) * (D + 2)) * X + I * (C – 1) + 2Q
           = ((4 + 5) * C + (D + 2) + C - 1 + 2 * 10) * X = (10 * C + D + 21) * X
           Length (C) includes start/stop chars */
        const float min_height_min = 11.6279068f; /* 5.0 / 0.43 */
        float min_height = stripf((10.0f * ((add_checksum ? length + 1 : length) + 2.0f) + d_chars + 21.0f) * 0.15f);
        if (min_height < min_height_min) {
            min_height = min_height_min;
        }
        /* Using 50 as default as none recommended */
        error_number = set_height(symbol, min_height, min_height > 50.0f ? min_height : 50.0f, 0.0f, 0 /*no_errtxt*/);
    } else {
        (void) set_height(symbol, 0.0f, 50.0f, 0.0f, 1 /*no_errtxt*/);
    }

    /* If visible check char, place before final A/B/C/D character (BS EN 798:1995 A.3) */
    hrt_cpy_cat_nochk(symbol, source, length - 1, symbol->option_2 == 2 ? CALCIUM[checksum] : '\xFF',
                        source + length - 1, 1);

    if (raw_text && rt_cpy_cat(symbol, source, length - 1,
                                add_checksum ? CALCIUM[checksum] : '\xFF', source + length - 1, 1)) {
        return ZINT_ERROR_MEMORY; /* `rt_cpy_cat()` only fails with OOM */
    }

    return error_number;
}

/* vim: set ts=4 sw=4 et : */
