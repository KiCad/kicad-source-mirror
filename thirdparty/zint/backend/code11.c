/* code11.c - Handles Code 11 */
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

/* Was in "code.c" */

#include <assert.h>
#include <stdio.h>
#include "common.h"

#define SODIUM_MNS_F    (IS_NUM_F | IS_MNS_F) /* SODIUM "0123456789-" */

static const char C11Table[11 + 1][6] = {
    {'1','1','1','1','2','1'}, {'2','1','1','1','2','1'}, {'1','2','1','1','2','1'}, {'2','2','1','1','1','1'},
    {'1','1','2','1','2','1'}, {'2','1','2','1','1','1'}, {'1','2','2','1','1','1'}, {'1','1','1','2','2','1'},
    {'2','1','1','2','1','1'}, {'2','1','1','1','1','1'}, {'1','1','2','1','1','1'},
    {'1','1','2','2','1','1'} /* Start character (full 6), Stop character (first 5) */
};

/* Code 11 */
INTERNAL int code11(struct zint_symbol *symbol, unsigned char source[], int length) {
    static const unsigned char checkchrs[11] = { '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '-' };
    int i;
    int h;
    int weight[141]; /* 140 + 1 extra for 1st check */
    char dest[864]; /* 6 + 140 * 6 + 2 * 6 + 5 + 1 = 864 */
    char *d = dest;
    int num_check_digits;
    unsigned char checkstr[2];
    int error_number = 0;
    const int raw_text = symbol->output_options & BARCODE_RAW_TEXT;

    /* Suppresses clang-tidy clang-analyzer-core.UndefinedBinaryOperatorResult warning */
    assert(length > 0);

    if (length > 140) { /* 8 (Start) + 140 * 8 + 2 * 8 (Check) + 7 (Stop) = 1151 */
        return errtxtf(ZINT_ERROR_TOO_LONG, symbol, 320, "Input length %d too long (maximum 140)", length);
    }
    if ((i = not_sane(SODIUM_MNS_F, source, length))) {
        return errtxtf(ZINT_ERROR_INVALID_DATA, symbol, 321,
                        "Invalid character at position %d in input (digits and \"-\" only)", i);
    }

    if (symbol->option_2 < 0 || symbol->option_2 > 2) {
        return errtxtf(ZINT_ERROR_INVALID_OPTION, symbol, 339, "Invalid check digit version '%d' (1 or 2 only)",
                        symbol->option_2);
    }
    if (symbol->option_2 == 2) {
        num_check_digits = 0;
    } else if (symbol->option_2 == 1) {
        num_check_digits = 1;
    } else {
        num_check_digits = 2;
    }


    /* start character */
    memcpy(d, C11Table[11], 6);
    d +=  6;

    /* Draw main body of barcode */
    for (i = 0; i < length; i++, d += 6) {
        if (source[i] == '-')
            weight[i] = 10;
        else
            weight[i] = ctoi(source[i]);
        memcpy(d, C11Table[weight[i]], 6);
    }

    if (num_check_digits) {
        int c_weight = 1, c_count = 0, c_digit;
        /* Calculate C checksum */
        for (h = length - 1; h >= 0; h--) {
            c_count += (c_weight * weight[h]);
            c_weight++;

            if (c_weight > 10) {
                c_weight = 1;
            }
        }
        c_digit = c_count % 11;

        checkstr[0] = checkchrs[c_digit];
        memcpy(d, C11Table[c_digit], 6);
        d += 6;

        if (num_check_digits == 2) {
            int k_weight = 1, k_count = 0, k_digit;
            weight[length] = c_digit;

            /* Calculate K checksum */
            for (h = length; h >= 0; h--) {
                k_count += (k_weight * weight[h]);
                k_weight++;

                if (k_weight > 9) {
                    k_weight = 1;
                }
            }
            k_digit = k_count % 11;

            checkstr[1] = checkchrs[k_digit];
            memcpy(d, C11Table[k_digit], 6);
            d += 6;
        }
    }

    if (symbol->debug & ZINT_DEBUG_PRINT) {
        printf("Check digits (%d): %.*s%s\n", num_check_digits, num_check_digits, checkstr,
                num_check_digits ? "" : "<none>");
    }

    /* Stop character */
    memcpy(d, C11Table[11], 5);
    d += 5;

    expand(symbol, dest, d - dest);

    /* TODO: Find documentation on BARCODE_CODE11 dimensions/height */

    hrt_cpy_nochk(symbol, source, length);
    if (num_check_digits) {
        hrt_cat_nochk(symbol, checkstr, num_check_digits);
    }

    if (raw_text && rt_cpy_cat(symbol, source, length, '\xFF' /*separator (none)*/, checkstr, num_check_digits)) {
        return ZINT_ERROR_MEMORY; /* `rt_cpy_cat()` only fails with OOM */
    }

    return error_number;
}

/* vim: set ts=4 sw=4 et : */
