/* 2of5.c - Handles non-interleaved Code 2 of 5 barcodes */
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

#include <stdio.h>
#include "common.h"
#include "gs1.h"

static const char C25MatrixTable[10][6] = {
    {'1','1','3','3','1','1'}, {'3','1','1','1','3','1'}, {'1','3','1','1','3','1'}, {'3','3','1','1','1','1'},
    {'1','1','3','1','3','1'}, {'3','1','3','1','1','1'}, {'1','3','3','1','1','1'}, {'1','1','1','3','3','1'},
    {'3','1','1','3','1','1'}, {'1','3','1','3','1','1'}
};

static const char C25IndustTable[10][10] = {
    {'1','1','1','1','3','1','3','1','1','1'}, {'3','1','1','1','1','1','1','1','3','1'},
    {'1','1','3','1','1','1','1','1','3','1'}, {'3','1','3','1','1','1','1','1','1','1'},
    {'1','1','1','1','3','1','1','1','3','1'}, {'3','1','1','1','3','1','1','1','1','1'},
    {'1','1','3','1','3','1','1','1','1','1'}, {'1','1','1','1','1','1','3','1','3','1'},
    {'3','1','1','1','1','1','3','1','1','1'}, {'1','1','3','1','1','1','3','1','1','1'}
};

/* Note `c25_common()` assumes Stop string length one less than Start */
static const char C25MatrixStartStop[2][6] =    { {'4', '1', '1', '1', '1', '1'}, {'4', '1', '1', '1', '1'} };
static const char C25IndustStartStop[2][6] =    { {'3', '1', '3', '1', '1', '1'}, {'3', '1', '1', '1', '3'} };
static const char C25IataLogicStartStop[2][6] = { {'1', '1', '1', '1'},           {'3', '1', '1'} };

/* Common to Standard (Matrix), Industrial, IATA, and Data Logic */
static int c25_common(struct zint_symbol *symbol, const unsigned char source[], int length, const int max,
            const int is_matrix, const char start_stop[2][6], const int start_length, const int error_base) {

    int i;
    char dest[818]; /* Largest destination 4 + (80 + 1) * 10 + 3 + 1 = 818 */
    char *d = dest;
    unsigned char local_source[113 + 1]; /* Largest maximum 113 + optional check digit */
    const int have_checkdigit = symbol->option_2 == 1 || symbol->option_2 == 2;
    const int raw_text = symbol->output_options & BARCODE_RAW_TEXT;

    if (length > max) {
        /* errtxt 301: 303: 305: 307: */
        return ZEXT errtxtf(ZINT_ERROR_TOO_LONG, symbol, error_base, "Input length %1$d too long (maximum %2$d)",
                            length, max);
    }
    if ((i = not_sane(NEON_F, source, length))) {
        /* Note: for all "at position" error messages, escape sequences not accounted for */
        /* errtxt 302: 304: 306: 308: */
        return errtxtf(ZINT_ERROR_INVALID_DATA, symbol, error_base + 1,
                        "Invalid character at position %d in input (digits only)", i);
    }

    memcpy(local_source, source, length);

    if (have_checkdigit) {
        /* Add standard GS1 check digit */
        local_source[length] = gs1_check_digit(source, length);
        length++;
        if (symbol->debug & ZINT_DEBUG_PRINT) printf("Check digit: %c\n", local_source[length - 1]);
    }

    /* Start character */
    memcpy(d, start_stop[0], start_length);
    d += start_length;

    if (is_matrix) {
        for (i = 0; i < length; i++, d += 6) {
            memcpy(d, C25MatrixTable[local_source[i] - '0'], 6);
        }
    } else {
        for (i = 0; i < length; i++, d += 10) {
            memcpy(d, C25IndustTable[local_source[i] - '0'], 10);
        }
    }

    /* Stop character */
    memcpy(d, start_stop[1], start_length - 1);
    d += start_length - 1;

    expand(symbol, dest, d - dest);

    /* Exclude check digit from HRT if hidden */
    hrt_cpy_nochk(symbol, local_source, length - (symbol->option_2 == 2));

    if (raw_text && rt_cpy(symbol, local_source, length)) {
        return ZINT_ERROR_MEMORY; /* `rt_cpy()` only fails with OOM */
    }

    return 0;
}

/* Code 2 of 5 Standard (Code 2 of 5 Matrix) */
INTERNAL int c25standard(struct zint_symbol *symbol, unsigned char source[], int length) {
    /* 9 + (112 + 1) * 10 + 8 = 1147 */
    return c25_common(symbol, source, length, 112, 1 /*is_matrix*/, C25MatrixStartStop, 6, 301);
}

/* Code 2 of 5 IATA */
INTERNAL int c25iata(struct zint_symbol *symbol, unsigned char source[], int length) {
    /* 4 + (80 + 1) * 14 + 5 = 1143 */
    return c25_common(symbol, source, length, 80, 0 /*is_matrix*/, C25IataLogicStartStop, 4, 305);
}

/* Code 2 of 5 Data Logic */
INTERNAL int c25logic(struct zint_symbol *symbol, unsigned char source[], int length) {
    /* 4 + (113 + 1) * 10 + 5 = 1149 */
    return c25_common(symbol, source, length, 113, 1 /*is_matrix*/, C25IataLogicStartStop, 4, 307);
}

/* Code 2 of 5 Industrial */
INTERNAL int c25ind(struct zint_symbol *symbol, unsigned char source[], int length) {
    /* 10 + (79 + 1) * 14 + 9 = 1139 */
    return c25_common(symbol, source, length, 79, 0 /*is_matrix*/, C25IndustStartStop, 6, 303);
}

/* vim: set ts=4 sw=4 et : */
