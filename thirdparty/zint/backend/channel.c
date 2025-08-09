/* channel.c - Handles Channel */
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

#include <stdio.h>
#include "common.h"

typedef const struct s_channel_precalc {
    int value; unsigned char B[8]; unsigned char S[8]; unsigned char bmax[7]; unsigned char smax[7];
} channel_precalc;

#if 0
#define CHANNEL_GENERATE_PRECALCS
#endif

#ifdef CHANNEL_GENERATE_PRECALCS
/* To generate precalc tables uncomment CHANNEL_GENERATE_PRECALCS define and run
   "backend/tests/test_channel -f generate -g" and place result in "channel_precalcs.h" */
static void channel_generate_precalc(int channels, int value, int mod, int last, int B[8], int S[8], int bmax[7],
            int smax[7]) {
    int i;
    if (value == mod) printf("static channel_precalc channel_precalcs%d[] = {\n", channels);
    printf("    { %7ld, {", value); for (i = 0; i < 8; i++) printf(" %d,", B[i]); fputs(" },", stdout);
    fputs(" {", stdout); for (i = 0; i < 8; i++) printf(" %d,", S[i]); fputs(" },", stdout);
    fputs(" {", stdout); for (i = 0; i < 7; i++) printf(" %d,", bmax[i]); fputs(" },", stdout);
    fputs(" {", stdout); for (i = 0; i < 7; i++) printf(" %d,", smax[i]); fputs(" }, },\n", stdout);
    if (value == last) fputs("};\n", stdout);
}
#else
#include "channel_precalcs.h"
#endif

static int channel_copy_precalc(channel_precalc *const precalc, int B[8], int S[8], int bmax[7], int smax[7]) {
    int i;

    for (i = 0; i < 7; i++) {
        B[i] = precalc->B[i];
        S[i] = precalc->S[i];
        bmax[i] = precalc->bmax[i];
        smax[i] = precalc->smax[i];
    }
    B[7] = precalc->B[7];
    S[7] = precalc->S[7];

    return precalc->value;
}

/* CHNCHR is adapted from ANSI/AIM BC12-1998 Annex D Figure D5 and is Copyright (c) AIM 1997 */

/* It is used here on the understanding that it forms part of the specification
   for Channel Code and therefore its use is permitted under the following terms
   set out in that document:

   "It is the intent and understanding of AIM [t]hat the symbology presented in this
   specification is entirely in the public domain and free of all use restrictions,
   licenses and fees. AIM USA, its member companies, or individual officers
   assume no liability for the use of this document." */
static void CHNCHR(int channels, int target_value, int B[8], int S[8]) {
    /* Use of initial pre-calculations taken from Barcode Writer in Pure PostScript (BWIPP)
     * Copyright (c) 2004-2020 Terry Burton (MIT/X-Consortium license) */
    static channel_precalc initial_precalcs[6] = {
        { 0, { 1, 1, 1, 1, 1, 2, 1, 2, }, { 1, 1, 1, 1, 1, 1, 1, 3, }, { 1, 1, 1, 1, 1, 3, 2, },
            { 1, 1, 1, 1, 1, 3, 3, }, },
        { 0, { 1, 1, 1, 1, 2, 1, 1, 3, }, { 1, 1, 1, 1, 1, 1, 1, 4, }, { 1, 1, 1, 1, 4, 3, 3, },
            { 1, 1, 1, 1, 4, 4, 4, }, },
        { 0, { 1, 1, 1, 2, 1, 1, 2, 3, }, { 1, 1, 1, 1, 1, 1, 1, 5, }, { 1, 1, 1, 5, 4, 4, 4, },
            { 1, 1, 1, 5, 5, 5, 5, }, },
        { 0, { 1, 1, 2, 1, 1, 2, 1, 4, }, { 1, 1, 1, 1, 1, 1, 1, 6, }, { 1, 1, 6, 5, 5, 5, 4, },
            { 1, 1, 6, 6, 6, 6, 6, }, },
        { 0, { 1, 2, 1, 1, 2, 1, 1, 5, }, { 1, 1, 1, 1, 1, 1, 1, 7, }, { 1, 7, 6, 6, 6, 5, 5, },
            { 1, 7, 7, 7, 7, 7, 7, }, },
        { 0, { 2, 1, 1, 2, 1, 1, 2, 5, }, { 1, 1, 1, 1, 1, 1, 1, 8, }, { 8, 7, 7, 7, 6, 6, 6, },
            { 8, 8, 8, 8, 8, 8, 8, }, },
    };
    int bmax[7], smax[7];
    int value = 0;

    channel_copy_precalc(&initial_precalcs[channels - 3], B, S, bmax, smax);

#ifndef CHANNEL_GENERATE_PRECALCS
    if (channels == 7 && target_value >= channel_precalcs7[0].value) {
        value = channel_copy_precalc(&channel_precalcs7[(target_value / channel_precalcs7[0].value) - 1], B, S, bmax,
                                    smax);
    } else if (channels == 8 && target_value >= channel_precalcs8[0].value) {
        value = channel_copy_precalc(&channel_precalcs8[(target_value / channel_precalcs8[0].value) - 1], B, S, bmax,
                                    smax);
    }
#endif

    goto chkchr;

ls0:smax[1] = smax[0] + 1 - S[0]; B[0] = 1;
    if (S[0] == 1) goto nb0;
lb0:    bmax[1] = bmax[0] + 1 - B[0]; S[1] = 1;
ls1:        smax[2] = smax[1] + 1 - S[1]; B[1] = 1;
            if (S[0] + B[0] + S[1] == 3) goto nb1;
lb1:            bmax[2] = bmax[1] + 1 - B[1]; S[2] = 1;
ls2:                smax[3] = smax[2] + 1 - S[2]; B[2] = 1;
                    if (B[0] + S[1] + B[1] + S[2] == 4) goto nb2;
lb2:                    bmax[3] = bmax[2] + 1 - B[2]; S[3] = 1;
ls3:                        smax[4] = smax[3] + 1 - S[3]; B[3] = 1;
                            if (B[1] + S[2] + B[2] + S[3] == 4) goto nb3;
lb3:                            bmax[4] = bmax[3] + 1 - B[3]; S[4] = 1;
ls4:                                smax[5] = smax[4] + 1 - S[4]; B[4] = 1;
                                    if (B[2] + S[3] + B[3] + S[4] == 4) goto nb4;
lb4:                                    bmax[5] = bmax[4] + 1 - B[4]; S[5] = 1;
ls5:                                        smax[6] = smax[5] + 1 - S[5]; B[5] = 1;
                                            if (B[3] + S[4] + B[4] + S[5] == 4) goto nb5;
lb5:                                            bmax[6] = bmax[5] + 1 - B[5]; S[6] = 1;
ls6:                                                S[7] = smax[6] + 1 - S[6]; B[6] = 1;
                                                    if (B[4] + S[5] + B[5] + S[6] == 4) goto nb6;
lb6:                                                    B[7] = bmax[6] + 1 - B[6];
                                                        if (B[5] + S[6] + B[6] + S[7] + B[7] == 5) goto nb6;
chkchr:
#ifdef CHANNEL_GENERATE_PRECALCS
                                                        /* 115338 == (576688 + 2) / 5 */
                                                        if (channels == 7 && value && value % 115338 == 0) {
                                                            channel_generate_precalc(channels, value, 115338,
                                                                                115338 * (5 - 1), B, S, bmax, smax);
                                                        /* 119121 == (7742862 + 3) / 65 */
                                                        } else if (channels == 8 && value && value % 119121 == 0) {
                                                            channel_generate_precalc(channels, value, 119121,
                                                                                119121 * (65 - 1), B, S, bmax, smax);
                                                        }
#endif
                                                        if (value == target_value) return;
                                                        value++;
nb6:                                                    if (++B[6] <= bmax[6]) goto lb6;
                                                    if (++S[6] <= smax[6]) goto ls6;
nb5:                                            if (++B[5] <= bmax[5]) goto lb5;
                                            if (++S[5] <= smax[5]) goto ls5;
nb4:                                    if (++B[4] <= bmax[4]) goto lb4;
                                    if (++S[4] <= smax[4]) goto ls4;
nb3:                            if (++B[3] <= bmax[3]) goto lb3;
                            if (++S[3] <= smax[3]) goto ls3;
nb2:                    if (++B[2] <= bmax[2]) goto lb2;
                    if (++S[2] <= smax[2]) goto ls2;
nb1:            if (++B[1] <= bmax[1]) goto lb1;
            if (++S[1] <= smax[1]) goto ls1;
nb0:    if (++B[0] <= bmax[0]) goto lb0;
    if (++S[0] <= smax[0]) goto ls0;
}

/* Channel Code - According to ANSI/AIM BC12-1998 */
INTERNAL int channel(struct zint_symbol *symbol, unsigned char source[], int length) {
    static const int max_ranges[] = { -1, -1, -1, 26, 292, 3493, 44072, 576688, 7742862 };
    static const unsigned char zeroes_str[] = "0000000"; /* 7 zeroes */
    int S[8] = {0}, B[8] = {0};
    int target_value;
    char dest[30];
    char *d = dest;
    int channels, i;
    int error_number = 0, zeroes;
    const int raw_text = symbol->output_options & BARCODE_RAW_TEXT;

    if (length > 7) {
        return errtxtf(ZINT_ERROR_TOO_LONG, symbol, 333, "Input length %d too long (maximum 7)", length);
    }
    if ((i = not_sane(NEON_F, source, length))) {
        return errtxtf(ZINT_ERROR_INVALID_DATA, symbol, 334,
                        "Invalid character at position %d in input (digits only)", i);
    }
    target_value = to_int(source, length);

    if ((symbol->option_2 < 3) || (symbol->option_2 > 8)) {
        channels = 0;
    } else {
        channels = symbol->option_2;
    }

    if (channels == 0) {
        channels = length + 1;
        if (target_value > 576688 && channels < 8) {
            channels = 8;
        } else if (target_value > 44072 && channels < 7) {
            channels = 7;
        } else if (target_value > 3493 && channels < 6) {
            channels = 6;
        } else if (target_value > 292 && channels < 5) {
            channels = 5;
        } else if (target_value > 26 && channels < 4) {
            channels = 4;
        }
    }
    if (channels == 2) {
        channels = 3;
    }

    if (target_value > max_ranges[channels]) {
        if (channels == 8) {
            return ZEXT errtxtf(ZINT_ERROR_INVALID_DATA, symbol, 318, "Input value \"%1$d\" out of range (0 to %2$d)",
                                target_value, max_ranges[channels]);
        }
        return ZEXT errtxtf(ZINT_ERROR_INVALID_DATA, symbol, 335,
                            "Input value \"%1$d\" out of range (0 to %2$d for %3$d channels)", target_value,
                            max_ranges[channels], channels);
    }

    /* Feedback options */
    symbol->option_2 = channels;

    CHNCHR(channels, target_value, B, S);

    memcpy(d, "111111111", 9); /* Finder pattern */
    d += 9;
    for (i = 8 - channels; i < 8; i++) {
        *d++ = itoc(S[i]);
        *d++ = itoc(B[i]);
    }

    expand(symbol, dest, d - dest);

    if (symbol->output_options & COMPLIANT_HEIGHT) {
        /* ANSI/AIM BC12-1998 gives min height as 5mm or 15% of length; X left as application specification so use
           length = 1X (left qz) + (9 (finder) + 4 * 8 - 2) * X + 2X (right qz);
           use 20 as default based on figures in spec */
        const float min_height = stripf((1 + 9 + 4 * channels - 2 + 2) * 0.15f);
        error_number = set_height(symbol, min_height, 20.0f, 0.0f, 0 /*no_errtxt*/);
    } else {
        (void) set_height(symbol, 0.0f, 50.0f, 0.0f, 1 /*no_errtxt*/);
    }

    zeroes = channels - 1 - length;
    hrt_cpy_cat_nochk(symbol, zeroes_str, zeroes, '\xFF' /*separator (none)*/, source, length);

    if (raw_text && rt_cpy_cat(symbol, zeroes_str, zeroes, '\xFF' /*separator (none)*/, source, length)) {
        return ZINT_ERROR_MEMORY; /* `rt_cpy_cat()` only fails with OOM */
    }
    return error_number;
}

/* vim: set ts=4 sw=4 et : */
