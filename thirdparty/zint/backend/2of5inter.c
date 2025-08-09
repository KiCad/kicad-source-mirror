/* 2of5inter.c - Handles Code 2 of 5 Interleaved */
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

/* Was in "2of5.c" */

#include <stdio.h>
#include "common.h"
#include "gs1.h"

static const char C25InterTable[10][5] = {
    {'1','1','3','3','1'}, {'3','1','1','1','3'}, {'1','3','1','1','3'}, {'3','3','1','1','1'},
    {'1','1','3','1','3'}, {'3','1','3','1','1'}, {'1','3','3','1','1'}, {'1','1','1','3','3'},
    {'3','1','1','3','1'}, {'1','3','1','3','1'}
};

/* Common to Interleaved, and to ITF-14, DP Leitcode, DP Identcode */
INTERNAL int c25_inter_common(struct zint_symbol *symbol, unsigned char source[], int length,
                const int checkdigit_option, const int dont_set_height) {
    int i, j, error_number = 0;
    char dest[638]; /* 4 + (125 + 1) * 5 + 3 + 1 = 638 */
    char *d = dest;
    unsigned char local_source[125 + 1];
    const int have_checkdigit = checkdigit_option == 1 || checkdigit_option == 2;
    const int raw_text = symbol->output_options & BARCODE_RAW_TEXT;

    if (length > 125) { /* 4 + (125 + 1) * 9 + 5 = 1143 */
        return errtxtf(ZINT_ERROR_TOO_LONG, symbol, 309, "Input length %d too long (maximum 125)", length);
    }
    if ((i = not_sane(NEON_F, source, length))) {
        return errtxtf(ZINT_ERROR_INVALID_DATA, symbol, 310,
                        "Invalid character at position %d in input (digits only)", i);
    }

    /* Input must be an even number of characters for Interlaced 2 of 5 to work:
       if an odd number of characters has been entered and no check digit or an even number and have check digit
       then add a leading zero */
    if (have_checkdigit == !(length & 1)) {
        local_source[0] = '0';
        memcpy(local_source + 1, source, length++);
    } else {
        memcpy(local_source, source, length);
    }

    if (have_checkdigit) {
        /* Add standard GS1 check digit */
        local_source[length] = gs1_check_digit(local_source, length);
        length++;
    }

    /* Start character */
    memcpy(d, "1111", 4);
    d += 4;

    for (i = 0; i < length; i += 2) {
        /* Look up the bars and the spaces */
        const char *const bars = C25InterTable[local_source[i] - '0'];
        const char *const spaces = C25InterTable[local_source[i + 1] - '0'];

        /* Then merge (interlace) the strings together */
        for (j = 0; j < 5; j++) {
            *d++ = bars[j];
            *d++ = spaces[j];
        }
    }

    /* Stop character */
    memcpy(d, "311", 3);
    d += 3;

    expand(symbol, dest, d - dest);

    if (!dont_set_height) {
        if (symbol->output_options & COMPLIANT_HEIGHT) {
            /* ISO/IEC 16390:2007 Section 4.4 min height 5mm or 15% of symbol width whichever greater where
               (P = character pairs, N = wide/narrow ratio = 3)
               width = (P(4N + 6) + N + 6)X = (length / 2) * 18 + 9 */
            /* Taking min X = 0.330mm from Annex D.3.1 (application specification) */
            const float min_height_min = 15.151515f; /* 5.0 / 0.33 */
            float min_height = stripf((18.0f * (length / 2) + 9.0f) * 0.15f);
            if (min_height < min_height_min) {
                min_height = min_height_min;
            }
            /* Using 50 as default as none recommended */
            error_number = set_height(symbol, min_height, min_height > 50.0f ? min_height : 50.0f, 0.0f,
                                        0 /*no_errtxt*/);
        } else {
            (void) set_height(symbol, 0.0f, 50.0f, 0.0f, 1 /*no_errtxt*/);
        }
    }

    /* Exclude check digit from HRT if hidden */
    hrt_cpy_nochk(symbol, local_source, length - (symbol->option_2 == 2));

    if (raw_text && rt_cpy(symbol, local_source, length)) {
        return ZINT_ERROR_MEMORY; /* `rt_cpy()` only fails with OOM */
    }

    return error_number;
}

/* Code 2 of 5 Interleaved ISO/IEC 16390:2007 */
INTERNAL int c25inter(struct zint_symbol *symbol, unsigned char source[], int length) {
    return c25_inter_common(symbol, source, length, symbol->option_2 /*checkdigit_option*/, 0 /*dont_set_height*/);
}

/* vim: set ts=4 sw=4 et : */
