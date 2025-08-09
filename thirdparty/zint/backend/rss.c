/* rss.c - GS1 DataBar (formerly Reduced Space Symbology) */
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

/* The functions "dbar_combins" and "dbar_getWidths" are copyright BSI and are
   released with permission under the following terms:

   "Copyright subsists in all BSI publications. BSI also holds the copyright, in the
   UK, of the international standardisation bodies. Except as
   permitted under the Copyright, Designs and Patents Act 1988 no extract may be
   reproduced, stored in a retrieval system or transmitted in any form or by any
   means - electronic, photocopying, recording or otherwise - without prior written
   permission from BSI.

   "This does not preclude the free use, in the course of implementing the standard,
   of necessary details such as symbols, and size, type or grade designations. If these
   details are to be used for any other purpose than implementation then the prior
   written permission of BSI must be obtained."

   The date of publication for these functions is 30 November 2006
 */

/* Includes numerous bugfixes thanks to Pablo Orduña @ the PIRAmIDE project */

/* Note: The symbol names used in ISO/IEC 24724:2006
 * were updated in ISO/IEC 24724:2011 as follows:
 *
 * RSS-14 > GS1 DataBar Omnidirectional
 * RSS-14 Truncated > GS1 DataBar Truncated
 * RSS-14 Stacked > GS1 DataBar Stacked
 * RSS-14 Stacked Omnidirectional > GS1 DataBar Stacked Omnidirectional
 * RSS Limited > GS1 DataBar Limited
 * RSS Expanded > GS1 DataBar Expanded
 * RSS Expanded Stacked > GS1 DataBar Expanded Stacked
 */

#include <assert.h>
#include <stdio.h>
#include "common.h"
#include "general_field.h"
#include "gs1.h"
#include "rss.h"

/* `combins()' in ISO/IEC 24724:2011 Annex B */
/****************************************************************************
 * dbar_combins(n,r): returns the number of Combinations of r selected from n:
 *   Combinations = n! / ((n - r)! * r!)
 ****************************************************************************/
static int dbar_combins(const int n, const int r) {
    int i;
    int maxDenom, minDenom;
    int val = 1, j = 1;

    if (n - r > r) {
        minDenom = r;
        maxDenom = n - r;
    } else {
        minDenom = n - r;
        maxDenom = r;
    }
    for (i = n; i > maxDenom; i--) {
        val *= i;
        if (j <= minDenom) {
            val /= j;
            j++;
        }
    }
    for (; j <= minDenom; j++) {
        val /= j;
    }
    return val;
}

/* `getRSSwidths()' in ISO/IEC 24724:2011 Annex B, modified to use arg `widths` instead of static,
    and with `noNarrow` inverted (0 -> 1) so boolean */
/**********************************************************************
 * dbar_getWidths
 * routine to generate widths for DataBar elements for a given value.
 *
 * Calling arguments:
 * int widths[] = element widths
 * val = required value
 * n = number of modules
 * elements = elements in a set (Databar Limited = 7, all others = 4)
 * maxWidth = maximum module width of an element
 * noNarrow = 1 will skip patterns without a one module wide element
 *
 **********************************************************************/
static void dbar_getWidths(int widths[], int val, int n, const int elements, const int maxWidth, const int noNarrow) {
    int bar;
    int elmWidth;
    int mxwElement;
    int subVal, lessVal;
    int narrowMask = 0;

    for (bar = 0; bar < elements - 1; bar++) {
        for (elmWidth = 1, narrowMask |= (1 << bar); ; elmWidth++, narrowMask &= ~(1 << bar)) {
            /* Get all combinations */
            subVal = dbar_combins(n - elmWidth - 1, elements - bar - 2);
            /* Less combinations with no single-module element */
            if (noNarrow && !narrowMask && (n - elmWidth - (elements - bar - 1) >= elements - bar - 1)) {
                subVal -= dbar_combins(n - elmWidth - (elements - bar), elements - bar - 2);
            }
            /* Less combinations with elements > maxVal */
            if (elements - bar - 1 > 1) {
                lessVal = 0;
                for (mxwElement = n - elmWidth - (elements - bar - 2); mxwElement > maxWidth; mxwElement--) {
                    lessVal += dbar_combins(n - elmWidth - mxwElement - 1, elements - bar - 3);
                }
                subVal -= lessVal * (elements - 1 - bar);
            } else if (n - elmWidth > maxWidth) {
                subVal--;
            }
            val -= subVal;
            if (val < 0) break;
        }
        val += subVal;
        n -= elmWidth;
        widths[bar] = elmWidth;
    }
    widths[bar] = n;
}

/* Interleave `dbar_getWidths()` */
static void dbar_widths(int *ret_widths, int v_odd, int v_even, int n_odd, int n_even, const int elements,
                const int maxWidth, const int noNarrow) {
    int widths[2][7];
    int i;

    assert(elements <= 7);

    dbar_getWidths(widths[0], v_odd, n_odd, elements, maxWidth, noNarrow);
    dbar_getWidths(widths[1], v_even, n_even, elements, 9 - maxWidth, !noNarrow);

    for (i = 0; i < elements; i++) {
        ret_widths[i << 1] = widths[0][i];
        ret_widths[(i << 1) + 1] = widths[1][i];
    }
}

/* Converts decimal string of length <= 19 to 64-bit unsigned int */
static uint64_t dbar_to_uint64(const unsigned char source[], const int length) {
    uint64_t val = 0;
    int i;

    for (i = 0; i < length; i++) {
        val *= 10;
        val += source[i] - '0';
    }

    return val;
}

/* Helper to construct zero-padded GTIN14 with check digit, returning `buf` for convenience */
static unsigned char *dbar_gtin14(const unsigned char *source, const int length, unsigned char buf[14]) {
    const int zeroes = 13 - length;

    assert(zeroes >= 0);
    memset(buf, '0', zeroes);
    memcpy(buf + zeroes, source, length);
    buf[zeroes + length] = gs1_check_digit(buf, 13);

    return buf;
}

/* Set GTIN-14 human readable text */
static void dbar_set_gtin14_hrt(struct zint_symbol *symbol, const unsigned char *source, const int length) {
    unsigned char buf[14];

    hrt_cpy_nochk(symbol, ZCUCP("(01)"), 4);
    hrt_cat_nochk(symbol, dbar_gtin14(source, length, buf), 14);
}

/* Expand from a width pattern to a bit pattern */
static int dbar_expand(struct zint_symbol *symbol, int writer, int latch, const int *const widths, const int start,
                const int end) {
    int i, j;

    for (i = start; i < end; i++) {
        const int width = widths[i];
        if (latch) {
            for (j = 0; j < width; j++) {
                set_module(symbol, symbol->rows, writer);
                writer++;
            }
        } else {
            for (j = 0; j < width; j++) {
                unset_module(symbol, symbol->rows, writer);
                writer++;
            }
        }
        latch = !latch;
    }

    return writer;
}

/* DataBar Omnidirectional (incl. Truncated/Stacked/Stacked Omnidirectional) stuff */

/* Adjust top/bottom separator for DataBar Omnidirectional finder patterns */
static void dbar_omn_finder_adjust(struct zint_symbol *symbol, const int separator_row, const int above_below,
                const int finder_start) {
    int i, finder_end;
    int module_row = separator_row + above_below;
    int latch;

    /* Alternation is always left-to-right for Omnidirectional separators (unlike for Expanded) */
    latch = 1;
    for (i = finder_start, finder_end = finder_start + 13; i < finder_end; i++) {
        if (!module_is_set(symbol, module_row, i)) {
            if (latch) {
                set_module(symbol, separator_row, i);
                latch = 0;
            } else {
                unset_module(symbol, separator_row, i);
                latch = 1;
            }
        } else {
            unset_module(symbol, separator_row, i);
            latch = 1;
        }
    }
}

/* Top/bottom separator for DataBar Omnidirectional (Composite and DataBar Stacked Omnidirectional) */
static void dbar_omn_separator(struct zint_symbol *symbol, int width, const int separator_row, const int above_below,
                const int finder_start, const int finder2_start, const int bottom_finder_value_3) {
    int i, finder_end, finder_value_3_set;
    int module_row = separator_row + above_below;

    for (i = 4, width -= 4; i < width; i++) {
        if (!module_is_set(symbol, module_row, i)) {
            set_module(symbol, separator_row, i);
        }
    }
    if (bottom_finder_value_3) {
        /* ISO/IEC 24724:2011 5.3.2.2 "The single dark module that occurs in the 13 modules over finder value 3 is
           shifted one module to the right so that it is over the start of the three module-wide finder bar." */
        finder_value_3_set = finder_start + 10;
        for (i = finder_start, finder_end = finder_start + 13; i < finder_end; i++) {
            if (i == finder_value_3_set) {
                set_module(symbol, separator_row, i);
            } else {
                unset_module(symbol, separator_row, i);
            }
        }
    } else {
        if (finder_start) {
            dbar_omn_finder_adjust(symbol, separator_row, above_below, finder_start);
        }
        if (finder2_start) {
            dbar_omn_finder_adjust(symbol, separator_row, above_below, finder2_start);
        }
    }
}

/* Set Databar Stacked height, maintaining 5:7 ratio of the 2 main row heights */
INTERNAL int dbar_omnstk_set_height(struct zint_symbol *symbol, const int first_row) {
    float fixed_height = 0.0f;
    const int second_row = first_row + 2; /* 2 row separator */
    int i;

    for (i = 0; i < symbol->rows; i++) {
        if (i != first_row && i != second_row) {
            fixed_height += symbol->row_height[i];
        }
    }
    if (symbol->height) {
        symbol->row_height[first_row] = stripf((symbol->height - fixed_height) * symbol->row_height[first_row] /
                                                (symbol->row_height[first_row] + symbol->row_height[second_row]));
        if (symbol->row_height[first_row] < 0.5f) { /* Absolute minimum */
            symbol->row_height[first_row] = 0.5f;
            symbol->row_height[second_row] = 0.7f;
        } else {
            symbol->row_height[second_row] = stripf(symbol->height - fixed_height - symbol->row_height[first_row]);
            if (symbol->row_height[second_row] < 0.7f) {
                symbol->row_height[second_row] = 0.7f;
            }
        }
    }
    symbol->height = stripf(stripf(symbol->row_height[first_row] + symbol->row_height[second_row]) + fixed_height);

    if (symbol->output_options & COMPLIANT_HEIGHT) {
        if (symbol->row_height[first_row] < 5.0f || symbol->row_height[second_row] < 7.0f) {
            return errtxt(ZINT_WARN_NONCOMPLIANT, symbol, 379, "Height not compliant with standards");
        }
    }

    return 0;
}

/* Return DataBar Omnidirectional group (outside -1, inside +5-1) */
static int dbar_omn_group(const int val, const int outside) {
    const int end = 8 >> outside;
    int i;
    for (i = outside ? 0 : 5; i < end; i++) {
        if (val < dbar_omn_g_sum[i + 1]) {
            return i;
        }
    }
    return i;
}

/* GS1 DataBar Omnidirectional/Truncated/Stacked/Stacked Omnidirectional, allowing for composite if `cc_rows` set */
INTERNAL int dbar_omn_cc(struct zint_symbol *symbol, unsigned char source[], int length, const int cc_rows) {
    int error_number = 0, i, j;
    uint64_t val;
    int left_pair, right_pair;
    int data_character[4];
    int data_widths[4][8], checksum, c_left, c_right, total_widths[46], writer;
    int separator_row = 0;
    const int raw_text = symbol->output_options & BARCODE_RAW_TEXT;

    /* Allow and ignore any AI prefix */
    if ((length == 17 || length == 18) && (memcmp(source, "[01]", 4) == 0 || memcmp(source, "(01)", 4) == 0)) {
        source += 4;
        length -= 4;
    /* Likewise initial '01' */
    } else if ((length == 15 || length == 16) && source[0] == '0' && source[1] == '1') {
        source += 2;
        length -= 2;
    }
    if (length > 14) { /* Allow check digit to be specified (will be verified and ignored) */
        return errtxtf(ZINT_ERROR_TOO_LONG, symbol, 380, "Input length %d too long (maximum 14)", length);
    }
    if ((i = not_sane(NEON_F, source, length))) {
        return errtxtf(ZINT_ERROR_INVALID_DATA, symbol, 381,
                        "Invalid character at position %d in input (digits only)", i);
    }

    if (length == 14) { /* Verify check digit */
        if (gs1_check_digit(source, 13) != source[13]) {
            return ZEXT errtxtf(ZINT_ERROR_INVALID_CHECK, symbol, 388, "Invalid check digit '%1$c', expecting '%2$c'",
                                source[13], gs1_check_digit(source, 13));
        }
        length--; /* Ignore */
    }

    if (symbol->symbology != BARCODE_DBAR_OMN) {
        symbol->rows = 0; /* Stacked (and composites) are not stackable */
    }

    /* Make some room for a separator row for composite symbols */
    switch (symbol->symbology) {
        case BARCODE_DBAR_OMN_CC:
        case BARCODE_DBAR_STK_CC:
        case BARCODE_DBAR_OMNSTK_CC:
            separator_row = symbol->rows++;
            symbol->row_height[separator_row] = 1;
            break;
    }

    val = dbar_to_uint64(source, length);

    if (cc_rows) {
        /* Add symbol linkage flag */
        val += 10000000000000;
    }

    /* Calculate left and right pair values */

    left_pair = (int) (val / 4537077);
    right_pair = (int) (val % 4537077);

    /* Calculate four data characters */

    data_character[0] = left_pair / 1597;
    data_character[1] = left_pair % 1597;

    data_character[2] = right_pair / 1597;
    data_character[3] = right_pair % 1597;

    /* Calculate odd and even subset values */

    /* Use DataBar subset width algorithm */
    for (i = 0; i < 4; i++) {
        /* Counting 1-based 1, 2, 4, 3, i.e. 0-based 0, 1, 3, 2, so 0 & 2 outside */
        const int group = dbar_omn_group(data_character[i], !(i & 1) /*outside*/);
        const int v = data_character[i] - dbar_omn_g_sum[group];
        const int v_div = v / dbar_omn_t_even_odd[group];
        const int v_mod = v % dbar_omn_t_even_odd[group];
        dbar_widths(data_widths[i], !(i & 1) ? v_div : v_mod, i & 1 ? v_div : v_mod, dbar_omn_modules[group],
                    dbar_omn_modules[group + 9], 4 /*elements*/, dbar_omn_widest[group], i & 1 /*noNarrow*/);
    }

    checksum = 0;
    /* Calculate the checksum */
    for (i = 0; i < 4; i++) {
        for (j = 0; j < 8; j++) {
            checksum += dbar_omn_checksum_weight[i][j] * data_widths[i][j];
        }
    }
    checksum %= 79;

    /* Calculate the two check characters */
    if (checksum >= 8) {
        checksum++;
    }
    if (checksum >= 72) {
        checksum++;
    }
    c_left = checksum / 9;
    c_right = checksum % 9;

    if (symbol->debug & ZINT_DEBUG_PRINT) {
        printf("checksum %d, c_left: %d,  c_right: %d\n", checksum, c_left, c_right);
    }

    /* Put element widths together */
    total_widths[0] = 1;
    total_widths[1] = 1;
    total_widths[44] = 1;
    total_widths[45] = 1;
    for (i = 0; i < 8; i++) {
        total_widths[i + 2] = data_widths[0][i];
        total_widths[i + 15] = data_widths[1][7 - i];
        total_widths[i + 23] = data_widths[3][i];
        total_widths[i + 36] = data_widths[2][7 - i];
    }
    for (i = 0; i < 5; i++) {
        total_widths[i + 10] = dbar_omn_finder_pattern[c_left][i];
        total_widths[i + 31] = dbar_omn_finder_pattern[c_right][4 - i];
    }

    /* Put this data into the symbol */
    if (symbol->symbology == BARCODE_DBAR_OMN || symbol->symbology == BARCODE_DBAR_OMN_CC) {
        writer = dbar_expand(symbol, 0 /*writer*/, 0 /*latch*/, total_widths, 0 /*start*/, 46 /*end*/);
        if (symbol->width < writer) {
            symbol->width = writer;
        }
        if (symbol->symbology == BARCODE_DBAR_OMN_CC) {
            /* Separator pattern for composite symbol */
            dbar_omn_separator(symbol, 96, separator_row, 1 /*above*/, 18, 63, 0 /*bottom_finder_value_3*/);
        }
        symbol->rows++;

        /* Set human readable text */
        dbar_set_gtin14_hrt(symbol, source, length);

        if (symbol->output_options & COMPLIANT_HEIGHT) {
            /* Minimum height is 13X for truncated symbol ISO/IEC 24724:2011 5.3.1
               Default height is 33X for DataBar Omnidirectional ISO/IEC 24724:2011 5.2 */
            if (symbol->symbology == BARCODE_DBAR_OMN_CC) {
                symbol->height = symbol->height ? 13.0f : 33.0f; /* Pass back min row or default height */
            } else {
                error_number = set_height(symbol, 13.0f, 33.0f, 0.0f, 0 /*no_errtxt*/);
            }
        } else {
            if (symbol->symbology == BARCODE_DBAR_OMN_CC) {
                symbol->height = 14.0f; /* 14X truncated min row height used (should be 13X) */
            } else {
                (void) set_height(symbol, 0.0f, 50.0f, 0.0f, 1 /*no_errtxt*/);
            }
        }

    } else if (symbol->symbology == BARCODE_DBAR_STK || symbol->symbology == BARCODE_DBAR_STK_CC) {
        /* Top row */
        writer = dbar_expand(symbol, 0 /*writer*/, 0 /*latch*/, total_widths, 0 /*start*/, 23 /*end*/);
        set_module(symbol, symbol->rows, writer);
        unset_module(symbol, symbol->rows, writer + 1);
        symbol->row_height[symbol->rows] = 5.0f; /* ISO/IEC 24724:2011 5.3.2.1 set to 5X */

        /* Bottom row */
        symbol->rows += 2;
        set_module(symbol, symbol->rows, 0);
        unset_module(symbol, symbol->rows, 1);
        (void) dbar_expand(symbol, 2 /*writer*/, 1 /*latch*/, total_widths, 23 /*start*/, 46 /*end*/);
        symbol->row_height[symbol->rows] = 7.0f; /* ISO/IEC 24724:2011 5.3.2.1 set to 7X */

        /* Separator pattern */
        /* See #183 for this interpretation of ISO/IEC 24724:2011 5.3.2.1 */
        for (i = 1; i < 46; i++) {
            if (module_is_set(symbol, symbol->rows - 2, i) == module_is_set(symbol, symbol->rows, i)) {
                if (!(module_is_set(symbol, symbol->rows - 2, i))) {
                    set_module(symbol, symbol->rows - 1, i);
                }
            } else {
                if (!(module_is_set(symbol, symbol->rows - 1, i - 1))) {
                    set_module(symbol, symbol->rows - 1, i);
                }
            }
        }
        unset_module(symbol, symbol->rows - 1, 1);
        unset_module(symbol, symbol->rows - 1, 2);
        unset_module(symbol, symbol->rows - 1, 3);
        symbol->row_height[symbol->rows - 1] = 1;

        if (symbol->symbology == BARCODE_DBAR_STK_CC) {
            /* Separator pattern for composite symbol */
            dbar_omn_separator(symbol, 50, separator_row, 1 /*above*/, 18, 0, 0 /*bottom_finder_value_3*/);
        }
        symbol->rows++;
        if (symbol->width < 50) {
            symbol->width = 50;
        }

        if (symbol->symbology != BARCODE_DBAR_STK_CC) { /* Composite calls dbar_omnstk_set_height() itself */
            error_number = dbar_omnstk_set_height(symbol, 0 /*first_row*/);
        }

    } else if (symbol->symbology == BARCODE_DBAR_OMNSTK || symbol->symbology == BARCODE_DBAR_OMNSTK_CC) {
        /* Top row */
        writer = dbar_expand(symbol, 0 /*writer*/, 0 /*latch*/, total_widths, 0 /*start*/, 23 /*end*/);
        set_module(symbol, symbol->rows, writer);
        unset_module(symbol, symbol->rows, writer + 1);

        /* Bottom row */
        symbol->rows += 4;
        set_module(symbol, symbol->rows, 0);
        unset_module(symbol, symbol->rows, 1);
        (void) dbar_expand(symbol, 2 /*writer*/, 1 /*latch*/, total_widths, 23 /*start*/, 46 /*end*/);

        /* Middle separator */
        for (i = 5; i < 46; i += 2) {
            set_module(symbol, symbol->rows - 2, i);
        }
        symbol->row_height[symbol->rows - 2] = 1;

        /* Top separator */
        dbar_omn_separator(symbol, 50, symbol->rows - 3, -1 /*below*/, 18, 0, 0 /*bottom_finder_value_3*/);
        symbol->row_height[symbol->rows - 3] = 1;

        /* Bottom separator */
        /* 17 == 2 (guard) + 15 (inner char); +2 to skip over finder elements 4 & 5 (right to left) */
        dbar_omn_separator(symbol, 50, symbol->rows - 1, 1 /*above*/, 17 + 2, 0, c_right == 3);
        symbol->row_height[symbol->rows - 1] = 1;
        if (symbol->width < 50) {
            symbol->width = 50;
        }

        if (symbol->symbology == BARCODE_DBAR_OMNSTK_CC) {
            /* Separator pattern for composite symbol */
            dbar_omn_separator(symbol, 50, separator_row, 1 /*above*/, 18, 0, 0 /*bottom_finder_value_3*/);
        }
        symbol->rows++;

        /* ISO/IEC 24724:2011 5.3.2.2 minimum 33X height per row */
        if (symbol->symbology == BARCODE_DBAR_OMNSTK_CC) {
            symbol->height = symbol->height ? 33.0f : 66.0f; /* Pass back min row or default height */
        } else {
            if (symbol->output_options & COMPLIANT_HEIGHT) {
                error_number = set_height(symbol, 33.0f, 66.0f, 0.0f, 0 /*no_errtxt*/);
            } else {
                (void) set_height(symbol, 0.0f, 66.0f, 0.0f, 1 /*no_errtxt*/);
            }
        }
    }

    if (raw_text) {
        unsigned char buf[14];
        if (rt_cpy_cat(symbol, ZCUCP("01"), 2, '\xFF' /*none*/, dbar_gtin14(source, length, buf), 14)) {
            return ZINT_ERROR_MEMORY; /* `rt_cpy_cat()` only fails with OOM */
        }
    }

    return error_number;
}

/* GS1 DataBar Omnidirectional/Truncated/Stacked */
INTERNAL int dbar_omn(struct zint_symbol *symbol, unsigned char source[], int length) {
    return dbar_omn_cc(symbol, source, length, 0 /*cc_rows*/);
}

/* DataBar Limited stuff */

/* Return DataBar Limited group (-1) */
static int dbar_ltd_group(int *p_val) {
    static int g_sum[7] = {
        0, 183064, 820064, 1000776, 1491021, 1979845, 1996939
    };
    int i;
    for (i = 6; i > 0; i--) {
        if (*p_val >= g_sum[i]) {
            *p_val -= g_sum[i];
            return i;
        }
    }
    return 0;
}

/* GS1 DataBar Limited, allowing for composite if `cc_rows` set  */
INTERNAL int dbar_ltd_cc(struct zint_symbol *symbol, unsigned char source[], int length, const int cc_rows) {
    int error_number = 0, i;
    uint64_t val;
    int pair_vals[2];
    int pair_widths[2][14];
    int checksum, total_widths[47], writer;
    const char *checksum_finder_pattern;
    int separator_row = 0;
    const int raw_text = symbol->output_options & BARCODE_RAW_TEXT;

    /* Allow and ignore any AI prefix */
    if ((length == 17 || length == 18) && (memcmp(source, "[01]", 4) == 0 || memcmp(source, "(01)", 4) == 0)) {
        source += 4;
        length -= 4;
    /* Likewise initial '01' */
    } else if ((length == 15 || length == 16) && source[0] == '0' && source[1] == '1') {
        source += 2;
        length -= 2;
    }
    if (length > 14) { /* Allow check digit to be specified (will be verified and ignored) */
        return errtxtf(ZINT_ERROR_TOO_LONG, symbol, 382, "Input length %d too long (maximum 14)", length);
    }
    if ((i = not_sane(NEON_F, source, length))) {
        return errtxtf(ZINT_ERROR_INVALID_DATA, symbol, 383,
                        "Invalid character at position %d in input (digits only)", i);
    }

    if (length == 14) { /* Verify check digit */
        if (gs1_check_digit(source, 13) != source[13]) {
            return ZEXT errtxtf(ZINT_ERROR_INVALID_CHECK, symbol, 389, "Invalid check digit '%1$c', expecting '%2$c'",
                                source[13], gs1_check_digit(source, 13));
        }
        length--; /* Ignore */
    }

    if (length == 13) {
        if (source[0] != '0' && source[0] != '1') {
            return errtxt(ZINT_ERROR_INVALID_DATA, symbol, 384, "Input value out of range (0 to 1999999999999)");
        }
    }

    /* Make some room for a separator row for composite symbols */
    if (symbol->symbology == BARCODE_DBAR_LTD_CC) {
        separator_row = symbol->rows++;
        symbol->row_height[separator_row] = 1;
    }

    val = dbar_to_uint64(source, length);

    if (cc_rows) {
        /* Add symbol linkage flag */
        val += 2015133531096;
    }

    /* Calculate left (0) and right (1) pair values */

    pair_vals[0] = (int) (val / 2013571);
    pair_vals[1] = (int) (val % 2013571);

    for (i = 0; i < 2; i++) {
        const int group = dbar_ltd_group(&pair_vals[i]);
        const int odd = pair_vals[i] / dbar_ltd_t_even[group];
        const int even = pair_vals[i] % dbar_ltd_t_even[group];
        dbar_widths(pair_widths[i], odd, even, dbar_ltd_modules[group], 26 - dbar_ltd_modules[group], 7 /*elements*/,
                    dbar_ltd_widest[group], 0 /*noNarrow*/);
    }

    checksum = 0;
    /* Calculate the checksum */
    for (i = 0; i < 14; i++) {
#if defined(_MSC_VER) && _MSC_VER == 1900 && defined(_WIN64) /* MSVC 2015 x64 */
        checksum %= 89; /* Hack to get around optimizer bug */
#endif
        checksum += dbar_ltd_checksum_weight[0][i] * pair_widths[0][i];
        checksum += dbar_ltd_checksum_weight[1][i] * pair_widths[1][i];
    }
    checksum %= 89;

    checksum_finder_pattern = dbar_ltd_finder_pattern[checksum];

    total_widths[0] = 1;
    total_widths[1] = 1;
    total_widths[44] = 1;
    total_widths[45] = 1;
    total_widths[46] = 5;
    for (i = 0; i < 14; i++) {
        total_widths[i + 2] = pair_widths[0][i];
        total_widths[i + 16] = checksum_finder_pattern[i];
        total_widths[i + 30] = pair_widths[1][i];
    }

    writer = dbar_expand(symbol, 0 /*writer*/, 0 /*latch*/, total_widths, 0 /*start*/, 47 /*end*/);
    if (symbol->width < writer) {
        symbol->width = writer;
    }
    symbol->rows++;

    /* Add separator pattern if composite symbol */
    if (symbol->symbology == BARCODE_DBAR_LTD_CC) {
        for (i = 4; i < 70; i++) {
            if (!module_is_set(symbol, separator_row + 1, i)) {
                set_module(symbol, separator_row, i);
            }
        }
    }

    /* Set human readable text */
    dbar_set_gtin14_hrt(symbol, source, length);

    if (raw_text) {
        unsigned char buf[14];
        if (rt_cpy_cat(symbol, ZCUCP("01"), 2, '\xFF' /*none*/, dbar_gtin14(source, length, buf), 14)) {
            return ZINT_ERROR_MEMORY; /* `rt_cpy_cat()` only fails with OOM */
        }
    }

    /* ISO/IEC 24724:2011 6.2 10X minimum height, use as default also */
    if (symbol->symbology == BARCODE_DBAR_LTD_CC) {
        symbol->height = 10.0f; /* Pass back min row == default height */
    } else {
        if (symbol->output_options & COMPLIANT_HEIGHT) {
            error_number = set_height(symbol, 10.0f, 10.0f, 0.0f, 0 /*no_errtxt*/);
        } else {
            (void) set_height(symbol, 0.0f, 50.0f, 0.0f, 1 /*no_errtxt*/);
        }
    }

    return error_number;
}

/* GS1 DataBar Limited */
INTERNAL int dbar_ltd(struct zint_symbol *symbol, unsigned char source[], int length) {
    return dbar_ltd_cc(symbol, source, length, 0 /*cc_rows*/);
}

/* DataBar Expanded stuff */

/* Check and convert date to DataBar Expanded date value */
INTERNAL int dbar_exp_date(const unsigned char source[], const int length, const int position) {
    int yy, mm, dd;

    if (position + 4 + 2 > length) {
        return -1;
    }
    yy = to_int(source + position, 2);
    mm = to_int(source + position + 2, 2);
    dd = to_int(source + position + 4, 2);

    /* Month can't be zero but day can (means last day of month,
       GS1 General Specifications Sections 3.4.2 to 3.4.7) */
    if (yy < 0 || mm <= 0 || mm > 12 || dd < 0 || dd > 31) {
        return -1;
    }
    return yy * 384 + (mm - 1) * 32 + dd;
}

/* Handles all data encodation for DataBar Expanded, section 7.2.5 of ISO/IEC 24724:2011 */
static int dbar_exp_binary_string(struct zint_symbol *symbol, const unsigned char source[], const int length,
                char binary_string[], int *p_cols_per_row, const int max_rows, int *p_bp) {
    int encoding_method, i, j, read_posn, mode = NUMERIC;
    char last_digit = '\0';
    int symbol_characters, characters_per_row = *p_cols_per_row * 2;
    int min_cols_per_row = 0;
    char *general_field = (char *) z_alloca(length + 1);
    int bp = *p_bp;
    int remainder;
    int cdf_bp_start; /* Compressed data field start - debug only */
    const int debug_print = symbol->debug & ZINT_DEBUG_PRINT;

    if (length > 77) { /* ISO/IEC 24724:2011 4.2.d.2 */
        /* Caught below anyway but catch here also for better feedback */
        return errtxtf(ZINT_ERROR_TOO_LONG, symbol, 378, "Processed input length %d too long (maximum 77)", length);
    }

    /* Decide whether a compressed data field is required and if so what
       method to use - method 2 = no compressed data field */

    if (length >= 16 && source[0] == '0' && source[1] == '1') {
        /* (01) and other AIs */
        encoding_method = 1;
        if (debug_print) fputs("Choosing Method 1\n", stdout);
    } else {
        /* Any AIs */
        encoding_method = 2;
        if (debug_print) fputs("Choosing Method 2\n", stdout);
    }

    if (length >= 20 && encoding_method == 1 && source[2] == '9' && source[16] == '3') {
        /* Possibly encoding method > 2 */

        if (debug_print) fputs("Checking for other methods\n", stdout);

        if (length >= 26 && source[17] == '1' && source[18] == '0') {
            /* Methods 3, 7, 9, 11 and 13 */

            /* (01) and (310x) */
            int weight = to_int(source + 20, 6);

            /* Maximum weight = 99999 for 7 to 14 (ISO/IEC 24724:2011 7.2.5.4.4) */
            if (weight >= 0 && weight <= 99999) {

                if (length == 26) {
                    if (source[19] == '3' && weight <= 32767) { /* In grams, max 32.767 kilos */
                        /* (01) and (3103) */
                        encoding_method = 3;
                    } else {
                        /* (01), (310x) - use method 7 with dummy date 38400 */
                        encoding_method = 7;
                    }

                } else if (length == 34 && source[26] == '1'
                            && (source[27] == '1' || source[27] == '3' || source[27] == '5' || source[27] == '7')
                            && dbar_exp_date(source, length, 28) >= 0) {

                    /* (01), (310x) and (11) - metric weight and production date */
                    /* (01), (310x) and (13) - metric weight and packaging date */
                    /* (01), (310x) and (15) - metric weight and "best before" date */
                    /* (01), (310x) and (17) - metric weight and expiration date */
                    encoding_method = 6 + (source[27] - '0');
                }
            }

        } else if (length >= 26 && source[17] == '2' && source[18] == '0') {
            /* Methods 4, 8, 10, 12 and 14 */

            /* (01) and (320x) */
            int weight = to_int(source + 20, 6);

            /* Maximum weight = 99999 for 7 to 14 (ISO/IEC 24724:2011 7.2.5.4.4) */
            if (weight >= 0 && weight <= 99999) {

                /* (3202) in 0.01 pounds, max 99.99 pounds; (3203) in 0.001 pounds, max 22.767 pounds */
                if (length == 26) {
                    if ((source[19] == '2' && weight <= 9999) || (source[19] == '3' && weight <= 22767)) {
                        /* (01) and (3202)/(3203) */
                        encoding_method = 4;
                    } else {
                        /* (01), (320x) - use method 8 with dummy date 38400 */
                        encoding_method = 8;
                    }

                } else if (length == 34 && source[26] == '1'
                            && (source[27] == '1' || source[27] == '3' || source[27] == '5' || source[27] == '7')
                            && dbar_exp_date(source, length, 28) >= 0) {

                    /* (01), (320x) and (11) - English weight and production date */
                    /* (01), (320x) and (13) - English weight and packaging date */
                    /* (01), (320x) and (15) - English weight and "best before" date */
                    /* (01), (320x) and (17) - English weight and expiration date */
                    encoding_method = 7 + (source[27] - '0');
                }
            }

        } else if (source[17] == '9' && (source[19] >= '0' && source[19] <= '3')) {
            /* Methods 5 and 6 */
            if (source[18] == '2') {
                /* (01) and (392x) */
                encoding_method = 5;
            } else if (source[18] == '3' && to_int(source + 20, 3) >= 0) { /* Check 3-digit currency string */
                /* (01) and (393x) */
                encoding_method = 6;
            }
        }

        if (debug_print && encoding_method != 1) printf("Now using method %d\n", encoding_method);
    }

    switch (encoding_method) { /* Encoding method - Table 10 */
        case 1:
            bp = bin_append_posn(4, 3, binary_string, bp); /* "1XX" */
            read_posn = 16;
            break;
        case 2:
            bp = bin_append_posn(0, 4, binary_string, bp); /* "00XX" */
            read_posn = 0;
            break;
        case 3: /* 0100 */
        case 4: /* 0101 */
            bp = bin_append_posn(4 + (encoding_method - 3), 4, binary_string, bp);
            read_posn = 26;
            break;
        case 5:
            bp = bin_append_posn(0x30, 7, binary_string, bp); /* "01100XX" */
            read_posn = 20;
            break;
        case 6:
            bp = bin_append_posn(0x34, 7, binary_string, bp); /* "01101XX" */
            read_posn = 23;
            break;
        default: /* Modes 7 to 14 */
            bp = bin_append_posn(56 + (encoding_method - 7), 7, binary_string, bp);
            read_posn = length; /* 34 or 26 */
            break;
    }
    if (debug_print) printf("Setting binary = %.*s\n", bp, binary_string);

    /* Variable length symbol bit field is just given a place holder (XX) for the time being */

    /* Verify that the data to be placed in the compressed data field is all numeric data
       before carrying out compression */
    for (i = 0; i < read_posn; i++) {
        if (!z_isdigit(source[i]) && source[i] != '\x1D') {
            /* Something is wrong */
            return errtxt(ZINT_ERROR_INVALID_DATA, symbol, 385,
                            "Invalid character in Compressed Field data (digits only)");
        }
    }

    /* Now encode the compressed data field */

    if (debug_print) fputs("Proceeding to encode data\n", stdout);
    cdf_bp_start = bp; /* Debug use only */

    if (encoding_method == 1) {
        /* Encoding method field "1" - general item identification data */

        bp = bin_append_posn(ctoi(source[2]), 4, binary_string, bp); /* Leading digit after stripped "01" */

        for (i = 3; i < 15; i += 3) { /* Next 12 digits, excluding final check digit */
            bp = bin_append_posn(to_int(source + i, 3), 10, binary_string, bp);
        }

    } else if (encoding_method == 3 || encoding_method == 4) {
        /* Encoding method field "0100" - variable weight item (0,001 kilogram increments) */
        /* Encoding method field "0101" - variable weight item (0,01 or 0,001 pound increment) */

        for (i = 3; i < 15; i += 3) { /* Leading "019" stripped, and final check digit excluded */
            bp = bin_append_posn(to_int(source + i, 3), 10, binary_string, bp);
        }

        if (encoding_method == 4 && source[19] == '3') {
            bp = bin_append_posn(to_int(source + 20, 6) + 10000, 15, binary_string, bp);
        } else {
            bp = bin_append_posn(to_int(source + 20, 6), 15, binary_string, bp);
        }

    } else if (encoding_method == 5 || encoding_method == 6) {
        /* Encoding method "01100" - variable measure item and price */
        /* Encoding method "01101" - variable measure item and price with ISO 4217 Currency Code */

        for (i = 3; i < 15; i += 3) { /* Leading "019" stripped, and final check digit excluded */
            bp = bin_append_posn(to_int(source + i, 3), 10, binary_string, bp);
        }

        bp = bin_append_posn(source[19] - '0', 2, binary_string, bp); /* 0-3 x of 392x/393x */

        if (encoding_method == 6) {
            bp = bin_append_posn(to_int(source + 20, 3), 10, binary_string, bp); /* 3-digit currency */
        }

    } else if (encoding_method >= 7 && encoding_method <= 14) {
        /* Encoding method fields "0111000" through "0111111" - variable weight item plus date */
        int group_val;
        unsigned char weight_str[7];

        for (i = 3; i < 15; i += 3) { /* Leading "019" stripped, and final check digit excluded */
            bp = bin_append_posn(to_int(source + i, 3), 10, binary_string, bp);
        }

        weight_str[0] = source[19]; /* 0-9 x of 310x/320x */

        for (i = 1; i < 6; i++) { /* Leading "0" of weight excluded */
            weight_str[i] = source[20 + i];
        }
        weight_str[6] = '\0';

        bp = bin_append_posn(to_int(weight_str, 6), 20, binary_string, bp);

        if (length == 34) {
            /* Date information is included */
            group_val = dbar_exp_date(source, length, 28);
        } else {
            group_val = 38400;
        }

        bp = bin_append_posn((int) group_val, 16, binary_string, bp);
    }

    if (debug_print && bp > cdf_bp_start) {
        printf("Compressed data field (%d) = %.*s\n", bp - cdf_bp_start, bp - cdf_bp_start,
            binary_string + cdf_bp_start);
    }

    /* The compressed data field has been processed if appropriate - the
       rest of the data (if any) goes into a general-purpose data compaction field */

    j = 0;
    for (i = read_posn; i < length; i++) {
        general_field[j++] = source[i];
    }

    if (debug_print) printf("General field data (%d): %.*s\n", j, j, general_field);

    if (j != 0) { /* If general field not empty */
        general_field[j] = '\0';
        if (!general_field_encode(general_field, j, &mode, &last_digit, binary_string, &bp)) {
            /* Will happen if character not in CSET 82 + space */
            return errtxt(ZINT_ERROR_INVALID_DATA, symbol, 386, "Invalid character in General Field data");
        }
    }

    if (debug_print) printf("Resultant binary (%d): %.*s\n", bp, bp, binary_string);

    remainder = 12 - (bp % 12);
    if (remainder == 12) {
        remainder = 0;
    }
    symbol_characters = ((bp + remainder) / 12) + 1;

    if (max_rows) {
        min_cols_per_row = ((symbol_characters + 1) / 2 + max_rows - 1) / max_rows;
        if (min_cols_per_row > *p_cols_per_row) {
            characters_per_row = min_cols_per_row * 2;
        }
    }

    if (characters_per_row && (symbol_characters % characters_per_row) == 1) { /* DBAR_EXPSTK */
        symbol_characters++;
    }

    if (symbol_characters < 4) {
        symbol_characters = 4;
    }

    remainder = (12 * (symbol_characters - 1)) - bp;

    if (last_digit) {
        /* There is still one more numeric digit to encode */
        if (debug_print) fputs("Adding extra (odd) numeric digit\n", stdout);

        if (remainder >= 4 && remainder <= 6) {
            bp = bin_append_posn(ctoi(last_digit) + 1, 4, binary_string, bp); /* 7.2.5.5.1 (c) (2) */
        } else {
            /* 7.2.5.5.1 (c) (1) */
            bp = bin_append_posn(ctoi(last_digit) * 11 + 10 /*FNC1*/ + 8, 7, binary_string, bp);
        }

        remainder = 12 - (bp % 12);
        if (remainder == 12) {
            remainder = 0;
        }
        symbol_characters = ((bp + remainder) / 12) + 1;

        if (max_rows) {
            min_cols_per_row = ((symbol_characters + 1) / 2 + max_rows - 1) / max_rows;
            if (min_cols_per_row > *p_cols_per_row) {
                characters_per_row = min_cols_per_row * 2;
            }
        }

        if (characters_per_row && (symbol_characters % characters_per_row) == 1) { /* DBAR_EXPSTK */
            symbol_characters++;
        }

        if (symbol_characters < 4) {
            symbol_characters = 4;
        }

        remainder = (12 * (symbol_characters - 1)) - bp;

        if (debug_print) printf(" Expanded binary (%d): %.*s\n", bp, bp, binary_string);
    }

    if (bp > 252) { /* 252 = (21 * 12) */
        return errtxtf(ZINT_ERROR_TOO_LONG, symbol, 387, /* TODO: Better error message */
                        "Input too long, requires %d symbol characters (maximum 21)", (bp + 11) / 12);
    }

    if (min_cols_per_row && min_cols_per_row > *p_cols_per_row) {
        *p_cols_per_row = min_cols_per_row;
    }

    /* Now add padding to binary string (7.2.5.5.4) */
    i = remainder;
    if (mode == NUMERIC) {
        bp = bin_append_posn(0, 4, binary_string, bp); /* "0000" */
        i -= 4;
    }
    for (; i > 0; i -= 5) {
        bp = bin_append_posn(4, 5, binary_string, bp); /* "00100" */
    }

    if (encoding_method == 1 || encoding_method == 2 || encoding_method == 5 || encoding_method == 6) {
        /* Patch variable length symbol bit field */
        const char variable_bit1 = symbol_characters & 1 ? '1' : '0';
        const char variable_bit2 = symbol_characters > 14 ? '1' : '0';

        if (encoding_method == 1) {
            binary_string[2] = variable_bit1;
            binary_string[3] = variable_bit2;
        } else if (encoding_method == 2) {
            binary_string[3] = variable_bit1;
            binary_string[4] = variable_bit2;
        } else {
            binary_string[6] = variable_bit1;
            binary_string[7] = variable_bit2;
        }
    }

    if (debug_print) {
        printf("    Final binary (%d): %.*s\n    Symbol chars: %d, Remainder: %d\n",
                bp, bp, binary_string, symbol_characters, remainder);
    }

    *p_bp = bp;

    return 0;
}

/* Separator for DataBar Expanded Stacked and DataBar Expanded Composite */
static void dbar_exp_separator(struct zint_symbol *symbol, int width, const int cols, const int separator_row,
                const int above_below, const int special_case_row, const int left_to_right, const int odd_last_row,
                int *p_v2_latch) {
    int i, i_start, i_end, j, k;
    int module_row = separator_row + above_below;
    int v2_latch = p_v2_latch ? *p_v2_latch : 0;
    int space_latch = 0;

    for (j = 4 + special_case_row, width -= 4; j < width; j++) {
        if (module_is_set(symbol, module_row, j)) {
            unset_module(symbol, separator_row, j);
        } else {
            set_module(symbol, separator_row, j);
        }
    }

    /* Finder adjustment */
    for (j = 0; j < cols; j++) {
        /* 49 == data (17) + finder (15) + data(17) triplet, 19 == 2 (guard) + 17 (initial check/data character) */
        k = (49 * j) + 19 + special_case_row;
        if (left_to_right) {
            /* Last 13 modules of version 2 finder and first 13 modules of version 1 finder */
            i_start = v2_latch ? 2 : 0;
            i_end = v2_latch ? 15 : 13;
            for (i = i_start; i < i_end; i++) {
                if (module_is_set(symbol, module_row, i + k)) {
                    unset_module(symbol, separator_row, i + k);
                    space_latch = 0;
                } else {
                    if (space_latch) {
                        unset_module(symbol, separator_row, i + k);
                    } else {
                        set_module(symbol, separator_row, i + k);
                    }
                    space_latch = !space_latch;
                }
            }
        } else {
            if (odd_last_row) {
                k -= 17; /* No data char at beginning of row, i.e. ends with finder */
            }
            /* First 13 modules of version 1 finder and last 13 modules of version 2 finder */
            i_start = v2_latch ? 14 : 12;
            i_end = v2_latch ? 2 : 0;
            for (i = i_start; i >= i_end; i--) {
                if (module_is_set(symbol, module_row, i + k)) {
                    unset_module(symbol, separator_row, i + k);
                    space_latch = 0;
                } else {
                    if (space_latch) {
                        unset_module(symbol, separator_row, i + k);
                    } else {
                        set_module(symbol, separator_row, i + k);
                    }
                    space_latch = !space_latch;
                }
            }
        }
        v2_latch = !v2_latch;
    }

    if (p_v2_latch && above_below == -1) { /* Only set if below */
        *p_v2_latch = v2_latch;
    }
}

/* Set HRT for DataBar Expanded */
static void dbar_exp_hrt(struct zint_symbol *symbol, unsigned char source[], const int length) {

    /* Max possible length is 77 digits so will fit */
    if (symbol->input_mode & GS1PARENS_MODE) {
        hrt_cpy_nochk(symbol, source, length);
    } else {
        hrt_conv_gs1_brackets_nochk(symbol, source, length);
    }
}

/* Return DataBar Expanded group (-1) */
static int dbar_exp_group(const int val) {
    int i;
    for (i = 0; i < ARRAY_SIZE(dbar_exp_g_sum) - 1; i++) {
        if (val < dbar_exp_g_sum[i + 1]) {
            return i;
        }
    }
    return i;
}

/* GS1 DataBar Expanded, setting linkage for composite if `cc_rows` set */
INTERNAL int dbar_exp_cc(struct zint_symbol *symbol, unsigned char source[], int length, const int cc_rows) {
    int error_number, warn_number;
    int i, j, p, codeblocks, data_chars, symbol_chars, group;
    int char_widths[21][8], checksum, check_widths[8];
    int check_char, odd, even, elements[235], pattern_width, reader, writer;
    int separator_row = 0;
    int bp = 0;
    int cols_per_row = 0;
    int max_rows = 0;
    int stack_rows = 1;
    unsigned char *reduced = (unsigned char *) z_alloca(length + 1);
    int reduced_length;
    /* Allow for 8 bits + 5-bit latch per char + 200 bits overhead/padding */
    char *binary_string = (char *) z_alloca(13 * length + 200 + 1);
    const int raw_text = symbol->output_options & BARCODE_RAW_TEXT;
    const int debug_print = symbol->debug & ZINT_DEBUG_PRINT;

    error_number = gs1_verify(symbol, source, &length, reduced, &reduced_length);
    if (error_number >= ZINT_ERROR) {
        return error_number;
    }
    warn_number = error_number;

    if (debug_print) {
        printf("Reduced (%d): %s\n", reduced_length, reduced);
    }

    if (symbol->symbology != BARCODE_DBAR_EXP) {
        symbol->rows = 0; /* Stacked (and composites) are not stackable */
    }

    if (symbol->symbology == BARCODE_DBAR_EXP_CC || symbol->symbology == BARCODE_DBAR_EXPSTK_CC) {
        /* Make space for a composite separator pattern */
        separator_row = symbol->rows++;
        symbol->row_height[separator_row] = 1;
    }

    if (cc_rows) { /* The "component linkage" flag */
        binary_string[bp++] = '1';
    } else {
        binary_string[bp++] = '0';
    }

    if (symbol->symbology == BARCODE_DBAR_EXPSTK || symbol->symbology == BARCODE_DBAR_EXPSTK_CC) {
        cols_per_row = 2; /* Default */
        if (symbol->option_2 >= 1 && symbol->option_2 <= 11) {
            cols_per_row = symbol->option_2;
            if (cc_rows && (cols_per_row == 1)) {
                /* "There shall be a minimum of four symbol characters in the
                   first row of an RSS Expanded Stacked symbol when it is the linear
                   component of an EAN.UCC Composite symbol." */
                cols_per_row = 2;
            }
        } else if (symbol->option_3 >= 2 && symbol->option_3 <= 11) {
            max_rows = symbol->option_3;
        }
    }

    error_number = dbar_exp_binary_string(symbol, reduced, reduced_length, binary_string, &cols_per_row, max_rows,
                                            &bp);
    if (error_number != 0) {
        return error_number;
    }

    if (symbol->symbology == BARCODE_DBAR_EXPSTK || symbol->symbology == BARCODE_DBAR_EXPSTK_CC) {
        /* Feedback options */
        symbol->option_2 = cols_per_row;
        symbol->option_3 = max_rows;
    }

    data_chars = bp / 12;
    symbol_chars = data_chars + 1; /* Plus check char */

    if (debug_print) fputs("Data:", stdout);
    for (i = 0; i < data_chars; i++) {
        const int k = i * 12;
        int vs = 0;
        for (j = 0; j < 12; j++) {
            if (binary_string[k + j] == '1') {
                vs |= (0x800 >> j);
            }
        }
        if (debug_print) printf("%s%d", i == 0 || (i & 1) ? " " : ",", vs);

        group = dbar_exp_group(vs);
        odd = (vs - dbar_exp_g_sum[group]) / dbar_exp_t_even[group];
        even = (vs - dbar_exp_g_sum[group]) % dbar_exp_t_even[group];

        dbar_widths(char_widths[i], odd, even, dbar_exp_modules[group], 17 - dbar_exp_modules[group], 4 /*elements*/,
                    dbar_exp_widest[group], 1 /*noNarrow*/);
    }
    if (debug_print) fputc('\n', stdout);

    /* 7.2.6 Check character */
    /* The checksum value is equal to the mod 211 residue of the weighted sum of the widths of the
       elements in the data characters. */
    checksum = 0;
    for (i = 0; i < data_chars; i++) {
        const int row = dbar_exp_weight_rows[(data_chars - 2) / 2][i];
        for (j = 0; j < 8; j++) {
            checksum += char_widths[i][j] * dbar_exp_checksum_weight[row][j];

        }
    }

    check_char = 211 * (symbol_chars - 4) + checksum % 211;

    if (debug_print) {
        printf("Data chars: %d, Check char: %d\n", data_chars, check_char);
    }

    group = dbar_exp_group(check_char);

    odd = (check_char - dbar_exp_g_sum[group]) / dbar_exp_t_even[group];
    even = (check_char - dbar_exp_g_sum[group]) % dbar_exp_t_even[group];

    dbar_widths(check_widths, odd, even, dbar_exp_modules[group], 17 - dbar_exp_modules[group], 4 /*elements*/,
                dbar_exp_widest[group], 1 /*noNarrow*/);

    /* Initialise element array */
    codeblocks = (symbol_chars + 1) / 2;
    pattern_width = codeblocks * 5 + symbol_chars * 8 + 4;
    memset(elements, 0, sizeof(int) * pattern_width);

    /* Put finder patterns in element array */
    p = (symbol_chars - 1) / 2 - 1;
    for (i = 0; i < codeblocks; i++) {
        const int k = dbar_exp_finder_sequence[p][i] - 1;
        for (j = 0; j < 5; j++) {
            elements[(21 * i) + j + 10] = dbar_exp_finder_pattern[k][j];
        }
    }

    /* Put check character in element array */
    for (i = 0; i < 8; i++) {
        elements[i + 2] = check_widths[i];
    }

    /* Put forward reading data characters in element array */
    for (i = 1; i < data_chars; i += 2) {
        const int k = ((i - 1) / 2) * 21 + 23;
        for (j = 0; j < 8; j++) {
            elements[k + j] = char_widths[i][j];
        }
    }

    /* Put reversed data characters in element array */
    for (i = 0; i < data_chars; i += 2) {
        const int k = (i / 2) * 21 + 15;
        for (j = 0; j < 8; j++) {
            elements[k + j] = char_widths[i][7 - j];
        }
    }

    if (symbol->symbology == BARCODE_DBAR_EXP || symbol->symbology == BARCODE_DBAR_EXP_CC) {
        /* Copy elements into symbol */

        elements[0] = 1; /* Left guard */
        elements[1] = 1;

        elements[pattern_width - 2] = 1; /* Right guard */
        elements[pattern_width - 1] = 1;

        writer = dbar_expand(symbol, 0 /*writer*/, 0 /*latch*/, elements, 0 /*start*/, pattern_width /*end*/);
        if (symbol->width < writer) {
            symbol->width = writer;
        }
        symbol->rows++;

        dbar_exp_hrt(symbol, source, length);

        if (raw_text && rt_cpy(symbol, reduced, reduced_length)) {
            return ZINT_ERROR_MEMORY; /* `rt_cpy()` only fails with OOM */
        }

    } else {
        /* BARCODE_DBAR_EXPSTK || BARCODE_DBAR_EXPSTK_CC */
        int current_row, current_block, left_to_right;
        int v2_latch = 0;

        /* Bug corrected: Character missing for message
         * [01]90614141999996[10]1234222222222221
         * Patch by Daniel Frede
         */

        stack_rows = codeblocks / cols_per_row;
        if (codeblocks % cols_per_row > 0) {
            stack_rows++;
        }

        current_block = 0;
        for (current_row = 1; current_row <= stack_rows; current_row++) {
            int elements_in_sub = 2;
            int sub_elements[235] = {0};
            int special_case_row = 0;
            int num_columns;
            int latch;

            /* Number of columns in current row */
            num_columns = current_row * cols_per_row > codeblocks ? codeblocks - current_block : cols_per_row;

            /* Row Start */
            sub_elements[0] = 1; /* Left guard */
            sub_elements[1] = 1;

            /* If last row and is partial and even-numbered, and have even columns (segment pairs),
               and odd number of finders (== odd number of columns) */
            if (current_row == stack_rows && num_columns != cols_per_row && !(current_row & 1)
                    && !(cols_per_row & 1) && (num_columns & 1)) {
                /* Special case bottom row */
                special_case_row = 1;
                sub_elements[0] = 2; /* Extra space (latch set below) */
            }

            /* If odd number of columns or current row odd-numbered or special case last row then left-to-right,
               else right-to-left */
            left_to_right = (cols_per_row & 1) || (current_row & 1) || special_case_row;

            if (debug_print) {
                if (current_row == stack_rows) {
                    printf("Last row: number of columns: %d / %d, left to right: %d, special case: %d\n",
                            num_columns, cols_per_row, left_to_right, special_case_row);
                }
            }

            /* Row Data */
            reader = 0;
            do {
                i = 2 + (current_block * 21);
                for (j = 0; j < 21; j++) {
                    if (i + j < pattern_width) {
                        if (left_to_right) {
                            sub_elements[j + (reader * 21) + 2] = elements[i + j];
                        } else {
                            sub_elements[(20 - j) + (num_columns - 1 - reader) * 21 + 2] = elements[i + j];
                        }
                    }
                    elements_in_sub++;
                }
                reader++;
                current_block++;
            } while ((reader < cols_per_row) && (current_block < codeblocks));

            /* Row Stop */
            sub_elements[elements_in_sub++] = 1; /* Right guard */
            sub_elements[elements_in_sub++] = 1;

            latch = !((current_row & 1) || special_case_row);

            writer = dbar_expand(symbol, 0 /*writer*/, latch, sub_elements, 0 /*start*/, elements_in_sub /*end*/);
            if (symbol->width < writer) {
                symbol->width = writer;
            }

            if (current_row != 1) {
                int odd_last_row = (current_row == stack_rows) && (data_chars % 2 == 0);

                /* Middle separator pattern (above current row) */
                for (j = 5; j < (49 * cols_per_row); j += 2) {
                    set_module(symbol, symbol->rows - 2, j);
                }
                symbol->row_height[symbol->rows - 2] = 1;

                /* Bottom separator pattern (above current row) */
                dbar_exp_separator(symbol, writer, reader, symbol->rows - 1, 1 /*above*/, special_case_row,
                                    left_to_right, odd_last_row, &v2_latch);
                symbol->row_height[symbol->rows - 1] = 1;
            }

            if (current_row != stack_rows) {
                /* Top separator pattern (below current row) */
                dbar_exp_separator(symbol, writer, reader, symbol->rows + 1, -1 /*below*/, 0 /*special_case_row*/,
                                    left_to_right, 0 /*odd_last_row*/, &v2_latch);
                symbol->row_height[symbol->rows + 1] = 1;
            }

            symbol->rows += 4;
        }
        symbol->rows -= 3;

        if (raw_text && rt_cpy(symbol, reduced, reduced_length)) {
            return ZINT_ERROR_MEMORY; /* `rt_cpy()` only fails with OOM */
        }
    }

    if (symbol->symbology == BARCODE_DBAR_EXP_CC || symbol->symbology == BARCODE_DBAR_EXPSTK_CC) {
        /* Composite separator */
        dbar_exp_separator(symbol, symbol->width, 4, separator_row, 1 /*above*/, 0 /*special_case_row*/,
                            1 /*left_to_right*/, 0 /*odd_last_row*/, NULL);
    }

    /* DataBar Expanded ISO/IEC 24724:2011 7.2.1 and DataBar Expanded Stacked ISO/IEC 24724:2011 7.2.8
       34X min per row */
    if (symbol->symbology == BARCODE_DBAR_EXP_CC || symbol->symbology == BARCODE_DBAR_EXPSTK_CC) {
        symbol->height = symbol->height ? 34.0f : 34.0f * stack_rows; /* Pass back min row or default height */
    } else {
        if (symbol->output_options & COMPLIANT_HEIGHT) {
            if (warn_number == 0) {
                warn_number = set_height(symbol, 34.0f, 34.0f * stack_rows, 0.0f, 0 /*no_errtxt*/);
            } else {
                (void) set_height(symbol, 34.0f, 34.0f * stack_rows, 0.0f, 1 /*no_errtxt*/);
            }
        } else {
            (void) set_height(symbol, 0.0f, 34.0f * stack_rows, 0.0f, 1 /*no_errtxt*/);
        }
    }

    return warn_number;
}

/* GS1 DataBar Expanded */
INTERNAL int dbar_exp(struct zint_symbol *symbol, unsigned char source[], int length) {
    return dbar_exp_cc(symbol, source, length, 0 /*cc_rows*/);
}

/* vim: set ts=4 sw=4 et : */
