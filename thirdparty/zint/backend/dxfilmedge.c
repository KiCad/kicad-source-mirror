/* dxfilmedge.c - Handles DX Film Edge symbology */
/*
    libzint - the open source barcode library
    Copyright (C) 2024-2025 Antoine Merino <antoine.merino.dev@gmail.com>

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

/* DX Film Edge Barcode is used on 35mm and APS films:
 * https://en.wikipedia.org/wiki/DX_encoding
 *
 * A little information about decoding this symbology can be found at
 * https://www.merinorus.com/blog/identifying-manufacturer-35-mm-films/
 *
 * Partial specification and history can be found on this Kodak patent:
 * https://patents.google.com/patent/US4965628A/en
 */

#include <assert.h>
#include <stdio.h>
#include "common.h"

#define DX_DEBUG_STR_LEN    20
/* Max length of the DX info part. Eg: "018500", "150-10" */
#define DX_MAX_DX_INFO_LENGTH       6
#define DX_MAX_DX_INFO_MAX_STR      "6" /* String version of above */
/* Max length of the frame info part. Eg: "00A", "23A" */
#define DX_MAX_FRAME_INFO_LENGTH    3
#define DX_MAX_FRAME_INFO_MAX_STR   "3" /* String version of above */

static int dx_parse_code(struct zint_symbol *symbol, const unsigned char *source, const int length,
            char *binary_output, int *output_length, int *has_frame_info) {
    int i;
    int parity_bit = 0;
    int dx_code_1 = -1, dx_code_2 = -1, frame_number = -1;
    int bp;
    char half_frame_flag = '\0';
    char dx_info[DX_MAX_DX_INFO_LENGTH + 1] = {0};
    char frame_info[DX_MAX_FRAME_INFO_LENGTH + 1] = {0};
    int dx_length;
    const int raw_text = symbol->output_options & BARCODE_RAW_TEXT;
    const int debug_print = symbol->debug & ZINT_DEBUG_PRINT;

    *has_frame_info = 0;

    /* All codes should start with a digit*/
    if (!z_isdigit(source[0])) {
        return errtxtf(ZINT_ERROR_INVALID_DATA, symbol, 970,
                        "Invalid first character \"%c\", DX code should start with a number", source[0]);
    }

    /* Check if there is the '/' separator, which indicates the frame number is present. */
    dx_length = posn((const char *) source, '/');
    if (dx_length != -1) {
        /* Split the DX information from the frame number */
        const char *frame_start;
        int frame_info_len;
        if (dx_length > DX_MAX_DX_INFO_LENGTH) {
            return errtxtf(ZINT_ERROR_INVALID_DATA, symbol, 971,
                            "DX information length %d too long (maximum " DX_MAX_DX_INFO_MAX_STR ")", dx_length);
        }
        memcpy(dx_info, source, dx_length);
        frame_start = (const char *) source + dx_length + 1;
        frame_info_len = (int) strlen(frame_start);
        if (frame_info_len > DX_MAX_FRAME_INFO_LENGTH) {
            return errtxtf(ZINT_ERROR_INVALID_DATA, symbol, 972,
                    "Frame number part length %d too long (maximum " DX_MAX_FRAME_INFO_MAX_STR ")", frame_info_len);
        }
        memcpy(frame_info, frame_start, frame_info_len);
        *has_frame_info = 1;
        to_upper((unsigned char *) frame_info, frame_info_len);
        if (not_sane(IS_UPR_F | IS_NUM_F | IS_MNS_F, (const unsigned char *) frame_info, frame_info_len)) {
            return errtxtf(ZINT_ERROR_INVALID_DATA, symbol, 973,
                            "Frame number \"%s\" is invalid (expected digits, optionally followed by a single \"A\")",
                            frame_info);
        }
    } else {
        /* No "/" found, store the entire input in dx_info */
        dx_length = length;
        if (dx_length > DX_MAX_DX_INFO_LENGTH) {
            return errtxtf(ZINT_ERROR_INVALID_DATA, symbol, 974,
                            "DX information length %d too long (maximum " DX_MAX_DX_INFO_MAX_STR ")", dx_length);
        }
        memcpy(dx_info, source, dx_length);
    }

    if ((i = not_sane(IS_NUM_F | IS_MNS_F, (const unsigned char *) dx_info, dx_length))) {
        return errtxtf(ZINT_ERROR_INVALID_DATA, symbol, 975,
                        "Invalid character at position %d in DX info (digits and \"-\" character only)", i);
    }

    if (debug_print) printf("\nDX info part: \"%s\", Frame info part: \"%s\"\n", dx_info, frame_info);
    /* Parse the DX information */
    if (strchr(dx_info, '-')) {
        /* DX code parts 1 and 2 are given directly, separated by a '-'. Eg: "79-7" */
        if (debug_print) printf("DX code 1 and 2 are separated by a dash \"-\"\n");
        if (chr_cnt((const unsigned char *) dx_info, dx_length, '-') > 1) {
            return errtxt(ZINT_ERROR_INVALID_DATA, symbol, 976,
                            "The \"-\" is used to separate DX parts 1 and 2, and should be used no more than once");
        }
        if (sscanf(dx_info, "%d-%d", &dx_code_1, &dx_code_2) < 2) {
            return errtxt(ZINT_ERROR_INVALID_DATA, symbol, 977,
                            "Wrong format for DX parts 1 and 2 (expected format: NNN-NN, digits)");
        }
        if (dx_code_1 <= 0 || dx_code_1 > 127) {
            return errtxtf(ZINT_ERROR_INVALID_DATA, symbol, 978, "DX part 1 \"%d\" out of range (1 to 127)",
                            dx_code_1);
        }
        if (dx_code_2 < 0 || dx_code_2 > 15) {
            return errtxtf(ZINT_ERROR_INVALID_DATA, symbol, 979, "DX part 2 \"%d\" out of range (0 to 15)",
                            dx_code_2);
        }
    } else {
        int dx_extract;
        /* DX format is either 4 digits (DX Extract, eg: 1271) or 6 digits (DX Full, eg: 012710) */
        if (debug_print) printf("No \"-\" separator, computing from DX Extract (4 digits) or DX Full (6 digits)\n");
        assert(dx_length <= 6); /* I.e. DX_MAX_DX_INFO_LENGTH, guaranteed above */
        if (dx_length == 5) {
            return errtxtf(ZINT_ERROR_INVALID_DATA, symbol, 980,
                    "DX number \"%s\" is incorrect; expected 4 digits (DX extract) or 6 digits (DX full)", dx_info);
        }
        if (dx_length == 6) {
            if (debug_print) {
                printf("DX full format detected: %s. Removing the first and the last characters.\n", dx_info);
            }
            /* Convert DX Full to DX Extract (remove first and last character) */
            for (i = 0; i <= 3; ++i) {
                dx_info[i] = dx_info[i + 1];
            }
            dx_length = 4;
        }
        /* Compute the DX parts 1 and 2 from the DX extract */
        dx_extract = to_int((const unsigned char *) dx_info, dx_length);
        assert(dx_extract != -1); /* All digits (no hyphen) & length 6 max - can't fail */
        if (dx_extract < 16 || dx_extract > 2047) {
            return errtxtf(ZINT_ERROR_INVALID_DATA, symbol, 981, "DX extract \"%d\" out of range (16 to 2047)",
                            dx_extract);
        }
        if (debug_print) printf("Computed DX extract: %04d\n", dx_extract);
        dx_code_1 = dx_extract >> 4;
        dx_code_2 = dx_extract & 0xF;
    }

    if (debug_print) {
        printf("%-*s%d\n", DX_DEBUG_STR_LEN, "DX code 1:", dx_code_1);
        printf("%-*s%d\n", DX_DEBUG_STR_LEN, "DX code 2:", dx_code_2);
    }

    if (*has_frame_info) {
        int ret_sscanf, n;
        if (strlen(frame_info) < 1) {
            return errtxtf(ZINT_ERROR_INVALID_DATA, symbol, 982,
                            "Frame number indicator \"/\" at position %d, but frame number is empty",
                            dx_length + 1);
        }
        /* Some frame numbers are special values, convert them their equivalent number */
        if (strcmp(frame_info, "S") == 0 || strcmp(frame_info, "X") == 0) {
            memcpy(frame_info, "62", 3); /* Include terminating NUL */
        } else if (strcmp(frame_info, "SA") == 0 || strcmp(frame_info, "XA") == 0) {
            memcpy(frame_info, "62A", 4);
        } else if (strcmp(frame_info, "K") == 0 || strcmp(frame_info, "00") == 0) {
            memcpy(frame_info, "63", 3);
        } else if (strcmp(frame_info, "KA") == 0 || strcmp(frame_info, "00A") == 0) {
            memcpy(frame_info, "63A", 4);
        } else if (strcmp(frame_info, "F") == 0) {
            memcpy(frame_info, "0", 2);
        } else if (strcmp(frame_info, "FA") == 0) {
            memcpy(frame_info, "0A", 3);
        }

        ret_sscanf = sscanf(frame_info, "%d%c%n", &frame_number, &half_frame_flag, &n);
        if (ret_sscanf < 1 || (ret_sscanf == 2 && frame_info[n] != '\0')) {
            return errtxtf(ZINT_ERROR_INVALID_DATA, symbol, 983,
                            "Frame number \"%s\" is invalid (expected digits, optionally followed by a single \"A\")",
                            frame_info);
        }
        if (frame_number < 0 || frame_number > 63) {
            return errtxtf(ZINT_ERROR_INVALID_DATA, symbol, 984, "Frame number \"%d\" out of range (0 to 63)",
                            frame_number);
        }
        if (debug_print) {
            printf("%-*s%d\n", DX_DEBUG_STR_LEN, "Frame number:", frame_number);
        }
    }

    /* Build the binary output */
    memcpy(binary_output, "101010", 6); /* Start pattern */
    bp = bin_append_posn(dx_code_1, 7, binary_output, 6);
    binary_output[bp++] = '0'; /* Separator between DX part 1 and DX part 2 */
    bp = bin_append_posn(dx_code_2, 4, binary_output, bp);
    if (*has_frame_info) {
        bp = bin_append_posn(frame_number, 6, binary_output, bp);
        to_upper((unsigned char *) &half_frame_flag, 1);
        if (half_frame_flag == 'A') {
            if (debug_print) printf("%-*s'%c'\t-> 1\n", DX_DEBUG_STR_LEN, "Half frame flag:", half_frame_flag);
            binary_output[bp++] = '1'; /* Half-frame is set */
        } else {
            if (half_frame_flag) {
                return errtxtf(ZINT_ERROR_INVALID_DATA, symbol, 985,
                            "Frame number \"%s\" is invalid (expected digits, optionally followed by a single \"A\")",
                            frame_info);
            }
            if (debug_print) printf("%-*s'%c'\t-> 0\n", DX_DEBUG_STR_LEN, "Half frame flag:", half_frame_flag);
            binary_output[bp++] = '0'; /* Half-frame is NOT set */
        }
        binary_output[bp++] = '0'; /* Separator between half frame flag and parity bit*/
    }

    /* Parity bit */
    for (i = 6; i < bp; i++) {
        if (binary_output[i] == '1') {
            parity_bit ^= 1;
        }
    }
    if (debug_print) {
        printf("%-*s%s\t-> %d\n", DX_DEBUG_STR_LEN, "Parity bit:", parity_bit ? "yes" : "no", parity_bit);
    }
    binary_output[bp++] = parity_bit ? '1' : '0';

    memcpy(binary_output + bp, "0101", 4); /* Stop pattern */
    bp += 4;

    *output_length = bp;

    if (raw_text && rt_printf_256(symbol, (*has_frame_info ? "%d-%d%s%s" : "%d-%d"), dx_code_1, dx_code_2, "/",
                                    frame_info)) {
        return ZINT_ERROR_MEMORY; /* `rt_printf_256()` only fails with OOM */
    }

    return 0;
}

INTERNAL int dxfilmedge(struct zint_symbol *symbol, unsigned char source[], int length) {
    int i;
    int writer = 0;
    int error_number;

    char binary_output[32];
    int output_length = 0; /* Suppress gcc warning -Wmaybe-uninitialized */
    int has_frame_info;

    const char long_clock_pattern[] = "1111101010101010101010101010111";
    const char short_clock_pattern[] = "11111010101010101010111";
    const char *clock_pattern;
    const int debug_print = symbol->debug & ZINT_DEBUG_PRINT;

    if (length > 10) {
        return errtxtf(ZINT_ERROR_TOO_LONG, symbol, 986, "Input length %d too long (maximum 10)", length);
    }

    error_number = dx_parse_code(symbol, source, length, binary_output, &output_length, &has_frame_info);
    if (error_number != 0) {
        if (debug_print) printf("Error %s\n\n", symbol->errtxt);
        return error_number;
    }

    /* Clock signal is longer if the frame number is provided */
    if (has_frame_info) {
        clock_pattern = long_clock_pattern;
        assert(output_length == (int) sizeof(long_clock_pattern) - 1);
    } else {
        clock_pattern = short_clock_pattern;
        assert(output_length == (int) sizeof(short_clock_pattern) - 1);
    }

    /* First row: clock pattern */
    for (i = 0; i < output_length; i++) {
        if (clock_pattern[i] == '1') {
            set_module(symbol, 0, writer);
        } else if (clock_pattern[i] == '0') {
            unset_module(symbol, 0, writer);
        }
        writer++;
    }

    /* Reset writer X position for the second row */
    writer = 0;

    /* Second row: data signal */
    for (i = 0; i < output_length; i++) {
        if (binary_output[i] == '1') {
            set_module(symbol, 1, writer);
        } else if (binary_output[i] == '0') {
            unset_module(symbol, 1, writer);
        }
        writer++;
    }
    symbol->rows = 2;
    symbol->width = output_length;

    if (symbol->output_options & COMPLIANT_HEIGHT) {
        /* Measured ratio on 35mm films. Depending on the brands, one symbol height is about 3 * the X-dim.*/
        const float default_height = 6.0f;

        /* AFAIK There is no standard on minimum and maximum height, so we stay close to the measurements */
        const float min_row_height = 2.2f;
        const float max_height = 7.5f;
        error_number = set_height(symbol, min_row_height, default_height, max_height, 0 /*no_errtxt*/);
    } else {
        /* Using compliant height as default as no backwards compatibility to consider */
        const float default_height = 6.0f;
        (void) set_height(symbol, 0.0f, default_height, 0.0f, 1 /*no_errtxt*/);
    }

    return error_number;
}

/* vim: set ts=4 sw=4 et : */
